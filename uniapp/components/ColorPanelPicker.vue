<template>
  <view class="color-panel-picker">
    <view class="picker-header">
      <view class="picker-preview">
        <view
          class="preview-swatch"
          :style="{ backgroundColor: currentHex }"
        ></view>
        <view class="preview-meta">
          <text class="picker-label">{{ label }}</text>
          <text class="picker-value">{{ displayHex }}</text>
        </view>
      </view>

      <view v-if="presetColors.length > 0" class="preset-row">
        <view
          v-for="item in presetColors"
          :key="item.hex"
          class="preset-swatch"
          :class="{ active: currentHex === normalizeHex(item.hex) }"
          :style="{ backgroundColor: item.hex }"
          @click="selectPreset(item.hex)"
        ></view>
      </view>
    </view>

    <view
      :id="panelId"
      class="sv-panel"
      :style="{ backgroundColor: hueColor }"
      @touchstart.stop.prevent="handlePanelTouch"
      @touchmove.stop.prevent="handlePanelTouch"
    >
      <view class="sv-panel-white"></view>
      <view class="sv-panel-black"></view>
      <view
        class="sv-thumb"
        :style="{
          left: `${saturation}%`,
          top: `${100 - brightness}%`,
          backgroundColor: currentHex,
        }"
      ></view>
    </view>

    <view class="channel-row">
      <text class="channel-label">色相</text>
      <view
        :id="hueTrackId"
        class="hue-track"
        @touchstart.stop.prevent="handleHueTouch"
        @touchmove.stop.prevent="handleHueTouch"
      >
        <view
          class="hue-thumb"
          :style="{
            left: `${huePercent}%`,
            backgroundColor: hueColor,
          }"
        ></view>
      </view>
    </view>

    <view class="field-grid">
      <view class="field-item field-item-hex">
        <text class="field-label">HEX</text>
        <view class="field-input-shell">
          <input
            class="field-input"
            :value="hexInput"
            maxlength="7"
            @input="handleHexInput"
            @blur="commitHexInput"
            @confirm="commitHexInput"
          />
        </view>
      </view>

      <view class="field-item">
        <text class="field-label">R</text>
        <view class="field-input-shell">
          <input
            class="field-input"
            type="number"
            :value="redInput"
            maxlength="3"
            @input="handleRgbInput('r', $event)"
            @blur="commitRgbInput"
            @confirm="commitRgbInput"
          />
        </view>
      </view>

      <view class="field-item">
        <text class="field-label">G</text>
        <view class="field-input-shell">
          <input
            class="field-input"
            type="number"
            :value="greenInput"
            maxlength="3"
            @input="handleRgbInput('g', $event)"
            @blur="commitRgbInput"
            @confirm="commitRgbInput"
          />
        </view>
      </view>

      <view class="field-item">
        <text class="field-label">B</text>
        <view class="field-input-shell">
          <input
            class="field-input"
            type="number"
            :value="blueInput"
            maxlength="3"
            @input="handleRgbInput('b', $event)"
            @blur="commitRgbInput"
            @confirm="commitRgbInput"
          />
        </view>
      </view>
    </view>
  </view>
</template>

