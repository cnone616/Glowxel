<template>
  <div class="glx-page-shell game-mode-page">
    <section class="game-mode-layout">
      <article class="glx-section-card glx-section-card--stack game-preview-card">
        <div class="game-preview-card__head">
          <div>
            <h1 class="game-page-title">俄罗斯方块屏保</h1>
            <p class="game-page-meta">首屏预览直接走本地俄罗斯方块状态机，发送时保持和 uniapp 同一组时钟叠层字段。</p>
          </div>
          <span class="glx-chip" :class="deviceStore.connected ? 'glx-chip--green' : 'glx-chip--yellow'">
            {{ deviceStore.connected ? "已连接" : "未连接" }}
          </span>
        </div>

        <div class="game-preview-stage">
          <DevicePixelBoard :pixels="displayPixels" :grid-visible="true" />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送俄罗斯方块屏保"
            description="发送期间锁定当前预览快照，等待设备完成屏保参数事务提交。"
          >
            <DevicePixelBoard :pixels="sendingPixels" :grid-visible="true" />
          </DeviceSendingOverlay>
        </div>

        <div class="game-preview-actions">
          <button
            type="button"
            class="glx-button glx-button--primary"
            :disabled="isSending"
            @click="handleSend"
          >
            {{ isSending ? "发送中..." : "发送到设备" }}
          </button>
        </div>
      </article>

      <div class="game-mode-stack">
        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">分组</h2>
            <span class="glx-section-meta">屏保 / 时间 / 字体</span>
          </div>
          <DeviceModeTabs v-model="currentTab" :items="tabItems" />
        </article>

        <template v-if="currentTab === 'screensaver'">
          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">模式</h2>
              <span class="glx-section-meta">消除 / 满屏</span>
            </div>
            <div class="game-choice-grid game-choice-grid--double">
              <button
                v-for="item in clearModeOptions"
                :key="item.label"
                type="button"
                class="game-choice-button"
                :class="{ 'is-active': config.clearMode === item.value }"
                @click="config.clearMode = item.value"
              >
                {{ item.label }}
              </button>
            </div>
          </article>

          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">方块大小</h2>
              <span class="glx-section-meta">1px / 2px / 3px</span>
            </div>
            <div class="game-choice-grid game-choice-grid--triple">
              <button
                v-for="item in cellSizeOptions"
                :key="item.label"
                type="button"
                class="game-choice-button"
                :class="{ 'is-active': config.cellSize === item.value }"
                @click="config.cellSize = item.value"
              >
                {{ item.label }}
              </button>
            </div>
          </article>

          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">下落速度</h2>
              <span class="glx-section-meta">慢 / 中 / 快</span>
            </div>
            <div class="game-choice-grid game-choice-grid--triple">
              <button
                v-for="item in speedOptions"
                :key="item.label"
                type="button"
                class="game-choice-button"
                :class="{ 'is-active': config.speed === item.value }"
                @click="config.speed = item.value"
              >
                {{ item.label }}
              </button>
            </div>
          </article>

          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">时间显示</h2>
              <span class="glx-section-meta">显示 / 隐藏</span>
            </div>
            <div class="game-choice-grid game-choice-grid--double">
              <button
                v-for="item in showClockOptions"
                :key="item.label"
                type="button"
                class="game-choice-button"
                :class="{ 'is-active': config.showClock === item.value }"
                @click="config.showClock = item.value"
              >
                {{ item.label }}
              </button>
            </div>
          </article>
        </template>

        <article v-else-if="currentTab === 'time'" class="glx-section-card glx-section-card--stack">
          <ClockTextSettingsSection
            title="时间显示"
            description="时间层控制和 uniapp 保持同一套位置、字号、颜色与对齐语义。"
            :section="effectiveTimeSection"
            :preset-colors="presetColors"
            :show-font-size="true"
            :show-seconds-control="true"
            :show-seconds="clockConfig.showSeconds"
            :min-font-size="1"
            :max-font-size="3"
            @toggle-seconds="toggleShowSeconds"
            @adjust="handleTimeAdjust"
            @set-align="handleTimeAlign"
            @update-color="handleTimeColor"
            @toggle="toggleShowClock"
          />
        </article>

        <article v-else class="glx-section-card glx-section-card--stack">
          <GameModeFontSelector
            title="字体样式"
            description="保留秒钟和 12/24 小时切换，预览与发送统一走当前字宽计算。"
            :font-options="fontOptions"
            :selected-font="clockConfig.font"
            :show-seconds="clockConfig.showSeconds"
            :hour-format="clockConfig.hourFormat"
            :show-hour-format="true"
            @select-font="handleFontChange"
            @set-show-seconds="handleShowSecondsChange"
            @set-hour-format="handleHourFormatChange"
          />
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref, watch } from "vue";
import DeviceSendingOverlay from "@/components/device/DeviceSendingOverlay.vue";
import { useFeedback } from "@/composables/useFeedback.js";
import ClockTextSettingsSection from "@/components/device/clock/ClockTextSettingsSection.vue";
import DeviceModeTabs from "@/components/device/modes/DeviceModeTabs.vue";
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import GameModeFontSelector from "@/components/device/modes/GameModeFontSelector.vue";
import { useDeviceStore } from "@/stores/device.js";
import { getDeviceClockFontOptions } from "@/utils/device-clock-core.js";
import {
  hexToRgb,
  normalizeHexColor,
  readStorageJson,
  writeStorageJson,
} from "@/utils/device-mode-core.js";
import {
  getClockTextWidth,
  getCurrentTimeText,
} from "../../../uniapp/utils/clockCanvas.js";
import {
  createTetrisScreensaverPreviewState,
  renderTetrisScreensaverPreviewState,
  stepTetrisScreensaverPreviewState,
} from "../../../uniapp/utils/tetrisScreensaverPreview.js";

