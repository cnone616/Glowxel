<template>
  <view class="cloud-sync-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="goBack">
        <Icon name="direction-left" :size="32" color="var(--color-text-primary)" />
      </view>
      <text class="nav-title">云同步</text>
      <view class="nav-right"></view>
    </view>
    
    <scroll-view scroll-y class="content">
      <!-- 同步状态卡片 -->
      <view class="sync-status-card">
        <view class="status-header">
          <view class="status-icon" :class="{ active: syncStatus.enabled }">
            <Icon 
              :name="syncStatus.enabled ? 'cloud-check' : 'cloud-off'" 
              :size="80" 
              :color="syncStatus.enabled ? '#FFFFFF' : 'var(--color-text-disabled)'"
            />
          </view>
          <view class="status-info">
            <text class="status-title">{{ syncStatus.enabled ? '云同步已开启' : '云同步已关闭' }}</text>
            <text class="status-desc">{{ getStatusDescription() }}</text>
          </view>
        </view>
        
        <view class="sync-toggle">
          <switch 
            :checked="syncStatus.enabled" 
            @change="toggleSync"
            color="var(--color-brand-primary)"
          />
        </view>
      </view>
      
      <!-- 同步统计 -->
      <view v-if="syncStatus.enabled" class="sync-stats">
        <view class="stats-header">
          <text class="stats-title">同步统计</text>
          <text class="last-sync">最后同步: {{ formatLastSync() }}</text>
        </view>
        
        <view class="stats-grid">
          <view class="stat-item">
            <Icon name="picture" :size="40" color="var(--color-brand-primary)" />
            <text class="stat-number">{{ syncStats.projects }}</text>
            <text class="stat-label">作品</text>
          </view>
          
          <view class="stat-item">
            <Icon name="bookmark" :size="40" color="var(--color-brand-primary)" />
            <text class="stat-number">{{ syncStats.favorites }}</text>
            <text class="stat-label">收藏</text>
          </view>
          
          <view class="stat-item">
            <Icon name="settings" :size="40" color="var(--color-brand-primary)" />
            <text class="stat-number">{{ syncStats.settings }}</text>
            <text class="stat-label">设置</text>
          </view>
          
          <view class="stat-item">
            <Icon name="database" :size="40" color="var(--color-brand-primary)" />
            <text class="stat-number">{{ formatDataSize(syncStats.dataSize) }}</text>
            <text class="stat-label">数据量</text>
          </view>
        </view>
      </view>
      
      <!-- 同步设置 -->
      <view class="sync-settings">
        <view class="section-title">同步设置</view>
        
        <view class="setting-item">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="refresh" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">自动同步</text>
              <text class="setting-desc">应用启动时自动同步数据</text>
            </view>
          </view>
          <view class="setting-right">
            <switch 
              :checked="settings.autoSync" 
              @change="toggleAutoSync"
              color="var(--color-brand-primary)"
              :disabled="!syncStatus.enabled"
            />
          </view>
        </view>
        
        <view class="setting-item">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="wifi" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">仅WiFi同步</text>
              <text class="setting-desc">只在WiFi环境下进行同步</text>
            </view>
          </view>
          <view class="setting-right">
            <switch 
              :checked="settings.wifiOnly" 
              @change="toggleWifiOnly"
              color="var(--color-brand-primary)"
              :disabled="!syncStatus.enabled"
            />
          </view>
        </view>
        
        <view class="setting-item">
          <view class="setting-left">
            <view class="setting-icon">
              <Icon name="image" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="setting-info">
              <text class="setting-label">同步缩略图</text>
              <text class="setting-desc">同步作品缩略图（占用更多流量）</text>
            </view>
          </view>
          <view class="setting-right">
            <switch 
              :checked="settings.syncThumbnails" 
              @change="toggleSyncThumbnails"
              color="var(--color-brand-primary)"
              :disabled="!syncStatus.enabled"
            />
          </view>
        </view>
      </view>
      
      <!-- 同步操作 -->
      <view v-if="syncStatus.enabled" class="sync-actions">
        <view class="section-title">同步操作</view>
        
        <view class="action-item" @click="manualSync">
          <view class="action-left">
            <view class="action-icon">
              <Icon name="refresh" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="action-info">
              <text class="action-label">立即同步</text>
              <text class="action-desc">手动触发数据同步</text>
            </view>
          </view>
          <view class="action-right">
            <Icon name="arrow-right" :size="32" color="var(--color-text-disabled)" />
          </view>
        </view>
        
        <view class="action-item" @click="downloadFromCloud">
          <view class="action-left">
            <view class="action-icon">
              <Icon name="download" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="action-info">
              <text class="action-label">从云端恢复</text>
              <text class="action-desc">下载云端数据到本地</text>
            </view>
          </view>
          <view class="action-right">
            <Icon name="arrow-right" :size="32" color="var(--color-text-disabled)" />
          </view>
        </view>
        
        <view class="action-item" @click="uploadToCloud">
          <view class="action-left">
            <view class="action-icon">
              <Icon name="upload" :size="40" color="var(--color-brand-primary)" />
            </view>
            <view class="action-info">
              <text class="action-label">上传到云端</text>
              <text class="action-desc">将本地数据备份到云端</text>
            </view>
          </view>
          <view class="action-right">
            <Icon name="arrow-right" :size="32" color="var(--color-text-disabled)" />
          </view>
        </view>
        
        <view class="action-item danger" @click="clearCloudData">
          <view class="action-left">
            <view class="action-icon">
              <Icon name="trash" :size="40" color="var(--color-error)" />
            </view>
            <view class="action-info">
              <text class="action-label">清除云端数据</text>
              <text class="action-desc">删除所有云端备份数据</text>
            </view>
          </view>
          <view class="action-right">
            <Icon name="arrow-right" :size="32" color="var(--color-text-disabled)" />
          </view>
        </view>
      </view>
      
      <!-- 同步日志 -->
      <view v-if="syncStatus.enabled && syncLogs.length > 0" class="sync-logs">
        <view class="section-title">同步记录</view>
        
        <view class="logs-list">
          <view 
            v-for="log in syncLogs.slice(0, 5)"
            :key="log.id"
            class="log-item"
            :class="log.status"
          >
            <view class="log-icon">
              <Icon 
                :name="getLogIcon(log.status)" 
                :size="32" 
                :color="getLogColor(log.status)"
              />
            </view>
            <view class="log-info">
              <text class="log-action">{{ log.action }}</text>
              <text class="log-time">{{ formatLogTime(log.timestamp) }}</text>
            </view>
            <view v-if="log.status === 'error'" class="log-error">
              <text class="error-text">{{ log.error }}</text>
            </view>
          </view>
        </view>
        
        <view class="view-all-logs" @click="viewAllLogs">
          <text class="view-all-text">查看全部记录</text>
          <Icon name="arrow-right" :size="24" />
        </view>
      </view>
      
      <!-- 帮助说明 -->
      <view class="help-section">
        <view class="section-title">关于云同步</view>
        
        <view class="help-content">
          <text class="help-text">• 云同步可以在多个设备间同步您的作品和设置</text>
          <text class="help-text">• 数据采用端到端加密，确保隐私安全</text>
          <text class="help-text">• 建议在WiFi环境下进行同步以节省流量</text>
          <text class="help-text">• 同步失败时会自动重试，无需担心数据丢失</text>
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
      syncStatus: {
        enabled: false,
        lastSync: Date.now() - 3600000,
        syncing: false
      },
      syncStats: {
        projects: 12,
        favorites: 8,
        settings: 15,
        dataSize: 2.5 * 1024 * 1024 // 2.5MB
      },
      settings: {
        autoSync: true,
        wifiOnly: true,
        syncThumbnails: false
      },
      syncLogs: [
        {
          id: '1',
          action: '自动同步完成',
          status: 'success',
          timestamp: Date.now() - 3600000
        },
        {
          id: '2',
          action: '上传作品数据',
          status: 'success',
          timestamp: Date.now() - 7200000
        },
        {
          id: '3',
          action: '同步失败',
          status: 'error',
          error: '网络连接超时',
          timestamp: Date.now() - 10800000
        },
        {
          id: '4',
          action: '下载云端数据',
          status: 'success',
          timestamp: Date.now() - 14400000
        }
      ]
    }
  },
  
  onLoad() {
    this.toast = useToast()
    this.loadSyncSettings()
    
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
    
    loadSyncSettings() {
      // 从用户store加载同步状态
      this.syncStatus.enabled = this.userStore.syncEnabled || false
      
      // 从本地存储加载设置
      const savedSettings = uni.getStorageSync('sync-settings')
      if (savedSettings) {
        this.settings = { ...this.settings, ...savedSettings }
      }
    },
    
    saveSyncSettings() {
      uni.setStorageSync('sync-settings', this.settings)
    },
    
    getStatusDescription() {
      if (!this.syncStatus.enabled) {
        return '开启后可在多设备间同步数据'
      }
      
      if (this.syncStatus.syncing) {
        return '正在同步数据...'
      }
      
      return '数据已安全备份到云端'
    },
    
    async toggleSync(e) {
      const enabled = e.detail.value
      
      if (enabled) {
        // 开启云同步
        uni.showLoading({ title: '开启中...' })
        
        try {
          await this.userStore.enableSync()
          this.syncStatus.enabled = true
          this.toast.showSuccess('云同步已开启')
        } catch (error) {
          this.toast.showError('开启失败，请重试')
        } finally {
          uni.hideLoading()
        }
      } else {
        // 关闭云同步
        uni.showModal({
          title: '关闭云同步',
          content: '关闭后将无法在多设备间同步数据，确定要关闭吗？',
          success: async (res) => {
            if (res.confirm) {
              try {
                await this.userStore.disableSync()
                this.syncStatus.enabled = false
                this.toast.showInfo('云同步已关闭')
              } catch (error) {
                this.toast.showError('操作失败，请重试')
              }
            }
          }
        })
      }
    },
    
    toggleAutoSync(e) {
      this.settings.autoSync = e.detail.value
      this.saveSyncSettings()
      this.toast.showInfo(this.settings.autoSync ? '自动同步已开启' : '自动同步已关闭')
    },
    
    toggleWifiOnly(e) {
      this.settings.wifiOnly = e.detail.value
      this.saveSyncSettings()
      this.toast.showInfo(this.settings.wifiOnly ? '仅WiFi同步已开启' : '移动网络同步已允许')
    },
    
    toggleSyncThumbnails(e) {
      this.settings.syncThumbnails = e.detail.value
      this.saveSyncSettings()
      this.toast.showInfo(this.settings.syncThumbnails ? '缩略图同步已开启' : '缩略图同步已关闭')
    },
    
    async manualSync() {
      if (this.syncStatus.syncing) return
      
      this.syncStatus.syncing = true
      uni.showLoading({ title: '同步中...' })
      
      try {
        // 模拟同步过程
        await new Promise(resolve => setTimeout(resolve, 2000))
        
        this.syncStatus.lastSync = Date.now()
        this.addSyncLog('手动同步完成', 'success')
        this.toast.showSuccess('同步完成')
      } catch (error) {
        this.addSyncLog('同步失败', 'error', error.message)
        this.toast.showError('同步失败，请重试')
      } finally {
        this.syncStatus.syncing = false
        uni.hideLoading()
      }
    },
    
    async downloadFromCloud() {
      uni.showModal({
        title: '从云端恢复',
        content: '这将用云端数据覆盖本地数据，确定要继续吗？',
        success: async (res) => {
          if (res.confirm) {
            uni.showLoading({ title: '恢复中...' })
            
            try {
              // 模拟下载过程
              await new Promise(resolve => setTimeout(resolve, 3000))
              
              this.addSyncLog('从云端恢复数据', 'success')
              this.toast.showSuccess('恢复完成')
            } catch (error) {
              this.addSyncLog('恢复失败', 'error', error.message)
              this.toast.showError('恢复失败，请重试')
            } finally {
              uni.hideLoading()
            }
          }
        }
      })
    },
    
    async uploadToCloud() {
      uni.showLoading({ title: '上传中...' })
      
      try {
        // 模拟上传过程
        await new Promise(resolve => setTimeout(resolve, 2500))
        
        this.addSyncLog('上传到云端', 'success')
        this.toast.showSuccess('上传完成')
      } catch (error) {
        this.addSyncLog('上传失败', 'error', error.message)
        this.toast.showError('上传失败，请重试')
      } finally {
        uni.hideLoading()
      }
    },
    
    clearCloudData() {
      uni.showModal({
        title: '清除云端数据',
        content: '这将永久删除所有云端备份数据，此操作无法撤销！',
        confirmColor: '#E74C3C',
        success: async (res) => {
          if (res.confirm) {
            uni.showLoading({ title: '清除中...' })
            
            try {
              // 模拟清除过程
              await new Promise(resolve => setTimeout(resolve, 1500))
              
              this.addSyncLog('清除云端数据', 'success')
              this.toast.showSuccess('云端数据已清除')
            } catch (error) {
              this.addSyncLog('清除失败', 'error', error.message)
              this.toast.showError('清除失败，请重试')
            } finally {
              uni.hideLoading()
            }
          }
        }
      })
    },
    
    viewAllLogs() {
      this.toast.showInfo('同步日志详情功能开发中')
    },
    
    addSyncLog(action, status, error = null) {
      const log = {
        id: Date.now().toString(),
        action,
        status,
        error,
        timestamp: Date.now()
      }
      
      this.syncLogs.unshift(log)
      
      // 只保留最近20条记录
      if (this.syncLogs.length > 20) {
        this.syncLogs = this.syncLogs.slice(0, 20)
      }
    },
    
    formatLastSync() {
      const date = new Date(this.syncStatus.lastSync)
      const now = new Date()
      const diff = now - date
      
      if (diff < 60000) return '刚刚'
      if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
      if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
      
      return date.toLocaleDateString() + ' ' + date.toLocaleTimeString()
    },
    
    formatDataSize(bytes) {
      if (bytes < 1024) return bytes + 'B'
      if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + 'KB'
      return (bytes / (1024 * 1024)).toFixed(1) + 'MB'
    },
    
    formatLogTime(timestamp) {
      const date = new Date(timestamp)
      const now = new Date()
      const diff = now - date
      
      if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
      if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
      
      return date.toLocaleDateString()
    },
    
    getLogIcon(status) {
      switch (status) {
        case 'success': return 'check-circle'
        case 'error': return 'x-circle'
        case 'warning': return 'alert-circle'
        default: return 'info'
      }
    },
    
    getLogColor(status) {
      switch (status) {
        case 'success': return 'var(--color-success)'
        case 'error': return 'var(--color-error)'
        case 'warning': return 'var(--color-warning)'
        default: return 'var(--color-text-secondary)'
      }
    }
  }
}
</script>

