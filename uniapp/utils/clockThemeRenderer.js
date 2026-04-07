import {
  drawClockTextToPixels,
  drawTinyTextToPixels,
  getClockTextWidth,
} from "./clockCanvas.js";
import { getMaterialPreviewPixels } from "./materialPreviewLibrary.js";

function setPixel(pixelMap, x, y, color) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }
  pixelMap.set(`${x},${y}`, color);
}

function fillRect(pixelMap, x, y, width, height, color) {
  for (let py = y; py < y + height; py++) {
    for (let px = x; px < x + width; px++) {
      setPixel(pixelMap, px, py, color);
    }
  }
}

function drawRect(pixelMap, x, y, width, height, color) {
  for (let px = x; px < x + width; px++) {
    setPixel(pixelMap, px, y, color);
    setPixel(pixelMap, px, y + height - 1, color);
  }
  for (let py = y; py < y + height; py++) {
    setPixel(pixelMap, x, py, color);
    setPixel(pixelMap, x + width - 1, py, color);
  }
}

function drawLine(pixelMap, x0, y0, x1, y1, color) {
  let dx = Math.abs(x1 - x0);
  let sx = x0 < x1 ? 1 : -1;
  let dy = -Math.abs(y1 - y0);
  let sy = y0 < y1 ? 1 : -1;
  let error = dx + dy;

  while (true) {
    setPixel(pixelMap, x0, y0, color);
    if (x0 === x1 && y0 === y1) {
      break;
    }
    const doubleError = error * 2;
    if (doubleError >= dy) {
      error += dy;
      x0 += sx;
    }
    if (doubleError <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

function drawCircle(pixelMap, cx, cy, radius, color) {
  let x = radius;
  let y = 0;
  let error = 0;

  while (x >= y) {
    setPixel(pixelMap, cx + x, cy + y, color);
    setPixel(pixelMap, cx + y, cy + x, color);
    setPixel(pixelMap, cx - y, cy + x, color);
    setPixel(pixelMap, cx - x, cy + y, color);
    setPixel(pixelMap, cx - x, cy - y, color);
    setPixel(pixelMap, cx - y, cy - x, color);
    setPixel(pixelMap, cx + y, cy - x, color);
    setPixel(pixelMap, cx + x, cy - y, color);

    if (error <= 0) {
      y += 1;
      error += 2 * y + 1;
    }
    if (error > 0) {
      x -= 1;
      error -= 2 * x + 1;
    }
  }
}

function drawFilledCircle(pixelMap, cx, cy, radius, color) {
  for (let y = -radius; y <= radius; y++) {
    for (let x = -radius; x <= radius; x++) {
      if (x * x + y * y <= radius * radius) {
        setPixel(pixelMap, cx + x, cy + y, color);
      }
    }
  }
}

function mergePixels(targetMap, sourceMap) {
  if (!sourceMap) {
    return;
  }
  sourceMap.forEach((color, key) => {
    targetMap.set(key, color);
  });
}

function applySparkleOverlay(pixelMap, tick, colorList, sparklePoints) {
  for (let i = 0; i < sparklePoints.length; i++) {
    const point = sparklePoints[i];
    const frame = Math.floor(tick / 250) + i;
    const color = colorList[frame % colorList.length];
    setPixel(pixelMap, point[0], point[1], color);
    if (frame % 2 === 0) {
      setPixel(pixelMap, point[0] - 1, point[1], color);
      setPixel(pixelMap, point[0] + 1, point[1], color);
      setPixel(pixelMap, point[0], point[1] - 1, color);
      setPixel(pixelMap, point[0], point[1] + 1, color);
    }
  }
}

function drawClockBlock(pixelMap, x, y, text) {
  fillRect(pixelMap, x, y, 18, 12, "#f7ca4a");
  drawRect(pixelMap, x, y, 18, 12, "#633100");
  setPixel(pixelMap, x + 3, y + 3, "#633100");
  setPixel(pixelMap, x + 14, y + 3, "#633100");
  setPixel(pixelMap, x + 3, y + 8, "#633100");
  setPixel(pixelMap, x + 14, y + 8, "#633100");
  drawClockTextToPixels(text, x + 4, y + 2, "#1a1100", pixelMap, "classic_5x7");
}

function drawCloud(pixelMap, x, y) {
  const color = "#ffffff";
  drawFilledCircle(pixelMap, x + 3, y + 5, 4, color);
  drawFilledCircle(pixelMap, x + 8, y + 4, 5, color);
  drawFilledCircle(pixelMap, x + 13, y + 5, 4, color);
  fillRect(pixelMap, x + 2, y + 5, 12, 4, color);
}

function drawHill(pixelMap, x, y) {
  const dark = "#0c9a2d";
  const mid = "#26c146";
  fillRect(pixelMap, x + 6, y + 12, 16, 10, mid);
  drawFilledCircle(pixelMap, x + 8, y + 14, 10, dark);
  drawFilledCircle(pixelMap, x + 20, y + 12, 12, mid);
  drawFilledCircle(pixelMap, x + 14, y + 10, 9, "#32d45a");
}

function drawBush(pixelMap, x, y) {
  drawFilledCircle(pixelMap, x + 3, y + 5, 4, "#1eb13b");
  drawFilledCircle(pixelMap, x + 8, y + 4, 5, "#2ecc4b");
  drawFilledCircle(pixelMap, x + 13, y + 5, 4, "#1eb13b");
  fillRect(pixelMap, x + 1, y + 6, 14, 5, "#16912f");
}

function drawGround(pixelMap) {
  const top = "#f3d6a8";
  const dirt = "#9f5d2d";
  const dark = "#6f3915";
  const grass = "#34be45";

  fillRect(pixelMap, 0, 53, 64, 11, dirt);
  fillRect(pixelMap, 0, 51, 64, 2, grass);
  for (let x = 0; x < 64; x += 4) {
    fillRect(pixelMap, x, 53, 2, 2, top);
    fillRect(pixelMap, x + 2, 56, 2, 2, dark);
    fillRect(pixelMap, x + ((x / 4) % 2), 59, 2, 2, top);
  }
}

function drawMario(pixelMap, x, y) {
  const sprite = [
    "....RRR.....",
    "...RRSRR....",
    "...SSSSS....",
    "..SSKSK.....",
    "..SSSSSS....",
    "..RBBR......",
    ".RBBBBR.....",
    ".RRBBRR.....",
    "..BB..BB....",
    ".BB....BB...",
    ".WW....WW...",
  ];
  const colorMap = {
    R: "#d94a3a",
    S: "#f5c39b",
    K: "#24140e",
    B: "#2567e8",
    W: "#f7d85d",
    ".": null,
  };

  for (let row = 0; row < sprite.length; row++) {
    for (let col = 0; col < sprite[row].length; col++) {
      const color = colorMap[sprite[row][col]];
      if (color) {
        setPixel(pixelMap, x + col, y + row, color);
      }
    }
  }
}

function wordsForHour(hour) {
  const words = [
    "twelve",
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "ten",
    "eleven",
  ];
  return words[hour % 12];
}

function wordsForMinute(minute) {
  const special = {
    0: ["oclock"],
    1: ["one"],
    2: ["two"],
    3: ["three"],
    4: ["four"],
    5: ["five"],
    6: ["six"],
    7: ["seven"],
    8: ["eight"],
    9: ["nine"],
    10: ["ten"],
    11: ["eleven"],
    12: ["twelve"],
    13: ["thirteen"],
    14: ["fourteen"],
    15: ["fifteen"],
    16: ["sixteen"],
    17: ["seventeen"],
    18: ["eighteen"],
    19: ["nineteen"],
  };

  if (special[minute]) {
    return special[minute];
  }

  const tensMap = {
    20: "twenty",
    30: "thirty",
    40: "forty",
    50: "fifty",
  };
  const tens = Math.floor(minute / 10) * 10;
  const ones = minute % 10;
  if (ones === 0) {
    return [tensMap[tens]];
  }

  return [tensMap[tens], wordsForMinute(ones)[0]];
}

function formatShortDate(dateText) {
  const parts = dateText.split("-");
  if (parts.length !== 2) {
    return dateText;
  }
  return `${Number(parts[0])}/${Number(parts[1])}`;
}

function splitTimeParts(timeText) {
  const parts = timeText.split(":");
  return {
    hour: parts[0] || "00",
    minute: parts[1] || "00",
  };
}

function getNowParts(timeText) {
  const parts = splitTimeParts(timeText);
  return {
    hour: Number(parts.hour),
    minute: Number(parts.minute),
  };
}

function renderMarioTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 0, 0, 64, 64, "#2d78ff");
  drawCloud(pixels, 0, 16);
  drawCloud(pixels, 49, 8);
  drawHill(pixels, 0, 29);
  drawBush(pixels, 46, 44);
  drawGround(pixels);
  drawMario(pixels, 24, 38);

  const { hour, minute } = splitTimeParts(context.timeText);
  drawClockBlock(pixels, 18, 10, hour);
  drawClockBlock(pixels, 36, 10, minute);

  return pixels;
}

function renderWordsTheme(context) {
  const pixels = new Map();
  const nowParts = getNowParts(context.timeText);
  const minuteLines = wordsForMinute(nowParts.minute);
  drawClockTextToPixels(
    wordsForHour(nowParts.hour),
    2,
    4,
    "#62f4c8",
    pixels,
    "minimal_3x5",
    2,
  );
  drawClockTextToPixels(
    minuteLines[0],
    2,
    22,
    "#ffffff",
    pixels,
    "minimal_3x5",
    2,
  );
  if (minuteLines[1]) {
    drawClockTextToPixels(
      minuteLines[1],
      2,
      34,
      "#ffffff",
      pixels,
      "minimal_3x5",
      2,
    );
  }
  fillRect(pixels, 0, 48, 64, 1, "#ffffff");
  drawClockTextToPixels(
    formatShortDate(context.dateText),
    2,
    52,
    "#62f4c8",
    pixels,
    "minimal_3x5",
    2,
  );
  drawTinyTextToPixels(context.weekText, 42, 54, "#ffffff", pixels);
  setPixel(pixels, 2, 61, "#ffffff");
  setPixel(pixels, 3, 60, "#ffffff");
  setPixel(pixels, 4, 59, "#ffffff");
  setPixel(pixels, 5, 60, "#ffffff");
  setPixel(pixels, 6, 61, "#ffffff");
  fillRect(pixels, 3, 62, 3, 1, "#ffffff");
  return pixels;
}

function renderWorldMapTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 31, 0, 2, 64, "#ff6548");
  drawClockTextToPixels(
    context.timeText,
    2,
    55,
    "#ffffff",
    pixels,
    "classic_5x7",
  );
  return pixels;
}

