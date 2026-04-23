const FRAME_COUNT = 48;
const CANVAS_SIZE = 64;
const PREVIEW_DIAMETER = 52;
const PREVIEW_CENTER = CANVAS_SIZE * 0.5;
const TWO_PI = Math.PI * 2;

const PLANET_PREVIEW_MIN_PIXELS = 12;
const PLANET_PREVIEW_MAX_PIXELS = 5000;
const MAX_PLANET_PREVIEW_SEED = 999999999;
const PLANET_PREVIEW_MIN_SPEED = 1;
const PLANET_PREVIEW_MAX_SPEED = 7;
const PLANET_PREVIEW_PLAYBACK_INTERVAL_MS = 100;
const PLANET_SIZE_OPTIONS = [
  { id: "small", label: "小" },
  { id: "medium", label: "中" },
  { id: "large", label: "大" },
];

const PLANET_DIRECTION_OPTIONS = [
  { id: "left", label: "左转" },
  { id: "right", label: "右转" },
];

const FRAME_DELAY_BY_SPEED = [840, 720, 620, 530, 450, 380, 320, 270, 220, 180];

const PREVIEW_CACHE = new Map();
const STARFIELD_CACHE = new Map();

function rgba(red, green, blue, alpha = 1) {
  return [red, green, blue, alpha];
}

const COLOR_WHITE = rgba(1, 1, 1, 1);
const COLOR_WARM_WHITE = rgba(1, 0.937255, 0.619608, 1);
const COLOR_SPACE_BLUE = rgba(0.14902, 0.258824, 0.501961, 1);
const COLOR_SPACE_TEAL = rgba(0.109804, 0.388235, 0.431373, 1);
const COLOR_SPACE_ROSE = rgba(0.341176, 0.14902, 0.301961, 1);

const SMALL_STAR_PATTERNS = [
  [[0, 0]],
  [
    [0, -1],
    [-1, 0],
    [0, 0],
    [1, 0],
    [0, 1],
  ],
  [
    [0, -2],
    [-2, 0],
    [0, 0],
    [2, 0],
    [0, 2],
  ],
  [
    [-1, -1],
    [1, -1],
    [0, 0],
    [-1, 1],
    [1, 1],
  ],
];

const SPECIAL_STAR_PATTERNS = [
  [
    [0, -2],
    [0, -1],
    [-2, 0],
    [-1, 0],
    [0, 0],
    [1, 0],
    [2, 0],
    [0, 1],
    [0, 2],
  ],
  [
    [-2, -2],
    [-1, -1],
    [0, 0],
    [1, 1],
    [2, 2],
    [-2, 2],
    [-1, 1],
    [1, -1],
    [2, -2],
  ],
  [
    [0, -2],
    [-1, -1],
    [1, -1],
    [-2, 0],
    [0, 0],
    [2, 0],
    [-1, 1],
    [1, 1],
    [0, 2],
  ],
];

const PRESET_DEFINITIONS = {
  terran_wet: {
    id: "terran_wet",
    label: "湿润类地",
    hint: "对应原始生成器里的 Terran Wet，重点看河道、陆地和云层在 64×64 下会不会挤成一团。",
    relativeScale: 1,
  },
  terran_dry: {
    id: "terran_dry",
    label: "干旱类地",
    hint: "对应原始生成器里的 Terran Dry，适合观察高反差干裂地表在小屏上是否仍然清楚。",
    relativeScale: 1,
  },
  islands: {
    id: "islands",
    label: "岛屿星球",
    hint: "对应原始生成器里的 Islands，水面、岛屿边界和云层分离度是这类预览的关键。",
    relativeScale: 1,
  },
  no_atmosphere: {
    id: "no_atmosphere",
    label: "无大气",
    hint: "对应原始生成器里的 No atmosphere，主要看陨坑阴影和球体体积是否还能站住。",
    relativeScale: 1,
  },
  gas_giant_1: {
    id: "gas_giant_1",
    label: "气态巨星 1",
    hint: "对应原始生成器里的 Gas giant 1，能直接观察条带流动和明暗分层是否稳定。",
    relativeScale: 1,
  },
  gas_giant_2: {
    id: "gas_giant_2",
    label: "气态巨星 2",
    hint: "对应原始生成器里的 Gas giant 2，重点验证光环与球体的遮挡关系和厚度感。",
    relativeScale: 3,
  },
  ice_world: {
    id: "ice_world",
    label: "冰世界",
    hint: "对应原始生成器里的 Ice World，冰层、水带和冷色云层是否还能看清层次很关键。",
    relativeScale: 1,
  },
  lava_world: {
    id: "lava_world",
    label: "熔岩世界",
    hint: "对应原始生成器里的 Lava World，最适合判断裂隙亮线在 64×64 下会不会发糊。",
    relativeScale: 1,
  },
  asteroid: {
    id: "asteroid",
    label: "小行星",
    hint: "对应原始生成器里的 Asteroid，这类不规则轮廓最能看出小屏边缘是否干净。",
    relativeScale: 1,
  },
  black_hole: {
    id: "black_hole",
    label: "黑洞",
    hint: "对应原始生成器里的 Black Hole，重点看吸积盘弯曲和高亮边在 64×64 下是否还成立。",
    relativeScale: 2,
  },
  galaxy: {
    id: "galaxy",
    label: "星系",
    hint: "对应原始生成器里的 Galaxy，主要验证旋臂层叠、倾斜和旋转是否还能读出来。",
    relativeScale: 1,
  },
  star: {
    id: "star",
    label: "恒星",
    hint: "对应原始生成器里的 Star，核心、耀斑和外部 blob 的明暗关系是这一类的重点。",
    relativeScale: 2,
  },
};

const PLANET_SCREEN_PRESETS = Object.keys(PRESET_DEFINITIONS).map((key) => {
  const preset = PRESET_DEFINITIONS[key];
  return {
    id: preset.id,
    label: preset.label,
    hint: preset.hint,
  };
});

function createDefaultPlanetPreviewConfig() {
  return {
    preset: "terran_wet",
    pixels: 100,
    size: "medium",
    seed: 20260415,
    colorSeed: 20260415,
    direction: "right",
    speed: 3,
    dither: true,
  };
}

function createRandomPlanetPreviewSeed() {
  return Math.floor(Math.random() * (MAX_PLANET_PREVIEW_SEED + 1));
}

function createRandomPlanetColorSeed() {
  return Math.floor(Math.random() * (MAX_PLANET_PREVIEW_SEED + 1));
}

