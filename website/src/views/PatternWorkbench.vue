<template>
  <div class="pattern-workbench">
    <section class="workbench-header">
      <div>
        <p class="eyebrow">图片与图纸生成</p>
        <h1 class="title">先选一种导入方式</h1>
        <p class="desc">
          先走你当前需要的流程，生成完成后再进入编辑和检查，不把复杂操作一开始全堆出来。
        </p>
      </div>
      <div v-if="!showStarterHub" class="header-actions">
        <button class="secondary-btn" type="button" @click="triggerFilePick('numbered-sheet-image')">
          上传拼豆图纸
        </button>
        <button class="secondary-btn" type="button" @click="$router.push('/editor')">
          打开基础编辑器
        </button>
        <button class="primary-btn" type="button" @click="openCreateWizard('image')">
          图片生成
        </button>
      </div>
    </section>

    <section v-if="showStarterHub" class="starter-hub">
      <article class="starter-card">
        <span class="starter-tag">空白开始</span>
        <h2>空白画板</h2>
        <p>适合从零开始绘制，直接进入画板创作。</p>
        <button class="primary-btn" type="button" @click="openCreateWizard('blank')">
          开始新建
        </button>
      </article>

      <article class="starter-card">
        <span class="starter-tag">快速生成</span>
        <h2>图片生成</h2>
        <p>上传图片后按步骤处理：裁剪、尺寸、预览、确认。</p>
        <button class="primary-btn" type="button" @click="openCreateWizard('image')">
          选择图片
        </button>
      </article>

      <article class="starter-card">
        <span class="starter-tag">图纸处理</span>
        <h2>拼豆图纸生成</h2>
        <p>适合带编号的拼豆图纸，按图纸步骤识别并确认颜色。</p>
        <button class="primary-btn" type="button" @click="triggerFilePick('numbered-sheet-image')">
          上传图纸
        </button>
      </article>
    </section>

    <template v-else>
    <section class="overview-strip">
      <article
        v-for="item in overviewCards"
        :key="item.label"
        class="overview-card"
      >
        <span>{{ item.label }}</span>
        <strong>{{ item.value }}</strong>
        <p>{{ item.note }}</p>
      </article>
    </section>

    <section class="workbench-layout">
      <aside class="panel left-panel">
        <div class="panel-block">
          <PatternSourcePanel
            :document="document"
            :reference-document="referenceDocument"
            :capture-review-document="captureReviewDocument"
            :import-report="lastImportReport"
            @trigger-import="triggerFilePick"
            @clear-reference="workbenchStore.clearReferenceDocument"
            @clear-capture-review="workbenchStore.clearCaptureReviewDocument"
            @export-json="downloadCurrentJson"
          />
        </div>

        <div class="panel-block">
          <div class="panel-head">
            <h2>保真重点</h2>
            <span>只保留关键项</span>
          </div>
          <ul class="todo-list">
            <li>优先保证主体轮廓、颜色块和关键细节不要跑偏</li>
            <li>如果实物颜色有偏差，再上传实拍图做保真校正</li>
            <li>标准化替换只负责把明显偏色收回到更稳定的色号</li>
          </ul>
        </div>

        <div class="panel-block">
          <div class="panel-head">
            <h2>当前统计</h2>
            <span>{{ usedColors.length }} 种颜色</span>
          </div>
          <div class="stats-grid">
            <div class="stat-card">
              <span>问题数</span>
              <strong>{{ issues.length }}</strong>
            </div>
            <div class="stat-card">
              <span>缺失点</span>
              <strong>{{ diffResult.missing.length }}</strong>
            </div>
            <div class="stat-card">
              <span>多余点</span>
              <strong>{{ diffResult.extra.length }}</strong>
            </div>
            <div class="stat-card">
              <span>颜色偏差</span>
              <strong>{{ diffResult.changed.length }}</strong>
            </div>
          </div>
          <div v-if="usedColors.length" class="color-stats">
            <div
              v-for="item in usedColors.slice(0, 8)"
              :key="item.code"
              class="color-row"
            >
              <span class="color-dot" :style="{ backgroundColor: item.hex }"></span>
              <span>{{ item.code }}</span>
              <strong>{{ item.count }}</strong>
            </div>
          </div>
        </div>
      </aside>

      <main class="panel center-panel">
          <div class="canvas-toolbar">
            <div class="view-tabs">
              <button
              v-for="item in viewOptions"
              :key="item.value"
              class="view-tab"
              :class="{ active: activeView === item.value }"
              type="button"
              @click="workbenchStore.setActiveView(item.value)"
            >
              {{ item.label }}
            </button>
            </div>
            <div class="canvas-actions">
              <button class="toolbar-btn" type="button" @click="adjustStagePixelSize(-2)">缩小</button>
              <button class="toolbar-btn" type="button" @click="adjustStagePixelSize(2)">放大</button>
              <button class="toolbar-btn" type="button" @click="toggleMagnifier">
                {{ showMagnifier ? '关闭放大' : '打开放大' }}
              </button>
            </div>
          </div>

        <div class="canvas-stage">
          <div class="stage-card current">
            <span class="stage-label">{{ stageTitle }}</span>
            <div
              class="stage-grid real-grid"
              :style="stageGridStyle"
            >
              <span
                v-for="cell in stageCells"
                :key="cell.key"
                class="stage-pixel real"
                :class="[
                  cell.variant,
                  {
                    selected: cell.key === selectedPositionKey,
                    'region-selected': selectionKeySet.has(cell.key),
                    'draw-preview': drawStrokeKeys.includes(cell.key),
                  },
                ]"
                :style="{ background: cell.color }"
                @mousedown.prevent="beginSelection(cell)"
                @mouseenter="handleStageCellHover(cell)"
                @mouseleave="clearStageCellHover"
              ></span>
            </div>
            <div class="stage-focus">
              <div class="focus-row">
                <span>当前焦点</span>
                <strong>{{ selectedPositionText }}</strong>
              </div>
              <div class="focus-row">
                <span>当前颜色</span>
                <strong>{{ selectedPixelContext.currentCode }}</strong>
              </div>
              <div class="focus-row">
                <span>参考颜色</span>
                <strong>{{ selectedPixelContext.referenceCode }}</strong>
              </div>
              <div class="focus-row">
                <span>区域框选</span>
                <strong>{{ selectedRegionCount }} 点</strong>
              </div>
            </div>
          </div>

          <div class="stage-sidecars">
            <div class="stage-card ghost">
              <span class="stage-label">转换说明</span>
              <div class="placeholder-text">
                这里先看图片转像素后的结果，保真校正只在你上传实拍图后才会介入，不会默认塞很多复杂工具。
              </div>
              <ul class="diff-list">
                <li>图纸尺寸：{{ document.width }} × {{ document.height }}</li>
                <li>颜色数：{{ usedColors.length }}</li>
                <li>问题项：{{ issues.length }}</li>
                <li>实拍一致率：{{ captureReviewSummary ? `${Math.round(captureReviewSummary.matchRatio * 100)}%` : '未校验' }}</li>
              </ul>
            </div>

            <div class="stage-card ghost">
              <span class="stage-label">实拍规整图</span>
              <div
                v-if="captureReviewDocument"
                class="stage-grid side-grid"
                :style="{ gridTemplateColumns: `repeat(${captureReviewDocument.width}, minmax(0, 1fr))` }"
              >
                <span
                  v-for="cell in captureReviewCells"
                  :key="cell.key"
                  class="stage-pixel mini real"
                  :style="{ background: cell.color }"
                ></span>
              </div>
              <div v-else class="placeholder-text">
                上传拼豆实拍图后，这里会显示算法规整后的标准图纸结果。
              </div>
            </div>

            <div v-if="showMagnifier" class="stage-card ghost">
              <span class="stage-label">局部放大</span>
              <div class="focus-row magnifier-meta">
                <span>观察点</span>
                <strong>{{ focusPositionText }}</strong>
              </div>
              <div
                class="stage-grid magnifier-grid"
                :style="{ gridTemplateColumns: `repeat(${magnifierSize}, 20px)` }"
              >
                <span
                  v-for="cell in magnifierCells"
                  :key="cell.key"
                  class="stage-pixel magnifier-pixel"
                  :class="{ selected: cell.key === focusPositionKey }"
                  :style="{ background: cell.color }"
                ></span>
              </div>
            </div>

            <div v-if="compareDiffCellsBundle" class="stage-card ghost">
              <span class="stage-label">版本差异预览</span>
              <div class="focus-row magnifier-meta">
                <span>对比对象</span>
                <strong>{{ compareSnapshot.label }}</strong>
              </div>
              <div
                class="stage-grid side-grid"
                :style="{ gridTemplateColumns: `repeat(${compareDiffCellsBundle.width}, minmax(0, 1fr))` }"
              >
                <span
                  v-for="cell in compareDiffCellsBundle.cells"
                  :key="cell.key"
                  class="stage-pixel mini real"
                  :style="{ background: cell.color }"
                ></span>
              </div>
            </div>
          </div>
        </div>
      </main>

      <aside class="panel right-panel">
        <div class="right-panel-shell">
          <div class="right-tabs">
            <button
              v-for="item in rightPanelTabs"
              :key="item.value"
              class="right-tab"
              :class="{ active: rightPanelTab === item.value }"
              type="button"
              @click="rightPanelTab = item.value"
            >
              <span>{{ item.label }}</span>
              <strong>{{ item.badge }}</strong>
            </button>
          </div>

          <div v-if="rightPanelTab === 'confirm'" class="panel-stack">
            <div class="panel-block">
              <PatternColorConfirmPanel
                :groups="colorConfirmGroups"
                :report="lastImportReport"
                :focus-color-code="getDocumentCode(document, focusPosition.x, focusPosition.y)"
                @replace-group-color="handleColorGroupReplacement"
              />
            </div>
          </div>

          <div v-else-if="rightPanelTab === 'edit'" class="panel-stack">
            <div class="panel-block">
              <PatternEditPanel
                :tool-mode="toolMode"
                :selected-color-code="selectedColorCode"
                :selected-color-hex="selectedColorHex"
                :used-colors="usedColors"
                :all-colors="ARTKAL_COLORS_FULL"
                :selection-count="selectedRegionCount"
                :focus-color-code="getDocumentCode(document, focusPosition.x, focusPosition.y)"
                @change-tool="toolMode = $event"
                @select-color="selectedColorCode = $event"
                @selection-action="handleSelectionFill"
                @replace-focus-color="handleReplaceFocusColor"
              />
            </div>
            <div class="panel-block">
              <PatternSelectionPanel
                :selection-count="selectedRegionCount"
                :selection-bounds="selectionBounds"
                :has-reference="!!referenceDocument"
                @region-action="handleRegionAction"
                @clear-selection="clearSelection"
              />
            </div>
          </div>

          <div v-else class="panel-stack">
            <div class="panel-block">
              <div class="panel-head">
                <h2>实拍校验</h2>
                <span>{{ captureReviewSummary ? `${Math.round(captureReviewSummary.matchRatio * 100)}% 一致` : '未开始' }}</span>
              </div>
              <div v-if="captureReviewSummary" class="compare-summary">
                <div class="compare-row">
                  <span>实拍图</span>
                  <strong>{{ captureReviewDocument.name }}</strong>
                </div>
                <div class="compare-row">
                  <span>一致点</span>
                  <strong>{{ captureReviewSummary.matchedCount }}</strong>
                </div>
                <div class="compare-row">
                  <span>不一致点</span>
                  <strong>{{ captureReviewSummary.mismatchCount }}</strong>
                </div>
                <div class="compare-row">
                  <span>缺失</span>
                  <strong>{{ captureReviewDiffResult.missing.length }}</strong>
                </div>
                <div class="compare-row">
                  <span>多余</span>
                  <strong>{{ captureReviewDiffResult.extra.length }}</strong>
                </div>
                <div class="compare-row">
                  <span>颜色变化</span>
                  <strong>{{ captureReviewDiffResult.changed.length }}</strong>
                </div>
              </div>
              <div v-else class="empty-side-note">
                上传实拍图后，系统会自动规整并给出一致率，用户可以直接判断成品是否和图纸一致。
              </div>
            </div>
            <div class="panel-block">
              <PatternStandardizePanel
                :groups="captureReviewChangeGroups"
                @apply-group-replacement="handleCaptureGroupReplacement"
              />
            </div>
          </div>
        </div>
      </aside>
    </section>
    </template>

    <input
      ref="numberedSheetImageInputRef"
      type="file"
      accept=".jpg,.jpeg,.png,.webp,image/jpeg,image/png,image/webp"
      class="hidden-input"
      @change="handleFileChange($event, 'numbered-sheet-image')"
    >
    <input
      ref="currentCsvInputRef"
      type="file"
      accept=".csv,text/csv"
      class="hidden-input"
      @change="handleFileChange($event, 'current-csv')"
    >
    <input
      ref="currentJsonInputRef"
      type="file"
      accept=".json,application/json"
      class="hidden-input"
      @change="handleFileChange($event, 'current-json')"
    >
    <input
      ref="referenceImageInputRef"
      type="file"
      accept=".jpg,.jpeg,.png,.webp,image/jpeg,image/png,image/webp"
      class="hidden-input"
      @change="handleFileChange($event, 'reference-image')"
    >
    <input
      ref="referenceCsvInputRef"
      type="file"
      accept=".csv,text/csv"
      class="hidden-input"
      @change="handleFileChange($event, 'reference-csv')"
    >
    <input
      ref="referenceJsonInputRef"
      type="file"
      accept=".json,application/json"
      class="hidden-input"
      @change="handleFileChange($event, 'reference-json')"
    >
    <input
      ref="captureReviewImageInputRef"
      type="file"
      accept=".jpg,.jpeg,.png,.webp,image/jpeg,image/png,image/webp"
      class="hidden-input"
      @change="handleFileChange($event, 'capture-review-image')"
    >

    <div v-if="loading || error" class="floating-status" :class="{ error: !!error }">
      {{ error || "正在导入与分析..." }}
    </div>

    <PatternSheetImportDialog
      :visible="showNumberedSheetImportDialog"
      :image-url="pendingNumberedSheetUrl"
      :initial-mode="pendingImportMode"
      :preview-document="previewGeneratedDocument"
      :preview-report="previewGeneratedReport"
      :preview-loading="previewGenerating"
      @cancel="closeNumberedSheetImportDialog"
      @preview="handleImportPreview"
      @confirm="handleNumberedSheetImportConfirm"
      @replace-cell-color="handlePreviewCellReplace"
    />

    <PatternCreateWizard
      :visible="showCreateWizard"
      :mode="createWizardMode"
      :preview-document="previewGeneratedDocument"
      :preview-report="previewGeneratedReport"
      :preview-loading="previewGenerating"
      @cancel="closeCreateWizard"
      @preview="handleCreateWizardPreview"
      @confirm="handleCreateWizardConfirm"
    />
  </div>
