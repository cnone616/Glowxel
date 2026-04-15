<template>
  <view class="spirit-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--nb-ink)"
        />
      </view>
      <text class="nav-title glx-topbar__title">桌面宠物</text>
      <view class="nav-right"></view>
    </view>

    <view class="canvas-section">
      <view v-if="!previewHidden" class="preview-canvas-container">
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
      <view
        v-else
        class="preview-canvas-container preview-canvas-placeholder"
      ></view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-title">预览效果</text>
        </view>
        <view class="preview-actions">
          <view
            class="action-btn-sm primary glx-primary-action"
            :class="{ disabled: isSending }"
            @click="saveAndApply"
          >
            <Icon name="link" :size="36" color="var(--nb-ink)" />
            <text>{{ isSending ? "发送中" : "发送" }}</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content" :style="{ height: contentHeight }">
      <view class="content-wrapper glx-scroll-stack">
        <view v-show="currentTab === 0" class="tab-panel glx-tab-panel">
          <view class="card glx-panel-card glx-editor-card spirit-section-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">表情模式</text>
              <text class="card-subtitle glx-panel-subtitle">{{ expressionModeLabel }}</text>
            </view>
            <view class="option-row">
              <view
                v-for="item in expressionModeOptions"
                :key="item.value"
                class="option-btn glx-feature-option"
                :class="{ active: expressionModeValue === item.value }"
                @click="handleExpressionModeChange(item.value)"
              >
                <text class="glx-feature-option__label">{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view v-show="expressionModeValue === 'manual'" class="card glx-panel-card glx-editor-card spirit-section-card">
            <text class="card-title glx-panel-title">指定表情</text>
            <view class="expression-grid">
              <view
                v-for="item in expressionOptions"
                :key="item.value"
                class="expression-item glx-feature-option"
                :class="{ active: selectedEyesExpression === item.value }"
                @click="handleExpressionSelect(item.value)"
              >
                <text class="expression-cn glx-feature-option__label">{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view class="card glx-panel-card glx-editor-card spirit-section-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">眼睛颜色</text>
            </view>
            <ColorPanelPicker
              :value="eyesConfig.style.eyeColor"
              label="眼睛颜色"
              :preset-colors="eyeColorOptions"
              @input="handleEyeColorChange"
            />
          </view>

          <view class="card glx-panel-card glx-editor-card spirit-section-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">参数调整</text>
            </view>

            <view class="form-row">
              <text class="form-label">
                眨眼步频 {{ blinkRhythmLevel }}
              </text>
              <GlxStepper
                :value="blinkRhythmLevel"
                :min="1"
                :max="10"
                :step="1"
                @change="handleBlinkIntervalChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">
                游走步频 {{ lookRhythmLevel }}
              </text>
              <GlxStepper
                :value="lookRhythmLevel"
                :min="1"
                :max="10"
                :step="1"
                @change="handleLookIntervalChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">
                灵动幅度 {{ motionAmplitudeLevel }}
              </text>
              <GlxStepper
                :value="motionAmplitudeLevel"
                :min="1"
                :max="10"
                :step="1"
                @change="handleIdleMoveChange"
              />
            </view>
          </view>

          <view class="card glx-panel-card glx-editor-card spirit-section-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">互动预览</text>
            </view>
            <view class="option-row option-row-wrap">
              <view class="option-btn glx-feature-option" @click="triggerPreviewOnlyAction('blink')">
                <text>眨眼</text>
              </view>
              <view class="option-btn glx-feature-option" @click="triggerPreviewOnlyAction('look_left')">
                <text>看左</text>
              </view>
              <view class="option-btn glx-feature-option" @click="triggerPreviewOnlyAction('look_center')">
                <text>看中</text>
              </view>
              <view class="option-btn glx-feature-option" @click="triggerPreviewOnlyAction('look_right')">
                <text>看右</text>
              </view>
            </view>
          </view>
        </view>

        <view v-show="currentTab === 1" class="tab-panel glx-tab-panel">
          <ClockTextSettingsCard
            icon-name="time"
            title="时间显示"
            :section="spiritTimeSection"
            :preset-colors="timeColorOptions"
            :show-font-size="true"
            :min-font-size="1"
            :max-font-size="3"
            @toggle="toggleTimeShow"
            @adjust="handleSpiritTimeAdjust"
            @update-color="handleTimeColorChange"
            @set-align="handleSpiritTimeAlign"
          />
        </view>

        <view v-show="currentTab === 2" class="tab-panel glx-tab-panel">
          <ClockFontPanel
            :font-options="timeFontOptions"
            :selected-font="eyesConfig.time.font"
            :show-seconds="eyesConfig.time.showSeconds"
            :hour-format="24"
            :show-hour-format="false"
            @select-font="handleTimeFontChange"
            @toggle-seconds="toggleTimeSeconds"
          />
        </view>
      </view>
    </scroll-view>

    <view class="bottom-tabs">
      <view
        v-for="(tab, index) in tabs"
        :key="tab"
        class="glx-bottom-tab-item"
        :class="{ active: currentTab === index }"
        @click="currentTab = index"
      >
        <Icon
          :name="tabIconNames[index]"
          :size="36"
          :color="currentTab === index ? '#000000' : '#6b7280'"
        />
        <text class="glx-bottom-tab-text">{{ tab }}</text>
      </view>
    </view>

    <Toast ref="toastRef" @show="previewHidden = true" @hide="onToastHide" />
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
import GlxStepper from "../../components/GlxStepper.vue";
import ClockFontPanel from "../../components/clock-editor/ClockFontPanel.vue";
import ClockTextSettingsCard from "../../components/clock-editor/ClockTextSettingsCard.vue";
import {
  drawClockTextToPixels,
  getClockFontOptions,
  getClockTextHeight,
  getClockTextWidth,
} from "../../utils/clockCanvas.js";

const EYES_CONFIG_STORAGE_KEY = "eyes_config";
const EYES_EXPRESSION_STORAGE_KEY = "eyes_expression";
const EYES_TIME_FONT_OPTIONS = getClockFontOptions();
const EYES_TIME_FONT_IDS = new Set(
  EYES_TIME_FONT_OPTIONS.map((item) => item.id),
);
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
];

const ALL_EXPRESSION_VALUES = EXPRESSION_OPTIONS.map((item) => item.value);
const PREVIEW_TRANSITION_WEIGHTS = {
  Normal: [
    ["Normal", 20],
    ["Happy", 20],
    ["Focused", 20],
    ["Surprised", 15],
    ["Skeptic", 15],
    ["Determined", 10],
  ],
  Happy: [
    ["Happy", 20],
    ["Glee", 25],
    ["Excited", 20],
    ["Normal", 15],
    ["Surprised", 10],
  ],
  Angry: [
    ["Angry", 20],
    ["Furious", 25],
    ["Annoyed", 20],
    ["Frustrated", 20],
    ["Normal", 10],
  ],
  Sleepy: [
    ["Sleepy", 30],
    ["Squint", 25],
    ["Unimpressed", 20],
    ["Worried", 15],
    ["Normal", 10],
  ],
  Focused: [
    ["Focused", 25],
    ["Determined", 25],
    ["Normal", 20],
    ["Skeptic", 15],
    ["Annoyed", 10],
  ],
  Sad: [
    ["Sad", 25],
    ["Worried", 25],
    ["Frustrated", 20],
    ["Normal", 15],
    ["Unimpressed", 10],
  ],
  Excited: [
    ["Excited", 20],
    ["Happy", 25],
    ["Surprised", 20],
    ["Glee", 20],
    ["Normal", 10],
  ],
  Surprised: [
    ["Surprised", 15],
    ["Awe", 25],
    ["Scared", 20],
    ["Excited", 15],
    ["Normal", 15],
    ["Happy", 10],
  ],
};

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
  Sleepy: { offsetX: 0, offsetY: -1, height: 7, width: 20, slopeTop: -0.5, slopeBottom: 0, radiusTop: 2, radiusBottom: 1.5 },
  Suspicious: { offsetX: 0, offsetY: 0, height: 11, width: 20, slopeTop: 0, slopeBottom: 0, radiusTop: 4, radiusBottom: 2 },
  Squint: { offsetX: 3, offsetY: 0, height: 10, width: 10, slopeTop: 0, slopeBottom: 0, radiusTop: 3, radiusBottom: 3 },
  Furious: { offsetX: -1, offsetY: 0, height: 15, width: 20, slopeTop: 0.4, slopeBottom: 0, radiusTop: 1, radiusBottom: 4 },
  Scared: { offsetX: -2, offsetY: 0, height: 20, width: 20, slopeTop: -0.1, slopeBottom: 0, radiusTop: 6, radiusBottom: 4 },
  Awe: { offsetX: 1, offsetY: 0, height: 18, width: 23, slopeTop: -0.1, slopeBottom: 0.1, radiusTop: 6, radiusBottom: 6 },
  Excited: { offsetX: 0, offsetY: -2, height: 24, width: 22, slopeTop: 0.05, slopeBottom: -0.05, radiusTop: 7, radiusBottom: 7 },
  Determined: { offsetX: 0, offsetY: 0, height: 15, width: 20, slopeTop: 0.3, slopeBottom: 0, radiusTop: 2, radiusBottom: 2 },
};

