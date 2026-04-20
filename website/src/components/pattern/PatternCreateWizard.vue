<template>
  <div v-if="visible" class="wizard-mask">
    <div class="wizard-shell">
      <div class="wizard-head">
        <button class="nav-btn" type="button" @click="handleBack">
          {{ step === 0 ? '关闭' : '返回' }}
        </button>
        <h2>{{ mode === 'image' ? '图片生成' : '空白画板' }}</h2>
        <span class="step-mark">步骤 {{ step + 1 }}</span>
      </div>

      <div class="wizard-body">
        <div v-if="step === 0" class="step-content">
          <div class="field-panel">
            <label class="field-label">画布名称</label>
            <input
              v-model="name"
              type="text"
              class="text-input"
              placeholder="例如：2026新年快乐"
            >
          </div>
          <p class="step-hint">先给这次创建起个名字，和 uniapp 一样从名称开始。</p>
        </div>

        <div v-else-if="step === 1" class="step-content">
          <template v-if="mode === 'image'">
            <div class="field-panel">
              <div v-if="!previewUrl" class="upload-area" @click="chooseImage">
                <div class="upload-icon">上传</div>
                <strong>点击上传图片</strong>
                <span>支持 JPG、JPEG、PNG、WEBP</span>
              </div>

              <div v-else class="preview-area">
                <img :src="previewUrl" alt="预览图片" class="preview-image">
                <div class="preview-actions">
                  <button class="secondary-btn" type="button" @click="chooseImage">更换图片</button>
                  <button class="secondary-btn" type="button" @click="openCropper">重新裁剪</button>
                  <button class="ghost-btn" type="button" @click="clearImage">删除</button>
                </div>
              </div>
            </div>

            <button
              v-if="recommendedSize"
              class="recommend-tip"
              type="button"
              @click="applyRecommendedSize"
            >
              检测到内容尺寸，推荐 {{ recommendedSize.width }} × {{ recommendedSize.height }}
            </button>
          </template>

          <div class="field-panel">
            <div class="size-header">
              <h3>{{ mode === 'image' ? '目标尺寸（像素化）' : '画布尺寸' }}</h3>
              <span v-if="contentRatio">{{ contentRatio.w }}:{{ contentRatio.h }}</span>
            </div>

            <div class="size-inputs">
              <label class="size-field">
                <span>宽度</span>
                <input v-model.number="customWidth" type="number" min="1" :max="PATTERN_MAX_DIMENSION" @input="handleSizeInput">
              </label>
              <label class="size-field">
                <span>高度</span>
                <input v-model.number="customHeight" type="number" min="1" :max="PATTERN_MAX_DIMENSION" @input="handleSizeInput">
              </label>
            </div>

            <div class="preset-grid">
              <button
                v-for="preset in defaultSizePresets"
                :key="`${preset.width}-${preset.height}`"
                class="preset-chip"
                :class="{ active: customWidth === preset.width && customHeight === preset.height }"
                type="button"
                @click="selectPresetSize(preset.width, preset.height)"
              >
                {{ preset.width }} × {{ preset.height }}
              </button>
            </div>

            <div class="stats-box">
              <div class="stat-row">
                <span>板子布局</span>
                <strong>{{ boardsX }} × {{ boardsY }}（{{ totalBoards }}块）</strong>
              </div>
              <div class="stat-row">
                <span>总像素数</span>
                <strong>{{ (customWidth * customHeight).toLocaleString() }}</strong>
              </div>
            </div>
          </div>
        </div>

        <div v-else-if="step === 2" class="step-content">
          <div class="field-panel">
            <h3>选择 Artkal 拼豆套装</h3>
            <div class="preset-card-grid">
              <button
                v-for="preset in ARTKAL_PRESETS"
                :key="preset.key"
                class="preset-card"
                :class="{ active: selectedPreset === preset.key }"
                type="button"
                @click="selectedPreset = preset.key"
              >
                <strong>{{ preset.name }}</strong>
                <span>{{ preset.count }} 种颜色</span>
              </button>
            </div>
            <div class="stats-box">
              <div class="stat-row">
                <span>已选择套装</span>
                <strong>{{ selectedColorCodes.length }} 种颜色</strong>
              </div>
              <div class="stat-row">
                <span>当前模式</span>
                <strong>{{ mode === 'image' ? '图片生成' : '空白画板' }}</strong>
              </div>
            </div>
          </div>
        </div>

        <div v-else class="step-content">
          <div class="field-panel">
            <div class="size-header">
              <h3>预览效果</h3>
              <span>确认后进入工作台</span>
            </div>

            <div v-if="previewLoading" class="status-box">正在生成预览...</div>

            <div v-else-if="previewDocument" class="preview-grid-shell">
              <div
                class="preview-grid"
                :style="{ gridTemplateColumns: `repeat(${previewDocument.width}, minmax(0, 1fr))` }"
              >
                <span
                  v-for="cell in previewCells"
                  :key="cell.key"
                  class="preview-pixel"
                  :style="{ backgroundColor: cell.color }"
                ></span>
              </div>
            </div>

            <div v-else class="status-box">先生成预览再确认。</div>
          </div>

          <div v-if="usedColors.length" class="field-panel">
            <div class="size-header">
              <h3>使用的颜色</h3>
              <span>共 {{ usedColors.reduce((sum, item) => sum + item.count, 0) }} 个</span>
            </div>
            <div class="used-colors-grid">
              <div v-for="color in usedColors" :key="color.code" class="used-color-item">
                <i :style="{ backgroundColor: color.hex }"></i>
                <strong>{{ color.code }}</strong>
                <span>{{ color.count }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div class="wizard-footer">
        <button
          class="primary-btn wide-btn"
          type="button"
          :disabled="!canProceed || previewLoading"
          @click="handleNext"
        >
          {{ buttonText }}
        </button>
      </div>

      <input
        ref="fileInputRef"
        type="file"
        accept=".jpg,.jpeg,.png,.webp,image/jpeg,image/png,image/webp"
        class="hidden-input"
        @change="handleFileChange"
      >

      <PatternImageCropper
        :visible="showCropper"
        :src="cropperSrc"
        @cancel="showCropper = false"
        @confirm="handleCropConfirm"
      />
    </div>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import { ARTKAL_OFFICIAL_SETS, ARTKAL_PRESETS, getColorByCode } from '@/data/artkal-colors.js'
import PatternImageCropper from '@/components/pattern/PatternImageCropper.vue'
import {
  PATTERN_BOARD_SIZE,
  PATTERN_DEFAULT_DIMENSION,
  PATTERN_MAX_DIMENSION,
  PATTERN_SIZE_PRESETS,
} from '@/modules/pattern-workbench/core/patternBoard.js'

const props = defineProps({
  visible: {
    type: Boolean,
    default: false,
  },
  mode: {
    type: String,
    default: 'blank',
  },
  previewDocument: {
    type: Object,
    default: null,
  },
  previewReport: {
    type: Object,
    default: null,
  },
  previewLoading: {
    type: Boolean,
    default: false,
  },
})

const emit = defineEmits(['cancel', 'preview', 'confirm'])

const fileInputRef = ref(null)
const step = ref(0)
const name = ref('')
const customWidth = ref(PATTERN_DEFAULT_DIMENSION)
const customHeight = ref(PATTERN_DEFAULT_DIMENSION)
const selectedPreset = ref('set24')
const previewUrl = ref('')
const originalImageUrl = ref('')
const originalImageName = ref('')
const imageFile = ref(null)
const cropperSrc = ref('')
const showCropper = ref(false)
const recommendedSize = ref(null)
const contentRatio = ref(null)
const generatedSignature = ref('')

const defaultSizePresets = PATTERN_SIZE_PRESETS

function resetForm() {
  step.value = 0
  name.value = ''
  customWidth.value = PATTERN_DEFAULT_DIMENSION
  customHeight.value = PATTERN_DEFAULT_DIMENSION
  selectedPreset.value = 'set24'
  previewUrl.value = ''
  originalImageUrl.value = ''
  originalImageName.value = ''
  imageFile.value = null
  cropperSrc.value = ''
  showCropper.value = false
  recommendedSize.value = null
  contentRatio.value = null
  generatedSignature.value = ''
}

function gcd(left, right) {
  let a = left
  let b = right
  while (b) {
    [a, b] = [b, a % b]
  }
  return a
}

function applyDetectedSize(width, height) {
  const divisor = gcd(width, height)
  contentRatio.value = {
    w: width / divisor,
    h: height / divisor,
  }
  recommendedSize.value = { width, height }
}

function detectImageContentSize(imageUrl) {
  const img = new Image()
  img.onload = () => {
    applyDetectedSize(img.width, img.height)
  }
  img.src = imageUrl
}

function dataUrlToFile(dataUrl, fileName) {
  const parts = dataUrl.split(',')
  const match = parts[0].match(/:(.*?);/)
  const mime = match ? match[1] : 'image/png'
  const binary = atob(parts[1])
  let length = binary.length
  const buffer = new Uint8Array(length)

  while (length -= 1) {
    buffer[length] = binary.charCodeAt(length)
  }

  return new File([buffer], fileName, { type: mime })
}

function chooseImage() {
  fileInputRef.value?.click()
}

function handleSizeInput() {
  if (customWidth.value < 1) {
    customWidth.value = 1
  }
  if (customHeight.value < 1) {
    customHeight.value = 1
  }
  if (customWidth.value > PATTERN_MAX_DIMENSION) {
    customWidth.value = PATTERN_MAX_DIMENSION
  }
  if (customHeight.value > PATTERN_MAX_DIMENSION) {
    customHeight.value = PATTERN_MAX_DIMENSION
  }
}

function selectPresetSize(width, height) {
  customWidth.value = width
  customHeight.value = height
}

function clearImage() {
  previewUrl.value = ''
  originalImageUrl.value = ''
  originalImageName.value = ''
  imageFile.value = null
  cropperSrc.value = ''
  showCropper.value = false
  recommendedSize.value = null
  contentRatio.value = null
  if (fileInputRef.value) {
    fileInputRef.value.value = ''
  }
}

function openCropper() {
  if (!originalImageUrl.value) {
    return
  }
  cropperSrc.value = originalImageUrl.value
  showCropper.value = true
}

function handleFileChange(event) {
  const file = event.target.files?.[0]
  if (!file) {
    return
  }

  originalImageName.value = file.name

  const reader = new FileReader()
  reader.onload = () => {
    originalImageUrl.value = String(reader.result || '')
    cropperSrc.value = originalImageUrl.value
    showCropper.value = true
  }
  reader.readAsDataURL(file)
}

function handleCropConfirm(cropData) {
  const img = new Image()
  img.onload = () => {
    const canvas = globalThis.document.createElement('canvas')
    canvas.width = cropData.sw
    canvas.height = cropData.sh
    const context = canvas.getContext('2d')

    if (!context) {
      return
    }

    context.drawImage(
      img,
      cropData.sx,
      cropData.sy,
      cropData.sw,
      cropData.sh,
      0,
      0,
      cropData.sw,
      cropData.sh,
    )

    const dataUrl = canvas.toDataURL('image/png')
    previewUrl.value = dataUrl
    imageFile.value = dataUrlToFile(dataUrl, originalImageName.value || 'cropped-image.png')
    showCropper.value = false
    detectImageContentSize(dataUrl)
  }
  img.src = cropData.src
}

function applyRecommendedSize() {
  if (!recommendedSize.value) {
    return
  }

  customWidth.value = recommendedSize.value.width
  customHeight.value = recommendedSize.value.height
}

function resolveDimension(value) {
  const numericValue = Number(value)
  if (!Number.isFinite(numericValue) || numericValue <= 0) {
    return PATTERN_DEFAULT_DIMENSION
  }
  return numericValue
}

function buildPayload() {
  return {
    mode: props.mode,
    name: name.value.trim(),
    width: resolveDimension(customWidth.value),
    height: resolveDimension(customHeight.value),
    presetKey: selectedPreset.value,
    colorCodes: selectedColorCodes.value,
    imageFile: imageFile.value,
  }
}

function payloadSignatureValue() {
  const payload = buildPayload()
  return JSON.stringify({
    mode: payload.mode,
    name: payload.name,
    width: payload.width,
    height: payload.height,
    presetKey: payload.presetKey,
    imageName: payload.imageFile?.name || '',
    imageSize: payload.imageFile?.size || 0,
    imageTime: payload.imageFile?.lastModified || 0,
  })
}

function handleBack() {
  if (step.value === 0) {
    emit('cancel')
    return
  }
  step.value -= 1
}

function handleNext() {
  if (!canProceed.value || props.previewLoading) {
    return
  }

  if (props.mode === 'blank') {
    if (step.value === 0 || step.value === 1) {
      step.value += 1
      return
    }

    emit('confirm', buildPayload())
    return
  }

  if (step.value === 0 || step.value === 1) {
    step.value += 1
    return
  }

  if (step.value === 2) {
    if (!previewReady.value) {
      emit('preview', buildPayload())
      return
    }
    step.value = 3
    return
  }

  emit('confirm', buildPayload())
}

const paddedWidth = computed(() => Math.ceil(customWidth.value / PATTERN_BOARD_SIZE) * PATTERN_BOARD_SIZE)
const paddedHeight = computed(() => Math.ceil(customHeight.value / PATTERN_BOARD_SIZE) * PATTERN_BOARD_SIZE)
const boardsX = computed(() => Math.ceil(customWidth.value / PATTERN_BOARD_SIZE))
const boardsY = computed(() => Math.ceil(customHeight.value / PATTERN_BOARD_SIZE))
const totalBoards = computed(() => boardsX.value * boardsY.value)
const selectedColorCodes = computed(() => ARTKAL_OFFICIAL_SETS[selectedPreset.value]?.colors || [])
const canProceed = computed(() => {
  if (step.value === 0) {
    return name.value.trim().length > 0
  }

  if (step.value === 1) {
    if (props.mode === 'image') {
      return !!imageFile.value && customWidth.value > 0 && customHeight.value > 0 && paddedWidth.value <= PATTERN_MAX_DIMENSION && paddedHeight.value <= PATTERN_MAX_DIMENSION
    }
    return customWidth.value > 0 && customHeight.value > 0 && paddedWidth.value <= PATTERN_MAX_DIMENSION && paddedHeight.value <= PATTERN_MAX_DIMENSION
  }

  return true
})

const buttonText = computed(() => {
  if (step.value === 0) {
    return '下一步'
  }

  if (step.value === 1) {
    return '选择颜色'
  }

  if (step.value === 2) {
    if (props.mode === 'image') {
      return props.previewLoading ? '正在生成中...' : '生成预览'
    }
    return '创建画板'
  }

  return props.previewLoading ? '生成中...' : '确认创建'
})

const previewReady = computed(() => {
  return !!props.previewDocument && generatedSignature.value === payloadSignatureValue()
})

const previewCells = computed(() => {
  if (!props.previewDocument) {
    return []
  }

  const cells = []
  for (let y = 0; y < props.previewDocument.height; y += 1) {
    for (let x = 0; x < props.previewDocument.width; x += 1) {
      const code = props.previewDocument.pixels.get(`${x},${y}`) || ''
      cells.push({
        key: `${x},${y}`,
        color: getColorByCode(code)?.hex || '#152131',
      })
    }
  }
  return cells
})

const usedColors = computed(() => {
  if (!props.previewDocument) {
    return []
  }

  const counts = new Map()
  props.previewDocument.pixels.forEach((code) => {
    counts.set(code, (counts.get(code) || 0) + 1)
  })

  return Array.from(counts.entries())
    .map(([code, count]) => ({
      code,
      count,
      hex: getColorByCode(code)?.hex || '#152131',
    }))
    .sort((left, right) => right.count - left.count)
})

watch(
  () => props.visible,
  (visible) => {
    if (!visible) {
      resetForm()
      return
    }

    resetForm()
  },
  { immediate: true },
)

watch(
  () => props.previewReport,
  (report) => {
    if (!report) {
      return
    }
    generatedSignature.value = payloadSignatureValue()
    if (props.mode === 'image' && step.value === 2) {
      step.value = 3
    }
  },
)
</script>

<style scoped>
.wizard-mask {
  position: fixed;
  inset: 0;
  z-index: 70;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
  background: rgba(15, 27, 43, 0.56);
  backdrop-filter: blur(10px);
}

.wizard-shell {
  width: min(980px, 100%);
  max-height: calc(100vh - 40px);
  overflow: auto;
  border-radius: 28px;
  background: #ffffff;
  box-shadow: 0 28px 80px rgba(18, 32, 51, 0.26);
}

.wizard-head,
.wizard-footer,
.preview-actions,
.size-header,
.stat-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: center;
}

