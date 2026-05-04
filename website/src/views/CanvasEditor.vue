<template>
  <div class="glx-page-shell canvas-editor-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Canvas Mode</span>
      <h1 class="glx-page-shell__title">画板模式</h1>
      <p class="glx-page-shell__desc">
        这里按 `uniapp/canvas-editor` 的正式链路复刻 64×64 画板编辑：本地缓存恢复、预览编辑、清空撤销、切到画板模式后发送稀疏像素都在这一页完成。
      </p>

      <div class="glx-inline-actions">
        <router-link to="/device-control" class="glx-button glx-button--ghost">返回设备控制</router-link>
        <router-link to="/device-params" class="glx-button glx-button--ghost">设备参数</router-link>
      </div>

      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">连接状态</span>
          <strong class="glx-hero-metric__value">{{ deviceStore.connected ? "已连接" : "未连接" }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">已上色像素</span>
          <strong class="glx-hero-metric__value">{{ coloredPixelCount }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前缩放</span>
          <strong class="glx-hero-metric__value">{{ zoom }}x</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前业务模式</span>
          <strong class="glx-hero-metric__value">{{ businessModeText }}</strong>
        </article>
      </div>
    </section>

    <section class="canvas-editor-layout">
      <article class="glx-section-card glx-section-card--stack canvas-editor-preview-card">
        <div class="canvas-editor-preview-card__head">
          <div>
            <h2 class="glx-section-title">预览效果</h2>
            <p class="canvas-editor-preview-card__desc">
              黑底预览舞台和正式发送使用同一份 64×64 像素数据；拖动画板时切到“拖动”，绘制时切到“绘画”或“擦除”。
            </p>
          </div>
          <span class="glx-chip glx-chip--blue">64 × 64</span>
        </div>

        <div ref="stageRef" class="canvas-editor-stage">
          <canvas
            ref="canvasRef"
            class="canvas-editor-stage__canvas"
            :class="{
              'canvas-editor-stage__canvas--drag': currentTool === 'move',
            }"
            @pointerdown="handlePointerDown"
            @pointermove="handlePointerMove"
            @pointerup="handlePointerUp"
            @pointerleave="handlePointerUp"
            @pointercancel="handlePointerUp"
            @wheel.prevent="handleWheel"
          ></canvas>
          <DeviceSendingOverlay
            :visible="isSending"
            title="正在发送画板像素"
            description="发送期间锁定当前 64×64 快照，等待设备完成画板模式切换和稀疏像素写入。"
          >
            <DevicePixelBoard :pixels="sendingPixels" :grid-visible="true" />
          </DeviceSendingOverlay>
        </div>

        <div class="canvas-editor-preview-bar">
          <div class="canvas-editor-preview-bar__copy">
            <strong class="canvas-editor-preview-bar__title">首屏预览</strong>
            <span class="canvas-editor-preview-bar__desc">
              {{ currentToolLabel }} · 画笔 {{ brushSize }}x{{ brushSize }}
            </span>
          </div>

          <div class="glx-inline-actions">
            <button
              type="button"
              class="glx-button glx-button--primary"
              :disabled="isSending"
              @click="publishCanvas"
            >
              {{ isSending ? "发送中..." : "发送" }}
            </button>
            <button type="button" class="glx-button glx-button--ghost" @click="handleFit">
              适配
            </button>
          </div>
        </div>
      </article>

      <div class="canvas-editor-stack">
        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">快捷操作</h2>
            <span class="glx-section-meta">清空 / 撤销 / 视图</span>
          </div>

          <div class="canvas-editor-action-grid">
            <button
              type="button"
              class="canvas-editor-action-btn"
              :disabled="historyIndex <= 0"
              @click="handleUndo"
            >
              撤销
            </button>
            <button
              type="button"
              class="canvas-editor-action-btn"
              :disabled="historyIndex >= history.length - 1"
              @click="handleRedo"
            >
              重做
            </button>
            <button type="button" class="canvas-editor-action-btn" @click="handleZoom(-1)">
              缩小
            </button>
            <button type="button" class="canvas-editor-action-btn" @click="handleZoom(1)">
              放大
            </button>
            <button type="button" class="canvas-editor-action-btn" @click="handleFit">
              适配
            </button>
            <button type="button" class="canvas-editor-action-btn canvas-editor-action-btn--danger" @click="clearCanvas">
              清空
            </button>
          </div>
        </article>

        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">绘制工具</h2>
            <span class="glx-section-meta">拖动 / 绘画 / 擦除</span>
          </div>

          <DeviceModeTabs v-model="currentTool" :items="toolItems" />
        </article>

        <article
          v-if="currentTool !== 'move'"
          class="glx-section-card glx-section-card--stack"
        >
          <div class="glx-section-head">
            <h2 class="glx-section-title">笔触大小</h2>
            <span class="glx-section-meta">与 uniapp 对齐</span>
          </div>

          <DeviceModeTabs v-model="brushSize" :items="brushSizeItems" />
        </article>

        <article
          v-if="currentTool !== 'move'"
          class="glx-section-card glx-section-card--stack"
        >
          <div class="glx-section-head">
            <h2 class="glx-section-title">画笔颜色</h2>
            <span class="glx-section-meta">本地缓存画布数据</span>
          </div>

          <div class="canvas-editor-color-row">
            <label class="canvas-editor-color-picker">
              <span class="canvas-editor-color-picker__label">当前颜色</span>
              <input
                type="color"
                :value="selectedColor"
                class="canvas-editor-color-picker__input"
                @input="handleNativeColorInput($event.target.value)"
              />
            </label>

            <div class="canvas-editor-color-code">
              <span class="canvas-editor-color-code__label">颜色值</span>
              <strong class="canvas-editor-color-code__value">{{ selectedColor }}</strong>
            </div>
          </div>

          <DeviceColorSwatches v-model="selectedColor" :items="presetColors" />
        </article>

        <article class="glx-section-card glx-section-card--stack">
          <div class="glx-section-head">
            <h2 class="glx-section-title">当前状态</h2>
            <span class="glx-section-meta">本地缓存已启用</span>
          </div>

          <div class="glx-kv-grid">
            <div class="glx-kv-card">
              <span class="glx-kv-card__label">本地缓存键</span>
              <strong class="glx-kv-card__value">{{ storageKey }}</strong>
            </div>
            <div class="glx-kv-card">
              <span class="glx-kv-card__label">画布尺寸</span>
              <strong class="glx-kv-card__value">64 × 64</strong>
            </div>
            <div class="glx-kv-card">
              <span class="glx-kv-card__label">拖动偏移</span>
              <strong class="glx-kv-card__value">{{ panText }}</strong>
            </div>
            <div class="glx-kv-card">
              <span class="glx-kv-card__label">当前工具</span>
              <strong class="glx-kv-card__value">{{ currentToolLabel }}</strong>
            </div>
          </div>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, nextTick, onBeforeUnmount, onMounted, reactive, ref, watch } from "vue";