<style scoped>
.cloud-sync-page {
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

.content {
  flex: 1;
  padding: 32rpx 32rpx 80rpx;
}

.sync-status-card {
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 32rpx;
  margin-bottom: 32rpx;
  box-shadow: var(--shadow-card);
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.status-header {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
}

.status-icon {
  width: 120rpx;
  height: 120rpx;
  border-radius: 60rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  transition: all 0.3s ease;
}

.status-icon.active {
  background: linear-gradient(135deg, var(--color-brand-primary), var(--color-brand-accent));
  box-shadow: 0 8rpx 24rpx rgba(79, 127, 255, 0.3);
}

.status-info {
  flex: 1;
}

.status-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 8rpx;
  display: block;
}

.status-desc {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  line-height: 1.4;
}

.sync-stats {
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 32rpx;
  margin-bottom: 32rpx;
  box-shadow: var(--shadow-card);
}

.stats-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24rpx;
}

.stats-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.last-sync {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.stat-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 24rpx;
  background-color: var(--color-app-background);
  border-radius: 12rpx;
}

.stat-number {
  font-size: 28rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.stat-label {
  font-size: 22rpx;
  color: var(--color-text-secondary);
}

.sync-settings, .sync-actions, .sync-logs, .help-section {
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

.setting-item, .action-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 0;
  border-bottom: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.setting-item:last-child, .action-item:last-child {
  border-bottom: none;
}

.action-item:active {
  background-color: var(--color-app-background);
  margin: 0 -32rpx;
  padding: 24rpx 32rpx;
  border-radius: 12rpx;
}

.setting-left, .action-left {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
}

.setting-icon, .action-icon {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: rgba(79, 127, 255, 0.1);
  border-radius: 16rpx;
}

.setting-info, .action-info {
  flex: 1;
}

.setting-label, .action-label {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--color-text-primary);
  margin-bottom: 4rpx;
  display: block;
}

.setting-desc, .action-desc {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.action-item.danger .action-label {
  color: var(--color-error);
}

.logs-list {
  margin-bottom: 24rpx;
}

.log-item {
  display: flex;
  align-items: flex-start;
  gap: 16rpx;
  padding: 16rpx 0;
  border-bottom: 2rpx solid var(--border-primary);
}

.log-item:last-child {
  border-bottom: none;
}

.log-icon {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.log-info {
  flex: 1;
}

.log-action {
  font-size: 26rpx;
  color: var(--color-text-primary);
  margin-bottom: 4rpx;
  display: block;
}

.log-time {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.log-error {
  margin-top: 8rpx;
}

.error-text {
  font-size: 22rpx;
  color: var(--color-error);
}

.view-all-logs {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  padding: 16rpx;
  background-color: var(--color-app-background);
  border-radius: 12rpx;
  transition: all 0.2s ease;
}

.view-all-logs:active {
  background-color: var(--border-primary);
}

.view-all-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.help-content {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.help-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  line-height: 1.5;
}
</style>