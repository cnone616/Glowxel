<template>
  <view class="my-favorites-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="goBack">
        <Icon name="direction-left" :size="32" color="var(--color-text-primary)" />
      </view>
      <text class="nav-title">我的收藏</text>
      <view class="nav-right" @click="toggleEditMode">
        <text class="edit-btn">{{ isEditMode ? '完成' : '编辑' }}</text>
      </view>
    </view>
    
    <!-- 分类标签 -->
    <view class="category-tabs">
      <view 
        v-for="category in categories"
        :key="category.value"
        class="category-tab"
        :class="{ active: currentCategory === category.value }"
        @click="currentCategory = category.value"
      >
        <Icon :name="category.icon" :size="32" />
        <text class="category-text">{{ category.label }}</text>
        <text v-if="category.count > 0" class="category-count">{{ category.count }}</text>
      </view>
    </view>
    
    <!-- 收藏列表 -->
    <scroll-view scroll-y class="content">
      <view v-if="filteredFavorites.length === 0" class="empty-state">
        <Icon name="bookmark" :size="120" color="var(--color-text-disabled)" />
        <text class="empty-title">暂无收藏</text>
        <text class="empty-desc">去社区发现更多精彩作品吧</text>
        <button class="explore-btn" @click="goToExplore">
          <Icon name="compass" :size="32" />
          <text>去探索</text>
        </button>
      </view>
      
      <view v-else class="favorites-list">
        <view 
          v-for="item in filteredFavorites"
          :key="item.id"
          class="favorite-item"
          :class="{ 'edit-mode': isEditMode }"
          @click="openFavorite(item)"
        >
          <!-- 选择框 -->
          <view v-if="isEditMode" class="select-checkbox" @click.stop="toggleSelect(item.id)">
            <Icon 
              v-if="selectedItems.includes(item.id)"
              name="check-circle-fill" 
              :size="40" 
              color="var(--color-brand-primary)" 
            />
            <Icon 
              v-else
              name="circle" 
              :size="40" 
              color="var(--color-text-disabled)" 
            />
          </view>
          
          <!-- 缩略图 -->
          <view class="item-thumbnail">
            <image 
              v-if="item.thumbnail"
              :src="item.thumbnail"
              class="thumbnail-image"
              mode="aspectFill"
            />
            <view v-else class="thumbnail-placeholder">
              <Icon :name="getTypeIcon(item.type)" :size="60" color="var(--color-text-disabled)" />
            </view>
            
            <!-- 类型标识 -->
            <view class="type-badge" :class="item.type">
              <Icon :name="getTypeIcon(item.type)" :size="24" />
            </view>
          </view>
          
          <!-- 信息 -->
          <view class="item-info">
            <text class="item-title">{{ item.title }}</text>
            <text class="item-author">by {{ item.author }}</text>
            <view class="item-meta">
              <text class="item-size">{{ item.width }}×{{ item.height }}</text>
              <text class="item-date">{{ formatDate(item.favoriteTime) }}</text>
            </view>
            <view v-if="item.tags && item.tags.length > 0" class="item-tags">
              <text 
                v-for="tag in item.tags.slice(0, 3)"
                :key="tag"
                class="tag"
              >{{ tag }}</text>
            </view>
          </view>
          
          <!-- 操作按钮 -->
          <view v-if="!isEditMode" class="item-actions">
            <view class="action-btn" @click.stop="downloadFavorite(item)">
              <Icon name="download" :size="32" />
            </view>
            <view class="action-btn" @click.stop="removeFavorite(item)">
              <Icon name="bookmark-fill" :size="32" color="var(--color-brand-primary)" />
            </view>
          </view>
        </view>
      </view>
    </scroll-view>
    
    <!-- 批量操作栏 -->
    <view v-if="isEditMode" class="batch-actions">
      <view class="batch-info">
        <text class="selected-count">已选择 {{ selectedItems.length }} 项</text>
      </view>
      <view class="batch-buttons">
        <button class="batch-btn" @click="batchDownload" :disabled="selectedItems.length === 0">
          <Icon name="download" :size="32" />
          <text>下载</text>
        </button>
        <button class="batch-btn danger" @click="batchRemove" :disabled="selectedItems.length === 0">
          <Icon name="delete" :size="32" />
          <text>移除</text>
        </button>
      </view>
    </view>
    
    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Toast from '../../components/Toast.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast
  },
  
  data() {
    return {
      toast: null,
      currentCategory: 'all',
      isEditMode: false,
      selectedItems: [],
      categories: [
        { value: 'all', label: '全部', icon: 'grid', count: 0 },
        { value: 'artwork', label: '作品', icon: 'picture', count: 0 },
        { value: 'template', label: '模板', icon: 'layout', count: 0 },
        { value: 'pattern', label: '图案', icon: 'star', count: 0 }
      ],
      favorites: [
        // 模拟数据
        {
          id: '1',
          type: 'artwork',
          title: '像素小猫',
          author: '设计师A',
          width: 32,
          height: 32,
          thumbnail: '',
          favoriteTime: Date.now() - 86400000,
          tags: ['可爱', '动物', '像素']
        },
        {
          id: '2',
          type: 'template',
          title: '圣诞树模板',
          author: '官方',
          width: 64,
          height: 64,
          thumbnail: '',
          favoriteTime: Date.now() - 172800000,
          tags: ['节日', '圣诞', '模板']
        },
        {
          id: '3',
          type: 'pattern',
          title: '几何图案',
          author: '用户B',
          width: 16,
          height: 16,
          thumbnail: '',
          favoriteTime: Date.now() - 259200000,
          tags: ['几何', '抽象']
        }
      ]
    }
  },
  
  computed: {
    filteredFavorites() {
      if (this.currentCategory === 'all') {
        return this.favorites
      }
      return this.favorites.filter(item => item.type === this.currentCategory)
    }
  },
  
  onLoad() {
    this.toast = useToast()
    this.updateCategoryCounts()
    
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
    })
  },
  
  methods: {
    goBack() {
      uni.navigateBack()
    },
    
    goToExplore() {
      uni.switchTab({
        url: '/pages/community/community'
      })
    },
    
    updateCategoryCounts() {
      this.categories[0].count = this.favorites.length
      this.categories[1].count = this.favorites.filter(f => f.type === 'artwork').length
      this.categories[2].count = this.favorites.filter(f => f.type === 'template').length
      this.categories[3].count = this.favorites.filter(f => f.type === 'pattern').length
    },
    
    toggleEditMode() {
      this.isEditMode = !this.isEditMode
      if (!this.isEditMode) {
        this.selectedItems = []
      }
    },
    
    toggleSelect(id) {
      const index = this.selectedItems.indexOf(id)
      if (index > -1) {
        this.selectedItems.splice(index, 1)
      } else {
        this.selectedItems.push(id)
      }
    },
    
    openFavorite(item) {
      if (this.isEditMode) {
        this.toggleSelect(item.id)
        return
      }
      
      // 根据类型跳转到不同页面
      switch (item.type) {
        case 'artwork':
          uni.navigateTo({
            url: `/pages/artwork-detail/artwork-detail?id=${item.id}`
          })
          break
        case 'template':
          // 跳转到模板详情或直接使用
          this.useTemplate(item)
          break
        case 'pattern':
          // 跳转到图案详情
          this.toast.showInfo('图案详情功能开发中')
          break
      }
    },
    
    useTemplate(template) {
      uni.showModal({
        title: '使用模板',
        content: `确定要使用模板"${template.title}"创建新作品吗？`,
        success: (res) => {
          if (res.confirm) {
            // 跳转到创建页面并传递模板信息
            uni.navigateTo({
              url: `/pages/create/create?template=${template.id}`
            })
          }
        }
      })
    },
    
    downloadFavorite(item) {
      this.toast.showInfo('下载功能开发中')
    },
    
    removeFavorite(item) {
      uni.showModal({
        title: '移除收藏',
        content: `确定要移除"${item.title}"吗？`,
        success: (res) => {
          if (res.confirm) {
            const index = this.favorites.findIndex(f => f.id === item.id)
            if (index > -1) {
              this.favorites.splice(index, 1)
              this.updateCategoryCounts()
              this.toast.showSuccess('已移除收藏')
            }
          }
        }
      })
    },
    
    batchDownload() {
      if (this.selectedItems.length === 0) return
      
      this.toast.showInfo(`批量下载 ${this.selectedItems.length} 项功能开发中`)
    },
    
    batchRemove() {
      if (this.selectedItems.length === 0) return
      
      uni.showModal({
        title: '批量移除',
        content: `确定要移除选中的 ${this.selectedItems.length} 项收藏吗？`,
        success: (res) => {
          if (res.confirm) {
            this.favorites = this.favorites.filter(f => !this.selectedItems.includes(f.id))
            this.selectedItems = []
            this.updateCategoryCounts()
            this.toast.showSuccess('批量移除成功')
          }
        }
      })
    },
    
    getTypeIcon(type) {
      const icons = {
        artwork: 'picture',
        template: 'layout',
        pattern: 'star'
      }
      return icons[type] || 'picture'
    },
    
    formatDate(timestamp) {
      const date = new Date(timestamp)
      const now = new Date()
      const diff = now - date
      
      if (diff < 86400000) return '今天'
      if (diff < 172800000) return '昨天'
      if (diff < 604800000) return `${Math.floor(diff / 86400000)}天前`
      
      return date.toLocaleDateString()
    }
  }
}
</script>

