import { clamp, normalizeIntensity, normalizeSpeed, setPixel } from "./common.js";

export function buildGithubWaveDemoMaps(speed, intensity) {
  const frameCount = 24;
  const maps = [];
  const safeSpeed = normalizeSpeed(speed);
  const safeIntensity = normalizeIntensity(intensity) / 100;

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const map = new Map();
    const time = frameIndex * (0.2 + safeSpeed * 0.05);
    for (let y = 0; y < 64; y += 1) {
      for (let x = 0; x < 64; x += 1) {
        const nx = (x - 32) / 22;
        const ny = (y - 32) / 22;
        const fieldA = Math.sin(nx * 3.8 + time) + Math.cos(ny * 4.1 - time * 0.9);
        const fieldB = Math.sin((nx - ny) * 5.3 + time * 0.7);
        const mix = clamp((fieldA * 0.5 + fieldB * 0.5 + 1) / 2, 0, 1);
        const contour = Math.pow(mix, 1.3);
        const r = 20 + Math.round(30 * contour);
        const g = 46 + Math.round(110 * contour);
        const b = 108 + Math.round(140 * contour + 40 * safeIntensity);
        setPixel(map, x, y, r, g, b);
      }
    }
    maps.push(map);
  }
  return maps;
}