import DeviceSendingOverlay from "@/components/device/DeviceSendingOverlay.vue";
import DevicePixelBoard from "@/components/device/modes/DevicePixelBoard.vue";
import DeviceColorSwatches from "@/components/device/modes/DeviceColorSwatches.vue";
import DeviceModeTabs from "@/components/device/modes/DeviceModeTabs.vue";
import { useFeedback } from "@/composables/useFeedback.js";
import { useDeviceStore } from "@/stores/device.js";
import { hexToRgb, normalizeHexColor, readStorageJson, writeStorageJson } from "@/utils/device-mode-core.js";

const PANEL_SIZE = 64;
const HISTORY_LIMIT = 50;
const MIN_ZOOM = 2;
const MAX_ZOOM = 20;
const CANVAS_PIXELS_KEY = "canvas_mode_pixels";

const toolItems = Object.freeze([
  { value: "move", label: "拖动" },
  { value: "pencil", label: "绘画" },
  { value: "eraser", label: "擦除" },
]);

const brushSizeItems = Object.freeze([
  { value: 1, label: "1x1" },
  { value: 2, label: "2x2" },
  { value: 3, label: "3x3" },
  { value: 4, label: "4x4" },
]);

const presetColors = Object.freeze([
  { label: "冰蓝", value: "#64c8ff" },
  { label: "亮黄", value: "#ffd23f" },
  { label: "橘红", value: "#ff8454" },
  { label: "薄荷", value: "#67d7a5" },
  { label: "紫粉", value: "#d57cff" },
  { label: "白色", value: "#ffffff" },
  { label: "深蓝", value: "#356dff" },
  { label: "暖红", value: "#ff5f6d" },
]);

