import { clamp } from "./common.js";

function parseRgbText(value) {
  const match = /rgb\((\d+),\s*(\d+),\s*(\d+)\)/i.exec(value);
  if (!match) {
    return null;
  }
  return {
    r: clamp(Number(match[1]), 0, 255),
    g: clamp(Number(match[2]), 0, 255),
    b: clamp(Number(match[3]), 0, 255),
  };
}

function parseHexText(value) {
  if (typeof value !== "string") {
    return null;
  }
  const normalized = value.trim().toLowerCase();
  const match = /^#([0-9a-f]{6})$/.exec(normalized);
  if (!match) {
    return null;
  }
  const hex = match[1];
  return {
    r: parseInt(hex.slice(0, 2), 16),
    g: parseInt(hex.slice(2, 4), 16),
    b: parseInt(hex.slice(4, 6), 16),
  };
}

function parseColorText(value) {
  const rgb = parseRgbText(value);
  if (rgb) {
    return rgb;
  }
  const hex = parseHexText(value);
  if (hex) {
    return hex;
  }
  return { r: 0, g: 0, b: 0 };
}

function rgbToText(r, g, b) {
  return `rgb(${clamp(Math.round(r), 0, 255)}, ${clamp(Math.round(g), 0, 255)}, ${clamp(Math.round(b), 0, 255)})`;
}

function frameToPixelMap(frame) {
  const map = new Map();
  if (!frame || !(frame.pixels instanceof Uint8Array)) {
    return map;
  }
  for (let index = 0; index < frame.pixels.length; index += 5) {
    const x = frame.pixels[index];
    const y = frame.pixels[index + 1];
    const r = frame.pixels[index + 2];
    const g = frame.pixels[index + 3];
    const b = frame.pixels[index + 4];
    map.set(`${x},${y}`, rgbToText(r, g, b));
  }
  return map;
}

function pixelMapToFrame(pixelMap, delay) {
  const bytes = [];
  pixelMap.forEach((colorText, key) => {
    const [xText, yText] = key.split(",");
    const x = Number(xText);
    const y = Number(yText);
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      return;
    }
    const rgb = parseColorText(colorText);
    bytes.push(x, y, rgb.r, rgb.g, rgb.b);
  });
  return {
    type: 1,
    delay,
    totalPixels: bytes.length / 5,
    pixels: new Uint8Array(bytes),
  };
}

function mapsToFrames(maps, delay) {
  return maps.map((map) => pixelMapToFrame(map, delay));
}

export { frameToPixelMap, mapsToFrames };
