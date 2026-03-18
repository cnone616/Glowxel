<template>
  <view class="workspace-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 顶部栏 -->
    <view class="header">
      <view class="header-content">
        <text class="header-title">创作中心</text>
      </view>
    </view>

    <!-- 主要内容 -->
    <scroll-view scroll-y class="main-content">
      <!-- 快速开始 -->
      <view class="quick-actions">
        <view class="action-grid">
          <view class="action-card" @click="startBlankCanvas">
            <view class="action-icon">
              <Icon name="add" :size="48" color="#4F7FFF" />
            </view>
            <text class="action-title">新建画布</text>
            <text class="action-subtitle">从空白开始创作</text>
          </view>

          <view class="action-card" @click="startImageImport">
            <view class="action-icon">
              <Icon name="upload" :size="48" color="#2ECC71" />
            </view>
            <text class="action-title">导入图片</text>
            <text class="action-subtitle">转换为像素画</text>
          </view>
        </view>
      </view>

      <!-- 我的画布 -->
      <view class="my-projects">
        <view class="section-header">
          <text class="section-title">我的画布</text>
          <text class="project-count">{{ filteredProjects.length }} 个</text>
        </view>

        <view v-if="filteredProjects.length === 0" class="empty-state">
          <view class="empty-icon">
            <Icon name="picture" :size="80" color="#AAAAAA" />
          </view>
          <text class="empty-title">还没有画布</text>
          <text class="empty-subtitle">开始您的第一个创作吧！</text>
          <view class="empty-btn" @click="startBlankCanvas">
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
  background-color: var(--color-app-background);
  overflow: hidden;
}

.header {
  background-color: var(--color-card-background);
  border-bottom: 2rpx solid var(--border-primary);
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
  color: var(--color-text-primary);
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
  color: var(--color-text-primary);
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
  color: var(--color-text-secondary);
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
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.action-card:active {
  transform: scale(0.95);
  box-shadow: var(--shadow-floating);
}

.action-icon {
  width: 96rpx;
  height: 96rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: var(--radius-medium);
  margin-bottom: 16rpx;
}

.action-title {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 8rpx;
  text-align: center;
}

.action-subtitle {
  font-size: 22rpx;
  color: var(--color-text-secondary);
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
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 64rpx 32rpx;
  text-align: center;
}

.empty-icon {
  width: 160rpx;
  height: 160rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: 50%;
  margin-bottom: 32rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 12rpx;
}

.empty-subtitle {
  font-size: 26rpx;
  color: var(--color-text-secondary);
  line-height: 1.5;
  margin-bottom: 32rpx;
}

.empty-btn {
  padding: 20rpx 40rpx;
  background-color: var(--color-brand-primary);
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
}

.empty-btn:active {
  transform: scale(0.95);
}

.empty-btn-text {
  font-size: 28rpx;
  font-weight: 600;
  color: #ffffff;
}
</style>
