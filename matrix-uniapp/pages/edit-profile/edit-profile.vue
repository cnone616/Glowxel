<template>
  <view class="edit-profile-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="goBack">
        <Icon name="arrow-left" :size="32" />
      </view>
      <text class="nav-title">编辑资料</text>
      <view class="nav-right" @click="handleSave">
        <text class="save-btn">保存</text>
      </view>
    </view>
    
    <scroll-view scroll-y class="content">
      <!-- 头像编辑 -->
      <view class="avatar-section">
        <view class="avatar-container" @click="chooseAvatar">
          <image 
            v-if="form.avatar" 
            :src="form.avatar" 
            class="avatar-image"
            mode="aspectFill"
          />
          <view v-else class="avatar-placeholder">
            <Icon name="user" :size="80" color="#AAAAAA" />
          </view>
          <view class="avatar-overlay">
            <Icon name="camera" :size="40" color="#FFFFFF" />
          </view>
        </view>
        <text class="avatar-tip">点击更换头像</text>
      </view>
      
      <!-- 基本信息 -->
      <view class="form-section">
        <view class="section-title">基本信息</view>
        
        <view class="form-item">
          <text class="form-label">昵称</text>
          <input 
            v-model="form.nickname"
            class="form-input"
            placeholder="请输入昵称"
            maxlength="20"
          />
          <text class="char-count">{{ form.nickname.length }}/20</text>
        </view>
        
        <view class="form-item">
          <text class="form-label">个人简介</text>
          <textarea 
            v-model="form.bio"
            class="form-textarea"
            placeholder="介绍一下自己吧..."
            maxlength="100"
            :auto-height="true"
          />
          <text class="char-count">{{ form.bio.length }}/100</text>
        </view>
        
        <view class="form-item">
          <text class="form-label">性别</text>
          <view class="gender-options">
            <view 
              v-for="option in genderOptions"
              :key="option.value"
              class="gender-option"
              :class="{ active: form.gender === option.value }"
              @click="form.gender = option.value"
            >
              <Icon :name="option.icon" :size="32" />
              <text class="gender-text">{{ option.label }}</text>
            </view>
          </view>
        </view>
      </view>
      
      <!-- 兴趣标签 -->
      <view class="form-section">
        <view class="section-title">兴趣标签</view>
        <view class="tags-container">
          <view 
            v-for="tag in availableTags"
            :key="tag"
            class="tag-item"
            :class="{ active: form.tags.includes(tag) }"
            @click="toggleTag(tag)"
          >
            <text class="tag-text">{{ tag }}</text>
          </view>
        </view>
        <text class="tags-tip">最多选择5个标签</text>
      </view>
      
      <!-- 隐私设置 -->
      <view class="form-section">
        <view class="section-title">隐私设置</view>
        
        <view class="form-item switch-item">
          <view class="switch-info">
            <text class="form-label">公开作品</text>
            <text class="form-desc">允许其他用户查看我的作品</text>
          </view>
          <switch 
            :checked="form.publicWorks" 
            @change="form.publicWorks = $event.detail.value"
            color="#4F7FFF"
          />
        </view>
        
        <view class="form-item switch-item">
          <view class="switch-info">
            <text class="form-label">接收关注通知</text>
            <text class="form-desc">有新粉丝时通知我</text>
          </view>
          <switch 
            :checked="form.followNotify" 
            @change="form.followNotify = $event.detail.value"
            color="#4F7FFF"
          />
        </view>
      </view>
    </scroll-view>
    
    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useUserStore } from '../../store/user.js'
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
      userStore: useUserStore(),
      toast: null,
      form: {
        avatar: '',
        nickname: '',
        bio: '',
        gender: 'unknown',
        tags: [],
        publicWorks: true,
        followNotify: true
      },
      genderOptions: [
        { value: 'male', label: '男', icon: 'user' },
        { value: 'female', label: '女', icon: 'user' },
        { value: 'unknown', label: '保密', icon: 'user' }
      ],
      availableTags: [
        '像素艺术', '游戏', '动漫', '设计', '创意',
        '手工', '拼豆', '艺术', '摄影', '音乐',
        '编程', '科技', '旅行', '美食', '运动'
      ]
    }
  },
  
  onLoad() {
    this.toast = useToast()
    this.loadUserData()
    
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
    })
  },
  
  methods: {
    loadUserData() {
      // 加载用户数据
      this.form = {
        avatar: this.userStore.avatar || '',
        nickname: this.userStore.nickname || '',
        bio: this.userStore.bio || '',
        gender: this.userStore.gender || 'unknown',
        tags: this.userStore.tags || [],
        publicWorks: this.userStore.publicWorks !== false,
        followNotify: this.userStore.followNotify !== false
      }
    },
    
    goBack() {
      uni.navigateBack()
    },
    
    chooseAvatar() {
      uni.chooseImage({
        count: 1,
        sizeType: ['compressed'],
        sourceType: ['album', 'camera'],
        success: (res) => {
          this.form.avatar = res.tempFilePaths[0]
        }
      })
    },
    
    toggleTag(tag) {
      const index = this.form.tags.indexOf(tag)
      if (index > -1) {
        this.form.tags.splice(index, 1)
      } else if (this.form.tags.length < 5) {
        this.form.tags.push(tag)
      } else {
        this.toast.showError('最多只能选择5个标签')
      }
    },
    
    async handleSave() {
      if (!this.form.nickname.trim()) {
        this.toast.showError('请输入昵称')
        return
      }
      
      uni.showLoading({ title: '保存中...' })
      
      try {
        // 更新用户信息
        await this.userStore.updateProfile(this.form)
        
        uni.hideLoading()
        this.toast.showSuccess('保存成功')
        
        setTimeout(() => {
          uni.navigateBack()
        }, 1000)
      } catch (error) {
        uni.hideLoading()
        this.toast.showError('保存失败，请重试')
      }
    }
  }
}
</script>

