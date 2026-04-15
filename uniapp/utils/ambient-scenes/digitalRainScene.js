import { fillRect, setPixel } from "../led-matrix-demos/common.js";

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state = (state * 1664525 + 1013904223) >>> 0;
    return state / 0x100000000;
  };
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

function buildDigitalRainAmbientFrames(frameCount, speedUnit) {
  const width = 64;
  const height = 64;
  const numDrops = 40;
  const baseSpeed = 0.5 + speedUnit * 1.5;
  const fadeFactor = 0.9;
  const rand = createSeededRandom(20260421);
  const matrixBrightness = Array.from({ length: width }, () => Array(height).fill(0));
  const drops = [];

  function resetDrop(drop) {
    drop.x = Math.floor(rand() * width);
    drop.length = 5 + Math.floor(rand() * (height / 2 - 4));
    drop.speed = baseSpeed * (0.5 + rand());
    drop.y = -drop.length;
  }

  for (let index = 0; index < numDrops; index += 1) {
    const drop = { x: 0, y: 0, speed: 0, length: 0 };
    resetDrop(drop);
    drop.y = -height + rand() * (height * 2);
    drops.push(drop);
  }

  const frames = [];
  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const frame = new Map();
    fillRect(frame, 0, 0, width, height, 0, 0, 0);

    for (let x = 0; x < width; x += 1) {
      for (let y = 0; y < height; y += 1) {
        matrixBrightness[x][y] *= fadeFactor;
      }
    }

    for (let index = 0; index < drops.length; index += 1) {
      const drop = drops[index];
      const oldY = Math.floor(drop.y);
      drop.y += drop.speed;
      const newY = Math.floor(drop.y);

      if (newY >= 0 && newY < height) {
        for (let y = Math.max(0, oldY); y <= newY && y < height; y += 1) {
          matrixBrightness[drop.x][y] = 1;
        }
      }

      if (drop.y - drop.length > height) {
        resetDrop(drop);
      }
    }

    for (let x = 0; x < width; x += 1) {
      for (let y = 0; y < height; y += 1) {
        const brightness = matrixBrightness[x][y];
        if (brightness > 0.01) {
          const safe = clampUnit(brightness);
          setPixel(frame, x, y, 0, 255 * safe, 0);
        }
      }
    }

    for (let index = 0; index < drops.length; index += 1) {
      const headY = Math.floor(drops[index].y);
      if (headY >= 0 && headY < height) {
        setPixel(frame, drops[index].x, headY, 255, 255, 255);
      }
    }

    frames.push(frame);
  }

  return frames;
}

export { buildDigitalRainAmbientFrames };
