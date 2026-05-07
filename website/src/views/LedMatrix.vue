<template>
  <div class="glx-page-shell device-mode-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">LED Matrix</span>
      <h1 class="glx-page-shell__title">矩阵流光</h1>
      <p class="glx-page-shell__desc">
        这里完整承接 `uniapp/led-matrix` 的板载场景控制：场景选择、本地预览、参数调节和设备发送都走同一条 store 发送链。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前场景</span>
          <strong class="glx-hero-metric__value">{{ selectedDemoLabel }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">连接状态</span>
          <strong class="glx-hero-metric__value">{{ deviceStore.connected ? "已连接" : "未连接" }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">发送模式</span>
          <strong class="glx-hero-metric__value">板载原生</strong>
        </article>
      </div>
    </section>

    <section class="device-mode-layout">
      <article class="glx-section-card glx-section-card--stack device-preview-card">
        <div class="device-preview-card__head">
          <div>
            <h2 class="glx-section-title">预览效果</h2>
            <p class="device-preview-card__desc">黑底预览舞台与设备发送参数同源，刷新后保留上次选择。</p>
          </div>
          <span class="glx-chip glx-chip--blue">{{ selectedDemoLabel }}</span>
        </div>

        <div class="device-preview-stage">
          <DevicePixelBoard :pixels="currentPixels" :grid-visible="true" />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送矩阵流光"
            description="发送期间锁定当前预览快照，等待设备完成板载场景配置切换。"
          >
            <DevicePixelBoard :pixels="sendingPixels" :grid-visible="true" />
          </DeviceSendingOverlay>
        </div>

        <div class="glx-inline-actions">
          <button type="button" class="glx-button glx-button--primary" :disabled="isSending" @click="handleSend">
            {{ isSending ? "发送中..." : "发送到设备" }}
          </button>
          <button type="button" class="glx-button glx-button--ghost" @click="refreshPreview">刷新预览</button>
        </div>
      </article>

      <div class="device-mode-stack">
        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">场景选择</h2>
            <span class="glx-section-meta">{{ demos.length }} 个场景</span>
          </div>
          <div class="mode-grid">
            <button
              v-for="item in demos"
              :key="item.id"
              type="button"
              class="mode-grid__item"
              :class="{ 'is-active': config.demoId === item.id }"
              @click="config.demoId = item.id"
            >
              <strong>{{ item.label }}</strong>
              <span>{{ item.category }}</span>
            </button>
          </div>
        </article>

        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">参数</h2>
            <span class="glx-section-meta">与 uniapp 同步</span>
          </div>

          <div v-if="showSpeedControl" class="mode-row">
            <span class="mode-row__label">速度</span>
            <DeviceModeStepper v-model="config.speed" :min="1" :max="10" />
          </div>

          <div v-if="showIntensityControl" class="mode-row">
            <span class="mode-row__label">强度</span>
            <DeviceModeStepper v-model="config.intensity" :min="10" :max="100" />
          </div>

          <div v-if="showDensityControl" class="mode-row">
            <span class="mode-row__label">密度</span>
            <DeviceModeStepper v-model="config.density" :min="10" :max="100" />
          </div>

          <div v-if="showColorControl" class="mode-block">
            <span class="mode-row__label">雨滴颜色</span>
            <DeviceColorSwatches v-model="config.color" :items="rainColorOptions" />
          </div>

          <p v-if="hasParameterControl === false" class="mode-note">
            这个场景没有额外参数，直接预览后发送即可。
          </p>
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
import DeviceColorSwatches from "@/components/device/modes/DeviceColorSwatches.vue";
import DeviceModeStepper from "@/components/device/modes/DeviceModeStepper.vue";
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import { useDeviceStore } from "@/stores/device.js";
import { readStorageJson, writeStorageJson } from "@/utils/device-mode-core.js";
import {
  buildLedMatrixPreviewSequence,
  getLedMatrixDemoItems,
} from "../../../uniapp/utils/ledMatrixShowcase.js";

const STORAGE_KEY = "led_matrix_showcase_config";
const SPEED_ENABLED_IDS = Object.freeze([
  "starfield",
  "digital_rain",
  "neon_tunnel",
  "boids",
  "falling_sand",
  "rain",
  "sparks",
  "reaction_diffusion",
]);
const INTENSITY_ENABLED_IDS = Object.freeze([
  "boids",
  "falling_sand",
  "sparks",
  "game_of_life",
  "julia_set",
  "reaction_diffusion",
  "wave_pattern",
  "watermelon_plasma",
]);
const demos = getLedMatrixDemoItems();
const rainColorOptions = Object.freeze([
  { label: "青色", value: "#64c8ff" },
  { label: "浅青", value: "#89dcff" },
  { label: "冰蓝", value: "#36cfff" },
  { label: "亮蓝", value: "#4f7fff" },
  { label: "薄荷", value: "#8ee7f2" },
  { label: "天蓝", value: "#7fd8ff" },
]);

const savedConfig = readStorageJson(STORAGE_KEY);
const config = reactive({
  demoId: typeof savedConfig?.demoId === "string" ? savedConfig.demoId : "starfield",
  speed: Number.isFinite(Number(savedConfig?.speed)) ? Math.round(Number(savedConfig.speed)) : 6,
  intensity: Number.isFinite(Number(savedConfig?.intensity)) ? Math.round(Number(savedConfig.intensity)) : 72,
  density: Number.isFinite(Number(savedConfig?.density)) ? Math.round(Number(savedConfig.density)) : 72,
  color: typeof savedConfig?.color === "string" ? savedConfig.color : "#64c8ff",
});

const deviceStore = useDeviceStore();
const feedback = useFeedback();
const { currentPixels, playSequence, snapshot } = usePixelPreviewPlayer();
const isSending = ref(false);
const sendingPixels = ref(new Map());

const selectedDemo = computed(() => {
  return demos.find((item) => item.id === config.demoId) ?? null;
});

const selectedDemoLabel = computed(() => {
  return selectedDemo.value === null ? "--" : selectedDemo.value.label;
});

const showSpeedControl = computed(() => {
  if (selectedDemo.value === null) {
    return false;
  }
  return SPEED_ENABLED_IDS.includes(selectedDemo.value.id);
});

const showIntensityControl = computed(() => {
  if (selectedDemo.value === null) {
    return false;
  }
  return INTENSITY_ENABLED_IDS.includes(selectedDemo.value.id);
});

const showDensityControl = computed(() => {
  return selectedDemo.value !== null && selectedDemo.value.id === "rain";
});

const showColorControl = computed(() => {
  return selectedDemo.value !== null && selectedDemo.value.id === "rain";
});

const hasParameterControl = computed(() => {
  return showSpeedControl.value || showIntensityControl.value || showDensityControl.value || showColorControl.value;
});

function saveConfig() {
  writeStorageJson(STORAGE_KEY, {
    demoId: config.demoId,
    speed: config.speed,
    intensity: config.intensity,
    density: config.density,
    color: config.color,
  });
}

function refreshPreview() {
  playSequence(
    buildLedMatrixPreviewSequence({
      demoId: config.demoId,
      speed: config.speed,
      intensity: config.intensity,
      density: config.density,
      color: config.color,
    }),
  );
}

async function handleSend() {
  if (isSending.value) {
    return;
  }

  if (!deviceStore.connected) {
    feedback.warning("设备未连接", "请先返回设备控制页建立连接。");
    return;
  }
  if (selectedDemo.value === null) {
    feedback.error("发送失败", "请先选择场景。");
    return;
  }

  isSending.value = true;
  sendingPixels.value = snapshot();
  feedback.showBlocking("发送场景", `正在把 ${selectedDemo.value.label} 发送到设备。`);
  try {
    if (selectedDemo.value.id === "rain") {
      await deviceStore.setAmbientEffect({
        preset: selectedDemo.value.ambientPreset,
        speed: config.speed,
        density: config.density,
        color: config.color,
        loop: true,
      });
    } else {
      await deviceStore.setAmbientEffect({
        preset: selectedDemo.value.ambientPreset,
        speed: config.speed,
        intensity: config.intensity,
        loop: true,
      });
    }
    feedback.success("发送成功", `${selectedDemo.value.label} 已经发送到设备。`);
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "矩阵流光发送失败。");
    }
  } finally {
    isSending.value = false;
    feedback.hideBlocking();
  }
}

