/**
 * 完整的 GIF 解析器
 * 支持 GIF87a 和 GIF89a 格式
 * 提取所有帧数据和动画信息
 */

class GIFParser {
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
  }

  // 读取字节
  readByte() {
    return this.data[this.pos++];
  }

  // 读取 16 位小端序整数
  readUint16() {
    const low = this.readByte();
    const high = this.readByte();
    return low | (high << 8);
  }

  // 读取指定长度的字节数组
  readBytes(length) {
    const bytes = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
      bytes[i] = this.readByte();
    }
    return bytes;
  }

  // 跳过字节
  skipBytes(length) {
    this.pos += length;
  }

  // 解析 GIF 文件
  async parse(arrayBuffer) {
    this.data = new Uint8Array(arrayBuffer);
    this.pos = 0;
    this.frames = [];

    try {
      // 1. 解析文件头
      this.parseHeader();
      
      // 2. 解析逻辑屏幕描述符
      this.parseLogicalScreenDescriptor();
      
      // 3. 解析全局颜色表（如果存在）
      if (this.globalColorTableFlag) {
        this.parseGlobalColorTable();
      }
      
      // 4. 解析数据流
      this.parseDataStream();
      
      console.log(`GIF 解析完成: ${this.frames.length} 帧, 尺寸: ${this.width}x${this.height}`);
      return {
        width: this.width,
        height: this.height,
        frames: this.frames
      };
      
    } catch (error) {
      console.error('GIF 解析错误:', error);
      throw error;
    }
  }

  // 解析文件头
  parseHeader() {
    const signature = String.fromCharCode(...this.readBytes(6));
    if (signature !== 'GIF87a' && signature !== 'GIF89a') {
      throw new Error('不是有效的 GIF 文件');
    }
    console.log('GIF 版本:', signature);
  }

  // 解析逻辑屏幕描述符
  parseLogicalScreenDescriptor() {
    this.width = this.readUint16();
    this.height = this.readUint16();
    
    const packed = this.readByte();
    this.globalColorTableFlag = (packed & 0x80) !== 0;
    const colorResolution = (packed & 0x70) >> 4;
    const sortFlag = (packed & 0x08) !== 0;
    this.globalColorTableSize = 2 << (packed & 0x07);
    
    this.backgroundColorIndex = this.readByte();
    const pixelAspectRatio = this.readByte();
    
    console.log(`屏幕尺寸: ${this.width}x${this.height}, 全局颜色表: ${this.globalColorTableFlag}, 颜色数: ${this.globalColorTableSize}`);
  }

  // 解析全局颜色表
  parseGlobalColorTable() {
    this.globalColorTable = [];
    for (let i = 0; i < this.globalColorTableSize; i++) {
      const r = this.readByte();
      const g = this.readByte();
      const b = this.readByte();
      this.globalColorTable.push([r, g, b]);
    }
    console.log('全局颜色表解析完成:', this.globalColorTable.length, '种颜色');
  }

  // 解析数据流
  parseDataStream() {
    while (this.pos < this.data.length) {
      const separator = this.readByte();
      
      switch (separator) {
        case 0x21: // 扩展引导符
          this.parseExtension();
          break;
        case 0x2C: // 图像分隔符
          this.parseImage();
          break;
        case 0x3B: // 文件结束符
          console.log('到达文件结尾');
          return;
        default:
          console.warn('未知分隔符:', separator.toString(16));
          return;
      }
    }
  }

  // 解析扩展
  parseExtension() {
    const label = this.readByte();
    
    switch (label) {
      case 0xF9: // 图形控制扩展
        this.parseGraphicControlExtension();
        break;
      case 0xFE: // 注释扩展
        this.skipDataSubBlocks();
        break;
      case 0xFF: // 应用程序扩展
        this.skipDataSubBlocks();
        break;
      default:
        console.log('跳过扩展:', label.toString(16));
        this.skipDataSubBlocks();
        break;
    }
  }

  // 解析图形控制扩展
  parseGraphicControlExtension() {
    const blockSize = this.readByte(); // 应该是 4
    const packed = this.readByte();
    
    const disposalMethod = (packed & 0x1C) >> 2;
    const userInputFlag = (packed & 0x02) !== 0;
    const transparentColorFlag = (packed & 0x01) !== 0;
    
    const delayTime = this.readUint16(); // 1/100 秒
    const transparentColorIndex = this.readByte();
    
    this.readByte(); // 块结束符
    
    // 保存当前帧的控制信息
    this.currentFrameControl = {
      disposalMethod,
      userInputFlag,
      transparentColorFlag,
      delayTime: delayTime * 10, // 转换为毫秒
      transparentColorIndex
    };
    
    console.log('图形控制扩展:', this.currentFrameControl);
  }

  // 跳过数据子块
  skipDataSubBlocks() {
    let blockSize;
    do {
      blockSize = this.readByte();
      this.skipBytes(blockSize);
    } while (blockSize !== 0);
  }

  // 解析图像
  parseImage() {
    const left = this.readUint16();
    const top = this.readUint16();
    const width = this.readUint16();
    const height = this.readUint16();
    
    const packed = this.readByte();
    const localColorTableFlag = (packed & 0x80) !== 0;
    const interlaceFlag = (packed & 0x40) !== 0;
    const sortFlag = (packed & 0x20) !== 0;
    const localColorTableSize = localColorTableFlag ? 2 << (packed & 0x07) : 0;
    
    console.log(`图像: ${width}x${height} at (${left}, ${top}), 本地颜色表: ${localColorTableFlag}`);
    
    // 解析本地颜色表（如果存在）
    let colorTable = this.globalColorTable;
    if (localColorTableFlag) {
      colorTable = [];
      for (let i = 0; i < localColorTableSize; i++) {
        const r = this.readByte();
        const g = this.readByte();
        const b = this.readByte();
        colorTable.push([r, g, b]);
      }
    }
    
    // 解析 LZW 压缩的图像数据
    const lzwMinimumCodeSize = this.readByte();
    const imageData = this.readImageData();
    
    // 解压 LZW 数据
    const pixels = this.decompressLZW(imageData, lzwMinimumCodeSize);
    
    // 创建帧数据
    const frame = {
      left,
      top,
      width,
      height,
      pixels,
      colorTable,
      interlaceFlag,
      delay: this.currentFrameControl ? this.currentFrameControl.delayTime : 100,
      disposalMethod: this.currentFrameControl ? this.currentFrameControl.disposalMethod : 0,
      transparentColorFlag: this.currentFrameControl ? this.currentFrameControl.transparentColorFlag : false,
      transparentColorIndex: this.currentFrameControl ? this.currentFrameControl.transparentColorIndex : 0
    };
    
    this.frames.push(frame);
    this.currentFrameControl = null; // 重置控制信息
  }

  // 读取图像数据
  readImageData() {
    const data = [];
    let blockSize;
    do {
      blockSize = this.readByte();
      for (let i = 0; i < blockSize; i++) {
        data.push(this.readByte());
      }
    } while (blockSize !== 0);
    return new Uint8Array(data);
  }

  // LZW 解压缩
  decompressLZW(data, minCodeSize) {
    const clearCode = 1 << minCodeSize;
    const endCode = clearCode + 1;
    let codeSize = minCodeSize + 1;
    let nextCode = endCode + 1;
    
    const codeTable = [];
    const output = [];
    
    // 初始化代码表
    for (let i = 0; i < clearCode; i++) {
      codeTable[i] = [i];
    }
    
    let bitPos = 0;
    let currentCode = null;
    let previousCode = null;
    
    while (bitPos < data.length * 8) {
      // 读取下一个代码
      const code = this.readLZWCode(data, bitPos, codeSize);
      bitPos += codeSize;
      
      if (code === clearCode) {
        // 清除代码表
        codeTable.length = endCode + 1;
        for (let i = 0; i < clearCode; i++) {
          codeTable[i] = [i];
        }
        codeSize = minCodeSize + 1;
        nextCode = endCode + 1;
        previousCode = null;
        continue;
      }
      
      if (code === endCode) {
        break;
      }
      
      let sequence;
      if (code < codeTable.length) {
        sequence = codeTable[code];
      } else if (code === nextCode && previousCode !== null) {
        sequence = [...codeTable[previousCode], codeTable[previousCode][0]];
      } else {
        throw new Error('无效的 LZW 代码');
      }
      
      output.push(...sequence);
      
      if (previousCode !== null && nextCode < 4096) {
        codeTable[nextCode] = [...codeTable[previousCode], sequence[0]];
        nextCode++;
        
        // 增加代码大小
        if (nextCode === (1 << codeSize) && codeSize < 12) {
          codeSize++;
        }
      }
      
      previousCode = code;
    }
    
    return new Uint8Array(output);
  }

  // 读取 LZW 代码
  readLZWCode(data, bitPos, codeSize) {
    let code = 0;
    for (let i = 0; i < codeSize; i++) {
      const byteIndex = Math.floor((bitPos + i) / 8);
      const bitIndex = (bitPos + i) % 8;
      if (byteIndex < data.length) {
        const bit = (data[byteIndex] >> bitIndex) & 1;
        code |= bit << i;
      }
    }
    return code;
  }

  // 分析帧差异，生成差分数据
  analyzeFrameDifferences() {
    if (this.frames.length === 0) return [];
    
    const canvases = this.processAllFrames();
    const frameDiffs = [];
    
    for (let i = 0; i < canvases.length; i++) {
      const currentCanvas = canvases[i];
      const currentImageData = currentCanvas.getContext('2d').getImageData(0, 0, this.width, this.height);
      
      if (i === 0) {
        // 第一帧：全部像素
        const pixels = this.extractPixelData(currentImageData);
        frameDiffs.push({
          frameIndex: i,
          type: 'full',
          pixels: pixels,
          pixelCount: pixels.length,
          delay: this.frames[i].delay || 100
        });
      } else {
        // 后续帧：计算差异
        const prevCanvas = canvases[i - 1];
        const prevImageData = prevCanvas.getContext('2d').getImageData(0, 0, this.width, this.height);
        
        const changedPixels = this.compareFrames(prevImageData, currentImageData);
        const fullPixels = this.extractPixelData(currentImageData);
        
        // 判断使用差分还是全帧（如果变化超过50%，使用全帧）
        const changeRatio = changedPixels.length / fullPixels.length;
        
        if (changeRatio > 0.5) {
          frameDiffs.push({
            frameIndex: i,
            type: 'full',
            pixels: fullPixels,
            pixelCount: fullPixels.length,
            delay: this.frames[i].delay || 100,
            changeRatio: changeRatio
          });
        } else {
          frameDiffs.push({
            frameIndex: i,
            type: 'diff',
            pixels: changedPixels,
            pixelCount: changedPixels.length,
            delay: this.frames[i].delay || 100,
            changeRatio: changeRatio
          });
        }
      }
    }
    
    return frameDiffs;
  }
  
  // 提取像素数据
  extractPixelData(imageData) {
    const pixels = [];
    const data = imageData.data;
    
    for (let y = 0; y < this.height; y++) {
      for (let x = 0; x < this.width; x++) {
        const index = (y * this.width + x) * 4;
        const r = data[index];
        const g = data[index + 1];
        const b = data[index + 2];
        const a = data[index + 3];
        
        // 只保存非透明像素
        if (a > 0) {
          pixels.push({
            x: x,
            y: y,
            r: r,
            g: g,
            b: b
          });
        }
      }
    }
    
    return pixels;
  }
  
  // 比较两帧，返回变化的像素
  compareFrames(prevImageData, currentImageData) {
    const changedPixels = [];
    const prevData = prevImageData.data;
    const currentData = currentImageData.data;
    
    for (let y = 0; y < this.height; y++) {
      for (let x = 0; x < this.width; x++) {
        const index = (y * this.width + x) * 4;
        
        const prevR = prevData[index];
        const prevG = prevData[index + 1];
        const prevB = prevData[index + 2];
        const prevA = prevData[index + 3];
        
        const currentR = currentData[index];
        const currentG = currentData[index + 1];
        const currentB = currentData[index + 2];
        const currentA = currentData[index + 3];
        
        // 检查像素是否发生变化
        if (prevR !== currentR || prevG !== currentG || prevB !== currentB || prevA !== currentA) {
          // 只保存非透明的当前像素
          if (currentA > 0) {
            changedPixels.push({
              x: x,
              y: y,
              r: currentR,
              g: currentG,
              b: currentB
            });
          }
        }
      }
    }
    
    return changedPixels;
  }
  
  // 生成 ESP32 兼容的动画数据
  generateESP32AnimationData() {
    const frameDiffs = this.analyzeFrameDifferences();
    
    // 统计信息
    let totalPixels = 0;
    let totalDiffPixels = 0;
    let fullFrames = 0;
    let diffFrames = 0;
    
    frameDiffs.forEach(frame => {
      totalPixels += frame.pixelCount;
      if (frame.type === 'full') {
        fullFrames++;
      } else {
        diffFrames++;
        totalDiffPixels += frame.pixelCount;
      }
    });
    
    // 激进优化：大幅减少数据量
    const maxFrames = Math.min(8, frameDiffs.length); // 最多8帧
    const maxPixelsPerFrame = 150; // 每帧最多150像素
    
    const optimizedFrames = [];
    for (let i = 0; i < maxFrames; i++) {
      const frame = frameDiffs[i];
      let pixels = frame.pixels;
      
      // 如果像素太多，简单截断
      if (pixels.length > maxPixelsPerFrame) {
        pixels = pixels.slice(0, maxPixelsPerFrame);
      }
      
      // 紧凑格式
      const esp32Frame = {
        t: frame.type === 'full' ? 1 : 0,
        d: frame.delay,
        c: pixels.length,
        p: pixels.map(pixel => [pixel.x, pixel.y, pixel.r, pixel.g, pixel.b])
      };
      
      optimizedFrames.push(esp32Frame);
    }
    
    const animationData = {
      width: this.width,
      height: this.height,
      frameCount: frameDiffs.length,
      frames: frameDiffs,
      statistics: {
        totalPixels: totalPixels,
        totalDiffPixels: totalDiffPixels,
        fullFrames: fullFrames,
        diffFrames: diffFrames,
        compressionRatio: totalDiffPixels > 0 ? (totalPixels / totalDiffPixels).toFixed(2) : 'N/A',
        estimatedMemory: this.estimateMemoryUsage(frameDiffs)
      },
      // ESP32 兼容格式
      esp32Format: {
        f: optimizedFrames.length,
        d: optimizedFrames
      }
    };
    
    return animationData;
  }

  
  // 验证 ESP32 数据的正确性
  validateESP32Data(esp32Data) {
    const canvases = this.processAllFrames();
    const validation = {
      isValid: true,
      errors: [],
      frameValidation: []
    };
    
    for (let i = 0; i < esp32Data.frames.length; i++) {
      const esp32Frame = esp32Data.frames[i];
      const canvas = canvases[i];
      const frameValidation = {
        frameIndex: i,
        isValid: true,
        errors: [],
        pixelMatches: 0,
        pixelMismatches: 0
      };
      
      // 创建一个测试 Canvas 来验证 ESP32 数据
      const testCanvas = document.createElement('canvas');
      testCanvas.width = this.width;
      testCanvas.height = this.height;
      const testCtx = testCanvas.getContext('2d');
      
      // 模拟 ESP32 的渲染逻辑
      if (i === 0 || esp32Frame.type === 'full') {
        // 全帧渲染：清空画布，然后绘制所有像素
        testCtx.fillStyle = 'black';
        testCtx.fillRect(0, 0, this.width, this.height);
        
        esp32Frame.pixels.forEach(pixel => {
          testCtx.fillStyle = `rgb(${pixel.r}, ${pixel.g}, ${pixel.b})`;
          testCtx.fillRect(pixel.x, pixel.y, 1, 1);
        });
      } else {
        // 差分渲染：复制前一帧，然后更新变化的像素
        const prevTestCanvas = document.getElementById(`testCanvas_${i-1}`);
        if (prevTestCanvas) {
          testCtx.drawImage(prevTestCanvas, 0, 0);
        }
        
        esp32Frame.pixels.forEach(pixel => {
          testCtx.fillStyle = `rgb(${pixel.r}, ${pixel.g}, ${pixel.b})`;
          testCtx.fillRect(pixel.x, pixel.y, 1, 1);
        });
      }
      
      // 保存测试 Canvas 供下一帧使用
      testCanvas.id = `testCanvas_${i}`;
      testCanvas.style.display = 'none';
      document.body.appendChild(testCanvas);
      
      // 比较测试 Canvas 和原始 Canvas
      const originalImageData = canvas.getContext('2d').getImageData(0, 0, this.width, this.height);
      const testImageData = testCtx.getImageData(0, 0, this.width, this.height);
      
      for (let j = 0; j < originalImageData.data.length; j += 4) {
        const originalR = originalImageData.data[j];
        const originalG = originalImageData.data[j + 1];
        const originalB = originalImageData.data[j + 2];
        
        const testR = testImageData.data[j];
        const testG = testImageData.data[j + 1];
        const testB = testImageData.data[j + 2];
        
        if (originalR === testR && originalG === testG && originalB === testB) {
          frameValidation.pixelMatches++;
        } else {
          frameValidation.pixelMismatches++;
          if (frameValidation.pixelMismatches <= 5) { // 只记录前5个错误
            const pixelIndex = j / 4;
            const x = pixelIndex % this.width;
            const y = Math.floor(pixelIndex / this.width);
            frameValidation.errors.push({
              x, y,
              expected: {r: originalR, g: originalG, b: originalB},
              actual: {r: testR, g: testG, b: testB}
            });
          }
        }
      }
      
      if (frameValidation.pixelMismatches > 0) {
        frameValidation.isValid = false;
        validation.isValid = false;
      }
      
      validation.frameValidation.push(frameValidation);
      
      // 清理测试 Canvas
      setTimeout(() => {
        const testCanvas = document.getElementById(`testCanvas_${i}`);
        if (testCanvas) {
          document.body.removeChild(testCanvas);
        }
      }, 1000);
    }
    
    return validation;
  }
  estimateMemoryUsage(frameDiffs) {
    let totalBytes = 0;
    
    frameDiffs.forEach(frame => {
      // 每个像素：x(1) + y(1) + r(1) + g(1) + b(1) = 5 字节
      // 帧头：type(1) + pixelCount(2) + delay(2) = 5 字节
      totalBytes += 5 + (frame.pixelCount * 5);
    });
    
    return {
      bytes: totalBytes,
      kb: (totalBytes / 1024).toFixed(2),
      mb: (totalBytes / (1024 * 1024)).toFixed(3)
    };
  }
  processAllFrames() {
    const canvases = [];
    let previousCanvas = null;
    
    for (let i = 0; i < this.frames.length; i++) {
      const frame = this.frames[i];
      const canvas = document.createElement('canvas');
      canvas.width = this.width;
      canvas.height = this.height;
      const ctx = canvas.getContext('2d');
      
      // 创建图像数据
      const imageData = ctx.createImageData(this.width, this.height);
      const data = imageData.data;
      
      // 处理帧叠加逻辑
      if (i === 0) {
        // 第一帧：填充背景色
        const bgColor = this.globalColorTable && this.globalColorTable[this.backgroundColorIndex] 
          ? this.globalColorTable[this.backgroundColorIndex] 
          : [255, 255, 255]; // 默认白色背景
        
        for (let j = 0; j < data.length; j += 4) {
          data[j] = bgColor[0];     // R
          data[j + 1] = bgColor[1]; // G
          data[j + 2] = bgColor[2]; // B
          data[j + 3] = 255;        // A
        }
      } else if (previousCanvas) {
        // 后续帧：根据前一帧的处置方法处理
        const prevFrame = this.frames[i - 1];
        const prevImageData = previousCanvas.getContext('2d').getImageData(0, 0, this.width, this.height);
        
        // 根据处置方法处理
        switch (prevFrame.disposalMethod) {
          case 0: // 不处置，保留
          case 1: // 不处置，保留
            // 复制前一帧的数据
            for (let j = 0; j < data.length; j++) {
              data[j] = prevImageData.data[j];
            }
            break;
          case 2: // 恢复到背景色
            const bgColor = this.globalColorTable && this.globalColorTable[this.backgroundColorIndex] 
              ? this.globalColorTable[this.backgroundColorIndex] 
              : [255, 255, 255];
            for (let j = 0; j < data.length; j += 4) {
              data[j] = bgColor[0];
              data[j + 1] = bgColor[1];
              data[j + 2] = bgColor[2];
              data[j + 3] = 255;
            }
            break;
          case 3: // 恢复到前一帧
            // 暂时使用方法1
            for (let j = 0; j < data.length; j++) {
              data[j] = prevImageData.data[j];
            }
            break;
          default:
            // 默认保留前一帧
            for (let j = 0; j < data.length; j++) {
              data[j] = prevImageData.data[j];
            }
        }
      }
      
      // 绘制当前帧（只绘制非透明像素）
      for (let y = 0; y < frame.height; y++) {
        for (let x = 0; x < frame.width; x++) {
          const pixelIndex = y * frame.width + x;
          if (pixelIndex >= frame.pixels.length) continue;
          
          const colorIndex = frame.pixels[pixelIndex];
          
          // 检查透明色
          if (frame.transparentColorFlag && colorIndex === frame.transparentColorIndex) {
            continue; // 跳过透明像素，保留背景
          }
          
          // 获取颜色
          const color = frame.colorTable && frame.colorTable[colorIndex] 
            ? frame.colorTable[colorIndex] 
            : [0, 0, 0];
          
          const canvasX = frame.left + x;
          const canvasY = frame.top + y;
          
          // 确保在画布范围内
          if (canvasX >= 0 && canvasX < this.width && canvasY >= 0 && canvasY < this.height) {
            const dataIndex = (canvasY * this.width + canvasX) * 4;
            data[dataIndex] = color[0];     // R
            data[dataIndex + 1] = color[1]; // G
            data[dataIndex + 2] = color[2]; // B
            data[dataIndex + 3] = 255;      // A
          }
        }
      }
      
      ctx.putImageData(imageData, 0, 0);
      canvases.push(canvas);
      previousCanvas = canvas; // 保存当前帧作为下一帧的参考
    }
    
    return canvases;
  }

  // 将帧转换为 Canvas（已废弃，使用 processAllFrames）
  frameToCanvas(frameIndex) {
    console.warn('frameToCanvas 已废弃，请使用 processAllFrames');
    return null;
  }
}

// 导出 GIF 解析器
window.GIFParser = GIFParser;