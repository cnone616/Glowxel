<template>
  <div class="settings">
    <div class="container">
      <button class="back-btn" @click="$router.back()">← 返回</button>
      <h1 class="page-title">设置</h1>
      <div class="settings-card">
        <div class="section-title">账户</div>
        <div class="setting-item" @click="$router.push('/settings/profile')">
          <span>编辑资料</span>
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="9 18 15 12 9 6"/></svg>
        </div>
        <div class="setting-item" @click="$router.push('/profile')">
          <span>我的主页</span>
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="9 18 15 12 9 6"/></svg>
        </div>
        <div class="section-title" style="margin-top:24px">关于</div>
        <div class="setting-item">
          <span>版本</span>
          <span class="value">1.0.0</span>
        </div>
        <div class="setting-item">
          <span>联系我们</span>
          <span class="value">support@glowxel.com</span>
        </div>
        <div class="section-title" style="margin-top:24px">账户操作</div>
        <div class="setting-item danger" @click="handleLogout">
          <span>退出登录</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { useRouter } from 'vue-router'

const router = useRouter()

function handleLogout() {
  localStorage.removeItem('auth_token')
  localStorage.removeItem('user_info')
  router.push('/login')
}
</script>

<style scoped>
.container { max-width: 600px; margin: 0 auto; padding: 0 20px; }
.back-btn { background: none; border: none; font-size: 14px; color: #666; cursor: pointer; padding: 20px 0; display: block; }
.back-btn:hover { color: #1a1a1a; }
.page-title { font-size: 22px; font-weight: 700; color: #1a1a1a; margin-bottom: 24px; }
.settings-card { background: #fafafa; border: 1px solid #f0f0f0; border-radius: 16px; padding: 24px; }
.section-title { font-size: 11px; font-weight: 600; color: #999; text-transform: uppercase; letter-spacing: 0.05em; margin-bottom: 8px; }
.setting-item { display: flex; justify-content: space-between; align-items: center; padding: 14px 0; border-bottom: 1px solid #f0f0f0; cursor: pointer; font-size: 14px; color: #1a1a1a; }
.setting-item:last-child { border-bottom: none; }
.setting-item:hover { color: #555; }
.setting-item.danger { color: #e53e3e; }
.value { font-size: 13px; color: #999; }
</style>

