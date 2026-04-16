<template>
  <view class="my-works-page glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 导航栏 -->
    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--nb-ink)"
        />
      </view>
      <text class="nav-title glx-topbar__title">我的作品</text>
      <view class="nav-right" @click="showSortModal = true"> </view>
    </view>

    <!-- 搜索和筛选 -->
    <view class="search-section">
      <view class="search-bar glx-search-shell">
        <Icon name="search" :size="28" color="var(--text-tertiary)" />
        <input
          v-model="searchQuery"
          class="search-input"
          placeholder="搜索作品名称..."
        />
        <view v-if="searchQuery" class="clear-btn" @click="searchQuery = ''">
          <Icon name="close" :size="24" />
        </view>
      </view>

      <view class="filter-tabs">
        <view
          v-for="filter in filterOptions"
          :key="filter.value"
          class="filter-tab"
          :class="{ active: currentFilter === filter.value }"
          @click="currentFilter = filter.value"
        >
          <text class="filter-text">{{ filter.label }}</text>
          <text v-if="filter.count" class="filter-count">{{ filter.count }}</text>
        </view>
      </view>
    </view>

    <!-- 作品网格 -->
    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content" @scrolltolower="loadMore">
      <view v-if="filteredWorks.length === 0" class="empty-state glx-panel-card">
        <Icon name="picture" :size="120" color="var(--text-tertiary)" />
        <text class="empty-title">暂无作品</text>
        <text class="empty-desc">快去创建你的第一个作品吧</text>
        <button class="create-btn glx-cta-button" @click="goToCreate">
          <Icon name="plus" :size="32" />
          <text>创建作品</text>
        </button>
      </view>

      <view v-else class="works-grid">
        <ProjectCard
          v-for="work in filteredWorks"
          :key="work.id"
          :project="work"
          :defer-click="true"
          @card-click="openWork"
          @card-longpress="showWorkMenu"
        />
      </view>

      <!-- 加载更多 -->
      <view v-if="hasMore" class="load-more">
        <text class="load-text">加载更多...</text>
      </view>
    </scroll-view>

    <!-- 排序弹窗 -->
    <view
      v-if="showSortModal"
      class="modal-overlay"
      @click="showSortModal = false"
    >
      <view class="sort-modal" @click.stop>
        <view class="modal-header">
          <text class="modal-title">排序方式</text>
          <view class="modal-close" @click="showSortModal = false">
            <Icon name="close" :size="32" />
          </view>
        </view>

        <view class="sort-options">
          <view
            v-for="option in sortOptions"
            :key="option.value"
            class="sort-option"
            :class="{ active: currentSort === option.value }"
            @click="selectSort(option.value)"
          >
            <text class="sort-text">{{ option.label }}</text>
            <Icon v-if="currentSort === option.value" name="check" :size="24" />
          </view>
        </view>
      </view>
    </view>

    <!-- 作品菜单 -->
    <view
      v-if="selectedWork"
      class="modal-overlay"
      @click="selectedWork = null"
    >
      <view class="work-menu" @click.stop>
        <view class="menu-header">
          <text class="menu-title">{{ selectedWork.name }}</text>
        </view>

        <view class="menu-actions">
          <view class="menu-action" @click="editWork">
            <Icon name="edit" :size="32" />
            <text class="action-text">编辑</text>
          </view>

          <view class="menu-action" @click="duplicateWork">
            <Icon name="copy" :size="32" />
            <text class="action-text">复制</text>
          </view>

          <view class="menu-action" @click="shareWork">
            <Icon name="share" :size="32" />
            <text class="action-text">分享</text>
          </view>

          <view class="menu-action danger" @click="deleteWork">
            <Icon name="delete" :size="32" />
            <text class="action-text">删除</text>
          </view>
        </view>
      </view>
    </view>

    <ConfirmDialogHost />
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useProjectStore } from "../../store/project.js";
import { useToast } from "../../composables/useToast.js";
import { useDialog } from "../../composables/useDialog.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import ConfirmDialogHost from "../../components/ConfirmDialogHost.vue";
import Toast from "../../components/Toast.vue";
import ProjectCard from "../../components/ProjectCard.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    ConfirmDialogHost,
    Toast,
    ProjectCard,
  },

  data() {
    return {
      projectStore: null,
      toast: null,
      dialog: null,
      searchQuery: "",
      currentFilter: "all",
      currentSort: "updated",
      showSortModal: false,
      selectedWork: null,
      hasMore: false,
      filterOptions: [
        { value: "all", label: "全部", count: 0 },
        { value: "published", label: "已发布", count: 0 },
        { value: "reviewing", label: "审核中", count: 0 },
        { value: "rejected", label: "已退回", count: 0 },
        { value: "drafts", label: "草稿", count: 0 },
      ],
      sortOptions: [
        { value: "updated", label: "最近更新" },
        { value: "created", label: "创建时间" },
        { value: "name", label: "名称" },
        { value: "size", label: "尺寸" },
      ],
    };
  },

  computed: {
    // 获取已发布的项目
    publishedWorks() {
      if (!this.projectStore) return [];
      return (this.projectStore.projects || []).filter(
        (p) => p.status === "published",
      );
    },

    // 获取未发布的草稿项目
    draftWorks() {
      if (!this.projectStore) return [];
      return (this.projectStore.projects || []).filter(
        (p) => p.status === "draft",
      );
    },

    // 获取审核中的项目
    reviewingWorks() {
      if (!this.projectStore) return [];
      return (this.projectStore.projects || []).filter(
        (p) => p.status === "reviewing",
      );
    },

    // 获取已退回的项目
    rejectedWorks() {
      if (!this.projectStore) return [];
      return (this.projectStore.projects || []).filter(
        (p) => p.status === "rejected",
      );
    },

    // 根据当前筛选获取作品列表
    allWorks() {
      const formatList = (list, type) =>
        list.map((work) => ({
          ...work,
          type: type,
          updateTime: work.updateTime,
          createTime: work.createTime,
          progress: work.progress,
        }));

      switch (this.currentFilter) {
        case "published":
          return formatList(this.publishedWorks, "published");
        case "drafts":
          return formatList(this.draftWorks, "draft");
        case "reviewing":
          return formatList(this.reviewingWorks, "reviewing");
        case "rejected":
          return formatList(this.rejectedWorks, "rejected");
        case "all":
          return [
            ...formatList(this.publishedWorks, "published"),
            ...formatList(this.reviewingWorks, "reviewing"),
            ...formatList(this.rejectedWorks, "rejected"),
            ...formatList(this.draftWorks, "draft"),
          ];
        default:
          return [];
      }
    },

    filteredWorks() {
      let works = [...this.allWorks];

      // 搜索过滤
      if (this.searchQuery.trim()) {
        const query = this.searchQuery.toLowerCase();
        works = works.filter(
          (work) =>
            work.name.toLowerCase().includes(query) ||
            (work.title && work.title.toLowerCase().includes(query)),
        );
      }

      // 排序
      works.sort((a, b) => {
        switch (this.currentSort) {
          case "updated":
            return new Date(b.updateTime) - new Date(a.updateTime);
          case "created":
            return new Date(b.createTime) - new Date(a.createTime);
          case "name":
            return (a.name || a.title || "").localeCompare(
              b.name || b.title || "",
            );
          case "size":
            return b.width * b.height - a.width * a.height;
          default:
            return 0;
        }
      });

      return works;
    },
  },

  onLoad() {
    this.projectStore = useProjectStore();
    this.toast = useToast();
    this.dialog = useDialog();

    // 加载项目数据
    this.projectStore.loadProjects();
    this.updateFilterCounts();

    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
    });
  },

  onShow() {
    // 页面显示时刷新数据
    if (this.projectStore) {
      this.projectStore.loadProjects();
      this.updateFilterCounts();
    }
  },

  methods: {
    handleBack() {
      uni.navigateBack();
    },

    goToCreate() {
      uni.navigateTo({
        url: "/pages/create/create",
      });
    },

    updateFilterCounts() {
      const publishedCount = this.publishedWorks.length;
      const draftCount = this.draftWorks.length;
      const reviewingCount = this.reviewingWorks.length;
      const rejectedCount = this.rejectedWorks.length;
      const totalCount =
        publishedCount + draftCount + reviewingCount + rejectedCount;

      this.filterOptions[0].count = totalCount; // 全部
      this.filterOptions[1].count = publishedCount; // 已发布
      this.filterOptions[2].count = reviewingCount; // 审核中
      this.filterOptions[3].count = rejectedCount; // 已退回
      this.filterOptions[4].count = draftCount; // 草稿
    },

    selectSort(value) {
      this.currentSort = value;
      this.showSortModal = false;
    },

    openWork(work) {
      if (work.type === "published") {
        // 查看已发布的作品详情
        this.viewPublishedWork(work);
      } else {
        // 编辑草稿
        this.editDraft(work);
      }
    },

    async viewPublishedWork(work) {
      await this.dialog.alert({
        title: work.title || work.name,
        content: `作品尺寸: ${work.width}×${work.height}\n发布时间: ${this.formatDate(work.createTime)}\n\n${work.description || "暂无描述"}`,
      });
    },

    editDraft(work) {
      // 编辑草稿 - 先进入看板总览
      uni.navigateTo({
        url: `/pages/overview/overview?id=${work.id}`,
      });
    },

    showWorkMenu(work) {
      this.selectedWork = work;
    },

    editWork() {
      if (!this.selectedWork) return;

      if (this.selectedWork.type === "published") {
        this.toast.showError("已发布的作品无法编辑");
        this.selectedWork = null;
        return;
      }

      // 编辑草稿
      this.editDraft(this.selectedWork);
      this.selectedWork = null;
    },

    duplicateWork() {
      if (!this.selectedWork) return;

      this.projectStore.duplicateProject(this.selectedWork.id);
      this.toast.showSuccess("作品已复制");
      this.selectedWork = null;
      this.updateFilterCounts();
    },

    shareWork() {
      if (!this.selectedWork) return;

      const workName = this.selectedWork.name || this.selectedWork.title;
      const shareText =
        this.selectedWork.type === "published"
          ? `我在 Glowxel 发布了作品《${workName}》`
          : `我正在 Glowxel 创作《${workName}》`;

      uni.setClipboardData({
        data: shareText,
        success: () => {
          this.toast.showSuccess("分享文案已复制");
        },
        fail: () => {
          this.toast.showError("复制失败");
        },
      });
      this.selectedWork = null;
    },

    async deleteWork() {
      if (!this.selectedWork) return;

      const workName = this.selectedWork.name || this.selectedWork.title;
      const typeLabel =
        this.selectedWork.type === "published" ? "作品" : "草稿";

      const selectedWork = this.selectedWork;
      const confirmed = await this.dialog.confirm({
        title: "确认删除",
        content: `确定要删除${typeLabel}"${workName}"吗？`,
        danger: true,
      });

      if (!confirmed) {
        this.selectedWork = null;
        return;
      }

      try {
        this.projectStore.deleteProject(selectedWork.id);
        this.toast.showSuccess(`${typeLabel}已删除`);
        this.updateFilterCounts();
      } catch (error) {
        console.error("删除失败:", error);
        this.toast.showError("删除失败");
      }
      this.selectedWork = null;
    },

    loadMore() {
      // 加载更多逻辑
    },

    formatDate(timestamp) {
      const date = new Date(timestamp);
      const now = new Date();
      const diff = now - date;

      if (diff < 60000) return "刚刚";
      if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`;
      if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`;
      if (diff < 604800000) return `${Math.floor(diff / 86400000)}天前`;

      return date.toLocaleDateString();
    },
  },
};
</script>

