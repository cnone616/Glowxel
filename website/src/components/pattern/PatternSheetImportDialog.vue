<template>
  <div v-if="visible" class="sheet-dialog-mask">
    <div class="sheet-dialog">
      <div class="dialog-head">
        <div>
          <h2>{{ modeTitle }}</h2>
        </div>
        <button class="ghost-btn" type="button" @click="$emit('cancel')">
          取消
        </button>
      </div>

      <div
        class="step-strip"
        :style="{ gridTemplateColumns: `repeat(${stepItems.length}, minmax(0, 1fr))` }"
      >
        <button
          v-for="item in stepItems"
          :key="item.value"
          class="step-pill"
          :class="{ active: currentStep === item.value, done: currentStep > item.value }"
          type="button"
          @click="jumpToStep(item.value)"
        >
          <span>{{ item.index }}</span>
          <strong>{{ item.label }}</strong>
        </button>
      </div>

      <div v-if="currentStep === 1" class="step-panel">
        <div class="section-head">
          <h3>裁图</h3>
          <span>{{ cropSummary }}</span>
        </div>

        <div v-if="mode === 'sheet'" class="crop-compare-layout">
          <div class="compare-card">
            <div class="compare-head">
              <strong>原图</strong>
            </div>
            <div class="preview-stage compare-stage">
              <img
                v-if="imageUrl"
                ref="stageImageRef"
                :src="imageUrl"
                alt="导入原图"
                draggable="false"
                @load="handleStageImageLoad"
              >
              <div
                v-if="cropFrameStyle"
                class="crop-frame locked"
                :style="cropFrameStyle"
              ></div>
            </div>
          </div>

          <div class="compare-card">
            <div class="compare-head">
              <strong>裁后图</strong>
            </div>
            <div class="preview-stage compare-stage">
              <img
                v-if="cropPreviewUrl"
                class="cropped-preview-image"
                :src="cropPreviewUrl"
                alt="裁后图"
                draggable="false"
              >
              <div v-else class="empty-state compact">先裁图</div>
            </div>
          </div>
        </div>

        <div v-else class="preview-stage">
          <img
            v-if="imageUrl"
            ref="stageImageRef"
            :src="imageUrl"
            alt="导入原图"
            draggable="false"
            @load="handleStageImageLoad"
          >
          <div
            v-if="cropFrameStyle"
            class="crop-frame locked"
            :style="cropFrameStyle"
          ></div>
        </div>

        <div class="selection-actions">
          <button class="secondary-btn" type="button" @click="openCropper">
            {{ cropRect ? '重新裁图' : '开始裁图' }}
          </button>
          <button class="secondary-btn" type="button" @click="resetCrop">
            整张都要
          </button>
          <div class="mini-summary">
            <span>已选 {{ cropRect?.width || 0 }} × {{ cropRect?.height || 0 }}</span>
          </div>
        </div>

        <template v-if="mode === 'image'">
          <div class="size-grid">
            <label class="field-card">
              <span>宽</span>
              <input v-model.number="targetWidth" type="number" min="8" :max="PATTERN_MAX_DIMENSION">
            </label>
            <label class="field-card">
              <span>高</span>
              <input v-model.number="targetHeight" type="number" min="8" :max="PATTERN_MAX_DIMENSION">
            </label>
          </div>
        </template>
      </div>

      <div v-else-if="currentStep === 2 && mode === 'sheet'" class="step-panel">
        <div class="section-head">
          <h3>对齐格子</h3>
          <span>{{ cellSummary }}</span>
        </div>

        <div class="calibration-layout">
          <div class="preview-stage full-grid-stage">
            <img
              v-if="sheetStageImageUrl"
              ref="stageImageRef"
              :src="sheetStageImageUrl"
              alt="单格定标"
              draggable="false"
              @load="handleStageImageLoad"
            >
          </div>

          <div class="zoom-panel">
            <div class="zoom-head">
              <strong>中间参考区</strong>
            </div>
            <div class="zoom-stage" :style="zoomStageStyle">
              <div
                v-if="zoomSurfaceStyle"
                class="zoom-surface"
                :style="zoomSurfaceStyle"
              >
                <div
                  v-if="zoomFocusStyle"
                  class="zoom-focus-block"
                  :style="zoomFocusStyle"
                >
                  <span
                    v-for="item in zoomVerticalDividerStyles"
                    :key="`v-${item.key}`"
                    class="focus-divider vertical"
                    :style="item.style"
                  ></span>
                  <span
                    v-for="item in zoomHorizontalDividerStyles"
                    :key="`h-${item.key}`"
                    class="focus-divider horizontal"
                    :style="item.style"
                  ></span>
                </div>
              </div>
            </div>
            <div class="zoom-move">
              <span class="zoom-tip">挪一挪</span>
              <div class="move-pad">
                <button class="move-btn up" type="button" @click="adjustBasisRow(-1)" aria-label="上移">
                  <span class="arrow-icon up"></span>
                </button>
                <button class="move-btn left" type="button" @click="adjustBasisColumn(-1)" aria-label="左移">
                  <span class="arrow-icon left"></span>
                </button>
                <button class="move-btn right" type="button" @click="adjustBasisColumn(1)" aria-label="右移">
                  <span class="arrow-icon right"></span>
                </button>
                <button class="move-btn down" type="button" @click="adjustBasisRow(1)" aria-label="下移">
                  <span class="arrow-icon down"></span>
                </button>
              </div>
            </div>
          </div>
        </div>

        <div class="field-stack">
          <label class="field-card wide-card">
            <span>格子宽高</span>
            <div class="stepper-row">
              <strong class="stepper-label">宽</strong>
              <div class="stepper-control">
                <button class="stepper-btn" type="button" @click="adjustCellWidth(-1)">-</button>
                <input
                  v-model.number="cellWidth"
                  class="stepper-input"
                  type="number"
                  min="4"
                  :max="maxCellWidth"
                >
                <span class="stepper-unit">px</span>
                <button class="stepper-btn" type="button" @click="adjustCellWidth(1)">+</button>
              </div>
            </div>
            <div class="stepper-row">
              <strong class="stepper-label">高</strong>
              <div class="stepper-control">
                <button class="stepper-btn" type="button" @click="adjustCellHeight(-1)">-</button>
                <input
                  v-model.number="cellHeight"
                  class="stepper-input"
                  type="number"
                  min="4"
                  :max="maxCellHeight"
                >
                <span class="stepper-unit">px</span>
                <button class="stepper-btn" type="button" @click="adjustCellHeight(1)">+</button>
              </div>
            </div>
          </label>
        </div>

        <div class="field-stack">
          <label class="field-card wide-card">
            <span>参考区</span>
            <div class="stepper-row">
              <strong class="stepper-label">一起看几格</strong>
              <div class="stepper-control">
                <button class="stepper-btn" type="button" @click="adjustFocusGridCount(-1)">-</button>
                <input
                  v-model.number="focusGridCount"
                  class="stepper-input"
                  type="number"
                  min="3"
                  max="8"
                >
                <span class="stepper-unit">格</span>
                <button class="stepper-btn" type="button" @click="adjustFocusGridCount(1)">+</button>
              </div>
            </div>
            <div class="stepper-row">
              <strong class="stepper-label">线粗细</strong>
              <div class="stepper-control">
                <button class="stepper-btn" type="button" @click="adjustFocusLineWidth(-1)">-</button>
                <input
                  v-model.number="focusLineWidth"
                  class="stepper-input"
                  type="number"
                  min="1"
                  max="6"
                >
                <span class="stepper-unit">px</span>
                <button class="stepper-btn" type="button" @click="adjustFocusLineWidth(1)">+</button>
              </div>
            </div>
          </label>
        </div>

        <div class="field-stack">
          <div class="field-card wide-card">
            <span>边缘微调</span>
            <div class="trim-grid">
              <button class="trim-btn" type="button" @click="trimCrop('left')">
                <span class="arrow-icon left"></span>
                <span>左边减一点</span>
              </button>
              <button class="trim-btn" type="button" @click="trimCrop('right')">
                <span class="arrow-icon right"></span>
                <span>右边减一点</span>
              </button>
              <button class="trim-btn" type="button" @click="trimCrop('top')">
                <span class="arrow-icon up"></span>
                <span>上边减一点</span>
              </button>
              <button class="trim-btn" type="button" @click="trimCrop('bottom')">
                <span class="arrow-icon down"></span>
                <span>下边减一点</span>
              </button>
              <button class="ghost-btn" type="button" @click="currentStep = 1">上一步</button>
            </div>
          </div>
        </div>

        <div class="toggle-row">
          <strong>去掉底色</strong>
          <button
            class="switch-btn"
            :class="{ active: removeBackground }"
            type="button"
            @click="removeBackground = !removeBackground"
          >
            {{ removeBackground ? '开' : '关' }}
          </button>
        </div>

        <div v-if="previewLoading" class="status-box loading-box">
          正在切图...
        </div>
      </div>

      <div v-else-if="currentStep === 3 && mode === 'sheet'" class="step-panel review-panel">
        <div class="section-head">
          <h3>看颜色</h3>
          <span>{{ reviewSummary }}</span>
        </div>

        <div v-if="previewLoading" class="status-box">
          正在识别颜色...
        </div>

        <template v-else-if="reviewCells.length">
          <div class="group-switcher">
            <button
              v-for="group in groupedReviewCells"
              :key="group.code"
              class="group-chip"
              :class="{ active: group.code === activeReviewGroupCode }"
              type="button"
              @click="selectReviewGroup(group.code)"
            >
              <i :style="{ backgroundColor: colorForCode(group.code) }"></i>
              <span>{{ group.code }}</span>
              <strong>{{ group.cells.length }}</strong>
              <em v-if="group.priorityCount">{{ group.priorityCount }}</em>
            </button>
          </div>

          <div class="review-grid sheet-review-grid">
            <button
              v-for="cell in currentReviewGroupCells"
              :key="cell.key"
              class="review-tile simple-tile"
              :class="{ active: selectedReviewCellKey === cell.key }"
              type="button"
              @click="selectedReviewCellKey = cell.key"
            >
              <div class="tile-flags">
                <span v-if="cell.needsCheck" class="flag-chip warm">先看</span>
                <span v-else-if="cell.token" class="flag-chip">有字</span>
              </div>
              <div class="review-thumb-shell large-shell">
                <div class="review-fragment" :style="buildCellFragmentStyle(cell, 72)"></div>
              </div>
            </button>
          </div>

          <div v-if="selectedReviewCell" class="confirm-panel">
            <div class="confirm-preview">
              <div class="detail-fragment-shell">
                <div class="detail-fragment" :style="buildCellFragmentStyle(selectedReviewCell, 128)"></div>
              </div>
              <div class="detail-meta">
                <div class="detail-row">
                  <span>当前颜色</span>
                  <strong>{{ selectedReviewCell.code || '暂未判断' }}</strong>
                </div>
                <div class="detail-row">
                  <span>这格怎么判的</span>
                  <strong>{{ reviewMethodText(selectedReviewCell) }}</strong>
                </div>
                <div v-if="selectedReviewCell.token" class="detail-row">
                  <span>识别到的字</span>
                  <strong>{{ selectedReviewCell.token }}</strong>
                </div>
                <div class="detail-row">
                  <span>想改的话</span>
                  <strong>点下面颜色</strong>
                </div>
              </div>
            </div>

            <div class="confirm-options">
              <button
                v-for="item in selectedReviewOptions"
                :key="item.code"
                class="review-option"
                :class="{ active: item.code === selectedReviewCell.code }"
                type="button"
                @click="$emit('replace-cell-color', selectedReviewCell.key, item.code)"
              >
                <i :style="{ backgroundColor: item.hex }"></i>
                <span>{{ item.code }}</span>
              </button>
            </div>
          </div>
        </template>

        <div v-else class="empty-state compact">
          先生成一下。
        </div>
      </div>

      <div
        v-else-if="mode === 'image' && currentStep === 2"
        class="step-panel compact-panel"
      >
        <div class="section-head">
          <h3>预览</h3>
          <span>{{ previewReport ? `${previewReport.width} × ${previewReport.height}` : '未生成' }}</span>
        </div>

        <div v-if="previewLoading" class="status-box">
          正在生成预览...
        </div>

        <div v-else-if="previewDocument" class="preview-result">
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
        <div v-else class="empty-state">
          先生成一下。
        </div>
      </div>

      <div class="dialog-actions">
        <button class="ghost-btn" type="button" @click="handleBackAction">
          {{ currentStep === 1 ? '取消' : '上一步' }}
        </button>
        <button
          class="primary-btn"
          type="button"
          :disabled="primaryDisabled"
          @click="handlePrimaryAction"
        >
          {{ primaryText }}
        </button>
      </div>
    </div>

    <PatternImageCropper
      :visible="showCropper"
      :src="imageUrl"
      :initial-crop="cropRect"
      @cancel="showCropper = false"
      @confirm="handleCropConfirm"
    />
  </div>
