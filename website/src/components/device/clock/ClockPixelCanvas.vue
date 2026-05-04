<template>
  <canvas
    ref="canvasRef"
    class="clock-pixel-canvas"
    :class="{ 'clock-pixel-canvas--rounded': rounded }"
    :width="size"
    :height="size"
  />
</template>

<script setup>
import { onMounted, ref, watch } from "vue";

const props = defineProps({
  frame: {
    type: Object,
    required: true,
  },
  size: {
    type: Number,
    default: 64,
  },
  rounded: {
    type: Boolean,
    default: false,
  },
});

const canvasRef = ref(null);

function drawFrame() {
  const canvas = canvasRef.value;
  if (!canvas || !props.frame) {
    return;
  }

  const context = canvas.getContext("2d");
  if (!context) {
    return;
  }

  context.imageSmoothingEnabled = false;
  context.clearRect(0, 0, props.size, props.size);
  context.fillStyle = props.frame.background || "#05070c";
  context.fillRect(0, 0, props.size, props.size);

  if (!(props.frame.pixels instanceof Map)) {
    return;
  }

  props.frame.pixels.forEach((color, key) => {
    const [x, y] = key.split(",").map(Number);
    context.fillStyle = color;
    context.fillRect(x, y, 1, 1);
  });
}

onMounted(drawFrame);

watch(
  () => props.frame,
  () => {
    drawFrame();
  },
  { deep: true },
);
</script>

<style scoped>
.clock-pixel-canvas {
  width: 100%;
  height: auto;
  aspect-ratio: 1;
  display: block;
  image-rendering: pixelated;
}

.clock-pixel-canvas--rounded {
  border-radius: 18px;
}
</style>
