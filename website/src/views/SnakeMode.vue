<template>
  <div class="glx-page-shell game-mode-page">
    <section class="game-mode-layout">
      <article class="glx-section-card glx-section-card--stack game-preview-card">
        <div class="game-preview-card__head">
          <div>
            <h1 class="game-page-title">贪吃蛇</h1>
            <p class="game-page-meta">预览、随机蛇色、字体和发送都按 uniapp 同一组参数驱动。</p>
          </div>
          <span class="glx-chip" :class="deviceStore.connected ? 'glx-chip--green' : 'glx-chip--yellow'">
            {{ deviceStore.connected ? "已连接" : "未连接" }}
          </span>
        </div>

        <div class="game-preview-stage">
          <DevicePixelBoard :pixels="displayPixels" :grid-visible="true" />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送贪吃蛇"
            description="发送期间锁定当前预览快照，等待设备完成贪吃蛇参数事务提交。"
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
            <span class="glx-section-meta">外观 / 参数</span>
          </div>
          <DeviceModeTabs v-model="currentTab" :items="tabItems" />
        </article>

        <article v-if="currentTab === 'appearance'" class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">蛇皮肤</h2>
            <span class="glx-section-meta">纯色 / 渐变 / 斑点</span>
          </div>

          <div class="game-inline-actions">
            <button type="button" class="glx-button glx-button--ghost" @click="randomizeSkinColor">
              换个随机颜色
            </button>
          </div>

          <DeviceModeTabs v-model="config.snakeSkin" :items="snakeSkinOptions" />

          <GameModeFontSelector
            title="字体样式"
            description="沿用设备时钟字模，保持和 uniapp 同一套蛇身时间显示效果。"
            :font-options="fontOptions"
            :selected-font="config.font"
            :show-seconds="config.showSeconds"
            :show-hour-format="false"
            @select-font="handleFontChange"
            @set-show-seconds="handleShowSecondsChange"
          />

          <GameModeColorField
            v-model="config.foodColor"
            label="果子颜色"
            :preset-colors="foodPresetColors"
          />
        </article>

        <article v-else class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">参数</h2>
            <span class="glx-section-meta">速度 / 蛇宽 / 秒钟</span>
          </div>

          <div class="game-row">
            <span class="game-row__label">速度 {{ config.speed }}</span>
            <DeviceModeStepper v-model="config.speed" :min="1" :max="10" />
          </div>

          <div class="game-row">
            <span class="game-row__label">蛇宽 {{ config.snakeWidth }}</span>
            <DeviceModeStepper v-model="config.snakeWidth" :min="2" :max="4" />
          </div>

          <div class="game-row">
            <span class="game-row__label">显示秒钟</span>
            <GlxSwitch :checked="config.showSeconds" @change="handleShowSecondsChange" />
          </div>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, reactive, ref, watch } from "vue";
import DeviceSendingOverlay from "@/components/device/DeviceSendingOverlay.vue";
import { useFeedback } from "@/composables/useFeedback.js";
import { usePixelPreviewPlayer } from "@/composables/usePixelPreviewPlayer.js";
import GameModeColorField from "@/components/device/modes/GameModeColorField.vue";
import GameModeFontSelector from "@/components/device/modes/GameModeFontSelector.vue";
import DeviceModeStepper from "@/components/device/modes/DeviceModeStepper.vue";
import DeviceModeTabs from "@/components/device/modes/DeviceModeTabs.vue";
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import GlxSwitch from "@/components/glx/GlxSwitch.vue";
import { useDeviceStore } from "@/stores/device.js";
import { getDeviceClockFontOptions } from "@/utils/device-clock-core.js";
import { readStorageJson, writeStorageJson } from "@/utils/device-mode-core.js";
import { buildLedMatrixPreviewSequence } from "../../../uniapp/utils/ledMatrixShowcase.js";

const SNAKE_MODE_CONFIG_KEY = "snake_mode_config";
const fontOptions = getDeviceClockFontOptions();
const SNAKE_FONT_IDS = Object.freeze(fontOptions.map((item) => item.id));
const snakeSkinOptions = Object.freeze([
  { value: "solid", label: "纯色" },
  { value: "gradient", label: "渐变" },
  { value: "spotted", label: "斑点" },
]);
const tabItems = Object.freeze([
  { value: "appearance", label: "外观" },
  { value: "params", label: "参数" },
]);
const foodPresetColors = Object.freeze([
  { hex: "#ffa854", name: "#ffa854" },
  { hex: "#ff7f50", name: "#ff7f50" },
  { hex: "#ff5c5c", name: "#ff5c5c" },
  { hex: "#ffd166", name: "#ffd166" },
  { hex: "#c7f464", name: "#c7f464" },
  { hex: "#ffffff", name: "#ffffff" },
]);
const SNAKE_RANDOM_COLOR_MAP = Object.freeze({
  solid: ["#56d678", "#39c46a", "#8bff8a", "#4fd1c5", "#7dd3fc", "#facc15"],
  gradient: ["#ff8a5b", "#ffd166", "#7c9cff", "#fb7185", "#67e8f9", "#86efac"],
  spotted: ["#a3d977", "#d4a373", "#34d399", "#22c55e", "#f59e0b", "#c084fc"],
});

const deviceStore = useDeviceStore();
const feedback = useFeedback();
const { currentPixels, playSequence, snapshot } = usePixelPreviewPlayer();

const currentTab = ref("appearance");
const config = reactive(loadSnakeConfig());
const isSending = ref(false);
const sendingPixels = ref(new Map());

