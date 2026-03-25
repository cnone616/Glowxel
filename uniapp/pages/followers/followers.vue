<template>
  <view class="redirect-page"></view>
</template>

<script>
import { useUserStore } from "../../store/user.js";

export default {
  onLoad() {
    const userStore = useUserStore();
    if (!userStore.currentUser || !userStore.currentUser.id) {
      uni.showToast({
        title: "请先登录",
        icon: "none",
      });
      setTimeout(() => {
        uni.navigateBack();
      }, 500);
      return;
    }

    uni.redirectTo({
      url: `/pages/user-list/user-list?type=followers&userId=${userStore.currentUser.id}`,
    });
  },
};
</script>

<style scoped>
.redirect-page {
  min-height: 100vh;
}
</style>
