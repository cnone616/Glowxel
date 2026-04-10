<template>
  <view class="achievements-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">成就</text>
      <view class="nav-right"></view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content">
      <view class="hero-actions">
        <view class="hero-action-btn" @click="shareAchievements">
          <Icon name="share" :size="28" color="#0F172A" />
          <text class="hero-action-text">分享成就</text>
        </view>
      </view>

      <view class="achievement-hero">
        <view class="hero-copy">
          <text class="hero-badge">荣誉系统</text>
          <text class="hero-title">成就不只是图标，而是值得收藏的像素荣誉卡</text>
          <text class="hero-subtitle"
            >已解锁 {{ unlockedCount }} 项 · 当前完成度 {{ completionRate }}%</text
          >
        </view>
        <view class="hero-stats-card">
          <view class="hero-stat-item">
            <text class="hero-stat-number">{{ unlockedCount }}</text>
            <text class="hero-stat-label">已解锁</text>
          </view>
          <view class="hero-stat-divider"></view>
          <view class="hero-stat-item">
            <text class="hero-stat-number">{{ totalCount }}</text>
            <text class="hero-stat-label">总成就</text>
          </view>
          <view class="hero-stat-divider"></view>
          <view class="hero-stat-item">
            <text class="hero-stat-number">{{ completionRate }}%</text>
            <text class="hero-stat-label">完成度</text>
          </view>
          <view class="hero-progress-track">
            <view
              class="hero-progress-fill"
              :style="{ width: completionRate + '%' }"
            ></view>
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">稀有度层级</text>
          <text class="section-meta">当前成就体系</text>
        </view>
        <view class="rarity-grid">
          <view
            v-for="rarity in rarityOverview"
            :key="rarity.key"
            class="rarity-card"
            :class="rarity.key"
          >
            <view class="rarity-icon-shell">
              <Icon name="trophy" :size="50" color="#FFFFFF" />
            </view>
            <text class="rarity-name">{{ rarity.label }}</text>
            <text class="rarity-percent">{{ rarity.percent }}</text>
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">成就分类</text>
          <text class="section-meta">按系列查看</text>
        </view>
        <view class="category-tabs">
          <view
            v-for="category in categories"
            :key="category.value"
            class="category-tab"
            :class="[category.value, { active: currentCategory === category.value }]"
            @click="currentCategory = category.value"
          >
            <Icon :name="category.icon" :size="28" />
            <text class="category-text">{{ category.label }}</text>
            <text v-if="category.count > 0" class="category-count">{{
              category.count
            }}</text>
          </view>
        </view>
      </view>

      <view class="section-block">
        <view class="section-header glx-section-head">
          <text class="section-title glx-section-title">成就卡片</text>
          <text class="section-meta">点击查看详情</text>
        </view>
        <view class="achievements-grid">
          <view
            v-for="achievement in filteredAchievements"
            :key="achievement.id"
            class="achievement-card"
            :class="{
              unlocked: achievement.unlocked,
              rare: achievement.rarity === 'rare',
              epic: achievement.rarity === 'epic',
              legendary: achievement.rarity === 'legendary',
            }"
            @click="showAchievementDetail(achievement)"
          >
            <view class="achievement-card-top">
              <view class="achievement-icon-shell">
                <Icon
                  :name="achievement.icon"
                  :size="64"
                  :color="achievement.unlocked ? '#FFFFFF' : '#94A3B8'"
                />
                <view
                  v-if="achievement.unlocked && achievement.rarity !== 'common'"
                  class="rarity-glow"
                ></view>
                <view v-if="!achievement.unlocked" class="locked-overlay">
                  <Icon name="lock" :size="34" color="#94A3B8" />
                </view>
              </view>
              <view class="rarity-badge" :class="achievement.rarity">
                <text class="rarity-text">{{
                  getRarityText(achievement.rarity)
                }}</text>
              </view>
            </view>

            <view class="achievement-info">
              <text class="achievement-title">{{ achievement.title }}</text>
              <text class="achievement-desc">{{ achievement.description }}</text>

              <view
                v-if="achievement.progress !== undefined"
                class="achievement-progress"
              >
                <view class="progress-info">
                  <text class="progress-text">进度</text>
                  <text class="progress-percent"
                    >{{
                      Math.round(
                        (achievement.current / achievement.target) * 100,
                      )
                    }}%</text
                  >
                </view>
                <view class="progress-bar-small">
                  <view
                    class="progress-fill-small"
                    :style="{
                      width:
                        (achievement.current / achievement.target) * 100 + '%',
                    }"
                  ></view>
                </view>
                <text class="progress-detail"
                  >{{ achievement.current }}/{{ achievement.target }}</text
                >
              </view>

              <text
                v-if="achievement.unlocked && achievement.unlockedAt"
                class="unlock-time"
              >
                {{ formatUnlockTime(achievement.unlockedAt) }}
              </text>
            </view>
          </view>
        </view>
      </view>
      <view style="height: 80rpx"></view>
    </scroll-view>

    <view
      v-if="selectedAchievement"
      class="modal-overlay"
      @click="selectedAchievement = null"
    >
      <view class="achievement-modal" @click.stop>
        <view class="modal-close" @click="selectedAchievement = null">
          <Icon name="close" :size="32" color="#0F172A" />
        </view>

        <view class="modal-icon" :class="selectedAchievement.rarity">
          <Icon :name="selectedAchievement.icon" :size="88" color="#FFFFFF" />
          <view v-if="selectedAchievement.rarity !== 'common'" class="modal-glow"></view>
          <view class="modal-rarity-chip" :class="selectedAchievement.rarity">
            <text class="rarity-text">{{
              getRarityText(selectedAchievement.rarity)
            }}</text>
          </view>
        </view>

        <text class="modal-title">{{ selectedAchievement.title }}</text>
        <text class="modal-desc">{{ selectedAchievement.description }}</text>

        <view class="modal-meta-grid">
          <view class="modal-meta-card">
            <text class="modal-meta-label">状态</text>
            <text class="modal-meta-value">{{
              selectedAchievement.unlocked ? "已解锁" : "进行中"
            }}</text>
          </view>
          <view class="modal-meta-card">
            <text class="modal-meta-label">稀有度</text>
            <text class="modal-meta-value">{{ selectedAchievementRarityPercent }}</text>
          </view>
        </view>

        <view class="modal-progress">
          <view class="progress-info">
            <text class="progress-text">完成进度</text>
            <text class="progress-percent"
              >{{ selectedAchievement.current }}/{{
                selectedAchievement.target
              }}</text
            >
          </view>
          <view class="progress-bar-modal">
            <view
              class="progress-fill-modal"
              :style="{
                width:
                  (selectedAchievement.current / selectedAchievement.target) *
                    100 +
                  '%',
              }"
            ></view>
          </view>
          <text class="hint-text">{{
            selectedAchievement.unlocked
              ? formatUnlockTime(selectedAchievement.unlockedAt)
              : selectedAchievement.hint || "继续努力，即将解锁！"
          }}</text>
        </view>

        <view class="modal-actions">
          <view class="modal-action-button ghost" @click="selectedAchievement = null">
            <text class="modal-action-text">关闭</text>
          </view>
          <view class="modal-action-button primary" @click="shareSelectedAchievement">
            <Icon name="share" :size="28" color="#FFFFFF" />
            <text class="modal-action-text white">分享成就</text>
          </view>
        </view>
      </view>
    </view>

    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useToast } from "../../composables/useToast.js";
