<template>
  <view class="control-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    <!-- 导航栏 -->
    <view class="header">
      <view class="header-content">
        <text class="header-title">设备控制</text>
      </view>
    </view>
    <scroll-view scroll-y class="content">
      <!-- 连接状态卡片 -->
      <view class="card connection-card">
        <view
          v-if="connectionStatus === 'connected'"
          class="connected-info"
          @click="handleConnect"
        >
          <view class="connected-dot"></view>
          <view class="connected-detail">
            <text class="connected-label">已连接</text>
            <text class="connected-ip">{{ deviceIp }}</text>
          </view>
          <text class="disconnect-text">断开</text>
        </view>
        <view v-else class="connect-actions">
          <view class="connect-btn" @click="handleConnect">
            <view class="connect-btn-icon">
              <Icon name="scanning" :size="44" color="#4F7FFF" />
            </view>
            <view class="connect-btn-info">
              <text class="connect-btn-label">WiFi 连接</text>
              <text class="connect-btn-desc">输入设备 IP 地址连接</text>
            </view>
            <Icon name="direction-right" :size="28" color="#C0C0C0" />
          </view>
          <view class="divider"></view>
          <view class="connect-btn" @click="goToBleConfig">
            <view class="connect-btn-icon">
              <Icon name="mobile-phone" :size="44" color="#4F7FFF" />
            </view>
            <view class="connect-btn-info">
              <text class="connect-btn-label">蓝牙配网</text>
              <text class="connect-btn-desc">首次使用请先配置 WiFi</text>
            </view>
            <Icon name="direction-right" :size="28" color="#C0C0C0" />
          </view>
        </view>
      </view>

      <!-- 已连接：显示设备功能 -->
      <template v-if="connectionStatus === 'connected'">
        <!-- 设备模式 -->
        <view class="section-title">设备模式</view>
        <view class="card">
          <view class="mode-grid">
            <view
              class="mode-item"
              :class="{ active: deviceMode === 'clock' }"
              @click="switchMode('clock')"
            >
              <view
                class="mode-icon-wrap"
                :class="{ active: deviceMode === 'clock' }"
              >
                <Icon
                  name="time"
                  :size="40"
                  :color="deviceMode === 'clock' ? '#4F7FFF' : '#999'"
                />
              </view>
              <text class="mode-name">静态时钟</text>
            </view>
            <view
              class="mode-item"
              :class="{ active: deviceMode === 'animation' }"
              @click="switchMode('animation')"
            >
              <view
                class="mode-icon-wrap"
                :class="{ active: deviceMode === 'animation' }"
              >
                <Icon
                  name="play"
                  :size="40"
                  :color="deviceMode === 'animation' ? '#4F7FFF' : '#999'"
                />
              </view>
              <text class="mode-name">动态时钟</text>
            </view>
            <view
              class="mode-item"
              :class="{ active: deviceMode === 'canvas' }"
              @click="switchMode('canvas')"
            >
              <view
                class="mode-icon-wrap"
                :class="{ active: deviceMode === 'canvas' }"
              >
                <Icon
                  name="picture"
                  :size="40"
                  :color="deviceMode === 'canvas' ? '#4F7FFF' : '#999'"
                />
              </view>
              <text class="mode-name">画板模式</text>
            </view>
            <view
              class="mode-item"
              :class="{ active: deviceMode === 'tetris' }"
              @click="openTetrisSettings"
            >
              <view
                class="mode-icon-wrap"
                :class="{ active: deviceMode === 'tetris' }"
              >
                <Icon
                  name="3column"
                  :size="40"
                  :color="deviceMode === 'tetris' ? '#4F7FFF' : '#999'"
                />
              </view>
              <text class="mode-name">方块屏保</text>
            </view>
          </view>
        </view>

        <!-- 时钟快捷操作 -->
        <view
          v-if="deviceMode === 'clock' || deviceMode === 'animation'"
          class="card"
        >
          <view class="action-row" @click="editClock">
            <Icon name="edit" :size="36" color="#4F7FFF" />
            <view class="action-info">
              <text class="action-label">自定义时钟样式</text>
              <text class="action-desc">编辑时钟字体、颜色和背景</text>
            </view>
            <Icon name="direction-right" :size="28" color="#C0C0C0" />
          </view>
          <view class="divider"></view>
          <view class="action-row" @click="importJSON">
            <Icon name="add" :size="36" color="#4F7FFF" />
            <view class="action-info">
              <text class="action-label">导入 JSON 配置</text>
              <text class="action-desc">从模拟器导入完整配置</text>
            </view>
            <Icon name="direction-right" :size="28" color="#C0C0C0" />
          </view>
        </view>

        <!-- 亮度调节 -->
        <view class="section-title">亮度调节</view>
        <view class="card">
          <view class="brightness-row">
            <Icon name="prompt" :size="36" color="#4F7FFF" />
            <text class="brightness-label">屏幕亮度</text>
            <text class="brightness-value">{{ brightness }}%</text>
          </view>
          <slider
            :value="brightness"
            @change="handleBrightnessChange"
            min="0"
            max="100"
            activeColor="#4F7FFF"
            backgroundColor="#E8E8E8"
            block-size="24"
            class="brightness-slider"
          />
        </view>

        <!-- 高级操作 -->
        <view class="section-title">高级</view>
        <view class="card">
          <view class="action-row danger" @click="handleResetWifi">
            <Icon name="close" :size="36" color="#FF4D4F" />
            <view class="action-info">
              <text class="action-label danger-text">重置网络</text>
              <text class="action-desc">清除 WiFi 配置并重启设备</text>
            </view>
            <Icon name="direction-right" :size="28" color="#C0C0C0" />
          </view>
        </view>
      </template>

      <!-- 底部留白 -->
      <view style="height: 120rpx"></view>
    </scroll-view>

    <!-- Toast -->
    <Toast ref="toastRef" />

    <!-- 连接弹窗 -->
    <ConnectModal
      :visible="showConnectModal"
      title="连接设备"
      description="请输入 Glowxel 设备的 IP 地址"
      :placeholder="deviceIp || '192.168.31.84'"
      :defaultValue="deviceIp"
      ref="connectModal"
      @confirm="handleConnectConfirm"
      @cancel="handleConnectCancel"
      @timeout="handleConnectTimeout"
      @update:visible="(val) => (showConnectModal = val)"
    />

    <!-- JSON 导入弹窗 -->
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

      // 连接状态
      connectionStatus: "disconnected", // disconnected, connecting, connected
      deviceIp: "",

      // 设备模式
      deviceMode: "clock", // clock, animation, canvas

      // 设置
      brightness: 50,

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

    // 从缓存读取设备 IP
    const savedIp = uni.getStorageSync("device_ip");
    if (savedIp) {
      this.deviceIp = savedIp;
    }

    // 从缓存读取设备模式
    const savedMode = uni.getStorageSync("device_mode");
    if (savedMode) {
      this.deviceMode = savedMode;
    }

    // 从 URL 参数读取模式（优先级更高）
    if (options && options.mode) {
      this.deviceMode = options.mode;
      uni.setStorageSync("device_mode", options.mode);
    }

    // 从缓存读取亮度
    const savedBrightness = uni.getStorageSync("device_brightness");
    if (savedBrightness !== undefined && savedBrightness !== null) {
      this.brightness = savedBrightness;
    }

    // 监听设备连接状态
    this.updateConnectionStatus();
  },

  onShow() {
    this.updateConnectionStatus();
  },

  computed: {
    isDeviceConnected() {
      return this.deviceStore?.connected || false;
    },
  },

  watch: {
    isDeviceConnected(val) {
      this.connectionStatus = val ? "connected" : "disconnected";
    },
  },

  methods: {
    updateConnectionStatus() {
      if (this.deviceStore?.connected) {
        this.connectionStatus = "connected";
      } else {
        this.connectionStatus = "disconnected";
      }
    },

    async handleConnect() {
      if (this.connectionStatus === "connected") {
        // 已连接，断开
        this.deviceStore.disconnect();
        this.connectionStatus = "disconnected";
        this.toast.showInfo("设备已断开");
        return;
      }

      if (this.connectionStatus !== "disconnected") {
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
          this.connectionStatus = "connected";

          // 获取设备实际状态（模式、亮度等）
          try {
            const ws = this.deviceStore.getWebSocket();
            const status = await ws.getStatus();

            if (status) {
              // 同步设备模式
              const mode = status.mode || "canvas";
              this.deviceMode = mode;
              uni.setStorageSync("device_mode", mode);
              console.log("同步设备模式:", mode);

              if (status.brightness !== undefined) {
                // 将板子的亮度值（0-178）映射回前端显示（0-100）
                const maxBrightness = 178; // 255 * 0.7
                this.brightness = Math.round(
                  (status.brightness * 100) / maxBrightness,
                );
                uni.setStorageSync("device_brightness", this.brightness);
                console.log(
                  "同步板子亮度:",
                  status.brightness,
                  "→",
                  this.brightness + "%",
                );
              }
            }
          } catch (err) {
            console.error("获取设备状态失败:", err);
          }

          this.$refs.connectModal.onSuccess();
          this.toast.showSuccess("已连接到 Glowxel 设备");
        } else {
          this.$refs.connectModal.onError("连接失败，请检查 IP 地址");
        }
      } catch (err) {
        console.error("连接失败:", err);
        this.$refs.connectModal.onError("连接失败，请检查 IP 地址");
      }
    },

    handleConnectCancel() {
      // 取消连接
    },

    handleConnectTimeout() {
      this.toast.showError("连接超时，请重试");
    },

    async handleBrightnessChange(e) {
      this.brightness = e.detail.value;

      // 保存到缓存
      uni.setStorageSync("device_brightness", this.brightness);

      // 如果已连接，发送亮度控制命令
      if (this.connectionStatus === "connected") {
        try {
          // 将 0-100 的值映射到 0-178 (70% of 255)，限制最大亮度为70%
          const maxBrightness = 178; // 255 * 0.7
          const value = Math.round((this.brightness * maxBrightness) / 100);
          await this.deviceStore.setBrightness(value);
        } catch (err) {
          console.error("设置亮度失败:", err);
        }
      }
    },

    async switchMode(mode) {
      if (this.deviceMode === mode) return;

      try {
        const ws = this.deviceStore.getWebSocket();
        // clock 在 ESP32 端对应 canvas 模式（黑底+时钟）
        const espMode = mode === "clock" ? "clock" : mode;
        await ws.setMode(espMode);
        this.deviceMode = mode;

        // 保存到缓存
        uni.setStorageSync("device_mode", mode);

        if (mode === "clock") {
          this.toast.showSuccess("已切换到静态时钟");
        } else if (mode === "animation") {
          this.toast.showSuccess("已切换到动态时钟");
        } else if (mode === "canvas") {
          this.toast.showSuccess("已切换到画板模式");
        } else if (mode === "tetris") {
          this.toast.showSuccess("已切换到方块屏保");
        }
      } catch (err) {
        console.error("切换模式失败:", err);
        this.toast.showError("切换模式失败");
      }
    },

    editClock() {
      uni.navigateTo({
        url: `/pages/clock-editor/clock-editor?mode=${this.deviceMode}`,
      });
    },

    openTetrisSettings() {
      uni.navigateTo({
        url: "/pages/tetris-settings/tetris-settings",
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

    handleResetWifi() {
      uni.showModal({
        title: "重置网络",
        content: "确定要清除 WiFi 配置吗？设备将重启并进入配网模式。",
        confirmText: "确定重置",
        confirmColor: "#ff4d4f",
        success: async (res) => {
          if (!res.confirm) return;
          try {
            const url = `http://${this.deviceIp}/clear-wifi`;
            uni.request({
              url,
              method: "GET",
              success: () => {
                this.toast.showSuccess("网络已重置，设备重启中...");
                this.deviceStore.disconnect();
                this.connectionStatus = "disconnected";
              },
              fail: () => {
                this.toast.showError("重置失败，请检查设备连接");
              },
            });
          } catch (err) {
            this.toast.showError("重置失败：" + err.message);
          }
        },
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

    goToBleConfig() {
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
  background-color: #f5f5f7;
  overflow: hidden;
}

.header {
  background-color: var(--color-card-background);
  border-bottom: 2rpx solid var(--border-primary);
  padding: 0 32rpx;
}

.header-content {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.header-actions {
  display: flex;
  gap: 16rpx;
}

.content {
  flex: 1;
  padding: 24rpx 28rpx;
  box-sizing: border-box;
  overflow-y: scroll;
}

/* 通用卡片 */
.card {
  background-color: #ffffff;
  border-radius: 24rpx;
  margin-bottom: 24rpx;
  overflow: hidden;
}

.section-title {
  font-size: 26rpx;
  font-weight: 500;
  color: #999;
  padding: 20rpx 8rpx 12rpx;
}

.divider {
  height: 1rpx;
  background-color: #f0f0f0;
  margin: 0 32rpx;
}

/* 连接状态卡片 */
.connected-info {
  display: flex;
  align-items: center;
  padding: 32rpx 32rpx;
  gap: 20rpx;
}

.connected-dot {
  width: 20rpx;
  height: 20rpx;
  border-radius: 50%;
  background-color: #34c759;
  box-shadow: 0 0 8rpx rgba(52, 199, 89, 0.4);
  flex-shrink: 0;
}

.connected-detail {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.connected-label {
  font-size: 30rpx;
  font-weight: 600;
  color: #1a1a1a;
}

.connected-ip {
  font-size: 24rpx;
  color: #999;
  font-family: monospace;
}

.disconnect-text {
  font-size: 26rpx;
  color: #ff3b30;
  font-weight: 500;
}

.connect-actions {
  padding: 8rpx 0;
}

.connect-btn {
  display: flex;
  align-items: center;
  padding: 28rpx 32rpx;
  gap: 20rpx;
}

.connect-btn:active {
  background-color: #f8f8f8;
}

.connect-btn-icon {
  width: 80rpx;
  height: 80rpx;
  border-radius: 20rpx;
  background-color: #f0f4ff;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.connect-btn-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.connect-btn-label {
  font-size: 30rpx;
  font-weight: 500;
  color: #1a1a1a;
}

.connect-btn-desc {
  font-size: 24rpx;
  color: #999;
}

/* 模式切换 */
.mode-grid {
  display: flex;
  padding: 24rpx 20rpx;
  gap: 16rpx;
}

.mode-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 8rpx;
  border-radius: 20rpx;
  transition: all 0.2s;
}

.mode-item:active {
  transform: scale(0.96);
}

.mode-icon-wrap {
  width: 88rpx;
  height: 88rpx;
  border-radius: 22rpx;
  background-color: #f5f5f7;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.2s;
}

.mode-icon-wrap.active {
  background-color: #eef2ff;
  box-shadow: 0 4rpx 16rpx rgba(79, 127, 255, 0.15);
}

.mode-name {
  font-size: 24rpx;
  color: #666;
  font-weight: 500;
}

.mode-item.active .mode-name {
  color: #4f7fff;
  font-weight: 600;
}

/* 操作行 */
.action-row {
  display: flex;
  align-items: center;
  padding: 28rpx 32rpx;
  gap: 20rpx;
}

.action-row:active {
  background-color: #f8f8f8;
}

.action-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.action-label {
  font-size: 28rpx;
  font-weight: 500;
  color: #1a1a1a;
}

.action-desc {
  font-size: 22rpx;
  color: #999;
}

.danger-text {
  color: #ff3b30;
}

/* 亮度调节 */
.brightness-row {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 28rpx 32rpx 8rpx;
}

.brightness-label {
  flex: 1;
  font-size: 28rpx;
  font-weight: 500;
  color: #1a1a1a;
}

.brightness-value {
  font-size: 28rpx;
  font-weight: 600;
  color: #4f7fff;
  font-family: monospace;
}

.brightness-slider {
  margin: 0 32rpx 24rpx;
}
</style>
