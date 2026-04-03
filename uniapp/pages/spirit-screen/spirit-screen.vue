<template>
  <view class="spirit-page">
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
        <text class="project-name">像素眼</text>
      </view>
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
          canvas-id="spiritPreviewCanvas"
        />
      </view>
      <view class="preview-caption">
        <view class="preview-caption-info">
          <text class="preview-title">64 x 64 动态预览</text>
          <text class="preview-subtitle">{{ previewSubtitle }}</text>
        </view>
        <view class="preview-actions">
          <view class="action-btn-sm" @click="saveDraft">
            <Icon name="save" :size="36" color="var(--color-text-primary)" />
            <text>保存</text>
          </view>
          <view
            class="action-btn-sm primary"
            :class="{ disabled: isSending }"
            @click="saveAndApply"
          >
            <Icon name="link" :size="36" color="#ffffff" />
            <text>{{ isSending ? "发送中" : "发送" }}</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
        <view v-show="currentTab === 0" class="tab-panel">
          <view class="card">
            <view class="card-title-section">
              <text class="card-title">表情模式</text>
              <text class="card-subtitle">{{ expressionModeLabel }}</text>
            </view>
            <view class="option-row">
              <view
                v-for="item in expressionModeOptions"
                :key="item.value"
                class="option-btn"
                :class="{ active: expressionModeValue === item.value }"
                @click="handleExpressionModeChange(item.value)"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
            <text class="form-tip">{{ expressionModeDescription }}</text>
          </view>

          <view v-show="expressionModeValue === 'manual'" class="card">
            <view class="card-title-section">
              <text class="card-title">指定表情</text>
              <text class="card-subtitle">21 个表情</text>
            </view>
            <view class="expression-grid">
              <view
                v-for="item in expressionOptions"
                :key="item.value"
                class="expression-item"
                :class="{ active: selectedEyesExpression === item.value }"
                @click="handleExpressionSelect(item.value)"
              >
                <text class="expression-cn">{{ item.label }}</text>
              </view>
            </view>
          </view>
        </view>

        <view v-show="currentTab === 1" class="tab-panel">
          <view class="card">
            <view class="card-title-section">
              <text class="card-title">眼睛颜色</text>
              <text class="card-subtitle">使用现成颜色面板</text>
            </view>
            <ColorPanelPicker
              :value="eyesConfig.style.eyeColor"
              label="眼睛颜色"
              :preset-colors="eyeColorOptions"
              @input="handleEyeColorChange"
            />
          </view>

          <view class="card">
            <view class="card-title-section">
              <text class="card-title">时间显示</text>
              <text class="card-subtitle">顶部 5x3 像素字</text>
            </view>
            <view class="form-row inline-row">
              <text class="form-label">显示时间</text>
              <switch
                :checked="eyesConfig.time.show"
                color="#4F7FFF"
                @change="handleTimeShowChange"
              />
            </view>
            <view class="form-row inline-row">
              <text class="form-label">显示秒</text>
              <switch
                :checked="eyesConfig.time.showSeconds"
                color="#4F7FFF"
                @change="handleTimeSecondsChange"
              />
            </view>
          </view>
        </view>

        <view v-show="currentTab === 2" class="tab-panel">
          <view class="card">
            <view class="card-title-section">
              <text class="card-title">参数调整</text>
              <text class="card-subtitle">数值越大越活跃</text>
            </view>

            <view class="form-row">
              <text class="form-label">
                眨眼步频 {{ blinkRhythmLevel }}
              </text>
              <slider
                :value="blinkRhythmLevel"
                min="1"
                max="10"
                step="1"
                activeColor="#4F7FFF"
                @change="handleBlinkIntervalChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">
                游走步频 {{ lookRhythmLevel }}
              </text>
              <slider
                :value="lookRhythmLevel"
                min="1"
                max="10"
                step="1"
                activeColor="#4F7FFF"
                @change="handleLookIntervalChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">
                灵动幅度 {{ motionAmplitudeLevel }}
              </text>
              <slider
                :value="motionAmplitudeLevel"
                min="1"
                max="10"
                step="1"
                activeColor="#4F7FFF"
                @change="handleIdleMoveChange"
              />
            </view>
          </view>

          <view class="card">
            <view class="card-title-section">
              <text class="card-title">互动预览</text>
              <text class="card-subtitle">只影响当前预览</text>
            </view>
            <view class="option-row option-row-wrap">
              <view class="option-btn" @click="triggerPreviewOnlyAction('blink')">
                <text>眨眼</text>
              </view>
              <view class="option-btn" @click="triggerPreviewOnlyAction('look_left')">
                <text>看左</text>
              </view>
              <view class="option-btn" @click="triggerPreviewOnlyAction('look_center')">
                <text>看中</text>
              </view>
              <view class="option-btn" @click="triggerPreviewOnlyAction('look_right')">
                <text>看右</text>
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
import ColorPanelPicker from "../../components/ColorPanelPicker.vue";

const EYES_CONFIG_STORAGE_KEY = "eyes_config";
const EYES_EXPRESSION_STORAGE_KEY = "eyes_expression";
const BLINK_DURATION_MS = 150;
const REFERENCE_EYE_SIZE = 20;

const EXPRESSION_OPTIONS = [
  { label: "正常", value: "Normal" },
  { label: "生气", value: "Angry" },
  { label: "偷笑", value: "Glee" },
  { label: "开心", value: "Happy" },
  { label: "难过", value: "Sad" },
  { label: "担忧", value: "Worried" },
  { label: "专注", value: "Focused" },
  { label: "不耐烦", value: "Annoyed" },
  { label: "惊讶", value: "Surprised" },
  { label: "怀疑", value: "Skeptic" },
  { label: "挫败", value: "Frustrated" },
  { label: "无语", value: "Unimpressed" },
  { label: "困倦", value: "Sleepy" },
  { label: "警觉", value: "Suspicious" },
  { label: "眯眼", value: "Squint" },
  { label: "愤怒", value: "Furious" },
  { label: "受惊", value: "Scared" },
  { label: "惊叹", value: "Awe" },
  { label: "兴奋", value: "Excited" },
  { label: "坚定", value: "Determined" },
  { label: "疑惑", value: "Confused" },
];

