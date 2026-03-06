<template>
  <text 
    class="iconfont" 
    :class="'icon-' + name"
    :style="{ 
      fontSize: size + 'rpx', 
      color: color 
    }"
  ></text>
</template>

<script>
export default {
  name: 'Icon',
  props: {
    name: {
      type: String,
      required: true
    },
    size: {
      type: Number,
      default: 48
    },
    color: {
      type: String,
      default: 'currentColor'
    }
  }
}
</script>

<style>
@import url("../static/iconfont/iconfont.css");
</style>
