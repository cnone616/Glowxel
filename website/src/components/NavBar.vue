<template>
  <nav class="navbar">
    <div class="nav-container">
      <router-link to="/" class="logo">
        <BrandLogo />
      </router-link>

      <div class="nav-links" :class="{ active: mobileMenuOpen }">
        <router-link
          v-for="item in navItems"
          :key="item.key"
          :to="item.to"
          :class="{ 'is-active': activeNavKey === item.key }"
          @click="closeMobileMenu"
        >
          {{ item.label }}
        </router-link>
      </div>

      <div class="nav-actions">
        <router-link to="/create" class="btn-create" @click="closeMobileMenu">
          <svg
            width="14"
            height="14"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2.5"
          >
            <line x1="12" y1="5" x2="12" y2="19" />
            <line x1="5" y1="12" x2="19" y2="12" />
          </svg>
          新建
        </router-link>
        <template v-if="isLoggedIn">
          <router-link to="/profile" class="btn-login" @click="closeMobileMenu">我的</router-link>
          <button class="btn-login btn-logout" @click="handleLogout">
            退出
          </button>
        </template>
        <router-link
          v-else
          to="/login"
          class="btn-login"
          @click="closeMobileMenu"
        >
          登录
        </router-link>
      </div>

      <button
        class="mobile-menu-btn"
        @click="toggleMobileMenu"
        :aria-expanded="mobileMenuOpen"
        aria-label="菜单"
      >
        <svg
          v-if="!mobileMenuOpen"
          width="20"
          height="20"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
        >
          <line x1="3" y1="6" x2="21" y2="6" />
          <line x1="3" y1="12" x2="21" y2="12" />
          <line x1="3" y1="18" x2="21" y2="18" />
        </svg>
        <svg
          v-else
          width="20"
          height="20"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
        >
          <line x1="18" y1="6" x2="6" y2="18" />
          <line x1="6" y1="6" x2="18" y2="18" />
        </svg>
      </button>
    </div>
  </nav>
</template>

<script setup>
import { computed, ref } from "vue";
import { useRoute, useRouter } from "vue-router";
import BrandLogo from "@/components/BrandLogo.vue";
import { clearStoredSession } from "@/utils/session.js";

const router = useRouter();
const route = useRoute();
const mobileMenuOpen = ref(false);
const isLoggedIn = computed(() => !!localStorage.getItem("auth_token"));
const navItems = [
  { key: "home", label: "首页", to: "/" },
  { key: "workspace", label: "工作台", to: "/workspace" },
  { key: "device", label: "设备", to: "/device-control" },
  { key: "community", label: "社区", to: "/community" },
  { key: "design-system", label: "控件库", to: "/design-system" },
  { key: "templates", label: "边框", to: "/templates" },
  { key: "challenges", label: "挑战", to: "/challenges" },
];

const activeNavKey = computed(() => {
  const path = route.path;
  if (path === "/") return "home";
  if (
    path.startsWith("/workspace") ||
    path.startsWith("/create") ||
    path.startsWith("/pattern-workbench") ||
    path.startsWith("/editor")
  ) {
    return "workspace";
  }
  if (
    path.startsWith("/device-control") ||
    path.startsWith("/device-params") ||
    path.startsWith("/ble-config") ||
    path.startsWith("/canvas-editor")
  ) {
    return "device";
  }
  if (path.startsWith("/design-system")) return "design-system";
  if (path.startsWith("/templates")) return "templates";
  if (path.startsWith("/challenges") || path.startsWith("/challenge/")) {
    return "challenges";
  }
  if (
    path.startsWith("/community") ||
    path.startsWith("/artwork/") ||
    path.startsWith("/user/") ||
    path === "/followers" ||
    path === "/following"
  ) {
    return "community";
  }
  return "";
});
const toggleMobileMenu = () => {
  mobileMenuOpen.value = !mobileMenuOpen.value;
};
const closeMobileMenu = () => {
  mobileMenuOpen.value = false;
};
const handleLogout = () => {
  clearStoredSession();
  closeMobileMenu();
  router.push("/login");
};
</script>

