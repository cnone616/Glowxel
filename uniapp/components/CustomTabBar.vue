<template>
  <view class="custom-tabbar">
    <!-- 首页 -->
    <view 
      class="tab-item"
      :class="{ 'active': currentPath === '/pages/library/library' }"
      @click="navigateTo('/pages/library/library')"
    >
      <Icon name="home" :size="40" :color="getIconColor('/pages/library/library')" />
      <text class="tab-label">首页</text>
    </view>
    
    <!-- 创作 -->
    <view
      class="tab-item"
      :class="{ 'active': currentPath === '/pages/workspace/workspace' }"
      @click="navigateTo('/pages/workspace/workspace')"
    >
      <Icon name="add" :size="40" :color="getIconColor('/pages/workspace/workspace')" />
      <text class="tab-label">创作</text>
    </view>
    
    <!-- 社区 -->
    <view 
      class="tab-item"
      :class="{ 'active': currentPath === '/pages/community/community' }"
      @click="navigateTo('/pages/community/community')"
    >
      <Icon name="browse" :size="40" :color="getIconColor('/pages/community/community')" />
      <text class="tab-label">社区</text>
    </view>
    
    <!-- 设备 -->
    <view 
      class="tab-item"
      :class="{ 'active': currentPath === '/pages/control/control' }"
      @click="navigateTo('/pages/control/control')"
    >
      <Icon name="link" :size="40" :color="getIconColor('/pages/control/control')" />
      <text class="tab-label">设备</text>
    </view>
    
    <!-- 我的 -->
    <view 
      class="tab-item"
      :class="{ 'active': currentPath === '/pages/profile/profile' }"
      @click="navigateTo('/pages/profile/profile')"
    >
      <Icon name="user" :size="40" :color="getIconColor('/pages/profile/profile')" />
      <text class="tab-label">我的</text>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'

export default {
  components: {
    Icon
  },
  
  data() {
    return {
      currentPath: ''
    }
  },
  
  mounted() {
    this.updateCurrentPath()
  },
  
  methods: {
    getIconColor(path) {
      const isActive = this.currentPath === path
      return isActive ? '#4F7FFF' : '#666666'
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

      // tabBar 页面必须用 switchTab
      uni.switchTab({
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
  height: calc(128rpx + env(safe-area-inset-bottom));
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--border-primary);
  display: flex;
  align-items: center;
  justify-content: space-around;
  padding-bottom: env(safe-area-inset-bottom);
  z-index: 1000;
  box-sizing: border-box;
}

.tab-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 6rpx;
  height: 128rpx;
  transition: all 0.2s ease;
  box-sizing: border-box;
  padding: 16rpx 8rpx;
}

.tab-item:active {
  transform: scale(0.95);
}

.tab-label {
  font-size: 20rpx;
  font-weight: 500;
  letter-spacing: 0.05em;
  color: var(--text-secondary);
  transition: all 0.2s ease;
}

.tab-item.active .tab-label {
  color: var(--accent-primary);
  font-weight: 600;
}
</style>
