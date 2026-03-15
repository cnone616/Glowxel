<template>
  <view class="my-works-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="goBack">
        <Icon name="direction-left" :size="32" color="var(--color-text-primary)" />
      </view>
      <text class="nav-title">我的作品</text>
      <view class="nav-right" @click="showSortModal = true">
      </view>
    </view>
    
    <!-- 搜索和筛选 -->
    <view class="search-section">
      <view class="search-bar">
        <Icon name="search" :size="28" color="var(--color-text-disabled)" />
        <input 
          v-model="searchQuery"
          class="search-input"
          placeholder="搜索作品名称..."
        />
        <view v-if="searchQuery" class="clear-btn" @click="searchQuery = ''">
          <Icon name="close" :size="24" />
        </view>
      </view>
      
      <view class="filter-tabs">
        <view 
          v-for="filter in filterOptions"
          :key="filter.value"
          class="filter-tab"
          :class="{ active: currentFilter === filter.value }"
          @click="currentFilter = filter.value"
        >
          <text class="filter-text">{{ filter.label }}</text>
          <text v-if="filter.count" class="filter-count">{{ filter.count }}</text>
        </view>
      </view>
    </view>
    
    <!-- 作品网格 -->
    <scroll-view scroll-y class="content" @scrolltolower="loadMore">
      <view v-if="filteredWorks.length === 0" class="empty-state">
        <Icon name="picture" :size="120" color="var(--color-text-disabled)" />
        <text class="empty-title">暂无作品</text>
        <text class="empty-desc">快去创建你的第一个作品吧</text>
        <button class="create-btn" @click="goToCreate">
          <Icon name="plus" :size="32" />
          <text>创建作品</text>
        </button>
      </view>
      
      <view v-else class="works-grid">
        <view 
          v-for="work in filteredWorks"
          :key="work.id"
          class="work-card"
          @click="openWork(work)"
          @longpress="showWorkMenu(work)"
        >
          <view class="work-thumbnail">
            <image 
              v-if="work.thumbnail"
              :src="work.thumbnail"
              class="thumbnail-image"
              mode="aspectFill"
            />
            <view v-else class="thumbnail-placeholder">
              <Icon name="picture" :size="60" color="var(--color-text-disabled)" />
            </view>
            
            <!-- 类型标识 -->
            <view class="type-badge" :class="work.type">
              <text class="type-text">{{ work.type === 'published' ? '已发布' : '草稿' }}</text>
            </view>
            
            <!-- 发布状态标识 -->
            <view v-if="work.type === 'published'" class="published-badge">
              <Icon name="check" :size="20" color="#FFFFFF" />
            </view>
            <view v-else-if="work.isPublished" class="draft-published-badge">
              <Icon name="upload" :size="20" color="#FFFFFF" />
            </view>
          </view>
          
          <view class="work-info">
            <text class="work-name">{{ work.name || work.title }}</text>
            <view class="work-meta">
              <text class="work-size">{{ work.width }}×{{ work.height }}</text>
              <text class="work-date">{{ formatDate(work.updatedAt) }}</text>
            </view>
            <view v-if="work.type === 'published' && work.description" class="work-desc">
              <text class="desc-text">{{ work.description }}</text>
            </view>
          </view>
        </view>
      </view>
      
      <!-- 加载更多 -->
      <view v-if="hasMore" class="load-more">
        <text class="load-text">加载更多...</text>
      </view>
    </scroll-view>
    
    <!-- 排序弹窗 -->
    <view v-if="showSortModal" class="modal-overlay" @click="showSortModal = false">
      <view class="sort-modal" @click.stop>
        <view class="modal-header">
          <text class="modal-title">排序方式</text>
          <view class="modal-close" @click="showSortModal = false">
            <Icon name="close" :size="32" />
          </view>
        </view>
        
        <view class="sort-options">
          <view 
            v-for="option in sortOptions"
            :key="option.value"
            class="sort-option"
            :class="{ active: currentSort === option.value }"
            @click="selectSort(option.value)"
          >
            <text class="sort-text">{{ option.label }}</text>
            <Icon v-if="currentSort === option.value" name="check" :size="24" />
          </view>
        </view>
      </view>
    </view>
    
    <!-- 作品菜单 -->
    <view v-if="selectedWork" class="modal-overlay" @click="selectedWork = null">
      <view class="work-menu" @click.stop>
        <view class="menu-header">
          <text class="menu-title">{{ selectedWork.name }}</text>
        </view>
        
        <view class="menu-actions">
          <view class="menu-action" @click="editWork">
            <Icon name="edit" :size="32" />
            <text class="action-text">编辑</text>
          </view>
          
          <view class="menu-action" @click="duplicateWork">
            <Icon name="copy" :size="32" />
            <text class="action-text">复制</text>
          </view>
          
          <view class="menu-action" @click="shareWork">
            <Icon name="share" :size="32" />
            <text class="action-text">分享</text>
          </view>
          
          <view class="menu-action danger" @click="deleteWork">
            <Icon name="delete" :size="32" />
            <text class="action-text">删除</text>
          </view>
        </view>
      </view>
    </view>
    
    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useProjectStore } from '../../store/project.js'
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
      projectStore: null,
      toast: null,
      searchQuery: '',
      currentFilter: 'all',
      currentSort: 'updated',
      showSortModal: false,
      selectedWork: null,
      hasMore: false,
      filterOptions: [
        { value: 'all', label: '全部', count: 0 },
        { value: 'published', label: '已发布', count: 0 },
        { value: 'reviewing', label: '审核中', count: 0 },
        { value: 'rejected', label: '已退回', count: 0 },
        { value: 'drafts', label: '草稿', count: 0 }
      ],
      sortOptions: [
        { value: 'updated', label: '最近更新' },
        { value: 'created', label: '创建时间' },
        { value: 'name', label: '名称' },
        { value: 'size', label: '尺寸' }
      ]
    }
  },

  computed: {
    // 获取已发布的项目
    publishedWorks() {
      if (!this.projectStore) return []
      return (this.projectStore.projects || []).filter(p => p.status === 'published')
    },

    // 获取未发布的草稿项目
    draftWorks() {
      if (!this.projectStore) return []
      return (this.projectStore.projects || []).filter(p => p.status === 'draft')
    },

    // 获取审核中的项目
    reviewingWorks() {
      if (!this.projectStore) return []
      return (this.projectStore.projects || []).filter(p => p.status === 'reviewing')
    },

    // 获取已退回的项目
    rejectedWorks() {
      if (!this.projectStore) return []
      return (this.projectStore.projects || []).filter(p => p.status === 'rejected')
    },

    // 根据当前筛选获取作品列表
    allWorks() {
      const formatList = (list, type) => list.map(work => ({
        ...work,
        type: type,
        updatedAt: new Date(work.updatedAt || work.createdAt),
        createdAt: new Date(work.createdAt),
        progress: work.progress || 0
      }))

      switch (this.currentFilter) {
        case 'published':
          return formatList(this.publishedWorks, 'published')
        case 'drafts':
          return formatList(this.draftWorks, 'draft')
        case 'reviewing':
          return formatList(this.reviewingWorks, 'reviewing')
        case 'rejected':
          return formatList(this.rejectedWorks, 'rejected')
        case 'all':
          return [
            ...formatList(this.publishedWorks, 'published'),
            ...formatList(this.reviewingWorks, 'reviewing'),
            ...formatList(this.rejectedWorks, 'rejected'),
            ...formatList(this.draftWorks, 'draft')
          ]
        default:
          return []
      }
    },
    
    filteredWorks() {
      let works = [...this.allWorks]
      
      // 搜索过滤
      if (this.searchQuery.trim()) {
        const query = this.searchQuery.toLowerCase()
        works = works.filter(work => 
          work.name.toLowerCase().includes(query) ||
          (work.title && work.title.toLowerCase().includes(query))
        )
      }
      
      // 排序
      works.sort((a, b) => {
        switch (this.currentSort) {
          case 'updated':
            return b.updatedAt - a.updatedAt
          case 'created':
            return b.createdAt - a.createdAt
          case 'name':
            return (a.name || a.title || '').localeCompare(b.name || b.title || '')
          case 'size':
            return (b.width * b.height) - (a.width * a.height)
          default:
            return 0
        }
      })
      
      return works
    }
  },
  
  onLoad() {
    this.projectStore = useProjectStore()
    this.toast = useToast()

    // 加载项目数据
    this.projectStore.loadProjects()
    this.updateFilterCounts()

    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
    })
  },

  onShow() {
    // 页面显示时刷新数据
    if (this.projectStore) {
      this.projectStore.loadProjects()
      this.updateFilterCounts()
    }
  },
  
  methods: {
    goBack() {
      uni.navigateBack()
    },
    
    goToCreate() {
      uni.navigateTo({
        url: '/pages/create/create'
      })
    },
    
    updateFilterCounts() {
      const publishedCount = this.publishedWorks.length
      const draftCount = this.draftWorks.length
      const reviewingCount = this.reviewingWorks.length
      const rejectedCount = this.rejectedWorks.length
      const totalCount = publishedCount + draftCount + reviewingCount + rejectedCount

      this.filterOptions[0].count = totalCount        // 全部
      this.filterOptions[1].count = publishedCount    // 已发布
      this.filterOptions[2].count = reviewingCount    // 审核中
      this.filterOptions[3].count = rejectedCount     // 已退回
      this.filterOptions[4].count = draftCount        // 草稿
    },
    
    selectSort(value) {
      this.currentSort = value
      this.showSortModal = false
    },
    
    openWork(work) {
      if (work.type === 'published') {
        // 查看已发布的作品详情
        this.viewPublishedWork(work)
      } else {
        // 编辑草稿
        this.editDraft(work)
      }
    },
    
    viewPublishedWork(work) {
      // 创建作品详情页面或弹窗显示
      uni.showModal({
        title: work.title || work.name,
        content: `作品尺寸: ${work.width}×${work.height}\n发布时间: ${this.formatDate(work.createdAt)}\n\n${work.description || '暂无描述'}`,
        showCancel: false,
        confirmText: '确定'
      })
    },
    
    editDraft(work) {
      // 编辑草稿 - 先进入看板总览
      uni.navigateTo({
        url: `/pages/overview/overview?id=${work.id}`
      })
    },
    
    showWorkMenu(work) {
      this.selectedWork = work
    },
    
    editWork() {
      if (!this.selectedWork) return
      
      if (this.selectedWork.type === 'published') {
        this.toast.showError('已发布的作品无法编辑')
        this.selectedWork = null
        return
      }
      
      // 编辑草稿
      this.editDraft(this.selectedWork)
      this.selectedWork = null
    },
    
    duplicateWork() {
      if (!this.selectedWork) return
      
      this.projectStore.duplicateProject(this.selectedWork.id)
      this.toast.showSuccess('作品已复制')
      this.selectedWork = null
      this.updateFilterCounts()
    },
    
    shareWork() {
      if (!this.selectedWork) return
      
      // 分享功能
      this.toast.showInfo('分享功能开发中')
      this.selectedWork = null
    },
    
    deleteWork() {
      if (!this.selectedWork) return

      const workName = this.selectedWork.name || this.selectedWork.title
      const typeLabel = this.selectedWork.type === 'published' ? '作品' : '草稿'

      uni.showModal({
        title: '确认删除',
        content: `确定要删除${typeLabel}"${workName}"吗？`,
        success: (res) => {
          if (res.confirm) {
            try {
              this.projectStore.deleteProject(this.selectedWork.id)
              this.toast.showSuccess(`${typeLabel}已删除`)
              this.updateFilterCounts()
            } catch (error) {
              console.error('删除失败:', error)
              this.toast.showError('删除失败')
            }
          }
          this.selectedWork = null
        }
      })
    },
    
    loadMore() {
      // 加载更多逻辑
    },
    
    formatDate(timestamp) {
      const date = new Date(timestamp)
      const now = new Date()
      const diff = now - date
      
      if (diff < 60000) return '刚刚'
      if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
      if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
      if (diff < 604800000) return `${Math.floor(diff / 86400000)}天前`
      
      return date.toLocaleDateString()
    }
  }
}
</script>

