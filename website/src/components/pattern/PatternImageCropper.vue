<template>
  <div v-if="visible" class="cropper-overlay">
    <div class="cropper-title-bar">
      <h3 class="cropper-title">裁一下</h3>
    </div>

    <div class="cropper-body">
      <div ref="containerRef" class="image-container">
        <img
          v-if="src"
          :src="src"
          class="crop-image"
          alt="裁剪图片"
          draggable="false"
          @load="handleImageLoad"
        >

        <div
          v-if="cropBoxStyle"
          class="crop-box"
          :style="cropBoxStyle"
          @pointerdown="onBoxPointerDown"
        >
          <div class="crop-border"></div>
          <div class="handle tl" @pointerdown.stop="onHandlePointerDown('tl', $event)"></div>
          <div class="handle tr" @pointerdown.stop="onHandlePointerDown('tr', $event)"></div>
          <div class="handle bl" @pointerdown.stop="onHandlePointerDown('bl', $event)"></div>
          <div class="handle br" @pointerdown.stop="onHandlePointerDown('br', $event)"></div>
        </div>

        <div v-if="cropBoxStyle" class="mask-top" :style="maskTopStyle"></div>
        <div v-if="cropBoxStyle" class="mask-bottom" :style="maskBottomStyle"></div>
        <div v-if="cropBoxStyle" class="mask-left" :style="maskLeftStyle"></div>
        <div v-if="cropBoxStyle" class="mask-right" :style="maskRightStyle"></div>
      </div>
    </div>

    <div class="cropper-footer">
      <button class="footer-btn cancel" type="button" @click="$emit('cancel')">
        返回
      </button>
      <button class="footer-btn confirm" type="button" @click="handleConfirm">
        用这张
      </button>
    </div>
  </div>
</template>

<script setup>
import { computed, onUnmounted, ref, watch } from 'vue'

const props = defineProps({
  visible: {
    type: Boolean,
    default: false,
  },
  src: {
    type: String,
    default: '',
  },
  initialCrop: {
    type: Object,
    default: null,
  },
})

const emit = defineEmits(['confirm', 'cancel'])

const containerRef = ref(null)
const imgW = ref(0)
const imgH = ref(0)
const dispW = ref(0)
const dispH = ref(0)
const offsetX = ref(0)
const offsetY = ref(0)
const containerW = ref(375)
const containerH = ref(500)
const cropX = ref(20)
const cropY = ref(20)
const cropW = ref(200)
const cropH = ref(200)
const activeDrag = ref(null)

function clampValue(value, min, max) {
  return Math.min(max, Math.max(min, value))
}

function resetCropBox() {
  const margin = 4
  cropX.value = offsetX.value + margin
  cropY.value = offsetY.value + margin
  cropW.value = Math.max(40, dispW.value - margin * 2)
  cropH.value = Math.max(40, dispH.value - margin * 2)
}

function applyInitialCrop() {
  if (!props.initialCrop || !imgW.value || !imgH.value || !dispW.value || !dispH.value) {
    resetCropBox()
    return
  }

  const scaleX = dispW.value / imgW.value
  const scaleY = dispH.value / imgH.value

  cropX.value = offsetX.value + props.initialCrop.x * scaleX
  cropY.value = offsetY.value + props.initialCrop.y * scaleY
  cropW.value = Math.max(30, props.initialCrop.width * scaleX)
  cropH.value = Math.max(30, props.initialCrop.height * scaleY)
}

function initCropBox(imageWidth, imageHeight) {
  imgW.value = imageWidth
  imgH.value = imageHeight

  const rect = containerRef.value?.getBoundingClientRect()
  if (!rect) {
    return
  }

  containerW.value = rect.width
  containerH.value = rect.height

  const scale = Math.min(rect.width / imageWidth, rect.height / imageHeight)
  dispW.value = imageWidth * scale
  dispH.value = imageHeight * scale
  offsetX.value = (rect.width - dispW.value) / 2
  offsetY.value = (rect.height - dispH.value) / 2

  applyInitialCrop()
}

