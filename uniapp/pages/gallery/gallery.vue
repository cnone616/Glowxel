<template>
  <view class="gallery-page glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 顶部导航 -->
    <view class="header glx-topbar glx-page-shell__fixed">
      <view class="header-content">
        <view class="back-btn" @click="handleBack">
          <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
        </view>
        <text class="header-title glx-topbar__title">{{ pageTitle }}</text>
        <view class="header-placeholder"></view>
      </view>
      
      <!-- 分类标签 -->
      <view class="category-tabs">
        <view class="search-btn" @click="toggleSearch">
          <Icon name="search" :size="34" color="var(--nb-ink)" />
        </view>
        <scroll-view scroll-x class="tabs-scroll">
          <view class="tabs-container">
            <view 
              v-for="(category, index) in categories" 
              :key="category.key"
            class="tab-item glx-choice-chip"
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
      <view v-if="showSearch" class="search-section glx-page-shell__fixed">
      <view class="search-input-wrapper glx-search-shell">
        <Icon name="search" :size="32" color="var(--text-tertiary)" />
        <input 
          v-model="searchTerm"
          type="text"
          class="search-input"
          placeholder="搜索作品、用户..."
          @blur="handleSearchBlur"
          :focus="showSearch"
        />
        <view v-if="searchTerm" class="clear-btn" @click="clearSearch">
          <Icon name="close" :size="28" color="var(--text-tertiary)" />
        </view>
      </view>
    </view>
    
    <!-- 主要内容 -->
    <scroll-view 
      scroll-y 
      class="main-content glx-scroll-region glx-page-shell__content"
      @scrolltolower="loadMore"
      :refresher-enabled="true"
      :refresher-triggered="isRefreshing"
      @refresherrefresh="handleRefresh"
    >
      <!-- 内容列表 -->
      <view class="content-section">
        <view v-if="filteredContent.length === 0 && !isLoading" class="empty-state glx-panel-card">
          <view class="empty-icon">
            <Icon name="picture" :size="80" color="var(--text-tertiary)" />
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
          <Icon name="loading" :size="48" color="currentColor" class="loading-icon" />
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

    <Toast />
  </view>
</template>

<script>
import { artworkAPI, challengeAPI, likeAPI, templateAPI } from '../../api/index.js'
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import ArtworkCard from '../../components/ArtworkCard.vue'
import TemplateCard from '../../components/TemplateCard.vue'
import ChallengeCard from '../../components/ChallengeCard.vue'
import Icon from '../../components/Icon.vue'
import Toast from '../../components/Toast.vue'

const toast = useToast()

export default {
  mixins: [statusBarMixin],
  components: {
    ArtworkCard,
    TemplateCard,
    ChallengeCard,
    Icon,
    Toast
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
          artwork.author_name.toLowerCase().includes(term) ||
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
        toast.showError('加载失败')
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
            newArtworks = await this.getTemplateData()
            break
          case 'challenges':
            newArtworks = await this.getChallengeData()
            break
          default:
            newArtworks = await this.getArtworkData()
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
      if (this.activeCategory === 'following') {
        const res = await artworkAPI.getFollowingArtworks(1, 20)
        return res.success && res.data ? res.data.list : []
      }
      if (this.activeCategory === 'popular') {
        const res = await artworkAPI.getPopularArtworks(1, 20)
        return res.success && res.data ? res.data.list : []
      }
      if (this.activeCategory === 'latest') {
        const res = await artworkAPI.getLatestArtworks(1, 20)
        return res.success && res.data ? res.data.list : []
      }
      const res = await artworkAPI.getArtworks({ page: 1, limit: 20 })
      return res.success && res.data ? res.data.list : []
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
      return res.success && res.data ? res.data.list : []
    },

    async getChallengeData() {
      const status = this.activeCategory === 'all' ? undefined : this.activeCategory
      const res = await challengeAPI.getChallenges({ status, page: 1, limit: 20 })
      return res.success && res.data ? res.data.list : []
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
    
    async handleArtworkLike(data) {
      const { artwork, liked } = data
      
      try {
        if (liked) {
          await likeAPI.likeArtwork(artwork.id)
          this.likedArtworks.add(artwork.id)
          artwork.likes += 1
        } else {
          await likeAPI.unlikeArtwork(artwork.id)
          this.likedArtworks.delete(artwork.id)
          artwork.likes -= 1
        }
      } catch (error) {
        toast.showError('操作失败')
      }
    },
    
    handleArtworkComment(artwork) {
      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${artwork.id}`
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
    
    async handleChallengeJoin(challenge) {
      try {
        const res = await challengeAPI.joinChallenge(challenge.id)
        if (!(res.success && res.data)) {
          throw new Error('join failed')
        }
        challenge.joined = res.data.joined
        if (res.data.changed) {
          challenge.participants += 1
        }
        toast.showSuccess('已参与挑战')
      } catch (error) {
        toast.showError('参与失败')
      }
    }
  }
}
</script>

<style scoped>
.gallery-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--nb-paper);
  overflow: hidden;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--nb-surface);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background-color: var(--nb-surface);
  border-bottom: 1rpx solid var(--nb-ink);
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
  border-radius: 0;
  transition: all 0.2s ease;
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
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
  padding: 0 32rpx 12rpx;
}

.search-btn {
  width: 72rpx;
  height: 72rpx;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  background-color: var(--nb-surface);
  border: 2rpx solid var(--nb-ink);
  box-shadow: none;
  box-sizing: border-box;
  transition: all 0.2s ease;
}

.search-btn:active {
  background-color: var(--nb-yellow);
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
  border-radius: 0;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
}

.tab-item.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.tab-text {
  font-size: 26rpx;
  font-weight: 500;
  color: #4a4a4a;
  white-space: nowrap;
}

.tab-item.active .tab-text {
  color: #000000;
  font-weight: 700;
}

/* 搜索栏 */
.search-section {
  background-color: var(--nb-surface);
  padding: 0 32rpx 20rpx;
  border-bottom: 1rpx solid var(--nb-ink);
}

.search-input {
  flex: 1;
  font-size: 28rpx;
  color: var(--nb-ink);
  background: transparent;
  border: none;
  outline: none;
}

.search-input::placeholder {
  color: #777777;
}

.clear-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
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

.empty-icon {
  width: 160rpx;
  height: 160rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-paper);
  border-radius: 50%;
  margin-bottom: 32rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
  margin-bottom: 12rpx;
}

.empty-subtitle {
  font-size: 26rpx;
  color: #4a4a4a;
  line-height: 1.5;
}

/* 加载状态 */
.loading-more {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48rpx;
  gap: 16rpx;
  color: var(--nb-blue);
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
  color: currentColor;
}

.load-more-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  margin: 32rpx 0;
  background-color: var(--nb-surface);
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
}

.load-more-btn:active {
  background-color: var(--nb-paper);
}

.load-more-text {
  font-size: 28rpx;
  color: var(--nb-yellow);
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
  color: #777777;
}
</style>
