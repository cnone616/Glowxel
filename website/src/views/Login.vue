<template>
  <div class="login">
    <div class="login-card">
      <h1>登录 Glowxel</h1>
      <p class="subtitle">使用账号密码登录</p>
      <form @submit.prevent="handleLogin">
        <input v-model="form.username" type="text" placeholder="用户名" required />
        <input v-model="form.password" type="password" placeholder="密码" required />
        <button type="submit" class="login-btn" :disabled="loading">{{ loading ? '登录中...' : '登录' }}</button>
      </form>
      <p class="error" v-if="error">{{ error }}</p>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { userAPI } from '@/api/index.js'

const router = useRouter()
const form = ref({ username: '', password: '' })
const loading = ref(false)
const error = ref('')

async function handleLogin() {
  loading.value = true; error.value = ''
  try {
    const res = await userAPI.login(form.value)
    if (res.success) {
      localStorage.setItem('auth_token', res.data.token)
      localStorage.setItem('user_info', JSON.stringify(res.data.user))
      router.push('/')
    } else { error.value = res.message }
  } catch (e) { error.value = '网络错误' }
  loading.value = false
}
</script>

<style scoped>
.login { min-height: calc(100vh - 120px); display: flex; align-items: center; justify-content: center; padding: 20px; }
.login-card { width: 100%; max-width: 380px; background: #fafafa; border: 1px solid #f0f0f0; border-radius: 16px; padding: 40px 32px; }
.login-card h1 { font-size: 24px; font-weight: 700; color: #1a1a1a; text-align: center; }
.subtitle { text-align: center; color: #999; font-size: 14px; margin: 8px 0 28px; }
form { display: flex; flex-direction: column; gap: 14px; }
input { padding: 12px 14px; border: 1px solid #e0e0e0; border-radius: 8px; font-size: 14px; outline: none; }
input:focus { border-color: #1a1a1a; }
.login-btn { padding: 12px; border: none; border-radius: 8px; background: #1a1a1a; color: #fff; font-size: 15px; font-weight: 600; cursor: pointer; }
.login-btn:hover { background: #333; }
.login-btn:disabled { background: #ccc; cursor: not-allowed; }
.error { text-align: center; color: #e53e3e; font-size: 13px; margin-top: 12px; }
</style>

