import { showcaseItems, categories } from '@/mock/showcaseData'

// 模拟 API 延迟
const delay = (ms = 300) => new Promise(resolve => setTimeout(resolve, ms))

export const showcaseApi = {
  // 获取作品列表
  async getShowcaseItems(params = {}) {
    await delay()
    
    let items = [...showcaseItems]
    
    // 分类筛选
    if (params.category && params.category !== 'all') {
      items = items.filter(item => item.category === params.category)
    }
    
    // 搜索
    if (params.search) {
      const searchLower = params.search.toLowerCase()
      items = items.filter(item => 
        item.title.toLowerCase().includes(searchLower) ||
        item.author.toLowerCase().includes(searchLower)
      )
    }
    
    // 排序
    if (params.sortBy === 'likes') {
      items.sort((a, b) => b.likes - a.likes)
    } else if (params.sortBy === 'views') {
      items.sort((a, b) => b.views - a.views)
    } else {
      items.sort((a, b) => new Date(b.createdAt) - new Date(a.createdAt))
    }
    
    return {
      items,
      total: items.length
    }
  },
  
  // 获取分类列表
  async getCategories() {
    await delay(100)
    return categories
  },
  
  // 获取单个作品详情
  async getShowcaseItem(id) {
    await delay()
    return showcaseItems.find(item => item.id === parseInt(id))
  },
  
  // 点赞
  async likeItem(id) {
    await delay(200)
    const item = showcaseItems.find(item => item.id === parseInt(id))
    if (item) {
      item.likes++
    }
    return { success: true, likes: item.likes }
  }
}