const displayPixels = computed(() => {
  if (isSending.value) {
    return sendingPixels.value;
  }
  return currentPixels.value;
});

watch(
  config,
  () => {
    refreshPreview();
  },
  { deep: true, immediate: true },
);

onMounted(() => {
  deviceStore.init();
});

function createDefaultSnakeModeConfig() {
  return {
    speed: 6,
    snakeWidth: 2,
    snakeColor: "#56d678",
    foodColor: "#ffa854",
    font: "minimal_3x5",
    showSeconds: false,
    snakeSkin: "gradient",
  };
}

function cloneSnakeModeConfig(source) {
  return {
    speed: source.speed,
    snakeWidth: source.snakeWidth,
    snakeColor: source.snakeColor,
    foodColor: source.foodColor,
    font: source.font,
    showSeconds: source.showSeconds,
    snakeSkin: source.snakeSkin,
  };
}

function normalizeSavedSnakeModeConfig(saved) {
  const normalized = createDefaultSnakeModeConfig();
  if (!saved || typeof saved !== "object") {
    return normalized;
  }

  if (Number.isFinite(Number(saved.speed))) {
    normalized.speed = Math.max(1, Math.min(10, Number(saved.speed)));
  }
  if (Number.isFinite(Number(saved.snakeWidth))) {
    normalized.snakeWidth = Math.max(2, Math.min(4, Number(saved.snakeWidth)));
  }
  if (typeof saved.snakeColor === "string") {
    normalized.snakeColor = saved.snakeColor;
  }
  if (typeof saved.foodColor === "string") {
    normalized.foodColor = saved.foodColor;
  }
  if (typeof saved.font === "string" && SNAKE_FONT_IDS.includes(saved.font)) {
    normalized.font = saved.font;
  }
  if (saved.showSeconds === true || saved.showSeconds === false) {
    normalized.showSeconds = saved.showSeconds;
  }
  if (
    saved.snakeSkin === "solid" ||
    saved.snakeSkin === "gradient" ||
    saved.snakeSkin === "spotted"
  ) {
    normalized.snakeSkin = saved.snakeSkin;
  }

  return normalized;
}

function normalizeHexText(value) {
  if (typeof value !== "string") {
    return "";
  }
  return value.trim().toLowerCase();
}

function pickRandomSnakeColorBySkin(snakeSkin, currentColor) {
  const colorPool = SNAKE_RANDOM_COLOR_MAP[snakeSkin];
  if (!Array.isArray(colorPool) || colorPool.length === 0) {
    return currentColor;
  }
  const current = normalizeHexText(currentColor);
  const candidates = colorPool.filter((item) => {
    return item !== current;
  });
  const pool = candidates.length > 0 ? candidates : colorPool;
  return pool[Math.floor(Math.random() * pool.length)];
}

function shouldAvoidSnakeColor(color) {
  return normalizeHexText(color) === "#000000";
}

function resolveVisibleSnakeColor(snakeSkin, snakeColor) {
  if (shouldAvoidSnakeColor(snakeColor)) {
    return pickRandomSnakeColorBySkin(snakeSkin, snakeColor);
  }
  return snakeColor;
}

function loadSnakeConfig() {
  const saved = readStorageJson(SNAKE_MODE_CONFIG_KEY);
  const normalized = normalizeSavedSnakeModeConfig(saved);
  const visibleConfig = cloneSnakeModeConfig(normalized);
  visibleConfig.snakeColor = resolveVisibleSnakeColor(visibleConfig.snakeSkin, visibleConfig.snakeColor);
  return visibleConfig;
}

function buildSnakeConfig() {
  return {
    speed: config.speed,
    snakeWidth: config.snakeWidth,
    snakeColor: config.snakeColor,
    foodColor: config.foodColor,
    font: config.font,
    showSeconds: config.showSeconds,
    snakeSkin: config.snakeSkin,
  };
}

function saveSnakeConfig() {
  writeStorageJson(SNAKE_MODE_CONFIG_KEY, buildSnakeConfig());
}

function refreshPreview() {
  playSequence(
    buildLedMatrixPreviewSequence({
      demoId: "snake",
      speed: config.speed,
      snakeWidth: config.snakeWidth,
      snakeColor: config.snakeColor,
      foodColor: config.foodColor,
      font: config.font,
      showSeconds: config.showSeconds,
      snakeSkin: config.snakeSkin,
    }),
  );
}

function handleFontChange(fontId) {
  config.font = fontId;
}

function handleShowSecondsChange(value) {
  config.showSeconds = value === true;
}

function randomizeSkinColor() {
  config.snakeColor = pickRandomSnakeColorBySkin(config.snakeSkin, config.snakeColor);
}

async function handleSend() {
  if (deviceStore.connected !== true) {
    feedback.warning("设备未连接", "请先返回设备控制页建立连接。");
    return;
  }

  isSending.value = true;
  sendingPixels.value = snapshot();
  feedback.showBlocking("发送贪吃蛇", "正在把当前贪吃蛇参数发送到设备。");
  try {
    const nextConfig = buildSnakeConfig();
    await deviceStore.startSnake(nextConfig);
    saveSnakeConfig();
    feedback.success("发送成功", "贪吃蛇已发送到设备。");
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "贪吃蛇发送失败。");
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

.game-inline-actions {
  display: flex;
  justify-content: flex-start;
}

.game-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  align-items: center;
  gap: 12px;
}

.game-row__label {
  font-size: 14px;
  font-weight: 800;
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
  .game-row {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
