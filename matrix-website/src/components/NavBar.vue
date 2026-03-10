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
        <router-link to="/pindou-generator" @click="closeMobileMenu">拼豆生成器</router-link>
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
import logoUrl from '@/assets/images/matrix-logo.svg'

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
  background: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(20px);
  border-bottom: 2px solid rgba(255, 107, 107, 0.15);
  position: sticky;
  top: 0;
  z-index: 1000;
  box-shadow: 0 4px 24px rgba(255, 107, 107, 0.08);
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
  color: #2d3748;
  font-size: 22px;
  font-weight: 700;
  transition: transform 0.3s ease;
}

.logo:hover {
  transform: scale(1.05);
}

.logo-image {
  width: 32px;
  height: 32px;
}

.logo-text {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  font-family: 'SF Mono', 'Monaco', 'Courier New', monospace;
  font-weight: 800;
  letter-spacing: 0.15em;
}

.nav-links {
  display: flex;
  gap: 30px;
}

.nav-links a {
  color: #4a5568;
  text-decoration: none;
  transition: all 0.3s ease;
  font-size: 15px;
  font-weight: 500;
  position: relative;
  padding: 8px 0;
}

.nav-links a::after {
  content: '';
  position: absolute;
  bottom: 0;
  left: 0;
  width: 0;
  height: 2px;
  background: linear-gradient(90deg, #ff6b6b 0%, #ffa500 100%);
  transition: width 0.3s ease;
}

.nav-links a:hover::after,
.nav-links a.router-link-active::after {
  width: 100%;
}

.nav-links a:hover,
.nav-links a.router-link-active {
  color: #ff6b6b;
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
  height: 3px;
  background: #ff6b6b;
  transition: all 0.3s;
  border-radius: 2px;
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
    background: rgba(255, 255, 255, 0.98);
    backdrop-filter: blur(20px);
    flex-direction: column;
    padding: 30px 20px;
    gap: 24px;
    transform: translateY(-100%);
    opacity: 0;
    transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
    pointer-events: none;
    box-shadow: 0 8px 32px rgba(255, 107, 107, 0.15);
  }

  .nav-links.active {
    transform: translateY(0);
    opacity: 1;
    pointer-events: all;
  }
  
  .nav-links a {
    font-size: 16px;
    padding: 12px 0;
    text-align: center;
  }
  
  .logo-text {
    font-size: 18px;
  }
  
  .logo-image {
    width: 28px;
    height: 28px;
  }
}
</style>
