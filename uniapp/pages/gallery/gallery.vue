<template>
  <view class="gallery-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 顶部导航 -->
    <view class="header">
      <view class="header-content">
        <view class="back-btn" @click="handleBack">
          <Icon name="direction-left" :size="40" color="#1F1F1F" />
        </view>
        <text class="header-title">{{ pageTitle }}</text>
        <view class="header-placeholder"></view>
      </view>
      
      <!-- 分类标签 -->
      <view class="category-tabs">
        <view class="search-btn" @click="toggleSearch">
          <Icon name="search" :size="40" color="#4F7FFF" />
        </view>
        <scroll-view scroll-x class="tabs-scroll">
          <view class="tabs-container">
            <view 
              v-for="(category, index) in categories" 
              :key="category.key"
              class="tab-item"
              :class="{ 'active': activeCategory === category.key }"
              @click="switchCategory(category.key)"
            >
              <text class="tab-text">{{ category.name }}</text>
            </view>
          </view>
        </scroll-view>
      </view>
    </view>
    
    <!-- 搜索栏（可展开） -->
    <view v-if="showSearch" class="search-section">
      <view class="search-input-wrapper">
        <Icon name="search" :size="32" color="#666666" />
        <input 
          v-model="searchTerm"
          type="text"
          class="search-input"
          placeholder="搜索作品、用户..."
          @blur="handleSearchBlur"
          :focus="showSearch"
        />
        <view v-if="searchTerm" class="clear-btn" @click="clearSearch">
          <Icon name="close" :size="28" color="#AAAAAA" />
        </view>
      </view>
    </view>
    
    <!-- 主要内容 -->
    <scroll-view 
      scroll-y 
      class="main-content"
      @scrolltolower="loadMore"
      :refresher-enabled="true"
      :refresher-triggered="isRefreshing"
      @refresherrefresh="handleRefresh"
    >
      <!-- 内容列表 -->
      <view class="content-section">
        <view v-if="filteredContent.length === 0 && !isLoading" class="empty-state">
          <view class="empty-icon">
            <Icon name="picture" :size="80" color="#AAAAAA" />
          </view>
          <text class="empty-title">{{ getEmptyTitle() }}</text>
          <text class="empty-subtitle">{{ getEmptySubtitle() }}</text>
        </view>
        
        <view v-else class="content-grid">
          <!-- 作品卡片 -->
          <ArtworkCard 
            v-if="pageType === 'artworks'"
            v-for="artwork in filteredContent" 
            :key="artwork.id"
            :artwork="artwork"
            :liked="likedArtworks.has(artwork.id)"
            @click="handleArtworkClick"
            @like="handleArtworkLike"
            @comment="handleArtworkComment"
          />
          
          <!-- 模板卡片 -->
          <TemplateCard 
            v-if="pageType === 'templates'"
            v-for="template in filteredContent" 
            :key="template.id"
            :template="template"
            @click="handleTemplateClick"
          />
          
          <!-- 挑战卡片 -->
          <ChallengeCard 
            v-if="pageType === 'challenges'"
            v-for="challenge in filteredContent" 
            :key="challenge.id"
            :challenge="challenge"
            @click="handleChallengeClick"
            @join="handleChallengeJoin"
          />
        </view>
        
        <!-- 加载更多 -->
        <view v-if="isLoading" class="loading-more">
          <Icon name="loading" :size="48" color="#4F7FFF" class="loading-icon" />
          <text class="loading-text">加载中...</text>
        </view>
        
        <view v-if="hasMore && !isLoading && filteredContent.length > 0" class="load-more-btn" @click="loadMore">
          <text class="load-more-text">加载更多</text>
        </view>
        
        <view v-if="!hasMore && filteredContent.length > 0" class="no-more">
          <text class="no-more-text">没有更多了</text>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import { artworkAPI, templateAPI, challengeAPI } from '../../api/index.js'
