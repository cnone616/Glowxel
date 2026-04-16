<template>
  <div class="create-page">
    <div class="create-shell">
      <header class="topbar">
        <button class="back-btn" type="button" @click="handleBack" aria-label="返回">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.1">
            <path d="M15 18l-6-6 6-6"/>
          </svg>
        </button>
        <div class="topbar-copy">
          <h1 class="page-title">新建画布</h1>
          <p class="page-desc">先选一种开始方式，再进入对应流程</p>
        </div>
      </header>

      <section class="mode-panel">
        <button
          v-for="item in createModes"
          :key="item.value"
          class="mode-card"
          :class="{ active: selectedMode === item.value }"
          type="button"
          @click="selectedMode = item.value"
        >
          <div class="mode-icon" :class="item.colorClass" v-html="item.icon"></div>
          <div class="mode-copy">
            <strong class="mode-title">{{ item.title }}</strong>
            <span class="mode-subtitle">{{ item.subtitle }}</span>
          </div>
          <span class="mode-check">
            <svg v-if="selectedMode === item.value" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2">
              <path d="M20 6L9 17l-5-5"/>
            </svg>
          </span>
        </button>
      </section>

      <section class="summary-card">
        <div class="summary-head">
          <span class="summary-tag">{{ currentMode.summaryTag }}</span>
          <strong class="summary-title">{{ currentMode.summaryTitle }}</strong>
        </div>
        <p class="summary-text">{{ currentMode.summaryText }}</p>
        <div class="summary-points">
          <span v-for="point in currentMode.points" :key="point" class="summary-point">{{ point }}</span>
        </div>
      </section>

      <section class="advanced-card">
        <div class="advanced-copy">
          <strong>拼豆图纸生成</strong>
          <span>如果你上传的是带编号的拼豆图纸，走这个入口会更合适。</span>
        </div>
        <button class="advanced-btn" type="button" @click="openPatternWorkbench">
          进入图纸流程
        </button>
      </section>

      <footer class="action-bar">
        <button class="secondary-btn" type="button" @click="handleBack">上一步</button>
        <button class="primary-btn" type="button" @click="handleContinue">
          {{ currentMode.actionText }}
        </button>
      </footer>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

const router = useRouter()
const route = useRoute()

const createModes = [
  {
    value: 'blank',
    title: '新建画布',
    subtitle: '从空白开始创作',
    summaryTag: '空白开始',
    summaryTitle: '直接进入画布开始绘制',
    summaryText: '适合从零开始画图，先进入编辑器，再按你的想法自由创作。',
    points: ['直接进入画布', '适合手动绘制', '流程更轻'],
    actionText: '开始新建',
    target: '/editor',
    colorClass: 'blue',
    icon: `<svg width="26" height="26" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.9"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>`,
  },
  {
    value: 'image',
    title: '导入图片',
    subtitle: '转换为像素画',
    summaryTag: '图片生成',
    summaryTitle: '上传图片后按步骤生成',
    summaryText: '适合把照片、插画或现成图片转换成像素图，再进入后续编辑。',
    points: ['支持上传图片', '调整尺寸与预览', '完成后继续编辑'],
    actionText: '选择图片',
    target: '/pattern-workbench',
    colorClass: 'green',
    icon: `<svg width="26" height="26" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.9"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><path d="M21 15l-5-5L5 21"/></svg>`,
  },
]

const selectedMode = ref('blank')

const currentMode = computed(() => {
  const activeMode = createModes.find((item) => item.value === selectedMode.value)
  return activeMode || createModes[0]
})

function syncModeFromQuery(modeValue) {
  if (modeValue === 'blank' || modeValue === 'image') {
    selectedMode.value = modeValue
    return
  }

  selectedMode.value = 'blank'
}

function handleBack() {
  router.push('/workspace')
}

function handleContinue() {
  router.push(currentMode.value.target)
}

function openPatternWorkbench() {
  router.push('/pattern-workbench')
}

watch(
  () => route.query.mode,
  (modeValue) => {
    syncModeFromQuery(modeValue)
  },
  { immediate: true },
)
</script>

<style scoped>
.create-page {
  min-height: calc(100vh - 60px);
  padding: 20px 0 36px;
  background:
    radial-gradient(circle at top, rgba(136, 161, 186, 0.12), transparent 34%),
    transparent;
}

.create-shell {
  max-width: 680px;
  margin: 0 auto;
  padding: 0 20px;
}

.topbar {
  display: flex;
  align-items: center;
  gap: 14px;
  margin-bottom: 20px;
  padding: 18px 20px;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 28px;
  background: rgba(255, 255, 255, 0.84);
  box-shadow: var(--nb-shadow-card);
}

.back-btn {
  width: 44px;
  height: 44px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.72);
  color: var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  cursor: pointer;
  flex-shrink: 0;
}