const TETRIS_SPEED_OPTIONS = Object.freeze({
  slow: 300,
  normal: 150,
  fast: 80,
});
const TETRIS_ALL_PIECES = Object.freeze([0, 1, 2, 3, 4, 5, 6]);
const TETRIS_CONFIG_STORAGE_KEY = "tetris_config";
const TETRIS_OVERLAY_CLOCK_CONFIG_STORAGE_KEY = "tetris_overlay_clock_config";
const fontOptions = getDeviceClockFontOptions();
const fontIds = Object.freeze(fontOptions.map((item) => item.id));
const tabItems = Object.freeze([
  { value: "screensaver", label: "屏保" },
  { value: "time", label: "时间" },
  { value: "font", label: "字体" },
]);
const clearModeOptions = Object.freeze([
  { value: true, label: "消除模式" },
  { value: false, label: "满屏模式" },
]);
const cellSizeOptions = Object.freeze([
  { value: 1, label: "小 (1px)" },
  { value: 2, label: "中 (2px)" },
  { value: 3, label: "大 (3px)" },
]);
const speedOptions = Object.freeze([
  { value: "slow", label: "慢" },
  { value: "normal", label: "中" },
  { value: "fast", label: "快" },
]);
const showClockOptions = Object.freeze([
  { value: true, label: "显示时间" },
  { value: false, label: "隐藏时间" },
]);
const presetColors = Object.freeze([
  { name: "青色", hex: "#64c8ff" },
  { name: "绿色", hex: "#00ff9d" },
  { name: "黄色", hex: "#ffdc00" },
  { name: "橙色", hex: "#ffa500" },
  { name: "红色", hex: "#ff6464" },
  { name: "紫色", hex: "#c864ff" },
  { name: "白色", hex: "#ffffff" },
]);

const deviceStore = useDeviceStore();
const feedback = useFeedback();

const currentTab = ref("screensaver");
const config = reactive(loadTetrisConfig());
const clockConfig = reactive(loadTetrisClockConfig());
const currentPreviewMap = ref(new Map());
const previewState = ref(null);
const previewTimerId = ref(null);
const isSending = ref(false);
const sendingPixels = ref(new Map());

const displayPixels = computed(() => {
  if (isSending.value) {
    return sendingPixels.value;
  }
  return currentPreviewMap.value;
});

const effectiveTimeSection = computed(() => {
  return {
    show: config.showClock,
    fontSize: clockConfig.time.fontSize,
    x: clockConfig.time.x,
    y: clockConfig.time.y,
    color: clockConfig.time.color,
    align: clockConfig.time.align,
  };
});

watch(
  config,
  () => {
    refreshPreview();
  },
  { deep: true, immediate: true },
);

watch(
  clockConfig,
  () => {
    refreshPreview();
  },
  { deep: true },
);

onMounted(() => {
  deviceStore.init();
});

onBeforeUnmount(() => {
  stopPreviewPlayback();
});

function cloneAllTetrisPieces() {
  return TETRIS_ALL_PIECES.slice();
}

function createDefaultConfig() {
  return {
    clearMode: true,
    cellSize: 2,
    speed: "normal",
    showClock: true,
    pieces: cloneAllTetrisPieces(),
  };
}

