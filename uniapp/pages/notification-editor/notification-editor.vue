<template>
  <view class="notification-editor-page">
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
          v-if="previewCanvasReady && reminderReady"
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
          canvas-id="notificationPreviewCanvas"
        />
      </view>
      <view
        v-else
        class="preview-canvas-container preview-canvas-placeholder"
      ></view>
      <view class="preview-caption">
        <view class="preview-caption-info">
          <text class="preview-caption-title">预览效果</text>
        </view>
        <view class="preview-actions">
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

    <scroll-view
      v-if="reminderReady"
      scroll-y
      class="content"
      :style="{ height: contentHeight }"
    >
      <view class="content-wrapper">
        <view class="card">
          <view class="card-title-section">
            <text class="card-title">提醒规则</text>
          </view>

          <view class="form-row">
            <text class="form-label">提醒名称</text>
            <input
              v-model="reminder.reminderName"
              class="text-input"
              maxlength="12"
              placeholder="请输入提醒名称"
            />
          </view>

          <view class="form-row">
            <text class="form-label">提醒时间</text>
            <picker mode="time" :value="timePickerValue" @change="handleTimeChange">
              <view class="picker-field">
                <text class="picker-value">{{ timePickerValue }}</text>
                <Icon name="direction-right" :size="24" color="#8A8A8A" />
              </view>
            </picker>
          </view>

          <view class="form-row">
            <text class="form-label">重复方式</text>
            <view class="option-row option-row-wrap">
              <view
                v-for="item in repeatModeOptions"
                :key="item.value"
                class="option-btn option-btn-small"
                :class="{ active: reminder.repeatMode === item.value }"
                @click="reminder.repeatMode = item.value"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
          </view>
        </view>

        <view class="card">
          <view class="card-title-section">
            <text class="card-title">展示内容</text>
          </view>

          <view class="form-row">
            <text class="form-label">提示类型</text>
            <view class="option-row">
              <view
                v-for="item in contentTypeOptions"
                :key="item.value"
                class="option-btn"
                :class="{ active: reminder.contentType === item.value }"
                @click="handleContentTypeChange(item.value)"
              >
                <text>{{ item.label }}</text>
              </view>
            </view>
          </view>

          <view class="form-row">
            <text class="form-label">提示文案</text>
            <input
              v-model="reminder.text"
              class="text-input"
              maxlength="10"
              placeholder="例如 记得喝水"
            />
          </view>
        </view>

        <view v-if="reminder.contentType === 'text'" class="card">
          <view class="card-title-section">
            <text class="card-title">文字样式</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="item in textTemplateOptions"
              :key="item.value"
              class="piece-item"
              :class="{ active: reminder.textTemplate === item.value }"
              @click="reminder.textTemplate = item.value"
            >
              <text class="piece-name">{{ item.label }}</text>
              <text class="piece-hint">{{ item.hint }}</text>
            </view>
          </view>
        </view>

        <view v-if="reminder.contentType === 'static'" class="card">
          <view class="card-title-section">
            <text class="card-title">静态样式</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="item in staticTemplateOptions"
              :key="item.value"
              class="piece-item"
              :class="{ active: reminder.staticTemplate === item.value }"
              @click="reminder.staticTemplate = item.value"
            >
              <text class="piece-name">{{ item.label }}</text>
              <text class="piece-hint">{{ item.hint }}</text>
            </view>
          </view>
        </view>

        <view v-if="reminder.contentType === 'animation'" class="card">
          <view class="card-title-section">
            <text class="card-title">动图样式</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="item in animationTemplateOptions"
              :key="item.value"
              class="piece-item"
              :class="{ active: reminder.animationTemplate === item.value }"
              @click="reminder.animationTemplate = item.value"
            >
              <text class="piece-name">{{ item.label }}</text>
              <text class="piece-hint">{{ item.hint }}</text>
            </view>
          </view>
        </view>

        <view v-if="reminder.contentType !== 'text'" class="card">
          <view class="card-title-section">
            <text class="card-title">图标主题</text>
          </view>
          <view class="icon-grid">
            <view
              v-for="item in iconOptions"
              :key="item.value"
              class="icon-item"
              :class="{ active: reminder.icon === item.value }"
              @click="reminder.icon = item.value"
            >
              <text class="icon-item-name">{{ item.label }}</text>
            </view>
          </view>
        </view>

        <view class="card">
          <view class="card-title-section">
            <text class="card-title">主题颜色</text>
          </view>
          <ColorPanelPicker
            :value="reminder.accentColor"
            label="提醒颜色"
            :preset-colors="colorOptions"
            @input="handleAccentColorChange"
          />
        </view>
      </view>
    </scroll-view>

    <view v-if="isSending" class="sending-overlay" @touchmove.stop.prevent>
      <view class="sending-modal">
        <view class="sending-spinner"></view>
        <text class="sending-title">正在发送提醒...</text>
        <text class="sending-tip">设备应用完成后会自动恢复当前预览</text>
      </view>
    </view>

    <Toast ref="toastRef" @show="previewHidden = true" @hide="onToastHide" />
  </view>