watch(
  () => ({ ...config }),
  () => {
    saveConfig();
    refreshPreview();
  },
  { deep: true },
);

onMounted(() => {
  deviceStore.init();
  refreshPreview();
});
</script>

<style scoped>
.device-mode-page {
  gap: 24px;
}

.device-mode-layout {
  display: grid;
  grid-template-columns: minmax(320px, 0.92fr) minmax(0, 1.08fr);
  gap: 24px;
}

.device-preview-card {
  position: sticky;
  top: 88px;
  align-self: start;
}

.device-preview-card__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.device-preview-card__desc,
.mode-note {
  margin: 6px 0 0;
  color: var(--glx-text-muted);
  font-size: 13px;
  line-height: 1.6;
}

.device-preview-stage {
  position: relative;
  padding: 18px;
  border: 2px solid #000000;
  background: #000000;
}

.device-mode-stack {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.mode-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.mode-grid__item {
  min-height: 96px;
  padding: 14px;
  display: grid;
  gap: 6px;
  border: 2px solid #000000;
  background: #ffffff;
  text-align: left;
  cursor: pointer;
}

.mode-grid__item.is-active {
  background: #ffd23f;
}

.mode-grid__item strong {
  font-size: 14px;
}

.mode-grid__item span {
  color: var(--glx-text-muted);
  font-size: 12px;
}

.mode-row,
.mode-block {
  display: grid;
  gap: 10px;
}

.mode-row {
  grid-template-columns: minmax(0, 1fr) auto;
  align-items: center;
}

.mode-row__label {
  font-size: 14px;
  font-weight: 800;
  color: #111111;
}

@media (max-width: 1080px) {
  .device-mode-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .device-preview-card {
    position: static;
  }
}

@media (max-width: 720px) {
  .mode-grid {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