const PRESETS = {
  Normal: { offsetX: 0, offsetY: 0, height: 20, width: 20, slopeTop: 0, slopeBottom: 0, radiusTop: 4, radiusBottom: 4 },
  Angry: { offsetX: -2, offsetY: 0, height: 10, width: 20, slopeTop: 0.3, slopeBottom: 0, radiusTop: 1, radiusBottom: 6 },
  Glee: { offsetX: 0, offsetY: 0, height: 4, width: 20, slopeTop: 0, slopeBottom: 0, radiusTop: 4, radiusBottom: 0 },
  Happy: { offsetX: 0, offsetY: 0, height: 5, width: 20, slopeTop: 0, slopeBottom: 0, radiusTop: 5, radiusBottom: 0 },
  Sad: { offsetX: 0, offsetY: 0, height: 8, width: 20, slopeTop: -0.5, slopeBottom: 0, radiusTop: 1, radiusBottom: 5 },
  Worried: { offsetX: 0, offsetY: 0, height: 13, width: 20, slopeTop: -0.1, slopeBottom: 0, radiusTop: 3, radiusBottom: 5 },
  Focused: { offsetX: 0, offsetY: 0, height: 7, width: 20, slopeTop: 0.2, slopeBottom: 0, radiusTop: 2, radiusBottom: 1 },
  Annoyed: { offsetX: 0, offsetY: 0, height: 6, width: 20, slopeTop: 0, slopeBottom: 0, radiusTop: 0, radiusBottom: 5 },
  Surprised: { offsetX: -1, offsetY: 0, height: 23, width: 23, slopeTop: 0, slopeBottom: 0, radiusTop: 8, radiusBottom: 8 },
  Skeptic: { offsetX: 0, offsetY: -3, height: 13, width: 20, slopeTop: 0.3, slopeBottom: 0, radiusTop: 1, radiusBottom: 5 },
  Frustrated: { offsetX: 2, offsetY: -3, height: 6, width: 20, slopeTop: 0, slopeBottom: 0, radiusTop: 0, radiusBottom: 5 },
  Unimpressed: { offsetX: 2, offsetY: 0, height: 6, width: 20, slopeTop: 0, slopeBottom: 0, radiusTop: 1, radiusBottom: 5 },
  Sleepy: { offsetX: 0, offsetY: -1, height: 7, width: 20, slopeTop: -0.5, slopeBottom: -0.5, radiusTop: 2, radiusBottom: 2 },
  Suspicious: { offsetX: 0, offsetY: 0, height: 11, width: 20, slopeTop: 0, slopeBottom: 0, radiusTop: 4, radiusBottom: 2 },
  Squint: { offsetX: 3, offsetY: 0, height: 10, width: 10, slopeTop: 0, slopeBottom: 0, radiusTop: 3, radiusBottom: 3 },
  Furious: { offsetX: -1, offsetY: 0, height: 15, width: 20, slopeTop: 0.4, slopeBottom: 0, radiusTop: 1, radiusBottom: 4 },
  Scared: { offsetX: -2, offsetY: 0, height: 20, width: 20, slopeTop: -0.1, slopeBottom: 0, radiusTop: 6, radiusBottom: 4 },
  Awe: { offsetX: 1, offsetY: 0, height: 18, width: 23, slopeTop: -0.1, slopeBottom: 0.1, radiusTop: 6, radiusBottom: 6 },
  Excited: { offsetX: 0, offsetY: -2, height: 24, width: 22, slopeTop: 0.05, slopeBottom: -0.05, radiusTop: 7, radiusBottom: 7 },
  Determined: { offsetX: 0, offsetY: 0, height: 15, width: 20, slopeTop: 0.3, slopeBottom: 0, radiusTop: 2, radiusBottom: 2 },
  Confused: { offsetX: 0, offsetY: 0, height: 18, width: 20, slopeTop: -0.25, slopeBottom: 0.15, radiusTop: 5, radiusBottom: 5 },
};

const FONT_3X5 = {
  "0": ["111", "101", "101", "101", "111"],
  "1": ["010", "110", "010", "010", "111"],
  "2": ["111", "001", "111", "100", "111"],
  "3": ["111", "001", "111", "001", "111"],
  "4": ["101", "101", "111", "001", "001"],
  "5": ["111", "100", "111", "001", "111"],
  "6": ["111", "100", "111", "101", "111"],
  "7": ["111", "001", "001", "001", "001"],
  "8": ["111", "101", "111", "101", "111"],
  "9": ["111", "101", "111", "001", "111"],
  ":": ["000", "010", "000", "010", "000"],
};

function createDefaultEyesConfig() {
  return {
    layout: {
      eyeY: 24,
      eyeSpacing: 14,
      eyeWidth: 16,
      eyeHeight: 10,
      timeY: 4,
    },
    behavior: {
      autoSwitch: true,
      blinkIntervalMs: 3200,
      lookIntervalMs: 4200,
      idleMove: 2,
      sleepyAfterMs: 45000,
    },
    interaction: {
      lookHoldMs: 1200,
      moodHoldMs: 1800,
    },
    time: {
      show: true,
      showSeconds: false,
    },
    style: {
      eyeColor: "#9bdcff",
      pupilColor: "#1b6dff",
      timeColor: "#d8f3ff",
    },
  };
}

function cloneEyesConfig(config) {
  return JSON.parse(JSON.stringify(config));
}

function isValidEyesConfig(config) {
  if (!config || typeof config !== "object") {
    return false;
  }
  if (!config.layout || !config.behavior || !config.interaction || !config.time || !config.style) {
    return false;
  }
  if (
    config.layout.eyeY === undefined ||
    config.layout.eyeSpacing === undefined ||
    config.layout.eyeWidth === undefined ||
    config.layout.eyeHeight === undefined ||
    config.layout.timeY === undefined
  ) {
    return false;
  }
  if (
    config.behavior.autoSwitch === undefined ||
    config.behavior.blinkIntervalMs === undefined ||
    config.behavior.lookIntervalMs === undefined ||
    config.behavior.idleMove === undefined ||
    config.behavior.sleepyAfterMs === undefined
  ) {
    return false;
  }
  if (
    config.interaction.lookHoldMs === undefined ||
    config.interaction.moodHoldMs === undefined
  ) {
    return false;
  }
  if (
    config.time.show === undefined ||
    config.time.showSeconds === undefined
  ) {
    return false;
  }
  if (
    config.style.eyeColor === undefined ||
    config.style.pupilColor === undefined ||
    config.style.timeColor === undefined
  ) {
    return false;
  }
  return true;
}

function clamp(value, minValue, maxValue) {
  return Math.max(minValue, Math.min(maxValue, value));
}

function randomFloat(minValue, maxValue) {
  return minValue + Math.random() * (maxValue - minValue);
}

function jitteredInterval(baseMs, minPercent, maxPercent) {
  return baseMs * randomFloat(minPercent, maxPercent) / 100;
}

