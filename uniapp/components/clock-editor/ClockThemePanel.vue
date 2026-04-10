<template>
  <view class="settings-card">
    <view class="card-title-section">
      <Icon name="picture" :size="32" />
      <text class="card-title">主题库</text>
      <text class="card-count">{{ presets.length }} 个主题</text>
    </view>

    <view class="theme-sections">
      <view class="theme-grid">
        <view
          v-for="preset in presets"
          :key="preset.id"
          class="theme-card glx-feature-card-option"
          :class="{ active: selectedThemeId === preset.id }"
          @click="$emit('apply-theme', preset.id)"
        >
          <view class="theme-preview-shell">
            <image
              :src="preset.previewImage"
              class="theme-preview-image"
              mode="aspectFill"
            />
          </view>
          <view class="theme-meta">
            <text class="theme-name">{{ preset.name }}</text>
            <text v-if="preset.styleTag" class="theme-tag">{{ preset.styleTag }}</text>
          </view>
          <view v-if="currentThemeId === preset.id" class="theme-badge">
            <view class="theme-badge-text">当前</view>
          </view>
          <view
            v-if="preset.requiresImage"
            class="theme-image-badge"
            :class="{ warn: selectedThemeId === preset.id && !hasImage }"
          >
            <view class="theme-image-badge-text">图片</view>
          </view>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from "../Icon.vue";

export default {
  components: {
    Icon,
  },
  props: {
    presets: {
      type: Array,
      required: true,
    },
    selectedThemeId: {
      type: String,
      default: "",
    },
    currentThemeId: {
      type: String,
      default: "",
    },
    activePreset: {
      type: Object,
      default: null,
    },
    hasImage: {
      type: Boolean,
      default: false,
    },
    isModified: {
      type: Boolean,
      default: false,
    },
  },
  emits: ["apply-theme", "open-image-settings"],
};
</script>

<style scoped>
.settings-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  padding: 20rpx;
  margin-bottom: 16rpx;
}

.card-title-section {
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.card-count {
  margin-left: auto;
  min-height: 40rpx;
  padding: 0 12rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid #000000;
  font-size: 18rpx;
  font-weight: 700;
  color: #000000;
}

.theme-sections {
  margin-top: 18rpx;
}

.theme-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 14rpx;
}

.theme-card {
  position: relative;
  width: calc((100% - 28rpx) / 3);
  overflow: hidden;
}

.theme-card.active {
  box-shadow: none;
}

.theme-preview-shell {
  position: relative;
  width: 100%;
  aspect-ratio: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 8rpx;
  box-sizing: border-box;
  background: #000000;
}

.theme-preview-image {
  width: 100%;
  height: 100%;
  display: block;
  background: #000000;
  image-rendering: pixelated;
}

.theme-meta {
  padding: 12rpx 12rpx 14rpx;
}

.theme-name {
  display: block;
  font-size: 22rpx;
  font-weight: 600;
  line-height: 1.35;
  color: var(--text-primary);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  text-align: center;
}

.theme-tag {
  display: block;
  margin-top: 6rpx;
  font-size: 18rpx;
  line-height: 1.3;
  color: var(--text-secondary);
  text-align: center;
}

.theme-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 38rpx;
  min-width: 58rpx;
  padding: 6rpx 12rpx;
  border-radius: 0;
  background: var(--nb-yellow);
  border: 2rpx solid #000000;
}

.theme-badge-text {
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18rpx;
  font-weight: 600;
  line-height: 1;
  color: #000000;
}

.theme-image-badge {
  position: absolute;
  left: 12rpx;
  top: 12rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 38rpx;
  min-width: 58rpx;
  padding: 6rpx 10rpx;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid #000000;
}

.theme-image-badge.warn {
  background: var(--nb-yellow);
}

.theme-image-badge-text {
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18rpx;
  font-weight: 600;
  line-height: 1;
  color: #000000;
}
</style>