</template>

<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import { storeToRefs } from 'pinia'
import { ARTKAL_COLORS_FULL, getColorByCode } from '@/data/artkal-colors.js'
import PatternColorConfirmPanel from '@/components/pattern/PatternColorConfirmPanel.vue'
import PatternCreateWizard from '@/components/pattern/PatternCreateWizard.vue'
import PatternEditPanel from '@/components/pattern/PatternEditPanel.vue'
import PatternSelectionPanel from '@/components/pattern/PatternSelectionPanel.vue'
import PatternSheetImportDialog from '@/components/pattern/PatternSheetImportDialog.vue'
import PatternSourcePanel from '@/components/pattern/PatternSourcePanel.vue'
import PatternStandardizePanel from '@/components/pattern/PatternStandardizePanel.vue'
import { importPatternFromImage } from '@/modules/pattern-workbench/core/importers/patternImportImage.js'
import { importPatternFromNumberedSheet } from '@/modules/pattern-workbench/core/importers/patternImportNumberedSheet.js'
import { usePatternWorkbenchStore } from '@/stores/patternWorkbench.js'
import { createPatternDocument, deserializePatternDocument } from '@/modules/pattern-workbench/core/model/patternDocument.js'
import { diffPatternDocuments } from '@/modules/pattern-workbench/core/diff/patternDiff.js'

