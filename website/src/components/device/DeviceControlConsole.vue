<template>
  <div class="device-home glx-page-shell">
    <section class="device-home__summary glx-page-shell__hero">
      <div class="device-home__summary-top">
        <span class="glx-page-shell__eyebrow">Glowxel PixelBoard</span>
        <div class="device-home__status-pill">
          <span class="glx-status-dot" :class="{ 'is-online': isDeviceConnected }"></span>
          <span>{{ isDeviceConnected ? "已连接" : "未连接" }}</span>
        </div>
      </div>

      <div class="device-home__summary-body">
        <div class="device-home__brand">
          <div class="device-home__brand-logo">
            <div class="device-home__brand-logo-core"></div>
            <div class="device-home__brand-logo-core device-home__brand-logo-core--accent"></div>
          </div>

          <div class="device-home__brand-copy">
            <h1 class="device-home__title">设备控制</h1>
            <p class="device-home__desc">
              网站端当前作为设备控制入口，连接设备、热点配网、读取设备参数和切换常用模式都从这里进入。
            </p>
            <p v-if="surfaceErrorMessage.length > 0" class="device-home__inline-error">
              {{ surfaceErrorMessage }}
            </p>
          </div>
        </div>

        <div class="glx-hero-metrics device-home__metrics">
          <article class="glx-hero-metric">
            <span class="glx-hero-metric__label">设备地址</span>
            <strong class="glx-hero-metric__value">{{ deviceHostText }}</strong>
          </article>
          <article class="glx-hero-metric">
            <span class="glx-hero-metric__label">当前业务模式</span>
            <strong class="glx-hero-metric__value">{{ currentBusinessModeText }}</strong>
          </article>
          <article class="glx-hero-metric">
            <span class="glx-hero-metric__label">画板尺寸</span>
            <strong class="glx-hero-metric__value">{{ boardSizeText }}</strong>
          </article>
          <article class="glx-hero-metric">
            <span class="glx-hero-metric__label">当前亮度</span>
            <strong class="glx-hero-metric__value">{{ brightnessText }}</strong>
          </article>
        </div>
      </div>

      <div v-if="!isDeviceConnected" class="device-home__connect-grid">
        <button type="button" class="device-home__entry-card" @click="openConnectModal">
          <div class="device-home__entry-icon device-home__entry-icon--paper">
            <DeviceIcon name="wifi" :size="30" />
          </div>
          <div class="device-home__entry-copy">
            <strong class="device-home__entry-title">WiFi 连接</strong>
            <span class="device-home__entry-desc">输入设备 IP 地址连接</span>
          </div>
        </button>

        <router-link to="/ble-config" class="device-home__entry-card">
          <div class="device-home__entry-icon device-home__entry-icon--mint">
            <DeviceIcon name="phone" :size="30" />
          </div>
          <div class="device-home__entry-copy">
            <strong class="device-home__entry-title">热点配网</strong>
            <span class="device-home__entry-desc">连接设备热点后打开 192.168.4.1</span>
          </div>
        </router-link>
      </div>

      <button
        v-else
        type="button"
        class="device-home__disconnect-entry"
        @click="handleDisconnect"
      >
        <DeviceIcon name="disconnect" :size="28" />
        <span>断开连接</span>
      </button>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">设备工具</h2>
        <span class="glx-section-meta">配网与参数</span>
      </div>

      <div class="device-home__tool-grid">
        <router-link
          v-for="entry in utilityEntries"
          :key="entry.to"
          :to="entry.to"
          class="device-home__tool-card"
        >
          <div class="device-home__tool-icon" :class="entry.iconShellClass">
            <DeviceIcon :name="entry.icon" :size="28" />
          </div>
          <div class="device-home__tool-copy">
            <strong class="device-home__tool-title">{{ entry.title }}</strong>
            <span class="device-home__tool-desc">{{ entry.desc }}</span>
          </div>
          <span class="device-home__tool-cta">{{ entry.cta }}</span>
        </router-link>
      </div>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">模式入口</h2>
        <span class="glx-section-meta">{{ modeCatalog.length }} 个模式</span>
      </div>

      <div class="device-home__mode-grid">
        <button
          v-for="entry in modeCatalog"
          :key="entry.key"
          type="button"
          class="device-home__mode-card"
          :class="[
            entry.variantClass,
            {
              'is-active': currentBusinessMode === entry.key,
              'is-pending': modeSwitchingKey === entry.key,
            },
          ]"
          :disabled="modeSwitchingKey.length > 0"
          @click="handleModeSelect(entry)"
        >
          <div class="device-home__mode-icon-shell">
            <div class="device-home__mode-icon-core">
              <DeviceIcon :name="entry.icon" :size="34" />
            </div>
          </div>
          <strong class="device-home__mode-name">{{ entry.name }}</strong>
          <span class="device-home__mode-meta">
            {{
              currentBusinessMode === entry.key
                ? "当前模式"
                : modeSwitchingKey === entry.key
                  ? "切换中..."
                  : "点击切换"
            }}
          </span>
        </button>
      </div>

      <p class="device-home__section-note">
        这里负责设备当前运行模式切换；如果要调参数、看预览或恢复本地缓存，请直接进入下面对应的模式页。
      </p>
    </section>

    <section
      v-for="group in pageDirectoryGroups"
      :key="group.key"
      class="glx-section-card glx-section-card--stack"
    >
      <div class="glx-section-head">
        <h2 class="glx-section-title">{{ group.title }}</h2>
        <span class="glx-section-meta">{{ group.meta }}</span>
      </div>

      <div class="device-home__directory-grid">
        <router-link
          v-for="entry in group.entries"
          :key="entry.key"
          :to="entry.to"
          class="device-home__tool-card device-home__tool-card--directory"
        >
          <div class="device-home__tool-icon" :class="entry.iconShellClass">
            <DeviceIcon :name="entry.icon" :size="28" />
          </div>
          <div class="device-home__tool-copy">
            <strong class="device-home__tool-title">{{ entry.title }}</strong>
            <span class="device-home__tool-desc">{{ entry.desc }}</span>
          </div>
          <span class="device-home__tool-cta">打开页面</span>
        </router-link>
      </div>
    </section>

    <DeviceConnectModal
      :visible="showConnectModal"
      :default-value="deviceHostValue"
      :loading="deviceStore.connecting"
      :error-message="connectErrorMessage"
      @confirm="handleConnectConfirm"
      @cancel="handleConnectCancel"
      @update:visible="showConnectModal = $event"
    />
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch } from "vue";
import { useDeviceStore } from "@/stores/device.js";
import { useFeedback } from "@/composables/useFeedback.js";
import DeviceConnectModal from "@/components/device/DeviceConnectModal.vue";
import DeviceIcon from "@/components/device/DeviceIcon.vue";

