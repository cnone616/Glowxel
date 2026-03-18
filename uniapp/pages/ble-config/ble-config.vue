<template>
  <view class="ble-page">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <text class="nav-title">蓝牙配网</text>
    </view>

    <scroll-view scroll-y class="content">
      <!-- 步骤指示 -->
      <view class="step-indicator">
        <view class="step" :class="{ active: step >= 1, done: step > 1 }">
          <text class="step-num">1</text>
          <text class="step-text">搜索设备</text>
        </view>
        <view class="step-line" :class="{ active: step > 1 }"></view>
        <view class="step" :class="{ active: step >= 2, done: step > 2 }">
          <text class="step-num">2</text>
          <text class="step-text">配置网络</text>
        </view>
        <view class="step-line" :class="{ active: step > 2 }"></view>
        <view class="step" :class="{ active: step >= 3 }">
          <text class="step-num">3</text>
          <text class="step-text">完成</text>
        </view>
      </view>

      <!-- Step 1: 扫描设备 -->
      <view v-if="step === 1" class="card">
        <view class="card-title-section">
          <text class="card-title">搜索 Glowxel 设备</text>
          <text class="card-subtitle">请确保设备已开机且处于配网模式</text>
        </view>

        <view v-if="scanning" class="scan-status">
          <view class="loading-spinner"></view>
          <text class="scan-text">正在搜索蓝牙设备...</text>
        </view>

        <view v-if="devices.length > 0" class="device-list">
          <view
            v-for="(device, index) in devices"
            :key="index"
            class="device-item"
            @click="connectDevice(device)"
          >
            <view class="device-icon">
              <Icon name="link" :size="32" />
            </view>
            <view class="device-info">
              <text class="device-name">{{ device.name || "未知设备" }}</text>
              <text class="device-id">{{ device.deviceId }}</text>
            </view>
            <text class="device-rssi">{{ device.RSSI }}dBm</text>
          </view>
        </view>

        <view v-if="!scanning && devices.length === 0" class="empty-state">
          <text class="empty-text">未找到设备</text>
          <text class="empty-hint"
            >请确保 Glowxel 处于配网模式（WiFi 未配置状态）</text
          >
        </view>

        <view class="action-btn" @click="startScan">
          <text class="btn-text">{{
            scanning ? "搜索中..." : "开始搜索"
          }}</text>
        </view>
      </view>

      <!-- Step 2: 配置 WiFi -->
      <view v-if="step === 2" class="card">
        <view class="card-title-section">
          <text class="card-title">配置 WiFi 网络</text>
          <text class="card-subtitle"
            >已连接到 {{ connectedDevice?.name }}</text
          >
        </view>

        <view class="form-group">
          <text class="form-label">WiFi 名称 (SSID)</text>
          <input
            class="form-input"
            v-model="wifiSSID"
            placeholder="请输入 WiFi 名称"
          />
        </view>

        <view class="form-group">
          <text class="form-label">WiFi 密码</text>
          <input
            class="form-input"
            v-model="wifiPassword"
            type="password"
            placeholder="请输入 WiFi 密码"
          />
        </view>

        <view v-if="sending" class="scan-status">
          <view class="loading-spinner"></view>
          <text class="scan-text">正在发送配置...</text>
        </view>

        <view
          class="action-btn"
          :class="{ disabled: sending }"
          @click="sendWifiConfig"
        >
          <text class="btn-text">{{ sending ? "发送中..." : "发送配置" }}</text>
        </view>
      </view>

      <!-- Step 3: 完成 -->
      <view v-if="step === 3" class="card success-card">
        <view class="success-icon">✓</view>
        <text class="success-title">配网成功</text>
        <text class="success-desc">WiFi 配置已发送，设备正在重启连接网络</text>
        <text class="success-hint">请稍等几秒，然后在设备控制页面连接设备</text>

        <view class="action-btn" @click="goToControl">
          <text class="btn-text">返回设备控制</text>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";

const BLE_SERVICE_UUID = "4FAFC201-1FB5-459E-8FCC-C5C9C331914B";
const BLE_WIFI_CHAR_UUID = "BEB5483E-36E1-4688-B7F5-EA07361B26A8";
const BLE_STATUS_CHAR_UUID = "BEB5483E-36E1-4688-B7F5-EA07361B26A9";

