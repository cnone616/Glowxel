import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/Login.vue')
  },
  {
    path: '/',
    component: () => import('@/views/Layout.vue'),
    redirect: '/dashboard',
    children: [
      { path: 'dashboard', name: 'Dashboard', component: () => import('@/views/Dashboard.vue'), meta: { title: '数据概览' } },
      { path: 'users', name: 'Users', component: () => import('@/views/Users.vue'), meta: { title: '用户管理' } },
      { path: 'artworks', name: 'Artworks', component: () => import('@/views/Artworks.vue'), meta: { title: '作品管理' } },
      { path: 'templates', name: 'Templates', component: () => import('@/views/Templates.vue'), meta: { title: '模板管理' } },
      { path: 'challenges', name: 'Challenges', component: () => import('@/views/Challenges.vue'), meta: { title: '挑战管理' } },
      { path: 'activities', name: 'Activities', component: () => import('@/views/Activities.vue'), meta: { title: '活动管理' } },
      { path: 'comments', name: 'Comments', component: () => import('@/views/Comments.vue'), meta: { title: '评论管理' } }
    ]
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('admin_token')
  if (to.path !== '/login' && !token) {
    next('/login')
  } else {
    next()
  }
})

export default router

