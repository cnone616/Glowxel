import { ARTKAL_COLORS_FULL, findClosestColor } from "@/data/artkal-colors.js";
import { createPatternDocument } from "../model/patternDocument.js";
import { PATTERN_BOARD_SIZE } from "../patternBoard.js";

const DEFAULT_GRID_SIZE = PATTERN_BOARD_SIZE;
const MIN_GRID_SIZE = 8;
const MAX_GRID_SIZE = 160;
const OCR_WHITELIST = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const SIGNATURE_SIZE = 16;
const SIGNATURE_MATCH_THRESHOLD = 26;
const LOCAL_TESSERACT_LANG_PATH = "/tesseract";
const ARTKAL_CODE_LIST = ARTKAL_COLORS_FULL.map((item) => item.code);
const ARTKAL_CODE_SET = new Set(ARTKAL_CODE_LIST);
const ARTKAL_CODE_PREFIX_SET = new Set(ARTKAL_CODE_LIST.map((item) => item[0]));
const OCR_RETRY_CONFIDENCE = 62;
const MAX_CLUSTER_PREVIEW_COUNT = 4;
const MAX_RETRY_CELL_COUNT = 3;
const MAX_CELL_OCR_VARIANT_COUNT = 4;
const OCR_PRIMARY_PAGE_SEG_MODE = "10";
const OCR_RETRY_PAGE_SEG_MODES = ["8", "7"];

function hexToRgb(hex) {
  const normalizedHex = String(hex || "").replace("#", "");
  if (normalizedHex.length !== 6) {
    return {
      r: 255,
      g: 255,
      b: 255,
    };
  }

  return {
    r: Number.parseInt(normalizedHex.slice(0, 2), 16),
    g: Number.parseInt(normalizedHex.slice(2, 4), 16),
    b: Number.parseInt(normalizedHex.slice(4, 6), 16),
  };
}

const ARTKAL_COLOR_MAP = new Map(
  ARTKAL_COLORS_FULL.map((item) => [
    item.code,
    {
      code: item.code,
      rgb: hexToRgb(item.hex),
    },
  ]),
);

function clampNumber(value, min, max, fallback) {
  const numericValue = Number(value);
  if (!Number.isFinite(numericValue)) {
    return fallback;
  }
  if (numericValue < min) {
    return min;
  }
  if (numericValue > max) {
    return max;
  }
  return numericValue;
}

function readFileAsDataUrl(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(reader.result);
    reader.onerror = () => reject(new Error("读取图纸图片失败"));
    reader.readAsDataURL(file);
  });
}