const viewOptions = [
  { value: 'current', label: '当前图纸' },
  { value: 'diff', label: '差异图' },
  { value: 'overlay', label: '叠加预览' },
]

const workbenchStore = usePatternWorkbenchStore()
const {
  activeView,
  captureReviewDiffResult,
  captureReviewDocument,
  captureReviewSummary,
  currentDocumentExport,
  document,
  referenceDocument,
  diffResult,
  issues,
  lastImportReport,
  selectedIssueId,
  selectedIssue,
  snapshots,
  loading,
  error,
} = storeToRefs(workbenchStore)

const stagePixelSize = ref(8)
const showMagnifier = ref(true)
const hoverPosition = ref(null)
const magnifierSize = 7
const selectionStart = ref(null)
const selectionCurrent = ref(null)
const isSelecting = ref(false)
const toolMode = ref('inspect')
const selectedColorCode = ref('P13')
const drawStrokeKeys = ref([])
const isDrawing = ref(false)
const rightPanelTab = ref('confirm')
const issueFilterType = ref('all')
const compareSnapshotId = ref('')
const blinkUseReference = ref(false)

let blinkTimer = null

const numberedSheetImageInputRef = ref(null)
const currentCsvInputRef = ref(null)
const currentJsonInputRef = ref(null)
const referenceImageInputRef = ref(null)
const referenceCsvInputRef = ref(null)
const referenceJsonInputRef = ref(null)
const captureReviewImageInputRef = ref(null)
const showNumberedSheetImportDialog = ref(false)
const pendingNumberedSheetFile = ref(null)
const pendingNumberedSheetUrl = ref('')
const pendingImportMode = ref('image')
const previewGeneratedDocument = ref(null)
const previewGeneratedReport = ref(null)
const previewGenerating = ref(false)
const showCreateWizard = ref(false)
const createWizardMode = ref('blank')

const codeToHexMap = new Map(
  ARTKAL_COLORS_FULL.map((item) => [item.code, item.hex]),
)

function colorForCode(code) {
  if (!code) {
    return '#152131'
  }
  return codeToHexMap.get(code) || getColorByCode(code)?.hex || '#59a8ff'
}

function getColorDistance(leftCode, rightCode) {
  const left = getColorByCode(leftCode)
  const right = getColorByCode(rightCode)

  if (!left || !right) {
    return Number.POSITIVE_INFINITY
  }

  return Math.sqrt(
    (left.r - right.r) ** 2 +
    (left.g - right.g) ** 2 +
    (left.b - right.b) ** 2,
  )
}

function getDocumentCode(patternDocument, x, y) {
  if (!patternDocument) {
    return ''
  }
  return patternDocument.pixels.get(`${x},${y}`) || ''
}

function buildDocumentCells(patternDocument) {
  const cells = []
  for (let y = 0; y < patternDocument.height; y++) {
    for (let x = 0; x < patternDocument.width; x++) {
      const code = getDocumentCode(patternDocument, x, y)
      cells.push({
        key: `${x},${y}`,
        x,
        y,
        color: code ? colorForCode(code) : '#152131',
        variant: code ? 'filled' : 'empty',
      })
    }
  }
  return cells
}

function buildDiffCells() {
  const targetDocument = captureReviewDocument.value || referenceDocument.value
  const width = Math.max(document.value.width, targetDocument?.width || 0)
  const height = Math.max(document.value.height, targetDocument?.height || 0)
  const changedMap = new Map()

  const targetDiff = captureReviewDocument.value ? captureReviewDiffResult.value : diffResult.value

  targetDiff.missing.forEach((item) => {
    changedMap.set(`${item.x},${item.y}`, '#ffd166')
  })
  targetDiff.extra.forEach((item) => {
    changedMap.set(`${item.x},${item.y}`, '#ff6b6b')
  })
  targetDiff.changed.forEach((item) => {
    changedMap.set(`${item.x},${item.y}`, '#6ea8ff')
  })

  const cells = []
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const key = `${x},${y}`
      cells.push({
        key,
        x,
        y,
        color: changedMap.get(key) || '#152131',
        variant: changedMap.has(key) ? 'filled' : 'empty',
      })
    }
  }

  return {
    width: width || 1,
    cells,
  }
}

function buildOverlayCells() {
  const targetDocument = captureReviewDocument.value || referenceDocument.value
  const width = Math.max(document.value.width, targetDocument?.width || 0)
  const height = Math.max(document.value.height, targetDocument?.height || 0)
  const cells = []

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const key = `${x},${y}`
      const currentCode = getDocumentCode(document.value, x, y)
      const referenceCode = getDocumentCode(targetDocument, x, y)

      if (currentCode && referenceCode && currentCode !== referenceCode) {
        cells.push({
          key,
          x,
          y,
          color: `linear-gradient(135deg, ${colorForCode(currentCode)} 0 50%, ${colorForCode(referenceCode)} 50% 100%)`,
          variant: 'filled',
        })
        continue
      }

      if (!currentCode && referenceCode) {
        cells.push({
          key,
          x,
          y,
          color: colorForCode(referenceCode),
          variant: 'filled',
        })
        continue
      }

      cells.push({
        key,
        x,
        y,
        color: currentCode ? colorForCode(currentCode) : '#152131',
        variant: currentCode ? 'filled' : 'empty',
      })
    }
  }

  return {
    width: width || 1,
    cells,
  }
}

const currentCells = computed(() => buildDocumentCells(document.value))
const captureReviewCells = computed(() =>
  captureReviewDocument.value ? buildDocumentCells(captureReviewDocument.value) : [],
)
const diffCellsBundle = computed(() => buildDiffCells())
const overlayCellsBundle = computed(() => buildOverlayCells())
const filteredIssues = computed(() => issues.value)
const showStarterHub = computed(() => {
  return (
    document.value.id === 'draft-pattern' &&
    document.value.pixels.size === 0 &&
    !lastImportReport.value &&
    snapshots.value.length <= 1
  )
})

const stageTitle = computed(() => {
  if (activeView.value === 'diff') {
    return '差异图'
  }
  if (activeView.value === 'overlay') {
    return '叠加图'
  }
  return '当前图纸'
})