const deviceStore = useDeviceStore();
const feedback = useFeedback();

const showConnectModal = ref(false);
const connectErrorMessage = ref("");
const modeSwitchingKey = ref("");
const deviceHostValue = ref("");

const utilityEntries = [
  {
    to: "/device-params",
    icon: "params",
    title: "设备参数",
    desc: "亮度、旋转、色彩顺序和驱动参数统一在这里读取与保存。",
    cta: "打开",
    iconShellClass: "device-home__tool-icon--yellow",
  },
  {
    to: "/ble-config",
    icon: "phone",
    title: "热点配网",
    desc: "首次使用时连接设备热点，在浏览器里打开本地配网页完成联网。",
    cta: "打开",
    iconShellClass: "device-home__tool-icon--green",
  },
];
const devicePageGroups = [];
const pageDirectoryGroups = devicePageGroups.filter((group) => {
  return group.key !== "system";
});

const modeCatalog = [
  { key: "eyes", name: "桌面宠物", icon: "eyes", variantClass: "device-home__mode-card--pink" },
  { key: "clock", name: "静态时钟", icon: "clock", variantClass: "device-home__mode-card--cyan" },
  { key: "animation", name: "动态时钟", icon: "animation", variantClass: "device-home__mode-card--teal" },
  { key: "theme", name: "主题模式", icon: "theme", variantClass: "device-home__mode-card--purple" },
  { key: "tetris", name: "俄罗斯方块屏保", icon: "tetris", variantClass: "device-home__mode-card--indigo" },
  { key: "tetris_clock", name: "俄罗斯方块时钟", icon: "tetris-clock", variantClass: "device-home__mode-card--gold" },
  { key: "maze", name: "迷宫漫游", icon: "maze", variantClass: "device-home__mode-card--orange" },
  { key: "snake", name: "贪吃蛇", icon: "snake", variantClass: "device-home__mode-card--green" },
  { key: "led_matrix_showcase", name: "矩阵流光", icon: "matrix", variantClass: "device-home__mode-card--azure" },
  { key: "planet_screensaver", name: "星球屏保", icon: "planet", variantClass: "device-home__mode-card--slate" },
];

