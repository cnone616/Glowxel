<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Achievements</span>
      <h1 class="glx-page-shell__title">成就中心</h1>
      <p class="glx-page-shell__desc">
        成就页不该被直接删掉，所以我把这条正式入口恢复回来，继续承接个人成长展示。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">已解锁</span>
          <strong class="glx-hero-metric__value">{{ unlockedCount }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">总成就</span>
          <strong class="glx-hero-metric__value">{{ achievements.length }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">积分</span>
          <strong class="glx-hero-metric__value">{{ totalPoints }}</strong>
        </article>
      </div>
    </section>

    <section class="glx-grid glx-grid--three">
      <article v-for="achievement in achievements" :key="achievement.id" class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <strong class="glx-section-title">{{ achievement.name }}</strong>
          <span class="glx-chip" :class="rarityClass(achievement.rarity)">{{ achievement.rarity }}</span>
        </div>
        <p class="glx-page-shell__desc">{{ achievement.description }}</p>
        <div class="glx-kv-grid">
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">进度</span>
            <strong class="glx-kv-card__value">{{ achievement.progress }}/{{ achievement.target }}</strong>
          </div>
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">积分</span>
            <strong class="glx-kv-card__value">{{ achievement.points }}</strong>
          </div>
        </div>
        <span class="achievement-state" :class="{ 'is-unlocked': achievement.unlocked }">
          {{ achievement.unlocked ? "已解锁" : "未解锁" }}
        </span>
      </article>
    </section>
  </div>
</template>

<script setup>
import { computed } from "vue";

const achievements = [
  { id: 1, name: "初出茅庐", description: "完成第一个作品", progress: 1, target: 1, points: 10, rarity: "common", unlocked: true },
  { id: 2, name: "创作达人", description: "完成 10 个作品", progress: 6, target: 10, points: 50, rarity: "rare", unlocked: false },
  { id: 3, name: "人气新星", description: "获得 100 个点赞", progress: 89, target: 100, points: 30, rarity: "rare", unlocked: true },
  { id: 4, name: "收藏家", description: "收藏 20 个作品", progress: 5, target: 20, points: 20, rarity: "common", unlocked: false },
  { id: 5, name: "社交达人", description: "关注 50 个用户", progress: 45, target: 50, points: 25, rarity: "common", unlocked: false },
  { id: 6, name: "像素大师", description: "完成 100 个作品", progress: 6, target: 100, points: 200, rarity: "legendary", unlocked: false },
  { id: 7, name: "连续创作者", description: "连续 7 天创作", progress: 7, target: 7, points: 40, rarity: "rare", unlocked: true },
  { id: 8, name: "色彩专家", description: "使用超过 50 种颜色", progress: 32, target: 50, points: 75, rarity: "epic", unlocked: false },
];

const unlockedCount = computed(() => {
  return achievements.filter((item) => item.unlocked).length;
});

const totalPoints = computed(() => {
  return achievements.reduce((sum, item) => {
    if (item.unlocked) {
      return sum + item.points;
    }
    return sum;
  }, 0);
});

function rarityClass(rarity) {
  if (rarity === "legendary") {
    return "glx-chip--danger";
  }
  if (rarity === "epic") {
    return "glx-chip--blue";
  }
  if (rarity === "rare") {
    return "glx-chip--green";
  }
  return "glx-chip--yellow";
}
</script>

<style scoped>
.achievement-state {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 34px;
  padding: 6px 12px;
  border: 2px solid #111111;
  font-size: 12px;
  font-weight: 900;
  color: var(--nb-text-secondary);
}

.achievement-state.is-unlocked {
  background: var(--nb-green);
  color: var(--nb-ink);
}
</style>