function loadImage(url) {
  return new Promise((resolve, reject) => {
    const image = new Image();
    image.onload = () => resolve(image);
    image.onerror = () => reject(new Error("图纸图片加载失败"));
    image.src = url;
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
    throw new Error("无法创建图纸识别画布");
  }
  return context;
}

function getPixelOffset(width, x, y) {
  return (y * width + x) * 4;
}

function luminance(r, g, b) {
  return r * 0.299 + g * 0.587 + b * 0.114;
}

function colorDistance(left, right) {
  return Math.sqrt(
    (left.r - right.r) ** 2 +
      (left.g - right.g) ** 2 +
      (left.b - right.b) ** 2,
  );
}

function sampleBorderColor(imageData) {
  const width = imageData.width;
  const height = imageData.height;
  const data = imageData.data;
  let red = 0;
  let green = 0;
  let blue = 0;
  let alpha = 0;
  let count = 0;

  function sample(x, y) {
    const offset = getPixelOffset(width, x, y);
    red += data[offset];
    green += data[offset + 1];
    blue += data[offset + 2];
    alpha += data[offset + 3];
    count += 1;
  }

  for (let x = 0; x < width; x += 1) {
    sample(x, 0);
    sample(x, height - 1);
  }

  for (let y = 1; y < height - 1; y += 1) {
    sample(0, y);
    sample(width - 1, y);
  }

  if (!count) {
    return { r: 255, g: 255, b: 255, a: 255 };
  }

  return {
    r: red / count,
    g: green / count,
    b: blue / count,
    a: alpha / count,
  };
}

function normalizeCropArea(sourceWidth, sourceHeight, cropArea) {
  if (!cropArea) {
    return {
      x: 0,
      y: 0,
      width: sourceWidth,
      height: sourceHeight,
    };
  }

  const x = clampNumber(cropArea.x, 0, sourceWidth - 1, 0);
  const y = clampNumber(cropArea.y, 0, sourceHeight - 1, 0);
  const maxWidth = Math.max(1, sourceWidth - x);
  const maxHeight = Math.max(1, sourceHeight - y);

  return {
    x: Math.round(x),
    y: Math.round(y),
    width: Math.round(clampNumber(cropArea.width, 1, maxWidth, maxWidth)),
    height: Math.round(clampNumber(cropArea.height, 1, maxHeight, maxHeight)),
  };
}

function cropCanvas(sourceCanvas, cropArea) {
  const nextCanvas = createCanvas(cropArea.width, cropArea.height);
  const context = getContext(nextCanvas);
  context.drawImage(
    sourceCanvas,
    cropArea.x,
    cropArea.y,
    cropArea.width,
    cropArea.height,
    0,
    0,
    cropArea.width,
    cropArea.height,
  );
  return nextCanvas;
}

function removeBackgroundWithinCrop(imageData) {
  const data = imageData.data;
  const borderColor = sampleBorderColor(imageData);
  const borderLuminance = luminance(borderColor.r, borderColor.g, borderColor.b);
  const threshold = borderLuminance > 220 ? 26 : 20;

  for (let index = 0; index < data.length; index += 4) {
    const pixel = {
      r: data[index],
      g: data[index + 1],
      b: data[index + 2],
    };
    const diff = colorDistance(pixel, borderColor);
    if (diff > threshold) {
      continue;
    }

    data[index] = 255;
    data[index + 1] = 255;
    data[index + 2] = 255;
  }

  return imageData;
}

function buildAxisScores(imageData, axis) {
  const width = imageData.width;
  const height = imageData.height;
  const data = imageData.data;
  const scoreLength = axis === "x" ? width : height;
  const limit = axis === "x" ? height : width;
  const scores = new Array(scoreLength).fill(0);

  for (let primary = 0; primary < scoreLength; primary += 1) {
    let score = 0;

    for (let secondary = 0; secondary < limit; secondary += 1) {
      const x = axis === "x" ? primary : secondary;
      const y = axis === "x" ? secondary : primary;
      const offset = getPixelOffset(width, x, y);
      const alpha = data[offset + 3];

      if (alpha < 32) {
        continue;
      }

      const lum = luminance(data[offset], data[offset + 1], data[offset + 2]);
      score += Math.max(0, 220 - lum);
    }

    scores[primary] = score / Math.max(1, limit);
  }

  return smoothScores(scores, 2);
}

function smoothScores(scores, radius) {
  return scores.map((_, index) => {
    let sum = 0;
    let count = 0;
    const start = Math.max(0, index - radius);
    const end = Math.min(scores.length - 1, index + radius);

    for (let cursor = start; cursor <= end; cursor += 1) {
      sum += scores[cursor];
      count += 1;
    }

    return count ? sum / count : 0;
  });
}

function analyzeSpacing(scores) {
  if (!scores.length) {
    return {
      spacing: 0,
      score: 0,
      variance: 0,
    };
  }

  const average = scores.reduce((sum, value) => sum + value, 0) / scores.length;
  const normalized = scores.map((value) => value - average);
  const variance =
    normalized.reduce((sum, value) => sum + value * value, 0) /
    Math.max(normalized.length, 1);
  const minLag = Math.max(4, Math.floor(scores.length / 160));
  const maxLag = Math.min(48, Math.floor(scores.length / 4));
  let bestLag = 0;
  let bestScore = Number.NEGATIVE_INFINITY;

  for (let lag = minLag; lag <= maxLag; lag += 1) {
    let sum = 0;
    let count = 0;

    for (let index = 0; index + lag < normalized.length; index += 1) {
      sum += normalized[index] * normalized[index + lag];
      count += 1;
    }

    if (!count) {
      continue;
    }

    const score = sum / count;
    if (score > bestScore) {
      bestScore = score;
      bestLag = lag;
    }
  }

  return {
    spacing: bestLag,
    score: variance ? bestScore / variance : 0,
    variance,
  };
}

function estimateSpacing(scores) {
  return analyzeSpacing(scores).spacing;
}

function estimateGridOffset(scores, spacing) {
  if (!spacing) {
    return 0;
  }

  let bestOffset = 0;
  let bestScore = Number.NEGATIVE_INFINITY;

  for (let offset = 0; offset < spacing; offset += 1) {
    let score = 0;
    let count = 0;

    for (let cursor = offset; cursor < scores.length; cursor += spacing) {
      const rounded = Math.min(scores.length - 1, Math.round(cursor));
      score += scores[rounded];
      count += 1;
    }

    if (!count) {
      continue;
    }

    const average = score / count;
    if (average > bestScore) {
      bestScore = average;
      bestOffset = offset;
    }
  }

  return bestOffset;
}

function buildGridModel(imageData, options = {}) {
  if (options.cellSample) {
    const sampleWidth = clampNumber(
      options.cellSample.width,
      2,
      imageData.width,
      imageData.width / DEFAULT_GRID_SIZE,
    );
    const sampleHeight = clampNumber(
      options.cellSample.height,
      2,
      imageData.height,
      imageData.height / DEFAULT_GRID_SIZE,
    );
    const relativeX = clampNumber(options.cellSample.x, 0, imageData.width - 1, 0);
    const relativeY = clampNumber(options.cellSample.y, 0, imageData.height - 1, 0);
    const offsetX = relativeX - Math.floor(relativeX / sampleWidth) * sampleWidth;
    const offsetY = relativeY - Math.floor(relativeY / sampleHeight) * sampleHeight;
    const detectedWidth = Math.round((imageData.width - offsetX) / Math.max(sampleWidth, 1));
    const detectedHeight = Math.round((imageData.height - offsetY) / Math.max(sampleHeight, 1));
    const width = clampNumber(
      options.width || detectedWidth,
      MIN_GRID_SIZE,
      MAX_GRID_SIZE,
      DEFAULT_GRID_SIZE,
    );
    const height = clampNumber(
      options.height || detectedHeight,
      MIN_GRID_SIZE,
      MAX_GRID_SIZE,
      DEFAULT_GRID_SIZE,
    );

    return {
      width,
      height,
      cellWidth: sampleWidth,
      cellHeight: sampleHeight,
      offsetX,
      offsetY,
      detectedWidth,
      detectedHeight,
    };
  }

  const verticalScores = buildAxisScores(imageData, "x");
  const horizontalScores = buildAxisScores(imageData, "y");
  const verticalSpacing = analyzeSpacing(verticalScores);
  const horizontalSpacing = analyzeSpacing(horizontalScores);
  const detectedCellWidth = verticalSpacing.spacing;
  const detectedCellHeight = horizontalSpacing.spacing;
  const fallbackCellWidth = imageData.width / DEFAULT_GRID_SIZE;
  const fallbackCellHeight = imageData.height / DEFAULT_GRID_SIZE;
  const cellWidth = detectedCellWidth || fallbackCellWidth;
  const cellHeight = detectedCellHeight || fallbackCellHeight;
  const detectedWidth = Math.round(imageData.width / Math.max(cellWidth, 1));
  const detectedHeight = Math.round(imageData.height / Math.max(cellHeight, 1));
  const width = clampNumber(
    options.width || detectedWidth,
    MIN_GRID_SIZE,
    MAX_GRID_SIZE,
    DEFAULT_GRID_SIZE,
  );
  const height = clampNumber(
    options.height || detectedHeight,
    MIN_GRID_SIZE,
    MAX_GRID_SIZE,
    DEFAULT_GRID_SIZE,
  );
  const finalCellWidth = imageData.width / width;
  const finalCellHeight = imageData.height / height;
  const offsetX = estimateGridOffset(verticalScores, Math.max(1, Math.round(finalCellWidth)));
  const offsetY = estimateGridOffset(horizontalScores, Math.max(1, Math.round(finalCellHeight)));

  return {
    width,
    height,
    cellWidth: finalCellWidth,
    cellHeight: finalCellHeight,
    offsetX,
    offsetY,
    detectedWidth,
    detectedHeight,
  };
}

function resizeForDetection(image) {
  const maxDimension = 720;
  const scale = Math.min(maxDimension / image.width, maxDimension / image.height, 1);
  const canvas = createCanvas(image.width * scale, image.height * scale);
  const context = getContext(canvas, { willReadFrequently: true });
  context.drawImage(image, 0, 0, canvas.width, canvas.height);
  return canvas;
}

export async function detectNumberedSheetLike(file) {
  if (!file) {
    return {
      isNumberedSheet: false,
      confidence: 0,
      reason: "missing-file",
    };
  }

  const dataUrl = await readFileAsDataUrl(file);
  const image = await loadImage(dataUrl);
  const canvas = resizeForDetection(image);
  const context = getContext(canvas, { willReadFrequently: true });
  const imageData = context.getImageData(0, 0, canvas.width, canvas.height);
  const verticalScores = buildAxisScores(imageData, "x");
  const horizontalScores = buildAxisScores(imageData, "y");
  const verticalSpacing = analyzeSpacing(verticalScores);
  const horizontalSpacing = analyzeSpacing(horizontalScores);
  const estimatedColumns = verticalSpacing.spacing
    ? Math.round(canvas.width / verticalSpacing.spacing)
    : 0;
  const estimatedRows = horizontalSpacing.spacing
    ? Math.round(canvas.height / horizontalSpacing.spacing)
    : 0;
  const spacingConsistency =
    verticalSpacing.spacing && horizontalSpacing.spacing
      ? 1 -
        Math.min(
          Math.abs(verticalSpacing.spacing - horizontalSpacing.spacing) /
            Math.max(verticalSpacing.spacing, horizontalSpacing.spacing),
          1,
        )
      : 0;
  const gridCountConfidence =
    estimatedColumns >= 12 &&
    estimatedColumns <= 160 &&
    estimatedRows >= 12 &&
    estimatedRows <= 160
      ? 1
      : 0;
  const periodicityConfidence = Math.max(
    0,
    Math.min((verticalSpacing.score + horizontalSpacing.score) / 0.42 / 2, 1),
  );
  const densityConfidence = Math.max(
    0,
    Math.min((Math.sqrt(verticalSpacing.variance) + Math.sqrt(horizontalSpacing.variance)) / 44 / 2, 1),
  );
  const confidence =
    periodicityConfidence * 0.45 +
    spacingConsistency * 0.3 +
    gridCountConfidence * 0.15 +
    densityConfidence * 0.1;
  const isNumberedSheet =
    confidence >= 0.55 &&
    periodicityConfidence >= 0.35 &&
    spacingConsistency >= 0.45 &&
    gridCountConfidence > 0;

  return {
    isNumberedSheet,
    confidence,
    estimatedColumns,
    estimatedRows,
    reason: isNumberedSheet ? "grid-like-pattern" : "normal-image",
  };
}

function getCellBounds(gridModel, x, y, paddingRatio = 0.16) {
  const baseLeft = gridModel.offsetX + x * gridModel.cellWidth;
  const baseTop = gridModel.offsetY + y * gridModel.cellHeight;
  const baseRight = gridModel.offsetX + (x + 1) * gridModel.cellWidth;
  const baseBottom = gridModel.offsetY + (y + 1) * gridModel.cellHeight;
  const paddingX = gridModel.cellWidth * paddingRatio;
  const paddingY = gridModel.cellHeight * paddingRatio;

  return {
    left: Math.round(baseLeft + paddingX),
    top: Math.round(baseTop + paddingY),
    right: Math.round(baseRight - paddingX),
    bottom: Math.round(baseBottom - paddingY),
  };
}

function normalizeBounds(bounds, imageData) {
  const left = clampNumber(Math.floor(bounds.left), 0, imageData.width - 1, 0);
  const top = clampNumber(Math.floor(bounds.top), 0, imageData.height - 1, 0);
  const right = clampNumber(Math.ceil(bounds.right), left, imageData.width - 1, left);
  const bottom = clampNumber(Math.ceil(bounds.bottom), top, imageData.height - 1, top);

  return {
    left,
    top,
    right,
    bottom,
  };
}

function buildCellSamples(imageData, bounds) {
  const samples = [];
  const width = imageData.width;
  const data = imageData.data;
  const normalizedBounds = normalizeBounds(bounds, imageData);
  const left = normalizedBounds.left;
  const top = normalizedBounds.top;
  const right = normalizedBounds.right;
  const bottom = normalizedBounds.bottom;

  for (let y = top; y <= bottom; y += 1) {
    for (let x = left; x <= right; x += 1) {
      const offset = getPixelOffset(width, x, y);
      const alpha = data[offset + 3];
      if (alpha < 64) {
        continue;
      }

      const r = data[offset];
      const g = data[offset + 1];
      const b = data[offset + 2];
      samples.push({
        r,
        g,
        b,
        lum: luminance(r, g, b),
      });
    }
  }

  return samples;
}

function averageSamples(samples) {
  if (!samples.length) {
    return {
      r: 255,
      g: 255,
      b: 255,
    };
  }

  let red = 0;
  let green = 0;
  let blue = 0;

  samples.forEach((sample) => {
    red += sample.r;
    green += sample.g;
    blue += sample.b;
  });

  return {
    r: Math.round(red / samples.length),
    g: Math.round(green / samples.length),
    b: Math.round(blue / samples.length),
  };
}

function buildDominantColor(samples, options = {}) {
  const step = options.step || 12;
  const minLuminance = options.minLuminance ?? Number.NEGATIVE_INFINITY;
  const maxLuminance = options.maxLuminance ?? Number.POSITIVE_INFINITY;
  const bucketMap = new Map();

  samples.forEach((sample) => {
    if (sample.lum < minLuminance || sample.lum > maxLuminance) {
      return;
    }

    const key = [
      Math.round(sample.r / step),
      Math.round(sample.g / step),
      Math.round(sample.b / step),
    ].join("-");

    if (!bucketMap.has(key)) {
      bucketMap.set(key, {
        weight: 0,
        red: 0,
        green: 0,
        blue: 0,
        count: 0,
      });
    }

    const bucket = bucketMap.get(key);
    bucket.weight += 1;
    bucket.red += sample.r;
    bucket.green += sample.g;
    bucket.blue += sample.b;
    bucket.count += 1;
  });

  if (!bucketMap.size) {
    return averageSamples(samples);
  }

  let bestBucket = null;
  bucketMap.forEach((bucket) => {
    if (!bestBucket || bucket.weight > bestBucket.weight) {
      bestBucket = bucket;
    }
  });

  return {
    r: Math.round(bestBucket.red / Math.max(1, bestBucket.count)),
    g: Math.round(bestBucket.green / Math.max(1, bestBucket.count)),
    b: Math.round(bestBucket.blue / Math.max(1, bestBucket.count)),
  };
}

function resolveCellBaseColor(samples) {
  if (!samples.length) {
    return {
      r: 255,
      g: 255,
      b: 255,
    };
  }

  const nonDarkSamples = samples.filter((sample) => sample.lum >= 42);
  if (nonDarkSamples.length >= Math.max(6, Math.floor(samples.length * 0.35))) {
    return buildDominantColor(nonDarkSamples, {
      step: 12,
    });
  }

  return buildDominantColor(samples, {
    step: 12,
  });
}

function extractCellFillColor(imageData, gridModel, x, y) {
  const bounds = getCellBounds(gridModel, x, y, 0.16);
  const samples = buildCellSamples(imageData, bounds);

  if (!samples.length) {
    return {
      r: 255,
      g: 255,
      b: 255,
    };
  }

  return resolveCellBaseColor(samples);
}

function buildGlyphBinary(imageData, gridModel, x, y, fillColor) {
  const bounds = normalizeBounds(getCellBounds(gridModel, x, y, 0.2), imageData);
  const binary = new Array(SIGNATURE_SIZE * SIGNATURE_SIZE).fill(0);
  const width = imageData.width;
  const data = imageData.data;
  const fillLum = luminance(fillColor.r, fillColor.g, fillColor.b);
  let darkPixels = 0;

  for (let gridY = 0; gridY < SIGNATURE_SIZE; gridY += 1) {
    for (let gridX = 0; gridX < SIGNATURE_SIZE; gridX += 1) {
      const startX = Math.floor(
        bounds.left + ((bounds.right - bounds.left + 1) * gridX) / SIGNATURE_SIZE,
      );
      const endX = Math.floor(
        bounds.left + ((bounds.right - bounds.left + 1) * (gridX + 1)) / SIGNATURE_SIZE,
      );
      const startY = Math.floor(
        bounds.top + ((bounds.bottom - bounds.top + 1) * gridY) / SIGNATURE_SIZE,
      );
      const endY = Math.floor(
        bounds.top + ((bounds.bottom - bounds.top + 1) * (gridY + 1)) / SIGNATURE_SIZE,
      );
      let markedCount = 0;
      let totalCount = 0;

      for (let sampleY = startY; sampleY <= endY; sampleY += 1) {
        for (let sampleX = startX; sampleX <= endX; sampleX += 1) {
          const clampedX = Math.min(width - 1, Math.max(0, sampleX));
          const clampedY = Math.min(imageData.height - 1, Math.max(0, sampleY));
          const offset = getPixelOffset(width, clampedX, clampedY);
          const alpha = data[offset + 3];
          if (alpha < 64) {
            continue;
          }

          const sampleColor = {
            r: data[offset],
            g: data[offset + 1],
            b: data[offset + 2],
          };
          const sampleLum = luminance(sampleColor.r, sampleColor.g, sampleColor.b);
          const diff = colorDistance(sampleColor, fillColor);
          const isDarkText =
            sampleLum < fillLum - 16 ||
            (diff > 26 && sampleLum < fillLum - 6) ||
            (diff > 42 && sampleLum < 210);
          if (isDarkText) {
            markedCount += 1;
          }
          totalCount += 1;
        }
      }

      const index = gridY * SIGNATURE_SIZE + gridX;
      if (totalCount && markedCount / totalCount >= 0.24) {
        binary[index] = 1;
        darkPixels += 1;
      }
    }
  }

  return {
    binary,
    darkPixels,
  };
}

function getBinaryContentBounds(binary) {
  let left = SIGNATURE_SIZE;
  let top = SIGNATURE_SIZE;
  let right = -1;
  let bottom = -1;

  for (let y = 0; y < SIGNATURE_SIZE; y += 1) {
    for (let x = 0; x < SIGNATURE_SIZE; x += 1) {
      if (!binary[y * SIGNATURE_SIZE + x]) {
        continue;
      }

      left = Math.min(left, x);
      top = Math.min(top, y);
      right = Math.max(right, x);
      bottom = Math.max(bottom, y);
    }
  }

  if (right < left || bottom < top) {
    return {
      left: 0,
      top: 0,
      right: SIGNATURE_SIZE - 1,
      bottom: SIGNATURE_SIZE - 1,
    };
  }

  return { left, top, right, bottom };
}

function getMaskContentBounds(mask, width, height) {
  let left = width;
  let top = height;
  let right = -1;
  let bottom = -1;

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      if (!mask[y * width + x]) {
        continue;
      }

      left = Math.min(left, x);
      top = Math.min(top, y);
      right = Math.max(right, x);
      bottom = Math.max(bottom, y);
    }
  }

  if (right < left || bottom < top) {
    return null;
  }

  return { left, top, right, bottom };
}

