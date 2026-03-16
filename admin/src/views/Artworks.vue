<template>
  <div>
    <a-tabs v-model:activeKey="activeTab" @change="onTabChange">
      <!-- ── 作品管理 ── -->
      <a-tab-pane key="artworks" tab="作品管理">
        <div style="margin-bottom:16px;display:flex;gap:12px">
          <a-input-search v-model:value="keyword" placeholder="搜索作品" style="width:240px" @search="fetchList" allow-clear />
          <a-select v-model:value="status" style="width:120px" @change="fetchList" allow-clear placeholder="全部状态">
            <a-select-option value="published">已发布</a-select-option>
            <a-select-option value="hidden">已隐藏</a-select-option>
            <a-select-option value="deleted">已删除</a-select-option>
          </a-select>
        </div>
        <a-table :columns="artCols" :data-source="list" :loading="loading"
          :pagination="pagination" @change="handleTableChange" row-key="id">
          <template #bodyCell="{ column, record }">
            <template v-if="column.key === 'cover'">
              <img v-if="record.cover_url" :src="record.cover_url" style="width:50px;height:36px;object-fit:cover;border-radius:4px" />
              <span v-else style="color:#ccc">无</span>
            </template>
            <template v-if="column.key === 'status'">
              <a-tag :color="{ published:'green', hidden:'red' }[record.status] || 'default'">
                {{ { published:'已发布', hidden:'已隐藏', deleted:'已删除' }[record.status] || record.status }}
              </a-tag>
            </template>
            <template v-if="column.key === 'action'">
              <a-space>
                <a-button type="link" size="small" v-if="record.status !== 'published'" @click="review(record.id, 'published')">发布</a-button>
                <a-button type="link" size="small" danger v-if="record.status === 'published'" @click="review(record.id, 'hidden')">隐藏</a-button>
                <a-popconfirm title="确定删除？" @confirm="removeArtwork(record.id)">
                  <a-button type="link" size="small" danger>删除</a-button>
                </a-popconfirm>
              </a-space>
            </template>
          </template>
        </a-table>
      </a-tab-pane>

      <!-- ── 标签管理 ── -->
      <a-tab-pane key="tags" tab="标签管理">
        <div style="margin-bottom:16px;display:flex;justify-content:space-between;align-items:center">
          <a-space>
            <a-input-search v-model:value="tagKeyword" placeholder="搜索标签" style="width:200px" @search="fetchTags" allow-clear />
            <a-select v-model:value="tagCatFilter" style="width:140px" @change="fetchTags" allow-clear placeholder="全部分类">
              <a-select-option v-for="c in categories" :key="c.id" :value="c.id">{{ c.name }}</a-select-option>
            </a-select>
          </a-space>
          <a-button type="primary" @click="openTagCreate">新增标签</a-button>
        </div>
        <a-table :columns="tagCols" :data-source="tags" :loading="tagLoading" row-key="id" :pagination="false">
          <template #bodyCell="{ column, record }">
            <template v-if="column.key === 'is_active'">
              <a-tag :color="record.is_active ? 'green' : 'default'">{{ record.is_active ? '启用' : '停用' }}</a-tag>
            </template>
            <template v-if="column.key === 'action'">
              <a-space>
                <a-button type="link" size="small" @click="editTag(record)">编辑</a-button>
                <a-button type="link" size="small" @click="toggleTag(record)">{{ record.is_active ? '停用' : '启用' }}</a-button>
                <a-popconfirm title="确定删除？" @confirm="removeTag(record.id)">
                  <a-button type="link" size="small" danger>删除</a-button>
                </a-popconfirm>
              </a-space>
            </template>
          </template>
        </a-table>
      </a-tab-pane>

      <!-- ── 分类管理 ── -->
      <a-tab-pane key="categories" tab="分类管理">
        <div style="margin-bottom:16px;display:flex;justify-content:flex-end">
          <a-button type="primary" @click="openCatCreate">新增分类</a-button>
        </div>
        <a-table :columns="catCols" :data-source="categories" :loading="catLoading" row-key="id" :pagination="false">
          <template #bodyCell="{ column, record }">
            <template v-if="column.key === 'is_active'">
              <a-tag :color="record.is_active ? 'green' : 'default'">{{ record.is_active ? '启用' : '停用' }}</a-tag>
            </template>
            <template v-if="column.key === 'action'">
              <a-space>
                <a-button type="link" size="small" @click="editCat(record)">编辑</a-button>
                <a-button type="link" size="small" @click="toggleCat(record)">{{ record.is_active ? '停用' : '启用' }}</a-button>
                <a-popconfirm title="确定删除？该分类下的标签也会受影响" @confirm="removeCat(record.id)">
                  <a-button type="link" size="small" danger>删除</a-button>
                </a-popconfirm>
              </a-space>
            </template>
          </template>
        </a-table>
      </a-tab-pane>
    </a-tabs>

    <!-- 标签弹窗 -->
    <a-modal v-model:open="showTagModal" :title="editTagId ? '编辑标签' : '新增标签'"
      @ok="saveTag" :confirm-loading="tagSaving" :destroy-on-close="true">
      <a-form layout="vertical">
        <a-form-item label="标签名" required>
          <a-input v-model:value="tagForm.name" placeholder="如：像素风" />
        </a-form-item>
        <a-form-item label="所属分类" required>
          <a-select v-model:value="tagForm.category_id" placeholder="选择分类">
            <a-select-option v-for="c in categories" :key="c.id" :value="c.id">{{ c.name }}</a-select-option>
          </a-select>
        </a-form-item>
        <a-form-item label="排序权重（越大越靠前）">
          <a-input-number v-model:value="tagForm.sort_order" :min="0" style="width:100%" />
        </a-form-item>
      </a-form>
    </a-modal>

    <!-- 分类弹窗 -->
    <a-modal v-model:open="showCatModal" :title="editCatId ? '编辑分类' : '新增分类'"
      @ok="saveCat" :confirm-loading="catSaving" :destroy-on-close="true">
      <a-form layout="vertical">
        <a-form-item label="分类名" required>
          <a-input v-model:value="catForm.name" placeholder="如：风格、主题" />
        </a-form-item>
        <a-form-item label="排序权重（越大越靠前）">
          <a-input-number v-model:value="catForm.sort_order" :min="0" style="width:100%" />
        </a-form-item>
      </a-form>
    </a-modal>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { message } from 'ant-design-vue'