function createDefaultClockConfig() {
  return {
    font: "classic_5x7",
    showSeconds: false,
    hourFormat: 24,
    time: {
      show: true,
      fontSize: 1,
      x: 32,
      y: 2,
      color: "#ffffff",
      align: "center",
    },
    date: {
      show: false,
      fontSize: 1,
      x: 0,
      y: 0,
      color: "#787878",
      align: "left",
    },
    week: {
      show: false,
      x: 0,
      y: 0,
      color: "#646464",
      align: "left",
    },
    image: {
      show: false,
      x: 0,
      y: 0,
      width: 64,
      height: 64,
    },
  };
}

function normalizeSavedConfig(saved) {
  const normalized = createDefaultConfig();
  if (!saved || typeof saved !== "object") {
    return normalized;
  }

  if (typeof saved.clearMode === "boolean") {
    normalized.clearMode = saved.clearMode;
  }
  if (saved.cellSize === 1 || saved.cellSize === 2 || saved.cellSize === 3) {
    normalized.cellSize = saved.cellSize;
  }
  if (saved.speed === "slow" || saved.speed === "normal" || saved.speed === "fast") {
    normalized.speed = saved.speed;
  }
  if (typeof saved.showClock === "boolean") {
    normalized.showClock = saved.showClock;
  }

  return normalized;
}

function normalizeAlign(value, defaultValue) {
  if (value === "left" || value === "center" || value === "right") {
    return value;
  }
  return defaultValue;
}

function normalizeBool(value, defaultValue) {
  if (value === true || value === false) {
    return value;
  }
  return defaultValue;
}

function normalizeInt(value, min, max, defaultValue) {
  if (!Number.isFinite(Number(value))) {
    return defaultValue;
  }
  return Math.max(min, Math.min(max, Math.round(Number(value))));
}

function normalizeColor(value, defaultValue) {
  if (typeof value !== "string") {
    return defaultValue;
  }
  try {
    return normalizeHexColor(value);
  } catch (error) {
    return defaultValue;
  }
}

function normalizeSavedClockConfig(saved) {
  const normalized = createDefaultClockConfig();
  if (!saved || typeof saved !== "object") {
    return normalized;
  }

  if (typeof saved.font === "string" && fontIds.includes(saved.font)) {
    normalized.font = saved.font;
  }
  if (saved.showSeconds === true || saved.showSeconds === false) {
    normalized.showSeconds = saved.showSeconds;
  }
  if (saved.hourFormat === 12 || saved.hourFormat === 24) {
    normalized.hourFormat = saved.hourFormat;
  }

  if (saved.time && typeof saved.time === "object") {
    normalized.time.show = normalizeBool(saved.time.show, normalized.time.show);
    normalized.time.fontSize = normalizeInt(saved.time.fontSize, 1, 3, normalized.time.fontSize);
    normalized.time.x = normalizeInt(saved.time.x, 0, 64, normalized.time.x);
    normalized.time.y = normalizeInt(saved.time.y, 0, 64, normalized.time.y);
    normalized.time.color = normalizeColor(saved.time.color, normalized.time.color);
    normalized.time.align = normalizeAlign(saved.time.align, normalized.time.align);
  }

  if (saved.date && typeof saved.date === "object") {
    normalized.date.show = normalizeBool(saved.date.show, normalized.date.show);
    normalized.date.fontSize = normalizeInt(saved.date.fontSize, 1, 3, normalized.date.fontSize);
    normalized.date.x = normalizeInt(saved.date.x, 0, 64, normalized.date.x);
    normalized.date.y = normalizeInt(saved.date.y, 0, 64, normalized.date.y);
    normalized.date.color = normalizeColor(saved.date.color, normalized.date.color);
    normalized.date.align = normalizeAlign(saved.date.align, normalized.date.align);
  }

  if (saved.week && typeof saved.week === "object") {
    normalized.week.show = normalizeBool(saved.week.show, normalized.week.show);
    normalized.week.x = normalizeInt(saved.week.x, 0, 64, normalized.week.x);
    normalized.week.y = normalizeInt(saved.week.y, 0, 64, normalized.week.y);
    normalized.week.color = normalizeColor(saved.week.color, normalized.week.color);
    normalized.week.align = normalizeAlign(saved.week.align, normalized.week.align);
  }

  if (saved.image && typeof saved.image === "object") {
    normalized.image.show = normalizeBool(saved.image.show, normalized.image.show);
    normalized.image.x = normalizeInt(saved.image.x, 0, 64, normalized.image.x);
    normalized.image.y = normalizeInt(saved.image.y, 0, 64, normalized.image.y);
    normalized.image.width = normalizeInt(saved.image.width, 1, 64, normalized.image.width);
    normalized.image.height = normalizeInt(saved.image.height, 1, 64, normalized.image.height);
  }

  return normalized;
}

