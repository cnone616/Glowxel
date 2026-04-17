<template>
  <view
    class="glx-skeleton"
    :style="{
      width: width,
      height: height,
      borderRadius: radius,
    }"
  ></view>
</template>

<script>
export default {
  name: "GlxSkeleton",
  props: {
    width: {
      type: String,
      default: "100%",
    },
    height: {
      type: String,
      default: "28rpx",
    },
    radius: {
      type: String,
      default: "20rpx",
    },
  },
};
</script>

<style scoped lang="scss">
.glx-skeleton {
  background: linear-gradient(
    90deg,
    rgba(123, 171, 177, 0.12) 0%,
    rgba(123, 171, 177, 0.22) 50%,
    rgba(123, 171, 177, 0.12) 100%
  );
  background-size: 240% 100%;
  animation: glxSkeletonShimmer 1.4s linear infinite;
}

@keyframes glxSkeletonShimmer {
  from {
    background-position: 200% 0;
  }

  to {
    background-position: -40% 0;
  }
}
</style>
