function clampColorChannel(value) {
  const channel = Number(value);
  if (!Number.isFinite(channel)) {
    throw new Error("颜色通道无效");
  }
  if (channel < 0) {
    return 0;
  }
  if (channel > 255) {
    return 255;
  }
  return Math.round(channel);
}

function parseHexColor(color) {
  const normalized = color.toLowerCase();
  if (!/^#([0-9a-f]{6})$/.test(normalized)) {
    throw new Error("颜色格式无效");
  }
  return {
    r: parseInt(normalized.slice(1, 3), 16),
    g: parseInt(normalized.slice(3, 5), 16),
    b: parseInt(normalized.slice(5, 7), 16),
  };
}

function parseRgbColor(color) {
  const match = color.match(
    /^rgb\s*\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$/i,
  );
  if (!match) {
    throw new Error("颜色格式无效");
  }
  return {
    r: clampColorChannel(match[1]),
    g: clampColorChannel(match[2]),
    b: clampColorChannel(match[3]),
  };
}

export function parsePixelColor(color) {
  if (typeof color !== "string") {
    throw new Error("颜色格式无效");
  }
  if (color.startsWith("#")) {
    return parseHexColor(color);
  }
  if (color.startsWith("rgb")) {
    return parseRgbColor(color);
  }
  throw new Error("颜色格式无效");
}

export function pixelMapToAnimationFrame(pixelMap, delay = 180) {
  if (!(pixelMap instanceof Map)) {
    throw new Error("像素数据类型错误");
  }

  const bytes = [];
  pixelMap.forEach((color, key) => {
    const [xValue, yValue] = key.split(",");
    const x = Number(xValue);
    const y = Number(yValue);
    if (!Number.isInteger(x) || !Number.isInteger(y)) {
      return;
    }

    const rgb = parsePixelColor(color);
    bytes.push(x, y, rgb.r, rgb.g, rgb.b);
  });

  return {
    type: 1,
    delay: Number(delay),
    totalPixels: bytes.length / 5,
    pixels: new Uint8Array(bytes),
  };
}

export function pixelPreviewFramesToAnimationFrames(frames, defaultDelay = 180) {
  if (!Array.isArray(frames)) {
    throw new Error("预览帧数据类型错误");
  }

  return frames.map((frame) => {
    if (!frame || !(frame.pixels instanceof Map)) {
      throw new Error("预览帧字段不完整");
    }
    const frameDelay =
      typeof frame.delay === "number" ? frame.delay : Number(defaultDelay);
    return pixelMapToAnimationFrame(frame.pixels, frameDelay);
  });
}
