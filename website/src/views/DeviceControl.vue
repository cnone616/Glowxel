<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Device Control</span>
      <h1 class="glx-page-shell__title">设备控制</h1>
      <p class="glx-page-shell__desc">
        网站端只保留基础设备链与桌面创作强相关入口：连接、热点配网、设备参数、画板模式和拼豆工作台统一从这里进入。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">连接状态</span>
          <strong class="glx-hero-metric__value">{{ connectedText }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前 mode</span>
          <strong class="glx-hero-metric__value">{{ modeText }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前 businessMode</span>
          <strong class="glx-hero-metric__value">{{ businessModeText }}</strong>
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
          <h2 class="glx-section-title">连接设备</h2>
          <span class="glx-section-meta">WebSocket</span>
        </div>
        <div class="glx-form-grid glx-form-grid--two">
          <label class="glx-field">
            <span class="glx-field__label">设备地址</span>
            <input v-model="host" class="glx-input" placeholder="例如 192.168.1.88" />
          </label>
          <label class="glx-field">
            <span class="glx-field__label">端口</span>
            <input v-model.number="port" type="number" min="1" max="65535" class="glx-input" />
          </label>
        </div>
        <label class="glx-field">
          <span class="glx-field__label">协议</span>
          <select v-model="secureValue" class="glx-select">
            <option value="0">ws</option>
            <option value="1">wss</option>
          </select>
        </label>
        <div class="glx-inline-actions">
          <button type="button" class="glx-button glx-button--primary" :disabled="deviceStore.connecting" @click="handleConnect">
            {{ deviceStore.connecting ? "连接中..." : deviceStore.connected ? "重新连接" : "连接设备" }}
          </button>
          <button type="button" class="glx-button glx-button--ghost" :disabled="!deviceStore.connected" @click="handleDisconnect">
            断开连接
          </button>
          <button type="button" class="glx-button glx-button--ghost" :disabled="!deviceStore.connected" @click="refreshStatus">
            刷新状态
          </button>
        </div>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">设备入口</h2>
          <span class="glx-section-meta">主链入口</span>
        </div>
        <div class="glx-stack">
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">设备参数</strong>
              <span class="glx-list-card__desc">亮度、旋转、色彩顺序、NTP 和驱动参数统一在这里读写。</span>
            </div>
            <router-link to="/device-params" class="glx-button glx-button--ghost">打开</router-link>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">热点配网</strong>
              <span class="glx-list-card__desc">首次使用时先连接设备热点，再打开本地门户完成配网。</span>
            </div>
            <router-link to="/ble-config" class="glx-button glx-button--ghost">打开</router-link>
          </div>
        </div>
      </article>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">关键模式与发送</h2>
        <span class="glx-section-meta">{{ modeCatalog.length }} 个入口</span>
      </div>
      <div class="glx-grid glx-grid--three">
        <router-link
          v-for="entry in modeCatalog"
          :key="entry.key"
          :to="entry.to"
          class="glx-section-card glx-section-card--stack"
          style="text-decoration:none;"
        >
          <span class="glx-chip" :class="entry.chipClass">{{ entry.category }}</span>
          <strong class="glx-section-title" style="font-size:18px;">{{ entry.title }}</strong>
          <p class="glx-page-shell__desc">{{ entry.desc }}</p>
        </router-link>
      </div>
      <p class="glx-page-shell__desc">
        静态时钟、动态时钟、主题模式、桌面宠物和屏保类模式继续由 uniapp 负责，网站端本轮不再保留对应独立设置页。
      </p>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from "vue";
import { useDeviceStore } from "@/stores/device.js";
import { useFeedback } from "@/composables/useFeedback.js";

const deviceStore = useDeviceStore();
const feedback = useFeedback();

const host = ref("");
const port = ref(80);
const secureValue = ref("0");

const modeCatalog = [
  { key: "canvas", title: "画板模式", desc: "先把设备切到 `canvas`，再回编辑器继续像素创作。", to: "/canvas-editor", category: "关键模式", chipClass: "glx-chip--green" },
  { key: "pattern", title: "拼豆工作台", desc: "上传图片或带编号图纸，整理拼豆图案后再进入发送链。", to: "/pattern-workbench", category: "桌面创作", chipClass: "glx-chip--blue" },
];

const connectedText = computed(() => {
  return deviceStore.connected ? "已连接" : "未连接";
});

const modeText = computed(() => {
  if (deviceStore.mode.length > 0) {
    return deviceStore.mode;
  }
  return "--";
});

const businessModeText = computed(() => {
  if (deviceStore.businessMode.length > 0) {
    return deviceStore.businessMode;
  }
  return "--";
});

const hostText = computed(() => {
  if (deviceStore.host.length > 0) {
    return deviceStore.host;
  }
  return "--";
});

async function handleConnect() {
  try {
    feedback.showBlocking("连接设备", "正在建立 WebSocket 连接并读取当前设备状态。");
    await deviceStore.connect({
      host: host.value,
      port: port.value,
      secure: secureValue.value === "1",
    });

    if (deviceStore.deviceInfo) {
      feedback.success("连接成功", "设备状态已经同步到网站端。");
      return;
    }

    feedback.warning("连接已建立", "WebSocket 已打开，状态快照暂未返回，可稍后手动刷新。");
  } catch (error) {
    feedback.error("连接失败", error.message);
  } finally {
    feedback.hideBlocking();
  }
}

function handleDisconnect() {
  deviceStore.disconnect();
  feedback.info("已断开连接", "设备 WebSocket 已关闭。");
}

async function refreshStatus() {
  try {
    await deviceStore.syncDeviceStatus();
    feedback.success("状态已刷新", "设备当前状态已经重新读取。");
  } catch (error) {
    feedback.error("刷新失败", error.message);
  }
}

onMounted(() => {
  host.value = deviceStore.host;
  if (deviceStore.port > 0) {
    port.value = deviceStore.port;
  }
  secureValue.value = deviceStore.secure ? "1" : "0";
});
</script>