const stageWidth = computed(() => {
  if (activeView.value === 'diff' || activeView.value === 'overlay') {
    return diffCellsBundle.value.width
  }
  return document.value.width || 1
})

const stageCells = computed(() => {
  if (activeView.value === 'diff') {
    return diffCellsBundle.value.cells
  }
  if (activeView.value === 'overlay') {
    return overlayCellsBundle.value.cells
  }
  return currentCells.value
})

const stageGridStyle = computed(() => ({
  gridTemplateColumns: `repeat(${stageWidth.value}, ${stagePixelSize.value}px)`,
}))

const selectionBounds = computed(() => {
  if (!selectionStart.value || !selectionCurrent.value) {
    return null
  }

  const minX = Math.min(selectionStart.value.x, selectionCurrent.value.x)
  const minY = Math.min(selectionStart.value.y, selectionCurrent.value.y)
  const maxX = Math.max(selectionStart.value.x, selectionCurrent.value.x)
  const maxY = Math.max(selectionStart.value.y, selectionCurrent.value.y)

  return {
    minX,
    minY,
    maxX,
    maxY,
  }
})

const selectionKeySet = computed(() => {
  const bounds = selectionBounds.value
  if (!bounds) {
    return new Set()
  }

  const nextKeys = new Set()
  for (let y = bounds.minY; y <= bounds.maxY; y += 1) {
    for (let x = bounds.minX; x <= bounds.maxX; x += 1) {
      nextKeys.add(`${x},${y}`)
    }
  }
  return nextKeys
})

const selectionPositions = computed(() => {
  return Array.from(selectionKeySet.value).map((key) => {
    const [x, y] = key.split(',').map(Number)
    return { x, y }
  })
})

const selectedRegionCount = computed(() => selectionPositions.value.length)

const selectedPositionKey = computed(() => {
  if (!selectedIssue.value) {
    return ''
  }
  if (!selectedIssue.value.position) {
    return ''
  }
  return `${selectedIssue.value.position.x},${selectedIssue.value.position.y}`
})

const selectedPositionText = computed(() => {
  if (!selectedIssue.value) {
    return '未选中'
  }
  if (!selectedIssue.value.position) {
    return '未提供'
  }
  return `(${selectedIssue.value.position.x}, ${selectedIssue.value.position.y})`
})

const selectedPixelContext = computed(() => {
  if (!selectedIssue.value) {
    return {
      currentCode: '无',
      referenceCode: '无',
    }
  }

  const position = selectedIssue.value.position
  if (!position) {
    return {
      currentCode: '无',
      referenceCode: '无',
    }
  }

  const currentCode = getDocumentCode(document.value, position.x, position.y)
  const referenceCode = getDocumentCode(referenceDocument.value, position.x, position.y)

  return {
    currentCode: currentCode ? currentCode : '空',
    referenceCode: referenceCode ? referenceCode : '空',
  }
})

const selectedColorHex = computed(() => colorForCode(selectedColorCode.value))

const focusPosition = computed(() => {
  if (hoverPosition.value) {
    return hoverPosition.value
  }
  if (selectedIssue.value?.position) {
    return selectedIssue.value.position
  }
  return {
    x: Math.floor(document.value.width / 2),
    y: Math.floor(document.value.height / 2),
  }
})

const focusPositionKey = computed(() => `${focusPosition.value.x},${focusPosition.value.y}`)

const focusPositionText = computed(() => `(${focusPosition.value.x}, ${focusPosition.value.y})`)

const magnifierCells = computed(() => {
  const cells = []
  const halfSize = Math.floor(magnifierSize / 2)
  const startX = focusPosition.value.x - halfSize
  const startY = focusPosition.value.y - halfSize

  for (let offsetY = 0; offsetY < magnifierSize; offsetY += 1) {
    for (let offsetX = 0; offsetX < magnifierSize; offsetX += 1) {
      const x = startX + offsetX
      const y = startY + offsetY
      const code = getDocumentCode(document.value, x, y)
      cells.push({
        key: `${x},${y}`,
        color: code ? colorForCode(code) : '#152131',
      })
    }
  }

  return cells
})

const usedColors = computed(() => {
  const counts = new Map()
  document.value.pixels.forEach((code) => {
    counts.set(code, (counts.get(code) || 0) + 1)
  })

  return Array.from(counts.entries())
    .map(([code, count]) => ({
      code,
      count,
      hex: colorForCode(code),
    }))
    .sort((a, b) => b.count - a.count)
})

const colorConfirmGroups = computed(() => {
  return usedColors.value.map((item) => {
    const similarOptions = ARTKAL_COLORS_FULL
      .filter((color) => color.code !== item.code)
      .map((color) => ({
        code: color.code,
        hex: color.hex,
        distance: getColorDistance(item.code, color.code),
      }))
      .sort((left, right) => left.distance - right.distance)
      .slice(0, 6)

    return {
      ...item,
      name: getColorByCode(item.code)?.name || '',
      similarOptions,
    }
  })
})

const overviewCards = computed(() => {
  const matchRatioText = captureReviewSummary.value
    ? `${Math.round(captureReviewSummary.value.matchRatio * 100)}%`
    : '未校验'

  return [
    {
      label: '当前图纸',
      value: `${document.value.width} × ${document.value.height}`,
      note: `${usedColors.value.length} 种颜色正在使用`,
    },
    {
      label: '待处理问题',
      value: `${issues.value.length}`,
      note: `${filteredIssues.value.length} 项正在显示`,
    },
    {
      label: '实拍一致率',
      value: matchRatioText,
      note: captureReviewSummary.value
        ? `${captureReviewSummary.value.mismatchCount} 处待校正`
        : '上传实拍图后自动计算',
    },
    {
      label: '标准化建议',
      value: `${captureReviewChangeGroups.value.length}`,
      note: captureReviewChangeGroups.value.length
        ? '可直接应用颜色替换'
        : '暂无颜色偏差聚合',
    },
  ]
})

const rightPanelTabs = computed(() => [
  {
    value: 'confirm',
    label: '颜色确认',
    badge: `${colorConfirmGroups.value.length}`,
  },
  {
    value: 'edit',
    label: '手动调整',
    badge: toolMode.value === 'paint' ? '画笔' : toolMode.value === 'erase' ? '橡皮' : toolMode.value === 'select' ? '框选' : '查看',
  },
  {
    value: 'review',
    label: '实拍校验',
    badge: captureReviewSummary.value ? `${Math.round(captureReviewSummary.value.matchRatio * 100)}%` : '--',
  },
])

const compareSnapshot = computed(() => {
  if (!compareSnapshotId.value) {
    return null
  }
  return snapshots.value.find((item) => item.id === compareSnapshotId.value) || null
})

const compareSummary = computed(() => {
  if (!compareSnapshot.value || !compareSnapshot.value.document) {
    return null
  }

  const targetDocument = deserializePatternDocument(compareSnapshot.value.document)
  const compareDiff = diffPatternDocuments(document.value, targetDocument)

  return {
    missing: compareDiff.missing.length,
    extra: compareDiff.extra.length,
    changed: compareDiff.changed.length,
  }
})