function renderMaskToCanvas(mask, width, height, options = {}) {
  const bounds = getMaskContentBounds(mask, width, height);
  const targetSize = options.targetSize || 96;
  const padding = options.padding || 10;
  const bold = !!options.bold;
  const invert = !!options.invert;
  const canvas = createCanvas(targetSize, targetSize);
  const context = getContext(canvas, { willReadFrequently: false });

  context.fillStyle = invert ? "#111111" : "#ffffff";
  context.fillRect(0, 0, canvas.width, canvas.height);
  context.fillStyle = invert ? "#ffffff" : "#111111";

  if (!bounds) {
    return canvas;
  }

  const contentWidth = Math.max(1, bounds.right - bounds.left + 1);
  const contentHeight = Math.max(1, bounds.bottom - bounds.top + 1);
  const scale = Math.max(
    2,
    Math.floor(
      Math.min(
        (targetSize - padding * 2) / contentWidth,
        (targetSize - padding * 2) / contentHeight,
      ),
    ),
  );
  const drawWidth = contentWidth * scale;
  const drawHeight = contentHeight * scale;
  const offsetX = Math.floor((targetSize - drawWidth) / 2);
  const offsetY = Math.floor((targetSize - drawHeight) / 2);

  for (let y = bounds.top; y <= bounds.bottom; y += 1) {
    for (let x = bounds.left; x <= bounds.right; x += 1) {
      if (!mask[y * width + x]) {
        continue;
      }

      const drawLeft = offsetX + (x - bounds.left) * scale;
      const drawTop = offsetY + (y - bounds.top) * scale;
      if (bold) {
        context.fillRect(drawLeft - 1, drawTop, scale + 2, scale);
        context.fillRect(drawLeft, drawTop - 1, scale, scale + 2);
      } else {
        context.fillRect(drawLeft, drawTop, scale, scale);
      }
    }
  }

  return canvas;
}

