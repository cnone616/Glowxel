<template>
  <view class="home-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 顶部导航 -->
    <view class="header">
      <view class="header-content">
        <Logo :size="80" :show-text="true" />
        <view class="header-actions">
        </view>
      </view>
    </view>
    
    <!-- 主要内容 -->
    <scroll-view scroll-y class="main-content">
      <!-- 横幅轮播 -->
      <view class="banner-section">
        <swiper 
          class="banner-swiper" 
          indicator-dots 
          autoplay 
          interval="5000"
          indicator-color="rgba(255,255,255,0.3)"
          indicator-active-color="#FFFFFF"
        >
          <swiper-item v-for="(banner, index) in banners" :key="index">
            <view class="banner-item" :style="{ background: banner.gradient }">
              <view class="banner-content">
                <text class="banner-title">{{ banner.title }}</text>
                <text class="banner-subtitle">{{ banner.subtitle }}</text>
                <view class="banner-btn" @click="handleBannerClick(banner)">
                  <text class="banner-btn-text">{{ banner.buttonText }}</text>
                </view>
              </view>
              <view class="banner-icon">
                <Icon :name="banner.icon" :size="120" color="rgba(255,255,255,0.8)" />
              </view>
            </view>
          </swiper-item>
        </swiper>
      </view>
      

      
      <!-- 推荐作品 -->
      <view class="recommended-artworks">
        <view class="section-header">
          <text class="section-title">推荐作品</text>
          <view class="more-btn" @click="goToRecommendedArtworks">
            <text class="more-text">查看更多</text>
            <Icon name="arrow-right" :size="24" color="#4F7FFF" />
          </view>
        </view>
        
        <scroll-view scroll-x class="artworks-scroll">
          <view class="artworks-list">
            <ArtworkCard 
              v-for="artwork in recommendedArtworks" 
              :key="artwork.id"
              :artwork="artwork"
              @click="handleArtworkClick"
              @like="handleArtworkLike"
            />
          </view>
        </scroll-view>
      </view>
      
      <!-- 官方模板 -->
      <view class="official-templates">
        <view class="section-header">
          <text class="section-title">官方模板</text>
          <view class="more-btn" @click="goToTemplates">
            <text class="more-text">查看全部</text>
            <Icon name="arrow-right" :size="24" color="#4F7FFF" />
          </view>
        </view>
        
        <view class="templates-grid">
          <TemplateCard 
            v-for="template in officialTemplates" 
            :key="template.id"
            :template="template"
            @click="handleTemplateClick"
          />
        </view>
      </view>
      
      <!-- 每周挑战 -->
      <view class="weekly-challenge">
        <text class="section-title">每周挑战</text>
        <ChallengeCard 
          v-if="weeklyChallenge"
          :challenge="weeklyChallenge"
          @click="handleChallengeClick"
          @join="handleChallengeJoin"
        />
      </view>
    </scroll-view>
  </view>
</template>

