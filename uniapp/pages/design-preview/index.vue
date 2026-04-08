<template>
  <view class="nb-page nb-overview-page">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="nb-navbar">
      <view class="nb-nav-action" @click="goBack">
        <Icon name="arrow-left-bold" :size="28" color="#000000" />
      </view>
      <view class="nb-nav-title-wrap">
        <text class="nb-nav-title">新粗野总览</text>
        <text class="nb-nav-subtitle">Glowxel Style Lab</text>
      </view>
      <view class="nb-nav-spacer"></view>
    </view>

    <scroll-view scroll-y class="nb-scroll">
      <view class="nb-content">
        <view class="nb-card nb-card-lg hero-card">
          <view class="hero-topline">
            <text class="nb-eyebrow">Reference Locked</text>
            <text class="hero-ref">neubrutalism.com</text>
          </view>
          <text class="nb-display hero-title">白底主导的全局新粗野，不做软卡片，不做轻飘 UI。</text>
          <text class="nb-body hero-desc">
            我把你刚给的参考站拆成了可以直接落地的界面语法。下面不是抽象文档，而是准备拿来套你们项目的真实规则和场景预览。
          </text>
          <view class="nb-chip-row">
            <text class="nb-chip">纯黑描边</text>
            <text class="nb-chip">硬阴影</text>
            <text class="nb-chip">零渐变</text>
            <text class="nb-chip">高对比点缀色</text>
            <text class="nb-chip">标题强势 / 正文克制</text>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">视觉语法</text>
            <text class="nb-section-meta">从参考站提炼成 Glowxel 可执行规则</text>
          </view>
          <view class="nb-grid-2">
            <view class="nb-grid-item">
              <view class="nb-card spec-card">
                <text class="nb-heading">结构</text>
                <view class="spec-list">
                  <text class="nb-body spec-item">描边统一用黑色粗边，不再使用淡灰细线。</text>
                  <text class="nb-body spec-item">默认直角，少量场景最多只允许极小圆角。</text>
                  <text class="nb-body spec-item">层级靠硬阴影和块面，不靠模糊发光。</text>
                </view>
              </view>
            </view>
            <view class="nb-grid-item">
              <view class="nb-card spec-card">
                <text class="nb-heading">文字</text>
                <view class="spec-list">
                  <text class="nb-body spec-item">标题必须有压迫感，但正文恢复可读性。</text>
                  <text class="nb-body spec-item">数字、状态、标签可以更粗更紧。</text>
                  <text class="nb-body spec-item">不做全页面都在大喊的排版。</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">配色板</text>
            <text class="nb-section-meta">一套黑白骨架 + 少量高饱和模块色</text>
          </view>
          <view class="palette-grid">
            <view
              v-for="swatch in palette"
              :key="swatch.name"
              class="palette-item"
            >
              <view class="nb-swatch" :style="{ background: swatch.color }">
                <text class="nb-swatch-name">{{ swatch.name }}</text>
                <text class="nb-swatch-value">{{ swatch.color }}</text>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">组件标准</text>
            <text class="nb-section-meta">按钮、标签、图标容器、输入块统一一套语法</text>
          </view>
          <view class="nb-card component-card">
            <view class="nb-button-row">
              <view class="nb-button primary">主按钮</view>
              <view class="nb-button accent">次强调</view>
              <view class="nb-button alert">危险操作</view>
            </view>
            <view class="nb-chip-row component-chips">
              <text class="nb-chip">MODE</text>
              <text class="nb-chip">THEME</text>
              <text class="nb-chip">52×52</text>
              <text class="nb-chip">LIVE</text>
            </view>
            <view class="nb-icon-row">
              <view class="nb-icon-tile">
                <Icon name="clock-filling" :size="34" color="#000000" />
              </view>
              <view class="nb-icon-tile">
                <Icon name="picture-filling" :size="34" color="#000000" />
              </view>
              <view class="nb-icon-tile">
                <Icon name="dynamic-filling" :size="34" color="#000000" />
              </view>
              <view class="nb-icon-tile">
                <Icon name="smile-filling" :size="34" color="#000000" />
              </view>
            </view>
            <view class="input-mock">
              <text class="input-label">设备 IP</text>
              <text class="input-value">192.168.31.84</text>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">场景预览</text>
            <text class="nb-section-meta">不是概念板，直接看落到页面会长什么样</text>
          </view>
          <view class="nb-grid-2">
            <view class="nb-grid-item">
              <view class="nb-card nb-link-card scene-card control-scene" @click="openPage('/pages/design-preview/control')">
                <text class="nb-link-title">设备控制场景</text>
                <text class="nb-link-desc">看模式入口、主控制卡、预览屏幕和设置区如何全局新粗野化。</text>
              </view>
            </view>
            <view class="nb-grid-item">
              <view class="nb-card nb-link-card scene-card ecosystem-scene" @click="openPage('/pages/design-preview/ecosystem')">
                <text class="nb-link-title">主题 / 成就 / 素材场景</text>
                <text class="nb-link-desc">看缩略图、成就卡、素材卡和低噪设置区如何统一到一套视觉。</text>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">明确禁用</text>
            <text class="nb-section-meta">这些一旦混进来，味道就会立刻跑偏</text>
          </view>
          <view class="nb-rule-list">
            <view v-for="rule in forbiddenRules" :key="rule" class="nb-rule-item">
              <view class="nb-rule-bullet"></view>
              <text class="nb-rule-text">{{ rule }}</text>
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

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
  },
  data() {
    return {
      palette: [
        { name: "Ink", color: "#000000" },
        { name: "Paper", color: "#FFFDF5" },
        { name: "Punch Yellow", color: "#FFD23F" },
        { name: "Coral Pink", color: "#FF6B6B" },
        { name: "Sky Blue", color: "#74B9FF" },
        { name: "Soft Green", color: "#88D498" },
      ],
      forbiddenRules: [
        "不要把渐变、玻璃拟态、柔光阴影混进主系统，这会直接稀释新粗野的结构感。",
        "不要所有卡片都高饱和，真正有态度的是黑白骨架稳定，重点模块再上色。",
        "不要把设置页做成活动海报，信息密度高的操作区必须保留秩序和阅读节奏。",
        "不要继续使用轻描淡写的线框 iconfont 作为主入口图标，入口图标要有块面和重量感。",
      ],
    };
  },
  methods: {
    goBack() {
      uni.navigateBack();
    },
    openPage(url) {
      uni.navigateTo({ url });
    },
  },
};
</script>

