/**
 * 模板模块 API
 */
import { get, post, isMockMode } from './request.js'
import { MockAPI } from '../data/mock/index.js'

// 获取模板列表
export async function getTemplates(params = {}) {
  const { category, difficulty, page = 1, limit = 20 } = params
  if (isMockMode()) {
    let list
    if (category) {
      list = MockAPI.templates.getByCategory(category)
    } else if (difficulty) {
      list = MockAPI.templates.getByDifficulty(difficulty)
    } else {
      list = MockAPI.templates.getAll()
    }
    return { success: true, data: { list, total: list.length } }
  }
  return get('/api/template/list', { category, difficulty, page, limit })
}

// 获取模板详情
export async function getTemplateById(id) {
  if (isMockMode()) {
    const list = MockAPI.templates.getAll()
    return { success: true, data: list.find(t => t.id === id) }
  }
  return get(`/api/template/${id}`)
}

// 获取热门模板
export async function getPopularTemplates(limit = 10) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.templates.getPopular(limit) }
  }
  return get('/api/template/popular', { limit })
}

// 获取模板分类
export async function getCategories() {
  if (isMockMode()) {
    return { success: true, data: MockAPI.templates.getCategories() }
  }
  return get('/api/template/categories')
}

// 搜索模板
export async function searchTemplates(keyword) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.templates.search(keyword) }
  }
  return get('/api/template/search', { keyword })
}

// 使用模板（计数+1）
export async function useTemplate(templateId) {
  if (isMockMode()) {
    return { success: true }
  }
  return post(`/api/template/${templateId}/use`)
}

