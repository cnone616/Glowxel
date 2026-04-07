import {
  drawClockTextToPixels,
  drawTinyTextToPixels,
  getClockTextWidth,
} from "./clockCanvas.js";

function setPixel(pixels, x, y, color) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }
  pixels.set(`${x},${y}`, color);
}

function fillRect(pixels, x, y, width, height, color) {
  for (let row = 0; row < height; row += 1) {
    for (let col = 0; col < width; col += 1) {
      setPixel(pixels, x + col, y + row, color);
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

function drawPattern(pixels, pattern, x, y, color, scale) {
  for (let row = 0; row < pattern.length; row += 1) {
    const line = pattern[row];
    for (let col = 0; col < line.length; col += 1) {
      if (line[col] !== "1") {
        continue;
      }
      fillRect(
        pixels,
        x + col * scale,
        y + row * scale,
        scale,
        scale,
        color,
      );
    }
  }
}

function drawBadge(pixels, x, y, text, color, backgroundColor) {
  const width = Math.min(56, Math.max(14, text.length * 8 + 4));
  fillRect(pixels, x, y, width, 8, backgroundColor);
  drawTinyTextToPixels(text, x + Math.floor(width / 2), y + 1, color, pixels, 1, "center");
}

function hasWideGlyph(text) {
  return /[^\x00-\xff]/.test(text);
}

function drawWideGlyph(pixels, x, y, seed, color) {
  const patterns = [
    [
      "11111",
      "10001",
      "10101",
      "10001",
      "11111",
    ],
    [
      "11111",
      "10101",
      "11111",
      "10101",
      "11111",
    ],
    [
      "11111",
      "11011",
      "10101",
      "11011",
      "11111",
    ],
    [
      "11111",
      "10001",
      "11111",
      "10001",
      "11111",
    ],
  ];
  const pattern = patterns[seed % patterns.length];
  drawPattern(pixels, pattern, x, y, color, 1);
}

function drawMixedLabel(pixels, text, x, y, color, align) {
  const chars = Array.from(text);
  const widths = chars.map((char) => {
    if (hasWideGlyph(char)) {
      return 6;
    }
    return getClockTextWidth(char, "minimal_3x5", 1);
  });

  let totalWidth = 0;
  widths.forEach((width, index) => {
    totalWidth += width;
    if (index !== widths.length - 1) {
      totalWidth += 1;
    }
  });

  let startX = x;
  if (align === "center") {
    startX = x - Math.floor(totalWidth / 2);
  }
  if (align === "right") {
    startX = x - totalWidth;
  }

  let currentX = startX;
  chars.forEach((char) => {
    if (hasWideGlyph(char)) {
      drawWideGlyph(pixels, currentX, y, char.charCodeAt(0), color);
      currentX += 6;
      return;
    }
    drawTinyTextToPixels(char.toUpperCase(), currentX, y, color, pixels, 1, "left");
    currentX += getClockTextWidth(char.toUpperCase(), "minimal_3x5", 1) + 1;
  });
}

function getRepeatLabel(repeatMode) {
  if (repeatMode === "once") {
    return "单次";
  }
  if (repeatMode === "daily") {
    return "每天";
  }
  if (repeatMode === "weekday") {
    return "工作日";
  }
  if (repeatMode === "weekend") {
    return "休息日";
  }
  if (repeatMode === "yearly") {
    return "每年";
  }
  return "单次";
}

function getAsciiRepeatLabel(repeatMode) {
  if (repeatMode === "once") {
    return "ONCE";
  }
  if (repeatMode === "daily") {
    return "DAILY";
  }
  if (repeatMode === "weekday") {
    return "WKDY";
  }
  if (repeatMode === "weekend") {
    return "WKND";
  }
  return "YEAR";
}

function getTimeLabel(reminder) {
  return `${String(reminder.hour).padStart(2, "0")}:${String(reminder.minute).padStart(2, "0")}`;
}

const ICON_PATTERNS = {
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

function createFrame() {
  return new Map();
}

function decorateReminderFrame(pixels, reminder) {
  drawBadge(pixels, 4, 4, getTimeLabel(reminder), "#ffffff", "#1a2536");
  drawBadge(
    pixels,
    40,
    4,
    getAsciiRepeatLabel(reminder.repeatMode),
    reminder.accentColor,
    "#1a2536",
  );
  strokeRect(pixels, 4, 14, 56, 46, "#162236");
}

function buildTextHeadlineFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  fillRect(pixels, 8, 18, 48, 18, "#132032");
  strokeRect(pixels, 8, 18, 48, 18, reminder.accentColor);
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 24, "#ffffff", "center");
  drawTinyTextToPixels(getRepeatLabel(reminder.repeatMode), 32, 46, reminder.accentColor, pixels, 2, "center");
  drawBarGlow(pixels, 10, 54, 44, reminder.accentColor);
  addSparkleSet(pixels, [[14, 20], [50, 20]], "#ffffff");
  return pixels;
}

function drawBarGlow(pixels, x, y, width, color) {
  fillRect(pixels, x, y, width, 2, color);
  fillRect(pixels, x + 4, y - 2, width - 8, 1, color);
}

function addSparkle(pixels, x, y, color) {
  setPixel(pixels, x, y, color);
  setPixel(pixels, x - 1, y, color);
  setPixel(pixels, x + 1, y, color);
  setPixel(pixels, x, y - 1, color);
  setPixel(pixels, x, y + 1, color);
}

function addSparkleSet(pixels, points, color) {
  points.forEach(([x, y]) => addSparkle(pixels, x, y, color));
}

function buildTextBadgeFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  fillRect(pixels, 10, 20, 44, 24, "#182842");
  strokeRect(pixels, 10, 20, 44, 24, reminder.accentColor);
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 28, "#ffffff", "center");
  drawMixedLabel(pixels, reminder.reminderName.slice(0, 8), 32, 48, reminder.accentColor, "center");
  addSparkleSet(pixels, [[16, 24], [48, 24]], reminder.accentColor);
  return pixels;
}