import { useUserStore } from "../../store/user.js";
import { useProjectStore } from "../../store/project.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";

// 成就定义（静态配置）
const ACHIEVEMENT_DEFS = [
  {
    id: "1",
    category: "create",
    title: "初出茅庐",
    description: "完成第一个作品",
    icon: "picture",
    rarity: "common",
    key: "works_count",
    target: 1,
  },
  {
    id: "2",
    category: "create",
    title: "作品达人",
    description: "创作10个作品",
    icon: "gallery",
    rarity: "common",
    key: "works_count",
    target: 10,
  },
  {
    id: "3",
    category: "create",
    title: "像素大师",
    description: "创作50个作品",
    icon: "crown",
    rarity: "rare",
    key: "works_count",
    target: 50,
  },
  {
    id: "4",
    category: "create",
    title: "传奇创作者",
    description: "创作100个作品",
    icon: "trophy",
    rarity: "legendary",
    key: "works_count",
    target: 100,
  },
  {
    id: "5",
    category: "social",
    title: "社交新手",
    description: "获得第一个粉丝",
    icon: "heart",
    rarity: "common",
    key: "followers_count",
    target: 1,
  },
  {
    id: "6",
    category: "social",
    title: "人气作者",
    description: "获得100个粉丝",
    icon: "users",
    rarity: "rare",
    key: "followers_count",
    target: 100,
  },
  {
    id: "7",
    category: "social",
    title: "点赞收割机",
    description: "作品获得1000个点赞",
    icon: "thumbs-up",
    rarity: "epic",
    key: "total_likes",
    target: 1000,
  },
  {
    id: "8",
    category: "skill",
    title: "色彩搭配师",
    description: "使用超过20种颜色创作",
    icon: "palette",
    rarity: "common",
    key: "color_count",
    target: 20,
  },
  {
    id: "9",
    category: "skill",
    title: "精密工匠",
    description: "完成超过1000像素的作品",
    icon: "target",
    rarity: "rare",
    key: "max_pixels",
    target: 1000,
  },
  {
    id: "10",
    category: "special",
    title: "夜猫子",
    description: "在深夜12点后完成创作",
    icon: "moon",
    rarity: "epic",
    key: "night_create",
    target: 1,
  },
  {
    id: "11",
    category: "special",
    title: "完美主义者",
    description: "连续7天都有创作",
    icon: "calendar",
    rarity: "legendary",
    key: "streak_days",
    target: 7,
  },
];

