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
          class="theme-card"
          :class="{ active: activeThemeId === preset.id }"
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
          <view v-if="activeThemeId === preset.id" class="theme-badge">
            <text class="theme-badge-text">当前</text>
          </view>
          <view
            v-if="preset.requiresImage"
            class="theme-image-badge"
            :class="{ warn: activeThemeId === preset.id && !hasImage }"
          >
            <text class="theme-image-badge-text">图片</text>
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
    activeThemeId: {
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
  border: 2rpx solid var(--border-primary);
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
  padding: 4rpx 12rpx;
  border-radius: 999rpx;
  background: rgba(79, 127, 255, 0.12);
  font-size: 20rpx;
  color: var(--accent-primary);
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
  border-radius: 16rpx;
  overflow: hidden;
  border: 2rpx solid var(--border-primary);
  background: var(--bg-secondary);
}

.theme-card.active {
  border-color: var(--accent-primary);
  box-shadow: 0 0 0 2rpx rgba(79, 127, 255, 0.1);
}

.theme-preview-shell {
  position: relative;
  width: 100%;
  aspect-ratio: 1;
  background: #000000;
}

.theme-preview-image {
  width: 100%;
  height: 100%;
  display: block;
  background: #000000;
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
  top: 10rpx;
  right: 10rpx;
  padding: 6rpx 12rpx;
  border-radius: 999rpx;
  background: rgba(79, 127, 255, 0.92);
}

.theme-badge-text {
  font-size: 18rpx;
  font-weight: 600;
  color: #ffffff;
}

.theme-image-badge {
  position: absolute;
  left: 10rpx;
  top: 10rpx;
  padding: 6rpx 10rpx;
  border-radius: 999rpx;
  background: rgba(16, 16, 16, 0.72);
}

.theme-image-badge.warn {
  background: rgba(255, 140, 64, 0.92);
}

.theme-image-badge-text {
  font-size: 18rpx;
  font-weight: 600;
  color: #ffffff;
}
</style>