const deviceStore = useDeviceStore();
const feedback = useFeedback();

const stageRef = ref(null);
const canvasRef = ref(null);
const pixels = ref(new Map());
const history = ref([]);
const historyIndex = ref(-1);
const currentTool = ref("pencil");
const brushSize = ref(1);
const selectedColor = ref("#64c8ff");
const zoom = ref(4);
const pan = reactive({ x: 0, y: 0 });
const viewport = reactive({ width: 0, height: 0 });
const isSending = ref(false);
const sendingPixels = ref(new Map());

const moveState = reactive({
  active: false,
  startClientX: 0,
  startClientY: 0,
  startPanX: 0,
  startPanY: 0,
});

const drawState = reactive({
  active: false,
  dirty: false,
});

let strokePixels = null;
let resizeObserver = null;

const storageKey = CANVAS_PIXELS_KEY;

const coloredPixelCount = computed(() => {
  return pixels.value.size;
});

const businessModeText = computed(() => {
  if (typeof deviceStore.businessMode === "string" && deviceStore.businessMode.length > 0) {
    return deviceStore.businessMode;
  }
  return "--";
});

const panText = computed(() => {
  return `${Math.round(pan.x)}, ${Math.round(pan.y)}`;
});

const currentToolLabel = computed(() => {
  const matched = toolItems.find((item) => item.value === currentTool.value);
  if (matched) {
    return matched.label;
  }
  return "--";
});

watch(
  pixels,
  () => {
    renderCanvas();
  },
  { deep: true },
);

watch(
  () => zoom.value,
  () => {
    renderCanvas();
  },
);

watch(
  () => [pan.x, pan.y],
  () => {
    renderCanvas();
  },
);

watch(
  selectedColor,
  (value) => {
    selectedColor.value = normalizeHexColor(value);
  },
);

onMounted(async () => {
  loadPixels();
  deviceStore.init();

  try {
    await deviceStore.restoreConnection();
  } catch (error) {
    // 保持离线编辑，不阻断页面打开
  }

  if (deviceStore.connected) {
    try {
      await deviceStore.syncDeviceStatus();
    } catch (error) {
      // 保持当前本地态
    }
  }

  await nextTick();
  setupResizeObserver();
  renderCanvas();
});

onBeforeUnmount(() => {
  teardownResizeObserver();
});

function loadPixels() {
  const savedPixels = readStorageJson(CANVAS_PIXELS_KEY);
  if (Array.isArray(savedPixels)) {
    pixels.value = new Map(savedPixels);
  } else {
    pixels.value = new Map();
  }
  history.value = [new Map(pixels.value)];
  historyIndex.value = 0;
}

function persistPixels() {
  writeStorageJson(CANVAS_PIXELS_KEY, Array.from(pixels.value.entries()));
}

function pushHistory(nextPixels) {
  const nextHistory = history.value.slice(0, historyIndex.value + 1);
  nextHistory.push(new Map(nextPixels));
  if (nextHistory.length > HISTORY_LIMIT) {
    nextHistory.shift();
  }
  history.value = nextHistory;
  historyIndex.value = nextHistory.length - 1;
}

function setupResizeObserver() {
  const target = stageRef.value;
  if (target === null) {
    return;
  }

  if (typeof ResizeObserver === "undefined") {
    measureViewport();
    return;
  }

  resizeObserver = new ResizeObserver(() => {
    measureViewport();
  });
  resizeObserver.observe(target);
  measureViewport();
}

function teardownResizeObserver() {
  if (resizeObserver !== null) {
    resizeObserver.disconnect();
    resizeObserver = null;
  }
}

function measureViewport() {
  const target = stageRef.value;
  if (target === null) {
    return;
  }

  const nextWidth = Math.max(240, Math.floor(target.clientWidth));
  const nextHeight = Math.max(240, Math.floor(target.clientHeight));
  viewport.width = nextWidth;
  viewport.height = nextHeight;
  handleFit();
}

