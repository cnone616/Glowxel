import {
  drawClockTextToPixels,
  drawTinyTextToPixels,
} from "./clockCanvas.js";

function setPixel(pixels, x, y, color) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }
  pixels.set(`${x},${y}`, color);
}

function fillRect(pixels, x, y, width, height, color) {
  for (let row = 0; row < height; row++) {
    for (let col = 0; col < width; col++) {
      setPixel(pixels, x + col, y + row, color);
    }
  }
}

function drawPattern(pixels, pattern, x, y, color, scale) {
  let scaleValue = scale;
  if (typeof scaleValue !== "number") {
    scaleValue = 1;
  }
  for (let row = 0; row < pattern.length; row++) {
    const line = pattern[row];
    for (let col = 0; col < line.length; col++) {
      if (line[col] !== "1") {
        continue;
      }
      fillRect(
        pixels,
        x + col * scaleValue,
        y + row * scaleValue,
        scaleValue,
        scaleValue,
        color,
      );
    }
  }
}

function strokeRect(pixels, x, y, width, height, color) {
  for (let col = 0; col < width; col += 1) {
    setPixel(pixels, x + col, y, color);
    setPixel(pixels, x + col, y + height - 1, color);
  }
  for (let row = 0; row < height; row += 1) {
    setPixel(pixels, x, y + row, color);
    setPixel(pixels, x + width - 1, y + row, color);
  }
}

function drawCornerMarks(pixels, x, y, width, height, color) {
  fillRect(pixels, x, y, 4, 1, color);
  fillRect(pixels, x, y, 1, 4, color);
  fillRect(pixels, x + width - 4, y, 4, 1, color);
  fillRect(pixels, x + width - 1, y, 1, 4, color);
  fillRect(pixels, x, y + height - 1, 4, 1, color);
  fillRect(pixels, x, y + height - 4, 1, 4, color);
  fillRect(pixels, x + width - 4, y + height - 1, 4, 1, color);
  fillRect(pixels, x + width - 1, y + height - 4, 1, 4, color);
}

function drawDotCluster(pixels, points, color) {
  points.forEach((point) => {
    setPixel(pixels, point[0], point[1], color);
  });
}

function drawStripedBar(pixels, x, y, width, height, ratio, color, trackColor) {
  fillRect(pixels, x, y, width, height, trackColor);
  const activeWidth = Math.max(1, Math.round(width * ratio));
  fillRect(pixels, x, y, activeWidth, height, color);
  for (let cursor = x; cursor < x + width; cursor += 4) {
    fillRect(pixels, cursor, y, 1, height, "#0d1522");
  }
}

function drawSignalBars(pixels, x, y, heights, color) {
  heights.forEach((height, index) => {
    fillRect(pixels, x + index * 4, y - height, 2, height, color);
  });
}

function drawProgressTicks(pixels, x, y, width, color) {
  for (let cursor = 0; cursor <= width; cursor += 8) {
    fillRect(pixels, x + cursor, y, 1, 3, color);
  }
}

function drawBarProgress(pixels, x, y, width, height, ratio, color, trackColor) {
  fillRect(pixels, x, y, width, height, trackColor);
  fillRect(pixels, x, y, Math.max(1, Math.round(width * ratio)), height, color);
}

function drawRingProgress(pixels, x, y, size, ratio, color, trackColor) {
  for (let row = 0; row < size; row++) {
    for (let col = 0; col < size; col++) {
      const edge =
        row === 0 || row === size - 1 || col === 0 || col === size - 1;
      if (!edge) {
        continue;
      }
      setPixel(pixels, x + col, y + row, trackColor);
    }
  }

  const total = (size - 1) * 4;
  const active = Math.max(1, Math.round(total * ratio));
  let painted = 0;

  for (let col = 0; col < size && painted < active; col++, painted++) {
    setPixel(pixels, x + col, y, color);
  }
  for (let row = 1; row < size && painted < active; row++, painted++) {
    setPixel(pixels, x + size - 1, y + row, color);
  }
  for (let col = size - 2; col >= 0 && painted < active; col--, painted++) {
    setPixel(pixels, x + col, y + size - 1, color);
  }
  for (let row = size - 2; row > 0 && painted < active; row--, painted++) {
    setPixel(pixels, x, y + row, color);
  }
}

function drawBadge(pixels, x, y, text, color, backgroundColor) {
  const width = Math.min(56, Math.max(12, text.length * 8 + 4));
  fillRect(pixels, x, y, width, 8, backgroundColor);
  drawTinyTextToPixels(text, x + Math.floor(width / 2), y + 1, color, pixels, 1, "center");
}