<style scoped>
.my-favorites-page {
  height: 100vh;
  background-color: var(--color-app-background);
  display: flex;
  flex-direction: column;
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--color-card-background);
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
  width: calc(100% * 64rpx)
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 120rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.nav-right {
  position: absolute;
  right: 32rpx;
  width: 120rpx;
  display: flex;
  align-items: center;
  justify-content: flex-end;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.edit-btn {
  font-size: 28rpx;
  color: var(--color-brand-primary);
  font-weight: 600;
}

.category-tabs {
  display: flex;
  background-color: var(--color-card-background);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  gap: 16rpx;
}

.category-tab {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx 24rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  transition: all 0.2s ease;
  flex-shrink: 0;
}

.category-tab.active {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
  color: #FFFFFF;
}

.category-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.category-tab.active .category-text {
  color: #FFFFFF;
}

.category-count {
  font-size: 20rpx;
  background-color: rgba(255, 255, 255, 0.2);
  padding: 4rpx 8rpx;
  border-radius: 10rpx;
  color: inherit;
}

.content {
  flex: 1;
  padding: 32rpx;
  width: calc(100% - 64rpx)
}

.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 60vh;
  gap: 24rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-secondary);
}

.empty-desc {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.explore-btn {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 48rpx;
  background-color: var(--color-brand-primary);
  color: #FFFFFF;
  border: none;
  border-radius: 16rpx;
  font-size: 28rpx;
  font-weight: 600;
}

.explore-btn::after {
  border: none;
}

.favorites-list {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.favorite-item {
  display: flex;
  align-items: center;
  gap: 24rpx;
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 24rpx;
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.favorite-item:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.favorite-item.edit-mode {
  padding-left: 16rpx;
}

.select-checkbox {
  padding: 8rpx;
}

.item-thumbnail {
  position: relative;
  width: 120rpx;
  height: 120rpx;
  border-radius: 12rpx;
  overflow: hidden;
  background-color: var(--color-app-background);
  flex-shrink: 0;
}

.thumbnail-image {
  width: 100%;
  height: 100%;
}

.thumbnail-placeholder {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.type-badge {
  position: absolute;
  bottom: 8rpx;
  right: 8rpx;
  width: 32rpx;
  height: 32rpx;
  border-radius: 16rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #FFFFFF;
}

.type-badge.artwork {
  background-color: var(--color-brand-primary);
}

.type-badge.template {
  background-color: var(--color-success);
}

.type-badge.pattern {
  background-color: var(--color-warning);
}

.item-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.item-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  display: -webkit-box;
  -webkit-line-clamp: 1;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.item-author {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.item-meta {
  display: flex;
  gap: 16rpx;
}

.item-size, .item-date {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.item-tags {
  display: flex;
  gap: 8rpx;
  flex-wrap: wrap;
}

.tag {
  font-size: 20rpx;
  color: var(--color-brand-primary);
  background-color: rgba(79, 127, 255, 0.1);
  padding: 4rpx 8rpx;
  border-radius: 8rpx;
}

.item-actions {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.action-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  transition: all 0.2s ease;
}

.action-btn:active {
  transform: scale(0.95);
  background-color: var(--border-primary);
}

.batch-actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 32rpx;
  background-color: var(--color-card-background);
  border-top: 2rpx solid var(--border-primary);
}

.selected-count {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.batch-buttons {
  display: flex;
  gap: 16rpx;
}

.batch-btn {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx 24rpx;
  background-color: var(--color-brand-primary);
  color: #FFFFFF;
  border: none;
  border-radius: 12rpx;
  font-size: 24rpx;
  transition: all 0.2s ease;
}

.batch-btn:disabled {
  background-color: var(--color-text-disabled);
  opacity: 0.5;
}

.batch-btn.danger {
  background-color: var(--color-error);
}

.batch-btn::after {
  border: none;
}
</style>