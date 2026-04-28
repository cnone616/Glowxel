<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Profile</span>
      <h1 class="glx-page-shell__title">{{ profileName }}</h1>
      <p class="glx-page-shell__desc">{{ profileBio }}</p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">作品</span>
          <strong class="glx-hero-metric__value">{{ profileWorksCount }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">粉丝</span>
          <strong class="glx-hero-metric__value">{{ profileFollowersCount }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">关注</span>
          <strong class="glx-hero-metric__value">{{ profileFollowingCount }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">总点赞</span>
          <strong class="glx-hero-metric__value">{{ profileTotalLikes }}</strong>
        </article>
      </div>
      <div class="glx-inline-actions">
        <router-link to="/my-works" class="glx-button glx-button--primary">我的作品</router-link>
        <router-link to="/my-favorites" class="glx-button glx-button--ghost">我的收藏</router-link>
        <router-link to="/settings/profile" class="glx-button glx-button--ghost">编辑资料</router-link>
      </div>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">个人中心入口</h2>
          <span class="glx-section-meta">正式链路</span>
        </div>
        <div class="glx-stack">
          <router-link to="/my-works" class="glx-list-card profile-link">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">我的作品</strong>
              <span class="glx-list-card__desc">查看、进入和删除已发布作品。</span>
            </div>
          </router-link>
          <router-link to="/my-favorites" class="glx-list-card profile-link">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">我的收藏</strong>
              <span class="glx-list-card__desc">继续管理收藏过的公开作品。</span>
            </div>
          </router-link>
          <router-link to="/achievements" class="glx-list-card profile-link">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">成就中心</strong>
              <span class="glx-list-card__desc">恢复个人成就展示页，不再直接删掉入口。</span>
            </div>
          </router-link>
          <router-link to="/cloud-sync" class="glx-list-card profile-link">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">云端同步</strong>
              <span class="glx-list-card__desc">查看项目同步状态和云端草稿数量。</span>
            </div>
          </router-link>
        </div>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">最近作品</h2>
          <span class="glx-section-meta">{{ artworks.length }} 件</span>
        </div>
        <div v-if="artworks.length === 0" class="glx-empty-card">
          <strong class="glx-section-title">还没有已发布作品</strong>
          <p class="glx-page-shell__desc">先去创作或发布项目，这里就会出现你的作品列表。</p>
        </div>
        <div v-else class="glx-stack">
          <article v-for="item in artworks" :key="item.id" class="glx-list-card">
            <img
              v-if="coverUrl(item.cover_url).length > 0"
              :src="coverUrl(item.cover_url)"
              alt="cover"
              class="profile-cover"
            />
            <div v-else class="profile-cover profile-cover--empty">无</div>
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">{{ artworkTitle(item) }}</strong>
              <span class="glx-list-card__desc">{{ artworkMeta(item) }}</span>
            </div>
            <router-link :to="`/artwork/${item.id}`" class="glx-button glx-button--ghost">查看</router-link>
          </article>
        </div>
      </article>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from "vue";
import { artworkAPI, userAPI } from "@/api/index.js";
import { useFeedback } from "@/composables/useFeedback.js";
import { useUserStore } from "@/stores/user.js";

const feedback = useFeedback();
const userStore = useUserStore();

const artworks = ref([]);

const profileName = computed(() => {
  if (userStore.currentUser != null && typeof userStore.currentUser.name === "string" && userStore.currentUser.name.length > 0) {
    return userStore.currentUser.name;
  }
  return "个人中心";
});

const profileBio = computed(() => {
  if (userStore.currentUser != null && typeof userStore.currentUser.bio === "string" && userStore.currentUser.bio.length > 0) {
    return userStore.currentUser.bio;
  }
  return "这里会展示你的作品、收藏、成就和同步状态。";
});

const profileWorksCount = computed(() => {
  if (userStore.currentUser != null && typeof userStore.currentUser.works_count === "number") {
    return userStore.currentUser.works_count;
  }
  return 0;
});

const profileFollowersCount = computed(() => {
  if (userStore.currentUser != null && typeof userStore.currentUser.followers_count === "number") {
    return userStore.currentUser.followers_count;
  }
  return 0;
});

const profileFollowingCount = computed(() => {
  if (userStore.currentUser != null && typeof userStore.currentUser.following_count === "number") {
    return userStore.currentUser.following_count;
  }
  return 0;
});

const profileTotalLikes = computed(() => {
  if (userStore.currentUser != null && typeof userStore.currentUser.total_likes === "number") {
    return userStore.currentUser.total_likes;
  }
  return 0;
});

function artworkTitle(item) {
  if (typeof item.title === "string" && item.title.length > 0) {
    return item.title;
  }
  return "未命名作品";
}

function artworkMeta(item) {
  const parts = [];

  if (typeof item.likes === "number") {
    parts.push(`${item.likes} 赞`);
  }

  if (typeof item.views === "number") {
    parts.push(`${item.views} 浏览`);
  }

  if (typeof item.created_at === "string" && item.created_at.length > 0) {
    parts.push(item.created_at.slice(0, 10));
  }

  return parts.join(" · ");
}

function coverUrl(value) {
  if (typeof value === "string" && value.length > 0) {
    return value;
  }
  return "";
}

async function loadProfile() {
  const profileResponse = await userAPI.getProfile();
  if (profileResponse.success) {
    if (profileResponse.data != null && profileResponse.data.user != null) {
      userStore.currentUser = profileResponse.data.user;
    } else if (profileResponse.data != null) {
      userStore.currentUser = profileResponse.data;
    }
  }

  const artworkResponse = await artworkAPI.getMine({ page: 1, limit: 6 });
  if (artworkResponse.success && artworkResponse.data != null && Array.isArray(artworkResponse.data.list)) {
    artworks.value = artworkResponse.data.list;
    return;
  }

  artworks.value = [];
  feedback.error("作品加载失败", "个人中心没有成功取回我的作品列表。");
}

onMounted(async () => {
  await loadProfile();
});
</script>

<style scoped>
.profile-link {
  text-decoration: none;
  color: inherit;
}

.profile-cover {
  width: 64px;
  height: 64px;
  object-fit: cover;
  border: 2px solid #111111;
  flex-shrink: 0;
}

.profile-cover--empty {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background: #f2f2f2;
  font-size: 14px;
  font-weight: 900;
}
</style>
