<template>
  <view class="effect-page">
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
          v-if="previewCanvasReady"
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
          canvas-id="effectPreviewCanvas"
        />
      </view>
      <view
        v-else
        class="preview-canvas-container preview-canvas-placeholder"
      ></view>
      <view class="preview-caption">
        <view class="preview-caption-info">
          <view class="preview-status-chip" :class="previewStatusClass">
            <text class="preview-status-chip-text">{{ previewStatusLabel }}</text>
          </view>
          <text class="preview-title">64 x 64 模拟预览</text>
          <text class="preview-subtitle">{{ previewSubtitle }}</text>
        </view>
        <view class="preview-actions">
          <view
            class="action-btn-sm"
            :class="{ disabled: isSending }"
            @click="saveDraft"
          >
            <Icon name="save" :size="36" color="var(--color-text-primary)" />
            <text>保存</text>
          </view>
          <view
            class="action-btn-sm primary"
            :class="{ disabled: isSending }"
            @click="saveAndApply"
          >
            <Icon name="link" :size="36" color="#fff" />
            <text>{{ isSending ? "发送中" : "发送" }}</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
      <view v-if="effectType === 'text_display'" class="card">
        <view class="card-title-section">
          <text class="card-title">像素信息屏</text>
          <text class="card-subtitle">参考 AWTRIX 与 HUB75 信息屏的横幅展示</text>
        </view>

        <view class="piece-grid">
          <view
            v-for="preset in textPresets"
            :key="preset.key"
            class="piece-item"
            :class="{ active: selectedTextPresetKey === preset.key }"
            @click="applyTextPreset(preset)"
          >
            <text class="piece-name">{{ preset.label }}</text>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">文本内容</text>
          <input
            v-model="textConfig.text"
            class="text-input"
            maxlength="20"
            placeholder="请输入文本"
            @input="handleTextContentInput"
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
          <ColorPanelPicker
            :value="textConfig.color"
            label="文字颜色"
            :preset-colors="colorOptions"
            @input="handleTextColorSelect"
          />
        </view>

        <view class="form-row">
          <text class="form-label">背景颜色</text>
          <ColorPanelPicker
            :value="textConfig.bgColor"
            label="背景颜色"
            :preset-colors="bgColorOptions"
            @input="handleTextBgColorSelect"
          />
        </view>
      </view>

      <view v-if="effectType === 'breath_effect'" class="card">
        <view class="card-title-section">
          <text class="card-title">矩阵流光</text>
          <text class="card-subtitle">参考 WLED 边缘追光与矩阵彗尾流光</text>
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
          <text class="form-label">运行方式</text>
          <view class="option-row option-row-wrap">
            <view
              v-for="item in ringMotionOptions"
              :key="item.value"
              class="option-btn option-btn-small"
              :class="{ active: breathConfig.motion === item.value }"
              @click="handleBreathMotionSelect(item.value)"
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
          <text class="form-label">显示范围</text>
          <view class="option-row">
            <view
              v-for="item in ringScopeOptions"
              :key="item.value"
              class="option-btn"
              :class="{ active: breathConfig.scope === item.value }"
              @click="handleBreathScopeSelect(item.value)"
            >
              <text>{{ item.label }}</text>
            </view>
          </view>
        </view>

        <view class="form-row">
          <text class="form-label">颜色模式</text>
          <view class="option-row">
            <view
              v-for="item in ringColorModeOptions"
              :key="item.value"
              class="option-btn"
              :class="{ active: breathConfig.colorMode === item.value }"
              @click="handleBreathColorModeSelect(item.value)"
            >
              <text>{{ item.label }}</text>
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
          <text class="form-label">主色</text>
          <ColorPanelPicker
            :value="breathConfig.colorA"
            label="流光主色"
            :preset-colors="colorOptions"
            @input="handleBreathColorSelect"
          />
        </view>

        <view
          v-if="breathConfig.colorMode === 'gradient'"
          class="form-row"
        >
          <text class="form-label">渐变次色</text>
          <ColorPanelPicker
            :value="breathConfig.colorB"
            label="流光次色"
            :preset-colors="colorOptions"
            @input="handleBreathColorBSelect"
          />
        </view>
      </view>

      <view v-if="effectType === 'rhythm_effect'" class="card">
        <view class="card-title-section">
          <text class="card-title">音频频谱</text>
          <text class="card-subtitle">参考 ESP32_FFT_VU 的柱阵频谱和峰值保持</text>
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
          <ColorPanelPicker
            :value="rhythmConfig.colorA"
            label="频谱主色"
            :preset-colors="colorOptions"
            @input="handleRhythmColorASelect"
          />
        </view>

        <view class="form-row">
          <text class="form-label">次色</text>
          <ColorPanelPicker
            :value="rhythmConfig.colorB"
            label="频谱次色"
            :preset-colors="colorOptions"
            @input="handleRhythmColorBSelect"
          />
        </view>
      </view>
      </view>
    </scroll-view>

    <canvas
      canvas-id="textRenderCanvas"
      id="textRenderCanvas"
      type="2d"
      style="position: fixed; left: -9999px; top: -9999px; width: 256px; height: 256px; opacity: 0"
    />

    <view v-if="isSending" class="sending-overlay" @touchmove.stop.prevent>
      <view class="sending-modal">
        <view class="sending-spinner"></view>
        <text class="sending-title">正在发送效果...</text>
        <text class="sending-tip">设备应用完成后会自动恢复当前预览</text>
      </view>
    </view>

    <Toast ref="toastRef" @show="previewHidden = true" @hide="onToastHide" />
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import ColorPanelPicker from "../../components/ColorPanelPicker.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import { uploadAnimationFrames } from "../../utils/animationUploader.js";
import {
  drawTextToPixels,
  drawTinyTextToPixels,
  getTextWidth,
  getTinyTextWidth,
} from "../../utils/clockCanvas.js";
import {
  buildBreathFrames,
  buildRhythmFrames,
  hexToRgb,
} from "../../utils/effectFrameBuilders.js";

