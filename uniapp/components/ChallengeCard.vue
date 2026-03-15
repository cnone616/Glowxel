<template>
  <view class="challenge-card" @click="handleClick" v-if="challenge && typeof challenge === 'object'">
    <!-- 挑战横幅图片 -->
    <view class="challenge-banner">
      <image 
        :src="challenge.banner" 
        class="banner-image"
        mode="aspectFill"
        @error="handleImageError"
      />
      <!-- 图片加载失败占位 -->
      <view v-if="imageError" class="image-placeholder">
        <Icon name="picture" :size="64" color="#FFFFFF" />
      </view>
      
      <!-- 渐变遮罩 -->
      <view class="banner-overlay"></view>
      
      <!-- 挑战图标 -->
      <view class="challenge-icon">
        <Icon :name="challenge.icon" :size="48" color="#FFFFFF" />
      </view>
      
      <!-- 状态标签 -->
      <view class="status-badge" :class="`status-${challenge.status}`">
        <text class="status-text">{{ statusText }}</text>
      </view>
    </view>
    
    <!-- 挑战信息 -->
    <view class="challenge-info">
      <!-- 挑战标题 -->
      <text class="challenge-title">{{ challenge.title }}</text>
      
      <!-- 挑战描述 -->
      <text class="challenge-description">{{ challenge.description }}</text>
      
      <!-- 挑战统计 -->
      <view class="challenge-stats">
        <view class="stat-item">
          <Icon name="user" :size="24" color="#4F7FFF" />
          <text class="stat-text">{{ formatNumber(challenge.participants) }}人参与</text>
        </view>
        
        <view class="stat-item">
          <Icon name="time" :size="24" color="#666666" />
          <text class="stat-text">{{ timeText }}</text>
        </view>
      </view>
      
      <!-- 奖励信息 -->
      <view v-if="challenge.reward" class="reward-info">
        <Icon name="work" :size="24" color="#FF7A45" />
        <text class="reward-text">奖励: {{ challenge.reward }}</text>
      </view>
    </view>
    
    <!-- 操作按钮 -->
    <view class="challenge-actions">
      <view 
        class="join-button" 
        :class="[`button-${challenge.status}`, { 'button-joined': isJoined }]"
        @click.stop="handleJoin"
      >
        <Icon 
          :name="buttonIcon" 
          :size="32" 
          color="#FFFFFF" 
        />
        <text class="button-text">{{ buttonText }}</text>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'

export default {
  name: 'ChallengeCard',
  components: {
    Icon
  },
  props: {
    // 挑战数据
    challenge: {
      type: Object,
      required: true
    },
    // 是否已参与
    joined: {
      type: Boolean,
      default: false
    }
  },
  
  data() {
    return {
      imageError: false,
      isJoined: this.joined
    }
  },
  
  computed: {
    statusText() {
      const textMap = {
        active: '进行中',
        upcoming: '即将开始',
        ended: '已结束'
      }
      return textMap[this.challenge.status] || '未知状态'
    },
    
    buttonIcon() {
      if (this.isJoined) return 'success'
      
      const iconMap = {
        active: 'add',
        upcoming: 'notification',
        ended: 'browse'
      }
      return iconMap[this.challenge.status] || 'add'
    },
    
    buttonText() {
      if (this.isJoined) return '已参与'
      
      const textMap = {
        active: '立即参与',
        upcoming: '设置提醒',
        ended: '查看结果'
      }
      return textMap[this.challenge.status] || '参与挑战'
    },
    
    timeText() {
      if (this.challenge.status === 'active') {
        return `还剩 ${this.challenge.remainingDays} 天`
      } else if (this.challenge.status === 'upcoming') {
        return `${this.challenge.startDate} 开始`
      } else {
        return `${this.challenge.endDate} 结束`
      }
    }
  },
  
  methods: {
    handleClick() {
      this.$emit('click', this.challenge)
    },
    
    handleJoin() {
      if (this.challenge.status === 'ended') {
        this.$emit('viewResults', this.challenge)
        return
      }
      
      if (this.challenge.status === 'upcoming') {
        this.$emit('setReminder', this.challenge)
        return
      }
      
      if (!this.isJoined) {
        this.isJoined = true
        this.$emit('join', this.challenge)
      } else {
        this.$emit('viewProgress', this.challenge)
      }
    },
    
    handleImageError() {
      this.imageError = true
    },
    
    formatNumber(num) {
      if (num >= 10000) {
        return (num / 10000).toFixed(1) + '万'
      } else if (num >= 1000) {
        return (num / 1000).toFixed(1) + 'k'
      }
      return num.toString()
    }
  }
}
</script>