function clamp(value, min, max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

function clamp01(value) {
  return clamp(value, 0, 1);
}

function fract(value) {
  return value - Math.floor(value);
}

function mod(value, divisor) {
  return value - divisor * Math.floor(value / divisor);
}

function mix(left, right, amount) {
  return left * (1 - amount) + right * amount;
}

function step(edge, value) {
  if (value < edge) {
    return 0;
  }
  return 1;
}

function smoothstep(edge0, edge1, value) {
  if (edge0 === edge1) {
    if (value < edge0) {
      return 0;
    }
    return 1;
  }
  const t = clamp01((value - edge0) / (edge1 - edge0));
  return t * t * (3 - 2 * t);
}

function distance2(x1, y1, x2, y2) {
  const dx = x1 - x2;
  const dy = y1 - y2;
  return Math.sqrt(dx * dx + dy * dy);
}

function rotateUv(x, y, angle) {
  const cosValue = Math.cos(angle);
  const sinValue = Math.sin(angle);
  const dx = x - 0.5;
  const dy = y - 0.5;
  return [
    dx * cosValue - dy * sinValue + 0.5,
    dx * sinValue + dy * cosValue + 0.5,
  ];
}

function spherify(x, y) {
  const centeredX = x * 2 - 1;
  const centeredY = y * 2 - 1;
  const dotValue = centeredX * centeredX + centeredY * centeredY;
  const z = Math.sqrt(Math.max(0, 1 - dotValue));
  const divisor = z + 1;
  return [
    centeredX / divisor * 0.5 + 0.5,
    centeredY / divisor * 0.5 + 0.5,
  ];
}

function blendPixel(buffer, offset, color, alpha) {
  if (alpha <= 0) {
    return;
  }
  const blendAlpha = clamp01(alpha);
  const inverseAlpha = 1 - blendAlpha;
  buffer[offset] = buffer[offset] * inverseAlpha + color[0] * blendAlpha;
  buffer[offset + 1] = buffer[offset + 1] * inverseAlpha + color[1] * blendAlpha;
  buffer[offset + 2] = buffer[offset + 2] * inverseAlpha + color[2] * blendAlpha;
}

function createFrameBuffer() {
  return new Float32Array(CANVAS_SIZE * CANVAS_SIZE * 3);
}

function colorToRgbString(red, green, blue) {
  return `rgb(${Math.round(red * 255)}, ${Math.round(green * 255)}, ${Math.round(blue * 255)})`;
}

function bufferToMap(buffer) {
  const frameMap = new Map();
  for (let y = 0; y < CANVAS_SIZE; y += 1) {
    for (let x = 0; x < CANVAS_SIZE; x += 1) {
      const offset = (y * CANVAS_SIZE + x) * 3;
      const red = clamp01(buffer[offset]);
      const green = clamp01(buffer[offset + 1]);
      const blue = clamp01(buffer[offset + 2]);
      if (red <= 0 && green <= 0 && blue <= 0) {
        continue;
      }
      frameMap.set(`${x},${y}`, colorToRgbString(red, green, blue));
    }
  }
  return frameMap;
}

function hashString(value) {
  let hash = 2166136261;
  for (let index = 0; index < value.length; index += 1) {
    hash ^= value.charCodeAt(index);
    hash = Math.imul(hash, 16777619);
  }
  return hash >>> 0;
}

function createSeededRandom(seed, salt) {
  let state = (seed ^ salt) >>> 0;
  return function nextRandom() {
    state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
    return state / 4294967296;
  };
}

function seededUnit(seed, key) {
  const salt = hashString(key);
  const random = createSeededRandom(seed >>> 0, salt);
  return random();
}

function seededRange(seed, key, min, max) {
  return min + seededUnit(seed, key) * (max - min);
}

function quantizeUv(value, pixels) {
  return Math.floor(value * pixels) / pixels;
}

function buildTiledRand(seed, size, tileX = 1, tileY = 1) {
  const rounded = Math.max(1, Math.round(size));
  return function randValue(x, y) {
    const wrappedX = mod(x, tileX * rounded);
    const wrappedY = mod(y, tileY * rounded);
    return fract(Math.sin(wrappedX * 12.9898 + wrappedY * 78.233) * 15.5453 * seed);
  };
}

function buildUntiledRand(seed) {
  return function randValue(x, y) {
    return fract(Math.sin(x * 12.9898 + y * 78.233) * 15.5453 * seed);
  };
}

function noise2(x, y, randValue) {
  const ix = Math.floor(x);
  const iy = Math.floor(y);
  const fx = fract(x);
  const fy = fract(y);

  const a = randValue(ix, iy);
  const b = randValue(ix + 1, iy);
  const c = randValue(ix, iy + 1);
  const d = randValue(ix + 1, iy + 1);

  const cubicX = fx * fx * (3 - 2 * fx);
  const cubicY = fy * fy * (3 - 2 * fy);

  return mix(a, b, cubicX) + (c - a) * cubicY * (1 - cubicX) + (d - b) * cubicX * cubicY;
}

function fbm2(x, y, randValue, octaves) {
  let value = 0;
  let scale = 0.5;
  let coordX = x;
  let coordY = y;

  for (let index = 0; index < octaves; index += 1) {
    value += noise2(coordX, coordY, randValue) * scale;
    coordX *= 2;
    coordY *= 2;
    scale *= 0.5;
  }

  return value;
}

function circleNoiseCloud(x, y, randValue) {
  const uvY = Math.floor(y);
  const shiftedX = x + uvY * 0.31;
  const fx = fract(shiftedX);
  const fy = fract(y);
  const h = randValue(Math.floor(shiftedX), uvY);
  const dx = fx - 0.25 - h * 0.5;
  const dy = fy - 0.25 - h * 0.5;
  const magnitude = Math.sqrt(dx * dx + dy * dy);
  const radius = h * 0.25;
  return smoothstep(0, radius, magnitude * 0.75);
}

function circleNoiseCrater(x, y, randValue) {
  const uvY = Math.floor(y);
  const shiftedX = x + uvY * 0.31;
  const fx = fract(shiftedX);
  const fy = fract(y);
  const h = randValue(Math.floor(shiftedX), uvY);
  const dx = fx - 0.25 - h * 0.5;
  const dy = fy - 0.25 - h * 0.5;
  const magnitude = Math.sqrt(dx * dx + dy * dy);
  const radius = h * 0.25;
  return smoothstep(radius - 0.1 * radius, radius, magnitude);
}

function cellsHash2(x, y) {
  const r = 523 * Math.sin(x * 53.3158 + y * 43.6143);
  return [fract(15.32354 * r), fract(17.25865 * r)];
}

function cellsValue(x, y, numCells, tiles) {
  const scaledX = x * numCells;
  const scaledY = y * numCells;
  let minDistance = 1.0e10;

  for (let xo = -1; xo <= 1; xo += 1) {
    for (let yo = -1; yo <= 1; yo += 1) {
      const tileX = Math.floor(scaledX) + xo;
      const tileY = Math.floor(scaledY) + yo;
      const hash = cellsHash2(mod(tileX, numCells / tiles), mod(tileY, numCells / tiles));
      const pointX = scaledX - tileX - hash[0];
      const pointY = scaledY - tileY - hash[1];
      const distanceValue = pointX * pointX + pointY * pointY;
      if (distanceValue < minDistance) {
        minDistance = distanceValue;
      }
    }
  }

  return Math.sqrt(minDistance);
}

function useLayerPixels(configPixels, scale = 1) {
  return configPixels * scale;
}

function getLayerRotation(layer) {
  if (layer && typeof layer.rotationOffset === "number") {
    return layer.rotationOffset;
  }
  return 0;
}

function loopTimeFromMultiplier(progress, size, timeSpeed, motionFactor) {
  return progress * motionFactor * Math.max(1, Math.round(size)) * 2 / timeSpeed;
}

function loopTimeFromRotation(progress, timeSpeed, motionFactor) {
  return progress * motionFactor * TWO_PI / timeSpeed;
}

function convertShaderSeed(seed) {
  return mod(seed, 1000) / 100;
}

function rgbToHsv(red, green, blue) {
  const maxValue = Math.max(red, green, blue);
  const minValue = Math.min(red, green, blue);
  const delta = maxValue - minValue;
  let hue = 0;
  if (delta > 0) {
    if (maxValue === red) {
      hue = mod((green - blue) / delta, 6);
    } else if (maxValue === green) {
      hue = (blue - red) / delta + 2;
    } else {
      hue = (red - green) / delta + 4;
    }
    hue /= 6;
  }
  const saturation = maxValue === 0 ? 0 : delta / maxValue;
  return [hue, saturation, maxValue];
}

function hsvToRgb(hue, saturation, value) {
  const normalizedHue = mod(hue, 1);
  const segment = normalizedHue * 6;
  const chroma = value * saturation;
  const xValue = chroma * (1 - Math.abs(mod(segment, 2) - 1));
  const match = value - chroma;
  let red = 0;
  let green = 0;
  let blue = 0;

  if (segment < 1) {
    red = chroma;
    green = xValue;
  } else if (segment < 2) {
    red = xValue;
    green = chroma;
  } else if (segment < 3) {
    green = chroma;
    blue = xValue;
  } else if (segment < 4) {
    green = xValue;
    blue = chroma;
  } else if (segment < 5) {
    red = xValue;
    blue = chroma;
  } else {
    red = chroma;
    blue = xValue;
  }

  return [red + match, green + match, blue + match];
}

function buildColorVariant(frame) {
  const colorSeed = frame.config.colorSeed >>> 0;
  const prefix = `${frame.preset.id}_color_variant`;
  return {
    hueShift: seededRange(colorSeed, `${prefix}_hue`, -0.32, 0.32),
    saturationScale: seededRange(colorSeed, `${prefix}_sat`, 0.68, 1.42),
    valueScale: seededRange(colorSeed, `${prefix}_val`, 0.78, 1.26),
    tintMix: seededRange(colorSeed, `${prefix}_mix`, 0.04, 0.26),
    tint: [
      seededRange(colorSeed, `${prefix}_r`, 0.2, 1),
      seededRange(colorSeed, `${prefix}_g`, 0.2, 1),
      seededRange(colorSeed, `${prefix}_b`, 0.2, 1),
    ],
  };
}

function applyColorVariant(buffer, frame) {
  const variant = buildColorVariant(frame);
  for (let offset = 0; offset < buffer.length; offset += 3) {
    const red = buffer[offset];
    const green = buffer[offset + 1];
    const blue = buffer[offset + 2];
    if (red <= 0 && green <= 0 && blue <= 0) {
      continue;
    }
    const hsv = rgbToHsv(red, green, blue);
    const remapped = hsvToRgb(
      hsv[0] + variant.hueShift,
      clamp01(hsv[1] * variant.saturationScale),
      clamp01(hsv[2] * variant.valueScale),
    );
    buffer[offset] = clamp01(mix(remapped[0], variant.tint[0], variant.tintMix));
    buffer[offset + 1] = clamp01(mix(remapped[1], variant.tint[1], variant.tintMix));
    buffer[offset + 2] = clamp01(mix(remapped[2], variant.tint[2], variant.tintMix));
  }
}

function renderPresetFrame(buffer, frame) {
  renderPresetToBuffer(buffer, frame);
  applyColorVariant(buffer, frame);
  renderBackgroundStars(buffer, frame);
}

function createLayerIterator(relativeScale, planeScale, sizeScale, callback) {
  const planeSize = PREVIEW_DIAMETER * sizeScale * planeScale / relativeScale;
  const inversePlaneSize = 1 / planeSize;

  for (let y = 0; y < CANVAS_SIZE; y += 1) {
    const v = (y + 0.5 - PREVIEW_CENTER) * inversePlaneSize + 0.5;
    if (v < 0 || v > 1) {
      continue;
    }

    for (let x = 0; x < CANVAS_SIZE; x += 1) {
      const u = (x + 0.5 - PREVIEW_CENTER) * inversePlaneSize + 0.5;
      if (u < 0 || u > 1) {
        continue;
      }
      const offset = (y * CANVAS_SIZE + x) * 3;
      callback(offset, u, v, x, y);
    }
  }
}

function getSizeScale(sizeId) {
  if (sizeId === "small") {
    return 0.82;
  }
  if (sizeId === "large") {
    return 1.15;
  }
  return 1;
}

function getBackgroundStarExclusionRadius(frame) {
  if (frame.preset.id === "galaxy") {
    return 30;
  }
  if (frame.preset.id === "star") {
    return 28;
  }
  if (frame.preset.id === "black_hole") {
    return 29;
  }

  let radius = PREVIEW_DIAMETER * frame.sizeScale * 0.5 + 5;
  if (frame.preset.id === "gas_giant_2") {
    radius += 4;
  }
  return clamp(radius, 22, 30);
}

function getDirectionFactor(directionId) {
  if (directionId === "left") {
    return -1;
  }
  return 1;
}

function getSpeedMultiplier(speedId) {
  const numericSpeed = Number(speedId);
  if (!Number.isFinite(numericSpeed)) {
    return 1;
  }
  return clamp(
    Math.round(numericSpeed),
    PLANET_PREVIEW_MIN_SPEED,
    PLANET_PREVIEW_MAX_SPEED,
  );
}

function getFrameDelay(speedId) {
  const normalizedSpeed = getSpeedMultiplier(speedId);
  return FRAME_DELAY_BY_SPEED[normalizedSpeed - 1];
}

function getPlanetPreviewCycleDuration(speedId) {
  return getFrameDelay(speedId) * (FRAME_COUNT - 1);
}

function renderBackgroundStars(buffer, frame) {
  const cacheKey = `${frame.config.seed}:${frame.preset.id}:${frame.config.size}`;
  let background = STARFIELD_CACHE.get(cacheKey);
  if (!background) {
    background = [];
    const random = createSeededRandom(
      frame.config.seed >>> 0,
      hashString("planet_starfield_sparse"),
    );
    const exclusionRadius = getBackgroundStarExclusionRadius(frame);
    let attempts = 0;
    while (background.length < 10 && attempts < 400) {
      attempts += 1;
      const x = Math.floor(random() * CANVAS_SIZE);
      const y = Math.floor(random() * CANVAS_SIZE);
      const dx = x - PREVIEW_CENTER;
      const dy = y - PREVIEW_CENTER;
      if (Math.sqrt(dx * dx + dy * dy) < exclusionRadius) {
        continue;
      }

      background.push({
        x,
        y,
        color: random() > 0.45 ? COLOR_WHITE : COLOR_WARM_WHITE,
        alpha: 0.44 + random() * 0.24,
      });
    }

    STARFIELD_CACHE.set(cacheKey, background);
  }

  for (let index = 0; index < background.length; index += 1) {
    const star = background[index];
    blendPixel(
      buffer,
      (star.y * CANVAS_SIZE + star.x) * 3,
      star.color,
      star.alpha,
    );
  }
}

function renderPlanetUnder(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const shaderSeed = frame.shaderSeed;
  const randValue = buildTiledRand(shaderSeed, layer.size, 2, 1);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dith = mod(pixelU + v, 2 / pixels) <= 1 / pixels;
    const dLightBase = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const dCircle = distance2(pixelU, pixelV, 0.5, 0.5);
    const alphaCircle = step(dCircle, 0.49999);
    if (alphaCircle === 0) {
      return;
    }

    const sphere = spherify(pixelU, pixelV);
    const rotated = rotateUv(sphere[0], sphere[1], getLayerRotation(layer));
    let dLight = dLightBase;
    dLight += fbm2(
      rotated[0] * layer.size + time * layer.timeSpeed,
      rotated[1] * layer.size,
      randValue,
      layer.octaves,
    ) * 0.3;

    const ditherBorder = 1 / pixels * layer.ditherSize;
    let color = layer.colors[0];
    if (dLight > layer.lightBorder1) {
      color = layer.colors[1];
      if (
        dLight < layer.lightBorder1 + ditherBorder &&
        (dith || frame.config.dither === false)
      ) {
        color = layer.colors[0];
      }
    }
    if (dLight > layer.lightBorder2) {
      color = layer.colors[2];
      if (
        dLight < layer.lightBorder2 + ditherBorder &&
        (dith || frame.config.dither === false)
      ) {
        color = layer.colors[1];
      }
    }

    blendPixel(buffer, offset, color, alphaCircle * color[3]);
  });
}

