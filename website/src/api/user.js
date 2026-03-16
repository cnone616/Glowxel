import { get, post, put } from './request.js'

export const login = (data) => post('/api/user/login', data)
export const getProfile = () => get('/api/user/profile')
export const updateProfile = (data) => put('/api/user/profile', data)
export const getUserDetail = (id) => get(`/api/user/${id}`)
export const getUserStats = () => get('/api/user/stats')
export const getRecommended = (limit = 8) => get('/api/user/recommended', { limit })
export const searchUsers = (keyword) => get('/api/user/search', { keyword })