<script>
function clamp(value, min, max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

export default {
  props: {
    value: {
      type: String,
      required: true,
    },
    label: {
      type: String,
      default: "颜色",
    },
    presetColors: {
      type: Array,
      default() {
        return [];
      },
    },
  },
  data() {
    return {
      panelId: "",
      hueTrackId: "",
      panelRect: null,
      hueRect: null,
      hue: 0,
      saturation: 100,
      brightness: 100,
      hexInput: "#FFFFFF",
      redInput: "255",
      greenInput: "255",
      blueInput: "255",
    };
  },
  computed: {
    currentHex() {
      const rgb = this.hsvToRgb(this.hue, this.saturation, this.brightness);
      return this.rgbToHex(rgb.r, rgb.g, rgb.b);
    },
    displayHex() {
      return this.currentHex.toUpperCase();
    },
    hueColor() {
      const rgb = this.hsvToRgb(this.hue, 100, 100);
      return this.rgbToHex(rgb.r, rgb.g, rgb.b);
    },
    huePercent() {
      return (this.hue / 360) * 100;
    },
  },
  watch: {
    value(newValue) {
      const normalized = this.normalizeHex(newValue);
      if (normalized.length === 0) {
        return;
      }
      if (normalized !== this.currentHex) {
        this.syncFromHex(normalized);
      }
    },
  },
  created() {
    this.panelId = `color-panel-${this._uid}`;
    this.hueTrackId = `color-hue-${this._uid}`;
    this.syncFromHex(this.value);
  },
  mounted() {
    this.cacheRects();
  },
  methods: {
    cacheRects() {
      this.cachePanelRect();
      this.cacheHueRect();
    },
    cachePanelRect() {
      const query = uni.createSelectorQuery().in(this);
      query
        .select(`#${this.panelId}`)
        .boundingClientRect((rect) => {
          if (rect) {
            this.panelRect = rect;
          }
        })
        .exec();
    },
    cacheHueRect() {
      const query = uni.createSelectorQuery().in(this);
      query
        .select(`#${this.hueTrackId}`)
        .boundingClientRect((rect) => {
          if (rect) {
            this.hueRect = rect;
          }
        })
        .exec();
    },
    ensurePanelRect(forceRefresh) {
      return new Promise((resolve) => {
        if (forceRefresh !== true && this.panelRect) {
          resolve(this.panelRect);
          return;
        }
        const query = uni.createSelectorQuery().in(this);
        query
          .select(`#${this.panelId}`)
          .boundingClientRect((rect) => {
            if (rect) {
              this.panelRect = rect;
            }
            resolve(rect);
          })
          .exec();
      });
    },
    ensureHueRect(forceRefresh) {
      return new Promise((resolve) => {
        if (forceRefresh !== true && this.hueRect) {
          resolve(this.hueRect);
          return;
        }
        const query = uni.createSelectorQuery().in(this);
        query
          .select(`#${this.hueTrackId}`)
          .boundingClientRect((rect) => {
            if (rect) {
              this.hueRect = rect;
            }
            resolve(rect);
          })
          .exec();
      });
    },
    normalizeHex(value) {
      if (typeof value !== "string") {
        return "";
      }
      const raw = value.trim();
      const body = raw.startsWith("#") ? raw.slice(1) : raw;
      if (!/^[0-9a-fA-F]{6}$/.test(body)) {
        return "";
      }
      return `#${body.toLowerCase()}`;
    },
    sanitizeRgbInput(value) {
      if (typeof value !== "string") {
        return "0";
      }
      const digits = value.replace(/[^\d]/g, "");
      if (digits.length === 0) {
        return "0";
      }
      return String(clamp(Number(digits), 0, 255));
    },
    hexToRgb(hex) {
      const normalized = this.normalizeHex(hex);
      return {
        r: parseInt(normalized.slice(1, 3), 16),
        g: parseInt(normalized.slice(3, 5), 16),
        b: parseInt(normalized.slice(5, 7), 16),
      };
    },
    rgbToHex(r, g, b) {
      return `#${r.toString(16).padStart(2, "0")}${g
        .toString(16)
        .padStart(2, "0")}${b.toString(16).padStart(2, "0")}`;
    },
    rgbToHsv(r, g, b) {
      const red = r / 255;
      const green = g / 255;
      const blue = b / 255;
      const max = Math.max(red, green, blue);
      const min = Math.min(red, green, blue);
      const delta = max - min;
      let hue = 0;

      if (delta !== 0) {
        if (max === red) {
          hue = ((green - blue) / delta) % 6;
        } else if (max === green) {
          hue = (blue - red) / delta + 2;
        } else {
          hue = (red - green) / delta + 4;
        }
      }

      hue = Math.round(hue * 60);
      if (hue < 0) {
        hue = hue + 360;
      }

      const saturation = max === 0 ? 0 : Math.round((delta / max) * 100);
      const brightness = Math.round(max * 100);

      return { h: hue, s: saturation, v: brightness };
    },
    hsvToRgb(h, s, v) {
      const hue = ((h % 360) + 360) % 360;
      const saturation = clamp(s, 0, 100) / 100;
      const brightness = clamp(v, 0, 100) / 100;
      const chroma = brightness * saturation;
      const x = chroma * (1 - Math.abs(((hue / 60) % 2) - 1));
      const match = brightness - chroma;
      let red = 0;
      let green = 0;
      let blue = 0;

      if (hue < 60) {
        red = chroma;
        green = x;
      } else if (hue < 120) {
        red = x;
        green = chroma;
      } else if (hue < 180) {
        green = chroma;
        blue = x;
      } else if (hue < 240) {
        green = x;
        blue = chroma;
      } else if (hue < 300) {
        red = x;
        blue = chroma;
      } else {
        red = chroma;
        blue = x;
      }

      return {
        r: Math.round((red + match) * 255),
        g: Math.round((green + match) * 255),
        b: Math.round((blue + match) * 255),
      };
    },
    syncFromHex(hex) {
      const normalized = this.normalizeHex(hex);
      if (normalized.length === 0) {
        return;
      }
      const rgb = this.hexToRgb(normalized);
      const hsv = this.rgbToHsv(rgb.r, rgb.g, rgb.b);
      this.hue = hsv.h;
      this.saturation = hsv.s;
      this.brightness = hsv.v;
      this.syncInputFields(rgb.r, rgb.g, rgb.b);
    },
    syncInputFields(r, g, b) {
      this.hexInput = this.rgbToHex(r, g, b).toUpperCase();
      this.redInput = String(r);
      this.greenInput = String(g);
      this.blueInput = String(b);
    },
    emitCurrentColor() {
      const rgb = this.hsvToRgb(this.hue, this.saturation, this.brightness);
      const hex = this.rgbToHex(rgb.r, rgb.g, rgb.b);
      this.syncInputFields(rgb.r, rgb.g, rgb.b);
      this.$emit("input", hex);
      this.$emit("change", hex);
    },
    selectPreset(hex) {
      const normalized = this.normalizeHex(hex);
      if (normalized.length === 0) {
        return;
      }
      this.syncFromHex(normalized);
      this.$emit("input", normalized);
      this.$emit("change", normalized);
    },
    getTouchPoint(event) {
      if (event && event.touches && event.touches.length > 0) {
        return event.touches[0];
      }
      if (event && event.changedTouches && event.changedTouches.length > 0) {
        return event.changedTouches[0];
      }
      return null;
    },
    getTouchX(touch) {
      if (!touch) {
        return 0;
      }
      if (typeof touch.clientX === "number") {
        return touch.clientX;
      }
      if (typeof touch.x === "number") {
        return touch.x;
      }
      if (typeof touch.pageX === "number") {
        return touch.pageX;
      }
      return 0;
    },
    getTouchY(touch) {
      if (!touch) {
        return 0;
      }
      if (typeof touch.clientY === "number") {
        return touch.clientY;
      }
      if (typeof touch.y === "number") {
        return touch.y;
      }
      if (typeof touch.pageY === "number") {
        return touch.pageY;
      }
      return 0;
    },
    async handlePanelTouch(event) {
      const rect = await this.ensurePanelRect(true);
      const touch = this.getTouchPoint(event);
      if (!rect || !touch) {
        return;
      }
      const x = clamp(this.getTouchX(touch) - rect.left, 0, rect.width);
      const y = clamp(this.getTouchY(touch) - rect.top, 0, rect.height);
      this.saturation = Math.round((x / rect.width) * 100);
      this.brightness = Math.round(100 - (y / rect.height) * 100);
      this.emitCurrentColor();
    },
    async handleHueTouch(event) {
      const rect = await this.ensureHueRect(true);
      const touch = this.getTouchPoint(event);
      if (!rect || !touch) {
        return;
      }
      const x = clamp(this.getTouchX(touch) - rect.left, 0, rect.width);
      this.hue = Math.round((x / rect.width) * 360);
      if (this.hue > 359) {
        this.hue = 359;
      }
      this.emitCurrentColor();
    },
    handleHexInput(event) {
      if (!event || !event.detail) {
        return;
      }
      this.hexInput = event.detail.value.toUpperCase();
    },
    commitHexInput() {
      const normalized = this.normalizeHex(this.hexInput);
      if (normalized.length === 0) {
        this.syncInputFields(
          this.hexToRgb(this.currentHex).r,
          this.hexToRgb(this.currentHex).g,
          this.hexToRgb(this.currentHex).b,
        );
        return;
      }
      this.syncFromHex(normalized);
      this.$emit("input", normalized);
      this.$emit("change", normalized);
    },
    handleRgbInput(channel, event) {
      if (!event || !event.detail) {
        return;
      }
      const sanitized = this.sanitizeRgbInput(event.detail.value);
      if (channel === "r") {
        this.redInput = sanitized;
        return;
      }
      if (channel === "g") {
        this.greenInput = sanitized;
        return;
      }
      this.blueInput = sanitized;
    },
    commitRgbInput() {
      const r = clamp(Number(this.redInput), 0, 255);
      const g = clamp(Number(this.greenInput), 0, 255);
      const b = clamp(Number(this.blueInput), 0, 255);
      const hsv = this.rgbToHsv(r, g, b);
      this.hue = hsv.h;
      this.saturation = hsv.s;
      this.brightness = hsv.v;
      this.syncInputFields(r, g, b);
      const hex = this.rgbToHex(r, g, b);
      this.$emit("input", hex);
      this.$emit("change", hex);
    },
  },
};
</script>

<style scoped>
.color-panel-picker {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
  padding: 10rpx 0 4rpx;
  background: transparent;
  border: 0;
  border-radius: 0;
}

.picker-header {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.picker-preview {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 0 2rpx;
}

.preview-swatch {
  width: 72rpx;
  height: 72rpx;
  border-radius: 0;
  border: 2rpx solid #000000;
  box-shadow: none;
}

.preview-meta {
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.picker-label {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.picker-value {
  font-size: 28rpx;
  font-family: monospace;
  font-weight: 600;
  color: var(--text-primary);
}

.preset-row {
  display: flex;
  flex-wrap: wrap;
  gap: 10rpx;
  padding: 0 2rpx;
}

.preset-swatch {
  width: 42rpx;
  height: 42rpx;
  border-radius: 0;
  border: 3rpx solid #000000;
  box-shadow: none;
}

.preset-swatch.active {
  border-color: var(--nb-yellow);
  box-shadow: none;
}

.sv-panel {
  position: relative;
  height: 240rpx;
  border-radius: 0;
  overflow: hidden;
  border: 0;
}

.sv-panel-white {
  position: absolute;
  inset: 0;
  background: linear-gradient(to right, #ffffff, rgba(255, 255, 255, 0));
}

.sv-panel-black {
  position: absolute;
  inset: 0;
  background: linear-gradient(to top, #000000, rgba(0, 0, 0, 0));
}

.sv-thumb {
  position: absolute;
  width: 28rpx;
  height: 28rpx;
  border-radius: 0;
  border: 4rpx solid #ffffff;
  box-shadow: 0 0 0 2rpx rgba(0, 0, 0, 0.2);
  transform: translate(-14rpx, -14rpx);
}

.channel-row {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 0 2rpx;
}

.channel-label {
  width: 56rpx;
  font-size: 22rpx;
  color: var(--text-secondary);
}

.hue-track {
  position: relative;
  flex: 1;
  height: 24rpx;
  border-radius: 0;
  background: linear-gradient(
    to right,
    #ff0000 0%,
    #ffff00 17%,
    #00ff00 33%,
    #00ffff 50%,
    #0000ff 67%,
    #ff00ff 83%,
    #ff0000 100%
  );
  border: 0;
}

.hue-thumb {
  position: absolute;
  top: 50%;
  width: 26rpx;
  height: 26rpx;
  border-radius: 0;
  border: 4rpx solid #ffffff;
  box-shadow: 0 0 0 2rpx rgba(0, 0, 0, 0.16);
  transform: translate(-13rpx, -50%);
}

.field-grid {
  display: grid;
  grid-template-columns: 1.8fr 1fr 1fr 1fr;
  gap: 12rpx;
}

.field-item {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  min-width: 0;
}

.field-item-hex {
  grid-column: span 1;
}

.field-label {
  font-size: 20rpx;
  color: var(--text-secondary);
  text-transform: uppercase;
}

.field-input-shell {
  height: 64rpx;
  display: flex;
  align-items: center;
  padding: 0 14rpx;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.field-input {
  width: 100%;
  height: 100%;
  background: transparent;
  border: 0;
  color: var(--text-primary);
  font-size: 24rpx;
  box-sizing: border-box;
}
</style>
