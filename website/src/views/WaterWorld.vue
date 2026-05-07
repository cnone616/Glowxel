<template>
  <div class="glx-page-shell game-mode-page">
    <section class="game-mode-layout">
      <article class="glx-section-card glx-section-card--stack game-preview-card">
        <div class="game-preview-card__head">
          <div>
            <h1 class="game-page-title">水世界</h1>
            <p class="game-page-meta">海面波浪、深海海流和海底焦散都在网站端先做首屏预览，再按同一组配置发送到设备。</p>
          </div>
          <span class="glx-chip" :class="deviceStore.connected ? 'glx-chip--green' : 'glx-chip--yellow'">
            {{ deviceStore.connected ? "已连接" : "未连接" }}
          </span>
        </div>

        <div class="game-preview-stage">
          <DevicePixelBoard :pixels="currentPreviewPixels" :grid-visible="false" />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送水世界"
            description="发送期间锁定当前预览快照，等待设备完成水世界场景与时钟配置事务提交。"
          >
            <DevicePixelBoard :pixels="sendingPixels" :grid-visible="false" />
          </DeviceSendingOverlay>
        </div>

        <div class="game-preview-actions">
          <button type="button" class="glx-button glx-button--primary" :disabled="isSending" @click="handleSend">
            {{ isSending ? "发送中..." : "发送到设备" }}
          </button>
        </div>
      </article>

      <div class="game-mode-stack">
        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">分组</h2>
            <span class="glx-section-meta">场景 / 时间 / 字体</span>
          </div>
          <DeviceModeTabs v-model="currentTab" :items="tabItems" />
        </article>

        <template v-if="currentTab === 'scene'">
          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">路线</h2>
              <span class="glx-section-meta">{{ WATER_WORLD_OPTIONS.length }} 个水域</span>
            </div>
            <DeviceModeTabs
              v-model="config.preset"
              :items="WATER_WORLD_OPTIONS.map((item) => ({ value: item.preset, label: item.label }))"
            />
          </article>

          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">海水颜色</h2>
              <span class="glx-section-meta">随机海色也会一起缓存</span>
            </div>
            <div class="game-inline-actions">
              <button type="button" class="glx-button glx-button--ghost" @click="randomizeColorTheme">随机海色</button>
            </div>
            <div class="color-theme-grid">
              <button
                v-for="theme in WATER_WORLD_COLOR_THEME_OPTIONS"
                :key="theme.id"
                type="button"
                class="color-theme-card"
                :class="{ 'is-active': colorThemeId === theme.id }"
                @click="colorThemeId = theme.id"
              >
                <div class="color-theme-swatches">
                  <span
                    v-for="swatch in theme.swatches"
                    :key="`${theme.id}-${swatch}`"
                    class="color-theme-dot"
                    :style="{ backgroundColor: swatch }"
                  ></span>
                </div>
                <strong>{{ theme.label }}</strong>
              </button>
            </div>
          </article>
        </template>

        <article v-else-if="currentTab === 'time'" class="glx-section-card glx-section-card--stack">
          <ClockTextSettingsSection
            title="时间显示"
            description="时间层字段继续沿用时钟配置合同，位置、字号、颜色和对齐与 uniapp 同源。"
            :section="timeSection"
            :preset-colors="presetColors"
            :show-font-size="true"
            :show-seconds-control="true"
            :show-seconds="clockConfig.showSeconds"
            :min-font-size="1"
            :max-font-size="3"
            @toggle="toggleTimeShow"
            @toggle-seconds="toggleShowSeconds"
            @adjust="handleTimeAdjust"
            @set-align="handleTimeAlign"
            @update-color="handleTimeColor"
          />
        </article>

        <article v-else class="glx-section-card glx-section-card--stack">
          <GameModeFontSelector
            title="字体样式"
            description="水世界时钟覆盖和站内其它时钟页保持同一套字模与小时制切换。"
            :font-options="fontOptions"
            :selected-font="clockConfig.font"
            :show-seconds="clockConfig.showSeconds"
            :hour-format="clockConfig.hourFormat"
            :show-hour-format="true"
            @select-font="clockConfig.font = $event"
            @set-show-seconds="handleShowSecondsChange"
            @set-hour-format="setHourFormat"
          />
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref, watch } from "vue";
import ClockTextSettingsSection from "@/components/device/clock/ClockTextSettingsSection.vue";
import DeviceSendingOverlay from "@/components/device/DeviceSendingOverlay.vue";
import DeviceModeTabs from "@/components/device/modes/DeviceModeTabs.vue";
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import GameModeFontSelector from "@/components/device/modes/GameModeFontSelector.vue";
import { useFeedback } from "@/composables/useFeedback.js";
import { useDeviceStore } from "@/stores/device.js";
import { buildDeviceClockPayload, getDeviceClockFontOptions } from "@/utils/device-clock-core.js";
import {
  buildWaterWorldPreviewPixels,
  buildWaterWorldSendPlan,
  createDefaultWaterWorldClockConfig,
  createDefaultWaterWorldConfig,
  createWaterWorldPreviewState,
  DEFAULT_WATER_WORLD_COLOR_THEME_ID,
  normalizeWaterWorldClockConfig,
  normalizeWaterWorldConfig,
  pickRandomWaterWorldColorThemeId,
  stepWaterWorldPreviewState,
  WATER_WORLD_CLOCK_CONFIG_KEY,
  WATER_WORLD_COLOR_THEME_OPTIONS,
  WATER_WORLD_CONFIG_KEY,
  WATER_WORLD_OPTIONS,
  WATER_WORLD_PRESET_COLORS,
} from "@/utils/device-mode-water-world.js";
import { readStorageJson, writeStorageJson } from "@/utils/device-mode-core.js";

