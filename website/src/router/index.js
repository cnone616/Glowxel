import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  {
    path: '/',
    name: 'Home',
    component: () => import('@/views/Home.vue')
  },
  {
    path: '/community',
    name: 'Community',
    component: () => import('@/views/Community.vue')
  },
  {
    path: '/create',
    name: 'Create',
    component: () => import('@/views/Create.vue')
  },
  {
    path: '/templates',
    name: 'Templates',
    component: () => import('@/views/Templates.vue')
  },
  {
    path: '/challenges',
    name: 'Challenges',
    component: () => import('@/views/Challenges.vue')
  },
  {
    path: '/artwork/:id',
    name: 'ArtworkDetail',
    component: () => import('@/views/ArtworkDetail.vue')
  },
  {
    path: '/user/:id',
    name: 'UserProfile',
    component: () => import('@/views/UserProfile.vue')
  },
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/Login.vue')
  },
  {
    path: '/admin',
    name: 'Admin',
    component: () => import('@/views/admin/Admin.vue'),
    children: [
      { path: '', name: 'AdminDashboard', component: () => import('@/views/admin/Dashboard.vue') },
      { path: 'users', name: 'AdminUsers', component: () => import('@/views/admin/Users.vue') },
      { path: 'artworks', name: 'AdminArtworks', component: () => import('@/views/admin/Artworks.vue') },
      { path: 'templates', name: 'AdminTemplates', component: () => import('@/views/admin/AdminTemplates.vue') },
      { path: 'challenges', name: 'AdminChallenges', component: () => import('@/views/admin/AdminChallenges.vue') }
    ]
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