<style scoped>
.my-works-page {
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
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.nav-right {
  position: absolute;
  right: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-end;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.search-section {
  background-color: var(--color-card-background);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.search-bar {
  display: flex;
  align-items: center;
  gap: 16rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  padding: 16rpx 24rpx;
  margin-bottom: 24rpx;
}

.search-input {
  flex: 1;
  font-size: 28rpx;
  color: var(--color-text-primary);
}

.clear-btn {
  padding: 8rpx;
  border-radius: 50%;
  background-color: var(--color-text-disabled);
  color: #FFFFFF;
}

.filter-tabs {
  display: flex;
  gap: 16rpx;
}

.filter-tab {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx 24rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  transition: all 0.2s ease;
}

.filter-tab.active {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
}

.filter-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.filter-tab.active .filter-text {
  color: #FFFFFF;
}

.filter-count {
  font-size: 20rpx;
  background-color: rgba(255, 255, 255, 0.2);
  padding: 4rpx 8rpx;
  border-radius: 10rpx;
  color: inherit;
}

.content {
  flex: 1;
  padding: 32rpx;
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

.create-btn {
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

.create-btn::after {
  border: none;
}

.works-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.work-card {
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  overflow: hidden;
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.work-card:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.work-thumbnail {
  position: relative;
  width: 100%;
  height: 200rpx;
  background-color: var(--color-app-background);
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

.progress-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  background-color: rgba(0, 0, 0, 0.7);
  padding: 8rpx 12rpx;
  border-radius: 12rpx;
}

.progress-text {
  font-size: 20rpx;
  color: #FFFFFF;
}

.complete-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  width: 40rpx;
  height: 40rpx;
  background-color: var(--color-success);
  border-radius: 20rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

/* 新增：类型标识 */
.type-badge {
  position: absolute;
  top: 12rpx;
  left: 12rpx;
  padding: 6rpx 12rpx;
  border-radius: 12rpx;
  backdrop-filter: blur(10rpx);
}

.type-badge.published {
  background-color: rgba(0, 243, 255, 0.9);
}

.type-badge.draft {
  background-color: rgba(255, 170, 0, 0.9);
}

.type-text {
  font-size: 18rpx;
  font-weight: 600;
  color: #FFFFFF;
}

/* 发布状态标识 */
.published-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  width: 36rpx;
  height: 36rpx;
  background-color: var(--color-success);
  border-radius: 18rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.draft-published-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  width: 36rpx;
  height: 36rpx;
  background-color: var(--color-brand-primary);
  border-radius: 18rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.work-info {
  padding: 24rpx;
}

.work-name {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 8rpx;
  display: -webkit-box;
  -webkit-line-clamp: 1;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.work-meta {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.work-size, .work-date {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.work-desc {
  margin-top: 8rpx;
}

.desc-text {
  font-size: 20rpx;
  color: var(--color-text-secondary);
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
  line-height: 1.4;
}

.load-more {
  text-align: center;
  padding: 32rpx;
}

.load-text {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

/* 弹窗样式 */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}

.sort-modal, .work-menu {
  background-color: var(--color-card-background);
  border-radius: 24rpx;
  margin: 32rpx;
  max-width: 600rpx;
  width: 100%;
  box-shadow: var(--shadow-modal);
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.modal-title, .menu-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.modal-close {
  padding: 8rpx;
}

.sort-options, .menu-actions {
  padding: 16rpx 0;
}

.sort-option, .menu-action {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 32rpx;
  transition: all 0.2s ease;
}

.sort-option:active, .menu-action:active {
  background-color: var(--color-app-background);
}

.sort-option.active {
  background-color: rgba(79, 127, 255, 0.1);
}

.sort-text, .action-text {
  font-size: 28rpx;
  color: var(--color-text-primary);
}

.menu-action {
  gap: 24rpx;
  justify-content: flex-start;
}

.menu-action.danger .action-text {
  color: var(--color-error);
}

.menu-header {
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}
</style>