const WATER_WORLD_THEME_KEY = "water_world_preview_theme_id";

const tabItems = Object.freeze([
  { value: "scene", label: "场景" },
  { value: "time", label: "时间" },
  { value: "font", label: "字体" },
]);

const presetColors = WATER_WORLD_PRESET_COLORS.map((item) => ({
  name: item.label,
  hex: item.value,
}));

const fontOptions = getDeviceClockFontOptions();
const deviceStore = useDeviceStore();
const feedback = useFeedback();

const currentTab = ref("scene");
const config = reactive(loadWaterConfig());
const clockConfig = reactive(loadWaterClockConfig());
const colorThemeId = ref(loadColorThemeId());
const previewState = ref(createWaterWorldPreviewState(config, colorThemeId.value));
const currentPreviewPixels = ref(new Map());
const isSending = ref(false);
const sendingPixels = ref(new Map());
let previewTimerId = null;

const timeSection = computed(() => {
  return {
    show: clockConfig.time.show,
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
    persistState();
    rebuildPreview();
  },
  { deep: true },
);

watch(
  clockConfig,
  () => {
    persistState();
    rebuildPreview();
  },
  { deep: true },
);

watch(colorThemeId, () => {
  persistState();
  rebuildPreview();
});

onMounted(() => {
  deviceStore.init();
  rebuildPreview();
  startPreviewLoop();
});

onBeforeUnmount(() => {
  stopPreviewLoop();
});

function loadWaterConfig() {
  return normalizeWaterWorldConfig(readStorageJson(WATER_WORLD_CONFIG_KEY) || createDefaultWaterWorldConfig());
}

function loadWaterClockConfig() {
  return normalizeWaterWorldClockConfig(readStorageJson(WATER_WORLD_CLOCK_CONFIG_KEY) || createDefaultWaterWorldClockConfig());
}

function loadColorThemeId() {
  const saved = localStorage.getItem(WATER_WORLD_THEME_KEY);
  if (WATER_WORLD_COLOR_THEME_OPTIONS.some((item) => item.id === saved)) {
    return saved;
  }
  return DEFAULT_WATER_WORLD_COLOR_THEME_ID;
}

function persistState() {
  writeStorageJson(WATER_WORLD_CONFIG_KEY, config);
  writeStorageJson(WATER_WORLD_CLOCK_CONFIG_KEY, clockConfig);
  localStorage.setItem(WATER_WORLD_THEME_KEY, colorThemeId.value);
}

function rebuildPreview() {
  previewState.value = createWaterWorldPreviewState(config, colorThemeId.value);
  currentPreviewPixels.value = buildWaterWorldPreviewPixels(previewState.value, clockConfig);
}

function startPreviewLoop() {
  stopPreviewLoop();
  previewTimerId = window.setInterval(() => {
    stepWaterWorldPreviewState(previewState.value);
    currentPreviewPixels.value = buildWaterWorldPreviewPixels(previewState.value, clockConfig);
  }, 120);
}

function stopPreviewLoop() {
  if (previewTimerId !== null) {
    window.clearInterval(previewTimerId);
    previewTimerId = null;
  }
}

function randomizeColorTheme() {
  colorThemeId.value = pickRandomWaterWorldColorThemeId(colorThemeId.value);
}

function toggleTimeShow() {
  clockConfig.time.show = !clockConfig.time.show;
}

function toggleShowSeconds() {
  clockConfig.showSeconds = !clockConfig.showSeconds;
}

function handleShowSecondsChange(value) {
  clockConfig.showSeconds = value === true;
}

function setHourFormat(value) {
  if (value === 12 || value === 24) {
    clockConfig.hourFormat = value;
  }
}

function handleTimeAdjust(fieldKey, delta, min, max) {
  const nextValue = Math.max(min, Math.min(max, clockConfig.time[fieldKey] + delta));
  clockConfig.time[fieldKey] = nextValue;
}

function handleTimeAlign(align) {
  clockConfig.time.align = align;
  if (align === "left") {
    clockConfig.time.x = 0;
  } else if (align === "center") {
    clockConfig.time.x = 32;
  } else {
    clockConfig.time.x = 63;
  }
}

function handleTimeColor(color) {
  clockConfig.time.color = color;
}

async function handleSend() {
  if (deviceStore.connected !== true) {
    feedback.warning("设备未连接", "请先返回设备控制页建立连接。");
    return;
  }

  isSending.value = true;
  sendingPixels.value = new Map(currentPreviewPixels.value);
  feedback.showBlocking("发送水世界", "正在把当前水世界配置发送到设备。");
  try {
    const sendPlan = buildWaterWorldSendPlan(config.preset);
    await deviceStore.setAmbientEffect(
      {
        preset: sendPlan.command.preset,
        speed: sendPlan.command.speed,
        loop: sendPlan.command.loop,
      },
      { clockConfig: buildDeviceClockPayload(clockConfig, new Date()) },
    );
    persistState();
    feedback.success("发送成功", "水世界已发送到设备。");
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "水世界发送失败。");
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
  flex-wrap: wrap;
  gap: 10px;
}

.color-theme-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}

.color-theme-card {
  display: flex;
  flex-direction: column;
  gap: 10px;
  padding: 12px;
  border: 2px solid #000000;
  background: #ffffff;
  cursor: pointer;
  text-align: left;
}

.color-theme-card.is-active {
  background: #ffd23f;
}

.color-theme-swatches {
  display: flex;
  gap: 8px;
}

.color-theme-dot {
  width: 18px;
  height: 18px;
  border: 2px solid #000000;
}

@media (max-width: 1080px) {
  .game-mode-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .game-preview-card {
    position: static;
  }
}

@media (max-width: 720px) {
  .color-theme-grid {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
