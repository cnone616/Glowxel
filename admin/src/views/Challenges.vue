<template>
  <div>
    <div style="margin-bottom: 16px">
      <a-button type="primary" @click="showModal = true">新增挑战</a-button>
    </div>
    <a-table :columns="columns" :data-source="list" :loading="loading" :pagination="pagination" @change="handleTableChange" row-key="id">
      <template #bodyCell="{ column, record }">
        <template v-if="column.key === 'status'">
          <a-tag :color="{ active: 'green', ended: 'default', upcoming: 'orange' }[record.status]">
            {{ { active: '进行中', ended: '已结束', upcoming: '即将开始' }[record.status] }}
          </a-tag>
        </template>
        <template v-if="column.key === 'action'">
          <a-space>
            <a-button type="link" size="small" @click="editItem(record)">编辑</a-button>
            <a-popconfirm title="确定删除？" @confirm="remove(record.id)">
              <a-button type="link" size="small" danger>删除</a-button>
            </a-popconfirm>
          </a-space>
        </template>
      </template>
    </a-table>

    <a-modal v-model:open="showModal" :title="editId ? '编辑挑战' : '新增挑战'" @ok="handleSave" :confirm-loading="saving">
      <a-form layout="vertical">
        <a-form-item label="标题"><a-input v-model:value="form.title" /></a-form-item>
        <a-form-item label="描述"><a-textarea v-model:value="form.description" :rows="3" /></a-form-item>
        <a-form-item label="状态">
          <a-select v-model:value="form.status">
            <a-select-option value="upcoming">即将开始</a-select-option>
            <a-select-option value="active">进行中</a-select-option>
            <a-select-option value="ended">已结束</a-select-option>
          </a-select>
        </a-form-item>
        <a-form-item label="截止日期"><a-input v-model:value="form.endDate" placeholder="YYYY-MM-DD" /></a-form-item>
      </a-form>
    </a-modal>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { message } from 'ant-design-vue'
import http from '@/api/http.js'

const list = ref([])
const loading = ref(false)
const pagination = ref({ current: 1, pageSize: 10, total: 0 })
const showModal = ref(false)
const saving = ref(false)
const editId = ref(null)
const form = ref({ title: '', description: '', status: 'upcoming', endDate: '' })

const columns = [
  { title: '标题', dataIndex: 'title', key: 'title' },
  { title: '参与人数', dataIndex: 'participants', key: 'participants', width: 100 },
  { title: '截止日期', dataIndex: 'endDate', key: 'endDate' },
  { title: '状态', key: 'status', width: 100 },
  { title: '操作', key: 'action', width: 140 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/challenges', { params: { page: pagination.value.current, limit: pagination.value.pageSize } })
    list.value = res.data?.list || []
    pagination.value.total = res.data?.total || 0
  } catch (e) { /* ignore */ }
  loading.value = false
}

function handleTableChange(pag) { pagination.value.current = pag.current; fetchList() }

function editItem(record) {
  editId.value = record.id
  form.value = { title: record.title, description: record.description, status: record.status, endDate: record.endDate }
  showModal.value = true
}

async function handleSave() {
  saving.value = true
  try {
    if (editId.value) { await http.put(`/admin/challenges/${editId.value}`, form.value) }
    else { await http.post('/admin/challenges', form.value) }
    message.success('保存成功'); showModal.value = false; editId.value = null
    form.value = { title: '', description: '', status: 'upcoming', endDate: '' }; fetchList()
  } catch (e) { message.error('保存失败') }
  saving.value = false
}

async function remove(id) {
  try { await http.delete(`/admin/challenges/${id}`); message.success('已删除'); fetchList() }
  catch (e) { message.error('删除失败') }
}

onMounted(fetchList)
</script>