function renderLandMass(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size, 2, 1);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dLightBase = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const dCircle = distance2(pixelU, pixelV, 0.5, 0.5);
    const alphaCircle = step(dCircle, 0.49999);
    if (alphaCircle === 0) {
      return;
    }

    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const sphere = spherify(rotated[0], rotated[1]);
    const baseX = sphere[0] * layer.size + time * layer.timeSpeed;
    const baseY = sphere[1] * layer.size;

    const fbm1 = fbm2(baseX, baseY, randValue, layer.octaves);
    const fbm2Value = fbm2(
      baseX - layer.lightOrigin[0] * fbm1,
      baseY - layer.lightOrigin[1] * fbm1,
      randValue,
      layer.octaves,
    );
    const fbm3Value = fbm2(
      baseX - layer.lightOrigin[0] * 1.5 * fbm1,
      baseY - layer.lightOrigin[1] * 1.5 * fbm1,
      randValue,
      layer.octaves,
    );
    let fbm4Value = fbm2(
      baseX - layer.lightOrigin[0] * 2.0 * fbm1,
      baseY - layer.lightOrigin[1] * 2.0 * fbm1,
      randValue,
      layer.octaves,
    );
    let fbm2Adjusted = fbm2Value;
    let fbm3Adjusted = fbm3Value;

    if (dLightBase < layer.lightBorder1) {
      fbm4Value *= 0.9;
    }
    if (dLightBase > layer.lightBorder1) {
      fbm2Adjusted *= 1.05;
      fbm3Adjusted *= 1.05;
      fbm4Value *= 1.05;
    }
    if (dLightBase > layer.lightBorder2) {
      fbm2Adjusted *= 1.3;
      fbm3Adjusted *= 1.4;
      fbm4Value *= 1.8;
    }

    const lightValue = Math.pow(dLightBase, 2) * 0.1;
    let color = layer.colors[3];
    if (fbm4Value + lightValue < fbm1) {
      color = layer.colors[2];
    }
    if (fbm3Adjusted + lightValue < fbm1) {
      color = layer.colors[1];
    }
    if (fbm2Adjusted + lightValue < fbm1) {
      color = layer.colors[0];
    }

    const alpha = step(layer.landCutoff, fbm1) * alphaCircle;
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderClouds(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);
  const timeOffset = time * layer.timeSpeed;

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dLight = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const dCenter = distance2(pixelU, pixelV, 0.5, 0.5);
    const alphaCircle = step(dCenter, 0.49999);
    if (alphaCircle === 0) {
      return;
    }

    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const sphere = spherify(rotated[0], rotated[1]);
    let sphereY = sphere[1] + smoothstep(0, layer.cloudCurve, Math.abs(sphere[0] - 0.4));
    sphereY *= layer.stretch;

    let cloudNoise = 0;
    for (let index = 0; index < 9; index += 1) {
      cloudNoise += circleNoiseCloud(
        sphere[0] * layer.size * 0.3 + index + 11 + timeOffset,
        sphereY * layer.size * 0.3,
        randValue,
      );
    }

    const cloudValue = fbm2(
      sphere[0] * layer.size + cloudNoise + timeOffset,
      sphereY * layer.size,
      randValue,
      layer.octaves,
    );

    let color = layer.colors[0];
    if (cloudValue < layer.cloudCover + 0.03) {
      color = layer.colors[1];
    }
    if (dLight + cloudValue * 0.2 > layer.lightBorder1) {
      color = layer.colors[2];
    }
    if (dLight + cloudValue * 0.2 > layer.lightBorder2) {
      color = layer.colors[3];
    }

    const alpha = step(layer.cloudCover, cloudValue) * alphaCircle * step(dCenter, 0.5);
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderGasClouds(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);
  const timeOffset = time * layer.timeSpeed;

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dLight = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const dCircle = distance2(pixelU, pixelV, 0.5, 0.5);
    const alphaCircle = step(dCircle, 0.49999);
    if (alphaCircle === 0) {
      return;
    }

    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const sphere = spherify(rotated[0], rotated[1]);
    let sphereY = sphere[1] + smoothstep(0, layer.cloudCurve, Math.abs(sphere[0] - 0.4));
    sphereY *= layer.stretch;

    let cloudNoise = 0;
    for (let index = 0; index < 9; index += 1) {
      cloudNoise += circleNoiseCloud(
        sphere[0] * layer.size * 0.3 + index + 11 + timeOffset,
        sphereY * layer.size * 0.3,
        randValue,
      );
    }

    const cloudValue = fbm2(
      sphere[0] * layer.size + cloudNoise + timeOffset,
      sphereY * layer.size,
      randValue,
      layer.octaves,
    );

    let color = layer.colors[0];
    if (cloudValue < layer.cloudCover + 0.03) {
      color = layer.colors[1];
    }
    if (dLight + cloudValue * 0.2 > layer.lightBorder1) {
      color = layer.colors[2];
    }
    if (dLight + cloudValue * 0.2 > layer.lightBorder2) {
      color = layer.colors[3];
    }

    const alpha = step(layer.cloudCover, cloudValue) * alphaCircle;
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderIceLakes(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size, 2, 1);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dLight = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const dCircle = distance2(rotated[0], rotated[1], 0.5, 0.5);
    if (dCircle > 0.5) {
      return;
    }
    const sphere = spherify(rotated[0], rotated[1]);
    const lake = fbm2(
      sphere[0] * layer.size + time * layer.timeSpeed,
      sphere[1] * layer.size,
      randValue,
      layer.octaves,
    );

    let adjustedLight = Math.pow(dLight, 2) * 0.4;
    adjustedLight -= adjustedLight * lake;

    let color = layer.colors[0];
    if (adjustedLight > layer.lightBorder1) {
      color = layer.colors[1];
    }
    if (adjustedLight > layer.lightBorder2) {
      color = layer.colors[2];
    }

    const alpha = step(layer.lakeCutoff, lake);
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderNoAtmosphereBase(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dCircle = distance2(pixelU, pixelV, 0.5, 0.5);
    const dLightBase = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const alphaCircle = step(dCircle, 0.49999);
    if (alphaCircle === 0) {
      return;
    }

    const dith = mod(pixelU + v, 2 / pixels) <= 1 / pixels;
    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const fbm1 = fbm2(rotated[0], rotated[1], randValue, layer.octaves);
    let dLight = dLightBase;
    dLight += fbm2(
      rotated[0] * layer.size + fbm1 + time * layer.timeSpeed,
      rotated[1] * layer.size,
      randValue,
      layer.octaves,
    ) * 0.3;

    const ditherBorder = 1 / pixels * layer.ditherSize;
    let color = layer.colors[0];
    if (dLight > layer.lightBorder1) {
      color = layer.colors[1];
      if (
        dLight < layer.lightBorder1 + ditherBorder &&
        (dith || frame.config.dither === false)
      ) {
        color = layer.colors[0];
      }
    }
    if (dLight > layer.lightBorder2) {
      color = layer.colors[2];
      if (
        dLight < layer.lightBorder2 + ditherBorder &&
        (dith || frame.config.dither === false)
      ) {
        color = layer.colors[1];
      }
    }

    blendPixel(buffer, offset, color, alphaCircle * color[3]);
  });
}

