<template>
  <div>
    <div style="margin-bottom: 16px">
      <a-button type="primary" @click="showModal = true">新增模板</a-button>
    </div>
    <a-table :columns="columns" :data-source="list" :loading="loading" :pagination="pagination" @change="handleTableChange" row-key="id">
      <template #bodyCell="{ column, record }">
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

    <a-modal v-model:open="showModal" :title="editId ? '编辑模板' : '新增模板'" @ok="handleSave" :confirm-loading="saving">
      <a-form layout="vertical">
        <a-form-item label="模板名称"><a-input v-model:value="form.name" /></a-form-item>
        <a-form-item label="分类"><a-input v-model:value="form.category" /></a-form-item>
        <a-form-item label="描述"><a-textarea v-model:value="form.description" :rows="3" /></a-form-item>
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
const form = ref({ name: '', category: '', description: '' })

const columns = [
  { title: '名称', dataIndex: 'name', key: 'name' },
  { title: '分类', dataIndex: 'category', key: 'category' },
  { title: '使用次数', dataIndex: 'useCount', key: 'useCount', width: 100 },
  { title: '创建时间', dataIndex: 'createdAt', key: 'createdAt' },
  { title: '操作', key: 'action', width: 140 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/templates', { params: { page: pagination.value.current, limit: pagination.value.pageSize } })
    list.value = res.data?.list || []
    pagination.value.total = res.data?.total || 0
  } catch (e) { /* ignore */ }
  loading.value = false
}

function handleTableChange(pag) { pagination.value.current = pag.current; fetchList() }

function editItem(record) {
  editId.value = record.id
  form.value = { name: record.name, category: record.category, description: record.description }
  showModal.value = true
}

async function handleSave() {
  saving.value = true
  try {
    if (editId.value) {
      await http.put(`/admin/templates/${editId.value}`, form.value)
    } else {
      await http.post('/admin/templates', form.value)
    }
    message.success('保存成功')
    showModal.value = false
    editId.value = null
    form.value = { name: '', category: '', description: '' }
    fetchList()
  } catch (e) { message.error('保存失败') }
  saving.value = false
}

async function remove(id) {
  try { await http.delete(`/admin/templates/${id}`); message.success('已删除'); fetchList() }
  catch (e) { message.error('删除失败') }
}

onMounted(fetchList)
</script>

