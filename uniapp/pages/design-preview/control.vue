<template>
  <view class="nb-page nb-control-preview">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="nb-navbar">
      <view class="nb-nav-action" @click="goBack">
        <Icon name="arrow-left-bold" :size="28" color="#000000" />
      </view>
      <view class="nb-nav-title-wrap">
        <text class="nb-nav-title">设备控制预览</text>
        <text class="nb-nav-subtitle">Control as Neubrutalism</text>
      </view>
      <view class="nb-nav-spacer"></view>
    </view>

    <scroll-view scroll-y class="nb-scroll">
      <view class="nb-content">
        <view class="nb-card nb-card-lg control-hero">
          <view class="device-meta">
            <view class="device-copy">
              <text class="nb-eyebrow">Device Online</text>
              <text class="nb-display device-title">Glowxel 控制中心可以直接做成像街机主机台一样。</text>
              <text class="nb-body device-desc">
                顶部保留设备状态、连接入口和主操作；模式入口改成真正可识别的方形块面，而不是普通轻卡片。
              </text>
            </view>
            <view class="screen-shell">
              <view class="screen-topbar">
                <text class="screen-label">52 × 52 Live Preview</text>
                <text class="screen-dot"></text>
              </view>
              <view class="pixel-screen">
                <view
                  v-for="cell in pixelCells"
                  :key="cell.index"
                  class="pixel-cell"
                  :class="cell.tone"
                ></view>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">模式入口</text>
            <text class="nb-section-meta">方形缩略图 + 强标签 + 高识别颜色</text>
          </view>
          <view class="mode-grid">
            <view
              v-for="mode in modes"
              :key="mode.name"
              class="mode-card"
              :style="{ background: mode.color }"
            >
              <view class="mode-icon">
                <Icon :name="mode.icon" :size="34" color="#000000" />
              </view>
              <text class="mode-name">{{ mode.name }}</text>
              <text class="mode-tag">{{ mode.tag }}</text>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">主控制卡</text>
            <text class="nb-section-meta">强结构但不乱，重点信息只出现一次</text>
          </view>
          <view class="nb-grid-2">
            <view class="nb-grid-item">
              <view class="nb-card control-panel">
                <text class="panel-title">连接状态</text>
                <view class="status-row">
                  <view class="status-badge online">
                    <text class="status-badge-text">已连接</text>
                  </view>
                  <text class="status-ip">192.168.31.84</text>
                </view>
                <view class="metric-grid">
                  <view class="metric-card yellow">
                    <text class="metric-label">亮度</text>
                    <text class="metric-value">74%</text>
                  </view>
                  <view class="metric-card blue">
                    <text class="metric-label">模式</text>
                    <text class="metric-value">THEME</text>
                  </view>
                </view>
              </view>
            </view>
            <view class="nb-grid-item">
              <view class="nb-card control-panel">
                <text class="panel-title">快捷操作</text>
                <view class="quick-list">
                  <view v-for="action in actions" :key="action.label" class="quick-item" :style="{ background: action.color }">
                    <text class="quick-label">{{ action.label }}</text>
                    <text class="quick-desc">{{ action.desc }}</text>
                  </view>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">设置区降躁</text>
            <text class="nb-section-meta">全局铺满不代表每个块都要炸</text>
          </view>
          <view class="nb-card settings-shell">
            <view v-for="setting in settings" :key="setting.name" class="setting-row">
              <view class="setting-text">
                <text class="setting-name">{{ setting.name }}</text>
                <text class="setting-desc">{{ setting.desc }}</text>
              </view>
              <view class="setting-value">{{ setting.value }}</view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";

const PIXEL_TONES = [
  "empty",
  "yellow",
  "yellow",
  "empty",
  "pink",
  "pink",
  "empty",
  "empty",
  "empty",
  "yellow",
  "yellow",
  "empty",
  "pink",
  "pink",
  "empty",
  "blue",
  "empty",
  "empty",
  "blue",
  "blue",
  "blue",
  "blue",
  "empty",
  "blue",
  "green",
  "green",
  "empty",
  "orange",
  "orange",
  "empty",
  "green",
  "green",
  "empty",
  "empty",
  "orange",
  "orange",
  "empty",
  "empty",
  "green",
  "empty",
  "pink",
  "empty",
  "empty",
  "yellow",
  "yellow",
  "empty",
  "pink",
  "empty",
  "empty",
  "empty",
  "blue",
  "empty",
  "empty",
  "green",
  "empty",
  "orange",
  "empty",
  "blue",
  "blue",
  "empty",
  "green",
  "green",
  "orange",
  "orange"
];

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
  },
  data() {
    return {
      modes: [
        { name: "主题", tag: "Clock / Theme", icon: "clock-filling", color: "#FFD23F" },
        { name: "画板", tag: "Canvas", icon: "picture-filling", color: "#74B9FF" },
        { name: "桌宠", tag: "Spirit", icon: "smile-filling", color: "#FF6B6B" },
        { name: "特效", tag: "Effects", icon: "dynamic-filling", color: "#88D498" },
        { name: "功能", tag: "Feature", icon: "layout-filling", color: "#B8A9FA" },
        { name: "通知", tag: "Notify", icon: "notification-filling", color: "#FFA552" },
      ],
      actions: [
        { label: "同步网络时间", desc: "一键更新 RTC", color: "#FFFDF5" },
        { label: "发送当前配置", desc: "直接推到设备", color: "#FFD23F" },
        { label: "打开主题模式", desc: "快速切入展示", color: "#74B9FF" },
      ],
      settings: [
        { name: "亮度", desc: "硬件亮度保持大块结构，不用轻薄滑块", value: "74%" },
        { name: "WiFi", desc: "连接成功后保留必要信息，不重复展示入口", value: "已连接" },
        { name: "屏幕规格", desc: "拼豆区域与像素预览统一按 52×52 组织", value: "52×52" },
      ],
    };
  },
  computed: {
    pixelCells() {
      return PIXEL_TONES.map((tone, index) => ({
        index,
        tone,
      }));
    },
  },
  methods: {
    goBack() {
      uni.navigateBack();
    },
  },
};
</script>

