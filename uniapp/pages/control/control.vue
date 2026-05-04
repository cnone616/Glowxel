<template>
  <view class="control-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    <view class="header glx-topbar glx-page-shell__fixed">
      <view class="header-content">
        <view class="header-placeholder"></view>
        <text class="header-title glx-topbar__title">光格像素工坊</text>
        <view class="header-placeholder"></view>
      </view>
    </view>
    <scroll-view
      scroll-y
      class="content glx-scroll-region glx-page-shell__content"
    >
      <view class="console-card glx-panel-card">
        <view class="console-card-header">
          <view class="device-summary">
            <view class="device-logo">
              <view class="device-logo-core"></view>
            </view>
            <view class="device-summary-text">
              <text class="device-name">Glowxel PixelBoard</text>
              <view class="device-status-row">
                <view
                  class="device-status-dot"
                  :class="{ online: isDeviceConnected }"
                ></view>
                <text class="device-status-label">{{
                  isDeviceConnected ? "已连接" : "未连接"
                }}</text>
                <text
                  v-if="isDeviceConnected && deviceIp"
                  class="device-status-ip"
                  >{{ deviceIp }}</text
                >
              </view>
            </view>
          </view>
        </view>

        <view v-if="!isDeviceConnected" class="connect-entry-grid">
          <view class="connect-entry-card" @click="handleConnect">
            <view class="connect-entry-icon">
              <Icon name="scanning" :size="36" color="var(--nb-ink)" />
            </view>
            <view class="connect-entry-text">
              <text class="connect-entry-label">WiFi 连接</text>
              <text class="connect-entry-desc">输入设备 IP 地址连接</text>
            </view>
          </view>
          <view class="connect-entry-card" @click="showWifiConfigGuide">
            <view class="connect-entry-icon mint">
              <Icon name="mobile-phone" :size="36" color="var(--nb-ink)" />
            </view>
            <view class="connect-entry-text">
              <text class="connect-entry-label">热点配网</text>
              <text class="connect-entry-desc"
                >连接设备热点后在浏览器打开 192.168.4.1</text
              >
            </view>
          </view>
        </view>

        <view
          v-else
          class="device-disconnect-entry glx-secondary-action"
          @click="handleDisconnect"
        >
          <Icon name="close" :size="32" color="var(--nb-ink)" />
          <text class="device-disconnect-label">断开连接</text>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">模式入口</text>
          <text class="section-meta">{{ modeCatalog.length }} 个入口</text>
        </view>
        <view class="mode-badge-grid">
          <view
            v-for="entry in modeCatalog"
            :key="entry.key"
            class="mode-badge"
            :class="entry.variant"
            @click="handleModeSelect(entry)"
          >
            <view class="mode-badge-icon-shell">
              <view class="mode-badge-icon-core">
                <Icon :name="entry.icon" :size="42" color="currentColor" />
              </view>
            </view>
            <text class="mode-badge-name">{{ entry.name }}</text>
          </view>
        </view>
      </view>

      <view style="height: 120rpx"></view>
    </scroll-view>
    <Toast ref="toastRef" />
    <ConnectModal
      :visible="showConnectModal"
      title="连接设备"
      description="请输入 Glowxel PixelBoard 的 IP 地址"
      placeholder="输入设备 IP 地址"
      :defaultValue="deviceIp"
      ref="connectModal"
      @confirm="handleConnectConfirm"
      @cancel="handleConnectCancel"
      @timeout="handleConnectTimeout"
      @update:visible="(val) => (showConnectModal = val)"
    />
  </view>
</template>

