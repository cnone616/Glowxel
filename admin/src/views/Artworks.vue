<template>
  <div>
    <div style="margin-bottom: 16px; display: flex; gap: 12px">
      <a-input-search v-model:value="keyword" placeholder="搜索作品" style="width: 240px" @search="fetchList" />
      <a-select v-model:value="status" style="width: 120px" @change="fetchList">
        <a-select-option value="">全部</a-select-option>
        <a-select-option value="published">已发布</a-select-option>
        <a-select-option value="pending">待审核</a-select-option>
        <a-select-option value="rejected">已拒绝</a-select-option>
      </a-select>
    </div>
    <a-table :columns="columns" :data-source="list" :loading="loading" :pagination="pagination" @change="handleTableChange" row-key="id">
      <template #bodyCell="{ column, record }">
        <template v-if="column.key === 'status'">
          <a-tag :color="{ published: 'green', pending: 'orange', rejected: 'red' }[record.status]">
            {{ { published: '已发布', pending: '待审核', rejected: '已拒绝' }[record.status] }}
          </a-tag>
        </template>
        <template v-if="column.key === 'action'">
          <a-space>
            <a-button type="link" size="small" v-if="record.status === 'pending'" @click="review(record.id, 'published')">通过</a-button>
            <a-button type="link" size="small" danger v-if="record.status === 'pending'" @click="review(record.id, 'rejected')">拒绝</a-button>
            <a-popconfirm title="确定删除？" @confirm="remove(record.id)">
              <a-button type="link" size="small" danger>删除</a-button>
            </a-popconfirm>
          </a-space>
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
const status = ref('')
const list = ref([])
const loading = ref(false)
const pagination = ref({ current: 1, pageSize: 10, total: 0 })

const columns = [
  { title: '标题', dataIndex: 'title', key: 'title' },
  { title: '作者', dataIndex: 'author', key: 'author' },
  { title: '点赞', dataIndex: 'likes', key: 'likes', width: 80 },
  { title: '评论', dataIndex: 'commentCount', key: 'commentCount', width: 80 },
  { title: '发布时间', dataIndex: 'createdAt', key: 'createdAt' },
  { title: '状态', key: 'status', width: 80 },
  { title: '操作', key: 'action', width: 160 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/artworks', { params: { page: pagination.value.current, limit: pagination.value.pageSize, keyword: keyword.value, status: status.value } })
    list.value = res.data?.list || []
    pagination.value.total = res.data?.total || 0
  } catch (e) { /* ignore */ }
  loading.value = false
}

function handleTableChange(pag) { pagination.value.current = pag.current; fetchList() }

async function review(id, s) {
  try { await http.put(`/admin/artworks/${id}/status`, { status: s }); message.success('操作成功'); fetchList() }
  catch (e) { message.error('操作失败') }
}

async function remove(id) {
  try { await http.delete(`/admin/artworks/${id}`); message.success('已删除'); fetchList() }
  catch (e) { message.error('删除失败') }
}

onMounted(fetchList)
</script>

