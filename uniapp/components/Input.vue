<template>
  <view class="glx-input-shell" :class="wrapperClasses">
    <!-- 前置图标 -->
    <view v-if="prefixIcon" class="glx-input-prefix">
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
      class="glx-input-field"
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
      class="glx-input-textarea"
      :class="inputClasses"
      @input="handleInput"
      @focus="handleFocus"
      @blur="handleBlur"
    />
    
    <!-- 后置内容 -->
    <view v-if="suffixIcon || clearable || showWordCount" class="glx-input-suffix">
      <!-- 清除按钮 -->
      <view 
        v-if="clearable && inputValue && !disabled" 
        class="glx-input-clear"
        @click="handleClear"
      >
        <Icon name="close" :size="28" color="var(--text-tertiary)" />
      </view>
      
      <!-- 后置图标 -->
      <view v-if="suffixIcon" class="glx-input-suffix-icon">
        <Icon :name="suffixIcon" :size="iconSize" :color="iconColor" />
      </view>
      
      <!-- 字数统计 -->
      <text v-if="showWordCount && maxlength" class="glx-input-word-count">
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
          'glx-input--focused': this.isFocused,
          'glx-input--disabled': this.disabled,
          'glx-input--readonly': this.readonly,
          [`glx-input--${this.status}`]: this.status
        },
        this.customClass
      ].filter(Boolean)
    },
    
    inputClasses() {
      return {
        'glx-input--has-prefix': this.prefixIcon,
        'glx-input--has-suffix': this.suffixIcon || this.clearable || this.showWordCount
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
.glx-input-shell {
  display: flex;
  align-items: center;
  background: var(--tone-paper-soft);
  border: 3rpx solid #000000;
  border-radius: 0;
  box-shadow: var(--nb-shadow-soft);
  transition: border-color 0.18s ease, box-shadow 0.18s ease, background-color 0.18s ease;
  position: relative;
  overflow: hidden;
  box-sizing: border-box;
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
.glx-input--focused {
  border-color: #000000;
  box-shadow: 3rpx 3rpx 0 #000000;
}

.glx-input--disabled {
  background-color: #f1efe6;
  border-color: #000000;
  opacity: 0.6;
  box-shadow: none;
}

.glx-input--readonly {
  background-color: #f3f1ee;
  cursor: not-allowed;
}

.glx-input--error {
  background: #ffd7cf;
  border-color: #000000;
}

.glx-input--error.glx-input--focused {
  box-shadow: 3rpx 3rpx 0 #000000;
}

.glx-input--warning {
  background: #ffe1c4;
  border-color: #000000;
}

.glx-input--warning.glx-input--focused {
  box-shadow: 3rpx 3rpx 0 #000000;
}

.glx-input--success {
  background: #fff1a8;
  border-color: #000000;
}

.glx-input--success.glx-input--focused {
  box-shadow: 3rpx 3rpx 0 #000000;
}

/* 前置图标 */
.glx-input-prefix {
  display: flex;
  align-items: center;
  margin-right: 16rpx;
  flex-shrink: 0;
}

/* 输入框 */
.glx-input-field,
.glx-input-textarea {
  flex: 1;
  font-size: 28rpx;
  color: #000000;
  background: transparent;
  border: none;
  outline: none;
  line-height: 1.5;
  font-weight: 700;
}

.glx-input-field::placeholder,
.glx-input-textarea::placeholder {
  color: #8b8b8b;
}

.glx-input-textarea {
  min-height: 120rpx;
  resize: none;
}

/* 后置内容 */
.glx-input-suffix {
  display: flex;
  align-items: center;
  gap: 12rpx;
  margin-left: 16rpx;
  flex-shrink: 0;
}

.glx-input-clear {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid #000000;
  background-color: var(--tone-paper-soft);
  transition: border-color 0.18s ease, box-shadow 0.18s ease, background-color 0.18s ease;
  box-sizing: border-box;
}


.glx-input-suffix-icon {
  display: flex;
  align-items: center;
}

.glx-input-word-count {
  font-size: 24rpx;
  color: #4a4a4a;
  white-space: nowrap;
  font-weight: 800;
}

/* 搜索框特殊样式 */
.glx-input-shell.search-input {
  border-radius: 0;
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
  
  .glx-input-field,
  .glx-input-textarea {
    font-size: 26rpx;
  }
}

/* 禁用状态 */
.glx-input--disabled .glx-input-field,
.glx-input--disabled .glx-input-textarea {
  color: #8b8b8b;
  cursor: not-allowed;
}

/* 只读状态 */
.glx-input--readonly .glx-input-field,
.glx-input--readonly .glx-input-textarea {
  cursor: not-allowed;
}
</style>