function createPreviewRuntime(expression) {
  const now = Date.now();
  return {
    expression,
    lookX: 0,
    lookY: 0,
    targetLookX: 0,
    targetLookY: 0,
    leftLookOffsetX: 0,
    leftLookOffsetY: 0,
    rightLookOffsetX: 0,
    rightLookOffsetY: 0,
    targetLeftLookOffsetX: 0,
    targetLeftLookOffsetY: 0,
    targetRightLookOffsetX: 0,
    targetRightLookOffsetY: 0,
    blinkActive: false,
    blinkStartAt: 0,
    leftBlinkDelayMs: 0,
    rightBlinkDelayMs: 0,
    leftBlinkScale: 1,
    rightBlinkScale: 1,
    lastBlinkAt: now,
    nextBlinkAfterMs: 0,
    lastLookAt: now,
    nextLookAfterMs: 0,
    lastExpressionAt: now,
    nextExpressionAfterMs: 0,
    actionExpireAt: 0,
  };
}

function normalizedLoopPhase(now, periodMs, phaseOffset) {
  if (!periodMs) {
    return 0;
  }
  let phase = (now % periodMs) / periodMs + phaseOffset;
  phase = phase - Math.floor(phase);
  return phase;
}

function trianglePulse(now, periodMs, phaseOffset) {
  const phase = normalizedLoopPhase(now, periodMs, phaseOffset);
  if (phase < 0.5) {
    return phase * 2;
  }
  return (1 - phase) * 2;
}

function pointInRectangle(px, py, x0, y0, x1, y1) {
  const left = Math.min(x0, x1);
  const right = Math.max(x0, x1);
  const top = Math.min(y0, y1);
  const bottom = Math.max(y0, y1);
  return px >= left && px <= right && py >= top && py <= bottom;
}

function triangleSign(px, py, ax, ay, bx, by) {
  return (px - bx) * (ay - by) - (ax - bx) * (py - by);
}

function pointInTriangle(px, py, ax, ay, bx, by, cx, cy) {
  const b1 = triangleSign(px, py, ax, ay, bx, by) < 0;
  const b2 = triangleSign(px, py, bx, by, cx, cy) < 0;
  const b3 = triangleSign(px, py, cx, cy, ax, ay) < 0;
  return b1 === b2 && b2 === b3;
}

