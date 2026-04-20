import { createRouter, createWebHistory } from 'vue-router'
import LibraryView from '../views/LibraryView.vue'
import StudioView from '../views/StudioView.vue'
import ValidateView from '../views/ValidateView.vue'
import PublishView from '../views/PublishView.vue'

const routes = [
  { path: '/', name: 'library', component: LibraryView },
  { path: '/studio/:id', name: 'studio', component: StudioView, props: true },
  { path: '/editor/:id', redirect: (to) => ({ name: 'studio', params: to.params }) },
  { path: '/validate/:id', name: 'validate', component: ValidateView, props: true },
  { path: '/publish/:id', name: 'publish', component: PublishView, props: true },
]

export const router = createRouter({
  history: createWebHistory(),
  routes,
})