function renderCraters(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);
  const timeOffset = time * layer.timeSpeed;

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dCircle = distance2(pixelU, pixelV, 0.5, 0.5);
    const dLight = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    let alpha = step(dCircle, 0.49999);
    if (alpha === 0) {
      return;
    }

    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const sphere = spherify(rotated[0], rotated[1]);
    let craterOne = 1;
    let craterTwo = 1;
    for (let index = 0; index < 2; index += 1) {
      craterOne *= circleNoiseCrater(
        sphere[0] * layer.size + index + 11 + timeOffset,
        sphere[1] * layer.size,
        randValue,
      );
      craterTwo *= circleNoiseCrater(
        (sphere[0] + (layer.lightOrigin[0] - 0.5) * layer.offset) * layer.size + index + 11 + timeOffset,
        (sphere[1] + (layer.lightOrigin[1] - 0.5) * layer.offset) * layer.size,
        randValue,
      );
    }
    craterOne = 1 - craterOne;
    craterTwo = 1 - craterTwo;

    alpha *= step(0.5, craterOne);
    if (alpha === 0) {
      return;
    }

    let color = layer.colors[0];
    if (craterTwo < craterOne - (0.5 - dLight) * 2) {
      color = layer.colors[1];
    }
    if (dLight > layer.lightBorder) {
      color = layer.colors[1];
    }

    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderLavaRivers(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size, 2, 1);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dLight = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const dCircle = distance2(pixelU, pixelV, 0.5, 0.5);
    let alpha = step(dCircle, 0.49999);
    if (alpha === 0) {
      return;
    }

    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const sphere = spherify(rotated[0], rotated[1]);
    const fbm1 = fbm2(
      sphere[0] * layer.size + time * layer.timeSpeed,
      sphere[1] * layer.size,
      randValue,
      layer.octaves,
    );
    const river = step(
      layer.riverCutoff,
      fbm2(sphere[0] + fbm1 * 2.5, sphere[1] + fbm1 * 2.5, randValue, layer.octaves),
    );

    let color = layer.colors[0];
    if (dLight > layer.lightBorder1) {
      color = layer.colors[1];
    }
    if (dLight > layer.lightBorder2) {
      color = layer.colors[2];
    }

    alpha *= step(layer.riverCutoff, river);
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderDryTerran(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size, 2, 1);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dith = mod(pixelU + v, 2 / pixels) <= 1 / pixels;
    const dCircle = distance2(pixelU, pixelV, 0.5, 0.5);
    const alphaCircle = step(dCircle, 0.49999);
    if (alphaCircle === 0) {
      return;
    }

    const sphere = spherify(pixelU, pixelV);
    const dLightBase = smoothstep(-0.3, 1.2, distance2(sphere[0], sphere[1], layer.lightOrigin[0], layer.lightOrigin[1]));
    const rotated = rotateUv(sphere[0], sphere[1], getLayerRotation(layer));
    const noiseValue = fbm2(
      rotated[0] * layer.size + time * layer.timeSpeed,
      rotated[1] * layer.size,
      randValue,
      layer.octaves,
    );

    let dLight = dLightBase;
    if (dLight < layer.lightDistance1) {
      dLight *= 0.9;
    }
    if (dLight < layer.lightDistance2) {
      dLight *= 0.9;
    }

    let colorValue = dLight * Math.pow(noiseValue, 0.8) * 3.5;
    if (dith || frame.config.dither === false) {
      colorValue += 0.02;
      colorValue *= 1.05;
    }

    let posterize = Math.floor(colorValue * 4) / 4;
    if (posterize > 1) {
      posterize = 1;
    }
    const colorIndex = clamp(Math.round(posterize * (layer.colors.length - 1)), 0, layer.colors.length - 1);
    const color = layer.colors[colorIndex];
    blendPixel(buffer, offset, color, alphaCircle * color[3]);
  });
}

function renderLandRivers(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size, 2, 1);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dith = mod(pixelU + v, 2 / pixels) <= 1 / pixels;
    const alphaCircle = step(distance2(pixelU, pixelV, 0.5, 0.5), 0.49999);
    if (alphaCircle === 0) {
      return;
    }

    const sphere = spherify(pixelU, pixelV);
    const dLight = distance2(sphere[0], sphere[1], layer.lightOrigin[0], layer.lightOrigin[1]);
    const rotated = rotateUv(sphere[0], sphere[1], getLayerRotation(layer));
    const baseX = rotated[0] * layer.size + time * layer.timeSpeed;
    const baseY = rotated[1] * layer.size;
    const fbm1 = fbm2(baseX, baseY, randValue, layer.octaves);
    let fbm2Value = fbm2(
      baseX - layer.lightOrigin[0] * fbm1,
      baseY - layer.lightOrigin[1] * fbm1,
      randValue,
      layer.octaves,
    );
    let fbm3Value = fbm2(
      baseX - layer.lightOrigin[0] * 1.5 * fbm1,
      baseY - layer.lightOrigin[1] * 1.5 * fbm1,
      randValue,
      layer.octaves,
    );
    let fbm4Value = fbm2(
      baseX - layer.lightOrigin[0] * 2.0 * fbm1,
      baseY - layer.lightOrigin[1] * 2.0 * fbm1,
      randValue,
      layer.octaves,
    );
    const riverValue = step(
      layer.riverCutoff,
      fbm2(baseX + fbm1 * 6, baseY + fbm1 * 6, randValue, layer.octaves),
    );

    const ditherBorder = 1 / pixels * layer.ditherSize;
    if (dLight < layer.lightBorder1) {
      fbm4Value *= 0.9;
    }
    if (dLight > layer.lightBorder1) {
      fbm2Value *= 1.05;
      fbm3Value *= 1.05;
      fbm4Value *= 1.05;
    }
    if (dLight > layer.lightBorder2) {
      fbm2Value *= 1.3;
      fbm3Value *= 1.4;
      fbm4Value *= 1.8;
      if (dLight < layer.lightBorder2 + ditherBorder && (dith || frame.config.dither === false)) {
        fbm4Value *= 0.5;
      }
    }

    const lightValue = Math.pow(dLight, 2) * 0.4;
    let color = layer.colors[3];
    if (fbm4Value + lightValue < fbm1 * 1.5) {
      color = layer.colors[2];
    }
    if (fbm3Value + lightValue < fbm1) {
      color = layer.colors[1];
    }
    if (fbm2Value + lightValue < fbm1) {
      color = layer.colors[0];
    }
    if (riverValue < fbm1 * 0.5) {
      color = layer.colors[5];
      if (fbm4Value + lightValue < fbm1 * 1.5) {
        color = layer.colors[4];
      }
    }

    blendPixel(buffer, offset, color, alphaCircle * color[3]);
  });
}

