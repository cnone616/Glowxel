<template>
  <div class="glx-public-shell">
    <NavBar />
    <main class="glx-public-shell__main">
      <slot />
    </main>
    <Footer />
  </div>
</template>

<script setup>
import NavBar from "@/components/NavBar.vue";
import Footer from "@/components/Footer.vue";
</script>

<style scoped>
.glx-public-shell {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.glx-public-shell__main {
  flex: 1;
  width: 100%;
}
</style>
