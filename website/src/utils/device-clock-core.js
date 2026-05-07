import {
  drawClockTextToPixels,
  drawTinyTextToPixels,
  getClockFont,
  getClockFontOptions,
  getClockTextHeight,
  getClockTextWidth,
  hexToRgb,
} from "../../../uniapp/utils/clockCanvas.js";

export const DEVICE_CLOCK_BOARD_SIZE = 64;

export const DEVICE_CLOCK_COLOR_PRESETS = [
  { name: "青色", hex: "#64c8ff" },
  { name: "绿色", hex: "#00ff9d" },
  { name: "黄色", hex: "#ffdc00" },
  { name: "橙色", hex: "#ffa500" },
  { name: "红色", hex: "#ff6464" },
  { name: "紫色", hex: "#c864ff" },
  { name: "白色", hex: "#ffffff" },
  { name: "灰色", hex: "#787878" },
  { name: "深灰", hex: "#646464" },
];

const CLOCK_WEEKDAYS = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"];
const DEFAULT_IMAGE_CONFIG = {
  show: false,
  x: 6,
  y: 6,
  width: 64,
  height: 64,
  data: null,
};

export function createDefaultClockConfig() {
  return {
    font: "classic_5x7",
    showSeconds: false,
    hourFormat: 24,
    time: {
      show: true,
      fontSize: 1,
      x: 32,
      y: 5,
      color: "#64c8ff",
      align: "center",
    },
    date: {
      show: true,
      fontSize: 1,
      x: 32,
      y: 29,
      color: "#787878",
      align: "center",
    },
    week: {
      show: true,
      x: 32,
      y: 38,
      color: "#646464",
      align: "center",
    },
    image: { ...DEFAULT_IMAGE_CONFIG },
  };
}

export function cloneClockConfig(config) {
  return JSON.parse(JSON.stringify(config));
}

export function getDeviceClockStorageKey(mode) {
  if (mode === "animation") {
    return "clock_config_animation";
  }
  if (mode === "theme") {
    return "clock_config_theme";
  }
  return "clock_config_static";
}

export function getDeviceClockFontOptions() {
  return getClockFontOptions().map((item) => ({
    id: item.id,
    name: item.name,
    width: item.width,
    height: item.height,
    spacing: item.spacing,
    previewColor: item.previewColor,
    previewBg: item.previewBg,
  }));
}

