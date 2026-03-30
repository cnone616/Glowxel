/**
 * GIF 解析器
 * 从 ArrayBuffer 解析 GIF，生成 ESP32 动画数据
 */
export class GIFParser {
  constructor() {
    this.data = null;
    this.pos = 0;
    this.globalColorTable = null;
    this.frames = [];
    this.width = 0;
    this.height = 0;
    this.globalColorTableFlag = false;
    this.globalColorTableSize = 0;
    this.backgroundColorIndex = 0;
    this.currentFrameControl = null;
  }

  readByte() {
    return this.data[this.pos++];
  }

  readUint16() {
    const low = this.readByte();
    const high = this.readByte();
    return low | (high << 8);
  }

  readBytes(length) {
    const bytes = new Uint8Array(length);
    for (let i = 0; i < length; i += 1) {
      bytes[i] = this.readByte();
    }
    return bytes;
  }

  skipBytes(length) {
    this.pos += length;
  }

  parse(arrayBuffer) {
    this.data = new Uint8Array(arrayBuffer);
    this.pos = 0;
    this.frames = [];

    const sig = String.fromCharCode(...this.readBytes(6));
    if (sig !== "GIF87a" && sig !== "GIF89a") {
      throw new Error("不是有效的 GIF 文件");
    }

    this.width = this.readUint16();
    this.height = this.readUint16();
    const packed = this.readByte();
    this.globalColorTableFlag = (packed & 0x80) !== 0;
    this.globalColorTableSize = 2 << (packed & 0x07);
    this.backgroundColorIndex = this.readByte();
    this.readByte();

    if (this.globalColorTableFlag) {
      this.globalColorTable = [];
      for (let i = 0; i < this.globalColorTableSize; i += 1) {
        this.globalColorTable.push([
          this.readByte(),
          this.readByte(),
          this.readByte(),
        ]);
      }
    }

    this._parseDataStream();
    return {
      width: this.width,
      height: this.height,
      frameCount: this.frames.length,
    };
  }

  _parseDataStream() {
    while (this.pos < this.data.length) {
      const sep = this.readByte();
      if (sep === 0x21) {
        this._parseExtension();
      } else if (sep === 0x2c) {
        this._parseImage();
      } else if (sep === 0x3b) {
        return;
      } else {
        return;
      }
    }
  }

  _parseExtension() {
    const label = this.readByte();
    if (label === 0xf9) {
      this.readByte();
      const packed = this.readByte();
      const disposalMethod = (packed & 0x1c) >> 2;
      const transparentColorFlag = (packed & 0x01) !== 0;
      const delayTime = this.readUint16();
      const transparentColorIndex = this.readByte();
      this.readByte();
      this.currentFrameControl = {
        disposalMethod,
        transparentColorFlag,
        delayTime: Math.max(delayTime * 10, 50),
        transparentColorIndex,
      };
    } else {
      this._skipSubBlocks();
    }
  }

  _skipSubBlocks() {
    let size;
    do {
      size = this.readByte();
      this.skipBytes(size);
    } while (size !== 0);
  }

  _parseImage() {
    const left = this.readUint16();
    const top = this.readUint16();
    const width = this.readUint16();
    const height = this.readUint16();
    const packed = this.readByte();
    const localColorTableFlag = (packed & 0x80) !== 0;
    const localColorTableSize = localColorTableFlag ? 2 << (packed & 0x07) : 0;

    let colorTable = this.globalColorTable;
    if (localColorTableFlag) {
      colorTable = [];
      for (let i = 0; i < localColorTableSize; i += 1) {
        colorTable.push([this.readByte(), this.readByte(), this.readByte()]);
      }
    }

    const lzwMinCode = this.readByte();
    const compData = this._readImageData();
    const pixels = this._decompressLZW(compData, lzwMinCode);

    this.frames.push({
      left,
      top,
      width,
      height,
      pixels,
      colorTable,
      delay: this.currentFrameControl ? this.currentFrameControl.delayTime : 100,
      disposalMethod: this.currentFrameControl ? this.currentFrameControl.disposalMethod : 0,
      transparentColorFlag: this.currentFrameControl ? this.currentFrameControl.transparentColorFlag : false,
      transparentColorIndex: this.currentFrameControl ? this.currentFrameControl.transparentColorIndex : 0,
    });
    this.currentFrameControl = null;
  }

  _readImageData() {
    const data = [];
    let size;
    do {
      size = this.readByte();
      for (let i = 0; i < size; i += 1) {
        data.push(this.readByte());
      }
    } while (size !== 0);
    return new Uint8Array(data);
  }

