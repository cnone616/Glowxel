<template>
  <view class="notification-list-page">
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
        <text class="project-name">通知提醒</text>
      </view>
    </view>

    <scroll-view scroll-y class="content">
      <view class="content-wrapper">
        <view class="card hero-card">
          <view class="card-title-section">
            <text class="card-title">创建定时提醒</text>
          </view>
          <text class="hero-text">
            先创建提醒规则，再进入具体预览和展示内容编辑。支持文字提示、静态提示和动图提示。
          </text>
          <view class="create-grid">
            <view class="create-item" @click="createReminder('text')">
              <Icon name="text" :size="40" color="#4F7FFF" />
              <text class="create-name">文字提示</text>
            </view>
            <view class="create-item" @click="createReminder('static')">
              <Icon name="picture" :size="40" color="#4F7FFF" />
              <text class="create-name">静态提示</text>
            </view>
            <view class="create-item" @click="createReminder('animation')">
              <Icon name="play" :size="40" color="#4F7FFF" />
              <text class="create-name">动图提示</text>
            </view>
          </view>
        </view>

        <view class="section-heading">
          <text class="section-title">已创建提醒</text>
          <text class="section-count">{{ reminders.length }} 条</text>
        </view>

        <view v-if="reminders.length === 0" class="card empty-card">
          <text class="empty-title">还没有提醒</text>
          <text class="empty-text">从上面选择一种类型，新建第一条定时提醒。</text>
        </view>

        <view
          v-for="reminder in reminders"
          :key="reminder.reminderId"
          class="card reminder-card"
          @click="openReminder(reminder.reminderId)"
        >
          <view class="reminder-main">
            <view class="time-block">
              <text class="time-text">{{ formatTime(reminder) }}</text>
              <text class="repeat-text">{{ getReminderRepeatLabel(reminder.repeatMode) }}</text>
            </view>
            <view class="reminder-info">
              <view class="reminder-title-row">
                <text class="reminder-name">{{ reminder.reminderName }}</text>
                <text class="reminder-type">{{ getReminderTypeLabel(reminder.contentType) }}</text>
              </view>
              <text class="reminder-summary">{{ buildReminderSummary(reminder) }}</text>
            </view>
          </view>
          <view class="reminder-actions">
            <view class="mini-btn" @click.stop="openReminder(reminder.reminderId)">
              <text>编辑</text>
            </view>
            <view class="mini-btn" @click.stop="duplicateReminder(reminder.reminderId)">
              <text>复制</text>
            </view>
            <view class="mini-btn danger" @click.stop="deleteReminder(reminder.reminderId)">
              <text>删除</text>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <Toast ref="toastRef" />
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import { useToast } from "../../composables/useToast.js";
import {
  createNotificationReminder,
  duplicateNotificationReminder,
  getNotificationReminderList,
  getNotificationReminder,
  removeNotificationReminder,
  upsertNotificationReminder,
} from "../../utils/notificationReminderStore.js";
import {
  getReminderRepeatLabel,
  getReminderTypeLabel,
} from "../../utils/notificationReminderPreview.js";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
  },
  data() {
    return {
      toast: null,
      reminders: [],
    };
  },
  onShow() {
    this.loadReminders();
  },
  onReady() {
    this.toast = useToast();
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
  },
  methods: {
    loadReminders() {
      this.reminders = getNotificationReminderList();
    },
    handleBack() {
      uni.navigateBack();
    },
    createReminder(contentType) {
      const reminder = createNotificationReminder(contentType);
      upsertNotificationReminder(reminder);
      this.loadReminders();
      uni.navigateTo({
        url: `/pages/notification-editor/notification-editor?reminderId=${reminder.reminderId}`,
      });
    },
    openReminder(reminderId) {
      uni.navigateTo({
        url: `/pages/notification-editor/notification-editor?reminderId=${reminderId}`,
      });
    },
    duplicateReminder(reminderId) {
      const duplicatedReminder = duplicateNotificationReminder(reminderId);
      if (!duplicatedReminder) {
        this.toast.showError("复制失败");
        return;
      }
      this.loadReminders();
      this.toast.showSuccess("已复制提醒");
    },
    deleteReminder(reminderId) {
      const targetReminder = getNotificationReminder(reminderId);
      if (!targetReminder) {
        this.toast.showError("提醒不存在");
        return;
      }

      uni.showModal({
        title: "删除提醒",
        content: `确认删除“${targetReminder.reminderName}”吗？`,
        success: (result) => {
          if (!result.confirm) {
            return;
          }
          removeNotificationReminder(reminderId);
          this.loadReminders();
          this.toast.showSuccess("已删除提醒");
        },
      });
    },
    formatTime(reminder) {
      return `${String(reminder.hour).padStart(2, "0")}:${String(reminder.minute).padStart(2, "0")}`;
    },
    getReminderRepeatLabel(repeatMode) {
      return getReminderRepeatLabel(repeatMode);
    },
    getReminderTypeLabel(contentType) {
      return getReminderTypeLabel(contentType);
    },
    buildReminderSummary(reminder) {
      return `${reminder.text} · ${this.getReminderRepeatLabel(reminder.repeatMode)}`;
    },
  },
};
</script>

