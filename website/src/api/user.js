import { get, post, put } from './request.js'

export const login = (data) => post('/api/user/login', data)
export const adminLogin = (data) => post('/api/user/admin-login', data, { auth: false })
export const getProfile = () => get('/api/user/profile')
export const updateProfile = (data) => put('/api/user/profile', data)
export const getUserStats = () => get('/api/user/stats')
export const getRecommended = (limit = 8) => get('/api/user/recommended', { limit })
export const searchUsers = (keyword) => get('/api/user/search', { keyword })
export const getSettings = () => get('/api/user/settings')
export const updateSettings = (data) => put('/api/user/settings', data)
export const getUserDetail = (id) => get(`/api/user/${id}`)