function pointInQuarterEllipse(px, py, cx, cy, rx, ry, leftSide, topSide) {
  if (rx <= 0 || ry <= 0) {
    return false;
  }
  if (leftSide && px > cx) {
    return false;
  }
  if (!leftSide && px < cx) {
    return false;
  }
  if (topSide && py > cy) {
    return false;
  }
  if (!topSide && py < cy) {
    return false;
  }
  const dx = (px - cx) / rx;
  const dy = (py - cy) / ry;
  return dx * dx + dy * dy <= 1;
}

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    PixelCanvas,
    ColorPanelPicker,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isSending: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx - 112rpx)",
      previewCanvasReady: false,
      previewZoom: 4,
      previewOffset: { x: 16, y: 16 },
      previewContainerSize: { width: 320, height: 320 },
      previewPixels: new Map(),
      previewTimer: null,
      currentTab: 0,
      tabs: ["表情", "眼神设置", "参数调整"],
      tabIconNames: ["browse", "prompt", "task"],
      expressionModeOptions: [
        { label: "自动切换", value: "auto" },
        { label: "指定表情", value: "manual" },
      ],
      eyesConfig: createDefaultEyesConfig(),
      selectedEyesExpression: "Normal",
      previewRuntime: createPreviewRuntime("Normal"),
      expressionOptions: EXPRESSION_OPTIONS,
      eyeColorOptions: [
        { name: "浅蓝", hex: "#9bdcff" },
        { name: "冰蓝", hex: "#7fd8ff" },
        { name: "薄荷蓝", hex: "#8ee7f2" },
        { name: "天青", hex: "#68c9ff" },
        { name: "雾蓝", hex: "#8ebcff" },
        { name: "霓虹蓝", hex: "#52b7ff" },
      ],
    };
  },
  computed: {
    selectedExpressionLabel() {
      const matched = this.expressionOptions.find(
        (item) => item.value === this.selectedEyesExpression,
      );
      if (!matched) {
        return "正常";
      }
      return matched.label;
    },
    previewSubtitle() {
      if (this.eyesConfig.behavior.autoSwitch) {
        return `预览中：${this.previewExpressionLabel}`;
      }
      return `当前固定：${this.selectedExpressionLabel}`;
    },
    expressionModeValue() {
      return this.eyesConfig.behavior.autoSwitch ? "auto" : "manual";
    },
    expressionModeLabel() {
      return this.eyesConfig.behavior.autoSwitch ? "自动切换" : "指定表情";
    },
    expressionModeDescription() {
      if (this.eyesConfig.behavior.autoSwitch) {
        return "自动切换会自己变换表情。";
      }
      return "指定表情会固定当前选择。";
    },
    previewExpressionLabel() {
      const matched = this.expressionOptions.find(
        (item) => item.value === this.previewRuntime.expression,
      );
      if (!matched) {
        return "正常";
      }
      return matched.label;
    },
    blinkRhythmLevel() {
      return this.intervalToRhythmLevel(
        this.eyesConfig.behavior.blinkIntervalMs,
        800,
        6000,
      );
    },
    lookRhythmLevel() {
      return this.intervalToRhythmLevel(
        this.eyesConfig.behavior.lookIntervalMs,
        1200,
        8000,
      );
    },
    motionAmplitudeLevel() {
      return this.idleMoveToLevel(this.eyesConfig.behavior.idleMove);
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    this.loadEyesConfig();
    this.loadSelectedExpression();
    this.resetPreviewRuntime();
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    this.initPreviewCanvas();
    this.startPreviewLoop();
  },
  onUnload() {
    this.stopPreviewLoop();
  },
  methods: {
    handleBack() {
      uni.navigateBack();
    },

    initPreviewCanvas() {
      const systemInfo = uni.getSystemInfoSync();
      const statusBarHeight = systemInfo.statusBarHeight || 0;

      this.$nextTick(() => {
        setTimeout(() => {
          const query = uni.createSelectorQuery().in(this);
          query.select(".canvas-section").boundingClientRect((sectionRect) => {
            if (!sectionRect || !sectionRect.height) {
              return;
            }
            const nextHeight =
              systemInfo.windowHeight - statusBarHeight - 88 - sectionRect.height;
            this.contentHeight = `${Math.max(120, nextHeight - 76)}px`;
          });
          query
            .select(".preview-canvas-container")
            .boundingClientRect((data) => {
              if (!data || !data.width) {
                this.previewCanvasReady = true;
                return;
              }
              const fitZoom = Math.max(2, Math.floor((data.width * 0.92) / 64));
              this.previewContainerSize = {
                width: data.width,
                height: data.width,
              };
              this.previewZoom = fitZoom;
              this.previewOffset = {
                x: (data.width - 64 * fitZoom) / 2,
                y: (data.width - 64 * fitZoom) / 2,
              };
              this.previewCanvasReady = true;
            })
            .exec();
        }, 80);
      });
    },

    loadEyesConfig() {
      const savedEyesConfig = uni.getStorageSync(EYES_CONFIG_STORAGE_KEY);
      if (!savedEyesConfig) {
        this.eyesConfig = createDefaultEyesConfig();
        return;
      }

      try {
        const parsedConfig = JSON.parse(savedEyesConfig);
        if (!isValidEyesConfig(parsedConfig)) {
          this.eyesConfig = createDefaultEyesConfig();
          return;
        }
        this.eyesConfig = cloneEyesConfig(parsedConfig);
        this.eyesConfig.layout.timeY = 4;
      } catch (error) {
        console.error("读取像素眼配置失败:", error);
        this.eyesConfig = createDefaultEyesConfig();
      }
    },

    loadSelectedExpression() {
      const savedExpression = uni.getStorageSync(EYES_EXPRESSION_STORAGE_KEY);
      const matched = this.expressionOptions.find(
        (item) => item.value === savedExpression,
      );
      if (!matched) {
        this.selectedEyesExpression = "Normal";
        return;
      }
      this.selectedEyesExpression = matched.value;
    },

    saveDraft() {
      this.saveEyesConfig();
      this.saveSelectedExpression();
      this.toast.showSuccess("像素眼草稿已保存");
    },

    saveEyesConfig() {
      uni.setStorageSync(
        EYES_CONFIG_STORAGE_KEY,
        JSON.stringify(this.eyesConfig),
      );
    },

    saveSelectedExpression() {
      uni.setStorageSync(
        EYES_EXPRESSION_STORAGE_KEY,
        this.selectedEyesExpression,
      );
    },

    async saveAndApply() {
      if (this.isSending) {
        return;
      }
      if (!this.deviceStore.connected) {
        this.toast.showError("设备未连接");
        return;
      }

      this.isSending = true;
      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.setMode("eyes");
        await ws.setEyesConfig(cloneEyesConfig(this.eyesConfig));
        if (
          !this.eyesConfig.behavior.autoSwitch &&
          this.selectedEyesExpression !== "Normal"
        ) {
          await ws.eyesInteract(`set_expression:${this.selectedEyesExpression}`);
        }
        this.saveEyesConfig();
        this.saveSelectedExpression();
        this.deviceStore.setDeviceMode("eyes", { businessMode: true });
        this.toast.showSuccess("像素眼已发送到设备");
      } catch (error) {
        console.error("发送像素眼失败:", error);
        this.toast.showError("发送失败：" + error.message);
      } finally {
        this.isSending = false;
      }
    },

    async sendExpressionNow() {
      if (!this.deviceStore.connected) {
        this.toast.showError("设备未连接");
        return;
      }

      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.setMode("eyes");
        await ws.setEyesConfig(cloneEyesConfig(this.eyesConfig));
        if (
          !this.eyesConfig.behavior.autoSwitch &&
          this.selectedEyesExpression !== "Normal"
        ) {
          await ws.eyesInteract(`set_expression:${this.selectedEyesExpression}`);
        }
        this.deviceStore.setDeviceMode("eyes", { businessMode: true });
        this.toast.showSuccess("当前表情已应用到设备");
      } catch (error) {
        console.error("应用当前眼神失败:", error);
        this.toast.showError("应用失败：" + error.message);
      }
    },

    handleExpressionSelect(value) {
      this.selectedEyesExpression = value;
      this.saveSelectedExpression();
      this.previewRuntime.expression = value;
      this.previewRuntime.lastExpressionAt = Date.now();
      this.renderPreviewFrame();
    },

    handleExpressionModeChange(value) {
      this.eyesConfig.behavior.autoSwitch = value === "auto";
      if (this.eyesConfig.behavior.autoSwitch) {
        this.previewRuntime.lastExpressionAt = Date.now();
        this.previewRuntime.nextExpressionAfterMs = 0;
      } else {
        this.previewRuntime.expression = this.selectedEyesExpression;
      }
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleEyeColorChange(value) {
      this.eyesConfig.style.eyeColor = value;
      this.saveEyesConfig();
    },

    handleTimeShowChange(event) {
      this.eyesConfig.time.show = event.detail.value;
      this.saveEyesConfig();
    },

    handleTimeSecondsChange(event) {
      this.eyesConfig.time.showSeconds = event.detail.value;
      this.saveEyesConfig();
    },

    handleBlinkIntervalChange(event) {
      const level = Number(event.detail.value);
      this.eyesConfig.behavior.blinkIntervalMs = this.rhythmLevelToInterval(
        level,
        800,
        6000,
      );
      this.previewRuntime.nextBlinkAfterMs = 0;
      this.saveEyesConfig();
    },

    handleLookIntervalChange(event) {
      const level = Number(event.detail.value);
      this.eyesConfig.behavior.lookIntervalMs = this.rhythmLevelToInterval(
        level,
        1200,
        8000,
      );
      this.previewRuntime.nextLookAfterMs = 0;
      this.saveEyesConfig();
    },

    handleIdleMoveChange(event) {
      const level = Number(event.detail.value);
      this.eyesConfig.behavior.idleMove = this.levelToIdleMove(level);
      this.saveEyesConfig();
    },

    triggerPreviewOnlyAction(action) {
      const now = Date.now();
      if (action === "blink") {
        this.startPreviewBlink(now);
        return;
      }
      if (action === "look_left") {
        this.previewRuntime.targetLookX = -1;
        this.previewRuntime.targetLookY = 0;
      } else if (action === "look_center") {
        this.previewRuntime.targetLookX = 0;
        this.previewRuntime.targetLookY = 0;
      } else if (action === "look_right") {
        this.previewRuntime.targetLookX = 1;
        this.previewRuntime.targetLookY = 0;
      }
      this.previewRuntime.targetLeftLookOffsetX = 0;
      this.previewRuntime.targetLeftLookOffsetY = 0;
      this.previewRuntime.targetRightLookOffsetX = 0;
      this.previewRuntime.targetRightLookOffsetY = 0;
      this.previewRuntime.lastLookAt = now;
      this.previewRuntime.actionExpireAt =
        now + this.eyesConfig.interaction.lookHoldMs;
    },

    resetPreviewRuntime() {
      this.previewRuntime = createPreviewRuntime(this.selectedEyesExpression);
      this.previewRuntime.expression = this.selectedEyesExpression;
    },

    intervalToRhythmLevel(value, minInterval, maxInterval) {
      const normalizedValue = clamp(value, minInterval, maxInterval);
      const ratio =
        (maxInterval - normalizedValue) / (maxInterval - minInterval);
      return clamp(Math.round(ratio * 9) + 1, 1, 10);
    },

    rhythmLevelToInterval(level, minInterval, maxInterval) {
      const normalizedLevel = clamp(level, 1, 10);
      const ratio = (normalizedLevel - 1) / 9;
      return Math.round(maxInterval - (maxInterval - minInterval) * ratio);
    },

    idleMoveToLevel(value) {
      const normalizedValue = clamp(value, 1, 6);
      const ratio = (normalizedValue - 1) / 5;
      return clamp(Math.round(ratio * 9) + 1, 1, 10);
    },

    levelToIdleMove(level) {
      const normalizedLevel = clamp(level, 1, 10);
      const ratio = (normalizedLevel - 1) / 9;
      return Math.round(1 + ratio * 5);
    },

    startPreviewLoop() {
      this.stopPreviewLoop();
      this.renderPreviewFrame();
      this.previewTimer = setInterval(() => {
        this.renderPreviewFrame();
      }, 33);
    },

    stopPreviewLoop() {
      if (this.previewTimer) {
        clearInterval(this.previewTimer);
        this.previewTimer = null;
      }
    },

    scheduleNextBlink() {
      this.previewRuntime.nextBlinkAfterMs = jitteredInterval(
        this.eyesConfig.behavior.blinkIntervalMs,
        72,
        138,
      );
    },

    scheduleNextLook() {
      this.previewRuntime.nextLookAfterMs = jitteredInterval(
        this.eyesConfig.behavior.lookIntervalMs,
        68,
        150,
      );
    },

    scheduleNextExpression() {
      this.previewRuntime.nextExpressionAfterMs = jitteredInterval(4200, 70, 160);
    },

    chooseNextPreviewExpression() {
      const availableExpressions = this.expressionOptions.filter(
        (item) => item.value !== this.previewRuntime.expression,
      );
      if (availableExpressions.length === 0) {
        return this.previewRuntime.expression;
      }
      const randomIndex = Math.floor(Math.random() * availableExpressions.length);
      return availableExpressions[randomIndex].value;
    },

    startPreviewBlink(now) {
      this.previewRuntime.blinkActive = true;
      this.previewRuntime.blinkStartAt = now;
      this.previewRuntime.leftBlinkDelayMs = 0;
      this.previewRuntime.rightBlinkDelayMs = 0;
      this.previewRuntime.leftBlinkScale = 1;
      this.previewRuntime.rightBlinkScale = 1;

      if (Math.random() < 0.42) {
        const skew = Math.floor(Math.random() * 37) - 18;
        if (skew > 0) {
          this.previewRuntime.rightBlinkDelayMs = skew;
        } else if (skew < 0) {
          this.previewRuntime.leftBlinkDelayMs = -skew;
        }
      }

      if (Math.random() < 0.35) {
        const baseScale = randomFloat(0.93, 1.0);
        this.previewRuntime.leftBlinkScale = baseScale;
        this.previewRuntime.rightBlinkScale = baseScale;
      }
    },

    blinkAmountForEye(now, isLeftEye) {
      if (!this.previewRuntime.blinkActive) {
        return 0;
      }
      const delay = isLeftEye
        ? this.previewRuntime.leftBlinkDelayMs
        : this.previewRuntime.rightBlinkDelayMs;
      const scale = isLeftEye
        ? this.previewRuntime.leftBlinkScale
        : this.previewRuntime.rightBlinkScale;

      if (now < this.previewRuntime.blinkStartAt + delay) {
        return 0;
      }

      const phase =
        (now - this.previewRuntime.blinkStartAt - delay) / BLINK_DURATION_MS;
      if (phase < 0.5) {
        return clamp(phase * 2 * scale, 0, 1);
      }
      if (phase < 1) {
        return clamp((1 - (phase - 0.5) * 2) * scale, 0, 1);
      }
      return 0;
    },

    chooseIdleLook() {
      const activeExpression = this.previewRuntime.expression;
      let amplitudeScale = 1;
      if (
        activeExpression === "Sleepy" ||
        activeExpression === "Squint"
      ) {
        amplitudeScale = 0.72;
      } else if (
        activeExpression === "Excited" ||
        activeExpression === "Surprised"
      ) {
        amplitudeScale = 1.08;
      }

      const roll = Math.random() * 100;
      let baseX = 0;
      let baseY = 0;

      if (roll < 28) {
        baseX = randomFloat(-0.18, 0.18);
        baseY = randomFloat(-0.10, 0.10);
      } else if (roll < 54) {
        const side = Math.random() < 0.5 ? -1 : 1;
        baseX = side * randomFloat(0.42, 0.80);
        baseY = randomFloat(-0.18, 0.20);
      } else if (roll < 72) {
        baseX = randomFloat(-0.36, 0.36);
        baseY = randomFloat(-0.62, -0.22);
      } else if (roll < 84) {
        baseX = randomFloat(-0.28, 0.28);
        baseY = randomFloat(0.12, 0.34);
      } else {
        const side = Math.random() < 0.5 ? -1 : 1;
        baseX = side * randomFloat(0.18, 0.30);
        baseY = randomFloat(-0.08, 0.08);
      }

      this.previewRuntime.targetLookX = clamp(baseX * amplitudeScale, -0.9, 0.9);
      this.previewRuntime.targetLookY = clamp(baseY * amplitudeScale, -0.82, 0.6);

      this.previewRuntime.targetLeftLookOffsetX = 0;
      this.previewRuntime.targetLeftLookOffsetY = 0;
      this.previewRuntime.targetRightLookOffsetX = 0;
      this.previewRuntime.targetRightLookOffsetY = 0;

      let asymmetryChance = 22;
      if (
        activeExpression === "Confused" ||
        activeExpression === "Skeptic" ||
        activeExpression === "Worried"
      ) {
        asymmetryChance = 34;
      }

      if (Math.random() * 100 >= asymmetryChance) {
        return;
      }

      const microX = randomFloat(-0.10, 0.10);
      const microY = randomFloat(-0.05, 0.05);
      const lagFactor = randomFloat(0.28, 0.58);
      const leftLead = Math.random() < 0.5;

      if (leftLead) {
        this.previewRuntime.targetLeftLookOffsetX = microX;
        this.previewRuntime.targetLeftLookOffsetY = microY;
        this.previewRuntime.targetRightLookOffsetX = microX * lagFactor;
        this.previewRuntime.targetRightLookOffsetY = microY * lagFactor;
      } else {
        this.previewRuntime.targetLeftLookOffsetX = microX * lagFactor;
        this.previewRuntime.targetLeftLookOffsetY = microY * lagFactor;
        this.previewRuntime.targetRightLookOffsetX = microX;
        this.previewRuntime.targetRightLookOffsetY = microY;
      }
    },

    updatePreviewRuntime(now) {
      if (!this.eyesConfig.behavior.autoSwitch) {
        this.previewRuntime.expression = this.selectedEyesExpression;
      } else if (this.previewRuntime.nextExpressionAfterMs === 0) {
        this.scheduleNextExpression();
      }

      if (this.previewRuntime.nextBlinkAfterMs === 0) {
        this.scheduleNextBlink();
      }
      if (this.previewRuntime.nextLookAfterMs === 0) {
        this.scheduleNextLook();
      }

      if (this.previewRuntime.blinkActive) {
        const totalBlinkMs =
          BLINK_DURATION_MS +
          Math.max(
            this.previewRuntime.leftBlinkDelayMs,
            this.previewRuntime.rightBlinkDelayMs,
          );
        if (now - this.previewRuntime.blinkStartAt >= totalBlinkMs) {
          this.previewRuntime.blinkActive = false;
          this.previewRuntime.lastBlinkAt = now;
          this.scheduleNextBlink();
        }
      } else if (
        now - this.previewRuntime.lastBlinkAt >=
        this.previewRuntime.nextBlinkAfterMs
      ) {
        this.startPreviewBlink(now);
      }

      if (
        this.previewRuntime.actionExpireAt > 0 &&
        now >= this.previewRuntime.actionExpireAt
      ) {
        this.previewRuntime.actionExpireAt = 0;
        this.previewRuntime.targetLookX = 0;
        this.previewRuntime.targetLookY = 0;
        this.previewRuntime.targetLeftLookOffsetX = 0;
        this.previewRuntime.targetLeftLookOffsetY = 0;
        this.previewRuntime.targetRightLookOffsetX = 0;
        this.previewRuntime.targetRightLookOffsetY = 0;
      }

      if (
        this.eyesConfig.behavior.autoSwitch &&
        this.previewRuntime.actionExpireAt === 0 &&
        now - this.previewRuntime.lastExpressionAt >=
          this.previewRuntime.nextExpressionAfterMs
      ) {
        this.previewRuntime.expression = this.chooseNextPreviewExpression();
        this.previewRuntime.lastExpressionAt = now;
        this.scheduleNextExpression();
      }

      if (
        now - this.previewRuntime.lastLookAt >=
          this.previewRuntime.nextLookAfterMs &&
        this.previewRuntime.actionExpireAt === 0
      ) {
        this.previewRuntime.lastLookAt = now;
        this.chooseIdleLook();
        this.scheduleNextLook();
      }

      const lookEase = this.previewRuntime.actionExpireAt > 0 ? 0.24 : 0.14;
      this.previewRuntime.lookX +=
        (this.previewRuntime.targetLookX - this.previewRuntime.lookX) * lookEase;
      this.previewRuntime.lookY +=
        (this.previewRuntime.targetLookY - this.previewRuntime.lookY) * lookEase;
      this.previewRuntime.leftLookOffsetX +=
        (this.previewRuntime.targetLeftLookOffsetX -
          this.previewRuntime.leftLookOffsetX) *
        0.18;
      this.previewRuntime.leftLookOffsetY +=
        (this.previewRuntime.targetLeftLookOffsetY -
          this.previewRuntime.leftLookOffsetY) *
        0.18;
      this.previewRuntime.rightLookOffsetX +=
        (this.previewRuntime.targetRightLookOffsetX -
          this.previewRuntime.rightLookOffsetX) *
        0.18;
      this.previewRuntime.rightLookOffsetY +=
        (this.previewRuntime.targetRightLookOffsetY -
          this.previewRuntime.rightLookOffsetY) *
        0.18;
    },

    presetForEye(expression, isLeftEye, now) {
      const preset = { ...PRESETS[expression] };
      if (!isLeftEye) {
        return preset;
      }

      if (expression === "Worried") {
        preset.offsetY -= 1.5;
        preset.height += 3;
        preset.slopeTop -= 0.08;
      } else if (expression === "Annoyed") {
        preset.offsetY -= 2;
        preset.height += 4;
        preset.radiusBottom += 1.5;
      } else if (expression === "Skeptic") {
        preset.offsetY -= 1.5;
        preset.height += 2.5;
        preset.slopeTop += 0.05;
      } else if (expression === "Unimpressed") {
        preset.offsetY -= 2;
        preset.height += 5.5;
        preset.radiusBottom += 1.5;
      } else if (expression === "Sleepy") {
        preset.height -= 1.5;
        preset.offsetY -= 0.4;
      } else if (expression === "Suspicious") {
        preset.offsetY -= 1.8;
        preset.height += 2;
        preset.slopeTop += 0.12;
      } else if (expression === "Squint") {
        preset.offsetX = 1.2;
        preset.offsetY = -0.5;
        preset.width += 5;
        preset.height += 6;
      } else if (expression === "Confused") {
        preset.offsetY -= 1.4;
        preset.height += 2.8;
        preset.slopeTop -= 0.08;
        preset.slopeBottom += 0.04;
      }

      if (expression === "Normal") {
        preset.height += trianglePulse(now, 2200, isLeftEye ? 0.18 : 0.62) * 1.3;
      } else if (expression === "Happy" || expression === "Glee") {
        preset.offsetY += trianglePulse(now, 1500, isLeftEye ? 0.24 : 0.52) * 0.7;
      } else if (expression === "Confused") {
        preset.height += trianglePulse(now, 2100, isLeftEye ? 0.18 : 0.61) * 1.1;
      }

      return preset;
    },

    buildLookTransform(isLeftEye) {
      const lookX =
        this.previewRuntime.lookX +
        (isLeftEye
          ? this.previewRuntime.leftLookOffsetX
          : this.previewRuntime.rightLookOffsetX);
      const lookY =
        this.previewRuntime.lookY +
        (isLeftEye
          ? this.previewRuntime.leftLookOffsetY
          : this.previewRuntime.rightLookOffsetY);
      const normalizedX = clamp(lookX, -1, 1);
      const normalizedY = clamp(lookY, -1, 1);
      const lookSquash = 1 - Math.abs(normalizedY) * 0.18;
      const sideBias = isLeftEye
        ? 1 + normalizedX * 0.16
        : 1 - normalizedX * 0.16;
      const moveDirection = isLeftEye ? -1 : 1;

      return {
        moveX:
          this.eyesConfig.behavior.idleMove *
          0.85 *
          normalizedX *
          moveDirection,
        moveY: this.eyesConfig.behavior.idleMove * 0.65 * normalizedY,
        scaleX: 1,
        scaleY: clamp(lookSquash * sideBias, 0.72, 1.22),
      };
    },

    buildRasterEyeShape(centerX, centerY, preset, transform, blinkAmount) {
      const scaleX = this.eyesConfig.layout.eyeWidth / REFERENCE_EYE_SIZE;
      const scaleY = this.eyesConfig.layout.eyeHeight / REFERENCE_EYE_SIZE;
      const radiusScale = Math.min(scaleX, scaleY);

      const shape = {
        centerX,
        centerY,
        offsetX: (preset.offsetX + transform.moveX) * scaleX,
        offsetY: (preset.offsetY - transform.moveY) * scaleY,
        width: preset.width * scaleX * transform.scaleX,
        height: preset.height * scaleY * transform.scaleY,
        slopeTop: preset.slopeTop,
        slopeBottom: preset.slopeBottom,
        radiusTop: preset.radiusTop * radiusScale,
        radiusBottom: preset.radiusBottom * radiusScale,
      };

      const blinkScale = 1 - blinkAmount;
      shape.height = clamp(shape.height * blinkScale, 1, 64);
      shape.deltaTop = (shape.height * shape.slopeTop) / 2;
      shape.deltaBottom = (shape.height * shape.slopeBottom) / 2;

      const totalHeight = shape.height + shape.deltaTop - shape.deltaBottom;
      if (totalHeight <= 1) {
        shape.radiusTop = 0;
        shape.radiusBottom = 0;
      } else if (
        shape.radiusBottom > 0 &&
        shape.radiusTop > 0 &&
        totalHeight - 1 < shape.radiusBottom + shape.radiusTop
      ) {
        const scaleDown = (totalHeight - 1) / (shape.radiusBottom + shape.radiusTop);
        shape.radiusTop *= scaleDown;
        shape.radiusBottom *= scaleDown;
      }

      shape.TLcY =
        centerY + shape.offsetY - shape.height / 2 + shape.radiusTop - shape.deltaTop;
      shape.TLcX = centerX + shape.offsetX - shape.width / 2 + shape.radiusTop;
      shape.TRcY =
        centerY + shape.offsetY - shape.height / 2 + shape.radiusTop + shape.deltaTop;
      shape.TRcX = centerX + shape.offsetX + shape.width / 2 - shape.radiusTop;
      shape.BLcY =
        centerY + shape.offsetY + shape.height / 2 - shape.radiusBottom - shape.deltaBottom;
      shape.BLcX = centerX + shape.offsetX - shape.width / 2 + shape.radiusBottom;
      shape.BRcY =
        centerY + shape.offsetY + shape.height / 2 - shape.radiusBottom + shape.deltaBottom;
      shape.BRcX = centerX + shape.offsetX + shape.width / 2 - shape.radiusBottom;
      shape.minCX = Math.min(shape.TLcX, shape.BLcX);
      shape.maxCX = Math.max(shape.TRcX, shape.BRcX);
      shape.minCY = Math.min(shape.TLcY, shape.TRcY);
      shape.maxCY = Math.max(shape.BLcY, shape.BRcY);
      shape.minX = centerX + shape.offsetX - shape.width / 2;
      shape.maxX = centerX + shape.offsetX + shape.width / 2;
      shape.minY = Math.min(shape.TLcY - shape.radiusTop, shape.TRcY - shape.radiusTop);
      shape.maxY = Math.max(shape.BLcY + shape.radiusBottom, shape.BRcY + shape.radiusBottom);

      return shape;
    },

    containsShapePoint(shape, sampleX, sampleY) {
      if (pointInRectangle(sampleX, sampleY, shape.minCX, shape.minCY, shape.maxCX, shape.maxCY)) return true;
      if (pointInRectangle(sampleX, sampleY, shape.TRcX, shape.TRcY, shape.BRcX + shape.radiusBottom, shape.BRcY)) return true;
      if (pointInRectangle(sampleX, sampleY, shape.TLcX - shape.radiusTop, shape.TLcY, shape.BLcX, shape.BLcY)) return true;
      if (pointInRectangle(sampleX, sampleY, shape.TLcX, shape.TLcY - shape.radiusTop, shape.TRcX, shape.TRcY)) return true;
      if (pointInRectangle(sampleX, sampleY, shape.BLcX, shape.BLcY, shape.BRcX, shape.BRcY + shape.radiusBottom)) return true;

      if (
        shape.slopeTop > 0 &&
        pointInTriangle(
          sampleX,
          sampleY,
          shape.TLcX,
          shape.TLcY - shape.radiusTop,
          shape.TRcX,
          shape.TRcY - shape.radiusTop,
          shape.TRcX,
          shape.TLcY - shape.radiusTop,
        )
      ) {
        return true;
      }
      if (
        shape.slopeTop < 0 &&
        pointInTriangle(
          sampleX,
          sampleY,
          shape.TRcX,
          shape.TRcY - shape.radiusTop,
          shape.TLcX,
          shape.TLcY - shape.radiusTop,
          shape.TLcX,
          shape.TRcY - shape.radiusTop,
        )
      ) {
        return true;
      }
      if (
        shape.slopeBottom > 0 &&
        pointInTriangle(
          sampleX,
          sampleY,
          shape.BRcX + shape.radiusBottom,
          shape.BRcY + shape.radiusBottom,
          shape.BLcX - shape.radiusBottom,
          shape.BLcY + shape.radiusBottom,
          shape.BLcX - shape.radiusBottom,
          shape.BRcY + shape.radiusBottom,
        )
      ) {
        return true;
      }
      if (
        shape.slopeBottom < 0 &&
        pointInTriangle(
          sampleX,
          sampleY,
          shape.BLcX - shape.radiusBottom,
          shape.BLcY + shape.radiusBottom,
          shape.BRcX + shape.radiusBottom,
          shape.BRcY + shape.radiusBottom,
          shape.BRcX + shape.radiusBottom,
          shape.BLcY + shape.radiusBottom,
        )
      ) {
        return true;
      }

      if (shape.radiusTop > 0 && pointInQuarterEllipse(sampleX, sampleY, shape.TLcX, shape.TLcY, shape.radiusTop, shape.radiusTop, true, true)) return true;
      if (shape.radiusTop > 0 && pointInQuarterEllipse(sampleX, sampleY, shape.TRcX, shape.TRcY, shape.radiusTop, shape.radiusTop, false, true)) return true;
      if (shape.radiusBottom > 0 && pointInQuarterEllipse(sampleX, sampleY, shape.BLcX, shape.BLcY, shape.radiusBottom, shape.radiusBottom, true, false)) return true;
      if (shape.radiusBottom > 0 && pointInQuarterEllipse(sampleX, sampleY, shape.BRcX, shape.BRcY, shape.radiusBottom, shape.radiusBottom, false, false)) return true;

      return false;
    },

    drawEye(pixelMap, centerX, centerY, expression, isLeftEye, blinkAmount, now) {
      const preset = this.presetForEye(expression, isLeftEye, now);
      const transform = this.buildLookTransform(isLeftEye);
      const shape = this.buildRasterEyeShape(centerX, centerY, preset, transform, blinkAmount);
      const mirror = isLeftEye;
      const physicalMinX = mirror ? centerX * 2 - shape.maxX : shape.minX;
      const physicalMaxX = mirror ? centerX * 2 - shape.minX : shape.maxX;
      const startX = clamp(Math.floor(physicalMinX) - 1, 0, 63);
      const endX = clamp(Math.ceil(physicalMaxX) + 1, 0, 63);
      const startY = clamp(Math.floor(shape.minY) - 1, 0, 63);
      const endY = clamp(Math.ceil(shape.maxY) + 1, 0, 63);

      for (let y = startY; y <= endY; y++) {
        for (let x = startX; x <= endX; x++) {
          let sampleX = x + 0.5;
          const sampleY = y + 0.5;
          if (mirror) {
            sampleX = centerX * 2 - sampleX;
          }
          if (this.containsShapePoint(shape, sampleX, sampleY)) {
            pixelMap.set(`${x},${y}`, this.eyesConfig.style.eyeColor);
          }
        }
      }
    },

    drawTinyText(pixelMap, text, y, color) {
      const glyphWidth = 3;
      const spacing = 1;
      const totalWidth =
        text.length * glyphWidth + Math.max(0, text.length - 1) * spacing;
      let cursorX = Math.floor((64 - totalWidth) / 2);

      for (const char of text) {
        const glyph = FONT_3X5[char];
        if (!glyph) {
          cursorX += glyphWidth + spacing;
          continue;
        }
        for (let row = 0; row < glyph.length; row++) {
          for (let col = 0; col < glyph[row].length; col++) {
            if (glyph[row][col] === "1") {
              pixelMap.set(`${cursorX + col},${y + row}`, color);
            }
          }
        }
        cursorX += glyphWidth + spacing;
      }
    },

    drawTime(pixelMap) {
      if (!this.eyesConfig.time.show) {
        return;
      }
      const now = new Date();
      let text = now.toTimeString().slice(0, 5);
      if (this.eyesConfig.time.showSeconds) {
        text = now.toTimeString().slice(0, 8);
      }
      this.drawTinyText(
        pixelMap,
        text,
        this.eyesConfig.layout.timeY,
        this.eyesConfig.style.timeColor,
      );
    },

    renderPreviewFrame() {
      const now = Date.now();
      this.updatePreviewRuntime(now);

      const pixelMap = new Map();
      this.drawTime(pixelMap);

      const totalWidth =
        this.eyesConfig.layout.eyeWidth * 2 + this.eyesConfig.layout.eyeSpacing;
      const leftCenterX =
        (64 - totalWidth) / 2 + this.eyesConfig.layout.eyeWidth / 2;
      const rightCenterX =
        leftCenterX + this.eyesConfig.layout.eyeWidth + this.eyesConfig.layout.eyeSpacing;
      const centerY = this.eyesConfig.layout.eyeY;

      this.drawEye(
        pixelMap,
        leftCenterX,
        centerY,
        this.previewRuntime.expression,
        true,
        this.blinkAmountForEye(now, true),
        now,
      );
      this.drawEye(
        pixelMap,
        rightCenterX,
        centerY,
        this.previewRuntime.expression,
        false,
        this.blinkAmountForEye(now, false),
        now,
      );

      this.previewPixels = pixelMap;
    },
  },
};
</script>

