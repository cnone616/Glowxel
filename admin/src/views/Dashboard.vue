<template>
  <div>
    <a-row :gutter="16">
      <a-col :span="6" v-for="s in stats" :key="s.title">
        <a-card>
          <a-statistic :title="s.title" :value="s.value">
            <template #prefix>
              <span v-html="s.icon" style="margin-right:6px;color:#555;"></span>
            </template>
          </a-statistic>
        </a-card>
      </a-col>
    </a-row>
    <a-card title="近 7 天新增用户" style="margin-top: 24px">
      <div ref="chartRef" style="height: 300px"></div>
    </a-card>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import http from '@/api/http.js'

const USER_ICON = `<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>`
const ART_ICON  = `<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M8 14s1.5 2 4 2 4-2 4-2"/><line x1="9" y1="9" x2="9.01" y2="9"/><line x1="15" y1="9" x2="15.01" y2="9"/></svg>`
const TPL_ICON  = `<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg>`
const ACT_ICON  = `<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>`

const stats = ref([
  { title: '总用户数', value: 0, icon: USER_ICON },
  { title: '总作品数', value: 0, icon: ART_ICON },
  { title: '总模板数', value: 0, icon: TPL_ICON },
  { title: '今日活跃', value: 0, icon: ACT_ICON }
])

const chartRef = ref(null)

onMounted(async () => {
  try {
    const res = await http.get('/admin/stats')
    if (res.data) {
      stats.value[0].value = res.data.userCount || 0
      stats.value[1].value = res.data.artworkCount || 0
      stats.value[2].value = res.data.templateCount || 0
      stats.value[3].value = res.data.todayActive || 0
    }
  } catch (e) { /* ignore */ }
})
</script>

