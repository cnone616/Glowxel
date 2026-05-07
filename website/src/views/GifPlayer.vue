<template>
  <div class="glx-page-shell device-mode-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">GIF Player</span>
      <h1 class="glx-page-shell__title">GIF 播放器</h1>
      <p class="glx-page-shell__desc">
        网页端复刻 `uniapp/gif-player` 的离线素材播放器，当前提供内置场景预览、参数调整和 GIF 动画事务发送。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前素材</span>
          <strong class="glx-hero-metric__value">{{ selectedSceneLabel }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">速度</span>
          <strong class="glx-hero-metric__value">{{ config.speed }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">强度</span>
          <strong class="glx-hero-metric__value">{{ config.intensity }}</strong>
        </article>
      </div>
    </section>

    <section class="device-mode-layout">
      <article class="glx-section-card glx-section-card--stack device-preview-card">
        <div class="device-preview-card__head">
          <div>
            <h2 class="glx-section-title">预览效果</h2>
            <p class="device-preview-card__desc">内置场景预览和 GIF 二进制发送使用同一套帧数据。</p>
          </div>
          <span class="glx-chip glx-chip--green">{{ selectedSceneLabel }}</span>
        </div>

        <div class="device-preview-stage">
          <DevicePixelBoard :pixels="currentPixels" :grid-visible="true" />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送 GIF 动画"
            description="发送期间锁定当前预览快照，等待设备完成动画事务写入。"
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
            <h2 class="glx-section-title">离线素材</h2>
            <span class="glx-section-meta">{{ sceneItems.length }} 组</span>
          </div>
          <div class="mode-grid">
            <button
              v-for="item in sceneItems"
              :key="item.id"
              type="button"
              class="mode-grid__item"
              :class="{ 'is-active': config.sceneId === item.id }"
              @click="config.sceneId = item.id"
            >
              <strong>{{ item.label }}</strong>
              <span>{{ item.id }}</span>
            </button>
          </div>
        </article>

        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">参数</h2>
            <span class="glx-section-meta">本地缓存恢复</span>
          </div>

          <div class="mode-row">
            <span class="mode-row__label">速度</span>
            <DeviceModeStepper v-model="config.speed" :min="1" :max="10" />
          </div>

          <div class="mode-row">
            <span class="mode-row__label">强度</span>
            <DeviceModeStepper v-model="config.intensity" :min="10" :max="100" />
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
import DeviceModeStepper from "@/components/device/modes/DeviceModeStepper.vue";
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import { useDeviceStore } from "@/stores/device.js";
import { readStorageJson, writeStorageJson } from "@/utils/device-mode-core.js";
import {
  buildGifPlayerAnimationPayload,
  buildGifPlayerPreviewSequence,
  getGifPlayerSceneItems,
} from "@/utils/device-mode-gif-player.js";

const STORAGE_KEY = "gif_player_config";
const sceneItems = getGifPlayerSceneItems();
const savedConfig = readStorageJson(STORAGE_KEY);
const config = reactive({
  sceneId: typeof savedConfig?.sceneId === "string" ? savedConfig.sceneId : "matrix_fire",
  speed: Number.isFinite(Number(savedConfig?.speed)) ? Math.round(Number(savedConfig.speed)) : 6,
  intensity: Number.isFinite(Number(savedConfig?.intensity)) ? Math.round(Number(savedConfig.intensity)) : 72,
});

const deviceStore = useDeviceStore();
const feedback = useFeedback();
const { currentPixels, playSequence, snapshot } = usePixelPreviewPlayer();
const isSending = ref(false);
const sendingPixels = ref(new Map());

const selectedScene = computed(() => {
  return sceneItems.find((item) => item.id === config.sceneId) ?? null;
});

const selectedSceneLabel = computed(() => {
  return selectedScene.value === null ? "--" : selectedScene.value.label;
});

function saveConfig() {
  writeStorageJson(STORAGE_KEY, {
    sceneId: config.sceneId,
    speed: config.speed,
    intensity: config.intensity,
  });
}

function refreshPreview() {
  playSequence(
    buildGifPlayerPreviewSequence({
      sceneId: config.sceneId,
      speed: config.speed,
      intensity: config.intensity,
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

  isSending.value = true;
  sendingPixels.value = snapshot();
  feedback.showBlocking("发送 GIF", `正在把 ${selectedSceneLabel.value} 发送到设备。`);
  try {
    await deviceStore.setGifAnimation(
      buildGifPlayerAnimationPayload({
        sceneId: config.sceneId,
        speed: config.speed,
        intensity: config.intensity,
      }),
    );
    feedback.success("发送成功", `${selectedSceneLabel.value} 已经发送到设备。`);
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "GIF 播放器发送失败。");
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

.device-preview-card__desc {
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
  min-height: 88px;
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

.mode-grid__item span {
  color: var(--glx-text-muted);
  font-size: 12px;
}

.mode-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  align-items: center;
  gap: 12px;
}

.mode-row__label {
  font-size: 14px;
  font-weight: 800;
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
