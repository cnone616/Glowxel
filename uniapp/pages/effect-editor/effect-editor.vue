<template>
  <view class="effect-page">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <text class="nav-title">{{ pageTitle }}</text>
    </view>

    <scroll-view scroll-y class="content">
      <view class="card">
        <view class="card-title-section">
          <text class="card-title">连接状态</text>
          <text class="card-subtitle">{{
            deviceConnected ? "已连接" : "未连接"
          }}</text>
        </view>
        <text class="desc-text">{{
          deviceConnected ? "可直接保存并应用到设备" : "请先回设备页连接后再发送"
        }}</text>
      </view>

      <view v-if="effectType === 'text_display'" class="card">
        <view class="card-title-section">
          <text class="card-title">文本展示参数</text>
          <text class="card-subtitle">支持中文</text>
        </view>

        <view class="form-row">
          <text class="form-label">文本内容</text>
          <input
            v-model="textConfig.text"
            class="text-input"
            maxlength="20"
            placeholder="请输入文本"
          />
        </view>

        <view class="form-row">
          <text class="form-label">字体</text>
          <view class="option-row">
            <view
              v-for="item in textFontOptions"
              :key="item.value"
              class="option-btn"
              :class="{ active: textConfig.font === item.value }"
              @click="handleTextFontSelect(item.value)"
            >
              <text>{{ item.label }}</text>
            </view>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">方向</text>
          <view class="option-row">
            <view
              v-for="item in directionOptions"
              :key="item.value"
              class="option-btn"
              :class="{ active: textConfig.direction === item.value }"
              @click="handleTextDirectionSelect(item.value)"
            >
              <text>{{ item.label }}</text>
            </view>
          </view>
        </view>

        <view class="form-row inline-row">
          <text class="form-label">循环</text>
          <view class="toggle-switch" @click="handleTextLoopToggle">
            <view class="switch-track" :class="{ active: textConfig.loop }">
              <view class="switch-thumb"></view>
            </view>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">速度 {{ textConfig.speed }}</text>
          <slider
            :value="textConfig.speed"
            min="1"
            max="10"
            activeColor="#4F7FFF"
            @change="handleTextSpeedChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label">X 坐标 {{ textConfig.x }}</text>
          <slider
            :value="textConfig.x"
            min="-64"
            max="64"
            activeColor="#4F7FFF"
            @change="handleTextXChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label">Y 坐标 {{ textConfig.y }}</text>
          <slider
            :value="textConfig.y"
            min="0"
            max="64"
            activeColor="#4F7FFF"
            @change="handleTextYChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label">文字颜色</text>
          <view class="color-grid">
            <view
              v-for="item in colorOptions"
              :key="`text-${item.hex}`"
              class="color-item"
              :class="{ active: textConfig.color === item.hex }"
              @click="handleTextColorSelect(item.hex)"
            >
              <view
                class="color-dot"
                :style="{ backgroundColor: item.hex }"
              ></view>
              <text class="color-name">{{ item.name }}</text>
            </view>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">背景颜色</text>
          <view class="color-grid">
            <view
              v-for="item in bgColorOptions"
              :key="`bg-${item.hex}`"
              class="color-item"
              :class="{ active: textConfig.bgColor === item.hex }"
              @click="handleTextBgColorSelect(item.hex)"
            >
              <view
                class="color-dot"
                :style="{ backgroundColor: item.hex }"
              ></view>
              <text class="color-name">{{ item.name }}</text>
            </view>
          </view>
        </view>
      </view>

      <view v-if="effectType === 'breath_effect'" class="card">
        <view class="card-title-section">
          <text class="card-title">呼吸灯参数</text>
          <text class="card-subtitle">快速方式</text>
        </view>

        <view class="piece-grid">
          <view
            v-for="preset in breathPresets"
            :key="preset.key"
            class="piece-item"
            :class="{ active: selectedBreathPresetKey === preset.key }"
            @click="applyBreathPreset(preset)"
          >
            <text class="piece-name">{{ preset.label }}</text>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">波形</text>
          <view class="option-row">
            <view
              v-for="item in waveformOptions"
              :key="item.value"
              class="option-btn"
              :class="{ active: breathConfig.waveform === item.value }"
              @click="handleWaveformSelect(item.value)"
            >
              <text>{{ item.label }}</text>
            </view>
          </view>
        </view>

        <view class="form-row inline-row">
          <text class="form-label">循环</text>
          <view class="toggle-switch" @click="handleBreathLoopToggle">
            <view class="switch-track" :class="{ active: breathConfig.loop }">
              <view class="switch-thumb"></view>
            </view>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">速度 {{ breathConfig.speed }}</text>
          <slider
            :value="breathConfig.speed"
            min="1"
            max="10"
            activeColor="#4F7FFF"
            @change="handleBreathSpeedChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label"
            >最小亮度 {{ breathConfig.minBrightness }}%</text
          >
          <slider
            :value="breathConfig.minBrightness"
            min="0"
            max="100"
            activeColor="#4F7FFF"
            @change="handleBreathMinBrightnessChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label"
            >最大亮度 {{ breathConfig.maxBrightness }}%</text
          >
          <slider
            :value="breathConfig.maxBrightness"
            min="0"
            max="100"
            activeColor="#4F7FFF"
            @change="handleBreathMaxBrightnessChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label">周期 {{ breathConfig.periodMs }}ms</text>
          <slider
            :value="breathConfig.periodMs"
            min="200"
            max="10000"
            step="100"
            activeColor="#4F7FFF"
            @change="handleBreathPeriodChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label">颜色</text>
          <view class="color-grid">
            <view
              v-for="item in colorOptions"
              :key="`breath-${item.hex}`"
              class="color-item"
              :class="{ active: breathConfig.color === item.hex }"
              @click="handleBreathColorSelect(item.hex)"
            >
              <view
                class="color-dot"
                :style="{ backgroundColor: item.hex }"
              ></view>
              <text class="color-name">{{ item.name }}</text>
            </view>
          </view>
        </view>
      </view>

      <view v-if="effectType === 'rhythm_effect'" class="card">
        <view class="card-title-section">
          <text class="card-title">律动参数</text>
          <text class="card-subtitle">快速方式</text>
        </view>

        <view class="piece-grid">
          <view
            v-for="preset in rhythmPresets"
            :key="preset.key"
            class="piece-item"
            :class="{ active: selectedRhythmPresetKey === preset.key }"
            @click="applyRhythmPreset(preset)"
          >
            <text class="piece-name">{{ preset.label }}</text>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">模式</text>
          <view class="option-row">
            <view
              v-for="item in rhythmModeOptions"
              :key="item.value"
              class="option-btn"
              :class="{ active: rhythmConfig.mode === item.value }"
              @click="handleRhythmModeSelect(item.value)"
            >
              <text>{{ item.label }}</text>
            </view>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">方向</text>
          <view class="option-row">
            <view
              v-for="item in directionOptions"
              :key="item.value"
              class="option-btn"
              :class="{ active: rhythmConfig.direction === item.value }"
              @click="handleRhythmDirectionSelect(item.value)"
            >
              <text>{{ item.label }}</text>
            </view>
          </view>
        </view>

        <view class="form-row inline-row">
          <text class="form-label">循环</text>
          <view class="toggle-switch" @click="handleRhythmLoopToggle">
            <view class="switch-track" :class="{ active: rhythmConfig.loop }">
              <view class="switch-thumb"></view>
            </view>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">BPM {{ rhythmConfig.bpm }}</text>
          <slider
            :value="rhythmConfig.bpm"
            min="40"
            max="240"
            activeColor="#4F7FFF"
            @change="handleRhythmBpmChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label">速度 {{ rhythmConfig.speed }}</text>
          <slider
            :value="rhythmConfig.speed"
            min="1"
            max="10"
            activeColor="#4F7FFF"
            @change="handleRhythmSpeedChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label">强度 {{ rhythmConfig.strength }}%</text>
          <slider
            :value="rhythmConfig.strength"
            min="0"
            max="100"
            activeColor="#4F7FFF"
            @change="handleRhythmStrengthChange"
          />
        </view>

        <view class="form-row">
          <text class="form-label">主色</text>
          <view class="color-grid">
            <view
              v-for="item in colorOptions"
              :key="`rhythm-a-${item.hex}`"
              class="color-item"
              :class="{ active: rhythmConfig.colorA === item.hex }"
              @click="handleRhythmColorASelect(item.hex)"
            >
              <view
                class="color-dot"
                :style="{ backgroundColor: item.hex }"
              ></view>
              <text class="color-name">{{ item.name }}</text>
            </view>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">次色</text>
          <view class="color-grid">
            <view
              v-for="item in colorOptions"
              :key="`rhythm-b-${item.hex}`"
              class="color-item"
              :class="{ active: rhythmConfig.colorB === item.hex }"
              @click="handleRhythmColorBSelect(item.hex)"
            >
              <view
                class="color-dot"
                :style="{ backgroundColor: item.hex }"
              ></view>
              <text class="color-name">{{ item.name }}</text>
            </view>
          </view>
        </view>
      </view>

      <view class="save-section">
        <view class="save-btn" :class="{ disabled: isSending }" @click="saveAndApply">
          <text class="save-btn-text">{{
            isSending ? "发送中..." : "保存并应用"
          }}</text>
        </view>
      </view>
    </scroll-view>

    <canvas
      canvas-id="textRenderCanvas"
      id="textRenderCanvas"
      type="2d"
      style="position: fixed; left: -9999px; top: -9999px; width: 64px; height: 64px; opacity: 0"
    />

    <Toast ref="toastRef" />
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import { uploadAnimationFrames } from "../../utils/animationUploader.js";
import {
  buildBreathFrames,
  buildRhythmFrames,
  hexToRgb,
} from "../../utils/effectFrameBuilders.js";