const captureReviewChangeGroups = computed(() => {
  if (!captureReviewDiffResult.value.changed.length) {
    return []
  }

  const groupMap = new Map()
  const documentColorCounts = new Map()

  document.value.pixels.forEach((code) => {
    documentColorCounts.set(code, (documentColorCounts.get(code) || 0) + 1)
  })

  captureReviewDiffResult.value.changed.forEach((item) => {
    const groupKey = `${item.current}->${item.reference}`
    if (!groupMap.has(groupKey)) {
      groupMap.set(groupKey, {
        id: groupKey,
        sourceCode: item.current,
        targetCode: item.reference,
        count: 0,
        positions: [],
      })
    }

    const targetGroup = groupMap.get(groupKey)
    targetGroup.count += 1
    targetGroup.positions.push({
      x: item.x,
      y: item.y,
    })
  })

  return Array.from(groupMap.values())
    .map((group) => {
      const similarOptions = ARTKAL_COLORS_FULL
        .filter((item) => item.code !== group.targetCode && item.code !== group.sourceCode)
        .map((item) => ({
          code: item.code,
          hex: item.hex,
          distance: getColorDistance(group.targetCode, item.code),
        }))
        .sort((left, right) => left.distance - right.distance)
        .slice(0, 3)

      return {
        ...group,
        estimatedFixCount: group.count,
        globalSourceCount: documentColorCounts.get(group.sourceCode) || group.count,
        sourceHex: colorForCode(group.sourceCode),
        targetHex: colorForCode(group.targetCode),
        similarOptions,
      }
    })
    .sort((left, right) => right.count - left.count)
})

const compareDiffCellsBundle = computed(() => {
  if (!compareSnapshot.value || !compareSnapshot.value.document) {
    return null
  }

  const targetDocument = deserializePatternDocument(compareSnapshot.value.document)
  const compareDiff = diffPatternDocuments(document.value, targetDocument)
  const width = Math.max(document.value.width, targetDocument.width)
  const height = Math.max(document.value.height, targetDocument.height)
  const changedMap = new Map()

  compareDiff.missing.forEach((item) => {
    changedMap.set(`${item.x},${item.y}`, '#ffd166')
  })
  compareDiff.extra.forEach((item) => {
    changedMap.set(`${item.x},${item.y}`, '#ff6b6b')
  })
  compareDiff.changed.forEach((item) => {
    changedMap.set(`${item.x},${item.y}`, '#6ea8ff')
  })

  const cells = []
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const key = `${x},${y}`
      cells.push({
        key,
        color: changedMap.get(key) || '#152131',
      })
    }
  }

  return {
    width: width || 1,
    cells,
  }
})

const inputMap = {
  'numbered-sheet-image': numberedSheetImageInputRef,
  'current-csv': currentCsvInputRef,
  'current-json': currentJsonInputRef,
  'reference-image': referenceImageInputRef,
  'reference-csv': referenceCsvInputRef,
  'reference-json': referenceJsonInputRef,
  'capture-review-image': captureReviewImageInputRef,
}

function triggerFilePick(type) {
  if (type === 'current-image') {
    openCreateWizard('image')
    return
  }
  inputMap[type]?.value?.click()
}

function clearGeneratedPreview() {
  previewGeneratedDocument.value = null
  previewGeneratedReport.value = null
  previewGenerating.value = false
}

function openCreateWizard(mode) {
  createWizardMode.value = mode
  showCreateWizard.value = true
  clearGeneratedPreview()
}

function closeCreateWizard() {
  showCreateWizard.value = false
  clearGeneratedPreview()
}

function releasePendingNumberedSheet() {
  if (pendingNumberedSheetUrl.value) {
    URL.revokeObjectURL(pendingNumberedSheetUrl.value)
  }
  pendingNumberedSheetFile.value = null
  pendingNumberedSheetUrl.value = ''
  clearGeneratedPreview()
}

function closeNumberedSheetImportDialog() {
  showNumberedSheetImportDialog.value = false
  releasePendingNumberedSheet()
}

function openNumberedSheetImportDialog(file, mode = 'image') {
  releasePendingNumberedSheet()
  pendingNumberedSheetFile.value = file
  pendingNumberedSheetUrl.value = URL.createObjectURL(file)
  pendingImportMode.value = mode
  showNumberedSheetImportDialog.value = true
}

function handleIssueFilterChange(value) {
  issueFilterType.value = value
}

function beginSelection(cell) {
  if (toolMode.value === 'paint' || toolMode.value === 'erase') {
    beginDraw(cell)
    return
  }
  if (toolMode.value !== 'select') {
    return
  }
  selectionStart.value = {
    x: cell.x,
    y: cell.y,
  }
  selectionCurrent.value = {
    x: cell.x,
    y: cell.y,
  }
  isSelecting.value = true
}

function adjustStagePixelSize(step) {
  const nextValue = stagePixelSize.value + step
  if (nextValue < 4) {
    stagePixelSize.value = 4
    return
  }
  if (nextValue > 18) {
    stagePixelSize.value = 18
    return
  }
  stagePixelSize.value = nextValue
}

function toggleMagnifier() {
  showMagnifier.value = !showMagnifier.value
}

function handleStageCellHover(cell) {
  hoverPosition.value = {
    x: cell.x,
    y: cell.y,
  }
  if (isDrawing.value) {
    appendDrawCell(cell)
  }
  if (isSelecting.value) {
    selectionCurrent.value = {
      x: cell.x,
      y: cell.y,
    }
  }
}

function clearStageCellHover() {
  hoverPosition.value = null
}

function finishSelection() {
  finishDraw()
  isSelecting.value = false
}

function clearSelection() {
  selectionStart.value = null
  selectionCurrent.value = null
  isSelecting.value = false
}

function handleIssueAction(actionType) {
  if (!selectedIssueId.value) {
    return
  }
  workbenchStore.applyIssueAction(selectedIssueId.value, actionType)
}

function handleBatchAction(actionType) {
  const issueIds = filteredIssues.value.map((item) => item.id)
  workbenchStore.applyIssueActionBatch(issueIds, actionType)
}

function handleRegionAction(actionType) {
  if (!selectionPositions.value.length) {
    return
  }
  const handled = workbenchStore.applyRegionAction(selectionPositions.value, actionType)
  if (handled) {
    clearSelection()
  }
}

function beginDraw(cell) {
  if (toolMode.value !== 'paint' && toolMode.value !== 'erase') {
    return
  }
  drawStrokeKeys.value = [`${cell.x},${cell.y}`]
  isDrawing.value = true
}

function appendDrawCell(cell) {
  const key = `${cell.x},${cell.y}`
  if (drawStrokeKeys.value.includes(key)) {
    return
  }
  drawStrokeKeys.value = [...drawStrokeKeys.value, key]
}

function finishDraw() {
  if (!isDrawing.value || !drawStrokeKeys.value.length) {
    isDrawing.value = false
    drawStrokeKeys.value = []
    return
  }

  const positions = drawStrokeKeys.value.map((key) => {
    const [x, y] = key.split(',').map(Number)
    return { x, y }
  })
  workbenchStore.applyBrushStroke(positions, toolMode.value, selectedColorCode.value)
  isDrawing.value = false
  drawStrokeKeys.value = []
}

function handleSelectionFill() {
  if (!selectionPositions.value.length || !selectedColorCode.value) {
    return
  }
  const handled = workbenchStore.applyRegionFill(selectionPositions.value, selectedColorCode.value)
  if (handled) {
    clearSelection()
  }
}

function handleReplaceFocusColor() {
  const sourceCode = getDocumentCode(document.value, focusPosition.value.x, focusPosition.value.y)
  if (!sourceCode || !selectedColorCode.value) {
    return
  }
  workbenchStore.replaceColor(sourceCode, selectedColorCode.value)
}

function handleColorGroupReplacement(sourceCode, targetCode) {
  if (!sourceCode || !targetCode) {
    return
  }
  workbenchStore.replaceColor(sourceCode, targetCode)
}

