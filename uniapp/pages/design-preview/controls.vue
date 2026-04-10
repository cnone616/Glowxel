<template>
  <view class="nb-page nb-controls-preview glx-scroll-region">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="nb-navbar glx-topbar">
      <view class="nb-nav-action" @click="goBack">
        <Icon name="arrow-left-bold" :size="28" color="#000000" />
      </view>
      <view class="nb-nav-title-wrap">
        <text class="nb-nav-title">控件总览</text>
      </view>
      <view class="nb-nav-spacer"></view>
    </view>

    <scroll-view scroll-y class="nb-scroll">
      <view class="nb-content">
        <view class="nb-card nb-card-lg nb-controls-hero">
          <text class="nb-eyebrow">控件体系</text>
          <text class="nb-display nb-controls-title">先把输入、提示、切换、勾选、拖拽条全部看齐，再落项目。</text>
          <text class="nb-body nb-controls-desc">
            这一页只做控件语法，不掺页面业务。后面项目里的表单、提示、面板交互，都按这一页的风格统一替换。
          </text>
          <view class="nb-chip-row">
            <text class="nb-chip">输入框</text>
            <text class="nb-chip">Toast</text>
            <text class="nb-chip">Select</text>
            <text class="nb-chip">Checkbox</text>
            <text class="nb-chip">Radio</text>
            <text class="nb-chip">Switch</text>
            <text class="nb-chip">Slider</text>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">提示条</text>
            <text class="nb-section-meta">四种状态都要统一，不是只做成功提示</text>
          </view>
            <view class="nb-card nb-controls-section">
            <view class="nb-controls-toast-list">
              <view
                v-for="item in toastExamples"
                :key="item.key"
                class="nb-controls-toast"
                :class="`is-${item.key}`"
              >
                <view class="nb-controls-toast__main">
                  <text class="nb-controls-toast__icon">{{ item.icon }}</text>
                  <text class="nb-controls-toast__message">{{ item.message }}</text>
                </view>
                <view class="nb-controls-toast__close">
                  <text class="nb-controls-toast__close-text">×</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">输入与选择</text>
            <text class="nb-section-meta">输入框不一定有搜索标，但结构语法必须一致</text>
          </view>
          <view class="nb-card nb-controls-section">
            <view class="nb-controls-grid-3">
              <view class="nb-controls-block">
                <text class="nb-controls-label">文本输入</text>
                <view class="nb-field">
                  <text class="nb-field__value">Glowxel Pixel Board</text>
                </view>
              </view>
              <view class="nb-controls-block">
                <text class="nb-controls-label">数值输入</text>
                <view class="nb-field nb-field--mono">
                  <text class="nb-field__value">52 × 52</text>
                </view>
              </view>
              <view class="nb-controls-block">
                <text class="nb-controls-label">下拉选择</text>
                <view class="nb-field nb-select">
                  <text class="nb-field__value">选择阴影层级</text>
                  <text class="nb-select__arrow">⌄</text>
                </view>
              </view>
            </view>

            <view class="nb-controls-block nb-controls-textarea-block">
              <text class="nb-controls-label">多行输入</text>
              <view class="nb-textarea">
                <text class="nb-textarea__value">这里用于备注设备场景、主题说明或发布描述，整体风格和输入框保持同一套边框与阴影规则。</text>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">勾选与切换</text>
            <text class="nb-section-meta">复选、单选、开关都要和主系统一样硬朗</text>
          </view>
          <view class="nb-card nb-controls-section">
            <view class="nb-controls-grid-3">
              <view class="nb-controls-block">
                <text class="nb-controls-label">复选框</text>
                <view class="nb-controls-choice-stack">
                  <view
                    v-for="item in checkboxItems"
                    :key="item.key"
                    class="nb-controls-choice-row"
                    @click="toggleCheckbox(item.key)"
                  >
                    <view class="nb-controls-choice-box" :class="{ active: checkboxState[item.key] }">
                      <text v-if="checkboxState[item.key]" class="nb-controls-choice-mark">✓</text>
                    </view>
                    <text class="nb-controls-choice-text">{{ item.label }}</text>
                  </view>
                </view>
              </view>

              <view class="nb-controls-block">
                <text class="nb-controls-label">单选框</text>
                <view class="nb-controls-choice-stack">
                  <view
                    v-for="item in radioItems"
                    :key="item.key"
                    class="nb-controls-choice-row"
                    @click="radioValue = item.key"
                  >
                    <view class="nb-controls-radio-box" :class="{ active: radioValue === item.key }">
                      <view v-if="radioValue === item.key" class="nb-controls-radio-dot"></view>
                    </view>
                    <text class="nb-controls-choice-text">{{ item.label }}</text>
                  </view>
                </view>
              </view>

              <view class="nb-controls-block">
                <text class="nb-controls-label">开关</text>
                <view class="nb-controls-choice-stack">
                  <view
                    v-for="item in switchItems"
                    :key="item.key"
                    class="nb-controls-switch-row"
                    @click="toggleSwitch(item.key)"
                  >
                    <view class="nb-controls-switch" :class="{ active: switchState[item.key] }">
                      <view class="nb-controls-switch__thumb"></view>
                    </view>
                    <text class="nb-controls-choice-text">{{ item.label }}</text>
                  </view>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="nb-section">
          <view class="nb-section-header">
            <text class="nb-section-title">滑杆与色板</text>
            <text class="nb-section-meta">拖拽条、值显示、颜色块都先统一再进项目</text>
          </view>
          <view class="nb-card nb-controls-section">
            <view class="nb-controls-slider-layout">
              <view class="nb-controls-live-card">
                <text class="nb-controls-live-card__title">Live Preview</text>
                <text class="nb-controls-live-card__desc">拖拽右侧控件感受边框厚度、阴影深度和块面色彩的整体语法。</text>
                <view
                  class="nb-controls-live-card__button"
                  :style="liveCardStyle"
                >
                  <text class="nb-controls-live-card__button-text">Click me</text>
                </view>
              </view>

              <view class="nb-controls-slider-panel">
                <view
                  v-for="item in sliders"
                  :key="item.key"
                  class="nb-controls-slider-control"
                >
                  <text class="nb-controls-slider-label">{{ item.label }}</text>
                  <view
                    :id="`preview-slider-${item.key}`"
                    class="nb-controls-slider"
                    @touchstart.stop.prevent="updateSliderFromTouch(item.key, $event)"
                    @touchmove.stop.prevent="updateSliderFromTouch(item.key, $event)"
                    @click.stop="updateSliderFromTouch(item.key, $event)"
                  >
                    <view class="nb-controls-slider__track"></view>
                    <view
                      class="nb-controls-slider__fill"
                      :style="{ width: `${sliderPercent(item.key)}%` }"
                    ></view>
                    <view
                      class="nb-controls-slider__thumb"
                      :style="{ left: `calc(${sliderPercent(item.key)}% - 18rpx)` }"
                    ></view>
                  </view>
                  <text class="nb-controls-slider-value">{{ sliderValues[item.key] }}{{ item.unit }}</text>
                </view>

                <view class="nb-controls-palette">
                  <text class="nb-controls-slider-label">背景色板</text>
                  <view class="nb-controls-swatch-row">
                    <view
                      v-for="color in swatches"
                      :key="color.value"
                      class="nb-controls-swatch-button"
                      :class="{ active: selectedSwatch === color.value }"
                      :style="{ background: color.value }"
                      @click="selectedSwatch = color.value"
                    ></view>
                  </view>
                </view>
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
  },
  data() {
    return {
      toastExamples: [
        { key: "success", icon: "✓", message: "更改已成功保存。" },
        { key: "error", icon: "×", message: "发送失败，请检查设备连接。" },
        { key: "info", icon: "i", message: "正在同步网络时间，请稍候。" },
        { key: "warning", icon: "!", message: "该操作会覆盖当前配置。" },
      ],
      checkboxItems: [
        { key: "border", label: "粗边框" },
        { key: "shadow", label: "硬阴影" },
        { key: "radius", label: "弱圆角（少量场景）" },
      ],
      checkboxState: {
        border: true,
        shadow: true,
        radius: false,
      },
      radioItems: [
        { key: "bold", label: "Bold" },
        { key: "bolder", label: "Bolder" },
        { key: "boldest", label: "Boldest" },
      ],
      radioValue: "bold",
      switchItems: [
        { key: "dark", label: "深色背景模式" },
        { key: "gradient", label: "渐变（禁用示例）" },
      ],
      switchState: {
        dark: false,
        gradient: false,
      },
      sliders: [
        { key: "borderWidth", label: "边框厚度", min: 2, max: 8, unit: "px" },
        { key: "shadowOffset", label: "阴影偏移", min: 0, max: 18, unit: "px" },
        { key: "radius", label: "圆角半径", min: 0, max: 24, unit: "px" },
      ],
      sliderValues: {
        borderWidth: 4,
        shadowOffset: 6,
        radius: 0,
      },
      swatches: [
        { value: "#FFFFFF" },
        { value: "#FFD23F" },
        { value: "#FF6B6B" },
        { value: "#74B9FF" },
        { value: "#88D498" },
      ],
      selectedSwatch: "#FFFFFF",
    };
  },
  computed: {
    liveCardStyle() {
      return {
        borderWidth: `${this.sliderValues.borderWidth}px`,
        boxShadow: `${this.sliderValues.shadowOffset}px ${this.sliderValues.shadowOffset}px 0 #000000`,
        borderRadius: `${this.sliderValues.radius}px`,
        background: this.selectedSwatch,
      };
    },
  },
  methods: {
    goBack() {
      uni.navigateBack();
    },
    toggleCheckbox(key) {
      this.checkboxState[key] = !this.checkboxState[key];
    },
    toggleSwitch(key) {
      this.switchState[key] = !this.switchState[key];
    },
    sliderPercent(key) {
      const config = this.sliders.find((item) => item.key === key);
      if (!config) return 0;
      const range = config.max - config.min;
      if (range <= 0) return 0;
      return ((this.sliderValues[key] - config.min) / range) * 100;
    },
    updateSliderFromTouch(key, event) {
      const config = this.sliders.find((item) => item.key === key);
      if (!config) return;
      const point =
        (event.touches && event.touches[0]) ||
        (event.changedTouches && event.changedTouches[0]) ||
        event.detail;
      const clientX =
        point && typeof point.clientX === "number"
          ? point.clientX
          : point && typeof point.x === "number"
            ? point.x
            : null;
      if (clientX === null) return;

      const query = uni.createSelectorQuery().in(this);
      query
        .select(`#preview-slider-${key}`)
        .boundingClientRect((rect) => {
          if (!rect || !rect.width) return;
          const clampedX = Math.min(
            Math.max(clientX - rect.left, 0),
            rect.width,
          );
          const percent = clampedX / rect.width;
          const nextValue = Math.round(
            config.min + percent * (config.max - config.min),
          );
          this.sliderValues[key] = nextValue;
        })
        .exec();
    },
  },
};
</script>

