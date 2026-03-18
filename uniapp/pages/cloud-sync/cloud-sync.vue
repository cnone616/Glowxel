<template>
  <view class="cloud-sync-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
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
              :color="
                syncStatus.enabled ? '#FFFFFF' : 'var(--color-text-disabled)'
              "
            />
          </view>
          <view class="status-info">
            <text class="status-title">{{
              syncStatus.enabled ? "云同步已开启" : "云同步已关闭"
            }}</text>
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
            <Icon
              name="picture"
              :size="40"
              color="var(--color-brand-primary)"
            />
            <text class="stat-number">{{ syncStats.projects }}</text>
            <text class="stat-label">作品</text>
          </view>

          <view class="stat-item">
            <Icon
              name="favorite"
              :size="40"
              color="var(--color-brand-primary)"
            />
            <text class="stat-number">{{ syncStats.favorites }}</text>
            <text class="stat-label">收藏</text>
          </view>

          <view class="stat-item">
            <Icon
              name="setting"
              :size="40"
              color="var(--color-brand-primary)"
            />
            <text class="stat-number">{{ syncStats.settings }}</text>
            <text class="stat-label">设置</text>
          </view>

          <view class="stat-item">
            <Icon
              name="database"
              :size="40"
              color="var(--color-brand-primary)"
            />
            <text class="stat-number">{{
              formatDataSize(syncStats.dataSize)
            }}</text>
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
              <Icon
                name="refresh"
                :size="40"
                color="var(--color-brand-primary)"
              />
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
              <Icon
                name="image"
                :size="40"
                color="var(--color-brand-primary)"
              />
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
              <Icon
                name="refresh"
                :size="40"
                color="var(--color-brand-primary)"
              />
            </view>
            <view class="action-info">
              <text class="action-label">立即同步</text>
              <text class="action-desc">手动触发数据同步</text>
            </view>
          </view>
          <view class="action-right">
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>

        <view class="action-item" @click="downloadFromCloud">
          <view class="action-left">
            <view class="action-icon">
              <Icon
                name="download"
                :size="40"
                color="var(--color-brand-primary)"
              />
            </view>
            <view class="action-info">
              <text class="action-label">从云端恢复</text>
              <text class="action-desc">下载云端数据到本地</text>
            </view>
          </view>
          <view class="action-right">
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
          </view>
        </view>

        <view class="action-item" @click="uploadToCloud">
          <view class="action-left">
            <view class="action-icon">
              <Icon
                name="upload"
                :size="40"
                color="var(--color-brand-primary)"
              />
            </view>
            <view class="action-info">
              <text class="action-label">上传到云端</text>
              <text class="action-desc">将本地数据备份到云端</text>
            </view>
          </view>
          <view class="action-right">
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
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
            <Icon
              name="arrow-right"
              :size="32"
              color="var(--color-text-disabled)"
            />
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
          <text class="help-text"
            >• 云同步可以在多个设备间同步您的作品和设置</text
          >
          <text class="help-text">• 数据采用端到端加密，确保隐私安全</text>
          <text class="help-text">• 建议在WiFi环境下进行同步以节省流量</text>
          <text class="help-text"
            >• 同步失败时会自动重试，无需担心数据丢失</text
          >
        </view>
      </view>
    </scroll-view>

    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useUserStore } from "../../store/user.js";
import { useProjectStore } from "../../store/project.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import {
  getSyncStatus,
  syncProject,
  batchSyncProjects,
  getCloudProjects,
  deleteCloudProject,
} from "../../api/project.js";

