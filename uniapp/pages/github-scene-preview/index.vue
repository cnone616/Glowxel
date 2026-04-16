<template>
  <view class="github-scene-preview-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">GitHub 候选预览</text>
      <view class="nav-right"></view>
    </view>

    <view v-if="currentScene" class="canvas-section">
      <view class="preview-canvas-container" :style="previewBoxStyle" @click="handlePreviewCurrentScene">
        <image
          class="preview-image"
          :src="toAssetPath(currentScene.boardFile)"
          mode="aspectFit"
        />
      </view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-title">{{ currentScene.title }}</text>
          <text class="preview-subtitle">
            {{ currentSceneSectionLabel }} · {{ getThemeLabel(currentScene.theme) }}
          </text>
          <text class="preview-source">
            {{ currentScene.sourceRepo }}
          </text>
        </view>
        <view class="preview-actions">
          <view class="action-btn-sm primary glx-primary-action" @click.stop="handlePreviewSheet">
            <Icon name="picture" :size="32" color="var(--nb-ink)" />
            <text>总览图</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view
      scroll-y
      class="content glx-scroll-region glx-page-shell__content"
      :style="{ height: contentHeight }"
    >
      <view class="content-wrapper glx-scroll-stack">
        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">整理说明</text>
          </view>
          <text class="summary-text">
            这里收口的是已经缩到 64×64 的 GitHub 场景候选，先看主推，再看备选；点按顶部预览可以放大看原图。
          </text>
          <view class="summary-chip-row">
            <view class="summary-chip glx-choice-chip--compact">
              <text>64×64 已缩板</text>
            </view>
            <view class="summary-chip glx-choice-chip--compact">
              <text>主推 {{ priorityScenes.length }}</text>
            </view>
            <view class="summary-chip glx-choice-chip--compact">
              <text>备选 {{ backupScenes.length }}</text>
            </view>
            <view class="summary-chip glx-choice-chip--compact">
              <text>合计 {{ totalSceneCount }}</text>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">主推候选</text>
            <text class="section-count">{{ priorityScenes.length }} 张</text>
          </view>
          <view class="scene-list">
            <view
              v-for="item in priorityScenes"
              :key="item.id"
              class="scene-entry glx-list-card"
              :class="{ active: selectedSceneId === item.id }"
              @click="handleSceneSelect(item.id)"
            >
              <view class="scene-entry__thumb">
                <image
                  class="scene-entry__image"
                  :src="toAssetPath(item.boardFile)"
                  mode="aspectFit"
                />
              </view>
              <view class="scene-entry__body">
                <view class="scene-entry__header">
                  <text class="scene-entry__title">{{ item.title }}</text>
                  <text v-if="selectedSceneId === item.id" class="scene-entry__badge">当前预览</text>
                </view>
                <text class="scene-entry__note">{{ item.note }}</text>
                <text class="scene-entry__source">
                  {{ getThemeLabel(item.theme) }} · {{ item.sourceRepo }}
                </text>
                <text class="scene-entry__asset">{{ item.sourceAsset }}</text>
              </view>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">备选候选</text>
            <text class="section-count">{{ backupScenes.length }} 张</text>
          </view>
          <view class="scene-list">
            <view
              v-for="item in backupScenes"
              :key="item.id"
              class="scene-entry glx-list-card"
              :class="{ active: selectedSceneId === item.id }"
              @click="handleSceneSelect(item.id)"
            >
              <view class="scene-entry__thumb">
                <image
                  class="scene-entry__image"
                  :src="toAssetPath(item.boardFile)"
                  mode="aspectFit"
                />
              </view>
              <view class="scene-entry__body">
                <view class="scene-entry__header">
                  <text class="scene-entry__title">{{ item.title }}</text>
                  <text v-if="selectedSceneId === item.id" class="scene-entry__badge">当前预览</text>
                </view>
                <text class="scene-entry__note">{{ item.note }}</text>
                <text class="scene-entry__source">
                  {{ getThemeLabel(item.theme) }} · {{ item.sourceRepo }}
                </text>
                <text class="scene-entry__asset">{{ item.sourceAsset }}</text>
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import githubSceneManifest from "../../utils/githubSceneFinalistsManifest.js";

