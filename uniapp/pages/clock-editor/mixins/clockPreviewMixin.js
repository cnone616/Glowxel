import {
  drawClockTextToPixels,
  getClockFont,
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
      this.drawCanvas();
      return Promise.resolve();
    },

    ensureValidPreviewFont() {
      if (getClockFont(this.config.font)) {
        return true;
      }

      const defaultConfig = this.getDefaultConfigSnapshot();
      if (!defaultConfig || !getClockFont(defaultConfig.font)) {
        return false;
      }

      this.config.font = defaultConfig.font;
      return true;
    },

    drawCanvas() {
      if (!this.ensureValidPreviewFont()) {
        this.previewPixels = new Map();
        return;
      }
      this.previewPixels = this.buildAllPreviewPixels();
    },

    drawGIFFrame() {
      if (!this.ensureValidPreviewFont()) {
        this.previewPixels = new Map();
        return;
      }
      this.previewPixels = this.buildAllPreviewPixels();
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

    getDefaultPreviewPixels() {
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

    buildAllPreviewPixels() {
      const pixels = this.buildImageLayerPixels();

      if (this.showPreview === false) {
        return pixels;
      }

      this.getPreviewPixels().forEach((color, key) => pixels.set(key, color));
      return pixels;
    },

    buildImageLayerPixels() {
      const pixels = new Map();

      if (!this.config.image.show || !this.imagePixels) {
        return pixels;
      }

      const offsetX = this.config.image.x;
      const offsetY = this.config.image.y;

      this.imagePixels.forEach((color, key) => {
        const [rawX, rawY] = key.split(",").map(Number);
        const finalX = rawX + offsetX;
        const finalY = rawY + offsetY;

        if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
          pixels.set(`${finalX},${finalY}`, color);
        }
      });

      return pixels;
    },

    getDefaultConfigSnapshot() {
      if (!this.$options || typeof this.$options.data !== "function") {
        return null;
      }

      const initialState = this.$options.data.call(this);
      if (!initialState || !initialState.config) {
        return null;
      }

      return JSON.parse(JSON.stringify(initialState.config));
    },

    isReadablePreviewColor(color) {
      if (typeof color !== "string" || !/^#?[0-9a-fA-F]{6}$/.test(color)) {
        return false;
      }

      const normalized = color.startsWith("#") ? color : `#${color}`;
      const r = parseInt(normalized.slice(1, 3), 16);
      const g = parseInt(normalized.slice(3, 5), 16);
      const b = parseInt(normalized.slice(5, 7), 16);
      const luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

      return luminance >= 22;
    },

    hasReadablePreviewPixels(pixelMap) {
      if (!pixelMap || pixelMap.size === 0) {
        return false;
      }

      for (const color of pixelMap.values()) {
        if (this.isReadablePreviewColor(color)) {
          return true;
        }
      }

      return false;
    },

    ensureVisiblePreviewBaseline() {
      if (this.clockMode === "theme") {
        return;
      }

      const imageLayerPixels = this.buildImageLayerPixels();
      if (this.hasReadablePreviewPixels(imageLayerPixels)) {
        return;
      }

      const previewPixels = this.getDefaultPreviewPixels();
      if (this.hasReadablePreviewPixels(previewPixels)) {
        return;
      }

      const defaultConfig = this.getDefaultConfigSnapshot();
      if (!defaultConfig) {
        return;
      }

      this.config = defaultConfig;
      this.drawCanvas();
    },

    getPreviewPixels() {
      return this.getDefaultPreviewPixels();
    },
  },
};
