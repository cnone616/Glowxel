import { clamp, setPixel } from "@/utils/device-mode-core.js";

const SCENE_ITEMS = Object.freeze([
  { id: "matrix_fire", label: "矩阵火焰" },
  { id: "confetti_party", label: "彩纸粒子" },
  { id: "heart_shader", label: "心形波场" },
]);

function createFrameMap() {
  return new Map();
}

function sceneDelay(speed) {
  return clamp(198 - speed * 12, 58, 188);
}

function drawMatrixFireFrame(frameIndex, intensity) {
  const map = createFrameMap();
  for (let x = 0; x < 64; x += 1) {
    const flameHeight = Math.floor(((Math.sin((x + frameIndex * 1.7) / 5) + 1) * 0.5) * (18 + intensity * 0.18));
    for (let offset = 0; offset < flameHeight; offset += 1) {
      const y = 63 - offset;
      const green = clamp(40 + offset * 6, 0, 255);
      const red = clamp(140 + offset * 4, 0, 255);
      const blue = clamp(Math.floor(offset * 0.7), 0, 60);
      setPixel(map, x, y, `rgb(${red}, ${green}, ${blue})`);
    }
  }
  return map;
}

function drawConfettiFrame(frameIndex, intensity) {
  const map = createFrameMap();
  const particles = 42 + Math.floor(intensity / 4);
  for (let index = 0; index < particles; index += 1) {
    const x = (index * 11 + frameIndex * 3) % 64;
    const y = (index * 7 + frameIndex * 5) % 64;
    const hue = (index * 37 + frameIndex * 19) % 360;
    const red = Math.floor(128 + 127 * Math.sin((hue * Math.PI) / 180));
    const green = Math.floor(128 + 127 * Math.sin(((hue + 120) * Math.PI) / 180));
    const blue = Math.floor(128 + 127 * Math.sin(((hue + 240) * Math.PI) / 180));
    setPixel(map, x, y, `rgb(${red}, ${green}, ${blue})`);
  }
  return map;
}

function drawHeartFrame(frameIndex, intensity) {
  const map = createFrameMap();
  const phase = frameIndex / 12;
  const scale = 14 + Math.sin(phase) * 2 + intensity * 0.04;
  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const nx = (x - 32) / scale;
      const ny = (y - 28) / scale;
      const formula = Math.pow(nx * nx + ny * ny - 1, 3) - nx * nx * Math.pow(ny, 3);
      if (formula > 0) {
        continue;
      }
      const glow = Math.max(0, 1 - Math.abs(formula) * 6);
      const red = clamp(Math.floor(180 + glow * 75), 0, 255);
      const green = clamp(Math.floor(30 + glow * 40), 0, 255);
      const blue = clamp(Math.floor(70 + glow * 85), 0, 255);
      setPixel(map, x, y, `rgb(${red}, ${green}, ${blue})`);
    }
  }
  return map;
}

function buildSceneFrames(sceneId, speed, intensity) {
  const frames = [];
  const frameTotal = 10;
  for (let frameIndex = 0; frameIndex < frameTotal; frameIndex += 1) {
    if (sceneId === "matrix_fire") {
      frames.push(drawMatrixFireFrame(frameIndex, intensity));
    } else if (sceneId === "confetti_party") {
      frames.push(drawConfettiFrame(frameIndex, intensity));
    } else if (sceneId === "heart_shader") {
      frames.push(drawHeartFrame(frameIndex, intensity));
    }
  }
  return frames;
}

function parseRgb(color) {
  const match = /rgb\((\d+),\s*(\d+),\s*(\d+)\)/i.exec(color);
  if (!match) {
    return null;
  }
  return {
    r: Number(match[1]),
    g: Number(match[2]),
    b: Number(match[3]),
  };
}

function buildSparseFrame(map) {
  const pixels = [];
  map.forEach((color, key) => {
    const rgb = parseRgb(color);
    if (!rgb) {
      return;
    }
    const parts = key.split(",");
    pixels.push([Number(parts[0]), Number(parts[1]), rgb.r, rgb.g, rgb.b]);
  });
  return pixels;
}

function buildDiffPixels(previousMap, currentMap) {
  const diff = [];
  currentMap.forEach((color, key) => {
    if (previousMap.get(key) === color) {
      return;
    }
    const rgb = parseRgb(color);
    if (!rgb) {
      return;
    }
    const parts = key.split(",");
    diff.push([Number(parts[0]), Number(parts[1]), rgb.r, rgb.g, rgb.b]);
  });
  previousMap.forEach((_, key) => {
    if (currentMap.has(key)) {
      return;
    }
    const parts = key.split(",");
    diff.push([Number(parts[0]), Number(parts[1]), 0, 0, 0]);
  });
  return diff;
}

export function getGifPlayerSceneItems() {
  return SCENE_ITEMS;
}

export function buildGifPlayerPreviewSequence({ sceneId, speed, intensity }) {
  const safeSpeed = clamp(Number(speed) || 1, 1, 10);
  const safeIntensity = clamp(Number(intensity) || 10, 10, 100);
  const maps = buildSceneFrames(sceneId, safeSpeed, safeIntensity);
  const delay = sceneDelay(safeSpeed);
  return {
    maps,
    delays: maps.map(() => delay),
  };
}

export function buildGifPlayerAnimationPayload({ sceneId, speed, intensity }) {
  const sequence = buildGifPlayerPreviewSequence({ sceneId, speed, intensity });
  const animationData = [];
  let previousMap = new Map();
  sequence.maps.forEach((frameMap, index) => {
    const pixels = index === 0 ? buildSparseFrame(frameMap) : buildDiffPixels(previousMap, frameMap);
    animationData.push([index === 0 ? 1 : 0, sequence.delays[index], pixels.length, pixels]);
    previousMap = new Map(frameMap);
  });
  return { animationData };
}
