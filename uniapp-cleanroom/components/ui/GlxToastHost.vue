<template>
  <view v-if="visible" class="glx-toast" :class="toneClass">
    <GlxIcon :name="iconName" :size="26" />
    <text class="glx-toast__text">{{ message }}</text>
  </view>
</template>

<script>
import GlxIcon from "./GlxIcon.vue";

export default {
  name: "GlxToastHost",
  components: {
    GlxIcon,
  },
  data() {
    return {
      visible: false,
      message: "",
      tone: "primary",
      timerId: null,
    };
  },
  computed: {
    toneClass() {
      return `glx-toast--${this.tone}`;
    },
    iconName() {
      if (this.tone === "danger") {
        return "close";
      }
      if (this.tone === "edit") {
        return "edit";
      }
      return "success";
    },
  },
  methods: {
    show({ message, tone = "primary", duration = 1800 }) {
      this.visible = true;
      this.message = message;
      this.tone = tone;
      if (this.timerId) {
        clearTimeout(this.timerId);
      }
      this.timerId = setTimeout(() => {
        this.visible = false;
        this.timerId = null;
      }, duration);
    },
  },
};
</script>

<style scoped lang="scss">
.glx-toast {
  position: fixed;
  left: 24rpx;
  right: 24rpx;
  bottom: 210rpx;
  z-index: 60;
  min-height: 88rpx;
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 0 24rpx;
  border-radius: 24rpx;
  border: 2rpx solid transparent;
  box-shadow: 0 20rpx 48rpx rgba(20, 50, 54, 0.12);
}

.glx-toast__text {
  color: inherit;
  font-size: 24rpx;
  font-weight: 700;
  line-height: 1.4;
}

.glx-toast--primary {
  background: #ffffff;
  border-color: rgba(46, 170, 150, 0.24);
  color: var(--glx-text);
}

.glx-toast--edit {
  background: rgba(74, 134, 239, 0.12);
  border-color: rgba(74, 134, 239, 0.18);
  color: var(--glx-blue-deep);
}

.glx-toast--danger {
  background: rgba(232, 95, 103, 0.12);
  border-color: rgba(207, 73, 84, 0.18);
  color: var(--glx-danger-deep);
}
</style>
