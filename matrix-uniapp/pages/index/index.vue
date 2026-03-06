<template>
  <view class="index-page">
    <text>Index Page - 这个页面应该重定向到 library</text>
  </view>
</template>

<script>
export default {
  onLoad() {
    // 重定向到 library 页面
    uni.reLaunch({
      url: '/pages/library/library'
    })
  }
}
</script>

<style scoped>
.index-page {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100vh;
}
</style>
