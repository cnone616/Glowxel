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

function addSparkles(pixels, points, color) {
  points.forEach(([x, y]) => {
    setPixel(pixels, x, y, color);
    setPixel(pixels, x - 1, y, color);
    setPixel(pixels, x + 1, y, color);
    setPixel(pixels, x, y - 1, color);
    setPixel(pixels, x, y + 1, color);
  });
}

function drawRainDrop(pixels, x, y, color, highlightColor) {
  setPixel(pixels, x, y, color);
  setPixel(pixels, x, y + 1, color);
  setPixel(pixels, x + 1, y + 1, highlightColor);
}

function drawSnowFlake(pixels, x, y, color) {
  setPixel(pixels, x, y, color);
  setPixel(pixels, x - 1, y, color);
  setPixel(pixels, x + 1, y, color);
  setPixel(pixels, x, y - 1, color);
  setPixel(pixels, x, y + 1, color);
}

function drawPulseHalo(pixels, centerX, centerY, radius, color) {
  for (let offset = -radius; offset <= radius; offset += 1) {
    setPixel(pixels, centerX + offset, centerY - radius, color);
    setPixel(pixels, centerX + offset, centerY + radius, color);
    setPixel(pixels, centerX - radius, centerY + offset, color);
    setPixel(pixels, centerX + radius, centerY + offset, color);
  }
}

function drawSweepDots(pixels, x, y, width, frameIndex, color, mutedColor) {
  for (let index = 0; index < width; index += 1) {
    const active = (frameIndex + index) % 7 < 2;
    setPixel(pixels, x + index, y, active ? color : mutedColor);
  }
}

