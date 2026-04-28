<template>
  <div class="glx-page-shell settings-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Settings</span>
      <h1 class="glx-page-shell__title">设置</h1>
      <p class="glx-page-shell__desc">
        设置页需要保留，它继续承接资料编辑、个人中心回跳和退出登录。
      </p>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <router-link to="/settings/profile" class="glx-list-card settings-link">
        <div class="glx-list-card__copy">
          <strong class="glx-list-card__title">编辑资料</strong>
          <span class="glx-list-card__desc">修改昵称和个人简介。</span>
        </div>
      </router-link>
      <router-link to="/profile" class="glx-list-card settings-link">
        <div class="glx-list-card__copy">
          <strong class="glx-list-card__title">我的主页</strong>
          <span class="glx-list-card__desc">返回个人中心继续管理作品和收藏。</span>
        </div>
      </router-link>
      <router-link to="/cloud-sync" class="glx-list-card settings-link">
        <div class="glx-list-card__copy">
          <strong class="glx-list-card__title">云端同步</strong>
          <span class="glx-list-card__desc">查看项目同步状态和最近同步时间。</span>
        </div>
      </router-link>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">账户操作</h2>
        <span class="glx-section-meta">当前会话</span>
      </div>
      <div class="glx-inline-actions">
        <button type="button" class="glx-button glx-button--danger" @click="handleLogout">
          退出登录
        </button>
      </div>
    </section>
  </div>
</template>

<script setup>
import { useRouter } from "vue-router";
import { useUserStore } from "@/stores/user.js";

const router = useRouter();
const userStore = useUserStore();

function handleLogout() {
  userStore.logout();
  router.push("/login");
}
</script>

<style scoped>
.settings-page {
  max-width: 900px;
}

.settings-link {
  text-decoration: none;
  color: inherit;
}
</style>