import http from '@/api/http.js'

const activeTab = ref('artworks')

// ── 作品管理 ──
const keyword = ref('')
const status = ref(undefined)
const list = ref([])
const loading = ref(false)
const pagination = ref({ current: 1, pageSize: 10, total: 0 })

const artCols = [
  { title: '封面', key: 'cover', width: 70 },
  { title: '标题', dataIndex: 'title', key: 'title' },
  { title: '作者', dataIndex: 'author', key: 'author', width: 100 },
  { title: '点赞', dataIndex: 'likes', key: 'likes', width: 70 },
  { title: '评论', dataIndex: 'comments_count', key: 'comments_count', width: 70 },
  { title: '发布时间', dataIndex: 'created_at', key: 'created_at', width: 160 },
  { title: '状态', key: 'status', width: 90 },
  { title: '操作', key: 'action', width: 160 }
]

async function fetchList() {
  loading.value = true
  try {
    const res = await http.get('/admin/artworks', {
      params: { page: pagination.value.current, limit: pagination.value.pageSize, keyword: keyword.value, status: status.value }
    })
    list.value = res.data?.list || []
    pagination.value.total = res.data?.total || 0
  } catch {}
  loading.value = false
}

function handleTableChange(pag) { pagination.value.current = pag.current; fetchList() }

async function review(id, s) {
  try { await http.put(`/admin/artworks/${id}/status`, { status: s }); message.success('操作成功'); fetchList() }
  catch { message.error('操作失败') }
}

async function removeArtwork(id) {
  try { await http.delete(`/admin/artworks/${id}`); message.success('已删除'); fetchList() }
  catch { message.error('删除失败') }
}

// ── 分类管理 ──
const categories = ref([])
const catLoading = ref(false)
const showCatModal = ref(false)
const catSaving = ref(false)
const editCatId = ref(null)
const catForm = ref({ name: '', sort_order: 0 })