</template>

<script>
import { useDeviceStore } from "../../store/device.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import ColorPanelPicker from "../../components/ColorPanelPicker.vue";
import { useToast } from "../../composables/useToast.js";
import { uploadAnimationFrames } from "../../utils/animationUploader.js";
import {
  getNotificationReminder,
  upsertNotificationReminder,
} from "../../utils/notificationReminderStore.js";
import { buildNotificationReminderFrames } from "../../utils/notificationReminderPreview.js";
import { pixelPreviewFramesToAnimationFrames } from "../../utils/pixelAnimationFrames.js";

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
      reminderReady: false,
      reminder: null,
      isSending: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      previewCanvasReady: false,
      previewHidden: false,
      previewZoom: 4,
      previewOffset: { x: 16, y: 16 },
      previewContainerSize: { width: 320, height: 320 },
      previewFrames: [],
      previewFrameIndex: 0,
      previewTimer: null,
      repeatModeOptions: [
        { label: "单次", value: "once" },
        { label: "每天", value: "daily" },
        { label: "工作日", value: "weekday" },
        { label: "休息日", value: "weekend" },
        { label: "每年", value: "yearly" },
      ],
      contentTypeOptions: [
        { label: "文字提示", value: "text" },
        { label: "静态提示", value: "static" },
        { label: "动图提示", value: "animation" },
      ],
      textTemplateOptions: [
        { label: "大标题", value: "headline", hint: "适合短文案" },
        { label: "标签卡", value: "badge", hint: "显示更稳" },
        { label: "滚动条", value: "ticker", hint: "适合长文案" },
      ],
      staticTemplateOptions: [
        { label: "图标卡", value: "badge", hint: "图标居中" },
        { label: "海报卡", value: "poster", hint: "适合祝福" },
        { label: "角标卡", value: "corner", hint: "更像通知牌" },
      ],
      animationTemplateOptions: [
        { label: "脉冲", value: "pulse", hint: "适合普通提醒" },
        { label: "烟花", value: "fireworks", hint: "适合庆祝" },
        { label: "环绕", value: "orbit", hint: "适合强调提醒" },
      ],
      iconOptions: [
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
    };
  },
  computed: {
    pageTitle() {
      if (!this.reminderReady || !this.reminder) {
        return "提醒编辑";
      }
      return this.reminder.reminderName;
    },
    timePickerValue() {
      if (!this.reminderReady || !this.reminder) {
        return "00:00";
      }
      return `${String(this.reminder.hour).padStart(2, "0")}:${String(this.reminder.minute).padStart(2, "0")}`;
    },
    currentPreviewPixels() {
      if (this.previewFrames.length === 0) {
        return new Map();
      }
      return this.previewFrames[this.previewFrameIndex].pixels;
    },
    previewCaptionText() {
      if (!this.reminderReady || !this.reminder) {
        return "加载完成后会展示提醒触发效果";
      }
      if (this.reminder.contentType === "text") {
        return "先定时间，再看标题、信息条和文字节奏";
      }
      if (this.reminder.contentType === "static") {
        return "适合节日、祝福和简洁整屏提醒";
      }
      return "适合庆祝、强调和提醒感更强的场景";
    },
    previewStatusLabel() {
      if (this.isSending) {
        return "发送中";
      }
      if (!this.previewCanvasReady || !this.reminderReady) {
        return "加载中";
      }
      return "就绪";
    },
    previewStatusClass() {
      if (this.isSending) {
        return "is-sending";
      }
      if (!this.previewCanvasReady || !this.reminderReady) {
        return "is-loading";
      }
      return "is-preview";
    },
  },
  watch: {
    reminder: {
      handler() {
        if (!this.reminderReady) {
          return;
        }
        this.refreshPreview();
      },
      deep: true,
    },
  },
  onLoad(options) {
    this.toast = useToast();
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();

    if (!options || typeof options.reminderId !== "string") {
      uni.showToast({
        title: "提醒参数缺失",
        icon: "none",
      });
      setTimeout(() => {
        uni.navigateBack();
      }, 300);
      return;
    }

    const reminder = getNotificationReminder(options.reminderId);
    if (!reminder) {
      uni.showToast({
        title: "提醒不存在",
        icon: "none",
      });
      setTimeout(() => {
        uni.navigateBack();
      }, 300);
      return;
    }

    this.reminder = reminder;
    this.reminderReady = true;
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }

    if (!this.reminderReady) {
      return;
    }

    this.initPreviewCanvas();
    this.refreshPreview();
  },
  onShow() {
    if (!this.isSending) {
      this.previewHidden = false;
    }
    if (this.reminderReady) {
      this.refreshPreview();
    }
  },
  onUnload() {
    this.stopPreviewPlayback();
  },
  onHide() {
    this.stopPreviewPlayback();
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
    refreshPreview() {
      this.stopPreviewPlayback();
      this.previewFrames = buildNotificationReminderFrames(this.reminder);
      this.previewFrameIndex = 0;
      if (this.previewFrames.length > 1) {
        this.playNextFrame();
      }
    },
    playNextFrame() {
      if (this.previewFrames.length <= 1) {
        return;
      }

      const currentFrame = this.previewFrames[this.previewFrameIndex];
      const delay = currentFrame.delay;

      this.previewTimer = setTimeout(() => {
        this.previewFrameIndex += 1;
        if (this.previewFrameIndex >= this.previewFrames.length) {
          this.previewFrameIndex = 0;
        }
        this.playNextFrame();
      }, delay);
    },
    stopPreviewPlayback() {
      if (this.previewTimer) {
        clearTimeout(this.previewTimer);
        this.previewTimer = null;
      }
    },
    handleTimeChange(event) {
      const value = event.detail.value;
      const parts = value.split(":");
      if (parts.length !== 2) {
        return;
      }

      this.reminder.hour = Number(parts[0]);
      this.reminder.minute = Number(parts[1]);
    },
    handleContentTypeChange(contentType) {
      this.reminder.contentType = contentType;

      if (contentType === "text") {
        this.reminder.textTemplate = "headline";
        this.reminder.icon = "drink";
        return;
      }

      if (contentType === "static") {
        this.reminder.staticTemplate = "poster";
        this.reminder.icon = "moon";
        return;
      }

      this.reminder.animationTemplate = "pulse";
      this.reminder.icon = "fireworks";
    },
    handleAccentColorChange(color) {
      this.reminder.accentColor = color;
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
        upsertNotificationReminder(this.reminder);
        if (this.previewFrames.length === 0) {
          this.refreshPreview();
        }
        const ws = this.deviceStore.getWebSocket();
        const frames = pixelPreviewFramesToAnimationFrames(this.previewFrames, 180);
        await uploadAnimationFrames(ws, frames, "notification");
        this.deviceStore.setDeviceMode("notification", { businessMode: true });
        this.toast.showSuccess("已保存并发送");
      } catch (err) {
        console.error("发送提醒预览失败:", err);
        this.toast.showError(`发送失败: ${err.message}`);
      } finally {
        this.isSending = false;
      }
    },
  },
};
</script>

