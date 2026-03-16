<template>
  <div class="login-page">
    <div class="login-card">
      <h1>Glowxel 管理后台</h1>
      <a-form :model="form" @finish="handleLogin">
        <a-form-item name="username" :rules="[{ required: true, message: '请输入用户名' }]">
          <a-input v-model:value="form.username" placeholder="用户名" size="large" />
        </a-form-item>
        <a-form-item name="password" :rules="[{ required: true, message: '请输入密码' }]">
          <a-input-password v-model:value="form.password" placeholder="密码" size="large" />
        </a-form-item>
        <a-form-item>
          <a-button type="primary" html-type="submit" block size="large" :loading="loading">登录</a-button>
        </a-form-item>
      </a-form>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { message } from 'ant-design-vue'
import http from '@/api/http.js'

const router = useRouter()
const form = ref({ username: '', password: '' })
const loading = ref(false)

async function handleLogin() {
  loading.value = true
  try {
    const res = await http.post('/user/admin-login', form.value)
    if (res.data?.token) {
      localStorage.setItem('admin_token', res.data.token)
      message.success('登录成功')
      router.push('/dashboard')
    } else {
      message.error(res.message || '登录失败')
    }
  } catch (e) {
    message.error('网络错误')
  }
  loading.value = false
}
</script>

<style scoped>
.login-page {
  min-height: 100vh; display: flex; align-items: center; justify-content: center;
  background: #f5f5f5;
}
.login-card {
  width: 380px; background: #fff; border-radius: 12px; padding: 40px 32px;
  box-shadow: 0 2px 12px rgba(0,0,0,0.06);
}
.login-card h1 {
  text-align: center; font-size: 22px; font-weight: 700; margin-bottom: 32px; color: #1a1a1a;
}
</style>

