<template>
  <view class="pet-source-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">鸡狗素材预览</text>
      <view class="nav-right"></view>
    </view>

    <view class="canvas-section">
      <view class="preview-canvas-container">
        <PixelCanvas
          v-if="previewCanvasReady"
          :width="64"
          :height="64"
          :pixels="previewPixels"
          :zoom="previewZoom"
          :offset-x="previewOffset.x"
          :offset-y="previewOffset.y"
          :canvas-width="previewContainerSize.width"
          :canvas-height="previewContainerSize.height"
          :grid-visible="true"
          :is-dark-mode="true"
          :touch-enabled="false"
          canvas-id="petSourcePreviewCanvas"
        />
      </view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-title">{{ currentFrameLabel }}</text>
          <text class="preview-subtitle">仅展示 Husky / Chick 源帧，不影响原桌宠页面</text>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import {
  PET_CHICK_ALL,
  PET_FRAME_COLORS,
  PET_FRAMES,
  PET_HUSKY_REST,
  PET_HUSKY_WALK,
  PET_SOURCE_INFO,
  PET_SPECIES_BASE_OFFSETS,
  PET_SPECIES_SEQUENCE,
} from "../spirit-screen/petFrames.js";

const PET_SPECIES_SWITCH_MS = 9000;
const PET_HUSKY_WALK_FRAME_MS = 140;
const PET_HUSKY_REST_FRAME_MS = 220;
const PET_CHICK_FRAME_MS = 130;
const PET_HUSKY_REST_HOLD_MS = 1600;
const PREVIEW_LOOP_MS = 66;

function fillRect(pixelMap, x0, y0, x1, y1, color) {
  const left = x0 < x1 ? x0 : x1;
  const right = x0 > x1 ? x0 : x1;
  const top = y0 < y1 ? y0 : y1;
  const bottom = y0 > y1 ? y0 : y1;
  for (let y = top; y <= bottom; y++) {
    if (y < 0 || y > 63) {
      continue;
    }
    for (let x = left; x <= right; x++) {
      if (x < 0 || x > 63) {
        continue;
      }
      pixelMap.set(`${x},${y}`, color);
    }
  }
}

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    PixelCanvas,
  },
  data() {
    return {
      previewCanvasReady: false,
      previewPixels: new Map(),
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: {
        width: 320,
        height: 320,
      },
      currentFrameLabel: "加载中...",
      previewTimer: null,
    };
  },
  onLoad() {
    this.previewCanvasReady = true;
    this.$nextTick(() => {
      this.startPreviewLoop();
    });
  },
  onShow() {
    this.startPreviewLoop();
  },
  onHide() {
    this.stopPreviewLoop();
  },
  onUnload() {
    this.stopPreviewLoop();
  },
  methods: {
    handleBack() {
      uni.navigateBack();
    },

    startPreviewLoop() {
      if (this.previewTimer !== null) {
        return;
      }
      this.renderPreviewFrame();
      this.previewTimer = setInterval(() => {
        this.renderPreviewFrame();
      }, PREVIEW_LOOP_MS);
    },

    stopPreviewLoop() {
      if (this.previewTimer === null) {
        return;
      }
      clearInterval(this.previewTimer);
      this.previewTimer = null;
    },

    drawPetSpriteRows(pixelMap, rows, shiftX, shiftY) {
      for (const row of rows) {
        const y = row[0] + shiftY;
        if (y < 0 || y > 63) {
          continue;
        }
        const x0 = row[1] + shiftX;
        const x1 = x0 + row[2] - 1;
        const color = PET_FRAME_COLORS[row[3]];
        if (!color) {
          continue;
        }
        fillRect(pixelMap, x0, y, x1, y, color);
      }
    },

    resolvePetFrame(now) {
      const speciesIndex =
        Math.floor(now / PET_SPECIES_SWITCH_MS) % PET_SPECIES_SEQUENCE.length;
      const species = PET_SPECIES_SEQUENCE[speciesIndex];

      if (species === "husky") {
        const phaseInSpecies = now % PET_SPECIES_SWITCH_MS;
        const useRest = phaseInSpecies >= PET_SPECIES_SWITCH_MS - PET_HUSKY_REST_HOLD_MS;
        const frameNames = useRest ? PET_HUSKY_REST : PET_HUSKY_WALK;
        const frameStep = useRest ? PET_HUSKY_REST_FRAME_MS : PET_HUSKY_WALK_FRAME_MS;
        const frameIndex = Math.floor(now / frameStep) % frameNames.length;
        return {
          speciesLabel: useRest ? "Husky 休息段" : "Husky 动作段",
          frameName: frameNames[frameIndex],
          offset: PET_SPECIES_BASE_OFFSETS.husky,
        };
      }

      const chickIndex = Math.floor(now / PET_CHICK_FRAME_MS) % PET_CHICK_ALL.length;
      return {
        speciesLabel: "Chick 全序列",
        frameName: PET_CHICK_ALL[chickIndex],
        offset: PET_SPECIES_BASE_OFFSETS.chick,
      };
    },

    renderPreviewFrame() {
      const now = Date.now();
      const petFrame = this.resolvePetFrame(now);
      const rows = PET_FRAMES[petFrame.frameName];
      const pixelMap = new Map();
      this.drawPetSpriteRows(pixelMap, rows, petFrame.offset.x, petFrame.offset.y);
      this.previewPixels = pixelMap;
      this.currentFrameLabel = `${petFrame.speciesLabel} · ${petFrame.frameName} · Husky:${PET_SOURCE_INFO.husky.frameCount}帧 Chick:${PET_SOURCE_INFO.chick.frameCount}帧`;
    },
  },
};
</script>

<style scoped>
.pet-source-page {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--bg-secondary);
}

.status-bar {
  background-color: #1a1a1a;
}

.canvas-section {
  padding: 20rpx;
}

.preview-canvas-container {
  background: #000000;
  border-radius: 20rpx;
  padding: 20rpx;
  min-height: 360rpx;
  box-sizing: border-box;
}

.preview-caption {
  margin-top: 16rpx;
}

.preview-title {
  font-size: 24rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.preview-subtitle {
  margin-top: 8rpx;
  font-size: 22rpx;
  color: var(--text-secondary);
}
</style>
