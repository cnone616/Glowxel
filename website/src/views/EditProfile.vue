<template>
  <div class="edit-profile">
    <div class="container">
      <button class="back-btn" @click="$router.back()">← 返回</button>
      <h1 class="page-title">编辑资料</h1>
      <div class="form-card">
        <div class="avatar-section">
          <div class="avatar">{{ (form.name || '?')[0].toUpperCase() }}</div>
          <span class="avatar-tip">头像暂不支持上传</span>
        </div>
        <div class="form-group">
          <label>昵称</label>
          <input v-model="form.name" maxlength="20" placeholder="请输入昵称" />
          <span class="char-count">{{ form.name.length }}/20</span>
        </div>
        <div class="form-group">
          <label>个人简介</label>
          <textarea v-model="form.bio" maxlength="100" placeholder="介绍一下自己吧..."></textarea>
          <span class="char-count">{{ form.bio.length }}/100</span>
        </div>
        <button class="btn-save" :disabled="saving" @click="handleSave">{{ saving ? '保存中...' : '保存' }}</button>
        <p class="msg success" v-if="msg">{{ msg }}</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { userAPI } from '@/api/index.js'

const router = useRouter()
const saving = ref(false)
const msg = ref('')
const form = ref({ name: '', bio: '' })

onMounted(async () => {
  const token = localStorage.getItem('auth_token')
  if (!token) { router.push('/login'); return }
  const res = await userAPI.getProfile()
  if (res.success) {
    const u = res.data?.user || res.data || {}
    form.value.name = u.name || ''
    form.value.bio = u.bio || ''
  }
})

async function handleSave() {
  if (!form.value.name.trim()) return
  saving.value = true; msg.value = ''
  const res = await userAPI.updateProfile({ name: form.value.name, bio: form.value.bio })
  if (res.success) {
    msg.value = '保存成功'
    const info = JSON.parse(localStorage.getItem('user_info') || '{}')
    localStorage.setItem('user_info', JSON.stringify({ ...info, ...form.value }))
    setTimeout(() => router.push('/profile'), 1000)
  } else { msg.value = res.message || '保存失败' }
  saving.value = false
}
</script>

<style scoped>
.container { max-width: 600px; margin: 0 auto; padding: 24px 20px 48px; }
.back-btn { min-height: 42px; padding: 0 16px; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); box-shadow: var(--nb-shadow-soft); font-size: 14px; font-weight: 800; color: var(--nb-ink); cursor: pointer; display: inline-flex; align-items: center; margin-bottom: 18px; }
.back-btn:hover { background: #f6f6f6; }
.page-title { font-size: 28px; font-weight: 800; color: var(--nb-ink); margin-bottom: 20px; }
.form-card { background: var(--tone-paper-soft); border: 3px solid var(--nb-ink); border-radius: 0; padding: 32px; box-shadow: var(--nb-shadow-card); }
.avatar-section { display: flex; flex-direction: column; align-items: center; margin-bottom: 28px; gap: 8px; }
.avatar { width: 72px; height: 72px; border: 3px solid var(--nb-ink); border-radius: 0; background: var(--tone-blue-soft); display: flex; align-items: center; justify-content: center; font-size: 28px; font-weight: 800; color: var(--nb-ink); box-shadow: var(--nb-shadow-soft); }
.avatar-tip { font-size: 12px; color: var(--text-secondary); }
.form-group { margin-bottom: 20px; position: relative; }
.form-group label { display: block; font-size: 12px; font-weight: 800; color: var(--text-secondary); margin-bottom: 8px; text-transform: uppercase; letter-spacing: 0.04em; }
.form-group input, .form-group textarea { width: 100%; padding: 10px 12px; border: 2px solid var(--nb-ink); border-radius: 0; font-size: 14px; outline: none; box-sizing: border-box; background: var(--tone-paper-soft); color: var(--nb-ink); }
.form-group input:focus, .form-group textarea:focus { border-color: var(--nb-ink); }
.form-group textarea { min-height: 80px; resize: vertical; }
.char-count { position: absolute; right: 10px; bottom: 10px; font-size: 11px; color: var(--text-secondary); }
.btn-save { width: 100%; padding: 12px; border: 2px solid var(--nb-ink); border-radius: 0; background: var(--nb-yellow); color: var(--nb-ink); font-size: 15px; font-weight: 800; cursor: pointer; margin-top: 8px; box-shadow: var(--nb-shadow-soft); }
.btn-save:hover { background: var(--color-primary-hover); }
.btn-save:disabled { background: #ccc; cursor: not-allowed; }
.msg { text-align: center; font-size: 13px; margin-top: 12px; color: var(--nb-green); font-weight: 700; }
</style>
