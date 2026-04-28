<template>
  <div class="glx-page-shell edit-profile-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Edit Profile</span>
      <h1 class="glx-page-shell__title">编辑资料</h1>
      <p class="glx-page-shell__desc">
        资料页也恢复回来，继续承接昵称和个人简介修改。
      </p>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-form-grid">
        <label class="glx-field">
          <span class="glx-field__label">昵称</span>
          <input v-model="form.name" class="glx-input" maxlength="20" placeholder="请输入昵称" />
        </label>
        <label class="glx-field">
          <span class="glx-field__label">个人简介</span>
          <textarea
            v-model="form.bio"
            class="glx-textarea"
            maxlength="100"
            placeholder="介绍一下自己吧"
          ></textarea>
        </label>
      </div>
      <div class="glx-inline-actions">
        <button type="button" class="glx-button glx-button--primary" :disabled="saving" @click="handleSave">
          {{ saving ? "保存中..." : "保存" }}
        </button>
        <router-link to="/profile" class="glx-button glx-button--ghost">返回个人中心</router-link>
      </div>
      <p v-if="message.length > 0" class="edit-profile-message">{{ message }}</p>
    </section>
  </div>
</template>

<script setup>
import { onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import { useUserStore } from "@/stores/user.js";

const router = useRouter();
const userStore = useUserStore();

const form = reactive({
  name: "",
  bio: "",
});
const saving = ref(false);
const message = ref("");

onMounted(async () => {
  await userStore.fetchProfile();
  if (userStore.currentUser != null) {
    if (typeof userStore.currentUser.name === "string") {
      form.name = userStore.currentUser.name;
    }
    if (typeof userStore.currentUser.bio === "string") {
      form.bio = userStore.currentUser.bio;
    }
  }
});

async function handleSave() {
  if (form.name.trim().length === 0) {
    message.value = "昵称不能为空";
    return;
  }

  saving.value = true;
  message.value = "";

  try {
    const response = await userStore.updateProfile({
      name: form.name.trim(),
      bio: form.bio.trim(),
    });

    if (response.success) {
      message.value = "保存成功";
      window.setTimeout(() => {
        router.push("/profile");
      }, 800);
      return;
    }

    if (typeof response.message === "string" && response.message.length > 0) {
      message.value = response.message;
      return;
    }

    message.value = "保存失败";
  } finally {
    saving.value = false;
  }
}
</script>

<style scoped>
.edit-profile-page {
  max-width: 860px;
}

.edit-profile-message {
  color: var(--nb-green);
  font-size: 13px;
  font-weight: 800;
}
</style>