function drawScanBars(pixels, x, y, width, rowCount, color) {
  for (let row = 0; row < rowCount; row += 1) {
    const barWidth = Math.max(8, width - row * 4);
    fillRect(pixels, x + row * 2, y + row * 3, barWidth, 1, color);
  }
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

function drawInsetFrame(pixels, x, y, width, height, outerColor, innerColor) {
  strokeRect(pixels, x, y, width, height, outerColor);
  if (width > 2 && height > 2) {
    strokeRect(pixels, x + 1, y + 1, width - 2, height - 2, innerColor);
  }
}

function drawCheckeredBar(pixels, x, y, width, height, colorA, colorB) {
  for (let row = 0; row < height; row += 1) {
    for (let col = 0; col < width; col += 1) {
      const color = (row + col) % 2 === 0 ? colorA : colorB;
      setPixel(pixels, x + col, y + row, color);
    }
  }
}

function drawVerticalMeter(pixels, x, y, width, height, ratio, color, trackColor) {
  fillRect(pixels, x, y, width, height, trackColor);
  const activeHeight = Math.max(1, Math.round(height * ratio));
  fillRect(pixels, x, y + height - activeHeight, width, activeHeight, color);
  for (let row = y; row < y + height; row += 4) {
    fillRect(pixels, x, row, width, 1, "#0d1522");
  }
}

function drawTickerDots(pixels, x, y, count, color, mutedColor) {
  for (let index = 0; index < count; index += 1) {
    fillRect(pixels, x + index * 4, y, 2, 2, index % 3 === 0 ? color : mutedColor);
  }
}

function clonePixels(pixels) {
  return new Map(pixels);
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
    addSparkles(
      pixels,
      [
        [24, 10],
        [34, 16],
        [40, 26],
      ],
      "#ffe7a6",
    );
  }

  if (layout === "standard") {
    fillRect(pixels, 0, 0, 64, 64, "#08111c");
    drawInsetFrame(pixels, 4, 4, 56, 56, "#142438", "#0f1d2d");
    drawCornerMarks(pixels, 4, 4, 56, 56, accentColor);
    fillRect(pixels, 8, 14, 24, 26, "#122336");
    fillRect(pixels, 34, 14, 24, 18, "#101b2b");
    fillRect(pixels, 34, 36, 24, 10, "#0e1826");
    drawBadge(pixels, 6, 6, cityLabel, "#d8e4f6", "#1a2536");
    drawPattern(pixels, weatherPattern, 6, 16, accentColor, 2);
    drawClockTextToPixels(
      formatTemp(config && config.temperature, unit),
      46,
      17,
      tempColor,
      pixels,
      "rounded_4x6",
      2,
      "center",
    );
    drawTinyTextToPixels("TEMP", 46, 31, "#879eb8", pixels, 1, "center");
    drawTinyTextToPixels(unit === "fahrenheit" ? "FAH" : "CEL", 46, 39, "#9db0c7", pixels, 1, "center");
    drawTinyTextToPixels(`${humidity}%`, 18, 46, humidityColor, pixels, 2, "center");
    drawVerticalMeter(pixels, 53, 38, 3, 14, humidity / 100, humidityColor, "#1a2538");
    drawStripedBar(pixels, 8, 55, 44, 4, humidity / 100, humidityColor, "#1a2538");
    addSparkles(
      pixels,
      [
        [38, 12],
        [52, 12],
      ],
      "#d9ecff",
    );
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
    drawSignalBars(pixels, 42, 56, [4, 8, 12], accentColor);
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
    addSparkles(
      pixels,
      [
        [14, 10],
        [20, 14],
        [46, 12],
      ],
      "#f8f0c0",
    );
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
    fillRect(pixels, 0, 0, 64, 64, "#07101a");
    drawInsetFrame(pixels, 4, 4, 56, 56, "#18293d", "#102033");
    drawCornerMarks(pixels, 4, 4, 56, 56, "#2b4060");
    drawClockTextToPixels(
      timeText,
      32,
      8,
      "#ffffff",
      pixels,
      "minimal_3x5",
      3,
      "center",
    );
    drawPattern(pixels, COUNTDOWN_PATTERNS.hourglass, 24, 18, mainColor, 2);
    fillRect(pixels, 26, 24, 12, Math.max(2, Math.round(10 * progress)), mainColor);
    fillRect(pixels, 28, 40, 8, Math.max(2, Math.round(10 * (1 - progress))), mainColor);
    fillRect(pixels, 24, 34, 16, 1, "#dcecff");
    fillRect(pixels, 30, 35, 4, 2, "#dcecff");
    drawStripedBar(pixels, 10, 55, 44, 4, progress, mainColor, "#152131");
    drawTinyTextToPixels("LEFT", 32, 48, "#8ca4be", pixels, 1, "center");
    addSparkles(
      pixels,
      [
        [18, 20],
        [46, 20],
      ],
      "#d7e5ff",
    );
  } else if (style === "sprint") {
    fillRect(pixels, 0, 0, 64, 64, "#110d13");
    drawInsetFrame(pixels, 4, 4, 56, 56, "#311b2d", "#20111d");
    drawCornerMarks(pixels, 4, 4, 56, 56, "#553046");
    drawClockTextToPixels(
      timeText,
      32,
      8,
      "#ffffff",
      pixels,
      "minimal_3x5",
      3,
      "center",
    );
    for (let row = 18; row < 50; row += 5) {
      fillRect(pixels, 10, row, 3, 2, "#ffdbe6");
      fillRect(pixels, 51, row + 2, 3, 2, "#ffdbe6");
    }
    drawPattern(pixels, COUNTDOWN_PATTERNS.sprint, 21, 20, mainColor, 2);
    drawStripedBar(pixels, 14, 48, 36, 6, progress, mainColor, "#2b1a28");
    drawBarProgress(pixels, 54, 18, 3, 28, progress, mainColor, "#2b1a28");
    drawTinyTextToPixels("GO", 16, 40, "#ffdbe6", pixels, 2, "center");
    addSparkles(
      pixels,
      [
        [18, 22],
        [46, 22],
      ],
      "#fff0f4",
    );
  } else if (style === "ring") {
    fillRect(pixels, 0, 0, 64, 64, "#07111c");
    drawInsetFrame(pixels, 4, 4, 56, 56, "#18293d", "#112132");
    drawCornerMarks(pixels, 4, 4, 56, 56, "#2b4060");
    drawRingProgress(pixels, 8, 8, 48, progress, mainColor, "#152232");
    drawClockTextToPixels(
      timeText,
      32,
      25,
      "#ffffff",
      pixels,
      "minimal_3x5",
      3,
      "center",
    );
    drawTinyTextToPixels("SYNC", 32, 47, "#8ca4be", pixels, 1, "center");
    drawStripedBar(pixels, 14, 55, 36, 3, progress, mainColor, "#152232");
    addSparkles(pixels, [[32, 49]], mainColor);
  } else if (style === "bar") {
    fillRect(pixels, 0, 0, 64, 64, "#08111c");
    drawInsetFrame(pixels, 4, 4, 56, 56, "#162236", "#0e1a28");
    drawCornerMarks(pixels, 4, 4, 56, 56, "#28405f");
    drawClockTextToPixels(
      timeText,
      32,
      8,
      "#ffffff",
      pixels,
      "minimal_3x5",
      3,
      "center",
    );
    fillRect(pixels, 12, 24, 40, 16, "#101a28");
    drawStripedBar(pixels, 12, 24, 40, 16, progress, mainColor, "#101a28");
    drawProgressTicks(pixels, 12, 44, 40, "#50647e");
    drawTinyTextToPixels(`${Math.round(progress * 100)}%`, 32, 50, mainColor, pixels, 2, "center");
    drawTickerDots(pixels, 16, 18, 8, "#8ca4be", "#33475f");
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
  const timeSize = config && config.showMilliseconds ? 2 : 3;

  if (style === "training") {
    fillRect(pixels, 0, 0, 64, 64, "#08111c");
    drawInsetFrame(pixels, 4, 4, 56, 56, "#162236", "#0f1a28");
    drawCornerMarks(pixels, 4, 4, 56, 56, "#28405f");
    drawPattern(pixels, STOPWATCH_PATTERN, 8, 18, accentColor, 2);
    drawClockTextToPixels(
      timeText,
      38,
      10,
      "#ffffff",
      pixels,
      "minimal_3x5",
      timeSize,
      "center",
    );
    drawTinyTextToPixels("LAP", 10, 8, accentColor, pixels, 2, "left");
    drawTinyTextToPixels(
      String(config && config.lapCount ? config.lapCount : 3),
      22,
      8,
      "#ffffff",
      pixels,
      2,
      "center",
    );
    drawSignalBars(pixels, 28, 56, [4, 8, 12, 16], accentColor);
    drawStripedBar(pixels, 10, 44, 44, 4, 0.76, accentColor, "#162236");
    drawTickerDots(pixels, 10, 14, 7, accentColor, "#33475f");
  } else if (style === "racing") {
    fillRect(pixels, 0, 0, 64, 64, "#140d0d");
    drawInsetFrame(pixels, 4, 4, 56, 56, "#2b1212", "#1c0e0e");
    drawCornerMarks(pixels, 4, 4, 56, 56, "#663737");
    drawCheckeredBar(pixels, 10, 16, 4, 34, "#ffffff", "#140d0d");
    drawCheckeredBar(pixels, 50, 16, 4, 34, "#ffffff", "#140d0d");
    drawClockTextToPixels(
      timeText,
      32,
      10,
      "#ffffff",
      pixels,
      "minimal_3x5",
      timeSize,
      "center",
    );
    drawTinyTextToPixels("RACE", 32, 18, accentColor, pixels, 1, "center");
    drawStripedBar(pixels, 12, 48, 40, 5, 0.72, accentColor, "#302010");
    addSparkles(
      pixels,
      [
        [16, 18],
        [48, 18],
      ],
      "#ffdbc0",
    );
  } else if (style === "lap_focus") {
    fillRect(pixels, 0, 0, 64, 64, "#08111c");
    drawInsetFrame(pixels, 4, 4, 56, 56, "#162236", "#0f1a28");
    drawCornerMarks(pixels, 4, 4, 56, 56, "#28405f");
    drawTinyTextToPixels("LAP", 10, 8, "#7ddf8a", pixels, 2, "left");
    drawTinyTextToPixels(
      String(config && config.lapCount ? config.lapCount : 3),
      22,
      8,
      "#ffffff",
      pixels,
      2,
      "center",
    );
    drawClockTextToPixels(
      timeText,
      32,
      12,
      "#ffffff",
      pixels,
      "minimal_3x5",
      timeSize,
      "center",
    );
    drawSignalBars(pixels, 10, 54, [6, 10, 14, 9], accentColor);
    drawStripedBar(pixels, 28, 46, 24, 6, 0.58, accentColor, "#162236");
    drawVerticalMeter(pixels, 52, 18, 3, 20, 0.58, accentColor, "#162236");
    addSparkles(
      pixels,
      [
        [14, 18],
        [50, 18],
      ],
      "#9dd8ff",
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

  fillRect(pixels, 0, 0, 64, 64, "#08111c");
  drawInsetFrame(pixels, 4, 4, 56, 56, "#162236", "#0f1a28");
  drawCornerMarks(pixels, 4, 4, 56, 56, accentColor);
  fillRect(pixels, 8, 14, 48, 6, "#101a28");
  fillRect(pixels, 10, 22, 44, 22, "#0f1724");
  fillRect(pixels, 8, 48, 48, 8, "#101a28");
  fillRect(pixels, 8, 22, 2, 22, accentColor);
  fillRect(pixels, 54, 22, 2, 22, accentColor);
  fillRect(pixels, 12, 24, 40, 18, "#111c2b");
  drawClockTextToPixels(
    timeText,
    32,
    9,
    "#ffffff",
    pixels,
    "minimal_3x5",
    2,
    "center",
  );
  drawTinyTextToPixels(repeatLabel, 32, 16, accentColor, pixels, 1, "center");
  drawPattern(pixels, notificationPattern, 24, 25, accentColor, 2);
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
  drawStripedBar(pixels, 12, 57, 40, 2, 1, accentColor, "#1a2538");
  drawTickerDots(pixels, 12, 52, 8, accentColor, "#33475f");
  addSparkles(
    pixels,
    [
      [14, 10],
      [50, 10],
    ],
    "#dbe9ff",
  );

  if (icon === "fireworks") {
    addSparkles(
      pixels,
      [
        [12, 14],
        [16, 18],
        [50, 12],
        [46, 18],
      ],
      "#ffffff",
    );
  }

  if (icon === "christmas") {
    setPixel(pixels, 32, 14, "#ffd24d");
  }

  if (icon === "heart" || icon === "birthday") {
    addSparkles(
      pixels,
      [
        [16, 18],
        [48, 18],
      ],
      "#ffd7e7",
    );
  }

  return pixels;
}

export function buildWeatherPreviewFrames(config) {
  const frames = [];
  const weatherType = config && typeof config.weatherType === "string"
    ? config.weatherType
    : "sunny";
  const layout = config && typeof config.layout === "string"
    ? config.layout
    : "standard";
  const accentColor = config && typeof config.accentColor === "string"
    ? config.accentColor
    : "#64c8ff";
  const humidityColor = config && typeof config.humidityColor === "string"
    ? config.humidityColor
    : "#6ed0ff";
  let iconCenterX = 14;
  let iconCenterY = 24;
  let tickerX = 8;
  let tickerY = 54;
  let tickerWidth = 44;

  if (layout === "detail") {
    iconCenterX = 42;
    iconCenterY = 22;
    tickerX = 10;
    tickerY = 49;
    tickerWidth = 44;
  } else if (layout === "night") {
    iconCenterX = 16;
    iconCenterY = 28;
    tickerX = 12;
    tickerY = 53;
    tickerWidth = 40;
  }

  for (let frameIndex = 0; frameIndex < 18; frameIndex += 1) {
    const pixels = clonePixels(buildWeatherPreview(config));
    const pulse = 0.5 + 0.5 * Math.sin((frameIndex / 16) * Math.PI * 2);
    drawSweepDots(pixels, tickerX, tickerY, tickerWidth, frameIndex, humidityColor, "#203245");

    if (weatherType === "sunny") {
      if (pulse > 0.55) {
        drawPulseHalo(
          pixels,
          iconCenterX,
          iconCenterY,
          9 + (frameIndex % 2),
          pulse > 0.82 ? "#fff2a8" : "#ffd965",
        );
      }
      addSparkles(
        pixels,
        [
          [iconCenterX - 4 + (frameIndex % 3), iconCenterY - 10],
          [iconCenterX + 8, iconCenterY + (frameIndex % 2)],
          [iconCenterX - 10, iconCenterY + 6],
        ],
        pulse > 0.45 ? "#fff5c2" : "#ffd24d",
      );
    } else if (weatherType === "rainy") {
      for (let index = 0; index < 5; index += 1) {
        const offset = (frameIndex * 2 + index * 3) % 14;
        const dropX = iconCenterX - 8 + index * 4;
        const dropY = iconCenterY + 6 + offset;
        drawRainDrop(pixels, dropX, dropY, "#8ddcff", "#d7f4ff");
        if (offset > 10) {
          setPixel(pixels, dropX - 1, iconCenterY + 20, "#b8ecff");
          setPixel(pixels, dropX + 1, iconCenterY + 20, "#b8ecff");
        }
      }
    } else if (weatherType === "snow") {
      for (let index = 0; index < 4; index += 1) {
        const flakeX = 10 + index * 12 + ((frameIndex + index) % 3);
        const flakeY = 14 + ((frameIndex * 2 + index * 5) % 28);
        drawSnowFlake(pixels, flakeX, flakeY, "#eef8ff");
      }
    } else if (weatherType === "thunder") {
      if (frameIndex % 6 === 1 || frameIndex % 6 === 2) {
        fillRect(pixels, iconCenterX + 6, iconCenterY - 4, 2, 7, "#fff3a0");
        fillRect(pixels, iconCenterX + 4, iconCenterY + 2, 5, 2, "#fff3a0");
        addSparkles(
          pixels,
          [
            [iconCenterX + 1, iconCenterY - 8],
            [iconCenterX + 11, iconCenterY - 10],
            [iconCenterX + 14, iconCenterY - 2],
          ],
          frameIndex % 6 === 1 ? "#fffbea" : "#ffd966",
        );
      }
    } else if (weatherType === "cloudy") {
      fillRect(pixels, iconCenterX - 12 + (frameIndex % 5), iconCenterY - 2, 6, 1, "#dbeafe");
      fillRect(pixels, iconCenterX + 2 - (frameIndex % 4), iconCenterY + 4, 5, 1, "#dbeafe");
      fillRect(pixels, iconCenterX - 4 + ((frameIndex + 1) % 3), iconCenterY + 9, 4, 1, accentColor);
    }

    frames.push({
      pixels,
      delay: weatherType === "thunder" ? 100 : 110,
    });
  }

  return frames;
}

export function buildCountdownPreviewFrames(config) {
  const frames = [];
  const baseProgress = Math.max(0, Math.min(1, Number(config && config.progress)));
  const style = config && typeof config.displayStyle === "string"
    ? config.displayStyle
    : "hourglass";
  const accentColor = config && config.colorMode === "fixed" && typeof config.accentColor === "string"
    ? config.accentColor
    : null;

  for (let frameIndex = 0; frameIndex < 18; frameIndex += 1) {
    const drift = frameIndex / 15;
    const pulseOffset = Math.sin((frameIndex / 18) * Math.PI * 2) * 0.02;
    const currentProgress = Math.max(0, Math.min(1, baseProgress - drift * 0.08 + pulseOffset));
    const currentConfig = {
      ...config,
      progress: currentProgress,
    };
    const pixels = clonePixels(buildCountdownPreview(currentConfig));
    const currentAccentColor = accentColor !== null
      ? accentColor
      : currentProgress > 0.5
        ? "#3fe07d"
        : currentProgress > 0.2
          ? "#ffd24d"
          : "#ff6464";

    if (style === "hourglass") {
      const streamY = 34 + (frameIndex % 6);
      fillRect(pixels, 31, streamY, 2, 2, "#ffffff");
      fillRect(pixels, 28, 43, 8 + (frameIndex % 3), 1, currentAccentColor);
      if (frameIndex % 3 === 0) {
        addSparkles(pixels, [[32, 18], [20, 28], [44, 28]], "#eaf4ff");
      }
    } else if (style === "ring") {
      const orbitStep = frameIndex % 12;
      const orbitPoints = [
        [14, 8], [26, 8], [38, 8], [50, 10],
        [55, 22], [55, 34], [53, 46], [44, 54],
        [32, 55], [20, 54], [10, 46], [8, 32],
      ];
      const orbitPoint = orbitPoints[orbitStep];
      if (orbitPoint) {
        drawSnowFlake(pixels, orbitPoint[0], orbitPoint[1], currentAccentColor);
      }
      addSparkles(
        pixels,
        [
          [10 + (frameIndex % 6), 10],
          [54 - (frameIndex % 6), 54],
        ],
        currentAccentColor,
      );
    } else if (style === "sprint") {
      fillRect(pixels, 12 + (frameIndex % 8) * 5, 46, 3, 1, "#ffe5ec");
      fillRect(pixels, 15 + (frameIndex % 7) * 5, 44, 2, 1, currentAccentColor);
      fillRect(pixels, 8 + (frameIndex % 10) * 5, 20, 2, 1, "#fff4f7");
    } else if (style === "bar") {
      fillRect(pixels, 14 + (frameIndex % 10) * 3, 22, 2, 2, "#dbeafe");
      drawSweepDots(pixels, 13, 43, 38, frameIndex, currentAccentColor, "#3b4d62");
      if (frameIndex % 4 < 2) {
        setPixel(pixels, 11, 24 + (frameIndex % 6), currentAccentColor);
        setPixel(pixels, 52, 34 - (frameIndex % 6), currentAccentColor);
      }
    }

    frames.push({
      pixels,
      delay: 100,
    });
  }

  return frames;
}

export function buildStopwatchPreviewFrames(config) {
  const frames = [];
  const baseSeconds = Math.max(0, Number(config && config.previewSeconds));
  const style =
    config && typeof config.displayStyle === "string"
      ? config.displayStyle
      : "training";
  const accentColor =
    config && typeof config.accentColor === "string"
      ? config.accentColor
      : "#ffa500";

  for (let frameIndex = 0; frameIndex < 18; frameIndex += 1) {
    const currentConfig = {
      ...config,
      previewSeconds: baseSeconds + frameIndex * 0.09,
    };
    const pixels = clonePixels(buildStopwatchPreview(currentConfig));
    drawSweepDots(pixels, 10, 58, 44, frameIndex, accentColor, "#3c4a5d");

    if (style === "training") {
      fillRect(pixels, 28 + (frameIndex % 6) * 4, 44, 2, 1, "#dff6ff");
      fillRect(pixels, 8 + (frameIndex % 4) * 4, 52 - (frameIndex % 3), 2, 2, accentColor);
    } else if (style === "racing") {
      const sideY = 18 + (frameIndex * 2 % 24);
      fillRect(pixels, 10, sideY, 4, 1, "#ffffff");
      fillRect(pixels, 50, 40 - (frameIndex * 2 % 24), 4, 1, "#ffffff");
      addSparkles(pixels, [[18 + (frameIndex % 5), 20], [46 - (frameIndex % 5), 20]], "#ffe8d7");
    } else if (style === "lap_focus") {
      fillRect(pixels, 52, 18 + (frameIndex % 12), 3, 3, accentColor);
      fillRect(pixels, 28 + (frameIndex % 5) * 4, 46, 3, 2, "#dff6ff");
    }

    frames.push({
      pixels,
      delay: 90,
    });
  }

  return frames;
}

export function buildNotificationPreviewFrames(config) {
  const frames = [];
  const accentColor = config && typeof config.accentColor === "string"
    ? config.accentColor
    : "#ffb020";
  const icon = config && typeof config.icon === "string" ? config.icon : "drink";

  for (let frameIndex = 0; frameIndex < 16; frameIndex += 1) {
    const pixels = clonePixels(buildNotificationPreview(config));
    const pulse = frameIndex % 4 < 2;

    fillRect(pixels, 8, 22, 2, 22, pulse ? accentColor : "#2b3a50");
    fillRect(pixels, 54, 22, 2, 22, pulse ? accentColor : "#2b3a50");
    fillRect(pixels, 12 + (frameIndex % 8) * 5, 52, 2, 1, accentColor);
    drawSweepDots(pixels, 12, 57, 40, frameIndex, accentColor, "#314257");
    if (frameIndex % 5 < 2) {
      fillRect(pixels, 12 + (frameIndex % 6) * 6, 14, 4, 1, "#ffffff");
    }

    if (icon === "fireworks") {
      addSparkles(
        pixels,
        [
          [18 + (frameIndex % 4), 18],
          [46 - (frameIndex % 3), 16 + (frameIndex % 2)],
        ],
        "#ffffff",
      );
    } else if (icon === "heart" || icon === "birthday") {
      addSparkles(
        pixels,
        [
          [18, 20 + (frameIndex % 3)],
          [46, 20 + ((frameIndex + 1) % 3)],
        ],
        "#ffd7e7",
      );
    } else if (icon === "drink" || icon === "break") {
      fillRect(pixels, 22 + (frameIndex % 4), 41, 20, 1, "#fdf3cf");
      fillRect(pixels, 24 + (frameIndex % 4), 42, 16, 1, accentColor);
    } else {
      addSparkles(
        pixels,
        [
          [14 + (frameIndex % 4), 12],
          [50 - (frameIndex % 4), 12],
        ],
        "#dbe9ff",
      );
    }

    frames.push({
      pixels,
      delay: 120,
    });
  }

  return frames;
}
