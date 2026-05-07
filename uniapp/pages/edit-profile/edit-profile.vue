<template>
  <view class="edit-profile-page glx-page-shell">
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
      <text class="nav-title glx-topbar__title">编辑资料</text>
      <view class="nav-right"></view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content">
      <!-- 头像编辑 -->
      <view class="avatar-section">
        <view class="avatar-container" @click="chooseAvatar">
          <image
            v-if="form.avatar"
            :src="form.avatar"
            class="avatar-image"
            mode="aspectFill"
          />
          <view v-else class="avatar-placeholder">
            <Icon name="user" :size="80" color="var(--text-tertiary)" />
          </view>
        </view>
        <text class="avatar-tip">点击更换头像</text>
      </view>

      <!-- 基本信息 -->
      <view class="form-section glx-panel-card">
        <view class="section-title glx-section-title">基本信息</view>

        <view class="form-item">
          <text class="form-label">昵称</text>
          <input
            v-model="form.nickname"
            class="form-input"
            placeholder="请输入昵称"
            maxlength="20"
          />
          <text class="char-count">{{ form.nickname.length }}/20</text>
        </view>

        <view class="form-item">
          <text class="form-label">个人简介</text>
          <textarea
            v-model="form.bio"
            class="form-textarea"
            placeholder="介绍一下自己吧..."
            maxlength="100"
            :auto-height="true"
          />
          <text class="char-count">{{ form.bio.length }}/100</text>
        </view>

        <view class="form-item">
          <text class="form-label">性别</text>
          <view class="gender-options">
            <view
              v-for="option in genderOptions"
              :key="option.value"
              class="gender-option"
              :class="{ active: form.gender === option.value }"
              @click="form.gender = option.value"
            >
              <Icon :name="option.icon" :size="32" />
              <text class="gender-text">{{ option.label }}</text>
            </view>
          </view>
        </view>
      </view>

      <!-- 兴趣标签 -->
      <view class="form-section glx-panel-card">
        <view class="section-title glx-section-title">兴趣标签</view>
        <view class="tags-container">
          <view
            v-for="tag in availableTags"
            :key="tag"
            class="tag-item"
            :class="{ active: form.tags.includes(tag) }"
            @click="toggleTag(tag)"
          >
            <text class="tag-text">{{ tag }}</text>
          </view>
        </view>
        <text class="tags-tip">最多选择5个标签</text>
      </view>

      <!-- 隐私设置 -->
      <view class="form-section glx-panel-card">
        <view class="section-title glx-section-title">隐私设置</view>

        <view class="form-item switch-item">
          <view class="switch-info">
            <text class="form-label">公开作品</text>
            <text class="form-desc">允许其他用户查看我的作品</text>
          </view>
          <GlxSwitch
            :checked="form.publicWorks"
            @change="form.publicWorks = $event.detail.value"
          />
        </view>

        <view class="form-item switch-item">
          <view class="switch-info">
            <text class="form-label">接收关注通知</text>
            <text class="form-desc">有新粉丝时通知我</text>
          </view>
          <GlxSwitch
            :checked="form.followNotify"
            @change="form.followNotify = $event.detail.value"
          />
        </view>
      </view>
    </scroll-view>

    <!-- 底部保存按钮 -->
    <view class="footer-actions">
      <view class="save-button" @click="handleSave">
        <text class="save-button-text">保存</text>
      </view>
    </view>

    <LoadingOverlay />
    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useUserStore } from "../../store/user.js";
import { useToast } from "../../composables/useToast.js";
import { getUploadUrl } from "../../config/api.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import LoadingOverlay from "../../components/LoadingOverlay.vue";
import Toast from "../../components/Toast.vue";
import GlxSwitch from "../../components/GlxSwitch.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    LoadingOverlay,
    Toast,
    GlxSwitch,
  },

  data() {
    return {
      userStore: useUserStore(),
      toast: null,
      form: {
        avatar: "",
        nickname: "",
        bio: "",
        gender: "unknown",
        tags: [],
        publicWorks: true,
        followNotify: true,
      },
      genderOptions: [
        { value: "male", label: "男", icon: "user" },
        { value: "female", label: "女", icon: "user" },
        { value: "unknown", label: "保密", icon: "user" },
      ],
      availableTags: [
        "像素艺术",
        "游戏",
        "动漫",
        "设计",
        "创意",
        "手工",
        "拼豆",
        "艺术",
        "摄影",
        "音乐",
        "编程",
        "科技",
        "旅行",
        "美食",
        "运动",
      ],
    };
  },

  onLoad() {
    this.toast = useToast();
    this.loadUserData();

    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
    });
  },

  methods: {
    loadUserData() {
      // 加载用户数据
      this.form = {
        avatar: this.userStore.avatar || "",
        nickname: this.userStore.nickname || "",
        bio: this.userStore.bio || "",
        gender: this.userStore.gender || "unknown",
        tags: this.userStore.tags || [],
        publicWorks: this.userStore.publicWorks !== false,
        followNotify: this.userStore.followNotify !== false,
      };
    },

    handleBack() {
      uni.navigateBack();
    },

    chooseAvatar() {
      uni.chooseImage({
        count: 1,
        sizeType: ["compressed"],
        sourceType: ["album", "camera"],
        success: (res) => {
          const tempPath = res.tempFilePaths[0];
          this.toast.showLoading("上传中...");
          uni.uploadFile({
            url: getUploadUrl("avatar"),
            filePath: tempPath,
            name: "file",
            header: {
              Authorization: `Bearer ${uni.getStorageSync("auth_token")}`,
            },
            success: (uploadRes) => {
              try {
                const data = JSON.parse(uploadRes.data);
                if (data.code === 0) {
                  this.form.avatar = data.data.url;
                } else {
                  this.toast.showError(data.message || "上传失败");
                }
              } catch (e) {
                this.toast.showError("上传失败");
              }
            },
            fail: () => this.toast.showError("上传失败，请重试"),
            complete: () => this.toast.hideLoading(),
          });
        },
      });
    },

    toggleTag(tag) {
      const index = this.form.tags.indexOf(tag);
      if (index > -1) {
        this.form.tags.splice(index, 1);
      } else if (this.form.tags.length < 5) {
        this.form.tags.push(tag);
      } else {
        this.toast.showError("最多只能选择5个标签");
      }
    },

    async handleSave() {
      if (!this.form.nickname.trim()) {
        this.toast.showError("请输入昵称");
        return;
      }

      this.toast.showLoading("保存中...");

      try {
        // 更新用户信息
        await this.userStore.updateProfile(this.form);

        this.toast.hideLoading();
        this.toast.showSuccess("保存成功");

        setTimeout(() => {
          uni.navigateBack();
        }, 1000);
      } catch (error) {
        this.toast.hideLoading();
        this.toast.showError("保存失败，请重试");
      }
    },
  },
};
</script>