const TEXT_CONFIG_KEY = "text_display_config";
const BREATH_CONFIG_KEY = "breath_effect_config";
const RHYTHM_CONFIG_KEY = "rhythm_effect_config";

function fillPreviewRect(pixelMap, x, y, width, height, color) {
  for (let row = 0; row < height; row++) {
    for (let col = 0; col < width; col++) {
      const px = x + col;
      const py = y + row;
      if (px < 0 || px >= 64 || py < 0 || py >= 64) {
        continue;
      }
      pixelMap.set(`${px},${py}`, color);
    }
  }
}

function rgbToHexString(r, g, b) {
  return `#${r.toString(16).padStart(2, "0")}${g
    .toString(16)
    .padStart(2, "0")}${b.toString(16).padStart(2, "0")}`;
}

function blendHexColor(colorA, colorB, alpha) {
  const rgbA = hexToRgb(colorA);
  const rgbB = hexToRgb(colorB);
  const safeAlpha = Math.max(0, Math.min(1, alpha));
  return rgbToHexString(
    Math.round(rgbA.r * (1 - safeAlpha) + rgbB.r * safeAlpha),
    Math.round(rgbA.g * (1 - safeAlpha) + rgbB.g * safeAlpha),
    Math.round(rgbA.b * (1 - safeAlpha) + rgbB.b * safeAlpha),
  );
}

function addTextEffectDecor(pixelMap, cfg, accentColor) {
  const edgeColor = blendHexColor(cfg.bgColor, accentColor, 0.24);
  const frameColor = blendHexColor(cfg.bgColor, accentColor, 0.18);
  const chipColor = blendHexColor(cfg.bgColor, accentColor, 0.28);
  const panelColor = blendHexColor(cfg.bgColor, "#ffffff", 0.06);
  fillPreviewRect(pixelMap, 0, 0, 64, 64, cfg.bgColor);
  fillPreviewRect(pixelMap, 0, 0, 64, 1, edgeColor);
  fillPreviewRect(pixelMap, 0, 63, 64, 1, edgeColor);
  fillPreviewRect(pixelMap, 0, 0, 1, 64, frameColor);
  fillPreviewRect(pixelMap, 63, 0, 1, 64, frameColor);
  fillPreviewRect(pixelMap, 3, 3, 58, 8, panelColor);
  fillPreviewRect(pixelMap, 4, 13, 56, 33, panelColor);
  fillPreviewRect(pixelMap, 4, 49, 56, 11, panelColor);
  fillPreviewRect(pixelMap, 5, 4, 14, 6, chipColor);
  fillPreviewRect(pixelMap, 47, 4, 10, 6, chipColor);
  fillPreviewRect(pixelMap, 6, 58, 52, 1, edgeColor);
}

