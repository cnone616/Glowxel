<template>
  <view class="wifi-config-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">热点配网</text>
    </view>

    <scroll-view
      scroll-y
      class="content glx-scroll-region glx-page-shell__content"
    >
      <view class="steps-card glx-panel-card">
        <view class="step-card">
          <view class="step-badge">1</view>
          <view class="step-body">
            <text class="step-title">连接设备热点</text>
            <text class="step-desc">
              在系统 WiFi 列表里选择
              <text class="inline-strong">Glowxel PixelBoard-设备序列号</text>。
            </text>
          </view>
        </view>

        <view class="step-card">
          <view class="step-badge">2</view>
          <view class="step-body">
            <text class="step-title">打开 192.168.4.1</text>
            <text class="step-desc">进入配网页后可任选一种：</text>
            <view class="step-options">
              <text class="step-option">扫描后直接选择 WiFi</text>
              <text class="step-option">手动输入 WiFi 名称</text>
            </view>
          </view>
        </view>

        <view class="step-card">
          <view class="step-badge">3</view>
          <view class="step-body">
            <text class="step-title">输入密码并保存</text>
            <text class="step-desc">只支持 2.4GHz，保存后设备会自动重启。</text>
            <text class="step-result">
              成功后看屏幕上的新 IP；失败会回到热点模式。
            </text>
          </view>
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
  padding: 32rpx 30rpx 44rpx;
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.steps-card {
  background: var(--nb-surface);
  padding: 0;
}

.step-card {
  display: flex;
  gap: 18rpx;
  align-items: flex-start;
  padding: 20rpx 24rpx;
  border-bottom: 2rpx solid rgba(0, 0, 0, 0.12);
}

.step-card:last-child {
  border-bottom: 0;
}

.step-badge {
  width: 52rpx;
  height: 52rpx;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 2rpx solid var(--nb-ink);
  background: #74b9ff;
  color: var(--nb-ink);
  font-size: 28rpx;
  font-weight: 700;
}

.step-body {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  padding-top: 2rpx;
}

.step-title {
  font-size: 28rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.step-desc {
  font-size: 22rpx;
  line-height: 1.5;
  color: #444444;
}

.step-options {
  display: flex;
  flex-direction: column;
  gap: 10rpx;
}

.step-option {
  display: flex;
  align-items: center;
  min-height: 52rpx;
  padding: 0 16rpx;
  border: 2rpx solid var(--nb-ink);
  background: #ffffff;
  font-size: 21rpx;
  font-weight: 600;
  color: var(--nb-ink);
}

.step-result {
  font-size: 22rpx;
  line-height: 1.5;
  color: #444444;
}

.inline-strong {
  font-weight: 700;
  color: var(--nb-ink);
}
</style>
