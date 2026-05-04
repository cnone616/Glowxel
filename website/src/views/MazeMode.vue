<template>
  <div class="glx-page-shell game-mode-page">
    <section class="game-mode-layout">
      <article class="glx-section-card glx-section-card--stack game-preview-card">
        <div class="game-preview-card__head">
          <div>
            <h1 class="game-page-title">迷宫漫游</h1>
            <p class="game-page-meta">首屏直接显示 64×64 本地预览，颜色参数和发送 payload 同源。</p>
          </div>
          <span class="glx-chip" :class="deviceStore.connected ? 'glx-chip--green' : 'glx-chip--yellow'">
            {{ deviceStore.connected ? "已连接" : "未连接" }}
          </span>
        </div>

        <div class="game-preview-stage">
          <DevicePixelBoard :pixels="displayPixels" :grid-visible="true" />
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送迷宫漫游"
            description="发送期间锁定当前预览快照，等待设备完成迷宫参数事务提交。"
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
            <h2 class="glx-section-title">信息框</h2>
            <span class="glx-section-meta">背景 / 边框 / 时间 / 日期</span>
          </div>

          <div class="game-fields">
            <GameModeColorField
              v-model="config.panelBgColor"
              label="背景颜色"
              :preset-colors="panelPresetColors"
            />
            <GameModeColorField
              v-model="config.borderColor"
              label="边框颜色"
              :preset-colors="borderPresetColors"
            />
            <GameModeColorField
              v-model="config.timeColor"
              label="时间颜色"
              :preset-colors="textPresetColors"
            />
            <GameModeColorField
              v-model="config.dateColor"
              label="月份/日期颜色"
              :preset-colors="textPresetColors"
            />
          </div>
        </article>

        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">生成与寻路</h2>
            <span class="glx-section-meta">生成阶段 / 已搜索 / 待搜索</span>
          </div>

          <div class="game-fields">
            <GameModeColorField
              v-model="config.generationPathColor"
              label="生成阶段路径颜色"
              :preset-colors="stagePresetColors"
            />
            <GameModeColorField
              v-model="config.searchVisitedColor"
              label="寻路已搜索颜色"
              :preset-colors="stagePresetColors"
            />
            <GameModeColorField
              v-model="config.searchFrontierColor"
              label="寻路待搜索颜色"
              :preset-colors="stagePresetColors"
            />
          </div>
        </article>

        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">完成路径</h2>
            <span class="glx-section-meta">首尾渐变</span>
          </div>

          <div class="game-fields">
            <GameModeColorField
              v-model="config.solvedPathStartColor"
              label="最终路径起始色"
              :preset-colors="stagePresetColors"
            />
            <GameModeColorField
              v-model="config.solvedPathEndColor"
              label="最终路径结束色"
              :preset-colors="stagePresetColors"
            />
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
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import GameModeColorField from "@/components/device/modes/GameModeColorField.vue";
import { useDeviceStore } from "@/stores/device.js";
import { readStorageJson, writeStorageJson } from "@/utils/device-mode-core.js";
import { buildLedMatrixPreviewSequence } from "../../../uniapp/utils/ledMatrixShowcase.js";
import {
  MAZE_MODE_CONFIG_KEY,
  cloneMazeModeConfig,
  createDefaultMazeModeConfig,
  createMazeModeConfig,
} from "../../../uniapp/utils/mazeModeConfig.js";

const panelPresetColors = Object.freeze([
  { hex: "#05070f", name: "#05070f" },
  { hex: "#0c1220", name: "#0c1220" },
  { hex: "#101826", name: "#101826" },
  { hex: "#16161b", name: "#16161b" },
  { hex: "#1c2430", name: "#1c2430" },
  { hex: "#2a1d12", name: "#2a1d12" },
]);
const borderPresetColors = Object.freeze([
  { hex: "#182c4c", name: "#182c4c" },
  { hex: "#1d3b66", name: "#1d3b66" },
  { hex: "#20528a", name: "#20528a" },
  { hex: "#2c4f7c", name: "#2c4f7c" },
  { hex: "#44618e", name: "#44618e" },
  { hex: "#7c3aed", name: "#7c3aed" },
]);
const textPresetColors = Object.freeze([
  { hex: "#ffd400", name: "#ffd400" },
  { hex: "#ffe066", name: "#ffe066" },
  { hex: "#ff6464", name: "#ff6464" },
  { hex: "#ff8fab", name: "#ff8fab" },
  { hex: "#7dd3fc", name: "#7dd3fc" },
  { hex: "#ffffff", name: "#ffffff" },
]);
const stagePresetColors = Object.freeze([
  { hex: "#4f4f55", name: "#4f4f55" },
  { hex: "#70ff9c", name: "#70ff9c" },
  { hex: "#ff4444", name: "#ff4444" },
  { hex: "#1a60ff", name: "#1a60ff" },
  { hex: "#42bcff", name: "#42bcff" },
  { hex: "#ffd166", name: "#ffd166" },
]);

const deviceStore = useDeviceStore();
const feedback = useFeedback();
const { currentPixels, playSequence, snapshot } = usePixelPreviewPlayer();

const config = reactive(loadMazeConfig());
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

function loadMazeConfig() {
  const saved = readStorageJson(MAZE_MODE_CONFIG_KEY);
  const normalized = createMazeModeConfig(saved);
  if (normalized !== null) {
    return cloneMazeModeConfig(normalized);
  }
  return createDefaultMazeModeConfig();
}

function buildMazeConfig() {
  const normalized = createMazeModeConfig({
    speed: config.speed,
    mazeSizeMode: config.mazeSizeMode,
    showClock: config.showClock,
    panelBgColor: config.panelBgColor,
    borderColor: config.borderColor,
    timeColor: config.timeColor,
    dateColor: config.dateColor,
    generationPathColor: config.generationPathColor,
    searchVisitedColor: config.searchVisitedColor,
    searchFrontierColor: config.searchFrontierColor,
    solvedPathStartColor: config.solvedPathStartColor,
    solvedPathEndColor: config.solvedPathEndColor,
  });
  if (normalized === null) {
    throw new Error("迷宫颜色配置无效");
  }
  return normalized;
}

function saveMazeConfig() {
  writeStorageJson(MAZE_MODE_CONFIG_KEY, buildMazeConfig());
}

function refreshPreview() {
  const previewSequence = buildLedMatrixPreviewSequence({
    demoId: "maze",
    speed: config.speed,
    mazeSizeMode: config.mazeSizeMode,
    showClock: config.showClock,
    panelBgColor: config.panelBgColor,
    borderColor: config.borderColor,
    timeColor: config.timeColor,
    dateColor: config.dateColor,
    generationPathColor: config.generationPathColor,
    searchVisitedColor: config.searchVisitedColor,
    searchFrontierColor: config.searchFrontierColor,
    solvedPathStartColor: config.solvedPathStartColor,
    solvedPathEndColor: config.solvedPathEndColor,
  });
  playSequence(previewSequence);
}

async function handleSend() {
  if (deviceStore.connected !== true) {
    feedback.warning("设备未连接", "请先返回设备控制页建立连接。");
    return;
  }

  isSending.value = true;
  sendingPixels.value = snapshot();
  feedback.showBlocking("发送迷宫漫游", "正在把当前迷宫参数发送到设备。");
  try {
    const nextConfig = buildMazeConfig();
    await deviceStore.startMaze(nextConfig);
    saveMazeConfig();
    feedback.success("发送成功", "迷宫漫游已发送到设备。");
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "迷宫漫游发送失败。");
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
  display: flex;
  flex-direction: column;
  gap: 18px;
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
