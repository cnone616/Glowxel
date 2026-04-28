<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Social</span>
      <h1 class="glx-page-shell__title">{{ pageTitle }}</h1>
      <p class="glx-page-shell__desc">
        关系链页继续承接粉丝和关注列表，关注动作也一起恢复回来。
      </p>
      <div class="glx-inline-actions">
        <button type="button" class="glx-button glx-button--ghost" @click="switchTab('followers')">看粉丝</button>
        <button type="button" class="glx-button glx-button--ghost" @click="switchTab('following')">看关注</button>
      </div>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-tabs">
        <button type="button" class="glx-tab" :class="{ 'is-active': tab === 'followers' }" @click="switchTab('followers')">
          粉丝
        </button>
        <button type="button" class="glx-tab" :class="{ 'is-active': tab === 'following' }" @click="switchTab('following')">
          关注
        </button>
      </div>
    </section>

    <div v-if="loading" class="glx-grid glx-grid--two">
      <GlxSkeletonCard />
      <GlxSkeletonCard />
    </div>

    <section v-else-if="list.length === 0" class="glx-empty-card">
      <strong class="glx-section-title">当前列表为空</strong>
      <p class="glx-page-shell__desc">等关系链建立后，这里会自动显示对应用户列表。</p>
    </section>

    <section v-else class="glx-stack">
      <article v-for="user in list" :key="user.id" class="glx-list-card follow-row">
        <div class="follow-row__avatar">{{ resolveAvatarText(user.name) }}</div>
        <div class="glx-list-card__copy">
          <strong class="glx-list-card__title">{{ resolveUserName(user) }}</strong>
          <span class="glx-list-card__desc">{{ resolveUserBio(user) }}</span>
        </div>
        <div class="glx-inline-actions">
          <router-link :to="`/user/${user.id}`" class="glx-button glx-button--ghost">查看主页</router-link>
          <button
            v-if="showToggleButton(user)"
            type="button"
            class="glx-button glx-button--ghost"
            @click="handleToggle(user)"
          >
            {{ user.is_following ? "已关注" : "关注" }}
          </button>
        </div>
      </article>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch } from "vue";
import { useRoute, useRouter } from "vue-router";
import { followAPI } from "@/api/index.js";
import { useFeedback } from "@/composables/useFeedback.js";
import GlxSkeletonCard from "@/components/glx/GlxSkeletonCard.vue";
import { useUserStore } from "@/stores/user.js";

const route = useRoute();
const router = useRouter();
const feedback = useFeedback();
const userStore = useUserStore();

const tab = ref("followers");
const list = ref([]);
const loading = ref(false);

const pageTitle = computed(() => {
  if (profileUserId.value.length > 0) {
    return tab.value === "following" ? "TA 的关注" : "TA 的粉丝";
  }
  return tab.value === "following" ? "我的关注" : "我的粉丝";
});

const profileUserId = computed(() => {
  if (typeof route.params.id === "string" && route.params.id.length > 0) {
    return route.params.id;
  }
  return "";
});

const targetUserId = computed(() => {
  if (profileUserId.value.length > 0) {
    return profileUserId.value;
  }
  if (userStore.userId != null) {
    return String(userStore.userId);
  }
  return "";
});

function syncTabFromRoute() {
  if (route.path.includes("/following")) {
    tab.value = "following";
    return;
  }
  tab.value = "followers";
}

function resolveUserName(user) {
  if (typeof user.name === "string" && user.name.length > 0) {
    return user.name;
  }
  return "未命名用户";
}

function resolveUserBio(user) {
  if (typeof user.bio === "string" && user.bio.length > 0) {
    return user.bio;
  }
  return "暂无简介";
}

function resolveAvatarText(name) {
  if (typeof name === "string" && name.length > 0) {
    return name.slice(0, 1).toUpperCase();
  }
  return "?";
}

async function load(currentTab) {
  if (targetUserId.value.length === 0) {
    list.value = [];
    return;
  }

  loading.value = true;
  try {
    const response = currentTab === "followers"
      ? await followAPI.getFollowers(targetUserId.value, { page: 1, limit: 50 })
      : await followAPI.getFollowing(targetUserId.value, { page: 1, limit: 50 });

    if (response.success && response.data != null && Array.isArray(response.data.list)) {
      list.value = response.data.list;
      return;
    }

    list.value = [];
    feedback.error("列表加载失败", response.message || "关系链列表没有成功返回。");
  } catch (error) {
    list.value = [];
    if (error instanceof Error) {
      feedback.error("列表加载失败", error.message);
    } else {
      feedback.error("列表加载失败", "关系链列表读取失败。");
    }
  } finally {
    loading.value = false;
  }
}

async function switchTab(nextTab) {
  if (tab.value === nextTab) {
    return;
  }

  tab.value = nextTab;
  if (profileUserId.value.length > 0) {
    router.push(`/user/${profileUserId.value}/${nextTab}`);
    return;
  }
  router.push(`/${nextTab}`);
}

function showToggleButton(user) {
  if (userStore.userId == null) {
    return true;
  }
  return String(user.id) !== String(userStore.userId);
}

async function handleToggle(user) {
  if (!userStore.isLoggedIn) {
    router.push({
      name: "Login",
      query: {
        redirect: route.fullPath,
      },
    });
    return;
  }

  const response = await followAPI.toggle(user.id);
  if (!response.success) {
    feedback.error("操作失败", "关注状态没有成功更新。");
    return;
  }

  if (response.data != null && typeof response.data.followed === "boolean") {
    user.is_following = response.data.followed;
    return;
  }

  user.is_following = !user.is_following;
}

onMounted(async () => {
  syncTabFromRoute();
  await load(tab.value);
});

watch(
  () => route.fullPath,
  async () => {
    syncTabFromRoute();
    await load(tab.value);
  },
);
</script>

<style scoped>
.follow-row {
  display: flex;
  align-items: center;
  gap: 16px;
}

.follow-row__avatar {
  width: 48px;
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 2px solid #111111;
  background: var(--tone-blue-soft);
  font-size: 18px;
  font-weight: 900;
  color: var(--nb-ink);
  flex-shrink: 0;
}
</style>