export default {
  mixins: [statusBarMixin],
  components: { Icon, Toast },

  data() {
    return {
      toast: null,
      currentCategory: "all",
      selectedAchievement: null,
      categories: [
        { value: "all", label: "全部", icon: "grid", count: 0 },
        { value: "create", label: "创作", icon: "palette", count: 0 },
        { value: "social", label: "社交", icon: "users", count: 0 },
        { value: "skill", label: "技能", icon: "star", count: 0 },
        { value: "special", label: "特殊", icon: "award", count: 0 },
      ],
      achievements: [],
    };
  },

  computed: {
    filteredAchievements() {
      if (this.currentCategory === "all") return this.achievements;
      return this.achievements.filter(
        (a) => a.category === this.currentCategory,
      );
    },
    rarityOverview() {
      return [
        { key: "common", label: "普通", percent: "60%" },
        { key: "rare", label: "稀有", percent: "25%" },
        { key: "epic", label: "史诗", percent: "10%" },
        { key: "legendary", label: "传说", percent: "5%" },
      ];
    },
    unlockedCount() {
      return this.achievements.filter((a) => a.unlocked).length;
    },
    totalCount() {
      return this.achievements.length;
    },
    completionRate() {
      return this.totalCount
        ? Math.round((this.unlockedCount / this.totalCount) * 100)
        : 0;
    },
    selectedAchievementRarityPercent() {
      if (!this.selectedAchievement) return "";
      if (this.selectedAchievement.rarity === "common") return "60%";
      if (this.selectedAchievement.rarity === "rare") return "25%";
      if (this.selectedAchievement.rarity === "epic") return "10%";
      return "5%";
    },
  },

  onLoad() {
    this.toast = useToast();
    this.$nextTick(() => {
      if (this.$refs.toastRef) this.toast.setToastInstance(this.$refs.toastRef);
    });
    this.buildAchievements();
    this.updateCategoryCounts();
  },

  methods: {
    handleBack() {
      uni.navigateBack();
    },

    buildAchievements() {
      const userStore = useUserStore();
      const projectStore = useProjectStore();

      // 从用户数据取指标
      const stats = {
        works_count: userStore.works_count || 0,
        followers_count: userStore.followers_count || 0,
        total_likes: userStore.total_likes || 0,
        color_count: this._getMaxColorCount(projectStore),
        max_pixels: this._getMaxPixels(projectStore),
        night_create: uni.getStorageSync("achievement_night_create") || 0,
        streak_days: uni.getStorageSync("achievement_streak_days") || 0,
      };

      // 已解锁记录（本地持久化）
      const unlocked = uni.getStorageSync("achievements_unlocked") || {};

      this.achievements = ACHIEVEMENT_DEFS.map((def) => {
        const current = Math.min(stats[def.key] || 0, def.target);
        const wasUnlocked = !!unlocked[def.id];
        const nowUnlocked = current >= def.target;

        // 新解锁：写入本地存储
        if (nowUnlocked && !wasUnlocked) {
          unlocked[def.id] = Date.now();
          uni.setStorageSync("achievements_unlocked", unlocked);
        }

        return {
          ...def,
          current,
          progress: current,
          unlocked: nowUnlocked,
          unlockedAt: unlocked[def.id] || null,
          hint: nowUnlocked ? null : `还差 ${def.target - current} 即可解锁`,
        };
      });
    },

    _getMaxColorCount(projectStore) {
      let max = 0;
      for (const p of projectStore.projects) {
        if (p.palette && p.palette.length > max) max = p.palette.length;
      }
      return max;
    },

    _getMaxPixels(projectStore) {
      let max = 0;
      for (const p of projectStore.projects) {
        const pixels = (p.width || 0) * (p.height || 0);
        if (pixels > max) max = pixels;
      }
      return max;
    },

    updateCategoryCounts() {
      this.categories[0].count = this.achievements.length;
      this.categories[1].count = this.achievements.filter(
        (a) => a.category === "create",
      ).length;
      this.categories[2].count = this.achievements.filter(
        (a) => a.category === "social",
      ).length;
      this.categories[3].count = this.achievements.filter(
        (a) => a.category === "skill",
      ).length;
      this.categories[4].count = this.achievements.filter(
        (a) => a.category === "special",
      ).length;
    },

    showAchievementDetail(achievement) {
      this.selectedAchievement = achievement;
    },
    shareAchievements() {
      const shareText = `我在 Glowxel 已解锁 ${this.unlockedCount} 项成就，当前完成度 ${this.completionRate}%`;
      uni.setClipboardData({
        data: shareText,
        success: () => {
          this.toast.showSuccess("成就文案已复制");
        },
        fail: () => {
          this.toast.showError("复制失败");
        },
      });
    },

    shareSelectedAchievement() {
      if (!this.selectedAchievement) return;
      const shareText = `我在 Glowxel 的${this.getRarityText(this.selectedAchievement.rarity)}成就「${this.selectedAchievement.title}」进度为 ${this.selectedAchievement.current}/${this.selectedAchievement.target}`;
      uni.setClipboardData({
        data: shareText,
        success: () => {
          this.toast.showSuccess("成就文案已复制");
        },
        fail: () => {
          this.toast.showError("复制失败");
        },
      });
    },

    getIconColor(achievement) {
      if (!achievement.unlocked) return "#777777";
      const map = { rare: "#4F7FFF", epic: "#9333EA", legendary: "#F59E0B" };
      return map[achievement.rarity] || "var(--nb-ink)";
    },

    getRarityText(rarity) {
      return (
        { common: "普通", rare: "稀有", epic: "史诗", legendary: "传说" }[
          rarity
        ] || "普通"
      );
    },

    formatUnlockTime(timestamp) {
      return new Date(timestamp).toLocaleDateString() + " 解锁";
    },
  },
};
</script>