import statusBarMixin from '../../mixins/statusBar.js'
import ArtworkCard from '../../components/ArtworkCard.vue'
import TemplateCard from '../../components/TemplateCard.vue'
import ChallengeCard from '../../components/ChallengeCard.vue'
import Icon from '../../components/Icon.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    ArtworkCard,
    TemplateCard,
    ChallengeCard,
    Icon
  },
  
  data() {
    return {
      // 页面参数
      pageType: 'artworks', // artworks, templates, challenges
      pageTitle: '推荐作品',
      
      // 搜索相关
      searchTerm: '',
      showSearch: false,
      
      // 分类相关
      activeCategory: 'all',
      categories: [
        { key: 'all', name: '全部' },
        { key: 'popular', name: '最热' },
        { key: 'latest', name: '最新' },
        { key: 'following', name: '关注' }
      ],
      
      // 数据相关
      artworks: [],
      likedArtworks: new Set(),
      isLoading: false,
      isRefreshing: false,
      hasMore: true,
      currentPage: 1,
      pageSize: 20
    }
  },
  
  computed: {
    filteredContent() {
      if (!this.searchTerm) {
        return this.artworks
      }
      
      const term = this.searchTerm.toLowerCase()
      
      // 根据页面类型进行不同的搜索
      if (this.pageType === 'templates') {
        return this.artworks.filter(template => 
          template.name.toLowerCase().includes(term) ||
          template.category.toLowerCase().includes(term) ||
          (template.tags && template.tags.some(tag => tag.toLowerCase().includes(term)))
        )
      } else if (this.pageType === 'challenges') {
        return this.artworks.filter(challenge => 
          challenge.title.toLowerCase().includes(term) ||
          challenge.description.toLowerCase().includes(term) ||
          (challenge.tags && challenge.tags.some(tag => tag.toLowerCase().includes(term)))
        )
      } else {
        // artworks
        return this.artworks.filter(artwork => 
          artwork.title.toLowerCase().includes(term) ||
          artwork.author.name.toLowerCase().includes(term) ||
          (artwork.tags && artwork.tags.some(tag => tag.toLowerCase().includes(term)))
        )
      }
    }
  },
  
  onLoad(options) {
    // 根据传入参数设置页面类型和标题
    this.pageType = options.type || 'artworks'
    this.pageTitle = this.getPageTitle(options.title)
    
    // 根据页面类型设置分类
    this.setupCategories()
    
    // 加载数据
    this.loadInitialData()
  },
  
  methods: {
    getPageTitle(customTitle) {
      if (customTitle) return decodeURIComponent(customTitle)
      
      const titleMap = {
        artworks: '推荐作品',
        templates: '官方模板',
        challenges: '热门挑战',
        popular: '热门作品',
        latest: '最新作品'
      }
      return titleMap[this.pageType] || '作品展示'
    },
    
    setupCategories() {
      // 根据页面类型设置不同的分类
      const categoryMap = {
        artworks: [
          { key: 'all', name: '全部' },
          { key: 'popular', name: '最热' },
          { key: 'latest', name: '最新' },
          { key: 'following', name: '关注' }
        ],
        templates: [
          { key: 'all', name: '全部' },
          { key: 'game', name: '游戏角色' },
          { key: 'anime', name: '动漫角色' },
          { key: 'symbol', name: '图案符号' },
          { key: 'fantasy', name: '奇幻生物' },
          { key: 'nature', name: '自然植物' },
          { key: 'scifi', name: '科幻载具' },
          { key: 'food', name: '美食' },
          { key: 'building', name: '建筑' }
        ],
        challenges: [
          { key: 'all', name: '全部' },
          { key: 'active', name: '进行中' },
          { key: 'upcoming', name: '即将开始' },
          { key: 'ended', name: '已结束' }
        ]
      }
      
      this.categories = categoryMap[this.pageType] || categoryMap.artworks
    },
    
    async loadInitialData() {
      this.isLoading = true
      
      try {
        await this.loadArtworks(true)
      } catch (error) {
        console.error('加载数据失败:', error)
        uni.showToast({
          title: '加载失败',
          icon: 'error'
        })
      } finally {
        this.isLoading = false
      }
    },
    
    async loadArtworks(reset = false) {
      if (reset) {
        this.currentPage = 1
        this.artworks = []
        this.hasMore = true
      }
      
      try {
        let newArtworks = []
        
        // 根据页面类型和分类获取数据
        switch (this.pageType) {
          case 'templates':
            newArtworks = this.getTemplateData()
            break
          case 'challenges':
            newArtworks = this.getChallengeData()
            break
          default:
            newArtworks = this.getArtworkData()
            break
        }
        
        // 模拟分页
        const startIndex = (this.currentPage - 1) * this.pageSize
        const endIndex = startIndex + this.pageSize
        const pageData = newArtworks.slice(startIndex, endIndex)
        
        if (reset) {
          this.artworks = pageData
        } else {
          this.artworks = [...this.artworks, ...pageData]
        }
        
        this.hasMore = endIndex < newArtworks.length
        this.currentPage++
      } catch (error) {
        console.error('加载作品失败:', error)
        throw error
      }
    },
    
    async getArtworkData() {
      const sort = this.activeCategory === 'popular' ? 'popular' : this.activeCategory === 'latest' ? 'latest' : 'latest'
      const res = this.activeCategory === 'following'
        ? await artworkAPI.getFollowingArtworks(1, 20)
        : await artworkAPI.getArtworks({ page: 1, limit: 20, sort })
      return res.success ? (res.data?.list || []) : []
    },

    async getTemplateData() {
      const categoryNameMap = {
        'game': '游戏角色',
        'anime': '动漫角色',
        'symbol': '图案符号',
        'fantasy': '奇幻生物',
        'nature': '自然植物',
        'scifi': '科幻载具',
        'food': '美食',
        'building': '建筑'
      }
      const category = this.activeCategory === 'all' ? undefined : categoryNameMap[this.activeCategory]
      const res = await templateAPI.getTemplates({ category, page: 1, limit: 20 })
      return res.success ? (res.data?.list || []) : []
    },

    async getChallengeData() {
      const status = this.activeCategory === 'all' ? undefined : this.activeCategory
      const res = await challengeAPI.getChallenges({ status, page: 1, limit: 20 })
      return res.success ? (res.data?.list || []) : []
    },
    
    async handleRefresh() {
      this.isRefreshing = true
      
      try {
        await this.loadArtworks(true)
      } catch (error) {
        console.error('刷新失败:', error)
      } finally {
        this.isRefreshing = false
      }
    },
    
    async loadMore() {
      if (this.isLoading || !this.hasMore) return
      
      this.isLoading = true
      
      try {
        await this.loadArtworks(false)
      } catch (error) {
        console.error('加载更多失败:', error)
      } finally {
        this.isLoading = false
      }
    },
    
    async switchCategory(category) {
      if (this.activeCategory === category) return
      
      this.activeCategory = category
      await this.loadArtworks(true)
    },
    
    handleBack() {
      uni.navigateBack()
    },
    
    toggleSearch() {
      this.showSearch = !this.showSearch
      if (!this.showSearch) {
        this.searchTerm = ''
      }
    },
    
    handleSearchBlur() {
      if (!this.searchTerm) {
        this.showSearch = false
      }
    },
    
    clearSearch() {
      this.searchTerm = ''
    },
    
    handleArtworkClick(artwork) {
      // 跳转到作品详情页
      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${artwork.id}`
      })
    },
    
    handleArtworkLike(data) {
      const { artwork, liked } = data
      
      if (liked) {
        this.likedArtworks.add(artwork.id)
        artwork.likes = (artwork.likes || 0) + 1
      } else {
        this.likedArtworks.delete(artwork.id)
        artwork.likes = Math.max(0, (artwork.likes || 0) - 1)
      }
      
      console.log('点赞作品:', artwork.id, liked)
    },
    
    handleArtworkComment(artwork) {
      // TODO: 跳转到评论页面
      uni.showToast({
        title: '评论功能开发中',
        icon: 'none'
      })
    },
    
    getEmptyTitle() {
      if (this.searchTerm) {
        return '未找到相关内容'
      }
      
      const titleMap = {
        artworks: '暂无作品',
        templates: '暂无模板',
        challenges: '暂无挑战'
      }
      return titleMap[this.pageType] || '暂无内容'
    },
    
    getEmptySubtitle() {
      if (this.searchTerm) {
        return '尝试其他关键词'
      }
      
      const subtitleMap = {
        artworks: '快来发布第一个作品吧！',
        templates: '敬请期待更多模板！',
        challenges: '敬请期待精彩挑战！'
      }
      return subtitleMap[this.pageType] || '敬请期待！'
    },
    
    handleTemplateClick(template) {
      // 使用模板创建画布
      uni.navigateTo({
        url: `/pages/create/create?templateId=${template.id}`
      })
    },
    
    handleChallengeClick(challenge) {
      // 跳转到挑战详情页
      uni.navigateTo({
        url: `/pages/challenge-detail/challenge-detail?id=${challenge.id}`
      })
    },
    
    handleChallengeJoin(challenge) {
      uni.showToast({
        title: '已参与挑战',
        icon: 'success'
      })
    }
  }
}
</script>

<style scoped>
.gallery-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--color-app-background);
  overflow: hidden;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--color-card-background);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background-color: var(--color-card-background);
  border-bottom: 1rpx solid var(--border-primary);
  flex-shrink: 0;
}

.header-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 32rpx;
}

.back-btn {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-medium);
  transition: all 0.2s ease;
}

.back-btn:active {
  transform: scale(0.95);
  background-color: var(--color-app-background);
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
  flex: 1;
  text-align: center;
}

.header-placeholder {
  width: 80rpx;
  height: 80rpx;
}

/* 分类标签 */
.category-tabs {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 0 32rpx 24rpx;
}

.search-btn {
  width: 80rpx;
  height: 80rpx;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-medium);
  background-color: var(--color-app-background);
  transition: all 0.2s ease;
}

.search-btn:active {
  transform: scale(0.95);
  background-color: rgba(79, 127, 255, 0.1);
}

.tabs-scroll {
  flex: 1;
  white-space: nowrap;
}

.tabs-container {
  display: flex;
  gap: 16rpx;
}

.tab-item {
  flex-shrink: 0;
  padding: 16rpx 24rpx;
  border-radius: var(--radius-small);
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.tab-item.active {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
}

.tab-item:active {
  transform: scale(0.95);
}

.tab-text {
  font-size: 26rpx;
  font-weight: 500;
  color: var(--color-text-secondary);
  white-space: nowrap;
}

.tab-item.active .tab-text {
  color: #FFFFFF;
  font-weight: 600;
}

/* 搜索栏 */
.search-section {
  background-color: var(--color-card-background);
  padding: 0 32rpx 24rpx;
  border-bottom: 1rpx solid var(--border-primary);
}

.search-input-wrapper {
  display: flex;
  align-items: center;
  gap: 16rpx;
  background-color: var(--color-app-background);
  border-radius: var(--radius-medium);
  padding: 20rpx 24rpx;
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
  box-sizing: border-box; /* 修复溢出问题 */
}

.search-input-wrapper:focus-within {
  border-color: var(--color-brand-primary);
  box-shadow: 0 0 0 4rpx rgba(79, 127, 255, 0.1);
}

.search-input {
  flex: 1;
  font-size: 28rpx;
  color: var(--color-text-primary);
  background: transparent;
  border: none;
  outline: none;
}

.search-input::placeholder {
  color: var(--color-text-disabled);
}

.clear-btn {
  width: 56rpx;
  height: 56rpx;
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

/* 主要内容 */
.main-content {
  flex: 1;
  overflow-y: auto;
  padding-bottom: 80rpx; /* 增加底部内边距以避免被底部导航栏遮挡 */
}

/* 内容区域 */
.content-section {
  margin: 32rpx;
}

.content-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
  margin-bottom: 48rpx;
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 96rpx 32rpx;
  text-align: center;
}

.empty-icon {
  width: 160rpx;
  height: 160rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: 50%;
  margin-bottom: 32rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 12rpx;
}

.empty-subtitle {
  font-size: 26rpx;
  color: var(--color-text-secondary);
  line-height: 1.5;
}

/* 加载状态 */
.loading-more {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48rpx;
  gap: 16rpx;
}

.loading-icon {
  animation: rotate 1s linear infinite;
}

@keyframes rotate {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

.loading-text {
  font-size: 26rpx;
  color: var(--color-text-secondary);
}

.load-more-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  margin: 32rpx 0;
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.load-more-btn:active {
  transform: scale(0.98);
  background-color: var(--color-app-background);
}

.load-more-text {
  font-size: 28rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}

.no-more {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
}

.no-more-text {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}
</style>