function renderCastlevaniaTheme(context) {
  const pixels = new Map();
  const centerX = 32;
  const centerY = 28;
  drawCircle(pixels, centerX, centerY, 17, "#1bf3db");
  drawCircle(pixels, centerX, centerY, 18, "#0ea894");
  drawRect(pixels, 6, 2, 6, 60, "#147d73");
  drawRect(pixels, 52, 2, 6, 60, "#147d73");
  fillRect(pixels, 8, 8, 2, 48, "#0ea894");
  fillRect(pixels, 54, 8, 2, 48, "#0ea894");
  fillRect(pixels, 16, 51, 32, 3, "#1c756d");

  for (let hour = 0; hour < 12; hour++) {
    const angle = (Math.PI * 2 * hour) / 12 - Math.PI / 2;
    const x = Math.round(centerX + Math.cos(angle) * 14);
    const y = Math.round(centerY + Math.sin(angle) * 14);
    setPixel(pixels, x, y, "#7ffff0");
  }

  const nowParts = getNowParts(context.timeText);
  const minuteAngle = (Math.PI * 2 * nowParts.minute) / 60 - Math.PI / 2;
  const hourAngle =
    (Math.PI * 2 * ((nowParts.hour % 12) + nowParts.minute / 60)) / 12 -
    Math.PI / 2;

  drawLine(
    pixels,
    centerX,
    centerY,
    Math.round(centerX + Math.cos(minuteAngle) * 11),
    Math.round(centerY + Math.sin(minuteAngle) * 11),
    "#dffdf9",
  );
  drawLine(
    pixels,
    centerX,
    centerY,
    Math.round(centerX + Math.cos(hourAngle) * 8),
    Math.round(centerY + Math.sin(hourAngle) * 8),
    "#75fff4",
  );
  drawFilledCircle(pixels, centerX, centerY, 1, "#dffdf9");
  return pixels;
}

