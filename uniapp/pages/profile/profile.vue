<template>
  <view class="profile-page glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    <scroll-view
      scroll-y
      class="content glx-scroll-region glx-page-shell__content"
    >
      <!-- 用户信息卡片 -->
      <view class="user-card glx-panel-card">
        <view v-if="!userStore.hasLogin" class="login-prompt">
          <view class="avatar-placeholder">
            <Icon name="user" :size="80" />
          </view>
          <text class="prompt-text">登录后可同步数据到云端</text>
          <button class="login-btn glx-cta-button" @click="handleLogin">
            <Icon name="user" :size="28" />
            <text>微信登录</text>
          </button>
        </view>

        <view v-else class="user-info">
          <view class="user-avatar">
            <image
              v-if="userStore.avatar"
              :src="userStore.avatar"
              class="avatar"
              mode="aspectFill"
            />
            <view v-else class="avatar-placeholder">
              <Icon name="user" :size="80" />
            </view>
          </view>

          <view class="user-details">
            <view class="user-name-row">
              <text class="nickname">{{ userStore.nickname || "陈晓明" }}</text>
              <button
                class="edit-btn glx-cta-button"
                @click="handleEditProfile"
              >
                <text>编辑</text>
              </button>
            </view>
            <text class="user-id"
              >@{{
                userStore.openid ? userStore.openid.slice(0, 8) : "alexchen"
              }}</text
            >
          </view>
        </view>

        <!-- 统计数据 -->
        <view v-if="userStore.hasLogin" class="user-stats">
          <view class="stat-item" @click="goToMyWorks">
            <text class="stat-number">{{ publishedCount }}</text>
            <text class="stat-label">作品</text>
          </view>
          <view class="stat-item" @click="goToFollowers">
            <text class="stat-number">1.2K</text>
            <text class="stat-label">粉丝</text>
          </view>
          <view class="stat-item" @click="goToFollowing">
            <text class="stat-number">342</text>
            <text class="stat-label">关注</text>
          </view>
        </view>
      </view>

      <!-- 功能菜单 -->
      <view class="menu-section glx-scroll-stack">
        <!-- 我的作品 -->
        <view
          class="menu-item menu-item--works glx-list-card"
          @click="goToMyWorks"
        >
          <view class="menu-left">
            <view class="menu-icon menu-icon--works">
              <Icon name="picture" :size="40" color="var(--nb-ink)" />
            </view>
            <text class="menu-label">我的作品</text>
          </view>
          <view class="menu-right glx-list-meta">
            <text class="menu-count">{{ publishedCount }}</text>
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>

        <!-- 我的收藏 -->
        <view
          class="menu-item menu-item--favorites glx-list-card"
          @click="goToMyFavorites"
        >
          <view class="menu-left">
            <view class="menu-icon menu-icon--favorites">
              <Icon name="favorite" :size="40" color="var(--nb-ink)" />
            </view>
            <text class="menu-label">我的收藏</text>
          </view>
          <view class="menu-right glx-list-meta">
            <text class="menu-count">12</text>
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>

        <!-- 成就 -->
        <view
          class="menu-item menu-item--achievements glx-list-card"
          @click="goToAchievements"
        >
          <view class="menu-left">
            <view class="menu-icon menu-icon--achievements">
              <Icon name="task" :size="40" color="var(--nb-ink)" />
            </view>
            <text class="menu-label">成就</text>
          </view>
          <view class="menu-right glx-list-meta">
            <text class="menu-count">8</text>
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>

        <!-- 云同步设置 -->
        <view
          v-if="userStore.hasLogin"
          class="menu-item menu-item--sync glx-list-card"
          @click="goToCloudSync"
        >
          <view class="menu-left">
            <view class="menu-icon menu-icon--sync">
              <Icon name="switch" :size="40" color="var(--nb-ink)" />
            </view>
            <view class="menu-info">
              <text class="menu-label">自动同步</text>
              <text class="menu-desc">自动同步项目到云端</text>
            </view>
          </view>
          <view class="menu-right">
            <GlxSwitch
              :checked="userStore.syncEnabled"
              @change="handleSyncChange"
            />
          </view>
        </view>

        <!-- 设置 -->
        <view
          class="menu-item menu-item--settings glx-list-card"
          @click="goToSettings"
        >
          <view class="menu-left">
            <view class="menu-icon menu-icon--settings">
              <Icon name="setting" :size="40" color="var(--nb-ink)" />
            </view>
            <text class="menu-label">设置</text>
          </view>
          <view class="menu-right glx-list-meta">
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>
      </view>

      <!-- 退出登录 -->
      <view v-if="userStore.hasLogin" class="logout-section">
        <button class="logout-btn" @click="handleLogout">
          <Icon name="sign-out" :size="32" />
          <text>退出登录</text>
        </button>
      </view>
    </scroll-view>

    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useUserStore } from "../../store/user.js";
