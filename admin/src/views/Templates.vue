<template>
  <div>
    <div style="margin-bottom:16px;display:flex;justify-content:space-between;align-items:center">
      <a-input-search v-model:value="keyword" placeholder="搜索名称..." style="width:240px" @search="fetchList" allow-clear />
      <a-button type="primary" @click="openCreate">新增模板</a-button>
    </div>

    <a-table :columns="columns" :data-source="list" :loading="loading"
      :pagination="pagination" @change="handleTableChange" row-key="id">
      <template #bodyCell="{ column, record }">
        <template v-if="column.key === 'cover'">
          <img v-if="record.image_url" :src="record.image_url" style="width:60px;height:40px;object-fit:cover;border-radius:4px" />
          <span v-else style="color:#ccc">无封面</span>
        </template>
        <template v-if="column.key === 'featured'">
          <a-tag v-if="record.is_featured" color="gold">推荐</a-tag>
          <span v-else style="color:#ccc">—</span>
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

    <a-modal v-model:open="showModal" :title="editId ? '编辑模板' : '新增模板'"
      @ok="handleSave" :confirm-loading="saving" width="640px" :destroy-on-close="true">
      <a-form layout="vertical" :model="form">
        <a-row :gutter="16">
          <a-col :span="16">
            <a-form-item label="模板名称" required>
              <a-input v-model:value="form.name" placeholder="模板名称" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="分类">
              <a-input v-model:value="form.category" placeholder="如：动物、风景" />
            </a-form-item>
          </a-col>
          <a-col :span="24">
            <a-form-item label="封面图 URL">
              <a-input v-model:value="form.image_url" placeholder="https://..." />
              <img v-if="form.image_url" :src="form.image_url" style="margin-top:6px;width:100%;max-height:80px;object-fit:cover;border-radius:4px" />
            </a-form-item>
          </a-col>
          <a-col :span="24">
            <a-form-item label="描述">
              <a-textarea v-model:value="form.description" :rows="2" placeholder="模板简介" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="尺寸">
              <a-input v-model:value="form.size" placeholder="如：32x32" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="颜色数">
              <a-input-number v-model:value="form.color_count" :min="0" style="width:100%" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="难度">
              <a-select v-model:value="form.difficulty">
                <a-select-option value="简单">简单</a-select-option>
                <a-select-option value="中等">中等</a-select-option>
                <a-select-option value="困难">困难</a-select-option>
              </a-select>
            </a-form-item>
          </a-col>
          <a-col :span="12">
            <a-form-item label="排序权重（越大越靠前）">
              <a-input-number v-model:value="form.sort_order" :min="0" style="width:100%" />
            </a-form-item>
          </a-col>
          <a-col :span="12">
            <a-form-item label="是否推荐">
              <a-switch v-model:checked="form.is_featured" checked-children="推荐" un-checked-children="普通" />
            </a-form-item>
          </a-col>
        </a-row>
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
const keyword = ref('')
const pagination = ref({ current: 1, pageSize: 10, total: 0 })
const showModal = ref(false)
const saving = ref(false)
const editId = ref(null)

const defaultForm = () => ({
  name: '', category: '', description: '', image_url: '',
  size: '', color_count: 0, difficulty: '简单',
  is_featured: false, sort_order: 0
})
const form = ref(defaultForm())

const columns = [
  { title: '封面', key: 'cover', width: 80 },
  { title: '名称', dataIndex: 'name', key: 'name' },
  { title: '分类', dataIndex: 'category', key: 'category', width: 90 },
  { title: '尺寸', dataIndex: 'size', key: 'size', width: 80 },
  { title: '颜色数', dataIndex: 'color_count', key: 'color_count', width: 80 },
  { title: '难度', dataIndex: 'difficulty', key: 'difficulty', width: 80 },
  { title: '使用次数', dataIndex: 'usage_count', key: 'usage_count', width: 90 },
  { title: '推荐', key: 'featured', width: 70 },
  { title: '排序', dataIndex: 'sort_order', key: 'sort_order', width: 70 },
  { title: '操作', key: 'action', width: 120 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/templates', {
      params: { page: pagination.value.current, limit: pagination.value.pageSize, keyword: keyword.value }
    })
    list.value = res.data?.list || []
    pagination.value.total = res.data?.total || 0
  } catch {}
  loading.value = false
}

function handleTableChange(pag) { pagination.value.current = pag.current; fetchList() }
function openCreate() { editId.value = null; form.value = defaultForm(); showModal.value = true }

function editItem(record) {
  editId.value = record.id
  form.value = {
    name: record.name || '', category: record.category || '',
    description: record.description || '', image_url: record.image_url || '',
    size: record.size || '', color_count: record.color_count || 0,
    difficulty: record.difficulty || '简单',
    is_featured: !!record.is_featured, sort_order: record.sort_order || 0
  }
  showModal.value = true
}

async function handleSave() {
  if (!form.value.name.trim()) { message.warning('名称不能为空'); return }
  saving.value = true
  try {
    const payload = { ...form.value, is_featured: form.value.is_featured ? 1 : 0 }
    if (editId.value) await http.put(`/admin/templates/${editId.value}`, payload)
    else await http.post('/admin/templates', payload)
    message.success('保存成功'); showModal.value = false; fetchList()
  } catch { message.error('保存失败') }
  saving.value = false
}

async function remove(id) {
  try { await http.delete(`/admin/templates/${id}`); message.success('已删除'); fetchList() }
  catch { message.error('删除失败') }
}

onMounted(fetchList)
</script>


