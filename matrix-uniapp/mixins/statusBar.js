// 状态栏高度 mixin - 处理小程序顶部安全区域
export default {
  data() {
    return {
      statusBarHeight: 0,
      menuButtonInfo: null
    }
  },
  
  onLoad() {
    // #ifdef MP-WEIXIN
    // 使用新的API替代getSystemInfoSync
    const windowInfo = uni.getWindowInfo()
    this.statusBarHeight = windowInfo.statusBarHeight || 0
    this.menuButtonInfo = uni.getMenuButtonBoundingClientRect()
    // #endif
  }
}
