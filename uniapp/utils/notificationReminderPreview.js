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
      fillRect(pixels, x + col * scale, y + row * scale, scale, scale, color);
    }
  }
}

function hasWideGlyph(text) {
  return /[^\x00-\xff]/.test(text);
}

function drawWideGlyph(pixels, x, y, seed, color) {
  const patterns = [
    ["11111", "10001", "10101", "10001", "11111"],
    ["11111", "10101", "11111", "10101", "11111"],
    ["11111", "11011", "10101", "11011", "11111"],
    ["11111", "10001", "11111", "10001", "11111"],
  ];
  drawPattern(pixels, patterns[seed % patterns.length], x, y, color, 1);
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

function getRepeatCapsuleLabel(repeatMode) {
  if (repeatMode === "daily") {
    return "DAY";
  }
  if (repeatMode === "weekday") {
    return "WKDY";
  }
  if (repeatMode === "weekend") {
    return "WKND";
  }
  if (repeatMode === "yearly") {
    return "YEAR";
  }
  return "ONCE";
}

function getTimeLabel(reminder) {
  return `${String(reminder.hour).padStart(2, "0")}:${String(reminder.minute).padStart(2, "0")}`;
}

const ICON_PATTERNS = {
  drink: ["00111100", "00100100", "00100100", "00111100", "00111100", "00100110", "00011100", "00000000"],
  break: ["00110000", "00110000", "01111100", "11111110", "01111100", "00110000", "00110000", "00000000"],
  medicine: ["01111000", "11111100", "11111100", "01111110", "00111111", "00111111", "00011110", "00001100"],
  birthday: ["00011000", "00011000", "00111100", "01111110", "11111111", "11111111", "01111110", "00000000"],
  fireworks: ["10011001", "01011010", "00111100", "11111111", "00111100", "01011010", "10011001", "00000000"],
  christmas: ["00011000", "00111100", "01111110", "00111100", "01111110", "11111111", "00011000", "00011000"],
  heart: ["01100110", "11111111", "11111111", "11111111", "01111110", "00111100", "00011000", "00000000"],
  moon: ["00011110", "00111111", "01111110", "01111100", "01111100", "01111110", "00111111", "00011110"],
  star: ["00011000", "10011001", "01011010", "00111100", "11111111", "00111100", "01011010", "10011001"],
};

function createFrame() {
  return new Map();
}

function decorateReminderFrame(pixels, reminder) {
  fillRect(pixels, 0, 0, 64, 64, "#050a12");
  fillRect(pixels, 0, 0, 64, 1, "#122133");
  drawTinyTextToPixels(getRepeatCapsuleLabel(reminder.repeatMode), 6, 6, reminder.accentColor, pixels, 1, "left");
  drawClockTextToPixels(getTimeLabel(reminder), 48, 4, "#ffffff", pixels, "minimal_3x5", 1, "center");
}

function drawAccentBar(pixels, width, color) {
  fillRect(pixels, 6, 56, 52, 2, "#0d1724");
  fillRect(pixels, 6, 56, width, 2, color);
}

function addSparkle(pixels, x, y, color) {
  setPixel(pixels, x, y, color);
  setPixel(pixels, x - 1, y, color);
  setPixel(pixels, x + 1, y, color);
  setPixel(pixels, x, y - 1, color);
  setPixel(pixels, x, y + 1, color);
}

function buildTextHeadlineFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  drawMixedLabel(pixels, reminder.reminderName.slice(0, 8), 8, 18, reminder.accentColor, "left");
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 8, 30, "#ffffff", "left");
  drawTinyTextToPixels(getRepeatLabel(reminder.repeatMode), 8, 43, "#7d94ad", pixels, 1, "left");
  drawAccentBar(pixels, 28, reminder.accentColor);
  return pixels;
}

function buildTextBadgeFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  fillRect(pixels, 8, 18, 48, 9, "#111d2b");
  drawMixedLabel(pixels, reminder.reminderName.slice(0, 8), 32, 20, reminder.accentColor, "center");
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 34, "#ffffff", "center");
  drawAccentBar(pixels, 36, reminder.accentColor);
  return pixels;
}