export default {
  mixins: [statusBarMixin],
  components: { Icon },
  data() {
    return {
      step: 1,
      scanning: false,
      devices: [],
      connectedDevice: null,
      connectedDeviceId: "",
      wifiSSID: "",
      wifiPassword: "",
      sending: false,
    };
  },

  onLoad() {
    this.autoFillWifi();
  },

  onUnload() {
    this.cleanup();
  },

  methods: {
    autoFillWifi() {
      uni.startWifi({
        success: () => {
          uni.getConnectedWifi({
            success: (res) => {
              if (res.wifi && res.wifi.SSID) {
                this.wifiSSID = res.wifi.SSID;
              }
            },
          });
        },
      });
    },
    startScan() {
      if (this.scanning) return;
      this.devices = [];
      this.scanning = true;

      uni.openBluetoothAdapter({
        success: () => {
          uni.startBluetoothDevicesDiscovery({
            allowDuplicatesKey: false,
            success: () => {
              uni.onBluetoothDeviceFound((res) => {
                res.devices.forEach((device) => {
                  if (device.name && device.name.includes("Glowxel")) {
                    const exists = this.devices.find(
                      (d) => d.deviceId === device.deviceId,
                    );
                    if (!exists) {
                      this.devices.push(device);
                    }
                  }
                });
              });

              // 10秒后停止扫描
              setTimeout(() => {
                this.stopScan();
              }, 10000);
            },
            fail: () => {
              this.scanning = false;
              uni.showToast({ title: "扫描失败", icon: "none" });
            },
          });
        },
        fail: (err) => {
          this.scanning = false;
          if (err.errCode === 10001) {
            uni.showToast({ title: "请打开手机蓝牙", icon: "none" });
          } else {
            uni.showToast({ title: "蓝牙初始化失败", icon: "none" });
          }
        },
      });
    },

    stopScan() {
      this.scanning = false;
      uni.stopBluetoothDevicesDiscovery({ success: () => {} });
    },

    connectDevice(device) {
      this.stopScan();
      uni.showLoading({ title: "连接中..." });

      uni.createBLEConnection({
        deviceId: device.deviceId,
        success: () => {
          this.connectedDevice = device;
          this.connectedDeviceId = device.deviceId;

          // 延迟获取服务，部分安卓需要
          setTimeout(() => {
            uni.getBLEDeviceServices({
              deviceId: device.deviceId,
              success: (res) => {
                const found = res.services.find((s) =>
                  s.uuid.toUpperCase().includes("4FAFC201"),
                );
                if (found) {
                  this.step = 2;
                  uni.hideLoading();

                  // 监听状态通知
                  this.listenNotify();
                } else {
                  uni.hideLoading();
                  uni.showToast({ title: "设备服务不匹配", icon: "none" });
                }
              },
              fail: () => {
                uni.hideLoading();
                uni.showToast({ title: "获取服务失败", icon: "none" });
              },
            });
          }, 1000);
        },
        fail: () => {
          uni.hideLoading();
          uni.showToast({ title: "连接失败，请重试", icon: "none" });
        },
      });
    },

    listenNotify() {
      uni.notifyBLECharacteristicValueChange({
        deviceId: this.connectedDeviceId,
        serviceId: BLE_SERVICE_UUID,
        characteristicId: BLE_STATUS_CHAR_UUID,
        state: true,
        success: () => {
          uni.onBLECharacteristicValueChange((res) => {
            const value = String.fromCharCode.apply(
              null,
              new Uint8Array(res.value),
            );
            console.log("[BLE] 状态通知:", value);
            if (value === "saved") {
              this.sending = false;
              this.step = 3;
            } else if (value.startsWith("error")) {
              this.sending = false;
              uni.showToast({ title: "配置失败: " + value, icon: "none" });
            }
          });
        },
      });
    },

    sendWifiConfig() {
      if (!this.wifiSSID || !this.wifiPassword) {
        uni.showToast({ title: "请填写完整的 WiFi 信息", icon: "none" });
        return;
      }
      if (this.sending) return;
      this.sending = true;

      const data = this.wifiSSID + "\n" + this.wifiPassword;
      const buffer = new ArrayBuffer(data.length);
      const view = new Uint8Array(buffer);
      for (let i = 0; i < data.length; i++) {
        view[i] = data.charCodeAt(i);
      }

      uni.writeBLECharacteristicValue({
        deviceId: this.connectedDeviceId,
        serviceId: BLE_SERVICE_UUID,
        characteristicId: BLE_WIFI_CHAR_UUID,
        value: buffer,
        success: () => {
          console.log("[BLE] WiFi 配置已发送");
          // 等待设备通知 saved，如果 3 秒没收到就直接跳转
          setTimeout(() => {
            if (this.step !== 3) {
              this.sending = false;
              this.step = 3;
            }
          }, 3000);
        },
        fail: () => {
          this.sending = false;
          uni.showToast({ title: "发送失败，请重试", icon: "none" });
        },
      });
    },

    cleanup() {
      if (this.connectedDeviceId) {
        uni.closeBLEConnection({ deviceId: this.connectedDeviceId });
      }
      uni.closeBluetoothAdapter({ success: () => {} });
    },

    goToControl() {
      this.cleanup();
      uni.switchTab({ url: "/pages/control/control" });
    },

    handleBack() {
      this.cleanup();
      uni.navigateBack();
    },
  },
};
</script>