function drawPacmanMaze(pixelMap) {
  const wall = "#3b83ff";
  drawRect(pixelMap, 2, 2, 60, 60, wall);
  drawRect(pixelMap, 8, 8, 48, 48, wall);
  drawRect(pixelMap, 15, 18, 34, 26, wall);
  drawLine(pixelMap, 8, 24, 14, 24, wall);
  drawLine(pixelMap, 50, 24, 56, 24, wall);
  drawLine(pixelMap, 8, 39, 14, 39, wall);
  drawLine(pixelMap, 50, 39, 56, 39, wall);
  drawLine(pixelMap, 24, 8, 24, 17, wall);
  drawLine(pixelMap, 40, 8, 40, 17, wall);
  drawLine(pixelMap, 24, 45, 24, 55, wall);
  drawLine(pixelMap, 40, 45, 40, 55, wall);

  for (let x = 6; x <= 58; x += 6) {
    setPixel(pixelMap, x, 5, "#ffe97a");
    setPixel(pixelMap, x, 59, "#ffe97a");
  }
  for (let y = 10; y <= 54; y += 6) {
    setPixel(pixelMap, 5, y, "#ffe97a");
    setPixel(pixelMap, 59, y, "#ffe97a");
  }
}

function renderPacmanTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 0, 0, 64, 64, "#000000");
  drawPacmanMaze(pixels);
  fillRect(pixels, 16, 20, 32, 22, "#091218");

  const { hour, minute } = splitTimeParts(context.timeText);
  drawClockTextToPixels(hour, 17, 23, "#fffdf0", pixels, "lcd_6x8");
  drawClockTextToPixels(minute, 34, 23, "#fffdf0", pixels, "lcd_6x8");
  fillRect(pixels, 31, 24, 2, 2, "#ffb240");
  fillRect(pixels, 31, 29, 2, 2, "#ffb240");

  drawTinyTextToPixels(
    `${formatShortDate(context.dateText)} ${context.weekText}`,
    17,
    35,
    "#7fc7ff",
    pixels,
  );

  drawFilledCircle(pixels, 51, 6, 2, "#ffe45f");
  return pixels;
}

function renderPokedexTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 0, 0, 31, 64, "#ef392c");
  fillRect(pixels, 31, 0, 33, 64, "#11111a");
  fillRect(pixels, 33, 0, 31, 64, "#2c2c36");
  fillRect(pixels, 35, 6, 25, 18, "#402858");
  fillRect(pixels, 5, 13, 21, 23, "#d9e2d8");
  drawRect(pixels, 4, 12, 23, 25, "#151515");
  drawRect(pixels, 34, 0, 30, 24, "#1f1f2c");
  drawRect(pixels, 2, 2, 8, 8, "#0d0d15");
  drawFilledCircle(pixels, 6, 6, 2, "#67dbff");

  drawClockTextToPixels(
    context.timeText.slice(0, 2),
    38,
    8,
    "#ffffff",
    pixels,
    "rounded_4x6",
    2,
  );
  drawClockTextToPixels(
    context.timeText.slice(3),
    42,
    21,
    "#ffffff",
    pixels,
    "rounded_4x6",
    2,
  );

  drawFilledCircle(pixels, 13, 24, 6, "#7ad6b3");
  setPixel(pixels, 11, 22, "#222222");
  setPixel(pixels, 15, 22, "#222222");
  drawLine(pixels, 11, 28, 15, 28, "#222222");

  for (let row = 0; row < 2; row++) {
    for (let col = 0; col < 4; col++) {
      fillRect(pixels, 37 + col * 6, 33 + row * 6, 4, 4, "#39bfff");
    }
  }

  fillRect(pixels, 8, 49, 10, 5, "#69dd5d");
  fillRect(pixels, 21, 47, 3, 11, "#111111");
  fillRect(pixels, 17, 51, 11, 3, "#111111");
  fillRect(pixels, 35, 49, 10, 2, "#ffffff");
  fillRect(pixels, 35, 55, 22, 2, "#ef392c");
  return pixels;
}

function renderMinimalDigitalTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 0, 0, 64, 64, "#07110e");
  for (let x = 5; x <= 59; x += 9) {
    fillRect(pixels, x, 50, 4, 1, "#15362e");
  }
  drawClockTextToPixels(
    context.timeText,
    32,
    16,
    "#7bf2d0",
    pixels,
    "lcd_6x8",
    1,
    "center",
  );
  drawTinyTextToPixels(
    formatShortDate(context.dateText),
    32,
    37,
    "#d2fff3",
    pixels,
    1,
    "center",
  );
  drawTinyTextToPixels(context.weekText, 32, 46, "#4cc7a5", pixels, 1, "center");
  fillRect(pixels, 18, 56, 28, 1, "#1e5b4a");
  return pixels;
}

function renderFrameClockTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 0, 0, 64, 64, "#1b1208");
  drawRect(pixels, 2, 2, 60, 60, "#ffcf5a");
  drawRect(pixels, 6, 6, 52, 52, "#7b4b14");
  fillRect(pixels, 2, 2, 4, 4, "#fff3b4");
  fillRect(pixels, 58, 2, 4, 4, "#fff3b4");
  fillRect(pixels, 2, 58, 4, 4, "#fff3b4");
  fillRect(pixels, 58, 58, 4, 4, "#fff3b4");
  drawClockTextToPixels(
    context.timeText,
    32,
    18,
    "#ffcf5a",
    pixels,
    "classic_5x7",
    1,
    "center",
  );
  drawTinyTextToPixels(
    formatShortDate(context.dateText),
    32,
    35,
    "#fff5cb",
    pixels,
    1,
    "center",
  );
  drawTinyTextToPixels(context.weekText, 32, 44, "#ff9f43", pixels, 1, "center");
  return pixels;
}

function renderCenterpieceBadgeTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 0, 0, 64, 64, "#180d12");
  drawFilledCircle(pixels, 27, 33, 15, "#ff5678");
  drawFilledCircle(pixels, 27, 33, 10, "#ff8fa5");
  drawRect(pixels, 12, 18, 30, 30, "#ffd15f");
  drawLine(pixels, 17, 23, 37, 43, "#fff1d2");
  drawLine(pixels, 37, 23, 17, 43, "#fff1d2");
  drawClockTextToPixels(
    context.timeText,
    60,
    6,
    "#ffe0e7",
    pixels,
    "rounded_4x6",
    1,
    "right",
  );
  drawTinyTextToPixels(
    formatShortDate(context.dateText),
    60,
    15,
    "#ff9daf",
    pixels,
    1,
    "right",
  );
  drawTinyTextToPixels(context.weekText, 60, 23, "#ffd15f", pixels, 1, "right");
  return pixels;
}

function renderTopbarMotionTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 0, 0, 64, 64, "#061019");
  fillRect(pixels, 0, 0, 64, 14, "#10283a");
  drawClockTextToPixels(
    context.timeText,
    4,
    4,
    "#5ecbff",
    pixels,
    "minimal_3x5",
    2,
    "left",
  );
  for (let x = 0; x < 64; x += 4) {
    const waveHeight = 28 + ((x / 4) % 4) * 3;
    fillRect(pixels, x, waveHeight, 2, 20, "#1a4260");
    fillRect(pixels, x + 2, waveHeight + 4, 2, 16, "#2e86b7");
  }
  fillRect(pixels, 0, 42, 64, 1, "#5ecbff");
  drawTinyTextToPixels(
    formatShortDate(context.dateText),
    4,
    46,
    "#d8f5ff",
    pixels,
  );
  drawTinyTextToPixels(context.weekText, 60, 46, "#7ff0ff", pixels, 1, "right");
  drawFilledCircle(pixels, 56, 18, 2, "#7ff0ff");
  drawFilledCircle(pixels, 49, 24, 1, "#5ecbff");
  return pixels;
}

function renderFestivalPosterTheme(context) {
  const pixels = new Map();
  const tick = Date.now();
  fillRect(pixels, 0, 0, 64, 64, "#2a0909");
  drawFilledCircle(pixels, 14, 12, 6, "#ff9b3d");
  drawFilledCircle(pixels, 50, 14, 5, "#ffcf5a");
  drawLine(pixels, 14, 0, 14, 6, "#ffcf5a");
  drawLine(pixels, 50, 0, 50, 8, "#ffcf5a");
  fillRect(pixels, 8, 48, 48, 8, "#5c1010");
  fillRect(pixels, 12, 50, 40, 4, "#7a1515");
  drawClockTextToPixels(
    context.timeText,
    32,
    17,
    "#fff0d6",
    pixels,
    "retro_5x7",
    1,
    "center",
  );
  drawTinyTextToPixels(
    formatShortDate(context.dateText),
    32,
    35,
    "#ffd059",
    pixels,
    1,
    "center",
  );
  drawTinyTextToPixels(context.weekText, 32, 44, "#ff9e63", pixels, 1, "center");
  applySparkleOverlay(
    pixels,
    tick,
    ["#ffcf5a", "#fff7cf", "#ff9b3d"],
    [
      [8, 8],
      [24, 10],
      [41, 7],
      [56, 11],
    ],
  );
  return pixels;
}