<style scoped>
.achievements-page {
  height: 100vh;
  background:
    radial-gradient(circle at top right, rgba(167, 139, 250, 0.12), transparent 34%),
    linear-gradient(180deg, #f8f9fd 0%, #eef3fb 100%);
  display: flex;
  flex-direction: column;
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background: rgba(255, 255, 255, 0.86);
  border-bottom: 2rpx solid rgba(148, 163, 184, 0.16);
  backdrop-filter: none;
  position: relative;
}

.nav-left,
.nav-right {
  position: absolute;
  width: 80rpx;
  display: flex;
  align-items: center;
}

.nav-left {
  left: 32rpx;
  justify-content: flex-start;
}

.nav-right {
  right: 32rpx;
  justify-content: flex-end;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 700;
  color: #0f172a;
}

.content {
  flex: 1;
  padding: 28rpx 28rpx 0;
  box-sizing: border-box;
}

.hero-actions {
  display: flex;
  justify-content: flex-end;
  margin-bottom: 20rpx;
}

.hero-action-btn {
  min-height: 68rpx;
  padding: 0 24rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 10rpx;
  background: #ffffff;
  border: 4rpx solid #000000;
  border-radius: 16rpx;
  box-shadow: none;
}

.hero-action-text {
  font-size: 24rpx;
  font-weight: 700;
  color: #0f172a;
}

.achievement-hero {
  margin-bottom: 28rpx;
}

.hero-copy {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
  margin-bottom: 20rpx;
}

.hero-badge {
  align-self: flex-start;
  padding: 8rpx 18rpx;
  border-radius: 999rpx;
  background: rgba(79, 127, 255, 0.12);
  color: #4f7fff;
  font-size: 22rpx;
  font-weight: 600;
}

.hero-title {
  font-size: 42rpx;
  line-height: 1.35;
  font-weight: 700;
  color: #0f172a;
}

.hero-subtitle {
  font-size: 24rpx;
  color: #64748b;
}

.hero-stats-card {
  padding: 28rpx;
  border-radius: 32rpx;
  background: linear-gradient(145deg, #ffffff 0%, #f7faff 100%);
  border: 2rpx solid rgba(148, 163, 184, 0.14);
  box-shadow: 0 16rpx 44rpx rgba(15, 23, 42, 0.05);
  display: flex;
  align-items: center;
  flex-wrap: wrap;
}

.hero-stat-item {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
}

.hero-stat-number {
  font-size: 38rpx;
  font-weight: 700;
  color: #0f172a;
}

.hero-stat-label {
  font-size: 24rpx;
  color: #64748b;
}

.hero-stat-divider {
  width: 2rpx;
  height: 60rpx;
  background: rgba(148, 163, 184, 0.18);
  margin: 0 32rpx;
}

.hero-progress-track {
  width: 100%;
  height: 16rpx;
  border-radius: 999rpx;
  background: #e2e8f0;
  overflow: hidden;
  margin-top: 24rpx;
}

.hero-progress-fill {
  height: 100%;
  border-radius: inherit;
  background: linear-gradient(90deg, #4f7fff 0%, #8b5cf6 100%);
}

.section-block {
  margin-bottom: 28rpx;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 4rpx 8rpx 16rpx;
}

.section-title {
  font-size: 30rpx;
  font-weight: 700;
  color: #0f172a;
}

.section-meta {
  font-size: 22rpx;
  color: #64748b;
}

.rarity-grid,
.achievements-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 18rpx;
}

.rarity-card {
  border-radius: 28rpx;
  padding: 24rpx 16rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  border: 2rpx solid transparent;
}

.rarity-icon-shell {
  width: 96rpx;
  height: 96rpx;
  border-radius: 26rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 14rpx 30rpx rgba(15, 23, 42, 0.12);
}

.rarity-name {
  font-size: 28rpx;
  font-weight: 700;
}

.rarity-percent {
  font-size: 22rpx;
  color: #64748b;
}

.rarity-card.common {
  background: linear-gradient(180deg, rgba(156, 163, 175, 0.12), rgba(156, 163, 175, 0.04));
  border-color: rgba(156, 163, 175, 0.18);
}

.rarity-card.common .rarity-icon-shell {
  background: linear-gradient(135deg, #9ca3af 0%, #6b7280 100%);
}

.rarity-card.rare {
  background: linear-gradient(180deg, rgba(96, 165, 250, 0.12), rgba(96, 165, 250, 0.04));
  border-color: rgba(96, 165, 250, 0.18);
}

.rarity-card.rare .rarity-icon-shell {
  background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
}

.rarity-card.epic {
  background: linear-gradient(180deg, rgba(167, 139, 250, 0.12), rgba(167, 139, 250, 0.04));
  border-color: rgba(167, 139, 250, 0.18);
}

.rarity-card.epic .rarity-icon-shell {
  background: linear-gradient(135deg, #a78bfa 0%, #8b5cf6 100%);
}

.rarity-card.legendary {
  background: linear-gradient(180deg, rgba(251, 191, 36, 0.12), rgba(251, 191, 36, 0.04));
  border-color: rgba(251, 191, 36, 0.18);
}

.rarity-card.legendary .rarity-icon-shell {
  background: linear-gradient(135deg, #fbbf24 0%, #f59e0b 100%);
}

.category-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
}

.category-tab {
  padding: 16rpx 22rpx;
  border-radius: 999rpx;
  border: 2rpx solid rgba(148, 163, 184, 0.18);
  background: rgba(255, 255, 255, 0.86);
  display: flex;
  align-items: center;
  gap: 10rpx;
}

.category-tab.active {
  box-shadow: 0 10rpx 24rpx rgba(15, 23, 42, 0.08);
}

.category-tab.all.active {
  color: #6b7280;
  border-color: rgba(107, 114, 128, 0.24);
}

.category-tab.create.active {
  color: #4f7fff;
  border-color: rgba(79, 127, 255, 0.26);
}

.category-tab.social.active {
  color: #00d9d9;
  border-color: rgba(0, 217, 217, 0.26);
}

.category-tab.skill.active {
  color: #8b5cf6;
  border-color: rgba(139, 92, 246, 0.26);
}

.category-tab.special.active {
  color: #f59e0b;
  border-color: rgba(245, 158, 11, 0.26);
}

.category-text,
.category-count {
  font-size: 24rpx;
  font-weight: 600;
}

.achievement-card {
  position: relative;
  padding: 24rpx;
  border-radius: 32rpx;
  border: 2rpx solid rgba(156, 163, 175, 0.18);
  background: linear-gradient(180deg, rgba(156, 163, 175, 0.12), rgba(255, 255, 255, 0.92));
  box-shadow: 0 16rpx 40rpx rgba(15, 23, 42, 0.05);
}

.achievement-card.rare {
  border-color: rgba(79, 127, 255, 0.18);
  background: linear-gradient(180deg, rgba(79, 127, 255, 0.12), rgba(255, 255, 255, 0.92));
}

.achievement-card.epic {
  border-color: rgba(139, 92, 246, 0.18);
  background: linear-gradient(180deg, rgba(139, 92, 246, 0.12), rgba(255, 255, 255, 0.92));
}

.achievement-card.legendary {
  border-color: rgba(245, 158, 11, 0.18);
  background: linear-gradient(180deg, rgba(245, 158, 11, 0.12), rgba(255, 255, 255, 0.92));
}

.achievement-card:not(.unlocked) {
  opacity: 0.82;
}

.achievement-card-top {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16rpx;
  margin-bottom: 20rpx;
}

.achievement-icon-shell {
  width: 112rpx;
  height: 112rpx;
  border-radius: 30rpx;
  background: linear-gradient(135deg, #9ca3af 0%, #6b7280 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  box-shadow: 0 16rpx 30rpx rgba(15, 23, 42, 0.14);
}

.achievement-card.rare .achievement-icon-shell {
  background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
}

.achievement-card.epic .achievement-icon-shell {
  background: linear-gradient(135deg, #a78bfa 0%, #8b5cf6 100%);
}

.achievement-card.legendary .achievement-icon-shell {
  background: linear-gradient(135deg, #fbbf24 0%, #f59e0b 100%);
}

.rarity-glow {
  position: absolute;
  inset: -6rpx;
  border-radius: inherit;
  background: rgba(255, 255, 255, 0.24);
}

.locked-overlay {
  position: absolute;
  inset: 0;
  background: rgba(248, 250, 252, 0.82);
  border-radius: inherit;
  display: flex;
  align-items: center;
  justify-content: center;
}

.achievement-info {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.achievement-title {
  font-size: 30rpx;
  font-weight: 700;
  color: #0f172a;
}

.achievement-desc {
  font-size: 24rpx;
  line-height: 1.5;
  color: #64748b;
  min-height: 72rpx;
}

.achievement-progress,
.modal-progress {
  display: flex;
  flex-direction: column;
  gap: 10rpx;
}

.progress-info {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.progress-text,
.progress-detail,
.unlock-time,
.hint-text,
.modal-meta-label {
  font-size: 22rpx;
  color: #64748b;
}

.progress-percent,
.modal-meta-value {
  font-size: 24rpx;
  font-weight: 700;
  color: #0f172a;
}

.progress-bar-small,
.progress-bar-modal {
  height: 14rpx;
  border-radius: 999rpx;
  background: #e2e8f0;
  overflow: hidden;
}

.progress-fill-small,
.progress-fill-modal {
  height: 100%;
  border-radius: inherit;
  background: linear-gradient(90deg, #4f7fff 0%, #8b5cf6 100%);
}

.rarity-badge,
.modal-rarity-chip {
  align-self: flex-start;
  padding: 8rpx 16rpx;
  border-radius: 999rpx;
}

.rarity-badge.common,
.modal-rarity-chip.common {
  background: rgba(156, 163, 175, 0.16);
  color: #6b7280;
}

.rarity-badge.rare,
.modal-rarity-chip.rare {
  background: rgba(96, 165, 250, 0.16);
  color: #3b82f6;
}

.rarity-badge.epic,
.modal-rarity-chip.epic {
  background: rgba(167, 139, 250, 0.16);
  color: #8b5cf6;
}

.rarity-badge.legendary,
.modal-rarity-chip.legendary {
  background: rgba(251, 191, 36, 0.18);
  color: #d97706;
}

.rarity-text {
  font-size: 22rpx;
  font-weight: 700;
}

.modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(15, 23, 42, 0.46);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  z-index: 20;
}

.achievement-modal {
  width: 100%;
  max-width: 640rpx;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  background: #ffffff;
  padding: 40rpx 32rpx 32rpx;
  position: relative;
  box-shadow: var(--nb-shadow-strong);
}

.modal-close {
  position: absolute;
  top: 24rpx;
  right: 24rpx;
  width: 64rpx;
  height: 64rpx;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  background: #ffffff;
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
  display: flex;
  align-items: center;
  justify-content: center;
}

.modal-icon {
  width: 160rpx;
  height: 160rpx;
  margin: 0 auto 28rpx;
  border-radius: 42rpx;
  background: linear-gradient(135deg, #9ca3af 0%, #6b7280 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  box-shadow: 0 24rpx 48rpx rgba(15, 23, 42, 0.18);
}

.modal-icon.rare {
  background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
}

.modal-icon.epic {
  background: linear-gradient(135deg, #a78bfa 0%, #8b5cf6 100%);
}

.modal-icon.legendary {
  background: linear-gradient(135deg, #fbbf24 0%, #f59e0b 100%);
}

.modal-glow {
  position: absolute;
  inset: -8rpx;
  border-radius: inherit;
  background: rgba(255, 255, 255, 0.24);
}

.modal-rarity-chip {
  position: absolute;
  top: -12rpx;
  right: -12rpx;
}

.modal-title {
  display: block;
  text-align: center;
  font-size: 38rpx;
  font-weight: 700;
  color: #0f172a;
  margin-bottom: 12rpx;
}

.modal-desc {
  display: block;
  text-align: center;
  font-size: 24rpx;
  line-height: 1.55;
  color: #64748b;
  margin-bottom: 24rpx;
}

.modal-meta-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16rpx;
  margin-bottom: 24rpx;
}

.modal-meta-card {
  border-radius: 0;
  background: rgba(148, 163, 184, 0.08);
  padding: 20rpx;
  display: flex;
  flex-direction: column;
  gap: 10rpx;
}

.modal-progress {
  margin-bottom: 28rpx;
}

.modal-actions {
  display: flex;
  gap: 16rpx;
}

.modal-action-button {
  flex: 1;
  min-height: 88rpx;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10rpx;
}

.modal-action-button.ghost {
  background: #ffffff;
}

.modal-action-button.primary {
  background: var(--nb-yellow);
}

.modal-action-text {
  font-size: 26rpx;
  font-weight: 700;
  color: #0f172a;
}

.modal-action-text.white {
  color: #000000;
}
</style>
