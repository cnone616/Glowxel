<template>
  <view class="my-favorites-page glx-page-shell">
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
      <text class="nav-title glx-topbar__title">我的收藏</text>
      <view class="nav-right"></view>
    </view>

    <view class="page-actions">
      <view class="page-action-btn" @click="toggleEditMode">
        <text class="page-action-text">{{ isEditMode ? "完成" : "编辑" }}</text>
      </view>
    </view>

    <!-- 分类标签 -->
    <view class="category-tabs">
      <view
        v-for="category in categories"
        :key="category.value"
        class="category-tab glx-choice-chip"
        :class="{ active: currentCategory === category.value }"
        @click="currentCategory = category.value"
      >
        <Icon :name="category.icon" :size="32" />
        <text class="category-text">{{ category.label }}</text>
        <text v-if="category.count > 0" class="category-count">{{
          category.count
        }}</text>
      </view>
    </view>

    <!-- 收藏列表 -->
    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content">
      <view v-if="filteredFavorites.length === 0" class="empty-state glx-panel-card">
        <Icon name="favorite" :size="120" color="var(--text-tertiary)" />
        <text class="empty-title">暂无收藏</text>
        <text class="empty-desc">去社区发现更多精彩作品吧</text>
        <button class="explore-btn glx-cta-button" @click="goToExplore">
          <Icon name="compass" :size="32" />
          <text>去探索</text>
        </button>
      </view>

      <view v-else class="favorites-list">
        <view
          v-for="item in filteredFavorites"
          :key="item.id"
          class="favorite-item glx-panel-card"
          :class="{ 'edit-mode': isEditMode }"
          @click="openFavorite(item)"
        >
          <!-- 选择框 -->
          <view
            v-if="isEditMode"
            class="select-checkbox"
            @click.stop="toggleSelect(item.id)"
          >
            <Icon
              v-if="selectedItems.includes(item.id)"
              name="check-circle-fill"
              :size="40"
              color="var(--nb-yellow)"
            />
            <Icon
              v-else
              name="circle"
              :size="40"
              color="var(--text-tertiary)"
            />
          </view>

          <!-- 缩略图 -->
          <view class="item-thumbnail">
            <image
              v-if="item.thumbnail"
              :src="item.thumbnail"
              class="thumbnail-image"
              mode="aspectFill"
            />
            <view v-else class="thumbnail-placeholder">
              <Icon
                :name="getTypeIcon(item.type)"
                :size="60"
                color="var(--text-tertiary)"
              />
            </view>

            <!-- 类型标识 -->
            <view class="type-badge" :class="item.type">
              <Icon :name="getTypeIcon(item.type)" :size="24" />
            </view>
          </view>

          <!-- 信息 -->
          <view class="item-info">
            <text class="item-title">{{ item.title }}</text>
            <text class="item-author">by {{ item.author }}</text>
            <view class="item-meta">
              <text class="item-size">{{ item.width }}×{{ item.height }}</text>
              <text class="item-date">{{ formatDate(item.favoriteTime) }}</text>
            </view>
            <view v-if="item.tags && item.tags.length > 0" class="item-tags">
              <text
                v-for="tag in item.tags.slice(0, 3)"
                :key="tag"
                class="tag"
                >{{ tag }}</text
              >
            </view>
          </view>

          <!-- 操作按钮 -->
          <view v-if="!isEditMode" class="item-actions">
            <view class="action-btn" @click.stop="downloadFavorite(item)">
              <Icon name="download" :size="32" />
            </view>
            <view class="action-btn danger" @click.stop="removeFavorite(item)">
              <Icon name="ashbin" :size="32" color="currentColor" />
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <!-- 批量操作栏 -->
    <view v-if="isEditMode" class="batch-actions">
      <view class="batch-info">
        <text class="selected-count">已选择 {{ selectedItems.length }} 项</text>
      </view>
      <view class="batch-buttons">
        <button
          class="batch-btn"
          @click="batchDownload"
          :disabled="selectedItems.length === 0"
        >
          <Icon name="download" :size="32" />
          <text>下载</text>
        </button>
        <button
          class="batch-btn danger"
          @click="batchRemove"
          :disabled="selectedItems.length === 0"
        >
          <Icon name="delete" :size="32" />
          <text>移除</text>
        </button>
      </view>
    </view>

    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { collectionAPI } from "../../api/index.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
  },

  data() {
    return {
      toast: null,
      currentCategory: "all",
      isEditMode: false,
      selectedItems: [],
      categories: [
        { value: "all", label: "全部", icon: "grid", count: 0 },
        { value: "artwork", label: "作品", icon: "picture", count: 0 },
      ],
      favorites: [],
    };
  },

  computed: {
    filteredFavorites() {
      if (this.currentCategory === "all") {
        return this.favorites;
      }
      return this.favorites.filter(
        (item) => item.type === this.currentCategory,
      );
    },
  },

  onLoad() {
    this.toast = useToast();
    this.loadFavorites();

    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
    });
  },

  onShow() {
    this.loadFavorites();
  },

  methods: {
    async saveFavoriteImage(imageUrl, fileName) {
      if (!imageUrl) {
        throw new Error("image url missing");
      }

      // #ifdef H5
      const link = document.createElement("a");
      link.href = imageUrl;
      link.download = `${fileName}-${Date.now()}.png`;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      return;
      // #endif

      // #ifndef H5
      const downloadRes = await new Promise((resolve, reject) => {
        uni.downloadFile({
          url: imageUrl,
          success: (res) => {
            if (res.statusCode === 200) {
              resolve(res);
              return;
            }
            reject(new Error("download failed"));
          },
          fail: reject,
        });
      });

      await new Promise((resolve, reject) => {
        uni.saveImageToPhotosAlbum({
          filePath: downloadRes.tempFilePath,
          success: resolve,
          fail: reject,
        });
      });
      // #endif
    },

    handleBack() {
      uni.navigateBack();
    },

    goToExplore() {
      uni.switchTab({
        url: "/pages/community/community",
      });
    },

    async loadFavorites() {
      try {
        const res = await collectionAPI.getMyCollections(1, 100);
        if (res.success && res.data) {
          this.favorites = res.data.list.map((item) => ({
            id: item.id,
            type: "artwork",
            title: item.title,
            author: item.author_name,
            width: item.width,
            height: item.height,
            thumbnail: item.cover_url,
            favoriteTime: item.created_at,
            tags: [],
          }));
        } else {
          this.favorites = [];
        }
        this.updateCategoryCounts();
      } catch (error) {
        console.error("加载收藏失败:", error);
        this.toast.showError("加载收藏失败");
      }
    },

    updateCategoryCounts() {
      this.categories[0].count = this.favorites.length;
      this.categories[1].count = this.favorites.filter(
        (f) => f.type === "artwork",
      ).length;
    },

    toggleEditMode() {
      this.isEditMode = !this.isEditMode;
      if (!this.isEditMode) {
        this.selectedItems = [];
      }
    },

    toggleSelect(id) {
      const index = this.selectedItems.indexOf(id);
      if (index > -1) {
        this.selectedItems.splice(index, 1);
      } else {
        this.selectedItems.push(id);
      }
    },

    openFavorite(item) {
      if (this.isEditMode) {
        this.toggleSelect(item.id);
        return;
      }

      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${item.id}`,
      });
    },

    async downloadFavorite(item) {
      try {
        await this.saveFavoriteImage(item.thumbnail, item.title);
        this.toast.showSuccess("下载成功");
      } catch (error) {
        console.error("下载收藏失败:", error);
        this.toast.showError("下载失败");
      }
    },

    async removeFavorite(item) {
      uni.showModal({
        title: "移除收藏",
        content: `确定要移除"${item.title}"吗？`,
        success: async (res) => {
          if (res.confirm) {
            try {
              await collectionAPI.uncollectArtwork(item.id);
              const index = this.favorites.findIndex((f) => f.id === item.id);
              if (index > -1) {
                this.favorites.splice(index, 1);
                this.updateCategoryCounts();
              }
              this.toast.showSuccess("已移除收藏");
            } catch (error) {
              console.error("移除收藏失败:", error);
              this.toast.showError("移除失败");
            }
          }
        },
      });
    },

    async batchDownload() {
      if (this.selectedItems.length === 0) return;

      try {
        const selectedFavorites = this.favorites.filter((item) =>
          this.selectedItems.includes(item.id),
        );
        for (const item of selectedFavorites) {
          await this.saveFavoriteImage(item.thumbnail, item.title);
        }
        this.toast.showSuccess("批量下载成功");
      } catch (error) {
        console.error("批量下载收藏失败:", error);
        this.toast.showError("批量下载失败");
      }
    },

    async batchRemove() {
      if (this.selectedItems.length === 0) return;

      uni.showModal({
        title: "批量移除",
        content: `确定要移除选中的 ${this.selectedItems.length} 项收藏吗？`,
        success: async (res) => {
          if (res.confirm) {
            try {
              await Promise.all(
                this.selectedItems.map((id) => collectionAPI.uncollectArtwork(id)),
              );
              this.favorites = this.favorites.filter(
                (f) => !this.selectedItems.includes(f.id),
              );
              this.selectedItems = [];
              this.updateCategoryCounts();
              this.toast.showSuccess("批量移除成功");
            } catch (error) {
              console.error("批量移除收藏失败:", error);
              this.toast.showError("批量移除失败");
            }
          }
        },
      });
    },

    getTypeIcon(type) {
      const icons = {
        artwork: "picture",
      };
      return icons[type] || "picture";
    },

    formatDate(timestamp) {
      const date = new Date(timestamp);
      const now = new Date();
      const diff = now - date;

      if (diff < 86400000) return "今天";
      if (diff < 172800000) return "昨天";
      if (diff < 604800000) return `${Math.floor(diff / 86400000)}天前`;

      return date.toLocaleDateString();
    },
  },
};
</script>

<style scoped>
.my-favorites-page {
  height: 100vh;
  background-color: var(--nb-paper);
  display: flex;
  flex-direction: column;
}

.edit-btn {
  font-size: 28rpx;
  color: var(--nb-yellow);
  font-weight: 600;
}

.page-actions {
  display: flex;
  justify-content: flex-end;
  padding: 20rpx 32rpx 0;
}

.page-action-btn {
  min-height: 68rpx;
  padding: 0 24rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background: #ffffff;
  border: 4rpx solid #000000;
  border-radius: 0;
  box-shadow: none;
}

.page-action-text {
  font-size: 24rpx;
  font-weight: 700;
  color: #000000;
}

.category-tabs {
  display: flex;
  background-color: var(--nb-surface);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
  gap: 16rpx;
}

.category-tab {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx 24rpx;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: all 0.2s ease;
  flex-shrink: 0;
}

.category-tab.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
  color: #000000;
}

.category-text {
  font-size: 24rpx;
  color: #4a4a4a;
}

.category-tab.active .category-text {
  color: #000000;
  font-weight: 700;
}

.category-count {
  font-size: 20rpx;
  background-color: #ffffff;
  padding: 4rpx 8rpx;
  border-radius: 0;
  color: inherit;
  border: 2rpx solid var(--nb-ink);
}

.content {
  flex: 1;
  padding: 32rpx;
  box-sizing: border-box;
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

.explore-btn {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 48rpx;
  background-color: var(--nb-yellow);
  color: #000000;
  border: none;
  border-radius: 0;
  font-size: 28rpx;
  font-weight: 600;
}

.explore-btn::after {
  border: none;
}

.favorites-list {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.favorite-item {
  display: flex;
  align-items: center;
  gap: 24rpx;
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 24rpx;
  box-shadow: var(--nb-shadow-strong);
  transition: all 0.2s ease;
}

.favorite-item:active {
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
}

.favorite-item.edit-mode {
  padding-left: 16rpx;
}

.select-checkbox {
  padding: 8rpx;
}

.item-thumbnail {
  position: relative;
  width: 120rpx;
  height: 120rpx;
  border-radius: 0;
  overflow: hidden;
  background-color: var(--nb-paper);
  flex-shrink: 0;
}

.thumbnail-image {
  width: 100%;
  height: 100%;
}

.thumbnail-placeholder {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.type-badge {
  position: absolute;
  bottom: 8rpx;
  right: 8rpx;
  width: 32rpx;
  height: 32rpx;
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #000000;
  border: 2rpx solid var(--nb-ink);
}

.type-badge.artwork {
  background-color: var(--nb-yellow);
}

.type-badge.template {
  background-color: #111111;
  color: #ffffff;
}

.type-badge.pattern {
  background-color: var(--nb-coral);
  color: #ffffff;
}

.item-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.item-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--nb-ink);
  display: -webkit-box;
  -webkit-line-clamp: 1;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.item-author {
  font-size: 24rpx;
  color: #4a4a4a;
}

.item-meta {
  display: flex;
  gap: 16rpx;
}

.item-size,
.item-date {
  font-size: 22rpx;
  color: #777777;
}

.item-tags {
  display: flex;
  gap: 8rpx;
  flex-wrap: wrap;
}

.tag {
  font-size: 20rpx;
  color: #000000;
  background-color: #fff4c4;
  padding: 4rpx 8rpx;
  border-radius: 0;
}

.item-actions {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.action-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: all 0.2s ease;
  box-shadow: 2rpx 2rpx 0 #000000;
}

.action-btn:active {
  box-shadow: none;
}

.action-btn.danger {
  background-color: #d92d20;
  border-color: #000000;
  color: #ffffff;
}

.batch-actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 32rpx;
  background-color: var(--nb-surface);
  border-top: 2rpx solid var(--nb-ink);
}

.selected-count {
  font-size: 24rpx;
  color: #4a4a4a;
}

.batch-buttons {
  display: flex;
  gap: 16rpx;
}

.batch-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  min-width: 176rpx;
  height: 88rpx;
  padding: 0 24rpx;
  background-color: var(--nb-yellow);
  color: #000000;
  border: 3rpx solid #000000;
  border-radius: 0;
  box-shadow: 2rpx 2rpx 0 #000000;
  font-size: 24rpx;
  transition: all 0.2s ease;
}

.batch-btn:disabled {
  background-color: #777777;
  opacity: 0.5;
}

.batch-btn.danger {
  background-color: #d92d20;
  color: #ffffff;
}

.batch-btn::after {
  border: none;
}
</style>
