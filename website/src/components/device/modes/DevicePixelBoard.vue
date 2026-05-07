<template>
  <div class="device-pixel-board" :class="{ 'device-pixel-board--dark': dark }">
    <canvas ref="canvasRef" class="device-pixel-board__canvas"></canvas>
  </div>
</template>

<script setup>
import { onBeforeUnmount, onMounted, ref, watch } from "vue";

const props = defineProps({
  pixels: {
    type: [Map, Array, Object],
    required: true,
  },
  gridVisible: {
    type: Boolean,
    default: true,
  },
  dark: {
    type: Boolean,
    default: true,
  },
});

const canvasRef = ref(null);
const PANEL_SIZE = 64;
const INTERNAL_SIZE = 640;

function iteratePixels(pixels, callback) {
  if (pixels instanceof Map) {
    pixels.forEach((value, key) => callback(key, value));
    return;
  }

  if (Array.isArray(pixels)) {
    pixels.forEach((entry) => {
      if (Array.isArray(entry) && entry.length >= 2) {
        callback(entry[0], entry[1]);
      }
    });
    return;
  }

  if (pixels && typeof pixels === "object") {
    Object.keys(pixels).forEach((key) => callback(key, pixels[key]));
  }
}

function drawGrid(ctx, step) {
  ctx.strokeStyle = "rgba(255,255,255,0.08)";
  ctx.lineWidth = 1;
  for (let index = 0; index <= PANEL_SIZE; index += 1) {
    const offset = Math.round(index * step) + 0.5;
    ctx.beginPath();
    ctx.moveTo(offset, 0);
    ctx.lineTo(offset, INTERNAL_SIZE);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(0, offset);
    ctx.lineTo(INTERNAL_SIZE, offset);
    ctx.stroke();
  }
}

function render() {
  const canvas = canvasRef.value;
  if (!canvas) {
    return;
  }

  canvas.width = INTERNAL_SIZE;
  canvas.height = INTERNAL_SIZE;
  const ctx = canvas.getContext("2d");
  if (!ctx) {
    return;
  }

  const cellSize = INTERNAL_SIZE / PANEL_SIZE;
  ctx.imageSmoothingEnabled = false;
  ctx.clearRect(0, 0, INTERNAL_SIZE, INTERNAL_SIZE);
  ctx.fillStyle = props.dark ? "#000000" : "#ffffff";
  ctx.fillRect(0, 0, INTERNAL_SIZE, INTERNAL_SIZE);

  iteratePixels(props.pixels, (key, color) => {
    if (typeof key !== "string" || typeof color !== "string") {
      return;
    }
    const parts = key.split(",");
    if (parts.length !== 2) {
      return;
    }
    const x = Number(parts[0]);
    const y = Number(parts[1]);
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      return;
    }
    ctx.fillStyle = color;
    ctx.fillRect(
      Math.round(x * cellSize),
      Math.round(y * cellSize),
      Math.ceil(cellSize),
      Math.ceil(cellSize),
    );
  });

  if (props.gridVisible) {
    drawGrid(ctx, cellSize);
  }
}

watch(
  () => props.pixels,
  () => {
    render();
  },
  { deep: true },
);

watch(
  () => props.gridVisible,
  () => {
    render();
  },
);

onMounted(() => {
  render();
});

onBeforeUnmount(() => {
  const canvas = canvasRef.value;
  if (!canvas) {
    return;
  }
  const ctx = canvas.getContext("2d");
  if (ctx) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
  }
});
</script>

<style scoped>
.device-pixel-board {
  width: 100%;
  aspect-ratio: 1;
  border: 2px solid #000000;
  background: #000000;
  box-shadow: 6px 6px 0 #000000;
  overflow: hidden;
}

.device-pixel-board--dark {
  background: #000000;
}

.device-pixel-board__canvas {
  display: block;
  width: 100%;
  height: 100%;
  image-rendering: pixelated;
}
</style>
