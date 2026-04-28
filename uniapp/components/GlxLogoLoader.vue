<template>
  <view
    class="glx-logo-loader"
    :class="[
      `glx-logo-loader--${resolvedVariant}`,
      `glx-logo-loader--${resolvedSurface}`,
    ]"
    :style="loaderStyle"
  >
    <view
      v-for="index in 9"
      :key="index"
      class="glx-logo-loader__cell"
    ></view>
  </view>
</template>

<script>
const SUPPORTED_VARIANTS = ["chase", "flip", "stamp"];
const SUPPORTED_SURFACES = ["plain", "boxed"];

export default {
  name: "GlxLogoLoader",

  props: {
    variant: {
      type: String,
      default: "chase",
    },
    cellSize: {
      type: Number,
      default: 12,
    },
    gap: {
      type: Number,
      default: 4,
    },
    padding: {
      type: Number,
      default: 4,
    },
    borderWidth: {
      type: Number,
      default: 2,
    },
    shadowOffset: {
      type: Number,
      default: 4,
    },
    surface: {
      type: String,
      default: "plain",
    },
  },

  computed: {
    resolvedVariant() {
      if (SUPPORTED_VARIANTS.includes(this.variant)) {
        return this.variant;
      }
      return "chase";
    },

    resolvedSurface() {
      if (SUPPORTED_SURFACES.includes(this.surface)) {
        return this.surface;
      }
      return "plain";
    },

    resolvedBorderWidth() {
      if (this.resolvedSurface === "boxed") {
        return this.borderWidth;
      }
      return 0;
    },

    resolvedShadowOffset() {
      if (this.resolvedSurface === "boxed") {
        return this.shadowOffset;
      }
      return 0;
    },

    totalSize() {
      return (
        this.cellSize * 3 +
        this.gap * 2 +
        this.padding * 2 +
        this.resolvedBorderWidth * 2
      );
    },

    loaderStyle() {
      return {
        "--glx-logo-loader-cell-size": `${this.cellSize}rpx`,
        "--glx-logo-loader-gap": `${this.gap}rpx`,
        "--glx-logo-loader-padding": `${this.padding}rpx`,
        "--glx-logo-loader-border-width": `${this.resolvedBorderWidth}rpx`,
        "--glx-logo-loader-shadow-offset": `${this.resolvedShadowOffset}rpx`,
        width: `${this.totalSize}rpx`,
        height: `${this.totalSize}rpx`,
      };
    },
  },
};
</script>

<style scoped>
.glx-logo-loader {
  display: flex;
  flex-wrap: wrap;
  align-content: flex-start;
  box-sizing: border-box;
  padding: var(--glx-logo-loader-padding);
  border: var(--glx-logo-loader-border-width) solid transparent;
  background: transparent;
  box-shadow: none;
}

.glx-logo-loader--boxed {
  border-color: #000000;
  background: #ffffff;
  box-shadow: var(--glx-logo-loader-shadow-offset) var(--glx-logo-loader-shadow-offset) 0 #000000;
}

.glx-logo-loader__cell {
  width: var(--glx-logo-loader-cell-size);
  height: var(--glx-logo-loader-cell-size);
  margin-right: var(--glx-logo-loader-gap);
  margin-bottom: var(--glx-logo-loader-gap);
  background: #f97316;
  box-sizing: border-box;
  transform-origin: center;
  backface-visibility: hidden;
}

.glx-logo-loader__cell:nth-child(3n) {
  margin-right: 0;
}

.glx-logo-loader__cell:nth-child(n + 7) {
  margin-bottom: 0;
}

.glx-logo-loader__cell:nth-child(3) {
  background: #ef4444;
}

.glx-logo-loader__cell:nth-child(5),
.glx-logo-loader__cell:nth-child(6) {
  background: #fbbf24;
}

.glx-logo-loader__cell:nth-child(8),
.glx-logo-loader__cell:nth-child(9) {
  background: #3b82f6;
}

.glx-logo-loader--chase .glx-logo-loader__cell {
  animation: glx-loader-chase 1.05s steps(1, end) infinite;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(1) {
  animation-delay: 0s;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(2) {
  animation-delay: 0.08s;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(3) {
  animation-delay: 0.16s;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(6) {
  animation-delay: 0.24s;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(9) {
  animation-delay: 0.32s;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(8) {
  animation-delay: 0.4s;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(7) {
  animation-delay: 0.48s;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(4) {
  animation-delay: 0.56s;
}

.glx-logo-loader--chase .glx-logo-loader__cell:nth-child(5) {
  animation-delay: 0.64s;
}

.glx-logo-loader--flip .glx-logo-loader__cell {
  animation: glx-loader-flip 1.2s ease-in-out infinite;
}

.glx-logo-loader--flip .glx-logo-loader__cell:nth-child(odd) {
  animation-delay: 0.12s;
}

.glx-logo-loader--flip .glx-logo-loader__cell:nth-child(3n) {
  animation-delay: 0.24s;
}

.glx-logo-loader--stamp .glx-logo-loader__cell {
  animation: glx-loader-stamp 1.1s steps(1, end) infinite;
}

.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(1),
.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(5),
.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(9) {
  animation-delay: 0s;
}

.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(2),
.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(4),
.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(6),
.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(8) {
  animation-delay: 0.1s;
}

.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(3),
.glx-logo-loader--stamp .glx-logo-loader__cell:nth-child(7) {
  animation-delay: 0.2s;
}

@keyframes glx-loader-chase {
  0%,
  100% {
    opacity: 0.45;
    transform: scale(0.82);
  }

  45% {
    opacity: 1;
    transform: scale(1);
  }
}

@keyframes glx-loader-flip {
  0%,
  100% {
    transform: rotateY(0deg);
    opacity: 0.65;
  }

  50% {
    transform: rotateY(180deg);
    opacity: 1;
  }
}

@keyframes glx-loader-stamp {
  0%,
  100% {
    transform: scale(0.6);
    opacity: 0.35;
  }

  45% {
    transform: scale(1);
    opacity: 1;
  }
}
</style>
