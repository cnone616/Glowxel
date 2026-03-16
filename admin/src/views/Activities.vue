<template>
  <div>
    <div style="margin-bottom:16px;display:flex;justify-content:space-between;align-items:center">
      <a-space>
        <a-input-search v-model:value="keyword" placeholder="搜索标题..." style="width:220px" @search="fetchList" allow-clear />
        <a-select v-model:value="filterType" style="width:120px" @change="fetchList" allow-clear placeholder="类型">
          <a-select-option value="activity">活动</a-select-option>
          <a-select-option value="announcement">公告</a-select-option>
          <a-select-option value="tutorial">教程</a-select-option>
        </a-select>
        <a-select v-model:value="filterStatus" style="width:120px" @change="fetchList" allow-clear placeholder="状态">
          <a-select-option value="draft">草稿</a-select-option>
          <a-select-option value="published">已发布</a-select-option>
          <a-select-option value="archived">已归档</a-select-option>
        </a-select>
      </a-space>
      <a-button type="primary" @click="openCreate">新增活动/公告</a-button>
    </div>

    <a-table :columns="columns" :data-source="list" :loading="loading"
      :pagination="pagination" @change="handleTableChange" row-key="id">
      <template #bodyCell="{ column, record }">
        <template v-if="column.key === 'cover'">
          <img v-if="record.cover_url" :src="record.cover_url"
            style="width:60px;height:40px;object-fit:cover;border-radius:4px" />
          <span v-else style="color:#ccc">无封面</span>
        </template>
        <template v-if="column.key === 'type'">
          <a-tag :color="{ activity:'blue', announcement:'orange', tutorial:'green' }[record.type]">
            {{ { activity:'活动', announcement:'公告', tutorial:'教程' }[record.type] }}
          </a-tag>
        </template>
        <template v-if="column.key === 'status'">
          <a-tag :color="{ draft:'default', published:'green', archived:'red' }[record.status]">
            {{ { draft:'草稿', published:'已发布', archived:'已归档' }[record.status] }}
          </a-tag>
        </template>
        <template v-if="column.key === 'pinned'">
          <a-tag v-if="record.is_pinned" color="red">置顶</a-tag>
          <span v-else style="color:#ccc">—</span>
        </template>
        <template v-if="column.key === 'time'">
          <div style="font-size:12px;color:#666">
            <div v-if="record.start_time">开始：{{ record.start_time?.slice(0,16) }}</div>
            <div v-if="record.end_time">结束：{{ record.end_time?.slice(0,16) }}</div>
            <span v-if="!record.start_time && !record.end_time">—</span>
          </div>
        </template>
        <template v-if="column.key === 'action'">
          <a-space>
            <a-button type="link" size="small" @click="editItem(record)">编辑</a-button>
            <a-button type="link" size="small"
              @click="toggleStatus(record)">
              {{ record.status === 'published' ? '下架' : '发布' }}
            </a-button>
            <a-popconfirm title="确定删除？" @confirm="remove(record.id)">
              <a-button type="link" size="small" danger>删除</a-button>
            </a-popconfirm>
          </a-space>
        </template>
      </template>
    </a-table>

    <a-modal v-model:open="showModal" :title="editId ? '编辑' : '新增活动/公告'"
      @ok="handleSave" :confirm-loading="saving" width="720px" :destroy-on-close="true">
      <a-form layout="vertical" :model="form">
        <a-row :gutter="16">
          <a-col :span="16">
            <a-form-item label="标题" required>
              <a-input v-model:value="form.title" placeholder="活动/公告标题" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="类型">
              <a-select v-model:value="form.type">
                <a-select-option value="activity">活动</a-select-option>
                <a-select-option value="announcement">公告</a-select-option>
                <a-select-option value="tutorial">教程</a-select-option>
              </a-select>
            </a-form-item>
          </a-col>
          <a-col :span="24">
            <a-form-item label="副标题/摘要">
              <a-input v-model:value="form.subtitle" placeholder="简短描述，显示在列表页" />
            </a-form-item>
          </a-col>
          <a-col :span="24">
            <a-form-item label="封面图 URL">
              <a-input v-model:value="form.cover_url" placeholder="https://..." />
              <img v-if="form.cover_url" :src="form.cover_url"
                style="margin-top:6px;width:100%;max-height:100px;object-fit:cover;border-radius:4px" />
            </a-form-item>
          </a-col>
          <a-col :span="24">
            <a-form-item label="正文内容（支持 Markdown）">
              <a-textarea v-model:value="form.content" :rows="6" placeholder="活动详情、规则说明等..." />
            </a-form-item>
          </a-col>
          <a-col :span="12">
            <a-form-item label="活动开始时间（可选）">
              <a-input v-model:value="form.start_time" placeholder="YYYY-MM-DD HH:mm" />
            </a-form-item>
          </a-col>
          <a-col :span="12">
            <a-form-item label="活动结束时间（可选）">
              <a-input v-model:value="form.end_time" placeholder="YYYY-MM-DD HH:mm" />
            </a-form-item>
          </a-col>
          <a-col :span="12">
            <a-form-item label="外链 URL（可选，点击跳转）">
              <a-input v-model:value="form.link_url" placeholder="https://..." />
            </a-form-item>
          </a-col>
          <a-col :span="6">
            <a-form-item label="排序权重（越大越靠前）">
              <a-input-number v-model:value="form.sort_order" :min="0" style="width:100%" />
            </a-form-item>
          </a-col>
          <a-col :span="6">
            <a-form-item label="是否置顶">
              <a-switch v-model:checked="form.is_pinned" checked-children="置顶" un-checked-children="普通" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="状态">
              <a-select v-model:value="form.status">
                <a-select-option value="draft">草稿</a-select-option>
                <a-select-option value="published">立即发布</a-select-option>
                <a-select-option value="archived">归档</a-select-option>
              </a-select>
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
const filterType = ref(undefined)
const filterStatus = ref(undefined)
const pagination = ref({ current: 1, pageSize: 10, total: 0 })
const showModal = ref(false)
const saving = ref(false)
const editId = ref(null)

