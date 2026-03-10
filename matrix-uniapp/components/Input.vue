<template>
  <view class="input-wrapper" :class="wrapperClasses">
    <!-- 前置图标 -->
    <view v-if="prefixIcon" class="input-prefix">
      <Icon :name="prefixIcon" :size="iconSize" :color="iconColor" />
    </view>
    
    <!-- 输入框 -->
    <input
      v-if="type !== 'textarea'"
      v-model="inputValue"
      :type="inputType"
      :placeholder="placeholder"
      :disabled="disabled"
      :maxlength="maxlength"
      :focus="focus"
      :password="type === 'password'"
      class="input-field"
      :class="inputClasses"
      @input="handleInput"
      @focus="handleFocus"
      @blur="handleBlur"
      @confirm="handleConfirm"
    />
    
    <!-- 多行文本框 -->
    <textarea
      v-else
      v-model="inputValue"
      :placeholder="placeholder"
      :disabled="disabled"
      :maxlength="maxlength"
      :focus="focus"
      :auto-height="autoHeight"
      class="textarea-field"
      :class="inputClasses"
      @input="handleInput"
      @focus="handleFocus"
      @blur="handleBlur"
    />
    
    <!-- 后置内容 -->
    <view v-if="suffixIcon || clearable || showWordCount" class="input-suffix">
      <!-- 清除按钮 -->
      <view 
        v-if="clearable && inputValue && !disabled" 
        class="clear-btn"
        @click="handleClear"
      >
        <Icon name="close" :size="28" color="#AAAAAA" />
      </view>
      
      <!-- 后置图标 -->
      <view v-if="suffixIcon" class="suffix-icon">
        <Icon :name="suffixIcon" :size="iconSize" :color="iconColor" />
      </view>
      
      <!-- 字数统计 -->
      <text v-if="showWordCount && maxlength" class="word-count">
        {{ inputValue.length }}/{{ maxlength }}
      </text>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'

export default {
  name: 'Input',
  components: {
    Icon
  },
  
  props: {
    // 双向绑定值
    modelValue: {
      type: [String, Number],
      default: ''
    },
    
    // 输入框类型
    type: {
      type: String,
      default: 'text', // text, password, number, search, textarea
      validator: (value) => ['text', 'password', 'number', 'search', 'textarea'].includes(value)
    },
    
    // 占位符
    placeholder: {
      type: String,
      default: '请输入内容'
    },
    
    // 是否禁用
    disabled: {
      type: Boolean,
      default: false
    },
    
    // 最大长度
    maxlength: {
      type: Number,
      default: -1
    },
    
    // 是否自动获取焦点
    focus: {
      type: Boolean,
      default: false
    },
    
    // 尺寸
    size: {
      type: String,
      default: 'medium', // small, medium, large
      validator: (value) => ['small', 'medium', 'large'].includes(value)
    },
    
    // 前置图标
    prefixIcon: {
      type: String,
      default: ''
    },
    
    // 后置图标
    suffixIcon: {
      type: String,
      default: ''
    },
    
    // 是否可清除
    clearable: {
      type: Boolean,
      default: false
    },
    
    // 是否显示字数统计
    showWordCount: {
      type: Boolean,
      default: false
    },
    
    // 是否只读
    readonly: {
      type: Boolean,
      default: false
    },
    
    // 状态
    status: {
      type: String,
      default: '', // success, warning, error
      validator: (value) => ['', 'success', 'warning', 'error'].includes(value)
    },
    
    // 自定义样式类
    customClass: {
      type: String,
      default: ''
    },
    
    // textarea 自动高度
    autoHeight: {
      type: Boolean,
      default: false
    }
  },
  
  emits: ['update:modelValue', 'input', 'focus', 'blur', 'clear', 'confirm'],
  
  data() {
    return {
      isFocused: false
    }
  },
  
  computed: {
    inputValue: {
      get() {
        return this.modelValue
      },
      set(value) {
        this.$emit('update:modelValue', value)
      }
    },
    
    inputType() {
      const typeMap = {
        text: 'text',
        password: 'text', // 使用 password 属性控制
        number: 'number',
        search: 'text'
      }
      return typeMap[this.type] || 'text'
    },
    
    wrapperClasses() {
      return [
        `input-${this.size}`,
        {
          'input-focused': this.isFocused,
          'input-disabled': this.disabled,
          'input-readonly': this.readonly,
          [`input-${this.status}`]: this.status
        },
        this.customClass
      ].filter(Boolean)
    },
    
    inputClasses() {
      return {
        'has-prefix': this.prefixIcon,
        'has-suffix': this.suffixIcon || this.clearable || this.showWordCount
      }
    },
    
    iconSize() {
      const sizeMap = {
        small: 28,
        medium: 32,
        large: 36
      }
      return sizeMap[this.size] || 32
    },
    
    iconColor() {
      if (this.disabled) return '#CCCCCC'
      if (this.status === 'error') return '#FF4757'
      if (this.status === 'warning') return '#FFA726'
      if (this.status === 'success') return '#4CAF50'
      return '#666666'
    }
  },
  
  methods: {
    handleInput(e) {
      const value = e.detail.value
      this.inputValue = value
      this.$emit('input', value)
    },
    
    handleFocus(e) {
      this.isFocused = true
      this.$emit('focus', e)
    },
    
    handleBlur(e) {
      this.isFocused = false
      this.$emit('blur', e)
    },
    
    handleClear() {
      this.inputValue = ''
      this.$emit('clear')
      this.$emit('input', '')
    },
    
    handleConfirm(e) {
      this.$emit('confirm', e.detail.value)
    }
  }
}
</script>

