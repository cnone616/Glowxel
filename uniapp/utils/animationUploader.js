function normalizeCompactPixelBytes(pixels, totalPixels, frameIndex) {
  if (pixels instanceof Uint8Array) {
    if (pixels.length !== totalPixels * 5) {
      throw new Error(`第 ${frameIndex + 1} 帧像素长度不匹配`);
    }
    return pixels;
  }

  if (!Array.isArray(pixels)) {
    throw new Error(`第 ${frameIndex + 1} 帧像素数据类型错误`);
  }
  if (pixels.length !== totalPixels) {
    throw new Error(`第 ${frameIndex + 1} 帧像素数量不匹配`);
  }

  const bytes = new Uint8Array(totalPixels * 5);
  pixels.forEach((pixel, pixelIndex) => {
    if (!Array.isArray(pixel) || pixel.length < 5) {
      throw new Error(`第 ${frameIndex + 1} 帧第 ${pixelIndex + 1} 个像素格式错误`);
    }

    const offset = pixelIndex * 5;
    for (let channelIndex = 0; channelIndex < 5; channelIndex += 1) {
      const value = Number(pixel[channelIndex]);
      if (!Number.isInteger(value) || value < 0 || value > 255) {
        throw new Error(`第 ${frameIndex + 1} 帧第 ${pixelIndex + 1} 个像素数据无效`);
      }
      bytes[offset + channelIndex] = value;
    }
  });
  return bytes;
}

function normalizeCompactFrame(frame, frameIndex) {
  let type;
  let delay;
  let totalPixels;
  let pixels;

  if (Array.isArray(frame)) {
    if (frame.length < 4) {
      throw new Error(`第 ${frameIndex + 1} 帧数据格式错误`);
    }
    type = frame[0];
    delay = frame[1];
    totalPixels = frame[2];
    pixels = frame[3];
  } else if (frame && typeof frame === "object") {
    if (
      frame.type === undefined ||
      frame.delay === undefined ||
      frame.totalPixels === undefined ||
      frame.pixels === undefined
    ) {
      throw new Error(`第 ${frameIndex + 1} 帧字段不完整`);
    }
    type = frame.type;
    delay = frame.delay;
    totalPixels = frame.totalPixels;
    pixels = frame.pixels;
  } else {
    throw new Error(`第 ${frameIndex + 1} 帧数据格式错误`);
  }

  if (!Number.isInteger(type) || (type !== 0 && type !== 1)) {
    throw new Error(`第 ${frameIndex + 1} 帧类型无效`);
  }
  if (!Number.isFinite(Number(delay)) || Number(delay) < 0) {
    throw new Error(`第 ${frameIndex + 1} 帧延迟无效`);
  }
  if (!Number.isInteger(totalPixels) || totalPixels < 0) {
    throw new Error(`第 ${frameIndex + 1} 帧像素数量无效`);
  }

  return [
    type,
    Number(delay),
    totalPixels,
    normalizeCompactPixelBytes(pixels, totalPixels, frameIndex),
  ];
}

export function buildCompactAnimationData(frames) {
  if (!Array.isArray(frames) || frames.length === 0) {
    throw new Error("动画帧不能为空");
  }

  return frames.map((frame, frameIndex) =>
    normalizeCompactFrame(frame, frameIndex),
  );
}

export function buildCompactAnimationBinaryBuffer(frames) {
  const animationData = buildCompactAnimationData(frames);
  let totalBytes = 2;

  animationData.forEach((frame) => {
    totalBytes += 5 + frame[2] * 5;
  });

  const buffer = new ArrayBuffer(totalBytes);
  const bytes = new Uint8Array(buffer);
  const view = new DataView(buffer);
  let offset = 0;

  view.setUint16(offset, animationData.length, true);
  offset += 2;

  animationData.forEach((frame) => {
    bytes[offset++] = frame[0];
    view.setUint16(offset, frame[1], true);
    offset += 2;
    view.setUint16(offset, frame[2], true);
    offset += 2;
    bytes.set(frame[3], offset);
    offset += frame[3].length;
  });

  return buffer;
}

export async function applyCompactAnimation(ws, frames, targetMode) {
  if (typeof targetMode !== "string" || targetMode.length === 0) {
    throw new Error("目标模式不能为空");
  }

  return ws.runModeTransaction({
    mode: targetMode,
    params: {},
    binary: buildCompactAnimationBinaryBuffer(frames),
  });
}