function buildTickerFrames(reminder) {
  const frames = [];
  const maxOffset = reminder.text.length * 6 + 64;
  for (let step = 0; step < 14; step += 1) {
    const pixels = createFrame();
    decorateReminderFrame(pixels, reminder);
    fillRect(pixels, 6, 24, 52, 14, "#111d2d");
    strokeRect(pixels, 6, 24, 52, 14, reminder.accentColor);
    const offsetX = 64 - Math.floor((maxOffset / 14) * step);
    drawMixedLabel(pixels, reminder.text, offsetX, 28, "#ffffff", "left");
    drawTinyTextToPixels("TICKER", 32, 46, reminder.accentColor, pixels, 1, "center");
    fillRect(pixels, 8 + (step * 3) % 44, 39, 8, 1, reminder.accentColor);
    frames.push({ pixels, delay: 90 });
  }
  return frames;
}

function buildStaticBadgeFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  drawPattern(pixels, ICON_PATTERNS[reminder.icon], 24, 18, reminder.accentColor, 2);
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 46, "#ffffff", "center");
  drawBarGlow(pixels, 12, 54, 40, reminder.accentColor);
  return pixels;
}

function buildStaticPosterFrame(reminder) {
  const pixels = createFrame();
  fillRect(pixels, 0, 0, 64, 64, "#09111d");
  fillRect(pixels, 6, 6, 52, 52, "#101a2b");
  strokeRect(pixels, 6, 6, 52, 52, reminder.accentColor);
  drawBadge(pixels, 10, 10, getTimeLabel(reminder), "#ffffff", "#162236");
  drawPattern(pixels, ICON_PATTERNS[reminder.icon], 18, 18, reminder.accentColor, 2);
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 48, "#ffffff", "center");
  drawTinyTextToPixels(getRepeatLabel(reminder.repeatMode), 32, 55, "#8aa0c0", pixels, 1, "center");
  addSparkleSet(pixels, [[14, 14], [50, 14]], "#ffffff");
  return pixels;
}

function buildStaticCornerFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  drawPattern(pixels, ICON_PATTERNS[reminder.icon], 22, 18, reminder.accentColor, 2);
  fillRect(pixels, 8, 50, 48, 6, "#162236");
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 51, "#ffffff", "center");
  setPixel(pixels, 8, 18, reminder.accentColor);
  setPixel(pixels, 9, 18, reminder.accentColor);
  setPixel(pixels, 8, 19, reminder.accentColor);
  setPixel(pixels, 56, 18, reminder.accentColor);
  setPixel(pixels, 55, 18, reminder.accentColor);
  setPixel(pixels, 56, 19, reminder.accentColor);
  addSparkleSet(pixels, [[14, 24], [50, 24]], "#ffffff");
  return pixels;
}