</template>

<script setup>
import { computed, nextTick, ref, watch } from 'vue'
import { ARTKAL_COLORS_FULL, getColorByCode } from '@/data/artkal-colors.js'
import PatternImageCropper from '@/components/pattern/PatternImageCropper.vue'
import {
  PATTERN_BOARD_SIZE,
  PATTERN_DEFAULT_DIMENSION,
  PATTERN_MAX_DIMENSION,
} from '@/modules/pattern-workbench/core/patternBoard.js'

const props = defineProps({
  visible: {
    type: Boolean,
    default: false,
  },
  imageUrl: {
    type: String,
    default: '',
  },
  initialMode: {
    type: String,
    default: 'image',
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

const emit = defineEmits(['cancel', 'preview', 'confirm', 'replace-cell-color'])

const stageImageRef = ref(null)
const cropRect = ref(null)
const cropPreviewUrl = ref('')
const stageMetrics = ref(null)
const showCropper = ref(false)
const hasAutoOpenedCropper = ref(false)
const mode = ref('image')
const currentStep = ref(1)
const removeBackground = ref(false)
const targetWidth = ref(PATTERN_DEFAULT_DIMENSION)
const targetHeight = ref(PATTERN_DEFAULT_DIMENSION)
const cellWidth = ref(0)
const cellHeight = ref(0)
const gridOffsetX = ref(0)
const gridOffsetY = ref(0)
const basisOffsetX = ref(0)
const basisOffsetY = ref(0)
const focusGridCount = ref(4)
const focusLineWidth = ref(2)
const basisPositionInitialized = ref(false)
const generatedSignature = ref('')
const pendingPreviewStep = ref(0)
const selectedReviewCellKey = ref('')
const activeReviewGroupCode = ref('')
const zoomStageBaseSize = 420

function clampValue(value, min, max) {
  return Math.min(max, Math.max(min, value))
}

function normalizePositiveInteger(value, fallback = 0) {
  const numericValue = Number(value)
  if (!Number.isFinite(numericValue)) {
    return fallback
  }
  return Math.max(0, Math.round(numericValue))
}

function colorForCode(code) {
  return getColorByCode(code)?.hex || '#152131'
}

function rgbDistance(left, right) {
  if (!left || !right) {
    return Number.POSITIVE_INFINITY
  }

  return Math.sqrt(
    (left.r - right.r) ** 2 +
      (left.g - right.g) ** 2 +
      (left.b - right.b) ** 2,
  )
}

function ensureCropRect() {
  const imageElement = stageImageRef.value
  if (!imageElement || cropRect.value) {
    return
  }

  cropRect.value = {
    x: 0,
    y: 0,
    width: imageElement.naturalWidth,
    height: imageElement.naturalHeight,
  }
}

function ensureCellSize() {
  if (mode.value !== 'sheet' || !cropRect.value) {
    return
  }

  const recommendedWidth = Math.max(
    4,
    Math.round(cropRect.value.width / PATTERN_BOARD_SIZE),
  )
  const recommendedHeight = Math.max(
    4,
    Math.round(cropRect.value.height / PATTERN_BOARD_SIZE),
  )

  if (!cellWidth.value) {
    cellWidth.value = recommendedWidth
  }

  if (!cellHeight.value) {
    cellHeight.value = recommendedHeight
  }

  cellWidth.value = clampValue(cellWidth.value, 4, maxCellWidth.value)
  cellHeight.value = clampValue(cellHeight.value, 4, maxCellHeight.value)
  gridOffsetX.value = clampValue(gridOffsetX.value, 0, maxGridOffsetX.value)
  gridOffsetY.value = clampValue(gridOffsetY.value, 0, maxGridOffsetY.value)

  if (!basisPositionInitialized.value) {
    basisOffsetX.value = Math.max(
      0,
      Math.round((cropRect.value.width - cellWidth.value * focusGridCount.value) / 2),
    )
    basisOffsetY.value = Math.max(
      0,
      Math.round((cropRect.value.height - cellHeight.value * focusGridCount.value) / 2),
    )
    basisPositionInitialized.value = true
  }
}

function handleStageImageLoad() {
  ensureCropRect()
  ensureCellSize()
  syncStageMetrics()
}

function openCropper() {
  showCropper.value = true
}

function resetCrop() {
  const imageElement = stageImageRef.value
  if (!imageElement) {
    return
  }

  cropRect.value = {
    x: 0,
    y: 0,
    width: imageElement.naturalWidth,
    height: imageElement.naturalHeight,
  }
  basisPositionInitialized.value = false
  ensureCellSize()
}

function handleCropConfirm(cropData) {
  showCropper.value = false
  cropRect.value = {
    x: cropData.sx,
    y: cropData.sy,
    width: cropData.sw,
    height: cropData.sh,
  }
  basisPositionInitialized.value = false
  ensureCellSize()
}

function adjustCellWidth(delta) {
  if (!cropRect.value) {
    return
  }

  cellWidth.value = clampValue(
    normalizePositiveInteger(cellWidth.value + delta, 4),
    4,
    maxCellWidth.value,
  )
}

function adjustCellHeight(delta) {
  if (!cropRect.value) {
    return
  }

  cellHeight.value = clampValue(
    normalizePositiveInteger(cellHeight.value + delta, 4),
    4,
    maxCellHeight.value,
  )
}

function adjustFocusGridCount(delta) {
  focusGridCount.value = clampValue(
    normalizePositiveInteger(focusGridCount.value + delta, 4),
    3,
    8,
  )
}

function adjustFocusLineWidth(delta) {
  focusLineWidth.value = clampValue(
    normalizePositiveInteger(focusLineWidth.value + delta, 2),
    1,
    6,
  )
}

function adjustGridOffsetX(delta) {
  if (!cropRect.value) {
    return
  }

  gridOffsetX.value = clampValue(
    normalizePositiveInteger(gridOffsetX.value + delta, 0),
    0,
    maxGridOffsetX.value,
  )
}

function adjustGridOffsetY(delta) {
  if (!cropRect.value) {
    return
  }

  gridOffsetY.value = clampValue(
    normalizePositiveInteger(gridOffsetY.value + delta, 0),
    0,
    maxGridOffsetY.value,
  )
}

function adjustBasisColumn(delta) {
  if (!calibrationGridModel.value) {
    return
  }

  basisOffsetX.value = clampValue(
    basisOffsetX.value + delta,
    zoomMoveBounds.value.minX,
    zoomMoveBounds.value.maxX,
  )
}

function adjustBasisRow(delta) {
  if (!calibrationGridModel.value) {
    return
  }

  basisOffsetY.value = clampValue(
    basisOffsetY.value + delta,
    zoomMoveBounds.value.minY,
    zoomMoveBounds.value.maxY,
  )
}

function trimCrop(side) {
  if (!cropRect.value) {
    return
  }

  const minWidth = Math.max(8, cellWidth.value || 8)
  const minHeight = Math.max(8, cellHeight.value || 8)
  const nextCrop = { ...cropRect.value }

  if (side === 'left' && nextCrop.width > minWidth) {
    nextCrop.x += 1
    nextCrop.width -= 1
  }
  if (side === 'right' && nextCrop.width > minWidth) {
    nextCrop.width -= 1
  }
  if (side === 'top' && nextCrop.height > minHeight) {
    nextCrop.y += 1
    nextCrop.height -= 1
  }
  if (side === 'bottom' && nextCrop.height > minHeight) {
    nextCrop.height -= 1
  }

  cropRect.value = nextCrop
}

function getImageMetrics() {
  if (stageMetrics.value) {
    return stageMetrics.value
  }

  const imageElement = stageImageRef.value
  if (!imageElement) {
    return null
  }

  const rect = imageElement.getBoundingClientRect()
  if (!rect.width || !rect.height) {
    return null
  }

  const naturalWidth = imageElement.naturalWidth || 1
  const naturalHeight = imageElement.naturalHeight || 1
  const scale = Math.min(rect.width / naturalWidth, rect.height / naturalHeight)
  const displayWidth = naturalWidth * scale
  const displayHeight = naturalHeight * scale
  const offsetX = (rect.width - displayWidth) / 2
  const offsetY = (rect.height - displayHeight) / 2

  return {
    rect,
    naturalWidth,
    naturalHeight,
    scale,
    displayWidth,
    displayHeight,
    offsetX,
    offsetY,
  }
}

function syncStageMetrics() {
  nextTick(() => {
    const imageElement = stageImageRef.value
    if (!imageElement) {
      stageMetrics.value = null
      return
    }

    const rect = imageElement.getBoundingClientRect()
    if (!rect.width || !rect.height) {
      stageMetrics.value = null
      return
    }

    const naturalWidth = imageElement.naturalWidth || 1
    const naturalHeight = imageElement.naturalHeight || 1
    const scale = Math.min(rect.width / naturalWidth, rect.height / naturalHeight)
    const displayWidth = naturalWidth * scale
    const displayHeight = naturalHeight * scale
    const offsetX = (rect.width - displayWidth) / 2
    const offsetY = (rect.height - displayHeight) / 2

    stageMetrics.value = {
      rect,
      naturalWidth,
      naturalHeight,
      scale,
      displayWidth,
      displayHeight,
      offsetX,
      offsetY,
    }
  })
}

function buildCropPreview() {
  if (!props.imageUrl || !cropRect.value) {
    cropPreviewUrl.value = ''
    return
  }

  const image = new Image()
  image.onload = () => {
    const canvas = globalThis.document.createElement('canvas')
    canvas.width = cropRect.value.width
    canvas.height = cropRect.value.height
    const context = canvas.getContext('2d')

    if (!context) {
      cropPreviewUrl.value = ''
      return
    }

    context.drawImage(
      image,
      cropRect.value.x,
      cropRect.value.y,
      cropRect.value.width,
      cropRect.value.height,
      0,
      0,
      cropRect.value.width,
      cropRect.value.height,
    )

    cropPreviewUrl.value = canvas.toDataURL('image/png')
  }
  image.src = props.imageUrl
}

function buildPayload() {
  return {
    mode: mode.value,
    crop: cropRect.value,
    cellSample:
      mode.value === 'sheet' && cropRect.value
        ? {
            x: cropRect.value.x + resolvedGridOffsetX.value,
            y: cropRect.value.y + resolvedGridOffsetY.value,
            width: cellWidth.value,
            height: cellHeight.value,
          }
        : null,
    removeBackground: removeBackground.value,
    width:
      mode.value === 'sheet'
        ? derivedSheetSize.value.width
        : normalizePositiveInteger(targetWidth.value),
    height:
      mode.value === 'sheet'
        ? derivedSheetSize.value.height
        : normalizePositiveInteger(targetHeight.value),
  }
}

function requestPreview() {
  emit('preview', buildPayload())
}

function jumpToStep(stepValue) {
  if (stepValue > currentStep.value) {
    return
  }

  currentStep.value = stepValue
}

function handlePrimaryAction() {
  if (mode.value === 'image') {
    if (currentStep.value === 1) {
      if (!previewReady.value) {
        pendingPreviewStep.value = 2
        requestPreview()
        return
      }
      currentStep.value = 2
      return
    }

    emit('confirm', buildPayload())
    return
  }

  if (currentStep.value === 1) {
    currentStep.value = 2
    return
  }

  if (currentStep.value === 2) {
    if (!previewReady.value) {
      pendingPreviewStep.value = 3
      requestPreview()
      return
    }
    currentStep.value = 3
    return
  }

  emit('confirm', buildPayload())
}

function handleBackAction() {
  if (currentStep.value === 1) {
    pendingPreviewStep.value = 0
    emit('cancel')
    return
  }

  pendingPreviewStep.value = 0
  currentStep.value -= 1
}

function resetState() {
  cropRect.value = null
  cropPreviewUrl.value = ''
  stageMetrics.value = null
  showCropper.value = false
  hasAutoOpenedCropper.value = false
  removeBackground.value = false
  targetWidth.value = PATTERN_DEFAULT_DIMENSION
  targetHeight.value = PATTERN_DEFAULT_DIMENSION
  cellWidth.value = 0
  cellHeight.value = 0
  gridOffsetX.value = 0
  gridOffsetY.value = 0
  basisOffsetX.value = 0
  basisOffsetY.value = 0
  focusGridCount.value = 4
  focusLineWidth.value = 2
  basisPositionInitialized.value = false
  generatedSignature.value = ''
  pendingPreviewStep.value = 0
  selectedReviewCellKey.value = ''
  activeReviewGroupCode.value = ''
  currentStep.value = 1
}

function buildCellFragmentStyle(cell, targetSize) {
  if (!props.previewReport?.cropPreviewUrl || !cell?.bounds) {
    return {}
  }

  const sourceWidth = props.previewReport.croppedWidth || 1
  const sourceHeight = props.previewReport.croppedHeight || 1
  const maxDimension = Math.max(cell.bounds.width, cell.bounds.height, 1)
  const scale = targetSize / maxDimension

  return {
    width: `${Math.max(24, Math.round(cell.bounds.width * scale))}px`,
    height: `${Math.max(24, Math.round(cell.bounds.height * scale))}px`,
    backgroundImage: `url(${props.previewReport.cropPreviewUrl})`,
    backgroundRepeat: 'no-repeat',
    backgroundSize: `${Math.round(sourceWidth * scale)}px ${Math.round(sourceHeight * scale)}px`,
    backgroundPosition: `-${Math.round(cell.bounds.left * scale)}px -${Math.round(cell.bounds.top * scale)}px`,
  }
}

function toHexPart(value) {
  return value.toString(16).padStart(2, '0')
}

function sampleColorText(cell) {
  if (!cell?.sampleColor) {
    return '--'
  }

  return `#${toHexPart(cell.sampleColor.r)}${toHexPart(cell.sampleColor.g)}${toHexPart(cell.sampleColor.b)}`
}

const stepItems = computed(() => {
  if (mode.value === 'sheet') {
    return [
      { value: 1, index: '1', label: '裁图' },
      { value: 2, index: '2', label: '对齐' },
      { value: 3, index: '3', label: '颜色' },
    ]
  }

  return [
    { value: 1, index: '1', label: '裁图' },
    { value: 2, index: '2', label: '结果' },
  ]
})

const modeTitle = computed(() => (mode.value === 'sheet' ? '导入拼豆图纸' : '图片生成'))
const useCropPreviewAsStage = computed(() => {
  return mode.value === 'sheet' && currentStep.value === 2 && !!cropPreviewUrl.value
})

const sheetStageImageUrl = computed(() => {
  if (useCropPreviewAsStage.value) {
    return cropPreviewUrl.value
  }
  return props.imageUrl
})

const cropSummary = computed(() => {
  if (!cropRect.value) {
    return '先圈出要用的部分'
  }

  return `已选 ${cropRect.value.width} × ${cropRect.value.height}`
})

const cellSummary = computed(() => {
  if (!cellWidth.value || !cellHeight.value) {
    return '把中间参考区对齐就行'
  }

  return `宽 ${cellWidth.value} / 高 ${cellHeight.value} / 参考 ${focusGridCount.value} 格`
})

const derivedSheetSize = computed(() => {
  if (!cropRect.value || !cellWidth.value || !cellHeight.value) {
    return {
      width: PATTERN_DEFAULT_DIMENSION,
      height: PATTERN_DEFAULT_DIMENSION,
    }
  }

  const effectiveWidth = Math.max(1, cropRect.value.width - resolvedGridOffsetX.value)
  const effectiveHeight = Math.max(1, cropRect.value.height - resolvedGridOffsetY.value)

  return {
    width: Math.max(1, Math.floor(effectiveWidth / Math.max(1, cellWidth.value))),
    height: Math.max(1, Math.floor(effectiveHeight / Math.max(1, cellHeight.value))),
  }
})

const gridRemainderText = computed(() => {
  if (!cropRect.value || !cellWidth.value || !cellHeight.value) {
    return '--'
  }

  return `横向 ${(cropRect.value.width - resolvedGridOffsetX.value) % cellWidth.value}px / 纵向 ${(cropRect.value.height - resolvedGridOffsetY.value) % cellHeight.value}px`
})

const maxCellWidth = computed(() => {
  if (!cropRect.value) {
    return 128
  }

  return Math.max(4, Math.floor(cropRect.value.width))
})

const maxCellHeight = computed(() => {
  if (!cropRect.value) {
    return 128
  }

  return Math.max(4, Math.floor(cropRect.value.height))
})

const maxGridOffsetX = computed(() => {
  if (!cellWidth.value) {
    return 0
  }
  return Math.max(0, cellWidth.value - 1)
})

const maxGridOffsetY = computed(() => {
  if (!cellHeight.value) {
    return 0
  }
  return Math.max(0, cellHeight.value - 1)
})

const resolvedGridOffsetX = computed(() => {
  if (!cellWidth.value) {
    return 0
  }

  return ((basisOffsetX.value % cellWidth.value) + cellWidth.value) % cellWidth.value
})

const resolvedGridOffsetY = computed(() => {
  if (!cellHeight.value) {
    return 0
  }

  return ((basisOffsetY.value % cellHeight.value) + cellHeight.value) % cellHeight.value
})

const zoomWindowModel = computed(() => {
  if (!cropRect.value) {
    return null
  }

  const viewportWidth = Math.min(cropRect.value.width, Math.max(220, Math.round(cropRect.value.width * 0.68)))
  const viewportHeight = Math.min(cropRect.value.height, Math.max(220, Math.round(cropRect.value.height * 0.68)))

  return {
    x: Math.max(0, Math.round((cropRect.value.width - viewportWidth) / 2)),
    y: Math.max(0, Math.round((cropRect.value.height - viewportHeight) / 2)),
    width: viewportWidth,
    height: viewportHeight,
  }
})

const zoomMoveBounds = computed(() => {
  if (!zoomWindowModel.value || !cellWidth.value || !cellHeight.value) {
    return null
  }

  const focusWidth = cellWidth.value * focusGridCount.value
  const focusHeight = cellHeight.value * focusGridCount.value

  return {
    minX: zoomWindowModel.value.x,
    maxX: Math.max(zoomWindowModel.value.x, zoomWindowModel.value.x + zoomWindowModel.value.width - focusWidth),
    minY: zoomWindowModel.value.y,
    maxY: Math.max(zoomWindowModel.value.y, zoomWindowModel.value.y + zoomWindowModel.value.height - focusHeight),
  }
})

const calibrationGridModel = computed(() => {
  if (!cropRect.value || !cellWidth.value || !cellHeight.value || !zoomMoveBounds.value) {
    return null
  }

  const focusX = clampValue(basisOffsetX.value, zoomMoveBounds.value.minX, zoomMoveBounds.value.maxX)
  const focusY = clampValue(basisOffsetY.value, zoomMoveBounds.value.minY, zoomMoveBounds.value.maxY)

  return {
    focusColumns: focusGridCount.value,
    focusRows: focusGridCount.value,
    focusX,
    focusY,
    focusWidth: cellWidth.value * focusGridCount.value,
    focusHeight: cellHeight.value * focusGridCount.value,
  }
})

const payloadSignature = computed(() => JSON.stringify(buildPayload()))

const previewReady = computed(() => {
  return !!props.previewDocument && generatedSignature.value === payloadSignature.value
})

const cropFrameStyle = computed(() => {
  const metrics = getImageMetrics()
  if (!metrics || !cropRect.value) {
    return null
  }

  return {
    left: `${metrics.offsetX + cropRect.value.x * metrics.scale}px`,
    top: `${metrics.offsetY + cropRect.value.y * metrics.scale}px`,
    width: `${cropRect.value.width * metrics.scale}px`,
    height: `${cropRect.value.height * metrics.scale}px`,
  }
})

const gridLayerMetrics = computed(() => {
  const metrics = getImageMetrics()
  if (!metrics || !cropRect.value || !cellWidth.value || !cellHeight.value) {
    return null
  }

  const displayLeft = useCropPreviewAsStage.value
    ? metrics.offsetX
    : metrics.offsetX + cropRect.value.x * metrics.scale
  const displayTop = useCropPreviewAsStage.value
    ? metrics.offsetY
    : metrics.offsetY + cropRect.value.y * metrics.scale
  const displayWidth = useCropPreviewAsStage.value
    ? metrics.displayWidth
    : cropRect.value.width * metrics.scale
  const displayHeight = useCropPreviewAsStage.value
    ? metrics.displayHeight
    : cropRect.value.height * metrics.scale
  const displayCellWidth = Math.max(2, cellWidth.value * metrics.scale)
  const displayCellHeight = Math.max(2, cellHeight.value * metrics.scale)
  const displayOffsetX = Math.max(0, gridOffsetX.value * metrics.scale)
  const displayOffsetY = Math.max(0, gridOffsetY.value * metrics.scale)

  return {
    left: displayLeft,
    top: displayTop,
    width: displayWidth,
    height: displayHeight,
    cellWidth: displayCellWidth,
    cellHeight: displayCellHeight,
    offsetX: displayOffsetX,
    offsetY: displayOffsetY,
  }
})

const gridLayerStyle = computed(() => {
  if (!gridLayerMetrics.value) {
    return null
  }

  return {
    left: `${gridLayerMetrics.value.left}px`,
    top: `${gridLayerMetrics.value.top}px`,
    width: `${gridLayerMetrics.value.width}px`,
    height: `${gridLayerMetrics.value.height}px`,
  }
})

const gridFocusCellStyle = computed(() => {
  if (!gridLayerMetrics.value || !calibrationGridModel.value) {
    return null
  }

  return {
    left: `${gridLayerMetrics.value.offsetX + calibrationGridModel.value.focusColumn * gridLayerMetrics.value.cellWidth}px`,
    top: `${gridLayerMetrics.value.offsetY + calibrationGridModel.value.focusRow * gridLayerMetrics.value.cellHeight}px`,
    width: `${Math.max(8, calibrationGridModel.value.focusColumns * gridLayerMetrics.value.cellWidth)}px`,
    height: `${Math.max(8, calibrationGridModel.value.focusRows * gridLayerMetrics.value.cellHeight)}px`,
  }
})

const verticalGridLines = computed(() => {
  if (!gridLayerMetrics.value) {
    return []
  }

  const lines = []
  const cell = Math.max(1, gridLayerMetrics.value.cellWidth)
  const total = gridLayerMetrics.value.width
  const start = gridLayerMetrics.value.offsetX

  for (let position = start, index = 0; position <= total; position += cell, index += 1) {
    lines.push({
      key: index,
      position: Math.min(position, total),
    })
  }

  if (!lines.length || lines[lines.length - 1].position !== total) {
    lines.push({
      key: 'end',
      position: total,
    })
  }

  return lines
})

const horizontalGridLines = computed(() => {
  if (!gridLayerMetrics.value) {
    return []
  }

  const lines = []
  const cell = Math.max(1, gridLayerMetrics.value.cellHeight)
  const total = gridLayerMetrics.value.height
  const start = gridLayerMetrics.value.offsetY

  for (let position = start, index = 0; position <= total; position += cell, index += 1) {
    lines.push({
      key: index,
      position: Math.min(position, total),
    })
  }

  if (!lines.length || lines[lines.length - 1].position !== total) {
    lines.push({
      key: 'end',
      position: total,
    })
  }

  return lines
})

const zoomViewport = computed(() => {
  if (!zoomWindowModel.value) {
    return null
  }

  const scale = Math.min(
    zoomStageBaseSize / Math.max(1, zoomWindowModel.value.width),
    zoomStageBaseSize / Math.max(1, zoomWindowModel.value.height),
  )
  const width = Math.max(180, Math.round(zoomWindowModel.value.width * scale))
  const height = Math.max(180, Math.round(zoomWindowModel.value.height * scale))

  return {
    x: zoomWindowModel.value.x,
    y: zoomWindowModel.value.y,
    width: zoomWindowModel.value.width,
    height: zoomWindowModel.value.height,
    scale,
    stageWidth: width,
    stageHeight: height,
    surfaceLeft: Math.max(0, Math.round((zoomStageBaseSize - width) / 2)),
    surfaceTop: Math.max(0, Math.round((zoomStageBaseSize - height) / 2)),
  }
})

const zoomStageStyle = computed(() => {
  return {
    '--zoom-stage-size': `${zoomStageBaseSize}px`,
  }
})

const zoomSurfaceStyle = computed(() => {
  if (!zoomViewport.value || !cropPreviewUrl.value || !cropRect.value) {
    return null
  }

  return {
    left: `${zoomViewport.value.surfaceLeft}px`,
    top: `${zoomViewport.value.surfaceTop}px`,
    width: `${zoomViewport.value.stageWidth}px`,
    height: `${zoomViewport.value.stageHeight}px`,
    backgroundImage: `url(${cropPreviewUrl.value})`,
    backgroundRepeat: 'no-repeat',
    backgroundSize: `${Math.round(cropRect.value.width * zoomViewport.value.scale)}px ${Math.round(cropRect.value.height * zoomViewport.value.scale)}px`,
    backgroundPosition: `-${Math.round(zoomViewport.value.x * zoomViewport.value.scale)}px -${Math.round(zoomViewport.value.y * zoomViewport.value.scale)}px`,
  }
})

const zoomFocusStyle = computed(() => {
  if (!zoomViewport.value || !calibrationGridModel.value) {
    return null
  }

  return {
    left: `${Math.round((calibrationGridModel.value.focusX - zoomViewport.value.x) * zoomViewport.value.scale)}px`,
    top: `${Math.round((calibrationGridModel.value.focusY - zoomViewport.value.y) * zoomViewport.value.scale)}px`,
    width: `${Math.max(18, Math.round(calibrationGridModel.value.focusWidth * zoomViewport.value.scale))}px`,
    height: `${Math.max(18, Math.round(calibrationGridModel.value.focusHeight * zoomViewport.value.scale))}px`,
    '--focus-line-size': `${focusLineWidth.value}px`,
  }
})

const zoomVerticalDividerStyles = computed(() => {
  if (!zoomViewport.value || !calibrationGridModel.value) {
    return []
  }

  const list = []
  for (let index = 1; index < calibrationGridModel.value.focusColumns; index += 1) {
    list.push({
      key: index,
      style: {
        left: `${Math.round(index * cellWidth.value * zoomViewport.value.scale - focusLineWidth.value / 2)}px`,
      },
    })
  }
  return list
})

const zoomHorizontalDividerStyles = computed(() => {
  if (!zoomViewport.value || !calibrationGridModel.value) {
    return []
  }

  const list = []
  for (let index = 1; index < calibrationGridModel.value.focusRows; index += 1) {
    list.push({
      key: index,
      style: {
        top: `${Math.round(index * cellHeight.value * zoomViewport.value.scale - focusLineWidth.value / 2)}px`,
      },
    })
  }
  return list
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
        color: colorForCode(code),
      })
    }
  }
  return cells
})