const TEXT_CONFIG_KEY = "text_display_config";
const BREATH_CONFIG_KEY = "breath_effect_config";
const RHYTHM_CONFIG_KEY = "rhythm_effect_config";

export default {
  mixins: [statusBarMixin],
  components: { Icon, Toast },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isSending: false,
      effectType: "text_display",
      selectedBreathPresetKey: "",
      selectedRhythmPresetKey: "",
      textFontOptions: [
        { label: "无衬线", value: "sans" },
        { label: "等宽", value: "mono" },
        { label: "衬线", value: "serif" },
      ],
      directionOptions: [
        { label: "向左", value: "left" },
        { label: "向右", value: "right" },
        { label: "向上", value: "up" },
        { label: "向下", value: "down" },
      ],
      waveformOptions: [
        { label: "正弦", value: "sine" },
        { label: "三角", value: "triangle" },
        { label: "方波", value: "square" },
      ],
      rhythmModeOptions: [
        { label: "脉冲", value: "pulse" },
        { label: "渐变", value: "gradient" },
        { label: "跳变", value: "jump" },
      ],
      colorOptions: [
        { name: "青色", hex: "#64c8ff" },
        { name: "绿色", hex: "#00ff9d" },
        { name: "黄色", hex: "#ffdc00" },
        { name: "橙色", hex: "#ffa500" },
        { name: "红色", hex: "#ff6464" },
        { name: "紫色", hex: "#c864ff" },
        { name: "蓝色", hex: "#6464ff" },
        { name: "白色", hex: "#ffffff" },
      ],
      bgColorOptions: [
        { name: "黑色", hex: "#000000" },
        { name: "深灰", hex: "#1a1a1a" },
        { name: "灰色", hex: "#333333" },
        { name: "深蓝", hex: "#102038" },
      ],
      breathPresets: [
        {
          key: "soft_wave",
          label: "柔和呼吸",
          config: {
            speed: 3,
            loop: true,
            minBrightness: 8,
            maxBrightness: 65,
            periodMs: 2600,
            waveform: "sine",
            color: "#64c8ff",
          },
        },
        {
          key: "energy_blink",
          label: "能量脉冲",
          config: {
            speed: 8,
            loop: true,
            minBrightness: 25,
            maxBrightness: 100,
            periodMs: 900,
            waveform: "square",
            color: "#00ff9d",
          },
        },
        {
          key: "warm_ambient",
          label: "暖光氛围",
          config: {
            speed: 4,
            loop: true,
            minBrightness: 6,
            maxBrightness: 55,
            periodMs: 3200,
            waveform: "triangle",
            color: "#ffa500",
          },
        },
        {
          key: "night_guard",
          label: "夜间守护",
          config: {
            speed: 2,
            loop: true,
            minBrightness: 3,
            maxBrightness: 30,
            periodMs: 4200,
            waveform: "sine",
            color: "#6464ff",
          },
        },
        {
          key: "warning_pulse",
          label: "警示呼吸",
          config: {
            speed: 7,
            loop: true,
            minBrightness: 15,
            maxBrightness: 95,
            periodMs: 1200,
            waveform: "triangle",
            color: "#ff6464",
          },
        },
        {
          key: "single_breath",
          label: "单次呼吸",
          config: {
            speed: 5,
            loop: false,
            minBrightness: 10,
            maxBrightness: 90,
            periodMs: 1500,
            waveform: "sine",
            color: "#ffffff",
          },
        },
      ],
      rhythmPresets: [
        {
          key: "left_wave",
          label: "左向波浪",
          config: {
            bpm: 110,
            speed: 5,
            loop: true,
            direction: "left",
            strength: 70,
            mode: "pulse",
            colorA: "#64c8ff",
            colorB: "#c864ff",
          },
        },
        {
          key: "right_sweep",
          label: "右扫渐变",
          config: {
            bpm: 96,
            speed: 4,
            loop: true,
            direction: "right",
            strength: 75,
            mode: "gradient",
            colorA: "#00ff9d",
            colorB: "#64c8ff",
          },
        },
        {
          key: "top_drop",
          label: "上落脉冲",
          config: {
            bpm: 132,
            speed: 6,
            loop: true,
            direction: "up",
            strength: 80,
            mode: "pulse",
            colorA: "#ffdc00",
            colorB: "#ff6464",
          },
        },
        {
          key: "bottom_jump",
          label: "下冲跳变",
          config: {
            bpm: 148,
            speed: 7,
            loop: true,
            direction: "down",
            strength: 90,
            mode: "jump",
            colorA: "#ff6464",
            colorB: "#ffffff",
          },
        },
        {
          key: "calm_flow",
          label: "平静流光",
          config: {
            bpm: 78,
            speed: 3,
            loop: true,
            direction: "left",
            strength: 50,
            mode: "gradient",
            colorA: "#6464ff",
            colorB: "#64c8ff",
          },
        },
        {
          key: "single_hit",
          label: "单次节拍",
          config: {
            bpm: 120,
            speed: 5,
            loop: false,
            direction: "right",
            strength: 85,
            mode: "jump",
            colorA: "#ffffff",
            colorB: "#ff6464",
          },
        },
      ],
      textConfig: {
        text: "你好 Glowxel",
        font: "sans",
        x: 64,
        y: 34,
        speed: 3,
        direction: "left",
        loop: true,
        color: "#64c8ff",
        bgColor: "#000000",
      },
      breathConfig: {
        speed: 5,
        loop: true,
        minBrightness: 10,
        maxBrightness: 100,
        periodMs: 1800,
        waveform: "sine",
        color: "#64c8ff",
      },
      rhythmConfig: {
        bpm: 120,
        speed: 5,
        loop: true,
        direction: "left",
        strength: 70,
        mode: "pulse",
        colorA: "#64c8ff",
        colorB: "#ff6464",
      },
    };
  },
  computed: {
    deviceConnected() {
      return this.deviceStore && this.deviceStore.connected;
    },
    pageTitle() {
      if (this.effectType === "text_display") {
        return "文本展示";
      }
      if (this.effectType === "breath_effect") {
        return "呼吸灯";
      }
      return "律动";
    },
  },
  onLoad(options) {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();

    if (
      options &&
      (options.mode === "text_display" ||
        options.mode === "breath_effect" ||
        options.mode === "rhythm_effect")
    ) {
      this.effectType = options.mode;
    }

    this.loadSavedConfig();
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
  },
  methods: {
    handleBack() {
      uni.navigateBack();
    },

    handleTextFontSelect(value) {
      this.textConfig.font = value;
    },
    handleTextDirectionSelect(value) {
      this.textConfig.direction = value;
    },
    handleTextLoopToggle() {
      this.textConfig.loop = !this.textConfig.loop;
    },
    handleTextSpeedChange(e) {
      this.textConfig.speed = Number(e.detail.value);
    },
    handleTextXChange(e) {
      this.textConfig.x = Number(e.detail.value);
    },
    handleTextYChange(e) {
      this.textConfig.y = Number(e.detail.value);
    },
    handleTextColorSelect(hex) {
      this.textConfig.color = hex;
    },
    handleTextBgColorSelect(hex) {
      this.textConfig.bgColor = hex;
    },

    handleWaveformSelect(value) {
      this.breathConfig.waveform = value;
      this.selectedBreathPresetKey = "";
    },
    handleBreathLoopToggle() {
      this.breathConfig.loop = !this.breathConfig.loop;
      this.selectedBreathPresetKey = "";
    },
    handleBreathSpeedChange(e) {
      this.breathConfig.speed = Number(e.detail.value);
      this.selectedBreathPresetKey = "";
    },
    handleBreathMinBrightnessChange(e) {
      this.breathConfig.minBrightness = Number(e.detail.value);
      this.selectedBreathPresetKey = "";
    },
    handleBreathMaxBrightnessChange(e) {
      this.breathConfig.maxBrightness = Number(e.detail.value);
      this.selectedBreathPresetKey = "";
    },
    handleBreathPeriodChange(e) {
      this.breathConfig.periodMs = Number(e.detail.value);
      this.selectedBreathPresetKey = "";
    },
    handleBreathColorSelect(hex) {
      this.breathConfig.color = hex;
      this.selectedBreathPresetKey = "";
    },

    handleRhythmModeSelect(value) {
      this.rhythmConfig.mode = value;
      this.selectedRhythmPresetKey = "";
    },
    handleRhythmDirectionSelect(value) {
      this.rhythmConfig.direction = value;
      this.selectedRhythmPresetKey = "";
    },
    handleRhythmLoopToggle() {
      this.rhythmConfig.loop = !this.rhythmConfig.loop;
      this.selectedRhythmPresetKey = "";
    },
    handleRhythmBpmChange(e) {
      this.rhythmConfig.bpm = Number(e.detail.value);
      this.selectedRhythmPresetKey = "";
    },
    handleRhythmSpeedChange(e) {
      this.rhythmConfig.speed = Number(e.detail.value);
      this.selectedRhythmPresetKey = "";
    },
    handleRhythmStrengthChange(e) {
      this.rhythmConfig.strength = Number(e.detail.value);
      this.selectedRhythmPresetKey = "";
    },
    handleRhythmColorASelect(hex) {
      this.rhythmConfig.colorA = hex;
      this.selectedRhythmPresetKey = "";
    },
    handleRhythmColorBSelect(hex) {
      this.rhythmConfig.colorB = hex;
      this.selectedRhythmPresetKey = "";
    },

    loadSavedConfig() {
      const textSaved = uni.getStorageSync(TEXT_CONFIG_KEY);
      if (
        textSaved &&
        typeof textSaved === "object" &&
        this.isValidTextConfig(textSaved)
      ) {
        this.textConfig = textSaved;
      }

      const breathSaved = uni.getStorageSync(BREATH_CONFIG_KEY);
      if (
        breathSaved &&
        typeof breathSaved === "object" &&
        this.isValidBreathConfig(breathSaved)
      ) {
        this.breathConfig = breathSaved;
      }

      const rhythmSaved = uni.getStorageSync(RHYTHM_CONFIG_KEY);
      if (
        rhythmSaved &&
        typeof rhythmSaved === "object" &&
        this.isValidRhythmConfig(rhythmSaved)
      ) {
        this.rhythmConfig = rhythmSaved;
      }
    },

    isValidTextConfig(cfg) {
      return (
        typeof cfg.text === "string" &&
        cfg.text.length > 0 &&
        (cfg.font === "sans" || cfg.font === "mono" || cfg.font === "serif") &&
        Number.isFinite(Number(cfg.x)) &&
        Number.isFinite(Number(cfg.y)) &&
        Number.isFinite(Number(cfg.speed)) &&
        (cfg.direction === "left" ||
          cfg.direction === "right" ||
          cfg.direction === "up" ||
          cfg.direction === "down") &&
        typeof cfg.loop === "boolean" &&
        typeof cfg.color === "string" &&
        typeof cfg.bgColor === "string"
      );
    },
    isValidBreathConfig(cfg) {
      return (
        Number.isFinite(Number(cfg.speed)) &&
        typeof cfg.loop === "boolean" &&
        Number.isFinite(Number(cfg.minBrightness)) &&
        Number.isFinite(Number(cfg.maxBrightness)) &&
        Number.isFinite(Number(cfg.periodMs)) &&
        (cfg.waveform === "sine" ||
          cfg.waveform === "triangle" ||
          cfg.waveform === "square") &&
        typeof cfg.color === "string"
      );
    },
    isValidRhythmConfig(cfg) {
      return (
        Number.isFinite(Number(cfg.bpm)) &&
        Number.isFinite(Number(cfg.speed)) &&
        typeof cfg.loop === "boolean" &&
        (cfg.direction === "left" ||
          cfg.direction === "right" ||
          cfg.direction === "up" ||
          cfg.direction === "down") &&
        Number.isFinite(Number(cfg.strength)) &&
        (cfg.mode === "pulse" ||
          cfg.mode === "gradient" ||
          cfg.mode === "jump") &&
        typeof cfg.colorA === "string" &&
        typeof cfg.colorB === "string"
      );
    },

    saveConfig() {
      uni.setStorageSync(TEXT_CONFIG_KEY, this.textConfig);
      uni.setStorageSync(BREATH_CONFIG_KEY, this.breathConfig);
      uni.setStorageSync(RHYTHM_CONFIG_KEY, this.rhythmConfig);
    },

    applyBreathPreset(preset) {
      if (!preset || !preset.config) {
        return;
      }
      this.breathConfig = { ...preset.config };
      this.selectedBreathPresetKey = preset.key;
    },
    applyRhythmPreset(preset) {
      if (!preset || !preset.config) {
        return;
      }
      this.rhythmConfig = { ...preset.config };
      this.selectedRhythmPresetKey = preset.key;
    },

    ensureColorHex(hex) {
      const color = hexToRgb(hex);
      return `#${color.r.toString(16).padStart(2, "0")}${color.g
        .toString(16)
        .padStart(2, "0")}${color.b.toString(16).padStart(2, "0")}`;
    },

    async renderTextFrameToPixels(cfg, drawX, drawY) {
      const ctx = uni.createCanvasContext("textRenderCanvas", this);
      const bg = this.ensureColorHex(cfg.bgColor);
      const fg = this.ensureColorHex(cfg.color);

      ctx.setFillStyle(bg);
      ctx.fillRect(0, 0, 64, 64);
      ctx.setFillStyle(fg);

      if (cfg.font === "mono") {
        ctx.setFontSize(15);
      } else if (cfg.font === "serif") {
        ctx.setFontSize(16);
      } else {
        ctx.setFontSize(16);
      }

      if (ctx.setTextBaseline) {
        ctx.setTextBaseline("middle");
      }

      ctx.fillText(cfg.text, drawX, drawY);

      await new Promise((resolve) => {
        ctx.draw(false, () => {
          setTimeout(resolve, 20);
        });
      });

      const imageData = await new Promise((resolve, reject) => {
        uni.canvasGetImageData({
          canvasId: "textRenderCanvas",
          x: 0,
          y: 0,
          width: 64,
          height: 64,
          success: resolve,
          fail: reject,
        });
      });

      const bgRgb = hexToRgb(bg);
      const includeAll = bg !== "#000000";
      const bytes = [];
      const rgba = imageData.data;
      let p = 0;

      for (let y = 0; y < 64; y++) {
        for (let x = 0; x < 64; x++) {
          const srcR = rgba[p];
          const srcG = rgba[p + 1];
          const srcB = rgba[p + 2];
          const srcA = rgba[p + 3] / 255;
          p += 4;

          const r = Math.round(srcR * srcA + bgRgb.r * (1 - srcA));
          const g = Math.round(srcG * srcA + bgRgb.g * (1 - srcA));
          const b = Math.round(srcB * srcA + bgRgb.b * (1 - srcA));

          if (includeAll || r > 0 || g > 0 || b > 0) {
            bytes.push(x, y, r, g, b);
          }
        }
      }

      const pixelBytes = new Uint8Array(bytes);
      return {
        type: 1,
        delay: Math.max(40, 220 - Number(cfg.speed) * 15),
        totalPixels: pixelBytes.length / 5,
        pixels: pixelBytes,
      };
    },

    async buildTextFrames() {
      const cfg = { ...this.textConfig };
      cfg.color = this.ensureColorHex(cfg.color);
      cfg.bgColor = this.ensureColorHex(cfg.bgColor);

      if (typeof cfg.text !== "string" || cfg.text.length === 0) {
        throw new Error("文本内容不能为空");
      }

      const frameCount = cfg.loop ? 20 : 1;
      const step = Number(cfg.speed) * 2;
      const frames = [];

      for (let i = 0; i < frameCount; i++) {
        const offset = i * step;
        let drawX = Number(cfg.x);
        let drawY = Number(cfg.y);

        if (cfg.direction === "left") {
          drawX = Number(cfg.x) - offset;
        } else if (cfg.direction === "right") {
          drawX = Number(cfg.x) + offset;
        } else if (cfg.direction === "up") {
          drawY = Number(cfg.y) - offset;
        } else if (cfg.direction === "down") {
          drawY = Number(cfg.y) + offset;
        } else {
          throw new Error("文本方向参数错误");
        }

        const frame = await this.renderTextFrameToPixels(cfg, drawX, drawY);
        frames.push(frame);
      }

      return frames;
    },

    async buildFrames() {
      if (this.effectType === "text_display") {
        return this.buildTextFrames();
      }
      if (this.effectType === "breath_effect") {
        this.breathConfig.color = this.ensureColorHex(this.breathConfig.color);
        return buildBreathFrames(this.breathConfig);
      }
      if (this.effectType === "rhythm_effect") {
        this.rhythmConfig.colorA = this.ensureColorHex(this.rhythmConfig.colorA);
        this.rhythmConfig.colorB = this.ensureColorHex(this.rhythmConfig.colorB);
        return buildRhythmFrames(this.rhythmConfig);
      }
      throw new Error("未知效果类型");
    },

    async saveAndApply() {
      if (this.isSending) {
        return;
      }
      if (!this.deviceConnected) {
        this.toast.showError("设备未连接");
        return;
      }

      this.isSending = true;
      try {
        const ws = this.deviceStore.getWebSocket();
        if (this.effectType === "breath_effect") {
          const breathColor = hexToRgb(
            this.ensureColorHex(this.breathConfig.color),
          );
          try {
            await ws.send({
              cmd: "set_breath_effect",
              speed: Number(this.breathConfig.speed),
              loop: this.breathConfig.loop,
              minBrightness: Number(this.breathConfig.minBrightness),
              maxBrightness: Number(this.breathConfig.maxBrightness),
              periodMs: Number(this.breathConfig.periodMs),
              waveform: this.breathConfig.waveform,
              color: {
                r: breathColor.r,
                g: breathColor.g,
                b: breathColor.b,
              },
            });
          } catch (nativeErr) {
            const frames = await this.buildFrames();
            await uploadAnimationFrames(ws, frames, this.effectType);
          }
        } else if (this.effectType === "rhythm_effect") {
          const colorA = hexToRgb(this.ensureColorHex(this.rhythmConfig.colorA));
          const colorB = hexToRgb(this.ensureColorHex(this.rhythmConfig.colorB));
          try {
            await ws.send({
              cmd: "set_rhythm_effect",
              bpm: Number(this.rhythmConfig.bpm),
              speed: Number(this.rhythmConfig.speed),
              loop: this.rhythmConfig.loop,
              direction: this.rhythmConfig.direction,
              strength: Number(this.rhythmConfig.strength),
              mode: this.rhythmConfig.mode,
              colorA: {
                r: colorA.r,
                g: colorA.g,
                b: colorA.b,
              },
              colorB: {
                r: colorB.r,
                g: colorB.g,
                b: colorB.b,
              },
            });
          } catch (nativeErr) {
            const frames = await this.buildFrames();
            await uploadAnimationFrames(ws, frames, this.effectType);
          }
        } else {
          const frames = await this.buildFrames();
          await uploadAnimationFrames(ws, frames, this.effectType);
        }

        this.saveConfig();
        this.deviceStore.setDeviceMode(this.effectType, { businessMode: true });
        this.toast.showSuccess("已保存并应用");
      } catch (err) {
        console.error("应用效果失败:", err);
        this.toast.showError(`发送失败: ${err.message}`);
      } finally {
        this.isSending = false;
      }
    },
  },
};
</script>

