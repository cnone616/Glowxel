<template>
  <div class="login">
    <div class="login-card">
      <h1>登录光格像素工坊</h1>
      <p class="subtitle">当前 Web 端使用账号密码登录</p>
      <form @submit.prevent="handleLogin">
        <input
          v-model="form.username"
          type="text"
          placeholder="用户名"
          required
        />
        <input
          v-model="form.password"
          type="password"
          placeholder="密码"
          required
        />
        <button type="submit" class="login-btn" :disabled="loading">
          {{ loading ? "登录中..." : "登录" }}
        </button>
      </form>
      <p class="error" v-if="error">{{ error }}</p>
    </div>
  </div>
</template>

<script setup>
import { ref } from "vue";
import { useRouter } from "vue-router";
import { userAPI } from "@/api/index.js";

const router = useRouter();
const form = ref({ username: "", password: "" });
const loading = ref(false);
const error = ref("");

async function handleLogin() {
  loading.value = true;
  error.value = "";
  try {
    const res = await userAPI.adminLogin(form.value);
    if (res.success) {
      localStorage.setItem("auth_token", res.data.token);
      localStorage.setItem("user_info", JSON.stringify(res.data.user));
      router.push("/");
    } else {
      error.value = res.message;
    }
  } catch (e) {
    error.value = "网络错误";
  }
  loading.value = false;
}
</script>

<style scoped>
.login {
  min-height: calc(100vh - 120px);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
}
.login-card {
  width: 100%;
  max-width: 380px;
  background: var(--tone-paper-soft);
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  padding: 40px 32px;
  box-shadow: var(--nb-shadow-card);
}
.login-card h1 {
  font-size: 24px;
  font-weight: 800;
  color: var(--nb-ink);
  text-align: center;
}
.subtitle {
  text-align: center;
  color: var(--text-secondary);
  font-size: 14px;
  margin: 8px 0 28px;
}
form {
  display: flex;
  flex-direction: column;
  gap: 14px;
}
input {
  padding: 12px 14px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  font-size: 14px;
  outline: none;
  color: var(--nb-ink);
  background: var(--tone-paper-soft);
}
input:focus {
  border-color: var(--nb-ink);
}
.login-btn {
  padding: 12px;
  border: 2px solid var(--nb-ink);
  border-radius: 0;
  background: var(--nb-yellow);
  color: var(--nb-ink);
  font-size: 15px;
  font-weight: 800;
  cursor: pointer;
  box-shadow: var(--nb-shadow-soft);
}
.login-btn:hover {
  background: var(--color-primary-hover);
}
.login-btn:disabled {
  background: #ccc;
  cursor: not-allowed;
}
.error {
  text-align: center;
  color: var(--nb-coral);
  font-size: 13px;
  margin-top: 12px;
}
</style>
