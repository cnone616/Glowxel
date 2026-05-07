import { clamp, setPixel } from "@/utils/device-mode-core.js";
import { DEVICE_CLOCK_FONTS, drawClockTextToMap, getCurrentTimeText } from "@/utils/device-mode-clock.js";

export const WATER_WORLD_CONFIG_KEY = "water_world_preview_config";
export const WATER_WORLD_CLOCK_CONFIG_KEY = "water_world_preview_clock_config";
export const WATER_WORLD_OPTIONS = Object.freeze([
  { preset: "surface", label: "海面波浪" },
  { preset: "current", label: "深海海流" },
  { preset: "caustics", label: "海底焦散" },
]);

export const WATER_WORLD_COLOR_THEME_OPTIONS = Object.freeze([
  { id: "cyan", label: "清澈蓝", swatches: ["#0f2d5c", "#1c5fa8", "#64c8ff"] },
  { id: "green", label: "海藻绿", swatches: ["#032f2a", "#0f6b5a", "#4be3b2"] },
  { id: "sunset", label: "晚霞海", swatches: ["#1d2c52", "#b25d44", "#ffd79a"] },
]);

export const DEFAULT_WATER_WORLD_COLOR_THEME_ID = "cyan";
export const WATER_WORLD_PRESET_COLORS = Object.freeze([
  { label: "青色", value: "#64c8ff" },
  { label: "绿色", value: "#00ff9d" },
  { label: "黄色", value: "#ffdc00" },
  { label: "橙色", value: "#ffa500" },
  { label: "红色", value: "#ff6464" },
  { label: "紫色", value: "#c864ff" },
  { label: "白色", value: "#ffffff" },
]);

export function getWaterWorldFontOptions() {
  return DEVICE_CLOCK_FONTS;
}

export function createDefaultWaterWorldConfig() {
  return {
    preset: "surface",
  };
}

export function createDefaultWaterWorldClockConfig() {
  return {
    font: "classic_5x7",
    showSeconds: false,
    hourFormat: 24,
    time: {
      show: true,
      fontSize: 2,
      x: 32,
      y: 7,
      color: "#ffffff",
      align: "center",
    },
    date: {
      show: false,
      fontSize: 1,
      x: 32,
      y: 22,
      color: "#787878",
      align: "center",
    },
    week: {
      show: false,
      x: 32,
      y: 38,
      color: "#646464",
      align: "center",
    },
    image: {
      show: false,
      x: 0,
      y: 0,
      width: 64,
      height: 64,
    },
  };
}

export function normalizeWaterWorldConfig(saved) {
  const next = createDefaultWaterWorldConfig();
  if (!saved || typeof saved !== "object") {
    return next;
  }
  if (WATER_WORLD_OPTIONS.some((item) => item.preset === saved.preset)) {
    next.preset = saved.preset;
  }
  return next;
}

export function normalizeWaterWorldClockConfig(saved) {
  const next = createDefaultWaterWorldClockConfig();
  if (!saved || typeof saved !== "object") {
    return next;
  }
  if (typeof saved.font === "string") {
    next.font = saved.font;
  }
  if (saved.showSeconds === true || saved.showSeconds === false) {
    next.showSeconds = saved.showSeconds;
  }
  if (saved.hourFormat === 12 || saved.hourFormat === 24) {
    next.hourFormat = saved.hourFormat;
  }
  if (saved.time && typeof saved.time === "object") {
    if (Number.isFinite(Number(saved.time.fontSize))) {
      next.time.fontSize = clamp(Math.round(Number(saved.time.fontSize)), 1, 3);
    }
    if (Number.isFinite(Number(saved.time.x))) {
      next.time.x = clamp(Math.round(Number(saved.time.x)), 0, 63);
    }
    if (Number.isFinite(Number(saved.time.y))) {
      next.time.y = clamp(Math.round(Number(saved.time.y)), 0, 63);
    }
    if (typeof saved.time.color === "string") {
      next.time.color = saved.time.color;
    }
    if (
      saved.time.align === "left" ||
      saved.time.align === "center" ||
      saved.time.align === "right"
    ) {
      next.time.align = saved.time.align;
    }
  }
  return next;
}

