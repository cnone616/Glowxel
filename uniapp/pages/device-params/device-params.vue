<template>
  <view class="device-params-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="goBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">设备参数</text>
      <view class="nav-right" @click="loadDeviceParams">
        <Icon name="refresh" :size="28" color="var(--nb-ink)" />
      </view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content">
      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">特殊LED</text>
          <text class="section-meta">旋转、色彩、反转相位</text>
        </view>
        <view class="panel-card glx-panel-card">
          <view class="param-card">
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">旋转</text>
                <text class="param-desc">画面侧着时在这里切换</text>
              </view>
              <picker
                class="param-picker"
                mode="selector"
                :range="rotationLabels"
                :value="rotationIndex"
                @change="handleRotationChange"
              >
                <view class="param-picker-text">{{ rotationLabels[rotationIndex] }}</view>
              </picker>
            </view>
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">色彩</text>
                <text class="param-desc">颜色不对时在这里切换</text>
              </view>
              <picker
                class="param-picker"
                mode="selector"
                :range="colorOrderLabels"
                :value="colorOrderIndex"
                @change="handleColorOrderChange"
              >
                <view class="param-picker-text">{{ colorOrderLabels[colorOrderIndex] }}</view>
              </picker>
            </view>
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">反转相位</text>
                <text class="param-desc">残影、分裂、左右错位时切换</text>
              </view>
              <GlxSwitch
                :checked="params.clkphase"
                @change="handleSwitchChange('clkphase', $event)"
              />
            </view>
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">日夜亮度</text>
          <text class="section-meta">只补基础定时亮度</text>
        </view>
        <view class="panel-card glx-panel-card">
          <view class="param-card">
            <view class="param-stack compact-gap">
              <text class="param-label">日间亮度</text>
              <text class="param-desc">默认作为白天显示亮度</text>
              <view class="slider-inline-head">
                <text class="slider-inline-value">{{ brightnessDay }}%</text>
              </view>
              <GlxSlider :value="brightnessDay" :min="0" :max="100" @change="handleBrightnessDayChange" />
            </view>
            <view class="param-stack">
              <text class="param-label">夜间亮度</text>
              <text class="param-desc">夜间时段会切换到这个亮度</text>
              <view class="slider-inline-head">
                <text class="slider-inline-value">{{ brightnessNight }}%</text>
              </view>
              <GlxSlider :value="brightnessNight" :min="0" :max="100" @change="handleBrightnessNightChange" />
            </view>
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">夜间开始时间</text>
                <text class="param-desc">夜间开始时间</text>
              </view>
              <picker class="param-picker param-picker-wide" mode="time" :value="params.nightStart" @change="handleNightStartChange">
                <view class="param-picker-text">{{ params.nightStart }}</view>
              </picker>
            </view>
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">夜间结束时间</text>
                <text class="param-desc">夜间结束时间</text>
              </view>
              <picker class="param-picker param-picker-wide" mode="time" :value="params.nightEnd" @change="handleNightEndChange">
                <view class="param-picker-text">{{ params.nightEnd }}</view>
              </picker>
            </view>
            <view class="param-tip-box">
              <text class="param-tip-text">只有当日间亮度和夜间亮度设置成不同数值时，设备才会按时间自动切换。</text>
            </view>
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">网络参数</text>
          <text class="section-meta">国内优先 NTP</text>
        </view>
        <view class="panel-card glx-panel-card">
          <view class="param-card">
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">时间服务器预设</text>
                <text class="param-desc">默认优先国内可用地址</text>
              </view>
              <picker
                class="param-picker param-picker-wide"
                mode="selector"
                :range="ntpPresetLabels"
                :value="ntpPresetIndex"
                @change="handleNtpPresetChange"
              >
                <view class="param-picker-text">{{ ntpPresetLabels[ntpPresetIndex] }}</view>
              </picker>
            </view>
            <view class="param-stack">
              <text class="param-label">自定义时间服务器</text>
              <text class="param-desc">如果默认地址慢或不可用，可以手动填写</text>
              <input
                class="text-input"
                type="text"
                :value="params.ntpServer"
                @input="handleNtpServerInput"
                maxlength="63"
                placeholder="例如 ntp2.aliyun.com"
              />
            </view>
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">设备信息</text>
          <text class="section-meta">只读</text>
        </view>
        <view class="panel-card glx-panel-card">
          <view class="param-card">
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">固件版本</text>
                <text class="param-desc">当前固件版本</text>
              </view>
              <text class="readonly-value value-box">{{ info.firmwareVersion.length > 0 ? info.firmwareVersion : '--' }}</text>
            </view>
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">运行时长</text>
                <text class="param-desc">设备已运行时长</text>
              </view>
              <text class="readonly-value value-box">{{ formattedUptime }}</text>
            </view>
            <view class="param-row">
              <view class="param-copy">
                <text class="param-label">当前 WiFi</text>
                <text class="param-desc">当前已连接 WiFi</text>
              </view>
              <text class="readonly-value value-box">{{ info.wifiSsid.length > 0 ? info.wifiSsid : '未连接' }}</text>
            </view>
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">应用操作</text>
          <text class="section-meta">逐项下发到设备</text>
        </view>
        <view class="panel-card glx-panel-card">
          <view class="panel-action" @click="saveDeviceParams">
            <view class="panel-action-icon blue">
              <Icon name="success" :size="32" color="var(--nb-ink)" />
            </view>
            <view class="panel-action-text">
              <text class="panel-action-label">保存并应用</text>
              <text class="panel-action-desc">把当前参数写入设备并立即生效</text>
            </view>
            <Icon name="direction-right" :size="28" color="var(--nb-ink)" />
          </view>
          <view class="panel-divider"></view>
          <view class="panel-action" @click="loadDeviceParams">
            <view class="panel-action-icon">
              <Icon name="refresh" :size="32" color="var(--nb-ink)" />
            </view>
            <view class="panel-action-text">
              <text class="panel-action-label">重新读取</text>
              <text class="panel-action-desc">从设备拉取当前实际参数</text>
            </view>
            <Icon name="direction-right" :size="28" color="var(--nb-ink)" />
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">高级</text>
          <text class="section-meta">危险操作请谨慎</text>
        </view>
        <view class="panel-card glx-panel-card">
          <view class="panel-action danger" @click="handleResetWifi">
            <view class="panel-action-icon danger">
              <Icon name="close" :size="32" color="var(--nb-ink)" />
            </view>
            <view class="panel-action-text">
              <text class="panel-action-label danger-text">重置网络</text>
              <text class="panel-action-desc">清除 WiFi 配置并自动重启设备</text>
            </view>
            <Icon name="direction-right" :size="28" color="var(--nb-ink)" />
          </view>
        </view>
      </view>

      <view style="height: 120rpx"></view>
    </scroll-view>

    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useDeviceStore } from '../../store/device.js'
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Toast from '../../components/Toast.vue'
import GlxSwitch from '../../components/GlxSwitch.vue'
import GlxSlider from '../../components/GlxSlider.vue'

