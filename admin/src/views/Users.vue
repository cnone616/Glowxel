<template>
  <div>
    <div style="margin-bottom: 16px; display: flex; gap: 12px">
      <a-input-search v-model:value="keyword" placeholder="搜索用户" style="width: 240px" @search="fetchList" />
    </div>
    <a-table :columns="columns" :data-source="list" :loading="loading" :pagination="pagination" @change="handleTableChange" row-key="id">
      <template #bodyCell="{ column, record }">
        <template v-if="column.key === 'avatar'">
          <a-avatar>{{ (record.name || '?')[0] }}</a-avatar>
        </template>
        <template v-if="column.key === 'status'">
          <a-tag :color="record.status === 'active' ? 'green' : 'red'">{{ record.status === 'active' ? '正常' : '禁用' }}</a-tag>
        </template>
        <template v-if="column.key === 'action'">
          <a-button type="link" size="small" @click="toggleStatus(record)">{{ record.status === 'active' ? '禁用' : '启用' }}</a-button>
        </template>
      </template>
    </a-table>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { message } from 'ant-design-vue'
import http from '@/api/http.js'

const keyword = ref('')
const list = ref([])
const loading = ref(false)
const pagination = ref({ current: 1, pageSize: 10, total: 0 })

const columns = [
  { title: '头像', key: 'avatar', width: 60 },
  { title: '昵称', dataIndex: 'name', key: 'name' },
  { title: '作品数', dataIndex: 'works_count', key: 'works_count', width: 90 },
  { title: '粉丝数', dataIndex: 'followers_count', key: 'followers_count', width: 90 },
  { title: '注册时间', dataIndex: 'created_at', key: 'created_at' },
  { title: '状态', key: 'status', width: 80 },
  { title: '操作', key: 'action', width: 80 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/users', { params: { page: pagination.value.current, limit: pagination.value.pageSize, keyword: keyword.value } })
    list.value = res.data?.list || []
    pagination.value.total = res.data?.total || 0
  } catch (e) { /* ignore */ }
  loading.value = false
}

function handleTableChange(pag) { pagination.value.current = pag.current; fetchList() }

async function toggleStatus(record) {
  try {
    await http.put(`/admin/users/${record.id}/status`, { status: record.status === 'active' ? 'banned' : 'active' })
    message.success('操作成功')
    fetchList()
  } catch (e) { message.error('操作失败') }
}

onMounted(fetchList)
</script>

