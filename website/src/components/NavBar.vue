<template>
  <nav class="navbar">
    <div class="container">
      <router-link to="/" class="logo">
        <img :src="iconUrl" alt="Glowxel" class="logo-icon">
        <span class="logo-text">Glowxel</span>
      </router-link>

      <div class="nav-links" :class="{ active: mobileMenuOpen }">
        <router-link to="/" @click="closeMobileMenu">首页</router-link>
        <router-link to="/community" @click="closeMobileMenu">社区</router-link>
        <router-link to="/create" @click="closeMobileMenu">创作</router-link>
        <router-link to="/templates" @click="closeMobileMenu">模板</router-link>
        <router-link to="/challenges" @click="closeMobileMenu">挑战</router-link>
      </div>

      <button class="mobile-menu-btn" @click="toggleMobileMenu" aria-label="菜单">
        <span></span>
        <span></span>
        <span></span>
      </button>
    </div>
  </nav>
</template>

<script setup>
import { ref } from 'vue'
import iconUrl from '@/assets/images/glowxel-icon.svg'

const mobileMenuOpen = ref(false)
const toggleMobileMenu = () => { mobileMenuOpen.value = !mobileMenuOpen.value }
const closeMobileMenu = () => { mobileMenuOpen.value = false }
</script>

<style scoped>
.navbar {
  background: #fff;
  border-bottom: 1px solid #f0f0f0;
  position: sticky;
  top: 0;
  z-index: 1000;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  height: 60px;
}

.logo {
  display: flex;
  align-items: center;
  gap: 10px;
  text-decoration: none;
}

.logo-icon {
  width: 28px;
  height: 28px;
}

.logo-text {
  font-size: 20px;
  font-weight: 700;
  color: #1a1a1a;
  letter-spacing: 0.02em;
}

.nav-links {
  display: flex;
  gap: 32px;
}

.nav-links a {
  color: #666;
  text-decoration: none;
  font-size: 15px;
  font-weight: 500;
  padding: 6px 0;
  transition: color 0.2s;
}

.nav-links a:hover,
.nav-links a.router-link-active {
  color: #1a1a1a;
}

.mobile-menu-btn {
  display: none;
  flex-direction: column;
  gap: 5px;
  background: none;
  border: none;
  cursor: pointer;
  padding: 5px;
}

.mobile-menu-btn span {
  width: 22px;
  height: 2px;
  background: #1a1a1a;
  border-radius: 1px;
}

@media (max-width: 768px) {
  .mobile-menu-btn { display: flex; }

  .nav-links {
    position: fixed;
    top: 60px;
    left: 0;
    right: 0;
    background: #fff;
    flex-direction: column;
    padding: 20px;
    gap: 16px;
    transform: translateY(-100%);
    opacity: 0;
    transition: all 0.3s ease;
    pointer-events: none;
    border-bottom: 1px solid #f0f0f0;
  }

  .nav-links.active {
    transform: translateY(0);
    opacity: 1;
    pointer-events: all;
  }
}
</style>