const reviewCells = computed(() => props.previewReport?.reviewCells || [])

function reviewCellPriority(cell) {
  if (!cell) {
    return 0
  }

  if (!cell.token) {
    return 0
  }

  if (cell.source === 'ocr') {
    return cell.confidence >= 72 ? 1 : 3
  }

  return 2
}

function reviewMethodText(cell) {
  if (!cell) {
    return '--'
  }

  if (cell.source === 'ocr') {
    return cell.confidence >= 72 ? '按格子里的字' : '按字先猜的'
  }

  if (cell.token) {
    return '看到字，但颜色更像这个'
  }

  return '按颜色判断'
}

function compareReviewCells(left, right) {
  const priorityGap = reviewCellPriority(right) - reviewCellPriority(left)
  if (priorityGap) {
    return priorityGap
  }

  const leftConfidence = Number(left?.confidence || 0)
  const rightConfidence = Number(right?.confidence || 0)
  if (left.token || right.token) {
    const confidenceGap = leftConfidence - rightConfidence
    if (confidenceGap) {
      return confidenceGap
    }
  }

  return String(left?.key || '').localeCompare(String(right?.key || ''), 'zh-Hans-CN')
}

const sortedReviewCells = computed(() => {
  return [...reviewCells.value].sort(compareReviewCells).map((cell) => ({
    ...cell,
    needsCheck: reviewCellPriority(cell) >= 2,
  }))
})

