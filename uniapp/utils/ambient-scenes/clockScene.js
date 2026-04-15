import { blendPixel, fillRect, setPixel } from "../led-matrix-demos/common.js";

const DIGITS = [
  ["111", "101", "101", "101", "111"],
  ["010", "110", "010", "010", "111"],
  ["111", "001", "111", "100", "111"],
  ["111", "001", "111", "001", "111"],
  ["101", "101", "111", "001", "001"],
  ["111", "100", "111", "001", "111"],
  ["111", "100", "111", "101", "111"],
  ["111", "001", "001", "001", "001"],
  ["111", "101", "111", "101", "111"],
  ["111", "101", "111", "001", "111"],
];

function drawLine(map, x0, y0, x1, y1, r, g, b) {
  let startX = Math.round(x0);
  let startY = Math.round(y0);
  const endX = Math.round(x1);
  const endY = Math.round(y1);
  const deltaX = Math.abs(endX - startX);
  const stepX = startX < endX ? 1 : -1;
  const deltaY = -Math.abs(endY - startY);
  const stepY = startY < endY ? 1 : -1;
  let error = deltaX + deltaY;

  while (true) {
    setPixel(map, startX, startY, r, g, b);
    if (startX === endX && startY === endY) {
      break;
    }
    const twice = error * 2;
    if (twice >= deltaY) {
      error += deltaY;
      startX += stepX;
    }
    if (twice <= deltaX) {
      error += deltaX;
      startY += stepY;
    }
  }
}

function drawDigit(map, digit, x, y, color) {
  const glyph = DIGITS[digit] || DIGITS[0];
  for (let row = 0; row < glyph.length; row += 1) {
    for (let col = 0; col < glyph[row].length; col += 1) {
      if (glyph[row][col] !== "1") {
        continue;
      }
      setPixel(map, x + col, y + row, color.r, color.g, color.b);
    }
  }
}

function drawDigitalClock(map, hour24, minute, hourColor, minuteColor) {
  const isPm = hour24 >= 12;
  let hour = hour24 % 12;
  if (hour === 0) {
    hour = 12;
  }
  const xStart = 20;
  const y = 48;

  drawDigit(map, Math.floor(hour / 10), xStart, y, hourColor);
  drawDigit(map, hour % 10, xStart + 5, y, hourColor);
  setPixel(map, xStart + 9, y + 1, 255, 255, 255);
  setPixel(map, xStart + 9, y + 3, 255, 255, 255);
  drawDigit(map, Math.floor(minute / 10), xStart + 11, y, minuteColor);
  drawDigit(map, minute % 10, xStart + 16, y, minuteColor);

  if (isPm) {
    setPixel(map, xStart + 22, y, 255, 255, 255);
    setPixel(map, xStart + 22, y + 1, 255, 255, 255);
    setPixel(map, xStart + 23, y, 255, 255, 255);
    setPixel(map, xStart + 23, y + 1, 255, 255, 255);
    return;
  }

  setPixel(map, xStart + 22, y + 3, 255, 255, 255);
  setPixel(map, xStart + 22, y + 4, 255, 255, 255);
  setPixel(map, xStart + 23, y + 3, 255, 255, 255);
  setPixel(map, xStart + 23, y + 4, 255, 255, 255);
}

function drawClockFace(map, centerX, centerY, radius) {
  for (let index = 0; index < 12; index += 1) {
    const angle = (index * Math.PI) / 6;
    const x = centerX + Math.round(Math.sin(angle) * radius);
    const y = centerY - Math.round(Math.cos(angle) * radius);
    if (index % 3 === 0) {
      setPixel(map, x, y, 200, 200, 200);
      setPixel(map, x - 1, y, 150, 150, 150);
      setPixel(map, x + 1, y, 150, 150, 150);
      setPixel(map, x, y - 1, 150, 150, 150);
      setPixel(map, x, y + 1, 150, 150, 150);
    } else {
      setPixel(map, x, y, 100, 100, 100);
    }
  }
}

function buildClockSceneAmbientFrame(frameMap, elapsed) {
  fillRect(frameMap, 0, 0, 64, 64, 0, 0, 0);

  const now = new Date();
  const second = (now.getSeconds() + Math.floor(elapsed / 120)) % 60;
  const minute = now.getMinutes() + second / 60;
  const hour = (now.getHours() % 12) + minute / 60;

  const centerX = 32;
  const centerY = 21;
  const radius = 18;
  drawClockFace(frameMap, centerX, centerY, radius);

  const hourAngle = (hour / 12) * Math.PI * 2 - Math.PI / 2;
  const minuteAngle = (minute / 60) * Math.PI * 2 - Math.PI / 2;
  const secondAngle = (second / 60) * Math.PI * 2 - Math.PI / 2;

  drawLine(
    frameMap,
    centerX,
    centerY,
    centerX + Math.cos(hourAngle) * 9,
    centerY + Math.sin(hourAngle) * 9,
    84,
    196,
    255,
  );
  drawLine(
    frameMap,
    centerX,
    centerY,
    centerX + Math.cos(minuteAngle) * 13,
    centerY + Math.sin(minuteAngle) * 13,
    220,
    220,
    220,
  );
  drawLine(
    frameMap,
    centerX,
    centerY,
    centerX + Math.cos(secondAngle) * 15,
    centerY + Math.sin(secondAngle) * 15,
    255,
    100,
    130,
  );

  blendPixel(frameMap, centerX, centerY, 255, 255, 255, 1);
  fillRect(frameMap, centerX - 1, centerY - 1, 3, 3, 255, 255, 255);

  const hour24 = now.getHours();
  drawDigitalClock(
    frameMap,
    hour24,
    now.getMinutes(),
    { r: 84, g: 196, b: 255 },
    { r: 220, g: 220, b: 220 },
  );
}

export { buildClockSceneAmbientFrame };
