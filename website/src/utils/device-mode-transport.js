import { clamp } from "@/utils/device-mode-core.js";

export function buildPixelBinaryBufferFromPackedPixels(pixelData, width = 64, height = 64) {
  if (!Array.isArray(pixelData) || pixelData.length % 5 !== 0) {
    throw new Error("像素数据格式无效");
  }
  const offsetX = Math.floor((64 - width) / 2);
  const offsetY = Math.floor((64 - height) / 2);
  const buffer = new Uint8Array(pixelData.length);
  for (let index = 0; index < pixelData.length; index += 5) {
    const x = Number(pixelData[index]);
    const y = Number(pixelData[index + 1]);
    const r = Number(pixelData[index + 2]);
    const g = Number(pixelData[index + 3]);
    const b = Number(pixelData[index + 4]);
    if (
      !Number.isInteger(x) ||
      !Number.isInteger(y) ||
      !Number.isInteger(r) ||
      !Number.isInteger(g) ||
      !Number.isInteger(b)
    ) {
      throw new Error("像素数据格式无效");
    }
    buffer[index] = x + offsetX;
    buffer[index + 1] = y + offsetY;
    buffer[index + 2] = clamp(r, 0, 255);
    buffer[index + 3] = clamp(g, 0, 255);
    buffer[index + 4] = clamp(b, 0, 255);
  }
  return buffer.buffer;
}

function normalizeCompactAnimationPixelBytes(pixels, totalPixels, frameIndex) {
  if (!Array.isArray(pixels) || pixels.length !== totalPixels) {
    throw new Error(`第 ${frameIndex + 1} 帧像素数量无效`);
  }
  const bytes = new Uint8Array(totalPixels * 5);
  for (let pixelIndex = 0; pixelIndex < totalPixels; pixelIndex += 1) {
    const pixel = pixels[pixelIndex];
    if (!Array.isArray(pixel) || pixel.length < 5) {
      throw new Error(`第 ${frameIndex + 1} 帧像素格式无效`);
    }
    for (let channelIndex = 0; channelIndex < 5; channelIndex += 1) {
      const value = Number(pixel[channelIndex]);
      if (!Number.isInteger(value) || value < 0 || value > 255) {
        throw new Error(`第 ${frameIndex + 1} 帧像素数据无效`);
      }
      bytes[pixelIndex * 5 + channelIndex] = value;
    }
  }
  return bytes;
}

export function buildCompactAnimationBinaryBuffer(animationData) {
  if (!Array.isArray(animationData) || animationData.length === 0) {
    throw new Error("动画帧不能为空");
  }

  const normalizedFrames = animationData.map((frame, frameIndex) => {
    if (!Array.isArray(frame) || frame.length < 4) {
      throw new Error(`第 ${frameIndex + 1} 帧格式无效`);
    }
    const type = Number(frame[0]);
    const delay = Number(frame[1]);
    const totalPixels = Number(frame[2]);
    if (!Number.isInteger(type) || (type !== 0 && type !== 1)) {
      throw new Error(`第 ${frameIndex + 1} 帧类型无效`);
    }
    if (!Number.isInteger(totalPixels) || totalPixels < 0 || totalPixels > 65535) {
      throw new Error(`第 ${frameIndex + 1} 帧像素数无效`);
    }
    return {
      type,
      delay: clamp(Math.round(delay), 0, 65535),
      totalPixels,
      pixels: normalizeCompactAnimationPixelBytes(frame[3], totalPixels, frameIndex),
    };
  });

  let totalBytes = 2;
  normalizedFrames.forEach((frame) => {
    totalBytes += 5 + frame.totalPixels * 5;
  });

  const buffer = new ArrayBuffer(totalBytes);
  const view = new DataView(buffer);
  const bytes = new Uint8Array(buffer);
  let offset = 0;
  view.setUint16(offset, normalizedFrames.length, true);
  offset += 2;
  normalizedFrames.forEach((frame) => {
    bytes[offset] = frame.type;
    offset += 1;
    view.setUint16(offset, frame.delay, true);
    offset += 2;
    view.setUint16(offset, frame.totalPixels, true);
    offset += 2;
    bytes.set(frame.pixels, offset);
    offset += frame.pixels.length;
  });
  return buffer;
}