function normalizeDisplayText(text) {
  if (typeof text !== "string") {
    return "";
  }
  return text.toUpperCase().replace(/\s+/g, " ").trim();
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
      deviceStore: null,
      toast: null,
      isSending: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      effectType: "text_display",
      selectedTextPresetKey: "",
      selectedBreathPresetKey: "",
      selectedRhythmPresetKey: "",
      previewFrameMaps: [],
      previewFrameDelays: [],
      previewFrameIndex: 0,
      previewTimer: null,
      previewRefreshTimer: null,
      previewRefreshToken: 0,
      previewCanvasReady: false,
      previewHidden: false,
      previewZoom: 4,
      previewOffset: { x: 16, y: 16 },
      previewContainerSize: { width: 320, height: 320 },
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
      ringMotionOptions: [
        { label: "顺时针", value: "clockwise" },
        { label: "逆时针", value: "counterclockwise" },
        { label: "外向内", value: "inward" },
        { label: "内向外", value: "outward" },
      ],
      ringScopeOptions: [
        { label: "一圈", value: "single_ring" },
        { label: "全屏", value: "full_screen" },
      ],
      ringColorModeOptions: [
        { label: "纯色", value: "solid" },
        { label: "渐变", value: "gradient" },
      ],
      rhythmModeOptions: [
        { label: "柱阵", value: "pulse" },
        { label: "瀑布", value: "gradient" },
        { label: "爆闪", value: "jump" },
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
      textPresets: [
        {
          key: "neon_banner",
          label: "AWTRIX 横幅",
          config: {
            text: "HELLO GLOWXEL",
            font: "sans",
            x: 64,
            y: 34,
            speed: 4,
            direction: "left",
            loop: true,
            color: "#64c8ff",
            bgColor: "#000000",
          },
        },
        {
          key: "terminal_feed",
          label: "HUB75 播报",
          config: {
            text: "SYSTEM ONLINE",
            font: "mono",
            x: 64,
            y: 30,
            speed: 3,
            direction: "left",
            loop: true,
            color: "#00ff9d",
            bgColor: "#000000",
          },
        },
        {
          key: "warning_sign",
          label: "状态告警牌",
          config: {
            text: "WARNING",
            font: "serif",
            x: 32,
            y: 34,
            speed: 2,
            direction: "up",
            loop: true,
            color: "#ffdc00",
            bgColor: "#1a1a1a",
          },
        },
        {
          key: "vertical_drop",
          label: "纵向弹幕",
          config: {
            text: "PIXEL FLOW",
            font: "mono",
            x: 12,
            y: 64,
            speed: 4,
            direction: "up",
            loop: true,
            color: "#c864ff",
            bgColor: "#000000",
          },
        },
        {
          key: "soft_board",
          label: "夜间信息牌",
          config: {
            text: "GOOD NIGHT",
            font: "serif",
            x: 64,
            y: 34,
            speed: 2,
            direction: "right",
            loop: true,
            color: "#ffffff",
            bgColor: "#102038",
          },
        },
      ],
      breathPresets: [
        {
          key: "clockwise_single",
          label: "WLED 单环",
          config: {
            speed: 5,
            loop: true,
            motion: "clockwise",
            scope: "single_ring",
            colorMode: "solid",
            colorA: "#64c8ff",
            colorB: "#64c8ff",
          },
        },
        {
          key: "neon_comet",
          label: "WLED 彗尾",
          config: {
            speed: 8,
            loop: true,
            motion: "clockwise",
            scope: "single_ring",
            colorMode: "gradient",
            colorA: "#64c8ff",
            colorB: "#c864ff",
          },
        },
        {
          key: "counter_gradient",
          label: "反向流光",
          config: {
            speed: 6,
            loop: true,
            motion: "counterclockwise",
            scope: "single_ring",
            colorMode: "gradient",
            colorA: "#00ff9d",
            colorB: "#64c8ff",
          },
        },
        {
          key: "aurora_tide",
          label: "满屏流光",
          config: {
            speed: 5,
            loop: true,
            motion: "counterclockwise",
            scope: "full_screen",
            colorMode: "gradient",
            colorA: "#00ff9d",
            colorB: "#64c8ff",
          },
        },
        {
          key: "inward_single",
          label: "内收单环",
          config: {
            speed: 4,
            loop: true,
            motion: "inward",
            scope: "single_ring",
            colorMode: "solid",
            colorA: "#ffa500",
            colorB: "#ffa500",
          },
        },
        {
          key: "glacier_core",
          label: "核心脉冲",
          config: {
            speed: 4,
            loop: true,
            motion: "inward",
            scope: "full_screen",
            colorMode: "solid",
            colorA: "#64c8ff",
            colorB: "#64c8ff",
          },
        },
        {
          key: "outward_single",
          label: "外放单环",
          config: {
            speed: 4,
            loop: true,
            motion: "outward",
            scope: "single_ring",
            colorMode: "solid",
            colorA: "#6464ff",
            colorB: "#6464ff",
          },
        },
        {
          key: "solar_flare",
          label: "暖色扩散",
          config: {
            speed: 6,
            loop: true,
            motion: "outward",
            scope: "full_screen",
            colorMode: "gradient",
            colorA: "#ff6464",
            colorB: "#ffdc00",
          },
        },
        {
          key: "clockwise_full",
          label: "整屏巡航",
          config: {
            speed: 7,
            loop: true,
            motion: "clockwise",
            scope: "full_screen",
            colorMode: "gradient",
            colorA: "#ff6464",
            colorB: "#ffdc00",
          },
        },
        {
          key: "outward_full",
          label: "冷色扩散",
          config: {
            speed: 5,
            loop: false,
            motion: "outward",
            scope: "full_screen",
            colorMode: "gradient",
            colorA: "#ffffff",
            colorB: "#64c8ff",
          },
        },
        {
          key: "violet_mirror",
          label: "紫色镜流",
          config: {
            speed: 5,
            loop: true,
            motion: "outward",
            scope: "single_ring",
            colorMode: "gradient",
            colorA: "#c864ff",
            colorB: "#ffffff",
          },
        },
      ],
      rhythmPresets: [
        {
          key: "left_wave",
          label: "FFT 柱阵",
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
          key: "aurora_flow",
          label: "FFT 柔瀑",
          config: {
            bpm: 72,
            speed: 3,
            loop: true,
            direction: "left",
            strength: 58,
            mode: "gradient",
            colorA: "#64c8ff",
            colorB: "#00ff9d",
          },
        },
        {
          key: "right_sweep",
          label: "扫频频谱",
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
          key: "cyber_scan",
          label: "高能柱阵",
          config: {
            bpm: 126,
            speed: 6,
            loop: true,
            direction: "right",
            strength: 78,
            mode: "pulse",
            colorA: "#6464ff",
            colorB: "#c864ff",
          },
        },
        {
          key: "top_drop",
          label: "冲顶频峰",
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
          key: "lava_surge",
          label: "暖色频峰",
          config: {
            bpm: 92,
            speed: 4,
            loop: true,
            direction: "up",
            strength: 82,
            mode: "pulse",
            colorA: "#ffa500",
            colorB: "#ff6464",
          },
        },
        {
          key: "bottom_jump",
          label: "爆闪模式",
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
          key: "matrix_rain",
          label: "矩阵频瀑",
          config: {
            bpm: 104,
            speed: 6,
            loop: true,
            direction: "down",
            strength: 76,
            mode: "gradient",
            colorA: "#00ff9d",
            colorB: "#64c8ff",
          },
        },
        {
          key: "calm_flow",
          label: "冷色缓频",
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
          key: "strobe_grid",
          label: "白蓝爆频",
          config: {
            bpm: 164,
            speed: 8,
            loop: true,
            direction: "right",
            strength: 94,
            mode: "jump",
            colorA: "#ffffff",
            colorB: "#64c8ff",
          },
        },
        {
          key: "single_hit",
          label: "单击冲击",
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
        motion: "clockwise",
        scope: "single_ring",
        colorMode: "solid",
        colorA: "#64c8ff",
        colorB: "#64c8ff",
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
        return "像素信息屏";
      }
      if (this.effectType === "breath_effect") {
        return "矩阵流光";
      }
      return "音频频谱";
    },
    currentPreviewPixels() {
      if (this.previewFrameMaps.length === 0) {
        return new Map();
      }
      if (this.previewFrameIndex >= this.previewFrameMaps.length) {
        return this.previewFrameMaps[0];
      }
      return this.previewFrameMaps[this.previewFrameIndex];
    },
    previewSubtitle() {
      if (!this.previewCanvasReady) {
        return "预览网格加载中，完成后会自动播放当前效果";
      }
      if (this.effectType === "text_display") {
        return "参考 AWTRIX 与 HUB75 的信息屏排版、滚动和阅读层级";
      }
      if (this.effectType === "breath_effect") {
        return "参考 WLED 与 led-matrix 的流光、彗尾和整屏扫动层次";
      }
      if (this.effectType === "rhythm_effect") {
        return "参考 ESP32_FFT_VU 的柱阵频谱、峰值保持和扫频节奏";
      }
      if (this.previewFrameMaps.length > 1) {
        return "自动播放当前参数生成的效果";
      }
      return "显示当前参数对应的静态结果";
    },
    previewStatusLabel() {
      if (this.isSending) {
        return "发送中";
      }
      if (!this.previewCanvasReady) {
        return "预览加载中";
      }
      return "模拟预览";
    },
    previewStatusClass() {
      if (this.isSending) {
        return "is-sending";
      }
      if (!this.previewCanvasReady) {
        return "is-loading";
      }
      return "is-preview";
    },
  },
  watch: {
    textConfig: {
      handler() {
        if (this.effectType === "text_display") {
          this.schedulePreviewRefresh();
        }
      },
      deep: true,
    },
    breathConfig: {
      handler() {
        if (this.effectType === "breath_effect") {
          this.schedulePreviewRefresh();
        }
      },
      deep: true,
    },
    rhythmConfig: {
      handler() {
        if (this.effectType === "rhythm_effect") {
          this.schedulePreviewRefresh();
        }
      },
      deep: true,
    },
    effectType() {
      this.schedulePreviewRefresh();
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
  onUnload() {
    this.cleanupPreviewTimers();
  },
  onHide() {
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
    initPreviewCanvas() {
      const systemInfo = uni.getSystemInfoSync();
      const statusBarHeight = systemInfo.statusBarHeight || 0;

      this.$nextTick(() => {
        setTimeout(() => {
          const query = uni.createSelectorQuery().in(this);
          query.select(".canvas-section").boundingClientRect((sectionRect) => {
            if (!sectionRect || !sectionRect.height) {
              return;
            }

            const nextHeight =
              systemInfo.windowHeight - statusBarHeight - 88 - sectionRect.height;
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
    frameToPixelMap(frame) {
      const pixels = new Map();
      if (!frame || !frame.pixels || typeof frame.pixels.length !== "number") {
        return pixels;
      }
      for (let i = 0; i < frame.pixels.length; i += 5) {
        const x = frame.pixels[i];
        const y = frame.pixels[i + 1];
        const r = frame.pixels[i + 2];
        const g = frame.pixels[i + 3];
        const b = frame.pixels[i + 4];
        pixels.set(`${x},${y}`, `rgb(${r}, ${g}, ${b})`);
      }
      return pixels;
    },
    schedulePreviewRefresh() {
      if (this.previewRefreshTimer) {
        clearTimeout(this.previewRefreshTimer);
        this.previewRefreshTimer = null;
      }
      this.previewRefreshTimer = setTimeout(() => {
        this.refreshPreviewFrames();
      }, 120);
    },
    async refreshPreviewFrames() {
      const token = this.previewRefreshToken + 1;
      this.previewRefreshToken = token;
      try {
        const frames = await this.buildFrames();
        if (token !== this.previewRefreshToken) {
          return;
        }
        this.previewFrameMaps = frames.map((frame) => this.frameToPixelMap(frame));
        this.previewFrameDelays = frames.map((frame) => {
          if (!frame || !frame.delay) {
            return 120;
          }
          return frame.delay;
        });
        this.previewFrameIndex = 0;
        this.startPreviewPlayback();
      } catch (err) {
        console.error("预览生成失败:", err);
        this.previewFrameMaps = [];
        this.previewFrameDelays = [];
        this.previewFrameIndex = 0;
        this.stopPreviewPlayback();
      }
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

    applyTextPreset(preset) {
      if (!preset || !preset.config) {
        return;
      }
      this.textConfig = { ...preset.config };
      this.selectedTextPresetKey = preset.key;
    },
    handleTextFontSelect(value) {
      this.textConfig.font = value;
      this.selectedTextPresetKey = "";
    },
    handleTextDirectionSelect(value) {
      this.textConfig.direction = value;
      this.selectedTextPresetKey = "";
    },
    handleTextLoopToggle() {
      this.textConfig.loop = !this.textConfig.loop;
      this.selectedTextPresetKey = "";
    },
    handleTextContentInput(e) {
      this.textConfig.text = e.detail.value;
      this.selectedTextPresetKey = "";
    },
    handleTextSpeedChange(e) {
      this.textConfig.speed = Number(e.detail.value);
      this.selectedTextPresetKey = "";
    },
    handleTextXChange(e) {
      this.textConfig.x = Number(e.detail.value);
      this.selectedTextPresetKey = "";
    },
    handleTextYChange(e) {
      this.textConfig.y = Number(e.detail.value);
      this.selectedTextPresetKey = "";
    },
    handleTextColorSelect(hex) {
      this.textConfig.color = hex;
      this.selectedTextPresetKey = "";
    },
    handleTextBgColorSelect(hex) {
      this.textConfig.bgColor = hex;
      this.selectedTextPresetKey = "";
    },

    handleBreathMotionSelect(value) {
      this.breathConfig.motion = value;
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
    handleBreathScopeSelect(value) {
      this.breathConfig.scope = value;
      this.selectedBreathPresetKey = "";
    },
    handleBreathColorModeSelect(value) {
      this.breathConfig.colorMode = value;
      this.selectedBreathPresetKey = "";
    },
    handleBreathColorSelect(hex) {
      this.breathConfig.colorA = hex;
      this.selectedBreathPresetKey = "";
    },
    handleBreathColorBSelect(hex) {
      this.breathConfig.colorB = hex;
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
        (cfg.motion === "clockwise" ||
          cfg.motion === "counterclockwise" ||
          cfg.motion === "inward" ||
          cfg.motion === "outward") &&
        (cfg.scope === "single_ring" || cfg.scope === "full_screen") &&
        (cfg.colorMode === "solid" || cfg.colorMode === "gradient") &&
        typeof cfg.colorA === "string" &&
        typeof cfg.colorB === "string"
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

    saveDraft() {
      this.saveConfig();
      this.toast.showSuccess("草稿已保存");
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
    getTextDisplayFontProfile(cfg) {
      if (cfg.font === "mono") {
        return {
          measure(text) {
            return getTinyTextWidth(text, 2);
          },
          draw(text, x, y, color, pixelMap) {
            drawTinyTextToPixels(text, x, y, color, pixelMap, 2, "left");
          },
          height: 10,
          source: "HUB75",
        };
      }
      return {
        measure(text) {
          return getTextWidth(text, 1);
        },
        draw(text, x, y, color, pixelMap) {
          drawTextToPixels(text, x, y, color, 1, pixelMap, "left");
        },
        height: 7,
        source: cfg.font === "serif" ? "SIGN" : "AWTRIX",
      };
    },
    getTextFrameMetrics(cfg) {
      const profile = this.getTextDisplayFontProfile(cfg);
      const text = normalizeDisplayText(cfg.text);
      const step = Math.max(1, Math.round(0.6 + Number(cfg.speed) * 0.55));
      const textWidth = profile.measure(text);
      const textHeight = profile.height;
      let distance = 0;

      if (cfg.direction === "left") {
        distance = Math.max(step, Number(cfg.x) - (-textWidth - 8));
      } else if (cfg.direction === "right") {
        distance = Math.max(step, 72 - Number(cfg.x));
      } else if (cfg.direction === "up") {
        distance = Math.max(step, Number(cfg.y) - (-textHeight - 8));
      } else if (cfg.direction === "down") {
        distance = Math.max(step, 72 - Number(cfg.y));
      } else {
        throw new Error("文本方向参数错误");
      }

      return {
        step,
        frameCount: cfg.loop ? Math.max(28, Math.ceil(distance / step) + 8) : 1,
        textWidth,
        textHeight,
        source: profile.source,
      };
    },
    getTextFramePosition(cfg, metrics, frameIndex) {
      const travel = metrics.step * frameIndex;
      if (cfg.direction === "left") {
        return { drawX: Number(cfg.x) - travel, drawY: Number(cfg.y) };
      }
      if (cfg.direction === "right") {
        return { drawX: Number(cfg.x) + travel, drawY: Number(cfg.y) };
      }
      if (cfg.direction === "up") {
        return { drawX: Number(cfg.x), drawY: Number(cfg.y) - travel };
      }
      return { drawX: Number(cfg.x), drawY: Number(cfg.y) + travel };
    },
    buildFrameFromPixelMap(pixelMap, delay) {
      const bytes = [];
      pixelMap.forEach((color, key) => {
        const [x, y] = key.split(",").map(Number);
        const rgb = hexToRgb(color.startsWith("rgb") ? this.rgbStringToHex(color) : color);
        bytes.push(x, y, rgb.r, rgb.g, rgb.b);
      });
      const pixelBytes = new Uint8Array(bytes);
      return {
        type: 1,
        delay,
        totalPixels: pixelBytes.length / 5,
        pixels: pixelBytes,
      };
    },

    buildNativeBreathPayload() {
      const colorA = hexToRgb(this.ensureColorHex(this.breathConfig.colorA));
      const colorB = hexToRgb(this.ensureColorHex(this.breathConfig.colorB));
      return {
        cmd: "set_breath_effect",
        speed: Number(this.breathConfig.speed),
        loop: this.breathConfig.loop,
        motion: this.breathConfig.motion,
        scope: this.breathConfig.scope,
        colorMode: this.breathConfig.colorMode,
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
      };
    },
    renderTextDisplayFrame(cfg, metrics, frameIndex) {
      const pixelMap = new Map();
      const accentColor = cfg.color;
      const guideColor = blendHexColor(cfg.bgColor, accentColor, 0.42);
      const softColor = blendHexColor(cfg.bgColor, accentColor, 0.26);
      const profile = this.getTextDisplayFontProfile(cfg);
      const text = normalizeDisplayText(cfg.text);
      const position = this.getTextFramePosition(cfg, metrics, frameIndex);

      addTextEffectDecor(pixelMap, cfg, accentColor);
      drawTinyTextToPixels(metrics.source, 7, 5, accentColor, pixelMap, 1, "left");
      drawTinyTextToPixels(
        String(Math.max(1, Number(cfg.speed))).padStart(2, "0"),
        49,
        5,
        guideColor,
        pixelMap,
        1,
        "left",
      );

      for (let index = 0; index < 5; index += 1) {
        const color = (frameIndex + index) % 5 < 2 ? accentColor : softColor;
        fillPreviewRect(pixelMap, 48 + index * 2, 8, 1, 1, color);
      }

      fillPreviewRect(pixelMap, 57, 17 + (frameIndex % 16), 2, 3, accentColor);
      fillPreviewRect(pixelMap, 5, 19 + ((frameIndex + 8) % 14), 1, 8, softColor);
      fillPreviewRect(pixelMap, 6, 44, 52, 1, softColor);

      profile.draw(text, position.drawX, position.drawY, accentColor, pixelMap);
      if (cfg.loop) {
        if (cfg.direction === "left") {
          profile.draw(
            text,
            position.drawX + metrics.textWidth + 12,
            position.drawY,
            guideColor,
            pixelMap,
          );
        } else if (cfg.direction === "right") {
          profile.draw(
            text,
            position.drawX - metrics.textWidth - 12,
            position.drawY,
            guideColor,
            pixelMap,
          );
        }
      }

      const tickerText = `${metrics.source} ${text} `;
      const tickerWidth = getTinyTextWidth(tickerText, 1);
      const tickerX =
        56 - ((frameIndex * Math.max(1, Math.round(Number(cfg.speed)))) % (tickerWidth + 10));
      drawTinyTextToPixels(tickerText, tickerX, 52, guideColor, pixelMap, 1, "left");
      drawTinyTextToPixels(
        tickerText,
        tickerX + tickerWidth + 10,
        52,
        guideColor,
        pixelMap,
        1,
        "left",
      );

      if (cfg.direction === "left" || cfg.direction === "right") {
        const arrowX = cfg.direction === "left" ? 7 : 54;
        fillPreviewRect(pixelMap, arrowX, 57, 3, 1, guideColor);
        fillPreviewRect(
          pixelMap,
          arrowX + (cfg.direction === "left" ? 0 : 2),
          56,
          1,
          3,
          guideColor,
        );
      } else {
        const arrowY = cfg.direction === "up" ? 16 : 39;
        fillPreviewRect(pixelMap, 8, arrowY, 1, 3, guideColor);
        fillPreviewRect(pixelMap, 7, arrowY + (cfg.direction === "up" ? 0 : 2), 3, 1, guideColor);
      }

      return pixelMap;
    },

    async buildTextFrames() {
      const cfg = { ...this.textConfig };
      cfg.color = this.ensureColorHex(cfg.color);
      cfg.bgColor = this.ensureColorHex(cfg.bgColor);

      if (typeof cfg.text !== "string" || cfg.text.length === 0) {
        throw new Error("文本内容不能为空");
      }

      const metrics = this.getTextFrameMetrics(cfg);
      const frames = [];
      const delay = Math.max(42, 168 - Number(cfg.speed) * 10);

      for (let i = 0; i < metrics.frameCount; i++) {
        const pixelMap = this.renderTextDisplayFrame(
          cfg,
          metrics,
          i,
        );
        frames.push(this.buildFrameFromPixelMap(pixelMap, delay));
      }

      return frames;
    },

    rgbStringToHex(value) {
      const match = /rgb\((\d+),\s*(\d+),\s*(\d+)\)/i.exec(value);
      if (!match) {
        return "#000000";
      }
      const r = Number(match[1]);
      const g = Number(match[2]);
      const b = Number(match[3]);
      return `#${r.toString(16).padStart(2, "0")}${g
        .toString(16)
        .padStart(2, "0")}${b.toString(16).padStart(2, "0")}`;
    },

    async buildFrames() {
      if (this.effectType === "text_display") {
        return this.buildTextFrames();
      }
      if (this.effectType === "breath_effect") {
        this.breathConfig.colorA = this.ensureColorHex(this.breathConfig.colorA);
        this.breathConfig.colorB = this.ensureColorHex(this.breathConfig.colorB);
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
          await ws.send(this.buildNativeBreathPayload());
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
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

.status-bar {
  background-color: #1a1a1a;
}

.header {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--bg-elevated);
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

.action-btn-sm.primary {
  border-color: var(--accent-primary);
  background: var(--accent-primary);
}

.action-btn-sm.disabled {
  opacity: 0.4;
}

.action-btn-sm text {
  font-size: 22rpx;
  font-weight: 600;
  color: var(--text-primary);
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

.preview-actions .action-btn-sm.primary text {
  color: #ffffff;
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
  justify-content: space-between;
  gap: 8rpx;
  margin-bottom: 16rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.card-subtitle {
  font-size: 20rpx;
  color: var(--text-secondary);
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
  align-items: center;
  justify-content: center;
  min-height: 76rpx;
  padding: 18rpx 10rpx;
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

.piece-name {
  font-size: 24rpx;
  color: var(--text-secondary);
  font-weight: 600;
  text-align: center;
  line-height: 1.35;
}

.piece-item.active .piece-name {
  color: var(--accent-primary);
}

</style>