<script>
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import ConnectModal from "../../components/ConnectModal.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    ConnectModal,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,

      deviceIp: "",
      showConnectModal: false,
      modeSwitchingKey: "",
    };
  },

  onLoad() {
    this.deviceStore = useDeviceStore();
    this.toast = useToast();
    this.deviceStore.init();

    // 从缓存读取设备 IP
    const savedIp = uni.getStorageSync("device_ip");
    if (savedIp) {
      this.deviceIp = savedIp;
    }
  },

  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
  },

  async onShow() {
    if (
      this.deviceStore &&
      typeof this.deviceStore.deviceIp === "string" &&
      this.deviceStore.deviceIp.length > 0
    ) {
      this.deviceIp = this.deviceStore.deviceIp;
    }
    if (this.deviceStore?.connected) {
      await this.deviceStore.syncDeviceStatus();
    }
  },

  computed: {
    isDeviceConnected() {
      if (!this.deviceStore) {
        return false;
      }
      return this.deviceStore.connected === true;
    },
    modeCatalog() {
      return [
        {
          key: "eyes",
          name: "桌面宠物",
          icon: "smile",
          variant: "pink",
        },
        {
          key: "clock",
          name: "静态时钟",
          icon: "time",
          variant: "cyan",
        },
        {
          key: "animation",
          name: "动态时钟",
          icon: "dynamic-filling",
          variant: "teal",
        },
        {
          key: "theme",
          name: "主题模式",
          icon: "picture",
          variant: "purple",
        },
        {
          key: "tetris",
          name: "俄罗斯方块屏保",
          icon: "modular",
          variant: "indigo",
        },
        {
          key: "tetris_clock",
          name: "俄罗斯方块时钟",
          icon: "clock-filling",
          variant: "gold",
        },
        {
          key: "maze",
          name: "迷宫漫游",
          icon: "map",
          variant: "indigo",
        },
        {
          key: "snake",
          name: "贪吃蛇",
          icon: "move",
          variant: "indigo",
        },
        {
          key: "led_matrix_showcase",
          name: "矩阵流光",
          icon: "layers",
          variant: "azure",
        },
        {
          key: "planet_screensaver",
          name: "星球屏保",
          icon: "navigation",
          variant: "indigo",
        },
      ];
    },
  },
  methods: {
    async handleConnect() {
      this.showConnectModal = true;
    },

    async handleDisconnect() {
      await this.deviceStore.disconnect();
      this.toast.showInfo("设备已断开");
    },

    async handleConnectConfirm(ip) {
      this.deviceIp = ip;

      try {
        const result = await this.deviceStore.connect(ip);
        if (result.success) {
          this.$refs.connectModal.onSuccess();
          this.toast.showSuccess("已连接到 Glowxel PixelBoard");
        } else {
          let errorMessage = "连接失败，请检查 IP 地址";
          if (result.error && result.error.errMsg) {
            errorMessage = result.error.errMsg;
          } else if (result.error && result.error.message) {
            errorMessage = result.error.message;
          }
          this.$refs.connectModal.onError(errorMessage);
        }
      } catch (err) {
        console.error("连接失败:", err);
        let errorMessage = "连接失败，请检查 IP 地址";
        if (err && err.errMsg) {
          errorMessage = err.errMsg;
        } else if (err && err.message) {
          errorMessage = err.message;
        }
        this.$refs.connectModal.onError(errorMessage);
      }
    },

    handleConnectCancel() {
      // 取消连接
    },

    handleConnectTimeout() {
      this.toast.showError("连接超时，请重试");
    },

    async handleModeSelect(entry) {
      if (!this.isDeviceConnected) {
        this.toast.showInfo("请先连接设备");
        return;
      }

      if (this.modeSwitchingKey.length > 0) {
        return;
      }

      this.modeSwitchingKey = entry.key;

      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.setMode(entry.key);
        await this.deviceStore.syncDeviceStatus();
        this.toast.showSuccess(`已切换到${entry.name}`);
      } catch (error) {
        console.error(`切换模式失败: ${entry.key}`, error);
        if (error && typeof error.message === "string" && error.message.length > 0) {
          this.toast.showError(error.message);
        } else {
          this.toast.showError(`切换${entry.name}失败`);
        }
      } finally {
        this.modeSwitchingKey = "";
      }
    },

    showWifiConfigGuide() {
      this.toast.showInfo("连接设备热点后，在浏览器打开 192.168.4.1 完成配网");
    },
  },
};
</script>

<style scoped>
.control-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--nb-paper);
  overflow: hidden;
}

.header {
  background: var(--nb-surface);
  border-bottom: 2rpx solid var(--nb-ink);
  padding: 0;
}

