<template>
  <view class="workspace-page glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 顶部栏 -->
    <view class="header glx-topbar glx-page-shell__fixed">
      <view class="header-content">
        <view class="header-placeholder"></view>
        <text class="header-title glx-topbar__title">创作中心</text>
        <view class="header-placeholder"></view>
      </view>
    </view>

    <!-- 主要内容 -->
    <scroll-view scroll-y class="main-content glx-scroll-region glx-page-shell__content">
      <!-- 快速开始 -->
      <view class="quick-actions">
        <view class="action-grid">
          <view class="action-card glx-panel-card" @click="startBlankCanvas">
            <view class="action-icon">
              <Icon name="add" :size="48" color="var(--nb-ink)" />
            </view>
            <text class="action-title">新建画布</text>
            <text class="action-subtitle">从空白开始创作</text>
          </view>

          <view class="action-card glx-panel-card" @click="startImageImport">
            <view class="action-icon">
              <Icon name="upload" :size="48" color="var(--nb-ink)" />
            </view>
            <text class="action-title">导入图片</text>
            <text class="action-subtitle">转换为像素画</text>
          </view>
        </view>
      </view>

      <!-- 我的画布 -->
      <view class="my-projects">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">我的画布</text>
          <text class="project-count">{{ filteredProjects.length }} 个</text>
        </view>

        <view v-if="filteredProjects.length === 0" class="empty-state glx-panel-card">
          <view class="empty-icon">
            <Icon name="picture" :size="80" color="var(--text-tertiary)" />
          </view>
          <text class="empty-title">还没有画布</text>
          <text class="empty-subtitle">开始您的第一个创作吧！</text>
          <view class="empty-btn glx-cta-button" @click="startBlankCanvas">
            <text class="empty-btn-text">立即创建</text>
          </view>
        </view>

        <view v-else class="projects-grid">
          <ProjectCard
            v-for="project in filteredProjects"
            :key="project.id"
            :project="project"
          />
        </view>
      </view>
    </scroll-view>

    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useProjectStore } from "../../store/project.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import ProjectCard from "../../components/ProjectCard.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    ProjectCard,
  },

  data() {
    return {
      projectStore: null,
      toast: null,
    };
  },

  computed: {
    filteredProjects() {
      if (!this.projectStore) return [];
      return this.projectStore.projects || [];
    },
  },

  onLoad() {
    this.projectStore = useProjectStore();
    this.toast = useToast();

    // 注册自定义 Toast 实例
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
    });
  },

  onShow() {
    // 页面显示时刷新项目列表
    if (this.projectStore) {
      this.projectStore.loadProjects();
    }
  },

  methods: {
    startBlankCanvas() {
      uni.navigateTo({
        url: "/pages/create/create?mode=blank",
      });
    },

    startImageImport() {
      uni.navigateTo({
        url: "/pages/create/create?mode=image",
      });
    },
  },
};
</script>

<style scoped>
.workspace-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--nb-paper);
  overflow: hidden;
}

.header {
  background-color: var(--nb-surface);
  border-bottom: 2rpx solid var(--nb-ink);
  padding: 0 32rpx;
}

.header-content {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.header-actions {
  display: flex;
  gap: 16rpx;
}

.main-content {
  flex: 1;
  overflow-y: auto;
}

.section-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
  margin-bottom: 24rpx;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24rpx;
}

.project-count {
  font-size: 24rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

/* 快速操作 */
.quick-actions {
  margin: 32rpx;
  margin-bottom: 48rpx;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.action-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 32rpx 16rpx;
  background-color: var(--nb-surface);
  border-radius: 0;
  box-shadow: var(--nb-shadow-strong);
  transition: all 0.2s ease;
}

.action-card:active {
  box-shadow: var(--nb-shadow-strong);
}

.action-icon {
  width: 96rpx;
  height: 96rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-blue);
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  border-radius: 0;
  margin-bottom: 16rpx;
}

.action-card:nth-child(2) .action-icon {
  background-color: var(--nb-green);
}

.action-title {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--nb-ink);
  margin-bottom: 8rpx;
  text-align: center;
}

.action-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
  text-align: center;
  line-height: 1.4;
}

/* 我的画布 */
.my-projects {
  margin: 0 32rpx 48rpx;
}

.projects-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

/* 空状态 */

.empty-icon {
  width: 152rpx;
  height: 152rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-surface);
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  border-radius: 0;
  margin-bottom: 32rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
  margin-bottom: 12rpx;
}

.empty-subtitle {
  font-size: 26rpx;
  color: var(--text-secondary);
  line-height: 1.5;
  margin-bottom: 32rpx;
}

.empty-btn {
  padding: 20rpx 40rpx;
  background-color: var(--nb-yellow);
  border-radius: 0;
  border: 3rpx solid #000000;
  transition: all 0.2s ease;
}

.empty-btn-text {
  font-size: 28rpx;
  font-weight: 600;
  color: #000000;
}
</style>