<style scoped>
.ble-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
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
  padding: 32rpx;
}

.step-indicator {
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 40rpx;
  padding: 0 40rpx;
}

.step {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
}

.step-num {
  width: 48rpx;
  height: 48rpx;
  border-radius: 50%;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 24rpx;
  color: var(--text-secondary);
  text-align: center;
  line-height: 48rpx;
}

.step.active .step-num {
  background-color: rgba(79, 127, 255, 0.15);
  border-color: #4f7fff;
  color: #4f7fff;
}

.step.done .step-num {
  background-color: #4f7fff;
  color: #fff;
}
.step-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}
.step.active .step-text {
  color: #4f7fff;
}

.step-line {
  width: 80rpx;
  height: 4rpx;
  background-color: var(--border-primary);
  margin: 0 16rpx;
  margin-bottom: 28rpx;
}
.step-line.active {
  background-color: #4f7fff;
}

.card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  padding: 48rpx;
  box-shadow: var(--shadow-md);
}

.card-title-section {
  margin-bottom: 32rpx;
}
.card-title {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
  display: block;
}
.card-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
  margin-top: 8rpx;
  display: block;
}

.scan-status {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  padding: 32rpx 0;
}
.scan-text {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.loading-spinner {
  width: 32rpx;
  height: 32rpx;
  border: 4rpx solid var(--border-primary);
  border-top-color: #4f7fff;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}
@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.device-list {
  margin-bottom: 24rpx;
}
.device-item {
  display: flex;
  align-items: center;
  gap: 20rpx;
  padding: 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  margin-bottom: 16rpx;
}
.device-item:active {
  border-color: #4f7fff;
}
.device-icon {
  flex-shrink: 0;
}
.device-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}
.device-name {
  font-size: 26rpx;
  color: var(--text-primary);
  font-weight: 500;
}
.device-id {
  font-size: 18rpx;
  color: var(--text-secondary);
  font-family: monospace;
}
.device-rssi {
  font-size: 22rpx;
  color: var(--text-secondary);
  flex-shrink: 0;
}

.empty-state {
  text-align: center;
  padding: 48rpx 0;
}
.empty-text {
  font-size: 28rpx;
  color: var(--text-secondary);
  display: block;
}
.empty-hint {
  font-size: 22rpx;
  color: var(--text-tertiary);
  margin-top: 12rpx;
  display: block;
}

.form-group {
  margin-bottom: 24rpx;
}
.form-label {
  font-size: 24rpx;
  color: var(--text-secondary);
  margin-bottom: 12rpx;
  display: block;
}
.form-input {
  width: 100%;
  padding: 24rpx;
  height: 88rpx;
  line-height: 40rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  font-size: 28rpx;
  color: var(--text-primary);
  box-sizing: border-box;
}

.action-btn {
  margin-top: 24rpx;
  padding: 28rpx;
  background: linear-gradient(135deg, #4f7fff, #6c5ce7);
  border-radius: 16rpx;
  text-align: center;
}
.action-btn:active {
  opacity: 0.85;
  transform: scale(0.98);
}
.action-btn.disabled {
  opacity: 0.5;
}
.btn-text {
  font-size: 28rpx;
  color: #fff;
  font-weight: 600;
}

.success-card {
  text-align: center;
}
.success-icon {
  font-size: 96rpx;
  color: #00c853;
  margin-bottom: 24rpx;
}
.success-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--text-primary);
  display: block;
}
.success-desc {
  font-size: 26rpx;
  color: var(--text-secondary);
  margin-top: 16rpx;
  display: block;
}
.success-hint {
  font-size: 22rpx;
  color: var(--text-tertiary);
  margin-top: 12rpx;
  display: block;
}
</style>