<style scoped>
.effect-page {
  min-height: 100vh;
  background-color: #f5f5f7;
}

.status-bar {
  background-color: #1a1a1a;
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

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.content {
  padding: 24rpx;
}

.card {
  background: #fff;
  border-radius: 24rpx;
  padding: 28rpx;
  margin-bottom: 24rpx;
}

.card-title-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 20rpx;
}

.card-title {
  font-size: 30rpx;
  font-weight: 600;
  color: #1a1a1a;
}

.card-subtitle {
  font-size: 24rpx;
  color: #999;
}

.desc-text {
  font-size: 24rpx;
  color: #666;
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
  color: #1a1a1a;
  font-weight: 500;
}

.text-input {
  margin-top: 10rpx;
  height: 72rpx;
  border-radius: 16rpx;
  background: #f5f5f7;
  padding: 0 20rpx;
  box-sizing: border-box;
  font-size: 26rpx;
  color: #1a1a1a;
}

.option-row {
  margin-top: 10rpx;
  display: flex;
  gap: 16rpx;
}

.option-btn {
  flex: 1;
  height: 72rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  background: #f5f5f7;
  transition: all 0.2s;
}

.option-btn text {
  font-size: 25rpx;
  color: #666;
}

.option-btn.active {
  background: #eef2ff;
  border: 2rpx solid #4f7fff;
}