const groupedReviewCells = computed(() => {
  const groups = new Map()

  sortedReviewCells.value.forEach((cell) => {
    const code = cell.code || '未识别'
    if (!groups.has(code)) {
      groups.set(code, {
        code,
        cells: [],
        priorityCount: 0,
      })
    }
    const group = groups.get(code)
    group.cells.push(cell)
    if (cell.needsCheck) {
      group.priorityCount += 1
    }
  })

  return Array.from(groups.values()).sort((left, right) => {
    if (right.priorityCount !== left.priorityCount) {
      return right.priorityCount - left.priorityCount
    }
    if (right.cells.length !== left.cells.length) {
      return right.cells.length - left.cells.length
    }
    return left.code.localeCompare(right.code, 'zh-Hans-CN')
  })
})

const currentReviewGroupCells = computed(() => {
  if (!groupedReviewCells.value.length) {
    return []
  }

  const matchedGroup = groupedReviewCells.value.find((group) => group.code === activeReviewGroupCode.value)
  return matchedGroup?.cells || groupedReviewCells.value[0].cells
})

const selectedReviewCell = computed(() => {
  if (!sortedReviewCells.value.length) {
    return null
  }

  const matchedCell = sortedReviewCells.value.find((item) => item.key === selectedReviewCellKey.value)
  return matchedCell || sortedReviewCells.value[0]
})

