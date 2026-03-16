/**
 * 统一 HTTP 请求基础层
 */

const BASE_URL = 'https://glowxel.com'

// 获取本地存储的 token
function getToken() {
  return uni.getStorageSync('auth_token') || ''
}

/**
 * 统一请求方法
 * @param {string} url - 接口路径 (如 /api/user/profile)
 * @param {object} options - 请求选项
 * @param {string} options.method - 请求方法 GET/POST/PUT/DELETE
 * @param {object} options.data - 请求数据
 * @param {boolean} options.auth - 是否需要鉴权，默认 true
 * @returns {Promise<{success: boolean, data: any, message: string}>}
 */
export async function request(url, options = {}) {
  const { method = 'GET', data = {}, auth = true } = options

  const header = {
    'Content-Type': 'application/json'
  }

  if (auth) {
    const token = getToken()
    if (token) {
      header['Authorization'] = `Bearer ${token}`
    }
  }

  try {
    const res = await new Promise((resolve, reject) => {
      uni.request({
        url: `${BASE_URL}${url}`,
        method,
        data,
        header,
        success: resolve,
        fail: reject
      })
    })

    if (res.statusCode === 200) {
      return {
        success: true,
        data: res.data.data || res.data,
        message: res.data.message || 'ok'
      }
    }

    if (res.statusCode === 401) {
      // token 过期，清除登录状态
      uni.removeStorageSync('auth_token')
      uni.removeStorageSync('isLoggedIn')
      return { success: false, data: null, message: '登录已过期，请重新登录' }
    }

    return {
      success: false,
      data: null,
      message: res.data?.message || `请求失败 (${res.statusCode})`
    }
  } catch (error) {
    console.error(`[API] ${method} ${url} 失败:`, error)
    return {
      success: false,
      data: null,
      message: error.errMsg || error.message || '网络请求失败'
    }
  }
}

// 便捷方法
export const get = (url, data, options = {}) =>
  request(url, { ...options, method: 'GET', data })

export const post = (url, data, options = {}) =>
  request(url, { ...options, method: 'POST', data })

export const put = (url, data, options = {}) =>
  request(url, { ...options, method: 'PUT', data })

export const del = (url, data, options = {}) =>
  request(url, { ...options, method: 'DELETE', data })
