import { fillRect, setPixel } from "../led-matrix-demos/common.js";

function clampUnit(value) {
  if (value < 0) {
    return 0;
  }
  if (value > 1) {
    return 1;
  }
  return value;
}

function hslToRgb(hue, saturation, lightness) {
  const chroma = (1 - Math.abs(2 * lightness - 1)) * saturation;
  const huePrime = ((hue % 360) + 360) % 360 / 60;
  const x = chroma * (1 - Math.abs((huePrime % 2) - 1));
  const match = lightness - chroma / 2;

  let r1 = 0;
  let g1 = 0;
  let b1 = 0;

  if (huePrime < 1) {
    r1 = chroma;
    g1 = x;
  } else if (huePrime < 2) {
    r1 = x;
    g1 = chroma;
  } else if (huePrime < 3) {
    g1 = chroma;
    b1 = x;
  } else if (huePrime < 4) {
    g1 = x;
    b1 = chroma;
  } else if (huePrime < 5) {
    r1 = x;
    b1 = chroma;
  } else {
    r1 = chroma;
    b1 = x;
  }

  return {
    r: Math.round((r1 + match) * 255),
    g: Math.round((g1 + match) * 255),
    b: Math.round((b1 + match) * 255),
  };
}

function buildNeonTunnelAmbientFrame(frameMap, elapsed, speedUnit) {
  fillRect(frameMap, 0, 0, 64, 64, 0, 0, 0);

  const timeCounter = elapsed / 120 * 0.05;
  const centerX = 32;
  const centerY = 32;
  const oscX = centerX + Math.sin(timeCounter * 0.5) * 16;
  const oscY = centerY + Math.cos(timeCounter * 0.7) * 16;
  const distanceFactor = 1.25 + speedUnit * 0.8;
  const angleFactor = 2.4;
  const hueShiftSpeed = 0.2 + speedUnit * 0.35;

  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const dx = x - oscX;
      const dy = y - oscY;
      const distance = Math.max(Math.sqrt(dx * dx + dy * dy), 0.001);
      const angle = Math.atan2(dy, dx);

      const uvX = distanceFactor / distance + speedUnit * timeCounter;
      const uvY = (angle * angleFactor) / Math.PI + Math.sin(timeCounter) * 2;
      const texX = Math.round(Math.abs(uvX * 32)) % 256;
      const texY = Math.round(Math.abs(uvY * 32)) % 256;
      const pattern = texX ^ texY;
      const depthShade = clampUnit(1 - distance / 96);
      const hue = (timeCounter * hueShiftSpeed * 50 + distance * 2) % 360;
      const lightness = pattern > 128 ? 0.5 * depthShade : 0;

      if (lightness <= 0) {
        continue;
      }

      const color = hslToRgb(hue, 1, lightness);
      setPixel(frameMap, x, y, color.r, color.g, color.b);
    }
  }
}

export { buildNeonTunnelAmbientFrame };
