<template>
  <view class="community-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 顶部导航 -->
    <view class="header">
      <view class="header-content">
        <text class="header-title">社区</text>
        <view class="header-actions"> </view>
      </view>

      <!-- 分类标签 -->
      <view class="category-tabs">
        <scroll-view scroll-x class="tabs-scroll">
          <view class="tabs-container">
            <view
              v-for="(category, index) in categories"
              :key="category.key"
              class="tab-item"
              :class="{ active: activeCategory === category.key }"
              @click="switchCategory(category.key)"
            >
              <text class="tab-text">{{ category.name }}</text>
            </view>
          </view>
        </scroll-view>
      </view>
    </view>
    <!-- 主要内容 -->
    <scroll-view
      scroll-y
      class="main-content"
      @scrolltolower="loadMore"
      :refresher-enabled="true"
      :refresher-triggered="isRefreshing"
      @refresherrefresh="handleRefresh"
    >
      <!-- 推荐用户（仅推荐分类显示） -->
      <view v-if="activeCategory === 'recommended'" class="recommended-users">
        <view class="section-header">
          <text class="section-title">推荐创作者</text>
          <view class="more-btn" @click="goToUserList">
            <text class="more-text">查看更多</text>
            <Icon name="arrow-right" :size="24" color="#4F7FFF" />
          </view>
        </view>

        <scroll-view scroll-x class="users-scroll">
          <view class="users-list">
            <view
              v-for="user in recommendedUsers"
              :key="user.id"
              class="user-card"
              @click="goToUserProfile(user)"
            >
              <Avatar
                :src="user.avatar"
                size="large"
                :showBadge="user.isOnline"
                :status="user.isOnline ? 'online' : 'offline'"
              />
              <text class="user-name">{{ user.name }}</text>
              <text class="user-works">{{ user.worksCount }} 作品</text>
              <view
                class="follow-btn"
                :class="{ following: user.isFollowing }"
                @click.stop="toggleFollow(user)"
              >
                <text class="follow-text">{{
                  user.isFollowing ? "已关注" : "关注"
                }}</text>
              </view>
            </view>
          </view>
        </scroll-view>
      </view>

      <!-- 作品列表 -->
      <view class="artworks-section">
        <view
          v-if="filteredArtworks.length === 0 && !isLoading"
          class="empty-state"
        >
          <view class="empty-icon">
            <Icon name="picture" :size="80" color="#AAAAAA" />
          </view>
          <text class="empty-title">{{
            searchTerm ? "未找到相关作品" : "暂无作品"
          }}</text>
          <text class="empty-subtitle">{{
            searchTerm ? "尝试其他关键词" : "快来发布第一个作品吧！"
          }}</text>
        </view>

        <view v-else class="artworks-grid">
          <ArtworkCard
            v-for="artwork in filteredArtworks"
            :key="artwork.id"
            :artwork="artwork"
            :liked="likedArtworks.has(artwork.id)"
            @click="handleArtworkClick"
            @like="handleArtworkLike"
            @comment="handleArtworkComment"
          />
        </view>

        <!-- 加载更多 -->
        <view v-if="isLoading" class="loading-more">
          <Icon
            name="loading"
            :size="48"
            color="#4F7FFF"
            class="loading-icon"
          />
          <text class="loading-text">加载中...</text>
        </view>

        <view
          v-if="hasMore && !isLoading && filteredArtworks.length > 0"
          class="load-more-btn"
          @click="loadMore"
        >
          <text class="load-more-text">加载更多</text>
        </view>

        <view v-if="!hasMore && filteredArtworks.length > 0" class="no-more">
          <text class="no-more-text">没有更多了</text>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import { artworkAPI, userAPI } from "../../api/index.js";
