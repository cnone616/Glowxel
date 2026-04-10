<template>
  <view class="clock-editor-page glx-page-shell">
    <!-- 状态栏占位 -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>

    <!-- 头部 -->
    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">{{ pageHeaderTitle }}</text>
      <view class="nav-right"></view>
    </view>

    <!-- Canvas 预览区域 -->
    <view class="canvas-section">
      <view v-if="!canvasHidden" class="canvas-container" ref="canvasContainer">
        <template v-if="useThemeImagePreview">
          <view class="theme-canvas-stage">
            <image
              :src="themeCanvasPreviewImage"
              class="theme-canvas-preview"
              mode="aspectFit"
            />
            <view class="theme-canvas-grid"></view>
            <view class="theme-preview-badge">
              <view class="theme-preview-badge-text">官方参考图</view>
            </view>
          </view>
        </template>
        <PixelCanvas
          v-else-if="canvasReady"
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
      <view v-else class="canvas-placeholder"></view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-caption-title">{{ previewPanelTitle }}</text>
          <view
            v-if="clockMode === 'theme' && displayClockThemePreset"
            class="preview-theme-chip"
          >
            <text class="preview-theme-chip-text">
              {{ displayClockThemePreset.name }}
              {{ isClockThemeModified ? " · 已微调" : "" }}
            </text>
          </view>
        </view>
        <view class="preview-actions">
          <view
            class="action-btn-sm primary glx-primary-action"
            :class="{ disabled: isSending }"
            @click="sendToDevice"
          >
            <Icon name="link" :size="36" color="#000000" />
            <text>发送</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 主内容：当前 Tab 的表单 -->
    <scroll-view
      scroll-y
      class="content glx-scroll-region glx-page-shell__content"
      :style="{ height: contentHeight }"
    >
      <view class="content-wrapper glx-scroll-stack">
        <ClockThemePanel
          v-if="clockMode === 'theme'"
          :presets="clockThemePresets"
          :selected-theme-id="displayClockThemeId"
          :current-theme-id="currentDeviceThemeId"
          :active-preset="displayClockThemePreset"
          :has-image="!!config.image.data || !!imagePixels"
          :is-modified="isClockThemeModified"
          @apply-theme="applyClockTheme"
        />

        <!-- 时间设置 -->
        <ClockTextSettingsCard
          v-show="currentTab === 1"
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
          v-show="currentTab === 2"
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
          v-show="currentTab === 4"
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
          v-show="currentTab === 5"
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
        <view v-show="currentTab === 3" class="settings-card">
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
                  : clockMode === "theme"
                    ? "点击上传主题图片"
                    : "点击上传背景图片"
              }}</text>
              <text class="upload-hint">{{
                clockMode === "animation"
                  ? "支持静态图片和 GIF 动画，建议 64x64 以内"
                  : clockMode === "theme"
                    ? "将自动适配到主题预设区域"
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
                  <Icon name="refresh" :size="28" color="#FFFFFF" />
                  <text>更换</text>
                </view>
                <view class="image-action-btn danger" @click="removeImage">
                  <Icon name="ashbin" :size="28" color="#FFFFFF" />
                  <text>删除</text>
                </view>
              </view>
            </view>
          </view>

          <!-- 图片尺寸和位置控制 -->
          <view
            v-if="config.image.data && clockMode !== 'theme'"
            class="setting-group"
          >
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
    <view v-if="clockMode !== 'theme'" class="bottom-tabs">
      <view
        v-for="tab in visibleTabDefinitions"
        :key="tab.index"
        class="bottom-tab-item"
        :class="{ active: currentTab === tab.index }"
        @click="currentTab = tab.index"
      >
        <Icon
          :name="tab.icon"
          :size="36"
          :color="currentTab === tab.index ? '#000000' : '#666666'"
        />
        <text class="bottom-tab-text">{{ tab.label }}</text>
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
import ClockThemePanel from "../../components/clock-editor/ClockThemePanel.vue";
import ClockTextSettingsCard from "../../components/clock-editor/ClockTextSettingsCard.vue";
import { getClockFontOptions } from "../../utils/clockCanvas.js";
import {
  applyClockThemePreset,
  findClockThemePreset,
  getClockThemePresets,
  getMatchingClockThemeId,
} from "../../utils/clockThemePresets.js";

const CLOCK_DEVICE_THEME_ID_KEY = "clock_device_theme_id";

export default {
  mixins: [statusBarMixin, clockPreviewMixin, imageGifMixin, deviceSyncMixin],
  components: {
    Icon,
    Toast,
    PixelCanvas,
    ClockFontPanel,
    ClockThemePanel,
    ClockTextSettingsCard,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isReady: false,
      clockMode: "clock", // clock=静态时钟, animation=动态时钟, theme=主题模式
      gifAnimationData: null, // GIF 动画数据 { frameCount, frames }
      gifRenderedFrameMaps: null, // GIF 所有帧的像素 Map 数组
      gifFrameIndex: 0, // 当前播放帧索引
      gifTimer: null, // GIF 动画定时器
      gifIsPlaying: false, // 是否正在播放
      gifPlaySpeed: 1.0, // 播放速度倍率
      lastAppliedClockThemeId: "",
      deviceThemeId: "",
      isSending: false, // 传输锁，防止传输过程中操作
      _gifParser: null, // GIF 解析器实例，改宽高时重新生成数据
      _imageConvertToken: 0,

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
      clockThemePresets: getClockThemePresets(),

      currentTab: 1,
      tabDefinitions: [
        { index: 0, label: "主题", icon: "picture" },
        { index: 1, label: "时间", icon: "time" },
        { index: 2, label: "字体", icon: "text" },
        { index: 3, label: "图片", icon: "picture" },
        { index: 4, label: "日期", icon: "calendar" },
        { index: 5, label: "星期", icon: "rili5" },
      ],

      config: {
        font: "classic_5x7",
        showSeconds: false,
        hourFormat: 24,
        time: {
          show: true,
          fontSize: 1,
          x: 32,
          y: 5,
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
      } else if (newMode === "clock" || newMode === "theme") {
        this.stopGifAnimation();
      }
      this.ensureValidCurrentTab();
    },
    visibleTabDefinitions: {
      handler(nextTabs) {
        const validTabIndexes = nextTabs.map((tab) => tab.index);
        if (
          !validTabIndexes.includes(this.currentTab) &&
          validTabIndexes.length > 0
        ) {
          this.currentTab = validTabIndexes[0];
        }
      },
      deep: true,
      immediate: true,
    },
  },

  computed: {
    accentColor() {
      return "#4F7FFF";
    },
    pageHeaderTitle() {
      if (this.clockMode === "theme") {
        return "主题模式";
      }
      if (this.clockMode === "animation") {
        return "动态时钟";
      }
      return "静态时钟";
    },
    displayClockThemeId() {
      if (this.clockMode === "theme") {
        return this.lastAppliedClockThemeId;
      }
      return this.activeClockThemeId || this.lastAppliedClockThemeId;
    },
    displayClockThemePreset() {
      if (!this.displayClockThemeId) {
        return null;
      }
      return findClockThemePreset(this.displayClockThemeId);
    },
    currentDeviceThemeId() {
      if (!this.deviceStore || !this.deviceStore.connected) {
        return "";
      }
      if (this.deviceStore.deviceMode !== "theme") {
        return "";
      }
      return this.deviceThemeId;
    },
    isClockThemeModified() {
      if (this.clockMode === "theme") {
        return false;
      }
      return (
        !!this.lastAppliedClockThemeId &&
        this.activeClockThemeId !== this.lastAppliedClockThemeId
      );
    },
    visibleTabDefinitions() {
      if (this.clockMode === "theme") {
        return [];
      }
      return this.tabDefinitions.filter((tab) => tab.index !== 0);
    },
    activeClockThemeId() {
      return getMatchingClockThemeId(this.config);
    },
    useThemeImagePreview() {
      return (
        this.clockMode === "theme" &&
        !!this.displayClockThemePreset &&
        !!this.displayClockThemePreset.previewImage
      );
    },
    themeCanvasPreviewImage() {
      if (!this.displayClockThemePreset) {
        return "";
      }
      return this.displayClockThemePreset.previewImage;
    },
    previewPanelTitle() {
      return "模拟预览";
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
    this.cleanupTransientState();
  },

  onHide() {
    this.cleanupTransientState();
  },

  onLoad(options) {
    const savedMode = uni.getStorageSync("device_mode");
    const defaultMode =
      savedMode === "animation" || savedMode === "theme" ? savedMode : "clock";
    this.clockMode = options.mode || defaultMode;
    this._clockModeFromOptions = !!options.mode;
    this.ensureValidCurrentTab();

    this.loadConfig();
    const savedDeviceThemeId = uni.getStorageSync(CLOCK_DEVICE_THEME_ID_KEY);
    if (typeof savedDeviceThemeId === "string") {
      this.deviceThemeId = savedDeviceThemeId;
    }
    if (!this.lastAppliedClockThemeId && this.clockMode !== "theme") {
      this.lastAppliedClockThemeId = getMatchingClockThemeId(this.config);
    }
    if (this.clockMode === "theme") {
      this.ensureValidThemeSelection();
    }
    if (this.lastAppliedClockThemeId) {
      const preset = findClockThemePreset(this.lastAppliedClockThemeId);
      if (preset && preset.config.image.data) {
        this.config.image.data = preset.config.image.data;
        this.config.image.show = true;
      }
    }

    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();

    const systemInfo = uni.getSystemInfoSync();
    const statusBarHeight = systemInfo.statusBarHeight || 0;
    const headerHeight = 56;
    this.contentHeight = `${systemInfo.windowHeight - statusBarHeight - headerHeight - 360}px`;

    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
      setTimeout(() => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select(".canvas-container")
          .boundingClientRect((data) => {
            if (data && data.width > 0) {
              this.containerSize = { width: data.width, height: data.width };
              const fitZoom = Math.floor((data.width * 0.96) / 64);
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
            this.isReady = true;
          })
          .exec();
      }, 150);
    });
  },

  onShow() {
    if (
      this.clockMode === "animation" &&
      this.gifRenderedFrameMaps &&
      this.gifRenderedFrameMaps.length > 1 &&
      !this.gifIsPlaying
    ) {
      this.startGifAnimation();
    }
    if (!this.isSending) {
      this.canvasHidden = false;
    }
  },

  methods: {
    ensureValidCurrentTab() {
      if (this.clockMode === "theme") {
        this.currentTab = 0;
        return;
      }

      const validTabs = this.visibleTabDefinitions.map((tab) => tab.index);
      if (!validTabs.includes(this.currentTab) && validTabs.length > 0) {
        this.currentTab = validTabs[0];
      }
    },

    onToastHide() {
      if (!this.isSending) {
        this.canvasHidden = false;
      }
    },
    cleanupTransientState() {
      this.stopGifAnimation();
      this.stopLoading();
      if (this._resizeDebounceTimer) {
        clearTimeout(this._resizeDebounceTimer);
        this._resizeDebounceTimer = null;
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
    ensureValidThemeSelection() {
      if (this.clockThemePresets.length === 0) {
        this.lastAppliedClockThemeId = "";
        return;
      }

      const hasMatchedPreset = this.clockThemePresets.some(
        (preset) => preset.id === this.lastAppliedClockThemeId,
      );
      if (hasMatchedPreset) {
        return;
      }

      this.applyClockTheme(this.clockThemePresets[0].id);
    },
    applyClockTheme(themeId) {
      const preset = findClockThemePreset(themeId);
      this._imageConvertToken += 1;
      this.config = applyClockThemePreset(this.config, themeId);
      this.lastAppliedClockThemeId = themeId;
      this.stopGifAnimation();
      this.gifAnimationData = null;
      this.gifRenderedFrameMaps = null;
      this.gifFrameIndex = 0;
      this._gifParser = null;
      this.imagePixels = null;

      if (preset && preset.config.image.data) {
        this.config.image.show = true;
        this.convertImageToPixels(preset.config.image.data);
      } else if (preset && !preset.requiresImage) {
        this.config.image.data = null;
        this.config.image.show = false;
        this.imagePixels = null;
        this.drawCanvas();
      } else {
        this.drawCanvas();
      }

      if (preset && preset.requiresImage) {
        this.currentTab = 3;
      } else {
        this.currentTab = 0;
      }
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

.icon {
  font-size: 56rpx;
  color: var(--text-primary);
}

.action-btn-sm {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  background-color: var(--bg-tertiary);
  transition: var(--transition-base);
}

.action-btn-sm.primary {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.preview-hint {
  padding: 20rpx 32rpx;
  background-color: var(--bg-tertiary);
  border-bottom: 2rpx solid var(--nb-ink);
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
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  background: #000000;
}

.canvas-placeholder {
  width: 100%;
  aspect-ratio: 1;
  background: #000000;
}

.theme-canvas-stage {
  position: relative;
  width: calc(100% - 32rpx);
  height: calc(100% - 32rpx);
  margin: 16rpx;
  background: #000000;
}

.theme-canvas-preview {
  width: 100%;
  height: 100%;
  display: block;
  background: #000000;
  image-rendering: pixelated;
}

.theme-canvas-grid {
  position: absolute;
  inset: 0;
  pointer-events: none;
  background-image:
    linear-gradient(rgba(255, 255, 255, 0.24) 1px, transparent 1px),
    linear-gradient(90deg, rgba(255, 255, 255, 0.24) 1px, transparent 1px);
  background-size: calc(100% / 64) calc(100% / 64);
  box-shadow: inset 0 0 0 2rpx rgba(255, 255, 255, 0.32);
}

.theme-preview-badge {
  position: absolute;
  top: 14rpx;
  right: 14rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 42rpx;
  padding: 8rpx 14rpx;
  border-radius: 0;
  background: var(--nb-yellow);
  border: 2rpx solid #000000;
  backdrop-filter: none;
}

.theme-preview-badge-text {
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18rpx;
  font-weight: 600;
  line-height: 1;
  color: #000000;
}

.preview-caption {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12rpx;
  padding: 10rpx 16rpx 12rpx;
  background: var(--bg-tertiary);
}

.preview-caption-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 0;
}

.preview-caption-title {
  font-size: 24rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.preview-theme-chip {
  display: inline-flex;
  width: fit-content;
  margin-top: 4rpx;
  padding: 6rpx 12rpx;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid #000000;
}

.preview-theme-chip-text {
  font-size: 20rpx;
  font-weight: 600;
  color: #000000;
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
  border-radius: 0;
}

.preview-actions .action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
  line-height: 1;
}

.preview-actions .action-btn-sm.primary text {
  color: #000000;
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
  border: 2rpx solid var(--nb-yellow);
  border-radius: 0;
  box-shadow: none;
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
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: var(--transition-base);
}

.tool-option.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
  box-shadow: none;
}

.tool-name {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.tool-option.active .tool-name {
  color: #000000;
  font-weight: 700;
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
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  font-size: 22rpx;
  color: var(--text-primary);
  transition: var(--transition-base);
}

.action-button.danger {
  background-color: #d92d20;
  border-color: #000000;
  color: #ffffff;
}

.action-button.danger:active {
  background-color: #d92d20;
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
  border-radius: 0;
  margin-top: 12rpx;
  border: 2rpx solid var(--nb-ink);
}

.current-color-box {
  width: 80rpx;
  height: 80rpx;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  box-shadow: none;
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
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  margin: 24rpx 0;
  transition: var(--transition-base);
}

.palette-toggle:active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.palette-toggle-text {
  font-size: 24rpx;
  color: var(--text-primary);
  font-weight: 500;
}

.palette-toggle-icon {
  font-size: 24rpx;
  color: var(--nb-ink);
}

.full-color-palette {
  display: grid;
  grid-template-columns: repeat(12, 1fr);
  gap: 8rpx;
  padding: 16rpx;
  background-color: var(--bg-secondary);
  border-radius: 0;
  margin-bottom: 24rpx;
  max-height: 600rpx;
  overflow-y: auto;
  border: 2rpx solid var(--nb-ink);
}

.palette-color-item {
  aspect-ratio: 1;
  border-radius: 0;
  border: 2rpx solid #000000;
  transition: var(--transition-base);
}

.palette-color-item.active {
  border-color: var(--nb-yellow);
  border-width: 4rpx;
  box-shadow: none;
  transform: none;
  z-index: auto;
}

.setting-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12rpx;
}

.quick-size-btn {
  min-height: 60rpx;
  padding: 12rpx 24rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background-color: #ffffff;
  border: 2rpx solid #000000;
  border-radius: 0;
  box-shadow: none;
  transition: var(--transition-base);
}

.quick-size-btn .btn-text {
  font-size: 22rpx;
  color: #000000;
  font-weight: 700;
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
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.content-wrapper {
  padding: 0 0 56rpx;
}

/* 底部 Tab 栏：与拼豆辅助页底部控制区风格一致 */
.bottom-tabs {
  display: flex;
  flex-shrink: 0;
  padding: 2rpx 10rpx 0;
  padding-bottom: var(--layout-bottom-offset);
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--nb-ink);
  gap: 2rpx;
}

.bottom-tab-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 2rpx;
  min-height: 68rpx;
  padding: 2rpx 0;
  background-color: transparent;
}

.bottom-tab-item:active {
  background-color: transparent;
}

.bottom-tab-item.active {
  background-color: transparent;
}

.bottom-tab-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.bottom-tab-item.active .bottom-tab-text {
  color: #000000;
  font-weight: 900;
  font-size: 22rpx;
}

.settings-card {
  background-color: transparent;
  border: 0;
  padding: 0;
  margin-bottom: 16rpx;
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
  border: 2rpx solid var(--nb-yellow);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.quick-size-btn .btn-text {
  font-size: 22rpx;
  color: var(--nb-yellow);
  font-weight: 500;
}

.size-input {
  width: 120rpx;
  height: 56rpx;
  padding: 0 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 12rpx;
  font-size: 28rpx;
  color: var(--text-primary);
  text-align: center;
  font-family: monospace;
  transition: var(--transition-base);
}

.size-input:focus {
  border-color: var(--nb-yellow);
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
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: var(--transition-base);
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
  color: var(--nb-yellow);
  min-width: 64rpx;
  text-align: center;
}

.setting-value {
  font-size: 20rpx;
  font-family: monospace;
  color: var(--nb-yellow);
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
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  position: relative;
  transition: var(--transition-base);
}

.switch-track.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.switch-thumb {
  width: 36rpx;
  height: 36rpx;
  background-color: var(--text-primary);
  border-radius: 0;
  position: absolute;
  top: 2rpx;
  left: 2rpx;
  transition: var(--transition-base);
}

.switch-track.active .switch-thumb {
  left: 38rpx;
  background-color: #000000;
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
  border: 4rpx dashed var(--nb-ink);
  border-radius: 0;
  transition: var(--transition-base);
}

.upload-placeholder:active {
  border-color: var(--nb-yellow);
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
  border-radius: 0;
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
  border-radius: 0;
  /* border: 2rpx solid var(--nb-ink); */
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
  background-color: #2563eb;
  border: 2rpx solid #000000;
  border-radius: 12rpx;
  font-size: 24rpx;
  color: #ffffff;
  transition: var(--transition-base);
}

.image-action-btn.danger {
  background-color: #d92d20;
  border-color: #000000;
  color: #ffffff;
}

.image-action-btn.danger:active {
  background-color: #d92d20;
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
  background: var(--nb-surface);
  border-radius: 0;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
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
  border: 6rpx solid rgba(0, 0, 0, 0.16);
  border-top-color: var(--nb-ink);
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
  font-weight: 900;
  color: var(--nb-ink);
}

.sending-tip {
  font-size: 24rpx;
  color: #4a4a4a;
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
