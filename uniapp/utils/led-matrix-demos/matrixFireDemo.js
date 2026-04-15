import { clamp, drawGlow, fillRect, normalizeIntensity, normalizeSpeed, setPixel } from "./common.js";

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state += 0x6d2b79f5;
    let tmp = Math.imul(state ^ (state >>> 15), state | 1);
    tmp ^= tmp + Math.imul(tmp ^ (tmp >>> 7), tmp | 61);
    return ((tmp ^ (tmp >>> 14)) >>> 0) / 4294967296;
  };
}

function firePalette(value, intensity) {
  const v = clamp(value, 0, 1);
  const boost = 0.2 + intensity * 0.55;
  if (v < 0.25) {
    const t = v / 0.25;
    return {
      r: Math.round(40 + t * 120 * boost),
      g: Math.round(6 + t * 48),
      b: Math.round(2 + t * 14),
    };
  }
  if (v < 0.55) {
    const t = (v - 0.25) / 0.3;
    return {
      r: Math.round(140 + t * 98),
      g: Math.round(40 + t * 110),
      b: Math.round(8 + t * 36),
    };
  }
  if (v < 0.82) {
    const t = (v - 0.55) / 0.27;
    return {
      r: Math.round(238 + t * 17),
      g: Math.round(150 + t * 68),
      b: Math.round(44 + t * 50),
    };
  }
  const t = (v - 0.82) / 0.18;
  return {
    r: 255,
    g: Math.round(218 + t * 26),
    b: Math.round(94 + t * 80),
  };
}

export function buildMatrixFireDemoMaps(speed, intensity) {
  const frameCount = 40;
  const maps = [];
  const safeSpeed = normalizeSpeed(speed);
  const intensityUnit = normalizeIntensity(intensity) / 100;
  const rand = createSeededRandom(20260414);
  const width = 64;
  const height = 64;
  const heat = new Array(height);
  const nextHeat = new Array(height);
  for (let y = 0; y < height; y += 1) {
    heat[y] = new Float32Array(width);
    nextHeat[y] = new Float32Array(width);
  }

  const subSteps = 1 + Math.floor(safeSpeed / 4);
  const coolBias = 0.013 + (1 - intensityUnit) * 0.02;
  for (let frameIndex = 0; frameIndex < frameCount + 18; frameIndex += 1) {
    for (let step = 0; step < subSteps; step += 1) {
      for (let x = 0; x < width; x += 1) {
        const base = 0.58 + intensityUnit * 0.32;
        const flare = rand() < 0.06 + intensityUnit * 0.06 ? rand() * 0.5 : 0;
        heat[height - 1][x] = clamp(base + flare + (rand() - 0.5) * 0.26, 0, 1);
      }

      for (let y = 0; y < height - 1; y += 1) {
        for (let x = 0; x < width; x += 1) {
          const left = heat[Math.min(height - 1, y + 1)][(x + width - 1) % width];
          const center = heat[Math.min(height - 1, y + 1)][x];
          const right = heat[Math.min(height - 1, y + 1)][(x + 1) % width];
          const below = heat[Math.min(height - 1, y + 2)][x];
          const average = (left + center * 1.35 + right + below) / 4.35;
          const turbulence = (rand() - 0.5) * (0.05 + safeSpeed * 0.003);
          const cool = coolBias + (y / height) * 0.016;
          nextHeat[y][x] = clamp(average - cool + turbulence, 0, 1);
        }
      }
      for (let x = 0; x < width; x += 1) {
        nextHeat[height - 1][x] = heat[height - 1][x];
      }
      for (let y = 0; y < height; y += 1) {
        const temp = heat[y];
        heat[y] = nextHeat[y];
        nextHeat[y] = temp;
      }
    }

    if (frameIndex < 18) {
      continue;
    }

    const map = new Map();
    fillRect(map, 0, 0, 64, 64, 0, 0, 0);
    for (let y = 0; y < height; y += 1) {
      for (let x = 0; x < width; x += 1) {
        const value = heat[y][x];
        if (value <= 0.03) {
          continue;
        }
        const palette = firePalette(value, intensityUnit);
        setPixel(map, x, y, palette.r, palette.g, palette.b);
      }
    }

    for (let spark = 0; spark < 26; spark += 1) {
      const sx = Math.floor(rand() * 64);
      const sy = 45 - Math.floor(rand() * 42);
      if (sy < 0) {
        continue;
      }
      drawGlow(map, sx, sy, 1.8, 255, 212, 132, 0.48 + intensityUnit * 0.2);
    }
    maps.push(map);
  }

  return maps;
}