<style scoped>
.navbar {
  background: var(--nb-paper);
  border-bottom: var(--nb-border-width) solid var(--nb-ink);
  position: sticky;
  top: 0;
  z-index: 1000;
}

.nav-container {
  max-width: var(--content-max-width);
  margin: 0 auto;
  padding: 0 24px;
  display: flex;
  align-items: center;
  min-height: 88px;
  gap: 28px;
}

.logo {
  display: flex;
  align-items: center;
  text-decoration: none;
  flex-shrink: 0;
  padding: 0;
}

.nav-links {
  display: flex;
  align-items: center;
  gap: 28px;
  flex: 1;
  flex-wrap: wrap;
}

.nav-links a {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
  text-decoration: none;
  font-size: 15px;
  font-weight: 700;
  min-height: 42px;
  padding: 0 14px;
  line-height: 1;
  border: var(--nb-border-width) solid transparent;
  box-shadow: none;
  transition:
    color 0.15s ease,
    background-color 0.15s ease,
    border-color 0.15s ease,
    box-shadow 0.15s ease;
}

.nav-links a:hover {
  color: var(--nb-ink);
  background: #f4f4f4;
}

.nav-links a.is-active {
  color: var(--nb-ink);
  font-weight: 900;
  background: var(--nb-yellow);
  border-color: var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.nav-actions {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-shrink: 0;
}

.btn-create {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  min-height: 40px;
  padding: 8px 16px;
  background: var(--nb-yellow);
  color: var(--nb-ink);
  border: var(--nb-border-width) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  font-size: 13px;
  font-weight: 800;
  text-decoration: none;
  transition: background-color 0.15s ease;
}

.btn-create:hover {
  background: var(--color-primary-hover);
}

.btn-login {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 40px;
  padding: 8px 16px;
  color: var(--text-secondary);
  border: var(--nb-border-width) solid var(--nb-ink);
  background: var(--tone-paper-soft);
  box-shadow: var(--nb-shadow-soft);
  font-size: 13px;
  font-weight: 800;
  text-decoration: none;
  transition:
    background-color 0.15s ease,
    color 0.15s ease;
}

.btn-login:hover {
  color: var(--nb-ink);
  background: var(--nb-muted);
}

.btn-logout {
  background: var(--tone-paper-soft);
  cursor: pointer;
}

.mobile-menu-btn {
  display: none;
  align-items: center;
  justify-content: center;
  width: 44px;
  height: 44px;
  background: var(--tone-paper-soft);
  border: var(--nb-border-width) solid var(--nb-ink);
  box-shadow: 3px 3px 0 #000000;
  cursor: pointer;
  padding: 0;
  color: var(--nb-text-primary);
}

.mobile-menu-btn:hover {
  background: var(--nb-muted);
}

@media (max-width: 768px) {
  .nav-container {
    min-height: 72px;
    gap: 0;
  }

  .logo {
    padding: 4px 0;
  }

  .mobile-menu-btn {
    display: flex;
    margin-left: auto;
  }
  .nav-actions {
    display: none;
  }

  .nav-links {
    display: none;
    position: fixed;
    top: 72px;
    left: 12px;
    right: 12px;
    background: var(--nb-paper);
    flex-direction: column;
    padding: 14px;
    gap: 8px;
    border: var(--nb-border-width) solid var(--nb-ink);
    box-shadow: var(--nb-shadow-strong);
  }

  .nav-links.active {
    display: flex;
  }

  .nav-links a {
    width: 100%;
    justify-content: center;
    min-height: 42px;
    padding: 0 12px;
    font-size: 15px;
    border: var(--nb-border-width) solid var(--nb-ink);
    background: #ffffff;
    box-shadow: var(--nb-shadow-soft);
  }

  .nav-links a.is-active {
    background: var(--nb-yellow);
  }
}
</style>
