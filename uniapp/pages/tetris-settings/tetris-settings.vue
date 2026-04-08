<template>
  <view class="tetris-page">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="header">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <view class="nav-title">
        <text class="project-name">俄罗斯方块时钟</text>
      </view>
    </view>

    <view class="canvas-section">
      <view class="preview-canvas-container">
        <PixelCanvas
          v-if="previewCanvasReady"
          :width="64"
          :height="64"
          :pixels="currentPreviewPixels"
          :zoom="previewZoom"
          :offset-x="previewOffset.x"
          :offset-y="previewOffset.y"
          :canvas-width="previewContainerSize.width"
          :canvas-height="previewContainerSize.height"
          :grid-visible="true"
          :is-dark-mode="true"
          :touch-enabled="false"
          canvas-id="tetrisPreviewCanvas"
        />
      </view>
      <view class="preview-caption">
        <text class="preview-title">预览效果</text>
        <view
          class="action-btn-sm primary"
          :class="{ disabled: isSending }"
          @click="saveAndApply"
        >
          <Icon name="link" :size="32" color="#ffffff" />
          <text>{{ isSending ? "发送中" : "发送" }}</text>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
        <view v-show="currentTab === 0" class="tab-panel">
          <view class="card">
            <view class="card-title-section">
              <text class="card-title">效果风格</text>
              <text class="card-subtitle">{{ currentSceneLabel }}</text>
            </view>
            <view class="scene-grid">
              <view
                v-for="preset in scenePresets"
                :key="preset.key"
                class="scene-card"
                :class="{ active: config.sceneKey === preset.key }"
                @click="applyScenePreset(preset)"
              >
                <text class="scene-card-title">{{ preset.label }}</text>
                <text class="scene-card-desc">{{ preset.desc }}</text>
              </view>
            </view>
          </view>
        </view>

        <view v-show="currentTab === 1" class="tab-panel">
          <view class="card">
            <view class="card-title-section">
              <text class="card-title">画面内容</text>
            </view>
            <view class="option-row">
              <view
                class="option-btn"
                :class="{ active: config.showClock }"
                @click="setDisplayMode(true)"
              >
                <text>拼时间</text>
              </view>
              <view
                class="option-btn"
                :class="{ active: !config.showClock }"
                @click="setDisplayMode(false)"
              >
                <text>只下落</text>
              </view>
            </view>
          </view>

          <view class="card">
            <view class="card-title-section">
              <text class="card-title">下落速度</text>
            </view>
            <view class="option-row">
              <view
                v-for="item in dropSpeedOptions"
                :key="item.value"
                class="option-btn"
                :class="{ active: config.speed === item.value }"
                @click="setSpeed(item.value)"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view class="card">
            <view class="card-title-section">
              <text class="card-title">格子大小</text>
            </view>
            <view class="option-row">
              <view
                v-for="item in sizeOptions"
                :key="item.value"
                class="option-btn"
                :class="{ active: config.cellSize === item.value }"
                @click="setCellSize(item.value)"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view v-if="!config.showClock" class="card">
            <view class="card-title-section">
              <text class="card-title">堆叠方式</text>
            </view>
            <view class="option-row">
              <view
                class="option-btn"
                :class="{ active: config.clearMode }"
                @click="setClearMode(true)"
              >
                <text>自动消行</text>
              </view>
              <view
                class="option-btn"
                :class="{ active: !config.clearMode }"
                @click="setClearMode(false)"
              >
                <text>堆满重来</text>
              </view>
            </view>
          </view>

          <view class="summary-card">
            <text class="summary-text">{{ motionSummary }}</text>
          </view>
        </view>

        <view v-show="currentTab === 2" class="tab-panel">
          <view class="card">
            <view class="card-title-section">
              <text class="card-title">常用组合</text>
              <text class="card-subtitle">{{ currentPieceGroupLabel }}</text>
            </view>
            <view class="scene-grid">
              <view
                v-for="group in pieceGroups"
                :key="group.key"
                class="scene-card group-card"
                :class="{ active: config.pieceGroupKey === group.key }"
                @click="applyPieceGroup(group)"
              >
                <text class="scene-card-title">{{ group.label }}</text>
                <text class="scene-card-desc">{{ group.desc }}</text>
              </view>
            </view>
          </view>

          <view class="card">
            <view class="card-title-section">
              <text class="card-title">单独开关</text>
              <text class="card-subtitle">至少保留一种</text>
            </view>
            <view class="piece-grid">
              <view
                v-for="(piece, idx) in pieceTypes"
                :key="idx"
                class="piece-item"
                :class="{ active: config.pieces.includes(idx) }"
                @click="togglePiece(idx)"
              >
                <view class="piece-preview-grid">
                  <view
                    v-for="(block, blockIndex) in piece.blocks"
                    :key="blockIndex"
                    class="piece-block"
                    :style="getPieceBlockStyle(block, piece.color)"
                  ></view>
                </view>
                <text class="piece-name">{{ piece.name }}</text>
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <view class="bottom-tabs">
      <view
        v-for="(tab, index) in tabs"
        :key="tab"
        class="bottom-tab-item"
        :class="{ active: currentTab === index }"
        @click="currentTab = index"
      >
        <Icon
          :name="tabIconNames[index]"
          :size="36"
          :color="currentTab === index ? '#4F7FFF' : 'var(--text-secondary)'"
        />
        <text class="bottom-tab-text">{{ tab }}</text>
      </view>
    </view>

    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import { buildTetrisPreviewFrames } from "../../utils/tetrisClockPreview.js";

