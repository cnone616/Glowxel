import { ARTKAL_COLORS_FULL } from "../../../data/artkal-colors-full.js";

export default {
  methods: {
    detectPixelArt(imageData, width, height) {
      const data = imageData.data;
      const colorSet = new Set();
      let totalPixels = 0;
      const sampleRate = Math.max(1, Math.floor((width * height) / 10000));

      for (let i = 0; i < data.length; i += 4 * sampleRate) {
        const alpha = data[i + 3];
        if (alpha < 128) continue;

        const color = `${data[i]},${data[i + 1]},${data[i + 2]}`;
        colorSet.add(color);
        totalPixels++;
      }

      const uniqueColors = colorSet.size;
      const colorRatio = uniqueColors / totalPixels;
      return colorRatio < 0.3 && uniqueColors < 256;
    },

    chooseImage() {
      uni.chooseImage({
        count: 1,
        sizeType: ["compressed"],
        sourceType: ["album", "camera"],
        success: (res) => {
          this.cropperSrc = res.tempFilePaths[0];
          this.showCropper = true;
        },
      });
    },

    onCropConfirm(cropData) {
      this.showCropper = false;
      this.isCropped = true;
      this.cropParams = cropData;

      // #ifdef H5
      const img = new Image();
      img.crossOrigin = "anonymous";
      img.onload = () => {
        const canvas = document.createElement("canvas");
        canvas.width = cropData.sw;
        canvas.height = cropData.sh;
        const ctx = canvas.getContext("2d");

        ctx.drawImage(
          img,
          cropData.sx,
          cropData.sy,
          cropData.sw,
          cropData.sh,
          0,
          0,
          cropData.sw,
          cropData.sh,
        );

        const dataUrl = canvas.toDataURL("image/png");
        this.imageFile = dataUrl;
        this.previewUrl = dataUrl;
        this.recommendedSize = null;
        this._detectImageContentSize(dataUrl);
      };
      img.src = cropData.src;
      // #endif

      // #ifdef MP-WEIXIN
      this.imageFile = cropData.src;
      this.previewUrl = cropData.src;
      this.recommendedSize = null;
      this._detectImageContentSize(cropData.src);
      // #endif
    },

    onCropCancel() {
      this.showCropper = false;
    },

    _detectImageContentSize(filePath) {
      // #ifdef H5
      const img = new Image();
      img.onload = () => {
        const canvas = document.createElement("canvas");
        canvas.width = img.width;
        canvas.height = img.height;
        const ctx = canvas.getContext("2d");
        ctx.drawImage(img, 0, 0);
        const data = ctx.getImageData(0, 0, img.width, img.height).data;

        let minX = img.width;
        let minY = img.height;
        let maxX = 0;
        let maxY = 0;
        let hasContent = false;

        for (let y = 0; y < img.height; y++) {
          for (let x = 0; x < img.width; x++) {
            if (data[(y * img.width + x) * 4 + 3] >= 128) {
              hasContent = true;
              if (x < minX) minX = x;
              if (x > maxX) maxX = x;
              if (y < minY) minY = y;
              if (y > maxY) maxY = y;
            }
          }
        }

        if (hasContent) {
          this._applyRecommendedSize(maxX - minX + 1, maxY - minY + 1);
        }
      };
      img.src = filePath;
      // #endif

      // #ifdef MP-WEIXIN
      uni.getImageInfo({
        src: filePath,
        success: (info) => {
          this._applyRecommendedSize(info.width, info.height);
        },
      });
      // #endif
    },

    _gcd(a, b) {
      while (b) {
        [a, b] = [b, a % b];
      }
      return a;
    },

    _applyRecommendedSize(contentW, contentH) {
      const gcd = this._gcd(contentW, contentH);
      const ratioW = contentW / gcd;
      const ratioH = contentH / gcd;

      this.contentRatio = { w: ratioW, h: ratioH };
      this.recommendedSize = { width: contentW, height: contentH };
      console.log(
        `内容尺寸: ${contentW}x${contentH}，宽高比: ${ratioW}:${ratioH}`,
      );
    },

    applyRecommendedSize() {
      if (!this.recommendedSize) {
        return;
      }

      this.customWidth = this.recommendedSize.width;
      this.customHeight = this.recommendedSize.height;
      this.toast.showSuccess(
        `已应用原始尺寸 ${this.recommendedSize.width}×${this.recommendedSize.height}`,
      );
    },

    clearImage() {
      this.imageFile = null;
      this.previewUrl = null;
      this.previewPixels = null;
      this.usedColors = [];
      this.recommendedSize = null;
      this.contentRatio = null;
      this.isCropped = false;
      this.cropParams = null;
    },

    async generatePreview() {
      if (!this.imageFile) {
        return;
      }

      this.isProcessing = true;
      this.previewImageUrl = "";
      this.toast.showInfo("正在生成预览...");

      try {
        const selectedArtkalColors = Array.from(this.selectedColors)
          .map((code) => ARTKAL_COLORS_FULL.find((color) => color.code === code))
          .filter((color) => color !== undefined);
        const artkalPalette = selectedArtkalColors.map((color) => color.hex);

        // #ifdef H5
        const img = new Image();
        img.crossOrigin = "anonymous";
        img.onload = () => {
          const tempCanvas = document.createElement("canvas");
          tempCanvas.width = img.width;
          tempCanvas.height = img.height;
          const tempCtx = tempCanvas.getContext("2d");
          tempCtx.drawImage(img, 0, 0);
          const detectData = tempCtx.getImageData(0, 0, img.width, img.height);
          const isPixelArt = this.detectPixelArt(
            detectData,
            img.width,
            img.height,
          );

          if (isPixelArt) {
            this.toast.showSuccess("检测到像素画，已自动优化处理");
          }

          const canvas = document.createElement("canvas");
          canvas.width = this.customWidth;
          canvas.height = this.customHeight;
          const ctx = canvas.getContext("2d");

          if (!ctx) {
            this.toast.showError("Canvas 初始化失败");
            this.isProcessing = false;
            return;
          }

          if (this.isCropped) {
            ctx.imageSmoothingEnabled = !isPixelArt;
            if (!isPixelArt) ctx.imageSmoothingQuality = "high";
            ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight);
          } else if (isPixelArt) {
            const boundsCanvas = document.createElement("canvas");
            boundsCanvas.width = img.width;
            boundsCanvas.height = img.height;
            const boundsCtx = boundsCanvas.getContext("2d");
            boundsCtx.drawImage(img, 0, 0);
            const boundsData = boundsCtx.getImageData(
              0,
              0,
              img.width,
              img.height,
            ).data;

            let minX = img.width;
            let minY = img.height;
            let maxX = 0;
            let maxY = 0;

            for (let py = 0; py < img.height; py++) {
              for (let px = 0; px < img.width; px++) {
                if (boundsData[(py * img.width + px) * 4 + 3] >= 128) {
                  if (px < minX) minX = px;
                  if (px > maxX) maxX = px;
                  if (py < minY) minY = py;
                  if (py > maxY) maxY = py;
                }
              }
            }

            const contentW = maxX - minX + 1;
            const contentH = maxY - minY + 1;
            ctx.imageSmoothingEnabled = false;
            ctx.drawImage(
              boundsCanvas,
              minX,
              minY,
              contentW,
              contentH,
              0,
              0,
              this.customWidth,
              this.customHeight,
            );
          } else {
            ctx.imageSmoothingEnabled = true;
            ctx.imageSmoothingQuality = "high";

            const boundsData = tempCtx.getImageData(
              0,
              0,
              img.width,
              img.height,
            ).data;
            let minX = img.width;
            let minY = img.height;
            let maxX = 0;
            let maxY = 0;
            let hasContent = false;

            for (let py = 0; py < img.height; py++) {
              for (let px = 0; px < img.width; px++) {
                if (boundsData[(py * img.width + px) * 4 + 3] >= 128) {
                  hasContent = true;
                  if (px < minX) minX = px;
                  if (px > maxX) maxX = px;
                  if (py < minY) minY = py;
                  if (py > maxY) maxY = py;
                }
              }
            }

            if (
              hasContent &&
              (minX > 0 ||
                minY > 0 ||
                maxX < img.width - 1 ||
                maxY < img.height - 1)
            ) {
              const contentW = maxX - minX + 1;
              const contentH = maxY - minY + 1;
              ctx.drawImage(
                tempCanvas,
                minX,
                minY,
                contentW,
                contentH,
                0,
                0,
                this.customWidth,
                this.customHeight,
              );
            } else {
              ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight);
            }
          }

          const imageData = ctx.getImageData(
            0,
            0,
            this.customWidth,
            this.customHeight,
          );
          this.processImageData(
            imageData.data,
            artkalPalette,
            selectedArtkalColors,
          );
        };

        img.onerror = () => {
          this.toast.showError("图片加载失败");
          this.isProcessing = false;
        };

        img.src = this.imageFile;
        // #endif

        // #ifdef MP-WEIXIN
        await new Promise((resolve, reject) => {
          uni.getImageInfo({
            src: this.imageFile,
            success: resolve,
            fail: reject,
          });
        });

        await new Promise((resolve) => setTimeout(resolve, 500));

        const query = uni.createSelectorQuery().in(this);
        query
          .select("#tempProcessCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0] || !res[0].node) {
              this.toast.showError("Canvas 初始化失败，请重试");
              this.isProcessing = false;
              return;
            }

            const canvas = res[0].node;
            const ctx = canvas.getContext("2d", { willReadFrequently: true });

            if (!ctx) {
              this.toast.showError("Canvas context 初始化失败");
              this.isProcessing = false;
              return;
            }

            canvas.width = this.customWidth;
            canvas.height = this.customHeight;

            const img = canvas.createImage();
            img.onload = () => {
              const tempCanvas = canvas;
              const tempCtx = ctx;
              tempCanvas.width = img.width;
              tempCanvas.height = img.height;
              tempCtx.drawImage(img, 0, 0);
              const detectData = tempCtx.getImageData(
                0,
                0,
                img.width,
                img.height,
              );
              const isPixelArt = this.detectPixelArt(
                detectData,
                img.width,
                img.height,
              );

              if (isPixelArt) {
                this.toast.showSuccess("检测到像素画，已自动优化处理");
              }

              canvas.width = this.customWidth;
              canvas.height = this.customHeight;

              if (this.isCropped && this.cropParams) {
                ctx.imageSmoothingEnabled = !isPixelArt;
                ctx.drawImage(
                  img,
                  this.cropParams.sx,
                  this.cropParams.sy,
                  this.cropParams.sw,
                  this.cropParams.sh,
                  0,
                  0,
                  this.customWidth,
                  this.customHeight,
                );
                const imageData = ctx.getImageData(
                  0,
                  0,
                  this.customWidth,
                  this.customHeight,
                );
                this.processImageData(
                  imageData.data,
                  artkalPalette,
                  selectedArtkalColors,
                );
                return;
              }

              if (isPixelArt) {
                ctx.imageSmoothingEnabled = false;

                const scaleX = this.customWidth / img.width;
                const scaleY = this.customHeight / img.height;
                const scale = Math.max(scaleX, scaleY);
                const integerScale =
                  scale >= 1 ? Math.ceil(scale) : 1 / Math.ceil(1 / scale);
                const intermediateWidth = Math.round(img.width * integerScale);
                const intermediateHeight = Math.round(
                  img.height * integerScale,
                );

                const query2 = uni.createSelectorQuery().in(this);
                query2
                  .select("#importCanvas")
                  .fields({ node: true })
                  .exec((res2) => {
                    if (!res2 || !res2[0] || !res2[0].node) {
                      ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight);
                      const imageData = ctx.getImageData(
                        0,
                        0,
                        this.customWidth,
                        this.customHeight,
                      );
                      this.processImageData(
                        imageData.data,
                        artkalPalette,
                        selectedArtkalColors,
                      );
                      return;
                    }

                    const intermediateCanvas = res2[0].node;
                    const intermediateCtx = intermediateCanvas.getContext("2d");
                    intermediateCanvas.width = intermediateWidth;
                    intermediateCanvas.height = intermediateHeight;
                    intermediateCtx.imageSmoothingEnabled = false;
                    intermediateCtx.drawImage(
                      img,
                      0,
                      0,
                      intermediateWidth,
                      intermediateHeight,
                    );

                    const offsetX = Math.floor(
                      (intermediateWidth - this.customWidth) / 2,
                    );
                    const offsetY = Math.floor(
                      (intermediateHeight - this.customHeight) / 2,
                    );

                    ctx.drawImage(
                      intermediateCanvas,
                      Math.max(0, offsetX),
                      Math.max(0, offsetY),
                      Math.min(intermediateWidth, this.customWidth),
                      Math.min(intermediateHeight, this.customHeight),
                      Math.max(0, -offsetX),
                      Math.max(0, -offsetY),
                      Math.min(intermediateWidth, this.customWidth),
                      Math.min(intermediateHeight, this.customHeight),
                    );

                    const imageData = ctx.getImageData(
                      0,
                      0,
                      this.customWidth,
                      this.customHeight,
                    );
                    this.processImageData(
                      imageData.data,
                      artkalPalette,
                      selectedArtkalColors,
                    );
                  });
              } else {
                ctx.imageSmoothingEnabled = true;

                const bData = detectData.data;
                let minX = img.width;
                let minY = img.height;
                let maxX = 0;
                let maxY = 0;
                let hasContent = false;

                for (let py = 0; py < img.height; py++) {
                  for (let px = 0; px < img.width; px++) {
                    if (bData[(py * img.width + px) * 4 + 3] >= 128) {
                      hasContent = true;
                      if (px < minX) minX = px;
                      if (px > maxX) maxX = px;
                      if (py < minY) minY = py;
                      if (py > maxY) maxY = py;
                    }
                  }
                }

                canvas.width = this.customWidth;
                canvas.height = this.customHeight;

                if (
                  hasContent &&
                  (minX > 0 ||
                    minY > 0 ||
                    maxX < img.width - 1 ||
                    maxY < img.height - 1)
                ) {
                  const contentW = maxX - minX + 1;
                  const contentH = maxY - minY + 1;
                  ctx.drawImage(
                    img,
                    minX,
                    minY,
                    contentW,
                    contentH,
                    0,
                    0,
                    this.customWidth,
                    this.customHeight,
                  );
                } else {
                  ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight);
                }

                const imageData = ctx.getImageData(
                  0,
                  0,
                  this.customWidth,
                  this.customHeight,
                );
                this.processImageData(
                  imageData.data,
                  artkalPalette,
                  selectedArtkalColors,
                );
              }
            };

            img.onerror = (err) => {
              console.error("图片加载失败:", err);
              this.toast.showError("图片加载失败");
              this.isProcessing = false;
            };

            img.src = this.imageFile;
          });
        // #endif
      } catch (error) {
        console.error("生成预览失败:", error);
        this.toast.showError("生成预览失败，请重试");
        this.isProcessing = false;
      }
    },

    processImageData(data, artkalPalette, selectedArtkalColors) {
      const offsetX = Math.floor((this.paddedWidth - this.customWidth) / 2);
      const offsetY = Math.floor((this.paddedHeight - this.customHeight) / 2);
      const artkalPixels = new Map();
      const colorUsage = new Map();

      for (let y = 0; y < this.customHeight; y++) {
        for (let x = 0; x < this.customWidth; x++) {
          const idx = (y * this.customWidth + x) * 4;
          const alpha = data[idx + 3];
          if (alpha < 128) continue;

          const r = data[idx];
          const g = data[idx + 1];
          const b = data[idx + 2];
          let minDist = Infinity;
          let closestColor = artkalPalette[0];

          for (const artkalHex of artkalPalette) {
            const artkalRgb = this.hexToRgb(artkalHex);
            const dist = Math.sqrt(
              Math.pow(r - artkalRgb[0], 2) +
                Math.pow(g - artkalRgb[1], 2) +
                Math.pow(b - artkalRgb[2], 2),
            );

            if (dist < minDist) {
              minDist = dist;
              closestColor = artkalHex;
            }
          }

          artkalPixels.set(`${x + offsetX},${y + offsetY}`, closestColor);
          colorUsage.set(closestColor, (colorUsage.get(closestColor) || 0) + 1);
        }
      }

      this.previewPixels = artkalPixels;
      this.usedColors = Array.from(colorUsage.entries())
        .map(([hex, count]) => {
          const colorInfo = selectedArtkalColors.find((color) => color.hex === hex);
          return {
            hex,
            code: colorInfo?.code || "",
            name: colorInfo?.name || "",
            count,
          };
        })
        .sort((a, b) => b.count - a.count);

      this.drawPreview(artkalPixels);
      this.isProcessing = false;
      this.stepAnimationClass = "step-exit";
      setTimeout(() => {
        this.step = 3;
        this.stepAnimationClass = "step-enter";
      }, 150);
    },

    drawPreview(pixels) {
      setTimeout(() => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select("#previewDisplayCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0]) {
              setTimeout(() => {
                this.drawPreview(pixels);
              }, 500);
              return;
            }

            const canvas = res[0].node;
            if (!canvas) {
              return;
            }

            const ctx = canvas.getContext("2d");
            if (!ctx) {
              return;
            }

            const containerWidth = res[0].width;
            const containerHeight = res[0].height;
            canvas.width = containerWidth;
            canvas.height = containerHeight;
            ctx.clearRect(0, 0, containerWidth, containerHeight);

            const maxDimension = Math.max(this.paddedWidth, this.paddedHeight);

            let multiplier = 1;
            // #ifdef H5
            multiplier = maxDimension > 64 ? 3 : 2;
            // #endif
            // #ifdef MP-WEIXIN
            multiplier = maxDimension > 64 ? 1.5 : 1;
            // #endif

            let scale = Math.min(
              containerWidth / (maxDimension * multiplier),
              containerHeight / (maxDimension * multiplier),
            );

            if (scale >= 4) {
              // #ifdef H5
              scale = 1.65;
              // #endif
              // #ifdef MP-WEIXIN
              scale = 4;
              // #endif
            } else {
              // #ifdef H5
              scale = Math.round(scale * 100) / 100;
              // #endif
              // #ifdef MP-WEIXIN
              scale = (Math.round(scale * 100) / 100) * 1.5;
              // #endif
            }

            const contentWidth = this.paddedWidth * scale;
            const contentHeight = this.paddedHeight * scale;
            const canvasCenterOffsetX = (containerWidth - contentWidth) / 2;
            const canvasCenterOffsetY = (containerHeight - contentHeight) / 2;
            const centerOffsetX = (maxDimension - this.paddedWidth) / 2;
            const centerOffsetY = (maxDimension - this.paddedHeight) / 2;

            pixels.forEach((color, key) => {
              const [x, y] = key.split(",").map(Number);
              ctx.fillStyle = color;
              ctx.fillRect(
                canvasCenterOffsetX + (x + centerOffsetX) * scale,
                canvasCenterOffsetY + (y + centerOffsetY) * scale,
                scale,
                scale,
              );
            });

            // #ifdef MP-WEIXIN
            setTimeout(() => {
              uni.canvasToTempFilePath(
                {
                  canvas: canvas,
                  success: (fileRes) => {
                    this.previewImageUrl = fileRes.tempFilePath;
                  },
                  fail: (err) => {
                    console.error("Canvas转图片失败:", err);
                  },
                },
                this,
              );
            }, 100);
            // #endif
          });
      }, 500);
    },

    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return result
        ? [
            parseInt(result[1], 16),
            parseInt(result[2], 16),
            parseInt(result[3], 16),
          ]
        : [0, 0, 0];
    },
  },
};
