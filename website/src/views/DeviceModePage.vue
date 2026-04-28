<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <div class="device-mode-hero__eyebrow-row">
        <span class="glx-page-shell__eyebrow">{{ modeEyebrow }}</span>
        <span v-if="hiddenFeature" class="glx-chip glx-chip--ghost">隐藏能力</span>
      </div>
      <h1 class="glx-page-shell__title">{{ modeTitle }}</h1>
      <p class="glx-page-shell__desc">{{ modeDescription }}</p>
      <div class="glx-inline-actions">
        <button
          type="button"
          class="glx-button glx-button--primary"
          :disabled="!deviceStore.connected || switching"
          @click="activateMode"
        >
          {{ switching ? "切换中..." : actionLabel }}
        </button>
        <button
          type="button"
          class="glx-button glx-button--ghost"
          :disabled="!deviceStore.connected || refreshing"
          @click="refreshStatus"
        >
          {{ refreshing ? "刷新中..." : "刷新状态" }}
        </button>
        <router-link to="/device-params" class="glx-button glx-button--ghost">设备参数</router-link>
        <router-link to="/device-control" class="glx-button glx-button--ghost">返回设备控制</router-link>
      </div>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">连接状态</span>
          <strong class="glx-hero-metric__value">{{ connectionText }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前 businessMode</span>
          <strong class="glx-hero-metric__value">{{ businessModeText }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前亮度</span>
          <strong class="glx-hero-metric__value">{{ brightnessText }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">设备地址</span>
          <strong class="glx-hero-metric__value">{{ hostText }}</strong>
        </article>
      </div>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">当前设备状态</h2>
          <span class="glx-section-meta">运行时回显</span>
        </div>
        <div class="glx-kv-grid">
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">当前顶层 mode</span>
            <strong class="glx-kv-card__value">{{ topModeText }}</strong>
          </div>
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">当前 effectMode</span>
            <strong class="glx-kv-card__value">{{ effectModeText }}</strong>
          </div>
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">画布尺寸</span>
            <strong class="glx-kv-card__value">{{ boardSizeText }}</strong>
          </div>
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">模式目标</span>
            <strong class="glx-kv-card__value">{{ modeKeyText }}</strong>
          </div>
        </div>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">页面说明</h2>
          <span class="glx-section-meta">正式反馈链</span>
        </div>
        <div class="glx-stack">
          <div v-for="note in modeNotes" :key="note" class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">说明</strong>
              <span class="glx-list-card__desc">{{ note }}</span>
            </div>
          </div>
        </div>
      </article>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">下一步动作</h2>
          <span class="glx-section-meta">主链路提示</span>
        </div>
        <div class="glx-stack">
          <div v-for="item in modeActions" :key="item" class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">步骤</strong>
              <span class="glx-list-card__desc">{{ item }}</span>
            </div>
          </div>
        </div>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">模式入口</h2>
          <span class="glx-section-meta">快捷跳转</span>
        </div>
        <div class="glx-inline-actions">
          <router-link
            v-for="link in modeLinks"
            :key="link.to"
            :to="link.to"
            class="glx-button"
            :class="resolveToneClass(link.tone)"
          >
            {{ link.label }}
          </router-link>
        </div>
        <p class="glx-page-shell__desc">
          当前页负责模式切换、状态回显和正式反馈；更细的创作编辑仍回到对应功能页继续完成。
        </p>
      </article>
    </section>
  </div>
</template>

<script setup>
import { computed, ref } from "vue";
import { useRoute } from "vue-router";
import { useDeviceStore } from "@/stores/device.js";
import { useFeedback } from "@/composables/useFeedback.js";

const route = useRoute();
const deviceStore = useDeviceStore();
const feedback = useFeedback();

const switching = ref(false);
const refreshing = ref(false);

const hiddenFeature = computed(() => route.meta.hiddenFeature === true);

const modeEyebrow = computed(() => {
  if (typeof route.meta.modeEyebrow === "string" && route.meta.modeEyebrow.length > 0) {
    return route.meta.modeEyebrow;
  }
  return "Device Mode";
});

const modeTitle = computed(() => {
  if (typeof route.meta.modeTitle === "string" && route.meta.modeTitle.length > 0) {
    return route.meta.modeTitle;
  }
  return "设备模式";
});

const modeDescription = computed(() => {
  if (typeof route.meta.modeDescription === "string" && route.meta.modeDescription.length > 0) {
    return route.meta.modeDescription;
  }
  return "当前模式页已经接入网站应用主端、设备状态回显和正式反馈链。";
});

const modeKey = computed(() => {
  if (typeof route.meta.businessMode === "string" && route.meta.businessMode.length > 0) {
    return route.meta.businessMode;
  }
  return "";
});

const actionLabel = computed(() => {
  if (typeof route.meta.modeActionLabel === "string" && route.meta.modeActionLabel.length > 0) {
    return route.meta.modeActionLabel;
  }
  return "切换到当前模式";
});

const modeNotes = computed(() => {
  if (Array.isArray(route.meta.modeNotes)) {
    return route.meta.modeNotes.filter((item) => typeof item === "string" && item.length > 0);
  }
  return ["当前模式页已经进入网站应用壳。"];
});

const modeActions = computed(() => {
  if (Array.isArray(route.meta.modeActions)) {
    return route.meta.modeActions.filter((item) => typeof item === "string" && item.length > 0);
  }
  return ["连接设备后先切换模式，再进入对应创作或参数入口继续完成操作。"];
});

const modeLinks = computed(() => {
  if (!Array.isArray(route.meta.modeLinks)) {
    return [];
  }

  return route.meta.modeLinks.filter((item) => {
    if (item == null || typeof item !== "object") {
      return false;
    }
    if (typeof item.label !== "string" || item.label.length === 0) {
      return false;
    }
    if (typeof item.to !== "string" || item.to.length === 0) {
      return false;
    }
    return true;
  });
});

const connectionText = computed(() => {
  if (deviceStore.connected) {
    return "已连接";
  }
  return "未连接";
});

const topModeText = computed(() => {
  if (typeof deviceStore.mode === "string" && deviceStore.mode.length > 0) {
    return deviceStore.mode;
  }
  return "--";
});

const businessModeText = computed(() => {
  if (typeof deviceStore.businessMode === "string" && deviceStore.businessMode.length > 0) {
    return deviceStore.businessMode;
  }
  return "--";
});

const effectModeText = computed(() => {
  if (typeof deviceStore.effectMode === "string" && deviceStore.effectMode.length > 0) {
    return deviceStore.effectMode;
  }
  return "--";
});

const boardSizeText = computed(() => {
  if (typeof deviceStore.width === "number" && typeof deviceStore.height === "number" && deviceStore.width > 0 && deviceStore.height > 0) {
    return `${deviceStore.width} × ${deviceStore.height}`;
  }
  return "--";
});

const brightnessText = computed(() => {
  if (typeof deviceStore.brightness === "number" && deviceStore.brightness >= 0) {
    return String(deviceStore.brightness);
  }
  return "--";
});

const hostText = computed(() => {
  if (typeof deviceStore.host === "string" && deviceStore.host.length > 0) {
    return deviceStore.host;
  }
  return "--";
});

const modeKeyText = computed(() => {
  if (modeKey.value.length > 0) {
    return modeKey.value;
  }
  return "--";
});

function resolveToneClass(tone) {
  if (tone === "primary") {
    return "glx-button--primary";
  }
  if (tone === "accent") {
    return "glx-button--accent";
  }
  if (tone === "danger") {
    return "glx-button--danger";
  }
  return "glx-button--ghost";
}

async function refreshStatus() {
  if (!deviceStore.connected) {
    feedback.warning("设备未连接", "请先在设备控制页建立连接，再读取模式状态。");
    return;
  }

  try {
    refreshing.value = true;
    feedback.showBlocking("刷新状态", "正在重新读取当前设备状态。");
    await deviceStore.syncDeviceStatus();
    feedback.success("状态已刷新", `${modeTitle.value} 页面的设备状态已经更新。`);
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("刷新失败", error.message);
    } else {
      feedback.error("刷新失败", "设备状态读取失败。");
    }
  } finally {
    refreshing.value = false;
    feedback.hideBlocking();
  }
}

async function activateMode() {
  if (!deviceStore.connected) {
    feedback.warning("设备未连接", "请先在设备控制页建立连接，再切换模式。");
    return;
  }

  if (modeKey.value.length === 0) {
    feedback.error("模式未配置", "当前页面没有提供有效的 businessMode。");
    return;
  }

  try {
    switching.value = true;
    feedback.showBlocking("切换模式", `正在把设备切换到 ${modeTitle.value}。`);
    await deviceStore.setMode(modeKey.value);
    await deviceStore.syncDeviceStatus();
    feedback.success("模式已切换", `${modeTitle.value} 已进入当前设备链路。`);
  } catch (error) {
    if (error instanceof Error) {
      feedback.error("切换失败", error.message);
    } else {
      feedback.error("切换失败", "设备没有成功进入当前模式。");
    }
  } finally {
    switching.value = false;
    feedback.hideBlocking();
  }
}
</script>

<style scoped>
.device-mode-hero__eyebrow-row {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
  margin-bottom: 12px;
}
</style>