<script>
import { MockAPI } from '../../data/mock/index.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Logo from '../../components/Logo.vue'
import ArtworkCard from '../../components/ArtworkCard.vue'
import TemplateCard from '../../components/TemplateCard.vue'
import ChallengeCard from '../../components/ChallengeCard.vue'
import Icon from '../../components/Icon.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Logo,
    ArtworkCard,
    TemplateCard,
    ChallengeCard,
    Icon
  },
  
  data() {
    return {
      searchTerm: '',
      showSearch: false,
      recommendedArtworks: [],
      officialTemplates: [],
      weeklyChallenge: null,
      banners: [
        {
          title: '创意无限',
          subtitle: '用像素点亮想象',
          buttonText: '开始创作',
          icon: 'picture',
          gradient: 'linear-gradient(135deg, #4F7FFF, #FF7A45)',
          action: 'create'
        },
        {
          title: '社区精选',
          subtitle: '发现优秀作品',
          buttonText: '立即探索',
          icon: 'browse',
          gradient: 'linear-gradient(135deg, #2ECC71, #4F7FFF)',
          action: 'community'
        },
        {
          title: '每周挑战',
          subtitle: '参与创作比赛',
          buttonText: '查看挑战',
          icon: 'work',
          gradient: 'linear-gradient(135deg, #FF7A45, #E74C3C)',
          action: 'challenge'
        }
      ]
    }
  },
  
  computed: {
  },

  onLoad() {
    // 加载推荐内容
    this.loadRecommendedContent()
  },

  onShow() {
    // 页面显示时刷新数据
  },

  methods: {
    async loadRecommendedContent() {
      try {
        // 加载推荐作品
        this.recommendedArtworks = MockAPI.artworks.getPopular().slice(0, 6)
        
        // 加载官方模板
        this.officialTemplates = MockAPI.templates.getPopular().slice(0, 4)
        
        // 加载每周挑战
        const challenges = MockAPI.challenges.getActive()
        this.weeklyChallenge = challenges.find(c => c.type === 'weekly') || challenges[0]
      } catch (error) {
        console.error('加载推荐内容失败:', error)
      }
    },
    
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
    
    handleBannerClick(banner) {
      switch (banner.action) {
        case 'create':
          this.goToCreate()
          break
        case 'community':
          this.goToCommunity()
          break
        case 'challenge':
          this.goToChallenges()
          break
      }
    },
    
    goToCreate() {
      uni.navigateTo({
        url: '/pages/create/create'
      })
    },
    
    goToTemplates() {
      uni.navigateTo({
        url: '/pages/gallery/gallery?type=templates&title=' + encodeURIComponent('官方模板')
      })
    },
    
    goToImport() {
      uni.navigateTo({
        url: '/pages/create/create?mode=image'
      })
    },
    
    goToCommunity() {
      uni.switchTab({
        url: '/pages/community/community'
      })
    },
    
    goToRecommendedArtworks() {
      uni.navigateTo({
        url: '/pages/gallery/gallery?type=artworks&title=' + encodeURIComponent('推荐作品')
      })
    },
    
    goToChallenges() {
      uni.navigateTo({
        url: '/pages/gallery/gallery?type=challenges&title=' + encodeURIComponent('热门挑战')
      })
    },
    
    handleArtworkClick(artwork) {
      // 跳转到作品详情页
      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${artwork.id}`
      })
    },
    
    handleArtworkLike(data) {
      console.log('点赞作品:', data)
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
.home-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--color-app-background);
  overflow: hidden;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--color-app-background);
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

.header-actions {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.search-btn {
  width: 80rpx;
  height: 80rpx;
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
}

/* 横幅轮播 */
.banner-section {
  margin: 32rpx;
  margin-bottom: 48rpx;
}

.banner-swiper {
  height: 320rpx;
  border-radius: var(--radius-large);
  overflow: hidden;
}

.banner-item {
  position: relative;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 48rpx;
  overflow: hidden;
}

.banner-content {
  flex: 1;
  z-index: 2;
}

.banner-title {
  font-size: 48rpx;
  font-weight: 700;
  color: #FFFFFF;
  line-height: 1.2;
  margin-bottom: 12rpx;
}

.banner-subtitle {
  font-size: 28rpx;
  color: rgba(255, 255, 255, 0.9);
  line-height: 1.4;
  margin-bottom: 32rpx;
}

.banner-btn {
  display: inline-flex;
  align-items: center;
  padding: 16rpx 32rpx;
  background-color: rgba(255, 255, 255, 0.2);
  border-radius: var(--radius-small);
  backdrop-filter: blur(10rpx);
  transition: all 0.2s ease;
}

.banner-btn:active {
  transform: scale(0.95);
  background-color: rgba(255, 255, 255, 0.3);
}

.banner-btn-text {
  font-size: 28rpx;
  font-weight: 600;
  color: #FFFFFF;
}

.banner-icon {
  position: absolute;
  right: 48rpx;
  top: 50%;
  transform: translateY(-50%);
  opacity: 0.6;
  z-index: 1;
}

/* 通用区块样式 */
.section-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
  margin-bottom: 24rpx;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24rpx;
}

.project-count {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  font-weight: 500;
}

.more-btn {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 12rpx 16rpx;
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
}

.more-btn:active {
  background-color: rgba(79, 127, 255, 0.1);
  transform: scale(0.95);
}

.more-text {
  font-size: 26rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}

/* 我的画布 */
.my-projects {
  margin: 0 32rpx 48rpx;
}

.projects-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 64rpx 32rpx;
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
  margin-bottom: 32rpx;
}

.empty-btn {
  padding: 20rpx 40rpx;
  background-color: var(--color-brand-primary);
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
}

.empty-btn:active {
  transform: scale(0.95);
}

.empty-btn-text {
  font-size: 28rpx;
  font-weight: 600;
  color: #FFFFFF;
}

/* 推荐作品 */
.recommended-artworks {
  margin: 0 0 48rpx;
}

.recommended-artworks .section-header {
  margin: 0 32rpx 24rpx;
}

.artworks-scroll {
  white-space: nowrap;
}

.artworks-list {
  display: flex;
  gap: 24rpx;
  padding: 0 32rpx;
}

.artworks-list .artwork-card {
  flex-shrink: 0;
  width: 280rpx;
}

/* 官方模板 */
.official-templates {
  margin: 0 32rpx 48rpx;
}

.templates-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

/* 每周挑战 */
.weekly-challenge {
  margin: 0 32rpx 48rpx;
}
</style>

