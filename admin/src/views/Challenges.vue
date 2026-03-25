<template>
  <div>
    <div style="margin-bottom:16px;display:flex;justify-content:space-between;align-items:center">
      <a-input-search v-model:value="keyword" placeholder="搜索标题..." style="width:240px" @search="fetchList" allow-clear />
      <a-button type="primary" @click="openCreate">新增挑战</a-button>
    </div>

    <a-table :columns="columns" :data-source="list" :loading="loading"
      :pagination="pagination" @change="handleTableChange" row-key="id">
      <template #bodyCell="{ column, record }">
        <template v-if="column.key === 'cover'">
          <img v-if="record.cover_url" :src="record.cover_url" style="width:60px;height:40px;object-fit:cover;border-radius:4px" />
          <span v-else style="color:#ccc">无封面</span>
        </template>
        <template v-if="column.key === 'status'">
          <a-tag :color="{ active:'green', ended:'default', upcoming:'orange' }[record.status]">
            {{ { active:'进行中', ended:'已结束', upcoming:'即将开始' }[record.status] }}
          </a-tag>
        </template>
        <template v-if="column.key === 'dates'">
          <div style="font-size:12px">
            <div>开始：{{ record.start_date || '—' }}</div>
            <div>截止：{{ record.end_date || '—' }}</div>
          </div>
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

    <a-modal v-model:open="showModal" :title="editId ? '编辑挑战' : '新增挑战'"
      @ok="handleSave" :confirm-loading="saving" width="680px" :destroy-on-close="true">
      <a-form layout="vertical" :model="form">
        <a-row :gutter="16">
          <a-col :span="24">
            <a-form-item label="标题" required>
              <a-input v-model:value="form.title" placeholder="挑战标题" />
            </a-form-item>
          </a-col>
          <a-col :span="24">
            <a-form-item label="描述">
              <a-textarea v-model:value="form.description" :rows="3" placeholder="挑战说明" />
            </a-form-item>
          </a-col>
          <a-col :span="12">
            <a-form-item label="封面图 URL（列表缩略图）">
              <a-input v-model:value="form.cover_url" placeholder="https://..." />
              <img v-if="form.cover_url" :src="form.cover_url" style="margin-top:6px;width:100%;max-height:80px;object-fit:cover;border-radius:4px" />
            </a-form-item>
          </a-col>
          <a-col :span="12">
            <a-form-item label="横幅图 URL（详情页大图）">
              <a-input v-model:value="form.banner_url" placeholder="https://..." />
              <img v-if="form.banner_url" :src="form.banner_url" style="margin-top:6px;width:100%;max-height:80px;object-fit:cover;border-radius:4px" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="状态">
              <a-select v-model:value="form.status">
                <a-select-option value="upcoming">即将开始</a-select-option>
                <a-select-option value="active">进行中</a-select-option>
                <a-select-option value="ended">已结束</a-select-option>
              </a-select>
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="开始日期">
              <a-input v-model:value="form.start_date" placeholder="YYYY-MM-DD" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="截止日期">
              <a-input v-model:value="form.end_date" placeholder="YYYY-MM-DD" />
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
          <a-col :span="8">
            <a-form-item label="奖励类型">
              <a-select v-model:value="form.reward_type">
                <a-select-option value="实物">实物</a-select-option>
                <a-select-option value="虚拟">虚拟</a-select-option>
                <a-select-option value="荣誉">荣誉</a-select-option>
              </a-select>
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="最大参与人数（0=不限）">
              <a-input-number v-model:value="form.max_participants" :min="0" style="width:100%" />
            </a-form-item>
          </a-col>
          <a-col :span="16">
            <a-form-item label="奖励说明">
              <a-input v-model:value="form.prize" placeholder="如：Artkal 拼豆套装一份" />
            </a-form-item>
          </a-col>
          <a-col :span="8">
            <a-form-item label="排序权重（越大越靠前）">
              <a-input-number v-model:value="form.sort_order" :min="0" style="width:100%" />
            </a-form-item>
          </a-col>
          <a-col :span="24">
            <a-form-item label="参赛规则（每行一条）">
              <a-textarea v-model:value="form.rulesText" :rows="4"
                placeholder="每行一条规则，如：&#10;作品尺寸不超过 64×64&#10;必须使用 Artkal 颜色" />
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
  title: '', description: '', cover_url: '', banner_url: '',
  status: 'upcoming', start_date: '', end_date: '',
  difficulty: '简单', reward_type: '荣誉', max_participants: 0,
  prize: '', sort_order: 0, rulesText: ''
})
const form = ref(defaultForm())

const columns = [
  { title: '封面', key: 'cover', width: 80 },
  { title: '标题', dataIndex: 'title', key: 'title' },
  { title: '参与人数', dataIndex: 'participants', key: 'participants', width: 90 },
  { title: '时间', key: 'dates', width: 160 },
  { title: '难度', dataIndex: 'difficulty', key: 'difficulty', width: 80 },
  { title: '状态', key: 'status', width: 100 },
  { title: '排序', dataIndex: 'sort_order', key: 'sort_order', width: 70 },
  { title: '操作', key: 'action', width: 120 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/challenges', {
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
  let rules = record.rules
  if (typeof rules === 'string') {
    try { rules = JSON.parse(rules) } catch { rules = [] }
  }
  editId.value = record.id
  form.value = {
    title: record.title || '', description: record.description || '',
    cover_url: record.cover_url || '', banner_url: record.banner_url || '',
    status: record.status || 'upcoming', start_date: record.start_date || '',
    end_date: record.end_date || '', difficulty: record.difficulty || '简单',
    reward_type: record.reward_type || '荣誉', max_participants: record.max_participants || 0,
    prize: record.prize || '', sort_order: record.sort_order || 0,
    rulesText: Array.isArray(rules) ? rules.join('\n') : ''
  }
  showModal.value = true
}

async function handleSave() {
  if (!form.value.title.trim()) { message.warning('标题不能为空'); return }
  saving.value = true
  try {
    const payload = {
      ...form.value,
      rules: form.value.rulesText ? form.value.rulesText.split('\n').filter(Boolean) : []
    }
    delete payload.rulesText
    if (editId.value) await http.put(`/admin/challenges/${editId.value}`, payload)
    else await http.post('/admin/challenges', payload)
    message.success('保存成功')
    showModal.value = false
    fetchList()
  } catch { message.error('保存失败') }
  saving.value = false
}

async function remove(id) {
  try { await http.delete(`/admin/challenges/${id}`); message.success('已删除'); fetchList() }
  catch { message.error('删除失败') }
}

onMounted(fetchList)
</script>