function createGlyphPreviewCanvas(binary) {
  const bounds = getBinaryContentBounds(binary);
  const contentWidth = Math.max(1, bounds.right - bounds.left + 1);
  const contentHeight = Math.max(1, bounds.bottom - bounds.top + 1);
  const targetSize = 96;
  const padding = 14;
  const scale = Math.max(
    2,
    Math.floor(
      Math.min(
        (targetSize - padding * 2) / contentWidth,
        (targetSize - padding * 2) / contentHeight,
      ),
    ),
  );
  const canvas = createCanvas(targetSize, targetSize);
  const context = getContext(canvas, { willReadFrequently: false });
  const drawWidth = contentWidth * scale;
  const drawHeight = contentHeight * scale;
  const offsetX = Math.floor((targetSize - drawWidth) / 2);
  const offsetY = Math.floor((targetSize - drawHeight) / 2);

  context.fillStyle = "#ffffff";
  context.fillRect(0, 0, canvas.width, canvas.height);
  context.fillStyle = "#111111";

  for (let y = bounds.top; y <= bounds.bottom; y += 1) {
    for (let x = bounds.left; x <= bounds.right; x += 1) {
      if (!binary[y * SIGNATURE_SIZE + x]) {
        continue;
      }
      context.fillRect(
        offsetX + (x - bounds.left) * scale,
        offsetY + (y - bounds.top) * scale,
        scale,
        scale,
      );
    }
  }

  return canvas;
}