const defaultForm = () => ({
  title: '', subtitle: '', cover_url: '', content: '',
  type: 'activity', status: 'draft',
  is_pinned: false, sort_order: 0,
  start_time: '', end_time: '', link_url: ''
})
const form = ref(defaultForm())

const columns = [
  { title: '封面', key: 'cover', width: 80 },
  { title: '标题', dataIndex: 'title', key: 'title' },
  { title: '类型', key: 'type', width: 90 },
  { title: '状态', key: 'status', width: 90 },
  { title: '置顶', key: 'pinned', width: 70 },
  { title: '活动时间', key: 'time', width: 160 },
  { title: '浏览', dataIndex: 'views', key: 'views', width: 70 },
  { title: '排序', dataIndex: 'sort_order', key: 'sort_order', width: 70 },
  { title: '操作', key: 'action', width: 160 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/activities', {
      params: { page: pagination.value.current, limit: pagination.value.pageSize,
        keyword: keyword.value, type: filterType.value, status: filterStatus.value }
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
    title: record.title || '', subtitle: record.subtitle || '',
    cover_url: record.cover_url || '', content: record.content || '',
    type: record.type || 'activity', status: record.status || 'draft',
    is_pinned: !!record.is_pinned, sort_order: record.sort_order || 0,
    start_time: record.start_time?.slice(0, 16) || '',
    end_time: record.end_time?.slice(0, 16) || '',
    link_url: record.link_url || ''
  }
  showModal.value = true
}

async function toggleStatus(record) {
  const newStatus = record.status === 'published' ? 'archived' : 'published'
  try {
    await http.put(`/admin/activities/${record.id}`, { status: newStatus })
    message.success(newStatus === 'published' ? '已发布' : '已下架')
    fetchList()
  } catch { message.error('操作失败') }
}

async function handleSave() {
  if (!form.value.title.trim()) { message.warning('标题不能为空'); return }
  saving.value = true
  try {
    const payload = {
      ...form.value,
      is_pinned: form.value.is_pinned ? 1 : 0,
      start_time: form.value.start_time || null,
      end_time: form.value.end_time || null,
      published_at: form.value.status === 'published' ? new Date().toISOString() : null
    }
    if (editId.value) await http.put(`/admin/activities/${editId.value}`, payload)
    else await http.post('/admin/activities', payload)
    message.success('保存成功'); showModal.value = false; fetchList()
  } catch { message.error('保存失败') }
  saving.value = false
}

async function remove(id) {
  try { await http.delete(`/admin/activities/${id}`); message.success('已删除'); fetchList() }
  catch { message.error('删除失败') }
}

onMounted(fetchList)
</script>
