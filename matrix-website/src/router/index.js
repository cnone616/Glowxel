import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  {
    path: '/',
    name: 'Home',
    component: () => import('@/views/Home.vue')
  },
  {
    path: '/showcase',
    name: 'Showcase',
    component: () => import('@/views/Showcase.vue')
  },
  {
    path: '/clock-designer',
    name: 'ClockDesigner',
    component: () => import('@/views/ClockDesigner.vue')
  },
  {
    path: '/pindou-generator',
    name: 'PindouGenerator',
    component: () => import('@/views/PindouGenerator.vue')
  },
  {
    path: '/docs',
    name: 'Docs',
    component: () => import('@/views/Docs.vue')
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes,
  scrollBehavior(to, from, savedPosition) {
    if (savedPosition) {
      return savedPosition
    }
    return { top: 0 }
  }
})

export default router
