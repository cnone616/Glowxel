<template>
  <view class="settings-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <text class="nav-title">设置</text>
      <view class="nav-right"></view>
    </view>

    <scroll-view scroll-y class="content">
      <!-- 账户设置 -->
      <view class="settings-section">
        <view class="section-header">
          <text class="section-title">账户设置</text>
        </view>

        <view class="setting-item" @click="goToEditProfile">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="user" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">个人资料</text>
              <text class="setting-desc">编辑头像、昵称等信息</text>
            </view>
          </view>
          <view class="setting-right">
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>

        <view class="setting-item" @click="goToPrivacy">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon
                name="shield"
                :size="40"
                color="var(--color-brand-primary)"
              />
            </view>
            <view class="setting-info">
              <text class="setting-label">隐私设置</text>
              <text class="setting-desc">管理个人信息可见性</text>
            </view>
          </view>
          <view class="setting-right">
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>
      </view>

      <!-- 通知设置 -->
      <view class="settings-section">
        <view class="section-header">
          <text class="section-title">通知设置</text>
        </view>

        <view class="setting-item switch-item">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="bell" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">推送通知</text>
              <text class="setting-desc">接收应用推送消息</text>
            </view>
          </view>
          <view class="setting-right">
            <switch
              :checked="settings.pushNotification"
              @change="togglePushNotification"
              color="var(--color-brand-primary)"
            />
          </view>
        </view>

        <view class="setting-item switch-item">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon
                name="heart"
                :size="40"
                color="var(--color-brand-primary)"
              />
            </view>
            <view class="setting-info">
              <text class="setting-label">互动通知</text>
              <text class="setting-desc">点赞、评论、关注通知</text>
            </view>
          </view>
          <view class="setting-right">
            <switch
              :checked="settings.interactionNotification"
              @change="toggleInteractionNotification"
              color="var(--color-brand-primary)"
            />
          </view>
        </view>
      </view>

      <!-- 设备管理 -->
      <view class="settings-section">
        <view class="section-header">
          <text class="section-title">设备管理</text>
        </view>

        <view class="setting-item" @click="checkFirmwareUpdate">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon
                name="refresh"
                :size="40"
                color="var(--color-brand-primary)"
              />
            </view>
            <view class="setting-info">
              <text class="setting-label">固件更新</text>
              <text class="setting-desc"
                >当前版本: {{ firmwareVersion || "未连接" }}</text
              >
            </view>
          </view>
          <view class="setting-right">
            <view v-if="hasUpdate" class="update-badge">
              <text class="update-badge-text">新版本</text>
            </view>
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>

        <view class="setting-item" @click="goToBleConfig">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="link" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">蓝牙配网</text>
              <text class="setting-desc">配置设备 WiFi 网络</text>
            </view>
          </view>
          <view class="setting-right">
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>
      </view>

      <!-- 其他设置 -->
      <view class="settings-section">
        <view class="section-header">
          <text class="section-title">其他</text>
        </view>

        <view class="setting-item" @click="goToHelp">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="help" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">帮助与反馈</text>
              <text class="setting-desc">使用帮助和问题反馈</text>
            </view>
          </view>
          <view class="setting-right">
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>

        <view class="setting-item" @click="goToAbout">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="info" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">关于应用</text>
              <text class="setting-desc">版本信息和开发团队</text>
            </view>
          </view>
          <view class="setting-right">
            <text class="version-text">v{{ appVersion }}</text>
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>

        <view class="setting-item danger" @click="clearCache">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="trash" :size="40" color="var(--color-error)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">清除缓存</text>
              <text class="setting-desc">清理应用缓存数据</text>
            </view>
          </view>
          <view class="setting-right">
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>
      </view>
    </scroll-view>

    <!-- 语言选择弹窗 -->
    <view
      v-if="showLanguageModal"
      class="modal-overlay"
      @click="showLanguageModal = false"
    >
      <view class="language-modal" @click.stop>
        <view class="modal-header">
          <text class="modal-title">选择语言</text>
          <view class="modal-close" @click="showLanguageModal = false">
            <Icon name="close" :size="32" />
          </view>
        </view>

        <view class="language-options">
          <view
            v-for="lang in languages"
            :key="lang.code"
            class="language-option"
            :class="{ active: settings.language === lang.code }"
            @click="selectLanguage(lang.code)"
          >
            <text class="language-name">{{ lang.name }}</text>
            <text class="language-native">{{ lang.native }}</text>
            <Icon
              v-if="settings.language === lang.code"
              name="check"
              :size="24"
            />
          </view>
        </view>
      </view>
    </view>

    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
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
      showLanguageModal: false,
      appVersion: "1.0.0",
      firmwareVersion: "",
      hasUpdate: false,
      latestFirmware: null,
      settings: {
        darkMode: false,
        hapticFeedback: true,
        language: "zh-CN",
        autoBackup: true,
        wifiOnlySync: true,
        pushNotification: true,
        interactionNotification: true,
      },
      storageInfo: {
        used: "12.5MB",
        total: "64GB",
        projects: "8.2MB",
        cache: "4.3MB",
      },
      languages: [
        { code: "zh-CN", name: "简体中文", native: "简体中文" },
        { code: "zh-TW", name: "繁體中文", native: "繁體中文" },
        { code: "en-US", name: "English", native: "English" },
        { code: "ja-JP", name: "日本語", native: "日本語" },
        { code: "ko-KR", name: "한국어", native: "한국어" },
      ],
    };
  },

  onLoad() {
    this.toast = useToast();
    this.loadSettings();

    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
    });
  },

  methods: {
    handleBack() {
      uni.navigateBack();
    },

    loadSettings() {
      // 从本地存储加载设置
      const savedSettings = uni.getStorageSync("app-settings");
      if (savedSettings) {
        this.settings = { ...this.settings, ...savedSettings };
      }
    },

    saveSettings() {
      // 保存设置到本地存储
      uni.setStorageSync("app-settings", this.settings);
    },

    goToEditProfile() {
      uni.navigateTo({
        url: "/pages/edit-profile/edit-profile",
      });
    },

    goToPrivacy() {
      this.toast.showInfo("隐私设置功能开发中");
    },

    goToHelp() {
      this.toast.showInfo("帮助页面功能开发中");
    },

    goToAbout() {
      this.toast.showInfo("关于页面功能开发中");
    },

    goToBleConfig() {
      uni.navigateTo({ url: "/pages/ble-config/ble-config" });
    },

    checkFirmwareUpdate() {
      const deviceStore = this.getDeviceStore();
      if (!deviceStore || !deviceStore.connected) {
        this.toast.showInfo("请先连接设备");
        return;
      }

      uni.showLoading({ title: "检查更新中..." });
      const ws = deviceStore.getWebSocket();

      // 发送检查更新命令
      ws.send({ cmd: "ota_check" })
        .then((res) => {
          uni.hideLoading();
          this.firmwareVersion = res.firmware_version || "";
          this.hasUpdate = res.has_update || false;

          if (this.hasUpdate) {
            this.latestFirmware = {
              version: res.latest_version,
              changelog: res.changelog,
              isForce: res.is_force,
            };
            this.showUpdateDialog();
          } else {
            this.toast.showSuccess("已是最新版本 v" + this.firmwareVersion);
          }
        })
        .catch(() => {
          uni.hideLoading();
          this.toast.showError("检查更新失败");
        });
    },

    showUpdateDialog() {
      if (!this.latestFirmware) return;
      uni.showModal({
        title: "发现新版本 v" + this.latestFirmware.version,
        content: this.latestFirmware.changelog || "修复已知问题，提升稳定性",
        confirmText: "立即更新",
        cancelText: this.latestFirmware.isForce ? "" : "稍后",
        showCancel: !this.latestFirmware.isForce,
        success: (res) => {
          if (res.confirm) {
            this.startFirmwareUpdate();
          }
        },
      });
    },

    startFirmwareUpdate() {
      const deviceStore = this.getDeviceStore();
      if (!deviceStore || !deviceStore.connected) return;

      uni.showLoading({ title: "正在更新固件...", mask: true });
      const ws = deviceStore.getWebSocket();

      ws.send({ cmd: "ota_update" })
        .then(() => {
          // 设备会重启，连接会断开
          setTimeout(() => {
            uni.hideLoading();
            uni.showModal({
              title: "更新中",
              content: "设备正在更新固件并重启，请稍等约30秒后重新连接设备",
              showCancel: false,
            });
          }, 3000);
        })
        .catch(() => {
          uni.hideLoading();
          this.toast.showError("更新失败");
        });
    },

    getDeviceStore() {
      try {
        const { useDeviceStore } = require("../../store/device");
        return useDeviceStore();
      } catch (e) {
        return null;
      }
    },

    toggleDarkMode(e) {
      this.settings.darkMode = e.detail.value;
      this.saveSettings();
      this.toast.showInfo(
        this.settings.darkMode ? "已切换到深色模式" : "已切换到浅色模式",
      );
    },

    toggleHapticFeedback(e) {
      this.settings.hapticFeedback = e.detail.value;
      this.saveSettings();

      if (this.settings.hapticFeedback) {
        uni.vibrateShort();
        this.toast.showSuccess("触觉反馈已开启");
      } else {
        this.toast.showInfo("触觉反馈已关闭");
      }
    },

    toggleAutoBackup(e) {
      this.settings.autoBackup = e.detail.value;
      this.saveSettings();
      this.toast.showInfo(
        this.settings.autoBackup ? "自动备份已开启" : "自动备份已关闭",
      );
    },

    toggleWifiOnlySync(e) {
      this.settings.wifiOnlySync = e.detail.value;
      this.saveSettings();
      this.toast.showInfo(
        this.settings.wifiOnlySync ? "仅WiFi同步已开启" : "移动网络同步已允许",
      );
    },

    togglePushNotification(e) {
      this.settings.pushNotification = e.detail.value;
      this.saveSettings();
      this.toast.showInfo(
        this.settings.pushNotification ? "推送通知已开启" : "推送通知已关闭",
      );
    },

    toggleInteractionNotification(e) {
      this.settings.interactionNotification = e.detail.value;
      this.saveSettings();
      this.toast.showInfo(
        this.settings.interactionNotification
          ? "互动通知已开启"
          : "互动通知已关闭",
      );
    },

    getCurrentLanguageLabel() {
      const lang = this.languages.find(
        (l) => l.code === this.settings.language,
      );
      return lang ? lang.native : "简体中文";
    },

    selectLanguage(code) {
      this.settings.language = code;
      this.saveSettings();
      this.showLanguageModal = false;
      this.toast.showSuccess("语言设置已更新");
    },

    showStorageInfo() {
      uni.showModal({
        title: "存储空间",
        content: `已使用: ${this.storageInfo.used}\n作品数据: ${this.storageInfo.projects}\n缓存数据: ${this.storageInfo.cache}`,
        showCancel: false,
      });
    },

    clearCache() {
      uni.showModal({
        title: "清除缓存",
        content: "确定要清除应用缓存吗？这不会删除您的作品数据。",
        success: (res) => {
          if (res.confirm) {
            // 清除缓存逻辑
            this.toast.showSuccess("缓存已清除");
            this.storageInfo.cache = "0MB";
            this.storageInfo.used = this.storageInfo.projects;
          }
        },
      });
    },
  },
};
</script>