function renderDenseGas(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size, 2, 1);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);
  const timeOffset = time * layer.timeSpeed;

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const lightDistance = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const dith = mod(pixelU + v, 2 / pixels) <= 1 / pixels;
    const alphaCircle = step(distance2(pixelU, pixelV, 0.5, 0.5), 0.49999);
    if (alphaCircle === 0) {
      return;
    }

    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const sphere = spherify(rotated[0], rotated[1]);
    const band = fbm2(0, sphere[1] * layer.size * layer.bands, randValue, layer.octaves);

    let turbulence = 0;
    for (let index = 0; index < 10; index += 1) {
      turbulence += circleNoiseCloud(
        sphere[0] * layer.size * 0.3 + index + 11 + timeOffset,
        sphere[1] * layer.size * 0.3,
        randValue,
      );
    }

    const fbm1 = fbm2(sphere[0] * layer.size, sphere[1] * layer.size, randValue, layer.octaves);
    let fbm2Value = fbm2(
      sphere[0] * layer.size + fbm1 - timeOffset + turbulence,
      sphere[1] * layer.size * 2,
      randValue,
      layer.octaves,
    );
    fbm2Value *= Math.pow(band, 2) * 7;
    const lightValue = fbm2Value + lightDistance * 1.8;
    fbm2Value += Math.pow(lightDistance, 1) - 0.3;
    fbm2Value = smoothstep(-0.2, 4 - fbm2Value, lightValue);

    if (dith && frame.config.dither) {
      fbm2Value *= 1.1;
    }

    const posterized = Math.floor(fbm2Value * 4) / 2;
    let color;
    if (fbm2Value < 0.625) {
      const index = clamp(Math.round(posterized * (layer.colors.length - 1)), 0, layer.colors.length - 1);
      color = layer.colors[index];
    } else {
      const index = clamp(
        Math.round((posterized - 1) * (layer.darkColors.length - 1)),
        0,
        layer.darkColors.length - 1,
      );
      color = layer.darkColors[index];
    }

    blendPixel(buffer, offset, color, alphaCircle * color[3]);
  });
}

function renderRing(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size, 2, 1);
  const time = loopTimeFromRotation(frame.progress, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const lightDistance = distance2(pixelU, pixelV, layer.lightOrigin[0], layer.lightOrigin[1]);
    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    let centerX = rotated[0];
    let centerY = rotated[1] - 0.5;
    centerY *= layer.ringPerspective;
    const centerDistance = distance2(centerX, centerY, 0.5, 0);

    let ringValue = smoothstep(0.5 - layer.ringWidth * 2, 0.5 - layer.ringWidth, centerDistance);
    ringValue *= smoothstep(centerDistance - layer.ringWidth, centerDistance, 0.4);

    if (rotated[1] < 0.5) {
      ringValue *= step(1 / layer.scaleRelToPlanet, distance2(rotated[0], rotated[1], 0.5, 0.5));
    }

    const material = rotateUv(centerX, centerY + 0.5, time * layer.timeSpeed);
    ringValue *= fbm2(material[0] * layer.size, material[1] * layer.size, randValue, layer.octaves);

    const posterized = Math.min(2, Math.floor((ringValue + Math.pow(lightDistance, 2) * 2) * 4) / 4);
    let color;
    if (posterized <= 1) {
      const index = clamp(Math.round(posterized * (layer.colors.length - 1)), 0, layer.colors.length - 1);
      color = layer.colors[index];
    } else {
      const index = clamp(
        Math.round((posterized - 1) * (layer.darkColors.length - 1)),
        0,
        layer.darkColors.length - 1,
      );
      color = layer.darkColors[index];
    }

    const alpha = step(0.28, ringValue);
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderAsteroid(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildUntiledRand(frame.shaderSeed);
  const rotation = frame.spinAngle + getLayerRotation(layer);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dith = mod(pixelU + v, 2 / pixels) <= 1 / pixels;
    const distanceFromCenter = distance2(pixelU, pixelV, 0.5, 0.5);
    const rotated = rotateUv(pixelU, pixelV, rotation);
    const rotatedLight = rotateUv(layer.lightOrigin[0], layer.lightOrigin[1], rotation);
    const n = fbm2(rotated[0] * layer.size, rotated[1] * layer.size, randValue, layer.octaves);
    const n2 = fbm2(
      rotated[0] * layer.size + (rotatedLight[0] - 0.5) * 0.5,
      rotated[1] * layer.size + (rotatedLight[1] - 0.5) * 0.5,
      randValue,
      layer.octaves,
    );
    const nStep = step(0.2, n - distanceFromCenter);
    if (nStep === 0) {
      return;
    }
    const n2Step = step(0.2, n2 - distanceFromCenter);
    const noiseRelation = n2Step + n2 - (nStep + n);

    let craterOne = 1;
    let craterTwo = 1;
    for (let index = 0; index < 2; index += 1) {
      craterOne *= circleNoiseCrater(rotated[0] * layer.size + index + 11, rotated[1] * layer.size, randValue);
      craterTwo *= circleNoiseCrater(
        (rotated[0] + (layer.lightOrigin[0] - 0.5) * 0.03) * layer.size + index + 11,
        (rotated[1] + (layer.lightOrigin[1] - 0.5) * 0.03) * layer.size,
        randValue,
      );
    }
    craterOne = 1 - craterOne;
    craterTwo = 1 - craterTwo;

    let color = layer.colors[1];
    if (noiseRelation < -0.06 || (noiseRelation < -0.04 && (dith || frame.config.dither === false))) {
      color = layer.colors[0];
    }
    if (noiseRelation > 0.05 || (noiseRelation > 0.03 && (dith || frame.config.dither === false))) {
      color = layer.colors[2];
    }
    if (craterOne > 0.4) {
      color = layer.colors[1];
    }
    if (craterTwo < craterOne) {
      color = layer.colors[2];
    }

    blendPixel(buffer, offset, color, nStep * color[3]);
  });
}

function renderStarCore(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const time = frame.progress * frame.motionFactor / layer.timeSpeed;

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const alphaCircle = step(distance2(pixelU, pixelV, 0.5, 0.5), 0.49999);
    if (alphaCircle === 0) {
      return;
    }
    const dith = mod(u + pixelV, 2 / pixels) <= 1 / pixels;

    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const sphere = spherify(rotated[0], rotated[1]);
    let cellValue = cellsValue(sphere[0] - time * layer.timeSpeed * 2, sphere[1], 10, layer.tiles);
    cellValue *= cellsValue(sphere[0] - time * layer.timeSpeed, sphere[1], 20, layer.tiles);
    cellValue *= 2;
    cellValue = clamp01(cellValue);
    if (dith || frame.config.dither === false) {
      cellValue *= 1.3;
    }

    const interpolate = Math.floor(cellValue * (layer.colors.length - 1)) / (layer.colors.length - 1);
    const colorIndex = clamp(
      Math.round(interpolate * (layer.colors.length - 1)),
      0,
      layer.colors.length - 1,
    );
    const color = layer.colors[colorIndex];
    blendPixel(buffer, offset, color, alphaCircle * color[3]);
  });
}

function renderStarBlobs(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const angle = Math.atan2(rotated[0] - 0.5, rotated[1] - 0.5);
    const distanceFromCenter = distance2(pixelU, pixelV, 0.5, 0.5);

    let value = 0;
    for (let index = 0; index < 15; index += 1) {
      const randomValue = randValue(index, 0);
      const circleUvX = distanceFromCenter * layer.size - time * layer.timeSpeed - 0.1 / Math.max(distanceFromCenter, 0.0001) + randomValue;
      const circleUvY = angle * layer.size;

      const invert = 1 / layer.circleAmount;
      let adjustedX = circleUvX;
      if (mod(circleUvY, invert * 2) < invert) {
        adjustedX += invert * 0.5;
      }
      const randCoordX = Math.floor(adjustedX * layer.circleAmount) / layer.circleAmount;
      const randCoordY = Math.floor(circleUvY * layer.circleAmount) / layer.circleAmount;
      const localX = mod(adjustedX, invert) * layer.circleAmount;
      const localY = mod(circleUvY, invert) * layer.circleAmount;
      let radius = randValue(randCoordX, randCoordY);
      radius = clamp(radius, invert, 1 - invert);
      const circleDistance = distance2(localX, localY, radius, radius);
      const circleAlpha = smoothstep(
        circleDistance,
        circleDistance + 0.5,
        invert * layer.circleSize * randValue(randCoordX * 1.5, randCoordY * 1.5),
      );
      value += circleAlpha;
    }

    value *= 0.37 - distanceFromCenter;
    value = step(0.07, value - distanceFromCenter);
    blendPixel(buffer, offset, layer.color, value * layer.color[3]);
  });
}

