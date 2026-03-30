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

function waveformValue(waveform, t) {
  if (waveform === "sine") {
    return (Math.sin(t * Math.PI * 2) + 1) / 2;
  }
  if (waveform === "triangle") {
    if (t < 0.5) {
      return t * 2;
    }
    return (1 - t) * 2;
  }
  if (waveform === "square") {
    if (t < 0.5) {
      return 1;
    }
    return 0;
  }
  throw new Error("波形参数错误");
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

export function buildBreathFrames(config) {
  if (!config) {
    throw new Error("呼吸灯配置缺失");
  }
  if (config.speed === undefined || config.loop === undefined || config.minBrightness === undefined || config.maxBrightness === undefined || config.periodMs === undefined || config.waveform === undefined || config.color === undefined) {
    throw new Error("呼吸灯配置字段不完整");
  }

  const speed = clampInt(config.speed, 1, 10);
  const loop = config.loop === true;
  const minBrightness = clampInt(config.minBrightness, 0, 100);
  const maxBrightness = clampInt(config.maxBrightness, 0, 100);
  const periodMs = clampInt(config.periodMs, 200, 10000);
  const baseColor = hexToRgb(config.color);

  const frameCount = loop ? 20 : 1;
  const delay = Math.max(40, Math.round(periodMs / frameCount / speed));
  const frames = [];

  for (let i = 0; i < frameCount; i++) {
    const t = frameCount === 1 ? 1 : i / (frameCount - 1);
    const wave = waveformValue(config.waveform, t);
    const brightPct = minBrightness + (maxBrightness - minBrightness) * wave;
    const bright = brightPct / 100;
    const color = {
      r: Math.round(baseColor.r * bright),
      g: Math.round(baseColor.g * bright),
      b: Math.round(baseColor.b * bright),
    };
    frames.push(makeUniformFrame(delay, color));
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
