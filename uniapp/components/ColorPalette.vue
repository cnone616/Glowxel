<template>
  <view class="color-palette-container">
    <!-- 颜色网格 -->
    <view class="color-grid-wrapper">
      <!-- 顶部信息栏 -->
      <view class="palette-header">
        <text class="palette-info">调色板 ({{ colors.length }}色)</text>
        <text v-if="selectedColorInfo" class="selected-code">当前选中:{{ selectedColorInfo.code }}</text>
      </view>
      
      <!-- 颜色网格 -->
      <scroll-view 
        scroll-y 
        class="color-grid-scroll"
        :scroll-into-view="scrollIntoView"
        @scroll="handleScroll"
      >
        <view 
          v-for="group in colorGroups" 
          :key="group.letter" 
          class="color-group"
        >
          <!-- 分组标题 -->
          <view 
            :id="`letter-${group.letter}`"
            class="group-header"
          >
            <text class="group-letter">{{ group.letter }}</text>
            <text class="group-count">({{ group.colors.length }})</text>
          </view>
          
          <!-- 该字母下的颜色 -->
          <view class="color-grid">
            <view
              v-for="color in group.colors"
              :key="color.code"
              class="color-item"
              :class="{ 'active': selectedColor === color.hex }"
              @click="handleColorClick(color.hex)"
            >
              <!-- 颜色圆圈 -->
              <view 
                class="color-swatch" 
                :style="{ backgroundColor: color.hex }"
              ></view>
              
              <!-- 色号 -->
              <text class="color-code">{{ color.code }}</text>
            </view>
          </view>
        </view>
      </scroll-view>
    </view>
    
    <!-- 右侧字母索引条 -->
    <view 
      class="index-bar"
      @touchstart="handleIndexTouchStart"
      @touchmove="handleIndexTouchMove"
      @touchend="handleIndexTouchEnd"
    >
      <view class="index-letters">
        <view
          v-for="(letter, index) in availableLetters"
          :key="letter"
          :data-letter="letter"
          :data-index="index"
          class="index-letter"
          :class="{ 'active': currentVisibleLetter === letter }"
          @click="scrollToLetter(letter)"
        >
          <text class="letter-text">{{ letter }}</text>
        </view>
      </view>
    </view>
    
    <!-- 字母提示气泡 -->
    <view 
      v-if="showLetterBubble && currentTouchLetter"
      class="letter-bubble"
      :style="{ 
        left: bubblePosition.x + 'px', 
        top: bubblePosition.y + 'px'
      }"
    >
      <text class="bubble-letter">{{ currentTouchLetter }}</text>
    </view>
  </view>
</template>

