<template>
  <div class="glx-page-shell game-mode-page">
    <section class="game-mode-layout">
      <article class="glx-section-card glx-section-card--stack game-preview-card">
        <div class="game-preview-card__head">
          <div>
            <h1 class="game-page-title">俄罗斯方块时钟</h1>
            <p class="game-page-meta">顶部预览直接复用 uniapp 的俄罗斯方块时钟像素帧，发送字段仍然只有速度和小时制式。</p>
          </div>
          <span class="glx-chip" :class="deviceStore.connected ? 'glx-chip--green' : 'glx-chip--yellow'">
            {{ deviceStore.connected ? "已连接" : "未连接" }}
          </span>
        </div>

        <div class="game-preview-stage">
          <DevicePixelBoard :pixels="displayPixels" :grid-visible="true" />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送俄罗斯方块时钟"
            description="发送期间锁定当前预览快照，等待设备完成方块时钟事务提交。"
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
            <h2 class="glx-section-title">下落速度</h2>
            <span class="glx-section-meta">慢 / 中 / 快</span>
          </div>
          <DeviceModeTabs v-model="config.speed" :items="speedOptions" />
        </article>

        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">小时制式</h2>
            <span class="glx-section-meta">24 小时 / 12 小时</span>
          </div>
          <DeviceModeTabs v-model="config.hourFormat" :items="hourFormatOptions" />
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, reactive, ref, watch } from "vue";
import DeviceSendingOverlay from "@/components/device/DeviceSendingOverlay.vue";
import DeviceModeTabs from "@/components/device/modes/DeviceModeTabs.vue";
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import { useFeedback } from "@/composables/useFeedback.js";
import { usePixelPreviewPlayer } from "@/composables/usePixelPreviewPlayer.js";
import { useDeviceStore } from "@/stores/device.js";
import { readStorageJson, writeStorageJson } from "@/utils/device-mode-core.js";
import { buildTetrisPreviewFrames } from "@/utils/device-mode-tetris-clock.js";

const TETRIS_CLOCK_STORAGE_KEY = "tetris_clock_config";
const TETRIS_SPEED_OPTIONS = Object.freeze({
  slow: 300,
  normal: 150,
  fast: 80,
});

const speedOptions = Object.freeze([
  { value: "slow", label: "慢" },
  { value: "normal", label: "中" },
  { value: "fast", label: "快" },
]);

const hourFormatOptions = Object.freeze([
  { value: 24, label: "24 小时" },
  { value: 12, label: "12 小时" },
]);

const deviceStore = useDeviceStore();
const feedback = useFeedback();
const { currentPixels, playSequence, snapshot } = usePixelPreviewPlayer();

const config = reactive(loadConfig());
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
    writeStorageJson(TETRIS_CLOCK_STORAGE_KEY, config);
    refreshPreview();
  },
  { deep: true, immediate: true },
);

onMounted(() => {
  deviceStore.init();
});

function loadConfig() {
  const saved = readStorageJson(TETRIS_CLOCK_STORAGE_KEY);
  if (!saved || typeof saved !== "object") {
    return {
      speed: "normal",
      hourFormat: 24,
    };
  }
  return {
    speed: saved.speed === "slow" || saved.speed === "normal" || saved.speed === "fast" ? saved.speed : "normal",
    hourFormat: saved.hourFormat === 12 ? 12 : 24,
  };
}

function refreshPreview() {
  const maps = buildTetrisPreviewFrames(config);
  const delay = TETRIS_SPEED_OPTIONS[config.speed];
  playSequence({
    maps,
    delays: maps.map(() => delay),
  });
}

async function handleSend() {
  if (deviceStore.connected !== true) {
    feedback.warning("设备未连接", "请先返回设备控制页建立连接。");
    return;
  }

  isSending.value = true;
  sendingPixels.value = snapshot();
  feedback.showBlocking("发送俄罗斯方块时钟", "正在把当前俄罗斯方块时钟发送到设备。");
  try {
    const payload = {
      speed: TETRIS_SPEED_OPTIONS[config.speed],
      hourFormat: config.hourFormat,
    };
    writeStorageJson(TETRIS_CLOCK_STORAGE_KEY, config);
    await deviceStore.startTetrisClock(payload);
    feedback.success("发送成功", "俄罗斯方块时钟已发送到设备。");
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "俄罗斯方块时钟发送失败。");
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

@media (max-width: 1080px) {
  .game-mode-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .game-preview-card {
    position: static;
  }
}
</style>
