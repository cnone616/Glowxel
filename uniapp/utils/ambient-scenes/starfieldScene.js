import { blendPixel, fillRect, setPixel } from "../led-matrix-demos/common.js";

function fract(value) {
  return value - Math.floor(value);
}

function seed(index, shift) {
  return fract(Math.sin((index + 1) * 17.371 + shift * 3.147) * 24634.6345);
}

function clampUnit(value) {
  if (value < 0) {
    return 0;
  }
  if (value > 1) {
    return 1;
  }
  return value;
}

function buildStarfieldAmbientFrame(frameMap, elapsed, speedUnit) {
  fillRect(frameMap, 0, 0, 64, 64, 0, 0, 0);

  const centerX = 32;
  const centerY = 32;
  const maxDepth = 3;
  const starCount = 50;
  const travelRate = 0.00022 + speedUnit * 0.00022;

  for (let index = 0; index < starCount; index += 1) {
    const seedX = seed(index, 0.11);
    const seedY = seed(index, 1.73);
    const seedZ = seed(index, 2.91);
    const seedT = seed(index, 3.71);

    const x3d = seedX * 2 - 1;
    const y3d = seedY * 2 - 1;
    const travel = fract(seedZ + elapsed * travelRate * (0.65 + seedT * 0.7));
    const z = Math.max(0.08, maxDepth - travel * maxDepth);
    const perspective = 0.5 / z;
    const x = Math.round(x3d * perspective * centerX + centerX);
    const y = Math.round(y3d * perspective * centerY + centerY);

    if (x < 0 || x >= 64 || y < 0 || y >= 64) {
      continue;
    }

    const depthFactor = clampUnit((maxDepth - z) / maxDepth);
    const twinkle =
      0.8 +
      0.2 * (0.5 + 0.5 * Math.sin(elapsed * 0.0014 * (0.7 + seedT) + seedX * Math.PI * 2));
    const brightness = Math.round(255 * Math.sqrt(depthFactor) * twinkle);

    setPixel(frameMap, x, y, brightness, brightness, brightness);

    if (z < maxDepth * 0.3) {
      const glow = brightness / 4;
      for (let offsetY = -1; offsetY <= 1; offsetY += 1) {
        for (let offsetX = -1; offsetX <= 1; offsetX += 1) {
          if (offsetX === 0 && offsetY === 0) {
            continue;
          }
          blendPixel(frameMap, x + offsetX, y + offsetY, glow, glow, glow, 0.5);
        }
      }
    }
  }
}

export { buildStarfieldAmbientFrame };
