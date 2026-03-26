import { API_BASE_URL } from '../config/api'

function getToken() {
  return localStorage.getItem('auth_token') || ''
}

export async function request(url, options = {}) {
  const { method = 'GET', data = {}, auth = true } = options

  const headers = { 'Content-Type': 'application/json' }
  if (auth) {
    const token = getToken()
    if (token) headers['Authorization'] = `Bearer ${token}`
  }

  const config = { method, headers }
  if (method !== 'GET' && Object.keys(data).length) {
    config.body = JSON.stringify(data)
  }

  const queryStr = method === 'GET' && Object.keys(data).length
    ? '?' + new URLSearchParams(data).toString()
    : ''

  try {
    const res = await fetch(`${API_BASE_URL}${url}${queryStr}`, config)
    const json = await res.json()
    const bizCode = typeof json?.code === 'number' ? json.code : 0

    if (res.status === 200 && bizCode === 0) {
      return { success: true, data: json.data || json, message: json.message || 'ok' }
    }
    if (res.status === 401) {
      localStorage.removeItem('auth_token')
      localStorage.removeItem('user_info')
      return { success: false, data: null, message: '登录已过期' }
    }
    if (bizCode === 401) {
      localStorage.removeItem('auth_token')
      localStorage.removeItem('user_info')
    }
    return {
      success: false,
      data: json.data || null,
      message: json.message || `请求失败 (${res.status})`,
      code: bizCode,
    }
  } catch (error) {
    console.error(`[API] ${method} ${url} 失败:`, error)
    return { success: false, data: null, message: '网络请求失败' }
  }
}

export const get = (url, data, opts = {}) => request(url, { ...opts, method: 'GET', data })
export const post = (url, data, opts = {}) => request(url, { ...opts, method: 'POST', data })
export const put = (url, data, opts = {}) => request(url, { ...opts, method: 'PUT', data })
export const del = (url, data, opts = {}) => request(url, { ...opts, method: 'DELETE', data })
