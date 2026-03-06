<template>
  <div class="showcase">
    <div class="container">
      <h1 class="page-title">作品展示</h1>
      
      <div class="controls">
        <div class="search-box">
          <input 
            v-model="searchQuery" 
            type="text" 
            placeholder="搜索作品或作者..."
            @input="handleSearch"
          >
        </div>
        
        <div class="filters">
          <button 
            v-for="cat in categories" 
            :key="cat.id"
            :class="['filter-btn', { active: selectedCategory === cat.id }]"
            @click="selectCategory(cat.id)"
          >
            {{ cat.name }} ({{ cat.count }})
          </button>
        </div>
        
        <select v-model="sortBy" @change="handleSort" class="sort-select">
          <option value="date">最新发布</option>
          <option value="likes">最多点赞</option>
          <option value="views">最多浏览</option>
        </select>
      </div>

      <div v-if="loading" class="loading">加载中...</div>
      
      <div v-else class="showcase-grid">
        <div 
          v-for="item in items" 
          :key="item.id" 
          class="showcase-item"
          @click="viewDetail(item)"
        >
          <div class="item-image">
            <img :src="item.imageUrl" :alt="item.title">
          </div>
          <div class="item-info">
            <h3>{{ item.title }}</h3>
            <p class="author">by {{ item.author }}</p>
            <div class="stats">
              <span>点赞 {{ item.likes }}</span>
              <span>浏览 {{ item.views }}</span>
            </div>
          </div>
        </div>
      </div>

      <div v-if="!loading && items.length === 0" class="empty">
        <p>暂无作品</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { showcaseApi } from '@/api/showcase'

const items = ref([])
const categories = ref([])
const loading = ref(true)
const searchQuery = ref('')
const selectedCategory = ref('all')
const sortBy = ref('date')

const loadData = async () => {
  loading.value = true
  try {
    const [itemsData, catsData] = await Promise.all([
      showcaseApi.getShowcaseItems({
        category: selectedCategory.value,
        search: searchQuery.value,
        sortBy: sortBy.value
      }),
      showcaseApi.getCategories()
    ])
    items.value = itemsData.items
    categories.value = catsData
  } finally {
    loading.value = false
  }
}

const selectCategory = (catId) => {
  selectedCategory.value = catId
  loadData()
}

const handleSearch = () => {
  loadData()
}

const handleSort = () => {
  loadData()
}

const viewDetail = (item) => {
  alert(`查看作品: ${item.title}\n作者: ${item.author}\n点赞: ${item.likes}`)
}

onMounted(() => {
  loadData()
})
</script>

<style scoped>
.showcase {
  padding: 40px 0 80px;
  min-height: calc(100vh - 200px);
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}

.page-title {
  font-size: 42px;
  margin-bottom: 40px;
  text-align: center;
  background: linear-gradient(135deg, #00f3ff 0%, #c864ff 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.controls {
  margin-bottom: 40px;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.search-box input {
  width: 100%;
  padding: 12px 20px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  color: #fff;
  font-size: 15px;
}

.search-box input:focus {
  outline: none;
  border-color: #64c8ff;
}

.filters {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.filter-btn {
  padding: 8px 16px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 20px;
  color: rgba(255, 255, 255, 0.7);
  cursor: pointer;
  transition: all 0.3s;
  font-size: 14px;
}

.filter-btn:hover {
  background: rgba(255, 255, 255, 0.1);
  color: #fff;
}

.filter-btn.active {
  background: #00f3ff;
  color: #0a0a0a;
  border-color: #00f3ff;
}

.sort-select {
  padding: 10px 16px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  color: #fff;
  cursor: pointer;
  max-width: 200px;
}

.sort-select:focus {
  outline: none;
  border-color: #00f3ff;
}

.loading {
  text-align: center;
  padding: 60px 0;
  color: rgba(255, 255, 255, 0.5);
}

.showcase-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 24px;
}

.showcase-item {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 12px;
  overflow: hidden;
  cursor: pointer;
  transition: all 0.3s;
}

.showcase-item:hover {
  transform: translateY(-5px);
  border-color: #00f3ff;
  box-shadow: 0 10px 30px rgba(0, 243, 255, 0.2);
}

.item-image {
  width: 100%;
  height: 280px;
  background: #111;
  display: flex;
  align-items: center;
  justify-content: center;
}

.item-image img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.item-info {
  padding: 20px;
}

.item-info h3 {
  color: #fff;
  margin-bottom: 8px;
  font-size: 18px;
}

.author {
  color: rgba(255, 255, 255, 0.5);
  font-size: 14px;
  margin-bottom: 12px;
}

.stats {
  display: flex;
  gap: 15px;
  font-size: 14px;
  color: rgba(255, 255, 255, 0.6);
}

.empty {
  text-align: center;
  padding: 80px 0;
  color: rgba(255, 255, 255, 0.4);
}

@media (max-width: 768px) {
  .showcase-grid {
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    gap: 16px;
  }
  
  .item-image {
    height: 200px;
  }
}
</style>