function handleFit() {
  if (viewport.width <= 0 || viewport.height <= 0) {
    return;
  }

  const fitZoomWidth = Math.floor((viewport.width * 0.96) / PANEL_SIZE);
  const fitZoomHeight = Math.floor((viewport.height * 0.96) / PANEL_SIZE);
  const fitZoom = Math.min(fitZoomWidth, fitZoomHeight, MAX_ZOOM);
  zoom.value = Math.max(MIN_ZOOM, fitZoom);
  pan.x = Math.round((viewport.width - PANEL_SIZE * zoom.value) / 2);
  pan.y = Math.round((viewport.height - PANEL_SIZE * zoom.value) / 2);
  renderCanvas();
}

function handleZoom(delta) {
  const nextZoom = Math.max(MIN_ZOOM, Math.min(MAX_ZOOM, zoom.value + delta));
  if (nextZoom === zoom.value) {
    return;
  }

  const centerX = viewport.width / 2;
  const centerY = viewport.height / 2;
  const scale = nextZoom / zoom.value;
  pan.x = centerX - (centerX - pan.x) * scale;
  pan.y = centerY - (centerY - pan.y) * scale;
  zoom.value = nextZoom;
  renderCanvas();
}

function renderCanvas() {
  const canvas = canvasRef.value;
  if (canvas === null || viewport.width <= 0 || viewport.height <= 0) {
    return;
  }

  const dpr = globalThis.devicePixelRatio || 1;
  const width = viewport.width;
  const height = viewport.height;
  canvas.width = Math.round(width * dpr);
  canvas.height = Math.round(height * dpr);
  canvas.style.width = `${width}px`;
  canvas.style.height = `${height}px`;

  const context = canvas.getContext("2d");
  if (context === null) {
    return;
  }

  context.setTransform(dpr, 0, 0, dpr, 0, 0);
  context.imageSmoothingEnabled = false;
  context.clearRect(0, 0, width, height);
  context.fillStyle = "#000000";
  context.fillRect(0, 0, width, height);

  context.save();
  context.beginPath();
  context.rect(0, 0, width, height);
  context.clip();

  const cellSize = zoom.value;
  const boardWidth = PANEL_SIZE * cellSize;
  const boardHeight = PANEL_SIZE * cellSize;
  const startX = pan.x;
  const startY = pan.y;

  context.fillStyle = "#000000";
  context.fillRect(startX, startY, boardWidth, boardHeight);

  pixels.value.forEach((color, key) => {
    if (typeof color !== "string" || typeof key !== "string") {
      return;
    }
    const parts = key.split(",");
    if (parts.length !== 2) {
      return;
    }
    const x = Number(parts[0]);
    const y = Number(parts[1]);
    if (!Number.isInteger(x) || !Number.isInteger(y)) {
      return;
    }

    context.fillStyle = color;
    context.fillRect(
      Math.round(startX + x * cellSize),
      Math.round(startY + y * cellSize),
      Math.ceil(cellSize),
      Math.ceil(cellSize),
    );
  });

  context.strokeStyle = "rgba(255,255,255,0.08)";
  context.lineWidth = 1;
  for (let index = 0; index <= PANEL_SIZE; index += 1) {
    const offsetX = Math.round(startX + index * cellSize) + 0.5;
    context.beginPath();
    context.moveTo(offsetX, startY);
    context.lineTo(offsetX, startY + boardHeight);
    context.stroke();

    const offsetY = Math.round(startY + index * cellSize) + 0.5;
    context.beginPath();
    context.moveTo(startX, offsetY);
    context.lineTo(startX + boardWidth, offsetY);
    context.stroke();
  }

  context.strokeStyle = "#ffffff";
  context.strokeRect(
    Math.round(startX) + 0.5,
    Math.round(startY) + 0.5,
    Math.round(boardWidth),
    Math.round(boardHeight),
  );
  context.restore();
}

function resolveBoardCell(event) {
  const canvas = canvasRef.value;
  if (canvas === null) {
    return null;
  }

  const rect = canvas.getBoundingClientRect();
  const localX = event.clientX - rect.left;
  const localY = event.clientY - rect.top;
  const boardX = Math.floor((localX - pan.x) / zoom.value);
  const boardY = Math.floor((localY - pan.y) / zoom.value);

  if (
    !Number.isInteger(boardX) ||
    !Number.isInteger(boardY) ||
    boardX < 0 ||
    boardX >= PANEL_SIZE ||
    boardY < 0 ||
    boardY >= PANEL_SIZE
  ) {
    return null;
  }

  return { x: boardX, y: boardY };
}

