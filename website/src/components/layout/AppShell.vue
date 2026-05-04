<template>
  <div class="glx-app-shell">
    <header class="glx-app-shell__header">
      <div class="glx-app-shell__header-inner">
        <router-link to="/" class="glx-app-shell__brand">
          <BrandLogo :show-tagline="false" />
        </router-link>

        <nav class="glx-app-shell__nav">
          <router-link
            v-for="item in appNavigation"
            :key="item.key"
            :to="item.to"
            class="glx-app-shell__nav-link"
            :class="{ 'is-active': activeKey === item.key }"
          >
            {{ item.label }}
          </router-link>
        </nav>

        <div class="glx-app-shell__actions">
          <span class="glx-app-shell__device-chip" :class="{ 'is-connected': deviceStore.connected }">
            {{ deviceStore.connected ? "设备已连接" : "设备未连接" }}
          </span>
        </div>
      </div>
    </header>

    <main class="glx-app-shell__content">
      <slot />
    </main>
  </div>
</template>

<script setup>
import { computed, onMounted } from "vue";
import { useRoute } from "vue-router";
import BrandLogo from "@/components/BrandLogo.vue";
import { appNavigation, resolveAppNavKey } from "@/config/appNavigation.js";
import { useDeviceStore } from "@/stores/device.js";

const route = useRoute();
const deviceStore = useDeviceStore();

const activeKey = computed(() => {
  return resolveAppNavKey(route.path);
});

onMounted(async () => {
  deviceStore.init();
  await deviceStore.restoreConnection();
});
</script>