import { useProjectStore } from "../../store/project.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import GlxSwitch from "../../components/GlxSwitch.vue";
import { userAPI, artworkAPI, collectionAPI } from "../../api/index.js";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    GlxSwitch,
  },

  data() {
    // 直接初始化 store
    const userStore = useUserStore();
    const projectStore = useProjectStore();

    return {
      userStore,
      projectStore,
      toast: null,
    };
  },

  computed: {
    publishedCount() {
      if (!this.projectStore || !this.projectStore.projects) return 0;
      return this.projectStore.projects.filter((p) => p.status === "published")
        .length;
    },
  },

  onLoad() {
    this.toast = useToast();

    // 注册 Toast
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
    });
  },

  methods: {
    // 登录
    async handleLogin() {
      uni.showLoading({ title: "登录中..." });

      const res = await this.userStore.login();

      uni.hideLoading();

      if (res.success) {
        this.toast.showSuccess("登录成功");
      } else {
        this.toast.showError(res.error || "登录失败");
      }
    },

    // 编辑个人资料
    handleEditProfile() {
      uni.navigateTo({
        url: "/pages/edit-profile/edit-profile",
      });
    },

    // 跳转到我的作品
    goToMyWorks() {
      uni.navigateTo({
        url: "/pages/my-works/my-works",
      });
    },

    // 跳转到我的收藏
    goToMyFavorites() {
      uni.navigateTo({
        url: "/pages/my-favorites/my-favorites",
      });
    },

    // 跳转到成就页面
    goToAchievements() {
      uni.navigateTo({
        url: "/pages/achievements/achievements",
      });
    },

    // 跳转到粉丝页面
    goToFollowers() {
      if (!(this.userStore.currentUser && this.userStore.currentUser.id)) {
        return;
      }
      uni.navigateTo({
        url: `/pages/user-list/user-list?type=followers&userId=${this.userStore.currentUser.id}`,
      });
    },

    // 跳转到关注页面
    goToFollowing() {
      if (!(this.userStore.currentUser && this.userStore.currentUser.id)) {
        return;
      }
      uni.navigateTo({
        url: `/pages/user-list/user-list?type=following&userId=${this.userStore.currentUser.id}`,
      });
    },

    // 跳转到云同步设置
    goToCloudSync() {
      uni.navigateTo({
        url: "/pages/cloud-sync/cloud-sync",
      });
    },

    // 跳转到设置页面
    goToSettings() {
      uni.navigateTo({
        url: "/pages/settings/settings",
      });
    },

    // 切换同步状态
    handleSyncChange(e) {
      if (e.detail.value) {
        this.userStore.enableSync();
        this.toast.showSuccess("已开启云同步");
      } else {
        this.userStore.disableSync();
        this.toast.showInfo("已关闭云同步");
      }
    },

    // 退出登录
    handleLogout() {
      uni.showModal({
        title: "确认退出",
        content: "退出后将无法同步数据到云端",
        success: (res) => {
          if (res.confirm) {
            this.userStore.logout();
            this.toast.showSuccess("已退出登录");
          }
        },
      });
    },
  },
};
</script>

