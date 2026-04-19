import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  { path: '/', name: 'Home', component: () => import('@/views/Home.vue') },
  { path: '/community', name: 'Community', component: () => import('@/views/Community.vue') },
  { path: '/workspace', name: 'Workspace', component: () => import('@/views/Workspace.vue') },
  { path: '/create', name: 'Create', component: () => import('@/views/Create.vue') },
  { path: '/pattern-workbench', name: 'PatternWorkbench', component: () => import('@/views/PatternWorkbench.vue') },
  { path: '/clock-editor', name: 'ClockEditor', component: () => import('@/views/ClockEditor.vue') },
  { path: '/device-control', name: 'DeviceControl', component: () => import('@/views/DeviceControl.vue') },
  { path: '/design-system', name: 'DesignSystem', component: () => import('@/views/DesignSystem.vue') },
  { path: '/editor/:id?', name: 'Editor', component: () => import('@/views/Editor.vue'), meta: { auth: true } },
  { path: '/templates', name: 'Templates', component: () => import('@/views/Templates.vue') },
  { path: '/challenges', name: 'Challenges', component: () => import('@/views/Challenges.vue') },
  { path: '/challenge/:id', name: 'ChallengeDetail', component: () => import('@/views/ChallengeDetail.vue') },
  { path: '/artwork/:id', name: 'ArtworkDetail', component: () => import('@/views/ArtworkDetail.vue') },
  { path: '/user/:id', name: 'UserProfile', component: () => import('@/views/UserProfile.vue') },
  { path: '/user/:id/followers', name: 'UserFollowers', component: () => import('@/views/FollowList.vue') },
  { path: '/user/:id/following', name: 'UserFollowing', component: () => import('@/views/FollowList.vue') },
  { path: '/login', name: 'Login', component: () => import('@/views/Login.vue') },
  { path: '/profile', name: 'MyProfile', component: () => import('@/views/MyProfile.vue'), meta: { auth: true } },
  { path: '/settings', name: 'Settings', component: () => import('@/views/Settings.vue'), meta: { auth: true } },
  { path: '/settings/profile', name: 'EditProfile', component: () => import('@/views/EditProfile.vue'), meta: { auth: true } },
  { path: '/followers', name: 'MyFollowers', component: () => import('@/views/FollowList.vue'), meta: { auth: true } },
  { path: '/following', name: 'MyFollowing', component: () => import('@/views/FollowList.vue'), meta: { auth: true } },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
  scrollBehavior(to, from, savedPosition) {
    return savedPosition || { top: 0 }
  }
})

// 登录守卫
router.beforeEach((to) => {
  if (to.meta.auth && !localStorage.getItem('auth_token')) {
    return { name: 'Login', query: { redirect: to.fullPath } }
  }
})

export default router
