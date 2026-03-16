<template>
  <div>
    <a-table :columns="columns" :data-source="list" :loading="loading" :pagination="pagination" @change="handleTableChange" row-key="id">
      <template #bodyCell="{ column, record }">
        <template v-if="column.key === 'action'">
          <a-popconfirm title="确定删除该评论？" @confirm="remove(record.id)">
            <a-button type="link" size="small" danger>删除</a-button>
          </a-popconfirm>
        </template>
      </template>
    </a-table>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { message } from 'ant-design-vue'
import http from '@/api/http.js'

const list = ref([])
const loading = ref(false)
const pagination = ref({ current: 1, pageSize: 20, total: 0 })

const columns = [
  { title: '评论内容', dataIndex: 'content', key: 'content', ellipsis: true },
  { title: '评论者', dataIndex: 'author', key: 'author', width: 120 },
  { title: '作品', dataIndex: 'artwork_title', key: 'artwork_title', width: 160 },
  { title: '时间', dataIndex: 'created_at', key: 'created_at', width: 160 },
  { title: '操作', key: 'action', width: 80 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/comments', { params: { page: pagination.value.current, limit: pagination.value.pageSize } })
    list.value = res.data?.list || []
    pagination.value.total = res.data?.total || 0
  } catch (e) { /* ignore */ }
  loading.value = false
}

function handleTableChange(pag) { pagination.value.current = pag.current; fetchList() }

async function remove(id) {
  try { await http.delete(`/admin/comments/${id}`); message.success('已删除'); fetchList() }
  catch (e) { message.error('删除失败') }
}

onMounted(fetchList)
</script>