.wizard-head {
  padding: 20px 24px;
  border-bottom: 1px solid #e3ebf5;
}

.wizard-head h2 {
  color: #20324a;
  font-size: 22px;
}

.step-mark {
  color: #7b889f;
  font-size: 13px;
}

.wizard-body {
  padding: 24px;
}

.step-content {
  display: grid;
  gap: 18px;
}

.field-panel {
  display: grid;
  gap: 14px;
  padding: 18px;
  border-radius: 22px;
  border: 1px solid #dbe4f2;
  background: #f8fbff;
}

.field-panel h3,
.field-label {
  color: #20324a;
  font-size: 18px;
  font-weight: 700;
}

.text-input,
.size-field input {
  height: 46px;
  padding: 0 14px;
  border: 1px solid #dbe4f2;
  border-radius: 14px;
  background: #ffffff;
  color: #20324a;
}

.step-hint {
  color: #6a7890;
  font-size: 14px;
  line-height: 1.7;
}

.upload-area {
  display: grid;
  place-items: center;
  gap: 10px;
  min-height: 220px;
  border: 2px dashed #b7c8df;
  border-radius: 22px;
  background: #ffffff;
  cursor: pointer;
}

.upload-icon {
  width: 72px;
  height: 72px;
  border-radius: 999px;
  display: grid;
  place-items: center;
  background: #4f7fff;
  color: var(--nb-ink);
  font-weight: 700;
}

