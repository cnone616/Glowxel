import { clamp, hexToRgb, setPixel } from "@/utils/device-mode-core.js";
import { DEVICE_CLOCK_FONTS, drawClockTextToMap, getCurrentTimeText } from "@/utils/device-mode-clock.js";

export const PLANET_PAGE_STORAGE_KEY = "planet_screensaver_page_state";
export const PLANET_SCREEN_PRESETS = Object.freeze([
  { id: "terran_wet", label: "湿润行星" },
  { id: "terran_dry", label: "干旱行星" },
  { id: "islands", label: "群岛行星" },
  { id: "gas_giant_1", label: "风暴巨星" },
  { id: "gas_giant_2", label: "星环巨星" },
  { id: "ice_world", label: "冰封行星" },
  { id: "lava_world", label: "熔核行星" },
  { id: "black_hole", label: "黑洞" },
  { id: "galaxy", label: "星系" },
  { id: "star", label: "恒星" },
]);
export const PLANET_SIZE_OPTIONS = Object.freeze([
  { id: "small", label: "小" },
  { id: "medium", label: "中" },
  { id: "large", label: "大" },
]);
export const PLANET_DIRECTION_OPTIONS = Object.freeze([
  { id: "left", label: "左转" },
  { id: "right", label: "右转" },
]);
export const PLANET_PREVIEW_MIN_SPEED = 1;
export const PLANET_PREVIEW_MAX_SPEED = 7;
export const PLANET_PREVIEW_PLAYBACK_INTERVAL_MS = 120;
export const PLANET_TIME_COLOR_OPTIONS = Object.freeze([
  { label: "青色", value: "#64c8ff" },
  { label: "绿色", value: "#00ff9d" },
  { label: "黄色", value: "#ffdc00" },
  { label: "橙色", value: "#ffa500" },
  { label: "红色", value: "#ff6464" },
  { label: "紫色", value: "#c864ff" },
  { label: "白色", value: "#ffffff" },
]);

function paletteForPreset(preset) {
  if (preset === "terran_dry") {
    return ["#3b2415", "#aa6c39", "#d5c087"];
  }
  if (preset === "islands") {
    return ["#194167", "#2ea8df", "#7ff0b2"];
  }
  if (preset === "gas_giant_1") {
    return ["#53311f", "#c87d42", "#f0d398"];
  }
  if (preset === "gas_giant_2") {
    return ["#4d2f66", "#9d74ce", "#f0dbff"];
  }
  if (preset === "ice_world") {
    return ["#0e3557", "#6dbce3", "#dff9ff"];
  }
  if (preset === "lava_world") {
    return ["#220000", "#a31515", "#ffb347"];
  }
  if (preset === "black_hole") {
    return ["#070707", "#5a2d8c", "#fba8ff"];
  }
  if (preset === "galaxy") {
    return ["#070b22", "#584dd8", "#ff9be8"];
  }
  if (preset === "star") {
    return ["#402100", "#ff9f1c", "#fff0a8"];
  }
  return ["#0d2745", "#2c7fb8", "#93d5ff"];
}

export function getPlanetFontOptions() {
  return DEVICE_CLOCK_FONTS;
}

export function createDefaultPlanetPreviewConfig() {
  return {
    preset: "terran_wet",
    size: "medium",
    seed: 20260415,
    colorSeed: 20260415,
    planetX: 32,
    planetY: 32,
    direction: "right",
    speed: 3,
  };
}

export function createDefaultPlanetClockConfig() {
  return {
    font: "classic_5x7",
    showSeconds: false,
    time: {
      show: true,
      fontSize: 1,
      x: 32,
      y: 5,
      color: "#ffffff",
      align: "center",
    },
  };
}

export function createRandomPlanetPreviewSeed() {
  return Math.floor(Math.random() * 1000000000);
}

export function createRandomPlanetColorSeed() {
  return Math.floor(Math.random() * 1000000000);
}

export function normalizePlanetPageState(saved) {
  const config = createDefaultPlanetPreviewConfig();
  const clockConfig = createDefaultPlanetClockConfig();
  if (saved && typeof saved === "object") {
    if (saved.config && typeof saved.config === "object") {
      if (PLANET_SCREEN_PRESETS.some((item) => item.id === saved.config.preset)) {
        config.preset = saved.config.preset;
      }
      if (PLANET_SIZE_OPTIONS.some((item) => item.id === saved.config.size)) {
        config.size = saved.config.size;
      }
      if (PLANET_DIRECTION_OPTIONS.some((item) => item.id === saved.config.direction)) {
        config.direction = saved.config.direction;
      }
      ["planetX", "planetY", "speed", "seed", "colorSeed"].forEach((field) => {
        if (Number.isFinite(Number(saved.config[field]))) {
          config[field] = Math.round(Number(saved.config[field]));
        }
      });
      config.planetX = clamp(config.planetX, 0, 63);
      config.planetY = clamp(config.planetY, 0, 63);
      config.speed = clamp(config.speed, PLANET_PREVIEW_MIN_SPEED, PLANET_PREVIEW_MAX_SPEED);
    }
    if (saved.clockConfig && typeof saved.clockConfig === "object") {
      if (typeof saved.clockConfig.font === "string") {
        clockConfig.font = saved.clockConfig.font;
      }
      if (saved.clockConfig.showSeconds === true || saved.clockConfig.showSeconds === false) {
        clockConfig.showSeconds = saved.clockConfig.showSeconds;
      }
      if (saved.clockConfig.time && typeof saved.clockConfig.time === "object") {
        const time = saved.clockConfig.time;
        if (time.show === true || time.show === false) {
          clockConfig.time.show = time.show;
        }
        if (Number.isFinite(Number(time.fontSize))) {
          clockConfig.time.fontSize = clamp(Math.round(Number(time.fontSize)), 1, 3);
        }
        if (Number.isFinite(Number(time.x))) {
          clockConfig.time.x = clamp(Math.round(Number(time.x)), 0, 63);
        }
        if (Number.isFinite(Number(time.y))) {
          clockConfig.time.y = clamp(Math.round(Number(time.y)), 0, 63);
        }
        if (typeof time.color === "string") {
          clockConfig.time.color = time.color;
        }
        if (time.align === "left" || time.align === "center" || time.align === "right") {
          clockConfig.time.align = time.align;
        }
      }
    }
  }
  return { config, clockConfig };
}

