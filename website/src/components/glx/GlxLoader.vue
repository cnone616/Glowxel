<template>
  <span class="glx-loader" aria-hidden="true">
    <span
      v-for="cell in cells"
      :key="cell"
      class="glx-loader__cell"
    ></span>
  </span>
</template>

<script setup>
const cells = [1, 2, 3, 4, 5, 6, 7, 8, 9];
</script>
