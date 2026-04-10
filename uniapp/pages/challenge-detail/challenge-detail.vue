<template>
  <view class="challenge-detail-page glx-page-shell">
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
        <view class="header-title">
          <text class="glx-topbar__title">挑战详情</text>
        </view>
        <view class="header-actions">
          <view class="action-btn" @click="handleShare">
            <Icon name="share" :size="32" color="var(--nb-ink)" />
          </view>
        </view>
      </view>
    </view>
    
    <!-- 挑战横幅 -->
    <view class="challenge-banner glx-page-shell__fixed">
      <image 
        :src="challenge.banner_url" 
        class="banner-image"
        mode="aspectFill"
      />
      <view class="banner-overlay"></view>
      
      <!-- 挑战图标 -->
      <view class="challenge-icon">
        <Icon name="work" :size="64" color="#FFFFFF" />
      </view>
      
      <!-- 状态标签 -->
      <view class="status-badge" :class="`status-${challenge.status}`">
        <text class="status-text">{{ statusText }}</text>
      </view>
    </view>
    
    <!-- 主要内容 -->
    <scroll-view scroll-y class="main-content glx-scroll-region glx-page-shell__content">
      <!-- 挑战信息卡片 -->
      <view class="info-card glx-panel-card">
        <text class="challenge-title">{{ challenge.title }}</text>
        <text class="challenge-description">{{ challenge.description }}</text>
        
        <!-- 挑战统计 -->
        <view class="stats-row">
          <view class="stat-item">
            <Icon name="user" :size="32" color="#4F7FFF" />
            <text class="stat-value">{{ challenge.participants }}</text>
            <text class="stat-label">参与者</text>
          </view>
          <view class="stat-item">
            <Icon name="picture" :size="32" color="#FF7A45" />
            <text class="stat-value">{{ challenge.submissions }}</text>
            <text class="stat-label">作品数</text>
          </view>
          <view class="stat-item">
            <Icon name="work" :size="32" color="#FFD700" />
            <text class="stat-value">{{ challenge.difficulty }}</text>
            <text class="stat-label">难度</text>
          </view>
        </view>
        
        <!-- 时间信息 -->
        <view class="time-info">
          <view class="time-item">
            <Icon name="time" :size="28" color="#666666" />
            <text class="time-text">开始: {{ formatDate(challenge.start_date) }}</text>
          </view>
          <view class="time-item">
            <Icon name="time" :size="28" color="#666666" />
            <text class="time-text">结束: {{ formatDate(challenge.end_date) }}</text>
          </view>
        </view>
        
        <!-- 奖励信息 -->
        <view class="prize-section">
          <view class="section-header glx-section-head">
            <Icon name="work" :size="32" color="#FF7A45" />
            <text class="section-title glx-section-title">挑战奖励</text>
          </view>
          <text class="prize-text">{{ challenge.prize }}</text>
        </view>
      </view>
      
      <!-- 挑战规则 -->
      <view class="rules-card glx-panel-card">
        <view class="section-header glx-section-head">
          <Icon name="list" :size="32" color="#4F7FFF" />
          <text class="section-title glx-section-title">挑战规则</text>
        </view>
        <view class="rules-list">
          <view 
            v-for="(rule, index) in challenge.rules" 
            :key="index"
            class="rule-item"
          >
            <text class="rule-number">{{ index + 1 }}</text>
            <text class="rule-text">{{ rule }}</text>
          </view>
        </view>
      </view>
      
      <!-- 评审标准 -->
      <view class="criteria-card glx-panel-card">
        <view class="section-header glx-section-head">
          <Icon name="star" :size="32" color="#FFD700" />
          <text class="section-title glx-section-title">评审标准</text>
        </view>
        <view class="criteria-list">
          <view 
            v-for="(criterion, index) in challenge.judges_criteria" 
            :key="index"
            class="criterion-item"
          >
            <text class="criterion-text">{{ criterion }}</text>
          </view>
        </view>
      </view>
      
      <!-- 标签 -->
      <view class="tags-section">
        <view 
          v-for="tag in challenge.tags" 
          :key="tag"
          class="tag-item"
        >
          <text class="tag-text">#{{ tag }}</text>
        </view>
      </view>
      
      <!-- 底部占位 -->
      <view class="bottom-spacer"></view>
    </scroll-view>
    
    <!-- 底部操作栏 -->
    <view class="bottom-bar glx-page-shell__fixed">
      <view 
        class="join-button glx-primary-action"
        :class="[`button-${challenge.status}`, { 'button-joined': isJoined }]"
        @click="handleJoin"
      >
        <Icon 
          :name="buttonIcon" 
          :size="36" 
          color="#FFFFFF" 
        />
        <text class="button-text">{{ buttonText }}</text>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from '@/components/Icon.vue'