function createDefaultEyesConfig() {
  return {
    layout: {
      eyeY: 24,
      eyeSpacing: 14,
      eyeWidth: 16,
      eyeHeight: 10,
      timeX: 32,
      timeY: 5,
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
      font: "classic_5x7",
      fontSize: 1,
      align: "center",
    },
    style: {
      eyeColor: "#9bdcff",
      timeColor: "#64c8ff",
    },
  };
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
    config.layout.timeX === undefined ||
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
    config.time.showSeconds === undefined ||
    config.time.font === undefined ||
    config.time.fontSize === undefined
  ) {
    return false;
  }
  if (!EYES_TIME_FONT_IDS.has(config.time.font)) {
    return false;
  }
  if (
    config.time.align !== undefined &&
    config.time.align !== "left" &&
    config.time.align !== "center" &&
    config.time.align !== "right"
  ) {
    return false;
  }
  if (
    config.style.eyeColor === undefined ||
    config.style.timeColor === undefined
  ) {
    return false;
  }
  return true;
}

function resolveLegacyEyesTimeAnchorX(config) {
  const font = EYES_TIME_FONT_IDS.has(config?.time?.font)
    ? config.time.font
    : "classic_5x7";
  const fontSize = clamp(Number(config?.time?.fontSize) || 1, 1, 3);
  const previewText = config?.time?.showSeconds ? "12:34:56" : "12:34";
  const legacyX = clamp(Number(config?.layout?.timeX) || 0, 0, 64);
  const textWidth = getClockTextWidth(previewText, font, fontSize);
  return clamp(legacyX + Math.floor(textWidth / 2), 0, 64);
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
    history: [expression],
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

function getPreviewTimeOfDay() {
  const hour = new Date().getHours();
  if (hour < 5) {
    return "deepNight";
  }
  if (hour < 8) {
    return "earlyMorning";
  }
  if (hour < 12) {
    return "morning";
  }
  if (hour < 14) {
    return "noon";
  }
  if (hour < 18) {
    return "afternoon";
  }
  if (hour < 22) {
    return "evening";
  }
  return "night";
}

function minPreviewExpressionInterval(timeOfDay) {
  switch (timeOfDay) {
    case "deepNight":
      return 5800;
    case "earlyMorning":
      return 4200;
    case "noon":
      return 3600;
    case "evening":
      return 3200;
    case "night":
      return 4000;
    case "morning":
    case "afternoon":
    default:
      return 2800;
  }
}

function createPreviewWeights() {
  const weights = {};
  ALL_EXPRESSION_VALUES.forEach((value) => {
    weights[value] = 0.45;
  });
  weights.Sleepy = 0.18;
  weights.Scared = 0.2;
  weights.Furious = 0.2;
  weights.Angry = 0.24;
  return weights;
}

function fillPreviewTimeWeights(timeOfDay, weights) {
  switch (timeOfDay) {
    case "deepNight":
      weights.Sleepy += 18;
      weights.Squint += 11;
      weights.Normal += 8;
      weights.Worried += 7;
      weights.Sad += 6;
      weights.Unimpressed += 4;
      break;
    case "earlyMorning":
      weights.Sleepy += 12;
      weights.Normal += 8;
      weights.Focused += 5;
      weights.Worried += 5;
      weights.Squint += 5;
      weights.Unimpressed += 4;
      weights.Happy += 3;
      weights.Determined += 2;
      break;
    case "morning":
      weights.Normal += 7;
      weights.Focused += 9;
      weights.Happy += 8;
      weights.Determined += 8;
      weights.Excited += 5;
      weights.Glee += 4;
      weights.Surprised += 3;
      weights.Awe += 2;
      weights.Suspicious += 2;
      break;
    case "noon":
      weights.Focused += 7;
      weights.Normal += 6;
      weights.Happy += 5;
      weights.Determined += 4;
      weights.Sleepy += 4;
      weights.Unimpressed += 4;
      weights.Squint += 4;
      weights.Frustrated += 3;
      weights.Annoyed += 2;
      break;
    case "afternoon":
      weights.Focused += 8;
      weights.Determined += 7;
      weights.Happy += 6;
      weights.Normal += 5;
      weights.Excited += 5;
      weights.Skeptic += 4;
      weights.Suspicious += 4;
      weights.Surprised += 3;
      weights.Annoyed += 3;
      weights.Glee += 3;
      break;
    case "evening":
      weights.Happy += 8;
      weights.Glee += 7;
      weights.Excited += 7;
      weights.Surprised += 5;
      weights.Awe += 5;
      weights.Normal += 4;
      weights.Determined += 3;
      weights.Suspicious += 3;
      weights.Focused += 2;
      break;
    case "night":
      weights.Sleepy += 8;
      weights.Normal += 6;
      weights.Squint += 5;
      weights.Unimpressed += 5;
      weights.Worried += 4;
      weights.Sad += 4;
      weights.Happy += 2;
      weights.Awe += 2;
      break;
    default:
      break;
  }
}

function applyPreviewTransitionWeights(currentExpression, weights) {
  const transitions = PREVIEW_TRANSITION_WEIGHTS[currentExpression];
  if (!transitions) {
    return;
  }
  transitions.forEach(([expression, transitionWeight]) => {
    weights[expression] = weights[expression] * 0.6 + transitionWeight * 0.4;
  });
}

function pushPreviewHistory(runtime, expression) {
  runtime.history.push(expression);
  if (runtime.history.length > 5) {
    runtime.history.shift();
  }
}

function weightedPreviewChoice(weights, fallbackExpression) {
  const entries = Object.entries(weights);
  const totalWeight = entries.reduce((sum, [, value]) => sum + value, 0);
  if (totalWeight <= 0) {
    return fallbackExpression;
  }

  let target = Math.random() * totalWeight;
  for (const [expression, value] of entries) {
    target -= value;
    if (target <= 0) {
      return expression;
    }
  }
  return fallbackExpression;
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
    GlxStepper,
    ClockFontPanel,
    ClockTextSettingsCard,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isSending: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx - 112rpx)",
      previewCanvasReady: false,
      previewHidden: false,
      previewZoom: 4,
      previewOffset: { x: 16, y: 16 },
      previewContainerSize: { width: 320, height: 320 },
      previewPixels: new Map(),
      previewTimer: null,
      currentTab: 0,
      tabs: ["表情", "时间", "字体"],
      tabIconNames: ["browse", "time", "text"],
      expressionModeOptions: [
        { label: "自动切换", value: "auto" },
        { label: "指定表情", value: "manual" },
      ],
      eyesConfig: createDefaultEyesConfig(),
      selectedEyesExpression: "Normal",
      previewRuntime: createPreviewRuntime("Normal"),
      expressionOptions: EXPRESSION_OPTIONS,
      timeFontOptions: EYES_TIME_FONT_OPTIONS,
      eyeColorOptions: [
        { name: "浅蓝", hex: "#9bdcff" },
        { name: "冰蓝", hex: "#7fd8ff" },
        { name: "薄荷蓝", hex: "#8ee7f2" },
        { name: "天青", hex: "#68c9ff" },
        { name: "雾蓝", hex: "#8ebcff" },
        { name: "霓虹蓝", hex: "#52b7ff" },
      ],
      timeColorOptions: [
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
        return `自动：${this.previewExpressionLabel}`;
      }
      return `固定：${this.selectedExpressionLabel}`;
    },
    expressionModeValue() {
      return this.eyesConfig.behavior.autoSwitch ? "auto" : "manual";
    },
    expressionModeLabel() {
      return this.eyesConfig.behavior.autoSwitch ? "自动切换" : "指定表情";
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
    spiritTimeSection() {
      return {
        show: this.eyesConfig.time.show,
        fontSize: this.eyesConfig.time.fontSize,
        x: this.eyesConfig.layout.timeX,
        y: this.eyesConfig.layout.timeY,
        color: this.eyesConfig.style.timeColor,
        align: this.eyesConfig.time.align,
      };
    },
    timePreviewText() {
      return this.eyesConfig.time.showSeconds ? "12:34:56" : "12:34";
    },
    timeTextWidth() {
      return getClockTextWidth(
        this.timePreviewText,
        this.eyesConfig.time.font,
        this.eyesConfig.time.fontSize,
      );
    },
    timeTextHeight() {
      return getClockTextHeight(
        this.eyesConfig.time.font,
        this.eyesConfig.time.fontSize,
      );
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
    onToastHide() {
      if (!this.isSending) {
        this.previewHidden = false;
      }
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
              const fitZoom = Math.max(2, Math.floor((data.width * 0.96) / 64));
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
        const nextConfig = createDefaultEyesConfig();
        nextConfig.layout = {
          ...nextConfig.layout,
          ...parsedConfig.layout,
        };
        nextConfig.behavior = {
          ...nextConfig.behavior,
          ...parsedConfig.behavior,
        };
        nextConfig.interaction = {
          ...nextConfig.interaction,
          ...parsedConfig.interaction,
        };
        nextConfig.time = {
          ...nextConfig.time,
          ...parsedConfig.time,
        };
        nextConfig.style.eyeColor = parsedConfig.style.eyeColor;
        nextConfig.style.timeColor = parsedConfig.style.timeColor;
        if (parsedConfig.time.align === undefined) {
          nextConfig.time.align = "center";
          nextConfig.layout.timeX = resolveLegacyEyesTimeAnchorX(nextConfig);
        }
        this.eyesConfig = nextConfig;
        this.normalizeTimeLayout();
      } catch (error) {
        console.error("读取桌面宠物配置失败:", error);
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
        uni.setStorageSync(EYES_EXPRESSION_STORAGE_KEY, "Normal");
        return;
      }
      this.selectedEyesExpression = matched.value;
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
        this.normalizeTimeLayout();
        const ws = this.deviceStore.getWebSocket();
        await ws.setMode("eyes");
        await ws.setEyesConfig(this.buildEyesConfigPayload());
        if (!this.eyesConfig.behavior.autoSwitch) {
          await ws.eyesInteract(`set_expression:${this.selectedEyesExpression}`);
        }
        this.saveEyesConfig();
        this.saveSelectedExpression();
        this.deviceStore.setDeviceMode("eyes", { businessMode: true });
        this.toast.showSuccess("桌面宠物已发送到设备");
      } catch (error) {
        console.error("发送桌面宠物失败:", error);
        this.toast.showError("发送失败：" + error.message);
      } finally {
        this.isSending = false;
      }
    },

    handleExpressionSelect(value) {
      this.selectedEyesExpression = value;
      this.saveSelectedExpression();
      this.previewRuntime.expression = value;
      this.previewRuntime.history = [value];
      this.previewRuntime.lastExpressionAt = Date.now();
      this.renderPreviewFrame();
    },

    handleExpressionModeChange(value) {
      this.eyesConfig.behavior.autoSwitch = value === "auto";
      if (this.eyesConfig.behavior.autoSwitch) {
        this.previewRuntime.expression = this.chooseNextPreviewExpression();
        this.previewRuntime.history = [this.previewRuntime.expression];
        this.previewRuntime.lastExpressionAt = Date.now();
        this.scheduleNextExpression();
        this.previewRuntime.actionExpireAt = 0;
      } else {
        this.previewRuntime.expression = this.selectedEyesExpression;
        this.previewRuntime.history = [this.selectedEyesExpression];
        this.previewRuntime.actionExpireAt = 0;
      }
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleEyeColorChange(value) {
      this.eyesConfig.style.eyeColor = value;
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleTimeShowChange(event) {
      this.eyesConfig.time.show = event.detail.value;
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    toggleTimeShow() {
      this.eyesConfig.time.show = !this.eyesConfig.time.show;
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleSpiritTimeAdjust(field, delta, minValue, maxValue) {
      if (field === "fontSize") {
        this.eyesConfig.time.fontSize = clamp(
          Number(this.eyesConfig.time.fontSize) + delta,
          minValue,
          maxValue,
        );
        this.normalizeTimeLayout();
      }

      if (field === "x") {
        const bounds = this.getSpiritTimeLayoutBounds();
        this.eyesConfig.layout.timeX = clamp(
          Number(this.eyesConfig.layout.timeX) + delta,
          bounds.minAnchorX,
          bounds.maxAnchorX,
        );
      }

      if (field === "y") {
        const bounds = this.getSpiritTimeLayoutBounds();
        this.eyesConfig.layout.timeY = clamp(
          Number(this.eyesConfig.layout.timeY) + delta,
          bounds.minTimeY,
          bounds.maxTimeY,
        );
      }

      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleSpiritTimeAlign(align) {
      this.eyesConfig.time.align = align;
      if (align === "left") {
        this.eyesConfig.layout.timeX = 0;
      } else if (align === "center") {
        this.eyesConfig.layout.timeX = 32;
      } else if (align === "right") {
        this.eyesConfig.layout.timeX = 63;
      }
      this.normalizeTimeLayout();
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleTimeSecondsChange(event) {
      this.eyesConfig.time.showSeconds = event.detail.value;
      this.normalizeTimeLayout();
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    toggleTimeSeconds() {
      this.eyesConfig.time.showSeconds = !this.eyesConfig.time.showSeconds;
      this.normalizeTimeLayout();
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleTimeFontChange(value) {
      this.eyesConfig.time.font = value;
      this.normalizeTimeLayout();
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleTimeFontSizeChange(event) {
      this.eyesConfig.time.fontSize = clamp(
        Number(event.detail.value),
        1,
        3,
      );
      this.normalizeTimeLayout();
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleTimeXChange(event) {
      const bounds = this.getSpiritTimeLayoutBounds();
      this.eyesConfig.layout.timeX = clamp(
        Number(event.detail.value),
        bounds.minAnchorX,
        bounds.maxAnchorX,
      );
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleTimeYChange(event) {
      const bounds = this.getSpiritTimeLayoutBounds();
      this.eyesConfig.layout.timeY = clamp(
        Number(event.detail.value),
        bounds.minTimeY,
        bounds.maxTimeY,
      );
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    handleTimeColorChange(value) {
      this.eyesConfig.style.timeColor = value;
      this.saveEyesConfig();
      this.renderPreviewFrame();
    },

    normalizeTimeLayout() {
      this.eyesConfig.time.fontSize = clamp(
        Number(this.eyesConfig.time.fontSize),
        1,
        3,
      );
      if (
        this.eyesConfig.time.align !== "left" &&
        this.eyesConfig.time.align !== "center" &&
        this.eyesConfig.time.align !== "right"
      ) {
        this.eyesConfig.time.align = "center";
      }
      const bounds = this.getSpiritTimeLayoutBounds();
      this.eyesConfig.layout.timeX = clamp(
        Number(this.eyesConfig.layout.timeX),
        bounds.minAnchorX,
        bounds.maxAnchorX,
      );
      this.eyesConfig.layout.timeY = clamp(
        Number(this.eyesConfig.layout.timeY),
        bounds.minTimeY,
        bounds.maxTimeY,
      );
    },

    getSpiritTimeLayoutBounds() {
      const panelSize = 64;
      const textWidth = Math.max(0, Math.round(this.timeTextWidth));
      const textHeight = Math.max(0, Math.round(this.timeTextHeight));
      const maxStartX = Math.max(0, panelSize - textWidth);
      const maxTimeY = Math.max(0, panelSize - textHeight);

      if (this.eyesConfig.time.align === "center") {
        const anchorOffset = Math.floor(textWidth / 2);
        return {
          minAnchorX: anchorOffset,
          maxAnchorX: maxStartX + anchorOffset,
          minTimeY: 0,
          maxTimeY,
          minStartX: 0,
          maxStartX,
        };
      }

      if (this.eyesConfig.time.align === "right") {
        return {
          minAnchorX: textWidth,
          maxAnchorX: maxStartX + textWidth,
          minTimeY: 0,
          maxTimeY,
          minStartX: 0,
          maxStartX,
        };
      }

      return {
        minAnchorX: 0,
        maxAnchorX: maxStartX,
        minTimeY: 0,
        maxTimeY,
        minStartX: 0,
        maxStartX,
      };
    },

    getSpiritTimeStartX() {
      const bounds = this.getSpiritTimeLayoutBounds();
      const align = this.eyesConfig.time.align;
      const anchorX = Number(this.eyesConfig.layout.timeX);
      if (align === "center") {
        return clamp(
          anchorX - Math.floor(this.timeTextWidth / 2),
          bounds.minStartX,
          bounds.maxStartX,
        );
      }
      if (align === "right") {
        return clamp(
          anchorX - this.timeTextWidth,
          bounds.minStartX,
          bounds.maxStartX,
        );
      }
      return clamp(anchorX, bounds.minStartX, bounds.maxStartX);
    },

    buildEyesConfigPayload() {
      const bounds = this.getSpiritTimeLayoutBounds();
      return {
        layout: {
          eyeY: this.eyesConfig.layout.eyeY,
          eyeSpacing: this.eyesConfig.layout.eyeSpacing,
          eyeWidth: this.eyesConfig.layout.eyeWidth,
          eyeHeight: this.eyesConfig.layout.eyeHeight,
          timeX: this.getSpiritTimeStartX(),
          timeY: clamp(
            Number(this.eyesConfig.layout.timeY),
            bounds.minTimeY,
            bounds.maxTimeY,
          ),
        },
        behavior: {
          autoSwitch: this.eyesConfig.behavior.autoSwitch,
          blinkIntervalMs: this.eyesConfig.behavior.blinkIntervalMs,
          lookIntervalMs: this.eyesConfig.behavior.lookIntervalMs,
          idleMove: this.eyesConfig.behavior.idleMove,
          sleepyAfterMs: this.eyesConfig.behavior.sleepyAfterMs,
        },
        interaction: {
          lookHoldMs: this.eyesConfig.interaction.lookHoldMs,
          moodHoldMs: this.eyesConfig.interaction.moodHoldMs,
        },
        time: {
          show: this.eyesConfig.time.show,
          showSeconds: this.eyesConfig.time.showSeconds,
          font: this.eyesConfig.time.font,
          fontSize: this.eyesConfig.time.fontSize,
        },
        style: {
          eyeColor: this.eyesConfig.style.eyeColor,
          timeColor: this.eyesConfig.style.timeColor,
        },
      };
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
      this.previewRuntime.history = [this.selectedEyesExpression];
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
        45,
        100,
      );
    },

    scheduleNextExpression() {
      this.previewRuntime.nextExpressionAfterMs = jitteredInterval(
        minPreviewExpressionInterval(getPreviewTimeOfDay()),
        72,
        130,
      );
    },

    chooseNextPreviewExpression() {
      const weights = createPreviewWeights();
      fillPreviewTimeWeights(getPreviewTimeOfDay(), weights);
      applyPreviewTransitionWeights(
        this.previewRuntime.expression,
        weights,
      );
      weights[this.previewRuntime.expression] *= 0.12;

      this.previewRuntime.history.slice(-4).forEach((expression, index) => {
        const penalty = 0.05 + index * 0.03;
        weights[expression] *= penalty;
      });

      return weightedPreviewChoice(weights, this.previewRuntime.expression);
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

      if (roll < 10) {
        baseX = randomFloat(-0.18, 0.18);
        baseY = randomFloat(-0.10, 0.10);
      } else if (roll < 42) {
        const side = Math.random() < 0.5 ? -1 : 1;
        baseX = side * randomFloat(0.42, 0.80);
        baseY = randomFloat(-0.18, 0.20);
      } else if (roll < 66) {
        baseX = randomFloat(-0.36, 0.36);
        baseY = randomFloat(-0.62, -0.22);
      } else if (roll < 78) {
        baseX = randomFloat(-0.28, 0.28);
        baseY = randomFloat(0.12, 0.34);
      } else if (roll < 92) {
        const side = Math.random() < 0.5 ? -1 : 1;
        baseX = side * randomFloat(0.32, 0.64);
        baseY = Math.random() < 0.5
          ? randomFloat(-0.42, -0.14)
          : randomFloat(0.08, 0.24);
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

      let asymmetryChance = 30;
      if (
        activeExpression === "Skeptic" ||
        activeExpression === "Worried"
      ) {
        asymmetryChance = 42;
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
        pushPreviewHistory(this.previewRuntime, this.previewRuntime.expression);
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
      }

      if (expression === "Normal") {
        preset.height += trianglePulse(now, 2200, isLeftEye ? 0.18 : 0.62) * 1.3;
      } else if (expression === "Happy" || expression === "Glee") {
        preset.offsetY += trianglePulse(now, 1500, isLeftEye ? 0.24 : 0.52) * 0.7;
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

    drawTime(pixelMap) {
      if (!this.eyesConfig.time.show) {
        return;
      }
      const now = new Date();
      let text = now.toTimeString().slice(0, 5);
      if (this.eyesConfig.time.showSeconds) {
        text = now.toTimeString().slice(0, 8);
      }
      drawClockTextToPixels(
        text,
        this.eyesConfig.layout.timeX,
        this.eyesConfig.layout.timeY,
        this.eyesConfig.style.timeColor,
        pixelMap,
        this.eyesConfig.time.font,
        this.eyesConfig.time.fontSize,
        this.eyesConfig.time.align,
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

.preview-canvas-placeholder {
  background-color: #000000;
}

.preview-title {
  font-size: 24rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.preview-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.tab-panel {
  min-height: 100%;
}

.spirit-section-card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

:deep(.settings-card.glx-panel-card) {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

.inline-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.option-row,
.expression-grid {
  overflow: visible;
  box-sizing: border-box;
  padding-bottom: 4rpx;
}

.expression-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14rpx;
}

.expression-item {
  min-height: 74rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 14rpx 8rpx;
  box-sizing: border-box;
}

.option-btn.glx-feature-option,
.expression-item.glx-feature-option {
  position: relative;
  z-index: 1;
  overflow: visible;
  border-width: var(--nb-border-width-control) !important;
}

.option-btn.glx-feature-option.active,
.expression-item.glx-feature-option.active {
  background: var(--nb-yellow) !important;
  border-color: var(--nb-ink) !important;
  color: var(--nb-ink) !important;
}

.option-btn.glx-feature-option.active .glx-feature-option__label,
.expression-item.glx-feature-option.active .glx-feature-option__label {
  color: var(--nb-ink) !important;
  font-weight: 900 !important;
}

.expression-cn {
  font-size: 23rpx;
  font-weight: 700;
}

</style>