const reviewSummary = computed(() => {
  if (!sortedReviewCells.value.length) {
    return '下一步后会自动识别'
  }

  const priorityCount = sortedReviewCells.value.filter((cell) => cell.needsCheck).length
  if (!priorityCount) {
    return `识别很稳，可直接导入`
  }

  return `先看 ${priorityCount} 张 / 共 ${sortedReviewCells.value.length} 张`
})

function selectReviewGroup(code) {
  activeReviewGroupCode.value = code
  const nextCell = groupedReviewCells.value.find((group) => group.code === code)?.cells?.[0]
  if (nextCell) {
    selectedReviewCellKey.value = nextCell.key
  }
}

const selectedReviewOptions = computed(() => {
  if (!selectedReviewCell.value) {
    return []
  }

  const options = ARTKAL_COLORS_FULL
    .map((item) => ({
      code: item.code,
      hex: item.hex,
      distance: rgbDistance(selectedReviewCell.value.sampleColor, item),
    }))
    .sort((left, right) => left.distance - right.distance)

  const currentCode = selectedReviewCell.value.code
  const ocrCode = selectedReviewCell.value.ocrCode
  const currentOption = options.find((item) => item.code === currentCode)
  const ocrOption = options.find((item) => item.code === ocrCode)
  const pickedOptions = options.slice(0, 12)
  const prioritized = []

  if (currentCode && currentOption) {
    prioritized.push(currentOption)
  }

  if (ocrCode && ocrOption && !prioritized.some((item) => item.code === ocrCode)) {
    prioritized.push(ocrOption)
  }

  pickedOptions.forEach((item) => {
    if (prioritized.some((picked) => picked.code === item.code)) {
      return
    }
    prioritized.push(item)
  })

  if (prioritized.length > 12) {
    return prioritized.slice(0, 12)
  }

  return prioritized
})

