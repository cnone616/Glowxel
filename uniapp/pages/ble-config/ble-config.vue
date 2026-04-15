<template>
  <view class="wifi-config-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--nb-ink)"
        />
      </view>
      <text class="nav-title glx-topbar__title">热点配网</text>
    </view>

    <scroll-view
      scroll-y
      class="content glx-scroll-region glx-page-shell__content"
    >
      <view class="hero-card glx-panel-card">
        <view class="card-title-section glx-panel-head">
          <text class="card-title glx-panel-title">首次使用先连接设备热点</text>
          <text class="card-subtitle glx-panel-subtitle">
            Glowxel 不再走蓝牙配网。请先连接设备热点，再在浏览器中打开设备门户完成
            WiFi 配置。
          </text>
        </view>

        <view class="tip-box">
          <text class="tip-title">仅支持 2.4GHz WiFi</text>
          <text class="tip-text">
            请不要使用 5GHz 网络。如果你的路由器开启了双频同名，请先确认当前选择的是
            2.4GHz 频段。
          </text>
        </view>
      </view>

      <view class="step-list">
        <view class="step-card glx-panel-card">
          <view class="step-badge">1</view>
          <view class="step-body">
            <text class="step-title">连接设备热点</text>
            <text class="step-desc">
              打开系统 WiFi 列表，找到以
              <text class="inline-strong">Glowxel-</text>
              开头的设备热点。热点名会带设备序列号后缀，用来区分多台设备。
            </text>
          </view>
        </view>

        <view class="step-card glx-panel-card">
          <view class="step-badge">2</view>
          <view class="step-body">
            <text class="step-title">打开设备配网页</text>
            <text class="step-desc">
              连接热点后，如果系统没有自动弹出门户，请手动在浏览器里输入
              <text class="inline-strong">192.168.4.1</text>。
            </text>
          </view>
        </view>

        <view class="step-card glx-panel-card">
          <view class="step-badge">3</view>
          <view class="step-body">
            <text class="step-title">选择 2.4GHz WiFi 并保存</text>
            <text class="step-desc">
              在设备门户中点击附近热点名称或手动输入 SSID，填写密码后保存。设备会自动重启并尝试连接。
            </text>
          </view>
        </view>
      </view>

      <view class="info-card glx-panel-card">
        <view class="info-row">
          <text class="info-label">热点名称</text>
          <text class="info-value">Glowxel-设备序列号</text>
        </view>
        <view class="info-row">
          <text class="info-label">门户地址</text>
          <text class="info-value">192.168.4.1</text>
        </view>
      </view>

      <view class="guide-card glx-panel-card">
        <view class="card-title-section glx-panel-head">
          <text class="card-title glx-panel-title">配网完成后</text>
          <text class="card-subtitle glx-panel-subtitle">
            设备重启并连接成功后，会在屏幕上显示新的 IP 地址。回到设备控制页，输入这个
            IP 即可连接设备。
          </text>
        </view>
      </view>

      <view class="actions">
        <view class="secondary-btn" @click="handleBack">
          <text class="secondary-btn-text">返回上一级</text>
        </view>
        <view class="action-btn glx-cta-button" @click="goToControl">
          <text class="btn-text">返回设备控制</text>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";

export default {
  mixins: [statusBarMixin],
  components: { Icon },
  methods: {
    goToControl() {
      uni.switchTab({ url: "/pages/control/control" });
    },

    handleBack() {
      uni.navigateBack();
    },
  },
};
</script>

<style scoped>
.wifi-config-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--nb-paper);
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background: var(--nb-surface);
  border-bottom: 2rpx solid var(--nb-ink);
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

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
}

.content {
  flex: 1;
  padding: 32rpx;
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.hero-card,
.guide-card,
.info-card,
.step-card {
  background: var(--nb-surface);
}

.card-title-section {
  gap: 10rpx;
}

.tip-box {
  margin-top: 24rpx;
  padding: 24rpx;
  border: 2rpx solid var(--nb-ink);
  background: #fff4d3;
  box-shadow: 6rpx 6rpx 0 var(--nb-ink);
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.tip-title {
  font-size: 26rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.tip-text {
  font-size: 24rpx;
  line-height: 1.7;
  color: #444444;
}

.step-list {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.step-card {
  display: flex;
  gap: 20rpx;
  align-items: flex-start;
}

.step-badge {
  width: 56rpx;
  height: 56rpx;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 2rpx solid var(--nb-ink);
  background: #74b9ff;
  color: var(--nb-ink);
  font-size: 28rpx;
  font-weight: 700;
  box-shadow: 4rpx 4rpx 0 var(--nb-ink);
}

.step-body {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.step-title {
  font-size: 28rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.step-desc {
  font-size: 24rpx;
  line-height: 1.7;
  color: #444444;
}

.inline-strong {
  font-weight: 700;
  color: var(--nb-ink);
}

.info-row {
  display: flex;
  justify-content: space-between;
  gap: 24rpx;
  padding: 20rpx 0;
  border-bottom: 2rpx solid rgba(0, 0, 0, 0.12);
}

.info-row:first-child {
  padding-top: 0;
}

.info-row:last-child {
  padding-bottom: 0;
  border-bottom: 0;
}

.info-label {
  font-size: 24rpx;
  color: #444444;
}

.info-value {
  flex: 1;
  text-align: right;
  font-size: 24rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.actions {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
  padding-bottom: 40rpx;
}

.secondary-btn {
  min-height: 96rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 2rpx solid var(--nb-ink);
  background: #ffffff;
  box-shadow: 6rpx 6rpx 0 var(--nb-ink);
}

.secondary-btn-text,
.btn-text {
  font-size: 28rpx;
  font-weight: 700;
  color: var(--nb-ink);
}
</style>
