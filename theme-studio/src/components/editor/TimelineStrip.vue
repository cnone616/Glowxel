<script setup>
import { computed, nextTick, ref, watch } from 'vue'
import FrameMiniPreview from './FrameMiniPreview.vue'
import { FRAME_TYPE_FULL } from '../../utils/themeContract'

const props = defineProps({
  project: {
    type: Object,
    required: true,
  },
  selectedFrameIndex: {
    type: Number,
    required: true,
  },
  playheadFrameIndex: {
    type: Number,
    required: true,
  },
})

defineEmits(['select'])

const frameRefs = ref([])

const frames = computed(() => props.project.timeline.frames)

function setFrameRef(element, frameIndex) {
  frameRefs.value[frameIndex] = element
}

function buildFrameRole(frameIndex) {
  if (props.project.timeline.introEnabled && frameIndex <= props.project.timeline.introEndIndex) {
    return 'Intro'
  }

  if (frameIndex >= props.project.timeline.loopStartIndex) {
    return 'Loop'
  }

  return '普通'
}

watch(
  () => props.selectedFrameIndex,
  async (frameIndex) => {
    await nextTick()
    frameRefs.value[frameIndex]?.scrollIntoView({ behavior: 'smooth', block: 'nearest', inline: 'center' })
  },
  { immediate: true },
)
</script>

<template>
  <div class="timeline-strip">
    <article
      v-for="(frame, frameIndex) in frames"
      :key="frame.id"
      :ref="(element) => setFrameRef(element, frameIndex)"
      class="frame-card"
      :class="{
        active: frameIndex === selectedFrameIndex,
        playing: frameIndex === playheadFrameIndex,
      }"
      @click="$emit('select', frameIndex)"
    >
      <div class="frame-top">
        <strong>F{{ frameIndex + 1 }}</strong>
        <span class="frame-role">{{ buildFrameRole(frameIndex) }}</span>
      </div>
      <FrameMiniPreview :project="project" :frame-index="frameIndex" />
      <div class="frame-meta">
        <span>{{ frame.delay }}ms</span>
        <span>{{ frame.type === FRAME_TYPE_FULL ? '全量' : '差分' }}</span>
      </div>
      <div class="frame-flags">
        <span v-if="project.timeline.introEnabled && frameIndex === project.timeline.introEndIndex">Intro 终点</span>
        <span v-if="frameIndex === project.timeline.loopStartIndex">Loop 起点</span>
        <span v-if="frameIndex === playheadFrameIndex">播放头</span>
      </div>
    </article>
  </div>
</template>

<style scoped>
.timeline-strip {
  display: grid;
  grid-auto-flow: column;
  grid-auto-columns: minmax(132px, 132px);
  gap: 8px;
  overflow-x: auto;
  padding-bottom: 4px;
}

.frame-card {
  display: grid;
  gap: 6px;
  padding: 6px;
  border: 1px solid rgba(255, 255, 255, 0.08);
  background: rgba(255, 255, 255, 0.03);
  border-radius: 6px;
  cursor: pointer;
}

.frame-card.active {
  border-color: rgba(102, 234, 214, 0.42);
  background: rgba(102, 234, 214, 0.08);
}

.frame-card.playing {
  box-shadow: inset 0 0 0 1px rgba(255, 191, 105, 0.3);
}

.frame-top,
.frame-meta,
.frame-flags {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  align-items: center;
  justify-content: space-between;
}

.frame-top strong {
  font-size: 13px;
}

.frame-role,
.frame-meta span,
.frame-flags span {
  min-height: 22px;
  display: inline-flex;
  align-items: center;
  padding: 0 6px;
  background: rgba(255, 255, 255, 0.04);
  color: var(--text-dim);
  font-size: 11px;
}

.frame-flags {
  justify-content: flex-start;
}
</style>
