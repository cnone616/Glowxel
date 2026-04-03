<template>
  <view class="settings-card">
    <view class="card-title-section">
      <Icon :name="iconName" :size="32" />
      <text class="card-title">{{ title }}</text>
      <view class="toggle-switch" @click="$emit('toggle')">
        <view class="switch-track" :class="{ active: section.show }">
          <view class="switch-thumb"></view>
        </view>
      </view>
    </view>

    <view v-if="section.show" class="setting-group">
      <view v-if="showFontSize" class="setting-item-row">
        <text class="setting-label">字体大小</text>
        <view class="setting-control-buttons">
          <view
            class="control-btn"
            @click="$emit('adjust', 'fontSize', -1, minFontSize, maxFontSize)"
          >
            <text class="control-icon">-</text>
          </view>
          <text class="setting-value-large">{{ section.fontSize }}</text>
          <view
            class="control-btn"
            @click="$emit('adjust', 'fontSize', 1, minFontSize, maxFontSize)"
          >
            <text class="control-icon">+</text>
          </view>
        </view>
      </view>

      <view class="setting-item-row">
        <text class="setting-label">X 位置</text>
        <view class="setting-control-buttons">
          <view class="control-btn" @click="$emit('adjust', 'x', -1, 0, 64)">
            <text class="control-icon">-</text>
          </view>
          <text class="setting-value-large">{{ section.x }}</text>
          <view class="control-btn" @click="$emit('adjust', 'x', 1, 0, 64)">
            <text class="control-icon">+</text>
          </view>
        </view>
      </view>

      <view class="setting-item-row">
        <text class="setting-label">Y 位置</text>
        <view class="setting-control-buttons">
          <view class="control-btn" @click="$emit('adjust', 'y', -1, 0, 64)">
            <text class="control-icon">-</text>
          </view>
          <text class="setting-value-large">{{ section.y }}</text>
          <view class="control-btn" @click="$emit('adjust', 'y', 1, 0, 64)">
            <text class="control-icon">+</text>
          </view>
        </view>
      </view>

      <view class="setting-item">
        <text class="setting-label">颜色</text>
        <ColorPanelPicker
          :value="section.color"
          :label="`${title}颜色`"
          :preset-colors="presetColors"
          @input="$emit('update-color', $event)"
        />
      </view>

      <view class="setting-item">
        <text class="setting-label">对齐方式</text>
        <view class="align-buttons">
          <view
            class="align-btn"
            :class="{ active: section.align === 'left' }"
            @click="$emit('set-align', 'left')"
          >
            <text class="align-text">左对齐</text>
          </view>
          <view
            class="align-btn"
            :class="{ active: section.align === 'center' }"
            @click="$emit('set-align', 'center')"
          >
            <text class="align-text">居中</text>
          </view>
          <view
            class="align-btn"
            :class="{ active: section.align === 'right' }"
            @click="$emit('set-align', 'right')"
          >
            <text class="align-text">右对齐</text>
          </view>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from "../Icon.vue";
import ColorPanelPicker from "../ColorPanelPicker.vue";

export default {
  components: {
    Icon,
    ColorPanelPicker,
  },
  props: {
    iconName: {
      type: String,
      required: true,
    },
    title: {
      type: String,
      required: true,
    },
    section: {
      type: Object,
      required: true,
    },
    presetColors: {
      type: Array,
      required: true,
    },
    showFontSize: {
      type: Boolean,
      default: true,
    },
    minFontSize: {
      type: Number,
      default: 1,
    },
    maxFontSize: {
      type: Number,
      default: 3,
    },
  },
  emits: ["toggle", "adjust", "update-color", "set-align"],
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
  margin-bottom: 16rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
  flex: 1;
}

.setting-group {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.setting-item {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.setting-item-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.setting-label {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.setting-control-buttons {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.control-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.control-btn:active {
  transform: scale(0.9);
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
}

.control-icon {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.setting-value-large {
  font-size: 32rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--accent-primary);
  min-width: 64rpx;
  text-align: center;
}

.align-buttons {
  display: flex;
  gap: 12rpx;
  margin-top: 12rpx;
}

.align-btn {
  flex: 1;
  padding: 20rpx 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  text-align: center;
  transition: var(--transition-base);
}

.align-btn:active {
  transform: scale(0.98);
}

.align-btn.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
}

.align-text {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.align-btn.active .align-text {
  color: var(--accent-primary);
  font-weight: 500;
}

.toggle-switch {
  margin-left: auto;
}

.switch-track {
  width: 80rpx;
  height: 44rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 22rpx;
  position: relative;
  transition: var(--transition-base);
}

.switch-track.active {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.switch-thumb {
  width: 36rpx;
  height: 36rpx;
  background-color: var(--text-primary);
  border-radius: 18rpx;
  position: absolute;
  top: 2rpx;
  left: 2rpx;
  transition: var(--transition-base);
}

.switch-track.active .switch-thumb {
  left: 38rpx;
  background-color: #ffffff;
}
</style>
