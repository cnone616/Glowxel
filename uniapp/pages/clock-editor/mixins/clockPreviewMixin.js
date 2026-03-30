import {
  drawClockTextToPixels,
  getClockTextWidth,
  getClockTextHeight,
  getCurrentTimeText,
  getCurrentDateText,
  getCurrentWeekText,
} from "../../../utils/clockCanvas.js";

export default {
  methods: {
    getTimeText() {
      return getCurrentTimeText(this.config.showSeconds, this.config.hourFormat);
    },

    getDateText() {
      return getCurrentDateText();
    },

    getWeekText() {
      return getCurrentWeekText();
    },

    selectFont(fontId) {
      this.config.font = fontId;
      this.drawCanvas();
    },

    toggleShowSeconds() {
      this.config.showSeconds = !this.config.showSeconds;
      this.drawCanvas();
    },

    setHourFormat(hourFormat) {
      this.config.hourFormat = hourFormat;
      this.drawCanvas();
    },

    handleTimeAdjust(key, delta, min, max) {
      this.adjustValue("time", key, delta, min, max);
    },

    handleTimeColor(color) {
      this.updateConfigAndRedraw("time", "color", color);
    },

    handleTimeAlign(align) {
      this.setAlignment("time", align);
    },

    handleDateAdjust(key, delta, min, max) {
      this.adjustValue("date", key, delta, min, max);
    },

    handleDateColor(color) {
      this.updateConfigAndRedraw("date", "color", color);
    },

    handleDateAlign(align) {
      this.setAlignment("date", align);
    },

    handleWeekAdjust(key, delta, min, max) {
      this.adjustValue("week", key, delta, min, max);
    },

    handleWeekColor(color) {
      this.updateConfigAndRedraw("week", "color", color);
    },

    handleWeekAlign(align) {
      this.setAlignment("week", align);
    },

    adjustValue(section, key, delta, min, max) {
      const currentValue = this.config[section][key];
      const newValue = Math.max(min, Math.min(max, currentValue + delta));

      if (newValue !== currentValue) {
        this.config[section][key] = newValue;
        this.drawCanvas();
      }
    },

    updateConfigAndRedraw(section, key, value) {
      this.config[section][key] = value;
      this.drawCanvas();
    },

    setAlignment(section, align) {
      this.config[section].align = align;
      if (align === "left") {
        this.config[section].x = 0;
      } else if (align === "center") {
        this.config[section].x = 32;
      } else if (align === "right") {
        this.config[section].x = 63;
      }
      this.drawCanvas();
    },

    toggleTimeShow() {
      this.config.time.show = !this.config.time.show;
      this.drawCanvas();
    },

    toggleDateShow() {
      this.config.date.show = !this.config.date.show;
      this.drawCanvas();
    },

    toggleWeekShow() {
      this.config.week.show = !this.config.week.show;
      this.drawCanvas();
    },

    initCanvas() {
      return new Promise((resolve, reject) => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select("#clockCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            if (res && res[0]) {
              const canvas = res[0].node;
              this.canvasNode = canvas;
              this.canvasCtx = canvas.getContext("2d");

              const dpr = uni.getSystemInfoSync().pixelRatio || 1;
              canvas.width = 320 * dpr;
              canvas.height = 320 * dpr;
              this.canvasCtx.scale(dpr, dpr);

              console.log("Canvas 初始化成功");
              this.drawCanvas();
              resolve();
            } else {
              console.error("Canvas 节点获取失败");
              reject(new Error("Canvas 节点获取失败"));
            }
          });
      });
    },

    drawCanvas() {
      if (!this.canvasCtx || !this.canvasNode) return;
      const ctx = this.canvasCtx;
      const pixelSize = 5;

      ctx.fillStyle = "#000000";
      ctx.fillRect(0, 0, 320, 320);

      ctx.strokeStyle = "rgba(255, 255, 255, 0.15)";
      ctx.lineWidth = 0.5;
      for (let i = 0; i <= 64; i++) {
        ctx.beginPath();
        ctx.moveTo(i * pixelSize, 0);
        ctx.lineTo(i * pixelSize, 320);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(0, i * pixelSize);
        ctx.lineTo(320, i * pixelSize);
        ctx.stroke();
      }

      this._drawPixelLayer(ctx, pixelSize, false);

      if (this.showPreview) {
        this._drawClockLayer(ctx, pixelSize);
      }

      this._lastClockPixels = this.showPreview
        ? this.getPreviewPixels()
        : new Map();
    },

    drawGIFFrame() {
      if (!this.canvasCtx || !this.canvasNode) return;
      const ctx = this.canvasCtx;
      const pixelSize = 5;

      if (this._lastImagePixels) {
        const offsetX = this.config.image.x || 0;
        const offsetY = this.config.image.y || 0;
        this._lastImagePixels.forEach((_, key) => {
          const [rx, ry] = key.split(",").map(Number);
          const fx = rx + offsetX;
          const fy = ry + offsetY;
          if (fx >= 0 && fx < 64 && fy >= 0 && fy < 64) {
            ctx.fillStyle = "#000000";
            ctx.fillRect(fx * pixelSize, fy * pixelSize, pixelSize, pixelSize);
            ctx.strokeStyle = "rgba(255,255,255,0.15)";
            ctx.lineWidth = 0.5;
          }
        });
      }

      const offsetX = this.config.image.x || 0;
      const offsetY = this.config.image.y || 0;
      if (this.imagePixels) {
        this.imagePixels.forEach((color, key) => {
          const [rx, ry] = key.split(",").map(Number);
          const fx = rx + offsetX;
          const fy = ry + offsetY;
          if (fx >= 0 && fx < 64 && fy >= 0 && fy < 64) {
            ctx.fillStyle = color;
            ctx.fillRect(fx * pixelSize, fy * pixelSize, pixelSize, pixelSize);
          }
        });
      }
      this._lastImagePixels = this.imagePixels;

      if (this.showPreview) {
        this._redrawClockOnly(ctx, pixelSize);
      }
    },

    _redrawClockOnly(ctx, pixelSize) {
      if (this._lastClockPixels) {
        this._lastClockPixels.forEach((_, key) => {
          const [x, y] = key.split(",").map(Number);
          const imgKey = this._getImagePixelAt(x, y);
          ctx.fillStyle = imgKey || "#000000";
          ctx.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
        });
      }
      this._drawClockLayer(ctx, pixelSize);
      this._lastClockPixels = this.getPreviewPixels();
    },

    _getImagePixelAt(x, y) {
      if (!this.imagePixels) return null;
      const offsetX = this.config.image.x || 0;
      const offsetY = this.config.image.y || 0;
      const rx = x - offsetX;
      const ry = y - offsetY;
      return this.imagePixels.get(`${rx},${ry}`) || null;
    },

    _drawPixelLayer(ctx, pixelSize, includeClockText) {
      const allPixels = includeClockText
        ? this.getAllPixels()
        : (() => {
            const pixels = new Map();
            if (this.imagePixels) {
              const offsetX = this.config.image.x || 0;
              const offsetY = this.config.image.y || 0;
              this.imagePixels.forEach((color, key) => {
                const [rx, ry] = key.split(",").map(Number);
                const fx = rx + offsetX;
                const fy = ry + offsetY;
                if (fx >= 0 && fx < 64 && fy >= 0 && fy < 64) {
                  pixels.set(`${fx},${fy}`, color);
                }
              });
            }
            return pixels;
          })();

      allPixels.forEach((color, key) => {
        const [x, y] = key.split(",").map(Number);
        ctx.fillStyle = color;
        ctx.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
      });
    },

    _drawClockLayer(ctx, pixelSize) {
      const previewPixels = this.getPreviewPixels();
      previewPixels.forEach((color, key) => {
        const [x, y] = key.split(",").map(Number);
        ctx.fillStyle = color;
        ctx.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
      });
    },

    getClockExcludePixels() {
      const pixelSet = new Set();
      const dummyMap = new Map();
      const config = this.config;

      if (config.time.show) {
        const text = this.getTimeText();
        drawClockTextToPixels(
          text,
          config.time.x,
          config.time.y,
          "#ffffff",
          dummyMap,
          config.font,
          config.time.fontSize || 1,
          config.time.align || "left",
        );

        const size = config.time.fontSize || 1;
        const width = getClockTextWidth(text, config.font, size);
        const height = getClockTextHeight(config.font, size);
        let startX = config.time.x;
        if (config.time.align === "center") {
          startX = config.time.x - Math.floor(width / 2);
        } else if (config.time.align === "right") {
          startX = config.time.x - width;
        }

        for (let y = config.time.y; y < config.time.y + height && y < 64; y++) {
          for (let x = startX; x < startX + width && x < 64; x++) {
            if (x >= 0 && y >= 0) {
              pixelSet.add(`${x},${y}`);
            }
          }
        }
      }

      if (config.date.show) {
        const text = this.getDateText();
        drawClockTextToPixels(
          text,
          config.date.x,
          config.date.y,
          "#ffffff",
          dummyMap,
          config.font,
          config.date.fontSize || 1,
          config.date.align || "left",
        );

        const size = config.date.fontSize || 1;
        const width = getClockTextWidth(text, config.font, size);
        const height = getClockTextHeight(config.font, size);
        let startX = config.date.x;
        if (config.date.align === "center") {
          startX = config.date.x - Math.floor(width / 2);
        } else if (config.date.align === "right") {
          startX = config.date.x - width;
        }

        for (let y = config.date.y; y < config.date.y + height && y < 64; y++) {
          for (let x = startX; x < startX + width && x < 64; x++) {
            if (x >= 0 && y >= 0) {
              pixelSet.add(`${x},${y}`);
            }
          }
        }
      }

      if (config.week.show) {
        const text = this.getWeekText();
        drawClockTextToPixels(
          text,
          config.week.x,
          config.week.y,
          "#ffffff",
          dummyMap,
          config.font,
          1,
          config.week.align || "left",
        );

        const width = getClockTextWidth(text, config.font, 1);
        const height = getClockTextHeight(config.font, 1);
        let startX = config.week.x;
        if (config.week.align === "center") {
          startX = config.week.x - Math.floor(width / 2);
        } else if (config.week.align === "right") {
          startX = config.week.x - width;
        }

        for (let y = config.week.y; y < config.week.y + height && y < 64; y++) {
          for (let x = startX; x < startX + width && x < 64; x++) {
            if (x >= 0 && y >= 0) {
              pixelSet.add(`${x},${y}`);
            }
          }
        }
      }

      dummyMap.forEach((_, key) => pixelSet.add(key));
      return pixelSet;
    },

    getAllPixels() {
      const allPixels = new Map();

      if (this.imagePixels && this.imagePixels.size > 0) {
        const offsetX = this.config.image.x || 0;
        const offsetY = this.config.image.y || 0;
        this.imagePixels.forEach((color, key) => {
          const [rx, ry] = key.split(",").map(Number);
          const finalX = rx + offsetX;
          const finalY = ry + offsetY;
          if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
            allPixels.set(`${finalX},${finalY}`, color);
          }
        });
      }

      if (this.showPreview) {
        const previewPixels = this.getPreviewPixels();
        previewPixels.forEach((color, key) => {
          allPixels.set(key, color);
        });
      }

      return allPixels;
    },

    getPreviewPixels() {
      const pixels = new Map();

      if (this.config.time.show) {
        drawClockTextToPixels(
          this.getTimeText(),
          this.config.time.x,
          this.config.time.y,
          this.config.time.color,
          pixels,
          this.config.font,
          this.config.time.fontSize || 1,
          this.config.time.align || "left",
        );
      }

      if (this.config.date.show) {
        drawClockTextToPixels(
          this.getDateText(),
          this.config.date.x,
          this.config.date.y,
          this.config.date.color,
          pixels,
          this.config.font,
          this.config.date.fontSize || 1,
          this.config.date.align || "left",
        );
      }

      if (this.config.week.show) {
        drawClockTextToPixels(
          this.getWeekText(),
          this.config.week.x,
          this.config.week.y,
          this.config.week.color,
          pixels,
          this.config.font,
          1,
          this.config.week.align || "left",
        );
      }

      return pixels;
    },
  },
};