function renderStarFlares(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size);
  const time = loopTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dith = mod(u + pixelV, 2 / pixels) <= 1 / pixels;
    const rotated = rotateUv(pixelU, pixelV, getLayerRotation(layer));
    const angle = Math.atan2(rotated[0] - 0.5, rotated[1] - 0.5) * 0.4;
    const distanceFromCenter = distance2(pixelU, pixelV, 0.5, 0.5);
    const circleUvX = distanceFromCenter;
    const circleUvY = angle;

    const n = fbm2(circleUvX * layer.size - time * layer.timeSpeed, circleUvY * layer.size, randValue, layer.octaves);

    const invert = 1 / layer.circleAmount;
    let adjustedX = circleUvX * layer.scale - time * layer.timeSpeed + n;
    const adjustedY = circleUvY * layer.scale;
    if (mod(adjustedY, invert * 2) < invert) {
      adjustedX += invert * 0.5;
    }
    const randCoordX = Math.floor(adjustedX * layer.circleAmount) / layer.circleAmount;
    const randCoordY = Math.floor(adjustedY * layer.circleAmount) / layer.circleAmount;
    const localX = mod(adjustedX, invert) * layer.circleAmount;
    const localY = mod(adjustedY, invert) * layer.circleAmount;
    let radius = randValue(randCoordX, randCoordY);
    radius = clamp(radius, invert, 1 - invert);
    const circleDistance = distance2(localX, localY, radius, radius);
    let nc = smoothstep(
      circleDistance,
      circleDistance + 0.5,
      invert * layer.circleScale * randValue(randCoordX * 1.5, randCoordY * 1.5),
    );
    nc *= 1.5;
    const n2 = fbm2(
      circleUvX * layer.size - time + 100,
      circleUvY * layer.size + 100,
      randValue,
      layer.octaves,
    );
    nc -= n2 * 0.1;

    let alpha = 0;
    if (1 - distanceFromCenter > nc) {
      if (nc > layer.stormWidth - layer.stormDitherWidth + distanceFromCenter && (dith || frame.config.dither === false)) {
        alpha = 1;
      } else if (nc > layer.stormWidth + distanceFromCenter) {
        alpha = 1;
      }
    }

    const colorIndex = clamp(Math.floor(n2 + nc), 0, layer.colors.length - 1);
    const color = layer.colors[colorIndex];
    alpha *= step(n2 * 0.25, distanceFromCenter);
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderBlackHoleCore(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const distanceFromCenter = distance2(pixelU, pixelV, 0.5, 0.5);
    let color = layer.colors[0];
    if (distanceFromCenter > layer.radius - layer.lightWidth) {
      color = layer.colors[1];
    }
    if (distanceFromCenter > layer.radius - layer.lightWidth * 0.5) {
      color = layer.colors[2];
    }
    const alpha = step(distanceFromCenter, layer.radius);
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderBlackHoleDisk(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildTiledRand(frame.shaderSeed, layer.size, 2, 1);
  const wobbleTime = loopTimeFromRotation(frame.progress, layer.timeSpeed, frame.motionFactor);
  const flowTime = loopTimeFromRotation(frame.progress, layer.timeSpeed, frame.spinFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    const pixelU = quantizeUv(u, pixels);
    const pixelV = quantizeUv(v, pixels);
    const dith = mod(u + pixelV, 2 / pixels) <= 1 / pixels;
    // The original shader keeps the disk plane angle fixed and only animates
    // the accretion flow inside the ring over time.
    let rotated = rotateUv(pixelU, pixelV, layer.rotationOffset);
    const preserved = [rotated[0], rotated[1]];

    let compressedX = rotated[0] - 0.5;
    compressedX *= 1.3;
    compressedX += 0.5;
    let compressed = rotateUv(
      compressedX,
      rotated[1],
      Math.sin(wobbleTime * layer.timeSpeed * 2) * 0.01,
    );
    let lightOrigin = [0.5, 0.5];
    let diskWidth = layer.diskWidth;

    const distanceToCenter = distance2(compressed[0], compressed[1], 0.5, 0.5);
    if (compressed[1] < 0.5) {
      const lift = smoothstep(distanceToCenter, 0.5, 0.2);
      compressed[1] += lift;
      diskWidth += smoothstep(distanceToCenter, 0.5, 0.3);
      lightOrigin[1] -= lift;
    } else if (compressed[1] > 0.53) {
      const drop = smoothstep(distanceToCenter, 0.4, 0.17);
      compressed[1] -= drop;
      diskWidth += smoothstep(distanceToCenter, 0.5, 0.2);
      lightOrigin[1] += smoothstep(distanceToCenter, 0.5, 0.2);
    }

    const lightDistance = distance2(
      preserved[0],
      preserved[1] * layer.ringPerspective,
      lightOrigin[0],
      lightOrigin[1] * layer.ringPerspective,
    ) * 0.3;

    let centerX = compressed[0];
    let centerY = compressed[1] - 0.5;
    centerY *= layer.ringPerspective;
    const centerDistance = distance2(centerX, centerY, 0.5, 0);
    let disk = smoothstep(0.1 - diskWidth * 2, 0.5 - diskWidth, centerDistance);
    disk *= smoothstep(centerDistance - diskWidth, centerDistance, 0.4);

    const material = rotateUv(centerX, centerY + 0.5, flowTime * layer.timeSpeed * 3);
    disk *= Math.pow(fbm2(material[0] * layer.size, material[1] * layer.size, randValue, layer.octaves), 0.5);

    if (dith || frame.config.dither === false) {
      disk *= 1.2;
    }

    const posterized = clamp(Math.floor((disk + lightDistance) * (layer.colors.length - 1)), 0, layer.colors.length - 1);
    const color = layer.colors[posterized];
    const alpha = step(0.15, disk);
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function renderGalaxy(buffer, frame, layer) {
  const pixels = useLayerPixels(frame.config.pixels, layer.pixelsScale);
  const randValue = buildUntiledRand(frame.shaderSeed);
  const time = loopTimeFromRotation(frame.progress, layer.timeSpeed, frame.motionFactor);

  createLayerIterator(frame.preset.relativeScale, layer.planeScale, frame.sizeScale, (offset, u, v) => {
    let quantizedU = quantizeUv(u, pixels);
    let quantizedV = quantizeUv(v, pixels);
    const dith = mod(quantizedU + v, 2 / pixels) <= 1 / pixels;

    quantizedU = quantizedU * layer.zoom - (layer.zoom - 1) / 2;
    quantizedV = quantizedV * layer.zoom - (layer.zoom - 1) / 2;
    const rotated = rotateUv(quantizedU, quantizedV, getLayerRotation(layer));
    const preserved = [rotated[0], rotated[1]];

    let layerU = rotated[0];
    let layerV = rotated[1] * layer.tilt - (layer.tilt - 1) / 2;
    const distanceOne = distance2(layerU, layerV, 0.5, 0.5);
    const swirlOne = layer.swirl * Math.pow(distanceOne, 0.4);
    const rotatedLayer = rotateUv(layerU, layerV, swirlOne + time * layer.timeSpeed);
    let f1 = fbm2(rotatedLayer[0] * layer.size, rotatedLayer[1] * layer.size, randValue, layer.octaves);
    f1 = Math.floor(f1 * layer.layerCount) / layer.layerCount;

    let galaxyU = preserved[0];
    let galaxyV = preserved[1] * layer.tilt - (layer.tilt - 1) / 2 - f1 * layer.layerHeight;
    const distanceTwo = distance2(galaxyU, galaxyV, 0.5, 0.5);
    const swirlTwo = layer.swirl * Math.pow(distanceTwo, 0.4);
    const rotatedGalaxy = rotateUv(galaxyU, galaxyV, swirlTwo + time * layer.timeSpeed);
    let f2 = fbm2(
      rotatedGalaxy[0] * layer.size + f1 * 10,
      rotatedGalaxy[1] * layer.size + f1 * 10,
      randValue,
      layer.octaves,
    );

    const alpha = step(f2 + distanceTwo, 0.7);
    if (alpha === 0) {
      return;
    }

    f2 *= 2.3;
    if (frame.config.dither && dith) {
      f2 *= 0.94;
    }

    const colorIndex = clamp(Math.floor(f2 * layer.nColors), 0, layer.colors.length - 1);
    const color = layer.colors[colorIndex];
    blendPixel(buffer, offset, color, alpha * color[3]);
  });
}

function getWetTerranCloudCover(seed) {
  return seededRange(seed, "wet_terran_cloud_cover", 0.35, 0.6);
}

function getIslandCloudCover(seed) {
  return seededRange(seed, "islands_cloud_cover", 0.35, 0.6);
}

function getGasGiantCloudCover(seed) {
  return seededRange(seed, "gas_giant_one_cloud_cover", 0.28, 0.5);
}

function buildFrameState(config, preset, progressValue) {
  const progress = clamp(progressValue, 0, 1);
  const directionFactor = getDirectionFactor(config.direction);
  const speedValue = getSpeedMultiplier(config.speed);
  // In the upstream shaders, direction is primarily expressed by time-driven
  // texture flow, not by continuously rotating the whole preset silhouette.
  const motionFactor = directionFactor;
  const spinFactor = directionFactor;
  const spinAngle = progress * spinFactor * TWO_PI;
  return {
    config,
    preset,
    progress,
    directionFactor,
    motionFactor,
    spinFactor,
    spinAngle,
    frameDelay: getFrameDelay(speedValue),
    sizeScale: getSizeScale(config.size),
    shaderSeed: convertShaderSeed(config.seed),
  };
}

function buildPlanetScreensaverPreviewFrame(config, progressValue) {
  const normalized = normalizeConfig(config);
  const preset = PRESET_DEFINITIONS[normalized.preset];
  const frame = buildFrameState(normalized, preset, progressValue);
  const buffer = createFrameBuffer();
  renderPresetFrame(buffer, frame);
  return bufferToMap(buffer);
}

function renderTerranWet(buffer, frame) {
  renderLandRivers(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.39, 0.39],
    timeSpeed: 0.1,
    ditherSize: 3.951,
    lightBorder1: 0.287,
    lightBorder2: 0.476,
    riverCutoff: 0.368,
    size: 4.6,
    octaves: 6,
    colors: [
      rgba(0.388235, 0.670588, 0.247059),
      rgba(0.231373, 0.490196, 0.309804),
      rgba(0.184314, 0.341176, 0.32549),
      rgba(0.156863, 0.207843, 0.25098),
      rgba(0.309804, 0.643137, 0.721569),
      rgba(0.25098, 0.286275, 0.45098),
    ],
  });

  renderClouds(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.39, 0.39],
    cloudCover: getWetTerranCloudCover(frame.config.seed),
    timeSpeed: 0.1,
    stretch: 2,
    cloudCurve: 1.3,
    lightBorder1: 0.52,
    lightBorder2: 0.62,
    size: 7.315,
    octaves: 2,
    colors: [
      rgba(0.960784, 1, 0.909804),
      rgba(0.87451, 0.878431, 0.909804),
      rgba(0.407843, 0.435294, 0.6),
      rgba(0.25098, 0.286275, 0.45098),
    ],
  });
}