<style scoped>
.edit-profile-page {
  height: 100vh;
  background-color: var(--nb-paper);
  display: flex;
  flex-direction: column;
}

.save-btn {
  font-size: 28rpx;
  color: var(--nb-yellow);
  font-weight: 600;
}

.content {
  flex: 1;
  padding: 32rpx 32rpx 210rpx;
  overflow-y: scroll;
  box-sizing: border-box;
}

.avatar-section {
  display: flex;
  flex-direction: column;
  align-items: center;
  margin-bottom: 48rpx;
}

.avatar-container {
  position: relative;
  width: 160rpx;
  height: 160rpx;
  border-radius: 80rpx;
  overflow: hidden;
  margin-bottom: 16rpx;
  border-radius: 50%;
  border: 1px solid #4f7fff;
}

.avatar-image {
  width: 100%;
  height: 100%;
}

.avatar-placeholder {
  width: 100%;
  height: 100%;
  background-color: var(--nb-paper);
  display: flex;
  align-items: center;
  justify-content: center;
}

.avatar-overlay {
  position: absolute;
  bottom: 0;
  right: 0;
  width: 48rpx;
  height: 48rpx;
  background-color: var(--nb-yellow);
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 4rpx solid var(--nb-surface);
}

.avatar-tip {
  font-size: 24rpx;
  color: #4a4a4a;
}

.form-section {
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 32rpx;
  margin-bottom: 32rpx;
  box-shadow: var(--nb-shadow-strong);
}

.section-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--nb-ink);
  margin-bottom: 24rpx;
}

.form-item {
  margin-bottom: 32rpx;
  position: relative;
}

.form-item:last-child {
  margin-bottom: 0;
}

.form-input {
  width: 100%;
  padding: 24rpx;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  font-size: 28rpx;
  color: var(--nb-ink);
  transition: all 0.2s ease;
}

.form-input:focus {
  border-color: var(--nb-yellow);
  box-shadow: none;
}

.form-textarea {
  width: 100%;
  min-height: 120rpx;
  padding: 24rpx;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  font-size: 28rpx;
  color: var(--nb-ink);
  resize: none;
  transition: all 0.2s ease;
}

.form-textarea:focus {
  border-color: var(--nb-yellow);
  box-shadow: none;
}

.char-count {
  position: absolute;
  right: 16rpx;
  bottom: 16rpx;
  font-size: 20rpx;
  color: #777777;
}

.gender-options {
  display: flex;
  gap: 16rpx;
}

.gender-option {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 24rpx 16rpx;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: all 0.2s ease;
}

.gender-option.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
  color: #000000;
}

.gender-text {
  font-size: 24rpx;
  color: inherit;
}

.tags-container {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
  margin-bottom: 16rpx;
}

.tag-item {
  padding: 16rpx 24rpx;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: all 0.2s ease;
}

.tag-item.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.tag-text {
  font-size: 24rpx;
  color: #4a4a4a;
}

.tag-item.active .tag-text {
  color: #000000;
  font-weight: 700;
}

.tags-tip {
  font-size: 22rpx;
  color: #777777;
}

.switch-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.switch-info {
  flex: 1;
}

.form-desc {
  font-size: 24rpx;
  color: #4a4a4a;
  margin-top: 4rpx;
}

/* 底部操作栏 */
.footer-actions {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  padding: 24rpx 32rpx;
  padding-bottom: calc(24rpx + var(--layout-bottom-offset));
  background-color: var(--nb-surface);
  border-top: 2rpx solid var(--nb-ink);
  box-shadow: none;
  z-index: 100;
}

.save-button {
  width: 100%;
  height: 88rpx;
  background: var(--nb-yellow);
  border-radius: 0;
  border: 3rpx solid #000000;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 2rpx 2rpx 0 #000000;
  transition: all 0.2s ease;
}

.save-button:active {
  box-shadow: none;
}

.save-button-text {
  font-size: 32rpx;
  font-weight: 600;
  color: #000000;
}
</style>