function createBoldGlyphPreviewCanvas(binary) {
  const bounds = getBinaryContentBounds(binary);
  const contentWidth = Math.max(1, bounds.right - bounds.left + 1);
  const contentHeight = Math.max(1, bounds.bottom - bounds.top + 1);
  const targetSize = 96;
  const padding = 12;
  const scale = Math.max(
    2,
    Math.floor(
      Math.min(
        (targetSize - padding * 2) / contentWidth,
        (targetSize - padding * 2) / contentHeight,
      ),
    ),
  );
  const canvas = createCanvas(targetSize, targetSize);
  const context = getContext(canvas, { willReadFrequently: false });
  const drawWidth = contentWidth * scale;
  const drawHeight = contentHeight * scale;
  const offsetX = Math.floor((targetSize - drawWidth) / 2);
  const offsetY = Math.floor((targetSize - drawHeight) / 2);

  context.fillStyle = "#ffffff";
  context.fillRect(0, 0, canvas.width, canvas.height);
  context.fillStyle = "#111111";

  for (let y = bounds.top; y <= bounds.bottom; y += 1) {
    for (let x = bounds.left; x <= bounds.right; x += 1) {
      if (!binary[y * SIGNATURE_SIZE + x]) {
        continue;
      }

      const drawLeft = offsetX + (x - bounds.left) * scale;
      const drawTop = offsetY + (y - bounds.top) * scale;
      context.fillRect(drawLeft - 1, drawTop, scale + 2, scale);
      context.fillRect(drawLeft, drawTop - 1, scale, scale + 2);
    }
  }

  return canvas;
}

function buildPreviewCanvasSet(binary) {
  return [
    createGlyphPreviewCanvas(binary),
    createBoldGlyphPreviewCanvas(binary),
  ];
}

function buildCellOcrMask(imageData, bounds, fillColor) {
  const normalizedBounds = normalizeBounds(bounds, imageData);
  const width = normalizedBounds.right - normalizedBounds.left + 1;
  const height = normalizedBounds.bottom - normalizedBounds.top + 1;
  const mask = new Array(width * height).fill(0);
  const data = imageData.data;
  const sourceWidth = imageData.width;
  const fillLum = luminance(fillColor.r, fillColor.g, fillColor.b);

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const sourceX = normalizedBounds.left + x;
      const sourceY = normalizedBounds.top + y;
      const offset = getPixelOffset(sourceWidth, sourceX, sourceY);
      const alpha = data[offset + 3];
      if (alpha < 64) {
        continue;
      }

      const sampleColor = {
        r: data[offset],
        g: data[offset + 1],
        b: data[offset + 2],
      };
      const sampleLum = luminance(sampleColor.r, sampleColor.g, sampleColor.b);
      const diff = colorDistance(sampleColor, fillColor);
      const isTextPixel =
        sampleLum < fillLum - 10 ||
        (diff > 18 && sampleLum < fillLum + 12) ||
        (diff > 30 && sampleLum < 220);

      if (isTextPixel) {
        mask[y * width + x] = 1;
      }
    }
  }

  return {
    mask,
    width,
    height,
  };
}

