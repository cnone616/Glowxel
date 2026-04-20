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
.container { max-width: 600px; margin: 0 auto; padding: 24px 20px 56px; }
.back-btn { min-height: 42px; padding: 0 16px; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); box-shadow: var(--nb-shadow-soft); font-size: 14px; font-weight: 800; color: var(--nb-ink); cursor: pointer; display: inline-flex; align-items: center; margin-bottom: 18px; }
.back-btn:hover { background: #f6f6f6; }
.page-title { font-size: 28px; font-weight: 800; color: var(--nb-ink); margin-bottom: 20px; }
.settings-card { background: var(--tone-paper-soft); border: 3px solid var(--nb-ink); border-radius: 0; padding: 24px; box-shadow: var(--nb-shadow-card); }
.section-title { font-size: 11px; font-weight: 800; color: var(--text-secondary); text-transform: uppercase; letter-spacing: 0.05em; margin-bottom: 8px; }
.setting-item { display: flex; justify-content: space-between; align-items: center; padding: 14px 0; border-bottom: 2px solid var(--nb-ink); cursor: pointer; font-size: 14px; color: var(--nb-ink); font-weight: 700; }
.setting-item:last-child { border-bottom: none; }
.setting-item:hover { background: #fafafa; }
.setting-item.danger { color: var(--nb-coral); }
.value { font-size: 13px; color: var(--text-secondary); }
</style>
