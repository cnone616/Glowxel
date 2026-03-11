<template>
  <view class="achievements-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="goBack">
        <Icon name="arrow-left" :size="32" />
      </view>
      <text class="nav-title">成就</text>
      <view class="nav-right" @click="shareAchievements">
        <Icon name="share" :size="32" />
      </view>
    </view>
    
    <!-- 成就统计 -->
    <view class="stats-section">
      <view class="stats-card">
        <view class="stat-item">
          <text class="stat-number">{{ unlockedCount }}</text>
          <text class="stat-label">已解锁</text>
        </view>
        <view class="stat-divider"></view>
        <view class="stat-item">
          <text class="stat-number">{{ totalCount }}</text>
          <text class="stat-label">总成就</text>
        </view>
        <view class="stat-divider"></view>
        <view class="stat-item">
          <text class="stat-number">{{ completionRate }}%</text>
          <text class="stat-label">完成度</text>
        </view>
      </view>
      
      <view class="progress-section">
        <text class="progress-title">成就进度</text>
        <view class="progress-bar">
          <view 
            class="progress-fill"
            :style="{ width: completionRate + '%' }"
          ></view>
        </view>
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
    
    <!-- 成就列表 -->
    <scroll-view scroll-y class="content">
      <view class="achievements-grid">
        <view 
          v-for="achievement in filteredAchievements"
          :key="achievement.id"
          class="achievement-card"
          :class="{ 
            unlocked: achievement.unlocked,
            rare: achievement.rarity === 'rare',
            epic: achievement.rarity === 'epic',
            legendary: achievement.rarity === 'legendary'
          }"
          @click="showAchievementDetail(achievement)"
        >
          <!-- 成就图标 -->
          <view class="achievement-icon">
            <Icon 
              :name="achievement.icon" 
              :size="80" 
              :color="getIconColor(achievement)"
            />
            
            <!-- 稀有度光效 -->
            <view v-if="achievement.unlocked && achievement.rarity !== 'common'" class="rarity-glow"></view>
            
            <!-- 未解锁遮罩 -->
            <view v-if="!achievement.unlocked" class="locked-overlay">
              <Icon name="lock" :size="40" color="var(--color-text-disabled)" />
            </view>
          </view>
          
          <!-- 成就信息 -->
          <view class="achievement-info">
            <text class="achievement-title">{{ achievement.title }}</text>
            <text class="achievement-desc">{{ achievement.description }}</text>
            
            <!-- 进度条 -->
            <view v-if="achievement.progress !== undefined" class="achievement-progress">
              <view class="progress-info">
                <text class="progress-text">{{ achievement.current }}/{{ achievement.target }}</text>
                <text class="progress-percent">{{ Math.round((achievement.current / achievement.target) * 100) }}%</text>
              </view>
              <view class="progress-bar-small">
                <view 
                  class="progress-fill-small"
                  :style="{ width: ((achievement.current / achievement.target) * 100) + '%' }"
                ></view>
              </view>
            </view>
            
            <!-- 解锁时间 -->
            <text v-if="achievement.unlocked && achievement.unlockedAt" class="unlock-time">
              {{ formatUnlockTime(achievement.unlockedAt) }}
            </text>
            
            <!-- 稀有度标签 -->
            <view class="rarity-badge" :class="achievement.rarity">
              <text class="rarity-text">{{ getRarityText(achievement.rarity) }}</text>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>
    
    <!-- 成就详情弹窗 -->
    <view v-if="selectedAchievement" class="modal-overlay" @click="selectedAchievement = null">
      <view class="achievement-modal" @click.stop>
        <view class="modal-close" @click="selectedAchievement = null">
          <Icon name="close" :size="32" />
        </view>
        
        <view class="modal-icon">
          <Icon 
            :name="selectedAchievement.icon" 
            :size="120" 
            :color="getIconColor(selectedAchievement)"
          />
          <view v-if="selectedAchievement.unlocked && selectedAchievement.rarity !== 'common'" class="modal-glow"></view>
        </view>
        
        <text class="modal-title">{{ selectedAchievement.title }}</text>
        <text class="modal-desc">{{ selectedAchievement.description }}</text>
        
        <view v-if="selectedAchievement.unlocked" class="modal-unlocked">
          <Icon name="check-circle" :size="40" color="var(--color-success)" />
          <text class="unlocked-text">已解锁</text>
          <text class="unlock-date">{{ formatUnlockTime(selectedAchievement.unlockedAt) }}</text>
        </view>
        
        <view v-else class="modal-locked">
          <view v-if="selectedAchievement.progress !== undefined" class="modal-progress">
            <text class="progress-label">完成进度</text>
            <view class="progress-bar-modal">
              <view 
                class="progress-fill-modal"
                :style="{ width: ((selectedAchievement.current / selectedAchievement.target) * 100) + '%' }"
              ></view>
            </view>
            <text class="progress-detail">{{ selectedAchievement.current }}/{{ selectedAchievement.target }}</text>
          </view>
          
          <text class="hint-text">{{ selectedAchievement.hint || '继续努力，即将解锁！' }}</text>
        </view>
        
        <view class="modal-rarity">
          <text class="rarity-label">稀有度:</text>
          <view class="rarity-badge" :class="selectedAchievement.rarity">
            <text class="rarity-text">{{ getRarityText(selectedAchievement.rarity) }}</text>
          </view>
        </view>
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
      selectedAchievement: null,
      categories: [
        { value: 'all', label: '全部', icon: 'grid', count: 0 },
        { value: 'create', label: '创作', icon: 'palette', count: 0 },
        { value: 'social', label: '社交', icon: 'users', count: 0 },
        { value: 'skill', label: '技能', icon: 'star', count: 0 },
        { value: 'special', label: '特殊', icon: 'award', count: 0 }
      ],
      achievements: [
        // 创作类成就
        {
          id: '1',
          category: 'create',
          title: '初出茅庐',
          description: '完成第一个作品',
          icon: 'picture',
          rarity: 'common',
          unlocked: true,
          unlockedAt: Date.now() - 86400000 * 30,
          current: 1,
          target: 1
        },
        {
          id: '2',
          category: 'create',
          title: '作品达人',
          description: '创作10个作品',
          icon: 'gallery',
          rarity: 'common',
          unlocked: true,
          unlockedAt: Date.now() - 86400000 * 15,
          current: 12,
          target: 10
        },
        {
          id: '3',
          category: 'create',
          title: '像素大师',
          description: '创作50个作品',
          icon: 'crown',
          rarity: 'rare',
          unlocked: false,
          current: 12,
          target: 50,
          hint: '继续创作更多精彩作品'
        },
        {
          id: '4',
          category: 'create',
          title: '传奇创作者',
          description: '创作100个作品',
          icon: 'trophy',
          rarity: 'legendary',
          unlocked: false,
          current: 12,
          target: 100,
          hint: '成为真正的像素艺术传奇'
        },
        
        // 社交类成就
        {
          id: '5',
          category: 'social',
          title: '社交新手',
          description: '获得第一个粉丝',
          icon: 'heart',
          rarity: 'common',
          unlocked: true,
          unlockedAt: Date.now() - 86400000 * 20,
          current: 1,
          target: 1
        },
        {
          id: '6',
          category: 'social',
          title: '人气作者',
          description: '获得100个粉丝',
          icon: 'users',
          rarity: 'rare',
          unlocked: false,
          current: 45,
          target: 100,
          hint: '分享更多优质作品吸引粉丝'
        },
        {
          id: '7',
          category: 'social',
          title: '点赞收割机',
          description: '作品获得1000个点赞',
          icon: 'thumbs-up',
          rarity: 'epic',
          unlocked: false,
          current: 234,
          target: 1000,
          hint: '创作更受欢迎的作品'
        },
        
        // 技能类成就
        {
          id: '8',
          category: 'skill',
          title: '色彩搭配师',
          description: '使用超过20种颜色创作',
          icon: 'palette',
          rarity: 'common',
          unlocked: true,
          unlockedAt: Date.now() - 86400000 * 10,
          current: 25,
          target: 20
        },
        {
          id: '9',
          category: 'skill',
          title: '精密工匠',
          description: '完成一个超过1000像素的作品',
          icon: 'target',
          rarity: 'rare',
          unlocked: false,
          current: 0,
          target: 1,
          hint: '挑战更大尺寸的创作'
        },
        
        // 特殊成就
        {
          id: '10',
          category: 'special',
          title: '早起的鸟儿',
          description: '在早上6点前完成创作',
          icon: 'sun',
          rarity: 'epic',
          unlocked: false,
          current: 0,
          target: 1,
          hint: '早起创作有奇效'
        },
        {
          id: '11',
          category: 'special',
          title: '夜猫子',
          description: '在深夜12点后完成创作',
          icon: 'moon',
          rarity: 'epic',
          unlocked: true,
          unlockedAt: Date.now() - 86400000 * 5,
          current: 1,
          target: 1
        },
        {
          id: '12',
          category: 'special',
          title: '完美主义者',
          description: '连续7天都有创作',
          icon: 'calendar',
          rarity: 'legendary',
          unlocked: false,
          current: 3,
          target: 7,
          hint: '保持每日创作的习惯'
        }
      ]
    }
  },
  
  computed: {
    filteredAchievements() {
      if (this.currentCategory === 'all') {
        return this.achievements
      }
      return this.achievements.filter(a => a.category === this.currentCategory)
    },
    
    unlockedCount() {
      return this.achievements.filter(a => a.unlocked).length
    },
    
    totalCount() {
      return this.achievements.length
    },
    
    completionRate() {
      return Math.round((this.unlockedCount / this.totalCount) * 100)
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
    
    updateCategoryCounts() {
      this.categories[0].count = this.achievements.length
      this.categories[1].count = this.achievements.filter(a => a.category === 'create').length
      this.categories[2].count = this.achievements.filter(a => a.category === 'social').length
      this.categories[3].count = this.achievements.filter(a => a.category === 'skill').length
      this.categories[4].count = this.achievements.filter(a => a.category === 'special').length
    },
    
    showAchievementDetail(achievement) {
      this.selectedAchievement = achievement
    },
    
    shareAchievements() {
      this.toast.showInfo('分享功能开发中')
    },
    
    getIconColor(achievement) {
      if (!achievement.unlocked) {
        return 'var(--color-text-disabled)'
      }
      
      switch (achievement.rarity) {
        case 'rare': return '#4F7FFF'
        case 'epic': return '#9333EA'
        case 'legendary': return '#F59E0B'
        default: return 'var(--color-text-primary)'
      }
    },
    
    getRarityText(rarity) {
      const rarityMap = {
        common: '普通',
        rare: '稀有',
        epic: '史诗',
        legendary: '传说'
      }
      return rarityMap[rarity] || '普通'
    },
    
    formatUnlockTime(timestamp) {
      const date = new Date(timestamp)
      return date.toLocaleDateString() + ' 解锁'
    }
  }
}
</script>