function renderPixelArcadeTheme(context) {
  const pixels = new Map();
  fillRect(pixels, 0, 0, 64, 64, "#09110a");
  drawRect(pixels, 3, 3, 58, 58, "#365d28");
  fillRect(pixels, 6, 6, 52, 52, "#101b12");
  drawClockTextToPixels(
    context.timeText,
    32,
    17,
    "#f3fff0",
    pixels,
    "seven_seg_5x7",
    1,
    "center",
  );
  fillRect(pixels, 11, 14, 3, 3, "#ff6464");
  fillRect(pixels, 16, 14, 3, 3, "#ff6464");
  fillRect(pixels, 21, 14, 3, 3, "#ff6464");
  drawTinyTextToPixels(
    formatShortDate(context.dateText),
    6,
    40,
    "#8dff6a",
    pixels,
  );
  drawTinyTextToPixels(context.weekText, 58, 40, "#5ed2ff", pixels, 1, "right");
  drawRect(pixels, 8, 48, 48, 7, "#243f19");
  for (let i = 0; i < 6; i++) {
    fillRect(pixels, 11 + i * 7, 50 - (i % 3), 4, 3 + (i % 3), "#8dff6a");
  }
  return pixels;
}

function renderFireworksBloomTheme(context) {
  const pixels = new Map();
  const tick = Date.now();
  fillRect(pixels, 0, 0, 64, 64, "#10070b");
  fillRect(pixels, 0, 46, 64, 18, "#210d0d");
  drawLine(pixels, 0, 46, 63, 46, "#5a1c1c");
  const frameIndex = Math.floor(tick / 220) % 7;
  mergePixels(pixels, getMaterialPreviewPixels("fireworks_bloom", frameIndex));
  drawClockTextToPixels(
    context.timeText,
    32,
    10,
    "#fff6d2",
    pixels,
    "retro_5x7",
    1,
    "center",
  );
  drawTinyTextToPixels(
    formatShortDate(context.dateText),
    32,
    49,
    "#ffcf6a",
    pixels,
    1,
    "center",
  );
  drawTinyTextToPixels(context.weekText, 32, 57, "#ff8f66", pixels, 1, "center");
  applySparkleOverlay(
    pixels,
    tick,
    ["#ffb347", "#ffd95f", "#fff7cf"],
    [
      [6, 14],
      [14, 8],
      [50, 9],
      [57, 16],
    ],
  );
  return pixels;
}

function renderFireworksRiseTheme(context) {
  const pixels = new Map();
  const tick = Date.now();
  fillRect(pixels, 0, 0, 64, 64, "#040915");
  fillRect(pixels, 0, 52, 64, 12, "#08111c");
  for (let x = 0; x < 64; x += 8) {
    fillRect(pixels, x, 54 + ((x / 8) % 2), 4, 8, "#0d2032");
  }
  const frameIndex = Math.floor(tick / 220) % 6;
  mergePixels(pixels, getMaterialPreviewPixels("fireworks_rise", frameIndex));
  drawClockTextToPixels(
    context.timeText,
    60,
    6,
    "#f2fbff",
    pixels,
    "rounded_4x6",
    1,
    "right",
  );
  drawTinyTextToPixels(
    formatShortDate(context.dateText),
    60,
    15,
    "#9cd7ff",
    pixels,
    1,
    "right",
  );
  drawTinyTextToPixels(context.weekText, 60, 23, "#6bc0ff", pixels, 1, "right");
  applySparkleOverlay(
    pixels,
    tick,
    ["#5fb6ff", "#dff4ff", "#89d8ff"],
    [
      [8, 10],
      [18, 6],
      [29, 12],
      [39, 8],
      [52, 12],
    ],
  );
  return pixels;
}