function renderTerranDry(buffer, frame) {
  renderDryTerran(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.4, 0.3],
    lightDistance1: 0.362,
    lightDistance2: 0.525,
    timeSpeed: 0.1,
    size: 8,
    octaves: 3,
    colors: [
      rgba(1, 0.537255, 0.2),
      rgba(0.901961, 0.270588, 0.223529),
      rgba(0.678431, 0.184314, 0.270588),
      rgba(0.321569, 0.2, 0.247059),
      rgba(0.239216, 0.160784, 0.211765),
    ],
  });
}

function renderIslands(buffer, frame) {
  renderPlanetUnder(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.39, 0.39],
    timeSpeed: 0.1,
    ditherSize: 2,
    lightBorder1: 0.4,
    lightBorder2: 0.6,
    size: 5.228,
    octaves: 3,
    colors: [
      rgba(0.572549, 0.909804, 0.752941),
      rgba(0.309804, 0.643137, 0.721569),
      rgba(0.172549, 0.207843, 0.301961),
    ],
  });

  renderLandMass(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.39, 0.39],
    timeSpeed: 0.2,
    lightBorder1: 0.32,
    lightBorder2: 0.534,
    landCutoff: 0.633,
    size: 4.292,
    octaves: 6,
    colors: [
      rgba(0.784314, 0.831373, 0.364706),
      rgba(0.388235, 0.670588, 0.247059),
      rgba(0.184314, 0.341176, 0.32549),
      rgba(0.156863, 0.207843, 0.25098),
    ],
  });

  renderClouds(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.39, 0.39],
    cloudCover: getIslandCloudCover(frame.config.seed),
    timeSpeed: 0.47,
    stretch: 2,
    cloudCurve: 1.3,
    lightBorder1: 0.52,
    lightBorder2: 0.62,
    size: 7.745,
    octaves: 2,
    colors: [
      rgba(0.87451, 0.878431, 0.909804),
      rgba(0.639216, 0.654902, 0.760784),
      rgba(0.407843, 0.435294, 0.6),
      rgba(0.25098, 0.286275, 0.45098),
    ],
  });
}

function renderNoAtmosphere(buffer, frame) {
  renderNoAtmosphereBase(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.25, 0.25],
    timeSpeed: 0.4,
    ditherSize: 2,
    lightBorder1: 0.615,
    lightBorder2: 0.729,
    size: 8,
    octaves: 4,
    colors: [
      rgba(0.639216, 0.654902, 0.760784),
      rgba(0.298039, 0.407843, 0.521569),
      rgba(0.227451, 0.247059, 0.368627),
    ],
  });

  renderCraters(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.25, 0.25],
    timeSpeed: 0.001,
    lightBorder: 0.465,
    size: 5,
    offset: 0.03,
    colors: [
      rgba(0.298039, 0.407843, 0.521569),
      rgba(0.227451, 0.247059, 0.368627),
    ],
  });
}

function renderGasGiantOne(buffer, frame) {
  renderGasClouds(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.25, 0.25],
    cloudCover: 0,
    timeSpeed: 0.7,
    stretch: 1,
    cloudCurve: 1.3,
    lightBorder1: 0.692,
    lightBorder2: 0.666,
    size: 9,
    octaves: 5,
    colors: [
      rgba(0.231373, 0.12549, 0.152941),
      rgba(0.231373, 0.12549, 0.152941),
      rgba(0, 0, 0),
      rgba(0.129412, 0.0941176, 0.105882),
    ],
  });

  renderGasClouds(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.25, 0.25],
    cloudCover: getGasGiantCloudCover(frame.config.seed),
    timeSpeed: 0.47,
    stretch: 1,
    cloudCurve: 1.3,
    lightBorder1: 0.439,
    lightBorder2: 0.746,
    size: 9,
    octaves: 5,
    colors: [
      rgba(0.941176, 0.709804, 0.254902),
      rgba(0.811765, 0.458824, 0.168627),
      rgba(0.670588, 0.317647, 0.188235),
      rgba(0.490196, 0.219608, 0.2),
    ],
  });
}

function renderGasGiantTwo(buffer, frame) {
  renderDenseGas(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [-0.1, 0.3],
    timeSpeed: 0.05,
    bands: 0.892,
    size: 10.107,
    octaves: 3,
    colors: [
      rgba(0.933333, 0.764706, 0.603922),
      rgba(0.85098, 0.627451, 0.4),
      rgba(0.560784, 0.337255, 0.231373),
    ],
    darkColors: [
      rgba(0.4, 0.223529, 0.192157),
      rgba(0.270588, 0.156863, 0.235294),
      rgba(0.133333, 0.12549, 0.203922),
    ],
  });

  renderRing(buffer, frame, {
    planeScale: 3,
    pixelsScale: 3,
    lightOrigin: [-0.1, 0.3],
    timeSpeed: 0.2,
    ringWidth: 0.127,
    ringPerspective: 6,
    scaleRelToPlanet: 6,
    rotationOffset: 0.7,
    size: 15,
    octaves: 4,
    colors: [
      rgba(0.933333, 0.764706, 0.603922),
      rgba(0.701961, 0.478431, 0.313726),
      rgba(0.560784, 0.337255, 0.231373),
    ],
    darkColors: [
      rgba(0.333333, 0.188235, 0.211765),
      rgba(0.196078, 0.137255, 0.215686),
      rgba(0.133333, 0.12549, 0.203922),
    ],
  });
}

function renderIceWorld(buffer, frame) {
  renderPlanetUnder(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.3, 0.3],
    timeSpeed: 0.25,
    ditherSize: 2,
    lightBorder1: 0.48,
    lightBorder2: 0.632,
    size: 8,
    octaves: 2,
    colors: [
      rgba(0.980392, 1, 1),
      rgba(0.780392, 0.831373, 0.882353),
      rgba(0.572549, 0.560784, 0.721569),
    ],
  });

  renderIceLakes(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.3, 0.3],
    timeSpeed: 0.2,
    lightBorder1: 0.024,
    lightBorder2: 0.047,
    lakeCutoff: 0.55,
    size: 10,
    octaves: 3,
    colors: [
      rgba(0.309804, 0.643137, 0.721569),
      rgba(0.298039, 0.407843, 0.521569),
      rgba(0.227451, 0.247059, 0.368627),
    ],
  });

  renderClouds(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.3, 0.3],
    cloudCover: 0.546,
    timeSpeed: 0.1,
    stretch: 2.5,
    cloudCurve: 1.3,
    lightBorder1: 0.566,
    lightBorder2: 0.781,
    size: 4,
    octaves: 4,
    colors: [
      rgba(0.882353, 0.94902, 1),
      rgba(0.752941, 0.890196, 1),
      rgba(0.368627, 0.439216, 0.647059),
      rgba(0.25098, 0.286275, 0.45098),
    ],
  });
}

function renderLavaWorld(buffer, frame) {
  renderNoAtmosphereBase(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.3, 0.3],
    timeSpeed: 0.2,
    ditherSize: 2,
    lightBorder1: 0.4,
    lightBorder2: 0.6,
    size: 10,
    octaves: 3,
    colors: [
      rgba(0.560784, 0.301961, 0.341176),
      rgba(0.321569, 0.2, 0.247059),
      rgba(0.239216, 0.160784, 0.211765),
    ],
  });

  renderCraters(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.3, 0.3],
    timeSpeed: 0.2,
    lightBorder: 0.4,
    size: 3.5,
    offset: 0.03,
    colors: [
      rgba(0.321569, 0.2, 0.247059),
      rgba(0.239216, 0.160784, 0.211765),
    ],
  });

  renderLavaRivers(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0.3, 0.3],
    timeSpeed: 0.2,
    lightBorder1: 0.019,
    lightBorder2: 0.036,
    riverCutoff: 0.579,
    size: 10,
    octaves: 4,
    colors: [
      rgba(1, 0.537255, 0.2),
      rgba(0.901961, 0.270588, 0.223529),
      rgba(0.678431, 0.184314, 0.270588),
    ],
  });
}