<style scoped>
.achievements-page {
  height: 100vh;
  background-color: var(--color-app-background);
  display: flex;
  flex-direction: column;
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 32rpx;
  background-color: var(--color-card-background);
  border-bottom: 2rpx solid var(--border-primary);
}

.nav-left, .nav-right {
  width: 80rpx;
  display: flex;
  align-items: center;
}

.nav-right {
  justify-content: flex-end;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.stats-section {
  background-color: var(--color-card-background);
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.stats-card {
  display: flex;
  align-items: center;
  margin-bottom: 32rpx;
}

.stat-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
}

.stat-number {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.stat-label {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.stat-divider {
  width: 2rpx;
  height: 60rpx;
  background-color: var(--border-primary);
  margin: 0 32rpx;
}

.progress-section {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.progress-title {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  font-weight: 500;
}

.progress-bar {
  height: 16rpx;
  background-color: var(--color-app-background);
  border-radius: 8rpx;
  overflow: hidden;
}

.progress-fill {
  height: 100%;
  background: linear-gradient(90deg, var(--color-brand-primary), var(--color-brand-accent));
  transition: width 0.3s ease;
}

.category-tabs {
  display: flex;
  background-color: var(--color-card-background);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  gap: 16rpx;
  overflow-x: auto;
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
}

.achievements-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.achievement-card {
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 32rpx;
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
  position: relative;
  overflow: hidden;
}

.achievement-card:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.achievement-card.unlocked {
  border: 2rpx solid rgba(79, 127, 255, 0.3);
}

.achievement-card.rare.unlocked {
  border-color: rgba(79, 127, 255, 0.5);
  box-shadow: 0 0 20rpx rgba(79, 127, 255, 0.2);
}

.achievement-card.epic.unlocked {
  border-color: rgba(147, 51, 234, 0.5);
  box-shadow: 0 0 20rpx rgba(147, 51, 234, 0.2);
}

.achievement-card.legendary.unlocked {
  border-color: rgba(245, 158, 11, 0.5);
  box-shadow: 0 0 20rpx rgba(245, 158, 11, 0.2);
}

.achievement-icon {
  position: relative;
  width: 120rpx;
  height: 120rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: 60rpx;
  margin: 0 auto 24rpx;
}

.rarity-glow {
  position: absolute;
  inset: -8rpx;
  border-radius: 68rpx;
  background: conic-gradient(from 0deg, transparent, var(--color-brand-primary), transparent);
  animation: rotate 3s linear infinite;
  z-index: -1;
}

.locked-overlay {
  position: absolute;
  inset: 0;
  background-color: rgba(0, 0, 0, 0.5);
  border-radius: 60rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.achievement-info {
  text-align: center;
}

.achievement-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 8rpx;
  display: block;
}

.achievement-desc {
  font-size: 22rpx;
  color: var(--color-text-secondary);
  margin-bottom: 16rpx;
  display: block;
  line-height: 1.4;
}

.achievement-progress {
  margin-bottom: 16rpx;
}

.progress-info {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8rpx;
}

.progress-text, .progress-percent {
  font-size: 20rpx;
  color: var(--color-text-disabled);
}

.progress-bar-small {
  height: 8rpx;
  background-color: var(--color-app-background);
  border-radius: 4rpx;
  overflow: hidden;
}

.progress-fill-small {
  height: 100%;
  background-color: var(--color-brand-primary);
  transition: width 0.3s ease;
}

.unlock-time {
  font-size: 20rpx;
  color: var(--color-success);
  margin-bottom: 12rpx;
  display: block;
}

.rarity-badge {
  display: inline-block;
  padding: 4rpx 12rpx;
  border-radius: 12rpx;
  font-size: 18rpx;
  font-weight: 500;
}

.rarity-badge.common {
  background-color: rgba(156, 163, 175, 0.2);
  color: var(--color-text-secondary);
}

.rarity-badge.rare {
  background-color: rgba(79, 127, 255, 0.2);
  color: #4F7FFF;
}

.rarity-badge.epic {
  background-color: rgba(147, 51, 234, 0.2);
  color: #9333EA;
}

.rarity-badge.legendary {
  background-color: rgba(245, 158, 11, 0.2);
  color: #F59E0B;
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

.achievement-modal {
  background-color: var(--color-card-background);
  border-radius: 24rpx;
  padding: 48rpx;
  margin: 32rpx;
  max-width: 600rpx;
  width: 100%;
  box-shadow: var(--shadow-modal);
  text-align: center;
  position: relative;
}

.modal-close {
  position: absolute;
  top: 24rpx;
  right: 24rpx;
  padding: 8rpx;
}

.modal-icon {
  position: relative;
  width: 160rpx;
  height: 160rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: 80rpx;
  margin: 0 auto 32rpx;
}

.modal-glow {
  position: absolute;
  inset: -12rpx;
  border-radius: 92rpx;
  background: conic-gradient(from 0deg, transparent, var(--color-brand-primary), transparent);
  animation: rotate 3s linear infinite;
  z-index: -1;
}

.modal-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
  margin-bottom: 16rpx;
  display: block;
}

.modal-desc {
  font-size: 28rpx;
  color: var(--color-text-secondary);
  margin-bottom: 32rpx;
  display: block;
  line-height: 1.5;
}

.modal-unlocked {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  margin-bottom: 32rpx;
}

.unlocked-text {
  font-size: 28rpx;
  color: var(--color-success);
  font-weight: 600;
}

.unlock-date {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.modal-locked {
  margin-bottom: 32rpx;
}

.modal-progress {
  margin-bottom: 24rpx;
}

.progress-label {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  margin-bottom: 16rpx;
  display: block;
}

.progress-bar-modal {
  height: 16rpx;
  background-color: var(--color-app-background);
  border-radius: 8rpx;
  overflow: hidden;
  margin-bottom: 12rpx;
}

.progress-fill-modal {
  height: 100%;
  background-color: var(--color-brand-primary);
  transition: width 0.3s ease;
}

.progress-detail {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.hint-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  font-style: italic;
}

.modal-rarity {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
}

.rarity-label {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

@keyframes rotate {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}
</style>