const primaryDisabled = computed(() => {
  if (props.previewLoading) {
    return true
  }

  if (mode.value === 'image') {
    if (currentStep.value === 1) {
      return !cropRect.value || !targetWidth.value || !targetHeight.value || targetWidth.value < 1 || targetHeight.value < 1 || targetWidth.value > PATTERN_MAX_DIMENSION || targetHeight.value > PATTERN_MAX_DIMENSION
    }

    return !previewReady.value
  }

  if (currentStep.value === 1) {
    return !cropRect.value
  }

  if (currentStep.value === 2) {
    return !cellWidth.value || !cellHeight.value
  }

  if (currentStep.value === 3) {
    return !previewReady.value || !reviewCells.value.length
  }

  return false
})

const primaryText = computed(() => {
  if (props.previewLoading) {
    return '生成中...'
  }

  if (mode.value === 'image' && currentStep.value === 2) {
    return '导入画板'
  }

  if (mode.value === 'sheet' && currentStep.value === 3) {
    return '导入画板'
  }

  return '下一步'
})

watch(
  () => props.visible,
  (visible) => {
    if (!visible) {
      resetState()
      return
    }

    mode.value = props.initialMode
    currentStep.value = 1
  },
  { immediate: true },
)

watch(
  () => props.imageUrl,
  (imageUrl) => {
    if (!props.visible || !imageUrl || hasAutoOpenedCropper.value) {
      return
    }

    showCropper.value = true
    hasAutoOpenedCropper.value = true
  },
  { immediate: true },
)

watch(
  () => props.initialMode,
  (nextMode) => {
    mode.value = nextMode
    currentStep.value = 1
  },
)

watch(
  () => props.previewReport,
  (report) => {
    if (!report) {
      return
    }

    generatedSignature.value = payloadSignature.value
    const sortedCells = [...(report.reviewCells || [])].sort(compareReviewCells)
    selectedReviewCellKey.value = sortedCells[0]?.key || ''
    activeReviewGroupCode.value = sortedCells[0]?.code || ''
  },
)

watch(
  previewReady,
  (ready) => {
    if (!ready || !pendingPreviewStep.value) {
      return
    }

    currentStep.value = pendingPreviewStep.value
    pendingPreviewStep.value = 0
  },
)

watch(
  selectedReviewCell,
  (cell) => {
    if (!cell) {
      return
    }

    activeReviewGroupCode.value = cell.code || '未识别'
  },
)

watch(
  cropRect,
  () => {
    ensureCellSize()
    buildCropPreview()
    syncStageMetrics()
  },
)

watch(
  () => props.imageUrl,
  () => {
    buildCropPreview()
    stageMetrics.value = null
    syncStageMetrics()
  },
)

watch(
  sheetStageImageUrl,
  () => {
    stageMetrics.value = null
    syncStageMetrics()
  },
)

watch(
  cellWidth,
  (value) => {
    if (!cropRect.value) {
      return
    }

    cellWidth.value = clampValue(normalizePositiveInteger(value, 4), 4, maxCellWidth.value)
    if (zoomMoveBounds.value) {
      basisOffsetX.value = clampValue(basisOffsetX.value, zoomMoveBounds.value.minX, zoomMoveBounds.value.maxX)
    }
  },
)

watch(
  cellHeight,
  (value) => {
    if (!cropRect.value) {
      return
    }

    cellHeight.value = clampValue(normalizePositiveInteger(value, 4), 4, maxCellHeight.value)
    if (zoomMoveBounds.value) {
      basisOffsetY.value = clampValue(basisOffsetY.value, zoomMoveBounds.value.minY, zoomMoveBounds.value.maxY)
    }
  },
)

watch(
  focusGridCount,
  (value) => {
    focusGridCount.value = clampValue(normalizePositiveInteger(value, 4), 3, 8)
    if (zoomMoveBounds.value) {
      basisOffsetX.value = clampValue(basisOffsetX.value, zoomMoveBounds.value.minX, zoomMoveBounds.value.maxX)
      basisOffsetY.value = clampValue(basisOffsetY.value, zoomMoveBounds.value.minY, zoomMoveBounds.value.maxY)
    }
  },
)

watch(
  focusLineWidth,
  (value) => {
    focusLineWidth.value = clampValue(normalizePositiveInteger(value, 2), 1, 6)
  },
)
</script>

<style scoped>
.sheet-dialog-mask {
  position: fixed;
  inset: 0;
  z-index: 60;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24px;
  background: rgba(15, 27, 43, 0.54);
  backdrop-filter: blur(10px);
}

.sheet-dialog {
  width: min(1360px, 100%);
  max-height: calc(100vh - 48px);
  overflow: auto;
  padding: 24px;
  border-radius: 28px;
  background: #ffffff;
  box-shadow: 0 24px 60px rgba(18, 32, 51, 0.24);
}

.dialog-head,
.selection-actions,
.toggle-row,
.status-row,
.dialog-actions,
.detail-row,
.review-code-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: center;
}

.dialog-head h2 {
  font-size: 24px;
  color: #122033;
}

.step-strip {
  display: grid;
  gap: 10px;
  margin-top: 18px;
}

.step-pill {
  display: grid;
  gap: 4px;
  padding: 12px;
  border: 1px solid #dbe4f2;
  border-radius: 16px;
  background: #f7fbff;
  text-align: left;
  cursor: pointer;
}

.step-pill span {
  color: #7b889f;
  font-size: 11px;
  font-weight: 700;
}

.step-pill strong {
  color: #18304f;
  font-size: 13px;
}