function handleImageLoad(event) {
  const target = event.target
  initCropBox(target.naturalWidth || 1, target.naturalHeight || 1)
}

function startPointerDrag(type, pointerEvent) {
  activeDrag.value = {
    type,
    pointerId: pointerEvent.pointerId,
    startX: pointerEvent.clientX,
    startY: pointerEvent.clientY,
    cropX: cropX.value,
    cropY: cropY.value,
    cropW: cropW.value,
    cropH: cropH.value,
  }

  globalThis.addEventListener('pointermove', handlePointerMove)
  globalThis.addEventListener('pointerup', stopPointerDrag)
}

function onBoxPointerDown(event) {
  startPointerDrag('move', event)
}

function onHandlePointerDown(handle, event) {
  startPointerDrag(handle, event)
}

function handlePointerMove(event) {
  if (!activeDrag.value) {
    return
  }

  const deltaX = event.clientX - activeDrag.value.startX
  const deltaY = event.clientY - activeDrag.value.startY
  const minSize = 30
  let nextX = activeDrag.value.cropX
  let nextY = activeDrag.value.cropY
  let nextW = activeDrag.value.cropW
  let nextH = activeDrag.value.cropH

  if (activeDrag.value.type === 'move') {
    nextX = clampValue(
      activeDrag.value.cropX + deltaX,
      offsetX.value,
      offsetX.value + dispW.value - activeDrag.value.cropW,
    )
    nextY = clampValue(
      activeDrag.value.cropY + deltaY,
      offsetY.value,
      offsetY.value + dispH.value - activeDrag.value.cropH,
    )

    cropX.value = nextX
    cropY.value = nextY
    return
  }

  if (activeDrag.value.type.includes('l')) {
    nextX = activeDrag.value.cropX + deltaX
    nextW = activeDrag.value.cropW - deltaX
  }

  if (activeDrag.value.type.includes('r')) {
    nextW = activeDrag.value.cropW + deltaX
  }

  if (activeDrag.value.type.includes('t')) {
    nextY = activeDrag.value.cropY + deltaY
    nextH = activeDrag.value.cropH - deltaY
  }

  if (activeDrag.value.type.includes('b')) {
    nextH = activeDrag.value.cropH + deltaY
  }

  if (nextW < minSize) {
    nextW = minSize
    if (activeDrag.value.type.includes('l')) {
      nextX = activeDrag.value.cropX + activeDrag.value.cropW - minSize
    }
  }

  if (nextH < minSize) {
    nextH = minSize
    if (activeDrag.value.type.includes('t')) {
      nextY = activeDrag.value.cropY + activeDrag.value.cropH - minSize
    }
  }

  nextX = Math.max(offsetX.value, nextX)
  nextY = Math.max(offsetY.value, nextY)

  if (nextX + nextW > offsetX.value + dispW.value) {
    nextW = offsetX.value + dispW.value - nextX
  }

  if (nextY + nextH > offsetY.value + dispH.value) {
    nextH = offsetY.value + dispH.value - nextY
  }

  cropX.value = nextX
  cropY.value = nextY
  cropW.value = nextW
  cropH.value = nextH
}

function stopPointerDrag() {
  activeDrag.value = null
  globalThis.removeEventListener('pointermove', handlePointerMove)
  globalThis.removeEventListener('pointerup', stopPointerDrag)
}

function handleConfirm() {
  if (!imgW.value || !imgH.value || !dispW.value || !dispH.value) {
    return
  }

  const scaleX = imgW.value / dispW.value
  const scaleY = imgH.value / dispH.value
  const sx = Math.round((cropX.value - offsetX.value) * scaleX)
  const sy = Math.round((cropY.value - offsetY.value) * scaleY)
  const sw = Math.round(cropW.value * scaleX)
  const sh = Math.round(cropH.value * scaleY)

  emit('confirm', {
    sx,
    sy,
    sw,
    sh,
    src: props.src,
  })
}

