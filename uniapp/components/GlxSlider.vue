<template>
  <view class="glx-slider">
    <view class="glx-slider__track"></view>
    <view class="glx-slider__fill" :style="{ width: `${percent}%` }"></view>
    <view class="glx-slider__thumb" :style="{ left: `calc(${percent}% - 18rpx)` }"></view>
    <slider
      class="glx-slider__native"
      :value="value"
      :min="min"
      :max="max"
      :step="step"
      :activeColor="nativeActiveColor"
      :backgroundColor="nativeBackgroundColor"
      :block-size="1"
      @change="$emit('change', $event)"
      @changing="$emit('changing', $event)"
    />
  </view>
</template>

<script>
export default {
  name: "GlxSlider",
  props: {
    value: {
      type: Number,
      default: 0,
    },
    min: {
      type: Number,
      default: 0,
    },
    max: {
      type: Number,
      default: 100,
    },
    step: {
      type: Number,
      default: 1,
    },
    nativeActiveColor: {
      type: String,
      default: "rgba(0,0,0,0)",
    },
    nativeBackgroundColor: {
      type: String,
      default: "rgba(0,0,0,0)",
    },
  },
  computed: {
    percent() {
      const range = this.max - this.min;
      if (range <= 0) {
        return 0;
      }
      return ((this.value - this.min) / range) * 100;
    },
  },
};
</script>

<style scoped>
.glx-slider {
  position: relative;
  height: 52rpx;
}

.glx-slider__track,
.glx-slider__fill {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  height: 10rpx;
  box-sizing: border-box;
}

.glx-slider__track {
  left: 0;
  right: 0;
  background: #ffffff;
  border: 2rpx solid #000000;
}

.glx-slider__fill {
  left: 0;
  background: #ffd23f;
  border: 2rpx solid #000000;
  border-right: 0;
}

.glx-slider__thumb {
  position: absolute;
  top: 50%;
  width: 36rpx;
  height: 36rpx;
  transform: translateY(-50%);
  background: #ffd23f;
  border: 2rpx solid #000000;
  box-sizing: border-box;
}

.glx-slider__native {
  position: absolute;
  inset: 0;
  opacity: 0;
}
</style>
