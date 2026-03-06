<template>
  <view class="library-page" :class="{ 'light-theme': themeStore && !themeStore.isDarkMode }">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 顶部栏 -->
    <view class="header">
      <view class="header-top">
        <Logo :size="96" :show-text="true" />
      </view>
      <view class="header-info">
        <text class="header-title">我的画布</text>
        <text class="project-count">{{ projectStore?.projects?.length || 0 }} 个画布</text>
      </view>
    </view>
    
    <!-- 搜索栏 -->
    <view class="search-bar">
      <view 
        class="search-input-wrapper"
        :class="{ 'focused': isSearchFocused }"
      >
        <view class="search-icon">
          <Icon name="search" :size="36" />
        </view>
        <view class="custom-input-wrapper">
          <text v-if="!searchTerm && !isSearchFocused" class="search-placeholder">搜索画布...</text>
          <text class="search-input-text">{{ searchTerm }}</text>
          <input 
            v-model="searchTerm"
            type="text"
            class="search-input-hidden"
            :cursor-color="(themeStore && themeStore.isDarkMode) ? '#ffffff' : '#2c2c2c'"
            @focus="isSearchFocused = true"
            @blur="isSearchFocused = false"
          />
        </view>
      </view>
    </view>
    
    <!-- 项目列表 -->
    <scroll-view scroll-y class="project-list">
      <view v-if="filteredProjects.length === 0" class="empty-state" @click="goToCreate">
        <view class="empty-icon-wrapper">
          <Icon name="add" :size="64" />
        </view>
        <text class="empty-text">{{ searchTerm ? '未找到画布' : '还没有画布' }}</text>
        <text class="empty-hint">{{ searchTerm ? '尝试其他关键词' : '点击开始一个新的创作！' }}</text>
      </view>
      
      <view v-else class="project-grid">
        <ProjectCard 
          v-for="project in filteredProjects" 
          :key="project.id"
          :project="project"
        />
      </view>
    </scroll-view>
    
    <!-- 自定义底部导航栏 -->
    <CustomTabBar />
  </view>
</template>

<script>
import { useThemeStore } from '../../store/theme.js'
import { useProjectStore } from '../../store/project.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Logo from '../../components/Logo.vue'
import ProjectCard from '../../components/ProjectCard.vue'
import Icon from '../../components/Icon.vue'
import CustomTabBar from '../../components/CustomTabBar.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Logo,
    ProjectCard,
    Icon,
    CustomTabBar
  },
  
  data() {
    return {
      themeStore: null,
      projectStore: null,
      searchTerm: '',
      isSearchFocused: false
    }
  },
  
  computed: {
    filteredProjects() {
      if (!this.projectStore || !this.projectStore.projects) {
        return []
      }
      
      if (!this.searchTerm) {
        return this.projectStore.projects
      }
      
      const term = this.searchTerm.toLowerCase()
      return this.projectStore.projects.filter(p => 
        p.name.toLowerCase().includes(term)
      )
    }
  },

  onLoad() {
    this.themeStore = useThemeStore()
    this.projectStore = useProjectStore()
    
    // 立即应用主题，避免闪烁
    this.themeStore.applyTheme()
    
    // 清理无效的临时文件路径缩略图（兼容旧版本数据）
    this.cleanupInvalidThumbnails()
  },

  onShow() {
    this.themeStore.applyTheme()
  },

  methods: {
    cleanupInvalidThumbnails() {
      // 清理所有非 Base64 的缩略图（临时文件路径）
      if (!this.projectStore || !this.projectStore.projects) return
      
      let cleaned = false
      this.projectStore.projects.forEach(project => {
        if (project.thumbnail && !project.thumbnail.startsWith('data:image')) {
          // 这是临时文件路径，清除它
          project.thumbnail = ''
          cleaned = true
        }
      })
      
      if (cleaned) {
        console.log('已清理无效的缩略图路径')
        this.projectStore.saveProjects()
      }
    },
    
    goToCreate() {
      uni.navigateTo({
        url: '/pages/create/create'
      })
    }
  }
}
</script>

<style scoped>
.library-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  overflow: hidden;
  box-sizing: border-box;
}

/* 顶部栏 */
.header {
  padding: 32rpx 32rpx 16rpx;
  background-color: var(--bg-secondary);
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--bg-secondary);
}

.header-top {
  margin-bottom: 24rpx;
}

.header-info {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
}

.header-title {
  font-size: 36rpx;
  font-weight: 500;
  color: var(--text-secondary);
  letter-spacing: 0.05em;
}

.project-count {
  font-size: 22rpx;
  color: var(--text-tertiary);
  font-family: monospace;
}

/* 搜索栏 */
.search-bar {
  padding: 16rpx 32rpx;
  background-color: var(--bg-secondary);
}

.search-input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  padding: 24rpx 32rpx;
  transition: var(--transition-base);
}

.search-input-wrapper.focused {
  border-color: var(--accent-primary);
}

.search-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
  margin-right: 20rpx;
  transition: var(--transition-base);
}

.search-input-wrapper.focused .search-icon {
  color: var(--accent-primary);
}

.custom-input-wrapper {
  position: relative;
  flex: 1;
  min-height: 48rpx;
  display: flex;
  align-items: center;
}

.search-placeholder {
  position: absolute;
  left: 0;
  font-size: 28rpx;
  color: var(--text-tertiary);
  pointer-events: none;
}

.search-input-text {
  font-size: 28rpx;
  color: var(--text-primary);
  min-width: 20rpx;
}

.search-input-hidden {
  position: absolute;
  left: 0;
  right: 0;
  top: 0;
  bottom: 0;
  opacity: 0;
  font-size: 28rpx;
}

/* 项目列表 */
.project-list {
  flex: 1;
  overflow-y: auto;
  padding: 0 0 160rpx;
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 96rpx 48rpx;
  text-align: center;
}

.empty-icon-wrapper {
  width: 96rpx;
  height: 96rpx;
  border-radius: 50%;
  background-color: var(--bg-tertiary);
  border: 3rpx dashed var(--border-secondary);
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 24rpx;
  color: var(--text-tertiary);
}

.empty-text {
  font-size: 26rpx;
  color: var(--text-secondary);
  margin-bottom: 8rpx;
}

.empty-hint {
  font-size: 22rpx;
  color: var(--text-tertiary);
  margin-top: 8rpx;
}

/* 项目网格 */
.project-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
  padding: 24rpx 32rpx;
  box-sizing: border-box;
  align-items: stretch;
}

/* 小程序grid对齐修复 */
/* #ifdef MP-WEIXIN */
.project-grid {
  grid-auto-rows: 1fr;
}
/* #endif */

</style>

