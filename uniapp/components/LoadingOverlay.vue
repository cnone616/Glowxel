<template>
  <view v-if="visible" class="glx-loading-overlay">
    <view class="glx-loading-card">
      <view class="glx-loading-logo-stage">
        <view class="glx-loading-logo" :class="activeVariantClass">
          <view
            v-for="cell in logoCells"
            :key="cell.key"
            class="glx-loading-cell"
            :class="{ 'glx-loading-cell--center': cell.isCenter }"
            :style="cell.style"
          ></view>
        </view>
      </view>
      <text class="glx-loading-text">{{ message }}</text>
    </view>
  </view>
</template>

<script>
import { bindLoadingInstance, unbindLoadingInstance } from '../composables/useToast.js'

const LOADING_VARIANTS = ['chase', 'flip', 'stamp']
const TRANSPORT_VARIANT = 'chase'
const TRANSPORT_MESSAGE_KEYWORDS = ['发送', '传输', '上传', '下发']

const LOGO_CELLS = [
  { key: 'cell-1', x: -1, y: -1, color: '#f97316', ring: 0, isCenter: false },
  { key: 'cell-2', x: 0, y: -1, color: '#f97316', ring: 1, isCenter: false },
  { key: 'cell-3', x: 1, y: -1, color: '#ef4444', ring: 2, isCenter: false },
  { key: 'cell-4', x: -1, y: 0, color: '#f97316', ring: 7, isCenter: false },
  { key: 'cell-5', x: 0, y: 0, color: '#fbbf24', ring: 8, isCenter: true },
  { key: 'cell-6', x: 1, y: 0, color: '#fbbf24', ring: 3, isCenter: false },
  { key: 'cell-7', x: -1, y: 1, color: '#f97316', ring: 6, isCenter: false },
  { key: 'cell-8', x: 0, y: 1, color: '#3b82f6', ring: 5, isCenter: false },
  { key: 'cell-9', x: 1, y: 1, color: '#3b82f6', ring: 4, isCenter: false }
].map((cell, index) => ({
  key: cell.key,
  isCenter: cell.isCenter,
  style: {
    '--tile-color': cell.color,
    '--x': String(cell.x),
    '--y': String(cell.y),
    '--i': String(index),
    '--ring': String(cell.ring)
  }
}))

export default {
  data() {
    return {
      visible: false,
      message: '加载中...',
      activeVariant: LOADING_VARIANTS[0],
      logoCells: LOGO_CELLS
    }
  },

  computed: {
    activeVariantClass() {
      return `glx-loading-logo--${this.activeVariant}`
    }
  },

  mounted() {
    bindLoadingInstance(this)
  },

  beforeDestroy() {
    unbindLoadingInstance(this)
  },

  methods: {
    show(message = '加载中...', options = {}) {
      this.message = message
      this.activeVariant = this.resolveVariant(message, options)
      this.visible = true
    },

    hide() {
      this.visible = false
    },

    resolveVariant(message, options) {
      if (options && typeof options.variant === 'string' && LOADING_VARIANTS.includes(options.variant)) {
        return options.variant
      }

      if (this.isTransportMessage(message)) {
        return TRANSPORT_VARIANT
      }

      return this.pickRandomVariant()
    },

    isTransportMessage(message = '') {
      return TRANSPORT_MESSAGE_KEYWORDS.some((keyword) => message.includes(keyword))
    },

    pickRandomVariant() {
      const randomIndex = Math.floor(Math.random() * LOADING_VARIANTS.length)
      const randomVariant = LOADING_VARIANTS[randomIndex]
      if (LOADING_VARIANTS.length > 1 && randomVariant === this.activeVariant) {
        const nextIndex = (randomIndex + 1) % LOADING_VARIANTS.length
        return LOADING_VARIANTS[nextIndex]
      }

      return randomVariant
    }
  }
}
</script>

<style scoped>
.glx-loading-overlay {
  position: fixed;
  inset: 0;
  z-index: 10001;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  box-sizing: border-box;
  background: rgba(255, 255, 255, 0.72);
}

