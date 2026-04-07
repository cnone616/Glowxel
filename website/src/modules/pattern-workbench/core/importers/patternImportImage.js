import { ARTKAL_COLORS_FULL, getColorByCode } from "@/data/artkal-colors.js";
import { createPatternDocument } from "../model/patternDocument.js";
import { PATTERN_DEFAULT_DIMENSION } from "../patternBoard.js";

function loadImage(url) {
  return new Promise((resolve, reject) => {
    const image = new Image();
    image.onload = () => resolve(image);
    image.onerror = () => reject(new Error("图片加载失败"));
    image.src = url;
  });
}

function loadImageFromFile(file) {
  return new Promise((resolve, reject) => {
    if (!file) {
      reject(new Error("缺少图片文件"));
      return;
    }

    const objectUrl = globalThis.URL.createObjectURL(file);
    const image = new Image();
    image.onload = () => {
      globalThis.URL.revokeObjectURL(objectUrl);
      resolve(image);
    };
    image.onerror = () => {
      globalThis.URL.revokeObjectURL(objectUrl);
      reject(new Error("图片加载失败"));
    };
    image.src = objectUrl;
  });
}

function createCanvas(width, height) {
  const canvas = globalThis.document.createElement("canvas");
  canvas.width = Math.max(1, Math.round(width));
  canvas.height = Math.max(1, Math.round(height));
  return canvas;
}

function getContext(canvas, options = {}) {
  const context = canvas.getContext("2d", options);
  if (!context) {
    throw new Error("Canvas 初始化失败");
  }
  return context;
}

function normalizeCropArea(image, crop) {
  if (!crop) {
    return {
      x: 0,
      y: 0,
      width: image.width,
      height: image.height,
    };
  }

  const x = Math.max(0, Math.min(image.width - 1, Math.round(Number(crop.x) || 0)));
  const y = Math.max(0, Math.min(image.height - 1, Math.round(Number(crop.y) || 0)));
  const width = Math.max(1, Math.min(image.width - x, Math.round(Number(crop.width) || image.width)));
  const height = Math.max(1, Math.min(image.height - y, Math.round(Number(crop.height) || image.height)));

  return {
    x,
    y,
    width,
    height,
  };
}

function resolveTargetDimension(value) {
  const numericValue = Number(value);
  if (!Number.isFinite(numericValue) || numericValue <= 0) {
    return PATTERN_DEFAULT_DIMENSION;
  }
  return numericValue;
}

function colorDistance(color1, color2) {
  return Math.sqrt(
    (color1.r - color2.r) ** 2 +
      (color1.g - color2.g) ** 2 +
      (color1.b - color2.b) ** 2,
  );
}

function resolvePalette(colorCodes) {
  const codes = Array.isArray(colorCodes) && colorCodes.length
    ? colorCodes
    : ARTKAL_COLORS_FULL.map((item) => item.code);

  return codes
    .map((code) => getColorByCode(code))
    .filter((item) => item)
    .map((item) => ({
      code: item.code,
      r: item.r,
      g: item.g,
      b: item.b,
    }));
}

function findClosestPaletteColor(rgb, palette) {
  let bestColor = palette[0];
  let bestDistance = Number.POSITIVE_INFINITY;

  palette.forEach((color) => {
    const distance = colorDistance(rgb, color);
    if (distance < bestDistance) {
      bestDistance = distance;
      bestColor = color;
    }
  });

  return bestColor;
}

function detectPixelArt(imageData, width, height) {
  const data = imageData.data;
  const colorSet = new Set();
  let totalPixels = 0;
  const sampleRate = Math.max(1, Math.floor((width * height) / 10000));

  for (let index = 0; index < data.length; index += 4 * sampleRate) {
    const alpha = data[index + 3];
    if (alpha < 128) {
      continue;
    }

    colorSet.add(`${data[index]},${data[index + 1]},${data[index + 2]}`);
    totalPixels += 1;
  }

  const uniqueColors = colorSet.size;
  const colorRatio = uniqueColors / Math.max(1, totalPixels);

  return colorRatio < 0.3 && uniqueColors < 256;
}

function detectContentBounds(imageData, width, height) {
  const data = imageData.data;
  let minX = width;
  let minY = height;
  let maxX = -1;
  let maxY = -1;

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      if (data[(y * width + x) * 4 + 3] < 128) {
        continue;
      }

      minX = Math.min(minX, x);
      minY = Math.min(minY, y);
      maxX = Math.max(maxX, x);
      maxY = Math.max(maxY, y);
    }
  }

  if (maxX < minX || maxY < minY) {
    return null;
  }

  return {
    x: minX,
    y: minY,
    width: maxX - minX + 1,
    height: maxY - minY + 1,
  };
}

function buildPixelMapFromImageData(imageData, width, height, palette) {
  const data = imageData.data;
  const pixels = new Map();

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const index = (y * width + x) * 4;
      const alpha = data[index + 3];
      if (alpha < 128) {
        continue;
      }

      const closestColor = findClosestPaletteColor(
        {
          r: data[index],
          g: data[index + 1],
          b: data[index + 2],
        },
        palette,
      );

      if (closestColor) {
        pixels.set(`${x},${y}`, closestColor.code);
      }
    }
  }

  return pixels;
}

