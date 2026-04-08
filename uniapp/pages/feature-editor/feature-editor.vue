<template>
  <view class="feature-page">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="header">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <view class="nav-title">
        <text class="project-name">{{ pageTitle }}</text>
      </view>
    </view>

    <view class="canvas-section">
      <view v-if="!previewHidden" class="preview-canvas-container">
        <PixelCanvas
          v-if="previewCanvasReady && featureReady"
          :width="64"
          :height="64"
          :pixels="currentPreviewPixels"
          :zoom="previewZoom"
          :offset-x="previewOffset.x"
          :offset-y="previewOffset.y"
          :canvas-width="previewContainerSize.width"
          :canvas-height="previewContainerSize.height"
          :grid-visible="true"
          :is-dark-mode="true"
          :touch-enabled="false"
          canvas-id="featurePreviewCanvas"
        />
      </view>
      <view
        v-else
        class="preview-canvas-container preview-canvas-placeholder"
      ></view>
      <view class="preview-caption">
        <view class="preview-caption-info">
          <text class="preview-title">预览效果</text>
        </view>
        <view class="preview-actions">
          <view
            class="action-btn-sm primary"
            :class="{ disabled: isSending }"
            @click="saveAndApply"
          >
            <Icon name="link" :size="36" color="#ffffff" />
            <text>{{ isSending ? "发送中" : "发送" }}</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper" v-if="featureReady">
        <view v-if="featureType === 'weather'" class="card">
          <view class="card-title-section">
            <text class="card-title">天气快捷样式</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="preset in weatherPresets"
              :key="preset.key"
              class="piece-item"
              :class="{ active: selectedWeatherPresetKey === preset.key }"
              @click="applyWeatherPreset(preset)"
            >
              <view class="piece-top">
                <text class="piece-tag">{{ preset.tag }}</text>
                <view class="piece-palette">
                  <view
                    v-for="color in preset.colors"
                    :key="color"
                    class="piece-dot"
                    :style="{ backgroundColor: color }"
                  ></view>
                </view>
              </view>
              <text class="piece-name">{{ preset.label }}</text>
              <text class="piece-hint">{{ preset.hint }}</text>
            </view>
          </view>
        </view>

        <view v-if="featureType === 'weather'" class="card">
          <view class="card-title-section">
            <text class="card-title">天气内容</text>
          </view>

          <view class="form-row">
            <text class="form-label">天气图标</text>
            <view class="option-row option-row-wrap">
              <view
                v-for="item in weatherTypeOptions"
                :key="item.value"
                class="option-btn option-btn-small"
                :class="{ active: weatherConfig.weatherType === item.value }"
                @click="weatherConfig.weatherType = item.value"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view class="form-row">
            <text class="form-label">布局方式</text>
            <view class="option-row">
              <view
                v-for="item in weatherLayoutOptions"
                :key="item.value"
                class="option-btn"
                :class="{ active: weatherConfig.layout === item.value }"
                @click="weatherConfig.layout = item.value"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view class="form-row">
            <text class="form-label">城市标签</text>
            <input
              v-model="weatherConfig.city"
              class="text-input"
              maxlength="8"
              placeholder="例如 SHANGHAI"
            />
          </view>

          <view class="form-row">
            <text class="form-label">温度 {{ weatherConfig.temperature }}°</text>
            <slider
              :value="weatherConfig.temperature"
              min="-20"
              max="45"
              step="1"
              activeColor="#4F7FFF"
              @change="handleWeatherTemperatureChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">湿度 {{ weatherConfig.humidity }}%</text>
            <slider
              :value="weatherConfig.humidity"
              min="0"
              max="100"
              step="1"
              activeColor="#4F7FFF"
              @change="handleWeatherHumidityChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">温度单位</text>
            <view class="option-row">
              <view
                v-for="item in weatherUnitOptions"
                :key="item.value"
                class="option-btn"
                :class="{ active: weatherConfig.unit === item.value }"
                @click="weatherConfig.unit = item.value"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view class="form-row">
            <text class="form-label">图标颜色</text>
            <ColorPanelPicker
              :value="weatherConfig.accentColor"
              label="图标颜色"
              :preset-colors="colorOptions"
              @input="handleWeatherAccentColorChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">温度颜色</text>
            <ColorPanelPicker
              :value="weatherConfig.tempColor"
              label="温度颜色"
              :preset-colors="colorOptions"
              @input="handleWeatherTempColorChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">湿度颜色</text>
            <ColorPanelPicker
              :value="weatherConfig.humidityColor"
              label="湿度颜色"
              :preset-colors="colorOptions"
              @input="handleWeatherHumidityColorChange"
            />
          </view>
        </view>

        <view v-if="featureType === 'countdown'" class="card">
          <view class="card-title-section">
            <text class="card-title">倒计时快捷样式</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="preset in countdownPresets"
              :key="preset.key"
              class="piece-item"
              :class="{ active: selectedCountdownPresetKey === preset.key }"
              @click="applyCountdownPreset(preset)"
            >
              <view class="piece-top">
                <text class="piece-tag">{{ preset.tag }}</text>
                <view class="piece-palette">
                  <view
                    v-for="color in preset.colors"
                    :key="color"
                    class="piece-dot"
                    :style="{ backgroundColor: color }"
                  ></view>
                </view>
              </view>
              <text class="piece-name">{{ preset.label }}</text>
              <text class="piece-hint">{{ preset.hint }}</text>
            </view>
          </view>
        </view>

        <view v-if="featureType === 'countdown'" class="card">
          <view class="card-title-section">
            <text class="card-title">倒计时参数</text>
          </view>

          <view class="section-group">
            <view class="section-heading">
              <text class="section-title">表现样式</text>
            </view>

            <view class="form-row">
              <text class="form-label">显示样式</text>
              <view class="option-row option-row-wrap">
                <view
                  v-for="item in countdownStyleOptions"
                  :key="item.value"
                  class="option-btn option-btn-small"
                  :class="{ active: countdownConfig.displayStyle === item.value }"
                  @click="countdownConfig.displayStyle = item.value"
                >
                  <text>{{ item.label }}</text>
                </view>
              </view>
            </view>
          </view>

          <view class="section-divider"></view>

          <view class="section-group">
            <view class="section-heading">
              <text class="section-title">时间设定</text>
            </view>

            <view class="form-row">
              <text class="form-label">小时 {{ countdownConfig.hours }}</text>
              <slider
                :value="countdownConfig.hours"
                min="0"
                max="23"
                step="1"
                activeColor="#4F7FFF"
                @change="handleCountdownHoursChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">分钟 {{ countdownConfig.minutes }}</text>
              <slider
                :value="countdownConfig.minutes"
                min="0"
                max="59"
                step="1"
                activeColor="#4F7FFF"
                @change="handleCountdownMinutesChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">秒数 {{ countdownConfig.seconds }}</text>
              <slider
                :value="countdownConfig.seconds"
                min="0"
                max="59"
                step="1"
                activeColor="#4F7FFF"
                @change="handleCountdownSecondsChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label"
                >当前进度 {{ Math.round(countdownConfig.progress * 100) }}%</text
              >
              <slider
                :value="Math.round(countdownConfig.progress * 100)"
                min="0"
                max="100"
                step="1"
                activeColor="#4F7FFF"
                @change="handleCountdownProgressChange"
              />
            </view>
          </view>

          <view class="section-divider"></view>

          <view class="section-group">
            <view class="section-heading">
              <text class="section-title">颜色表现</text>
            </view>

            <view class="form-row">
              <text class="form-label">颜色模式</text>
              <view class="option-row">
                <view
                  v-for="item in countdownColorModeOptions"
                  :key="item.value"
                  class="option-btn"
                  :class="{ active: countdownConfig.colorMode === item.value }"
                  @click="countdownConfig.colorMode = item.value"
                >
                  <text>{{ item.label }}</text>
                </view>
              </view>
            </view>

            <view class="form-row">
              <text class="form-label">强调颜色</text>
              <ColorPanelPicker
                :value="countdownConfig.accentColor"
                label="强调颜色"
                :preset-colors="colorOptions"
                @input="handleCountdownAccentColorChange"
              />
            </view>
          </view>
        </view>

        <view v-if="featureType === 'stopwatch'" class="card">
          <view class="card-title-section">
            <text class="card-title">秒表快捷样式</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="preset in stopwatchPresets"
              :key="preset.key"
              class="piece-item"
              :class="{ active: selectedStopwatchPresetKey === preset.key }"
              @click="applyStopwatchPreset(preset)"
            >
              <view class="piece-top">
                <text class="piece-tag">{{ preset.tag }}</text>
                <view class="piece-palette">
                  <view
                    v-for="color in preset.colors"
                    :key="color"
                    class="piece-dot"
                    :style="{ backgroundColor: color }"
                  ></view>
                </view>
              </view>
              <text class="piece-name">{{ preset.label }}</text>
              <text class="piece-hint">{{ preset.hint }}</text>
            </view>
          </view>
        </view>

        <view v-if="featureType === 'stopwatch'" class="card">
          <view class="card-title-section">
            <text class="card-title">秒表参数</text>
          </view>

          <view class="form-row">
            <text class="form-label">显示样式</text>
            <view class="option-row">
              <view
                v-for="item in stopwatchStyleOptions"
                :key="item.value"
                class="option-btn"
                :class="{ active: stopwatchConfig.displayStyle === item.value }"
                @click="stopwatchConfig.displayStyle = item.value"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view class="form-row inline-row">
            <text class="form-label">显示百分秒</text>
            <view
              class="toggle-switch"
              @click="stopwatchConfig.showMilliseconds = !stopwatchConfig.showMilliseconds"
            >
              <view
                class="switch-track"
                :class="{ active: stopwatchConfig.showMilliseconds }"
              >
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>

          <view class="form-row">
            <text class="form-label"
              >预览时长 {{ stopwatchConfig.previewSeconds.toFixed(2) }}s</text
            >
            <slider
              :value="Math.round(stopwatchConfig.previewSeconds * 10)"
              min="0"
              max="3600"
              step="1"
              activeColor="#4F7FFF"
              @change="handleStopwatchPreviewSecondsChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">圈次数量 {{ stopwatchConfig.lapCount }}</text>
            <slider
              :value="stopwatchConfig.lapCount"
              min="0"
              max="9"
              step="1"
              activeColor="#4F7FFF"
              @change="handleStopwatchLapCountChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">强调颜色</text>
            <ColorPanelPicker
              :value="stopwatchConfig.accentColor"
              label="强调颜色"
              :preset-colors="colorOptions"
              @input="handleStopwatchAccentColorChange"
            />
          </view>
        </view>

        <view v-if="featureType === 'notification'" class="card">
          <view class="card-title-section">
            <text class="card-title">通知提醒快捷样式</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="preset in notificationPresets"
              :key="preset.key"
              class="piece-item"
              :class="{ active: selectedNotificationPresetKey === preset.key }"
              @click="applyNotificationPreset(preset)"
            >
              <view class="piece-top">
                <text class="piece-tag">{{ preset.tag }}</text>
                <view class="piece-palette">
                  <view
                    v-for="color in preset.colors"
                    :key="color"
                    class="piece-dot"
                    :style="{ backgroundColor: color }"
                  ></view>
                </view>
              </view>
              <text class="piece-name">{{ preset.label }}</text>
              <text class="piece-hint">{{ preset.hint }}</text>
            </view>
          </view>
        </view>

        <view v-if="featureType === 'notification'" class="card">
          <view class="card-title-section">
            <text class="card-title">触发规则</text>
          </view>

          <view class="section-group">
            <view class="section-heading">
              <text class="section-title">定时触发</text>
            </view>

            <view class="form-row inline-row">
              <text class="form-label">触发时间</text>
              <text class="inline-value">{{ notificationTimeLabel }}</text>
            </view>

            <view class="form-row">
              <text class="form-label">小时 {{ notificationConfig.hour }}</text>
              <slider
                :value="notificationConfig.hour"
                min="0"
                max="23"
                step="1"
                activeColor="#4F7FFF"
                @change="handleNotificationHourChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">分钟 {{ notificationConfig.minute }}</text>
              <slider
                :value="notificationConfig.minute"
                min="0"
                max="59"
                step="1"
                activeColor="#4F7FFF"
                @change="handleNotificationMinuteChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">重复规则</text>
              <view class="option-row">
                <view
                  v-for="item in notificationRepeatOptions"
                  :key="item.value"
                  class="option-btn"
                  :class="{ active: notificationConfig.repeatMode === item.value }"
                  @click="notificationConfig.repeatMode = item.value"
                >
                  <text>{{ item.label }}</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view v-if="featureType === 'notification'" class="card">
          <view class="card-title-section">
            <text class="card-title">到点展示内容</text>
          </view>

          <view class="section-group">
            <view class="section-heading">
              <text class="section-title">主题内容</text>
            </view>

            <view class="form-row">
              <text class="form-label">展示主题</text>
              <view class="option-row option-row-wrap">
                <view
                  v-for="item in notificationIconOptions"
                  :key="item.value"
                  class="option-btn option-btn-small"
                  :class="{ active: notificationConfig.icon === item.value }"
                  @click="notificationConfig.icon = item.value"
                >
                  <text>{{ item.label }}</text>
                </view>
              </view>
            </view>

            <view class="form-row">
              <text class="form-label">文案内容</text>
              <input
                v-model="notificationConfig.text"
                class="text-input"
                maxlength="12"
                placeholder="例如 DRINK / HAPPY"
              />
            </view>
          </view>

          <view class="section-divider"></view>

          <view class="section-group">
            <view class="section-heading">
              <text class="section-title">视觉主题</text>
            </view>

            <view class="form-row">
              <text class="form-label">强调颜色</text>
              <ColorPanelPicker
                :value="notificationConfig.accentColor"
                label="强调颜色"
                :preset-colors="colorOptions"
                @input="handleNotificationAccentColorChange"
              />
            </view>
          </view>
        </view>

      </view>
    </scroll-view>

    <Toast ref="toastRef" @show="previewHidden = true" @hide="onToastHide" />

    <view v-if="isSending" class="sending-overlay" @touchmove.stop.prevent>
      <view class="sending-modal">
        <view class="sending-spinner"></view>
        <text class="sending-title">正在发送功能预览...</text>
        <text class="sending-tip">请保持设备在线，发送完成后会自动恢复页面</text>
      </view>
    </view>
  </view>
