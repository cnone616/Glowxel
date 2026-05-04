<script>
import { useDeviceStore } from './store/device.js'

export default {
  onLaunch: function() {
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
@import './styles/neubrutalism-global.css';
@import './styles/glx-style-system.css';

/* 非小程序平台全局重置 */
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
  background-color: var(--bg-primary);
  /* #endif */
}

/* 小程序全局 view 元素也需要 box-sizing */
/* #ifdef MP-WEIXIN */
view, scroll-view, swiper, text, input, textarea, button, image {
  box-sizing: border-box;
}
/* #endif */

/* 禁用橡皮筋效果 */
/* #ifndef MP-WEIXIN */
body {
  overscroll-behavior-y: none;
  overscroll-behavior-x: none;
}
/* #endif */
</style>