import { challengeAPI } from '@/api/index.js'

export default {
  name: 'ChallengeDetail',
  components: {
    Icon
  },
  
  data() {
    return {
      statusBarHeight: 0,
      challengeId: null,
      challenge: {
        rules: [],
        judges_criteria: [],
        tags: []
      },
      isJoined: false
    }
  },
  
  computed: {
    statusText() {
      const textMap = {
        active: '进行中',
        upcoming: '即将开始',
        ended: '已结束'
      }
      return textMap[this.challenge?.status] || '未知状态'
    },
    
    buttonIcon() {
      if (this.isJoined) return 'success'
      
      const iconMap = {
        active: 'add',
        upcoming: 'notification',
        ended: 'browse'
      }
      return iconMap[this.challenge?.status] || 'add'
    },
    
    buttonText() {
      if (this.isJoined) return '已参与'
      
      const textMap = {
        active: '立即参与',
        upcoming: '设置提醒',
        ended: '查看结果'
      }
      return textMap[this.challenge?.status] || '参与挑战'
    }
  },
  
  onLoad(options) {
    // #ifdef MP-WEIXIN
    const systemInfo = uni.getSystemInfoSync()
    this.statusBarHeight = systemInfo.statusBarHeight || 0
    // #endif
    
    if (options.id) {
      this.challengeId = parseInt(options.id)
      this.loadChallengeData()
    }
  },
  
  methods: {
    async loadChallengeData() {
      const res = await challengeAPI.getChallengeById(this.challengeId)
      if (res.success && res.data && res.data.challenge) {
        this.challenge = res.data.challenge
        this.isJoined = this.challenge.joined
        return
      }

      uni.showToast({
        title: '挑战不存在',
        icon: 'none'
      })
      setTimeout(() => {
        uni.navigateBack()
      }, 1500)
    },
    
    handleBack() {
      uni.navigateBack()
    },
    
    handleShare() {
      uni.showShareMenu({
        withShareTicket: true
      })
    },
    
    async handleJoin() {
      if (this.challenge.status === 'ended') {
        uni.showToast({
          title: '挑战已结束',
          icon: 'none'
        })
        return
      }
      
      if (this.challenge.status === 'upcoming') {
        uni.showToast({
          title: '已设置提醒',
          icon: 'success'
        })
        return
      }
      
      if (!this.isJoined) {
        try {
          const res = await challengeAPI.joinChallenge(this.challengeId)
          if (!(res.success && res.data)) {
            throw new Error('join failed')
          }

          this.isJoined = res.data.joined
          if (res.data.changed) {
            this.challenge.participants += 1
          }

          uni.showToast({
            title: '参与成功！',
            icon: 'success'
          })

          setTimeout(() => {
            uni.navigateTo({
              url: `/pages/create/create?challengeId=${this.challengeId}`
            })
          }, 1500)
        } catch (error) {
          uni.showToast({
            title: '参与失败',
            icon: 'none'
          })
        }
        return
      }

      uni.navigateTo({
        url: `/pages/create/create?challengeId=${this.challengeId}`
      })
    },

    formatDate(dateString) {
      if (!dateString) {
        return ''
      }
      const date = new Date(dateString)
      const year = date.getFullYear()
      const month = `${date.getMonth() + 1}`.padStart(2, '0')
      const day = `${date.getDate()}`.padStart(2, '0')
      return `${year}-${month}-${day}`
    }
  }
}
</script>

<style scoped>
.challenge-detail-page {
  width: 100%;
  background-color: var(--nb-paper);
}

.status-bar {
  width: 100%;
  background-color: transparent;
}

.header {
  background: var(--nb-paper);
}

.back-btn {
  background-color: var(--nb-surface);
}

.header-title {
  flex: 1;
}

.header-actions {
  display: flex;
  gap: 16rpx;
}

.action-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-surface);
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  box-sizing: border-box;
}