function buildCellOcrCanvasSet(imageData, bounds, fillColor) {
  const { mask, width, height } = buildCellOcrMask(imageData, bounds, fillColor);
  const contentBounds = getMaskContentBounds(mask, width, height);
  if (!contentBounds) {
    return [];
  }

  return [
    renderMaskToCanvas(mask, width, height, { targetSize: 96, padding: 10 }),
    renderMaskToCanvas(mask, width, height, { targetSize: 96, padding: 8, bold: true }),
    renderMaskToCanvas(mask, width, height, { targetSize: 112, padding: 8 }),
    renderMaskToCanvas(mask, width, height, { targetSize: 112, padding: 8, invert: true }),
  ];
}

function hammingDistance(left, right) {
  let distance = 0;
  for (let index = 0; index < left.length; index += 1) {
    if (left[index] !== right[index]) {
      distance += 1;
    }
  }
  return distance;
}

function normalizeOcrToken(text) {
  return text.toUpperCase().replace(/[^0-9A-Z]/g, "").slice(0, 3);
}

function buildTokenVariants(token) {
  if (!token) {
    return [];
  }

  const firstCharMap = {
    "0": ["D", "Q", "O"],
    "1": ["I", "T"],
    "2": ["Z"],
    "5": ["S"],
    "6": ["G"],
    "8": ["B"],
  };
  const digitCharMap = {
    O: ["0"],
    Q: ["0"],
    D: ["0"],
    I: ["1"],
    L: ["1"],
    T: ["1"],
    Z: ["2"],
    S: ["5"],
    B: ["8"],
    G: ["6"],
    Y: ["7"],
  };

  let variants = [""];

  for (let index = 0; index < token.length; index += 1) {
    const rawChar = token[index];
    const replaceList =
      index === 0
        ? [rawChar, ...(firstCharMap[rawChar] || [])]
        : [rawChar, ...(digitCharMap[rawChar] || [])];
    const nextVariants = [];

    variants.forEach((prefix) => {
      replaceList.forEach((candidate) => {
        nextVariants.push(`${prefix}${candidate}`);
      });
    });

    variants = Array.from(new Set(nextVariants)).slice(0, 48);
  }

  return variants;
}

function resolveColorCodeFromToken(token, referenceColor = null) {
  if (!token) {
    return "";
  }

  if (ARTKAL_CODE_SET.has(token)) {
    return token;
  }

  const variants = buildTokenVariants(token);
  const matchedCodes = [];
  for (const variant of variants) {
    if (!ARTKAL_CODE_PREFIX_SET.has(variant[0])) {
      continue;
    }
    if (ARTKAL_CODE_SET.has(variant)) {
      matchedCodes.push(variant);
    }
  }

  if (!matchedCodes.length) {
    return "";
  }

  if (!referenceColor) {
    return matchedCodes[0];
  }

  let bestCode = matchedCodes[0];
  let bestDistance = Number.POSITIVE_INFINITY;

  matchedCodes.forEach((code) => {
    const colorEntry = ARTKAL_COLOR_MAP.get(code);
    if (!colorEntry) {
      return;
    }

    const distance = colorDistance(referenceColor, colorEntry.rgb);
    if (distance < bestDistance) {
      bestDistance = distance;
      bestCode = code;
    }
  });

  return bestCode;
}

function pickBestOcrCandidate(candidates, referenceColor = null) {
  if (!candidates.length) {
    return {
      token: "",
      resolvedCode: "",
      confidence: 0,
    };
  }

  const grouped = new Map();

  candidates.forEach((candidate) => {
    if (!candidate.token) {
      return;
    }

    const groupKey = candidate.resolvedCode || candidate.token;
    const colorEntry = candidate.resolvedCode ? ARTKAL_COLOR_MAP.get(candidate.resolvedCode) : null;
    const resolvedDistance =
      colorEntry && referenceColor ? colorDistance(referenceColor, colorEntry.rgb) : Number.POSITIVE_INFINITY;
    if (!grouped.has(groupKey)) {
      grouped.set(groupKey, {
        token: candidate.token,
        resolvedCode: candidate.resolvedCode,
        confidence: 0,
        score: 0,
        count: 0,
        resolvedDistance,
      });
    }

    const group = grouped.get(groupKey);
    group.token = candidate.token;
    group.resolvedCode = candidate.resolvedCode;
    group.confidence = Math.max(group.confidence, candidate.confidence);
    group.score += Math.max(candidate.confidence, 1);
    group.count += 1;
    group.resolvedDistance = Math.min(group.resolvedDistance, resolvedDistance);
  });

  let bestGroup = null;
  grouped.forEach((group) => {
    if (
      !bestGroup ||
      group.score > bestGroup.score ||
      (group.score === bestGroup.score && group.count > bestGroup.count) ||
      (group.score === bestGroup.score &&
        group.count === bestGroup.count &&
        group.resolvedDistance < bestGroup.resolvedDistance) ||
      (group.score === bestGroup.score &&
        group.count === bestGroup.count &&
        group.resolvedDistance === bestGroup.resolvedDistance &&
        group.confidence > bestGroup.confidence)
    ) {
      bestGroup = group;
    }
  });

  return (
    bestGroup || {
      token: "",
      resolvedCode: "",
      confidence: 0,
    }
  );
}

async function recognizeOcrCanvas(
  worker,
  canvas,
  referenceColor = null,
  pageSegMode = OCR_PRIMARY_PAGE_SEG_MODE,
) {
  await worker.setParameters({
    tessedit_char_whitelist: OCR_WHITELIST,
    tessedit_pageseg_mode: pageSegMode,
    user_defined_dpi: "300",
  });
  const result = await worker.recognize(canvas);
  const token = normalizeOcrToken(result.data?.text || "");

  return {
    token,
    resolvedCode: resolveColorCodeFromToken(token, referenceColor),
    confidence: result.data?.confidence || 0,
    pageSegMode,
  };
}

async function createOcrWorker() {
  const { createWorker } = await import("tesseract.js");
  const worker = await createWorker("eng", 1, {
    langPath: LOCAL_TESSERACT_LANG_PATH,
    logger: () => {},
  });

  await worker.setParameters({
    tessedit_char_whitelist: OCR_WHITELIST,
    tessedit_pageseg_mode: 10,
    user_defined_dpi: "300",
  });

  return worker;
}

