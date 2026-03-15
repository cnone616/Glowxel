<script>
import { useProjectStore } from './store/project.js'
import { useUserStore } from './store/user.js'
import { useDeviceStore } from './store/device.js'

export default {
  onLaunch: function() {
    // 初始化项目数据
    const projectStore = useProjectStore()
    projectStore.init()
    
    // 初始化用户状态
    const userStore = useUserStore()
    userStore.init()
    
    // 初始化设备连接
    const deviceStore = useDeviceStore()
    deviceStore.init()
  },
  onShow: function() {
  },
  onHide: function() {
  }
}
</script>

<style>
@import './styles/theme.css';
@import './static/iconfont/iconfont.css';

/* 全局样式 */
/* #ifndef MP-WEIXIN */
* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

html, body {
  width: 100%;
  height: 100%;
  overflow: hidden;
  position: fixed;
  overscroll-behavior: none;
  -webkit-overflow-scrolling: touch;
}
/* #endif */

page {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Roboto', 'Helvetica Neue', Arial, sans-serif;
  background-color: var(--bg-primary);
  color: var(--text-primary);
  width: 100%;
  height: 100%;
  /* #ifdef H5 */
  overflow: hidden;
  position: fixed;
  overscroll-behavior: none;
  /* #endif */
  
  /* #ifdef MP-WEIXIN */
  /* 小程序默认背景色，避免白屏 */
  background-color: #1a1a1a;
  /* #endif */
}

/* 禁用橡皮筋效果 */
/* #ifndef MP-WEIXIN */
body {
  overscroll-behavior-y: none;
  overscroll-behavior-x: none;
}
/* #endif */
</style>
