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

function scaleColor(color, scale) {
  const safe = clampUnit(scale);
  return {
    r: Math.round(color.r * safe),
    g: Math.round(color.g * safe),
    b: Math.round(color.b * safe),
  };
}

function blendFramePixel(frameMap, x, y, color, alpha = 1) {
  if (x < 0 || x >= PANEL_SIZE || y < 0 || y >= PANEL_SIZE) {
    return;
  }
  const safeAlpha = clampUnit(alpha);
  if (safeAlpha <= 0) {
    return;
  }
  const key = `${x},${y}`;
  const prev = frameMap.get(key);
  if (!prev) {
    frameMap.set(key, scaleColor(color, safeAlpha));
    return;
  }
  frameMap.set(key, blendColor(prev, color, safeAlpha));
}

function paintGlowDot(frameMap, x, y, color, intensity, radius = 1) {
  for (let offsetY = -radius; offsetY <= radius; offsetY++) {
    for (let offsetX = -radius; offsetX <= radius; offsetX++) {
      const distance = Math.abs(offsetX) + Math.abs(offsetY);
      const falloff = radius <= 0 ? 1 : 1 - distance / (radius + 1);
      if (falloff <= 0) {
        continue;
      }
      blendFramePixel(frameMap, x + offsetX, y + offsetY, color, intensity * falloff);
    }
  }
}

function paintPathChase(
  frameMap,
  path,
  headIndex,
  direction,
  trailLength,
  colorA,
  colorB,
  colorMode,
  alphaScale,
) {
  const pathLength = path.length;
  if (pathLength === 0) {
    return;
  }

  for (let step = 0; step < trailLength; step++) {
    let index = headIndex - step * direction;
    while (index < 0) {
      index += pathLength;
    }
    index %= pathLength;
    const point = path[index];
    const trailRatio = trailLength <= 1 ? 1 : step / (trailLength - 1);
    const tail = Math.pow(1 - trailRatio, 1.18);
    const color =
      colorMode === "gradient"
        ? blendColor(colorA, colorB, 0.2 + trailRatio * 0.8)
        : colorA;
    paintGlowDot(frameMap, point[0], point[1], color, alphaScale * tail, step < 2 ? 1 : 0);
  }
}

