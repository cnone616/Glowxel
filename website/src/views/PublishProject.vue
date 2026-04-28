<template>
  <div class="glx-page-shell publish-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Publish</span>
      <h1 class="glx-page-shell__title">发布作品</h1>
      <p class="glx-page-shell__desc">
        发布页需要保留，这里继续承接项目到社区作品的正式发布链路。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">项目</span>
          <strong class="glx-hero-metric__value">{{ projectName }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">尺寸</span>
          <strong class="glx-hero-metric__value">{{ projectSize }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">颜色数</span>
          <strong class="glx-hero-metric__value">{{ colorCount }}</strong>
        </article>
      </div>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">预览</h2>
          <span class="glx-section-meta">发布封面</span>
        </div>
        <canvas ref="previewCanvasRef" class="publish-preview"></canvas>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">发布信息</h2>
          <span class="glx-section-meta">作品内容</span>
        </div>
        <div class="glx-form-grid">
          <label class="glx-field">
            <span class="glx-field__label">标题</span>
            <input v-model="form.title" class="glx-input" placeholder="请输入作品标题" />
          </label>
          <label class="glx-field">
            <span class="glx-field__label">简介</span>
            <textarea v-model="form.description" class="glx-textarea" placeholder="介绍一下这件作品"></textarea>
          </label>
          <label class="glx-field">
            <span class="glx-field__label">标签</span>
            <input v-model="form.tagsInput" class="glx-input" placeholder="使用逗号分隔多个标签" />
          </label>
          <label class="glx-field">
            <span class="glx-field__label">难度</span>
            <input v-model="form.difficulty" class="glx-input" placeholder="例如 easy / medium / hard" />
          </label>
        </div>
        <div class="glx-inline-actions">
          <button type="button" class="glx-button glx-button--primary" :disabled="publishing" @click="publishProject">
            {{ publishing ? "发布中..." : "确认发布" }}
          </button>
          <router-link :to="`/overview/${route.params.id}`" class="glx-button glx-button--ghost">回总览</router-link>
        </div>
      </article>
    </section>
  </div>
</template>

<script setup>
import { computed, nextTick, onMounted, reactive, ref } from "vue";
import { useRoute, useRouter } from "vue-router";
import { artworkAPI, challengeAPI } from "@/api/index.js";
import { useFeedback } from "@/composables/useFeedback.js";
import { useProjectStore } from "@/stores/project.js";

const feedback = useFeedback();
const projectStore = useProjectStore();
const route = useRoute();
const router = useRouter();
const previewCanvasRef = ref(null);
const publishing = ref(false);

const form = reactive({
  title: "",
  description: "",
  tagsInput: "",
  difficulty: "",
});

const currentProject = computed(() => {
  if (projectStore.currentProject != null) {
    return projectStore.currentProject;
  }
  return {};
});

const projectName = computed(() => {
  if (typeof currentProject.value.name === "string" && currentProject.value.name.length > 0) {
    return currentProject.value.name;
  }
  return "未命名项目";
});

const projectSize = computed(() => {
  if (typeof currentProject.value.width === "number" && typeof currentProject.value.height === "number") {
    return `${currentProject.value.width} × ${currentProject.value.height}`;
  }
  return "--";
});

const colorCount = computed(() => {
  if (projectStore.currentPixels == null || typeof projectStore.currentPixels !== "object") {
    return 0;
  }

  const colors = new Set();
  Object.values(projectStore.currentPixels).forEach((value) => {
    if (typeof value === "string" && value.length > 0) {
      colors.add(value);
    }
  });
  return colors.size;
});

function parseStringArray(value) {
  if (Array.isArray(value)) {
    return value.filter((item) => typeof item === "string" && item.length > 0);
  }

  if (!(typeof value === "string" && value.length > 0)) {
    return [];
  }

  try {
    const parsed = JSON.parse(value);
    if (Array.isArray(parsed)) {
      return parsed.filter((item) => typeof item === "string" && item.length > 0);
    }
  } catch (error) {
    return [];
  }

  return [];
}

function buildTags() {
  return form.tagsInput
    .split(",")
    .map((item) => item.trim())
    .filter((item) => item.length > 0);
}

function resolveThumbnail() {
  if (
    typeof currentProject.value.thumbnail_url === "string" &&
    currentProject.value.thumbnail_url.length > 0
  ) {
    return currentProject.value.thumbnail_url;
  }

  if (previewCanvasRef.value != null) {
    return previewCanvasRef.value.toDataURL("image/png");
  }

  return "";
}

function drawPreview() {
  if (previewCanvasRef.value == null) {
    return;
  }

  const canvas = previewCanvasRef.value;
  const ctx = canvas.getContext("2d");

  if (
    typeof currentProject.value.width !== "number" ||
    typeof currentProject.value.height !== "number"
  ) {
    canvas.width = 320;
    canvas.height = 320;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    return;
  }

  const scale = Math.max(6, Math.floor(320 / Math.max(currentProject.value.width, currentProject.value.height)));
  const width = currentProject.value.width * scale;
  const height = currentProject.value.height * scale;

  canvas.width = width;
  canvas.height = height;
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, width, height);

  if (projectStore.currentPixels != null && typeof projectStore.currentPixels === "object") {
    Object.entries(projectStore.currentPixels).forEach(([key, color]) => {
      if (!(typeof color === "string" && color.length > 0)) {
        return;
      }

      const [xText, yText] = key.split(",");
      const x = Number(xText);
      const y = Number(yText);
      if (!Number.isFinite(x) || !Number.isFinite(y)) {
        return;
      }

      ctx.fillStyle = color;
      ctx.fillRect(x * scale, y * scale, scale, scale);
    });
  }

  ctx.strokeStyle = "rgba(0, 0, 0, 0.1)";
  ctx.lineWidth = 1;
  for (let x = 0; x <= currentProject.value.width; x += 1) {
    ctx.beginPath();
    ctx.moveTo(x * scale, 0);
    ctx.lineTo(x * scale, height);
    ctx.stroke();
  }
  for (let y = 0; y <= currentProject.value.height; y += 1) {
    ctx.beginPath();
    ctx.moveTo(0, y * scale);
    ctx.lineTo(width, y * scale);
    ctx.stroke();
  }
}

async function publishProject() {
  if (form.title.trim().length === 0) {
    feedback.error("标题不能为空", "请输入作品标题后再发布。");
    return;
  }

  if (projectStore.currentPixels == null || typeof projectStore.currentPixels !== "object") {
    feedback.error("项目为空", "当前项目没有可发布的像素数据。");
    return;
  }

  if (
    typeof currentProject.value.width !== "number" ||
    typeof currentProject.value.height !== "number"
  ) {
    feedback.error("尺寸缺失", "当前项目尺寸没有成功读取。");
    return;
  }

  publishing.value = true;
  feedback.showBlocking("发布中", "正在把当前项目发布到社区。");

  try {
    const payload = {
      title: form.title.trim(),
      thumbnail: resolveThumbnail(),
      width: currentProject.value.width,
      height: currentProject.value.height,
      pixelData: projectStore.currentPixels,
      projectId: route.params.id,
      colorCount: colorCount.value,
    };

    if (form.description.trim().length > 0) {
      payload.description = form.description.trim();
    }

    const tags = buildTags();
    if (tags.length > 0) {
      payload.tags = tags;
    }

    if (form.difficulty.trim().length > 0) {
      payload.difficulty = form.difficulty.trim();
    }

    const response = await artworkAPI.publish(payload);
    if (!response.success) {
      feedback.error("发布失败", "作品没有成功发布。");
      return;
    }

    if (
      typeof route.query.challengeId === "string" &&
      route.query.challengeId.length > 0 &&
      response.data != null &&
      typeof response.data.artworkId !== "undefined"
    ) {
      const submitResponse = await challengeAPI.submit(route.query.challengeId, response.data.artworkId);
      if (!submitResponse.success) {
        feedback.warning("投稿未完成", "作品已发布，但挑战投稿没有成功提交。");
      }
    }

    feedback.success("发布成功", "作品已经发布到社区。");
    if (response.data != null && typeof response.data.artworkId !== "undefined") {
      router.push(`/artwork/${response.data.artworkId}`);
      return;
    }

    router.push("/my-works");
  } finally {
    publishing.value = false;
    feedback.hideBlocking();
  }
}

onMounted(async () => {
  const response = await projectStore.loadProjectDetail(route.params.id);
  if (!response.success) {
    feedback.error("项目加载失败", "发布页没有成功读取项目内容。");
    return;
  }

  if (typeof currentProject.value.name === "string") {
    form.title = currentProject.value.name;
  }
  if (typeof currentProject.value.description === "string") {
    form.description = currentProject.value.description;
  }
  const tags = parseStringArray(currentProject.value.tags);
  if (tags.length > 0) {
    form.tagsInput = tags.join(", ");
  }

  await nextTick();
  drawPreview();
});
</script>

<style scoped>
.publish-page {
  max-width: 1180px;
}

.publish-preview {
  display: block;
  width: min(100%, 320px);
  height: auto;
  border: 2px solid #111111;
  background: #ffffff;
}
</style>