const isDeviceConnected = computed(() => deviceStore.connected === true);

const currentBusinessMode = computed(() => {
  if (typeof deviceStore.businessMode === "string") {
    return deviceStore.businessMode;
  }
  return "";
});

const currentBusinessModeText = computed(() => {
  if (currentBusinessMode.value.length > 0) {
    return currentBusinessMode.value;
  }
  return "--";
});

const brightnessText = computed(() => {
  if (typeof deviceStore.brightness === "number" && deviceStore.brightness >= 0) {
    return String(deviceStore.brightness);
  }
  return "--";
});

const boardSizeText = computed(() => {
  if (
    typeof deviceStore.width === "number" &&
    typeof deviceStore.height === "number" &&
    deviceStore.width > 0 &&
    deviceStore.height > 0
  ) {
    return `${deviceStore.width} × ${deviceStore.height}`;
  }
  return "--";
});

const deviceHostText = computed(() => {
  if (typeof deviceStore.deviceIp === "string" && deviceStore.deviceIp.length > 0) {
    return deviceStore.deviceIp;
  }
  if (typeof deviceStore.host === "string" && deviceStore.host.length > 0) {
    return deviceStore.host;
  }
  return "--";
});

const surfaceErrorMessage = computed(() => {
  if (showConnectModal.value && connectErrorMessage.value.length > 0) {
    return "";
  }
  if (typeof deviceStore.error === "string" && deviceStore.error.length > 0) {
    return deviceStore.error;
  }
  return "";
});

watch(
  () => deviceStore.host,
  (value) => {
    if (typeof value === "string") {
      deviceHostValue.value = value;
    }
  },
  { immediate: true },
);

onMounted(async () => {
  deviceStore.init();

  if (!deviceStore.connected) {
    await deviceStore.restoreConnection();
  }

  if (deviceStore.connected) {
    try {
      await deviceStore.syncDeviceStatus();
    } catch (error) {
      // keep current cached state
    }
  }
});

function openConnectModal() {
  connectErrorMessage.value = "";
  if (typeof deviceStore.host === "string" && deviceStore.host.length > 0) {
    deviceHostValue.value = deviceStore.host;
  }
  showConnectModal.value = true;
}

async function handleConnectConfirm(ip) {
  const normalizedIp = String(ip).trim();

  if (normalizedIp.length === 0) {
    connectErrorMessage.value = "请输入设备 IP 地址";
    return;
  }

  connectErrorMessage.value = "";

  try {
    feedback.showBlocking("连接设备", "正在建立 WebSocket 连接并同步设备状态。");
    await deviceStore.connect({
      host: normalizedIp,
      port: 80,
      secure: false,
    });
    await deviceStore.syncDeviceStatus();
    deviceHostValue.value = normalizedIp;
    showConnectModal.value = false;
    feedback.success("连接成功", "已经连接到 Glowxel PixelBoard。");
  } catch (error) {
    connectErrorMessage.value = resolveErrorMessage(error);
  } finally {
    feedback.hideBlocking();
  }
}

function handleConnectCancel() {
  connectErrorMessage.value = "";
}

function handleDisconnect() {
  deviceStore.disconnect();
  feedback.info("设备已断开", "当前 WebSocket 连接已经关闭。");
}