export default {
  mixins: [statusBarMixin],
  components: { Icon, Toast },

  data() {
    return {
      userStore: useUserStore(),
      projectStore: useProjectStore(),
      toast: null,
      syncStatus: {
        enabled: false,
        lastSync: null,
        syncing: false,
      },
      syncStats: {
        projects: 0,
        favorites: 0,
        settings: 1,
        dataSize: 0,
      },
      settings: {
        autoSync: true,
        wifiOnly: true,
        syncThumbnails: false,
      },
      syncLogs: [],
    };
  },

  onLoad() {
    this.toast = useToast();
    this.$nextTick(() => {
      if (this.$refs.toastRef) this.toast.setToastInstance(this.$refs.toastRef);
    });
    this.loadSyncSettings();
    if (this.userStore.isLoggedIn) {
      this.syncStatus.enabled = true;
      this.loadSyncStats();
    }
  },

  methods: {
    handleBack() {
      uni.navigateBack();
    },

    loadSyncSettings() {
      const saved = uni.getStorageSync("sync-settings");
      if (saved) this.settings = { ...this.settings, ...saved };
    },

    saveSyncSettings() {
      uni.setStorageSync("sync-settings", this.settings);
    },

    async loadSyncStats() {
      const res = await getSyncStatus();
      if (res.success) {
        this.syncStats.projects = res.data.count || 0;
        this.syncStatus.lastSync = res.data.lastSync
          ? new Date(res.data.lastSync).getTime()
          : null;
      }
    },

    getStatusDescription() {
      if (!this.userStore.isLoggedIn) return "请先登录以使用云同步";
      if (!this.syncStatus.enabled) return "开启后可在多设备间同步数据";
      if (this.syncStatus.syncing) return "正在同步数据...";
      return "数据已安全备份到云端";
    },

    async toggleSync(e) {
      if (!this.userStore.isLoggedIn) {
        this.toast.showError("请先登录");
        return;
      }
      this.syncStatus.enabled = e.detail.value;
      if (this.syncStatus.enabled) {
        await this.loadSyncStats();
        this.toast.showSuccess("云同步已开启");
      } else {
        uni.showModal({
          title: "关闭云同步",
          content: "关闭后将无法在多设备间同步数据，确定要关闭吗？",
          success: (res) => {
            if (!res.confirm) this.syncStatus.enabled = true;
            else this.toast.showInfo("云同步已关闭");
          },
        });
      }
    },

    toggleAutoSync(e) {
      this.settings.autoSync = e.detail.value;
      this.saveSyncSettings();
      this.toast.showInfo(
        this.settings.autoSync ? "自动同步已开启" : "自动同步已关闭",
      );
    },

    toggleWifiOnly(e) {
      this.settings.wifiOnly = e.detail.value;
      this.saveSyncSettings();
    },

    toggleSyncThumbnails(e) {
      this.settings.syncThumbnails = e.detail.value;
      this.saveSyncSettings();
    },

    async manualSync() {
      if (this.syncStatus.syncing) return;
      if (!this.userStore.isLoggedIn) {
        this.toast.showError("请先登录");
        return;
      }
      this.syncStatus.syncing = true;
      uni.showLoading({ title: "同步中..." });
      try {
        const projects = this.projectStore.projects;
        if (projects.length === 0) {
          this.toast.showInfo("没有需要同步的项目");
          return;
        }
        const items = projects.map((p) => {
          const pixels = this.projectStore.getProjectPixels(p.id);
          return {
            project: p,
            pixels: pixels.size > 0 ? Array.from(pixels.entries()) : null,
          };
        });
        const res = await batchSyncProjects(items);
        if (res.success) {
          this.syncStatus.lastSync = Date.now();
          this.addSyncLog(`同步 ${projects.length} 个项目`, "success");
          await this.loadSyncStats();
          this.toast.showSuccess("同步完成");
        } else {
          this.addSyncLog("同步失败", "error", res.message);
          this.toast.showError(res.message || "同步失败");
        }
      } catch (e) {
        this.addSyncLog("同步失败", "error", e.message);
        this.toast.showError("同步失败，请重试");
      } finally {
        this.syncStatus.syncing = false;
        uni.hideLoading();
      }
    },

    async downloadFromCloud() {
      uni.showModal({
        title: "从云端恢复",
        content: "这将用云端数据覆盖本地数据，确定要继续吗？",
        success: async (res) => {
          if (!res.confirm) return;
          uni.showLoading({ title: "恢复中..." });
          try {
            const listRes = await getCloudProjects(1, 100);
            if (!listRes.success) throw new Error(listRes.message);
            const cloudProjects = listRes.data.list || [];
            for (const cp of cloudProjects) {
              this.projectStore.updateProject(cp.id, {
                name: cp.name,
                width: cp.width,
                height: cp.height,
                thumbnail: cp.thumbnail_url,
                progress: cp.progress,
              });
            }
            this.addSyncLog(
              `从云端恢复 ${cloudProjects.length} 个项目`,
              "success",
            );
            this.toast.showSuccess("恢复完成");
          } catch (e) {
            this.addSyncLog("恢复失败", "error", e.message);
            this.toast.showError("恢复失败，请重试");
          } finally {
            uni.hideLoading();
          }
        },
      });
    },

    async uploadToCloud() {
      if (!this.userStore.isLoggedIn) {
        this.toast.showError("请先登录");
        return;
      }
      uni.showLoading({ title: "上传中..." });
      try {
        const projects = this.projectStore.projects;
        const items = projects.map((p) => ({ project: p, pixels: null }));
        const res = await batchSyncProjects(items);
        if (res.success) {
          this.addSyncLog(`上传 ${projects.length} 个项目`, "success");
          await this.loadSyncStats();
          this.toast.showSuccess("上传完成");
        } else {
          throw new Error(res.message);
        }
      } catch (e) {
        this.addSyncLog("上传失败", "error", e.message);
        this.toast.showError("上传失败，请重试");
      } finally {
        uni.hideLoading();
      }
    },

    clearCloudData() {
      uni.showModal({
        title: "清除云端数据",
        content: "这将永久删除所有云端备份数据，此操作无法撤销！",
        confirmColor: "#E74C3C",
        success: async (res) => {
          if (!res.confirm) return;
          uni.showLoading({ title: "清除中..." });
          try {
            const listRes = await getCloudProjects(1, 100);
            if (listRes.success) {
              for (const p of listRes.data.list || []) {
                await deleteCloudProject(p.id);
              }
            }
            this.syncStats.projects = 0;
            this.addSyncLog("清除云端数据", "success");
            this.toast.showSuccess("云端数据已清除");
          } catch (e) {
            this.addSyncLog("清除失败", "error", e.message);
            this.toast.showError("清除失败，请重试");
          } finally {
            uni.hideLoading();
          }
        },
      });
    },

    addSyncLog(action, status, error = null) {
      this.syncLogs.unshift({
        id: Date.now().toString(),
        action,
        status,
        error,
        timestamp: Date.now(),
      });
      if (this.syncLogs.length > 20) this.syncLogs = this.syncLogs.slice(0, 20);
    },

    viewAllLogs() {
      this.toast.showInfo("同步日志详情功能开发中");
    },

    formatLastSync() {
      if (!this.syncStatus.lastSync) return "从未同步";
      const diff = Date.now() - this.syncStatus.lastSync;
      if (diff < 60000) return "刚刚";
      if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`;
      if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`;
      return new Date(this.syncStatus.lastSync).toLocaleDateString();
    },

    formatDataSize(bytes) {
      if (!bytes) return "0B";
      if (bytes < 1024) return bytes + "B";
      if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + "KB";
      return (bytes / (1024 * 1024)).toFixed(1) + "MB";
    },

    formatLogTime(timestamp) {
      const diff = Date.now() - timestamp;
      if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`;
      if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`;
      return new Date(timestamp).toLocaleDateString();
    },

    getLogIcon(status) {
      return status === "success"
        ? "check-circle"
        : status === "error"
          ? "x-circle"
          : "info";
    },

    getLogColor(status) {
      return status === "success"
        ? "var(--color-success)"
        : status === "error"
          ? "var(--color-error)"
          : "var(--color-text-secondary)";
    },
  },
};
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
  background: linear-gradient(
    135deg,
    var(--color-brand-primary),
    var(--color-brand-accent)
  );
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

.sync-settings,
.sync-actions,
.sync-logs,
.help-section {
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

.setting-item,
.action-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 0;
  border-bottom: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.setting-item:last-child,
.action-item:last-child {
  border-bottom: none;
}

.action-item:active {
  background-color: var(--color-app-background);
  margin: 0 -32rpx;
  padding: 24rpx 32rpx;
  border-radius: 12rpx;
}

.setting-left,
.action-left {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
}

.setting-icon,
.action-icon {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: rgba(79, 127, 255, 0.1);
  border-radius: 16rpx;
}

.setting-info,
.action-info {
  flex: 1;
}

.setting-label,
.action-label {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--color-text-primary);
  margin-bottom: 4rpx;
  display: block;
}

.setting-desc,
.action-desc {
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