.challenge-banner {
  position: relative;
  width: 100%;
  height: 500rpx;
  background: linear-gradient(135deg, #4F7FFF, #FF7A45);
}

.banner-image {
  width: 100%;
  height: 100%;
}

.banner-overlay {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: linear-gradient(
    to bottom,
    rgba(0, 0, 0, 0.2) 0%,
    rgba(0, 0, 0, 0.4) 100%
  );
}

.challenge-icon {
  position: absolute;
  bottom: 32rpx;
  left: 32rpx;
  width: 96rpx;
  height: 96rpx;
  background-color: rgba(255, 255, 255, 0.2);
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  backdrop-filter: none;
}

.status-badge {
  position: absolute;
  top: 32rpx;
  right: 32rpx;
  padding: 12rpx 24rpx;
  border-radius: 0;
  backdrop-filter: none;
  display: flex;
  align-items: center;
  justify-content: center;
}

.status-active {
  background-color: rgba(46, 204, 113, 0.9);
}

.status-upcoming {
  background-color: rgba(241, 156, 18, 0.9);
}

.status-ended {
  background-color: rgba(170, 170, 170, 0.9);
}

.status-text {
  font-size: 24rpx;
  color: #FFFFFF;
  font-weight: 600;
  line-height: 1.2;
}

.main-content {
  flex: 1;
  min-height: 0;
  padding: 32rpx;
}

.info-card,
.rules-card,
.criteria-card {
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 32rpx;
  margin-bottom: 24rpx;
  box-shadow: var(--nb-shadow-strong);
}

.challenge-title {
  font-size: 40rpx;
  font-weight: 700;
  color: var(--nb-ink);
  line-height: 1.4;
  margin-bottom: 16rpx;
  display: block;
}

.challenge-description {
  font-size: 28rpx;
  color: #4a4a4a;
  line-height: 1.6;
  margin-bottom: 32rpx;
  display: block;
}

.stats-row {
  display: flex;
  justify-content: space-around;
  padding: 24rpx 0;
  border-top: 1rpx solid var(--nb-ink);
  border-bottom: 1rpx solid var(--nb-ink);
  margin-bottom: 24rpx;
}

.stat-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
}

.stat-value {
  font-size: 32rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.stat-label {
  font-size: 24rpx;
  color: #4a4a4a;
}

.time-info {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
  margin-bottom: 24rpx;
}

.time-item {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.time-text {
  font-size: 26rpx;
  color: #4a4a4a;
}

.prize-section {
  padding: 24rpx;
  background: linear-gradient(135deg, rgba(79, 127, 255, 0.1), rgba(255, 122, 69, 0.1));
  border-radius: 0;
}

.section-header {
  display: flex;
  align-items: center;
  gap: 12rpx;
  margin-bottom: 16rpx;
}

.section-title {
  font-size: 30rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.prize-text {
  font-size: 28rpx;
  color: var(--nb-blue);
  font-weight: 600;
  display: block;
}

.rules-list,
.criteria-list {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.rule-item {
  display: flex;
  gap: 16rpx;
  align-items: flex-start;
}

.rule-number {
  flex-shrink: 0;
  width: 40rpx;
  height: 40rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-yellow);
  color: #FFFFFF;
  border-radius: 50%;
  font-size: 22rpx;
  font-weight: 600;
}

.rule-text {
  flex: 1;
  font-size: 26rpx;
  color: #4a4a4a;
  line-height: 1.6;
}

.criterion-item {
  padding: 16rpx 20rpx;
  background-color: var(--nb-paper);
  border-radius: 0;
  border-left: 4rpx solid var(--nb-yellow);
}

.criterion-text {
  font-size: 26rpx;
  color: #4a4a4a;
  line-height: 1.5;
}

.tags-section {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
  padding: 0 32rpx 32rpx;
}

.tag-item {
  padding: 12rpx 24rpx;
  background-color: var(--nb-surface);
  border-radius: 0;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.tag-text {
  font-size: 24rpx;
  color: var(--nb-ink);
  font-weight: 900;
}

.bottom-spacer {
  height: 40rpx;
}

.bottom-bar {
  padding: 24rpx 32rpx;
  background-color: var(--nb-surface);
  box-shadow: 0 -4rpx 16rpx rgba(0, 0, 0, 0.1);
  border-top: var(--nb-border-width-control) solid var(--nb-ink);
}

.join-button {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  padding: 24rpx;
  border-radius: 0;
  transition: all 0.2s ease;
}

.button-active {
  background: linear-gradient(135deg, #4F7FFF, #6B9FFF);
}

.button-upcoming {
  background: linear-gradient(135deg, #F19C12, #F39C12);
}

.button-ended {
  background-color: #4a4a4a;
}

.button-joined {
  background: linear-gradient(135deg, #2ECC71, #27AE60);
}

.join-button:active {
  transform: scale(0.98);
}

.button-text {
  font-size: 32rpx;
  color: #FFFFFF;
  font-weight: 700;
}
</style>