const DEFAULT_PIECES = [0, 1, 2, 3, 4, 5, 6];

const SCENE_PRESETS = [
  {
    key: "clock_classic",
    label: "标准拼时",
    desc: "完整轮廓慢慢拼合，适合做主展示。",
    clearMode: true,
    cellSize: 2,
    speed: "normal",
    showClock: true,
    pieces: [0, 1, 2, 3, 4, 5, 6],
  },
  {
    key: "clock_compact",
    label: "快速拼时",
    desc: "格子更细，拼得更快，数字轮廓更完整。",
    clearMode: true,
    cellSize: 1,
    speed: "fast",
    showClock: true,
    pieces: [0, 1, 2, 5, 6],
  },
  {
    key: "clock_bold",
    label: "大块拼时",
    desc: "块更大，远看更醒目，适合桌面远距显示。",
    clearMode: true,
    cellSize: 3,
    speed: "slow",
    showClock: true,
    pieces: [0, 1, 2, 3, 4, 5, 6],
  },
  {
    key: "clock_clean",
    label: "规整拼时",
    desc: "只保留更利落的块型，画面更干净。",
    clearMode: true,
    cellSize: 2,
    speed: "slow",
    showClock: true,
    pieces: [0, 1, 5, 6],
  },
  {
    key: "free_fall",
    label: "自由掉落",
    desc: "不拼时间，只保留俄罗斯方块持续下落。",
    clearMode: true,
    cellSize: 2,
    speed: "normal",
    showClock: false,
    pieces: [0, 1, 2, 3, 4, 5, 6],
  },
];

const PIECE_GROUPS = [
  {
    key: "full_set",
    label: "全部方块",
    desc: "完整七种，更接近经典俄罗斯方块。",
    pieces: [0, 1, 2, 3, 4, 5, 6],
  },
  {
    key: "clean_set",
    label: "规整组合",
    desc: "形状更利落，拼出来的数字更干净。",
    pieces: [0, 1, 5, 6],
  },
  {
    key: "motion_set",
    label: "变化组合",
    desc: "拐角和折线更多，重组过程更活。",
    pieces: [0, 2, 3, 4],
  },
  {
    key: "corner_set",
    label: "直角组合",
    desc: "更偏边角块，轮廓变化更明显。",
    pieces: [1, 5, 6],
  },
];