export function getPreviewSnapshot(now = new Date(), showSeconds = false, hourFormat = 24) {
  const date = now instanceof Date ? now : new Date();
  const hours24 = date.getHours();
  const hours =
    hourFormat === 12 ? ((hours24 % 12) || 12).toString().padStart(2, "0") : hours24.toString().padStart(2, "0");
  const minutes = date.getMinutes().toString().padStart(2, "0");
  const seconds = date.getSeconds().toString().padStart(2, "0");
  const month = (date.getMonth() + 1).toString().padStart(2, "0");
  const day = date.getDate().toString().padStart(2, "0");

  return {
    time: showSeconds ? `${hours}:${minutes}:${seconds}` : `${hours}:${minutes}`,
    date: `${month}-${day}`,
    week: CLOCK_WEEKDAYS[date.getDay()] || "SUN",
  };
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function setPixel(pixelMap, x, y, color) {
  if (x < 0 || x >= DEVICE_CLOCK_BOARD_SIZE || y < 0 || y >= DEVICE_CLOCK_BOARD_SIZE) {
    return;
  }
  pixelMap.set(`${x},${y}`, color);
}

function fillRect(pixelMap, x, y, width, height, color) {
  for (let py = 0; py < height; py += 1) {
    for (let px = 0; px < width; px += 1) {
      setPixel(pixelMap, x + px, y + py, color);
    }
  }
}

function strokeRect(pixelMap, x, y, width, height, color) {
  for (let px = 0; px < width; px += 1) {
    setPixel(pixelMap, x + px, y, color);
    setPixel(pixelMap, x + px, y + height - 1, color);
  }
  for (let py = 0; py < height; py += 1) {
    setPixel(pixelMap, x, y + py, color);
    setPixel(pixelMap, x + width - 1, y + py, color);
  }
}

function drawHorizontalLine(pixelMap, x, y, width, color) {
  for (let offset = 0; offset < width; offset += 1) {
    setPixel(pixelMap, x + offset, y, color);
  }
}

function drawVerticalLine(pixelMap, x, y, height, color) {
  for (let offset = 0; offset < height; offset += 1) {
    setPixel(pixelMap, x, y + offset, color);
  }
}

function drawDottedField(pixelMap, color, step, offsetX = 0, offsetY = 0) {
  for (let y = offsetY; y < DEVICE_CLOCK_BOARD_SIZE; y += step) {
    for (let x = offsetX; x < DEVICE_CLOCK_BOARD_SIZE; x += step) {
      setPixel(pixelMap, x, y, color);
    }
  }
}

function blitImageLayer(pixelMap, imageLayer, offsetX, offsetY) {
  if (!imageLayer || !imageLayer.pixels) {
    return;
  }

  imageLayer.pixels.forEach((color, key) => {
    const [rawX, rawY] = key.split(",").map(Number);
    setPixel(pixelMap, rawX + offsetX, rawY + offsetY, color);
  });
}

function drawMarioScene(pixelMap) {
  fillRect(pixelMap, 0, 0, 64, 64, "#79cfff");
  fillRect(pixelMap, 0, 48, 64, 16, "#7c4b1f");
  fillRect(pixelMap, 0, 44, 64, 4, "#a46b2b");
  fillRect(pixelMap, 6, 18, 10, 10, "#ffffff");
  fillRect(pixelMap, 9, 15, 4, 4, "#ffffff");
  fillRect(pixelMap, 42, 34, 9, 14, "#2dd66b");
  fillRect(pixelMap, 44, 30, 5, 5, "#7df6a5");
  fillRect(pixelMap, 20, 39, 8, 9, "#d9593d");
  fillRect(pixelMap, 22, 35, 4, 5, "#ffcc99");
  drawDottedField(pixelMap, "#fef08a", 5, 2, 3);
}

function drawWordsScene(pixelMap) {
  fillRect(pixelMap, 0, 0, 64, 64, "#121212");
  fillRect(pixelMap, 0, 8, 64, 2, "#3f3f46");
  fillRect(pixelMap, 0, 26, 64, 2, "#27272a");
  fillRect(pixelMap, 0, 44, 64, 2, "#3f3f46");
  drawTinyTextToPixels("IT IS", 6, 6, "#71717a", pixelMap, 1, "left");
  drawTinyTextToPixels("HALF", 6, 16, "#a1a1aa", pixelMap, 1, "left");
  drawTinyTextToPixels("PAST", 6, 24, "#71717a", pixelMap, 1, "left");
  drawTinyTextToPixels("NOW", 6, 34, "#d4d4d8", pixelMap, 1, "left");
  drawTinyTextToPixels("STATUS", 6, 52, "#52525b", pixelMap, 1, "left");
}

function drawWorldmapScene(pixelMap) {
  fillRect(pixelMap, 0, 0, 64, 64, "#08130f");
  drawDottedField(pixelMap, "#123327", 8, 1, 1);
  fillRect(pixelMap, 8, 18, 16, 8, "#1c5d49");
  fillRect(pixelMap, 18, 12, 10, 7, "#1f7258");
  fillRect(pixelMap, 34, 20, 14, 9, "#1c5d49");
  fillRect(pixelMap, 46, 14, 8, 6, "#2a8f6d");
  fillRect(pixelMap, 24, 32, 10, 7, "#1c5d49");
  drawHorizontalLine(pixelMap, 0, 42, 64, "#113327");
  drawVerticalLine(pixelMap, 32, 0, 64, "#113327");
}

function drawCastlevaniaScene(pixelMap) {
  fillRect(pixelMap, 0, 0, 64, 64, "#09061a");
  fillRect(pixelMap, 44, 6, 10, 10, "#3b2b6e");
  fillRect(pixelMap, 46, 8, 6, 6, "#d8c6ff");
  fillRect(pixelMap, 18, 24, 6, 22, "#20133f");
  fillRect(pixelMap, 24, 18, 10, 28, "#2d1c5d");
  fillRect(pixelMap, 34, 22, 6, 24, "#20133f");
  drawVerticalLine(pixelMap, 29, 8, 10, "#b68cff");
  drawHorizontalLine(pixelMap, 22, 30, 20, "#5a3aa8");
  drawDottedField(pixelMap, "#4c1d95", 9, 4, 4);
}

function drawPacmanScene(pixelMap) {
  fillRect(pixelMap, 0, 0, 64, 64, "#05050a");
  strokeRect(pixelMap, 4, 4, 56, 56, "#244cff");
  strokeRect(pixelMap, 12, 12, 40, 40, "#244cff");
  drawHorizontalLine(pixelMap, 18, 26, 28, "#244cff");
  drawVerticalLine(pixelMap, 32, 18, 28, "#244cff");
  drawDottedField(pixelMap, "#f8fafc", 6, 8, 8);
  fillRect(pixelMap, 46, 10, 6, 6, "#ffd400");
  fillRect(pixelMap, 48, 12, 2, 2, "#05050a");
  fillRect(pixelMap, 18, 42, 5, 5, "#ff6b6b");
  fillRect(pixelMap, 24, 42, 5, 5, "#6ee7ff");
}

function drawPokedexScene(pixelMap) {
  fillRect(pixelMap, 0, 0, 64, 64, "#c92f33");
  fillRect(pixelMap, 0, 32, 64, 32, "#f5f5f5");
  strokeRect(pixelMap, 0, 0, 64, 64, "#1f2937");
  fillRect(pixelMap, 6, 8, 18, 18, "#94f0ff");
  fillRect(pixelMap, 32, 10, 24, 16, "#111827");
  strokeRect(pixelMap, 31, 9, 26, 18, "#111827");
  fillRect(pixelMap, 10, 40, 18, 14, "#111827");
  fillRect(pixelMap, 36, 40, 16, 5, "#ef4444");
  fillRect(pixelMap, 36, 48, 16, 5, "#22c55e");
  fillRect(pixelMap, 27, 28, 10, 4, "#111827");
}

function drawThemeBackdrop(themeId, pixelMap) {
  switch (themeId) {
    case "clockwise_mario":
      drawMarioScene(pixelMap);
      return "#79cfff";
    case "clockwise_words":
      drawWordsScene(pixelMap);
      return "#121212";
    case "clockwise_worldmap":
      drawWorldmapScene(pixelMap);
      return "#08130f";
    case "clockwise_castlevania":
      drawCastlevaniaScene(pixelMap);
      return "#09061a";
    case "clockwise_pacman":
      drawPacmanScene(pixelMap);
      return "#05050a";
    case "clockwise_pokedex":
      drawPokedexScene(pixelMap);
      return "#c92f33";
    default:
      return "#05070c";
  }
}

function drawClockLayer(pixelMap, config, snapshot) {
  if (config.time.show) {
    drawClockTextToPixels(
      snapshot.time,
      config.time.x,
      config.time.y,
      config.time.color,
      pixelMap,
      config.font,
      config.time.fontSize || 1,
      config.time.align || "left",
    );
  }

  if (config.date.show) {
    drawClockTextToPixels(
      snapshot.date,
      config.date.x,
      config.date.y,
      config.date.color,
      pixelMap,
      config.font,
      config.date.fontSize || 1,
      config.date.align || "left",
    );
  }

  if (config.week.show) {
    drawClockTextToPixels(
      snapshot.week,
      config.week.x,
      config.week.y,
      config.week.color,
      pixelMap,
      config.font,
      1,
      config.week.align || "left",
    );
  }
}

export function renderDeviceClockFrame({
  config,
  now = new Date(),
  imageLayer = null,
  themeId = "",
} = {}) {
  const safeConfig = cloneClockConfig(config || createDefaultClockConfig());
  const pixelMap = new Map();
  const background = themeId ? drawThemeBackdrop(themeId, pixelMap) : "#05070c";
  const snapshot = getPreviewSnapshot(now, safeConfig.showSeconds, safeConfig.hourFormat);

  if (safeConfig.image.show && imageLayer) {
    blitImageLayer(pixelMap, imageLayer, safeConfig.image.x, safeConfig.image.y);
  }

  drawClockLayer(pixelMap, safeConfig, snapshot);

  return {
    size: DEVICE_CLOCK_BOARD_SIZE,
    background,
    pixels: pixelMap,
  };
}

export function renderFontPreviewFrame(fontId, showSeconds = false, hourFormat = 24) {
  const font = getClockFont(fontId) || getClockFont("classic_5x7");
  const text = showSeconds
    ? getPreviewSnapshot(new Date("2024-01-01T12:34:56"), true, hourFormat).time
    : getPreviewSnapshot(new Date("2024-01-01T12:34:56"), false, hourFormat).time;
  const width = getClockTextWidth(text, font.id, 1);
  const height = getClockTextHeight(font.id, 1);
  const startX = Math.floor((DEVICE_CLOCK_BOARD_SIZE - width) / 2);
  const startY = Math.floor((DEVICE_CLOCK_BOARD_SIZE - height) / 2);
  const pixels = new Map();
  drawClockTextToPixels(text, startX, startY, font.previewColor, pixels, font.id, 1, "left");

  return {
    size: DEVICE_CLOCK_BOARD_SIZE,
    background: font.previewBg,
    pixels,
  };
}

export function buildDeviceClockPayload(config, now = new Date()) {
  const safeConfig = cloneClockConfig(config);
  const snapshot = getPreviewSnapshot(now, safeConfig.showSeconds, safeConfig.hourFormat);
  const timeWidth = getClockTextWidth(snapshot.time, safeConfig.font, safeConfig.time.fontSize || 1);
  const dateWidth = getClockTextWidth(snapshot.date, safeConfig.font, safeConfig.date.fontSize || 1);
  const weekWidth = getClockTextWidth(snapshot.week, safeConfig.font, 1);

  let timeX = safeConfig.time.x;
  if (safeConfig.time.align === "center") {
    timeX -= Math.floor(timeWidth / 2);
  } else if (safeConfig.time.align === "right") {
    timeX -= timeWidth;
  }

  let dateX = safeConfig.date.x;
  if (safeConfig.date.align === "center") {
    dateX -= Math.floor(dateWidth / 2);
  } else if (safeConfig.date.align === "right") {
    dateX -= dateWidth;
  }

  let weekX = safeConfig.week.x;
  if (safeConfig.week.align === "center") {
    weekX -= Math.floor(weekWidth / 2);
  } else if (safeConfig.week.align === "right") {
    weekX -= weekWidth;
  }

  return {
    font: safeConfig.font,
    showSeconds: safeConfig.showSeconds,
    hourFormat: safeConfig.hourFormat,
    time: {
      show: safeConfig.time.show,
      fontSize: safeConfig.time.fontSize,
      x: timeX,
      y: safeConfig.time.y,
      color: hexToRgb(safeConfig.time.color),
    },
    date: {
      show: safeConfig.date.show,
      fontSize: safeConfig.date.fontSize,
      x: dateX,
      y: safeConfig.date.y,
      color: hexToRgb(safeConfig.date.color),
    },
    week: {
      show: safeConfig.week.show,
      x: weekX,
      y: safeConfig.week.y,
      color: hexToRgb(safeConfig.week.color),
    },
    image: {
      show: safeConfig.image.show,
      x: safeConfig.image.x,
      y: safeConfig.image.y,
      width: safeConfig.image.width,
      height: safeConfig.image.height,
    },
  };
}

export function setClockSectionAlignment(config, sectionKey, align) {
  const nextConfig = cloneClockConfig(config);
  nextConfig[sectionKey].align = align;
  if (align === "left") {
    nextConfig[sectionKey].x = 0;
  } else if (align === "center") {
    nextConfig[sectionKey].x = 32;
  } else if (align === "right") {
    nextConfig[sectionKey].x = 63;
  }
  return nextConfig;
}

export async function loadDeviceClockImageFile(file) {
  if (!(file instanceof File)) {
    throw new Error("未选择图片文件");
  }

  const sourceUrl = await new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(reader.result);
    reader.onerror = () => reject(new Error("图片读取失败"));
    reader.readAsDataURL(file);
  });

  const bitmap = await new Promise((resolve, reject) => {
    const image = new Image();
    image.onload = () => resolve(image);
    image.onerror = () => reject(new Error("图片解码失败"));
    image.src = sourceUrl;
  });

  return {
    name: file.name,
    type: file.type,
    sourceUrl,
    width: bitmap.naturalWidth,
    height: bitmap.naturalHeight,
    isGif: file.type === "image/gif",
  };
}