.topbar-copy {
  display: grid;
  gap: 6px;
}

.page-title {
  margin: 0;
  font-size: 30px;
  font-weight: 700;
  color: var(--nb-ink);
}

.page-desc {
  margin: 0;
  font-size: 14px;
  color: var(--text-secondary);
}

.mode-panel {
  display: grid;
  gap: 14px;
}

.mode-card {
  width: 100%;
  display: grid;
  grid-template-columns: auto 1fr auto;
  align-items: center;
  gap: 14px;
  padding: 18px;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.78);
  box-shadow: var(--nb-shadow-soft);
  text-align: left;
  cursor: pointer;
  transition: transform 160ms ease, border-color 160ms ease, box-shadow 160ms ease;
}

.mode-card:hover {
  transform: translateY(-1px);
  border-color: rgba(136, 161, 186, 0.18);
}

.mode-card.active {
  border-color: rgba(136, 161, 186, 0.26);
  box-shadow: var(--nb-shadow-card);
}

.mode-icon {
  width: 56px;
  height: 56px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 18px;
}

.mode-icon.blue {
  color: var(--nb-ink);
  background: rgba(136, 161, 186, 0.18);
}

.mode-icon.green {
  color: var(--nb-ink);
  background: rgba(155, 179, 158, 0.2);
}

.mode-copy {
  display: grid;
  gap: 6px;
}

.mode-title {
  font-size: 18px;
  color: var(--nb-ink);
}

.mode-subtitle {
  font-size: 14px;
  color: var(--text-secondary);
}

.mode-check {
  width: 28px;
  height: 28px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 999px;
  color: var(--nb-ink);
  background: rgba(136, 161, 186, 0.14);
}

.summary-card,
.advanced-card {
  margin-top: 16px;
  padding: 20px;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.8);
  box-shadow: var(--nb-shadow-soft);
}

.summary-head {
  display: grid;
  gap: 8px;
  margin-bottom: 10px;
}

.summary-tag {
  display: inline-flex;
  align-items: center;
  width: fit-content;
  padding: 6px 10px;
  border-radius: 999px;
  background: rgba(136, 161, 186, 0.14);
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: 700;
}

.summary-title {
  font-size: 18px;
  color: var(--nb-ink);
}

.summary-text {
  margin: 0;
  font-size: 14px;
  line-height: 1.7;
  color: var(--text-secondary);
}

.summary-points {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 14px;
}

.summary-point {
  padding: 8px 12px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.84);
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 600;
  border: 1px solid rgba(36, 49, 66, 0.06);
}

.advanced-card {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.advanced-copy {
  display: grid;
  gap: 6px;
}

.advanced-copy strong {
  font-size: 16px;
  color: var(--nb-ink);
}

.advanced-copy span {
  font-size: 13px;
  line-height: 1.6;
  color: var(--text-secondary);
}

.advanced-btn,
.secondary-btn,
.primary-btn {
  height: 46px;
  padding: 0 18px;
  border: 1px solid transparent;
  border-radius: 16px;
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
}

.advanced-btn,
.secondary-btn {
  background: rgba(255, 255, 255, 0.78);
  border-color: rgba(36, 49, 66, 0.08);
  color: var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.primary-btn {
  min-width: 132px;
  background: var(--nb-yellow);
  border-color: rgba(215, 178, 109, 0.36);
  color: var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.action-bar {
  position: sticky;
  bottom: 16px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-top: 18px;
  padding: 14px;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.88);
  box-shadow: var(--nb-shadow-card);
  backdrop-filter: blur(14px);
}

@media (max-width: 768px) {
  .create-page {
    min-height: calc(100vh - 56px);
    padding: 10px 0 24px;
  }

  .create-shell {
    padding: 0 12px;
  }

  .topbar {
    gap: 12px;
    margin-bottom: 14px;
    padding: 14px;
    border-radius: 18px;
  }

  .back-btn {
    width: 40px;
    height: 40px;
    border-radius: 14px;
  }

  .page-title {
    font-size: 24px;
  }

  .page-desc {
    font-size: 13px;
  }

  .mode-card {
    gap: 12px;
    padding: 14px;
    border-radius: 18px;
  }

  .mode-icon {
    width: 48px;
    height: 48px;
    border-radius: 16px;
  }

  .mode-title {
    font-size: 16px;
  }

  .mode-subtitle {
    font-size: 13px;
  }

  .summary-card,
  .advanced-card {
    margin-top: 12px;
    padding: 16px;
    border-radius: 18px;
  }

  .advanced-card {
    align-items: stretch;
    flex-direction: column;
  }

  .advanced-btn,
  .secondary-btn,
  .primary-btn {
    width: 100%;
  }

  .action-bar {
    bottom: 12px;
    flex-direction: column;
    align-items: stretch;
    padding: 12px;
    border-radius: 18px;
  }
}
</style>