function sanitizeAsciiLabel(value, fallback) {
  if (typeof value !== "string" || value.trim().length === 0) {
    return fallback;
  }
  const cleaned = value
    .toUpperCase()
    .replace(/[^A-Z0-9 ]/g, " ")
    .trim()
    .slice(0, 8);
  if (cleaned.length === 0) {
    return fallback;
  }
  return cleaned;
}

function formatTemp(value, unit) {
  const rounded = Math.round(Number(value));
  const suffix = unit === "fahrenheit" ? "F" : "C";
  return `${rounded}${suffix}`;
}

function formatTimeFromSeconds(totalSeconds) {
  const total = Math.max(0, Math.round(Number(totalSeconds)));
  const minutes = Math.floor(total / 60);
  const seconds = total % 60;
  return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}

function formatStopwatch(totalSeconds, showMilliseconds) {
  const total = Math.max(0, Number(totalSeconds));
  const minutes = Math.floor(total / 60);
  const seconds = Math.floor(total % 60);
  const base = `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
  if (!showMilliseconds) {
    return base;
  }
  const hundredths = Math.floor((total - Math.floor(total)) * 100);
  return `${base}.${String(hundredths).padStart(2, "0")}`;
}

const WEATHER_PATTERNS = {
  sunny: [
    "00111000",
    "00111000",
    "11111111",
    "11111111",
    "01111110",
    "01111110",
    "00111000",
    "00111000",
  ],
  cloudy: [
    "00011100",
    "00111110",
    "01110011",
    "11100001",
    "11111111",
    "11111111",
    "01111110",
    "00000000",
  ],
  rainy: [
    "00011100",
    "00111110",
    "01110011",
    "11111111",
    "11111111",
    "00100100",
    "01001000",
    "10010000",
  ],
  snow: [
    "00100100",
    "00011000",
    "11111111",
    "00011000",
    "00100100",
    "01000010",
    "00100100",
    "00000000",
  ],
  thunder: [
    "00011100",
    "00111110",
    "01110011",
    "11111111",
    "11111111",
    "00011000",
    "00110000",
    "00010000",
  ],
};

const COUNTDOWN_PATTERNS = {
  hourglass: [
    "11111111",
    "01111110",
    "00111100",
    "00011000",
    "00011000",
    "00111100",
    "01111110",
    "11111111",
  ],
  sprint: [
    "11110000",
    "00111000",
    "00011100",
    "11111110",
    "01111111",
    "00111000",
    "00011100",
    "00001110",
  ],
};

const STOPWATCH_PATTERN = [
  "00111000",
  "01111100",
  "11000110",
  "11000110",
  "11010110",
  "11001110",
  "01111100",
  "00111000",
];

const NOTIFICATION_PATTERNS = {
  drink: [
    "00111100",
    "00100100",
    "00100100",
    "00111100",
    "00111100",
    "00100110",
    "00011100",
    "00000000",
  ],
  break: [
    "00110000",
    "00110000",
    "01111100",
    "11111110",
    "01111100",
    "00110000",
    "00110000",
    "00000000",
  ],
  medicine: [
    "01111000",
    "11111100",
    "11111100",
    "01111110",
    "00111111",
    "00111111",
    "00011110",
    "00001100",
  ],
  birthday: [
    "00011000",
    "00011000",
    "00111100",
    "01111110",
    "11111111",
    "11111111",
    "01111110",
    "00000000",
  ],
  fireworks: [
    "10011001",
    "01011010",
    "00111100",
    "11111111",
    "00111100",
    "01011010",
    "10011001",
    "00000000",
  ],
  christmas: [
    "00011000",
    "00111100",
    "01111110",
    "00111100",
    "01111110",
    "11111111",
    "00011000",
    "00011000",
  ],
  heart: [
    "01100110",
    "11111111",
    "11111111",
    "11111111",
    "01111110",
    "00111100",
    "00011000",
    "00000000",
  ],
  moon: [
    "00011110",
    "00111111",
    "01111110",
    "01111100",
    "01111100",
    "01111110",
    "00111111",
    "00011110",
  ],
  star: [
    "00011000",
    "10011001",
    "01011010",
    "00111100",
    "11111111",
    "00111100",
    "01011010",
    "10011001",
  ],
};

export function buildWeatherPreview(config) {
  const pixels = new Map();
  const weatherType =
    config && typeof config.weatherType === "string" ? config.weatherType : "sunny";
  const layout = config && typeof config.layout === "string" ? config.layout : "standard";
  const accentColor = config && typeof config.accentColor === "string"
    ? config.accentColor
    : "#64c8ff";
  const tempColor = config && typeof config.tempColor === "string"
    ? config.tempColor
    : "#ffffff";
  const humidityColor = config && typeof config.humidityColor === "string"
    ? config.humidityColor
    : "#6ed0ff";
  const unit = config && typeof config.unit === "string" ? config.unit : "celsius";
  const cityLabel = sanitizeAsciiLabel(config && config.city, "CITY");
  const humidity = Math.max(0, Math.min(100, Number(config && config.humidity)));
  let weatherPattern = WEATHER_PATTERNS.sunny;

  if (
    typeof weatherType === "string" &&
    Object.prototype.hasOwnProperty.call(WEATHER_PATTERNS, weatherType)
  ) {
    weatherPattern = WEATHER_PATTERNS[weatherType];
  }

  if (layout === "night") {
    fillRect(pixels, 0, 0, 64, 64, "#060d18");
    drawDotCluster(
      pixels,
      [
        [8, 8],
        [12, 16],
        [18, 10],
        [48, 8],
        [54, 14],
        [46, 18],
      ],
      "#d8d3a4",
    );
    drawPattern(pixels, NOTIFICATION_PATTERNS.moon, 42, 4, "#f7f0aa", 2);
  }

  if (layout === "standard") {
    fillRect(pixels, 0, 0, 64, 64, "#08111c");
    strokeRect(pixels, 4, 4, 56, 56, "#142438");
    drawCornerMarks(pixels, 4, 4, 56, 56, accentColor);
    fillRect(pixels, 8, 16, 24, 24, "#122336");
    fillRect(pixels, 36, 16, 20, 16, "#101b2b");
    drawBadge(pixels, 6, 6, cityLabel, "#d8e4f6", "#1a2536");
    drawPattern(pixels, weatherPattern, 6, 16, accentColor, 2);
    drawClockTextToPixels(
      formatTemp(config && config.temperature, unit),
      44,
      18,
      tempColor,
      pixels,
      "rounded_4x6",
      2,
      "center",
    );
    drawTinyTextToPixels("TEMP", 46, 35, "#879eb8", pixels, 1, "center");
    drawTinyTextToPixels(`${humidity}%`, 19, 46, humidityColor, pixels, 2, "center");
    drawTinyTextToPixels(unit === "fahrenheit" ? "FAH" : "CEL", 47, 46, "#9db0c7", pixels, 1, "center");
    drawStripedBar(pixels, 8, 55, 48, 4, humidity / 100, humidityColor, "#1a2538");
  } else if (layout === "detail") {
    fillRect(pixels, 0, 0, 64, 64, "#07111c");
    strokeRect(pixels, 4, 4, 56, 56, "#142438");
    fillRect(pixels, 6, 14, 52, 18, "#0d1b2b");
    fillRect(pixels, 6, 36, 52, 20, "#0f1826");
    drawBadge(pixels, 4, 4, cityLabel, "#d8e4f6", "#1a2536");
    drawClockTextToPixels(
      formatTemp(config && config.temperature, unit),
      18,
      16,
      tempColor,
      pixels,
      "rounded_4x6",
      2,
      "center",
    );
    drawTinyTextToPixels("NOW", 18, 30, "#8ea5c0", pixels, 1, "center");
    drawPattern(pixels, weatherPattern, 34, 14, accentColor, 2);
    drawTinyTextToPixels("HUM", 14, 41, "#8ea5c0", pixels, 1, "center");
    drawTinyTextToPixels(`${humidity}%`, 46, 40, humidityColor, pixels, 2, "center");
    drawStripedBar(pixels, 10, 50, 44, 4, humidity / 100, humidityColor, "#1a2538");
    drawTinyTextToPixels("AIR", 12, 57, "#7188a5", pixels, 1, "left");
  } else if (layout === "night") {
    fillRect(pixels, 8, 18, 22, 20, "#0b1523");
    drawPattern(pixels, weatherPattern, 8, 20, accentColor, 2);
    drawClockTextToPixels(
      formatTemp(config && config.temperature, unit),
      43,
      24,
      tempColor,
      pixels,
      "rounded_4x6",
      2,
      "center",
    );
    drawTinyTextToPixels(cityLabel, 30, 8, "#9db0c7", pixels, 1, "center");
    drawTinyTextToPixels(`${humidity}%`, 42, 40, humidityColor, pixels, 2, "center");
    drawStripedBar(pixels, 12, 54, 40, 3, humidity / 100, humidityColor, "#162236");
  }

  return pixels;
}

export function buildCountdownPreview(config) {
  const pixels = new Map();
  const style =
    config && typeof config.displayStyle === "string"
      ? config.displayStyle
      : "hourglass";
  const totalSeconds = Math.max(
    1,
    Number(config && config.hours) * 3600 +
      Number(config && config.minutes) * 60 +
      Number(config && config.seconds),
  );
  const progress = Math.max(0, Math.min(1, Number(config && config.progress)));
  let mainColor = "#64c8ff";

  if (config && config.colorMode === "fixed" && typeof config.accentColor === "string") {
    mainColor = config.accentColor;
  } else if (progress > 0.5) {
    mainColor = "#3fe07d";
  } else if (progress > 0.2) {
    mainColor = "#ffd24d";
  } else {
    mainColor = "#ff6464";
  }

  const remainSeconds = Math.round(totalSeconds * progress);
  const timeText =
    Number(config && config.hours) > 0
      ? `${String(Math.floor(remainSeconds / 3600)).padStart(2, "0")}:${String(
          Math.floor((remainSeconds % 3600) / 60),
        ).padStart(2, "0")}`
      : formatTimeFromSeconds(remainSeconds);

  if (style === "hourglass") {
    fillRect(pixels, 0, 0, 64, 64, "#09111b");
    strokeRect(pixels, 6, 6, 52, 52, "#142438");
    drawPattern(pixels, COUNTDOWN_PATTERNS.hourglass, 24, 8, mainColor, 2);
    fillRect(pixels, 26, 14, 12, Math.max(2, Math.round(10 * progress)), mainColor);
    fillRect(pixels, 28, 30, 8, Math.max(2, Math.round(10 * (1 - progress))), mainColor);
    drawClockTextToPixels(
      timeText,
      32,
      38,
      "#ffffff",
      pixels,
      "minimal_3x5",
      3,
      "center",
    );
    drawTinyTextToPixels("SAND", 32, 50, "#91a6be", pixels, 1, "center");
    drawStripedBar(pixels, 8, 56, 48, 4, progress, mainColor, "#1a2538");
  } else if (style === "sprint") {
    fillRect(pixels, 0, 0, 64, 64, "#120d12");
    fillRect(pixels, 6, 10, 52, 44, "#1b131d");
    drawPattern(pixels, COUNTDOWN_PATTERNS.sprint, 8, 14, mainColor, 2);
    drawStripedBar(pixels, 8, 50, 48, 5, progress, mainColor, "#291726");
    drawClockTextToPixels(
      timeText,
      42,
      20,
      "#ffffff",
      pixels,
      "minimal_3x5",
      3,
      "center",
    );
    drawTinyTextToPixels("RUSH", 42, 36, "#ffcfdb", pixels, 2, "center");
  } else if (style === "ring") {
    fillRect(pixels, 0, 0, 64, 64, "#07111c");
    drawRingProgress(pixels, 12, 8, 40, progress, mainColor, "#152232");
    strokeRect(pixels, 16, 12, 32, 32, "#203040");
    drawClockTextToPixels(
      timeText,
      32,
      24,
      "#ffffff",
      pixels,
      "minimal_3x5",
      2,
      "center",
    );
    drawTinyTextToPixels("LEFT", 32, 48, mainColor, pixels, 2, "center");
  } else if (style === "bar") {
    fillRect(pixels, 0, 0, 64, 64, "#08111c");
    strokeRect(pixels, 6, 8, 52, 48, "#162236");
    drawClockTextToPixels(
      timeText,
      32,
      16,
      "#ffffff",
      pixels,
      "minimal_3x5",
      3,
      "center",
    );
    drawTinyTextToPixels("PROGRESS", 32, 28, "#8ea5c0", pixels, 1, "center");
    drawStripedBar(pixels, 12, 34, 40, 12, progress, mainColor, "#162236");
    drawProgressTicks(pixels, 12, 48, 40, "#50647e");
    drawTinyTextToPixels(`${Math.round(progress * 100)}%`, 32, 52, mainColor, pixels, 2, "center");
  }

  return pixels;
}

export function buildStopwatchPreview(config) {
  const pixels = new Map();
  const style =
    config && typeof config.displayStyle === "string"
      ? config.displayStyle
      : "training";
  const accentColor = config && typeof config.accentColor === "string"
    ? config.accentColor
    : "#ffa500";
  const timeText = formatStopwatch(
    config && config.previewSeconds,
    config && config.showMilliseconds,
  );

  if (style === "training") {
    fillRect(pixels, 0, 0, 64, 64, "#08111c");
    strokeRect(pixels, 6, 8, 52, 48, "#162236");
    fillRect(pixels, 8, 12, 18, 24, "#102030");
    drawPattern(pixels, STOPWATCH_PATTERN, 8, 12, accentColor, 2);
    drawClockTextToPixels(
      timeText,
      42,
      18,
      "#ffffff",
      pixels,
      "minimal_3x5",
      2,
      "center",
    );
    drawTinyTextToPixels("TRAIN", 42, 32, "#8ea5c0", pixels, 1, "center");
    drawTinyTextToPixels("LAP", 10, 46, accentColor, pixels, 2, "left");
    drawTinyTextToPixels(
      String(config && config.lapCount ? config.lapCount : 3),
      46,
      46,
      "#ffffff",
      pixels,
      2,
      "center",
    );
  } else if (style === "racing") {
    fillRect(pixels, 0, 0, 64, 64, "#140d0d");
    fillRect(pixels, 8, 10, 48, 40, "#221414");
    drawBadge(pixels, 6, 6, "RACE", "#ffffff", "#2b1212");
    for (let row = 14; row < 46; row += 4) {
      setPixel(pixels, 12, row, "#ffffff");
      setPixel(pixels, 13, row, "#ffffff");
      setPixel(pixels, 51, row + 2, "#ffffff");
      setPixel(pixels, 52, row + 2, "#ffffff");
    }
    drawClockTextToPixels(
      timeText,
      32,
      22,
      "#ffffff",
      pixels,
      "minimal_3x5",
      3,
      "center",
    );
    drawStripedBar(pixels, 8, 54, 48, 4, 0.72, accentColor, "#302010");
  } else if (style === "lap_focus") {
    fillRect(pixels, 0, 0, 64, 64, "#08111c");
    strokeRect(pixels, 6, 8, 52, 48, "#162236");
    drawBadge(pixels, 6, 6, "BEST", "#7ddf8a", "#10261a");
    drawClockTextToPixels(
      timeText,
      32,
      20,
      "#ffffff",
      pixels,
      "minimal_3x5",
      2,
      "center",
    );
    drawSignalBars(pixels, 10, 52, [6, 10, 14, 9], accentColor);
    drawTinyTextToPixels("BEST", 24, 46, "#7ddf8a", pixels, 2, "center");
    drawTinyTextToPixels(
      String(config && config.lapCount ? config.lapCount : 3),
      48,
      46,
      accentColor,
      pixels,
      2,
      "center",
    );
  }

  return pixels;
}

export function buildNotificationPreview(config) {
  const pixels = new Map();
  const accentColor = config && typeof config.accentColor === "string"
    ? config.accentColor
    : "#ffb020";
  const icon =
    config && typeof config.icon === "string" ? config.icon : "drink";
  const hour = Number(config && config.hour);
  const minute = Number(config && config.minute);
  const timeText = `${String(hour).padStart(2, "0")}:${String(minute).padStart(2, "0")}`;
  const message = sanitizeAsciiLabel(config && config.text, "MSG");
  let repeatLabel = "ONCE";
  let notificationPattern = NOTIFICATION_PATTERNS.drink;

  if (
    typeof icon === "string" &&
    Object.prototype.hasOwnProperty.call(NOTIFICATION_PATTERNS, icon)
  ) {
    notificationPattern = NOTIFICATION_PATTERNS[icon];
  }
  if (config && config.repeatMode === "daily") {
    repeatLabel = "DAILY";
  } else if (config && config.repeatMode === "weekday") {
    repeatLabel = "WKDY";
  }

  const repeatBadgeWidth = Math.min(56, Math.max(12, repeatLabel.length * 8 + 4));
  drawBadge(pixels, 4, 4, timeText, "#ffffff", "#1a2536");
  drawBadge(pixels, 64 - repeatBadgeWidth - 4, 4, repeatLabel, accentColor, "#1a2536");
  drawPattern(pixels, notificationPattern, 22, 16, accentColor, 2);
  drawClockTextToPixels(
    message,
    32,
    48,
    "#ffffff",
    pixels,
    "minimal_3x5",
    2,
    "center",
  );
  drawBarProgress(pixels, 12, 58, 40, 2, 1, accentColor, "#1a2538");

  if (icon === "fireworks") {
    setPixel(pixels, 12, 14, "#ff6464");
    setPixel(pixels, 16, 18, "#ffd24d");
    setPixel(pixels, 50, 12, "#64c8ff");
    setPixel(pixels, 46, 18, "#ffffff");
  }

  if (icon === "christmas") {
    setPixel(pixels, 32, 14, "#ffd24d");
  }

  return pixels;
}