<script>
export default {
  props: {
    colors: {
      type: Array,
      required: true
    },
    selectedColor: {
      type: String,
      default: ''
    }
  },
  
  data() {
    return {
      currentVisibleLetter: '',
      showLetterBubble: false,
      currentTouchLetter: '',
      bubblePosition: { x: 0, y: 0 },
      scrollIntoView: '',
      isIndexTouching: false
    }
  },
  
  computed: {
    // 按色号首字母分组并排序
    colorGroups() {
      const groups = new Map()
      
      // 按色号排序
      const sortedColors = [...this.colors].sort((a, b) => a.code.localeCompare(b.code))
      
      // 按首字母分组
      sortedColors.forEach(color => {
        const letter = color.code.charAt(0).toUpperCase()
        if (!groups.has(letter)) {
          groups.set(letter, [])
        }
        groups.get(letter).push(color)
      })
      
      // 转换为数组并排序
      return Array.from(groups.entries())
        .sort((a, b) => a[0].localeCompare(b[0]))
        .map(([letter, colors]) => ({ letter, colors }))
    },
    
    // 可用的字母列表
    availableLetters() {
      return this.colorGroups.map(g => g.letter)
    },
    
    // 当前选中颜色的信息
    selectedColorInfo() {
      return this.colors.find(c => c.hex === this.selectedColor)
    }
  },
  
  methods: {
    // 滚动到指定字母
    scrollToLetter(letter) {
      this.scrollIntoView = `letter-${letter}`
      this.currentVisibleLetter = letter
    },
    
    // 处理滚动事件
    handleScroll(e) {
      // uni-app 中滚动事件处理较简单，这里简化处理
      // 实际应用中可以根据 scrollTop 计算当前可见字母
    },
    
    // 触摸开始
    handleIndexTouchStart(e) {
      this.isIndexTouching = true
      this.showLetterBubble = true
      
      const touch = e.touches[0]
      const letter = this.getLetterFromTouch(touch.pageY)
      
      if (letter) {
        this.currentTouchLetter = letter
        this.bubblePosition = { x: touch.pageX - 80, y: touch.pageY }
        this.scrollToLetter(letter)
      }
    },
    
    // 触摸移动
    handleIndexTouchMove(e) {
      if (!this.isIndexTouching) return
      
      const touch = e.touches[0]
      const letter = this.getLetterFromTouch(touch.pageY)
      
      if (letter && letter !== this.currentTouchLetter) {
        this.currentTouchLetter = letter
        this.bubblePosition = { x: touch.pageX - 80, y: touch.pageY }
        this.scrollToLetter(letter)
      } else if (letter) {
        this.bubblePosition = { x: touch.pageX - 80, y: touch.pageY }
      }
    },
    
    // 触摸结束
    handleIndexTouchEnd() {
      this.isIndexTouching = false
      this.showLetterBubble = false
    },
    
    // 根据触摸位置获取字母
    getLetterFromTouch(pageY) {
      // 简化实现：根据触摸位置计算对应的字母索引
      const query = uni.createSelectorQuery().in(this)
      query.select('.index-bar').boundingClientRect()
      query.selectAll('.index-letter').boundingClientRect()
      query.exec((res) => {
        if (!res || !res[0] || !res[1]) return null
        
        const barRect = res[0]
        const letterRects = res[1]
        
        // 找到最接近的字母
        let closestIndex = 0
        let minDistance = Infinity
        
        letterRects.forEach((rect, index) => {
          const letterCenterY = rect.top + rect.height / 2
          const distance = Math.abs(pageY - letterCenterY)
          
          if (distance < minDistance) {
            minDistance = distance
            closestIndex = index
          }
        })
        
        return this.availableLetters[closestIndex]
      })
      
      // 临时返回第一个字母（实际应该异步处理）
      return this.availableLetters[0]
    },
    
    // 处理颜色点击
    handleColorClick(hex) {
      if (this.selectedColor === hex) {
        this.$emit('select-color', '')
      } else {
        this.$emit('select-color', hex)
      }
    }
  }
}
</script>

<style scoped>
.color-palette-container {
  display: flex;
  gap: 16rpx;
  position: relative;
}

.color-grid-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.palette-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 8rpx;
}

.palette-info {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.selected-code {
  font-size: 20rpx;
  color: var(--accent-color);
  font-family: monospace;
  font-weight: bold;
}

.color-grid-scroll {
  max-height: 384rpx;
  padding: 8rpx;
}

.color-group {
  margin-bottom: 24rpx;
}

.group-header {
  position: sticky;
  top: 0;
  background-color: var(--bg-secondary);
  padding: 8rpx 16rpx;
  margin-bottom: 16rpx;
  border-left: 4rpx solid var(--accent-color);
  z-index: 10;
}

.group-letter {
  font-size: 20rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--accent-color);
}

.group-count {
  font-size: 16rpx;
  color: var(--text-tertiary);
  margin-left: 16rpx;
}

.color-grid {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 16rpx;
}

.color-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx;
  border-radius: 12rpx;
  border: 4rpx solid var(--border-color);
  position: relative;
  transition: all 0.2s;
}

.color-item.active {
  border-color: var(--accent-color);
  background-color: rgba(0, 243, 255, 0.1);
  box-shadow: 0 0 20rpx rgba(0, 243, 255, 0.3);
  z-index: 10;
}

.color-swatch {
  width: 64rpx;
  height: 64rpx;
  border-radius: 50%;
  border: 2rpx solid rgba(255, 255, 255, 0.2);
  box-shadow: 0 4rpx 12rpx rgba(0, 0, 0, 0.3);
}

.color-code {
  font-size: 16rpx;
  font-family: monospace;
  color: var(--text-primary);
  font-weight: bold;
  line-height: 1;
}

.index-bar {
  width: 40rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 16rpx 0;
}

.index-letters {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4rpx;
}

.index-letter {
  width: 32rpx;
  height: 32rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 8rpx;
  transition: all 0.2s;
}

.index-letter.active {
  transform: scale(1.25);
}

.letter-text {
  font-size: 16rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-tertiary);
}

.index-letter.active .letter-text {
  color: var(--accent-color);
}

.letter-bubble {
  position: fixed;
  z-index: 1000;
  pointer-events: none;
  transform: translate(-50%, -50%);
}

.bubble-letter {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 128rpx;
  height: 128rpx;
  background-color: var(--accent-color);
  color: #000000;
  font-family: monospace;
  font-weight: bold;
  font-size: 48rpx;
  border-radius: 32rpx;
  box-shadow: 0 8rpx 32rpx rgba(0, 0, 0, 0.5);
}
</style>
