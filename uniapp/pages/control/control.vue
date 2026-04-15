<template>
  <view class="control-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    <view class="header glx-topbar glx-page-shell__fixed">
      <view class="header-content">
        <view class="header-placeholder"></view>
        <text class="header-title glx-topbar__title">设备控制</text>
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
              <text class="device-name">Glowxel Pro</text>
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
          <view class="connect-entry-card" @click="goToWifiConfig">
            <view class="connect-entry-icon mint">
              <Icon name="mobile-phone" :size="36" color="var(--nb-ink)" />
            </view>
            <view class="connect-entry-text">
              <text class="connect-entry-label">热点配网</text>
              <text class="connect-entry-desc"
                >首次使用请连接设备热点完成配网</text
              >
            </view>
          </view>
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
                <view
                  v-if="entry.iconType === 'canvas'"
                  class="mode-pixel-icon"
                >
                  <view
                    v-for="index in 9"
                    :key="index"
                    class="mode-pixel-cell"
                  ></view>
                </view>
                <Icon
                  v-else
                  :name="entry.icon"
                  :size="42"
                  color="currentColor"
                />
              </view>
            </view>
            <text class="mode-badge-name">{{ entry.name }}</text>
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">设备参数</text>
        </view>
        <view class="panel-card glx-panel-card">
          <view class="panel-action" @click="openDeviceParams">
            <view class="panel-action-icon">
              <Icon name="setting" :size="32" color="var(--nb-ink)" />
            </view>
            <view class="panel-action-text">
              <text class="panel-action-label">设备参数</text>
              <text class="panel-action-desc"
                >调整亮度、方向、颜色、重置网络</text
              >
            </view>
            <Icon name="direction-right" :size="28" color="var(--nb-ink)" />
          </view>
        </view>
      </view>

      <view style="height: 120rpx"></view>
    </scroll-view>
    <Toast ref="toastRef" />
    <ConnectModal
      :visible="showConnectModal"
      title="连接设备"
      description="请输入 RenLight 设备的 IP 地址"
      :placeholder="deviceIp || '192.168.31.84'"
      :defaultValue="deviceIp"
      ref="connectModal"
      @confirm="handleConnectConfirm"
      @cancel="handleConnectCancel"
      @timeout="handleConnectTimeout"
      @update:visible="(val) => (showConnectModal = val)"
    />
    <JsonImportModal
      :visible="showJsonImportModal"
      :sending="jsonImportSending"
      title="导入 JSON 配置"
      description="从 LED 模拟器复制的完整配置数据"
      placeholder="粘贴 JSON 数据..."
      @confirm="handleJsonImport"
      @error="handleJsonImportError"
      @update:visible="(val) => (showJsonImportModal = val)"
      @cancel="showJsonImportModal = false"
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
import JsonImportModal from "../../components/JsonImportModal.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    ConnectModal,
    JsonImportModal,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,

      deviceIp: "",

      // 设备模式
      deviceMode: "clock", // clock, animation, canvas

      // 连接弹窗
      showConnectModal: false,

      // JSON 导入弹窗
      showJsonImportModal: false,
      jsonImportSending: false,
    };
  },

  onLoad(options) {
    this.deviceStore = useDeviceStore();
    this.toast = useToast();
    this.deviceStore.init();

    // 从缓存读取设备 IP
    const savedIp = uni.getStorageSync("device_ip");
    if (savedIp) {
      this.deviceIp = savedIp;
    }

    // 从缓存读取设备模式
    const savedMode = uni.getStorageSync("device_mode");
    if (savedMode) {
      this.deviceMode = savedMode;
      this.deviceStore.setDeviceMode(savedMode, {
        businessMode: false,
      });
    }

    // 从 URL 参数读取模式（优先级更高）
    if (options && options.mode) {
      this.deviceMode = options.mode;
      this.deviceStore.setDeviceMode(options.mode, {
        businessMode: false,
      });
    }
  },

  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
  },

  async onShow() {
    this.deviceIp = this.deviceStore?.deviceIp || this.deviceIp;
    this.deviceMode = this.deviceStore?.deviceMode || this.deviceMode;
    const status = this.deviceStore?.deviceInfo;
    if (status && typeof status.mode === "string") {
      this.deviceMode =
        this.deviceStore.resolveDeviceModeFromStatus(status) ||
        this.deviceStore.deviceMode ||
        this.deviceMode;
    }
  },

  computed: {
    isDeviceConnected() {
      return this.deviceStore?.connected || false;
    },
    modeCatalog() {
      return [
        {
          key: "eyes",
          name: "桌面宠物",
          icon: "browse",
          variant: "pink",
          type: "mode",
          bucket: "stable",
        },
        {
          key: "clock",
          name: "静态时钟",
          icon: "time",
          variant: "cyan",
          type: "mode",
          bucket: "stable",
        },
        {
          key: "animation",
          name: "动态时钟",
          icon: "play",
          variant: "teal",
          type: "mode",
          bucket: "stable",
        },
        {
          key: "theme",
          name: "主题模式",
          icon: "picture",
          variant: "purple",
          type: "mode",
          bucket: "stable",
        },
        {
          key: "canvas",
          name: "画板模式",
          icon: "edit",
          variant: "blue",
          type: "mode",
          bucket: "stable",
        },
        {
          key: "tetris",
          name: "俄罗斯方块屏保",
          icon: "modular",
          variant: "indigo",
          type: "mode",
          bucket: "secondary",
        },
        {
          key: "game_screensaver",
          name: "游戏屏保",
          icon: "pad",
          variant: "indigo",
          type: "mode",
          bucket: "secondary",
        },
        {
          key: "led_matrix_showcase",
          name: "像素场景集",
          icon: "layers",
          variant: "indigo",
          type: "mode",
          bucket: "secondary",
        },
        {
          key: "gif_player",
          name: "GIF播放器",
          icon: "film",
          variant: "teal",
          type: "mode",
          bucket: "migration",
        },
        {
          key: "countdown",
          name: "沙漏倒计时",
          icon: "history",
          variant: "red",
          type: "mode",
          bucket: "migration",
        },
      ];
    },
  },
  methods: {
    isModeActive(entry) {
      return this.deviceMode === entry.key;
    },

    handleModeSelect(entry) {
      this.switchMode(entry.key);
    },

    async handleConnect() {
      if (this.isDeviceConnected) {
        // 已连接，断开
        await this.deviceStore.disconnect();
        this.toast.showInfo("设备已断开");
        return;
      }

      // 显示连接弹窗
      this.showConnectModal = true;
    },

    async handleConnectConfirm(ip) {
      this.deviceIp = ip;

      try {
        const result = await this.deviceStore.connect(ip);
        if (result.success) {
          this.$refs.connectModal.onSuccess();
          this.toast.showSuccess("已连接到 RenLight 设备");
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

    async switchMode(mode) {
      this.deviceMode = mode;
      this.deviceStore.setDeviceMode(mode, {
        businessMode: false,
      });

      if (mode === "eyes") {
        this.openSpiritScreen();
      } else if (mode === "clock" || mode === "animation" || mode === "theme") {
        this.editClockWithMode(mode);
      } else if (mode === "countdown") {
        this.openCountdownBoard();
      } else if (mode === "ambient_effect" || mode === "led_matrix_showcase") {
        this.openLedMatrixShowcase();
      } else if (mode === "tetris") {
        this.openTetrisSettings();
      } else if (mode === "game_screensaver") {
        this.openGameScreensaver();
      } else if (mode === "gif_player") {
        this.openGifPlayer();
      } else if (mode === "canvas") {
        this.openCanvasEditor();
      }
    },

    editClockWithMode(mode) {
      let url = "/pages/clock-editor/clock-editor";
      if (mode === "animation") {
        url = "/pages/clock-editor/animation-clock";
      } else if (mode === "theme") {
        url = "/pages/clock-editor/theme-clock";
      }
      uni.navigateTo({
        url,
      });
    },

    openTetrisSettings() {
      uni.navigateTo({
        url: "/pages/tetris-settings/tetris-settings",
      });
    },

    openCanvasEditor() {
      uni.navigateTo({
        url: "/pages/canvas-editor/canvas-editor",
      });
    },

    openLedMatrixShowcase() {
      uni.navigateTo({
        url: "/pages/led-matrix/led-matrix",
      });
    },

    openCountdownBoard() {
      uni.navigateTo({
        url: "/pages/countdown-board/countdown-board",
      });
    },

    openGifPlayer() {
      uni.navigateTo({
        url: "/pages/gif-player/gif-player",
      });
    },

    openGameScreensaver() {
      uni.navigateTo({
        url: "/pages/game-screensaver/game-screensaver",
      });
    },

    openSpiritScreen() {
      uni.navigateTo({
        url: "/pages/spirit-screen/spirit-screen",
      });
    },

    openDeviceParams() {
      if (!this.isDeviceConnected) {
        this.toast.showInfo("请先连接设备");
        return;
      }
      uni.navigateTo({
        url: "/pages/device-params/device-params",
      });
    },

    importJSON() {
      this.showJsonImportModal = true;
    },

    async handleJsonImport(jsonData) {
      try {
        // 验证 JSON 数据格式
        if (!jsonData || typeof jsonData !== "object") {
          this.toast.showError("无效的配置数据");
          return;
        }

        // 开始传输，禁止关闭弹窗
        this.jsonImportSending = true;

        // 检查设备是否连接
        if (!this.deviceStore.connected) {
          this.jsonImportSending = false;
          this.showJsonImportModal = false;
          this.toast.showError("设备未连接");
          return;
        }

        const ws = this.deviceStore.getWebSocket();

        // 检查是否为动画数据（包含 f 和 d 字段的紧凑格式）
        if (
          jsonData.f !== undefined &&
          jsonData.d !== undefined &&
          Array.isArray(jsonData.d)
        ) {
          console.log("检测到动画数据，逐帧发送到 ESP32");

          try {
            const frameCount = jsonData.f;
            const frames = jsonData.d;

            // 1. 发送 animation_begin
            await ws.send({ cmd: "animation_begin", frameCount: frameCount });
            console.log(`动画初始化完成，准备发送 ${frameCount} 帧`);

            // 2. 逐帧发送（大帧自动拆分 chunk）
            const CHUNK_SIZE = 500; // 每个 chunk 最多 500 像素
            for (let i = 0; i < frames.length && i < frameCount; i++) {
              const frame = frames[i]; // [type, delay, count, pixels]
              const type = frame[0];
              const delay = frame[1];
              const pixels = frame[3];
              const totalPixels = pixels.length;

              if (totalPixels <= CHUNK_SIZE) {
                // 小帧直接发送
                await ws.send({
                  cmd: "animation_frame",
                  index: i,
                  frame: frame,
                });
                console.log(
                  `帧 ${i + 1}/${frameCount} 直接发送 (${totalPixels} 像素)`,
                );
              } else {
                // 大帧拆分发送
                await ws.send({
                  cmd: "frame_begin",
                  index: i,
                  type,
                  delay,
                  totalPixels,
                });
                console.log(
                  `帧 ${i + 1}/${frameCount} 开始分片 (${totalPixels} 像素)`,
                );

                for (
                  let offset = 0;
                  offset < totalPixels;
                  offset += CHUNK_SIZE
                ) {
                  const chunk = pixels.slice(offset, offset + CHUNK_SIZE);
                  await ws.send({
                    cmd: "frame_chunk",
                    index: i,
                    pixels: chunk,
                  });
                  console.log(
                    `  chunk ${Math.floor(offset / CHUNK_SIZE) + 1}: ${chunk.length} 像素`,
                  );
                  await new Promise((resolve) => setTimeout(resolve, 50));
                }
              }

              // 每帧间隔 100ms，让 ESP32 处理
              if (i < frameCount - 1) {
                await new Promise((resolve) => setTimeout(resolve, 100));
              }
            }

            // 3. 发送 animation_end
            await ws.send({ cmd: "animation_end" });

            this.toast.showSuccess(`动画已发送！${frameCount} 帧`);
          } catch (err) {
            console.error("发送动画数据失败:", err);
            this.toast.showError("发送动画数据失败：" + err.message);
          }

          this.jsonImportSending = false;
          this.showJsonImportModal = false;
          return;
        }

        // 原有的闹钟配置处理逻辑
        // 保存配置到本地存储
        uni.setStorageSync("clock_config", JSON.stringify(jsonData));

        // 如果有像素数据，也保存
        if (jsonData.imagePixels) {
          uni.setStorageSync(
            "clock_image_pixels",
            JSON.stringify(jsonData.imagePixels),
          );
        }

        // 直接发送到设备
        try {
          // 转换配置格式
          const configData = {
            time: {
              fontSize: jsonData.timeFontSize || 1,
              x: jsonData.timeX || 17,
              y: jsonData.timeY || 5,
              color: this.hexToRgb(jsonData.timeColor || "#64c8ff"),
            },
            date: {
              show: jsonData.showDate !== false,
              fontSize: jsonData.dateFontSize || 1,
              x: jsonData.dateX || 3,
              y: jsonData.dateY || 30,
              color: this.hexToRgb(jsonData.dateColor || "#787878"),
            },
            week: {
              show: jsonData.showWeek !== false,
              x: jsonData.weekX || 23,
              y: jsonData.weekY || 44,
              color: this.hexToRgb(jsonData.weekColor || "#646464"),
            },
            image: {
              show: jsonData.showImage || false,
              x: jsonData.imageX || 0,
              y: jsonData.imageY || 0,
              width: jsonData.imageWidth || 64,
              height: jsonData.imageHeight || 64,
            },
          };

          // 闹钟背景图必须用闹钟模式接收，先切换模式
          await ws.setMode("clock");

          // 发送配置
          await ws.send({ cmd: "set_clock_config", config: configData });

          // 如果有像素数据，使用二进制方式发送（闹钟背景图）
          if (jsonData.imagePixels && jsonData.imagePixels.length > 0) {
            await this.sendImagePixelsBinary(jsonData.imagePixels);
            this.toast.showSuccess(
              `配置已发送！包含 ${jsonData.imagePixels.length} 个像素点`,
            );
          } else {
            this.toast.showSuccess("配置已发送到设备！");
          }

          // 传输完成，关闭弹窗
          this.jsonImportSending = false;
          this.showJsonImportModal = false;
        } catch (err) {
          console.error("发送配置失败:", err);
          this.jsonImportSending = false;
          this.showJsonImportModal = false;
          this.toast.showError("发送失败：" + err.message);
        }
      } catch (e) {
        console.error("导入配置失败:", e);
        this.jsonImportSending = false;
        this.showJsonImportModal = false;
        this.toast.showError("导入失败：" + e.message);
      }
    },

    async sendImagePixelsBinary(pixels) {
      // 使用二进制方式发送像素数据
      // 格式：每个像素5字节 (x, y, r, g, b)
      const ws = this.deviceStore.getWebSocket();

      console.log(`开始发送像素数据，共 ${pixels.length} 个像素`);
      console.log(`前3个像素:`, pixels.slice(0, 3));

      // 每批 64 个像素，间隔 200ms
      const batchSize = 64;
      let sentCount = 0;

      for (let i = 0; i < pixels.length; i += batchSize) {
        const batch = pixels.slice(i, Math.min(i + batchSize, pixels.length));
        const binaryData = [];

        for (const pixel of batch) {
          binaryData.push(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
        }

        const buffer = new Uint8Array(binaryData);

        // 打印第一批的前15个字节（3个像素）
        if (i === 0) {
          console.log(`第一批前15字节:`, Array.from(buffer.slice(0, 15)));
        }

        await new Promise((resolve, reject) => {
          ws.socket.send({
            data: buffer.buffer,
            success: () => {
              sentCount += batch.length;
              if (sentCount % 100 === 0 || sentCount === pixels.length) {
                console.log(
                  `已发送 ${sentCount}/${pixels.length} 个像素 (${Math.round((sentCount / pixels.length) * 100)}%)`,
                );
              }
              resolve();
            },
            fail: (err) => {
              console.error("发送失败:", err);
              reject(err);
            },
          });
        });

        // 每批之间等待 200ms
        await new Promise((resolve) => setTimeout(resolve, 200));
      }

      console.log(`✓ 像素数据发送完成，共 ${sentCount} 个像素`);
      console.log(`最后3个像素:`, pixels.slice(-3));

      // 等待 ESP32 确认消息（发送完成后等待1秒，ESP32需要500ms保存）
      const receivedCount = await this.waitForPixelConfirmation(1000);

      if (receivedCount === null) {
        console.warn("⚠ 未收到 ESP32 确认消息，无法验证数量");
        return;
      }

      console.log(`ESP32 确认接收: ${receivedCount} 个像素`);

      if (receivedCount === sentCount) {
        console.log(`✓✓✓ 数量匹配！发送 ${sentCount}，接收 ${receivedCount}`);
      } else {
        console.error(
          `✗✗✗ 数量不匹配！发送 ${sentCount}，接收 ${receivedCount}，丢失 ${sentCount - receivedCount} 个像素`,
        );
        throw new Error(
          `数据传输不完整：发送 ${sentCount} 个，接收 ${receivedCount} 个`,
        );
      }
    },

    async waitForPixelConfirmation(timeout) {
      // 等待 ESP32 发送确认消息
      return new Promise((resolve) => {
        const ws = this.deviceStore.getWebSocket();
        let timeoutId = null;

        const messageHandler = (data) => {
          if (
            data.status === "ok" &&
            data.message === "pixels_received" &&
            data.count !== undefined
          ) {
            clearTimeout(timeoutId);
            ws.offMessage(messageHandler);
            resolve(data.count);
          }
        };

        ws.onMessage(messageHandler);

        timeoutId = setTimeout(() => {
          ws.offMessage(messageHandler);
          resolve(null);
        }, timeout);
      });
    },

    handleJsonImportError(message) {
      this.toast.showError(message);
      // 错误时不关闭弹窗，让用户可以修改
    },

    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return result
        ? {
            r: parseInt(result[1], 16),
            g: parseInt(result[2], 16),
            b: parseInt(result[3], 16),
          }
        : { r: 255, g: 255, b: 255 };
    },

    goToWifiConfig() {
      uni.navigateTo({ url: "/pages/ble-config/ble-config" });
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

.preview-entry-grid {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
  margin-bottom: 24rpx;
}

.preview-entry-card {
  padding: 24rpx 28rpx;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid #e9edf3;
  box-shadow: 0 12rpx 32rpx rgba(15, 23, 42, 0.05);
  display: flex;
  align-items: center;
  gap: 18rpx;
}

.preview-entry-icon {
  width: 80rpx;
  height: 80rpx;
  border-radius: 20rpx;
  background: #f4f6fb;
  border: 2rpx solid #e9edf3;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.preview-entry-icon.warm {
  background: #fff5df;
}

.preview-entry-icon.mint {
  background: #eafcf8;
}

.preview-entry-text {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.preview-entry-title {
  font-size: 30rpx;
  font-weight: 600;
  color: #111827;
}

.preview-entry-desc {
  font-size: 24rpx;
  color: #64748b;
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
  gap: 16rpx;
}

.connect-entry-card {
  border-radius: 0;
  background: var(--nb-surface);
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  padding: 20rpx;
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.connect-entry-icon,
.panel-action-icon {
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

.panel-action-icon.warm {
  background: var(--nb-yellow);
}

.panel-action-icon.danger {
  background: var(--nb-coral);
}

.connect-entry-icon.mint,
.panel-action-icon.mint {
  background: var(--nb-green);
}

.panel-action-icon.blue {
  background: var(--nb-blue);
}

.connect-entry-text,
.panel-action-text {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 6rpx;
}

.connect-entry-label,
.panel-action-label {
  font-size: 28rpx;
  font-weight: 600;
}

.connect-entry-label,
.panel-action-label {
  color: #111827;
}

.connect-entry-desc,
.panel-action-desc,
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
  gap: 12rpx;
}

.mode-badge {
  border-radius: 0;
  padding: 16rpx 8rpx 14rpx;
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10rpx;
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

.mode-pixel-icon {
  width: 52rpx;
  height: 52rpx;
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 4rpx;
}

.mode-pixel-cell {
  border-radius: 0;
  background: currentColor;
}

.mode-badge.pink {
  background: #fff1f6;
}

.mode-badge.pink .mode-badge-icon-core {
  background: #f472b6;
}

.mode-badge.cyan {
  background: #ecfbfd;
}

.mode-badge.cyan .mode-badge-icon-core {
  background: #00c9d8;
}

.mode-badge.teal {
  background: #e9faf6;
}

.mode-badge.teal .mode-badge-icon-core {
  background: #14b8a6;
}

.mode-badge.purple {
  background: #f1ecff;
}

.mode-badge.purple .mode-badge-icon-core {
  background: #8b5cf6;
}

.mode-badge.blue {
  background: var(--nb-surface);
}

.mode-badge.blue .mode-badge-icon-core {
  background: var(--nb-blue);
}

.mode-badge.indigo {
  background: #eef0ff;
}

.mode-badge.indigo .mode-badge-icon-core {
  background: #6f68ff;
}

.mode-badge.green {
  background: var(--nb-surface);
}

.mode-badge.green .mode-badge-icon-core {
  background: var(--nb-green);
}

.mode-badge.gold {
  background: var(--nb-surface);
}

.mode-badge.gold .mode-badge-icon-core {
  background: var(--nb-yellow);
}

.mode-badge.orange {
  background: var(--nb-surface);
}

.mode-badge.orange .mode-badge-icon-core {
  background: #f59e0b;
}

.mode-badge.red {
  background: var(--nb-surface);
}

.mode-badge.red .mode-badge-icon-core {
  background: var(--nb-coral);
}

.mode-badge.mint {
  background: #e8faf2;
}

.mode-badge.mint .mode-badge-icon-core {
  background: #42c7a0;
}

.mode-badge.azure {
  background: #e8f2ff;
}

.mode-badge.azure .mode-badge-icon-core {
  background: #5a90ff;
}

.panel-card {
  background: transparent;
  border: 0;
  box-shadow: none;
  border-radius: 0;
  overflow: hidden;
}

.panel-action {
  display: flex;
  align-items: center;
  gap: 18rpx;
  padding: 24rpx;
  border: 2rpx solid #000000;
  background: #ffffff;
}

.panel-divider {
  height: 2rpx;
  background: rgba(0, 0, 0, 0.12);
  margin: 0 24rpx;
}

@media (max-width: 720px) {
  .mode-badge-grid {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }
}
</style>