const catCols = [
  { title: '分类名', dataIndex: 'name', key: 'name' },
  { title: '排序权重', dataIndex: 'sort_order', key: 'sort_order', width: 100 },
  { title: '标签数', dataIndex: 'tag_count', key: 'tag_count', width: 80 },
  { title: '状态', key: 'is_active', width: 80 },
  { title: '操作', key: 'action', width: 160 }
]

async function fetchCategories() {
  catLoading.value = true
  try {
    const res = await http.get('/admin/tag-categories')
    categories.value = res.data?.list || []
  } catch {}
  catLoading.value = false
}

function openCatCreate() { editCatId.value = null; catForm.value = { name: '', sort_order: 0 }; showCatModal.value = true }
function editCat(record) { editCatId.value = record.id; catForm.value = { name: record.name, sort_order: record.sort_order }; showCatModal.value = true }

async function saveCat() {
  if (!catForm.value.name.trim()) { message.warning('分类名不能为空'); return }
  catSaving.value = true
  try {
    if (editCatId.value) await http.put(`/admin/tag-categories/${editCatId.value}`, catForm.value)
    else await http.post('/admin/tag-categories', catForm.value)
    message.success('保存成功'); showCatModal.value = false; fetchCategories()
  } catch (e) { message.error(e?.message || '保存失败') }
  catSaving.value = false
}

async function toggleCat(record) {
  try {
    await http.put(`/admin/tag-categories/${record.id}`, { is_active: record.is_active ? 0 : 1 })
    message.success('操作成功'); fetchCategories()
  } catch { message.error('操作失败') }
}

async function removeCat(id) {
  try { await http.delete(`/admin/tag-categories/${id}`); message.success('已删除'); fetchCategories(); fetchTags() }
  catch (e) { message.error(e?.response?.data?.message || '删除失败') }
}

// ── 标签管理 ──
const tags = ref([])
const tagLoading = ref(false)
const tagKeyword = ref('')
const tagCatFilter = ref(undefined)
const showTagModal = ref(false)
const tagSaving = ref(false)
const editTagId = ref(null)
const tagForm = ref({ name: '', category_id: undefined, sort_order: 0 })

const tagCols = [
  { title: '标签名', dataIndex: 'name', key: 'name' },
  { title: '所属分类', dataIndex: 'category_name', key: 'category_name', width: 120 },
  { title: '排序权重', dataIndex: 'sort_order', key: 'sort_order', width: 100 },
  { title: '使用次数', dataIndex: 'usage_count', key: 'usage_count', width: 90 },
  { title: '状态', key: 'is_active', width: 80 },
  { title: '操作', key: 'action', width: 180 }
]

async function fetchTags() {
  tagLoading.value = true
  try {
    const res = await http.get('/admin/tags', { params: { keyword: tagKeyword.value, category_id: tagCatFilter.value } })
    tags.value = res.data?.list || []
  } catch {}
  tagLoading.value = false
}

function openTagCreate() { editTagId.value = null; tagForm.value = { name: '', category_id: undefined, sort_order: 0 }; showTagModal.value = true }
function editTag(record) {
  editTagId.value = record.id
  tagForm.value = { name: record.name, category_id: record.category_id, sort_order: record.sort_order }
  showTagModal.value = true
}

async function saveTag() {
  if (!tagForm.value.name.trim() || !tagForm.value.category_id) { message.warning('标签名和分类不能为空'); return }
  tagSaving.value = true
  try {
    if (editTagId.value) await http.put(`/admin/tags/${editTagId.value}`, tagForm.value)
    else await http.post('/admin/tags', tagForm.value)
    message.success('保存成功'); showTagModal.value = false; fetchTags()
  } catch (e) { message.error(e?.response?.data?.message || '保存失败') }
  tagSaving.value = false
}

async function toggleTag(record) {
  try {
    await http.put(`/admin/tags/${record.id}`, { is_active: record.is_active ? 0 : 1 })
    message.success('操作成功'); fetchTags()
  } catch { message.error('操作失败') }
}

async function removeTag(id) {
  try { await http.delete(`/admin/tags/${id}`); message.success('已删除'); fetchTags() }
  catch { message.error('删除失败') }
}

function onTabChange(key) {
  if (key === 'artworks') fetchList()
  if (key === 'tags') { fetchCategories(); fetchTags() }
  if (key === 'categories') fetchCategories()
}

onMounted(() => { fetchList(); fetchCategories() })
</script>


