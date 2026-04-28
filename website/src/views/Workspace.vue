<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Workspace</span>
      <h1 class="glx-page-shell__title">创作中心</h1>
      <p class="glx-page-shell__desc">
        工作台需要继续承接创作入口、云端项目列表和项目管理，不应该被改成只剩说明文案的占位页。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">云端项目</span>
          <strong class="glx-hero-metric__value">{{ userStore.isLoggedIn ? projectStore.totalProjects : 0 }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">草稿</span>
          <strong class="glx-hero-metric__value">{{ userStore.isLoggedIn ? projectStore.draftProjects.length : 0 }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">已发布</span>
          <strong class="glx-hero-metric__value">{{ userStore.isLoggedIn ? projectStore.publishedProjects.length : 0 }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前状态</span>
          <strong class="glx-hero-metric__value">{{ userStore.isLoggedIn ? "已登录" : "未登录" }}</strong>
        </article>
      </div>
    </section>

    <section class="glx-grid glx-grid--two">
      <router-link to="/create?mode=blank" class="workspace-entry">
        <span class="glx-icon-chip glx-icon-chip--yellow workspace-entry__icon">+</span>
        <strong class="workspace-entry__title">新建画布</strong>
        <p class="workspace-entry__desc">从空白画布开始创作，继续走网站端正式创作链。</p>
      </router-link>

      <router-link to="/create?mode=image" class="workspace-entry">
        <span class="glx-icon-chip glx-icon-chip--blue workspace-entry__icon">图</span>
        <strong class="workspace-entry__title">导入图片</strong>
        <p class="workspace-entry__desc">把现有图片送进像素编辑器，再继续细修和保存。</p>
      </router-link>

      <router-link to="/pattern-workbench" class="workspace-entry">
        <span class="glx-icon-chip glx-icon-chip--green workspace-entry__icon">拼</span>
        <strong class="workspace-entry__title">拼豆工作台</strong>
        <p class="workspace-entry__desc">拼豆图纸导入、差异校对和分板整理继续保留。</p>
      </router-link>

      <router-link to="/device-control" class="workspace-entry">
        <span class="glx-icon-chip glx-icon-chip--paper workspace-entry__icon">设</span>
        <strong class="workspace-entry__title">设备发送</strong>
        <p class="workspace-entry__desc">设备连接、配网、参数和画板模式入口继续留在网站端。</p>
      </router-link>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">我的云端项目</h2>
        <span class="glx-section-meta">{{ userStore.isLoggedIn ? `${projectStore.totalProjects} 个` : "需登录" }}</span>
      </div>

      <div v-if="!userStore.isLoggedIn" class="glx-empty-card">
        <strong class="glx-section-title">登录后可管理项目</strong>
        <p class="glx-page-shell__desc">工作台的项目列表、总览、辅助处理和发布链路都需要登录后继续使用。</p>
        <div class="glx-inline-actions">
          <router-link to="/login" class="glx-button glx-button--primary">去登录</router-link>
        </div>
      </div>

      <div v-else-if="projectStore.loading" class="glx-stack">
        <article v-for="index in 3" :key="index" class="glx-skeleton-card">
          <div class="glx-skeleton workspace-skeleton"></div>
          <div class="glx-skeleton workspace-skeleton"></div>
        </article>
      </div>

      <div v-else-if="projectStore.projects.length === 0" class="glx-empty-card">
        <strong class="glx-section-title">还没有云端项目</strong>
        <p class="glx-page-shell__desc">先保存草稿或从模板开始创作，这里就会出现你的项目列表。</p>
      </div>

      <div v-else class="glx-stack">
        <article v-for="item in projectStore.projects" :key="item.id" class="glx-list-card workspace-project-row">
          <img
            v-if="coverUrl(item.thumbnail_url).length > 0"
            :src="coverUrl(item.thumbnail_url)"
            alt="thumbnail"
            class="workspace-project-row__cover"
          />
          <div v-else class="workspace-project-row__cover workspace-project-row__cover--empty">图</div>
          <div class="glx-list-card__copy">
            <strong class="glx-list-card__title">{{ projectName(item) }}</strong>
            <span class="glx-list-card__desc">{{ projectMeta(item) }}</span>
          </div>
          <div class="glx-inline-actions">
            <router-link :to="`/overview/${item.id}`" class="glx-button glx-button--ghost">总览</router-link>
            <router-link :to="`/editor/${item.id}`" class="glx-button glx-button--ghost">编辑</router-link>
            <button type="button" class="glx-button glx-button--danger" @click="removeProject(item.id)">删除</button>
          </div>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { onMounted } from "vue";
import { useFeedback } from "@/composables/useFeedback.js";
import { useProjectStore } from "@/stores/project.js";
import { useUserStore } from "@/stores/user.js";

const feedback = useFeedback();
const projectStore = useProjectStore();
const userStore = useUserStore();

function coverUrl(value) {
  if (typeof value === "string" && value.length > 0) {
    return value;
  }
  return "";
}

function projectName(item) {
  if (typeof item.name === "string" && item.name.length > 0) {
    return item.name;
  }
  return "未命名项目";
}

function projectMeta(item) {
  const parts = [];

  if (typeof item.width === "number" && typeof item.height === "number") {
    parts.push(`${item.width} × ${item.height}`);
  }

  if (typeof item.status === "string" && item.status.length > 0) {
    parts.push(item.status);
  }

  if (typeof item.updated_at === "string" && item.updated_at.length > 0) {
    parts.push(item.updated_at.slice(0, 10));
  }

  return parts.join(" · ");
}

async function removeProject(id) {
  const response = await projectStore.removeProject(id);
  if (response.success) {
    feedback.success("删除成功", "项目已经从云端项目列表中移除。");
    return;
  }

  feedback.error("删除失败", "项目没有成功删除。");
}

onMounted(async () => {
  await userStore.init();
  if (userStore.isLoggedIn) {
    await projectStore.loadProjects();
  }
});
</script>

<style scoped>
.workspace-entry {
  border: var(--glx-shell-border-strong);
  background: #ffffff;
  box-shadow: var(--glx-shadow-card);
  padding: 22px;
  text-decoration: none;
  color: inherit;
  display: grid;
  gap: 12px;
}

.workspace-entry__icon {
  font-size: 20px;
  font-weight: 900;
}

.workspace-entry__title {
  font-size: 22px;
  font-weight: 900;
  color: var(--nb-ink);
}

.workspace-entry__desc {
  font-size: 14px;
  line-height: 1.8;
  color: var(--nb-text-secondary);
}

.workspace-project-row {
  align-items: center;
}

.workspace-project-row__cover {
  width: 72px;
  height: 72px;
  object-fit: cover;
  border: 2px solid #111111;
  flex-shrink: 0;
}

.workspace-project-row__cover--empty {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background: #f0f0f0;
  font-size: 20px;
  font-weight: 900;
}

.workspace-skeleton {
  min-height: 18px;
}
</style>