.step-pill.active {
  border-color: #79b7ff;
  background: linear-gradient(180deg, #ffffff, #edf5ff);
}

.step-pill.done {
  background: #eef7ff;
}

.ghost-btn,
.secondary-btn,
.primary-btn,
.switch-btn {
  height: 42px;
  padding: 0 16px;
  border-radius: 14px;
  border: 1px solid transparent;
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 160ms ease,
    box-shadow 160ms ease,
    background-color 160ms ease,
    border-color 160ms ease,
    color 160ms ease;
}

.ghost-btn:hover,
.secondary-btn:hover,
.primary-btn:hover,
.switch-btn:hover,
.move-btn:hover,
.trim-btn:hover,
.stepper-btn:hover,
.group-chip:hover,
.review-option:hover {
  transform: translateY(-1px);
}

.ghost-btn {
  border-color: #d7e3f3;
  background: #ffffff;
  color: #36506f;
  box-shadow: 0 8px 18px rgba(18, 32, 51, 0.06);
}

.secondary-btn {
  border-color: #cfe0f4;
  background: linear-gradient(180deg, #ffffff, #f5f9ff);
  color: #18304f;
  box-shadow: 0 10px 20px rgba(18, 32, 51, 0.06);
}

.primary-btn {
  border-color: #2f7fff;
  background: linear-gradient(180deg, #4b97ff, #2f7fff);
  color: #ffffff;
  box-shadow: 0 14px 28px rgba(47, 127, 255, 0.28);
}

.primary-btn:disabled,
.ghost-btn:disabled,
.secondary-btn:disabled,
.switch-btn:disabled,
.move-btn:disabled,
.trim-btn:disabled,
.stepper-btn:disabled,
.group-chip:disabled,
.review-option:disabled {
  cursor: not-allowed;
  opacity: 0.56;
  transform: none;
  box-shadow: none;
}

.switch-btn {
  min-width: 72px;
  border-color: #d7e3f3;
  background: #eef4fb;
  color: #5f7390;
}

.switch-btn.active {
  border-color: #8dd5bb;
  background: linear-gradient(180deg, #ebfff6, #d8f7e8);
  color: #16724d;
  box-shadow: 0 10px 20px rgba(22, 114, 77, 0.12);
}

.step-panel {
  margin-top: 18px;
  padding: 18px;
  border-radius: 24px;
  border: 1px solid #dbe4f2;
  background: #f8fbff;
}

.compact-panel {
  max-width: 680px;
}

.section-head {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: baseline;
  margin-bottom: 12px;
}

.section-head h3 {
  font-size: 18px;
  color: #18304f;
}

.section-head span {
  font-size: 12px;
  color: #7b889f;
}

.preview-stage {
  position: relative;
  border-radius: 20px;
  overflow: hidden;
  background:
    linear-gradient(45deg, rgba(47, 109, 255, 0.04) 25%, transparent 25%),
    linear-gradient(-45deg, rgba(47, 109, 255, 0.04) 25%, transparent 25%),
    linear-gradient(45deg, transparent 75%, rgba(47, 109, 255, 0.04) 75%),
    linear-gradient(-45deg, transparent 75%, rgba(47, 109, 255, 0.04) 75%);
  background-size: 24px 24px;
  background-position: 0 0, 0 12px, 12px -12px, -12px 0;
}

.crop-compare-layout {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.compare-card {
  display: grid;
  gap: 10px;
}

.compare-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  color: #18304f;
  font-size: 14px;
  font-weight: 700;
}

.compare-stage {
  min-height: 320px;
}

.cropped-preview-image {
  display: block;
  width: 100%;
  max-height: 72vh;
  object-fit: contain;
}

.preview-stage img {
  display: block;
  width: 100%;
  max-height: 72vh;
  object-fit: contain;
  user-select: none;
  position: relative;
  z-index: 1;
}

.crop-frame,
.grid-layer {
  position: absolute;
}

.crop-frame {
  border: 2px solid #2f6dff;
  background: rgba(47, 109, 255, 0.08);
  box-shadow: 0 0 0 9999px rgba(15, 27, 43, 0.26);
}

.crop-frame.locked {
  pointer-events: none;
}

.grid-layer {
  z-index: 8;
  pointer-events: none;
  box-shadow:
    inset 0 0 0 1px rgba(255, 132, 132, 0.92);
}

.grid-line {
  position: absolute;
  background: rgba(255, 103, 103, 0.82);
}

.grid-line.vertical {
  top: 0;
  bottom: 0;
  width: 1px;
}

.grid-line.horizontal {
  left: 0;
  right: 0;
  height: 1px;
}

.calibration-layout {
  display: grid;
  grid-template-columns: minmax(0, 1.35fr) 460px;
  gap: 16px;
  align-items: start;
}

.full-grid-stage {
  min-height: 460px;
}

.grid-focus-cell {
  position: absolute;
  z-index: 9;
  pointer-events: none;
  border: 1px solid rgba(74, 214, 255, 0.96);
  background: rgba(74, 214, 255, 0.1);
  box-shadow:
    0 0 0 1px rgba(255, 255, 255, 0.55);
}

.zoom-panel {
  display: grid;
  gap: 12px;
  padding: 14px;
  border-radius: 18px;
  border: 1px solid #dbe4f2;
  background: #ffffff;
}

.zoom-head {
  display: grid;
  gap: 4px;
}

.zoom-head strong {
  color: #18304f;
  font-size: 15px;
}

.zoom-head span,
.zoom-tip {
  color: #617089;
  font-size: 12px;
  line-height: 1.6;
}

.zoom-stage {
  position: relative;
  width: var(--zoom-stage-size);
  height: var(--zoom-stage-size);
  max-width: 100%;
  margin: 0 auto;
  border-radius: 18px;
  overflow: hidden;
  border: 1px solid #d6e3f3;
  background:
    linear-gradient(45deg, rgba(47, 109, 255, 0.04) 25%, transparent 25%),
    linear-gradient(-45deg, rgba(47, 109, 255, 0.04) 25%, transparent 25%),
    linear-gradient(45deg, transparent 75%, rgba(47, 109, 255, 0.04) 75%),
    linear-gradient(-45deg, transparent 75%, rgba(47, 109, 255, 0.04) 75%);
  background-size: 24px 24px;
  background-position: 0 0, 0 12px, 12px -12px, -12px 0;
}

.zoom-surface {
  position: absolute;
}

.zoom-focus-block {
  position: absolute;
  border: var(--focus-line-size, 2px) solid rgba(74, 214, 255, 0.98);
  background: rgba(74, 214, 255, 0.05);
  box-shadow:
    0 0 0 1px rgba(255, 255, 255, 0.7);
}

.focus-divider {
  position: absolute;
  background: rgba(74, 214, 255, 0.98);
  box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.35);
}

.focus-divider.vertical {
  top: calc(var(--focus-line-size, 2px) * -1);
  bottom: calc(var(--focus-line-size, 2px) * -1);
  width: var(--focus-line-size, 2px);
}

.focus-divider.horizontal {
  left: calc(var(--focus-line-size, 2px) * -1);
  right: calc(var(--focus-line-size, 2px) * -1);
  height: var(--focus-line-size, 2px);
}

.zoom-move {
  display: grid;
  gap: 10px;
}

.move-pad {
  display: grid;
  grid-template-columns: repeat(3, 52px);
  grid-template-areas:
    ". up ."
    "left . right"
    ". down .";
  justify-content: center;
  gap: 8px;
}

.move-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  height: 40px;
  border: 1px solid #d8e3f2;
  border-radius: 14px;
  background: linear-gradient(180deg, #ffffff, #f6f9ff);
  color: #18304f;
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 10px 20px rgba(18, 32, 51, 0.08);
}

.move-btn.up {
  grid-area: up;
}

.move-btn.left {
  grid-area: left;
}

.move-btn.right {
  grid-area: right;
}

.move-btn.down {
  grid-area: down;
}

.arrow-icon {
  display: inline-block;
  width: 10px;
  height: 10px;
  border-top: 2px solid #18304f;
  border-right: 2px solid #18304f;
}

.arrow-icon.up {
  transform: rotate(-45deg);
}

.arrow-icon.right {
  transform: rotate(45deg);
}

.arrow-icon.down {
  transform: rotate(135deg);
}

.arrow-icon.left {
  transform: rotate(-135deg);
}

.selection-actions {
  margin-top: 12px;
  flex-wrap: wrap;
}

.mini-summary {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  color: #617089;
  font-size: 12px;
}

.field-stack {
  margin-top: 14px;
}

.trim-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, max-content));
  gap: 10px;
  align-items: center;
}

