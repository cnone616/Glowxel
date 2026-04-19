/**
 * Toast / Loading 统一提示组合式函数
 */

const toastInstances = []
const loadingInstances = []

const registerInstance = (instances, instance) => {
  if (!instance) {
    return
  }

  const currentIndex = instances.indexOf(instance)
  if (currentIndex !== -1) {
    instances.splice(currentIndex, 1)
  }
  instances.push(instance)
}

const unregisterInstance = (instances, instance) => {
  if (!instance) {
    instances.length = 0
    return
  }

  const currentIndex = instances.indexOf(instance)
  if (currentIndex !== -1) {
    instances.splice(currentIndex, 1)
  }
}

const getActiveInstance = (instances) => {
  if (instances.length === 0) {
    return null
  }

  return instances[instances.length - 1]
}

export const bindToastInstance = (instance) => {
  registerInstance(toastInstances, instance)
}

export const unbindToastInstance = (instance) => {
  unregisterInstance(toastInstances, instance)
}

export const bindLoadingInstance = (instance) => {
  registerInstance(loadingInstances, instance)
}

export const unbindLoadingInstance = (instance) => {
  unregisterInstance(loadingInstances, instance)
}

export function useToast() {
  const setToastInstance = (instance) => {
    bindToastInstance(instance)
  }

  const setLoadingInstance = (instance) => {
    bindLoadingInstance(instance)
  }

  const showToast = (message, type = 'success', duration = 2000) => {
    const toastInstance = getActiveInstance(toastInstances)
    if (toastInstance && typeof toastInstance.show === 'function') {
      toastInstance.show(message, type, duration)
      return
    }

    console.warn('[Glowxel] Toast 组件未挂载，已跳过提示:', message)
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

  const showLoading = (message = '加载中...', options = {}) => {
    const loadingInstance = getActiveInstance(loadingInstances)
    if (loadingInstance && typeof loadingInstance.show === 'function') {
      loadingInstance.show(message, options)
      return
    }

    console.warn('[Glowxel] LoadingOverlay 组件未挂载，已跳过加载提示:', message)
  }

  const hideLoading = () => {
    const loadingInstance = getActiveInstance(loadingInstances)
    if (loadingInstance && typeof loadingInstance.hide === 'function') {
      loadingInstance.hide()
    }
  }

  return {
    setToastInstance,
    setLoadingInstance,
    showToast,
    showSuccess,
    showError,
    showWarning,
    showInfo,
    showLoading,
    hideLoading
  }
}