function loadTetrisConfig() {
  return normalizeSavedConfig(readStorageJson(TETRIS_CONFIG_STORAGE_KEY));
}

function loadTetrisClockConfig() {
  return normalizeSavedClockConfig(readStorageJson(TETRIS_OVERLAY_CLOCK_CONFIG_STORAGE_KEY));
}

function buildEffectiveClockConfig() {
  return {
    font: clockConfig.font,
    showSeconds: clockConfig.showSeconds,
    hourFormat: clockConfig.hourFormat,
    time: {
      show: config.showClock,
      fontSize: clockConfig.time.fontSize,
      x: clockConfig.time.x,
      y: clockConfig.time.y,
      color: clockConfig.time.color,
      align: clockConfig.time.align,
    },
    date: {
      show: clockConfig.date.show,
      fontSize: clockConfig.date.fontSize,
      x: clockConfig.date.x,
      y: clockConfig.date.y,
      color: clockConfig.date.color,
      align: clockConfig.date.align,
    },
    week: {
      show: clockConfig.week.show,
      x: clockConfig.week.x,
      y: clockConfig.week.y,
      color: clockConfig.week.color,
      align: clockConfig.week.align,
    },
    image: {
      show: clockConfig.image.show,
      x: clockConfig.image.x,
      y: clockConfig.image.y,
      width: clockConfig.image.width,
      height: clockConfig.image.height,
    },
  };
}

function stopPreviewPlayback() {
  if (previewTimerId.value !== null) {
    window.clearTimeout(previewTimerId.value);
    previewTimerId.value = null;
  }
}

function startPreviewPlayback() {
  stopPreviewPlayback();
  if (previewState.value === null) {
    return;
  }

  const playNext = () => {
    if (previewState.value === null) {
      return;
    }
    const delay = Number.isFinite(previewState.value.frameDelay)
      ? previewState.value.frameDelay
      : 110;
    previewTimerId.value = window.setTimeout(() => {
      if (previewState.value === null) {
        return;
      }
      stepTetrisScreensaverPreviewState(previewState.value);
      currentPreviewMap.value = renderTetrisScreensaverPreviewState(previewState.value);
      playNext();
    }, delay);
  };

  playNext();
}

function refreshPreview() {
  const previewConfig = {
    clearMode: config.clearMode,
    cellSize: config.cellSize,
    speed: config.speed,
    showClock: config.showClock,
    pieces: cloneAllTetrisPieces(),
  };
  previewState.value = createTetrisScreensaverPreviewState(
    previewConfig,
    buildEffectiveClockConfig(),
  );
  currentPreviewMap.value = renderTetrisScreensaverPreviewState(previewState.value);
  startPreviewPlayback();
}

function getTimeText(clockSource) {
  return getCurrentTimeText(clockSource.showSeconds, clockSource.hourFormat);
}

function handleFontChange(fontId) {
  clockConfig.font = fontId;
}

function handleShowSecondsChange(value) {
  clockConfig.showSeconds = value === true;
}

function handleHourFormatChange(value) {
  if (value === 12 || value === 24) {
    clockConfig.hourFormat = value;
  }
}

function toggleShowSeconds() {
  clockConfig.showSeconds = !clockConfig.showSeconds;
}

function toggleShowClock() {
  config.showClock = !config.showClock;
}

function handleTimeAdjust(fieldKey, delta, min, max) {
  const nextValue = Math.max(min, Math.min(max, clockConfig.time[fieldKey] + delta));
  clockConfig.time[fieldKey] = nextValue;
}

function handleTimeColor(color) {
  clockConfig.time.color = color;
}

function handleTimeAlign(align) {
  clockConfig.time.align = align;
  if (align === "left") {
    clockConfig.time.x = 0;
  } else if (align === "center") {
    clockConfig.time.x = 32;
  } else if (align === "right") {
    clockConfig.time.x = 63;
  }
}