function renderAsteroidPreset(buffer, frame) {
  renderAsteroid(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    lightOrigin: [0, 0],
    size: 5.294,
    octaves: 2,
    colors: [
      rgba(0.639216, 0.654902, 0.760784),
      rgba(0.298039, 0.407843, 0.521569),
      rgba(0.227451, 0.247059, 0.368627),
    ],
  });
}

function renderBlackHolePreset(buffer, frame) {
  renderBlackHoleCore(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    radius: 0.247,
    lightWidth: 0.028,
    colors: [
      rgba(0.152941, 0.152941, 0.211765),
      rgba(1, 1, 0.921569),
      rgba(0.929412, 0.482353, 0.223529),
    ],
  });

  renderBlackHoleDisk(buffer, frame, {
    planeScale: 3,
    pixelsScale: 3,
    timeSpeed: 0.2,
    diskWidth: 0.065,
    ringPerspective: 14,
    rotationOffset: 0.766,
    size: 6.598,
    octaves: 3,
    colors: [
      rgba(1, 1, 0.921569),
      rgba(1, 0.960784, 0.25098),
      rgba(1, 0.721569, 0.290196),
      rgba(0.929412, 0.482353, 0.223529),
      rgba(0.741176, 0.25098, 0.207843),
    ],
  });
}

function renderGalaxyPreset(buffer, frame) {
  renderGalaxy(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    timeSpeed: 1,
    rotationOffset: 0.674,
    size: 7,
    octaves: 1,
    tilt: 3,
    layerCount: 4,
    layerHeight: 0.4,
    zoom: 1.375,
    swirl: -9,
    nColors: 6,
    colors: [
      rgba(1, 1, 0.921569),
      rgba(1, 0.913725, 0.552941),
      rgba(0.709804, 0.878431, 0.4),
      rgba(0.396078, 0.647059, 0.4),
      rgba(0.223529, 0.364706, 0.392157),
      rgba(0.196078, 0.223529, 0.301961),
      rgba(0.196078, 0.160784, 0.278431),
    ],
  });
}

function renderStarPreset(buffer, frame) {
  const evenSeed = mod(frame.config.colorSeed, 2) === 0;
  const starColors = evenSeed
    ? [
        rgba(0.960784, 1, 0.909804),
        rgba(1, 0.847059, 0.196078),
        rgba(1, 0.509804, 0.231373),
        rgba(0.486275, 0.0980392, 0.101961),
      ]
    : [
        rgba(0.960784, 1, 0.909804),
        rgba(0.466667, 0.839216, 0.756863),
        rgba(0.109804, 0.572549, 0.654902),
        rgba(0.0117647, 0.243137, 0.368627),
      ];
  const flareColors = evenSeed
    ? [rgba(1, 0.847059, 0.196078), rgba(0.960784, 1, 0.909804)]
    : [rgba(0.466667, 0.839216, 0.756863), rgba(0.960784, 1, 0.909804)];

  renderStarBlobs(buffer, frame, {
    planeScale: 2,
    pixelsScale: 2,
    timeSpeed: 0.05,
    size: 4.93,
    circleAmount: 2,
    circleSize: 1,
    color: rgba(1, 1, 0.894118),
  });

  renderStarCore(buffer, frame, {
    planeScale: 1,
    pixelsScale: 1,
    timeSpeed: 0.05,
    tiles: 1,
    colors: starColors,
  });

  renderStarFlares(buffer, frame, {
    planeScale: 2,
    pixelsScale: 2,
    timeSpeed: 0.05,
    stormWidth: 0.3,
    stormDitherWidth: 0,
    circleAmount: 2,
    circleScale: 1,
    scale: 1,
    size: 1.6,
    octaves: 4,
    colors: flareColors,
  });
}

function renderPresetToBuffer(buffer, frame) {
  switch (frame.preset.id) {
    case "terran_wet":
      renderTerranWet(buffer, frame);
      return;
    case "terran_dry":
      renderTerranDry(buffer, frame);
      return;
    case "islands":
      renderIslands(buffer, frame);
      return;
    case "no_atmosphere":
      renderNoAtmosphere(buffer, frame);
      return;
    case "gas_giant_1":
      renderGasGiantOne(buffer, frame);
      return;
    case "gas_giant_2":
      renderGasGiantTwo(buffer, frame);
      return;
    case "ice_world":
      renderIceWorld(buffer, frame);
      return;
    case "lava_world":
      renderLavaWorld(buffer, frame);
      return;
    case "asteroid":
      renderAsteroidPreset(buffer, frame);
      return;
    case "black_hole":
      renderBlackHolePreset(buffer, frame);
      return;
    case "galaxy":
      renderGalaxyPreset(buffer, frame);
      return;
    case "star":
      renderStarPreset(buffer, frame);
      return;
    default:
      renderTerranWet(buffer, frame);
  }
}

function normalizeSeed(value) {
  const numericValue = Number(value);
  if (!Number.isFinite(numericValue)) {
    return createDefaultPlanetPreviewConfig().seed;
  }
  return clamp(Math.round(numericValue), 0, MAX_PLANET_PREVIEW_SEED);
}

function normalizeColorSeed(value) {
  const numericValue = Number(value);
  if (!Number.isFinite(numericValue)) {
    return createDefaultPlanetPreviewConfig().colorSeed;
  }
  return clamp(Math.round(numericValue), 0, MAX_PLANET_PREVIEW_SEED);
}

function normalizePixels(value) {
  const numericValue = Number(value);
  if (!Number.isFinite(numericValue)) {
    return createDefaultPlanetPreviewConfig().pixels;
  }
  return clamp(Math.round(numericValue), PLANET_PREVIEW_MIN_PIXELS, PLANET_PREVIEW_MAX_PIXELS);
}

function normalizeConfig(config) {
  const defaults = createDefaultPlanetPreviewConfig();
  const nextConfig = config && typeof config === "object" ? config : {};
  const presetKey = typeof nextConfig.preset === "string" ? nextConfig.preset : defaults.preset;
  const preset = PRESET_DEFINITIONS[presetKey] ? presetKey : defaults.preset;
  const size = PLANET_SIZE_OPTIONS.some((item) => item.id === nextConfig.size)
    ? nextConfig.size
    : defaults.size;
  const direction = PLANET_DIRECTION_OPTIONS.some((item) => item.id === nextConfig.direction)
    ? nextConfig.direction
    : defaults.direction;
  const nextSpeed = Number(nextConfig.speed);
  const speed = Number.isFinite(nextSpeed)
    ? clamp(
        Math.round(nextSpeed),
        PLANET_PREVIEW_MIN_SPEED,
        PLANET_PREVIEW_MAX_SPEED,
      )
    : defaults.speed;
  return {
    preset,
    pixels: normalizePixels(nextConfig.pixels),
    size,
    seed: normalizeSeed(nextConfig.seed),
    colorSeed: normalizeColorSeed(nextConfig.colorSeed),
    direction,
    speed,
    dither: typeof nextConfig.dither === "boolean" ? nextConfig.dither : defaults.dither,
  };
}

function readPreviewCache(key) {
  return PREVIEW_CACHE.get(key);
}

function writePreviewCache(key, sequence) {
  if (PREVIEW_CACHE.size >= 8) {
    const firstKey = PREVIEW_CACHE.keys().next().value;
    PREVIEW_CACHE.delete(firstKey);
  }
  PREVIEW_CACHE.set(key, sequence);
}

function buildPlanetScreensaverPreviewSequence(config) {
  const normalized = normalizeConfig(config);
  const cacheKey = [
    normalized.preset,
    normalized.pixels,
    normalized.size,
    normalized.seed,
    normalized.colorSeed,
    normalized.direction,
    normalized.speed,
    normalized.dither ? "1" : "0",
  ].join("|");

  const cached = readPreviewCache(cacheKey);
  if (cached) {
    return cached;
  }

  const preset = PRESET_DEFINITIONS[normalized.preset];
  const maps = [];
  const delays = [];

  for (let frameIndex = 0; frameIndex < FRAME_COUNT; frameIndex += 1) {
    const progress = FRAME_COUNT > 1 ? frameIndex / (FRAME_COUNT - 1) : 0;
    const frame = buildFrameState(normalized, preset, progress);
    const buffer = createFrameBuffer();
    renderPresetFrame(buffer, frame);
    maps.push(bufferToMap(buffer));
    delays.push(frame.frameDelay);
  }

  const sequence = {
    maps,
    delays,
  };
  writePreviewCache(cacheKey, sequence);
  return sequence;
}

export {
  FRAME_COUNT,
  PLANET_SCREEN_PRESETS,
  PLANET_PREVIEW_MAX_PIXELS,
  PLANET_PREVIEW_MIN_PIXELS,
  PLANET_PREVIEW_MIN_SPEED,
  PLANET_PREVIEW_MAX_SPEED,
  PLANET_PREVIEW_PLAYBACK_INTERVAL_MS,
  MAX_PLANET_PREVIEW_SEED,
  PLANET_SIZE_OPTIONS,
  PLANET_DIRECTION_OPTIONS,
  createDefaultPlanetPreviewConfig,
  createRandomPlanetPreviewSeed,
  createRandomPlanetColorSeed,
  getPlanetPreviewCycleDuration,
  buildPlanetScreensaverPreviewFrame,
  buildPlanetScreensaverPreviewSequence,
};