  _decompressLZW(data, minCodeSize) {
    const clearCode = 1 << minCodeSize;
    const endCode = clearCode + 1;
    let codeSize = minCodeSize + 1;
    let nextCode = endCode + 1;
    const codeTable = [];
    const output = [];
    for (let i = 0; i < clearCode; i += 1) {
      codeTable[i] = [i];
    }
    let bitPos = 0;
    let prevCode = null;

    while (bitPos < data.length * 8) {
      let code = 0;
      for (let i = 0; i < codeSize; i += 1) {
        const byteIdx = Math.floor((bitPos + i) / 8);
        const bitIdx = (bitPos + i) % 8;
        if (byteIdx < data.length) {
          code |= ((data[byteIdx] >> bitIdx) & 1) << i;
        }
      }
      bitPos += codeSize;

      if (code === clearCode) {
        codeTable.length = endCode + 1;
        for (let i = 0; i < clearCode; i += 1) {
          codeTable[i] = [i];
        }
        codeSize = minCodeSize + 1;
        nextCode = endCode + 1;
        prevCode = null;
        continue;
      }
      if (code === endCode) {
        break;
      }

      let seq;
      if (code < codeTable.length) {
        seq = codeTable[code];
      } else if (code === nextCode && prevCode !== null) {
        seq = [...codeTable[prevCode], codeTable[prevCode][0]];
      } else {
        break;
      }

      output.push(...seq);
      if (prevCode !== null && nextCode < 4096) {
        codeTable[nextCode] = [...codeTable[prevCode], seq[0]];
        nextCode += 1;
        if (nextCode === 1 << codeSize && codeSize < 12) {
          codeSize += 1;
        }
      }
      prevCode = code;
    }
    return new Uint8Array(output);
  }

  renderFrame(frameIndex = 0, targetW = 64, targetH = 64) {
    if (frameIndex < 0 || frameIndex >= this.frames.length) {
      return null;
    }

    const buf = new Uint8Array(this.width * this.height * 4);
    for (let fi = 0; fi <= frameIndex; fi += 1) {
      const frame = this.frames[fi];
      if (fi > 0 && this.frames[fi - 1].disposalMethod === 2) {
        const prevFrame = this.frames[fi - 1];
        for (let y = 0; y < prevFrame.height; y += 1) {
          for (let x = 0; x < prevFrame.width; x += 1) {
            const cx = prevFrame.left + x;
            const cy = prevFrame.top + y;
            if (cx >= 0 && cx < this.width && cy >= 0 && cy < this.height) {
              const di = (cy * this.width + cx) * 4;
              buf[di] = 0;
              buf[di + 1] = 0;
              buf[di + 2] = 0;
              buf[di + 3] = 0;
            }
          }
        }
      }

      for (let y = 0; y < frame.height; y += 1) {
        for (let x = 0; x < frame.width; x += 1) {
          const idx = y * frame.width + x;
          if (idx >= frame.pixels.length) {
            continue;
          }
          const ci = frame.pixels[idx];
          if (frame.transparentColorFlag && ci === frame.transparentColorIndex) {
            continue;
          }
          const color = frame.colorTable && frame.colorTable[ci] ? frame.colorTable[ci] : [0, 0, 0];
          const cx = frame.left + x;
          const cy = frame.top + y;
          if (cx >= 0 && cx < this.width && cy >= 0 && cy < this.height) {
            const di = (cy * this.width + cx) * 4;
            buf[di] = color[0];
            buf[di + 1] = color[1];
            buf[di + 2] = color[2];
            buf[di + 3] = 255;
          }
        }
      }
    }

    const scaled = new Uint8Array(targetW * targetH * 4);
    const sx = this.width / targetW;
    const sy = this.height / targetH;
    for (let y = 0; y < targetH; y += 1) {
      for (let x = 0; x < targetW; x += 1) {
        const srcX = Math.floor(x * sx);
        const srcY = Math.floor(y * sy);
        const si = (srcY * this.width + srcX) * 4;
        const di = (y * targetW + x) * 4;
        scaled[di] = buf[si];
        scaled[di + 1] = buf[si + 1];
        scaled[di + 2] = buf[si + 2];
        scaled[di + 3] = buf[si + 3];
      }
    }

    return {
      rgba: new Uint8Array(scaled),
      delay: this.frames[frameIndex].delay || 100,
    };
  }

