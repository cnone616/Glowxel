import { DEVICE_PANEL_SIZE, clamp, setPixel } from "@/utils/device-mode-core.js";

const FONT_5X7 = Object.freeze({
  "0": [0x3e, 0x51, 0x49, 0x45, 0x3e],
  "1": [0x00, 0x42, 0x7f, 0x40, 0x00],
  "2": [0x42, 0x61, 0x51, 0x49, 0x46],
  "3": [0x21, 0x41, 0x45, 0x4b, 0x31],
  "4": [0x18, 0x14, 0x12, 0x7f, 0x10],
  "5": [0x27, 0x45, 0x45, 0x45, 0x39],
  "6": [0x3c, 0x4a, 0x49, 0x49, 0x30],
  "7": [0x01, 0x71, 0x09, 0x05, 0x03],
  "8": [0x36, 0x49, 0x49, 0x49, 0x36],
  "9": [0x06, 0x49, 0x49, 0x29, 0x1e],
  ":": [0x00, 0x36, 0x36, 0x00, 0x00],
  "-": [0x08, 0x08, 0x08, 0x08, 0x08],
});

export const DEVICE_CLOCK_FONTS = Object.freeze([
  { id: "classic_5x7", label: "经典 5x7" },
  { id: "minimal_4x5", label: "紧凑 4x5" },
  { id: "minimal_3x5", label: "极简 3x5" },
]);

function resolveBaseFont(fontId) {
  if (fontId === "minimal_4x5") {
    return { width: 4, height: 5, gap: 1 };
  }
  if (fontId === "minimal_3x5") {
    return { width: 3, height: 5, gap: 1 };
  }
  return { width: 5, height: 7, gap: 1 };
}

function drawClassicChar(map, char, startX, startY, color, scale) {
  const columns = FONT_5X7[char];
  if (!columns) {
    return 6 * scale;
  }
  for (let col = 0; col < columns.length; col += 1) {
    const value = columns[col];
    for (let row = 0; row < 8; row += 1) {
      if ((value & (1 << row)) === 0) {
        continue;
      }
      for (let scaleY = 0; scaleY < scale; scaleY += 1) {
        for (let scaleX = 0; scaleX < scale; scaleX += 1) {
          setPixel(map, startX + col * scale + scaleX, startY + row * scale + scaleY, color);
        }
      }
    }
  }
  return 6 * scale;
}

function drawMinimalChar(map, char, startX, startY, color, scale, fontId) {
  const raw = FONT_5X7[char];
  if (!raw) {
    return (fontId === "minimal_3x5" ? 4 : 5) * scale;
  }
  const limitCol = fontId === "minimal_3x5" ? 3 : 4;
  const limitRow = 5;
  for (let col = 0; col < limitCol; col += 1) {
    const value = raw[col];
    for (let row = 0; row < limitRow; row += 1) {
      if ((value & (1 << row)) === 0) {
        continue;
      }
      for (let scaleY = 0; scaleY < scale; scaleY += 1) {
        for (let scaleX = 0; scaleX < scale; scaleX += 1) {
          setPixel(map, startX + col * scale + scaleX, startY + row * scale + scaleY, color);
        }
      }
    }
  }
  return (limitCol + 1) * scale;
}

export function getClockTextWidth(text, fontId, scale = 1) {
  const font = resolveBaseFont(fontId);
  return Math.max(0, text.length * (font.width + font.gap) * scale - font.gap * scale);
}

export function getClockTextHeight(fontId, scale = 1) {
  const font = resolveBaseFont(fontId);
  return font.height * scale;
}

export function resolveAlignedTextX(text, fontId, fontSize, align, anchorX) {
  const width = getClockTextWidth(text, fontId, fontSize);
  if (align === "center") {
    return clamp(anchorX - Math.floor(width / 2), 0, Math.max(0, DEVICE_PANEL_SIZE - width));
  }
  if (align === "right") {
    return clamp(anchorX - width, 0, Math.max(0, DEVICE_PANEL_SIZE - width));
  }
  return clamp(anchorX, 0, Math.max(0, DEVICE_PANEL_SIZE - width));
}

export function drawClockTextToMap({
  map,
  text,
  x,
  y,
  color,
  fontId = "classic_5x7",
  fontSize = 1,
  align = "left",
}) {
  const safeSize = clamp(Math.round(fontSize), 1, 3);
  const startX = resolveAlignedTextX(text, fontId, safeSize, align, x);
  const startY = clamp(Math.round(y), 0, DEVICE_PANEL_SIZE - getClockTextHeight(fontId, safeSize));
  let cursorX = startX;
  for (let index = 0; index < text.length; index += 1) {
    const char = text[index];
    if (fontId === "classic_5x7") {
      cursorX += drawClassicChar(map, char, cursorX, startY, color, safeSize);
    } else {
      cursorX += drawMinimalChar(map, char, cursorX, startY, color, safeSize, fontId);
    }
  }
}

export function getCurrentTimeText(showSeconds, hourFormat) {
  const now = new Date();
  let hour = now.getHours();
  if (hourFormat === 12) {
    hour = hour % 12;
    if (hour === 0) {
      hour = 12;
    }
  }
  const minute = now.getMinutes();
  const second = now.getSeconds();
  const hourText = String(hour).padStart(2, "0");
  const minuteText = String(minute).padStart(2, "0");
  if (showSeconds) {
    return `${hourText}:${minuteText}:${String(second).padStart(2, "0")}`;
  }
  return `${hourText}:${minuteText}`;
}

export function getCurrentDateText() {
  const now = new Date();
  return `${String(now.getMonth() + 1).padStart(2, "0")}-${String(now.getDate()).padStart(2, "0")}`;
}