const SCENE_THEME_LABELS = {
  sunset_landscape: "晚霞山脊",
  scifi_lake: "科幻湖景",
  city_evening: "霓虹傍晚",
  city_night: "霓虹夜景",
};

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
  },
  data() {
    return {
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      previewContainerSize: { width: 320, height: 320 },
      selectedSceneId: githubSceneManifest.priority[0].id,
    };
  },
  computed: {
    priorityScenes() {
      return githubSceneManifest.priority;
    },
    backupScenes() {
      return githubSceneManifest.backup;
    },
    totalSceneCount() {
      return this.priorityScenes.length + this.backupScenes.length;
    },
    sheetInfo() {
      return githubSceneManifest.sheet;
    },
    currentScene() {
      const allScenes = this.priorityScenes.concat(this.backupScenes);
      return allScenes.find((item) => item.id === this.selectedSceneId);
    },
    currentSceneSectionLabel() {
      const isPriorityScene = this.priorityScenes.some((item) => item.id === this.selectedSceneId);
      if (isPriorityScene) {
        return "主推候选";
      }
      return "备选候选";
    },
    previewBoxStyle() {
      const size = this.previewContainerSize.height
        ? this.previewContainerSize.height
        : 320;
      return {
        height: `${size}px`,
      };
    },
  },
  onReady() {
    this.initPreviewLayout();
  },
  methods: {
    handleBack() {
      uni.navigateBack();
    },
    initPreviewLayout() {
      const systemInfo = uni.getSystemInfoSync();
      const statusBarHeight = systemInfo.statusBarHeight
        ? systemInfo.statusBarHeight
        : 0;

      this.$nextTick(() => {
        setTimeout(() => {
          const query = uni.createSelectorQuery().in(this);
          query.select(".canvas-section").boundingClientRect((sectionRect) => {
            if (!sectionRect || !sectionRect.height) {
              return;
            }
            const nextHeight =
              systemInfo.windowHeight - statusBarHeight - 88 - sectionRect.height;
            this.contentHeight = `${Math.max(120, nextHeight)}px`;
          });
          query
            .select(".preview-canvas-container")
            .boundingClientRect((data) => {
              if (!data || !data.width) {
                return;
              }
              this.previewContainerSize = {
                width: data.width,
                height: data.width,
              };
            })
            .exec();
        }, 80);
      });
    },
    handleSceneSelect(sceneId) {
      this.selectedSceneId = sceneId;
    },
    handlePreviewCurrentScene() {
      if (!this.currentScene) {
        return;
      }
      this.openImagePreview(this.currentScene.boardFile);
    },
    handlePreviewSheet() {
      this.openImagePreview(this.sheetInfo.file);
    },
    openImagePreview(filePath) {
      const assetPath = this.toAssetPath(filePath);
      uni.previewImage({
        current: assetPath,
        urls: [assetPath],
      });
    },
    toAssetPath(filePath) {
      return `/${filePath}`;
    },
    getThemeLabel(theme) {
      return SCENE_THEME_LABELS[theme];
    },
  },
};
</script>

<style scoped>
.github-scene-preview-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-primary);
  overflow: hidden;
}

.status-bar {
  background-color: #1a1a1a;
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

.preview-image {
  width: 100%;
  height: 100%;
  display: block;
  image-rendering: pixelated;
}

.preview-subtitle {
  font-size: 22rpx;
  line-height: 1.35;
  color: var(--text-secondary);
  font-weight: 700;
}

.preview-source {
  font-size: 20rpx;
  line-height: 1.35;
  color: var(--text-secondary);
}

.summary-text {
  margin-top: 12rpx;
  font-size: 22rpx;
  line-height: 1.6;
  color: var(--text-secondary);
}

.summary-chip-row {
  margin-top: 16rpx;
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.summary-chip {
  background: var(--nb-surface);
}

.summary-chip text {
  color: var(--nb-ink);
  font-size: 20rpx;
  line-height: 1;
  font-weight: 900;
}

.section-count {
  font-size: 20rpx;
  line-height: 1.2;
  color: var(--text-secondary);
  font-weight: 800;
}

.scene-list {
  margin-top: 16rpx;
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.scene-entry {
  padding: 16rpx;
  display: flex;
  gap: 16rpx;
  box-sizing: border-box;
}

.scene-entry.active {
  background: var(--nb-yellow);
}

.scene-entry__thumb {
  width: 132rpx;
  height: 132rpx;
  flex-shrink: 0;
  border: 2rpx solid var(--nb-ink);
  background: #000000;
  box-sizing: border-box;
}

.scene-entry__image {
  width: 100%;
  height: 100%;
  display: block;
  image-rendering: pixelated;
}

.scene-entry__body {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 8rpx;
}

.scene-entry__header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12rpx;
}

.scene-entry__title {
  flex: 1;
  min-width: 0;
  font-size: 24rpx;
  line-height: 1.3;
  color: var(--nb-ink);
  font-weight: 900;
}

.scene-entry__badge {
  flex-shrink: 0;
  min-height: 40rpx;
  padding: 0 12rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
  color: var(--nb-ink);
  font-size: 18rpx;
  line-height: 1;
  font-weight: 900;
}

.scene-entry__note,
.scene-entry__source,
.scene-entry__asset {
  font-size: 20rpx;
  line-height: 1.5;
  color: var(--text-secondary);
}

.scene-entry__note {
  font-size: 22rpx;
}

.scene-entry.active .scene-entry__note,
.scene-entry.active .scene-entry__source,
.scene-entry.active .scene-entry__asset {
  color: var(--nb-ink);
}

</style>
