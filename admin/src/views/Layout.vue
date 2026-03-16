<template>
  <a-layout style="min-height: 100vh">
    <a-layout-sider v-model:collapsed="collapsed" collapsible theme="light" :width="220">
      <div class="logo">{{ collapsed ? 'G' : 'Glowxel Admin' }}</div>
      <a-menu v-model:selectedKeys="selectedKeys" mode="inline" @click="handleMenuClick">
        <a-menu-item key="/dashboard">
          <template #icon><DashboardOutlined /></template>
          <span>数据概览</span>
        </a-menu-item>
        <a-menu-item key="/users">
          <template #icon><UserOutlined /></template>
          <span>用户管理</span>
        </a-menu-item>
        <a-menu-item key="/artworks">
          <template #icon><PictureOutlined /></template>
          <span>作品管理</span>
        </a-menu-item>
        <a-menu-item key="/templates">
          <template #icon><AppstoreOutlined /></template>
          <span>模板管理</span>
        </a-menu-item>
        <a-menu-item key="/challenges">
          <template #icon><TrophyOutlined /></template>
          <span>挑战管理</span>
        </a-menu-item>
        <a-menu-item key="/activities">
          <template #icon><CalendarOutlined /></template>
          <span>活动管理</span>
        </a-menu-item>
        <a-menu-item key="/comments">
          <template #icon><MessageOutlined /></template>
          <span>评论管理</span>
        </a-menu-item>
      </a-menu>
    </a-layout-sider>
    <a-layout>
      <a-layout-header class="header">
        <span class="page-title">{{ route.meta.title || 'Glowxel 管理后台' }}</span>
        <a-button type="text" @click="handleLogout">退出登录</a-button>
      </a-layout-header>
      <a-layout-content class="content">
        <router-view />
      </a-layout-content>
    </a-layout>
  </a-layout>
</template>

<script setup>
import { ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { DashboardOutlined, UserOutlined, PictureOutlined, AppstoreOutlined, TrophyOutlined, MessageOutlined, CalendarOutlined } from '@ant-design/icons-vue'

const route = useRoute()
const router = useRouter()
const collapsed = ref(false)
const selectedKeys = ref([route.path])

watch(() => route.path, (val) => { selectedKeys.value = [val] })

function handleMenuClick({ key }) { router.push(key) }
function handleLogout() {
  localStorage.removeItem('admin_token')
  router.push('/login')
}
</script>

<style scoped>
.logo {
  height: 48px; display: flex; align-items: center; justify-content: center;
  font-size: 16px; font-weight: 700; color: #1a1a1a; border-bottom: 1px solid #f0f0f0;
}
.header {
  background: #fff; padding: 0 24px; display: flex; justify-content: space-between;
  align-items: center; border-bottom: 1px solid #f0f0f0; height: 56px; line-height: 56px;
}
.page-title { font-size: 16px; font-weight: 600; }
.content { margin: 24px; background: #fff; border-radius: 8px; padding: 24px; min-height: 400px; }
</style>

