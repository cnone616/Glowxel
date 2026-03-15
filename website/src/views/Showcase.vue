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
  font-size: 48px;
  font-weight: 800;
  margin-bottom: 48px;
  text-align: center;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
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
  padding: 16px 24px;
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border: 2px solid rgba(255, 107, 107, 0.2);
  border-radius: 16px;
  color: #2d3748;
  font-size: 15px;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.1);
  transition: all 0.3s ease;
}

.search-box input::placeholder {
  color: #a0aec0;
}

.search-box input:focus {
  outline: none;
  border-color: #ff6b6b;
  background: white;
  box-shadow: 0 6px 24px rgba(255, 107, 107, 0.2);
}

.filters {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.filter-btn {
  padding: 10px 20px;
  background: rgba(255, 255, 255, 0.9);
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 24px;
  color: #4a5568;
  cursor: pointer;
  transition: all 0.3s ease;
  font-size: 14px;
  font-weight: 600;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.08);
}

.filter-btn:hover {
  background: white;
  color: #ff6b6b;
  border-color: #ff6b6b;
  transform: translateY(-2px);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.15);
}

.filter-btn.active {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  color: white;
  border-color: #ff6b6b;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
}

.sort-select {
  padding: 12px 20px;
  background: rgba(255, 255, 255, 0.9);
  border: 2px solid rgba(255, 107, 107, 0.2);
  border-radius: 12px;
  color: #2d3748;
  cursor: pointer;
  max-width: 200px;
  font-weight: 600;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.08);
  transition: all 0.3s ease;
}

.sort-select:focus {
  outline: none;
  border-color: #ff6b6b;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.15);
}

.loading {
  text-align: center;
  padding: 80px 0;
  color: #a0aec0;
  font-size: 16px;
}

.showcase-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 24px;
}

.showcase-item {
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 20px;
  overflow: hidden;
  cursor: pointer;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.showcase-item:hover {
  transform: translateY(-8px);
  border-color: #ff6b6b;
  box-shadow: 0 12px 40px rgba(255, 107, 107, 0.25);
}

.item-image {
  width: 100%;
  height: 280px;
  background: linear-gradient(135deg, #fff5f5 0%, #ffe0e0 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}

.item-image img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  transition: transform 0.4s ease;
}

.showcase-item:hover .item-image img {
  transform: scale(1.08);
}

.item-info {
  padding: 24px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.95) 0%, rgba(255, 255, 255, 0.9) 100%);
}

.item-info h3 {
  color: #2d3748;
  margin-bottom: 8px;
  font-size: 18px;
  font-weight: 700;
}

.author {
  color: #718096;
  font-size: 14px;
  margin-bottom: 12px;
}

.stats {
  display: flex;
  gap: 16px;
  font-size: 14px;
  color: #a0aec0;
  font-weight: 500;
}

.empty {
  text-align: center;
  padding: 100px 0;
  color: #a0aec0;
  font-size: 16px;
}

@media (max-width: 768px) {
  .showcase {
    padding: 24px 0 60px;
  }
  
  .page-title {
    font-size: 36px;
    margin-bottom: 32px;
  }
  
  .controls {
    gap: 16px;
    margin-bottom: 32px;
  }
  
  .search-box input {
    padding: 14px 20px;
    font-size: 14px;
  }
  
  .filters {
    gap: 8px;
  }
  
  .filter-btn {
    padding: 8px 16px;
    font-size: 13px;
  }
  
  .sort-select {
    width: 100%;
    max-width: 100%;
    padding: 12px 18px;
  }
  
  .showcase-grid {
    grid-template-columns: 1fr;
    gap: 20px;
  }
  
  .item-image {
    height: 240px;
  }
  
  .item-info {
    padding: 20px;
  }
  
  .item-info h3 {
    font-size: 17px;
  }
}
</style>
