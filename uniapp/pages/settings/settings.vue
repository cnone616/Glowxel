<template>
  <view class="settings-page glx-page-shell">
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
      <text class="nav-title glx-topbar__title">设置</text>
      <view class="nav-right"></view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content">
      <!-- 账户设置 -->
      <view class="settings-section glx-panel-card">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">账户设置</text>
        </view>

        <view class="setting-item glx-setting-row" @click="goToEditProfile">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="user" :size="40" color="var(--nb-yellow)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">个人资料</text>
              <text class="setting-desc">编辑头像、昵称等信息</text>
            </view>
          </view>
          <view class="setting-right glx-list-meta">
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>

        <view class="setting-item glx-setting-row" @click="goToPrivacy">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon
                name="shield"
                :size="40"
                color="var(--nb-yellow)"
              />
            </view>
            <view class="setting-info">
              <text class="setting-label">隐私设置</text>
              <text class="setting-desc">管理个人信息可见性</text>
            </view>
          </view>
          <view class="setting-right glx-list-meta">
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>
      </view>

      <!-- 通知设置 -->
      <view class="settings-section glx-panel-card">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">通知设置</text>
        </view>

        <view class="setting-item switch-item glx-setting-row">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="bell" :size="40" color="var(--nb-yellow)" />
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
              color="var(--nb-yellow)"
            />
          </view>
        </view>

        <view class="setting-item switch-item glx-setting-row">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon
                name="heart"
                :size="40"
                color="var(--nb-yellow)"
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
              color="var(--nb-yellow)"
            />
          </view>
        </view>
      </view>

      <!-- 设备管理 -->
      <view class="settings-section glx-panel-card">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">设备管理</text>
        </view>

        <view class="setting-item glx-setting-row" @click="checkFirmwareUpdate">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon
                name="refresh"
                :size="40"
                color="var(--nb-yellow)"
              />
            </view>
            <view class="setting-info">
              <text class="setting-label">固件更新</text>
              <text class="setting-desc"
                >当前版本: {{ firmwareVersion || "未连接" }}</text
              >
            </view>
          </view>
          <view class="setting-right glx-list-meta">
            <view v-if="hasUpdate" class="update-badge">
              <text class="update-badge-text">新版本</text>
            </view>
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>

        <view class="setting-item glx-setting-row" @click="goToBleConfig">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="link" :size="40" color="var(--nb-yellow)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">蓝牙配网</text>
              <text class="setting-desc">配置设备 WiFi 网络</text>
            </view>
          </view>
          <view class="setting-right glx-list-meta">
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>
      </view>

      <!-- 其他设置 -->
      <view class="settings-section glx-panel-card">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">其他</text>
        </view>

        <view class="setting-item glx-setting-row" @click="goToHelp">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="help" :size="40" color="var(--nb-yellow)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">帮助与反馈</text>
              <text class="setting-desc">使用帮助和问题反馈</text>
            </view>
          </view>
          <view class="setting-right glx-list-meta">
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>

        <view class="setting-item glx-setting-row" @click="goToAbout">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="info" :size="40" color="var(--nb-yellow)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">关于应用</text>
              <text class="setting-desc">版本信息和开发团队</text>
            </view>
          </view>
          <view class="setting-right glx-list-meta">
            <text class="version-text">v{{ appVersion }}</text>
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
          </view>
        </view>

        <view class="setting-item danger glx-setting-row" @click="clearCache">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="trash" :size="40" color="var(--nb-coral)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">清除缓存</text>
              <text class="setting-desc">清理应用缓存数据</text>
            </view>
          </view>
          <view class="setting-right glx-list-meta">
            <Icon name="arrow-right" :size="32" class="glx-list-arrow" />
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
      uni.showModal({
        title: "隐私设置",
        content:
          "Glowxel 会在本地保存登录状态、项目草稿、应用设置和设备连接信息，用于创作、同步与设备控制。",
        showCancel: false,
      });
    },

    goToHelp() {
      uni.showModal({
        title: "帮助与反馈",
        content:
          "创作前请先创建或导入画布；设备功能需先完成蓝牙配网；如遇同步异常，可在云同步页面重新执行同步。",
        showCancel: false,
      });
    },

    goToAbout() {
      uni.showModal({
        title: "关于 Glowxel",
        content: `Glowxel 是面向像素创作与设备联动的创作平台。当前应用版本 v${this.appVersion}。`,
        showCancel: false,
      });
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
  background-color: var(--nb-paper);
  display: flex;
  flex-direction: column;
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--nb-surface);
  border-bottom: 2rpx solid var(--nb-ink);
  position: relative;
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
  color: #4a4a4a;
  text-transform: uppercase;
  letter-spacing: 1rpx;
}

.setting-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 32rpx;
  margin-bottom: 20rpx;
  box-shadow: none;
  transition: all 0.2s ease;
}

.setting-item:last-child {
  margin-bottom: 0;
}

.setting-item:active {
  transform: scale(0.98);
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
}

.setting-item.danger .setting-label {
  color: var(--nb-coral);
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
  background-color: transparent;
  border-radius: 0;
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
  color: var(--nb-ink);
}

.setting-desc {
  font-size: 24rpx;
  color: #4a4a4a;
}

.setting-right {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.storage-size,
.version-text {
  font-size: 24rpx;
  color: #777777;
}

.settings-section {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
  padding: 0 !important;
}

.update-badge {
  background: var(--nb-coral);
  border-radius: 0;
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
  background-color: var(--nb-surface);
  border-radius: 0;
  margin: 32rpx;
  max-width: 600rpx;
  width: 100%;
  max-height: 80vh;
  box-shadow: var(--nb-shadow-strong);
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
}

.modal-title {
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

.language-options {
  max-height: 60vh;
  overflow-y: auto;
}

.language-option {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
}

.language-option:last-child {
  border-bottom: none;
}

.language-option:active {
  background-color: var(--nb-paper);
}

.language-option.active {
  background-color: var(--nb-yellow);
}

.language-name {
  font-size: 28rpx;
  color: var(--nb-ink);
  font-weight: 500;
}

.language-native {
  font-size: 24rpx;
  color: #4a4a4a;
  margin-left: 16rpx;
}
</style>
