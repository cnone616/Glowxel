import { GIFParser } from "../../../utils/gifParser.js";

export default {
  methods: {
    adjustImageValue(key, delta) {
      const limits = {
        width: { min: 1, max: 256 },
        height: { min: 1, max: 256 },
        x: { min: -128, max: 128 },
        y: { min: -128, max: 128 },
      };

      const { min, max } = limits[key];
      const currentValue = this.config.image[key];
      const newValue = Math.max(min, Math.min(max, currentValue + delta));

      if (newValue !== currentValue) {
        this.config.image[key] = newValue;

        if (key === "x" || key === "y") {
          this.drawCanvas();
        } else if (
          (key === "width" || key === "height") &&
          this.config.image.data
        ) {
          if (this._resizeDebounceTimer) {
            clearTimeout(this._resizeDebounceTimer);
          }
          this._resizeDebounceTimer = setTimeout(() => {
            this._resizeDebounceTimer = null;
            if (this.gifAnimationData && this._gifParser) {
              this.pauseGifAnimation();
              const targetW = this.config.image.width || 64;
              const targetH = this.config.image.height || 64;
              const firstFrame = this._gifParser.renderFrame(
                0,
                targetW,
                targetH,
              );
              if (firstFrame) {
                const pixelMap = new Map();
                for (let y = 0; y < targetH; y++) {
                  for (let x = 0; x < targetW; x++) {
                    const idx = (y * targetW + x) * 4;
                    const r = firstFrame.rgba[idx];
                    const g = firstFrame.rgba[idx + 1];
                    const b = firstFrame.rgba[idx + 2];
                    if (r > 0 || g > 0 || b > 0) {
                      pixelMap.set(
                        `${x},${y}`,
                        "#" +
                          ((1 << 24) + (r << 16) + (g << 8) + b)
                            .toString(16)
                            .slice(1),
                      );
                    }
                  }
                }
                this.imagePixels = pixelMap;
                this._needsFullRender = true;
                this.gifRenderedFrameMaps = null;
                this.gifFrameIndex = 0;
              }
            } else {
              this.convertImageToPixels(this.config.image.data);
            }
            this.drawCanvas();
          }, 300);
        }

        if ((key === "width" || key === "height") && newValue > 64) {
          this.toast.showInfo(
            `${key === "width" ? "宽度" : "高度"}为 ${newValue}，超出部分不会显示`,
          );
        }
      }
    },

    setSquareSize() {
      const size = Math.min(
        this.config.image.width,
        this.config.image.height,
        64,
      );
      this.config.image.width = size;
      this.config.image.height = size;

      if (this.config.image.data) {
        this.convertImageToPixels(this.config.image.data);
      }

      this.toast.showSuccess(`已设置为 ${size}x${size}`);
    },

    toggleImageShow() {
      this.config.image.show = !this.config.image.show;
      this.drawCanvas();
    },

    chooseImage() {
      uni.chooseMessageFile({
        count: 1,
        type: "image",
        success: (res) => {
          const file = res.tempFiles[0];
          const tempFilePath = file.path || file.tempFilePath;
          const fileName = (file.name || tempFilePath || "").toLowerCase();
          const isGif = fileName.endsWith(".gif");

          console.log(
            "选择文件:",
            fileName,
            "是否GIF:",
            isGif,
            "模式:",
            this.clockMode,
          );

          if (isGif && this.clockMode === "animation") {
            this._handleGifFile(tempFilePath);
          } else {
            if (isGif && this.clockMode === "clock") {
              this.toast.showInfo("静态时钟模式下 GIF 将作为静态图片使用");
            }
            this._handleStaticImage(tempFilePath);
          }
        },
        fail: (err) => {
          console.error("选择文件失败:", err);
          this._fallbackChooseImage();
        },
      });
    },

    _fallbackChooseImage() {
      uni.chooseImage({
        count: 1,
        sizeType: ["compressed"],
        sourceType: ["album", "camera"],
        success: (res) => {
          this._handleStaticImage(res.tempFilePaths[0]);
        },
        fail: (err) => {
          console.error("选择图片失败:", err);
        },
      });
    },

    _handleStaticImage(tempFilePath) {
      this.stopGifAnimation();
      this.gifAnimationData = null;
      this.gifRenderedFrameMaps = null;
      this.gifFrameIndex = 0;
      uni.getFileSystemManager().readFile({
        filePath: tempFilePath,
        encoding: "base64",
        success: (fileRes) => {
          const imageData = "data:image/png;base64," + fileRes.data;
          this.config.image.data = imageData;
          this.config.image.show = true;
          this.convertImageToPixels(imageData);
          this.toast.showSuccess("图片已上传");
        },
        fail: (err) => {
          console.error("读取图片失败:", err);
          this.toast.showError("图片读取失败");
        },
      });
    },

    _handleGifFile(tempFilePath) {
      uni.getFileSystemManager().readFile({
        filePath: tempFilePath,
        success: (fileRes) => {
          try {
            this._saveGifToLocal(fileRes.data);
            this._parseAndInitGif(fileRes.data);

            uni.getFileSystemManager().readFile({
              filePath: tempFilePath,
              encoding: "base64",
              success: (b64Res) => {
                this.config.image.data = "data:image/gif;base64," + b64Res.data;
                this.config.image.show = true;
                this.drawCanvas();
                this.toast.showSuccess(
                  `GIF 已解析！${this.gifAnimationData.frameCount} 帧`,
                );
              },
            });
          } catch (err) {
            console.error("GIF 解析失败:", err);
            this.toast.showError("GIF 解析失败: " + err.message);
            this._handleStaticImage(tempFilePath);
          }
        },
        fail: (err) => {
          console.error("读取 GIF 失败:", err);
          this.toast.showError("GIF 文件读取失败");
        },
      });
    },

    _parseAndInitGif(arrayBuffer) {
      const parser = new GIFParser();
      parser.parse(arrayBuffer);
      this._gifParser = parser;

      const targetW = this.config.image.width || 64;
      const targetH = this.config.image.height || 64;
      const renderedFrames = parser.renderFrames(targetW, targetH);

      console.log(
        `GIF 配置: width=${targetW}, height=${targetH}, offsetX=${this.config.image.x || 0}, offsetY=${this.config.image.y || 0}`,
      );

      const offsetX = this.config.image.x || 0;
      const offsetY = this.config.image.y || 0;
      if (targetW + offsetX > 64 || targetH + offsetY > 64) {
        console.error(
          `⚠️ 配置错误：图片尺寸 ${targetW}x${targetH} + 偏移 (${offsetX},${offsetY}) 超出 64x64 屏幕！`,
        );
        this.toast.showError("图片尺寸 + 偏移超出屏幕范围，请调整配置");
        return;
      }

      this.gifAnimationData = parser.generateESP32Data(
        targetW,
        targetH,
        20,
        null,
        offsetX,
        offsetY,
        renderedFrames,
      );
      console.log(`GIF 动画数据: ${this.gifAnimationData.frameCount} 帧`);

      this.gifRenderedFrameMaps = renderedFrames.map((frame) => ({
        rgba: frame.rgba,
        delay: frame.delay || 100,
        width: targetW,
        height: targetH,
      }));

      if (this.gifRenderedFrameMaps.length > 0) {
        this.imagePixels = this._rgbaFrameToPixelMap(
          this.gifRenderedFrameMaps[0],
        );
        this.gifIsPlaying = false;
        this.gifFrameIndex = 0;
      }
      console.log("GIF 帧已生成:", this.gifRenderedFrameMaps.length, "帧");
    },

    _rgbaFrameToPixelMap(frame) {
      const { rgba, width, height } = frame;
      const pixelMap = new Map();
      for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
          const idx = (y * width + x) * 4;
          const r = rgba[idx];
          const g = rgba[idx + 1];
          const b = rgba[idx + 2];
          const a = rgba[idx + 3];
          if (a > 10 && (r > 0 || g > 0 || b > 0)) {
            pixelMap.set(
              `${x},${y}`,
              `#${[r, g, b].map((v) => v.toString(16).padStart(2, "0")).join("")}`,
            );
          }
        }
      }
      return pixelMap;
    },

    _saveGifToLocal(arrayBuffer) {
      try {
        const filePath = `${uni.env.USER_DATA_PATH}/clock_gif.bin`;
        uni.getFileSystemManager().writeFileSync(filePath, arrayBuffer);
        console.log("GIF 二进制已保存到:", filePath);
      } catch (e) {
        console.error("保存 GIF 文件失败:", e);
      }
    },

    _restoreGifFromLocal() {
      try {
        const filePath = `${uni.env.USER_DATA_PATH}/clock_gif.bin`;
        const fs = uni.getFileSystemManager();
        fs.access({
          path: filePath,
          success: () => {
            fs.readFile({
              filePath,
              success: (res) => {
                try {
                  this._parseAndInitGif(res.data);
                  const base64 = uni.arrayBufferToBase64(res.data);
                  this.config.image.data = "data:image/gif;base64," + base64;
                  this.config.image.show = true;
                  if (this.canvasCtx) {
                    this.drawCanvas();
                  }
                  console.log("GIF 动画已从本地恢复");
                } catch (e) {
                  console.error("恢复 GIF 解析失败:", e);
                }
              },
              fail: (err) => console.error("读取本地 GIF 失败:", err),
            });
          },
          fail: () => console.log("没有保存的 GIF 文件"),
        });
      } catch (e) {
        console.error("恢复 GIF 失败:", e);
      }
    },

    _deleteLocalGif() {
      try {
        const filePath = `${uni.env.USER_DATA_PATH}/clock_gif.bin`;
        uni.getFileSystemManager().unlink({ filePath, fail: () => {} });
      } catch (e) {
        // ignore
      }
    },

    async convertImageToPixels(imageData) {
      const conversionToken = ++this._imageConvertToken;
      const targetWidth = this.config.image.width;
      const targetHeight = this.config.image.height;

      console.log(
        "开始转换图片为像素数据，目标尺寸:",
        targetWidth,
        "x",
        targetHeight,
      );

      const query = uni.createSelectorQuery().in(this);
      query
        .select("#imageProcessCanvas")
        .fields({ node: true, size: true })
        .exec((res) => {
          if (conversionToken !== this._imageConvertToken) {
            return;
          }
          if (!res || !res[0] || !res[0].node) {
            console.error("图片处理 Canvas 查询失败");
            this.toast.showError("Canvas 未就绪");
            return;
          }

          const canvas = res[0].node;
          const ctx = canvas.getContext("2d");
          const img = canvas.createImage();

          img.onload = () => {
            if (conversionToken !== this._imageConvertToken) {
              return;
            }
            console.log("图片加载成功，原始尺寸:", img.width, "x", img.height);
            try {
              const originalWidth = canvas.width;
              const originalHeight = canvas.height;
              canvas.width = targetWidth;
              canvas.height = targetHeight;

              ctx.imageSmoothingEnabled = false;
              ctx.drawImage(img, 0, 0, targetWidth, targetHeight);

              const imgData = ctx.getImageData(0, 0, targetWidth, targetHeight);
              const pixels = imgData.data;

              canvas.width = originalWidth;
              canvas.height = originalHeight;
              const dpr = uni.getSystemInfoSync().pixelRatio || 1;
              ctx.scale(dpr, dpr);

              const pixelMap = new Map();
              for (let y = 0; y < targetHeight; y++) {
                for (let x = 0; x < targetWidth; x++) {
                  const idx = (y * targetWidth + x) * 4;
                  const r = pixels[idx];
                  const g = pixels[idx + 1];
                  const b = pixels[idx + 2];
                  const a = pixels[idx + 3];

                  if (a > 10) {
                    const hex = `#${[r, g, b].map((v) => v.toString(16).padStart(2, "0")).join("")}`;
                    pixelMap.set(`${x},${y}`, hex);
                  }
                }
              }

              this.imagePixels = pixelMap;
              console.log(
                "图片转换完成，像素数量:",
                pixelMap.size,
                "/ 总数:",
                targetWidth * targetHeight,
              );

              this.drawCanvas();
            } catch (err) {
              console.error("提取像素失败:", err);
              this.toast.showError("图片处理失败");
            }
          };

          img.onerror = (err) => {
            if (conversionToken !== this._imageConvertToken) {
              return;
            }
            console.error("图片加载失败:", err);
            this.toast.showError("图片加载失败");
          };

          img.src = imageData;
        });
    },

    startGifAnimation() {
      this.stopGifAnimation();

      if (this._needsFullRender && this._gifParser) {
        this._needsFullRender = false;
        const targetW = this.config.image.width || 64;
        const targetH = this.config.image.height || 64;
        const renderedFrames = this._gifParser.renderFrames(targetW, targetH);
        this.gifRenderedFrameMaps = renderedFrames.map((frame) => ({
          rgba: frame.rgba,
          delay: frame.delay || 100,
          width: targetW,
          height: targetH,
        }));
        this.gifFrameIndex = 0;
        this.imagePixels = this._rgbaFrameToPixelMap(
          this.gifRenderedFrameMaps[0],
        );
      }

      if (!this.gifRenderedFrameMaps || this.gifRenderedFrameMaps.length <= 1) {
        return;
      }

      if (this._gifParser) {
        const targetW = this.config.image.width || 64;
        const targetH = this.config.image.height || 64;
        this.gifAnimationData = this._gifParser.generateESP32Data(
          targetW,
          targetH,
          20,
          null,
          this.config.image.x || 0,
          this.config.image.y || 0,
          this.gifRenderedFrameMaps,
        );
      }

      this.gifIsPlaying = true;
      this.gifFrameIndex = 0;
      const playNextFrame = () => {
        if (!this.gifRenderedFrameMaps || !this.gifIsPlaying) {
          return;
        }

        this.gifFrameIndex =
          (this.gifFrameIndex + 1) % this.gifRenderedFrameMaps.length;
        const frameData = this.gifRenderedFrameMaps[this.gifFrameIndex];
        this.imagePixels = this._rgbaFrameToPixelMap(frameData);
        this.drawGIFFrame();

        const delay = Math.max(
          16,
          (frameData.delay || 100) / this.gifPlaySpeed,
        );
        this.gifTimer = setTimeout(playNextFrame, delay);
      };

      const firstDelay = Math.max(
        16,
        (this.gifRenderedFrameMaps[0].delay || 100) / this.gifPlaySpeed,
      );
      this.gifTimer = setTimeout(playNextFrame, firstDelay);
    },

    pauseGifAnimation() {
      this.gifIsPlaying = false;
      if (this.gifTimer) {
        clearTimeout(this.gifTimer);
        this.gifTimer = null;
      }
    },

    stopGifAnimation() {
      this.gifIsPlaying = false;
      if (this.gifTimer) {
        clearTimeout(this.gifTimer);
        this.gifTimer = null;
      }
    },

    removeImage() {
      this._imageConvertToken += 1;
      this.stopGifAnimation();
      this.config.image.data = null;
      this.config.image.show = false;
      this.imagePixels = null;
      this.gifAnimationData = null;
      this.gifRenderedFrameMaps = null;
      this.gifFrameIndex = 0;
      this._gifParser = null;
      this._deleteLocalGif();
      this.toast.showInfo("图片已删除");
      this.drawCanvas();
    },
  },
};
