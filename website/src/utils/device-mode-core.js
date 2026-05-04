export const DEVICE_PANEL_SIZE = 64;

export function clamp(value, min, max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

export function normalizeHexColor(value) {
  if (typeof value !== "string") {
    throw new Error("颜色值无效");
  }
  const body = value.trim().replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    throw new Error("颜色值无效");
  }
  return `#${body.toLowerCase()}`;
}

export function hexToRgb(value) {
  const hex = normalizeHexColor(value);
  return {
    r: parseInt(hex.slice(1, 3), 16),
    g: parseInt(hex.slice(3, 5), 16),
    b: parseInt(hex.slice(5, 7), 16),
  };
}

export function rgbToCss(color) {
  return `rgb(${color.r}, ${color.g}, ${color.b})`;
}

export function setPixel(map, x, y, color) {
  if (!(map instanceof Map)) {
    return;
  }
  if (x < 0 || x >= DEVICE_PANEL_SIZE || y < 0 || y >= DEVICE_PANEL_SIZE) {
    return;
  }
  map.set(`${x},${y}`, color);
}

export function deletePixel(map, x, y) {
  if (!(map instanceof Map)) {
    return;
  }
  map.delete(`${x},${y}`);
}

export function mapFromEntries(entries) {
  if (!Array.isArray(entries)) {
    return new Map();
  }
  return new Map(entries);
}

export function entriesFromMap(map) {
  if (!(map instanceof Map)) {
    return [];
  }
  return Array.from(map.entries());
}

export function readStorageJson(key) {
  const raw = localStorage.getItem(key);
  if (typeof raw !== "string" || raw.length === 0) {
    return null;
  }
  try {
    return JSON.parse(raw);
  } catch (error) {
    return null;
  }
}

export function writeStorageJson(key, value) {
  localStorage.setItem(key, JSON.stringify(value));
}
