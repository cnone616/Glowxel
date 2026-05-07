<template>
  <div class="glx-page-shell game-mode-page">
    <section class="game-mode-layout">
      <article class="glx-section-card glx-section-card--stack game-preview-card">
        <div class="game-preview-card__head">
          <div>
            <h1 class="game-page-title">桌面宠物</h1>
            <p class="game-page-meta">预览直接按桌面宠物本地状态机生成，时间、表情、眼睛颜色和互动预览都在这页完成。</p>
          </div>
          <span class="glx-chip" :class="deviceStore.connected ? 'glx-chip--green' : 'glx-chip--yellow'">
            {{ deviceStore.connected ? "已连接" : "未连接" }}
          </span>
        </div>

        <div class="game-preview-stage">
          <DevicePixelBoard :pixels="previewPixels" :grid-visible="true" />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送桌面宠物"
            description="发送期间锁定当前预览快照，等待设备完成桌面宠物配置事务提交。"
          >
            <DevicePixelBoard :pixels="sendingPixels" :grid-visible="true" />
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
            <span class="glx-section-meta">表情 / 时间 / 字体</span>
          </div>
          <DeviceModeTabs v-model="currentTab" :items="tabItems" />
        </article>

        <template v-if="currentTab === 'expression'">
          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">表情模式</h2>
              <span class="glx-section-meta">自动 / 手动</span>
            </div>
            <DeviceModeTabs v-model="expressionMode" :items="EXPRESSION_MODE_OPTIONS" />
          </article>

          <article v-if="expressionMode === 'manual'" class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">指定表情</h2>
              <span class="glx-section-meta">{{ EXPRESSION_OPTIONS.length }} 个表情</span>
            </div>
            <div class="expression-grid">
              <button
                v-for="item in EXPRESSION_OPTIONS"
                :key="item.value"
                type="button"
                class="expression-card"
                :class="{ 'is-active': selectedExpression === item.value }"
                @click="selectedExpression = item.value"
              >
                {{ item.label }}
              </button>
            </div>
          </article>

          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">自动节奏</h2>
              <span class="glx-section-meta">仅自动模式生效</span>
            </div>
            <DeviceModeTabs
              v-model="eyesConfig.behavior.expressionRhythm"
              :items="EXPRESSION_RHYTHM_OPTIONS"
            />
          </article>

          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">颜色</h2>
              <span class="glx-section-meta">眼睛 / 时间</span>
            </div>
            <div class="game-fields">
              <GameModeColorField
                v-model="eyesConfig.style.eyeColor"
                label="眼睛颜色"
                :preset-colors="colorOptions"
              />
              <GameModeColorField
                v-model="eyesConfig.style.timeColor"
                label="时间颜色"
                :preset-colors="colorOptions"
              />
            </div>
          </article>

          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">参数调整</h2>
              <span class="glx-section-meta">眨眼 / 游走 / 灵动幅度</span>
            </div>
            <div class="game-row">
              <span class="game-row__label">眨眼步频 {{ blinkLevel }}</span>
              <DeviceModeStepper v-model="blinkLevel" :min="1" :max="10" />
            </div>
            <div class="game-row">
              <span class="game-row__label">游走步频 {{ lookLevel }}</span>
              <DeviceModeStepper v-model="lookLevel" :min="1" :max="10" />
            </div>
            <div class="game-row">
              <span class="game-row__label">灵动幅度 {{ eyesConfig.behavior.idleMove }}</span>
              <DeviceModeStepper v-model="eyesConfig.behavior.idleMove" :min="1" :max="10" />
            </div>
          </article>

          <article class="glx-section-card glx-section-card--stack">
            <div class="glx-section-head">
              <h2 class="glx-section-title">互动预览</h2>
              <span class="glx-section-meta">只影响本地预览</span>
            </div>
            <div class="game-inline-actions">
              <button type="button" class="glx-button glx-button--ghost" @click="triggerPreviewAction('blink')">眨眼</button>
              <button type="button" class="glx-button glx-button--ghost" @click="triggerPreviewAction('look_left')">看左</button>
              <button type="button" class="glx-button glx-button--ghost" @click="triggerPreviewAction('look_center')">看中</button>
              <button type="button" class="glx-button glx-button--ghost" @click="triggerPreviewAction('look_right')">看右</button>
            </div>
          </article>
        </template>

        <article v-else-if="currentTab === 'time'" class="glx-section-card glx-section-card--stack">
          <ClockTextSettingsSection
            title="时间显示"
            description="桌面宠物的时间布局、颜色和对齐直接对齐 uniapp 当前语义。"
            :section="timeSection"
            :preset-colors="colorOptions"
            :show-font-size="true"
            :show-seconds-control="true"
            :show-seconds="eyesConfig.time.showSeconds"
            :min-font-size="1"
            :max-font-size="3"
            @toggle="toggleTimeShow"
            @toggle-seconds="toggleTimeSeconds"
            @adjust="handleTimeAdjust"
            @set-align="handleTimeAlign"
            @update-color="handleTimeColor"
          />
        </article>

        <article v-else class="glx-section-card glx-section-card--stack">
          <GameModeFontSelector
            title="字体样式"
            description="保留桌面宠物自己的时间字模设置，和发送 payload 共用同一字段。"
            :font-options="EYES_TIME_FONT_OPTIONS"
            :selected-font="eyesConfig.time.font"
            :show-seconds="eyesConfig.time.showSeconds"
            :hour-format="24"
            :show-hour-format="false"
            @select-font="eyesConfig.time.font = $event"
            @set-show-seconds="handleShowSecondsChange"
          />
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref, watch } from "vue";
import DeviceSendingOverlay from "@/components/device/DeviceSendingOverlay.vue";
import ClockTextSettingsSection from "@/components/device/clock/ClockTextSettingsSection.vue";
import DeviceModeStepper from "@/components/device/modes/DeviceModeStepper.vue";
import DeviceModeTabs from "@/components/device/modes/DeviceModeTabs.vue";
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import GameModeColorField from "@/components/device/modes/GameModeColorField.vue";
import GameModeFontSelector from "@/components/device/modes/GameModeFontSelector.vue";
import { useFeedback } from "@/composables/useFeedback.js";
import { useDeviceStore } from "@/stores/device.js";
import { readStorageJson, writeStorageJson } from "@/utils/device-mode-core.js";
import {
  buildEyesConfigPayload,
  buildSpiritPreviewPixels,
  createDefaultLocalPreviewState,
  createSpiritPreviewRuntime,
  EYES_CONFIG_STORAGE_KEY,
  EYES_EXPRESSION_STORAGE_KEY,
  EYES_LOCAL_PREVIEW_STORAGE_KEY,
  EYES_PRESET_COLORS,
  EYES_TIME_FONT_OPTIONS,
  EXPRESSION_MODE_OPTIONS,
  EXPRESSION_OPTIONS,
  EXPRESSION_RHYTHM_OPTIONS,
  normalizeEyesConfig,
  normalizeLocalPreview,
  normalizeSpiritTimeLayout,
  triggerSpiritPreviewAction,
  stepSpiritPreview,
  createDefaultEyesConfig,
} from "@/utils/device-mode-spirit.js";