function handlePreviewCellReplace(cellKey, targetCode) {
  if (!cellKey || !targetCode || !previewGeneratedDocument.value) {
    return
  }

  const nextDocument = deserializePatternDocument(previewGeneratedDocument.value)
  const currentCode = nextDocument.pixels.get(cellKey) || ''
  if (currentCode === targetCode) {
    return
  }

  nextDocument.pixels.set(cellKey, targetCode)
  previewGeneratedDocument.value = nextDocument

  if (!previewGeneratedReport.value?.reviewCells) {
    return
  }

  previewGeneratedReport.value = {
    ...previewGeneratedReport.value,
    reviewCells: previewGeneratedReport.value.reviewCells.map((item) =>
      item.key === cellKey
        ? {
          ...item,
          code: targetCode,
        }
        : item,
    ),
  }
}

function handleCaptureGroupReplacement(group, targetCode, scope) {
  if (!group || !targetCode) {
    return
  }

  if (scope === 'global') {
    workbenchStore.replaceColor(group.sourceCode, targetCode)
    return
  }

  workbenchStore.replaceColor(group.sourceCode, targetCode, group.positions)
}

function handleRestoreSnapshot(snapshotId) {
  workbenchStore.restoreSnapshot(snapshotId)
}

function toggleCompareSnapshot(snapshotId) {
  if (compareSnapshotId.value === snapshotId) {
    compareSnapshotId.value = ''
    return
  }
  compareSnapshotId.value = snapshotId
}

function downloadCurrentJson() {
  const jsonText = JSON.stringify(currentDocumentExport.value, null, 2)
  const blob = new Blob([jsonText], { type: 'application/json;charset=utf-8' })
  const objectUrl = URL.createObjectURL(blob)
  const link = globalThis.document.createElement('a')

  let fileName = 'pattern-workbench'
  if (document.value.name) {
    fileName = document.value.name
  }

  link.href = objectUrl
  link.download = `${fileName}.json`
  link.click()
  URL.revokeObjectURL(objectUrl)
}

watch(
  filteredIssues,
  (list) => {
    if (!list.length) {
      if (selectedIssueId.value) {
        workbenchStore.selectIssue('')
      }
      return
    }

    const hasSelectedIssue = list.some((item) => item.id === selectedIssueId.value)
    if (!hasSelectedIssue) {
      workbenchStore.selectIssue(list[0].id)
    }
  },
  { immediate: true },
)

onMounted(() => {
  blinkTimer = globalThis.setInterval(() => {
    blinkUseReference.value = !blinkUseReference.value
  }, 800)
  globalThis.addEventListener('mouseup', finishSelection)
})

onUnmounted(() => {
  if (blinkTimer) {
    globalThis.clearInterval(blinkTimer)
  }
  globalThis.removeEventListener('mouseup', finishSelection)
  releasePendingNumberedSheet()
})

async function handleNumberedSheetImportConfirm(payload) {
  if (!pendingNumberedSheetFile.value) {
    return
  }

  try {
    previewGenerating.value = true
    const result =
      previewGeneratedDocument.value && previewGeneratedReport.value
        ? {
            document: previewGeneratedDocument.value,
            report: previewGeneratedReport.value,
          }
        : payload.mode === 'sheet'
          ? await importPatternFromNumberedSheet(pendingNumberedSheetFile.value, {
              crop: payload.crop,
              cellSample: payload.cellSample,
              removeBackground: payload.removeBackground,
              width: payload.width,
              height: payload.height,
            })
          : await importPatternFromImage(pendingNumberedSheetFile.value, {
              crop: payload.crop,
              width: payload.width,
              height: payload.height,
            })

    workbenchStore.applyGeneratedResult(
      result,
      `${payload.mode === 'sheet' ? '图纸生成' : '图片生成'} ${result.document.name}`,
      'generated',
    )
    rightPanelTab.value = 'confirm'
    closeNumberedSheetImportDialog()
  } catch (importError) {
    console.error('编号图纸识别失败:', importError)
  } finally {
    previewGenerating.value = false
  }
}

async function handleCreateWizardPreview(payload) {
  if (!payload || payload.mode !== 'image' || !payload.imageFile) {
    return
  }

  try {
    previewGenerating.value = true
    const result = await importPatternFromImage(payload.imageFile, {
      width: payload.width,
      height: payload.height,
      colorCodes: payload.colorCodes,
      name: payload.name,
    })

    previewGeneratedDocument.value = result.document
    previewGeneratedReport.value = result.report
  } catch (previewError) {
    console.error('生成预览失败:', previewError)
  } finally {
    previewGenerating.value = false
  }
}

async function handleCreateWizardConfirm(payload) {
  if (!payload) {
    return
  }

  if (payload.mode === 'blank') {
    const nextDocument = createPatternDocument({
      id: `blank-${Date.now()}`,
      name: payload.name,
      width: payload.width,
      height: payload.height,
      pixels: new Map(),
      sourceType: 'blank',
    })

    workbenchStore.lastImportReport = null
    workbenchStore.applyCurrentDocument(nextDocument, `新建空白画板 ${payload.name}`, 'created')
    rightPanelTab.value = 'confirm'
    closeCreateWizard()
    return
  }

  if (!payload.imageFile) {
    return
  }

  try {
    previewGenerating.value = true
    const result =
      previewGeneratedDocument.value && previewGeneratedReport.value
        ? {
            document: previewGeneratedDocument.value,
            report: previewGeneratedReport.value,
          }
        : await importPatternFromImage(payload.imageFile, {
            width: payload.width,
            height: payload.height,
            colorCodes: payload.colorCodes,
            name: payload.name,
          })

    workbenchStore.applyGeneratedResult(
      result,
      `图片生成 ${payload.name}`,
      'generated',
    )
    rightPanelTab.value = 'confirm'
    closeCreateWizard()
  } catch (confirmError) {
    console.error('图片生成失败:', confirmError)
  } finally {
    previewGenerating.value = false
  }
}

async function handleImportPreview(payload) {
  if (!pendingNumberedSheetFile.value) {
    return
  }

  try {
    previewGenerating.value = true
    const result =
      payload.mode === 'sheet'
        ? await importPatternFromNumberedSheet(pendingNumberedSheetFile.value, {
            crop: payload.crop,
            cellSample: payload.cellSample,
            removeBackground: payload.removeBackground,
            width: payload.width,
            height: payload.height,
          })
        : await importPatternFromImage(pendingNumberedSheetFile.value, {
            crop: payload.crop,
            width: payload.width,
            height: payload.height,
          })

    previewGeneratedDocument.value = result.document
    previewGeneratedReport.value = result.report
  } catch (previewError) {
    console.error('生成预览失败:', previewError)
  } finally {
    previewGenerating.value = false
  }
}

async function handleFileChange(event, type) {
  const target = event.target
  const file = target.files?.[0]
  if (!file) {
    return
  }

  try {
    if (type === 'numbered-sheet-image') {
      openNumberedSheetImportDialog(file, 'sheet')
    } else if (type === 'reference-image') {
      await workbenchStore.importReferenceImage(file, {
        width: document.value.width,
        height: document.value.height,
      })
    } else if (type === 'capture-review-image') {
      await workbenchStore.importCaptureReviewImage(file, {
        width: document.value.width,
        height: document.value.height,
        denoisePasses: 2,
        contrast: 0.14,
        saturation: 0.1,
      })
    } else {
      const text = await file.text()
      if (type === 'current-csv') {
        workbenchStore.importCurrentCsv(text, { name: file.name })
      } else if (type === 'current-json') {
        workbenchStore.importCurrentJson(text)
      } else if (type === 'reference-csv') {
        workbenchStore.importReferenceCsv(text, { name: file.name })
      } else if (type === 'reference-json') {
        workbenchStore.importReferenceJson(text)
      }
    }
  } catch (importError) {
    console.error('导入失败:', importError)
  } finally {
    target.value = ''
  }
}
</script>

