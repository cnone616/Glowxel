import { setPixel } from "./common.js";

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state = (state * 1664525 + 1013904223) >>> 0;
    return state / 0x100000000;
  };
}

function xyToIndex(width, x, y) {
  return y * width + x;
}

function floatPixelToRgb(value) {
  const safe = Math.max(0, value);
  const r = Math.pow(safe, 4 + safe * 0.5) * Math.cos(safe);
  const g = Math.pow(safe, 3 + safe * 0.5) * Math.sin(safe);
  const b = Math.pow(safe, 2 + safe * 0.5);
  return {
    r: Math.max(0, Math.min(255, Math.round(r * 255))),
    g: Math.max(0, Math.min(255, Math.round(g * 255))),
    b: Math.max(0, Math.min(255, Math.round(b * 255))),
  };
}

export function buildWaveSceneDemoMaps() {
  const width = 64;
  const height = 64;
  const rand = createSeededRandom(20260422);
  let lastMap = new Float32Array(width * height);
  const maps = [];
  const frameCount = 32;

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      lastMap[xyToIndex(width, x, y)] = rand();
    }
  }

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const nextMap = new Float32Array(width * height);
    const frame = new Map();

    for (let y = 0; y < height; y += 1) {
      for (let x = 0; x < width; x += 1) {
        const index = xyToIndex(width, x, y);
        const lastValue = lastMap[index];
        nextMap[index] = lastValue * (0.96 + 0.02 * rand());

        if (lastValue <= 0.18 + 0.04 * rand()) {
          let neighbors = 0;
          for (let u = -1; u <= 1; u += 1) {
            for (let v = -1; v <= 1; v += 1) {
              if (u === 0) {
                continue;
              }
              const nextX = Math.abs((x + u) % width);
              const nextY = Math.abs((y + v) % height);
              const neighborValue = lastMap[xyToIndex(width, nextX, nextY)];
              if (neighborValue >= 0.5 + 0.04 * rand()) {
                neighbors += 1;
                nextMap[index] += neighborValue * (0.8 + 0.4 * rand());
              }
            }
          }

          if (neighbors > 0) {
            nextMap[index] *= 1 / neighbors;
          }
          if (nextMap[index] > 1) {
            nextMap[index] = 1;
          }
        }

        const rgb = floatPixelToRgb(nextMap[index]);
        setPixel(frame, x, y, rgb.r, rgb.g, rgb.b);
      }
    }

    maps.push(frame);
    lastMap = nextMap;
  }

  return maps;
}
