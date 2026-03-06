<template>
  <nav class="navbar">
    <div class="container">
      <router-link to="/" class="logo">
        <img :src="logoUrl" alt="MATRIX" class="logo-image">
        <span class="logo-text">MATRIX</span>
      </router-link>
      
      <div class="nav-links" :class="{ active: mobileMenuOpen }">
        <router-link to="/" @click="closeMobileMenu">首页</router-link>
        <router-link to="/showcase" @click="closeMobileMenu">作品展示</router-link>
        <router-link to="/clock-designer" @click="closeMobileMenu">时钟设计</router-link>
        <router-link to="/pattern-editor" @click="closeMobileMenu">图案编辑</router-link>
        <router-link to="/docs" @click="closeMobileMenu">文档</router-link>
      </div>

      <button class="mobile-menu-btn" @click="toggleMobileMenu">
        <span></span>
        <span></span>
        <span></span>
      </button>
    </div>
  </nav>
</template>

<script setup>
import { ref } from 'vue'
import logoUrl from '@/assets/images/matrix-logo.png'

const mobileMenuOpen = ref(false)

const toggleMobileMenu = () => {
  mobileMenuOpen.value = !mobileMenuOpen.value
}

const closeMobileMenu = () => {
  mobileMenuOpen.value = false
}
</script>

<style scoped>
.navbar {
  background: rgba(10, 10, 10, 0.95);
  backdrop-filter: blur(10px);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
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
  height: 70px;
}

.logo {
  display: flex;
  align-items: center;
  gap: 12px;
  text-decoration: none;
  color: #fff;
  font-size: 20px;
  font-weight: 600;
}

.logo-image {
  width: 32px;
  height: 32px;
}

.logo-text {
  color: #00f3ff;
  font-family: monospace;
  font-weight: bold;
  letter-spacing: 0.15em;
}

.nav-links {
  display: flex;
  gap: 30px;
}

.nav-links a {
  color: rgba(255, 255, 255, 0.7);
  text-decoration: none;
  transition: color 0.3s;
  font-size: 15px;
}

.nav-links a:hover,
.nav-links a.router-link-active {
  color: #00f3ff;
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
  width: 25px;
  height: 2px;
  background: #fff;
  transition: all 0.3s;
}

@media (max-width: 768px) {
  .mobile-menu-btn {
    display: flex;
  }

  .nav-links {
    position: fixed;
    top: 70px;
    left: 0;
    right: 0;
    background: rgba(10, 10, 10, 0.98);
    flex-direction: column;
    padding: 20px;
    gap: 20px;
    transform: translateY(-100%);
    opacity: 0;
    transition: all 0.3s;
    pointer-events: none;
  }

  .nav-links.active {
    transform: translateY(0);
    opacity: 1;
    pointer-events: all;
  }
}
</style>