function cropImageToCanvas(image, cropArea) {
  const canvas = createCanvas(cropArea.width, cropArea.height);
  const context = getContext(canvas, { willReadFrequently: true });
  context.drawImage(
    image,
    cropArea.x,
    cropArea.y,
    cropArea.width,
    cropArea.height,
    0,
    0,
    cropArea.width,
    cropArea.height,
  );
  return canvas;
}

function renderPixelArt(context, sourceCanvas, targetWidth, targetHeight) {
  context.imageSmoothingEnabled = false;

  const scaleX = targetWidth / sourceCanvas.width;
  const scaleY = targetHeight / sourceCanvas.height;
  const scale = Math.max(scaleX, scaleY);
  const integerScale = scale >= 1 ? Math.ceil(scale) : 1 / Math.ceil(1 / scale);
  const intermediateWidth = Math.max(1, Math.round(sourceCanvas.width * integerScale));
  const intermediateHeight = Math.max(1, Math.round(sourceCanvas.height * integerScale));
  const intermediateCanvas = createCanvas(intermediateWidth, intermediateHeight);
  const intermediateContext = getContext(intermediateCanvas, { willReadFrequently: true });

  intermediateContext.imageSmoothingEnabled = false;
  intermediateContext.drawImage(
    sourceCanvas,
    0,
    0,
    intermediateWidth,
    intermediateHeight,
  );

  const offsetX = Math.floor((intermediateWidth - targetWidth) / 2);
  const offsetY = Math.floor((intermediateHeight - targetHeight) / 2);

  context.drawImage(
    intermediateCanvas,
    Math.max(0, offsetX),
    Math.max(0, offsetY),
    Math.min(intermediateWidth, targetWidth),
    Math.min(intermediateHeight, targetHeight),
    Math.max(0, -offsetX),
    Math.max(0, -offsetY),
    Math.min(intermediateWidth, targetWidth),
    Math.min(intermediateHeight, targetHeight),
  );
}

function renderNormalImage(context, sourceCanvas, targetWidth, targetHeight) {
  context.imageSmoothingEnabled = true;
  context.imageSmoothingQuality = "high";

  const sourceContext = getContext(sourceCanvas, { willReadFrequently: true });
  const sourceImageData = sourceContext.getImageData(0, 0, sourceCanvas.width, sourceCanvas.height);
  const contentBounds = detectContentBounds(sourceImageData, sourceCanvas.width, sourceCanvas.height);

  if (contentBounds) {
    context.drawImage(
      sourceCanvas,
      contentBounds.x,
      contentBounds.y,
      contentBounds.width,
      contentBounds.height,
      0,
      0,
      targetWidth,
      targetHeight,
    );
    return;
  }

  context.drawImage(sourceCanvas, 0, 0, targetWidth, targetHeight);
}

function generatePixelsFromCanvas(sourceCanvas, width, height, palette) {
  const sourceContext = getContext(sourceCanvas, { willReadFrequently: true });
  const detectData = sourceContext.getImageData(0, 0, sourceCanvas.width, sourceCanvas.height);
  const isPixelArt = detectPixelArt(detectData, sourceCanvas.width, sourceCanvas.height);
  const targetCanvas = createCanvas(width, height);
  const targetContext = getContext(targetCanvas, { willReadFrequently: true });

  if (isPixelArt) {
    renderPixelArt(targetContext, sourceCanvas, width, height);
  } else {
    renderNormalImage(targetContext, sourceCanvas, width, height);
  }

  const imageData = targetContext.getImageData(0, 0, width, height);

  return {
    pixels: buildPixelMapFromImageData(imageData, width, height, palette),
    isPixelArt,
  };
}

export async function importPatternFromImage(file, options = {}) {
  if (!file) {
    throw new Error("缺少图片文件");
  }

  const image = await loadImageFromFile(file);
  const cropArea = normalizeCropArea(image, options.crop);
  const croppedCanvas = cropImageToCanvas(image, cropArea);
  const palette = resolvePalette(options.colorCodes);
  const width = resolveTargetDimension(options.width);
  const height = resolveTargetDimension(options.height);
  const result = generatePixelsFromCanvas(croppedCanvas, width, height, palette);

  const document = createPatternDocument({
    id: options.id || `image-${Date.now()}`,
    name: options.name || file.name || "图片导入",
    width,
    height,
    pixels: result.pixels,
    sourceType: "image",
  });

  return {
    document,
    report: {
      mode: "image",
      sourceWidth: image.width,
      sourceHeight: image.height,
      cropX: cropArea.x,
      cropY: cropArea.y,
      cropWidth: cropArea.width,
      cropHeight: cropArea.height,
      width,
      height,
      isPixelArt: result.isPixelArt,
    },
  };
}

export async function importPatternFromCaptureImage(file, options = {}) {
  if (!file) {
    throw new Error("缺少图片文件");
  }

  const image = await loadImageFromFile(file);
  const cropArea = normalizeCropArea(image, options.crop);
  const croppedCanvas = cropImageToCanvas(image, cropArea);
  const palette = resolvePalette();
  const width = resolveTargetDimension(options.width);
  const height = resolveTargetDimension(options.height);
  const result = generatePixelsFromCanvas(croppedCanvas, width, height, palette);

  return createPatternDocument({
    id: options.id || `capture-${Date.now()}`,
    name: options.name || file.name || "实拍图校验",
    width,
    height,
    pixels: result.pixels,
    sourceType: "capture-review",
  });
}