function buildTetrisClockPayload(effectiveClockConfig) {
  const timeText = getTimeText(effectiveClockConfig);
  let timeX = effectiveClockConfig.time.x;

  if (effectiveClockConfig.time.align === "center") {
    timeX =
      timeX -
      Math.floor(
        getClockTextWidth(
          timeText,
          effectiveClockConfig.font,
          effectiveClockConfig.time.fontSize,
        ) / 2,
      );
  } else if (effectiveClockConfig.time.align === "right") {
    timeX =
      timeX -
      getClockTextWidth(
        timeText,
        effectiveClockConfig.font,
        effectiveClockConfig.time.fontSize,
      );
  }

  return {
    font: effectiveClockConfig.font,
    showSeconds: effectiveClockConfig.showSeconds,
    hourFormat: effectiveClockConfig.hourFormat,
    time: {
      show: effectiveClockConfig.time.show,
      fontSize: effectiveClockConfig.time.fontSize,
      x: timeX,
      y: effectiveClockConfig.time.y,
      color: hexToRgb(effectiveClockConfig.time.color),
    },
    date: {
      show: effectiveClockConfig.date.show,
      fontSize: effectiveClockConfig.date.fontSize,
      x: effectiveClockConfig.date.x,
      y: effectiveClockConfig.date.y,
      color: hexToRgb(effectiveClockConfig.date.color),
    },
    week: {
      show: effectiveClockConfig.week.show,
      x: effectiveClockConfig.week.x,
      y: effectiveClockConfig.week.y,
      color: hexToRgb(effectiveClockConfig.week.color),
    },
    image: {
      show: effectiveClockConfig.image.show,
      x: effectiveClockConfig.image.x,
      y: effectiveClockConfig.image.y,
      width: effectiveClockConfig.image.width,
      height: effectiveClockConfig.image.height,
    },
  };
}

function saveConfigsBeforeSend(nextConfig, nextClockConfig) {
  writeStorageJson(TETRIS_CONFIG_STORAGE_KEY, nextConfig);
  writeStorageJson(TETRIS_OVERLAY_CLOCK_CONFIG_STORAGE_KEY, nextClockConfig);
}

async function handleSend() {
  if (deviceStore.connected !== true) {
    feedback.warning("设备未连接", "请先返回设备控制页建立连接。");
    return;
  }

  const nextConfig = {
    clearMode: config.clearMode,
    cellSize: config.cellSize,
    speed: config.speed,
    showClock: config.showClock,
    pieces: cloneAllTetrisPieces(),
  };
  const nextClockConfig = buildEffectiveClockConfig();
  saveConfigsBeforeSend(nextConfig, nextClockConfig);

  isSending.value = true;
  sendingPixels.value = new Map(currentPreviewMap.value);
  feedback.showBlocking("发送俄罗斯方块", "正在把当前俄罗斯方块屏保发送到设备。");
  try {
    await deviceStore.startTetris({
      clearMode: nextConfig.clearMode,
      cellSize: nextConfig.cellSize,
      speed: TETRIS_SPEED_OPTIONS[nextConfig.speed],
      showClock: nextConfig.showClock,
      pieces: nextConfig.pieces,
      config: buildTetrisClockPayload(nextClockConfig),
    });
    feedback.success("发送成功", "俄罗斯方块屏保已发送到设备。");
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "俄罗斯方块屏保发送失败。");
    }
  } finally {
    feedback.hideBlocking();
    isSending.value = false;
  }
}
</script>

<style scoped>
.game-mode-page {
  gap: 24px;
}

.game-mode-layout {
  display: grid;
  grid-template-columns: minmax(320px, 0.92fr) minmax(0, 1.08fr);
  gap: 24px;
}

.game-preview-card {
  position: sticky;
  top: 88px;
  align-self: start;
}

.game-preview-card__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.game-page-title {
  margin: 0;
  font-size: 28px;
  font-weight: 900;
}

.game-page-meta {
  margin: 8px 0 0;
  color: var(--glx-text-muted);
  font-size: 13px;
  line-height: 1.6;
}

.game-preview-stage {
  position: relative;
  padding: 18px;
  border: 2px solid #000000;
  background: #000000;
}

.game-preview-actions {
  display: flex;
  justify-content: flex-start;
}

.game-mode-stack {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.game-choice-grid {
  display: grid;
  gap: 12px;
}

.game-choice-grid--double {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.game-choice-grid--triple {
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.game-choice-button {
  min-height: 46px;
  padding: 10px 12px;
  border: 2px solid #000000;
  background: #ffffff;
  color: #000000;
  font: inherit;
  font-weight: 800;
  cursor: pointer;
}

.game-choice-button.is-active {
  background: #ffd23f;
}

@media (max-width: 1080px) {
  .game-mode-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .game-preview-card {
    position: static;
  }
}

@media (max-width: 640px) {
  .game-choice-grid--double,
  .game-choice-grid--triple {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