<style scoped>
.input-wrapper {
  display: flex;
  align-items: center;
  background-color: var(--color-card-background);
  border: 2rpx solid var(--border-primary);
  border-radius: var(--radius-medium);
  transition: all 0.2s ease;
  position: relative;
  overflow: hidden;
}

/* 尺寸变体 */
.input-small {
  padding: 16rpx 20rpx;
  min-height: 64rpx;
}

.input-medium {
  padding: 20rpx 24rpx;
  min-height: 80rpx;
}

.input-large {
  padding: 24rpx 28rpx;
  min-height: 96rpx;
}

/* 状态样式 */
.input-focused {
  border-color: var(--color-brand-primary);
  box-shadow: 0 0 0 4rpx rgba(79, 127, 255, 0.1);
}

.input-disabled {
  background-color: var(--color-app-background);
  border-color: var(--border-secondary);
  opacity: 0.6;
}

.input-readonly {
  background-color: var(--color-app-background);
  cursor: not-allowed;
}

.input-error {
  border-color: #FF4757;
}

.input-error.input-focused {
  box-shadow: 0 0 0 4rpx rgba(255, 71, 87, 0.1);
}

.input-warning {
  border-color: #FFA726;
}

.input-warning.input-focused {
  box-shadow: 0 0 0 4rpx rgba(255, 167, 38, 0.1);
}

.input-success {
  border-color: #4CAF50;
}

.input-success.input-focused {
  box-shadow: 0 0 0 4rpx rgba(76, 175, 80, 0.1);
}

/* 前置图标 */
.input-prefix {
  display: flex;
  align-items: center;
  margin-right: 16rpx;
  flex-shrink: 0;
}

/* 输入框 */
.input-field,
.textarea-field {
  flex: 1;
  font-size: 28rpx;
  color: var(--color-text-primary);
  background: transparent;
  border: none;
  outline: none;
  line-height: 1.5;
}

.input-field::placeholder,
.textarea-field::placeholder {
  color: var(--color-text-disabled);
}

.textarea-field {
  min-height: 120rpx;
  resize: none;
}

/* 后置内容 */
.input-suffix {
  display: flex;
  align-items: center;
  gap: 12rpx;
  margin-left: 16rpx;
  flex-shrink: 0;
}

.clear-btn {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  background-color: var(--color-app-background);
  transition: all 0.2s ease;
}

.clear-btn:active {
  transform: scale(0.9);
}

.suffix-icon {
  display: flex;
  align-items: center;
}

.word-count {
  font-size: 24rpx;
  color: var(--color-text-disabled);
  white-space: nowrap;
}

/* 搜索框特殊样式 */
.input-wrapper.search-input {
  border-radius: 40rpx;
}

/* 响应式适配 */
@media (max-width: 750rpx) {
  .input-small {
    padding: 12rpx 16rpx;
    min-height: 56rpx;
  }
  
  .input-medium {
    padding: 16rpx 20rpx;
    min-height: 72rpx;
  }
  
  .input-large {
    padding: 20rpx 24rpx;
    min-height: 88rpx;
  }
  
  .input-field,
  .textarea-field {
    font-size: 26rpx;
  }
}

/* 禁用状态 */
.input-disabled .input-field,
.input-disabled .textarea-field {
  color: var(--color-text-disabled);
  cursor: not-allowed;
}

/* 只读状态 */
.input-readonly .input-field,
.input-readonly .textarea-field {
  cursor: not-allowed;
}
</style>