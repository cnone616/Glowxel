<template>
  <div>
    <a-row :gutter="16">
      <a-col :span="6" v-for="s in stats" :key="s.title">
        <a-card>
          <a-statistic :title="s.title" :value="s.value" :prefix="s.icon" />
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

const stats = ref([
  { title: '总用户数', value: 0, icon: '👤' },
  { title: '总作品数', value: 0, icon: '🎨' },
  { title: '总模板数', value: 0, icon: '📦' },
  { title: '今日活跃', value: 0, icon: '🔥' }
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

