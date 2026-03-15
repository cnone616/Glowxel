/**
 * Toast 提示组合式函数
 * 支持自定义 Toast 组件（使用 cover-view 覆盖 canvas）
 */

let toastInstance = null

export function useToast() {
  // 设置 toast 实例（由页面组件调用）
  const setToastInstance = (instance) => {
    toastInstance = instance
  }
  
  const showToast = (message, type = 'success', duration = 2000) => {
    if (toastInstance) {
      // 使用自定义 toast 组件
      toastInstance.show(message, type, duration)
    } else {
      // 降级到原生 toast
      const iconMap = {
        success: 'success',
        error: 'error',
        warning: 'none',
        info: 'none'
      }
      
      uni.showToast({
        title: message,
        icon: iconMap[type] || 'none',
        duration: duration,
        mask: false
      })
    }
  }

  const showSuccess = (message) => {
    showToast(message, 'success')
  }

  const showError = (message) => {
    showToast(message, 'error')
  }

  const showWarning = (message) => {
    showToast(message, 'warning')
  }

  const showInfo = (message) => {
    showToast(message, 'info')
  }

  const showLoading = (message = '加载中...') => {
    uni.showLoading({
      title: message,
      mask: true
    })
  }

  const hideLoading = () => {
    uni.hideLoading()
  }

  return {
    setToastInstance,
    showToast,
    showSuccess,
    showError,
    showWarning,
    showInfo,
    showLoading,
    hideLoading
  }
}