<style scoped>
.pattern-workbench {
  max-width: 1520px;
  margin: 0 auto;
  padding: 24px 24px 48px;
}

.workbench-header {
  display: flex;
  justify-content: space-between;
  gap: 24px;
  align-items: flex-start;
  padding: 28px 32px;
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  background:
    linear-gradient(90deg, rgba(255, 243, 196, 0.62), rgba(255, 255, 255, 0.98) 56%, rgba(220, 235, 255, 0.62));
  box-shadow: var(--nb-shadow-card);
}

.starter-hub {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 18px;
  margin-top: 20px;
}

.starter-card {
  padding: 24px;
  border-radius: 0;
  border: 3px solid var(--nb-ink);
  background: var(--tone-paper-soft);
  box-shadow: var(--nb-shadow-card);
}

.starter-tag {
  display: inline-flex;
  padding: 6px 10px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: var(--tone-blue-soft);
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: 800;
  box-shadow: var(--nb-shadow-soft);
}

.starter-card h2 {
  margin-top: 14px;
  color: #122033;
  font-size: 24px;
}

.starter-card p {
  margin-top: 10px;
  min-height: 66px;
  color: #617089;
  font-size: 14px;
  line-height: 1.7;
}

.starter-card .primary-btn {
  margin-top: 10px;
}

.eyebrow {
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: #2f6dff;
}

.title {
  margin-top: 10px;
  font-size: 34px;
  line-height: 1.2;
  color: #122033;
}

.desc {
  margin-top: 12px;
  max-width: 760px;
  font-size: 15px;
  line-height: 1.7;
  color: #55657d;
}

.header-actions {
  display: flex;
  gap: 12px;
  flex-shrink: 0;
}

.sheet-dialog-mask {
  position: fixed;
  inset: 0;
  z-index: 70;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24px;
  background: rgba(15, 27, 43, 0.54);
  backdrop-filter: blur(10px);
}

.blank-dialog {
  width: min(520px, 100%);
  padding: 24px;
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  background: #ffffff;
  box-shadow: var(--nb-shadow-card);
}

.blank-size-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px;
  margin-top: 20px;
}

.size-preset {
  height: 42px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: var(--tone-paper-soft);
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: var(--nb-shadow-soft);
}