</template>

<script>
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import ColorPanelPicker from "../../components/ColorPanelPicker.vue";
import { uploadAnimationFrames } from "../../utils/animationUploader.js";
import {
  buildCountdownPreviewFrames,
  buildNotificationPreviewFrames,
  buildStopwatchPreviewFrames,
  buildWeatherPreviewFrames,
} from "../../utils/featurePreviewBuilders.js";
import { pixelPreviewFramesToAnimationFrames } from "../../utils/pixelAnimationFrames.js";

const WEATHER_DRAFT_KEY = "weather_feature_draft";
const COUNTDOWN_DRAFT_KEY = "countdown_feature_draft";
const STOPWATCH_DRAFT_KEY = "stopwatch_feature_draft";
const NOTIFICATION_DRAFT_KEY = "notification_feature_draft";

function createWeatherConfig() {
  return {
    weatherType: "sunny",
    layout: "standard",
    city: "SHANGHAI",
    temperature: 26,
    humidity: 56,
    unit: "celsius",
    accentColor: "#64C8FF",
    tempColor: "#FFFFFF",
    humidityColor: "#6ED0FF",
  };
}

function createCountdownConfig() {
  return {
    hours: 0,
    minutes: 25,
    seconds: 0,
    progress: 0.72,
    displayStyle: "hourglass",
    colorMode: "auto",
    accentColor: "#64C8FF",
  };
}

