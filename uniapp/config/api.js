export const API_BASE_URL = 'https://glowxel.com'

export const getUploadUrl = (type) => {
  return `${API_BASE_URL}/api/upload?type=${type}`
}