<style scoped>
.edit-profile-page {
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
  width: 120rpx;
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

.save-btn {
  font-size: 28rpx;
  color: var(--color-brand-primary);
  font-weight: 600;
}

.content {
  flex: 1;
  padding: 32rpx 32rpx 80rpx;
}

.avatar-section {
  display: flex;
  flex-direction: column;
  align-items: center;
  margin-bottom: 48rpx;
}

.avatar-container {
  position: relative;
  width: 160rpx;
  height: 160rpx;
  border-radius: 80rpx;
  overflow: hidden;
  margin-bottom: 16rpx;
}

.avatar-image {
  width: 100%;
  height: 100%;
}

.avatar-placeholder {
  width: 100%;
  height: 100%;
  background-color: var(--color-app-background);
  display: flex;
  align-items: center;
  justify-content: center;
}

.avatar-overlay {
  position: absolute;
  bottom: 0;
  right: 0;
  width: 48rpx;
  height: 48rpx;
  background-color: var(--color-brand-primary);
  border-radius: 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 4rpx solid var(--color-card-background);
}

.avatar-tip {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.form-section {
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 32rpx;
  margin-bottom: 32rpx;
  box-shadow: var(--shadow-card);
}

.section-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 24rpx;
}

.form-item {
  margin-bottom: 32rpx;
  position: relative;
}

.form-item:last-child {
  margin-bottom: 0;
}

.form-label {
  display: block;
  font-size: 28rpx;
  color: var(--color-text-primary);
  margin-bottom: 16rpx;
  font-weight: 500;
}

.form-input {
  width: 100%;
  padding: 24rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  font-size: 28rpx;
  color: var(--color-text-primary);
  transition: all 0.2s ease;
}

.form-input:focus {
  border-color: var(--color-brand-primary);
  box-shadow: 0 0 0 6rpx rgba(79, 127, 255, 0.1);
}

.form-textarea {
  width: 100%;
  min-height: 120rpx;
  padding: 24rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  font-size: 28rpx;
  color: var(--color-text-primary);
  resize: none;
  transition: all 0.2s ease;
}

.form-textarea:focus {
  border-color: var(--color-brand-primary);
  box-shadow: 0 0 0 6rpx rgba(79, 127, 255, 0.1);
}

.char-count {
  position: absolute;
  right: 16rpx;
  bottom: 16rpx;
  font-size: 20rpx;
  color: var(--color-text-disabled);
}

.gender-options {
  display: flex;
  gap: 16rpx;
}

.gender-option {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 24rpx 16rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  transition: all 0.2s ease;
}

.gender-option.active {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
  color: #FFFFFF;
}

.gender-text {
  font-size: 24rpx;
  color: inherit;
}

.tags-container {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
  margin-bottom: 16rpx;
}

.tag-item {
  padding: 16rpx 24rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  transition: all 0.2s ease;
}

.tag-item.active {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
}

.tag-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.tag-item.active .tag-text {
  color: #FFFFFF;
}

.tags-tip {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.switch-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.switch-info {
  flex: 1;
}

.form-desc {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  margin-top: 4rpx;
}
</style>