<template>
  <view class="glx-stepper">
    <view
      class="glx-stepper__button"
      :class="{ 'glx-stepper__button--disabled': isDecreaseDisabled }"
      @click="handleAdjust(-step)"
    >
      <text class="glx-stepper__button-text">{{ decreaseLabel }}</text>
    </view>
    <view class="glx-stepper__value">
      <text class="glx-stepper__value-text">{{ value }}</text>
    </view>
    <view
      class="glx-stepper__button"
      :class="{ 'glx-stepper__button--disabled': isIncreaseDisabled }"
      @click="handleAdjust(step)"
    >
      <text class="glx-stepper__button-text">{{ increaseLabel }}</text>
    </view>
  </view>
</template>

<script>
export default {
  name: "GlxStepper",
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
    decreaseLabel: {
      type: String,
      default: "-",
    },
    increaseLabel: {
      type: String,
      default: "+",
    },
  },
  computed: {
    isDecreaseDisabled() {
      return this.value <= this.min;
    },
    isIncreaseDisabled() {
      return this.value >= this.max;
    },
  },
  methods: {
    handleAdjust(delta) {
      const nextValue = Math.max(this.min, Math.min(this.max, this.value + delta));
      if (nextValue === this.value) {
        return;
      }
      this.$emit("change", {
        detail: {
          value: nextValue,
        },
      });
    },
  },
};
</script>

<style scoped>
.glx-stepper {
  display: flex;
  align-items: center;
  gap: 12rpx;
  margin-top: 10rpx;
}

.glx-stepper__button,
.glx-stepper__value {
  min-height: 60rpx;
  border: 2rpx solid var(--nb-ink);
  background: #ffffff;
  box-sizing: border-box;
}

.glx-stepper__button {
  min-width: 92rpx;
  padding: 0 16rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.glx-stepper__button--disabled {
  opacity: 0.35;
}

.glx-stepper__button-text {
  font-size: 24rpx;
  font-weight: 700;
  color: #000000;
}

.glx-stepper__value {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #fff7d1;
  min-width: 0;
}

.glx-stepper__value-text {
  font-size: 26rpx;
  font-weight: 700;
  color: #000000;
}
</style>
