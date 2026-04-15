function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function normalizeSpeed(value) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) {
    return 6;
  }
  return clamp(Math.round(parsed), 1, 10);
}

function normalizeIntensity(value) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) {
    return 72;
  }
  return clamp(Math.round(parsed), 10, 100);
}

function rgbText(r, g, b) {
  return `rgb(${clamp(Math.round(r), 0, 255)}, ${clamp(Math.round(g), 0, 255)}, ${clamp(Math.round(b), 0, 255)})`;
}

function setPixel(map, x, y, r, g, b) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }
  map.set(`${x},${y}`, rgbText(r, g, b));
}

function fillRect(map, x, y, width, height, r, g, b) {
  for (let row = 0; row < height; row += 1) {
    for (let col = 0; col < width; col += 1) {
      setPixel(map, x + col, y + row, r, g, b);
    }
  }
}

function parseColorText(value) {
  const match = /rgb\((\d+),\s*(\d+),\s*(\d+)\)/i.exec(value || "");
  if (!match) {
    return { r: 0, g: 0, b: 0 };
  }
  return {
    r: clamp(Number(match[1]), 0, 255),
    g: clamp(Number(match[2]), 0, 255),
    b: clamp(Number(match[3]), 0, 255),
  };
}

function blendPixel(map, x, y, r, g, b, alpha) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }
  const safeAlpha = clamp(alpha, 0, 1);
  const key = `${x},${y}`;
  const prev = parseColorText(map.get(key));
  setPixel(
    map,
    x,
    y,
    prev.r * (1 - safeAlpha) + r * safeAlpha,
    prev.g * (1 - safeAlpha) + g * safeAlpha,
    prev.b * (1 - safeAlpha) + b * safeAlpha,
  );
}

function drawCircle(map, cx, cy, radius, r, g, b, alpha = 1) {
  for (let step = 0; step < 360; step += 2) {
    const angle = (step * Math.PI) / 180;
    const x = cx + Math.cos(angle) * radius;
    const y = cy + Math.sin(angle) * radius;
    blendPixel(map, Math.round(x), Math.round(y), r, g, b, alpha);
  }
}

const DIGIT_FONT_3X5 = {
  0: ["111", "101", "101", "101", "111"],
  1: ["010", "110", "010", "010", "111"],
  2: ["111", "001", "111", "100", "111"],
  3: ["111", "001", "111", "001", "111"],
  4: ["101", "101", "111", "001", "001"],
  5: ["111", "100", "111", "001", "111"],
  6: ["111", "100", "111", "101", "111"],
  7: ["111", "001", "001", "001", "001"],
  8: ["111", "101", "111", "101", "111"],
  9: ["111", "101", "111", "001", "111"],
};

function drawDigit3x5(map, digit, x, y, r, g, b) {
  const rows = DIGIT_FONT_3X5[digit];
  if (!rows) {
    return;
  }
  for (let row = 0; row < rows.length; row += 1) {
    const pattern = rows[row];
    for (let col = 0; col < pattern.length; col += 1) {
      if (pattern[col] === "1") {
        setPixel(map, x + col, y + row, r, g, b);
      }
    }
  }
}

export function buildWeatherBoardDemoMaps(speed, intensity) {
  const frameCount = 72;
  const maps = [];
  const safeSpeed = normalizeSpeed(speed);
  const safeIntensity = normalizeIntensity(intensity) / 100;

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const map = new Map();
    const time = frameIndex * (0.14 + safeSpeed * 0.025);

    for (let y = 0; y < 64; y += 1) {
      const skyMix = y / 63;
      const skyR = Math.round(10 + 26 * (1 - skyMix));
      const skyG = Math.round(32 + 62 * (1 - skyMix));
      const skyB = Math.round(66 + 130 * (1 - skyMix));
      for (let x = 0; x < 64; x += 1) {
        const wobble = Math.sin((x * 0.18) + time * 0.55) * (4 + safeIntensity * 5);
        setPixel(map, x, y, skyR + wobble, skyG + wobble * 0.5, skyB + wobble * 0.2);
      }
    }

    const sunX = 11 + Math.round(Math.sin(time * 0.5) * 2);
    const sunY = 10 + Math.round(Math.cos(time * 0.36) * 1);
    for (let radius = 4; radius >= 1; radius -= 1) {
      drawCircle(map, sunX, sunY, radius, 255, 210 - radius * 14, 88, 0.78);
    }
    fillRect(map, sunX - 1, sunY - 1, 3, 3, 255, 214, 126);

    const cloudBaseX = 20 + Math.round(Math.sin(time * 0.72) * 6);
    const cloudBaseY = 15 + Math.round(Math.cos(time * 0.34) * 2);
    fillRect(map, cloudBaseX, cloudBaseY, 14, 6, 214, 228, 244);
    fillRect(map, cloudBaseX - 4, cloudBaseY + 1, 9, 5, 226, 236, 248);
    fillRect(map, cloudBaseX + 10, cloudBaseY + 2, 8, 4, 206, 222, 240);

    for (let drop = 0; drop < 18; drop += 1) {
      const dropX = 18 + ((drop * 5 + frameIndex * 2) % 26);
      const dropY = 25 + ((drop * 11 + frameIndex * 4) % 32);
      blendPixel(map, dropX, dropY, 98, 196, 255, 0.8);
      blendPixel(map, dropX, dropY + 1, 56, 124, 210, 0.52);
    }

    fillRect(map, 4, 45, 56, 16, 6, 14, 26);
    fillRect(map, 5, 46, 54, 14, 18, 34, 56);
    fillRect(map, 8, 48, 24, 10, 26, 62, 96);
    fillRect(map, 34, 48, 22, 10, 18, 48, 88);

    const temp = 24 + Math.round(Math.sin(time * 0.45) * 4);
    const tens = Math.floor(temp / 10);
    const ones = temp % 10;
    drawDigit3x5(map, tens, 10, 50, 255, 248, 220);
    drawDigit3x5(map, ones, 14, 50, 255, 248, 220);
    fillRect(map, 18, 50, 1, 1, 255, 248, 220);
    setPixel(map, 20, 51, 255, 248, 220);
    setPixel(map, 21, 50, 255, 248, 220);
    setPixel(map, 21, 52, 255, 248, 220);
    setPixel(map, 22, 53, 255, 248, 220);
    setPixel(map, 23, 50, 255, 248, 220);
    setPixel(map, 23, 52, 255, 248, 220);

    maps.push(map);
  }

  return maps;
}
