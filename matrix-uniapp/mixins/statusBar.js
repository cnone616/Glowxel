// Status bar mixin for handling status bar height across different platforms
export default {
  data() {
    return {
      statusBarHeight: 0
    }
  },
  
  onLoad() {
    this.initStatusBar()
  },
  
  methods: {
    initStatusBar() {
      // #ifdef MP-WEIXIN
      const systemInfo = uni.getSystemInfoSync()
      this.statusBarHeight = systemInfo.statusBarHeight || 0
      // #endif
      
      // #ifdef H5
      this.statusBarHeight = 0
      // #endif
      
      // #ifdef APP-PLUS
      const systemInfo = uni.getSystemInfoSync()
      this.statusBarHeight = systemInfo.statusBarHeight || 0
      // #endif
    }
  }
}
