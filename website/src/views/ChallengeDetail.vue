<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Challenge Detail</span>
      <h1 class="glx-page-shell__title">{{ challenge.title || "挑战详情" }}</h1>
      <p class="glx-page-shell__desc">{{ challenge.description || "当前挑战暂无描述。" }}</p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">状态</span>
          <strong class="glx-hero-metric__value">{{ statusText(challenge.status) }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">截止时间</span>
          <strong class="glx-hero-metric__value">{{ challenge.end_date ? challenge.end_date.slice(0, 10) : "--" }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">参与人数</span>
          <strong class="glx-hero-metric__value">{{ challenge.participants || 0 }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">作品数量</span>
          <strong class="glx-hero-metric__value">{{ submissions.length }}</strong>
        </article>
      </div>
      <div class="glx-inline-actions" v-if="challenge.status === 'active'">
        <button type="button" class="glx-button glx-button--primary" @click="handleJoin">
          {{ joined ? "继续创作" : "参与挑战" }}
        </button>
      </div>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <img
          v-if="typeof challenge.banner_url === 'string' && challenge.banner_url.length > 0"
          :src="challenge.banner_url"
          alt="banner"
          class="challenge-banner"
        />
        <div v-else class="glx-empty-card">
          <strong class="glx-section-title">暂无封面</strong>
          <p class="glx-page-shell__desc">当前挑战还没有横幅图。</p>
        </div>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">挑战动作</h2>
          <span class="glx-section-meta">恢复入口</span>
        </div>
        <div class="glx-stack">
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">挑战浏览</strong>
              <span class="glx-list-card__desc">网站当前继续保留挑战信息和参赛作品展示。</span>
            </div>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">参与与投稿</strong>
              <span class="glx-list-card__desc">参与挑战后会直接回到编辑器，发布页会继续承接投稿动作。</span>
            </div>
          </div>
        </div>
      </article>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">参赛作品</h2>
        <span class="glx-section-meta">{{ submissions.length }} 件</span>
      </div>

      <div v-if="submissions.length === 0" class="glx-empty-card">
        <strong class="glx-section-title">暂无参赛作品</strong>
        <p class="glx-page-shell__desc">后续有作品提交后，这里会自动展示对应列表。</p>
      </div>

      <div v-else class="glx-grid glx-grid--three">
        <article v-for="item in submissions" :key="item.id" class="glx-section-card glx-section-card--stack">
          <img
            v-if="typeof item.cover_url === 'string' && item.cover_url.length > 0"
            :src="item.cover_url"
            alt="submission"
            class="challenge-banner"
          />
          <div v-else class="glx-empty-card">
            <strong class="glx-section-title">暂无封面</strong>
            <p class="glx-page-shell__desc">当前作品没有封面图。</p>
          </div>
          <strong class="glx-section-title">{{ item.title || "未命名作品" }}</strong>
          <p class="glx-page-shell__desc">{{ item.author_name || "匿名创作者" }}</p>
          <div class="glx-inline-actions">
            <router-link :to="`/artwork/${item.id}`" class="glx-button glx-button--ghost">查看作品</router-link>
          </div>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { challengeAPI } from '@/api/index.js'
import { useUserStore } from '@/stores/user.js'

const route = useRoute()
const router = useRouter()
const userStore = useUserStore()
const challenge = ref({})
const submissions = ref([])
const joined = ref(false)

function statusText(s) {
  return { active: '进行中', ended: '已结束', upcoming: '即将开始' }[s] || '--'
}

async function loadChallenge() {
  const id = route.params.id
  challenge.value = {}
  submissions.value = []
  joined.value = false
  const [res, subRes] = await Promise.allSettled([
    challengeAPI.getDetail(id),
    challengeAPI.getSubmissions(id, { page: 1, limit: 20 })
  ])
  if (res.status === 'fulfilled' && res.value?.success) {
    challenge.value = res.value.data?.challenge || res.value.data || {}
    if (typeof challenge.value.joined === 'boolean') {
      joined.value = challenge.value.joined
    }
  }
  if (subRes.status === 'fulfilled' && subRes.value?.success) {
    submissions.value = subRes.value.data?.list || []
  }
}

async function handleJoin() {
  if (!userStore.isLoggedIn) {
    router.push({
      name: 'Login',
      query: {
        redirect: route.fullPath,
      },
    })
    return
  }

  if (joined.value) {
    router.push(`/editor?challengeId=${challenge.value.id}`)
    return
  }

  const response = await challengeAPI.join(challenge.value.id)
  if (!response.success) {
    return
  }

  joined.value = true
  if (response.data != null && response.data.changed === true && typeof challenge.value.participants === 'number') {
    challenge.value.participants += 1
  }
  router.push(`/editor?challengeId=${challenge.value.id}`)
}

onMounted(loadChallenge)
watch(() => route.params.id, loadChallenge)
</script>

<style scoped>
.challenge-banner {
  display: block;
  width: 100%;
  aspect-ratio: 16 / 10;
  object-fit: cover;
  border: 2px solid #111111;
}
</style>