const tabItems = Object.freeze([
  { value: "expression", label: "表情" },
  { value: "time", label: "时间" },
  { value: "font", label: "字体" },
]);

const colorOptions = EYES_PRESET_COLORS.map((item) => ({
  name: item.label,
  hex: item.value,
}));

const deviceStore = useDeviceStore();
const feedback = useFeedback();

const currentTab = ref("expression");
const eyesConfig = reactive(loadEyesConfig());
const selectedExpression = ref(loadSelectedExpression());
const localPreview = reactive(loadLocalPreview());
const runtime = ref(createSpiritPreviewRuntime(selectedExpression.value));
const previewPixels = ref(new Map());
const isSending = ref(false);
const sendingPixels = ref(new Map());
const blinkLevel = ref(levelFromInterval(eyesConfig.behavior.blinkIntervalMs));
const lookLevel = ref(levelFromInterval(eyesConfig.behavior.lookIntervalMs));

let previewTimerId = null;
let nextBlinkAt = Date.now() + eyesConfig.behavior.blinkIntervalMs;
let nextLookAt = Date.now() + eyesConfig.behavior.lookIntervalMs;

const expressionMode = computed({
  get() {
    return eyesConfig.behavior.autoSwitch ? "auto" : "manual";
  },
  set(value) {
    eyesConfig.behavior.autoSwitch = value === "auto";
    refreshPreview();
  },
});

const timeSection = computed(() => {
  return {
    show: eyesConfig.time.show,
    fontSize: eyesConfig.time.fontSize,
    x: eyesConfig.layout.timeX,
    y: eyesConfig.layout.timeY,
    color: eyesConfig.style.timeColor,
    align: eyesConfig.time.align,
  };
});

watch(
  eyesConfig,
  () => {
    blinkLevel.value = levelFromInterval(eyesConfig.behavior.blinkIntervalMs);
    lookLevel.value = levelFromInterval(eyesConfig.behavior.lookIntervalMs);
    persistSpiritState();
    resetPreviewTimers();
    refreshPreview();
  },
  { deep: true },
);

watch(selectedExpression, () => {
  persistSpiritState();
  refreshPreview();
});

watch(
  localPreview,
  () => {
    persistSpiritState();
    refreshPreview();
  },
  { deep: true },
);

watch(blinkLevel, (value) => {
  eyesConfig.behavior.blinkIntervalMs = intervalFromLevel(value);
});

watch(lookLevel, (value) => {
  eyesConfig.behavior.lookIntervalMs = intervalFromLevel(value) + 500;
});

onMounted(() => {
  deviceStore.init();
  startPreviewLoop();
  refreshPreview();
});

onBeforeUnmount(() => {
  stopPreviewLoop();
});

function loadEyesConfig() {
  return normalizeEyesConfig(readStorageJson(EYES_CONFIG_STORAGE_KEY) || createDefaultEyesConfig());
}