<style lang="scss">
@import "../../styles/themes/neubrutalism-preview.css";

.nb-controls-hero {
  padding: 28rpx;
  background: var(--nb-yellow);
}

.nb-controls-title {
  display: block;
  margin-top: 24rpx;
  margin-bottom: 24rpx;
}

.nb-controls-desc {
  display: block;
  margin-bottom: 24rpx;
}

.nb-controls-section {
  padding: 28rpx;
}

.nb-controls-toast-list {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.nb-controls-toast {
  min-height: 96rpx;
  padding: 0 18rpx 0 24rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18rpx;
  border: var(--nb-border-panel);
  box-shadow: var(--nb-shadow-md);
  box-sizing: border-box;
}

.nb-controls-toast.is-success {
  background: var(--nb-green);
}

.nb-controls-toast.is-error {
  background: var(--nb-coral);
}

.nb-controls-toast.is-info {
  background: var(--nb-blue);
}

.nb-controls-toast.is-warning {
  background: var(--nb-yellow);
}

.nb-controls-toast__main {
  min-width: 0;
  flex: 1;
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 22rpx 0;
}

.nb-controls-toast__icon {
  font-size: 36rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-controls-toast__message {
  flex: 1;
  min-width: 0;
  font-size: 28rpx;
  line-height: 1.35;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-controls-toast__close {
  width: 52rpx;
  height: 52rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border-control);
  background: rgba(255, 255, 255, 0.28);
  box-sizing: border-box;
  flex-shrink: 0;
}

.nb-controls-toast__close-text {
  font-size: 28rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-controls-grid-3 {
  display: flex;
  flex-wrap: wrap;
  gap: 24rpx;
}

.nb-controls-block {
  width: calc((100% - 48rpx) / 3);
}

.nb-controls-label {
  display: block;
  margin-bottom: 14rpx;
  font-size: 24rpx;
  line-height: 1.2;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-field,
.nb-textarea {
  background: var(--nb-surface);
  border: var(--nb-border-control);
  box-shadow: var(--nb-shadow-sm);
  box-sizing: border-box;
}

.nb-field {
  min-height: 88rpx;
  padding: 0 24rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12rpx;
}

.nb-field__value {
  font-size: 28rpx;
  line-height: 1.3;
  font-weight: 700;
  color: #8b8b8b;
}

.nb-field--mono .nb-field__value {
  color: var(--nb-ink);
  font-family: monospace;
  font-weight: 900;
}

.nb-select__arrow {
  font-size: 28rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-controls-textarea-block {
  width: 100%;
  margin-top: 24rpx;
}

.nb-textarea {
  min-height: 180rpx;
  padding: 24rpx;
}

.nb-textarea__value {
  font-size: 28rpx;
  line-height: 1.55;
  color: #8b8b8b;
}

.nb-controls-choice-stack {
  display: flex;
  flex-direction: column;
  gap: 18rpx;
}

.nb-controls-choice-row,
.nb-controls-switch-row {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.nb-controls-choice-box,
.nb-controls-radio-box {
  width: 40rpx;
  height: 40rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--nb-surface);
  border: var(--nb-border-control);
  box-shadow: var(--nb-shadow-sm);
  box-sizing: border-box;
  flex-shrink: 0;
}

.nb-controls-choice-box.active {
  background: var(--nb-yellow);
}

.nb-controls-choice-mark {
  font-size: 24rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-controls-radio-box.active {
  background: #ffd7cf;
}

.nb-controls-radio-dot {
  width: 18rpx;
  height: 18rpx;
  background: var(--nb-ink);
}

.nb-controls-choice-text {
  font-size: 24rpx;
  line-height: 1.35;
  color: var(--nb-ink);
  font-weight: 700;
}

.nb-controls-switch {
  width: 86rpx;
  height: 40rpx;
  padding: 4rpx;
  display: flex;
  align-items: center;
  background: var(--nb-surface);
  border: var(--nb-border-control);
  box-shadow: var(--nb-shadow-sm);
  box-sizing: border-box;
  flex-shrink: 0;
}

.nb-controls-switch.active {
  justify-content: flex-end;
  background: var(--nb-yellow);
}

.nb-controls-switch__thumb {
  width: 28rpx;
  height: 28rpx;
  background: var(--nb-ink);
}

.nb-controls-slider-layout {
  display: flex;
  gap: 0;
  border: var(--nb-border-panel);
  box-shadow: var(--nb-shadow-md);
}

.nb-controls-live-card {
  width: 48%;
  padding: 32rpx;
  border-right: var(--nb-border-panel);
  background:
    radial-gradient(circle, rgba(0, 0, 0, 0.12) 1.5rpx, transparent 1.5rpx)
      0 0 / 28rpx 28rpx,
    #fffaf0;
  box-sizing: border-box;
  min-height: 480rpx;
}

.nb-controls-live-card__title {
  display: block;
  margin-bottom: 18rpx;
  font-size: 34rpx;
  line-height: 1.1;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-controls-live-card__desc {
  display: block;
  max-width: 320rpx;
  font-size: 26rpx;
  line-height: 1.55;
  color: var(--nb-text-body);
}

.nb-controls-live-card__button {
  margin-top: 32rpx;
  width: 220rpx;
  min-height: 84rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border-control);
  box-sizing: border-box;
}

.nb-controls-live-card__button-text {
  font-size: 28rpx;
  line-height: 1;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-controls-slider-panel {
  width: 52%;
  padding: 32rpx;
  background: var(--nb-surface);
  box-sizing: border-box;
}

.nb-controls-slider-control + .nb-controls-slider-control {
  margin-top: 28rpx;
}

.nb-controls-slider-label {
  display: block;
  margin-bottom: 12rpx;
  font-size: 22rpx;
  line-height: 1.2;
  letter-spacing: 4rpx;
  color: var(--nb-text-muted);
  text-transform: uppercase;
}

.nb-controls-slider {
  position: relative;
  height: 32rpx;
}

.nb-controls-slider__track,
.nb-controls-slider__fill {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  height: 10rpx;
  border: var(--nb-border-control);
  box-sizing: border-box;
}

.nb-controls-slider__track {
  left: 0;
  right: 0;
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-sm);
}

.nb-controls-slider__fill {
  left: 0;
  background: #ffe89a;
  box-shadow: none;
  border-right: 0;
}

.nb-controls-slider__thumb {
  position: absolute;
  top: 50%;
  width: 36rpx;
  height: 36rpx;
  transform: translateY(-50%);
  background: var(--nb-yellow);
  border: var(--nb-border-control);
  box-shadow: var(--nb-shadow-sm);
  box-sizing: border-box;
}

.nb-controls-slider-value {
  display: block;
  margin-top: 8rpx;
  font-size: 22rpx;
  line-height: 1.2;
  font-weight: 900;
  color: var(--nb-ink);
}

.nb-controls-palette {
  margin-top: 32rpx;
}

.nb-controls-swatch-row {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
}

.nb-controls-swatch-button {
  width: 56rpx;
  height: 56rpx;
  border: var(--nb-border-control);
  box-shadow: var(--nb-shadow-sm);
  box-sizing: border-box;
}

.nb-controls-swatch-button.active {
  box-shadow: var(--nb-shadow-lg);
}

@media (max-width: 750rpx) {
  .nb-controls-block {
    width: 100%;
  }

  .nb-controls-slider-layout {
    flex-direction: column;
  }

  .nb-controls-live-card,
  .nb-controls-slider-panel {
    width: 100%;
  }

  .nb-controls-live-card {
    border-right: 0;
    border-bottom: var(--nb-border-panel);
  }
}
</style>
