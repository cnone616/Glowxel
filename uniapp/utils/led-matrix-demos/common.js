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

function drawGlow(map, x, y, radius, r, g, b, alpha) {
  const reach = Math.ceil(radius * 2);
  for (let dy = -reach; dy <= reach; dy += 1) {
    for (let dx = -reach; dx <= reach; dx += 1) {
      const distance = Math.sqrt(dx * dx + dy * dy);
      if (distance > radius * 1.8) {
        continue;
      }
      const intensity = clamp(1 - distance / (radius * 1.8 || 1), 0, 1) * alpha;
      if (intensity <= 0) {
        continue;
      }
      blendPixel(map, Math.round(x + dx), Math.round(y + dy), r, g, b, intensity);
    }
  }
}

export {
  blendPixel,
  clamp,
  drawGlow,
  fillRect,
  normalizeIntensity,
  normalizeSpeed,
  setPixel,
};