function shouldRetryCluster(cluster) {
  if (!cluster) {
    return false;
  }

  if (!cluster.cells.length) {
    return false;
  }

  if (!cluster.token) {
    return true;
  }

  if (cluster.colorCodeHint) {
    return cluster.confidence < OCR_RETRY_CONFIDENCE;
  }

  return true;
}

async function recognizeClusters(clusters) {
  if (!clusters.length) {
    return {
      recognizedClusterCount: 0,
      recognizedCellCount: 0,
    };
  }

  const worker = await createOcrWorker();
  let recognizedClusterCount = 0;
  let recognizedCellCount = 0;

  try {
    for (const cluster of clusters) {
      const candidates = [];
      for (const previewCanvas of cluster.previewCanvases) {
        candidates.push(
          await recognizeOcrCanvas(
            worker,
            previewCanvas,
            cluster.fillColor,
            OCR_PRIMARY_PAGE_SEG_MODE,
          ),
        );
      }

      const picked = pickBestOcrCandidate(candidates, cluster.fillColor);
      cluster.token = picked.token;
      cluster.colorCodeHint = picked.resolvedCode;
      cluster.confidence = picked.confidence;

      if (shouldRetryCluster(cluster)) {
        const retryCandidates = [];
        cluster.cells.slice(0, MAX_RETRY_CELL_COUNT).forEach((cell) => {
          cell.previewCanvases.forEach((previewCanvas) => {
            retryCandidates.push({ previewCanvas, fillColor: cell.fillColor });
          });
        });

        const retryResults = [];
        for (const retryItem of retryCandidates) {
          for (const pageSegMode of OCR_RETRY_PAGE_SEG_MODES) {
            retryResults.push(
              await recognizeOcrCanvas(
                worker,
                retryItem.previewCanvas,
                retryItem.fillColor,
                pageSegMode,
              ),
            );
          }
        }

        const retryPicked = pickBestOcrCandidate(retryResults, cluster.fillColor);
        if (
          retryPicked.token &&
          (
            !cluster.token ||
            retryPicked.confidence > cluster.confidence + 4 ||
            (!cluster.colorCodeHint && !!retryPicked.resolvedCode)
          )
        ) {
          cluster.token = retryPicked.token;
          cluster.colorCodeHint = retryPicked.resolvedCode;
          cluster.confidence = retryPicked.confidence;
        }
      }

      if (cluster.token) {
        recognizedClusterCount += 1;
        recognizedCellCount += cluster.cells.length;
      }
    }
  } finally {
    await worker.terminate();
  }

  return {
    recognizedClusterCount,
    recognizedCellCount,
  };
}

function mapClustersToColorCodes(clusters) {
  const groupedByToken = new Map();
  const colorCodeByToken = new Map();

  clusters.forEach((cluster) => {
    if (!cluster.token) {
      return;
    }

    if (cluster.colorCodeHint) {
      colorCodeByToken.set(cluster.token, cluster.colorCodeHint);
      return;
    }

    if (!groupedByToken.has(cluster.token)) {
      groupedByToken.set(cluster.token, {
        red: 0,
        green: 0,
        blue: 0,
        count: 0,
      });
    }

    const group = groupedByToken.get(cluster.token);
    cluster.cells.forEach((cell) => {
      group.red += cell.fillColor.r;
      group.green += cell.fillColor.g;
      group.blue += cell.fillColor.b;
      group.count += 1;
    });
  });

  groupedByToken.forEach((group, token) => {
    const closestColor = findClosestColor(
      Math.round(group.red / Math.max(1, group.count)),
      Math.round(group.green / Math.max(1, group.count)),
      Math.round(group.blue / Math.max(1, group.count)),
      ARTKAL_CODE_LIST,
    );

    if (closestColor) {
      colorCodeByToken.set(token, closestColor.code);
    }
  });

  return colorCodeByToken;
}

function buildCellClusters(imageData, gridModel) {
  const clusters = [];
  const allCells = [];

  for (let y = 0; y < gridModel.height; y += 1) {
    for (let x = 0; x < gridModel.width; x += 1) {
      const fillColor = extractCellFillColor(imageData, gridModel, x, y);
      const glyph = buildGlyphBinary(imageData, gridModel, x, y, fillColor);
      const ocrBounds = getCellBounds(gridModel, x, y, 0.12);
      const ocrPreviewCanvases = buildCellOcrCanvasSet(imageData, ocrBounds, fillColor);
      const previewCanvases = [
        ...ocrPreviewCanvases.slice(0, MAX_CELL_OCR_VARIANT_COUNT),
        ...buildPreviewCanvasSet(glyph.binary),
      ].slice(0, MAX_CELL_OCR_VARIANT_COUNT);
      const cell = {
        x,
        y,
        fillColor,
        binary: glyph.binary,
        darkPixels: glyph.darkPixels,
        bounds: getCellBounds(gridModel, x, y, 0.04),
        previewCanvases,
      };
      allCells.push(cell);

      if (glyph.darkPixels < 8) {
        continue;
      }

      let matchedCluster = null;
      let matchedDistance = Number.POSITIVE_INFINITY;

      for (const cluster of clusters) {
        const distance = hammingDistance(cluster.binary, glyph.binary);
        const fillDistance = colorDistance(cluster.fillColor, fillColor);
        if (fillDistance > 42) {
          continue;
        }
        if (distance < matchedDistance) {
          matchedDistance = distance;
          matchedCluster = cluster;
        }
      }

      if (!matchedCluster || matchedDistance > SIGNATURE_MATCH_THRESHOLD) {
        const nextCluster = {
          id: `cluster-${clusters.length + 1}`,
          binary: glyph.binary,
          fillColor,
          previewCanvases: [...cell.previewCanvases],
          cells: [cell],
          token: "",
          colorCodeHint: "",
          confidence: 0,
          fillRed: fillColor.r,
          fillGreen: fillColor.g,
          fillBlue: fillColor.b,
          fillCount: 1,
        };
        clusters.push(nextCluster);
        continue;
      }

      matchedCluster.cells.push(cell);
      matchedCluster.fillRed += fillColor.r;
      matchedCluster.fillGreen += fillColor.g;
      matchedCluster.fillBlue += fillColor.b;
      matchedCluster.fillCount += 1;
      matchedCluster.fillColor = {
        r: Math.round(matchedCluster.fillRed / matchedCluster.fillCount),
        g: Math.round(matchedCluster.fillGreen / matchedCluster.fillCount),
        b: Math.round(matchedCluster.fillBlue / matchedCluster.fillCount),
      };
      if (matchedCluster.previewCanvases.length < MAX_CLUSTER_PREVIEW_COUNT) {
        cell.previewCanvases.forEach((previewCanvas) => {
          if (matchedCluster.previewCanvases.length >= MAX_CLUSTER_PREVIEW_COUNT) {
            return;
          }
          matchedCluster.previewCanvases.push(previewCanvas);
        });
      }
    }
  }

  return {
    clusters,
    allCells,
  };
}

