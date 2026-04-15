import { setPixel } from "../led-matrix-demos/common.js";

function hueToRgb(hue) {
  let h = hue % 1;
  if (h < 0) {
    h += 1;
  }
  h *= 6;
  const index = Math.floor(h);
  const fraction = h - index;

  if (index === 0) {
    return { r: 255, g: 255 * fraction, b: 0 };
  }
  if (index === 1) {
    return { r: 255 * (1 - fraction), g: 255, b: 0 };
  }
  if (index === 2) {
    return { r: 0, g: 255, b: 255 * fraction };
  }
  if (index === 3) {
    return { r: 0, g: 255 * (1 - fraction), b: 255 };
  }
  if (index === 4) {
    return { r: 255 * fraction, g: 0, b: 255 };
  }
  return { r: 255, g: 0, b: 255 * (1 - fraction) };
}

function randSin(index) {
  return Math.sin(index * 1.64);
}

function getBlob(index, time) {
  let x = 0.5 + 0.1 * randSin(index);
  let y = 0.5 + 0.1 * randSin(index + 42);

  x += 0.5 * Math.sin(0.25 * time * randSin(index + 2)) * randSin(index + 56);
  y += 0.5 * -Math.sin(0.25 * time) * randSin(index * 9);

  const radius = 0.1 * Math.abs(randSin(index + 3));
  return {
    x: x * 64,
    y: y * 64,
    radius: radius * 64,
  };
}

function calculateField(x, y, blob) {
  const dx = x - blob.x;
  const dy = y - blob.y;
  const distance = Math.max(Math.sqrt(dx * dx + dy * dy) + blob.radius / 2, 0);
  const temp = distance * distance;
  return 1 / (temp * temp);
}

function buildMetaBlobAmbientFrame(frameMap, elapsed) {
  const time = elapsed / 1000;
  const blobs = [];
  const numBlobs = 10;
  const threshold = 0.0003;
  const hue = (time * 0.033) % 1;

  for (let index = 0; index < numBlobs; index += 1) {
    blobs.push(getBlob(index, time));
  }

  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      let field = 0;
      for (let index = 0; index < blobs.length; index += 1) {
        field += calculateField(x, y, blobs[index]);
      }

      if (field <= threshold) {
        continue;
      }

      let t = Math.min(1, (field - threshold) / threshold);
      t = t * t * (3 - 2 * t);
      const center = hueToRgb(hue);
      const edge = hueToRgb(hue + 0.25);
      setPixel(
        frameMap,
        x,
        y,
        center.r * t + edge.r * (1 - t),
        center.g * t + edge.g * (1 - t),
        center.b * t + edge.b * (1 - t),
      );
    }
  }
}

export { buildMetaBlobAmbientFrame };
