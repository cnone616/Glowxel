const dialogInstances = []

const registerDialogInstance = (instance) => {
  if (!instance) {
    return
  }

  const currentIndex = dialogInstances.indexOf(instance)
  if (currentIndex !== -1) {
    dialogInstances.splice(currentIndex, 1)
  }
  dialogInstances.push(instance)
}

const unregisterDialogInstance = (instance) => {
  if (!instance) {
    dialogInstances.length = 0
    return
  }

  const currentIndex = dialogInstances.indexOf(instance)
  if (currentIndex !== -1) {
    dialogInstances.splice(currentIndex, 1)
  }
}

const getActiveDialogInstance = () => {
  if (dialogInstances.length === 0) {
    return null
  }

  return dialogInstances[dialogInstances.length - 1]
}

export const bindDialogInstance = (instance) => {
  registerDialogInstance(instance)
}

export const unbindDialogInstance = (instance) => {
  unregisterDialogInstance(instance)
}

export function useDialog() {
  const setDialogInstance = (instance) => {
    bindDialogInstance(instance)
  }

  const confirm = (options = {}) => {
    const dialogInstance = getActiveDialogInstance()
    if (dialogInstance && typeof dialogInstance.open === 'function') {
      return dialogInstance.open(options)
    }

    console.warn('[Glowxel] ConfirmDialogHost 组件未挂载，已跳过确认弹窗:', options.title || options.content || '')
    return Promise.resolve(false)
  }

  const alert = (options = {}) => {
    return confirm({
      ...options,
      showCancel: false
    })
  }

  return {
    setDialogInstance,
    confirm,
    alert
  }
}