function paintRingLayer(frameMap, path, color, alphaScale) {
  for (let index = 0; index < path.length; index++) {
    const point = path[index];
    blendFramePixel(frameMap, point[0], point[1], color, alphaScale);
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

function addCrossGlow(frameMap, centerX, centerY, radius, color) {
  for (let offset = -radius; offset <= radius; offset++) {
    setFramePixel(frameMap, centerX + offset, centerY, color);
    setFramePixel(frameMap, centerX, centerY + offset, color);
  }
}

function addDiamond(frameMap, centerX, centerY, radius, color) {
  for (let y = -radius; y <= radius; y++) {
    const rowWidth = radius - Math.abs(y);
    for (let x = -rowWidth; x <= rowWidth; x++) {
      setFramePixel(frameMap, centerX + x, centerY + y, color);
    }
  }
}

function addOrbitDots(frameMap, frameIndex, colorA, colorB) {
  const orbit = [
    [32, 8],
    [44, 12],
    [52, 20],
    [56, 32],
    [52, 44],
    [44, 52],
    [32, 56],
    [20, 52],
    [12, 44],
    [8, 32],
    [12, 20],
    [20, 12],
  ];

  for (let i = 0; i < orbit.length; i++) {
    const point = orbit[(i + frameIndex) % orbit.length];
    const color = i % 2 === 0 ? colorA : colorB;
    setFramePixel(frameMap, point[0], point[1], color);
    setFramePixel(frameMap, point[0] + 1, point[1], color);
  }
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

function wrapUnit(value) {
  const wrapped = value - Math.floor(value);
  return wrapped < 0 ? wrapped + 1 : wrapped;
}

function smoothstep(edge0, edge1, value) {
  if (edge0 === edge1) {
    return value < edge0 ? 0 : 1;
  }
  const x = clampUnit((value - edge0) / (edge1 - edge0));
  return x * x * (3 - 2 * x);
}

function colorLuminance(color) {
  return (color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722) / 255;
}

function isWarmPalette(colorA, colorB) {
  const warmA = colorA.r + colorA.g * 0.4 - colorA.b * 0.2;
  const warmB = colorB.r + colorB.g * 0.4 - colorB.b * 0.2;
  return warmA + warmB > 290;
}

function isMatrixPalette(colorA, colorB) {
  const greenLeadA = colorA.g - Math.max(colorA.r, colorA.b);
  const greenLeadB = colorB.g - Math.max(colorB.r, colorB.b);
  return greenLeadA > 20 || greenLeadB > 20;
}

function columnSeed(index) {
  return wrapUnit(Math.sin((index + 1) * 12.9898) * 43758.5453);
}

function addColorHighlight(color, amount) {
  return blendColor(color, { r: 255, g: 255, b: 255 }, clampUnit(amount));
}

function paintMatrixSweepScreen(frameMap, phase, speed, colorA, colorB, energy) {
  const scanColor = blendColor(colorA, colorB, 0.3);
  const trailColor = blendColor(colorA, colorB, 0.65);

  for (let column = 0; column < 16; column += 1) {
    const seed = columnSeed(column);
    const laneX = column * 4 + 1;
    const travel = wrapUnit(phase * (0.75 + speed * 0.08) + seed);
    const headY = Math.round(travel * 76) - 6;

    for (let trail = 0; trail < 14; trail += 1) {
      const py = headY - trail;
      const alpha = clampUnit((1 - trail / 14) * (0.4 + energy * 0.45));
      if (alpha <= 0) {
        continue;
      }
      const color = trail < 2 ? addColorHighlight(scanColor, 0.32) : trailColor;
      blendFramePixel(frameMap, laneX, py, color, alpha);
      blendFramePixel(frameMap, laneX + 1, py, color, alpha * 0.72);
      if (trail < 4) {
        blendFramePixel(frameMap, laneX + 2, py, color, alpha * 0.2);
      }
    }
  }

  for (let row = 6; row < 64; row += 8) {
    for (let x = 0; x < 64; x += 1) {
      blendFramePixel(frameMap, x, row, scanColor, 0.06);
    }
  }
}

function paintFireScreen(frameMap, phase, speed, colorA, colorB, energy) {
  for (let x = 0; x < 64; x += 1) {
    const waveA = 0.5 + 0.5 * Math.sin(x * 0.22 + phase * Math.PI * (1.8 + speed * 0.06));
    const waveB = 0.5 + 0.5 * Math.sin(x * 0.11 - phase * Math.PI * (2.4 + speed * 0.08));
    const flameHeight = Math.round(10 + waveA * 12 + waveB * 18 + energy * 8);

    for (let depth = 0; depth < flameHeight; depth += 1) {
      const py = 63 - depth;
      const mix = clampUnit(depth / Math.max(flameHeight - 1, 1));
      const bodyColor = blendColor(colorA, colorB, clampUnit(mix * 0.85));
      const glowColor = addColorHighlight(bodyColor, clampUnit(mix * 0.3));
      const alpha = clampUnit(0.22 + mix * 0.7);
      blendFramePixel(frameMap, x, py, glowColor, alpha);
      if (depth < 6 && x < 63) {
        blendFramePixel(frameMap, x + 1, py, bodyColor, alpha * 0.28);
      }
    }

    const sparkGate = wrapUnit(phase * (1.3 + speed * 0.05) + x * 0.17);
    if (sparkGate > 0.82) {
      const sparkY = 40 - Math.round((sparkGate - 0.82) * 70);
      const sparkColor = addColorHighlight(colorB, 0.45);
      paintGlowDot(frameMap, x, sparkY, sparkColor, 0.42, 1);
    }
  }
}

function paintWaveScreen(frameMap, phase, speed, colorA, colorB, energy) {
  const waveCount = 4;
  const crestColor = addColorHighlight(blendColor(colorA, colorB, 0.3), 0.24);
  const troughColor = blendColor(colorA, colorB, 0.72);

  for (let band = 0; band < waveCount; band += 1) {
    const baseY = 12 + band * 12;
    const amplitude = 3 + band + energy * 5;
    const phaseShift = band * 0.65;

    for (let x = 0; x < 64; x += 1) {
      const y =
        baseY +
        Math.sin(x * 0.18 + phase * Math.PI * (1.5 + speed * 0.08) + phaseShift) *
          amplitude +
        Math.cos(x * 0.08 - phase * Math.PI * (2.2 + speed * 0.06) - phaseShift) *
          (1.5 + band * 0.6);
      const py = Math.round(y);
      const mix = clampUnit((band + x / 64) / waveCount);
      const waveColor = blendColor(crestColor, troughColor, mix);
      paintGlowDot(frameMap, x, py, waveColor, 0.28 + energy * 0.22, 1);
      blendFramePixel(frameMap, x, py + 1, troughColor, 0.18 + band * 0.04);
    }
  }

  for (let y = 10; y < 64; y += 12) {
    for (let x = 0; x < 64; x += 1) {
      blendFramePixel(frameMap, x, y, colorA, 0.03);
    }
  }
}

function paintRadarScreen(frameMap, phase, speed, colorA, colorB, energy) {
  const centerX = 32;
  const centerY = 32;
  const sweepAngle = phase * Math.PI * (2.2 + speed * 0.1) - Math.PI / 2;
  const ringColor = blendColor(colorA, colorB, 0.4);
  const headColor = addColorHighlight(colorA, 0.3);

  for (let radius = 8; radius <= 26; radius += 8) {
    for (let degree = 0; degree < 360; degree += 4) {
      const rad = (degree * Math.PI) / 180;
      const px = Math.round(centerX + Math.cos(rad) * radius);
      const py = Math.round(centerY + Math.sin(rad) * radius);
      blendFramePixel(frameMap, px, py, ringColor, 0.12 + radius / 120);
    }
  }

  for (let step = 0; step <= 28; step += 1) {
    const distance = step / 28;
    const px = Math.round(centerX + Math.cos(sweepAngle) * distance * 28);
    const py = Math.round(centerY + Math.sin(sweepAngle) * distance * 28);
    const alpha = clampUnit(0.18 + (1 - distance) * 0.62 + energy * 0.18);
    const sweepColor = blendColor(colorA, colorB, distance * 0.55);
    paintGlowDot(frameMap, px, py, sweepColor, alpha, step > 22 ? 1 : 0);
  }

  const blipSeed = wrapUnit(phase * (1.6 + speed * 0.05));
  const blipAngle = blipSeed * Math.PI * 2;
  const blipRadius = 10 + Math.round(14 * wrapUnit(phase * 1.1 + 0.23));
  const blipX = Math.round(centerX + Math.cos(blipAngle) * blipRadius);
  const blipY = Math.round(centerY + Math.sin(blipAngle) * blipRadius);
  paintGlowDot(frameMap, blipX, blipY, headColor, 0.65, 1);
  paintGlowDot(frameMap, centerX, centerY, headColor, 0.18 + energy * 0.16, 1);
}

function paintRectOutline(frameMap, left, top, size, color, alpha) {
  if (size <= 0) {
    return;
  }
  const right = left + size - 1;
  const bottom = top + size - 1;

  for (let x = left; x <= right; x += 1) {
    blendFramePixel(frameMap, x, top, color, alpha);
    blendFramePixel(frameMap, x, bottom, color, alpha);
  }
  for (let y = top; y <= bottom; y += 1) {
    blendFramePixel(frameMap, left, y, color, alpha);
    blendFramePixel(frameMap, right, y, color, alpha);
  }
}

function paintTunnelScreen(frameMap, phase, speed, colorA, colorB, energy) {
  const layerCount = 7;

  for (let layer = 0; layer < layerCount; layer += 1) {
    const loopPhase = wrapUnit(phase * (1 + speed * 0.04) + layer / layerCount);
    const size = Math.max(8, Math.round(10 + loopPhase * 50));
    const left = Math.round((64 - size) / 2);
    const top = Math.round((64 - size) / 2);
    const mix = clampUnit(layer / Math.max(layerCount - 1, 1));
    const layerColor = blendColor(colorA, colorB, mix);
    const alpha = clampUnit((1 - loopPhase) * 0.5 + energy * 0.18);

    paintRectOutline(frameMap, left, top, size, layerColor, alpha);
    if (size > 14) {
      paintRectOutline(frameMap, left + 1, top + 1, size - 2, addColorHighlight(layerColor, 0.18), alpha * 0.55);
    }
  }

  const glowColor = addColorHighlight(blendColor(colorA, colorB, 0.5), 0.22);
  paintGlowDot(frameMap, 32, 32, glowColor, 0.2 + energy * 0.18, 2);
}

export function buildBreathFrames(config) {
  if (!config) {
    throw new Error("矩阵流光配置缺失");
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
    throw new Error("矩阵流光配置字段不完整");
  }

  const speed = clampInt(config.speed, 1, 10);
  const loop = config.loop === true;
  const colorA = hexToRgb(config.colorA);
  const colorB = hexToRgb(config.colorB);
  const ringPaths = buildAllRingPaths();
  const displayLayers =
    config.scope === "single_ring"
      ? [0, 1, 2]
      : Array.from({ length: 16 }, (_, index) => index * 2);
  let frameCount = loop ? 28 : 14;
  const delay = Math.max(28, 156 - speed * 9);
  const frames = [];
  const warmPalette = isWarmPalette(colorA, colorB);
  const matrixPalette = isMatrixPalette(colorA, colorB);

  if (
    config.motion !== "clockwise" &&
    config.motion !== "counterclockwise" &&
    config.motion !== "inward" &&
    config.motion !== "outward"
  ) {
    throw new Error("矩阵流光运行方式错误");
  }
  if (config.scope !== "single_ring" && config.scope !== "full_screen") {
    throw new Error("矩阵流光范围参数错误");
  }
  if (config.colorMode !== "solid" && config.colorMode !== "gradient") {
    throw new Error("矩阵流光颜色模式错误");
  }
  if (config.motion === "inward" || config.motion === "outward") {
    frameCount = displayLayers.length + 12;
  }

  for (let i = 0; i < frameCount; i++) {
    const frameMap = new Map();
    const phase = frameCount <= 1 ? 0 : i / frameCount;
    const breath = 0.45 + 0.55 * Math.sin(phase * Math.PI * 2 - Math.PI / 2) * 0.5 + 0.275;
    const accentColor = blendColor(colorA, colorB, 0.75);

    if (config.scope === "full_screen" && warmPalette) {
      paintFireScreen(frameMap, phase, speed, colorA, colorB, breath);
      frames.push(frameMapToFrame(frameMap, delay));
      continue;
    }

    if (config.scope === "full_screen" && matrixPalette) {
      paintMatrixSweepScreen(frameMap, phase, speed, colorA, colorB, breath);
      frames.push(frameMapToFrame(frameMap, delay));
      continue;
    }

    if (
      config.scope === "full_screen" &&
      config.motion === "counterclockwise" &&
      config.colorMode === "gradient"
    ) {
      paintWaveScreen(frameMap, phase, speed, colorA, colorB, breath);
      frames.push(frameMapToFrame(frameMap, delay));
      continue;
    }

    if (
      config.scope === "full_screen" &&
      config.motion === "inward" &&
      config.colorMode === "solid"
    ) {
      paintRadarScreen(frameMap, phase, speed, colorA, colorB, breath);
      frames.push(frameMapToFrame(frameMap, delay));
      continue;
    }

    if (
      config.scope === "full_screen" &&
      config.motion === "outward" &&
      config.colorMode === "gradient"
    ) {
      paintTunnelScreen(frameMap, phase, speed, colorA, colorB, breath);
      frames.push(frameMapToFrame(frameMap, delay));
      continue;
    }

    if (config.motion === "clockwise" || config.motion === "counterclockwise") {
      const direction = config.motion === "clockwise" ? 1 : -1;
      for (let layerIndex = 0; layerIndex < displayLayers.length; layerIndex++) {
        const layer = displayLayers[layerIndex];
        const path = ringPaths[layer];
        if (!path || path.length === 0) {
          continue;
        }
        const travel = phase * (1.2 + speed * 0.18) * path.length;
        const headIndex = Math.round(travel + layerIndex * 11) % path.length;
        const trailLength =
          config.scope === "single_ring"
            ? Math.max(18, 34 - layerIndex * 6 + speed * 2)
            : Math.max(10, 24 - layerIndex + speed);
        const layerMix =
          config.scope === "single_ring"
            ? 0.08 + layerIndex * 0.18
            : layerIndex / Math.max(displayLayers.length - 1, 1);
        const ringColor =
          config.colorMode === "gradient"
            ? blendColor(colorA, colorB, layerMix)
            : colorA;
        paintPathChase(
          frameMap,
          path,
          headIndex,
          direction,
          trailLength,
          ringColor,
          accentColor,
          config.colorMode,
          0.46 + breath * 0.34,
        );
        if (config.scope === "full_screen" && layerIndex < 8) {
          paintRingLayer(
            frameMap,
            path,
            ringColor,
            0.04 + (displayLayers.length - layerIndex) / displayLayers.length * 0.05,
          );
        }
      }

      const cornerPulse = 0.18 + breath * 0.24;
      paintGlowDot(frameMap, 4, 4, accentColor, cornerPulse, 1);
      paintGlowDot(frameMap, 59, 4, accentColor, cornerPulse, 1);
      paintGlowDot(frameMap, 4, 59, accentColor, cornerPulse, 1);
      paintGlowDot(frameMap, 59, 59, accentColor, cornerPulse, 1);
      if (config.scope === "single_ring" && config.colorMode === "gradient") {
        addOrbitDots(frameMap, i, colorA, colorB);
        paintGlowDot(frameMap, 32, 32, accentColor, 0.12 + breath * 0.12, 1);
      }
    } else {
      const travel = phase * (displayLayers.length + 4);
      for (let layerIndex = 0; layerIndex < displayLayers.length; layerIndex++) {
        const layer = displayLayers[layerIndex];
        const path = ringPaths[layer];
        if (!path || path.length === 0) {
          continue;
        }
        const frontIndex =
          config.motion === "inward"
            ? travel - layerIndex * 0.85
            : travel - (displayLayers.length - 1 - layerIndex) * 0.85;
        const shell = clampUnit(1 - Math.abs(frontIndex) * 0.42);
        const residual = clampUnit(1 - Math.abs(frontIndex - 2.6) * 0.18);
        const ringAlpha = shell * (0.42 + breath * 0.24) + residual * 0.08;
        if (ringAlpha <= 0.01) {
          continue;
        }
        const ringMix =
          config.colorMode === "gradient"
            ? layerIndex / Math.max(displayLayers.length - 1, 1)
            : 0;
        const ringColor =
          config.colorMode === "gradient"
            ? blendColor(colorA, colorB, ringMix)
            : colorA;
        paintRingLayer(frameMap, path, ringColor, ringAlpha);
        const headIndex = Math.round((phase * (0.9 + speed * 0.16) + layerIndex * 0.09) * path.length) % path.length;
        paintPathChase(
          frameMap,
          path,
          headIndex,
          1,
          Math.max(8, 14 - Math.abs(Math.round(frontIndex))),
          ringColor,
          accentColor,
          config.colorMode,
          ringAlpha * 0.8,
        );
      }
    }

    frames.push(frameMapToFrame(frameMap, delay));
  }

  return frames;
}

export function buildRhythmFrames(config) {
  if (!config) {
    throw new Error("音频频谱配置缺失");
  }
  if (config.bpm === undefined || config.speed === undefined || config.loop === undefined || config.direction === undefined || config.strength === undefined || config.mode === undefined || config.colorA === undefined || config.colorB === undefined) {
    throw new Error("音频频谱配置字段不完整");
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
  const bandCount = 16;
  const axisVertical =
    config.direction === "left" || config.direction === "right";
  const reverseAxis =
    config.direction === "right" || config.direction === "down";
  const backgroundColor = blendColor(colorA, colorB, 0.14 + strength * 0.08);
  const gridColor = scaleColor(blendColor(colorA, colorB, 0.5), 0.12);
  const matrixPalette = isMatrixPalette(colorA, colorB);
  const bandHeights = [];

  function paintGrid(frameMap) {
    for (let index = 0; index < 64; index += 8) {
      for (let offset = 0; offset < 64; offset += 1) {
        blendFramePixel(frameMap, index, offset, gridColor, 0.34);
        blendFramePixel(frameMap, offset, index, gridColor, 0.22);
      }
    }
  }

  function getBandAmplitude(phase, band) {
    const seed = columnSeed(band);
    const beatFlash = 0.5 + 0.5 * Math.sin(phase * Math.PI * (2.1 + speed * 0.08));
    const spectrumWave =
      0.5 +
      0.5 *
        Math.sin(
          phase * Math.PI * (2.3 + speed * 0.07) +
            band * 0.44 +
            seed * 3.1,
        );
    const detailWave =
      0.5 +
      0.5 *
        Math.cos(
          phase * Math.PI * (4.7 + speed * 0.05) +
            band * 0.7 +
            seed * 4.4,
        );
    let amplitude = 0;

    if (config.mode === "pulse") {
      amplitude =
        0.16 + spectrumWave * 0.48 + detailWave * 0.16 + beatFlash * 0.22;
    } else if (config.mode === "gradient") {
      amplitude =
        0.14 +
        spectrumWave * 0.32 +
        detailWave * 0.18 +
        (matrixPalette ? beatFlash * 0.14 : beatFlash * 0.08);
    } else if (config.mode === "jump") {
      const gate = wrapUnit(phase * (1.25 + speed * 0.05) + seed);
      amplitude =
        gate > 0.48
          ? 0.78 + detailWave * 0.16
          : 0.08 + spectrumWave * 0.12;
    } else {
      throw new Error("音频频谱模式参数错误");
    }

    return clampUnit(amplitude * (0.38 + strength * 0.92));
  }

  function getBandPoint(bandIndex, depth, thickness) {
    const origin = 2 + bandIndex * 4;
    return {
      x: axisVertical
        ? origin + thickness
        : reverseAxis
          ? 61 - depth
          : 2 + depth,
      y: axisVertical
        ? reverseAxis
          ? 2 + depth
          : 61 - depth
        : origin + thickness,
    };
  }

  function paintSpectrumBar(frameMap, bandIndex, bandHeight, peakHeight, bandColor, peakColor) {
    const safeHeight = Math.max(1, bandHeight);

    for (let depth = 0; depth < safeHeight; depth += 1) {
      const fillRatio = safeHeight <= 1 ? 1 : depth / (safeHeight - 1);
      const bodyColor = blendColor(
        scaleColor(bandColor, 0.46 + fillRatio * 0.18),
        addColorHighlight(bandColor, 0.2 + fillRatio * 0.34),
        fillRatio,
      );
      const alpha = clampUnit(0.18 + fillRatio * 0.72);
      for (let thickness = 0; thickness < 3; thickness += 1) {
        const point = getBandPoint(bandIndex, depth, thickness);
        blendFramePixel(frameMap, point.x, point.y, bodyColor, alpha);
        if (thickness === 1 && depth >= safeHeight - 2) {
          paintGlowDot(frameMap, point.x, point.y, peakColor, 0.24 + fillRatio * 0.18, 1);
        }
      }
    }

    const safePeak = Math.max(0, peakHeight);
    for (let thickness = 0; thickness < 3; thickness += 1) {
      const point = getBandPoint(bandIndex, safePeak, thickness);
      blendFramePixel(frameMap, point.x, point.y, peakColor, 0.9);
      if (thickness === 1) {
        paintGlowDot(frameMap, point.x, point.y, peakColor, 0.48, 1);
      }
    }

    for (let reflection = 0; reflection < Math.min(6, safeHeight); reflection += 1) {
      const fade = clampUnit(0.22 - reflection * 0.03);
      if (fade <= 0) {
        continue;
      }
      for (let thickness = 0; thickness < 3; thickness += 1) {
        const point = getBandPoint(bandIndex, reflection, thickness);
        blendFramePixel(frameMap, point.x, point.y, peakColor, fade);
      }
    }
  }

  function paintWaterfallBand(
    frameMap,
    bandIndex,
    historyHeights,
    axisVertical,
    reverseAxis,
    colorA,
    colorB,
  ) {
    const laneOrigin = 2 + bandIndex * 4;
    const maxRows = Math.min(18, historyHeights.length);

    for (let row = 0; row < maxRows; row += 1) {
      const historyHeight = historyHeights[row];
      const ratio = clampUnit((historyHeight - 6) / 44);
      const rowColor = blendColor(colorA, colorB, ratio);
      const glowColor = addColorHighlight(rowColor, 0.18 + ratio * 0.24);

      for (let thickness = 0; thickness < 3; thickness += 1) {
        let px = 0;
        let py = 0;

        if (axisVertical) {
          px = laneOrigin + thickness;
          py = reverseAxis ? 6 + row * 3 : 57 - row * 3;
        } else {
          px = reverseAxis ? 57 - row * 3 : 6 + row * 3;
          py = laneOrigin + thickness;
        }

        blendFramePixel(frameMap, px, py, glowColor, 0.2 + ratio * 0.7);
        if (row === 0) {
          paintGlowDot(frameMap, px, py, glowColor, 0.34 + ratio * 0.18, 1);
        }
      }
    }
  }

  function paintJumpColumns(
    frameMap,
    bandIndex,
    bandHeight,
    axisVertical,
    reverseAxis,
    barColor,
    peakColor,
  ) {
    const origin = 2 + bandIndex * 4;
    const safeHeight = Math.max(2, Math.round(bandHeight * 0.72));

    for (let depth = 0; depth < safeHeight; depth += 1) {
      const alpha = depth > safeHeight - 3 ? 0.9 : 0.46;
      for (let thickness = 0; thickness < 3; thickness += 1) {
        const point = {
          x: axisVertical
            ? origin + thickness
            : reverseAxis
              ? 61 - depth
              : 2 + depth,
          y: axisVertical
            ? reverseAxis
              ? 2 + depth
              : 61 - depth
            : origin + thickness,
        };
        blendFramePixel(frameMap, point.x, point.y, barColor, alpha);
        if (depth > safeHeight - 2) {
          paintGlowDot(frameMap, point.x, point.y, peakColor, 0.3, 1);
        }
      }
    }
  }

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const phase = frameCount === 1 ? 0 : frameIndex / frameCount;
    const heights = [];
    for (let band = 0; band < bandCount; band += 1) {
      const amplitude = getBandAmplitude(phase, band);
      heights.push(Math.max(4, Math.round(7 + amplitude * 43)));
    }
    bandHeights.push(heights);
  }

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex++) {
    const phase = frameCount === 1 ? 0 : frameIndex / frameCount;
    const frameMap = new Map();
    const beatFlash =
      0.45 + 0.55 * Math.sin(phase * Math.PI * (2 + speed * 0.08));

    for (let y = 0; y < PANEL_SIZE; y += 1) {
      for (let x = 0; x < PANEL_SIZE; x += 1) {
        const diagonal = (x + y) / ((PANEL_SIZE - 1) * 2);
        const pulse =
          0.5 +
          0.5 *
            Math.sin(
              diagonal * Math.PI * 2.8 -
                phase * Math.PI * (2.4 + speed * 0.08),
            );
        const bg = blendColor(backgroundColor, colorA, pulse * 0.08);
        setFramePixel(frameMap, x, y, bg);
      }
    }

    paintGrid(frameMap);

    for (let band = 0; band < bandCount; band += 1) {
      const seed = columnSeed(band);
      const bandHeight = bandHeights[frameIndex][band];
      let peakHeight = bandHeight;
      for (let lookback = 1; lookback <= 4; lookback += 1) {
        const historyIndex = frameIndex - lookback;
        if (historyIndex < 0) {
          break;
        }
        peakHeight = Math.max(peakHeight, bandHeights[historyIndex][band] - lookback * 2);
      }
      const bandMix =
        config.mode === "gradient"
          ? band / Math.max(bandCount - 1, 1)
          : 0.18 + getBandAmplitude(phase, band) * 0.72;
      const barColor =
        config.mode === "jump"
          ? blendColor(colorA, colorB, (band % 2) * 0.6)
          : blendColor(colorA, colorB, clampUnit(bandMix));
      const glowColor = addColorHighlight(blendColor(barColor, colorB, 0.35), 0.22);

      if (config.mode === "pulse") {
        paintSpectrumBar(
          frameMap,
          band,
          bandHeight,
          peakHeight,
          barColor,
          glowColor,
        );
      } else if (config.mode === "gradient") {
        const historyHeights = [];
        for (let offset = 0; offset < frameCount; offset += 1) {
          const historyIndex = frameIndex - offset;
          if (historyIndex < 0) {
            break;
          }
          historyHeights.push(bandHeights[historyIndex][band]);
        }
        paintWaterfallBand(
          frameMap,
          band,
          historyHeights,
          axisVertical,
          reverseAxis,
          barColor,
          glowColor,
        );
      } else if (config.mode === "jump") {
        paintJumpColumns(
          frameMap,
          band,
          bandHeight,
          axisVertical,
          reverseAxis,
          barColor,
          glowColor,
        );
      }

      if (config.mode === "gradient" && matrixPalette) {
        const travel = wrapUnit(phase * (0.8 + speed * 0.06) + seed);
        const head = Math.round(travel * (PANEL_SIZE + 18)) - 9;
        for (let trail = 0; trail < 10; trail += 1) {
          const fade = clampUnit(1 - trail / 10);
          const px = axisVertical ? 3 + band * 4 : reverseAxis ? 61 - (head - trail) : 2 + head - trail;
          const py = axisVertical ? reverseAxis ? 2 + head - trail : 61 - (head - trail) : 3 + band * 4;
          blendFramePixel(frameMap, px, py, colorB, fade * 0.55);
        }
      }
    }

    const hubColor = blendColor(colorA, colorB, beatFlash * 0.7);
    paintGlowDot(frameMap, 32, 32, hubColor, 0.16 + strength * 0.2, 2);
    if (config.mode === "jump") {
      addCrossGlow(frameMap, 32, 32, 3, scaleColor(hubColor, 0.52));
      addDiamond(frameMap, 32, 32, 2, scaleColor(hubColor, 0.3));
      for (let pulse = 0; pulse < 4; pulse += 1) {
        const radius = 4 + pulse * 4 + Math.round(beatFlash * 2);
        const pulseColor = scaleColor(hubColor, 0.22 - pulse * 0.04);
        if (pulseColor.r <= 0 && pulseColor.g <= 0 && pulseColor.b <= 0) {
          continue;
        }
        for (let offset = -radius; offset <= radius; offset += 1) {
          blendFramePixel(frameMap, 32 + offset, 32 - radius, pulseColor, 0.36);
          blendFramePixel(frameMap, 32 + offset, 32 + radius, pulseColor, 0.36);
          blendFramePixel(frameMap, 32 - radius, 32 + offset, pulseColor, 0.36);
          blendFramePixel(frameMap, 32 + radius, 32 + offset, pulseColor, 0.36);
        }
      }
    }
    if (config.mode === "gradient") {
      const sweepAlpha = smoothstep(0, 1, 0.2 + beatFlash * 0.8) * 0.22;
      const sweepColor = blendColor(colorA, colorB, 0.5);
      for (let offset = 0; offset < 64; offset += 1) {
        const scan = Math.round(wrapUnit(phase + offset / 96) * 63);
        blendFramePixel(frameMap, scan, offset, sweepColor, sweepAlpha * 0.6);
      }
    } else if (config.mode === "pulse") {
      addOrbitDots(
        frameMap,
        frameIndex,
        scaleColor(colorA, 0.5 + beatFlash * 0.18),
        scaleColor(colorB, 0.42 + beatFlash * 0.16),
      );
    }

    frames.push(frameMapToFrame(frameMap, delay));
  }

  return frames;
}

export { hexToRgb };
