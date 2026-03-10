<template>
  <view class="device-card" @click="handleClick">
    <!-- 设备图标 -->
    <view class="device-icon-container">
      <Icon :name="device.icon" :size="48" :color="iconColor" />
      
      <!-- 状态指示器 -->
      <view class="status-indicator" :class="`status-${device.status}`">
        <view class="status-dot"></view>
      </view>
    </view>
    
    <!-- 设备信息 -->
    <view class="device-info">
      <!-- 设备名称 -->
      <text class="device-name">{{ device.name }}</text>
      
      <!-- 设备详情 -->
      <view class="device-details">
        <text class="device-type">{{ device.type }}</text>
        <text class="device-separator">•</text>
        <text class="device-connection">{{ connectionText }}</text>
      </view>
      
      <!-- 电池信息 -->
      <view v-if="device.batteryLevel !== undefined" class="battery-info">
        <Icon name="electronics" :size="24" :color="batteryColor" />
        <text class="battery-text" :style="{ color: batteryColor }">{{ device.batteryLevel }}%</text>
      </view>
    </view>
    
    <!-- 连接按钮 -->
    <view class="device-actions">
      <view 
        class="connect-button" 
        :class="[`button-${device.status}`, { 'button-loading': isConnecting }]"
        @click.stop="handleConnect"
      >
        <Icon 
          v-if="!isConnecting" 
          :name="buttonIcon" 
          :size="32" 
          color="#FFFFFF" 
        />
        <Icon 
          v-else 
          name="loading" 
          :size="32" 
          color="#FFFFFF" 
          class="loading-icon"
        />
        <text class="button-text">{{ buttonText }}</text>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'

export default {
  name: 'DeviceCard',
  components: {
    Icon
  },
  props: {
    // 设备数据
    device: {
      type: Object,
      required: true
    },
    // 连接状态
    connecting: {
      type: Boolean,
      default: false
    }
  },
  
  data() {
    return {
      isConnecting: this.connecting
    }
  },
  
  computed: {
    iconColor() {
      const colorMap = {
        online: '#4F7FFF',
        offline: '#AAAAAA',
        connecting: '#F39C12'
      }
      return colorMap[this.device.status] || '#AAAAAA'
    },
    
    connectionText() {
      const textMap = {
        online: '已连接',
        offline: '未连接',
        connecting: '连接中...'
      }
      return textMap[this.device.status] || '未知状态'
    },
    
    buttonIcon() {
      const iconMap = {
        online: 'close',
        offline: 'link',
        connecting: 'loading'
      }
      return iconMap[this.device.status] || 'link'
    },
    
    buttonText() {
      const textMap = {
        online: '断开',
        offline: '连接',
        connecting: '连接中'
      }
      return textMap[this.device.status] || '连接'
    },
    
    batteryColor() {
      if (this.device.batteryLevel > 50) return '#2ECC71'
      if (this.device.batteryLevel > 20) return '#F39C12'
      return '#E74C3C'
    }
  },
  
  watch: {
    connecting(newVal) {
      this.isConnecting = newVal
    }
  },
  
  methods: {
    handleClick() {
      this.$emit('click', this.device)
    },
    
    async handleConnect() {
      if (this.isConnecting) return
      
      this.isConnecting = true
      
      try {
        if (this.device.status === 'online') {
          await this.$emit('disconnect', this.device)
        } else {
          await this.$emit('connect', this.device)
        }
      } catch (error) {
        console.error('设备连接操作失败:', error)
      } finally {
        this.isConnecting = false
      }
    }
  }
}
</script>

<style scoped>
.device-card {
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  box-shadow: var(--shadow-card);
  padding: 24rpx;
  display: flex;
  align-items: center;
  gap: 20rpx;
  transition: all 0.2s ease;
}

.device-card:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.device-icon-container {
  position: relative;
  width: 96rpx;
  height: 96rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: var(--radius-medium);
}

.status-indicator {
  position: absolute;
  bottom: -4rpx;
  right: -4rpx;
  width: 24rpx;
  height: 24rpx;
  border-radius: 50%;
  border: 3rpx solid var(--color-card-background);
  display: flex;
  align-items: center;
  justify-content: center;
}

.status-dot {
  width: 12rpx;
  height: 12rpx;
  border-radius: 50%;
}

.status-online .status-dot {
  background-color: var(--color-success);
}

.status-offline .status-dot {
  background-color: var(--color-text-disabled);
}

.status-connecting .status-dot {
  background-color: var(--color-warning);
  animation: pulse 1.5s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

.device-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.device-name {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  line-height: 1.4;
}

.device-details {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.device-type {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.device-separator {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.device-connection {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.battery-info {
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.battery-text {
  font-size: 22rpx;
  font-weight: 500;
}

.device-actions {
  display: flex;
  align-items: center;
}

.connect-button {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 12rpx 20rpx;
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
  min-width: 120rpx;
  justify-content: center;
}

.button-online {
  background-color: var(--color-text-secondary);
}

.button-offline {
  background-color: var(--color-brand-primary);
}

.button-connecting {
  background-color: var(--color-warning);
}

.button-loading {
  opacity: 0.8;
  pointer-events: none;
}

.connect-button:active {
  transform: scale(0.95);
}

.button-text {
  font-size: 24rpx;
  color: #FFFFFF;
  font-weight: 500;
}

.loading-icon {
  animation: rotate 1s linear infinite;
}

@keyframes rotate {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}
</style>