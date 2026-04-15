import { clamp, normalizeIntensity, normalizeSpeed, setPixel } from "./common.js";

export function buildShadertoyHeartDemoMaps(speed, intensity) {
  const frameCount = 24;
  const maps = [];
  const safeSpeed = normalizeSpeed(speed);
  const safeIntensity = normalizeIntensity(intensity) / 100;

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const map = new Map();
    const phase = frameIndex / frameCount;
    const pulse = 0.86 + Math.sin(phase * Math.PI * 2 * (0.8 + safeSpeed * 0.09)) * 0.1;

    for (let y = 0; y < 64; y += 1) {
      for (let x = 0; x < 64; x += 1) {
        const nx = ((x - 31.5) / 24) / pulse;
        const ny = ((y - 30) / 24) / pulse;
        const a = nx * nx + ny * ny - 1;
        const heartValue = a * a * a - nx * nx * ny * ny * ny;
        const glow = clamp(0.45 - heartValue * (1.1 + safeIntensity * 0.9), 0, 1);

        const bgR = Math.round(14 + 20 * (0.5 + 0.5 * Math.sin(nx * 3 + phase * 6)));
        const bgG = Math.round(10 + 16 * (0.5 + 0.5 * Math.cos(ny * 2 - phase * 5)));
        const bgB = Math.round(20 + 30 * (0.5 + 0.5 * Math.sin((nx + ny) * 2 - phase * 4)));

        const heartR = Math.round(190 + 60 * glow);
        const heartG = Math.round(42 + 36 * glow);
        const heartB = Math.round(76 + 50 * glow);

        const r = Math.round(bgR * (1 - glow) + heartR * glow);
        const g = Math.round(bgG * (1 - glow) + heartG * glow);
        const b = Math.round(bgB * (1 - glow) + heartB * glow);
        setPixel(map, x, y, r, g, b);
      }
    }
    maps.push(map);
  }
  return maps;
}
