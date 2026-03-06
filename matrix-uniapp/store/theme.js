import { defineStore } from 'pinia'

// 预定义主题
export const themes = {
  dark: {
    id: 'dark',
    name: '深色'
  },
  light: {
    id: 'light',
    name: '浅色'
  }
}

export const useThemeStore = defineStore('theme', {
  state: () => ({
    currentTheme: 'dark', // 当前主题 ID
    isDarkMode: true // 兼容旧代码
  }),
  
  getters: {
    // 获取当前主题对象
    getTheme: (state) => themes[state.currentTheme],
    
    // 获取所有可用主题
    getAllThemes: () => Object.values(themes)
  },
  
  actions: {
    // 加载主题设置
    loadTheme() {
      try {
        const theme = uni.getStorageSync('theme')
        if (theme && themes[theme]) {
          this.currentTheme = theme
          this.isDarkMode = theme === 'dark'
        }
        this.applyTheme()
      } catch (e) {
        // 加载主题失败
      }
    },
    
    // 设置主题
    setTheme(themeId) {
      if (themes[themeId]) {
        this.currentTheme = themeId
        this.isDarkMode = themeId === 'dark'
        this.saveTheme()
        this.applyTheme()
      }
    },
    
    // 应用主题 - 通过添加/移除 class 到 page 元素
    applyTheme() {
      // #ifdef H5
      // 在 H5 平台，通过操作 DOM 给 page 元素添加 class
      setTimeout(() => {
        const pages = document.querySelectorAll('uni-page-body')
        pages.forEach(page => {
          if (this.currentTheme === 'light') {
            page.classList.add('light-theme')
          } else {
            page.classList.remove('light-theme')
          }
        })
      }, 0)
      // #endif
      
      // #ifndef H5
      // 在非 H5 平台，通过事件通知页面更新
      uni.$emit('themeChanged', this.isDarkMode)
      // #endif
    },
    
    // 切换主题
    toggleTheme() {
      const newTheme = this.currentTheme === 'dark' ? 'light' : 'dark'
      this.setTheme(newTheme)
    },
    
    // 保存主题设置
    saveTheme() {
      try {
        uni.setStorageSync('theme', this.currentTheme)
      } catch (e) {
        // 保存主题失败
      }
    }
  }
})