const DRIVER_OPTIONS = [
  { label: 'SHIFTREG', value: 0 },
  { label: 'FM6124', value: 1 },
  { label: 'FM6126A', value: 2 },
  { label: 'ICN2038S', value: 3 },
  { label: 'MBI5124', value: 4 },
  { label: 'DP3246', value: 5 },
]

const SPEED_OPTIONS = [
  { label: 'HZ_8M', value: 8000000 },
  { label: 'HZ_16M', value: 16000000 },
  { label: 'HZ_20M', value: 20000000 },
]

const ROTATION_OPTIONS = [
  { label: '0°', value: 0 },
  { label: '90°', value: 1 },
  { label: '180°', value: 2 },
  { label: '270°', value: 3 },
]

const COLOR_ORDER_OPTIONS = [
  { label: 'RGB', swapBlueGreen: false, swapBlueRed: false },
  { label: 'RBG', swapBlueGreen: true, swapBlueRed: false },
  { label: 'GBR', swapBlueGreen: false, swapBlueRed: true },
]

const NTP_PRESET_OPTIONS = [
  { label: '阿里云 2', value: 'ntp2.aliyun.com' },
  { label: '阿里云', value: 'ntp.aliyun.com' },
  { label: '腾讯云', value: 'ntp.tencent.com' },
  { label: '国家授时中心', value: 'ntp.ntsc.ac.cn' },
  { label: '自定义', value: '__custom__' },
]