<style scoped>
.settings-page {
  height: 100vh;
  background-color: var(--color-app-background);
  display: flex;
  flex-direction: column;
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--color-card-background);
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.nav-right {
  position: absolute;
  right: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-end;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.content {
  flex: 1;
  padding: 32rpx 32rpx 80rpx;
  overflow-y: scroll;
  box-sizing: border-box;
}

.settings-section {
  margin-bottom: 48rpx;
}

.section-header {
  margin-bottom: 24rpx;
}

.section-title {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--color-text-secondary);
  text-transform: uppercase;
  letter-spacing: 1rpx;
}

.setting-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 32rpx;
  margin-bottom: 16rpx;
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.setting-item:last-child {
  margin-bottom: 0;
}

.setting-item:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.setting-item.danger .setting-label {
  color: var(--color-error);
}

.setting-left {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
}

.setting-icon {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: rgba(79, 127, 255, 0.1);
  border-radius: 16rpx;
}

.setting-info {
  display: flex;
  flex-direction: column;
  gap: 4rpx;
  flex: 1;
}

.setting-label {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--color-text-primary);
}

.setting-desc {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.setting-right {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.storage-size,
.version-text {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.update-badge {
  background: var(--color-error);
  border-radius: 16rpx;
  padding: 4rpx 14rpx;
  margin-right: 8rpx;
}
.update-badge-text {
  font-size: 20rpx;
  color: #fff;
}

.switch-item .setting-right {
  gap: 0;
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

.language-modal {
  background-color: var(--color-card-background);
  border-radius: 24rpx;
  margin: 32rpx;
  max-width: 600rpx;
  width: 100%;
  max-height: 80vh;
  box-shadow: var(--shadow-modal);
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.modal-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.modal-close {
  padding: 8rpx;
}

.language-options {
  max-height: 60vh;
  overflow-y: auto;
}

.language-option {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.language-option:last-child {
  border-bottom: none;
}

.language-option:active {
  background-color: var(--color-app-background);
}

.language-option.active {
  background-color: rgba(79, 127, 255, 0.1);
}

.language-name {
  font-size: 28rpx;
  color: var(--color-text-primary);
  font-weight: 500;
}

.language-native {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  margin-left: 16rpx;
}
</style>