.size-preset.active {
  background: linear-gradient(135deg, #2f6dff, #46b9ff);
  border-color: transparent;
  color: var(--nb-ink);
}

.blank-fields {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-top: 16px;
}

.blank-field {
  display: grid;
  gap: 8px;
}

.blank-field span {
  color: #18304f;
  font-size: 13px;
  font-weight: 700;
}

.blank-field input {
  width: 100%;
  height: 42px;
  padding: 0 12px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: #fff;
  color: var(--nb-ink);
}

.primary-btn,
.secondary-btn,
.toolbar-btn,
.chip,
.view-tab,
.mode-btn {
  border: 0;
  background: none;
  cursor: pointer;
}

.primary-btn,
.secondary-btn {
  height: 44px;
  padding: 0 18px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  font-size: 14px;
  font-weight: 800;
  box-shadow: var(--nb-shadow-soft);
}

.primary-btn {
  color: var(--nb-ink);
  background: linear-gradient(135deg, #2f6dff, #46b9ff);
  box-shadow: 0 10px 24px rgba(70, 120, 255, 0.24);
}

.secondary-btn {
  color: var(--nb-ink);
  background: var(--tone-paper-soft);
}

.mode-strip {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
  margin-top: 22px;
}

.overview-strip {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
  margin-top: 16px;
}

.overview-card {
  padding: 18px 18px 16px;
  border-radius: 0;
  border: 3px solid var(--nb-ink);
  background: var(--tone-paper-soft);
  box-shadow: var(--nb-shadow-card);
}

.overview-card span {
  display: block;
  font-size: 12px;
  font-weight: 700;
  color: #6c7b90;
}

.overview-card strong {
  display: block;
  margin-top: 8px;
  font-size: 28px;
  color: #122033;
  line-height: 1.1;
}

.overview-card p {
  margin-top: 10px;
  color: #60708a;
  font-size: 13px;
  line-height: 1.6;
}

.mode-btn {
  display: flex;
  flex-direction: column;
  gap: 6px;
  align-items: flex-start;
  padding: 18px 18px 16px;
  border-radius: 0;
  border: 3px solid var(--nb-ink);
  background: #fff;
  text-align: left;
  transition: 0.2s ease;
  box-shadow: var(--nb-shadow-soft);
}

.mode-btn.active {
  border-color: var(--nb-ink);
  background: var(--tone-blue-soft);
  box-shadow: var(--nb-shadow-card);
}

.mode-name {
  font-size: 16px;
  font-weight: 700;
  color: #18304f;
}

.mode-note {
  font-size: 13px;
  color: #617089;
}

.workbench-layout {
  display: grid;
  grid-template-columns: 296px minmax(0, 1fr) 316px;
  gap: 18px;
  margin-top: 20px;
}

.panel {
  min-height: 720px;
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  background: #ffffff;
  box-shadow: var(--nb-shadow-card);
}

.left-panel,
.right-panel {
  padding: 20px;
}

.right-panel {
  padding: 16px;
}

.right-panel-shell {
  display: flex;
  flex-direction: column;
  gap: 14px;
  position: sticky;
  top: 20px;
}

.right-tabs {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 8px;
  padding: 8px;
  border-radius: 0;
  background: var(--tone-paper-soft);
  border: 2px solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.right-tab {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 4px;
  min-height: 58px;
  padding: 10px 12px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: var(--tone-paper-soft);
  color: #62728a;
  cursor: pointer;
  text-align: left;
  box-shadow: var(--nb-shadow-soft);
}

.right-tab span {
  font-size: 12px;
  font-weight: 700;
}

.right-tab strong {
  font-size: 14px;
  color: #18304f;
}

.right-tab.active {
  background: linear-gradient(135deg, #2f6dff, #58c0ff);
  color: var(--nb-ink);
  box-shadow: 0 10px 22px rgba(49, 109, 255, 0.22);
}

.right-tab.active strong {
  color: var(--nb-ink);
}

.panel-stack {
  display: grid;
  gap: 14px;
}

.center-panel {
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.panel-block + .panel-block {
  margin-top: 16px;
}

.panel-stack .panel-block + .panel-block {
  margin-top: 0;
}

.panel-head {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  gap: 12px;
  margin-bottom: 14px;
}

.panel-head h2 {
  font-size: 18px;
  color: #18304f;
}

.panel-head span {
  font-size: 12px;
  color: #7b889f;
}

.source-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.chip {
  padding: 9px 12px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: var(--tone-paper-soft);
  color: var(--nb-ink);
  font-size: 13px;
  box-shadow: var(--nb-shadow-soft);
}

.chip.action {
  border: 2px solid var(--nb-ink);
}

.chip.action.danger {
  color: #b64545;
  background: #fff3f3;
}

.chip:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.source-summary {
  margin-top: 14px;
  padding: 14px;
  border-radius: 0;
  background: var(--tone-paper-soft);
  border: 2px solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.summary-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  font-size: 13px;
  color: #617089;
}

.summary-row + .summary-row {
  margin-top: 8px;
}

.summary-row strong {
  color: #18304f;
  text-align: right;
}

.todo-list {
  list-style: none;
  display: grid;
  gap: 10px;
  color: #375374;
  font-size: 14px;
}

.todo-list li {
  position: relative;
  padding-left: 16px;
  line-height: 1.6;
}

.todo-list li::before {
  content: '';
  position: absolute;
  left: 0;
  top: 9px;
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: #57b3ff;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.stat-card {
  padding: 12px;
  border-radius: 0;
  background: var(--tone-paper-soft);
  border: 2px solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.stat-card span {
  display: block;
  font-size: 12px;
  color: #6d7c92;
}

.stat-card strong {
  display: block;
  margin-top: 8px;
  font-size: 24px;
  color: #18304f;
}

.color-stats {
  margin-top: 14px;
  display: grid;
  gap: 8px;
}

.color-row {
  display: grid;
  grid-template-columns: 14px minmax(0, 1fr) auto;
  gap: 10px;
  align-items: center;
  font-size: 13px;
  color: #375374;
}

.color-row strong {
  color: #18304f;
}

.color-dot {
  width: 14px;
  height: 14px;
  border-radius: 0;
  border: 2px solid var(--nb-ink);
}

.canvas-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  align-items: center;
}

.view-tabs,
.canvas-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.view-tab,
.toolbar-btn {
  height: 38px;
  padding: 0 14px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: var(--tone-paper-soft);
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 700;
  box-shadow: var(--nb-shadow-soft);
}

.view-tab.active {
  background: #1d68ff;
  color: var(--nb-ink);
}

.canvas-stage {
  flex: 1;
  display: grid;
  grid-template-columns: minmax(0, 1.3fr) minmax(280px, 0.7fr);
  gap: 16px;
  align-items: start;
}

.stage-sidecars {
  display: grid;
  gap: 16px;
}

.stage-card {
  min-height: 280px;
  padding: 16px;
  border-radius: 0;
  background: #0e1624;
  color: #fff;
  border: 3px solid var(--nb-ink);
  box-shadow: var(--nb-shadow-card);
}

.stage-card.ghost {
  background: linear-gradient(180deg, #132033, #101a2a);
}

.stage-label {
  display: inline-flex;
  margin-bottom: 14px;
  padding: 6px 10px;
  border: 2px solid currentColor;
  border-radius: 0;
  background: rgba(120, 181, 255, 0.16);
  color: #d9ebff;
  font-size: 12px;
}

.stage-grid {
  display: grid;
  gap: 4px;
  width: max-content;
  max-width: 100%;
}

.stage-grid.real-grid {
  gap: 2px;
  overflow: auto;
  padding-bottom: 4px;
}

.stage-grid.side-grid {
  gap: 1px;
}

.stage-pixel {
  aspect-ratio: 1;
  border-radius: 0;
}

.stage-pixel.real.empty {
  background: #152131;
}

.stage-pixel.real.filled {
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.04);
}

.stage-pixel.selected {
  box-shadow:
    inset 0 0 0 2px rgba(255, 255, 255, 0.82),
    0 0 0 2px rgba(82, 180, 255, 0.85);
}

.stage-pixel.region-selected {
  box-shadow:
    inset 0 0 0 2px rgba(255, 215, 90, 0.88),
    0 0 0 1px rgba(255, 193, 30, 0.45);
}

.stage-pixel.draw-preview {
  box-shadow:
    inset 0 0 0 2px rgba(120, 255, 200, 0.9),
    0 0 0 1px rgba(76, 205, 149, 0.45);
}

.magnifier-grid {
  gap: 4px;
  margin-top: 14px;
}

.magnifier-pixel {
  width: 20px;
  height: 20px;
  border-radius: 0;
}

.stage-focus {
  margin-top: 14px;
  padding: 12px 14px;
  border-radius: 0;
  background: rgba(255, 255, 255, 0.05);
  border: 2px solid rgba(132, 180, 255, 0.5);
}

.focus-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  font-size: 13px;
  color: #caddf8;
}

.focus-row + .focus-row {
  margin-top: 8px;
}

.focus-row strong {
  color: #ffffff;
}

.magnifier-meta {
  margin-top: 6px;
}

.placeholder-text {
  color: #a7bad4;
  line-height: 1.7;
  font-size: 14px;
}

.diff-list {
  margin-top: 14px;
  list-style: none;
  display: grid;
  gap: 8px;
  color: #d9ebff;
  font-size: 13px;
}

.version-list {
  display: grid;
  gap: 8px;
}

.compare-summary {
  margin-bottom: 12px;
  padding: 12px 14px;
  border-radius: 0;
  background: var(--tone-paper-soft);
  border: 2px solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.compare-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  font-size: 13px;
  color: #617089;
}

.compare-row + .compare-row {
  margin-top: 8px;
}

.compare-row strong {
  color: #18304f;
}

.empty-side-note {
  padding: 14px;
  border-radius: 0;
  background: #f8fafc;
  border: 2px dashed var(--nb-ink);
  color: #6d7c92;
  font-size: 13px;
  line-height: 1.7;
}

.version-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
  padding: 10px 12px;
  border-radius: 0;
  background: var(--tone-paper-soft);
  border: 2px solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.version-copy {
  min-width: 0;
}

.version-actions {
  display: flex;
  gap: 8px;
}

.version-row strong {
  color: #18304f;
  font-size: 13px;
}

.version-row span {
  color: #6d7c92;
  font-size: 12px;
  text-transform: uppercase;
}

.version-btn {
  height: 32px;
  padding: 0 12px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: #fff;
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: var(--nb-shadow-soft);
}

.hidden-input {
  display: none;
}

.floating-status {
  position: fixed;
  right: 24px;
  bottom: 24px;
  padding: 12px 16px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: #18304f;
  color: #fff;
  box-shadow: var(--nb-shadow-card);
  z-index: 30;
}

.floating-status.error {
  background: #b64545;
}

@media (max-width: 1200px) {
  .starter-hub {
    grid-template-columns: 1fr;
  }

  .overview-strip {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .workbench-layout {
    grid-template-columns: 1fr;
  }

  .panel {
    min-height: auto;
  }

  .right-panel-shell {
    position: static;
  }

  .canvas-stage {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 768px) {
  .pattern-workbench {
    padding: 14px 12px 32px;
  }

  .workbench-header {
    flex-direction: column;
    gap: 16px;
    padding: 18px 16px;
  }

  .blank-size-grid,
  .blank-fields {
    grid-template-columns: 1fr;
  }

  .eyebrow {
    font-size: 11px;
    letter-spacing: 0.08em;
  }

  .title {
    margin-top: 8px;
    font-size: 24px;
  }

  .desc {
    margin-top: 10px;
    font-size: 14px;
    line-height: 1.65;
  }

  .starter-hub {
    gap: 12px;
    margin-top: 14px;
  }

  .starter-card {
    padding: 16px;
  }

  .starter-card h2 {
    margin-top: 12px;
    font-size: 20px;
  }

  .starter-card p {
    min-height: auto;
    margin-top: 8px;
    font-size: 13px;
  }

  .mode-strip {
    grid-template-columns: 1fr;
  }

  .overview-strip {
    grid-template-columns: 1fr;
  }

  .header-actions {
    width: 100%;
    flex-wrap: wrap;
    gap: 10px;
  }

  .header-actions > button {
    flex: 1 1 calc(50% - 5px);
  }

  .starter-card .primary-btn {
    width: 100%;
  }

  .right-tabs {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
</style>