<style scoped>
.my-works-page {
  height: 100vh;
  background-color: var(--nb-paper);
  display: flex;
  flex-direction: column;
}

.search-section {
  background-color: var(--nb-surface);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
}

.search-bar {
  display: flex;
  align-items: center;
  gap: 16rpx;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  padding: 16rpx 24rpx;
  margin-bottom: 24rpx;
}

.search-input {
  flex: 1;
  font-size: 28rpx;
  color: var(--nb-ink);
}

.clear-btn {
  padding: 8rpx;
  border-radius: 0;
  background-color: #777777;
  color: #ffffff;
  border: 2rpx solid var(--nb-ink);
}

.filter-tabs {
  display: flex;
  gap: 16rpx;
  align-items: stretch;
}

.filter-tab {
  position: relative;
  flex: 1 1 0;
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 104rpx;
  min-height: 88rpx;
  padding: 16rpx 18rpx;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: all 0.2s ease;
  box-sizing: border-box;
}

.filter-tab.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.filter-text {
  font-size: 24rpx;
  color: #4a4a4a;
  font-weight: 700;
  line-height: 1;
  white-space: nowrap;
}

.filter-tab.active .filter-text {
  color: #000000;
  font-weight: 700;
}

.filter-count {
  position: absolute;
  top: 10rpx;
  right: 10rpx;
  min-width: 32rpx;
  height: 32rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  font-size: 18rpx;
  font-weight: 700;
  background-color: #ffffff;
  padding: 0 6rpx;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  color: var(--nb-ink);
}

