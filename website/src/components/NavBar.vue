<template>
  <nav class="navbar">
    <div class="nav-container">
      <router-link to="/" class="logo">
        <img :src="iconUrl" alt="Glowxel" class="logo-icon" />
        <img :src="logoUrl" alt="Glowxel" class="logo-logo" />
      </router-link>

      <div class="nav-links" :class="{ active: mobileMenuOpen }">
        <router-link to="/" @click="closeMobileMenu">首页</router-link>
        <router-link to="/workspace" @click="closeMobileMenu"
          >工作台</router-link
        >
        <router-link to="/community" @click="closeMobileMenu">社区</router-link>
        <router-link to="/templates" @click="closeMobileMenu">模板</router-link>
        <router-link to="/challenges" @click="closeMobileMenu"
          >挑战</router-link
        >
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
        <router-link to="/login" class="btn-login" @click="closeMobileMenu"
          >登录</router-link
        >
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
import { ref } from "vue";
import iconUrl from "@/assets/images/glowxel-icon.svg";
import logoUrl from "@/assets/images/glowxel-logo.png";

const mobileMenuOpen = ref(false);
const toggleMobileMenu = () => {
  mobileMenuOpen.value = !mobileMenuOpen.value;
};
const closeMobileMenu = () => {
  mobileMenuOpen.value = false;
};
</script>

<style scoped>
.navbar {
  background: #fff;
  border-bottom: 1px solid var(--color-border);
  position: sticky;
  top: 0;
  z-index: 1000;
}

.nav-container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 24px;
  display: flex;
  align-items: center;
  height: 60px;
  gap: 32px;
}

.logo {
  display: flex;
  align-items: center;
  gap: 8px;
  text-decoration: none;
  flex-shrink: 0;
}
.logo-icon {
  width: 26px;
  height: 26px;
}
.logo-logo {
  height: 15px;
  width: auto;
}

.nav-links {
  display: flex;
  gap: 4px;
  flex: 1;
}
.nav-links a {
  color: var(--color-text-secondary);
  text-decoration: none;
  font-size: 14px;
  font-weight: 500;
  padding: 6px 12px;
  border-radius: var(--radius-sm);
  transition: all 0.15s;
}
.nav-links a:hover {
  color: var(--color-text-primary);
  background: var(--color-bg-muted);
}
.nav-links a.router-link-exact-active {
  color: var(--color-text-primary);
  background: var(--color-bg-muted);
}

.nav-actions {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}

.btn-create {
  display: inline-flex;
  align-items: center;
  gap: 5px;
  padding: 7px 14px;
  background: var(--color-primary);
  color: #fff;
  border-radius: var(--radius-sm);
  font-size: 13px;
  font-weight: 500;
  text-decoration: none;
  transition: background 0.15s;
}
.btn-create:hover {
  background: var(--color-primary-hover);
}

.btn-login {
  padding: 7px 14px;
  color: var(--color-text-secondary);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-sm);
  font-size: 13px;
  font-weight: 500;
  text-decoration: none;
  transition: all 0.15s;
}
.btn-login:hover {
  color: var(--color-text-primary);
  border-color: var(--color-border-hover);
}

.mobile-menu-btn {
  display: none;
  background: none;
  border: none;
  cursor: pointer;
  padding: 6px;
  color: var(--color-text-primary);
  border-radius: var(--radius-sm);
}
.mobile-menu-btn:hover {
  background: var(--color-bg-muted);
}

@media (max-width: 768px) {
  .nav-container {
    gap: 0;
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
    top: 60px;
    left: 0;
    right: 0;
    background: #fff;
    flex-direction: column;
    padding: 12px 16px 20px;
    gap: 2px;
    border-bottom: 1px solid var(--color-border);
    box-shadow: var(--shadow-md);
  }
  .nav-links.active {
    display: flex;
  }
  .nav-links a {
    padding: 10px 12px;
    font-size: 15px;
  }
}
</style>
