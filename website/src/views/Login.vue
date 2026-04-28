<template>
  <div class="glx-page-shell login-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Login</span>
      <h1 class="glx-page-shell__title">登录光格像素工坊</h1>
      <p class="glx-page-shell__desc">
        网站的登录入口需要保留，登录后才能进入作品管理、发布和个人中心。当前页面继续承接仓库里已经存在的账号密码链路。
      </p>
    </section>

    <section class="glx-section-card glx-section-card--stack login-card">
      <div class="glx-form-grid">
        <label class="glx-field">
          <span class="glx-field__label">用户名</span>
          <input
            v-model="form.username"
            class="glx-input"
            type="text"
            autocomplete="username"
            placeholder="请输入用户名"
          />
        </label>

        <label class="glx-field">
          <span class="glx-field__label">密码</span>
          <input
            v-model="form.password"
            class="glx-input"
            type="password"
            autocomplete="current-password"
            placeholder="请输入密码"
          />
        </label>
      </div>

      <div class="glx-inline-actions">
        <button
          type="button"
          class="glx-button glx-button--primary"
          :disabled="userStore.loading"
          @click="handleLogin"
        >
          {{ userStore.loading ? "登录中..." : "登录" }}
        </button>
      </div>

      <p v-if="errorMessage.length > 0" class="login-error">{{ errorMessage }}</p>
    </section>
  </div>
</template>

<script setup>
import { reactive, ref } from "vue";
import { useRoute, useRouter } from "vue-router";
import { useUserStore } from "@/stores/user.js";

const route = useRoute();
const router = useRouter();
const userStore = useUserStore();

const form = reactive({
  username: "",
  password: "",
});
const errorMessage = ref("");

async function handleLogin() {
  errorMessage.value = "";

  if (form.username.trim().length === 0 || form.password.trim().length === 0) {
    errorMessage.value = "请输入用户名和密码";
    return;
  }

  const response = await userStore.loginWithAdmin({
    username: form.username.trim(),
    password: form.password,
  });

  if (response.success) {
    if (typeof route.query.redirect === "string" && route.query.redirect.length > 0) {
      router.push(route.query.redirect);
      return;
    }
    router.push("/profile");
    return;
  }

  if (typeof response.message === "string" && response.message.length > 0) {
    errorMessage.value = response.message;
    return;
  }

  errorMessage.value = "登录失败";
}
</script>

<style scoped>
.login-page {
  max-width: 760px;
  margin: 0 auto;
  padding: 48px 24px 80px;
}

.login-card {
  max-width: 420px;
}

.login-error {
  color: var(--nb-coral);
  font-size: 13px;
  font-weight: 800;
}
</style>