.glx-loading-card {
  width: 100%;
  max-width: 420rpx;
  padding: 36rpx 32rpx;
  border: 4rpx solid #000000;
  box-shadow: 10rpx 10rpx 0 #000000;
  background: #ffffff;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 22rpx;
  box-sizing: border-box;
}

.glx-loading-logo-stage {
  width: 220rpx;
  height: 220rpx;
  border: 4rpx solid #000000;
  border-radius: 14rpx;
  background: linear-gradient(160deg, #141b29 0%, #090d15 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  position: relative;
}

.glx-loading-logo {
  width: 128rpx;
  height: 128rpx;
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10rpx;
  transform-style: preserve-3d;
}

.glx-loading-cell {
  border: 3rpx solid #000000;
  border-radius: 10rpx;
  background: var(--tile-color);
  box-shadow: inset 0 0 0 1rpx rgba(255, 255, 255, 0.15);
  will-change: transform, opacity, filter;
}

.glx-loading-logo--chase .glx-loading-cell {
  opacity: 0.36;
  filter: brightness(0.32) saturate(0.45);
  animation: glxLoadingChaseRing 1.55s steps(1, end) infinite;
  animation-delay: calc(var(--ring) * 0.12s);
}

.glx-loading-logo--chase .glx-loading-cell--center {
  animation: glxLoadingChaseCenter 1.55s ease-in-out infinite;
  opacity: 1;
  filter: brightness(0.9);
}

.glx-loading-logo--flip {
  perspective: 820rpx;
}

.glx-loading-logo--flip .glx-loading-cell {
  transform-origin: center;
  backface-visibility: hidden;
  animation: glxLoadingFlip 2.1s ease-in-out infinite;
  animation-delay: calc(var(--i) * 0.08s);
}

.glx-loading-logo--stamp .glx-loading-cell {
  animation: glxLoadingStamp 2.2s cubic-bezier(0.2, 0.78, 0.26, 1) infinite;
  animation-delay: calc(var(--ring) * 0.14s);
}

.glx-loading-text {
  font-size: 28rpx;
  font-weight: 900;
  line-height: 1.4;
  color: #000000;
  text-align: center;
}

@keyframes glxLoadingChaseRing {
  0%, 22% {
    opacity: 0.35;
    filter: brightness(0.32) saturate(0.45);
  }

  23%, 58% {
    opacity: 1;
    filter: brightness(1.18) saturate(1.28);
  }

  59%, 100% {
    opacity: 0.35;
    filter: brightness(0.32) saturate(0.45);
  }
}

@keyframes glxLoadingChaseCenter {
  0%, 100% {
    transform: scale(0.9);
    filter: brightness(0.9);
  }

  45% {
    transform: scale(1.16);
    filter: brightness(1.28) saturate(1.4);
  }
}

@keyframes glxLoadingFlip {
  0%, 15% {
    transform: rotateX(0deg) rotateY(0deg);
    filter: none;
  }

  35% {
    transform: rotateX(180deg);
    filter: brightness(1.18) saturate(1.28);
  }

  58% {
    transform: rotateY(180deg);
    filter: brightness(0.76) saturate(0.66);
  }

  100% {
    transform: rotateX(360deg) rotateY(360deg);
    filter: none;
  }
}

@keyframes glxLoadingStamp {
  0%, 18% {
    transform: translateY(-16rpx) scale(0.66);
    opacity: 0.34;
    filter: brightness(0.58) saturate(0.8);
  }

  40% {
    transform: translateY(4rpx) scale(1.1);
    opacity: 1;
    filter: brightness(1.24) saturate(1.3);
  }

  62% {
    transform: translateY(0) scale(0.96);
    opacity: 0.9;
    filter: brightness(0.96) saturate(1);
  }

  100% {
    transform: translateY(0) scale(1);
    opacity: 1;
    filter: none;
  }
}
</style>