function clonePieces(source) {
  return source.slice();
}

function createDefaultConfig() {
  return {
    sceneKey: "clock_classic",
    pieceGroupKey: "full_set",
    clearMode: true,
    cellSize: 2,
    speed: "normal",
    showClock: true,
    pieces: clonePieces(DEFAULT_PIECES),
  };
}

function samePieces(left, right) {
  if (!Array.isArray(left) || !Array.isArray(right)) {
    return false;
  }
  if (left.length !== right.length) {
    return false;
  }
  for (let index = 0; index < left.length; index += 1) {
    if (left[index] !== right[index]) {
      return false;
    }
  }
  return true;
}

function findSceneKey(config) {
  for (let index = 0; index < SCENE_PRESETS.length; index += 1) {
    const preset = SCENE_PRESETS[index];
    if (
      preset.clearMode === config.clearMode &&
      preset.cellSize === config.cellSize &&
      preset.speed === config.speed &&
      preset.showClock === config.showClock &&
      samePieces(preset.pieces, config.pieces)
    ) {
      return preset.key;
    }
  }
  return "";
}

function findPieceGroupKey(pieces) {
  for (let index = 0; index < PIECE_GROUPS.length; index += 1) {
    const group = PIECE_GROUPS[index];
    if (samePieces(group.pieces, pieces)) {
      return group.key;
    }
  }
  return "";
}

