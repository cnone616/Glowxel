import axios from 'axios'

const http = axios.create({
  baseURL: '/api',
  timeout: 10000
})

http.interceptors.request.use(config => {
  const token = localStorage.getItem('admin_token')
  if (token) config.headers.Authorization = `Bearer ${token}`
  return config
})

http.interceptors.response.use(
  res => {
    const bizCode = typeof res.data?.code === 'number' ? res.data.code : 0
    if (bizCode === 0) {
      return res.data
    }

    if (bizCode === 401) {
      localStorage.removeItem('admin_token')
      window.location.href = '/login'
    }

    const error = new Error(res.data?.message || '请求失败')
    error.response = { ...res, data: res.data }
    return Promise.reject(error)
  },
  err => {
    if (err.response?.status === 401) {
      localStorage.removeItem('admin_token')
      window.location.href = '/login'
    }
    return Promise.reject(err)
  }
)

export default http
