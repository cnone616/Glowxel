<script setup>
import { onMounted, ref, watch } from 'vue'
import { buildPreviewFrameCanvas, drawScaledCanvas } from '../../utils/canvasRenderer'
import { buildCanvasTextLayout, composePixelFrameMap, parsePreviewTime } from '../../lib/studioProject'
import { pixelMapToTuples } from '../../utils/frameRuntime'

const props = defineProps({
  project: {
    type: Object,
    required: true,
  },
  frameIndex: {
    type: Number,
    required: true,
  },
})

const canvasRef = ref(null)

function render() {
  const canvas = canvasRef.value
  if (!canvas) {
    return
  }

  const context = canvas.getContext('2d')
  const frameMap = composePixelFrameMap(props.project, props.frameIndex)
  const tuples = pixelMapToTuples(frameMap)
  const previewCanvas = buildPreviewFrameCanvas(tuples, buildCanvasTextLayout(props.project), parsePreviewTime(props.project))
  drawScaledCanvas(context, previewCanvas, canvas.width, canvas.height)
}

watch(
  () => [props.project, props.frameIndex],
  () => {
    render()
  },
  { deep: true, immediate: true },
)

onMounted(() => {
  render()
})
</script>

<template>
  <canvas ref="canvasRef" class="frame-mini-preview" width="112" height="112"></canvas>
</template>

<style scoped>
.frame-mini-preview {
  width: 100%;
  aspect-ratio: 1;
  border-radius: 2px;
  background: #050809;
  border: 1px solid rgba(255, 255, 255, 0.08);
  image-rendering: pixelated;
}
</style>
