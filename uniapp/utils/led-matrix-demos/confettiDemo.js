import { blendPixel, clamp, drawGlow, fillRect, normalizeIntensity, normalizeSpeed } from "./common.js";

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state += 0x6d2b79f5;
    let tmp = Math.imul(state ^ (state >>> 15), state | 1);
    tmp ^= tmp + Math.imul(tmp ^ (tmp >>> 7), tmp | 61);
    return ((tmp ^ (tmp >>> 14)) >>> 0) / 4294967296;
  };
}

export function buildConfettiDemoMaps(speed, intensity) {
  const frameCount = 40;
  const maps = [];
  const safeSpeed = normalizeSpeed(speed);
  const intensityUnit = normalizeIntensity(intensity) / 100;
  const rand = createSeededRandom(20260415);
  const particleCount = 64;
  const palette = [
    [255, 78, 114],
    [255, 188, 56],
    [112, 232, 122],
    [82, 182, 255],
    [170, 118, 255],
    [255, 132, 214],
  ];
  const particles = [];
  for (let index = 0; index < particleCount; index += 1) {
    particles.push({
      x: rand() * 64,
      y: rand() * 64,
      vx: (rand() - 0.5) * (0.9 + safeSpeed * 0.08),
      vy: -rand() * (1.5 + safeSpeed * 0.1),
      life: 0.4 + rand() * 0.6,
      size: rand() > 0.65 ? 2 : 1,
      colorIndex: index % palette.length,
    });
  }

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const map = new Map();
    fillRect(map, 0, 0, 64, 64, 10, 12, 22);
    for (let y = 0; y < 64; y += 1) {
      const shade = Math.round(14 + 18 * (y / 63));
      for (let x = 0; x < 64; x += 1) {
        blendPixel(map, x, y, shade, shade, shade + 10, 0.12);
      }
    }

    for (let particle = 0; particle < particles.length; particle += 1) {
      const item = particles[particle];
      item.vy += 0.04 + (1 - intensityUnit) * 0.03;
      item.vx += (rand() - 0.5) * 0.08;
      item.x += item.vx;
      item.y += item.vy;
      item.life -= 0.012;

      if (item.life <= 0 || item.y > 70 || item.x < -4 || item.x > 68) {
        item.x = rand() * 64;
        item.y = -2 - rand() * 12;
        item.vx = (rand() - 0.5) * (1 + safeSpeed * 0.1);
        item.vy = 0.2 + rand() * (1.6 + safeSpeed * 0.13);
        item.life = 0.5 + rand() * 0.6;
        item.size = rand() > 0.65 ? 2 : 1;
        item.colorIndex = Math.floor(rand() * palette.length);
      }

      if (item.y < 0 || item.y >= 64) {
        continue;
      }

      const color = palette[item.colorIndex];
      const alpha = clamp(item.life, 0.25, 1);
      const px = Math.round(item.x);
      const py = Math.round(item.y);
      drawGlow(map, px, py, 1.4 + item.size * 0.7, color[0], color[1], color[2], alpha * 0.55);
      fillRect(map, px, py, item.size, item.size, color[0], color[1], color[2]);
      blendPixel(map, px, py + item.size, color[0], color[1], color[2], 0.35 * alpha);
    }

    maps.push(map);
  }

  return maps;
}
