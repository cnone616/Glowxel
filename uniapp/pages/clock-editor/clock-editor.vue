<template>
  <view class="clock-editor-page">
    <!-- 状态栏占位 -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>

    <!-- 头部 -->
    <view class="header">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <view class="nav-title">
        <text class="project-name">{{ pageHeaderTitle }}</text>
      </view>
    </view>

    <!-- Canvas 预览区域 -->
    <view class="canvas-section">
      <view
        class="canvas-container"
        v-show="!canvasHidden"
        ref="canvasContainer"
      >
        <PixelCanvas
          v-if="canvasReady"
          :width="64"
          :height="64"
          :pixels="allPixelsForPreview"
          :zoom="zoom"
          :offset-x="pan.x"
          :offset-y="pan.y"
          :canvas-width="containerSize.width"
          :canvas-height="containerSize.height"
          :grid-visible="true"
          :is-dark-mode="true"
          :touch-enabled="false"
          canvas-id="clockCanvas"
        />
      </view>
      <view v-if="canvasHidden" class="canvas-placeholder"></view>
      <view class="preview-caption">
        <view class="preview-caption-info">
          <text class="preview-caption-title">64 x 64 模拟预览</text>
          <text class="preview-caption-text">直接查看当前时钟与背景效果</text>
        </view>
        <view class="preview-actions">
          <view class="action-btn-sm" @click="saveConfig">
            <Icon name="save" :size="36" color="var(--color-text-primary)" />
            <text>保存</text>
          </view>
          <view
            class="action-btn-sm primary"
            :class="{ disabled: isSending }"
            @click="sendToDevice"
          >
            <Icon name="link" :size="36" color="#fff" />
            <text>发送</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 主内容：当前 Tab 的表单 -->
    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
        <!-- 时间设置 -->
        <ClockTextSettingsCard
          v-show="currentTab === 0"
          icon-name="time"
          title="时间显示"
          :section="config.time"
          :preset-colors="presetColors"
          :show-font-size="true"
          :min-font-size="1"
          :max-font-size="3"
          @toggle="toggleTimeShow"
          @adjust="handleTimeAdjust"
          @update-color="handleTimeColor"
          @set-align="handleTimeAlign"
        />

        <!-- 字体设置 -->
        <ClockFontPanel
          v-show="currentTab === 1"
          :font-options="fontOptions"
          :selected-font="config.font"
          :show-seconds="config.showSeconds"
          :hour-format="config.hourFormat"
          @select-font="selectFont"
          @toggle-seconds="toggleShowSeconds"
          @set-hour-format="setHourFormat"
        />

        <!-- 日期设置 -->
        <ClockTextSettingsCard
          v-show="currentTab === 3"
          icon-name="calendar"
          title="日期显示"
          :section="config.date"
          :preset-colors="presetColors"
          :show-font-size="true"
          :min-font-size="1"
          :max-font-size="2"
          @toggle="toggleDateShow"
          @adjust="handleDateAdjust"
          @update-color="handleDateColor"
          @set-align="handleDateAlign"
        />

        <!-- 星期设置 -->
        <ClockTextSettingsCard
          v-show="currentTab === 4"
          icon-name="rili5"
          title="星期显示"
          :section="config.week"
          :preset-colors="presetColors"
          :show-font-size="false"
          @toggle="toggleWeekShow"
          @adjust="handleWeekAdjust"
          @update-color="handleWeekColor"
          @set-align="handleWeekAlign"
        />

        <!-- 背景图片 -->
        <view v-show="currentTab === 2" class="settings-card">
          <view class="card-title-section">
            <Icon name="picture" :size="32" />
            <text class="card-title">背景图片</text>
            <view class="toggle-switch" @click="toggleImageShow">
              <view class="switch-track" :class="{ active: config.image.show }">
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>

          <view class="image-upload">
            <view
              v-if="!config.image.data"
              class="upload-placeholder"
              @click="chooseImage"
            >
              <Icon name="add" :size="64" />
              <text class="upload-text">{{
                clockMode === "animation"
                  ? "点击上传图片/GIF"
                  : "点击上传背景图片"
              }}</text>
              <text class="upload-hint">{{
                clockMode === "animation"
                  ? "支持静态图片和 GIF 动画，建议 64x64 以内"
                  : "支持静态图片，建议 64x64 以内"
              }}</text>
            </view>
            <view v-else class="image-preview">
              <image
                :src="config.image.data"
                class="preview-image"
                mode="aspectFit"
              />
              <view v-if="gifAnimationData" class="gif-badge">
                <text class="gif-badge-text"
                  >GIF {{ gifAnimationData.frameCount }}帧</text
                >
              </view>
              <view class="image-actions">
                <view class="image-action-btn" @click="chooseImage">
                  <Icon name="refresh" :size="28" />
                  <text>更换</text>
                </view>
                <view class="image-action-btn danger" @click="removeImage">
                  <Icon name="ashbin" :size="28" />
                  <text>删除</text>
                </view>
              </view>
            </view>
          </view>

          <!-- 图片尺寸和位置控制 -->
          <view v-if="config.image.data" class="setting-group">
            <view class="setting-item">
              <view class="setting-header-row">
                <text class="setting-label">尺寸设置</text>
                <view class="quick-size-btn" @click="setSquareSize">
                  <text class="btn-text">长宽一致</text>
                </view>
              </view>
            </view>

            <view class="setting-item-row">
              <text class="setting-label">宽度</text>
              <view class="setting-control-buttons">
                <view
                  class="control-btn"
                  @click="adjustImageValue('width', -1)"
                >
                  <text class="control-icon">-</text>
                </view>
                <text
                  class="setting-value-large"
                  :class="{ 'info-color': config.image.width > 64 }"
                  >{{ config.image.width }}</text
                >
                <view class="control-btn" @click="adjustImageValue('width', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
            <text v-if="config.image.width > 64" class="info-text"
              ><Icon name="prompt" :size="24" color="var(--accent-color)" />
              宽度超过64，超出部分不会显示</text
            >

            <view class="setting-item-row">
              <text class="setting-label">高度</text>
              <view class="setting-control-buttons">
                <view
                  class="control-btn"
                  @click="adjustImageValue('height', -1)"
                >
                  <text class="control-icon">-</text>
                </view>
                <text
                  class="setting-value-large"
                  :class="{ 'info-color': config.image.height > 64 }"
                  >{{ config.image.height }}</text
                >
                <view
                  class="control-btn"
                  @click="adjustImageValue('height', 1)"
                >
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
            <text v-if="config.image.height > 64" class="info-text"
              ><Icon name="prompt" :size="24" color="var(--accent-color)" />
              高度超过64，超出部分不会显示</text
            >

            <view class="setting-item-row">
              <text class="setting-label">X 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustImageValue('x', -1)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.image.x }}</text>
                <view class="control-btn" @click="adjustImageValue('x', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>

            <!-- GIF 播放控制 -->
            <view
              v-if="gifAnimationData && gifAnimationData.frameCount > 1"
              class="setting-item-row"
            >
              <text class="setting-label">预览</text>
              <view class="setting-control-buttons">
                <view
                  class="control-btn"
                  @click="
                    gifIsPlaying ? pauseGifAnimation() : startGifAnimation()
                  "
                >
                  <text class="control-icon">{{
                    gifIsPlaying ? "⏸" : "▶"
                  }}</text>
                </view>
                <text class="setting-value-large">{{ gifPlaySpeed }}x</text>
                <view
                  class="control-btn"
                  @click="
                    gifPlaySpeed = Math.min(4, +(gifPlaySpeed + 0.5).toFixed(1))
                  "
                >
                  <text class="control-icon">+</text>
                </view>
                <view
                  class="control-btn"
                  @click="
                    gifPlaySpeed = Math.max(
                      0.5,
                      +(gifPlaySpeed - 0.5).toFixed(1),
                    )
                  "
                >
                  <text class="control-icon">-</text>
                </view>
              </view>
            </view>

            <view class="setting-item-row">
              <text class="setting-label">Y 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustImageValue('y', -1)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.image.y }}</text>
                <view class="control-btn" @click="adjustImageValue('y', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <!-- 底部 Tab 切换 -->
    <view class="bottom-tabs">
      <view
        v-for="(tab, index) in tabs"
        :key="index"
        class="bottom-tab-item"
        :class="{ active: currentTab === index }"
        @click="currentTab = index"
      >
        <Icon
          :name="tabIconNames[index]"
          :size="36"
          :color="currentTab === index ? accentColor : 'var(--text-secondary)'"
        />
        <text class="bottom-tab-text">{{ tab }}</text>
      </view>
    </view>

    <!-- 传输遮罩弹窗 -->
    <view v-if="isSending" class="sending-overlay" @touchmove.stop.prevent>
      <view class="sending-modal">
        <view class="sending-spinner"></view>
        <text class="sending-title">正在传输数据...</text>
        <text class="sending-tip">请勿切换网络或关闭程序</text>
      </view>
    </view>

    <!-- 隐藏的 canvas，用于图片处理 -->
    <canvas
      id="imageProcessCanvas"
      type="2d"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 64px;
        height: 64px;
      "
    ></canvas>

    <Toast ref="toastRef" @show="canvasHidden = true" @hide="onToastHide" />
  </view>
