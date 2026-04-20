<script setup>
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { buildPreviewFrameCanvas, drawScaledCanvas } from '../../utils/canvasRenderer'
import { buildCanvasTextLayout, composePixelFrameMap, parsePreviewTime } from '../../lib/studioProject'
import { pixelMapToTuples } from '../../utils/frameRuntime'

const props = defineProps({
  project: {
    type: Object,
    required: true,
  },
  playing: {
    type: Boolean,
    required: true,
  },
  selectedFrameIndex: {
    type: Number,
    required: true,
  },
})

const emit = defineEmits(['frame-change', 'scrub'])

const canvasRef = ref(null)
const activeFrameIndex = ref(0)
let timerId = 0
let playbackIndex = 0

const frameMax = computed(() => {
  return Math.max(0, props.project.timeline.frames.length - 1)
})

function drawFrame(frameIndex) {
  const canvas = canvasRef.value
  if (!canvas) {
    return
  }

  const context = canvas.getContext('2d')
  const frameMap = composePixelFrameMap(props.project, frameIndex)
  const tuples = pixelMapToTuples(frameMap)
  const previewCanvas = buildPreviewFrameCanvas(tuples, buildCanvasTextLayout(props.project), parsePreviewTime(props.project))
  drawScaledCanvas(context, previewCanvas, canvas.width, canvas.height)
  activeFrameIndex.value = frameIndex
  emit('frame-change', frameIndex)
}

function nextFrameIndex(currentIndex) {
  const lastIndex = props.project.timeline.frames.length - 1
  const loopStartIndex = props.project.timeline.loopStartIndex

  if (currentIndex >= lastIndex) {
    return loopStartIndex
  }

  return currentIndex + 1
}

function stopPlayback() {
  if (timerId) {
    window.clearTimeout(timerId)
    timerId = 0
  }
}

function schedulePlayback() {
  stopPlayback()

  if (!props.playing) {
    drawFrame(props.selectedFrameIndex)
    return
  }

  playbackIndex = props.selectedFrameIndex

  const tick = () => {
    drawFrame(playbackIndex)
    const delay = props.project.timeline.frames[playbackIndex].delay
    playbackIndex = nextFrameIndex(playbackIndex)
    timerId = window.setTimeout(tick, delay)
  }

  tick()
}

function handleScrub(event) {
  emit('scrub', Number(event.target.value))
}

watch(
  () => [props.project, props.playing, props.selectedFrameIndex],
  () => {
    schedulePlayback()
  },
  { deep: true, immediate: true },
)

onMounted(() => {
  schedulePlayback()
})

onBeforeUnmount(() => {
  stopPlayback()
})
</script>

<template>
  <div class="preview-shell">
    <div class="preview-head">
      <span>输出 F{{ activeFrameIndex + 1 }}</span>
      <span>{{ props.playing ? '动态播放中' : '锁定编辑帧' }}</span>
    </div>
    <canvas ref="canvasRef" class="preview-canvas" width="320" height="320"></canvas>
    <div class="preview-meta">
      <span>编辑 F{{ selectedFrameIndex + 1 }}</span>
      <span>输出 F{{ activeFrameIndex + 1 }}</span>
    </div>
    <input
      class="preview-scrubber"
      type="range"
      min="0"
      :max="frameMax"
      :value="activeFrameIndex"
      @input="handleScrub"
    />
  </div>
</template>

<style scoped>
.preview-shell {
  display: grid;
  gap: 8px;
}

.preview-head,
.preview-meta {
  display: flex;
  gap: 6px;
  flex-wrap: wrap;
}

.preview-head span,
.preview-meta span {
  min-height: 28px;
  display: inline-flex;
  align-items: center;
  padding: 0 8px;
  border: 1px solid rgba(255, 255, 255, 0.06);
  background: rgba(255, 255, 255, 0.03);
  color: var(--text-soft);
  font-size: 12px;
}

.preview-canvas {
  width: 100%;
  aspect-ratio: 1;
  border-radius: 2px;
  border: 1px solid rgba(255, 255, 255, 0.12);
  image-rendering: pixelated;
  background: #050809;
}

.preview-scrubber {
  width: 100%;
}
</style>
