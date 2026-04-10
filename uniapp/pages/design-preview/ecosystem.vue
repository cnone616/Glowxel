<template>
  <view class="nb-page nb-ecosystem-preview glx-scroll-region">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="nb-navbar glx-topbar">
      <view class="nb-nav-action" @click="goBack">
        <Icon name="arrow-left-bold" :size="28" color="#000000" />
      </view>
      <view class="nb-nav-title-wrap">
        <text class="nb-nav-title">主题生态预览</text>
      </view>
      <view class="nb-nav-spacer"></view>
    </view>

    <scroll-view scroll-y class="nb-scroll">
      <view class="nb-content">
        <view class="nb-card nb-card-lg ecosystem-hero">
          <text class="nb-eyebrow">预览系统</text>
          <text class="nb-display ecosystem-title">主题缩略图、成就卡、素材卡都能统一到同一套语法里。</text>
          <text class="nb-body ecosystem-desc">
            这一页重点看“高娱乐性内容”怎么统一风格，而不是继续做普通 app 卡片。缩略图保持方形，卡片统一黑边和硬阴影，设置区再适当降躁。
          </text>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">主题缩略图</text>
            <text class="nb-section-meta">正方形缩略图 + 粗边框 + 强标签</text>
          </view>
          <view class="theme-grid">
            <view v-for="theme in themes" :key="theme.name" class="theme-card">
              <image class="theme-cover" :src="theme.cover" mode="aspectFill" />
              <view class="theme-body" :style="{ background: theme.color }">
                <text class="theme-name">{{ theme.name }}</text>
                <text class="theme-meta">{{ theme.meta }}</text>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">成就封面感</text>
            <text class="nb-section-meta">不是普通列表，要像可收藏的荣誉卡</text>
          </view>
          <view class="achievement-row">
            <view
              v-for="achievement in achievements"
              :key="achievement.title"
              class="achievement-card"
              :style="{ background: achievement.color }"
            >
              <view class="achievement-badge">{{ achievement.badge }}</view>
              <text class="achievement-title">{{ achievement.title }}</text>
              <text class="achievement-desc">{{ achievement.desc }}</text>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">素材 / 快捷选择</text>
            <text class="nb-section-meta">让用户一眼就知道这是能直接用的</text>
          </view>
          <view class="asset-grid">
            <view v-for="asset in assets" :key="asset.name" class="asset-card">
              <view class="asset-thumb" :style="{ background: asset.color }">
                <Icon :name="asset.icon" :size="36" color="#000000" />
              </view>
              <view class="asset-copy">
                <text class="asset-name">{{ asset.name }}</text>
                <text class="asset-desc">{{ asset.desc }}</text>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">低噪设置块</text>
            <text class="nb-section-meta">全局铺满，但不是每块都要抢戏</text>
          </view>
          <view class="nb-card quiet-panel">
            <view v-for="item in quietSettings" :key="item.name" class="quiet-row">
              <text class="quiet-name">{{ item.name }}</text>
              <text class="quiet-value">{{ item.value }}</text>
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
      themes: [
        {
          name: "世界地图时钟",
          meta: "世界地图",
          cover: "/static/clockwise-themes/cw-cf-0x03-worldmap.png",
          color: "#74B9FF",
        },
        {
          name: "高对比时钟",
          meta: "高对比时钟",
          cover: "/static/clockwise-themes/cw-cf-0x04-clock-high-contrast.png",
          color: "#FFD23F",
        },
        {
          name: "像素经典",
          meta: "像素主题",
          cover: "/static/clockwise-themes/cw-cf-0x01-thumb.jpg",
          color: "#88D498",
        },
        {
          name: "街机亮色",
          meta: "街机亮色",
          cover: "/static/clockwise-themes/cw-cf-0x05-thumb.jpg",
          color: "#FF6B6B",
        },
      ],
      achievements: [
        {
          badge: "S级",
          title: "主题收藏家",
          desc: "连续 14 天切换不同主题并保持设备在线。",
          color: "#FFD23F",
        },
        {
          badge: "限定",
          title: "像素驯服者",
          desc: "完成 52×52 拼豆区域的完整作品并发送到设备。",
          color: "#B8A9FA",
        },
        {
          badge: "稀有",
          title: "夜航者",
          desc: "连续 7 次使用氛围与主题组合完成展示。",
          color: "#74B9FF",
        },
      ],
      assets: [
        { name: "像素海洋馆", desc: "更适合展示设备休闲场景", icon: "picture-filling", color: "#74B9FF" },
        { name: "星空慢动效", desc: "低频循环，适合夜晚展示", icon: "dynamic-filling", color: "#B8A9FA" },
        { name: "庆祝烟花", desc: "节奏更强，适合成就和提醒", icon: "notification-filling", color: "#FF6B6B" },
        { name: "像素山水", desc: "静态展示更高级，适合默认封面", icon: "layout-filling", color: "#88D498" },
      ],
      quietSettings: [
        { name: "封面比例", value: "1:1" },
        { name: "卡片圆角", value: "0" },
        { name: "默认描边", value: "黑色粗边" },
      ],
    };
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

