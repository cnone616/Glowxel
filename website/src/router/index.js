import { createRouter, createWebHistory } from 'vue-router'
import { getStoredAuthToken } from '@/utils/session.js'

const routes = [
  { path: '/', name: 'Home', component: () => import('@/views/Home.vue'), meta: { shell: 'public' } },
  { path: '/community', name: 'Community', component: () => import('@/views/Community.vue'), meta: { shell: 'public' } },
  { path: '/templates', name: 'Templates', component: () => import('@/views/Templates.vue'), meta: { shell: 'public' } },
  { path: '/challenges', name: 'Challenges', component: () => import('@/views/Challenges.vue'), meta: { shell: 'public' } },
  { path: '/challenge/:id', name: 'ChallengeDetail', component: () => import('@/views/ChallengeDetail.vue'), meta: { shell: 'public' } },
  { path: '/artwork/:id', name: 'ArtworkDetail', component: () => import('@/views/ArtworkDetail.vue'), meta: { shell: 'public' } },
  { path: '/user/:id', name: 'UserProfile', component: () => import('@/views/UserProfile.vue'), meta: { shell: 'public' } },
  { path: '/user/:id/followers', name: 'UserFollowers', component: () => import('@/views/FollowList.vue'), meta: { shell: 'public' } },
  { path: '/user/:id/following', name: 'UserFollowing', component: () => import('@/views/FollowList.vue'), meta: { shell: 'public' } },
  { path: '/design-system', name: 'DesignSystem', component: () => import('@/views/DesignSystem.vue'), meta: { shell: 'public' } },
  { path: '/login', name: 'Login', component: () => import('@/views/Login.vue'), meta: { shell: 'public' } },

  { path: '/workspace', name: 'Workspace', component: () => import('@/views/Workspace.vue'), meta: { shell: 'app' } },
  { path: '/create', name: 'Create', component: () => import('@/views/Create.vue'), meta: { shell: 'app' } },
  { path: '/pattern-workbench', name: 'PatternWorkbench', component: () => import('@/views/PatternWorkbench.vue'), meta: { shell: 'app' } },
  { path: '/editor/:id?', name: 'Editor', component: () => import('@/views/Editor.vue'), meta: { shell: 'app' } },
  { path: '/gallery', name: 'Gallery', component: () => import('@/views/Gallery.vue'), meta: { shell: 'public' } },
  { path: '/overview/:id', name: 'Overview', component: () => import('@/views/Overview.vue'), meta: { shell: 'app', auth: true } },
  { path: '/assist/:id', name: 'Assist', component: () => import('@/views/Assist.vue'), meta: { shell: 'app', auth: true } },
  { path: '/publish-project/:id', name: 'PublishProject', component: () => import('@/views/PublishProject.vue'), meta: { shell: 'app', auth: true } },
  { path: '/profile', name: 'MyProfile', component: () => import('@/views/MyProfile.vue'), meta: { shell: 'app', auth: true } },
  { path: '/my-works', name: 'MyWorks', component: () => import('@/views/MyWorks.vue'), meta: { shell: 'app', auth: true } },
  { path: '/my-favorites', name: 'MyFavorites', component: () => import('@/views/MyFavorites.vue'), meta: { shell: 'app', auth: true } },
  { path: '/achievements', name: 'Achievements', component: () => import('@/views/Achievements.vue'), meta: { shell: 'app', auth: true } },
  { path: '/cloud-sync', name: 'CloudSync', component: () => import('@/views/CloudSync.vue'), meta: { shell: 'app', auth: true } },
  { path: '/settings', name: 'Settings', component: () => import('@/views/Settings.vue'), meta: { shell: 'app', auth: true } },
  { path: '/settings/profile', name: 'EditProfile', component: () => import('@/views/EditProfile.vue'), meta: { shell: 'app', auth: true } },
  { path: '/followers', name: 'MyFollowers', component: () => import('@/views/FollowList.vue'), meta: { shell: 'app', auth: true } },
  { path: '/following', name: 'MyFollowing', component: () => import('@/views/FollowList.vue'), meta: { shell: 'app', auth: true } },
  { path: '/design-compare', name: 'DesignCompare', component: () => import('@/views/DesignCompare.vue'), meta: { shell: 'app' } },

  { path: '/device-control', name: 'DeviceControl', component: () => import('@/views/DeviceControl.vue'), meta: { shell: 'app' } },
  { path: '/ble-config', name: 'BleConfig', component: () => import('@/views/BleConfig.vue'), meta: { shell: 'app' } },
  { path: '/device-params', name: 'DeviceParams', component: () => import('@/views/DeviceParams.vue'), meta: { shell: 'app' } },
  { path: '/clock', name: 'Clock', component: () => import('@/views/Clock.vue'), meta: { shell: 'app' } },
  { path: '/animation-clock', name: 'AnimationClock', component: () => import('@/views/AnimationClock.vue'), meta: { shell: 'app' } },
  { path: '/theme-clock', name: 'ThemeClock', component: () => import('@/views/ThemeClock.vue'), meta: { shell: 'app' } },
  { path: '/spirit-screen', name: 'SpiritScreen', component: () => import('@/views/SpiritScreen.vue'), meta: { shell: 'app' } },
  { path: '/canvas-editor', name: 'CanvasEditor', component: () => import('@/views/CanvasEditor.vue'), meta: { shell: 'app' } },
  { path: '/gif-player', name: 'GifPlayer', component: () => import('@/views/GifPlayer.vue'), meta: { shell: 'app' } },
  { path: '/led-matrix', name: 'LedMatrix', component: () => import('@/views/LedMatrix.vue'), meta: { shell: 'app' } },
  { path: '/maze-mode', name: 'MazeMode', component: () => import('@/views/MazeMode.vue'), meta: { shell: 'app' } },
  { path: '/snake-mode', name: 'SnakeMode', component: () => import('@/views/SnakeMode.vue'), meta: { shell: 'app' } },
  { path: '/planet-screensaver', name: 'PlanetScreensaver', component: () => import('@/views/PlanetScreensaver.vue'), meta: { shell: 'app' } },
  { path: '/tetris-settings', name: 'TetrisSettings', component: () => import('@/views/TetrisSettings.vue'), meta: { shell: 'app' } },
  { path: '/tetris-clock-settings', name: 'TetrisClockSettings', component: () => import('@/views/TetrisClockSettings.vue'), meta: { shell: 'app' } },
  { path: '/water-world', name: 'WaterWorld', component: () => import('@/views/WaterWorld.vue'), meta: { shell: 'app' } },

]

const router = createRouter({
  history: createWebHistory(),
  routes,
  scrollBehavior(to, from, savedPosition) {
    return savedPosition || { top: 0 }
  }
})

router.beforeEach((to) => {
  if (to.meta.auth && getStoredAuthToken().length === 0) {
    return { name: 'Login', query: { redirect: to.fullPath } }
  }
})

export default router