import statusBarMixin from "../../mixins/statusBar.js";
import ArtworkCard from "../../components/ArtworkCard.vue";
import Avatar from "../../components/Avatar.vue";
import Icon from "../../components/Icon.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    ArtworkCard,
    Avatar,
    Icon,
  },

  data() {
    return {
      searchTerm: "",
      showSearch: false,
      activeCategory: "recommended",
      artworks: [],
      recommendedUsers: [],
      likedArtworks: new Set(),
      isLoading: false,
      isRefreshing: false,
      hasMore: true,
      currentPage: 1,
      pageSize: 10,
      categories: [
        { key: "recommended", name: "推荐" },
        { key: "latest", name: "最新" },
        { key: "popular", name: "热门" },
        { key: "pixel-art", name: "像素画" },
        { key: "animation", name: "动画" },
        { key: "character", name: "角色" },
        { key: "landscape", name: "风景" },
        { key: "abstract", name: "抽象" },
      ],
    };
  },

  computed: {
    filteredArtworks() {
      if (!this.searchTerm) {
        return this.artworks;
      }

      const term = this.searchTerm.toLowerCase();
      return this.artworks.filter(
        (artwork) =>
          artwork.title.toLowerCase().includes(term) ||
          artwork.author.name.toLowerCase().includes(term) ||
          (artwork.tags &&
            artwork.tags.some((tag) => tag.toLowerCase().includes(term))),
      );
    },
  },

  onLoad() {
    this.loadInitialData();
  },

  onShow() {
    // 刷新数据
    this.handleRefresh();
  },

  methods: {
    async loadInitialData() {
      this.isLoading = true;

      try {
        // 加载推荐用户
        const recRes = await userAPI.getRecommendedUsers(8);
        this.recommendedUsers = recRes.success
          ? (recRes.data || []).slice(0, 8)
          : [];

        // 加载作品
        await this.loadArtworks(true);
      } catch (error) {
        console.error("加载数据失败:", error);
        uni.showToast({
          title: "加载失败",
          icon: "error",
        });
      } finally {
        this.isLoading = false;
      }
    },

    async loadArtworks(reset = false) {
      if (reset) {
        this.currentPage = 1;
        this.artworks = [];
        this.hasMore = true;
      }

      try {
        let newArtworks = [];

        switch (this.activeCategory) {
          case "recommended":
            const popRes = await artworkAPI.getPopularArtworks(1, 20);
            newArtworks = popRes.success ? popRes.data?.list || [] : [];
            break;
          case "latest":
            const latRes = await artworkAPI.getLatestArtworks(1, 20);
            newArtworks = latRes.success ? latRes.data?.list || [] : [];
            break;
          case "popular":
            const folRes = await artworkAPI.getFollowingArtworks(1, 20);
            newArtworks = folRes.success ? folRes.data?.list || [] : [];
            break;
          default:
            // 按标签搜索
            const tagRes = await artworkAPI.searchByTag(this.activeCategory);
            newArtworks = tagRes.success
              ? tagRes.data?.list || tagRes.data || []
              : [];
            break;
        }

        // 模拟分页
        const startIndex = (this.currentPage - 1) * this.pageSize;
        const endIndex = startIndex + this.pageSize;
        const pageData = newArtworks.slice(startIndex, endIndex);

        if (reset) {
          this.artworks = pageData;
        } else {
          this.artworks = [...this.artworks, ...pageData];
        }

        this.hasMore = endIndex < newArtworks.length;
        this.currentPage++;
      } catch (error) {
        console.error("加载作品失败:", error);
        throw error;
      }
    },

    async handleRefresh() {
      this.isRefreshing = true;

      try {
        await this.loadArtworks(true);

        // 刷新推荐用户
        if (this.activeCategory === "recommended") {
          const recRes2 = await userAPI.getRecommendedUsers(8);
          this.recommendedUsers = recRes2.success
            ? (recRes2.data || []).slice(0, 8)
            : [];
        }
      } catch (error) {
        console.error("刷新失败:", error);
      } finally {
        this.isRefreshing = false;
      }
    },

    async loadMore() {
      if (this.isLoading || !this.hasMore) return;

      this.isLoading = true;

      try {
        await this.loadArtworks(false);
      } catch (error) {
        console.error("加载更多失败:", error);
      } finally {
        this.isLoading = false;
      }
    },

    async switchCategory(category) {
      if (this.activeCategory === category) return;

      this.activeCategory = category;
      await this.loadArtworks(true);
    },

    toggleSearch() {
      this.showSearch = !this.showSearch;
      if (!this.showSearch) {
        this.searchTerm = "";
      }
    },

    handleSearchBlur() {
      if (!this.searchTerm) {
        this.showSearch = false;
      }
    },

    clearSearch() {
      this.searchTerm = "";
    },

    handleArtworkClick(artwork) {
      // 跳转到作品详情页
      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${artwork.id}`,
      });
    },

    handleArtworkLike(data) {
      const { artwork, liked } = data;

      if (liked) {
        this.likedArtworks.add(artwork.id);
        artwork.likes = (artwork.likes || 0) + 1;
      } else {
        this.likedArtworks.delete(artwork.id);
        artwork.likes = Math.max(0, (artwork.likes || 0) - 1);
      }

      // TODO: 发送点赞请求到服务器
      console.log("点赞作品:", artwork.id, liked);
    },

    handleArtworkComment(artwork) {
      // TODO: 跳转到评论页面
      uni.showToast({
        title: "评论功能开发中",
        icon: "none",
      });
    },

    toggleFollow(user) {
      user.isFollowing = !user.isFollowing;

      if (user.isFollowing) {
        user.followersCount = (user.followersCount || 0) + 1;
      } else {
        user.followersCount = Math.max(0, (user.followersCount || 0) - 1);
      }

      // TODO: 发送关注请求到服务器
      console.log("关注用户:", user.id, user.isFollowing);

      uni.showToast({
        title: user.isFollowing ? "已关注" : "已取消关注",
        icon: "success",
      });
    },

    goToUserProfile(user) {
      // 跳转到用户详情页
      uni.navigateTo({
        url: `/pages/user-detail/user-detail?id=${user.id}`,
      });
    },

    goToUserList() {
      // TODO: 跳转到用户列表页
      uni.showToast({
        title: "用户列表页开发中",
        icon: "none",
      });
    },
  },
};
</script>

<style scoped>
.community-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--color-app-background);
  overflow: hidden;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--color-card-background);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background-color: var(--color-card-background);
  border-bottom: 1rpx solid var(--border-primary);
  flex-shrink: 0;
}

.header-content {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24rpx 32rpx;
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.search-btn {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-medium);
  background-color: var(--color-app-background);
  transition: all 0.2s ease;
}

.search-btn:active {
  transform: scale(0.95);
  background-color: rgba(79, 127, 255, 0.1);
}

/* 分类标签 */
.category-tabs {
  padding: 0 32rpx 24rpx;
}

.tabs-scroll {
  white-space: nowrap;
}

.tabs-container {
  display: flex;
  gap: 16rpx;
}

.tab-item {
  flex-shrink: 0;
  padding: 16rpx 24rpx;
  border-radius: var(--radius-small);
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.tab-item.active {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
}

.tab-item:active {
  transform: scale(0.95);
}

.tab-text {
  font-size: 26rpx;
  font-weight: 500;
  color: var(--color-text-secondary);
  white-space: nowrap;
}

.tab-item.active .tab-text {
  color: #ffffff;
  font-weight: 600;
}

/* 搜索栏 */
.search-section {
  background-color: var(--color-card-background);
  padding: 0 32rpx 24rpx;
  border-bottom: 1rpx solid var(--border-primary);
}

/* 主要内容 */
.main-content {
  flex: 1;
  overflow-y: auto;
}

/* 搜索输入框 */
.search-section {
  background-color: var(--color-card-background);
  padding: 0 32rpx 24rpx;
  border-bottom: 1rpx solid var(--border-primary);
}

.search-input-wrapper {
  display: flex;
  align-items: center;
  gap: 16rpx;
  background-color: var(--color-app-background);
  border-radius: var(--radius-medium);
  padding: 20rpx 24rpx;
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.search-input-wrapper:focus-within {
  border-color: var(--color-brand-primary);
  box-shadow: 0 0 0 4rpx rgba(79, 127, 255, 0.1);
}

.search-input {
  flex: 1;
  font-size: 28rpx;
  color: var(--color-text-primary);
  background: transparent;
  border: none;
  outline: none;
}

.search-input::placeholder {
  color: var(--color-text-disabled);
}

.clear-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  background-color: var(--color-app-background);
  transition: all 0.2s ease;
}

.clear-btn:active {
  transform: scale(0.9);
}

/* 主要内容 */
.main-content {
  flex: 1;
  overflow-y: auto;
}

/* 通用区块样式 */
.section-title {
  font-size: 32rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24rpx;
}

.more-btn {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 12rpx 16rpx;
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
}

.more-btn:active {
  background-color: rgba(79, 127, 255, 0.1);
  transform: scale(0.95);
}

.more-text {
  font-size: 26rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}

/* 推荐用户 */
.recommended-users {
  margin: 32rpx 0 48rpx;
}

.recommended-users .section-header {
  margin: 0 32rpx 24rpx;
}

.users-scroll {
  white-space: nowrap;
}

.users-list {
  display: flex;
  gap: 24rpx;
  padding: 0 32rpx;
}

.user-card {
  flex-shrink: 0;
  width: 200rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 32rpx 24rpx;
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.user-card:active {
  transform: scale(0.95);
  box-shadow: var(--shadow-floating);
}

.user-name {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin: 16rpx 0 8rpx;
  text-align: center;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  width: 100%;
}

.user-works {
  font-size: 22rpx;
  color: var(--color-text-secondary);
  margin-bottom: 20rpx;
}

.follow-btn {
  padding: 12rpx 24rpx;
  border-radius: var(--radius-small);
  background-color: var(--color-brand-primary);
  transition: all 0.2s ease;
}

.follow-btn.following {
  background-color: var(--color-text-secondary);
}

.follow-btn:active {
  transform: scale(0.95);
}

.follow-text {
  font-size: 24rpx;
  font-weight: 500;
  color: #ffffff;
}

/* 作品区域 */
.artworks-section {
  margin: 0 32rpx;
}

.artworks-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
  margin-bottom: 48rpx;
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 96rpx 32rpx;
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
}

/* 加载状态 */
.loading-more {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48rpx;
  gap: 16rpx;
}

.loading-icon {
  animation: rotate 1s linear infinite;
}

@keyframes rotate {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}

.loading-text {
  font-size: 26rpx;
  color: var(--color-text-secondary);
}

.load-more-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  margin: 32rpx 0;
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.load-more-btn:active {
  transform: scale(0.98);
  background-color: var(--color-app-background);
}

.load-more-text {
  font-size: 28rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}

.no-more {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
}

.no-more-text {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}
</style>