function renderCanvasTheme(context) {
  const pixels = new Map();
  const tick = Date.now();
  fillRect(pixels, 0, 0, 64, 64, "#000000");
  drawRect(pixels, 1, 1, 62, 62, "#214a52");
  drawRect(pixels, 4, 12, 25, 25, "#64c8ff");
  fillRect(pixels, 5, 13, 23, 23, "#000000");
  drawRect(pixels, 33, 12, 27, 25, "#64c8ff");
  fillRect(pixels, 34, 13, 25, 23, "#28343c");
  drawLine(pixels, 33, 39, 60, 39, "#214a52");
  drawLine(pixels, 33, 50, 60, 50, "#214a52");
  fillRect(pixels, 5, 44, 8, 2, "#64c8ff");
  fillRect(pixels, 5, 49, 12, 2, "#8dff6a");
  fillRect(pixels, 5, 54, 16, 2, "#ffffff");

  if (!context.hasImage) {
    fillRect(pixels, 11, 17, 10, 12, "#ffffff");
    drawRect(pixels, 10, 16, 12, 14, "#8dff6a");
    drawLine(pixels, 11, 29, 9, 35, "#214a52");
    drawLine(pixels, 20, 29, 22, 35, "#214a52");
    drawLine(pixels, 8, 35, 23, 35, "#214a52");
    drawLine(pixels, 12, 20, 19, 27, "#64c8ff");
    drawLine(pixels, 19, 20, 12, 27, "#64c8ff");
  }

  drawTinyTextToPixels("CANVAS", 5, 10, "#64c8ff", pixels);
  drawTinyTextToPixels("CW", 52, 10, "#8dff6a", pixels);
  drawTinyTextToPixels("64X64", 5, 61, "#8dff6a", pixels);
  drawClockTextToPixels(
    context.timeText.slice(0, 2),
    36,
    18,
    "#ffffff",
    pixels,
    "lcd_6x8",
  );
  drawClockTextToPixels(
    context.timeText.slice(3, 5),
    44,
    28,
    "#ffffff",
    pixels,
    "rounded_4x6",
  );
  drawTinyTextToPixels(
    formatShortDate(context.dateText).toUpperCase(),
    35,
    47,
    "#8dff6a",
    pixels,
  );
  drawTinyTextToPixels(context.weekText, 35, 57, "#64c8ff", pixels);

  const sparkleX = 43 + (Math.floor(tick / 180) % 14);
  setPixel(pixels, sparkleX, 5, "#ffffff");
  setPixel(pixels, sparkleX - 1, 5, "#64c8ff");
  setPixel(pixels, sparkleX + 1, 5, "#64c8ff");
  const scanY = 15 + (Math.floor(tick / 120) % 33);
  setPixel(pixels, 58, scanY, "#ffffff");
  setPixel(pixels, 58, scanY - 1, "#64c8ff");
  setPixel(pixels, 58, scanY + 1, "#64c8ff");
  return pixels;
}

const THEME_RENDERERS = {
  glowxel_minimal_digital: renderMinimalDigitalTheme,
  glowxel_frame_clock: renderFrameClockTheme,
  glowxel_centerpiece_badge: renderCenterpieceBadgeTheme,
  glowxel_topbar_motion: renderTopbarMotionTheme,
  glowxel_festival_poster: renderFestivalPosterTheme,
  glowxel_pixel_arcade: renderPixelArcadeTheme,
  glowxel_fireworks_bloom: renderFireworksBloomTheme,
  glowxel_fireworks_rise: renderFireworksRiseTheme,
  clockwise_mario: renderMarioTheme,
  clockwise_words: renderWordsTheme,
  clockwise_worldmap: renderWorldMapTheme,
  clockwise_castlevania: renderCastlevaniaTheme,
  clockwise_pacman: renderPacmanTheme,
  clockwise_pokedex: renderPokedexTheme,
  clockwise_canvas: renderCanvasTheme,
};

export function getClockThemePreviewPixels(context) {
  const renderer = THEME_RENDERERS[context.themeId];
  if (!renderer) {
    return null;
  }
  return renderer(context);
}

export function hasClockThemeRenderer(themeId) {
  return !!THEME_RENDERERS[themeId];
}
