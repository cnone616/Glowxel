<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Theme Clock</span>
      <h1 class="glx-page-shell__title">主题模式</h1>
      <p class="glx-page-shell__desc">
        大预览沿用 uniapp `theme-clock` 的“主题主画面”语义，不切成设备实时帧主视图；网站端负责本地生成预览、管理主题选择和说明边界。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">选中主题</span>
          <strong class="glx-hero-metric__value">{{ activePresetName }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前设备主题</span>
          <strong class="glx-hero-metric__value">{{ currentDeviceThemeName }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">预览来源</span>
          <strong class="glx-hero-metric__value">{{ activePreviewImage.length > 0 ? "主题资源图" : "本地生成" }}</strong>
        </article>
      </div>
    </section>

    <section class="clock-theme-layout">
      <article class="glx-section-card glx-section-card--stack clock-theme-preview-card">
        <div class="clock-theme-preview-card__head">
          <div>
            <h2 class="glx-section-title">主题展示</h2>
            <p class="glx-page-shell__desc">这是网站端生成的主题主预览，不会被设备实时帧覆盖。</p>
          </div>
          <span v-if="activePreset" class="glx-chip glx-chip--yellow">{{ activePreset.styleTag }}</span>
        </div>

        <div class="clock-theme-stage">
          <img
            v-if="activePreviewImage.length > 0"
            :src="activePreviewImage"
            :alt="activePresetName"
            class="clock-theme-stage__image"
          />
          <ClockPixelCanvas v-else :frame="previewFrame" rounded />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送主题模式"
            description="发送期间锁定当前主题快照，等待设备完成主题模式事务提交。"
          >
            <img
              v-if="sendingPreviewImage.length > 0"
              :src="sendingPreviewImage"
              :alt="activePresetName"
              class="clock-theme-stage__image"
            />
            <ClockPixelCanvas v-else :frame="sendingFrame" rounded />
          </DeviceSendingOverlay>
        </div>

        <div class="clock-theme-facts">
          <div class="clock-theme-facts__item">
            <span>字体</span>
            <strong>{{ activePreset?.config.font || "--" }}</strong>
          </div>
          <div class="clock-theme-facts__item">
            <span>小时制</span>
            <strong>{{ activePreset?.config.hourFormat === 12 ? "12h" : "24h" }}</strong>
          </div>
          <div class="clock-theme-facts__item">
            <span>强调色</span>
            <strong :style="{ color: activePreset?.accentColor || '#000000' }">{{ activePreset?.accentColor || "--" }}</strong>
          </div>
        </div>

        <div class="glx-inline-actions">
          <button type="button" class="glx-button glx-button--primary" :disabled="isSending" @click="handleSend">
            {{ isSending ? "发送中..." : "发送到设备" }}
          </button>
          <button type="button" class="glx-button glx-button--ghost" @click="resetTheme">恢复默认主题</button>
        </div>

        <p class="clock-inline-note">
          主题发送现在走和 `uniapp` 一致的事务链：`themeId + clock config` 一起下发，预览区继续保持主题主画面语义。
        </p>
      </article>

      <div class="clock-theme-stack">
        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">主题库</h2>
            <span class="glx-section-meta">{{ presets.length }} 个主题</span>
          </div>
          <ClockThemePresetGrid
            :presets="presets"
            :selected-theme-id="selectedThemeId"
            :current-theme-id="currentDeviceThemeId"
            @select-theme="applyTheme"
          />
        </article>

        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">当前主题说明</h2>
            <span class="glx-section-meta">uniapp 对齐</span>
          </div>
          <div v-if="activePreset" class="clock-theme-copy">
            <strong>{{ activePreset.name }}</strong>
            <p>{{ activePreset.description }}</p>
            <p>当前网站端使用与 uniapp 一致的 preset 配置和字模宽度，主差异只剩设备发送链与状态回显。</p>
          </div>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { useDeviceStore } from "@/stores/device.js";
import { useFeedback } from "@/composables/useFeedback.js";
import DeviceSendingOverlay from "@/components/device/DeviceSendingOverlay.vue";
import { buildDeviceClockPayload, renderDeviceClockFrame } from "@/utils/device-clock-core.js";
import ClockPixelCanvas from "./ClockPixelCanvas.vue";
import ClockThemePresetGrid from "./ClockThemePresetGrid.vue";
import {
  applyWebsiteClockThemePreset,
  findWebsiteClockThemePreset,
  getWebsiteClockThemePresets,
} from "@/utils/device-clock-presets.js";

const deviceStore = useDeviceStore();
const feedback = useFeedback();
const presets = getWebsiteClockThemePresets();
const selectedThemeId = ref(presets[0]?.id || "");
const previewNow = ref(new Date());
const isSending = ref(false);
const sendingPreviewImage = ref("");
const sendingFrame = ref(renderDeviceClockFrame());
let previewTimer = null;

const activePreset = computed(() => {
  return findWebsiteClockThemePreset(selectedThemeId.value);
});

const activePresetName = computed(() => {
  if (activePreset.value === null) {
    return "--";
  }
  return activePreset.value.name;
});

const activePreviewImage = computed(() => {
  if (activePreset.value === null) {
    return "";
  }
  if (typeof activePreset.value.previewImage !== "string") {
    return "";
  }
  return activePreset.value.previewImage;
});

const currentDeviceThemeId = computed(() => {
  if (!deviceStore.connected) {
    return "";
  }
  if (deviceStore.deviceMode !== "theme") {
    return "";
  }
  const savedThemeId = localStorage.getItem("clock_device_theme_id");
  if (typeof savedThemeId !== "string") {
    return "";
  }
  return savedThemeId;
});

const currentDeviceThemeName = computed(() => {
  if (currentDeviceThemeId.value.length === 0) {
    return "--";
  }
  const preset = findWebsiteClockThemePreset(currentDeviceThemeId.value);
  if (preset === null) {
    return currentDeviceThemeId.value;
  }
  return preset.name;
});

const previewFrame = computed(() => {
  if (!activePreset.value) {
    return renderDeviceClockFrame();
  }
  return renderDeviceClockFrame({
    config: activePreset.value.config,
    now: previewNow.value,
    themeId: activePreset.value.id,
  });
});

function applyTheme(themeId) {
  selectedThemeId.value = themeId;
  localStorage.setItem("clock_device_theme_id", themeId);
  localStorage.setItem(
    "clock_config_theme",
    JSON.stringify({
      config: applyWebsiteClockThemePreset(activePreset.value?.config || presets[0].config, themeId),
      themeId,
    }),
  );
}

function resetTheme() {
  if (presets[0]) {
    selectedThemeId.value = presets[0].id;
  }
}

function handleSend() {
  if (isSending.value) {
    return;
  }

  if (!deviceStore.connected) {
    feedback.warning("设备未连接", "先去设备控制页建立 WebSocket，再从这里发送主题模式。");
    return;
  }

  if (activePreset.value === null) {
    feedback.error("发送失败", "请先选择主题。");
    return;
  }

  isSending.value = true;
  sendingPreviewImage.value = activePreviewImage.value;
  sendingFrame.value = previewFrame.value;
  feedback.showBlocking("发送主题", "正在把主题模式和主题配置发送到设备。");
  deviceStore
    .applyThemeMode(
      buildDeviceClockPayload(activePreset.value.config, previewNow.value),
      activePreset.value.id,
    )
    .then(() => {
      feedback.success("发送成功", `${activePreset.value.name} 已经发送到设备。`);
    })
    .catch((error) => {
      if (error instanceof Error) {
        feedback.error("发送失败", error.message);
        return;
      }
      feedback.error("发送失败", "主题模式发送失败。");
    })
    .finally(() => {
      isSending.value = false;
      feedback.hideBlocking();
    });
}

onMounted(() => {
  deviceStore.init();
  const savedThemeId = localStorage.getItem("clock_device_theme_id");
  if (savedThemeId && presets.some((preset) => preset.id === savedThemeId)) {
    selectedThemeId.value = savedThemeId;
  }
  previewTimer = window.setInterval(() => {
    previewNow.value = new Date();
  }, 1000);
});

onBeforeUnmount(() => {
  if (previewTimer) {
    window.clearInterval(previewTimer);
    previewTimer = null;
  }
});
</script>

<style scoped>
.clock-theme-layout {
  display: grid;
  grid-template-columns: minmax(340px, 0.92fr) minmax(0, 1.08fr);
  gap: 24px;
}

.clock-theme-preview-card {
  position: sticky;
  top: 88px;
  align-self: start;
}

.clock-theme-preview-card__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.clock-theme-stage {
  position: relative;
  padding: 20px;
  border: 2px solid #000000;
  background:
    radial-gradient(circle at top, rgba(255, 255, 255, 0.1), transparent 52%),
    linear-gradient(180deg, #15181c 0%, #0b0d11 100%);
  overflow: hidden;
}

.clock-theme-stage__image {
  width: 100%;
  aspect-ratio: 1;
  display: block;
  object-fit: contain;
  background: #000000;
  image-rendering: pixelated;
}

.clock-theme-facts {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}

.clock-theme-facts__item {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 12px;
  border: 2px solid #000000;
  background: #ffffff;
}

.clock-theme-facts__item span {
  font-size: 12px;
  color: var(--glx-text-muted);
}

.clock-inline-note {
  margin: 0;
  font-size: 13px;
  line-height: 1.6;
  color: var(--glx-text-muted);
}

.clock-theme-stack {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.clock-theme-copy {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.clock-theme-copy p {
  margin: 0;
  font-size: 14px;
  line-height: 1.6;
}

@media (max-width: 1080px) {
  .clock-theme-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .clock-theme-preview-card {
    position: static;
  }
}

@media (max-width: 640px) {
  .clock-theme-facts {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