async function handleModeSelect(entry) {
  if (!isDeviceConnected.value) {
    feedback.info("请先连接设备", "连接成功后才能切换设备模式。");
    return;
  }

  if (modeSwitchingKey.value.length > 0) {
    return;
  }

  modeSwitchingKey.value = entry.key;

  try {
    feedback.showBlocking("切换模式", `正在把设备切换到 ${entry.name}。`);
    await deviceStore.setMode(entry.key);
    await deviceStore.syncDeviceStatus();
    feedback.success("切换成功", `已切换到 ${entry.name}。`);
  } catch (error) {
    feedback.error("切换失败", resolveErrorMessage(error));
  } finally {
    feedback.hideBlocking();
    modeSwitchingKey.value = "";
  }
}

function resolveErrorMessage(error) {
  if (error instanceof Error && typeof error.message === "string" && error.message.length > 0) {
    return error.message;
  }

  if (error && typeof error === "object" && "message" in error) {
    const value = error.message;
    if (typeof value === "string" && value.length > 0) {
      return value;
    }
  }

  return "操作失败，请稍后重试";
}
</script>

<style scoped>
.device-home {
  gap: 20px;
}

.device-home__summary {
  gap: 18px;
  padding: 24px;
  background: #ffffff;
}

.device-home__summary-top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

.device-home__status-pill {
  min-height: 36px;
  padding: 6px 12px;
  display: inline-flex;
  align-items: center;
  gap: 10px;
  border: var(--glx-shell-border);
  background: #ffffff;
  box-shadow: var(--glx-shadow-soft);
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 900;
}

.device-home__summary-body {
  display: grid;
  grid-template-columns: minmax(0, 1.2fr) minmax(360px, 0.8fr);
  gap: 20px;
  align-items: start;
}

.device-home__brand {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 18px;
  align-items: start;
}

.device-home__brand-logo {
  width: 92px;
  height: 92px;
  padding: 14px;
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
  border: var(--glx-shell-border);
  background: #ffffff;
  box-shadow: var(--glx-shadow-soft);
}

.device-home__brand-logo-core {
  border: var(--glx-shell-border);
  background: var(--nb-blue);
}

.device-home__brand-logo-core--accent {
  background: var(--nb-yellow);
}

.device-home__brand-copy {
  display: grid;
  gap: 12px;
}

.device-home__title {
  color: var(--nb-ink);
  font-size: clamp(30px, 4vw, 42px);
  line-height: 1.02;
  font-weight: 900;
}

.device-home__desc {
  color: var(--nb-text-secondary);
  font-size: 15px;
  line-height: 1.8;
}

.device-home__inline-error {
  width: fit-content;
  max-width: 100%;
  padding: 10px 12px;
  border: var(--glx-shell-border);
  background: #ffe2e2;
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 800;
  line-height: 1.6;
}

.device-home__metrics {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.device-home__connect-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.device-home__entry-card,
.device-home__tool-card {
  min-height: 118px;
  padding: 16px 18px;
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 14px;
  align-items: center;
  border: var(--glx-shell-border);
  background: #ffffff;
  box-shadow: var(--glx-shadow-soft);
  color: inherit;
  text-align: left;
  text-decoration: none;
  cursor: pointer;
}

.device-home__entry-card {
  width: 100%;
}

.device-home__entry-icon,
.device-home__tool-icon {
  width: 64px;
  height: 64px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: var(--glx-shell-border);
  box-shadow: var(--glx-shadow-soft);
  color: var(--nb-ink);
}

.device-home__entry-icon--paper {
  background: #ffffff;
}

.device-home__entry-icon--mint,
.device-home__tool-icon--green {
  background: #8fe0bc;
}

.device-home__tool-icon--yellow {
  background: var(--nb-yellow);
}

.device-home__tool-icon--cyan {
  background: #5cdfe8;
}

.device-home__tool-icon--teal {
  background: #52d0b1;
}

.device-home__tool-icon--purple {
  background: #ae91ff;
}

.device-home__tool-icon--pink {
  background: #ff8abd;
}

.device-home__tool-icon--orange {
  background: #ffb06d;
}

.device-home__tool-icon--indigo {
  background: #8ca2ff;
}

.device-home__tool-icon--azure {
  background: #8dc8ff;
}

.device-home__tool-icon--gold {
  background: #ffd36a;
}

.device-home__tool-icon--slate {
  background: #b3bfd6;
}

.device-home__tool-icon--blue {
  background: #74b9ff;
}

.device-home__entry-copy,
.device-home__tool-copy {
  min-width: 0;
  display: grid;
  gap: 4px;
}

.device-home__entry-title,
.device-home__tool-title {
  color: var(--nb-ink);
  font-size: 15px;
  font-weight: 900;
}

.device-home__entry-desc,
.device-home__tool-desc {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.6;
}

.device-home__disconnect-entry {
  min-height: 54px;
  padding: 12px 18px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
  width: fit-content;
  border: var(--glx-shell-border);
  background: #ffffff;
  box-shadow: var(--glx-shadow-soft);
  color: var(--nb-ink);
  font-size: 14px;
  font-weight: 900;
}

.device-home__tool-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.device-home__tool-card {
  grid-template-columns: auto minmax(0, 1fr) auto;
}

.device-home__tool-cta {
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: 900;
  white-space: nowrap;
}

.device-home__mode-grid {
  display: grid;
  grid-template-columns: repeat(5, minmax(0, 1fr));
  gap: 16px;
}

.device-home__directory-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
}