export function getPlanetPreviewCycleDuration(speed) {
  const safeSpeed = clamp(Number(speed) || PLANET_PREVIEW_MIN_SPEED, PLANET_PREVIEW_MIN_SPEED, PLANET_PREVIEW_MAX_SPEED);
  return 2600 - safeSpeed * 180;
}

function radiusForSize(size) {
  if (size === "small") {
    return 11;
  }
  if (size === "large") {
    return 21;
  }
  return 16;
}

function shadeColor(hex, factor) {
  const rgb = hexToRgb(hex);
  return `rgb(${clamp(Math.round(rgb.r * factor), 0, 255)}, ${clamp(Math.round(rgb.g * factor), 0, 255)}, ${clamp(Math.round(rgb.b * factor), 0, 255)})`;
}

function drawRing(map, cx, cy, radius, tick, color) {
  for (let angle = 0; angle < Math.PI * 2; angle += 0.08) {
    const x = Math.round(cx + Math.cos(angle) * radius * 1.4);
    const y = Math.round(cy + Math.sin(angle + tick * 0.04) * (radius * 0.4));
    setPixel(map, x, y, color);
  }
}

function drawPlanetCore(map, config, frameIndex) {
  const palette = paletteForPreset(config.preset);
  const radius = radiusForSize(config.size);
  const direction = config.direction === "left" ? -1 : 1;
  const tick = frameIndex * (0.5 + config.speed * 0.16) * direction;
  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const dx = x - config.planetX;
      const dy = y - config.planetY;
      const distance = Math.sqrt(dx * dx + dy * dy);
      if (distance > radius) {
        continue;
      }
      const ratio = distance / Math.max(1, radius);
      const wave = Math.sin((dx + tick) / 4) + Math.cos((dy - tick) / 5);
      const color = wave > 0.8 ? palette[2] : wave > -0.2 ? palette[1] : palette[0];
      const brightness = 1 - ratio * 0.28;
      setPixel(map, x, y, shadeColor(color, brightness));
    }
  }
  if (config.preset === "gas_giant_2" || config.preset === "black_hole") {
    drawRing(map, config.planetX, config.planetY, radius + 3, tick, shadeColor(palette[2], 0.92));
  }
}

export function buildPlanetScreensaverPreviewSequence(config) {
  const maps = [];
  const delays = [];
  for (let frameIndex = 0; frameIndex < 24; frameIndex += 1) {
    const map = new Map();
    drawPlanetCore(map, config, frameIndex);
    maps.push(map);
    delays.push(PLANET_PREVIEW_PLAYBACK_INTERVAL_MS);
  }
  return { maps, delays };
}

export function buildPlanetPreviewPixels(config, clockConfig, frameIndex) {
  const sequence = buildPlanetScreensaverPreviewSequence(config);
  const base = sequence.maps[frameIndex % sequence.maps.length] || new Map();
  const map = new Map(base);
  if (clockConfig.time.show) {
    drawClockTextToMap({
      map,
      text: getCurrentTimeText(clockConfig.showSeconds, 24),
      x: clockConfig.time.x,
      y: clockConfig.time.y,
      color: clockConfig.time.color,
      fontId: clockConfig.font,
      fontSize: clockConfig.time.fontSize,
      align: clockConfig.time.align,
    });
  }
  return map;
}

export function buildPlanetSendPayload(config, clockConfig) {
  return {
    preset: config.preset,
    size: config.size,
    direction: config.direction,
    planetX: config.planetX,
    planetY: config.planetY,
    speed: config.speed,
    seed: config.seed,
    colorSeed: config.colorSeed,
    font: clockConfig.font,
    showSeconds: clockConfig.showSeconds,
    time: {
      show: clockConfig.time.show,
      fontSize: clockConfig.time.fontSize,
      x: clockConfig.time.x,
      y: clockConfig.time.y,
      color: hexToRgb(clockConfig.time.color),
    },
  };
}
