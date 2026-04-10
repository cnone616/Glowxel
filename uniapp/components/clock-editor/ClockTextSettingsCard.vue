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
        <text class="setting-label">{{ fontSizeLabel }}</text>
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
        <text class="setting-label">{{ xLabel }}</text>
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
        <text class="setting-label">{{ yLabel }}</text>
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

      <view v-if="showAlign" class="setting-item">
        <text class="setting-label">对齐方式</text>
        <view class="align-buttons">
          <view
            class="align-btn glx-feature-option"
            :class="{ active: section.align === 'left' }"
            @click="$emit('set-align', 'left')"
          >
            <text class="align-text">左对齐</text>
          </view>
          <view
            class="align-btn glx-feature-option"
            :class="{ active: section.align === 'center' }"
            @click="$emit('set-align', 'center')"
          >
            <text class="align-text">居中</text>
          </view>
          <view
            class="align-btn glx-feature-option"
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
    showAlign: {
      type: Boolean,
      default: true,
    },
    fontSizeLabel: {
      type: String,
      default: "字体大小",
    },
    xLabel: {
      type: String,
      default: "X 位置",
    },
    yLabel: {
      type: String,
      default: "Y 位置",
    },
  },
  emits: ["toggle", "adjust", "update-color", "set-align"],
};
</script>

<style scoped>
.settings-card {
  background-color: transparent;
  border: 0;
  padding: 0;
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
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: var(--transition-base);
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
  color: var(--nb-yellow);
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
  text-align: center;
  transition: var(--transition-base);
}

.align-text {
  font-size: 22rpx;
}

.toggle-switch {
  margin-left: auto;
}

.switch-track {
  width: 80rpx;
  height: 44rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  position: relative;
  transition: var(--transition-base);
}

.switch-track.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.switch-thumb {
  width: 36rpx;
  height: 36rpx;
  background-color: var(--text-primary);
  border-radius: 0;
  position: absolute;
  top: 2rpx;
  left: 2rpx;
  transition: var(--transition-base);
}

.switch-track.active .switch-thumb {
  left: 38rpx;
  background-color: #000000;
}
</style>
