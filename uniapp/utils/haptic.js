/**
 * 触觉反馈工具
 * 统一管理应用中的振动反馈
 */

/**
 * 触发触觉反馈
 * @param {string} type - 振动类型: 'light' | 'medium' | 'heavy'
 */
export function triggerHaptic(type = 'light') {
  // 检查用户是否启用了触觉反馈
  const enabled = uni.getStorageSync('hapticFeedback')
  
  // 默认关闭（如果用户没有设置过）
  if (enabled !== true) {
    return
  }
  
  // 触发振动
  try {
    uni.vibrateShort({
      type: type,
      success: () => {
        // console.log('触觉反馈触发:', type)
      },
      fail: (err) => {
        console.warn('触觉反馈失败:', err)
      }
    })
  } catch (error) {
    console.warn('触觉反馈不支持:', error)
  }
}

/**
 * 轻量触觉反馈 - 用于轻量交互
 * 场景：切换模式、选择颜色、点击按钮
 */
export function hapticLight() {
  triggerHaptic('light')
}

/**
 * 中等触觉反馈 - 用于成功操作
 * 场景：保存成功、导出成功、标记完成
 */
export function hapticMedium() {
  triggerHaptic('medium')
}

/**
 * 重度触觉反馈 - 用于警告/删除操作
 * 场景：删除确认、错误提示、清除数据
 */
export function hapticHeavy() {
  triggerHaptic('heavy')
}

/**
 * 成功反馈
 */
export function hapticSuccess() {
  hapticMedium()
}

/**
 * 错误反馈
 */
export function hapticError() {
  hapticHeavy()
}

/**
 * 警告反馈
 */
export function hapticWarning() {
  hapticMedium()
}
