import { clockTemplates, presetColors } from '@/mock/clockTemplates'

const delay = (ms = 300) => new Promise(resolve => setTimeout(resolve, ms))

export const clockApi = {
  // 获取时钟模板列表
  async getTemplates() {
    await delay()
    return clockTemplates
  },
  
  // 获取预设颜色
  async getPresetColors() {
    await delay(100)
    return presetColors
  },
  
  // 保存时钟配置
  async saveClockConfig(config) {
    await delay(200)
    // 模拟保存到本地存储
    localStorage.setItem('clockConfig', JSON.stringify(config))
    return { success: true, message: '配置已保存' }
  },
  
  // 加载时钟配置
  async loadClockConfig() {
    await delay(100)
    const saved = localStorage.getItem('clockConfig')
    if (saved) {
      return JSON.parse(saved)
    }
    // 返回默认配置
    return clockTemplates[0].config
  },
  
  // 导出配置为 JSON
  exportConfig(config) {
    const dataStr = JSON.stringify(config, null, 2)
    const blob = new Blob([dataStr], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url
    link.download = 'clock-config.json'
    link.click()
    URL.revokeObjectURL(url)
  }
}