.upload-area strong {
  color: #20324a;
  font-size: 18px;
}

.upload-area span {
  color: #7b889f;
  font-size: 13px;
}

.preview-image {
  width: 100%;
  max-height: 360px;
  object-fit: contain;
  border-radius: 18px;
  background: #ffffff;
}

.recommend-tip {
  height: 44px;
  padding: 0 16px;
  border: 1px solid #b6d3ff;
  border-radius: 14px;
  background: #eef6ff;
  color: #28599a;
  text-align: left;
  cursor: pointer;
}

.size-inputs {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.size-field {
  display: grid;
  gap: 8px;
}

.size-field span {
  color: #20324a;
  font-size: 14px;
  font-weight: 700;
}

.preset-grid,
.preset-card-grid,
.used-colors-grid {
  display: grid;
  gap: 10px;
}

.preset-grid {
  grid-template-columns: repeat(auto-fill, minmax(100px, 1fr));
}

.preset-chip {
  height: 40px;
  border: 1px solid #dbe4f2;
  border-radius: 12px;
  background: #ffffff;
  color: #3a4c64;
  cursor: pointer;
}

.preset-chip.active,
.preset-card.active {
  border-color: #7fb2ff;
  background: #eef6ff;
}

.stats-box {
  display: grid;
  gap: 8px;
  padding: 14px;
  border-radius: 16px;
  background: #ffffff;
  border: 1px solid #dbe4f2;
}

.preset-card-grid {
  grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
}

.preset-card {
  display: grid;
  gap: 6px;
  padding: 16px;
  border: 1px solid #dbe4f2;
  border-radius: 16px;
  background: #ffffff;
  color: #20324a;
  text-align: left;
  cursor: pointer;
}

.preset-card span,
.size-header span,
.used-color-item span,
.stat-row span {
  color: #7b889f;
  font-size: 13px;
}

.preview-grid-shell {
  padding: 14px;
  border-radius: 18px;
  background: #182331;
}

.preview-grid {
  display: grid;
  gap: 2px;
}

.preview-pixel {
  display: block;
  aspect-ratio: 1;
  border-radius: 2px;
}

.used-colors-grid {
  grid-template-columns: repeat(auto-fill, minmax(110px, 1fr));
}

.used-color-item {
  display: grid;
  gap: 6px;
  padding: 12px;
  border-radius: 14px;
  background: #ffffff;
  border: 1px solid #dbe4f2;
}

.used-color-item i {
  width: 18px;
  height: 18px;
  border-radius: 999px;
  border: 1px solid rgba(18, 32, 51, 0.08);
}

.used-color-item strong,
.stat-row strong {
  color: #20324a;
  font-size: 14px;
}

.status-box {
  padding: 16px;
  border-radius: 16px;
  border: 1px solid #dbe4f2;
  background: #ffffff;
  color: #6a7890;
}

.wizard-footer {
  padding: 0 24px 24px;
}

.wide-btn {
  width: 100%;
  height: 48px;
}

.nav-btn,
.secondary-btn,
.ghost-btn,
.primary-btn {
  height: 42px;
  padding: 0 16px;
  border-radius: 12px;
  cursor: pointer;
}

.nav-btn,
.secondary-btn,
.ghost-btn {
  border: 1px solid #dbe4f2;
  background: #ffffff;
  color: #4f6078;
}

.primary-btn {
  border: none;
  background: #4f7fff;
  color: var(--nb-ink);
  font-weight: 700;
}

.primary-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.hidden-input {
  display: none;
}

@media (max-width: 768px) {
  .wizard-mask {
    padding: 12px;
  }

  .wizard-shell {
    border-radius: 20px;
  }

  .wizard-head,
  .wizard-footer,
  .preview-actions,
  .size-header {
    flex-direction: column;
    align-items: stretch;
  }

  .size-inputs {
    grid-template-columns: 1fr;
  }
}
</style>
