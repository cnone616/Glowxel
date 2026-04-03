const PANEL_SIZE = 64;

function assertColorHex(hex) {
  if (typeof hex !== "string") {
    throw new Error("颜色格式错误");
  }
  const normalized = hex.toLowerCase();
  const ok = /^#([0-9a-f]{6})$/.test(normalized);
  if (!ok) {
    throw new Error("颜色格式错误");
  }
  return normalized;
}

function hexToRgb(hex) {
  const normalized = assertColorHex(hex);
  return {
    r: parseInt(normalized.slice(1, 3), 16),
    g: parseInt(normalized.slice(3, 5), 16),
    b: parseInt(normalized.slice(5, 7), 16),
  };
}

function clampInt(value, min, max) {
  const n = Number(value);
  if (!Number.isFinite(n)) {
    throw new Error("数值参数错误");
  }
  const rounded = Math.round(n);
  if (rounded < min) {
    return min;
  }
  if (rounded > max) {
    return max;
  }
  return rounded;
}

function makeUniformFrame(delay, color) {
  const totalPixels = PANEL_SIZE * PANEL_SIZE;
  const bytes = new Uint8Array(totalPixels * 5);
  let idx = 0;
  for (let y = 0; y < PANEL_SIZE; y++) {
    for (let x = 0; x < PANEL_SIZE; x++) {
      bytes[idx++] = x;
      bytes[idx++] = y;
      bytes[idx++] = color.r;
      bytes[idx++] = color.g;
      bytes[idx++] = color.b;
    }
  }

  return {
    type: 1,
    delay,
    totalPixels,
    pixels: bytes,
  };
}

function blendColor(colorA, colorB, alpha) {
  const a = Math.min(Math.max(alpha, 0), 1);
  return {
    r: Math.round(colorA.r * (1 - a) + colorB.r * a),
    g: Math.round(colorA.g * (1 - a) + colorB.g * a),
    b: Math.round(colorA.b * (1 - a) + colorB.b * a),
  };
}

function getDirectionPos(direction, x, y) {
  if (direction === "left") {
    return x / (PANEL_SIZE - 1);
  }
  if (direction === "right") {
    return 1 - x / (PANEL_SIZE - 1);
  }
  if (direction === "up") {
    return y / (PANEL_SIZE - 1);
  }
  if (direction === "down") {
    return 1 - y / (PANEL_SIZE - 1);
  }
  throw new Error("方向参数错误");
}

function buildRingPath(layer) {
  const left = layer;
  const top = layer;
  const right = PANEL_SIZE - 1 - layer;
  const bottom = PANEL_SIZE - 1 - layer;

  if (left > right || top > bottom) {
    return [];
  }

  if (left === right && top === bottom) {
    return [[left, top]];
  }

  const path = [];

  for (let x = left; x <= right; x++) {
    path.push([x, top]);
  }
  for (let y = top + 1; y <= bottom; y++) {
    path.push([right, y]);
  }
  if (bottom > top) {
    for (let x = right - 1; x >= left; x--) {
      path.push([x, bottom]);
    }
  }
  if (right > left) {
    for (let y = bottom - 1; y > top; y--) {
      path.push([left, y]);
    }
  }

  return path;
}

function buildAllRingPaths() {
  const result = [];
  for (let layer = 0; layer < Math.ceil(PANEL_SIZE / 2); layer++) {
    const path = buildRingPath(layer);
    if (path.length === 0) {
      continue;
    }
    result.push(path);
  }
  return result;
}

function setFramePixel(frameMap, x, y, color) {
  frameMap.set(`${x},${y}`, color);
}

function appendPathTrail(frameMap, path, headIndex, trailLength, colorA, colorB, colorMode) {
  const pathLength = path.length;
  if (pathLength === 0) {
    return;
  }

  for (let step = 0; step < trailLength; step++) {
    let index = headIndex - step;
    while (index < 0) {
      index += pathLength;
    }
    index = index % pathLength;
    const point = path[index];
    if (!point) {
      continue;
    }

    let color = colorA;
    if (colorMode === "gradient") {
      const ratio = trailLength <= 1 ? 1 : step / (trailLength - 1);
      color = blendColor(colorA, colorB, ratio);
    }

    setFramePixel(frameMap, point[0], point[1], color);
  }
}

function fillWholePath(frameMap, path, color) {
  for (let i = 0; i < path.length; i++) {
    const point = path[i];
    setFramePixel(frameMap, point[0], point[1], color);
  }
}

function frameMapToFrame(frameMap, delay) {
  const bytes = [];
  frameMap.forEach((color, key) => {
    const [x, y] = key.split(",").map(Number);
    bytes.push(x, y, color.r, color.g, color.b);
  });

  return {
    type: 1,
    delay,
    totalPixels: bytes.length / 5,
    pixels: new Uint8Array(bytes),
  };
}