export async function rasterizeDeviceClockImage(imageAsset, width, height) {
  if (!imageAsset || !imageAsset.sourceUrl) {
    return null;
  }

  const targetWidth = clamp(Number(width) || 0, 1, DEVICE_CLOCK_BOARD_SIZE);
  const targetHeight = clamp(Number(height) || 0, 1, DEVICE_CLOCK_BOARD_SIZE);
  const canvas = document.createElement("canvas");
  canvas.width = targetWidth;
  canvas.height = targetHeight;
  const context = canvas.getContext("2d", { willReadFrequently: true });
  if (!context) {
    throw new Error("浏览器不支持图片栅格化");
  }

  const image = await new Promise((resolve, reject) => {
    const nextImage = new Image();
    nextImage.onload = () => resolve(nextImage);
    nextImage.onerror = () => reject(new Error("图片加载失败"));
    nextImage.src = imageAsset.sourceUrl;
  });

  context.clearRect(0, 0, targetWidth, targetHeight);
  context.imageSmoothingEnabled = false;
  context.drawImage(image, 0, 0, targetWidth, targetHeight);
  const data = context.getImageData(0, 0, targetWidth, targetHeight).data;
  const pixels = new Map();

  for (let y = 0; y < targetHeight; y += 1) {
    for (let x = 0; x < targetWidth; x += 1) {
      const index = (y * targetWidth + x) * 4;
      const alpha = data[index + 3];
      if (alpha < 24) {
        continue;
      }
      const r = data[index];
      const g = data[index + 1];
      const b = data[index + 2];
      const color = `#${r.toString(16).padStart(2, "0")}${g.toString(16).padStart(2, "0")}${b.toString(16).padStart(2, "0")}`;
      pixels.set(`${x},${y}`, color);
    }
  }

  return {
    width: targetWidth,
    height: targetHeight,
    pixels,
  };
}
