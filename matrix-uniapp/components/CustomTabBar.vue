<template>
  <view class="custom-tabbar" :class="{ 'light-theme': !isDarkMode }">
    <view 
      class="tab-item"
      :class="{ 'active': currentPath === '/pages/library/library' }"
      @click="navigateTo('/pages/library/library')"
    >
      <Icon name="home" :size="48" :color="getIconColor('/pages/library/library')" />
      <text class="tab-label">画布库</text>
    </view>
    
    <view class="tab-center">
      <view 
        class="fab-button"
        @click="navigateTo('/pages/create/create')"
      >
        <Icon name="add" :size="64" :color="getFabIconColor()" />
      </view>
    </view>
    
    <view 
      class="tab-item"
      :class="{ 'active': currentPath === '/pages/user/user' }"
      @click="navigateTo('/pages/user/user')"
    >
      <Icon name="user" :size="48" :color="getIconColor('/pages/user/user')" />
      <text class="tab-label">我的</text>
    </view>
  </view>
</template>

<script>
import { useThemeStore } from '../store/theme.js'
import Icon from './Icon.vue'

export default {
  components: {
    Icon
  },
  
  data() {
    return {
      themeStore: null,
      currentPath: ''
    }
  },
  
  computed: {
    isDarkMode() {
      return this.themeStore?.isDarkMode ?? true
    }
  },
  
  mounted() {
    this.themeStore = useThemeStore()
    this.updateCurrentPath()
  },
  
  methods: {
    getIconColor(path) {
      const isActive = this.currentPath === path
      if (this.isDarkMode) {
        return isActive ? '#00f3ff' : '#666666'
      } else {
        return isActive ? '#0099cc' : '#666666'
      }
    },
    
    getFabIconColor() {
      return this.isDarkMode ? '#000000' : '#ffffff'
    },
    
    updateCurrentPath() {
      const pages = getCurrentPages()
      if (pages.length > 0) {
        const currentPage = pages[pages.length - 1]
        this.currentPath = '/' + currentPage.route
      }
    },
    
    navigateTo(path) {
      if (this.currentPath === path) return
      
      // 使用 reLaunch 来清空页面栈并跳转
      uni.reLaunch({
        url: path
      })
    }
  }
}
</script>

<style scoped>
.custom-tabbar {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  height: 128rpx;
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--border-primary);
  display: flex;
  align-items: center;
  justify-content: space-around;
  padding-bottom: env(safe-area-inset-bottom);
  z-index: 1000;
}

.tab-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  height: 100%;
  transition: var(--transition-base);
}

.tab-item:active {
  transform: scale(0.95);
}

.tab-label {
  font-size: 20rpx;
  font-weight: 500;
  letter-spacing: 0.05em;
  color: var(--text-secondary);
  transition: var(--transition-base);
}

.tab-item.active .tab-label {
  color: var(--accent-primary);
}

/* 浅色主题下选中文字颜色 */
.light-theme .tab-item.active .tab-label {
  color: #0099cc;
}

.tab-center {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  width: 128rpx;
  height: 128rpx;
}

.fab-button {
  position: absolute;
  top: -48rpx;
  width: 112rpx;
  height: 112rpx;
  border-radius: 50%;
  background: linear-gradient(135deg, #00f3ff 0%, #0099ff 100%);
  box-shadow: 0 0 30rpx rgba(0, 243, 255, 0.4);
  display: flex;
  align-items: center;
  justify-content: center;
  transition: var(--transition-base);
}

.fab-button:active {
  transform: scale(0.95);
  box-shadow: 0 0 40rpx rgba(0, 243, 255, 0.6);
}

/* 浅色主题 */
.light-theme .custom-tabbar {
  background-color: #ffffff;
  border-top-color: #e5e7eb;
}

.light-theme .tab-label {
  color: #6b7280;
}

.light-theme .fab-button {
  background: linear-gradient(135deg, #0099cc 0%, #0077aa 100%);
  box-shadow: 0 0 30rpx rgba(0, 153, 204, 0.4);
}

.light-theme .fab-button:active {
  box-shadow: 0 0 40rpx rgba(0, 153, 204, 0.6);
}
</style>
