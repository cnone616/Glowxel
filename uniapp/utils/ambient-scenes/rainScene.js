import { fillRect, setPixel } from "../led-matrix-demos/common.js";

function fract(value) {
  return value - Math.floor(value);
}

function seed(index, shift) {
  return fract(Math.sin((index + 1) * 17.371 + shift * 3.147) * 24634.6345);
}

function scaleColor(color, ratio) {
  return {
    r: Math.round(color.r * ratio),
    g: Math.round(color.g * ratio),
    b: Math.round(color.b * ratio),
  };
}

function buildRainAmbientFrame(frameMap, elapsed, speedUnit, density, color) {
  fillRect(frameMap, 0, 0, 64, 64, 0, 0, 0);

  const densityUnit = Math.max(0.1, Math.min(1, density / 100));
  const dropletCount = Math.round(96 + densityUnit * 560);
  const driftRate = 0.00018 + speedUnit * 0.00042;

  for (let index = 0; index < dropletCount; index += 1) {
    const seedA = seed(index, 0.27);
    const seedB = seed(index, 1.11);
    const seedC = seed(index, 2.43);
    const seedD = seed(index, 3.92);
    const sway = Math.sin(elapsed * (0.0007 + seedB * 0.0009) + seedD * 6.28318) * 0.35;
    const lane = Math.floor(seedA * 64);
    const x = Math.max(0, Math.min(63, Math.round(lane + sway)));
    const velocity = 0.45 + seedC * 1.65;
    const travel = fract(seedD + elapsed * driftRate * velocity);
    const y = Math.floor(travel * 68) - 2;
    if (y < 0 || y >= 64) {
      continue;
    }

    const brightness = 0.28 + seedB * 0.72;
    const dropColor = scaleColor(color, brightness);
    setPixel(frameMap, x, y, dropColor.r, dropColor.g, dropColor.b);

  }
}

export { buildRainAmbientFrame };