<style scoped>
.spirit-page {
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

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.project-name {
  font-size: 33rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.canvas-section {
  display: flex;
  flex-direction: column;
  background: #000000;
  border-bottom: 2rpx solid var(--border-primary);
  flex-shrink: 0;
}

.preview-canvas-container {
  width: 100%;
  aspect-ratio: 1;
  position: relative;
  overflow: hidden;
  background-color: #000000;
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

.preview-title {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.preview-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-actions {
  display: flex;
  align-items: center;
  gap: 12rpx;
  flex-shrink: 0;
}

.action-btn-sm {
  width: auto;
  min-width: 118rpx;
  height: 64rpx;
  padding: 0 18rpx;
  gap: 10rpx;
  border-radius: 18rpx;
  border: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
  display: flex;
  align-items: center;
  justify-content: center;
}

.action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
  line-height: 1;
}

.action-btn-sm.primary {
  border-color: var(--accent-primary);
  background: var(--accent-primary);
}

.action-btn-sm.primary text {
  color: #ffffff;
}

.action-btn-sm.disabled {
  opacity: 0.4;
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
  background: var(--bg-tertiary);
  padding: 20rpx;
}

.content-wrapper {
  padding-bottom: 48rpx;
}

.tab-panel {
  min-height: 100%;
}

.card {
  padding-top: 20rpx;
}

.info-card {
  padding-bottom: 8rpx;
}

.card-title-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8rpx;
  margin-bottom: 16rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.card-subtitle {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.info-text {
  font-size: 24rpx;
  line-height: 1.7;
  color: var(--text-secondary);
}

.form-row {
  margin-top: 18rpx;
}

.form-tip {
  display: block;
  margin-top: 14rpx;
  font-size: 22rpx;
  line-height: 1.5;
  color: var(--text-secondary);
}

.inline-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.form-label {
  display: block;
  font-size: 26rpx;
  color: var(--text-primary);
  font-weight: 500;
}

.expression-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14rpx;
}

.expression-item {
  min-height: 74rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 14rpx 8rpx;
  box-sizing: border-box;
}

.expression-item.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
}

.expression-cn {
  font-size: 23rpx;
  color: var(--text-primary);
  font-weight: 600;
}

.option-row {
  margin-top: 10rpx;
  display: flex;
  gap: 16rpx;
}

.option-row-wrap {
  flex-wrap: wrap;
}

.option-btn {
  min-width: calc(25% - 12rpx);
  height: 72rpx;
  padding: 0 16rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
}

.option-btn text {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.option-btn.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
  box-shadow: 0 8rpx 18rpx rgba(79, 127, 255, 0.12);
}

.option-btn.active text {
  color: var(--accent-primary);
  font-weight: 600;
}

.primary-option {
  min-width: 100%;
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
}

.primary-option text {
  color: var(--accent-primary);
  font-weight: 600;
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
</style>