function buildPulseFrames(reminder) {
  const frames = [];
  const radii = [0, 1, 2, 3, 4, 3, 2, 1];

  radii.forEach((radius) => {
    const pixels = createFrame();
    decorateReminderFrame(pixels, reminder);
    drawPattern(pixels, ICON_PATTERNS[reminder.icon], 24, 18, reminder.accentColor, 2);
    if (radius > 0) {
      strokeRect(pixels, 24 - radius * 2, 18 - radius * 2, 16 + radius * 4, 16 + radius * 4, reminder.accentColor);
    }
    if (radius > 1) {
      addSparkleSet(pixels, [[16, 18], [48, 18], [16, 46], [48, 46]], "#ffffff");
    }
    drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 48, "#ffffff", "center");
    frames.push({ pixels, delay: 110 });
  });

  return frames;
}

function buildFireworksFrames(reminder) {
  const frames = [];
  const sparkSets = [
    [[8, 18], [54, 18], [14, 48], [50, 48]],
    [[10, 14], [52, 14], [10, 52], [52, 52], [32, 16]],
    [[6, 22], [58, 22], [18, 54], [46, 54], [32, 12], [32, 56]],
    [[12, 18], [52, 18], [14, 46], [50, 46], [22, 12], [42, 12]],
    [[16, 16], [48, 16], [16, 48], [48, 48]],
  ];

  sparkSets.forEach((sparkSet, index) => {
    const pixels = createFrame();
    decorateReminderFrame(pixels, reminder);
    drawPattern(pixels, ICON_PATTERNS[reminder.icon], 24, 18, reminder.accentColor, 2);
    sparkSet.forEach((point, pointIndex) => {
      const color = pointIndex % 2 === 0 ? "#ffffff" : reminder.accentColor;
      addSparkle(pixels, point[0], point[1], color);
    });
    drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 48, "#ffffff", "center");
    frames.push({ pixels, delay: index === 0 ? 140 : 90 });
  });

  return frames;
}

function buildOrbitFrames(reminder) {
  const frames = [];
  const orbitPoints = [
    [[32, 16], [48, 32], [32, 48], [16, 32]],
    [[40, 18], [46, 40], [24, 46], [18, 24]],
    [[48, 24], [40, 48], [16, 40], [24, 16]],
    [[46, 32], [32, 46], [18, 32], [32, 18]],
  ];

  orbitPoints.forEach((pointSet) => {
    const pixels = createFrame();
    decorateReminderFrame(pixels, reminder);
    drawPattern(pixels, ICON_PATTERNS[reminder.icon], 24, 18, reminder.accentColor, 2);
    pointSet.forEach((point, index) => {
      const color = index % 2 === 0 ? reminder.accentColor : "#ffffff";
      setPixel(pixels, point[0], point[1], color);
      setPixel(pixels, point[0] + 1, point[1], color);
    });
    strokeRect(pixels, 20, 14, 24, 24, "#1e2d45");
    drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 48, "#ffffff", "center");
    frames.push({ pixels, delay: 100 });
  });

  return frames;
}

export function buildNotificationReminderFrames(reminder) {
  if (reminder.contentType === "text") {
    if (reminder.textTemplate === "headline") {
      return [{ pixels: buildTextHeadlineFrame(reminder), delay: 180 }];
    }
    if (reminder.textTemplate === "badge") {
      return [{ pixels: buildTextBadgeFrame(reminder), delay: 180 }];
    }
    return buildTickerFrames(reminder);
  }

  if (reminder.contentType === "static") {
    if (reminder.staticTemplate === "badge") {
      return [{ pixels: buildStaticBadgeFrame(reminder), delay: 220 }];
    }
    if (reminder.staticTemplate === "corner") {
      return [{ pixels: buildStaticCornerFrame(reminder), delay: 220 }];
    }
    return [{ pixels: buildStaticPosterFrame(reminder), delay: 220 }];
  }

  if (reminder.animationTemplate === "pulse") {
    return buildPulseFrames(reminder);
  }
  if (reminder.animationTemplate === "orbit") {
    return buildOrbitFrames(reminder);
  }
  return buildFireworksFrames(reminder);
}

export function getReminderRepeatLabel(repeatMode) {
  return getRepeatLabel(repeatMode);
}

export function getReminderTypeLabel(contentType) {
  if (contentType === "text") {
    return "文字提示";
  }
  if (contentType === "static") {
    return "静态提示";
  }
  return "动图提示";
}