<style scoped>
.challenge-card {
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  box-shadow: var(--shadow-card);
  overflow: hidden;
  transition: all 0.2s ease;
}

.challenge-card:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.challenge-banner {
  position: relative;
  width: 100%;
  height: 280rpx;
  background: linear-gradient(135deg, #4F7FFF, #FF7A45);
}

.banner-image {
  width: 100%;
  height: 100%;
}

.image-placeholder {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #4F7FFF, #FF7A45);
}

.banner-overlay {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: linear-gradient(
    to bottom,
    rgba(0, 0, 0, 0.1) 0%,
    rgba(0, 0, 0, 0.3) 100%
  );
}

.challenge-icon {
  position: absolute;
  top: 24rpx;
  left: 24rpx;
  width: 80rpx;
  height: 80rpx;
  background-color: rgba(255, 255, 255, 0.2);
  border-radius: var(--radius-medium);
  display: flex;
  align-items: center;
  justify-content: center;
  backdrop-filter: blur(10rpx);
}

.status-badge {
  position: absolute;
  top: 24rpx;
  right: 24rpx;
  padding: 8rpx 16rpx;
  border-radius: var(--radius-small);
  backdrop-filter: blur(10rpx);
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
  font-size: 22rpx;
  color: #FFFFFF;
  font-weight: 500;
}

.challenge-info {
  padding: 24rpx;
}

.challenge-title {
  font-size: 32rpx;
  font-weight: 700;
  color: var(--color-text-primary);
  line-height: 1.4;
  margin-bottom: 12rpx;
  display: -webkit-box;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 2;
  overflow: hidden;
}

.challenge-description {
  font-size: 26rpx;
  color: var(--color-text-secondary);
  line-height: 1.5;
  margin-bottom: 20rpx;
  display: -webkit-box;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 2;
  overflow: hidden;
}

.challenge-stats {
  display: flex;
  align-items: center;
  gap: 32rpx;
  margin-bottom: 16rpx;
}

.stat-item {
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.stat-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  font-weight: 500;
}

.reward-info {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 12rpx 16rpx;
  background-color: rgba(255, 122, 69, 0.1);
  border-radius: var(--radius-small);
  margin-bottom: 20rpx;
}

.reward-text {
  font-size: 24rpx;
  color: var(--color-brand-accent);
  font-weight: 500;
}

.challenge-actions {
  padding: 0 24rpx 24rpx;
}

.join-button {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  padding: 16rpx 24rpx;
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
}

.button-active {
  background-color: var(--color-brand-primary);
}

.button-upcoming {
  background-color: var(--color-warning);
}

.button-ended {
  background-color: var(--color-text-secondary);
}

.button-joined {
  background-color: var(--color-success);
}

.join-button:active {
  transform: scale(0.95);
}

.button-text {
  font-size: 28rpx;
  color: #FFFFFF;
  font-weight: 600;
}

/* 参与动画 */
.button-joined {
  animation: joinSuccess 0.5s ease;
}

@keyframes joinSuccess {
  0% { transform: scale(1); }
  50% { transform: scale(1.05); }
  100% { transform: scale(1); }
}
</style>