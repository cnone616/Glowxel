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
      @change="handleChange"
      @changing="handleChanging"
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
  data() {
    return {
      displayValue: 0,
    };
  },
  watch: {
    value: {
      immediate: true,
      handler(newValue) {
        this.displayValue = this.normalizeValue(newValue);
      },
    },
  },
  computed: {
    percent() {
      const range = this.max - this.min;
      if (range <= 0) {
        return 0;
      }
      return ((this.displayValue - this.min) / range) * 100;
    },
  },
  methods: {
    normalizeValue(value) {
      if (typeof value !== "number" || Number.isNaN(value)) {
        return this.min;
      }
      if (value < this.min) {
        return this.min;
      }
      if (value > this.max) {
        return this.max;
      }
      return value;
    },
    handleChanging(event) {
      const nextValue =
        event && event.detail ? this.normalizeValue(event.detail.value) : this.min;
      this.displayValue = nextValue;
      this.$emit("changing", event);
    },
    handleChange(event) {
      const nextValue =
        event && event.detail ? this.normalizeValue(event.detail.value) : this.min;
      this.displayValue = nextValue;
      this.$emit("change", event);
    },
  },
};
</script>

<style scoped>
.glx-slider {
  position: relative;
  height: 60rpx;
  padding: 0 18rpx;
  box-sizing: border-box;
}

.glx-slider__track,
.glx-slider__fill {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  height: 14rpx;
  box-sizing: border-box;
}

.glx-slider__track {
  left: 18rpx;
  right: 18rpx;
  background: #ffffff;
  border: 2rpx solid #000000;
  box-shadow: 3rpx 3rpx 0 #000000;
}

.glx-slider__fill {
  left: 18rpx;
  background: #ffd23f;
  border: 2rpx solid #000000;
  border-right: 0;
  transition: width 0.06s linear;
}

.glx-slider__thumb {
  position: absolute;
  top: 50%;
  width: 44rpx;
  height: 44rpx;
  transform: translateY(-50%);
  background: #ffd23f;
  border: 2rpx solid #000000;
  box-shadow: 3rpx 3rpx 0 #000000;
  box-sizing: border-box;
  transition: left 0.06s linear;
}

.glx-slider__native {
  position: absolute;
  top: 0;
  right: 18rpx;
  bottom: 0;
  left: 18rpx;
  opacity: 0;
}
</style>
