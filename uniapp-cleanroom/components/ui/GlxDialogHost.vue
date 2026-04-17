<template>
  <view v-if="visible" class="glx-dialog-host">
    <view class="glx-dialog-host__mask" @click="handleCancel"></view>
    <view class="glx-dialog-host__panel">
      <text class="glx-dialog-host__title">{{ title }}</text>
      <text class="glx-dialog-host__description">{{ description }}</text>
      <view class="glx-dialog-host__actions">
        <GlxActionButton label="取消" tone="secondary" @click="handleCancel" />
        <GlxActionButton
          :label="confirmLabel"
          :tone="confirmTone"
          @click="handleConfirm"
        />
      </view>
    </view>
  </view>
</template>

<script>
import GlxActionButton from "./GlxActionButton.vue";

export default {
  name: "GlxDialogHost",
  components: {
    GlxActionButton,
  },
  data() {
    return {
      visible: false,
      title: "",
      description: "",
      confirmLabel: "确认",
      confirmTone: "primary",
      onConfirm: null,
    };
  },
  methods: {
    open(options) {
      this.visible = true;
      this.title = options.title;
      this.description = options.description;
      this.confirmLabel = options.confirmLabel || "确认";
      this.confirmTone = options.confirmTone || "primary";
      this.onConfirm = options.onConfirm || null;
    },
    handleCancel() {
      this.visible = false;
    },
    handleConfirm() {
      if (typeof this.onConfirm === "function") {
        this.onConfirm();
      }
      this.visible = false;
    },
  },
};
</script>

<style scoped lang="scss">
.glx-dialog-host {
  position: fixed;
  inset: 0;
  z-index: 70;
  display: flex;
  align-items: flex-end;
  justify-content: center;
}

.glx-dialog-host__mask {
  position: absolute;
  inset: 0;
  background: rgba(20, 50, 54, 0.18);
}

.glx-dialog-host__panel {
  position: relative;
  width: 100%;
  padding: 34rpx 24rpx 28rpx;
  border-top-left-radius: 34rpx;
  border-top-right-radius: 34rpx;
  background: #ffffff;
  box-shadow: 0 -18rpx 56rpx rgba(20, 50, 54, 0.12);
}

.glx-dialog-host__title {
  display: block;
  color: var(--glx-text);
  font-size: 30rpx;
  font-weight: 800;
  line-height: 1.2;
}

.glx-dialog-host__description {
  display: block;
  margin-top: 14rpx;
  color: var(--glx-text-secondary);
  font-size: 24rpx;
  line-height: 1.6;
}

.glx-dialog-host__actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 16rpx;
  margin-top: 28rpx;
}
</style>
