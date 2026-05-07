<template>
  <span class="glx-inline-loader" :class="[`glx-inline-loader--${size}`]" aria-hidden="true">
    <span class="glx-inline-loader__dot"></span>
    <span class="glx-inline-loader__dot"></span>
    <span class="glx-inline-loader__dot"></span>
  </span>
</template>

<script setup>
defineProps({
  size: {
    type: String,
    default: "md",
  },
});
</script>

<style scoped>
.glx-inline-loader {
  display: inline-flex;
  align-items: center;
  gap: 6px;
}

.glx-inline-loader__dot {
  width: 10px;
  height: 10px;
  border: 2px solid #111111;
  background: #ffcb45;
  animation: glx-inline-loader-bounce 0.9s infinite ease-in-out;
}

.glx-inline-loader__dot:nth-child(2) {
  animation-delay: 0.12s;
}

.glx-inline-loader__dot:nth-child(3) {
  animation-delay: 0.24s;
}

.glx-inline-loader--sm .glx-inline-loader__dot {
  width: 8px;
  height: 8px;
}

.glx-inline-loader--lg .glx-inline-loader__dot {
  width: 14px;
  height: 14px;
}

@keyframes glx-inline-loader-bounce {
  0%,
  80%,
  100% {
    transform: translateY(0);
    opacity: 0.45;
  }

  40% {
    transform: translateY(-3px);
    opacity: 1;
  }
}
</style>