  renderFrames(targetW = 64, targetH = 64) {
    const results = [];
    const buf = new Uint8Array(this.width * this.height * 4);

    for (let fi = 0; fi < this.frames.length; fi += 1) {
      const frame = this.frames[fi];
      if (fi > 0 && this.frames[fi - 1].disposalMethod === 2) {
        const prevFrame = this.frames[fi - 1];
        for (let y = 0; y < prevFrame.height; y += 1) {
          for (let x = 0; x < prevFrame.width; x += 1) {
            const cx = prevFrame.left + x;
            const cy = prevFrame.top + y;
            if (cx >= 0 && cx < this.width && cy >= 0 && cy < this.height) {
              const di = (cy * this.width + cx) * 4;
              buf[di] = 0;
              buf[di + 1] = 0;
              buf[di + 2] = 0;
              buf[di + 3] = 0;
            }
          }
        }
      }

      for (let y = 0; y < frame.height; y += 1) {
        for (let x = 0; x < frame.width; x += 1) {
          const idx = y * frame.width + x;
          if (idx >= frame.pixels.length) {
            continue;
          }
          const ci = frame.pixels[idx];
          if (frame.transparentColorFlag && ci === frame.transparentColorIndex) {
            continue;
          }
          const color = frame.colorTable && frame.colorTable[ci] ? frame.colorTable[ci] : [0, 0, 0];
          const cx = frame.left + x;
          const cy = frame.top + y;
          if (cx >= 0 && cx < this.width && cy >= 0 && cy < this.height) {
            const di = (cy * this.width + cx) * 4;
            buf[di] = color[0];
            buf[di + 1] = color[1];
            buf[di + 2] = color[2];
            buf[di + 3] = 255;
          }
        }
      }

      const scaled = new Uint8Array(targetW * targetH * 4);
      const sx = this.width / targetW;
      const sy = this.height / targetH;
      for (let y = 0; y < targetH; y += 1) {
        for (let x = 0; x < targetW; x += 1) {
          const srcX = Math.floor(x * sx);
          const srcY = Math.floor(y * sy);
          const si = (srcY * this.width + srcX) * 4;
          const di = (y * targetW + x) * 4;
          scaled[di] = buf[si];
          scaled[di + 1] = buf[si + 1];
          scaled[di + 2] = buf[si + 2];
          scaled[di + 3] = buf[si + 3];
        }
      }
      results.push({
        rgba: new Uint8Array(scaled),
        delay: frame.delay || 100,
      });
    }
    return results;
  }

  generateESP32Data(
    targetW = 64,
    targetH = 64,
    maxFrames = 20,
    excludePixels = null,
    offsetX = 0,
    offsetY = 0,
    preRendered = null,
  ) {
    const rendered = preRendered === null ? this.renderFrames(targetW, targetH) : preRendered;
    const count = Math.min(rendered.length, maxFrames);
    const frames = [];
    let prevRgba = null;

    for (let i = 0; i < count; i += 1) {
      const renderedFrame = rendered[i];
      const rgba = renderedFrame.rgba;
      const safeDelay = Math.max(50, renderedFrame.delay || 100);

      const tmpFull = new Uint8Array(targetW * targetH * 5);
      let fullCount = 0;

      for (let y = 0; y < targetH; y += 1) {
        for (let x = 0; x < targetW; x += 1) {
          const fx = x + offsetX;
          const fy = y + offsetY;
          if (fx < 0 || fx >= 64 || fy < 0 || fy >= 64) {
            continue;
          }
          if (excludePixels && excludePixels.has(`${fx},${fy}`)) {
            continue;
          }
          const idx = (y * targetW + x) * 4;
          const r = rgba[idx];
          const g = rgba[idx + 1];
          const b = rgba[idx + 2];
          if (r > 0 || g > 0 || b > 0) {
            const base = fullCount * 5;
            tmpFull[base] = fx;
            tmpFull[base + 1] = fy;
            tmpFull[base + 2] = r;
            tmpFull[base + 3] = g;
            tmpFull[base + 4] = b;
            fullCount += 1;
          }
        }
      }

      const fullPixels = tmpFull.slice(0, fullCount * 5);
      if (i === 0 || prevRgba === null) {
        frames.push([1, safeDelay, fullCount, fullPixels]);
      } else {
        const tmpDiff = new Uint8Array(targetW * targetH * 5);
        let diffCount = 0;
        for (let y = 0; y < targetH; y += 1) {
          for (let x = 0; x < targetW; x += 1) {
            const fx = x + offsetX;
            const fy = y + offsetY;
            if (fx < 0 || fx >= 64 || fy < 0 || fy >= 64) {
              continue;
            }
            if (excludePixels && excludePixels.has(`${fx},${fy}`)) {
              continue;
            }
            const idx = (y * targetW + x) * 4;
            const cr = rgba[idx];
            const cg = rgba[idx + 1];
            const cb = rgba[idx + 2];
            if (prevRgba[idx] !== cr || prevRgba[idx + 1] !== cg || prevRgba[idx + 2] !== cb) {
              const base = diffCount * 5;
              tmpDiff[base] = fx;
              tmpDiff[base + 1] = fy;
              tmpDiff[base + 2] = cr;
              tmpDiff[base + 3] = cg;
              tmpDiff[base + 4] = cb;
              diffCount += 1;
            }
          }
        }
        if (diffCount > fullCount * 0.5) {
          frames.push([1, safeDelay, fullCount, fullPixels]);
        } else {
          frames.push([0, safeDelay, diffCount, tmpDiff.slice(0, diffCount * 5)]);
        }
      }
      prevRgba = rgba;
    }

    return {
      frameCount: frames.length,
      frames,
    };
  }
}