<style scoped>
.profile-page {
  height: 100vh;
  background-color: var(--nb-paper);
  overflow: hidden;
}

/* 页面标题 */
.page-header {
  background-color: var(--nb-surface);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
}

.page-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.content {
  flex: 1;
  overflow-y: auto;
  padding-bottom: 80rpx;
  height: 100%;
}

/* 用户卡片 */
.user-card {
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 32rpx;
  margin: 32rpx 20rpx;
  box-shadow: var(--nb-shadow-strong);
}

.login-prompt {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
}

.avatar-placeholder {
  width: 120rpx;
  height: 120rpx;
  border-radius: 60rpx;
  background-color: #edf4ff;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--nb-blue);
}

.prompt-text {
  font-size: 28rpx;
  color: var(--text-secondary);
  text-align: center;
}

.login-btn {
  min-height: 72rpx;
  padding: 0 32rpx;
  font-size: 28rpx;
  font-weight: 900;
}

.login-btn::after {
  border: none;
}

.user-info {
  display: flex;
  align-items: flex-start;
  gap: 24rpx;
  margin-bottom: 32rpx;
}

.user-avatar .avatar {
  width: 120rpx;
  height: 120rpx;
  border-radius: 60rpx;
}

.user-details {
  flex: 1;
}

.user-name-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 8rpx;
}

.nickname {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.edit-btn {
  min-height: 60rpx;
  padding: 0 24rpx;
  font-size: 24rpx;
  font-weight: 900;
  background-color: var(--nb-blue);
  color: #ffffff;
}

.edit-btn text,
.edit-btn .iconfont {
  color: inherit !important;
}

.edit-btn::after {
  border: none;
}

.user-id {
  font-size: 24rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

/* 统计数据 */
.user-stats {
  display: flex;
  justify-content: space-around;
  padding-top: 24rpx;
  border-top: 2rpx solid var(--nb-ink);
}

.stat-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  cursor: pointer;
  transition: all 0.2s ease;
}

.stat-number {
  font-size: 32rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.stat-label {
  font-size: 24rpx;
  color: var(--text-secondary);
}

/* 功能菜单 */
.menu-section {
  margin: 32rpx 20rpx;
  padding: 0;
  background-color: transparent;
}

.menu-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  transition: all 0.2s ease;
  min-height: 100rpx;
  box-shadow: var(--nb-shadow-strong);
}

.menu-item:last-child {
  margin-bottom: 0;
}

.menu-left {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
}

.menu-icon {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: #f5f6f8 !important;
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.menu-icon--works {
  background-color: #fff1b8 !important;
}

.menu-icon--favorites {
  background-color: #ffe1db !important;
}

.menu-icon--achievements {
  background-color: #e3f0ff !important;
}

.menu-icon--sync {
  background-color: #e2f8ee !important;
}

.menu-icon--settings {
  background-color: #eee8ff !important;
}

.menu-info {
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.menu-label {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--nb-ink);
}

.menu-desc {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.menu-right {
  display: flex;
  align-items: center;
}

.menu-count {
  font-size: 24rpx;
  font-weight: 900;
  color: var(--nb-ink);
  background-color: var(--nb-yellow);
  border: var(--nb-border-width) solid var(--nb-ink);
  padding: 6rpx 16rpx;
  min-width: 52rpx;
  text-align: center;
  line-height: 1.2;
}

/* 退出登录 */
.logout-section {
  margin: 32rpx 20rpx;
}

.logout-btn {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  padding: 32rpx;
  background-color: var(--nb-coral);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  color: #ffffff;
  font-size: 28rpx;
  font-weight: 600;
  transition: background-color 0.2s ease;
  box-shadow: var(--nb-shadow-soft);
}

.logout-btn::after {
  border: none;
}

.logout-btn:active {
  background-color: #ef6258;
}

.logout-btn text,
.logout-btn .iconfont {
  color: inherit !important;
}
</style>
