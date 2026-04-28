<template>
  <div class="glx-app-shell">
    <header class="glx-app-shell__header">
      <div class="glx-app-shell__header-inner">
        <router-link to="/workspace" class="glx-app-shell__brand">
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
          <router-link to="/create" class="glx-button glx-button--primary">
            新建
          </router-link>
          <router-link
            v-if="userStore.isLoggedIn"
            to="/profile"
            class="glx-button glx-button--ghost"
          >
            我的
          </router-link>
          <router-link
            v-else
            to="/login"
            class="glx-button glx-button--ghost"
          >
            登录
          </router-link>
          <button
            v-if="userStore.isLoggedIn"
            type="button"
            class="glx-button glx-button--ghost"
            @click="handleLogout"
          >
            退出
          </button>
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
import { useRoute, useRouter } from "vue-router";
import BrandLogo from "@/components/BrandLogo.vue";
import { appNavigation, resolveAppNavKey } from "@/config/appNavigation.js";
import { useUserStore } from "@/stores/user.js";
import { useDeviceStore } from "@/stores/device.js";

const router = useRouter();
const route = useRoute();
const userStore = useUserStore();
const deviceStore = useDeviceStore();

const activeKey = computed(() => {
  return resolveAppNavKey(route.path);
});

function handleLogout() {
  userStore.logout();
  router.push("/");
}

onMounted(async () => {
  deviceStore.init();
  await deviceStore.restoreConnection();
});
</script>
