<template>
  <div id="app">
    <PublicShell v-if="shellName === 'public'">
      <main class="main-content">
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </main>
    </PublicShell>

    <AppShell v-else-if="shellName === 'app'">
      <main class="main-content">
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </main>
    </AppShell>

    <main v-else class="main-content">
      <router-view v-slot="{ Component }">
        <transition name="fade" mode="out-in">
          <component :is="Component" />
        </transition>
      </router-view>
    </main>

    <GlxBlockingLayer />
    <GlxToastViewport />
  </div>
</template>

<script setup>
import { computed, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import PublicShell from '@/components/layout/PublicShell.vue'
import AppShell from '@/components/layout/AppShell.vue'
import GlxBlockingLayer from '@/components/glx/GlxBlockingLayer.vue'
import GlxToastViewport from '@/components/glx/GlxToastViewport.vue'
import { useUserStore } from '@/stores/user.js'

const route = useRoute()
const userStore = useUserStore()

const shellName = computed(() => {
  if (typeof route.meta.shell === 'string') {
    return route.meta.shell
  }
  return 'public'
})

onMounted(async () => {
  await userStore.init()
})
</script>

<style>
#app {
  min-height: 100vh;
}

.main-content {
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
</style>