const UI_BRIGHTNESS_MAX = 178

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    GlxSwitch,
    GlxSlider,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      deviceIp: '',
      brightness: 50,
      brightnessDay: 50,
      brightnessNight: 50,
      params: {
        displayRotation: 0,
        swapBlueGreen: false,
        swapBlueRed: false,
        clkphase: false,
        driver: 0,
        i2cSpeed: 8000000,
        E_pin: 18,
        ntpServer: 'ntp2.aliyun.com',
        nightStart: '22:00',
        nightEnd: '07:00',
      },
      info: {
        firmwareVersion: '',
        uptime: 0,
        wifiSsid: '',
      },
      snapshot: {
        displayBright: 89,
        brightnessDay: 89,
        brightnessNight: 89,
        displayRotation: 0,
        swapBlueGreen: false,
        swapBlueRed: false,
        clkphase: false,
        driver: 0,
        i2cSpeed: 8000000,
        E_pin: 18,
        ntpServer: 'ntp2.aliyun.com',
        nightStart: '22:00',
        nightEnd: '07:00',
      },
      saving: false,
      waitingForSaveSettle: false,
      saveSettled: false,
      saveFinalizeTimer: null,
      saveNavigateTimer: null,
      leavingForDisconnect: false,
    }
  },
  computed: {
    isDeviceConnected() {
      if (this.deviceStore == null) {
        return false
      }
      return this.deviceStore.connected
    },
    driverLabels() {
      return DRIVER_OPTIONS.map((item) => item.label)
    },
    driverIndex() {
      return DRIVER_OPTIONS.findIndex((item) => item.value === this.params.driver)
    },
    speedLabels() {
      return SPEED_OPTIONS.map((item) => item.label)
    },
    speedIndex() {
      return SPEED_OPTIONS.findIndex((item) => item.value === this.params.i2cSpeed)
    },
    rotationLabels() {
      return ROTATION_OPTIONS.map((item) => item.label)
    },
    rotationIndex() {
      return ROTATION_OPTIONS.findIndex((item) => item.value === this.params.displayRotation)
    },
    colorOrderLabels() {
      return COLOR_ORDER_OPTIONS.map((item) => item.label)
    },
    colorOrderIndex() {
      return COLOR_ORDER_OPTIONS.findIndex((item) => {
        return (
          item.swapBlueGreen === this.params.swapBlueGreen &&
          item.swapBlueRed === this.params.swapBlueRed
        )
      })
    },
    ntpPresetLabels() {
      return NTP_PRESET_OPTIONS.map((item) => item.label)
    },
    ntpPresetIndex() {
      const index = NTP_PRESET_OPTIONS.findIndex((item) => item.value === this.params.ntpServer)
      if (index >= 0) {
        return index
      }
      return NTP_PRESET_OPTIONS.length - 1
    },
    formattedUptime() {
      const totalSeconds = Number(this.info.uptime)
      if (!Number.isFinite(totalSeconds) || totalSeconds <= 0) {
        return '--'
      }
      const days = Math.floor(totalSeconds / 86400)
      const hours = Math.floor((totalSeconds % 86400) / 3600)
      const minutes = Math.floor((totalSeconds % 3600) / 60)
      if (days > 0) {
        return `${days}天 ${hours}小时 ${minutes}分`
      }
      if (hours > 0) {
        return `${hours}小时 ${minutes}分`
      }
      return `${minutes}分`
    },
  },
  watch: {
    isDeviceConnected(nextValue, previousValue) {
      if (nextValue !== false) {
        return
      }
      if (previousValue === true && this.saving && this.waitingForSaveSettle) {
        this.finishSaveAndLeave()
        return
      }
      this.leaveWhenDisconnected()
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore()
    this.toast = useToast()
    this.deviceStore.init()

    const savedIp = uni.getStorageSync('device_ip')
    if (typeof savedIp === 'string') {
      this.deviceIp = savedIp
    }
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef)
    }
  },
  onHide() {
    this.clearSaveTimers()
  },
  onUnload() {
    this.clearSaveTimers()
  },
  onShow() {
    if (this.deviceStore && typeof this.deviceStore.deviceIp === 'string' && this.deviceStore.deviceIp.length > 0) {
      this.deviceIp = this.deviceStore.deviceIp
    }
    if (!this.ensureConnectedOrLeave()) {
      return
    }
    this.loadDeviceParams()
  },
  methods: {
    goBack() {
      uni.navigateBack()
    },
    ensureConnectedOrLeave() {
      if (this.isDeviceConnected) {
        return true
      }
      this.leaveWhenDisconnected()
      return false
    },
    leaveWhenDisconnected() {
      if (this.leavingForDisconnect) {
        return
      }
      this.leavingForDisconnect = true
      this.clearSaveTimers()
      this.saving = false
      this.waitingForSaveSettle = false
      uni.hideLoading()
      setTimeout(() => {
        const pages = getCurrentPages()
        if (pages.length > 1) {
          uni.navigateBack()
          return
        }
        uni.switchTab({
          url: '/pages/control/control',
        })
      }, 0)
    },
    clearSaveTimers() {
      if (this.saveFinalizeTimer != null) {
        clearTimeout(this.saveFinalizeTimer)
        this.saveFinalizeTimer = null
      }
      if (this.saveNavigateTimer != null) {
        clearTimeout(this.saveNavigateTimer)
        this.saveNavigateTimer = null
      }
    },
    finishSaveAndLeave() {
      if (this.saveSettled) {
        return
      }
      this.saveSettled = true
      this.waitingForSaveSettle = false
      this.clearSaveTimers()
      this.saving = false
      uni.hideLoading()
      this.toast.showSuccess('设备参数已应用')
      this.saveNavigateTimer = setTimeout(() => {
        this.saveNavigateTimer = null
        uni.navigateBack()
      }, 260)
    },
    scheduleSaveFallback() {
      this.clearSaveTimers()
      this.saveFinalizeTimer = setTimeout(() => {
        this.saveFinalizeTimer = null
        this.finishSaveAndLeave()
      }, 1200)
    },
    convertBrightnessToPercent(rawValue) {
      const numericValue = Number(rawValue)
      if (!Number.isFinite(numericValue)) {
        return 0
      }
      const percent = Math.round((numericValue / UI_BRIGHTNESS_MAX) * 100)
      if (percent < 0) {
        return 0
      }
      if (percent > 100) {
        return 100
      }
      return percent
    },
    convertPercentToBrightness(percentValue) {
      const numericValue = Number(percentValue)
      if (!Number.isFinite(numericValue)) {
        return 0
      }
      const rawValue = Math.round((numericValue * UI_BRIGHTNESS_MAX) / 100)
      if (rawValue < 0) {
        return 0
      }
      if (rawValue > UI_BRIGHTNESS_MAX) {
        return UI_BRIGHTNESS_MAX
      }
      return rawValue
    },
    getBaseUrl() {
      if (this.deviceIp.length === 0) {
        throw new Error('请先连接设备')
      }
      return `http://${this.deviceIp}`
    },
    requestDevice(options) {
      return new Promise((resolve, reject) => {
        uni.request({
          ...options,
          success: (res) => {
            if (typeof res.statusCode === 'number' && res.statusCode >= 200 && res.statusCode < 300) {
              resolve(res)
              return
            }
            reject(new Error('设备请求失败'))
          },
          fail: (err) => {
            reject(err)
          },
        })
      })
    },
    parseJsonData(data) {
      if (typeof data === 'string') {
        return JSON.parse(data)
      }
      return data
    },
    async loadDeviceParams() {
      if (this.deviceIp.length === 0) {
        return
      }

      try {
        const baseUrl = this.getBaseUrl()
        const response = await this.requestDevice({
          url: `${baseUrl}/get`,
          method: 'GET',
        })
        const data = this.parseJsonData(response.data)
        const displayBright = Number(data.displayBright)
        const brightnessDay = Number(data.brightnessDay)
        const brightnessNight = Number(data.brightnessNight)

        this.snapshot = {
          displayBright,
          brightnessDay,
          brightnessNight,
          displayRotation: Number(data.displayRotation),
          swapBlueGreen: data.swapBlueGreen === true,
          swapBlueRed: data.swapBlueRed === true,
          clkphase: data.clkphase === true,
          driver: Number(data.driver),
          i2cSpeed: Number(data.i2cSpeed),
          E_pin: Number(data.E_pin),
          ntpServer: String(data.ntpServer),
          nightStart: String(data.nightStart),
          nightEnd: String(data.nightEnd),
        }

        this.brightness = this.convertBrightnessToPercent(displayBright)
        this.brightnessDay = this.convertBrightnessToPercent(brightnessDay)
        this.brightnessNight = this.convertBrightnessToPercent(brightnessNight)
        this.params = {
          displayRotation: this.snapshot.displayRotation,
          swapBlueGreen: this.snapshot.swapBlueGreen,
          swapBlueRed: this.snapshot.swapBlueRed,
          clkphase: this.snapshot.clkphase,
          driver: this.snapshot.driver,
          i2cSpeed: this.snapshot.i2cSpeed,
          E_pin: this.snapshot.E_pin,
          ntpServer: this.snapshot.ntpServer,
          nightStart: this.snapshot.nightStart,
          nightEnd: this.snapshot.nightEnd,
        }
        let firmwareVersion = ''
        if (typeof data.firmwareVersion === 'string') {
          firmwareVersion = data.firmwareVersion
        }
        let uptime = 0
        if (typeof data.uptime === 'number') {
          uptime = data.uptime
        }
        let wifiSsid = ''
        if (typeof data.wifiSsid === 'string') {
          wifiSsid = data.wifiSsid
        }
        this.info = {
          firmwareVersion,
          uptime,
          wifiSsid,
        }
      } catch (err) {
        console.error('加载设备参数失败:', err)
      }
    },
    handleBrightnessDayChange(e) {
      this.brightnessDay = Number(e.detail.value)
    },
    handleBrightnessNightChange(e) {
      this.brightnessNight = Number(e.detail.value)
    },
    handleRotationChange(e) {
      const index = Number(e.detail.value)
      this.params.displayRotation = ROTATION_OPTIONS[index].value
    },
    handleColorOrderChange(e) {
      const index = Number(e.detail.value)
      const option = COLOR_ORDER_OPTIONS[index]
      this.params.swapBlueGreen = option.swapBlueGreen
      this.params.swapBlueRed = option.swapBlueRed
    },
    handleDriverChange(e) {
      const index = Number(e.detail.value)
      this.params.driver = DRIVER_OPTIONS[index].value
    },
    handleSpeedChange(e) {
      const index = Number(e.detail.value)
      this.params.i2cSpeed = SPEED_OPTIONS[index].value
    },
    handleSwitchChange(key, event) {
      this.params[key] = event.detail.value === true
    },
    handleEPinInput(event) {
      const rawValue = String(event.detail.value)
      if (rawValue.length === 0) {
        this.params.E_pin = 0
        return
      }
      this.params.E_pin = Number(rawValue)
    },
    handleNightStartChange(event) {
      this.params.nightStart = String(event.detail.value)
    },
    handleNightEndChange(event) {
      this.params.nightEnd = String(event.detail.value)
    },
    handleNtpPresetChange(e) {
      const index = Number(e.detail.value)
      const option = NTP_PRESET_OPTIONS[index]
      if (option.value !== '__custom__') {
        this.params.ntpServer = option.value
      }
    },
    handleNtpServerInput(event) {
      this.params.ntpServer = String(event.detail.value).trim()
    },
    async updateDeviceParam(key, value) {
      const baseUrl = this.getBaseUrl()
      await this.requestDevice({
        url: `${baseUrl}/set?${key}=${encodeURIComponent(String(value))}`,
        method: 'POST',
      })
    },
    async updateBrightness(rawValue) {
      const baseUrl = this.getBaseUrl()
      await this.requestDevice({
        url: `${baseUrl}/brightness`,
        method: 'POST',
        data: `value=${rawValue}`,
        header: {
          'content-type': 'application/x-www-form-urlencoded',
        },
      })
    },
    async saveDeviceParams() {
      if (this.saving) {
        return
      }

      if (!Number.isInteger(this.params.E_pin) || this.params.E_pin < 0 || this.params.E_pin > 32) {
        this.toast.showError('E_pin 必须是 0 到 32 之间的整数')
        return
      }
      if (this.params.ntpServer.length === 0) {
        this.toast.showError('ntpServer 不能为空')
        return
      }

      try {
        this.saving = true
        this.saveSettled = false
        this.waitingForSaveSettle = false
        this.clearSaveTimers()
        const nextDisplayBright = this.convertPercentToBrightness(this.brightness)
        const nextBrightnessDay = this.convertPercentToBrightness(this.brightnessDay)
        const nextBrightnessNight = this.convertPercentToBrightness(this.brightnessNight)
        const updates = []

        if (this.snapshot.displayBright !== nextDisplayBright) {
          updates.push(() => this.updateBrightness(nextDisplayBright))
        }
        if (this.snapshot.brightnessDay !== nextBrightnessDay) {
          updates.push(() => this.updateDeviceParam('brightnessDay', nextBrightnessDay))
        }
        if (this.snapshot.brightnessNight !== nextBrightnessNight) {
          updates.push(() => this.updateDeviceParam('brightnessNight', nextBrightnessNight))
        }
        if (this.snapshot.displayRotation !== this.params.displayRotation) {
          updates.push(() => this.updateDeviceParam('displayRotation', this.params.displayRotation))
        }
        if (this.snapshot.swapBlueGreen !== this.params.swapBlueGreen) {
          updates.push(() => this.updateDeviceParam('swapBlueGreen', this.params.swapBlueGreen ? 1 : 0))
        }
        if (this.snapshot.swapBlueRed !== this.params.swapBlueRed) {
          updates.push(() => this.updateDeviceParam('swapBlueRed', this.params.swapBlueRed ? 1 : 0))
        }
        if (this.snapshot.clkphase !== this.params.clkphase) {
          updates.push(() => this.updateDeviceParam('clkphase', this.params.clkphase ? 1 : 0))
        }
        if (this.snapshot.driver !== this.params.driver) {
          updates.push(() => this.updateDeviceParam('driver', this.params.driver))
        }
        if (this.snapshot.i2cSpeed !== this.params.i2cSpeed) {
          updates.push(() => this.updateDeviceParam('i2cSpeed', this.params.i2cSpeed))
        }
        if (this.snapshot.E_pin !== this.params.E_pin) {
          updates.push(() => this.updateDeviceParam('E_pin', this.params.E_pin))
        }
        if (this.snapshot.nightStart !== this.params.nightStart) {
          updates.push(() => this.updateDeviceParam('nightStart', this.params.nightStart))
        }
        if (this.snapshot.nightEnd !== this.params.nightEnd) {
          updates.push(() => this.updateDeviceParam('nightEnd', this.params.nightEnd))
        }
        if (this.snapshot.ntpServer !== this.params.ntpServer) {
          updates.push(() => this.updateDeviceParam('ntpServer', this.params.ntpServer))
        }

        if (updates.length === 0) {
          this.toast.showInfo('参数没有变化')
          this.saving = false
          return
        }

        uni.showLoading({ title: '应用中...', mask: true })
        for (let i = 0; i < updates.length; i += 1) {
          await updates[i]()
        }
        this.waitingForSaveSettle = true
        this.scheduleSaveFallback()
        await this.loadDeviceParams()
        this.finishSaveAndLeave()
      } catch (err) {
        if (this.saveSettled) {
          return
        }
        if (this.waitingForSaveSettle) {
          this.finishSaveAndLeave()
          return
        }
        console.error('保存设备参数失败:', err)
        this.toast.showError('保存失败，请检查设备连接')
      } finally {
        if (!this.saveSettled) {
          this.saving = false
          this.waitingForSaveSettle = false
          this.clearSaveTimers()
          uni.hideLoading()
        }
      }
    },
    async handleResetWifi() {
      if (this.deviceIp.length === 0) {
        this.toast.showError('请先连接设备')
        return
      }

      uni.showModal({
        title: '重置网络',
        content: '将清除当前 WiFi 配置并重启设备，是否继续？',
        confirmColor: '#FF7A45',
        success: async (result) => {
          if (result.confirm !== true) {
            return
          }
          try {
            const baseUrl = this.getBaseUrl()
            await this.requestDevice({
              url: `${baseUrl}/clear-wifi`,
              method: 'GET',
            })
            this.toast.showSuccess('设备已开始重置网络')
          } catch (err) {
            console.error('重置网络失败:', err)
            this.toast.showError('重置网络失败')
          }
        },
      })
    },
  },
}
</script>