.content {
  flex: 1;
  padding: 32rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: #4a4a4a;
}

.empty-desc {
  font-size: 24rpx;
  color: #777777;
}

.create-btn {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 48rpx;
  background-color: var(--nb-yellow);
  color: #000000;
  border: 3rpx solid #000000;
  border-radius: 0;
  font-size: 28rpx;
  font-weight: 600;
}

.create-btn::after {
  border: none;
}

.works-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.load-more {
  text-align: center;
  padding: 32rpx;
}

.load-text {
  font-size: 24rpx;
  color: #777777;
}

/* 弹窗样式 */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}

.sort-modal,
.work-menu {
  background-color: var(--nb-surface);
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  margin: 32rpx;
  max-width: 600rpx;
  width: 100%;
  box-shadow: var(--nb-shadow-strong);
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
}

.modal-title,
.menu-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
}

.modal-close {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 2rpx solid var(--nb-ink);
  background: #ffffff;
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
}

.sort-options,
.menu-actions {
  padding: 16rpx 0;
}

.sort-option,
.menu-action {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 32rpx;
  transition: all 0.2s ease;
}

.sort-option:active,
.menu-action:active {
  background-color: var(--nb-paper);
}

.sort-option.active {
  background-color: var(--nb-yellow);
}

.sort-text,
.action-text {
  font-size: 28rpx;
  color: var(--nb-ink);
}

.menu-action {
  gap: 24rpx;
  justify-content: flex-start;
}

.menu-action.danger .action-text {
  color: var(--nb-coral);
}

.menu-header {
  padding: 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
}
</style>