<style scoped>
.notification-editor-page {
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

.preview-caption-title {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.preview-caption-text {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-actions {
  display: flex;
  align-items: center;
  gap: 12rpx;
  flex-shrink: 0;
}

.action-btn-sm {
  width: auto;
  min-width: 118rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 18rpx;
  gap: 10rpx;
  border-radius: 18rpx;
  border: 2rpx solid var(--accent-primary);
  background-color: var(--accent-primary);
}

.action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: #ffffff;
}

.action-btn-sm.disabled {
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
  justify-content: space-between;
  gap: 8rpx;
  margin-bottom: 16rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.form-row {
  margin-top: 18rpx;
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

.picker-field {
  margin-top: 10rpx;
  height: 72rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 20rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
}

.picker-value {
  font-size: 26rpx;
  font-weight: 600;
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
}

.option-btn-small {
  flex: none;
  min-width: calc(33.333% - 12rpx);
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

.piece-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14rpx;
}

.piece-item {
  min-height: 138rpx;
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 8rpx;
  padding: 18rpx 16rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
}

.piece-item.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
  box-shadow: 0 8rpx 18rpx rgba(79, 127, 255, 0.12);
}

.piece-name {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.piece-hint {
  font-size: 22rpx;
  line-height: 1.5;
  color: var(--text-secondary);
}

.icon-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14rpx;
}

.icon-item {
  min-height: 94rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 12rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
}

.icon-item.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
}

.icon-item-name {
  font-size: 23rpx;
  font-weight: 600;
  color: var(--text-primary);
}
</style>