function buildPixelMap(allCells, tokenColorCodes) {
  const pixels = new Map();
  const resolvedCells = [];

  allCells.forEach((cell) => {
    let colorCode = "";
    let source = "sample";

    if (cell.clusterResolvedCode) {
      colorCode = cell.clusterResolvedCode;
      source = "ocr";
    } else if (cell.clusterToken && tokenColorCodes.has(cell.clusterToken)) {
      colorCode = tokenColorCodes.get(cell.clusterToken) || "";
      source = "token";
    } else {
      const closestColor = findClosestColor(
        cell.fillColor.r,
        cell.fillColor.g,
        cell.fillColor.b,
        ARTKAL_CODE_LIST,
      );
      colorCode = closestColor?.code || "";
    }

    if (colorCode) {
      pixels.set(`${cell.x},${cell.y}`, colorCode);
    }

    resolvedCells.push({
      ...cell,
      colorCode,
      source,
    });
  });

  return {
    pixels,
    resolvedCells,
  };
}

function buildReviewCells(resolvedCells) {
  return resolvedCells.map((cell) => ({
    key: `${cell.x},${cell.y}`,
    x: cell.x,
    y: cell.y,
    code: cell.colorCode,
    token: cell.clusterToken || "",
    source: cell.source,
    confidence: cell.clusterConfidence || 0,
    ocrCode: cell.clusterResolvedCode || "",
    sampleColor: {
      r: cell.fillColor.r,
      g: cell.fillColor.g,
      b: cell.fillColor.b,
    },
    bounds: {
      left: Math.round(cell.bounds.left),
      top: Math.round(cell.bounds.top),
      width: Math.max(1, Math.round(cell.bounds.right - cell.bounds.left)),
      height: Math.max(1, Math.round(cell.bounds.bottom - cell.bounds.top)),
    },
  }));
}

export async function importPatternFromNumberedSheet(file, options = {}) {
  if (!file) {
    throw new Error("缺少图纸图片文件");
  }

  const dataUrl = await readFileAsDataUrl(file);
  const image = await loadImage(dataUrl);
  const sourceCanvas = createCanvas(image.width, image.height);
  const sourceContext = getContext(sourceCanvas, { willReadFrequently: true });
  sourceContext.drawImage(image, 0, 0);

  const cropArea = normalizeCropArea(sourceCanvas.width, sourceCanvas.height, options.crop);
  const croppedCanvas = cropCanvas(sourceCanvas, cropArea);
  const croppedContext = getContext(croppedCanvas, { willReadFrequently: true });
  let croppedImageData = croppedContext.getImageData(
    0,
    0,
    croppedCanvas.width,
    croppedCanvas.height,
  );
  if (options.removeBackground) {
    croppedImageData = removeBackgroundWithinCrop(croppedImageData);
    croppedContext.putImageData(croppedImageData, 0, 0);
  }
  const cellSample = options.cellSample
    ? {
        x: clampNumber(options.cellSample.x - cropArea.x, 0, croppedCanvas.width - 1, 0),
        y: clampNumber(options.cellSample.y - cropArea.y, 0, croppedCanvas.height - 1, 0),
        width: options.cellSample.width,
        height: options.cellSample.height,
      }
    : null;
  const gridModel = buildGridModel(croppedImageData, {
    ...options,
    cellSample,
  });
  const { clusters, allCells } = buildCellClusters(croppedImageData, gridModel);
  const ocrResult = await recognizeClusters(clusters);

  clusters.forEach((cluster) => {
    cluster.cells.forEach((cell) => {
      cell.clusterToken = cluster.token;
      cell.clusterResolvedCode = cluster.colorCodeHint;
      cell.clusterConfidence = cluster.confidence || 0;
    });
  });

  const tokenColorCodes = mapClustersToColorCodes(clusters);
  const pixelResult = buildPixelMap(allCells, tokenColorCodes);
  const document = createPatternDocument({
    id: options.id || `numbered-sheet-${Date.now()}`,
    name: options.name || file.name || "编号图纸导入",
    width: gridModel.width,
    height: gridModel.height,
    pixels: pixelResult.pixels,
    sourceType: "numbered-sheet",
  });
  const reviewCells = buildReviewCells(pixelResult.resolvedCells);

  return {
    document,
    report: {
      mode: "numbered-sheet",
      sourceWidth: image.width,
      sourceHeight: image.height,
      cropX: cropArea.x,
      cropY: cropArea.y,
      cropWidth: cropArea.width,
      cropHeight: cropArea.height,
      croppedWidth: croppedCanvas.width,
      croppedHeight: croppedCanvas.height,
      cellWidth: gridModel.cellWidth,
      cellHeight: gridModel.cellHeight,
      backgroundRemoved: !!options.removeBackground,
      detectedWidth: gridModel.detectedWidth,
      detectedHeight: gridModel.detectedHeight,
      width: gridModel.width,
      height: gridModel.height,
      clusterCount: clusters.length,
      recognizedClusterCount: ocrResult.recognizedClusterCount,
      recognizedCellCount: ocrResult.recognizedCellCount,
      totalCells: gridModel.width * gridModel.height,
      tokenCount: tokenColorCodes.size,
      cropPreviewUrl: croppedCanvas.toDataURL("image/png"),
      reviewCells,
    },
  };
}