</template>

<script>
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import clockPreviewMixin from "./mixins/clockPreviewMixin.js";
import imageGifMixin from "./mixins/imageGifMixin.js";
import deviceSyncMixin from "./mixins/deviceSyncMixin.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import ClockFontPanel from "../../components/clock-editor/ClockFontPanel.vue";
import ClockTextSettingsCard from "../../components/clock-editor/ClockTextSettingsCard.vue";
import { getClockFontOptions } from "../../utils/clockCanvas.js";

export default {
  mixins: [statusBarMixin, clockPreviewMixin, imageGifMixin, deviceSyncMixin],
  components: {
    Icon,
    Toast,
    PixelCanvas,
    ClockFontPanel,
    ClockTextSettingsCard,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isReady: false,
      clockMode: "clock", // clock=静态时钟, animation=动态时钟
      gifAnimationData: null, // GIF 动画数据 { frameCount, frames }
      gifRenderedFrameMaps: null, // GIF 所有帧的像素 Map 数组
      gifFrameIndex: 0, // 当前播放帧索引
      gifTimer: null, // GIF 动画定时器
      gifIsPlaying: false, // 是否正在播放
      gifPlaySpeed: 1.0, // 播放速度倍率
      isSending: false, // 传输锁，防止传输过程中操作
      _gifParser: null, // GIF 解析器实例，改宽高时重新生成数据

      contentHeight: "calc(100vh - 112rpx - 120rpx - 80rpx)",

      // Canvas 相关
      canvasHidden: false,
      canvasNode: null,
      canvasCtx: null,
      imagePixels: null,
      showPreview: true,

      // PixelCanvas 视图控制
      zoom: 4,
      pan: { x: 0, y: 0 },
      containerSize: { width: 320, height: 320 },
      canvasReady: false,

      // loading 动画定时器（实例变量，方便清理）
      loadingTimer: null,
      loadingActive: false,

      fontOptions: getClockFontOptions(),

      currentTab: 0,
      tabs: ["时间", "字体", "图片", "日期", "星期"],
      tabIconNames: ["time", "text", "picture", "calendar", "rili5"],

      config: {
        font: "classic_5x7",
        showSeconds: false,
        hourFormat: 24,
        time: {
          show: true,
          fontSize: 1,
          x: 32,
          y: 20,
          color: "#64c8ff",
          align: "center",
        },
        date: {
          show: true,
          fontSize: 1,
          x: 32,
          y: 29,
          color: "#787878",
          align: "center",
        },
        week: {
          show: true,
          x: 32,
          y: 38,
          color: "#646464",
          align: "center",
        },
        image: {
          show: false,
          x: 6,
          y: 6,
          width: 64,
          height: 64,
          data: null,
        },
      },

      presetColors: [
        { name: "青色", hex: "#64c8ff" },
        { name: "绿色", hex: "#00ff9d" },
        { name: "黄色", hex: "#ffdc00" },
        { name: "橙色", hex: "#ffa500" },
        { name: "红色", hex: "#ff6464" },
        { name: "紫色", hex: "#c864ff" },
        { name: "白色", hex: "#ffffff" },
        { name: "灰色", hex: "#787878" },
        { name: "深灰", hex: "#646464" },
      ],
    };
  },

  watch: {
    canvasHidden(newVal) {
      if (!newVal && this.canvasCtx) {
        this.$nextTick(() => {
          this.drawCanvas();
        });
      }
    },

    clockMode(newMode) {
      if (newMode === "animation" && !this._gifParser) {
        this._restoreGifFromLocal();
      } else if (newMode === "clock") {
        this.stopGifAnimation();
      }
    },
  },

  computed: {
    accentColor() {
      return "#4F7FFF";
    },
    pageHeaderTitle() {
      if (this.clockMode === "animation") {
        return "动态时钟";
      }
      return "静态时钟";
    },

    allPixelsForPreview() {
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

      if (this.showPreview) {
        this.getPreviewPixels().forEach((color, key) => pixels.set(key, color));
      }

      return pixels;
    },
  },

  onUnload() {
    this.stopGifAnimation();
    this.stopLoading();
  },

  onLoad(options) {
    const savedMode = uni.getStorageSync("device_mode");
    const defaultMode = savedMode === "animation" ? "animation" : "clock";
    this.clockMode = options.mode || defaultMode;
    this._clockModeFromOptions = !!options.mode;

    this.loadConfig();

    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();

    const systemInfo = uni.getSystemInfoSync();
    const statusBarHeight = systemInfo.statusBarHeight || 0;
    const headerHeight = 56;
    this.contentHeight = `${systemInfo.windowHeight - statusBarHeight - headerHeight - 360}px`;

    this.$nextTick(() => {
      setTimeout(() => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select(".canvas-container")
          .boundingClientRect((data) => {
            if (data && data.width > 0) {
              this.containerSize = { width: data.width, height: data.width };
              const fitZoom = Math.floor((data.width * 0.9) / 64);
              this.zoom = Math.max(2, fitZoom);
              this.pan = {
                x: (data.width - 64 * this.zoom) / 2,
                y: (data.width - 64 * this.zoom) / 2,
              };
            } else {
              this.zoom = 4;
              this.pan = { x: 16, y: 16 };
            }
            this.canvasReady = true;
            if (this.$refs.toastRef) {
              this.toast.setToastInstance(this.$refs.toastRef);
            }
            this.isReady = true;
          })
          .exec();
      }, 150);
    });
  },

  onShow() {
    // 页面显示时的处理
  },

  methods: {
    onToastHide() {
      if (!this.isSending) {
        this.canvasHidden = false;
      }
    },

    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return result
        ? {
            r: parseInt(result[1], 16),
            g: parseInt(result[2], 16),
            b: parseInt(result[3], 16),
          }
        : { r: 255, g: 255, b: 255 };
    },

    handleBack() {
      uni.navigateBack();
    },
  },
};
</script>

