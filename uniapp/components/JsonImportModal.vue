<template>
  <view v-if="visible" class="modal-overlay" @click="handleOverlayClick">
    <view class="modal-container" @click.stop>
      <view class="modal-header">
        <text class="modal-title">{{ title }}</text>
        <view class="close-btn" :class="{ 'disabled': sending }" @click="handleCancel">
          <text class="close-icon">×</text>
        </view>
      </view>
      
      <view class="modal-body">
        <text class="modal-description">{{ description }}</text>

        <view class="input-wrapper">
          <textarea
            v-model="inputValue"
            :placeholder="placeholder"
            class="json-input"
            :placeholder-style="placeholderStyle"
            :maxlength="-1"
            :disabled="sending"
            auto-height
          />
        </view>

        <view v-if="sending" class="status-box sending">
          <GlxInlineLoader class="loading-spinner" variant="chase" size="xs" />
          <text class="status-text">传输中...</text>
        </view>
      </view>
      
      <view class="modal-footer">
        <view class="modal-btn cancel-btn" :class="{ 'disabled': sending }" @click="handleCancel">
          <text class="btn-text">取消</text>
        </view>
        <view class="modal-btn confirm-btn" :class="{ 'disabled': sending }" @click="handleConfirm">
          <text class="btn-text">{{ sending ? '传输中...' : '确定' }}</text>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import GlxInlineLoader from "./GlxInlineLoader.vue";

export default {
  components: {
    GlxInlineLoader,
  },

  props: {
    visible: {
      type: Boolean,
      default: false
    },
    title: {
      type: String,
      default: '导入配置'
    },
    description: {
      type: String,
      default: '请粘贴 JSON 配置数据'
    },
    placeholder: {
      type: String,
      default: '粘贴 JSON 数据...'
    },
    sending: {
      type: Boolean,
      default: false
    }
  },
  
  data() {
    return {
      inputValue: '',
      placeholderStyle: 'color: #8a8a8a; font-size: 24rpx;'
    }
  },
  
  methods: {
    handleOverlayClick() {
      // 传输中不允许点击遮罩关闭
      if (this.sending) return
      this.handleCancel()
    },
    
    handleCancel() {
      // 传输中不允许取消
      if (this.sending) return
      
      this.inputValue = ''
      this.$emit('update:visible', false)
      this.$emit('cancel')
    },
    
    handleConfirm() {
      // 传输中不允许重复点击
      if (this.sending) return
      
      if (!this.inputValue.trim()) {
        this.$emit('error', '请输入 JSON 数据')
        return
      }
      
      try {
        const jsonData = JSON.parse(this.inputValue.trim())
        this.$emit('confirm', jsonData)
      } catch (e) {
        this.$emit('error', 'JSON 格式错误：' + e.message)
      }
    }
  },
  
  watch: {
    visible(newVal) {
      // 当弹窗关闭时，清空输入
      if (!newVal) {
        this.inputValue = ''
      }
    }
  }
}
</script>

<style scoped>
.close-btn.disabled,
.modal-btn.disabled {
  opacity: 0.5;
  pointer-events: none;
}

.json-input:disabled {
  opacity: 0.6;
}

.status-box {
  margin-top: 20rpx;
  padding: 18rpx 20rpx;
  display: flex;
  align-items: center;
  gap: 12rpx;
  border-radius: 0;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  animation: fadeIn 0.3s ease-out;
  box-sizing: border-box;
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

.status-box.sending {
  background-color: #e7f0ff;
}

.loading-spinner {
  flex-shrink: 0;
}

.status-text {
  font-size: 24rpx;
  font-weight: 800;
  color: var(--nb-ink);
}

.modal-container {
  width: 100%;
  max-width: 680rpx;
  background-color: var(--nb-surface);
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  border-radius: 0;
  box-shadow: var(--nb-shadow-strong);
  overflow: hidden;
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx;
  border-bottom: var(--nb-border-width-panel) solid var(--nb-ink);
}

.modal-title {
  font-size: 32rpx;
  font-weight: 900;
  color: var(--nb-ink);
}

.close-btn {
  width: 60rpx;
  height: 60rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  border-radius: 0;
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  box-sizing: border-box;
}


.close-icon {
  font-size: 40rpx;
  color: var(--nb-ink);
  line-height: 1;
  font-weight: 900;
}

.modal-body {
  padding: 24rpx;
}

.modal-description {
  font-size: 24rpx;
  color: #4a4a4a;
  margin-bottom: 24rpx;
  display: block;
}

.input-wrapper {
  background-color: #ffffff;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  border-radius: 0;
  padding: 18rpx;
}

.input-wrapper:focus-within {
  border-color: var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.json-input {
  width: 100%;
  min-height: 280rpx;
  max-height: 600rpx;
  font-size: 24rpx;
  color: var(--nb-ink);
  background-color: transparent;
  font-family: 'Courier New', monospace;
  line-height: 1.6;
}

.modal-footer {
  display: flex;
  gap: 16rpx;
  padding: 18rpx 24rpx 24rpx;
  border-top: var(--nb-border-width-panel) solid var(--nb-ink);
}

.modal-btn {
  flex: 1;
  min-height: 84rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  box-sizing: border-box;
}


.cancel-btn {
  background-color: var(--nb-surface);
}


.confirm-btn {
  background-color: #f2cf4a;
  border-color: var(--nb-ink);
}


.btn-text {
  font-size: 28rpx;
  font-weight: 900;
  color: var(--nb-ink);
}

.confirm-btn .btn-text {
  color: var(--nb-ink);
}
</style>