<style lang="scss">
@import "../../styles/themes/neubrutalism-preview.css";

.hero-card {
  padding: 28rpx;
  margin-bottom: 18rpx;
  background: var(--nb-yellow);
}

.hero-topline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  margin-bottom: 28rpx;
}

.hero-ref {
  font-size: 22rpx;
  line-height: 1.2;
  font-weight: 900;
  color: var(--nb-ink);
}

.hero-title {
  margin-bottom: 28rpx;
  display: block;
}

.hero-desc {
  margin-bottom: 26rpx;
  display: block;
  max-width: 620rpx;
}

.spec-card {
  padding: 22rpx;
  min-height: 280rpx;
}

.spec-list {
  margin-top: 18rpx;
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.spec-item {
  display: block;
}

.palette-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 20rpx;
}

.palette-item {
  width: calc((100% - 20rpx) / 2);
}

.component-card {
  padding: 24rpx;
}

.component-chips {
  margin-top: 20rpx;
  margin-bottom: 20rpx;
}

.input-mock {
  margin-top: 22rpx;
  padding: 20rpx;
  background: var(--nb-lavender);
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
}

.input-label {
  display: block;
  font-size: 22rpx;
  line-height: 1.2;
  font-weight: 900;
  color: var(--nb-ink);
  margin-bottom: 10rpx;
}

.input-value {
  display: block;
  font-size: 28rpx;
  line-height: 1.2;
  font-weight: 900;
  color: var(--nb-ink);
}

.scene-card {
  min-height: 260rpx;
}

.control-scene {
  background: var(--nb-blue);
}

.ecosystem-scene {
  background: var(--nb-green);
}
</style>