.device-home__mode-card {
  min-height: 188px;
  padding: 16px 12px 14px;
  display: grid;
  justify-items: center;
  align-content: start;
  gap: 12px;
  border: var(--glx-shell-border);
  background: #ffffff;
  box-shadow: var(--glx-shadow-soft);
  color: var(--nb-ink);
  text-align: center;
  cursor: pointer;
}

.device-home__mode-card[disabled] {
  cursor: wait;
}

.device-home__mode-icon-shell {
  width: 92px;
  height: 92px;
  padding: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--glx-shell-border);
  background: #ffffff;
}

.device-home__mode-icon-core {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--glx-shell-border);
  background: var(--device-home-mode-accent);
  color: var(--nb-ink);
}

.device-home__mode-name {
  color: var(--nb-ink);
  font-size: 14px;
  line-height: 1.35;
  font-weight: 900;
}

.device-home__mode-meta {
  color: var(--nb-text-secondary);
  font-size: 12px;
  font-weight: 800;
}

.device-home__section-note {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.7;
}

.device-home__mode-card.is-active {
  background: #fff8d8;
}

.device-home__mode-card.is-pending {
  background: #f4f4f4;
}

.device-home__mode-card--pink {
  --device-home-mode-accent: #ff8abd;
}

.device-home__mode-card--cyan {
  --device-home-mode-accent: #5cdfe8;
}

.device-home__mode-card--teal {
  --device-home-mode-accent: #52d0b1;
}

.device-home__mode-card--purple {
  --device-home-mode-accent: #ae91ff;
}

.device-home__mode-card--indigo {
  --device-home-mode-accent: #8ca2ff;
}

.device-home__mode-card--gold {
  --device-home-mode-accent: #ffd36a;
}

.device-home__mode-card--orange {
  --device-home-mode-accent: #ffb06d;
}

.device-home__mode-card--green {
  --device-home-mode-accent: #8fe0bc;
}

.device-home__mode-card--azure {
  --device-home-mode-accent: #8dc8ff;
}

.device-home__mode-card--slate {
  --device-home-mode-accent: #b3bfd6;
}

@media (max-width: 1180px) {
  .device-home__summary-body {
    grid-template-columns: 1fr;
  }

  .device-home__directory-grid,
  .device-home__mode-grid {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }
}

@media (max-width: 900px) {
  .device-home__connect-grid,
  .device-home__tool-grid {
    grid-template-columns: 1fr;
  }

  .device-home__directory-grid,
  .device-home__mode-grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .device-home__summary {
    padding: 20px;
  }

  .device-home__brand {
    grid-template-columns: 1fr;
  }

  .device-home__brand-logo {
    width: 84px;
    height: 84px;
  }

  .device-home__metrics {
    grid-template-columns: 1fr;
  }

  .device-home__directory-grid,
  .device-home__mode-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .device-home__mode-card {
    min-height: 172px;
  }

  .device-home__mode-icon-shell {
    width: 84px;
    height: 84px;
  }
}
</style>
