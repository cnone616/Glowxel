<template>
  <button
    v-if="isVisible"
    class="back-to-top"
    type="button"
    aria-label="返回顶部"
    @click="scrollToTop"
  >
    <span class="back-to-top__arrow">↑</span>
  </button>
</template>

<script setup>
import { onMounted, onUnmounted, ref } from "vue";

const isVisible = ref(false);

const updateVisibility = () => {
  const scrollTop = window.scrollY || document.documentElement.scrollTop || 0;
  const viewportHeight = window.innerHeight || document.documentElement.clientHeight || 0;
  const documentHeight = document.documentElement.scrollHeight || 0;
  const isNearBottom = scrollTop + viewportHeight >= documentHeight - 160;
  isVisible.value = isNearBottom && scrollTop > 240;
};

const scrollToTop = () => {
  window.scrollTo({
    top: 0,
    behavior: "smooth",
  });
};

onMounted(() => {
  updateVisibility();
  window.addEventListener("scroll", updateVisibility, { passive: true });
  window.addEventListener("resize", updateVisibility);
});

onUnmounted(() => {
  window.removeEventListener("scroll", updateVisibility);
  window.removeEventListener("resize", updateVisibility);
});
</script>

<style scoped>
.back-to-top {
  position: fixed;
  right: 24px;
  bottom: 24px;
  width: 56px;
  height: 56px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border-width) solid var(--nb-ink);
  background: var(--tone-paper-soft);
  box-shadow: 6px 6px 0 #000000;
  color: var(--nb-ink);
  cursor: pointer;
  z-index: 1100;
}

.back-to-top:hover {
  background: var(--nb-blue);
}

.back-to-top__arrow {
  font-size: 28px;
  font-weight: 900;
  line-height: 1;
}

@media (max-width: 768px) {
  .back-to-top {
    right: 16px;
    bottom: 16px;
    width: 52px;
    height: 52px;
    box-shadow: 4px 4px 0 #000000;
  }

  .back-to-top__arrow {
    font-size: 24px;
  }
}
</style>