<style scoped>
.notification-list-page {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--bg-secondary);
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
  background: var(--bg-elevated);
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
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
  font-size: 36rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.content {
  flex: 1;
}

.content-wrapper {
  padding: 20rpx 24rpx 28rpx;
}

.card {
  background: var(--bg-elevated);
  border: 2rpx solid var(--border-primary);
  border-radius: 24rpx;
  padding: 24rpx;
  margin-bottom: 20rpx;
}

.hero-card {
  background: linear-gradient(
    180deg,
    rgba(79, 127, 255, 0.14),
    rgba(79, 127, 255, 0.05)
  );
}

.card-title-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 18rpx;
}

.card-title {
  font-size: 30rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.hero-text {
  font-size: 24rpx;
  line-height: 1.7;
  color: var(--text-secondary);
}

.create-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14rpx;
  margin-top: 20rpx;
}

.create-item {
  min-height: 144rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  border-radius: 18rpx;
  border: 2rpx solid rgba(79, 127, 255, 0.18);
  background: rgba(255, 255, 255, 0.65);
}

.create-name {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.section-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin: 12rpx 4rpx 18rpx;
}

.section-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.section-count {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.empty-card {
  align-items: center;
}

.empty-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.empty-text {
  margin-top: 10rpx;
  font-size: 24rpx;
  line-height: 1.6;
  color: var(--text-secondary);
}

.reminder-card {
  padding: 22rpx 24rpx;
}

.reminder-main {
  display: flex;
  gap: 20rpx;
}

.time-block {
  width: 132rpx;
  flex-shrink: 0;
}

.time-text {
  display: block;
  font-size: 38rpx;
  font-weight: 700;
  color: var(--text-primary);
  line-height: 1.1;
}

.repeat-text {
  display: block;
  margin-top: 8rpx;
  font-size: 22rpx;
  color: var(--text-secondary);
}

.reminder-info {
  flex: 1;
  min-width: 0;
}

.reminder-title-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12rpx;
}

.reminder-name {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.reminder-type {
  padding: 6rpx 14rpx;
  border-radius: 999rpx;
  background: rgba(79, 127, 255, 0.14);
  font-size: 20rpx;
  color: var(--accent-primary);
  flex-shrink: 0;
}

.reminder-summary {
  display: block;
  margin-top: 12rpx;
  font-size: 24rpx;
  line-height: 1.6;
  color: var(--text-secondary);
}

.reminder-actions {
  display: flex;
  justify-content: flex-end;
  gap: 12rpx;
  margin-top: 18rpx;
}

.mini-btn {
  min-width: 90rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 18rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
}

.mini-btn text {
  font-size: 22rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.mini-btn.danger {
  border-color: rgba(255, 100, 100, 0.28);
}

.mini-btn.danger text {
  color: var(--error-color);
}
</style>