function normalizeSavedConfig(saved) {
  const config = createDefaultConfig();
  if (!saved || typeof saved !== "object") {
    return config;
  }

  if (typeof saved.clearMode === "boolean") {
    config.clearMode = saved.clearMode;
  }
  if (saved.cellSize === 1 || saved.cellSize === 2 || saved.cellSize === 3) {
    config.cellSize = saved.cellSize;
  }
  if (
    saved.speed === "slow" ||
    saved.speed === "normal" ||
    saved.speed === "fast"
  ) {
    config.speed = saved.speed;
  }
  if (typeof saved.showClock === "boolean") {
    config.showClock = saved.showClock;
  }
  if (Array.isArray(saved.pieces) && saved.pieces.length > 0) {
    const nextPieces = saved.pieces
      .filter((item) => Number.isInteger(item) && item >= 0 && item <= 6)
      .sort((left, right) => left - right);
    if (nextPieces.length > 0) {
      config.pieces = nextPieces;
    }
  }

  config.sceneKey = findSceneKey(config);
  config.pieceGroupKey = findPieceGroupKey(config.pieces);
  return config;
}

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    PixelCanvas,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isSending: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx - 112rpx)",
      previewCanvasReady: false,
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: { width: 320, height: 320 },
      previewFrameMaps: [],
      previewFrameIndex: 0,
      previewTimer: null,
      currentTab: 0,
      tabs: ["场景", "动画", "方块"],
      tabIconNames: ["browse", "time", "setting"],
      scenePresets: SCENE_PRESETS,
      pieceGroups: PIECE_GROUPS,
      sizeOptions: [
        { label: "细", value: 1 },
        { label: "中", value: 2 },
        { label: "粗", value: 3 },
      ],
      dropSpeedOptions: [
        { label: "慢", value: "slow" },
        { label: "中", value: "normal" },
        { label: "快", value: "fast" },
      ],
      pieceTypes: [
        {
          name: "I",
          color: "#00F0F0",
          blocks: [
            [0, 1],
            [1, 1],
            [2, 1],
            [3, 1],
          ],
        },
        {
          name: "O",
          color: "#F0F000",
          blocks: [
            [1, 0],
            [2, 0],
            [1, 1],
            [2, 1],
          ],
        },
        {
          name: "T",
          color: "#A000F0",
          blocks: [
            [1, 0],
            [0, 1],
            [1, 1],
            [2, 1],
          ],
        },
        {
          name: "S",
          color: "#00F000",
          blocks: [
            [1, 0],
            [2, 0],
            [0, 1],
            [1, 1],
          ],
        },
        {
          name: "Z",
          color: "#F00000",
          blocks: [
            [0, 0],
            [1, 0],
            [1, 1],
            [2, 1],
          ],
        },
        {
          name: "J",
          color: "#0000F0",
          blocks: [
            [0, 0],
            [0, 1],
            [1, 1],
            [2, 1],
          ],
        },
        {
          name: "L",
          color: "#F0A000",
          blocks: [
            [2, 0],
            [0, 1],
            [1, 1],
            [2, 1],
          ],
        },
      ],
      config: createDefaultConfig(),
    };
  },
  computed: {
    currentPreviewPixels() {
      if (this.previewFrameMaps.length === 0) {
        return new Map();
      }
      if (this.previewFrameIndex >= this.previewFrameMaps.length) {
        return this.previewFrameMaps[0];
      }
      return this.previewFrameMaps[this.previewFrameIndex];
    },
    previewInterval() {
      if (this.config.speed === "slow") {
        return 150;
      }
      if (this.config.speed === "fast") {
        return 86;
      }
      return 116;
    },
    currentScene() {
      const current = this.scenePresets.find((item) => item.key === this.config.sceneKey);
      if (current) {
        return current;
      }
      return null;
    },
    currentSceneLabel() {
      if (this.currentScene) {
        return this.currentScene.label;
      }
      return "自定义";
    },
    currentPieceGroupLabel() {
      const current = this.pieceGroups.find(
        (item) => item.key === this.config.pieceGroupKey,
      );
      if (current) {
        return current.label;
      }
      return "自定义";
    },
    motionSummary() {
      const modeText = this.config.showClock ? "当前会持续拼出时间" : "当前只展示方块下落";
      const sizeText =
        this.config.cellSize === 1
          ? "格子更细"
          : this.config.cellSize === 3
            ? "格子更粗"
            : "格子大小适中";
      const speedText =
        this.config.speed === "slow"
          ? "节奏更从容"
          : this.config.speed === "fast"
            ? "节奏更利落"
            : "节奏比较均衡";
      if (!this.config.showClock) {
        const stackText = this.config.clearMode ? "并且会自动消行" : "堆满后重新开始";
        return `${modeText}，${sizeText}，${speedText}，${stackText}。`;
      }
      return `${modeText}，${sizeText}，${speedText}。`;
    },
  },
  watch: {
    config: {
      deep: true,
      handler() {
        this.refreshPreview();
      },
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    const saved = uni.getStorageSync("tetris_config");
    this.config = normalizeSavedConfig(saved);
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    this.initPreviewCanvas();
  },
  onUnload() {
    this.stopPreview();
  },
  methods: {
    handleBack() {
      uni.navigateBack();
    },
    initPreviewCanvas() {
      const systemInfo = uni.getSystemInfoSync();
      const statusBarHeight = systemInfo.statusBarHeight || 0;
      const query = uni.createSelectorQuery().in(this);
      query
        .select(".preview-canvas-container")
        .boundingClientRect((rect) => {
          if (rect) {
            this.previewContainerSize = {
              width: rect.width,
              height: rect.height,
            };
            const nextHeight =
              systemInfo.windowHeight - statusBarHeight - 88 - rect.height - 112;
            this.contentHeight = `${Math.max(120, nextHeight)}px`;
          }
          this.previewCanvasReady = true;
          this.refreshPreview();
        })
        .exec();
    },
    refreshPreview() {
      if (!this.previewCanvasReady) {
        return;
      }
      this.previewFrameMaps = buildTetrisPreviewFrames(this.config);
      this.previewFrameIndex = 0;
      this.startPreview();
    },
    startPreview() {
      this.stopPreview();
      if (this.previewFrameMaps.length <= 1) {
        return;
      }
      this.previewTimer = setInterval(() => {
        this.previewFrameIndex =
          (this.previewFrameIndex + 1) % this.previewFrameMaps.length;
      }, this.previewInterval);
    },
    stopPreview() {
      if (this.previewTimer) {
        clearInterval(this.previewTimer);
        this.previewTimer = null;
      }
    },
    syncDerivedKeys() {
      this.config.sceneKey = findSceneKey(this.config);
      this.config.pieceGroupKey = findPieceGroupKey(this.config.pieces);
    },
    applyScenePreset(preset) {
      this.config.sceneKey = preset.key;
      this.config.clearMode = preset.clearMode;
      this.config.cellSize = preset.cellSize;
      this.config.speed = preset.speed;
      this.config.showClock = preset.showClock;
      this.config.pieces = clonePieces(preset.pieces);
      this.config.pieceGroupKey = findPieceGroupKey(this.config.pieces);
    },
    applyPieceGroup(group) {
      this.config.pieces = clonePieces(group.pieces);
      this.syncDerivedKeys();
    },
    setDisplayMode(showClock) {
      this.config.showClock = showClock;
      this.syncDerivedKeys();
    },
    setSpeed(value) {
      this.config.speed = value;
      this.syncDerivedKeys();
    },
    setCellSize(value) {
      this.config.cellSize = value;
      this.syncDerivedKeys();
    },
    setClearMode(value) {
      this.config.clearMode = value;
      this.syncDerivedKeys();
    },
    togglePiece(idx) {
      const currentIndex = this.config.pieces.indexOf(idx);
      if (currentIndex >= 0) {
        if (this.config.pieces.length <= 1) {
          this.toast.showError("至少保留一种方块");
          return;
        }
        this.config.pieces.splice(currentIndex, 1);
      } else {
        this.config.pieces.push(idx);
        this.config.pieces.sort((left, right) => left - right);
      }
      this.syncDerivedKeys();
    },
    getPieceBlockStyle(block, color) {
      return {
        left: `${block[0] * 15 + 6}rpx`,
        top: `${block[1] * 15 + 6}rpx`,
        backgroundColor: color,
      };
    },
    async saveAndApply() {
      if (this.isSending) {
        return;
      }

      uni.setStorageSync("tetris_config", this.config);

      if (!this.deviceStore.connected) {
        this.toast.showInfo("请先连接设备");
        return;
      }

      this.isSending = true;
      try {
        const ws = this.deviceStore.getWebSocket();
        const speedMap = {
          slow: 300,
          normal: 150,
          fast: 80,
        };
        const speed = speedMap[this.config.speed];
        if (speed === undefined) {
          this.toast.showError("速度参数无效");
          return;
        }

        await ws.send({
          cmd: "set_mode",
          mode: "tetris",
          clearMode: this.config.clearMode,
          cellSize: this.config.cellSize,
          speed,
          showClock: this.config.showClock,
          pieces: this.config.pieces,
        });

        this.toast.showSuccess("俄罗斯方块时钟已发送到设备");
        const pages = getCurrentPages();
        if (pages.length >= 2) {
          const prev = pages[pages.length - 2];
          if (prev && prev.deviceMode !== undefined) {
            prev.deviceMode = "tetris";
          }
        }
        this.deviceStore.setDeviceMode("tetris", { businessMode: true });
      } catch (error) {
        console.error("发送俄罗斯方块时钟失败:", error);
        this.toast.showError("发送失败");
      } finally {
        this.isSending = false;
      }
    },
  },
};
</script>