function handlePointerDown(event) {
  if (event.button !== 0) {
    return;
  }

  if (canvasRef.value !== null) {
    canvasRef.value.setPointerCapture(event.pointerId);
  }

  if (currentTool.value === "move") {
    moveState.active = true;
    moveState.startClientX = event.clientX;
    moveState.startClientY = event.clientY;
    moveState.startPanX = pan.x;
    moveState.startPanY = pan.y;
    return;
  }

  drawState.active = true;
  drawState.dirty = false;
  strokePixels = new Map(pixels.value);
  applyBrushFromEvent(event);
}

function handlePointerMove(event) {
  if (moveState.active) {
    const deltaX = event.clientX - moveState.startClientX;
    const deltaY = event.clientY - moveState.startClientY;
    pan.x = moveState.startPanX + deltaX;
    pan.y = moveState.startPanY + deltaY;
    renderCanvas();
    return;
  }

  if (drawState.active) {
    applyBrushFromEvent(event);
  }
}

function handlePointerUp(event) {
  if (canvasRef.value !== null && canvasRef.value.hasPointerCapture(event.pointerId)) {
    canvasRef.value.releasePointerCapture(event.pointerId);
  }

  if (moveState.active) {
    moveState.active = false;
  }

  if (!drawState.active) {
    return;
  }

  drawState.active = false;
  if (drawState.dirty) {
    pushHistory(pixels.value);
    persistPixels();
  }
  drawState.dirty = false;
  strokePixels = null;
}

function applyBrushFromEvent(event) {
  const cell = resolveBoardCell(event);
  if (cell === null || strokePixels === null) {
    return;
  }

  const changed = applyBrushToMap(strokePixels, cell.x, cell.y);
  if (!changed) {
    return;
  }

  drawState.dirty = true;
  pixels.value = new Map(strokePixels);
  renderCanvas();
}

function applyBrushToMap(targetMap, x, y) {
  const startX = x - Math.floor((brushSize.value - 1) / 2);
  const startY = y - Math.floor((brushSize.value - 1) / 2);
  let changed = false;

  for (let offsetX = 0; offsetX < brushSize.value; offsetX += 1) {
    for (let offsetY = 0; offsetY < brushSize.value; offsetY += 1) {
      const pixelX = startX + offsetX;
      const pixelY = startY + offsetY;

      if (pixelX < 0 || pixelX >= PANEL_SIZE || pixelY < 0 || pixelY >= PANEL_SIZE) {
        continue;
      }

      const key = `${pixelX},${pixelY}`;
      if (currentTool.value === "eraser") {
        if (targetMap.has(key)) {
          targetMap.delete(key);
          changed = true;
        }
        continue;
      }

      if (targetMap.get(key) === selectedColor.value) {
        continue;
      }
      targetMap.set(key, selectedColor.value);
      changed = true;
    }
  }

  return changed;
}

function handleWheel(event) {
  const direction = event.deltaY > 0 ? -1 : 1;
  handleZoom(direction);
}

function handleUndo() {
  if (historyIndex.value <= 0) {
    return;
  }
  historyIndex.value -= 1;
  pixels.value = new Map(history.value[historyIndex.value]);
  persistPixels();
  renderCanvas();
}

function handleRedo() {
  if (historyIndex.value >= history.value.length - 1) {
    return;
  }
  historyIndex.value += 1;
  pixels.value = new Map(history.value[historyIndex.value]);
  persistPixels();
  renderCanvas();
}

function clearCanvas() {
  if (pixels.value.size === 0) {
    feedback.info("画板已为空", "当前本地画板没有需要清除的像素。");
    return;
  }

  pixels.value = new Map();
  pushHistory(pixels.value);
  persistPixels();
  renderCanvas();
  feedback.info("画板已清空", "本地 64×64 画板已经清空。");
}

function handleNativeColorInput(value) {
  selectedColor.value = normalizeHexColor(value);
}