export function buildBreathFrames(config) {
  if (!config) {
    throw new Error("环绕灯配置缺失");
  }
  if (
    config.speed === undefined ||
    config.loop === undefined ||
    config.motion === undefined ||
    config.scope === undefined ||
    config.colorMode === undefined ||
    config.colorA === undefined ||
    config.colorB === undefined
  ) {
    throw new Error("环绕灯配置字段不完整");
  }

  const speed = clampInt(config.speed, 1, 10);
  const loop = config.loop === true;
  const colorA = hexToRgb(config.colorA);
  const colorB = hexToRgb(config.colorB);
  const ringPaths = buildAllRingPaths();
  let frameCount = loop ? 24 : 12;
  const delay = Math.max(35, 190 - speed * 12);
  const frames = [];

  if (
    config.motion !== "clockwise" &&
    config.motion !== "counterclockwise" &&
    config.motion !== "inward" &&
    config.motion !== "outward"
  ) {
    throw new Error("环绕灯运行方式错误");
  }
  if (config.scope !== "single_ring" && config.scope !== "full_screen") {
    throw new Error("环绕灯范围参数错误");
  }
  if (config.colorMode !== "solid" && config.colorMode !== "gradient") {
    throw new Error("环绕灯颜色模式错误");
  }
  if (config.motion === "inward" || config.motion === "outward") {
    frameCount = ringPaths.length;
  }

  for (let i = 0; i < frameCount; i++) {
    const frameMap = new Map();

    if (config.motion === "clockwise" || config.motion === "counterclockwise") {
      const direction = config.motion === "clockwise" ? 1 : -1;
      const trailLength = config.scope === "full_screen" ? 18 : 28;

      for (let layer = 0; layer < ringPaths.length; layer++) {
        if (config.scope === "single_ring" && layer > 0) {
          break;
        }
        const path = ringPaths[layer];
        const pathLength = path.length;
        if (pathLength === 0) {
          continue;
        }
        const headIndex = ((i * speed * direction) + layer * 6) % pathLength;
        const normalizedHead = headIndex < 0 ? headIndex + pathLength : headIndex;
        appendPathTrail(
          frameMap,
          path,
          normalizedHead,
          trailLength,
          colorA,
          colorB,
          config.colorMode,
        );
      }
    } else {
      const maxLayerIndex = ringPaths.length - 1;
      const travelIndex = i % ringPaths.length;
      const activeLayer =
        config.motion === "inward" ? travelIndex : maxLayerIndex - travelIndex;

      if (config.scope === "single_ring") {
        const path = ringPaths[activeLayer];
        const ringColor =
          config.colorMode === "gradient"
            ? blendColor(colorA, colorB, activeLayer / Math.max(maxLayerIndex, 1))
            : colorA;
        fillWholePath(frameMap, path, ringColor);
      } else {
        for (let layer = 0; layer < ringPaths.length; layer++) {
          const shouldDraw =
            config.motion === "inward" ? layer <= activeLayer : layer >= activeLayer;
          if (!shouldDraw) {
            continue;
          }
          const ringColor =
            config.colorMode === "gradient"
              ? blendColor(colorA, colorB, layer / Math.max(maxLayerIndex, 1))
              : colorA;
          fillWholePath(frameMap, ringPaths[layer], ringColor);
        }
      }
    }

    frames.push(frameMapToFrame(frameMap, delay));
  }

  return frames;
}

export function buildRhythmFrames(config) {
  if (!config) {
    throw new Error("律动配置缺失");
  }
  if (config.bpm === undefined || config.speed === undefined || config.loop === undefined || config.direction === undefined || config.strength === undefined || config.mode === undefined || config.colorA === undefined || config.colorB === undefined) {
    throw new Error("律动配置字段不完整");
  }

  const bpm = clampInt(config.bpm, 40, 240);
  const speed = clampInt(config.speed, 1, 10);
  const loop = config.loop === true;
  const strength = clampInt(config.strength, 0, 100) / 100;
  const colorA = hexToRgb(config.colorA);
  const colorB = hexToRgb(config.colorB);

  const frameCount = loop ? 20 : 8;
  const beatMs = 60000 / bpm;
  const delay = Math.max(30, Math.round(beatMs / speed / frameCount * 2));
  const frames = [];

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex++) {
    const phase = frameCount === 1 ? 1 : frameIndex / frameCount;
    const totalPixels = PANEL_SIZE * PANEL_SIZE;
    const pixels = new Uint8Array(totalPixels * 5);
    let idx = 0;

    for (let y = 0; y < PANEL_SIZE; y++) {
      for (let x = 0; x < PANEL_SIZE; x++) {
        const pos = getDirectionPos(config.direction, x, y);
        let alpha;
        if (config.mode === "pulse") {
          alpha = (Math.sin((phase + pos) * Math.PI * 2) + 1) / 2;
        } else if (config.mode === "gradient") {
          alpha = phase + pos;
          if (alpha > 1) {
            alpha = alpha - 1;
          }
        } else if (config.mode === "jump") {
          alpha = phase + pos;
          if (alpha > 1) {
            alpha = alpha - 1;
          }
          alpha = alpha > 0.5 ? 1 : 0;
        } else {
          throw new Error("律动模式参数错误");
        }

        const mixed = blendColor(colorA, colorB, alpha * strength);
        pixels[idx++] = x;
        pixels[idx++] = y;
        pixels[idx++] = mixed.r;
        pixels[idx++] = mixed.g;
        pixels[idx++] = mixed.b;
      }
    }

    frames.push({
      type: 1,
      delay,
      totalPixels,
      pixels,
    });
  }

  return frames;
}

export { hexToRgb };
