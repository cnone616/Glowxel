import { buildConfettiDemoMaps } from "../led-matrix-demos/confettiDemo.js";
import { clamp, normalizeIntensity, normalizeSpeed } from "../led-matrix-demos/common.js";
import { buildMatrixFireDemoMaps } from "../led-matrix-demos/matrixFireDemo.js";
import { buildShadertoyHeartDemoMaps } from "../led-matrix-demos/shadertoyHeartDemo.js";

const PLAYER_SCENE_ITEMS = [
  {
    id: "matrix_fire",
    label: "矩阵火焰",
    source: "MatrixFireFast/Fire",
    hint: "离线素材，优先用于板载播放器压测与日常屏保播放。",
  },
  {
    id: "confetti_party",
    label: "彩纸粒子",
    source: "oledAnimations/Confetti",
    hint: "离线粒子素材，适合验证短帧循环和色彩稳定性。",
  },
  {
    id: "heart_shader",
    label: "心形波场",
    source: "led-matrix/Shadertoy",
    hint: "离线着色场景，适合验证高对比场景的播放节奏。",
  },
];

function resolveSceneById(sceneId) {
  for (let index = 0; index < PLAYER_SCENE_ITEMS.length; index += 1) {
    const item = PLAYER_SCENE_ITEMS[index];
    if (item.id === sceneId) {
      return item;
    }
  }
  return null;
}

function parseRgbText(value) {
  if (typeof value !== "string") {
    return null;
  }
  const match = /rgb\((\d+),\s*(\d+),\s*(\d+)\)/i.exec(value);
  if (!match) {
    return null;
  }
  return {
    r: clamp(Number(match[1]), 0, 255),
    g: clamp(Number(match[2]), 0, 255),
    b: clamp(Number(match[3]), 0, 255),
  };
}

function parsePixelKey(key) {
  const keyParts = key.split(",");
  if (keyParts.length !== 2) {
    return null;
  }
  const x = Number(keyParts[0]);
  const y = Number(keyParts[1]);
  if (!Number.isFinite(x) || !Number.isFinite(y)) {
    return null;
  }
  return {
    x: Math.round(x),
    y: Math.round(y),
  };
}

function sampleArray(source, maxCount) {
  if (source.length <= maxCount) {
    return source;
  }
  const sampled = [];
  const step = source.length / maxCount;
  for (let index = 0; index < maxCount; index += 1) {
    const sourceIndex = Math.floor(index * step);
    const safeIndex = sourceIndex >= source.length ? source.length - 1 : sourceIndex;
    sampled.push(source[safeIndex]);
  }
  return sampled;
}

function buildSparseFrameMap(pixelMap, maxPixels) {
  const collected = [];
  pixelMap.forEach((colorText, key) => {
    const rgb = parseRgbText(colorText);
    if (!rgb) {
      return;
    }
    const brightness = rgb.r + rgb.g + rgb.b;
    if (brightness < 84) {
      return;
    }
    const point = parsePixelKey(key);
    if (!point) {
      return;
    }
    collected.push({
      key: `${point.x},${point.y}`,
      x: point.x,
      y: point.y,
      r: rgb.r,
      g: rgb.g,
      b: rgb.b,
      brightness,
    });
  });

  if (collected.length === 0) {
    pixelMap.forEach((colorText, key) => {
      const rgb = parseRgbText(colorText);
      if (!rgb) {
        return;
      }
      const point = parsePixelKey(key);
      if (!point) {
        return;
      }
      collected.push({
        key: `${point.x},${point.y}`,
        x: point.x,
        y: point.y,
        r: rgb.r,
        g: rgb.g,
        b: rgb.b,
        brightness: rgb.r + rgb.g + rgb.b,
      });
    });
  }

  collected.sort((left, right) => right.brightness - left.brightness);

  const sampled = sampleArray(collected, maxPixels);
  const sparseMap = new Map();
  for (let index = 0; index < sampled.length; index += 1) {
    const pixel = sampled[index];
    sparseMap.set(pixel.key, {
      x: pixel.x,
      y: pixel.y,
      r: pixel.r,
      g: pixel.g,
      b: pixel.b,
    });
  }
  return sparseMap;
}