.option-btn.active text {
  color: #4f7fff;
  font-weight: 600;
}

.option-btn:active {
  transform: scale(0.96);
}

.toggle-switch {
  cursor: pointer;
}

.switch-track {
  width: 88rpx;
  height: 48rpx;
  border-radius: 24rpx;
  background: #ddd;
  display: flex;
  align-items: center;
  padding: 4rpx;
  transition: all 0.3s;
}

.switch-track.active {
  background: #4f7fff;
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
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
}

.piece-item {
  width: calc(33.33% - 11rpx);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 18rpx 0;
  border-radius: 16rpx;
  background: #f5f5f7;
  transition: all 0.2s;
}

.piece-item.active {
  background: #eef2ff;
  border: 2rpx solid #4f7fff;
}

.piece-item:active {
  transform: scale(0.96);
}

.piece-name {
  font-size: 24rpx;
  color: #666;
  font-weight: 600;
  text-align: center;
}

.piece-item.active .piece-name {
  color: #4f7fff;
}

.color-grid {
  margin-top: 10rpx;
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.color-item {
  width: calc(25% - 9rpx);
  background: #f5f5f7;
  border-radius: 14rpx;
  padding: 14rpx 8rpx 12rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  border: 2rpx solid transparent;
}

.color-item.active {
  border-color: #4f7fff;
  background: #eef2ff;
}

.color-dot {
  width: 42rpx;
  height: 42rpx;
  border-radius: 10rpx;
  border: 2rpx solid rgba(0, 0, 0, 0.08);
}

.color-name {
  font-size: 22rpx;
  color: #666;
}

.color-item.active .color-name {
  color: #4f7fff;
  font-weight: 600;
}

.save-section {
  padding: 20rpx 0 60rpx;
}

.save-btn {
  height: 88rpx;
  background: #4f7fff;
  border-radius: 20rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.save-btn.disabled {
  opacity: 0.6;
}

.save-btn:active {
  opacity: 0.85;
}

.save-btn-text {
  font-size: 32rpx;
  font-weight: 600;
  color: #fff;
}
</style>
