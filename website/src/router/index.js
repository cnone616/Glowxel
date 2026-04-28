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
  { path: '/design-preview', name: 'DesignPreview', component: () => import('@/views/DesignPreview.vue'), meta: { shell: 'app' } },
  { path: '/design-compare', name: 'DesignCompare', component: () => import('@/views/DesignCompare.vue'), meta: { shell: 'app' } },

  { path: '/device-control', name: 'DeviceControl', component: () => import('@/views/DeviceControl.vue'), meta: { shell: 'app' } },
  { path: '/ble-config', name: 'BleConfig', component: () => import('@/views/BleConfig.vue'), meta: { shell: 'app' } },
  { path: '/device-params', name: 'DeviceParams', component: () => import('@/views/DeviceParams.vue'), meta: { shell: 'app' } },
  {
    path: '/canvas-editor',
    name: 'CanvasEditor',
    component: () => import('@/views/DeviceModePage.vue'),
    meta: {
      shell: 'app',
      businessMode: 'canvas',
      modeTitle: '画板模式',
      modeEyebrow: 'Canvas',
      modeActionLabel: '切换到画板模式',
      modeDescription: '画板模式页负责把设备切到 `canvas`，并把创作入口和设备反馈链合在同一页里。',
      modeNotes: ['进入当前页后可以先完成模式切换，再回到编辑器继续画布创作。', '设备状态回显、发送前准备和参数入口都统一放在网站主端。'],
      modeActions: ['切到画板模式后再进入编辑器，避免发图时模式不一致。', '若画面或亮度异常，直接去设备参数页检查兼容项。'],
      modeLinks: [
        { label: '继续编辑', to: '/editor', tone: 'accent' },
        { label: '设备参数', to: '/device-params', tone: 'ghost' },
      ],
    },
  },

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