<style scoped>
.clock-editor-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

.header {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  background-color: var(--bg-elevated);
  position: relative;
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

.back-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.back-btn:active {
  background-color: var(--bg-tertiary);
  transform: scale(0.95);
}

.icon {
  font-size: 56rpx;
  color: var(--text-primary);
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.project-name {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--text-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.header-actions-inline {
  display: flex;
  gap: 12rpx;
  margin-right: 16rpx;
}

.action-btn-sm {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 14rpx;
  border: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
  transition: var(--transition-base);
}

.action-btn-sm:active {
  transform: scale(0.95);
}

.action-btn-sm.primary {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.header-actions {
  display: flex;
  gap: 16rpx;
}

.action-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  border: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
  transition: var(--transition-base);
}

.action-btn:active {
  transform: scale(0.95);
  border-color: var(--accent-primary);
  box-shadow: var(--shadow-glow);
}

.action-btn.primary {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.preview-hint {
  padding: 20rpx 32rpx;
  background-color: var(--bg-tertiary);
  border-bottom: 2rpx solid var(--border-primary);
}

.preview-hint-text {
  font-size: 22rpx;
  color: var(--text-secondary);
  line-height: 1.4;
}

.canvas-section {
  display: flex;
  flex-direction: column;
  background: #000;
}

.canvas-container {
  width: 100%;
  aspect-ratio: 1;
  position: relative;
  overflow: hidden;
}

.canvas-placeholder {
  width: 100%;
  aspect-ratio: 1;
  background: #000000;
}

.preview-caption {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 14rpx 20rpx 18rpx;
  background: var(--bg-tertiary);
  border-bottom: 1rpx solid var(--border-color);
}

.preview-caption-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.preview-caption-title {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.preview-caption-text {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-actions {
  display: flex;
  align-items: center;
  gap: 12rpx;
  flex-shrink: 0;
}

.preview-actions .action-btn-sm {
  width: auto;
  min-width: 118rpx;
  height: 64rpx;
  padding: 0 18rpx;
  gap: 10rpx;
  border-radius: 18rpx;
}

.preview-actions .action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
  line-height: 1;
}

.preview-actions .action-btn-sm.primary text {
  color: #ffffff;
}

.canvas-wrapper {
  width: 100%;
  display: flex;
  justify-content: center;
}

.clock-canvas {
  width: 320px;
  height: 320px;
  background-color: #000;
  border: 2rpx solid var(--accent-primary);
  border-radius: 12rpx;
  box-shadow: 0 0 20rpx rgba(0, 243, 255, 0.3);
}

.tool-selection {
  display: flex;
  gap: 16rpx;
  margin-bottom: 24rpx;
}

.tool-option {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  padding: 32rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.tool-option:active {
  transform: scale(0.98);
}

.tool-option.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
  box-shadow: 0 0 16rpx rgba(0, 243, 255, 0.3);
}

.tool-name {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.tool-option.active .tool-name {
  color: var(--accent-primary);
  font-weight: 500;
}

.action-buttons {
  display: flex;
  gap: 16rpx;
  margin-top: 24rpx;
}

.action-button {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  font-size: 22rpx;
  color: var(--text-primary);
  transition: var(--transition-base);
}

.action-button:active {
  transform: scale(0.98);
  border-color: var(--accent-primary);
}

.action-button.danger {
  border-color: rgba(239, 68, 68, 0.3);
  color: var(--error-color);
}

.action-button.danger:active {
  background-color: rgba(239, 68, 68, 0.1);
}

.current-color-section {
  margin-bottom: 24rpx;
}

.current-color-display {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 16rpx;
  background-color: var(--bg-secondary);
  border-radius: 12rpx;
  margin-top: 12rpx;
}

.current-color-box {
  width: 80rpx;
  height: 80rpx;
  border-radius: 12rpx;
  border: 2rpx solid var(--border-primary);
  box-shadow: 0 2rpx 8rpx rgba(0, 0, 0, 0.2);
}

.current-color-hex {
  font-size: 28rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-primary);
}

.palette-toggle {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 20rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  margin: 24rpx 0;
  transition: var(--transition-base);
}

.palette-toggle:active {
  background-color: var(--bg-tertiary);
  border-color: var(--accent-primary);
}

.palette-toggle-text {
  font-size: 24rpx;
  color: var(--text-primary);
  font-weight: 500;
}

.palette-toggle-icon {
  font-size: 24rpx;
  color: var(--accent-primary);
}

.full-color-palette {
  display: grid;
  grid-template-columns: repeat(12, 1fr);
  gap: 8rpx;
  padding: 16rpx;
  background-color: var(--bg-secondary);
  border-radius: 12rpx;
  margin-bottom: 24rpx;
  max-height: 600rpx;
  overflow-y: auto;
}

.palette-color-item {
  aspect-ratio: 1;
  border-radius: 8rpx;
  border: 2rpx solid transparent;
  transition: var(--transition-base);
}

.palette-color-item:active {
  transform: scale(0.9);
}

.palette-color-item.active {
  border-color: var(--accent-primary);
  border-width: 3rpx;
  box-shadow: 0 0 12rpx var(--accent-primary);
  transform: scale(1.1);
  z-index: 1;
}

.align-buttons {
  display: flex;
  gap: 12rpx;
  margin-top: 12rpx;
}

.align-btn {
  flex: 1;
  padding: 20rpx 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  text-align: center;
  transition: var(--transition-base);
}

.align-btn:active {
  transform: scale(0.98);
}

.align-btn.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
}

.align-text {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.align-btn.active .align-text {
  color: var(--accent-primary);
  font-weight: 500;
}

.setting-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12rpx;
}

.quick-size-btn {
  padding: 12rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--accent-primary);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.quick-size-btn:active {
  transform: scale(0.95);
  background-color: rgba(0, 243, 255, 0.1);
}

.quick-size-btn .btn-text {
  font-size: 22rpx;
  color: var(--accent-primary);
  font-weight: 500;
}

.warning-color {
  color: #ff6464 !important;
}

.warning-text {
  font-size: 20rpx;
  color: #ff6464;
  margin-top: 8rpx;
  display: flex;
  align-items: center;
  gap: 4rpx;
}

.info-color {
  color: #4f7fff !important;
}

.info-text {
  font-size: 20rpx;
  color: #4f7fff;
  margin-top: 8rpx;
  display: flex;
  align-items: center;
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
}

.content-wrapper {
  /* padding: 24rpx; */
  padding-bottom: 48rpx;
}

/* 底部 Tab 栏：与拼豆辅助页底部控制区风格一致 */
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

.bottom-tab-item:active {
  background-color: var(--bg-tertiary);
}

.bottom-tab-item.active {
  background-color: var(--bg-tertiary);
}

.bottom-tab-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.bottom-tab-item.active .bottom-tab-text {
  color: var(--accent-primary);
  font-weight: 500;
}

.settings-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  /* border-radius: 16rpx; */
  padding: 20rpx;
  margin-bottom: 16rpx;
  /* box-shadow: var(--shadow-md); */
}

.card-title-section {
  display: flex;
  align-items: center;
  gap: 8rpx;
  margin-bottom: 16rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
  flex: 1;
}

.setting-group {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.setting-item {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.setting-item-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.setting-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.quick-size-btn {
  padding: 12rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--accent-primary);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.quick-size-btn:active {
  transform: scale(0.95);
  background-color: rgba(0, 243, 255, 0.1);
}

.quick-size-btn .btn-text {
  font-size: 22rpx;
  color: var(--accent-primary);
  font-weight: 500;
}

.size-input {
  width: 120rpx;
  height: 56rpx;
  padding: 0 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-secondary);
  border-radius: 12rpx;
  font-size: 28rpx;
  color: var(--text-primary);
  text-align: center;
  font-family: monospace;
  transition: var(--transition-base);
}

.size-input:focus {
  border-color: var(--accent-primary);
}

.size-input.warning {
  border-color: #ff6464;
  background-color: rgba(255, 100, 100, 0.1);
}

.warning-text {
  font-size: 18rpx;
  color: #ff6464;
  margin-top: 4rpx;
}

.setting-label {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.setting-control {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.setting-control-buttons {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.control-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.control-btn:active {
  transform: scale(0.9);
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
}

.control-icon {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.setting-value-large {
  font-size: 32rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--accent-primary);
  min-width: 64rpx;
  text-align: center;
}

.setting-value {
  font-size: 20rpx;
  font-family: monospace;
  color: var(--accent-primary);
  min-width: 40rpx;
  text-align: right;
}

.setting-slider {
  flex: 1;
}

.toggle-switch {
  margin-left: auto;
}

.switch-track {
  width: 80rpx;
  height: 44rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 22rpx;
  position: relative;
  transition: var(--transition-base);
}

.switch-track.active {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.switch-thumb {
  width: 36rpx;
  height: 36rpx;
  background-color: var(--text-primary);
  border-radius: 18rpx;
  position: absolute;
  top: 2rpx;
  left: 2rpx;
  transition: var(--transition-base);
}

.switch-track.active .switch-thumb {
  left: 38rpx;
  background-color: #ffffff;
}

.image-upload {
  margin-bottom: 24rpx;
}

.upload-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 64rpx 32rpx;
  background-color: var(--bg-secondary);
  border: 4rpx dashed var(--border-secondary);
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.upload-placeholder:active {
  border-color: var(--accent-primary);
  background-color: var(--bg-tertiary);
}

.upload-text {
  font-size: 26rpx;
  color: var(--text-primary);
  margin-top: 16rpx;
}

.upload-hint {
  font-size: 22rpx;
  color: var(--text-tertiary);
  margin-top: 8rpx;
}

.image-preview {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 16rpx;
  position: relative;
}

.gif-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  background: rgba(139, 92, 246, 0.9);
  border-radius: 8rpx;
  padding: 4rpx 12rpx;
}

.gif-badge-text {
  color: #fff;
  font-size: 22rpx;
  font-weight: 600;
}

.preview-image {
  width: 100%;
  height: 400rpx;
  background-color: var(--bg-secondary);
  border-radius: 16rpx;
  border: 2rpx solid var(--border-secondary);
}

.image-actions {
  display: flex;
  gap: 16rpx;
}

.image-action-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  padding: 20rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  font-size: 24rpx;
  color: var(--text-primary);
  transition: var(--transition-base);
}

.image-action-btn:active {
  transform: scale(0.98);
  border-color: var(--accent-primary);
}

.image-action-btn.danger {
  border-color: rgba(239, 68, 68, 0.3);
  color: var(--error-color);
}

.image-action-btn.danger:active {
  background-color: rgba(239, 68, 68, 0.1);
}

/* 传输遮罩弹窗 */
.sending-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}

.sending-modal {
  background: var(--color-bg-secondary, #fff);
  border-radius: 24rpx;
  padding: 60rpx 50rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
  min-width: 400rpx;
}

.sending-spinner {
  width: 60rpx;
  height: 60rpx;
  border: 6rpx solid rgba(99, 102, 241, 0.2);
  border-top-color: #6366f1;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.sending-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary, #333);
}

.sending-tip {
  font-size: 24rpx;
  color: var(--color-text-secondary, #999);
}

.sending-text {
  font-size: 24rpx;
  color: #fff;
}

.action-btn.disabled {
  opacity: 0.5;
  pointer-events: none;
}
</style>