function buildDiffPixels(previousMap, currentMap, maxPixels) {
  const diffPixels = [];

  currentMap.forEach((currentPixel, key) => {
    const previousPixel = previousMap.get(key);
    if (
      !previousPixel ||
      previousPixel.r !== currentPixel.r ||
      previousPixel.g !== currentPixel.g ||
      previousPixel.b !== currentPixel.b
    ) {
      diffPixels.push([
        currentPixel.x,
        currentPixel.y,
        currentPixel.r,
        currentPixel.g,
        currentPixel.b,
      ]);
    }
  });

  previousMap.forEach((previousPixel, key) => {
    if (!currentMap.has(key)) {
      diffPixels.push([previousPixel.x, previousPixel.y, 0, 0, 0]);
    }
  });

  return sampleArray(diffPixels, maxPixels);
}

function buildSceneMaps(sceneId, speed, intensity) {
  if (sceneId === "matrix_fire") {
    return buildMatrixFireDemoMaps(speed, intensity);
  }
  if (sceneId === "confetti_party") {
    return buildConfettiDemoMaps(speed, intensity);
  }
  if (sceneId === "heart_shader") {
    return buildShadertoyHeartDemoMaps(speed, intensity);
  }
  return [];
}

function buildGifPlayerPreviewSequence(options) {
  const safeOptions =
    options && typeof options === "object" ? options : {};
  const scene = resolveSceneById(safeOptions.sceneId);
  if (!scene) {
    return {
      maps: [],
      delays: [],
    };
  }

  const speed = normalizeSpeed(safeOptions.speed);
  const intensity = normalizeIntensity(safeOptions.intensity);
  const maps = buildSceneMaps(scene.id, speed, intensity);
  const delay = clamp(198 - speed * 12, 58, 188);

  return {
    maps,
    delays: maps.map(() => delay),
  };
}

function trimSequence(maps, delays, maxFrames) {
  if (maps.length <= maxFrames) {
    return {
      maps,
      delays,
    };
  }

  const sampledMaps = [];
  const sampledDelays = [];
  const step = maps.length / maxFrames;
  for (let index = 0; index < maxFrames; index += 1) {
    const sourceIndex = Math.floor(index * step);
    const safeIndex =
      sourceIndex >= maps.length ? maps.length - 1 : sourceIndex;
    sampledMaps.push(maps[safeIndex]);
    sampledDelays.push(delays[safeIndex]);
  }

  return {
    maps: sampledMaps,
    delays: sampledDelays,
  };
}

function mapToCompactFrame(frameType, pixels, delay) {
  const safeDelay = Number.isFinite(Number(delay)) ? Number(delay) : 120;
  return [frameType, safeDelay, pixels.length, pixels];
}

function buildGifPlayerAnimationPayload(options) {
  const preview = buildGifPlayerPreviewSequence(options);
  const trimmed = trimSequence(preview.maps, preview.delays, 10);
  const animationData = [];
  let previousSparseMap = new Map();

  for (let frameIndex = 0; frameIndex < trimmed.maps.length; frameIndex += 1) {
    const currentMap = trimmed.maps[frameIndex];
    const currentSparseMap = buildSparseFrameMap(currentMap, 360);
    const delay = trimmed.delays[frameIndex];
    if (frameIndex === 0) {
      const firstPixels = [];
      currentSparseMap.forEach((pixel) => {
        firstPixels.push([pixel.x, pixel.y, pixel.r, pixel.g, pixel.b]);
      });
      animationData.push(mapToCompactFrame(1, firstPixels, delay));
    } else {
      const diffPixels = buildDiffPixels(previousSparseMap, currentSparseMap, 480);
      animationData.push(mapToCompactFrame(0, diffPixels, delay));
    }
    previousSparseMap = currentSparseMap;
  }

  return {
    animationData,
  };
}

function getGifPlayerSceneItems() {
  return PLAYER_SCENE_ITEMS;
}

export {
  buildGifPlayerAnimationPayload,
  buildGifPlayerPreviewSequence,
  getGifPlayerSceneItems,
  resolveSceneById,
};