function createStopwatchConfig() {
  return {
    displayStyle: "training",
    previewSeconds: 83.42,
    showMilliseconds: true,
    lapCount: 3,
    accentColor: "#FFB020",
  };
}

function createNotificationConfig() {
  return {
    icon: "drink",
    text: "DRINK",
    hour: 9,
    minute: 30,
    repeatMode: "daily",
    accentColor: "#FFB020",
  };
}

function cloneConfig(config) {
  return JSON.parse(JSON.stringify(config));
}

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    PixelCanvas,
    ColorPanelPicker,
  },
  data() {
    return {
      toast: null,
      deviceStore: null,
      featureType: "",
      featureReady: false,
      invalidFeature: false,
      isSending: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      previewCanvasReady: false,
      previewHidden: false,
      previewZoom: 4,
      previewOffset: { x: 16, y: 16 },
      previewContainerSize: { width: 320, height: 320 },
      previewFrameMaps: [],
      previewFrameDelays: [],
      previewFrameIndex: 0,
      previewTimer: null,
      previewRefreshTimer: null,
      previewRefreshToken: 0,
      selectedWeatherPresetKey: "",
      selectedCountdownPresetKey: "",
      selectedStopwatchPresetKey: "",
      selectedNotificationPresetKey: "",
      weatherConfig: createWeatherConfig(),
      countdownConfig: createCountdownConfig(),
      stopwatchConfig: createStopwatchConfig(),
      notificationConfig: createNotificationConfig(),
      colorOptions: [
        { hex: "#FFFFFF" },
        { hex: "#64C8FF" },
        { hex: "#4F7FFF" },
        { hex: "#7DDF8A" },
        { hex: "#FFD24D" },
        { hex: "#FFB020" },
        { hex: "#FF6464" },
        { hex: "#FF6FAE" },
      ],
      weatherTypeOptions: [
        { label: "晴天", value: "sunny" },
        { label: "多云", value: "cloudy" },
        { label: "雨天", value: "rainy" },
        { label: "雪天", value: "snow" },
        { label: "雷暴", value: "thunder" },
      ],
      weatherLayoutOptions: [
        { label: "标准", value: "standard" },
        { label: "详情", value: "detail" },
        { label: "夜间", value: "night" },
      ],
      weatherUnitOptions: [
        { label: "摄氏", value: "celsius" },
        { label: "华氏", value: "fahrenheit" },
      ],
      countdownStyleOptions: [
        { label: "沙漏", value: "hourglass" },
        { label: "冲刺", value: "sprint" },
        { label: "环形", value: "ring" },
        { label: "条形", value: "bar" },
      ],
      countdownColorModeOptions: [
        { label: "自动变色", value: "auto" },
        { label: "固定颜色", value: "fixed" },
      ],
      stopwatchStyleOptions: [
        { label: "训练", value: "training" },
        { label: "竞速", value: "racing" },
        { label: "圈次", value: "lap_focus" },
      ],
      notificationIconOptions: [
        { label: "喝水", value: "drink" },
        { label: "休息", value: "break" },
        { label: "吃药", value: "medicine" },
        { label: "生日", value: "birthday" },
        { label: "烟花", value: "fireworks" },
        { label: "圣诞", value: "christmas" },
        { label: "爱心", value: "heart" },
        { label: "月亮", value: "moon" },
        { label: "星光", value: "star" },
      ],
      notificationRepeatOptions: [
        { label: "单次", value: "once" },
        { label: "每天", value: "daily" },
        { label: "工作日", value: "weekday" },
      ],
      weatherPresets: [
        {
          key: "sunny-standard",
          label: "晴天卡",
          tag: "信息卡",
          hint: "温度、天气、湿度一屏看清",
          colors: ["#FFD24D", "#DFF7FF", "#64C8FF"],
          config: {
            weatherType: "sunny",
            layout: "standard",
            city: "SHANGHAI",
            temperature: 28,
            humidity: 42,
            unit: "celsius",
            accentColor: "#FFD24D",
            tempColor: "#FFFFFF",
            humidityColor: "#64C8FF",
          },
        },
        {
          key: "rain-detail",
          label: "雨天卡",
          tag: "播报",
          hint: "适合潮湿和降温场景",
          colors: ["#64C8FF", "#DFF7FF", "#7DD3FC"],
          config: {
            weatherType: "rainy",
            layout: "detail",
            city: "HANGZHOU",
            temperature: 18,
            humidity: 88,
            unit: "celsius",
            accentColor: "#64C8FF",
            tempColor: "#FFFFFF",
            humidityColor: "#7DD3FC",
          },
        },
        {
          key: "night-cloud",
          label: "夜间天气卡",
          tag: "夜景",
          hint: "适合夜间和床头显示",
          colors: ["#9AABFF", "#F9F7D1", "#A0E9FF"],
          config: {
            weatherType: "cloudy",
            layout: "night",
            city: "SHENZHEN",
            temperature: 23,
            humidity: 64,
            unit: "celsius",
            accentColor: "#9AABFF",
            tempColor: "#F9F7D1",
            humidityColor: "#A0E9FF",
          },
        },
      ],
      countdownPresets: [
        {
          key: "quick-break",
          label: "沙漏倒数",
          tag: "沙漏",
          hint: "适合短时倒数",
          colors: ["#64C8FF", "#C7EEFF"],
          config: {
            hours: 0,
            minutes: 5,
            seconds: 0,
            progress: 0.68,
            displayStyle: "hourglass",
            colorMode: "auto",
            accentColor: "#64C8FF",
          },
        },
        {
          key: "focus-ring",
          label: "环形倒数",
          tag: "环形",
          hint: "适合专注计时",
          colors: ["#7DDF8A", "#D5FFD8"],
          config: {
            hours: 0,
            minutes: 25,
            seconds: 0,
            progress: 0.52,
            displayStyle: "ring",
            colorMode: "auto",
            accentColor: "#7DDF8A",
          },
        },
        {
          key: "event-sprint",
          label: "冲刺倒数",
          tag: "冲刺",
          hint: "适合最后冲刺",
          colors: ["#FF6464", "#FFD2D2"],
          config: {
            hours: 0,
            minutes: 1,
            seconds: 30,
            progress: 0.18,
            displayStyle: "sprint",
            colorMode: "fixed",
            accentColor: "#FF6464",
          },
        },
      ],
      stopwatchPresets: [
        {
          key: "training",
          label: "训练计时板",
          tag: "训练",
          hint: "主时间最醒目",
          colors: ["#FFB020", "#FFE3A4"],
          config: {
            displayStyle: "training",
            previewSeconds: 83.42,
            showMilliseconds: true,
            lapCount: 3,
            accentColor: "#FFB020",
          },
        },
        {
          key: "racing",
          label: "竞速计时板",
          tag: "竞速",
          hint: "适合竞速场景",
          colors: ["#FF6464", "#FFD2D2"],
          config: {
            displayStyle: "racing",
            previewSeconds: 38.17,
            showMilliseconds: true,
            lapCount: 1,
            accentColor: "#FF6464",
          },
        },
        {
          key: "lap-focus",
          label: "圈速计时板",
          tag: "圈次",
          hint: "适合看圈次变化",
          colors: ["#64C8FF", "#C7EEFF"],
          config: {
            displayStyle: "lap_focus",
            previewSeconds: 126.73,
            showMilliseconds: false,
            lapCount: 6,
            accentColor: "#64C8FF",
          },
        },
      ],
      notificationPresets: [
        {
          key: "drink",
          label: "喝水提醒卡",
          tag: "日常",
          hint: "适合日常提醒",
          colors: ["#64C8FF", "#C7EEFF"],
          config: {
            icon: "drink",
            text: "DRINK",
            hour: 9,
            minute: 30,
            repeatMode: "daily",
            accentColor: "#64C8FF",
          },
        },
        {
          key: "break",
          label: "休息提醒卡",
          tag: "办公",
          hint: "适合久坐休息",
          colors: ["#7DDF8A", "#D5FFD8"],
          config: {
            icon: "break",
            text: "REST",
            hour: 15,
            minute: 0,
            repeatMode: "weekday",
            accentColor: "#7DDF8A",
          },
        },
        {
          key: "fireworks",
          label: "烟花卡",
          tag: "节日",
          hint: "适合庆祝场景",
          colors: ["#FFB020", "#FFE3A4"],
          config: {
            icon: "fireworks",
            text: "HAPPY",
            hour: 0,
            minute: 0,
            repeatMode: "once",
            accentColor: "#FFB020",
          },
        },
        {
          key: "birthday",
          label: "生日祝福卡",
          tag: "庆祝",
          hint: "适合生日提醒",
          colors: ["#FF6FAE", "#FFD0E4"],
          config: {
            icon: "birthday",
            text: "PARTY",
            hour: 8,
            minute: 0,
            repeatMode: "once",
            accentColor: "#FF6FAE",
          },
        },
        {
          key: "christmas",
          label: "圣诞祝福卡",
          tag: "节日",
          hint: "适合节日祝福",
          colors: ["#7DDF8A", "#FFD24D"],
          config: {
            icon: "christmas",
            text: "XMAS",
            hour: 10,
            minute: 0,
            repeatMode: "once",
            accentColor: "#7DDF8A",
          },
        },
      ],
    };
  },
  computed: {
    currentPreviewPixels() {
      if (this.previewFrameMaps.length === 0) {
        return new Map();
      }
      if (this.previewFrameIndex >= this.previewFrameMaps.length) {
        return this.previewFrameMaps[0];
      }
      return this.previewFrameMaps[this.previewFrameIndex];
    },
    pageTitle() {
      if (this.featureType === "weather") {
        return "天气设置";
      }
      if (this.featureType === "countdown") {
        return "倒计时";
      }
      if (this.featureType === "stopwatch") {
        return "秒表设置";
      }
      if (this.featureType === "notification") {
        return "提醒设置";
      }
      return "扩展功能";
    },
    notificationTimeLabel() {
      return `${String(this.notificationConfig.hour).padStart(2, "0")}:${String(
        this.notificationConfig.minute,
      ).padStart(2, "0")}`;
    },
  },
  watch: {
    weatherConfig: {
      deep: true,
      handler() {
        if (this.featureType === "weather") {
          this.schedulePreviewRefresh();
        }
      },
    },
    countdownConfig: {
      deep: true,
      handler() {
        if (this.featureType === "countdown") {
          this.schedulePreviewRefresh();
        }
      },
    },
    stopwatchConfig: {
      deep: true,
      handler() {
        if (this.featureType === "stopwatch") {
          this.schedulePreviewRefresh();
        }
      },
    },
    notificationConfig: {
      deep: true,
      handler() {
        if (this.featureType === "notification") {
          this.schedulePreviewRefresh();
        }
      },
    },
    featureType() {
      this.schedulePreviewRefresh();
    },
  },
  onLoad(options) {
    this.toast = useToast();
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();

    if (!options || typeof options.feature !== "string") {
      this.invalidFeature = true;
      return;
    }

    if (
      options.feature !== "weather" &&
      options.feature !== "countdown" &&
      options.feature !== "stopwatch" &&
      options.feature !== "notification"
    ) {
      this.invalidFeature = true;
      return;
    }

    this.featureType = options.feature;
    this.featureReady = true;
    this.loadDraft();
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }

    if (this.invalidFeature) {
      uni.showToast({
        title: "功能参数缺失，无法打开页面",
        icon: "none",
      });
      setTimeout(() => {
        uni.navigateBack();
      }, 300);
      return;
    }

    this.initPreviewCanvas();
    this.schedulePreviewRefresh();
  },
  onShow() {
    if (!this.isSending) {
      this.previewHidden = false;
    }
    if (this.previewCanvasReady) {
      this.schedulePreviewRefresh();
    }
  },
  onHide() {
    this.cleanupPreviewTimers();
  },
  onUnload() {
    this.cleanupPreviewTimers();
  },
  methods: {
    handleBack() {
      uni.navigateBack();
    },
    onToastHide() {
      if (!this.isSending) {
        this.previewHidden = false;
      }
    },
    schedulePreviewRefresh() {
      if (this.previewRefreshTimer) {
        clearTimeout(this.previewRefreshTimer);
        this.previewRefreshTimer = null;
      }
      this.previewRefreshTimer = setTimeout(() => {
        this.refreshPreviewFrames();
      }, 80);
    },
    refreshPreviewFrames() {
      const token = this.previewRefreshToken + 1;
      this.previewRefreshToken = token;
      const frames = this.buildFeaturePreviewFrames();
      if (token !== this.previewRefreshToken) {
        return;
      }
      this.previewFrameMaps = frames.map((frame) => frame.pixels);
      this.previewFrameDelays = frames.map((frame) => frame.delay);
      this.previewFrameIndex = 0;
      this.startPreviewPlayback();
    },
    startPreviewPlayback() {
      this.stopPreviewPlayback();
      if (this.previewFrameMaps.length <= 1) {
        return;
      }
      const playNext = () => {
        const delay = this.previewFrameDelays[this.previewFrameIndex];
        const safeDelay = typeof delay === "number" ? delay : 120;
        this.previewTimer = setTimeout(() => {
          if (this.previewFrameMaps.length === 0) {
            return;
          }
          const nextIndex = this.previewFrameIndex + 1;
          this.previewFrameIndex =
            nextIndex >= this.previewFrameMaps.length ? 0 : nextIndex;
          playNext();
        }, safeDelay);
      };
      playNext();
    },
    stopPreviewPlayback() {
      if (this.previewTimer) {
        clearTimeout(this.previewTimer);
        this.previewTimer = null;
      }
    },
    cleanupPreviewTimers() {
      this.stopPreviewPlayback();
      if (this.previewRefreshTimer) {
        clearTimeout(this.previewRefreshTimer);
        this.previewRefreshTimer = null;
      }
    },
    initPreviewCanvas() {
      const systemInfo = uni.getSystemInfoSync();
      let statusBarHeight = 0;
      if (typeof systemInfo.statusBarHeight === "number") {
        statusBarHeight = systemInfo.statusBarHeight;
      }

      this.$nextTick(() => {
        setTimeout(() => {
          const query = uni.createSelectorQuery().in(this);
          query
            .select(".canvas-section")
            .boundingClientRect((sectionRect) => {
              if (!sectionRect || !sectionRect.height) {
                return;
              }

              const nextHeight =
                systemInfo.windowHeight -
                statusBarHeight -
                88 -
                sectionRect.height;
              this.contentHeight = `${Math.max(120, nextHeight)}px`;
            });
          query
            .select(".preview-canvas-container")
            .boundingClientRect((data) => {
              if (!data || !data.width) {
                this.previewCanvasReady = true;
                return;
              }

              const fitZoom = Math.max(2, Math.floor((data.width * 0.92) / 64));
              this.previewContainerSize = {
                width: data.width,
                height: data.width,
              };
              this.previewZoom = fitZoom;
              this.previewOffset = {
                x: (data.width - 64 * fitZoom) / 2,
                y: (data.width - 64 * fitZoom) / 2,
              };
              this.previewCanvasReady = true;
            })
            .exec();
        }, 80);
      });
    },
    getDraftKey() {
      if (this.featureType === "weather") {
        return WEATHER_DRAFT_KEY;
      }
      if (this.featureType === "countdown") {
        return COUNTDOWN_DRAFT_KEY;
      }
      if (this.featureType === "stopwatch") {
        return STOPWATCH_DRAFT_KEY;
      }
      if (this.featureType === "notification") {
        return NOTIFICATION_DRAFT_KEY;
      }
      return "";
    },
    loadDraft() {
      const draftKey = this.getDraftKey();
      if (draftKey.length === 0) {
        return;
      }

      const savedDraft = uni.getStorageSync(draftKey);
      if (!savedDraft || typeof savedDraft !== "object") {
        return;
      }

      if (this.featureType === "weather") {
        this.weatherConfig = savedDraft;
        return;
      }
      if (this.featureType === "countdown") {
        this.countdownConfig = savedDraft;
        return;
      }
      if (this.featureType === "stopwatch") {
        this.stopwatchConfig = savedDraft;
        return;
      }
      if (this.featureType === "notification") {
        this.notificationConfig = savedDraft;
      }
    },
    persistDraft() {
      const draftKey = this.getDraftKey();
      if (draftKey.length === 0) {
        return;
      }

      if (this.featureType === "weather") {
        uni.setStorageSync(draftKey, this.weatherConfig);
      }
      if (this.featureType === "countdown") {
        uni.setStorageSync(draftKey, this.countdownConfig);
      }
      if (this.featureType === "stopwatch") {
        uni.setStorageSync(draftKey, this.stopwatchConfig);
      }
      if (this.featureType === "notification") {
        uni.setStorageSync(draftKey, this.notificationConfig);
      }
    },
    buildFeaturePreviewFrames() {
      if (this.featureType === "weather") {
        return buildWeatherPreviewFrames(this.weatherConfig);
      }
      if (this.featureType === "countdown") {
        return buildCountdownPreviewFrames(this.countdownConfig);
      }
      if (this.featureType === "stopwatch") {
        return buildStopwatchPreviewFrames(this.stopwatchConfig);
      }
      if (this.featureType === "notification") {
        return buildNotificationPreviewFrames(this.notificationConfig);
      }
      return [{ pixels: new Map(), delay: 180 }];
    },
    buildFeatureFrames() {
      return pixelPreviewFramesToAnimationFrames(this.buildFeaturePreviewFrames(), 180);
    },
    async saveAndApply() {
      if (this.isSending) {
        return;
      }
      if (!this.deviceStore.connected) {
        this.toast.showError("设备未连接");
        return;
      }

      this.isSending = true;
      try {
        const ws = this.deviceStore.getWebSocket();
        const frames = this.buildFeatureFrames();
        await uploadAnimationFrames(ws, frames, this.featureType);
        this.persistDraft();
        this.deviceStore.setDeviceMode(this.featureType, { businessMode: true });
        this.toast.showSuccess("已保存并发送");
      } catch (err) {
        console.error("发送扩展功能失败:", err);
        this.toast.showError(`发送失败: ${err.message}`);
      } finally {
        this.isSending = false;
      }
    },
    applyWeatherPreset(preset) {
      this.weatherConfig = cloneConfig(preset.config);
      this.selectedWeatherPresetKey = preset.key;
    },
    applyCountdownPreset(preset) {
      this.countdownConfig = cloneConfig(preset.config);
      this.selectedCountdownPresetKey = preset.key;
    },
    applyStopwatchPreset(preset) {
      this.stopwatchConfig = cloneConfig(preset.config);
      this.selectedStopwatchPresetKey = preset.key;
    },
    applyNotificationPreset(preset) {
      this.notificationConfig = cloneConfig(preset.config);
      this.selectedNotificationPresetKey = preset.key;
    },
    handleWeatherTemperatureChange(event) {
      this.weatherConfig.temperature = Number(event.detail.value);
    },
    handleWeatherHumidityChange(event) {
      this.weatherConfig.humidity = Number(event.detail.value);
    },
    handleWeatherAccentColorChange(color) {
      this.weatherConfig.accentColor = color;
    },
    handleWeatherTempColorChange(color) {
      this.weatherConfig.tempColor = color;
    },
    handleWeatherHumidityColorChange(color) {
      this.weatherConfig.humidityColor = color;
    },
    handleCountdownHoursChange(event) {
      this.countdownConfig.hours = Number(event.detail.value);
    },
    handleCountdownMinutesChange(event) {
      this.countdownConfig.minutes = Number(event.detail.value);
    },
    handleCountdownSecondsChange(event) {
      this.countdownConfig.seconds = Number(event.detail.value);
    },
    handleCountdownProgressChange(event) {
      this.countdownConfig.progress = Number(event.detail.value) / 100;
    },
    handleCountdownAccentColorChange(color) {
      this.countdownConfig.accentColor = color;
    },
    handleStopwatchPreviewSecondsChange(event) {
      this.stopwatchConfig.previewSeconds = Number(event.detail.value) / 10;
    },
    handleStopwatchLapCountChange(event) {
      this.stopwatchConfig.lapCount = Number(event.detail.value);
    },
    handleStopwatchAccentColorChange(color) {
      this.stopwatchConfig.accentColor = color;
    },
    handleNotificationHourChange(event) {
      this.notificationConfig.hour = Number(event.detail.value);
    },
    handleNotificationMinuteChange(event) {
      this.notificationConfig.minute = Number(event.detail.value);
    },
    handleNotificationAccentColorChange(color) {
      this.notificationConfig.accentColor = color;
    },
    formatDuration(totalSeconds) {
      const safeTotal = Math.max(0, Number(totalSeconds) || 0);
      const hours = Math.floor(safeTotal / 3600);
      const minutes = Math.floor((safeTotal % 3600) / 60);
      const seconds = safeTotal % 60;

      if (hours > 0) {
        return `${String(hours).padStart(2, "0")}:${String(minutes).padStart(
          2,
          "0",
        )}:${String(seconds).padStart(2, "0")}`;
      }

      return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(
        2,
        "0",
      )}`;
    },
    findOptionLabel(options, value) {
      const target = options.find((item) => item.value === value);
      if (!target) {
        return "";
      }
      return target.label;
    },
  },
};
</script>

<style scoped>
.feature-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--bg-secondary);
  overflow: hidden;
}

.header {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background: var(--bg-elevated);
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
  flex-shrink: 0;
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.project-name {
  font-size: 33rpx;
  font-weight: 700;
  color: var(--text-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.action-btn-sm {
  width: auto;
  min-width: 88rpx;
  height: 72rpx;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  padding: 0 16rpx;
  border-radius: 14rpx;
  border: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
}

.action-btn-sm text {
  font-size: 22rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.action-btn-sm.primary {
  border-color: var(--accent-primary);
  background: var(--accent-primary);
}

.action-btn-sm.primary text {
  color: #ffffff;
}

.canvas-section {
  display: flex;
  flex-direction: column;
  background: #000000;
  border-bottom: 2rpx solid var(--border-primary);
  flex-shrink: 0;
}

.preview-canvas-container {
  width: 100%;
  aspect-ratio: 1;
  position: relative;
  overflow: hidden;
  background-color: #000000;
}

.preview-canvas-placeholder {
  background-color: #000000;
}

.preview-caption {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 14rpx 20rpx 18rpx;
  background: var(--bg-tertiary);
  border-bottom: 1rpx solid var(--border-color);
}

.preview-caption-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.preview-status-chip {
  display: inline-flex;
  width: fit-content;
  padding: 6rpx 12rpx;
  border-radius: 999rpx;
  background: rgba(255, 255, 255, 0.08);
}

.preview-status-chip.is-loading {
  background: rgba(255, 214, 102, 0.14);
}

.preview-status-chip.is-preview {
  background: rgba(79, 127, 255, 0.14);
}

.preview-status-chip.is-sending {
  background: rgba(52, 211, 153, 0.16);
}

.preview-status-chip-text {
  font-size: 20rpx;
  font-weight: 600;
  color: var(--text-secondary);
}

.preview-title {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.preview-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-actions {
  display: flex;
  align-items: center;
  gap: 12rpx;
  flex-shrink: 0;
}

.preview-actions .action-btn-sm {
  width: auto;
  min-width: 118rpx;
  height: 64rpx;
  padding: 0 18rpx;
  gap: 10rpx;
  border-radius: 18rpx;
}

.preview-actions .action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
  line-height: 1;
}

.preview-actions .action-btn-sm.disabled {
  opacity: 0.5;
  pointer-events: none;
}

.sending-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}

.sending-modal {
  min-width: 420rpx;
  padding: 60rpx 50rpx;
  border-radius: 24rpx;
  background: var(--bg-elevated);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
}

.sending-spinner {
  width: 60rpx;
  height: 60rpx;
  border-radius: 50%;
  border: 6rpx solid rgba(79, 127, 255, 0.2);
  border-top-color: var(--accent-primary);
  animation: spin 0.8s linear infinite;
}

.sending-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.sending-tip {
  font-size: 24rpx;
  color: var(--text-secondary);
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
  background: var(--bg-tertiary);
  padding: 20rpx;
}

.content-wrapper {
  padding-bottom: 48rpx;
}

.card {
  padding-top: 20rpx;
}

.card-title-section {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 8rpx;
  margin-bottom: 16rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.inline-value {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--accent-primary);
  font-family: monospace;
}

.form-row {
  margin-top: 18rpx;
}

.inline-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.form-label {
  display: block;
  font-size: 26rpx;
  color: var(--text-primary);
  font-weight: 500;
}

.text-input {
  margin-top: 10rpx;
  height: 72rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  padding: 0 20rpx;
  box-sizing: border-box;
  font-size: 26rpx;
  color: var(--text-primary);
}

.option-row {
  margin-top: 10rpx;
  display: flex;
  gap: 16rpx;
}

.option-row-wrap {
  flex-wrap: wrap;
}

.option-btn {
  flex: 1;
  min-width: 0;
  height: 72rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s;
}

.option-btn-small {
  flex: none;
  min-width: calc(25% - 12rpx);
  padding: 0 12rpx;
  box-sizing: border-box;
}

.option-btn text {
  font-size: 25rpx;
  color: var(--text-secondary);
}

.option-btn.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
  box-shadow: 0 8rpx 18rpx rgba(79, 127, 255, 0.12);
}

.option-btn.active text {
  color: var(--accent-primary);
  font-weight: 600;
}

.toggle-switch {
  cursor: pointer;
}

.switch-track {
  width: 88rpx;
  height: 48rpx;
  border-radius: 24rpx;
  background: var(--border-primary);
  display: flex;
  align-items: center;
  padding: 4rpx;
  transition: all 0.3s;
}

.switch-track.active {
  background: var(--accent-primary);
}

.switch-thumb {
  width: 40rpx;
  height: 40rpx;
  border-radius: 20rpx;
  background: #fff;
  transition: all 0.3s;
  box-shadow: 0 2rpx 8rpx rgba(0, 0, 0, 0.15);
}

.switch-track.active .switch-thumb {
  margin-left: 40rpx;
}

.piece-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14rpx;
}

.piece-item {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  gap: 10rpx;
  min-height: 164rpx;
  padding: 18rpx 16rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  box-sizing: border-box;
  transition: all 0.2s;
}

.piece-item.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
  box-shadow: 0 8rpx 18rpx rgba(79, 127, 255, 0.12);
}

.piece-top {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12rpx;
}

.piece-tag {
  padding: 4rpx 12rpx;
  border-radius: 999rpx;
  background: rgba(255, 255, 255, 0.08);
  color: var(--text-secondary);
  font-size: 20rpx;
  line-height: 1.2;
}

.piece-palette {
  display: flex;
  align-items: center;
  gap: 6rpx;
}

.piece-dot {
  width: 14rpx;
  height: 14rpx;
  border-radius: 50%;
  border: 2rpx solid rgba(255, 255, 255, 0.3);
}

.piece-name {
  font-size: 24rpx;
  color: var(--text-primary);
  font-weight: 600;
  text-align: left;
  line-height: 1.35;
}

.piece-hint {
  font-size: 20rpx;
  color: var(--text-secondary);
  line-height: 1.4;
}

.section-group {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.section-heading {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 16rpx;
}

.section-title {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.section-divider {
  height: 2rpx;
  background: var(--border-primary);
  margin: 8rpx 0 18rpx;
}

</style>
