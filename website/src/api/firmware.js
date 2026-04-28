import { get, post } from './request.js'

export const check = (params) => get('/api/firmware/check', params, { auth: false })
export const list = () => get('/api/firmware/list', undefined, { auth: false })
export const upload = (data) => post('/api/firmware/upload', data)
