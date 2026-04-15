<template>
  <view class="pixel-preview-board">
    <view class="pixel-preview-board__viewport">
      <view class="pixel-preview-board__surface" :style="surfaceStyle">
        <view
          v-for="pixel in pixelItems"
          :key="pixel.key"
          class="pixel-preview-board__pixel"
          :style="pixel.style"
        ></view>
        <view
          v-if="gridVisible"
          class="pixel-preview-board__grid"
          :style="gridStyle"
        ></view>
      </view>
    </view>
  </view>
</template>

<script>
export default {
  props: {
    width: {
      type: Number,
      required: true,
    },
    height: {
      type: Number,
      required: true,
    },
    pixels: {
      type: Map,
      required: true,
    },
    refreshToken: {
      type: Number,
      default: 0,
    },
    zoom: {
      type: Number,
      default: 4,
    },
    offsetX: {
      type: Number,
      default: 0,
    },
    offsetY: {
      type: Number,
      default: 0,
    },
    gridVisible: {
      type: Boolean,
      default: true,
    },
    isDarkMode: {
      type: Boolean,
      default: false,
    },
  },
  computed: {
    boardPixelSize() {
      return Math.max(2, Math.floor(this.zoom || 4));
    },
    boardWidth() {
      return this.width * this.boardPixelSize;
    },
    boardHeight() {
      return this.height * this.boardPixelSize;
    },
    surfaceStyle() {
      return {
        width: `${this.boardWidth}px`,
        height: `${this.boardHeight}px`,
        left: `${Math.round(this.offsetX)}px`,
        top: `${Math.round(this.offsetY)}px`,
      };
    },
    gridStyle() {
      const alpha = this.isDarkMode ? 0.28 : 0.16;
      const borderAlpha = this.isDarkMode ? 0.42 : 0.22;
      const gridColor = this.isDarkMode
        ? `rgba(238, 246, 255, ${alpha})`
        : `rgba(90, 100, 114, ${alpha})`;
      const borderColor = this.isDarkMode
        ? `rgba(238, 246, 255, ${borderAlpha})`
        : `rgba(90, 100, 114, ${borderAlpha})`;

      return {
        backgroundImage: `linear-gradient(to right, ${gridColor} 1px, transparent 1px), linear-gradient(to bottom, ${gridColor} 1px, transparent 1px)`,
        backgroundSize: `${this.boardPixelSize}px ${this.boardPixelSize}px`,
        boxShadow: `inset 0 0 0 1px ${borderColor}`,
      };
    },
    pixelItems() {
      const refreshToken = this.refreshToken;
      const items = [];
      const pixelSize = this.boardPixelSize;

      void refreshToken;

      this.pixels.forEach((color, key) => {
        const [x, y] = key.split(",").map(Number);
        items.push({
          key,
          style: {
            left: `${x * pixelSize}px`,
            top: `${y * pixelSize}px`,
            width: `${pixelSize}px`,
            height: `${pixelSize}px`,
            backgroundColor: color,
          },
        });
      });

      return items;
    },
  },
};
</script>

<style scoped>
.pixel-preview-board {
  position: absolute;
  inset: 0;
  background: #000000;
  overflow: hidden;
}

.pixel-preview-board__viewport {
  position: relative;
  width: 100%;
  height: 100%;
  overflow: hidden;
}

.pixel-preview-board__surface {
  position: absolute;
  transform-origin: top left;
}

.pixel-preview-board__pixel {
  position: absolute;
}

.pixel-preview-board__grid {
  position: absolute;
  inset: 0;
  pointer-events: none;
}
</style>
