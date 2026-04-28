<template>
  <view class="community-page glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 顶部导航 -->
    <view class="header glx-topbar glx-page-shell__fixed">
      <view class="header-content">
        <view class="header-placeholder"></view>
        <text class="header-title glx-topbar__title">社区</text>
        <view class="header-placeholder"></view>
      </view>

      <!-- 分类标签 -->
      <view class="category-tabs">
        <scroll-view scroll-x class="tabs-scroll">
          <view class="tabs-container">
            <view
              v-for="(category, index) in categories"
              :key="category.key"
              class="tab-item glx-choice-chip"
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
      class="main-content glx-scroll-region glx-page-shell__content"
      @scrolltolower="loadMore"
      :refresher-enabled="true"
      :refresher-triggered="isRefreshing"
      @refresherrefresh="handleRefresh"
    >
      <!-- 推荐用户（仅推荐分类显示） -->
      <view v-if="activeCategory === 'recommended'" class="recommended-users">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">推荐创作者</text>
          <view class="more-btn glx-inline-cta" @click="goToUserList">
            <text class="more-text">查看更多</text>
            <Icon name="arrow-right" :size="24" class="glx-inline-cta__icon" />
          </view>
        </view>

        <scroll-view scroll-x class="users-scroll">
          <view class="users-list">
            <view
              v-for="user in recommendedUsers"
              :key="user.id"
              class="user-card glx-panel-card"
              @click="goToUserProfile(user)"
            >
              <Avatar
                :src="user.avatar"
                size="large"
                :showBadge="user.isOnline"
                :status="user.isOnline ? 'online' : 'offline'"
              />
              <text class="user-name">{{ user.name }}</text>
              <text class="user-works">{{ user.works_count }} 作品</text>
              <view
                class="follow-btn glx-cta-button"
                :class="{ following: followingUserIds.has(user.id) }"
                @click.stop="toggleFollow(user)"
              >
                <text class="follow-text">{{
                  followingUserIds.has(user.id) ? "已关注" : "关注"
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
          class="empty-state glx-panel-card"
        >
          <view class="empty-icon">
            <Icon name="picture" :size="80" color="var(--text-tertiary)" />
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
          <GlxInlineLoader class="loading-inline-loader" variant="flip" size="sm" />
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

    <Toast />
  </view>
</template>

<script>
import { artworkAPI, followAPI, likeAPI, userAPI } from "../../api/index.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import ArtworkCard from "../../components/ArtworkCard.vue";
import Avatar from "../../components/Avatar.vue";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import GlxInlineLoader from "../../components/GlxInlineLoader.vue";

const toast = useToast();

export default {
  mixins: [statusBarMixin],
  components: {
    ArtworkCard,
    Avatar,
    Icon,
    Toast,
    GlxInlineLoader,
  },

  data() {
    return {
      searchTerm: "",
      showSearch: false,
      activeCategory: "recommended",
      artworks: [],
      recommendedUsers: [],
      likedArtworks: new Set(),
      followingUserIds: new Set(),
      currentUserId: null,
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
          artwork.author_name.toLowerCase().includes(term) ||
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
        await this.loadFollowingUserIds();

        // 加载推荐用户
        const recRes = await userAPI.getRecommendedUsers(8);
        if (recRes.success && recRes.data && recRes.data.list) {
          this.recommendedUsers = recRes.data.list.slice(0, 8);
        } else {
          this.recommendedUsers = [];
        }

        // 加载作品
        await this.loadArtworks(true);
      } catch (error) {
        console.error("加载数据失败:", error);
        toast.showError("加载失败");
      } finally {
        this.isLoading = false;
      }
    },

    async loadFollowingUserIds() {
      const token = uni.getStorageSync("auth_token");
      if (!token) {
        this.followingUserIds = new Set();
        this.currentUserId = null;
        return;
      }

      const profileRes = await userAPI.getProfile();
      if (!(profileRes.success && profileRes.data && profileRes.data.user)) {
        this.followingUserIds = new Set();
        this.currentUserId = null;
        return;
      }

      this.currentUserId = profileRes.data.user.id;
      const followRes = await followAPI.getFollowing(this.currentUserId, 1, 100);
      if (followRes.success && followRes.data && followRes.data.list) {
        this.followingUserIds = new Set(
          followRes.data.list.map((user) => user.id),
        );
      } else {
        this.followingUserIds = new Set();
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
            newArtworks =
              popRes.success && popRes.data ? popRes.data.list : [];
            break;
          case "latest":
            const latRes = await artworkAPI.getLatestArtworks(1, 20);
            newArtworks =
              latRes.success && latRes.data ? latRes.data.list : [];
            break;
          case "popular":
            const hotRes = await artworkAPI.getPopularArtworks(1, 20);
            newArtworks =
              hotRes.success && hotRes.data ? hotRes.data.list : [];
            break;
          default:
            // 按标签搜索
            const tagRes = await artworkAPI.searchByTag(this.activeCategory);
            newArtworks =
              tagRes.success && tagRes.data ? tagRes.data.list : [];
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
          await this.loadFollowingUserIds();
          const recRes2 = await userAPI.getRecommendedUsers(8);
          if (recRes2.success && recRes2.data && recRes2.data.list) {
            this.recommendedUsers = recRes2.data.list.slice(0, 8);
          } else {
            this.recommendedUsers = [];
          }
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

    async handleArtworkLike(data) {
      const { artwork, liked } = data;

      try {
        if (liked) {
          await likeAPI.likeArtwork(artwork.id);
          this.likedArtworks.add(artwork.id);
          artwork.likes += 1;
        } else {
          await likeAPI.unlikeArtwork(artwork.id);
          this.likedArtworks.delete(artwork.id);
          artwork.likes -= 1;
        }
      } catch (error) {
        console.error("点赞作品失败:", error);
        toast.showError("操作失败");
      }
    },

    handleArtworkComment(artwork) {
      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${artwork.id}`,
      });
    },

    async toggleFollow(user) {
      try {
        const res = await followAPI.toggleFollow(user.id);
        if (!(res.success && res.data)) {
          throw new Error("follow failed");
        }

        if (res.data.followed) {
          this.followingUserIds.add(user.id);
          user.followers_count += 1;
        } else {
          this.followingUserIds.delete(user.id);
          user.followers_count -= 1;
        }

        toast.showSuccess(res.data.followed ? "已关注" : "已取消关注");
      } catch (error) {
        console.error("关注用户失败:", error);
        toast.showError("操作失败");
      }
    },

    goToUserProfile(user) {
      // 跳转到用户详情页
      uni.navigateTo({
        url: `/pages/user-detail/user-detail?id=${user.id}`,
      });
    },

    goToUserList() {
      uni.navigateTo({
        url: "/pages/user-list/user-list?type=recommended",
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
  background-color: var(--nb-paper);
  overflow: hidden;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--nb-surface);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background-color: var(--nb-surface);
  border-bottom: 1rpx solid var(--nb-ink);
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
  color: var(--nb-ink);
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
  border-radius: 0;
  background-color: var(--nb-paper);
  transition: all 0.2s ease;
}

.search-btn:active {
  background-color: var(--nb-yellow);
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
  border-radius: 0;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
}

.tab-item.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.tab-text {
  font-size: 26rpx;
  font-weight: 500;
  color: #4a4a4a;
  white-space: nowrap;
}

.tab-item.active .tab-text {
  color: #000000;
  font-weight: 700;
}

/* 搜索栏 */
.search-section {
  background-color: var(--nb-surface);
  padding: 0 32rpx 24rpx;
  border-bottom: 1rpx solid var(--nb-ink);
}

/* 主要内容 */
.main-content {
  flex: 1;
  overflow-y: auto;
}

/* 搜索输入框 */
.search-section {
  background-color: var(--nb-surface);
  padding: 0 32rpx 24rpx;
  border-bottom: 1rpx solid var(--nb-ink);
}

.search-input {
  flex: 1;
  font-size: 28rpx;
  color: var(--nb-ink);
  background: transparent;
  border: none;
  outline: none;
}

.search-input::placeholder {
  color: #777777;
}

.clear-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
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
  color: var(--nb-ink);
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24rpx;
}

.more-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  transition: all 0.2s ease;
}

.more-text {
  font-size: 26rpx;
  color: var(--nb-ink);
  font-weight: 900;
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
  background-color: var(--nb-surface);
  border-radius: 0;
  box-shadow: var(--nb-shadow-strong);
  transition: all 0.2s ease;
}

.user-card:active {
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
}

.user-name {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--nb-ink);
  margin: 16rpx 0 8rpx;
  text-align: center;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  width: 100%;
}

.user-works {
  font-size: 22rpx;
  color: #4a4a4a;
  margin-bottom: 20rpx;
}

.follow-btn {
  padding: 12rpx 24rpx;
  border-radius: 0;
  background-color: var(--nb-yellow);
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
}

.follow-btn.following {
  background-color: #111111;
}

.follow-text {
  font-size: 24rpx;
  font-weight: 500;
  color: #000000;
}

.follow-btn.following .follow-text {
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

.empty-icon {
  width: 160rpx;
  height: 160rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-paper);
  border-radius: 50%;
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
  color: #4a4a4a;
  line-height: 1.5;
}

/* 加载状态 */
.loading-more {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48rpx;
  gap: 16rpx;
  color: var(--nb-blue);
}

.loading-inline-loader {
  flex-shrink: 0;
}

.loading-text {
  font-size: 26rpx;
  color: currentColor;
}

.load-more-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  margin: 32rpx 0;
  background-color: var(--nb-surface);
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
}

.load-more-btn:active {
  background-color: var(--nb-paper);
}

.load-more-text {
  font-size: 28rpx;
  color: #000000;
  font-weight: 700;
}

.no-more {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
}

.no-more-text {
  font-size: 24rpx;
  color: #777777;
}
</style>