.ecosystem-hero {
  padding: 28rpx;
  background: var(--nb-blue);
}

.ecosystem-title {
  display: block;
  margin-top: 24rpx;
  margin-bottom: 26rpx;
}

.ecosystem-desc {
  display: block;
}

.theme-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 20rpx;
}

.theme-card {
  width: calc((100% - 20rpx) / 2);
  background: var(--nb-surface);
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-md);
  box-sizing: border-box;
}

.theme-cover {
  width: 100%;
  height: 280rpx;
  display: block;
  border-bottom: var(--nb-border);
}

.theme-body {
  min-height: 124rpx;
  padding: 18rpx;
  box-sizing: border-box;
}

.theme-name {
  display: block;
  font-size: 28rpx;
  line-height: 1.15;
  font-weight: 900;
  color: var(--nb-ink);
  margin-bottom: 8rpx;
}

.theme-meta {
  display: block;
  font-size: 22rpx;
  line-height: 1.2;
  font-weight: 800;
  color: rgba(0, 0, 0, 0.72);
}

.achievement-row {
  display: flex;
  flex-direction: column;
  gap: 18rpx;
}

.achievement-card {
  padding: 22rpx;
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-md);
  box-sizing: border-box;
}

.achievement-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 8rpx 16rpx;
  background: var(--nb-surface);
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
  font-size: 20rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
  margin-bottom: 16rpx;
}

.achievement-title {
  display: block;
  font-size: 32rpx;
  line-height: 1.08;
  font-weight: 900;
  color: var(--nb-ink);
  margin-bottom: 10rpx;
}

.achievement-desc {
  display: block;
  font-size: 24rpx;
  line-height: 1.5;
  color: var(--nb-text-body);
}

.asset-grid {
  display: flex;
  flex-direction: column;
  gap: 18rpx;
}

.asset-card {
  display: flex;
  align-items: stretch;
  gap: 18rpx;
  padding: 18rpx;
  background: var(--nb-surface);
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
  box-sizing: border-box;
}

.asset-thumb {
  width: 108rpx;
  height: 108rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
  box-sizing: border-box;
  flex-shrink: 0;
}

.asset-copy {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  justify-content: center;
}

.asset-name {
  display: block;
  font-size: 28rpx;
  line-height: 1.15;
  font-weight: 900;
  color: var(--nb-ink);
  margin-bottom: 8rpx;
}

.asset-desc {
  display: block;
  font-size: 22rpx;
  line-height: 1.45;
  color: var(--nb-text-muted);
}

.quiet-panel {
  padding: 10rpx 20rpx;
  background: var(--nb-surface);
}

.quiet-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 18rpx 0;
  border-bottom: 4rpx solid var(--nb-ink);
}

.quiet-row:last-child {
  border-bottom: 0;
}

.quiet-name {
  font-size: 26rpx;
  line-height: 1.2;
  font-weight: 900;
  color: var(--nb-ink);
}

.quiet-value {
  font-size: 22rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
  background: var(--nb-yellow);
  padding: 10rpx 14rpx;
  border: var(--nb-border);
  box-shadow: var(--nb-shadow-sm);
}
</style>