.header-content {
  height: 96rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.header-title {
  font-size: 34rpx;
  font-weight: 900;
  color: var(--nb-ink);
}

.content {
  flex: 1;
  padding: 16rpx 20rpx 0;
  box-sizing: border-box;
  overflow-y: scroll;
}

.console-card {
  margin-bottom: 20rpx;
  padding: 24rpx;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid #000000;
  box-shadow: none;
}

.console-card-header {
  display: flex;
  align-items: center;
  margin-bottom: 24rpx;
}

.device-summary {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 20rpx;
}

.device-logo {
  width: 92rpx;
  height: 92rpx;
  border-radius: 0;
  background: var(--nb-surface);
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  display: flex;
  align-items: center;
  justify-content: center;
}

.device-logo-core {
  width: 42rpx;
  height: 42rpx;
  border-radius: 0;
  background: var(--nb-blue);
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.device-summary-text {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.device-name {
  font-size: 32rpx;
  font-weight: 600;
  color: #111827;
}

.device-status-row {
  display: flex;
  align-items: center;
  gap: 10rpx;
  flex-wrap: wrap;
}

.device-status-dot {
  width: 14rpx;
  height: 14rpx;
  border-radius: 999rpx;
  background: var(--text-tertiary);
}

.device-status-dot.online {
  background: var(--nb-green);
  box-shadow: none;
}

.device-status-label,
.device-status-ip {
  font-size: 22rpx;
  color: var(--text-tertiary);
}

.device-status-ip {
  font-family: monospace;
  color: var(--nb-ink);
}

.connect-entry-grid {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.connect-entry-card {
  border-radius: 0;
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  padding: 16rpx 18rpx;
  display: flex;
  align-items: center;
  gap: 20rpx;
}

.device-disconnect-entry {
  margin-top: 16rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  min-height: 84rpx;
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
}

.device-disconnect-label {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--nb-ink);
}

.connect-entry-icon {
  width: 72rpx;
  height: 72rpx;
  border-radius: 0;
  background: var(--nb-surface);
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.connect-entry-icon.mint {
  background: var(--nb-green);
}

.connect-entry-text {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 6rpx;
}

.connect-entry-label {
  font-size: 28rpx;
  font-weight: 600;
}

.connect-entry-label {
  color: #111827;
}

.connect-entry-desc,
.section-meta {
  font-size: 22rpx;
  color: var(--text-tertiary);
}

.section-block {
  margin-bottom: 24rpx;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 4rpx 4rpx 14rpx;
}

.section-title {
  font-size: 28rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.mode-badge-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 18rpx;
}

.mode-badge {
  border-radius: 0;
  padding: 12rpx 6rpx 10rpx;
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
}

.mode-badge-icon-shell {
  width: 96rpx;
  height: 96rpx;
  padding: 10rpx;
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.mode-badge-icon-core {
  width: 100%;
  height: 100%;
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: none;
  border: 0;
  color: var(--nb-ink);
}

.mode-badge-name {
  font-size: 21rpx;
  font-weight: 600;
  color: var(--nb-ink);
  text-align: center;
  line-height: 1.25;
  white-space: nowrap;
}

.mode-badge.pink {
  background: var(--nb-surface);
}

.mode-badge.pink .mode-badge-icon-core {
  background: #f472b6;
}

.mode-badge.cyan {
  background: var(--nb-surface);
}

.mode-badge.cyan .mode-badge-icon-core {
  background: #00c9d8;
}

.mode-badge.teal {
  background: var(--nb-surface);
}

.mode-badge.teal .mode-badge-icon-core {
  background: #14b8a6;
}

.mode-badge.purple {
  background: var(--nb-surface);
}

.mode-badge.purple .mode-badge-icon-core {
  background: #8b5cf6;
}

.mode-badge.indigo {
  background: var(--nb-surface);
}

.mode-badge.indigo .mode-badge-icon-core {
  background: #6f68ff;
}

.mode-badge.gold {
  background: var(--nb-surface);
}

.mode-badge.gold .mode-badge-icon-core {
  background: var(--nb-yellow);
}

.mode-badge.azure {
  background: var(--nb-surface);
}

.mode-badge.azure .mode-badge-icon-core {
  background: #5a90ff;
}

@media (max-width: 720px) {
  .mode-badge-grid {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }
}
</style>