function loadSelectedExpression() {
  const saved = localStorage.getItem(EYES_EXPRESSION_STORAGE_KEY);
  if (EXPRESSION_OPTIONS.some((item) => item.value === saved)) {
    return saved;
  }
  return "Normal";
}

function loadLocalPreview() {
  const saved = readStorageJson(EYES_LOCAL_PREVIEW_STORAGE_KEY);
  if (saved) {
    return normalizeLocalPreview(saved);
  }
  return createDefaultLocalPreviewState();
}

function persistSpiritState() {
  writeStorageJson(EYES_CONFIG_STORAGE_KEY, eyesConfig);
  writeStorageJson(EYES_LOCAL_PREVIEW_STORAGE_KEY, localPreview);
  localStorage.setItem(EYES_EXPRESSION_STORAGE_KEY, selectedExpression.value);
}

function levelFromInterval(intervalMs) {
  const safe = Number.isFinite(Number(intervalMs)) ? Number(intervalMs) : 3200;
  return Math.max(1, Math.min(10, Math.round((5600 - safe) / 400)));
}

function intervalFromLevel(level) {
  return 5600 - Math.max(1, Math.min(10, Number(level))) * 400;
}

function resetPreviewTimers() {
  nextBlinkAt = Date.now() + eyesConfig.behavior.blinkIntervalMs;
  nextLookAt = Date.now() + eyesConfig.behavior.lookIntervalMs;
}

function startPreviewLoop() {
  stopPreviewLoop();
  previewTimerId = window.setInterval(() => {
    const now = Date.now();
    stepSpiritPreview(runtime.value, eyesConfig, selectedExpression.value);

    if (now >= nextBlinkAt) {
      triggerSpiritPreviewAction(runtime.value, "blink");
      nextBlinkAt = now + eyesConfig.behavior.blinkIntervalMs;
    }

    if (now >= nextLookAt) {
      const actions = ["look_left", "look_center", "look_right"];
      const nextAction = actions[Math.floor(Math.random() * actions.length)];
      triggerSpiritPreviewAction(runtime.value, nextAction);
      nextLookAt = now + eyesConfig.behavior.lookIntervalMs;
    }

    refreshPreview();
  }, 120);
}

function stopPreviewLoop() {
  if (previewTimerId !== null) {
    window.clearInterval(previewTimerId);
    previewTimerId = null;
  }
}

function refreshPreview() {
  const cloned = JSON.parse(JSON.stringify(eyesConfig));
  normalizeSpiritTimeLayout(cloned);
  previewPixels.value = buildSpiritPreviewPixels(
    cloned,
    selectedExpression.value,
    localPreview,
    runtime.value,
  );
}

function triggerPreviewAction(action) {
  triggerSpiritPreviewAction(runtime.value, action);
  refreshPreview();
}

function toggleTimeShow() {
  eyesConfig.time.show = !eyesConfig.time.show;
}

function toggleTimeSeconds() {
  eyesConfig.time.showSeconds = !eyesConfig.time.showSeconds;
}

function handleShowSecondsChange(value) {
  eyesConfig.time.showSeconds = value === true;
}

function handleTimeAdjust(fieldKey, delta, min, max) {
  if (fieldKey === "fontSize") {
    const nextValue = Math.max(min, Math.min(max, eyesConfig.time.fontSize + delta));
    eyesConfig.time.fontSize = nextValue;
    return;
  }
  if (fieldKey === "x") {
    eyesConfig.layout.timeX = Math.max(min, Math.min(max, eyesConfig.layout.timeX + delta));
    return;
  }
  eyesConfig.layout.timeY = Math.max(min, Math.min(max, eyesConfig.layout.timeY + delta));
}

function handleTimeAlign(align) {
  eyesConfig.time.align = align;
  if (align === "left") {
    eyesConfig.layout.timeX = 0;
  } else if (align === "center") {
    eyesConfig.layout.timeX = 32;
  } else {
    eyesConfig.layout.timeX = 63;
  }
}

function handleTimeColor(color) {
  eyesConfig.style.timeColor = color;
}

async function handleSend() {
  if (deviceStore.connected !== true) {
    feedback.warning("设备未连接", "请先返回设备控制页建立连接。");
    return;
  }

  isSending.value = true;
  sendingPixels.value = new Map(previewPixels.value);
  feedback.showBlocking("发送桌面宠物", "正在把当前桌面宠物配置发送到设备。");
  try {
    const nextConfig = JSON.parse(JSON.stringify(eyesConfig));
    normalizeSpiritTimeLayout(nextConfig);
    await deviceStore.setEyesConfig(buildEyesConfigPayload(nextConfig));
    persistSpiritState();
    feedback.success("发送成功", "桌面宠物已发送到设备。");
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "桌面宠物发送失败。");
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

.game-fields {
  display: grid;
  gap: 18px;
}

.game-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.game-row__label {
  font-size: 14px;
  font-weight: 800;
}

.game-inline-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.expression-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px;
}

.expression-card {
  min-height: 44px;
  border: 2px solid #000000;
  background: #ffffff;
  font-weight: 700;
  cursor: pointer;
}

.expression-card.is-active {
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

@media (max-width: 720px) {
  .expression-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .game-row {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