const cropBoxStyle = computed(() => ({
  left: `${cropX.value}px`,
  top: `${cropY.value}px`,
  width: `${cropW.value}px`,
  height: `${cropH.value}px`,
}))

const maskTopStyle = computed(() => ({
  left: '0',
  top: '0',
  right: '0',
  height: `${cropY.value}px`,
}))

const maskBottomStyle = computed(() => {
  const bottom = containerH.value - cropY.value - cropH.value
  return {
    left: '0',
    bottom: '0',
    right: '0',
    height: `${Math.max(0, bottom)}px`,
  }
})

const maskLeftStyle = computed(() => ({
  left: '0',
  top: `${cropY.value}px`,
  width: `${cropX.value}px`,
  height: `${cropH.value}px`,
}))

const maskRightStyle = computed(() => {
  const right = containerW.value - cropX.value - cropW.value
  return {
    top: `${cropY.value}px`,
    right: '0',
    width: `${Math.max(0, right)}px`,
    height: `${cropH.value}px`,
  }
})

watch(
  () => [props.visible, props.src, props.initialCrop],
  ([visible]) => {
    if (!visible) {
      stopPointerDrag()
    }
  },
)

onUnmounted(() => {
  stopPointerDrag()
})
</script>

<style scoped>
.cropper-overlay {
  position: fixed;
  inset: 0;
  z-index: 90;
  display: flex;
  flex-direction: column;
  background: #ffffff;
}

.cropper-title-bar {
  height: 52px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 18px;
  border-bottom: 1px solid #e1e8f2;
}

.cropper-title {
  color: #24364d;
  font-size: 18px;
  font-weight: 700;
}

.cropper-body {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px 24px;
  position: relative;
  overflow: hidden;
  background: #f3f6fb;
}

.image-container {
  width: min(960px, 100%);
  height: min(76vh, 820px);
  max-height: 100%;
  position: relative;
  border-radius: 24px;
  overflow: hidden;
  box-shadow: 0 16px 32px rgba(18, 32, 51, 0.12);
}

.crop-image {
  width: 100%;
  height: 100%;
  object-fit: contain;
  user-select: none;
}

.crop-box {
  position: absolute;
  z-index: 10;
  touch-action: none;
}

.crop-border {
  position: absolute;
  inset: 0;
  border: 2px solid rgba(0, 120, 255, 0.82);
  box-sizing: border-box;
}

.handle {
  position: absolute;
  width: 20px;
  height: 20px;
  z-index: 11;
}

.handle::after {
  content: '';
  position: absolute;
  width: 12px;
  height: 12px;
  border-radius: 999px;
  background: #4f7fff;
  box-shadow: 0 0 0 3px rgba(79, 127, 255, 0.16);
}

.handle.tl {
  top: -10px;
  left: -10px;
}

.handle.tl::after {
  top: 0;
  left: 0;
}

.handle.tr {
  top: -10px;
  right: -10px;
}

.handle.tr::after {
  top: 0;
  right: 0;
}

.handle.bl {
  bottom: -10px;
  left: -10px;
}

.handle.bl::after {
  bottom: 0;
  left: 0;
}

.handle.br {
  bottom: -10px;
  right: -10px;
}

.handle.br::after {
  bottom: 0;
  right: 0;
}

.mask-top,
.mask-bottom,
.mask-left,
.mask-right {
  position: absolute;
  background: rgba(255, 255, 255, 0.52);
  z-index: 5;
}

.cropper-footer {
  display: flex;
  gap: 16px;
  padding: 12px 18px 16px;
  border-top: 1px solid #e1e8f2;
}

.footer-btn {
  flex: 1;
  height: 48px;
  border: none;
  border-radius: 14px;
  font-size: 15px;
  cursor: pointer;
}

.footer-btn.cancel {
  background: #eef2f7;
  color: #5d7086;
}

.footer-btn.confirm {
  background: #4f7fff;
  color: #ffffff;
  font-weight: 700;
}
</style>
