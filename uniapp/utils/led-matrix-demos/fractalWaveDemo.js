import { blendPixel, fillRect, normalizeIntensity, normalizeSpeed } from "./common.js";

export function buildFractalWaveDemoMaps(speed, intensity) {
  const frameCount = 24;
  const maps = [];
  const safeSpeed = normalizeSpeed(speed);
  const safeIntensity = normalizeIntensity(intensity) / 100;

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const map = new Map();
    fillRect(map, 0, 0, 64, 64, 0, 0, 0);
    const time = frameIndex * (0.28 + safeSpeed * 0.06);

    for (let x = 0; x < 64; x += 1) {
      const nx = x / 63;
      const waveA = Math.sin(nx * 10 + time) * 12;
      const waveB = Math.sin(nx * 18 - time * 1.2) * 6;
      const waveC = Math.cos(nx * 26 + time * 0.8) * 3;
      const y = Math.round(28 + waveA + waveB + waveC);
      for (let thickness = -1; thickness <= 1; thickness += 1) {
        const blend = 1 - Math.abs(thickness) * 0.35;
        const r = 188 + Math.round(55 * Math.sin(nx * 8 + time * 0.3));
        const g = 86 + Math.round(140 * nx * blend);
        const b = 255 - Math.round(120 * nx) + Math.round(55 * safeIntensity);
        blendPixel(map, x, y + thickness, r, g, b, 0.9 * blend);
      }
    }

    for (let spark = 0; spark < 28; spark += 1) {
      const px = Math.round((spark * 13 + frameIndex * 5) % 64);
      const py = Math.round((spark * 7 + Math.sin(frameIndex * 0.4 + spark) * 18 + 32) % 64);
      blendPixel(map, px, py, 255, 220, 96, 0.45);
    }
    maps.push(map);
  }

  return maps;
}