<style scoped>
.device-params-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--nb-paper);
  overflow: hidden;
}

.navbar {
  background: var(--nb-surface);
  border-bottom: 2rpx solid var(--nb-ink);
}

.content {
  flex: 1;
  padding: 16rpx 20rpx 0;
  box-sizing: border-box;
  overflow-y: scroll;
}

.section-block {
  margin-bottom: 24rpx;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 4rpx 4rpx 14rpx;
}

.section-title {
  font-size: 28rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.section-meta,
.panel-action-desc,
.param-desc {
  font-size: 22rpx;
  color: var(--text-tertiary);
}

.panel-card {
  background: transparent;
  border: 0;
  box-shadow: none;
  border-radius: 0;
  overflow: hidden;
}

.panel-action {
  display: flex;
  align-items: center;
  gap: 18rpx;
  padding: 24rpx;
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.panel-action.danger {
  background: var(--nb-surface);
}

.panel-action-icon {
  width: 72rpx;
  height: 72rpx;
  border-radius: 0;
  background: var(--nb-surface);
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.panel-action-icon.warm {
  background: var(--nb-yellow);
}

.panel-action-icon.blue {
  background: var(--nb-blue);
}

.panel-action-icon.danger {
  background: var(--nb-coral);
}

.panel-action-text {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 6rpx;
}

.panel-action-label {
  font-size: 28rpx;
  font-weight: 600;
  color: #111827;
}

.danger-text {
  color: var(--nb-coral);
}

.panel-divider {
  height: 2rpx;
  background: rgba(0, 0, 0, 0.12);
  margin: 0 24rpx;
}

.param-card {
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
  padding: 0 24rpx;
}

.param-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 20rpx;
  padding: 24rpx 0;
}

.param-row + .param-row {
  border-top: 2rpx solid rgba(0, 0, 0, 0.12);
}

.param-copy {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.param-label {
  font-size: 28rpx;
  font-weight: 600;
  color: #111827;
}

.param-stack {
  display: flex;
  flex-direction: column;
  gap: 10rpx;
  padding: 24rpx 0;
  border-top: 2rpx solid rgba(0, 0, 0, 0.12);
}

.param-stack.compact-gap {
  gap: 12rpx;
  border-top: 0;
}

.param-tip-box {
  border-top: 2rpx solid rgba(0, 0, 0, 0.12);
  padding: 24rpx 0;
}

.param-tip-text {
  font-size: 22rpx;
  line-height: 1.6;
  color: var(--text-tertiary);
}

.param-picker {
  width: 196rpx;
  min-width: 196rpx;
  min-height: 72rpx;
  padding: 0 20rpx;
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  flex-shrink: 0;
}

.param-picker-wide {
  width: 232rpx;
  min-width: 232rpx;
}

.param-picker-text {
  width: 100%;
  text-align: center;
  font-size: 26rpx;
  font-weight: 600;
  color: #111827;
  border: 0 !important;
  background: transparent !important;
  box-shadow: none !important;
}

.number-input {
  width: 136rpx;
  height: 82rpx;
  padding: 0 18rpx;
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  box-sizing: border-box;
  text-align: center;
  font-size: 28rpx;
  font-weight: 600;
  color: #111827;
}

.text-input {
  min-height: 88rpx;
  padding: 0 22rpx;
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  box-sizing: border-box;
  font-size: 26rpx;
  font-weight: 600;
  color: #111827;
}

.slider-inline-head {
  display: flex;
  justify-content: flex-end;
}

.slider-inline-value,
.readonly-value {
  font-size: 24rpx;
  font-weight: 600;
  color: #111827;
}

.value-box {
  min-width: 188rpx;
  min-height: 82rpx;
  padding: 0 20rpx;
  border: 2rpx solid #000000;
  background: #ffffff;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  text-align: center;
}
</style>