function buildTickerFrames(reminder) {
  const frames = [];
  const iconPattern = ICON_PATTERNS[reminder.icon];
  const travel = reminder.text.length * 6 + 54;

  for (let step = 0; step < 14; step += 1) {
    const pixels = createFrame();
    decorateReminderFrame(pixels, reminder);
    drawPattern(pixels, iconPattern, 6, 20, reminder.accentColor, 2);
    const offsetX = 58 - Math.floor((travel / 14) * step);
    drawMixedLabel(pixels, reminder.text, offsetX, 27, "#ffffff", "left");
    drawAccentBar(pixels, 10 + (step * 3) % 34, reminder.accentColor);
    frames.push({ pixels, delay: 90 });
  }

  return frames;
}

function buildStaticBadgeFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  drawPattern(pixels, ICON_PATTERNS[reminder.icon], 8, 18, reminder.accentColor, 2);
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 24, "#ffffff", "left");
  drawTinyTextToPixels(reminder.reminderName.slice(0, 8), 32, 38, "#7d94ad", pixels, 1, "left");
  drawAccentBar(pixels, 30, reminder.accentColor);
  return pixels;
}

function buildStaticPosterFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  drawPattern(pixels, ICON_PATTERNS[reminder.icon], 24, 16, reminder.accentColor, 2);
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 40, "#ffffff", "center");
  drawTinyTextToPixels(reminder.reminderName.slice(0, 8), 32, 50, "#7d94ad", pixels, 1, "center");
  return pixels;
}

function buildStaticCornerFrame(reminder) {
  const pixels = createFrame();
  decorateReminderFrame(pixels, reminder);
  drawPattern(pixels, ICON_PATTERNS[reminder.icon], 6, 18, reminder.accentColor, 2);
  drawMixedLabel(pixels, reminder.reminderName.slice(0, 8), 28, 20, reminder.accentColor, "left");
  drawMixedLabel(pixels, reminder.text.slice(0, 8), 28, 32, "#ffffff", "left");
  drawAccentBar(pixels, 24, reminder.accentColor);
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
    drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 48, "#ffffff", "center");
    drawAccentBar(pixels, 14 + radius * 7, reminder.accentColor);
    frames.push({ pixels, delay: 110 });
  });

  return frames;
}

function buildFireworksFrames(reminder) {
  const frames = [];
  const sparkSets = [
    [[10, 18], [54, 18], [18, 50], [46, 50]],
    [[14, 14], [50, 14], [10, 46], [54, 46], [32, 12]],
    [[8, 22], [56, 22], [18, 54], [46, 54], [32, 16], [32, 52]],
    [[14, 18], [50, 18], [18, 46], [46, 46], [22, 12], [42, 12]],
  ];

  sparkSets.forEach((points, index) => {
    const pixels = createFrame();
    decorateReminderFrame(pixels, reminder);
    drawPattern(pixels, ICON_PATTERNS[reminder.icon], 24, 18, reminder.accentColor, 2);
    points.forEach((point, pointIndex) => {
      addSparkle(pixels, point[0], point[1], pointIndex % 2 === 0 ? "#ffffff" : reminder.accentColor);
    });
    drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 48, "#ffffff", "center");
    drawAccentBar(pixels, 18 + index * 8, reminder.accentColor);
    frames.push({ pixels, delay: 100 });
  });

  return frames;
}

function buildOrbitFrames(reminder) {
  const frames = [];
  const orbitPoints = [
    [[32, 14], [50, 32], [32, 50], [14, 32]],
    [[40, 16], [48, 40], [24, 48], [16, 24]],
    [[50, 24], [40, 50], [14, 40], [24, 14]],
    [[48, 32], [32, 48], [16, 32], [32, 16]],
  ];

  orbitPoints.forEach((points, index) => {
    const pixels = createFrame();
    decorateReminderFrame(pixels, reminder);
    drawPattern(pixels, ICON_PATTERNS[reminder.icon], 24, 18, reminder.accentColor, 2);
    points.forEach((point, pointIndex) => {
      fillRect(pixels, point[0], point[1], 2, 2, pointIndex % 2 === 0 ? reminder.accentColor : "#ffffff");
    });
    drawMixedLabel(pixels, reminder.text.slice(0, 8), 32, 48, "#ffffff", "center");
    drawAccentBar(pixels, 20 + index * 6, reminder.accentColor);
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