<style lang="scss">
@import "../../styles/themes/neubrutalism-preview.css";

.control-hero {
  padding: 28rpx;
  background: var(--nb-coral);
}

.device-meta {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.device-title {
  display: block;
  margin-top: 24rpx;
  margin-bottom: 26rpx;
}

.device-desc {
  display: block;
  max-width: 640rpx;
}

.screen-shell {
  padding: 18rpx;
  background: var(--nb-surface);
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-md);
}

.screen-topbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16rpx;
}

.screen-label {
  font-size: 22rpx;
  line-height: 1.2;
  font-weight: 900;
  color: var(--nb-ink);
}

.screen-dot {
  width: 22rpx;
  height: 22rpx;
  background: var(--nb-coral);
  border: var(--nb-border);
  box-sizing: border-box;
}

.pixel-screen {
  display: flex;
  flex-wrap: wrap;
  gap: 6rpx;
  padding: 12rpx;
  background: #000000;
  border: var(--nb-border);
  box-sizing: border-box;
}

.pixel-cell {
  width: calc((100% - 42rpx) / 8);
  height: calc((100% - 42rpx) / 8);
  background: #111111;
}

.pixel-cell.yellow {
  background: var(--nb-yellow);
}

.pixel-cell.pink {
  background: var(--nb-coral);
}

.pixel-cell.blue {
  background: var(--nb-blue);
}

.pixel-cell.green {
  background: var(--nb-green);
}

.pixel-cell.orange {
  background: var(--nb-orange);
}

.mode-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 20rpx;
}

.mode-card {
  width: calc((100% - 40rpx) / 3);
  min-height: 210rpx;
  padding: 18rpx;
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-md);
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
}

.mode-icon {
  width: 76rpx;
  height: 76rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--nb-surface);
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
}

.mode-name {
  display: block;
  font-size: 30rpx;
  line-height: 1.15;
  font-weight: 900;
  color: var(--nb-ink);
}

.mode-tag {
  display: block;
  font-size: 20rpx;
  line-height: 1.25;
  font-weight: 800;
  color: rgba(0, 0, 0, 0.75);
}

.control-panel {
  padding: 22rpx;
  height: 100%;
}

.panel-title {
  display: block;
  font-size: 30rpx;
  line-height: 1.15;
  font-weight: 900;
  color: var(--nb-ink);
  margin-bottom: 18rpx;
}

.status-row {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 12rpx;
  margin-bottom: 16rpx;
}

.status-badge {
  padding: 10rpx 16rpx;
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
  background: var(--nb-green);
}

.status-badge-text {
  font-size: 22rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
}

.status-ip {
  font-size: 26rpx;
  line-height: 1.2;
  font-weight: 800;
  color: var(--nb-ink);
}

.metric-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 14rpx;
}

.metric-card {
  flex: 1;
  min-width: 160rpx;
  padding: 18rpx;
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
}

.metric-card.yellow {
  background: var(--nb-yellow);
}

.metric-card.blue {
  background: var(--nb-blue);
}

.metric-label {
  display: block;
  font-size: 20rpx;
  line-height: 1.2;
  font-weight: 800;
  color: rgba(0, 0, 0, 0.72);
  margin-bottom: 8rpx;
}

.metric-value {
  display: block;
  font-size: 30rpx;
  line-height: 1.1;
  font-weight: 900;
  color: var(--nb-ink);
}

.quick-list {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.quick-item {
  padding: 18rpx;
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
}

.quick-label {
  display: block;
  font-size: 28rpx;
  line-height: 1.15;
  font-weight: 900;
  color: var(--nb-ink);
  margin-bottom: 8rpx;
}

.quick-desc {
  display: block;
  font-size: 22rpx;
  line-height: 1.45;
  color: var(--nb-text-body);
}

.settings-shell {
  padding: 8rpx 20rpx;
}

.setting-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 18rpx 0;
  border-bottom: 4rpx solid var(--nb-ink);
}

.setting-row:last-child {
  border-bottom: 0;
}

.setting-text {
  flex: 1;
}

.setting-name {
  display: block;
  font-size: 28rpx;
  line-height: 1.15;
  font-weight: 900;
  color: var(--nb-ink);
  margin-bottom: 8rpx;
}

.setting-desc {
  display: block;
  font-size: 22rpx;
  line-height: 1.45;
  color: var(--nb-text-muted);
}

.setting-value {
  padding: 10rpx 14rpx;
  background: var(--nb-yellow);
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
  font-size: 22rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
}
</style>