export function buildWaterWorldSendPlan(preset) {
  if (preset === "surface") {
    return { deviceMode: "led_matrix_showcase", command: { cmd: "set_ambient_effect", preset: "surface", speed: 4, loop: true } };
  }
  if (preset === "current") {
    return { deviceMode: "led_matrix_showcase", command: { cmd: "set_ambient_effect", preset: "current", speed: 5, loop: true } };
  }
  if (preset === "caustics") {
    return { deviceMode: "led_matrix_showcase", command: { cmd: "set_ambient_effect", preset: "caustics", speed: 4, loop: true } };
  }
  throw new Error("未识别的水世界 preset");
}

function themeById(themeId) {
  const matched = WATER_WORLD_COLOR_THEME_OPTIONS.find((item) => item.id === themeId);
  if (matched === undefined) {
    throw new Error("未识别的水世界主题");
  }
  return matched;
}

export function pickRandomWaterWorldColorThemeId(currentThemeId) {
  const others = WATER_WORLD_COLOR_THEME_OPTIONS.filter((item) => item.id !== currentThemeId);
  const pool = others.length > 0 ? others : WATER_WORLD_COLOR_THEME_OPTIONS;
  return pool[Math.floor(Math.random() * pool.length)].id;
}

export function createWaterWorldPreviewState(config, themeId) {
  return {
    config: { ...config },
    themeId,
    tick: 0,
    frameDelay: 120,
  };
}

export function stepWaterWorldPreviewState(state) {
  state.tick += 1;
}

function drawSurface(map, tick, colors) {
  for (let x = 0; x < 64; x += 1) {
    const wave = Math.floor(18 + Math.sin((x + tick * 2) / 7) * 3);
    for (let y = wave; y < 64; y += 1) {
      const ratio = (y - wave) / Math.max(1, 64 - wave);
      const color = ratio < 0.2 ? colors[2] : ratio < 0.55 ? colors[1] : colors[0];
      setPixel(map, x, y, color);
    }
  }
}

function drawCurrent(map, tick, colors) {
  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const band = Math.sin((x + tick * 3 + y * 0.7) / 6);
      const color = band > 0.35 ? colors[2] : band > -0.1 ? colors[1] : colors[0];
      setPixel(map, x, y, color);
    }
  }
}

function drawCaustics(map, tick, colors) {
  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const shimmer = Math.sin((x * 0.9 + tick * 4) / 5) + Math.cos((y * 1.2 + tick * 3) / 7);
      const color = shimmer > 1 ? colors[2] : shimmer > 0 ? colors[1] : colors[0];
      setPixel(map, x, y, color);
    }
  }
}

export function renderWaterWorldPreviewState(state) {
  const map = new Map();
  const theme = themeById(state.themeId);
  const colors = theme.swatches;
  if (state.config.preset === "surface") {
    drawSurface(map, state.tick, colors);
  } else if (state.config.preset === "current") {
    drawCurrent(map, state.tick, colors);
  } else {
    drawCaustics(map, state.tick, colors);
  }
  return map;
}

export function buildWaterWorldPreviewPixels(state, clockConfig) {
  const map = renderWaterWorldPreviewState(state);
  const timeText = getCurrentTimeText(clockConfig.showSeconds, clockConfig.hourFormat);
  drawClockTextToMap({
    map,
    text: timeText,
    x: clockConfig.time.x,
    y: clockConfig.time.y,
    color: clockConfig.time.color,
    fontId: clockConfig.font,
    fontSize: clockConfig.time.fontSize,
    align: clockConfig.time.align,
  });
  return map;
}