async function publishCanvas() {
  persistPixels();

  if (!deviceStore.connected) {
    feedback.warning("设备未连接", "已保存到本地，请先返回设备控制页建立连接。");
    return;
  }

  isSending.value = true;
  sendingPixels.value = new Map(pixels.value);
  feedback.showBlocking("发送画板", "正在切换到画板模式并发送当前稀疏像素。");

  try {
    await deviceStore.ensureCanvasMode();
    await deviceStore.sendSparseImage(buildSparsePixels(), PANEL_SIZE, PANEL_SIZE);
    await deviceStore.syncDeviceStatus();
    feedback.success("发送成功", "画板像素已经发送到设备。");
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("发送失败", error.message);
    } else {
      feedback.error("发送失败", "画板像素发送失败。");
    }
  } finally {
    isSending.value = false;
    feedback.hideBlocking();
  }
}

function buildSparsePixels() {
  const sparsePixels = [];
  pixels.value.forEach((color, key) => {
    if (typeof key !== "string" || typeof color !== "string") {
      return;
    }

    const parts = key.split(",");
    if (parts.length !== 2) {
      return;
    }

    const x = Number(parts[0]);
    const y = Number(parts[1]);
    if (!Number.isInteger(x) || !Number.isInteger(y)) {
      return;
    }

    const rgb = hexToRgb(color);
    sparsePixels.push(x, y, rgb.r, rgb.g, rgb.b);
  });
  return sparsePixels;
}
</script>

<style scoped>
.canvas-editor-page {
  gap: 24px;
}

.canvas-editor-layout {
  display: grid;
  grid-template-columns: minmax(340px, 0.98fr) minmax(0, 1.02fr);
  gap: 24px;
  align-items: start;
}

.canvas-editor-preview-card {
  position: sticky;
  top: 88px;
}

.canvas-editor-preview-card__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.canvas-editor-preview-card__desc {
  margin: 6px 0 0;
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.7;
}

.canvas-editor-stage {
  position: relative;
  width: 100%;
  aspect-ratio: 1;
  border: 2px solid #000000;
  background: #000000;
  box-shadow: 4px 4px 0 #000000;
  overflow: hidden;
  touch-action: none;
}

.canvas-editor-stage__canvas {
  display: block;
  width: 100%;
  height: 100%;
  cursor: crosshair;
}

.canvas-editor-stage__canvas--drag {
  cursor: grab;
}

.canvas-editor-preview-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
  padding: 14px 16px;
  border: 2px solid #000000;
  background: #ffffff;
}

.canvas-editor-preview-bar__copy {
  display: grid;
  gap: 4px;
}

.canvas-editor-preview-bar__title {
  color: var(--nb-ink);
  font-size: 15px;
  font-weight: 900;
}

.canvas-editor-preview-bar__desc {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.6;
}

.canvas-editor-stack {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.canvas-editor-action-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}

.canvas-editor-action-btn {
  min-height: 54px;
  padding: 12px 10px;
  border: 2px solid #000000;
  background: #ffffff;
  color: #000000;
  font-size: 13px;
  font-weight: 900;
  cursor: pointer;
}

.canvas-editor-action-btn[disabled] {
  opacity: 0.45;
  cursor: not-allowed;
}

.canvas-editor-action-btn--danger {
  background: #ffd6d6;
}

.canvas-editor-color-row {
  display: grid;
  grid-template-columns: minmax(0, 220px) minmax(0, 1fr);
  gap: 16px;
  align-items: stretch;
}

.canvas-editor-color-picker,
.canvas-editor-color-code {
  min-height: 82px;
  padding: 14px;
  display: grid;
  gap: 8px;
  border: 2px solid #000000;
  background: #ffffff;
}

.canvas-editor-color-picker__label,
.canvas-editor-color-code__label {
  color: var(--nb-text-secondary);
  font-size: 12px;
  font-weight: 800;
}

.canvas-editor-color-picker__input {
  width: 100%;
  height: 40px;
  border: 2px solid #000000;
  background: #ffffff;
  padding: 0;
}

.canvas-editor-color-code__value {
  color: var(--nb-ink);
  font-size: 20px;
  line-height: 1.1;
  font-weight: 900;
  text-transform: lowercase;
}

@media (max-width: 1080px) {
  .canvas-editor-layout {
    grid-template-columns: 1fr;
  }

  .canvas-editor-preview-card {
    position: static;
  }
}

@media (max-width: 720px) {
  .canvas-editor-action-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .canvas-editor-color-row {
    grid-template-columns: 1fr;
  }
}
</style>
