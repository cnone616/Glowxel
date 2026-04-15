<template>
  <div id="app" :class="{ 'app-admin': isAdmin }">
    <NavBar v-if="!isAdmin" />
    <main class="main-content">
      <router-view v-slot="{ Component }">
        <transition name="fade" mode="out-in">
          <component :is="Component" />
        </transition>
      </router-view>
    </main>
    <Footer v-if="!isAdmin" />
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import NavBar from './components/NavBar.vue'
import Footer from './components/Footer.vue'

const route = useRoute()
const isAdmin = computed(() => route.path.startsWith('/admin'))
</script>

<style>
#app {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.main-content {
  flex: 1;
  position: relative;
  z-index: 1;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

#app:not(.app-admin) .main-content {
  padding-bottom: 24px;
}
</style>
