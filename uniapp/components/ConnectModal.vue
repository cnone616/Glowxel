<template>
  <view v-if="visible" class="modal-overlay" @tap="handleCancel">
    <view class="modal-content" @tap.stop>
      <view class="modal-header">
        <text class="modal-title">{{ title }}</text>
        <view class="close-btn" @tap="handleCancel">
          <Icon name="close" :size="32" />
        </view>
      </view>

      <view class="modal-body">
        <text class="modal-desc">{{ description }}</text>

        <view class="input-wrapper">
          <input
            v-model="inputValue"
            type="text"
            class="custom-input"
            :placeholder="placeholder"
            placeholder-class="input-placeholder"
            :adjust-position="false"
            @confirm="handleConfirm"
          />
        </view>

        <!-- 连接状态 -->
        <view v-if="connecting" class="status-box connecting">
          <GlxInlineLoader
            class="status-loader"
            variant="chase"
            size="sm"
          />
          <view class="status-copy">
            <text class="status-text">正在连接设备</text>
            <text class="status-tip">请稍候，连接成功后会自动关闭弹窗</text>
          </view>
        </view>

        <view v-if="error" class="status-box error">
          <Icon name="close" :size="32" />
          <text class="status-text">{{ error }}</text>
        </view>
      </view>

      <view class="modal-footer">
        <view class="modal-btn cancel-btn" @tap="handleCancel">
          <text class="btn-text">取消</text>
        </view>
        <view
          class="modal-btn confirm-btn"
          :class="{ disabled: !inputValue || connecting }"
          @tap="handleConfirm"
        >
          <text class="btn-text">{{ connecting ? "连接中" : "连接" }}</text>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from "./Icon.vue";
import GlxInlineLoader from "./GlxInlineLoader.vue";
export default {
  components: {
    Icon,
    GlxInlineLoader,
  },

  props: {
    visible: {
      type: Boolean,
      default: false,
    },
    title: {
      type: String,
      default: "连接设备",
    },
    description: {
      type: String,
      default: "请输入 Glowxel PixelBoard 的 IP 地址",
    },
    placeholder: {
      type: String,
      default: "192.168.31.84",
    },
    defaultValue: {
      type: String,
      default: "",
    },
  },

  data() {
    return {
      inputValue: "",
      connecting: false,
      error: "",
      timeoutTimer: null,
    };
  },

  watch: {
    visible(val) {
      if (val) {
        this.inputValue = this.defaultValue;
        this.connecting = false;
        this.error = "";
        if (this.timeoutTimer) {
          clearTimeout(this.timeoutTimer);
          this.timeoutTimer = null;
        }
      }
    },

    inputValue() {
      // 清除错误提示
      if (this.error) {
        this.error = "";
      }
    },
  },

  methods: {
    handleConfirm() {
      if (!this.inputValue || this.connecting) return;

      this.connecting = true;
      this.error = "";

      // 首连现在会经过运行态预检 + 最多两次握手尝试，UI 超时需要覆盖整条链路
      this.timeoutTimer = setTimeout(() => {
        if (this.connecting) {
          this.connecting = false;
          this.error = "连接超时，请重试";
          this.$emit("timeout");
        }
      }, 40000);

      this.$emit("confirm", this.inputValue);
    },

    handleCancel() {
      if (this.connecting) return;

      if (this.timeoutTimer) {
        clearTimeout(this.timeoutTimer);
        this.timeoutTimer = null;
      }

      this.$emit("cancel");
      this.$emit("update:visible", false);

      // 直接关闭弹窗
      this.$parent.showConnectModal = false;
    },

    // 外部调用：连接成功
    onSuccess() {
      this.connecting = false;
      this.error = "";
      if (this.timeoutTimer) {
        clearTimeout(this.timeoutTimer);
        this.timeoutTimer = null;
      }
      this.$emit("update:visible", false);

      // 直接关闭弹窗
      if (this.$parent && this.$parent.showConnectModal !== undefined) {
        this.$parent.showConnectModal = false;
      }
    },

    // 外部调用：连接失败
    onError(message) {
      this.connecting = false;
      this.error = message || "连接失败，请检查 IP 地址";
      if (this.timeoutTimer) {
        clearTimeout(this.timeoutTimer);
        this.timeoutTimer = null;
      }
    },
  },
};
</script>

<style scoped>
.modal-content {
  width: 100%;
  max-width: 620rpx;
  background-color: var(--nb-surface);
  border-radius: 0;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
  overflow: hidden;
  animation: scaleIn 0.18s ease-out;
  box-sizing: border-box;
}

@keyframes scaleIn {
  from {
    transform: scale(0.9);
    opacity: 0;
  }
  to {
    transform: scale(1);
    opacity: 1;
  }
}

.modal-header {
  padding: 24rpx 24rpx 22rpx;
  border-bottom: var(--nb-border-width-panel) solid var(--nb-ink);
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.modal-title {
  font-size: 32rpx;
  font-weight: 900;
  color: var(--nb-ink);
}

.close-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  border-radius: 0;
  box-sizing: border-box;
}


.modal-body {
  padding: 24rpx;
  display: flex;
  flex-direction: column;
  gap: 18rpx;
}

.modal-desc {
  font-size: 24rpx;
  color: #4a4a4a;
  line-height: 1.6;
}

.input-wrapper {
  display: flex;
  flex-direction: column;
}

.custom-input {
  width: 100%;
  padding: 0 20rpx;
  background-color: #ffffff;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  border-radius: 0;
  font-size: 30rpx;
  color: var(--nb-ink);
  font-family: monospace;
  transition: var(--transition-base);
  box-sizing: border-box;
  line-height: 1.2;
  height: 84rpx;
}

.custom-input:focus {
  border-color: var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.input-placeholder {
  color: var(--text-tertiary);
}

.status-box {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 18rpx;
  border-radius: 0;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  animation: fadeIn 0.3s ease-out;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(-10rpx);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.status-box.connecting {
  align-items: flex-start;
  background-color: #e7f0ff;
}

.status-loader {
  flex-shrink: 0;
  margin-top: 2rpx;
}

.status-box.error {
  background-color: #f8dede;
}

.status-copy {
  display: flex;
  flex-direction: column;
  gap: 6rpx;
  min-width: 0;
}

.status-text {
  font-size: 24rpx;
  font-weight: 800;
  line-height: 1.4;
}

.status-tip {
  font-size: 22rpx;
  line-height: 1.5;
  color: #4a4a4a;
}

.status-box.connecting .status-text {
  color: var(--nb-ink);
}

.status-box.connecting .status-tip {
  color: #4a4a4a;
}

.status-box.error .status-text {
  color: #ff6464;
}

.modal-footer {
  display: flex;
  gap: 12rpx;
  padding: 18rpx 24rpx 24rpx;
  border-top: 3rpx solid var(--nb-ink);
}

.modal-btn {
  flex: 1;
  min-height: 84rpx;
  padding: 0 20rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}


.modal-btn.disabled {
  opacity: 0.5;
}

.cancel-btn {
  background: #ffffff;
}

.btn-text {
  font-size: 28rpx;
  font-weight: 900;
}

.cancel-btn .btn-text {
  color: var(--nb-ink);
}

.confirm-btn {
  background: #f2cf4a;
}

.confirm-btn .btn-text {
  color: var(--nb-ink);
}
</style>