<style scoped>
.tetris-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

.status-bar {
  background-color: #1a1a1a;
}

.header {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--bg-elevated);
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
  flex-shrink: 0;
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.project-name {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--text-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.canvas-section {
  padding: 24rpx 24rpx 16rpx;
  background: linear-gradient(180deg, rgba(17, 24, 39, 0.98), rgba(10, 14, 24, 0.98));
  border-bottom: 2rpx solid rgba(148, 163, 184, 0.14);
  flex-shrink: 0;
}

.preview-canvas-container {
  width: 100%;
  height: 420rpx;
  border-radius: 28rpx;
  overflow: hidden;
  background: radial-gradient(circle at 50% 30%, rgba(59, 130, 246, 0.16), rgba(10, 16, 26, 0.98));
  border: 2rpx solid rgba(148, 163, 184, 0.18);
}

.preview-caption {
  margin-top: 18rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.preview-title {
  font-size: 28rpx;
  font-weight: 600;
  color: #f8fafc;
}

.action-btn-sm {
  min-width: 118rpx;
  height: 64rpx;
  padding: 0 18rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10rpx;
  border-radius: 18rpx;
  border: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
}

.action-btn-sm.primary {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.action-btn-sm.disabled {
  opacity: 0.6;
}

.action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: #ffffff;
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
}

.content-wrapper {
  padding: 24rpx;
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.tab-panel {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.card {
  background: var(--bg-elevated);
  border: 2rpx solid var(--border-primary);
  border-radius: 28rpx;
  padding: 24rpx;
}

.card-title-section {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  margin-bottom: 20rpx;
}

.card-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.card-subtitle {
  font-size: 22rpx;
  color: var(--color-text-secondary);
}

.scene-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
}

.scene-card {
  width: calc(50% - 8rpx);
  min-height: 164rpx;
  padding: 18rpx;
  border-radius: 22rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 14rpx;
  box-sizing: border-box;
}

.scene-card.active {
  background: rgba(79, 127, 255, 0.12);
  border-color: var(--accent-primary);
  box-shadow: 0 10rpx 24rpx rgba(79, 127, 255, 0.12);
}

.scene-card-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.scene-card-desc {
  font-size: 22rpx;
  line-height: 1.6;
  color: var(--text-secondary);
}

.scene-card.active .scene-card-title,
.scene-card.active .scene-card-desc {
  color: var(--accent-primary);
}

.group-card {
  min-height: 148rpx;
}

.option-row {
  display: flex;
  gap: 16rpx;
}

.option-btn {
  flex: 1;
  min-height: 76rpx;
  padding: 0 12rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 18rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  box-sizing: border-box;
}

.option-btn text {
  font-size: 26rpx;
  color: var(--text-secondary);
}

.option-btn.active {
  background: rgba(79, 127, 255, 0.12);
  border-color: var(--accent-primary);
}

.option-btn.active text {
  color: var(--accent-primary);
  font-weight: 600;
}

.summary-card {
  padding: 24rpx 28rpx;
  border-radius: 24rpx;
  background: rgba(79, 127, 255, 0.08);
  border: 2rpx solid rgba(79, 127, 255, 0.16);
}

.summary-text {
  font-size: 24rpx;
  line-height: 1.7;
  color: var(--text-secondary);
}

.piece-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
}

.piece-item {
  width: calc(33.333% - 11rpx);
  padding: 18rpx 0 14rpx;
  border-radius: 20rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  box-sizing: border-box;
}

.piece-item.active {
  background: rgba(79, 127, 255, 0.12);
  border-color: var(--accent-primary);
  box-shadow: 0 10rpx 24rpx rgba(79, 127, 255, 0.12);
}

.piece-preview-grid {
  width: 72rpx;
  height: 72rpx;
  position: relative;
}

.piece-block {
  position: absolute;
  width: 12rpx;
  height: 12rpx;
  border-radius: 4rpx;
  box-shadow: inset 0 2rpx 0 rgba(255, 255, 255, 0.28);
}

.piece-name {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-secondary);
}

.piece-item.active .piece-name {
  color: var(--accent-primary);
}

.bottom-tabs {
  display: flex;
  flex-shrink: 0;
  padding: 12rpx 16rpx;
  padding-bottom: calc(12rpx + env(safe-area-inset-bottom));
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--border-primary);
  gap: 8rpx;
}

.bottom-tab-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4rpx;
}

.bottom-tab-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.bottom-tab-item.active .bottom-tab-text {
  color: var(--accent-primary);
  font-weight: 500;
}
</style>