.trim-btn {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  height: 38px;
  padding: 0 12px;
  border: 1px solid #d8e3f2;
  border-radius: 14px;
  background: linear-gradient(180deg, #ffffff, #f7f9fd);
  color: #18304f;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 10px 20px rgba(18, 32, 51, 0.05);
}

.size-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
  margin-top: 14px;
}

.field-card {
  display: grid;
  gap: 8px;
  padding: 12px;
  border-radius: 16px;
  border: 1px solid #dbe4f2;
  background: #ffffff;
}

.wide-card {
  width: 100%;
}

.field-card span,
.toggle-row strong,
.option-head h4 {
  color: #18304f;
  font-size: 13px;
  font-weight: 700;
}

.field-card input {
  width: 100%;
}

.stepper-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: center;
}

.stepper-label {
  color: #18304f;
  font-size: 14px;
}

.stepper-control {
  display: flex;
  align-items: center;
  gap: 8px;
}

.stepper-btn {
  width: 36px;
  height: 36px;
  border: 1px solid #d8e3f2;
  border-radius: 12px;
  background: linear-gradient(180deg, #ffffff, #f6f9ff);
  color: #18304f;
  font-size: 18px;
  cursor: pointer;
  box-shadow: 0 10px 18px rgba(18, 32, 51, 0.05);
}

.stepper-input {
  width: 88px;
  height: 38px;
  padding: 0 10px;
  border: 1px solid #dbe4f2;
  border-radius: 12px;
  background: #fff;
  color: #18304f;
  text-align: center;
}

.stepper-unit {
  color: #617089;
  font-size: 13px;
  font-weight: 700;
}

.toggle-row {
  margin-top: 14px;
  padding: 14px;
  border-radius: 16px;
  border: 1px solid #dbe4f2;
  background: #ffffff;
}

.status-box {
  margin-top: 14px;
  padding: 14px;
  border-radius: 16px;
  background: #ffffff;
  border: 1px solid #dbe4f2;
  color: #617089;
  font-size: 13px;
}

.loading-box {
  border-color: #cfe0f4;
  background: linear-gradient(180deg, #ffffff, #f4f8ff);
  color: #24507a;
  font-weight: 700;
}

.status-box.info {
  display: grid;
  gap: 8px;
}

.status-row strong,
.detail-row strong {
  color: #18304f;
}

.review-panel {
  max-width: none;
}

.group-switcher {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-bottom: 14px;
}

.group-chip {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  height: 38px;
  padding: 0 12px;
  border: 1px solid #d7e3f3;
  border-radius: 999px;
  background: #ffffff;
  color: #35516f;
  cursor: pointer;
  box-shadow: 0 8px 16px rgba(18, 32, 51, 0.05);
}

.group-chip i {
  width: 12px;
  height: 12px;
  border-radius: 999px;
  border: 1px solid rgba(18, 32, 51, 0.08);
}

.group-chip span,
.group-chip strong {
  font-size: 12px;
  font-weight: 700;
}

.group-chip em {
  min-width: 22px;
  height: 22px;
  padding: 0 6px;
  border-radius: 999px;
  background: #fff2dd;
  color: #a45f16;
  font-size: 11px;
  font-style: normal;
  font-weight: 800;
  line-height: 22px;
  text-align: center;
}

.group-chip.active {
  border-color: #79b7ff;
  background: linear-gradient(180deg, #ffffff, #edf5ff);
  color: #18304f;
  box-shadow: 0 12px 24px rgba(47, 127, 255, 0.12);
}

.review-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(96px, 1fr));
  gap: 10px;
  max-height: 72vh;
  overflow: auto;
  padding-right: 4px;
}

.sheet-review-grid {
  grid-template-columns: repeat(auto-fill, minmax(128px, 1fr));
  gap: 12px;
}

.review-tile {
  display: grid;
  gap: 10px;
  padding: 10px;
  border: 1px solid #dbe4f2;
  border-radius: 16px;
  background: #ffffff;
  cursor: pointer;
}

.simple-tile {
  cursor: default;
}

.tile-flags {
  display: flex;
  justify-content: flex-end;
  min-height: 20px;
}

.flag-chip {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 40px;
  height: 20px;
  padding: 0 8px;
  border-radius: 999px;
  background: #edf5ff;
  color: #2f67b7;
  font-size: 11px;
  font-weight: 800;
}

.flag-chip.warm {
  background: #fff1db;
  color: #ac6615;
}

.review-tile.active {
  border-color: #79b7ff;
  box-shadow: 0 0 0 2px rgba(121, 183, 255, 0.16);
}

.review-thumb-shell,
.detail-fragment-shell {
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 14px;
  border: 1px solid #dbe4f2;
  background: #f6faff;
}

.review-thumb-shell {
  min-height: 64px;
  padding: 8px;
}

.large-shell {
  min-height: 96px;
  padding: 10px;
}

.detail-fragment-shell {
  min-height: 220px;
  padding: 14px;
}

.review-fragment,
.detail-fragment {
  box-shadow: 0 8px 20px rgba(18, 32, 51, 0.08);
}

.review-tile-meta {
  display: grid;
  gap: 6px;
}

.compact-meta {
  justify-items: center;
  text-align: center;
}

.review-tile-meta span,
.detail-row span,
.option-head span {
  color: #617089;
  font-size: 12px;
}

.review-code-row {
  justify-content: flex-start;
  gap: 8px;
}

.big-code-row {
  justify-content: center;
}

.review-code-row i,
.review-option i {
  width: 14px;
  height: 14px;
  border-radius: 999px;
  border: 1px solid rgba(18, 32, 51, 0.08);
}

.review-code-row strong,
.review-option span {
  color: #18304f;
  font-size: 12px;
  font-weight: 700;
}

.confirm-panel {
  display: grid;
  gap: 12px;
  margin-top: 16px;
  padding: 14px;
  border-radius: 18px;
  border: 1px solid #dbe4f2;
  background: #ffffff;
}

.confirm-preview {
  display: grid;
  grid-template-columns: 148px 1fr;
  gap: 14px;
  align-items: center;
}

.confirm-options {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(96px, 1fr));
  gap: 8px;
}

.review-option {
  display: grid;
  grid-template-columns: 14px 1fr;
  gap: 8px;
  align-items: center;
  height: 36px;
  padding: 0 10px;
  border: 1px solid #dbe4f2;
  border-radius: 12px;
  background: linear-gradient(180deg, #ffffff, #f7fbff);
  cursor: pointer;
  text-align: left;
  box-shadow: 0 8px 16px rgba(18, 32, 51, 0.05);
}

.review-option.active {
  border-color: #79b7ff;
  background: #eef6ff;
  box-shadow: 0 12px 22px rgba(47, 127, 255, 0.12);
}

.preview-result {
  margin-top: 14px;
}

.preview-grid {
  display: grid;
  gap: 2px;
  padding: 14px;
  border-radius: 18px;
  background: #182331;
}

.preview-pixel {
  display: block;
  aspect-ratio: 1;
  border-radius: 2px;
}

.empty-state {
  padding: 40px 18px;
  border-radius: 18px;
  border: 1px dashed #c8d7eb;
  background: #ffffff;
  color: #617089;
  text-align: center;
}

.empty-state.compact {
  margin-top: 12px;
}

.dialog-actions {
  margin-top: 18px;
  padding-top: 18px;
  border-top: 1px solid #e4ecf7;
}

@media (max-width: 960px) {
  .sheet-dialog-mask {
    padding: 14px;
  }

  .sheet-dialog {
    padding: 18px;
    border-radius: 20px;
  }

  .calibration-layout {
    grid-template-columns: 1fr;
  }

  .crop-compare-layout {
    grid-template-columns: 1fr;
  }

  .zoom-stage {
    width: min(100%, var(--zoom-stage-size));
    height: min(100vw - 120px, var(--zoom-stage-size));
  }

  .review-grid {
    max-height: 42vh;
  }

  .range-row {
    grid-template-columns: 1fr;
  }

  .confirm-preview {
    grid-template-columns: 1fr;
  }

  .trim-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .selection-actions,
  .dialog-actions,
  .toggle-row,
  .section-head {
    flex-direction: column;
    align-items: stretch;
  }

  .size-grid {
    grid-template-columns: 1fr;
  }
}
</style>
