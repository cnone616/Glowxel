<template>
  <div class="device-page">
    <div class="container">
      <div class="page-header">
        <h1 class="page-title">设备控制</h1>
        <p class="page-desc">在网页直接连接光格像素工坊设备并进行基础控制</p>
      </div>

      <div class="card">
        <h2 class="card-title">设备连接</h2>
        <div class="connection-form">
          <label class="field">
            <span>设备地址</span>
            <input v-model="host" placeholder="例如 192.168.1.88" />
          </label>
          <label class="field field-port">
            <span>端口</span>
            <input v-model.number="port" type="number" min="1" max="65535" />
          </label>
          <label class="secure-switch">
            <input v-model="secure" type="checkbox" />
            <span>使用 `wss`</span>
          </label>
        </div>

        <div class="actions">
          <button class="btn btn-primary" :disabled="deviceStore.connecting" @click="handleConnect">
            {{ deviceStore.connected ? "重新连接" : deviceStore.connecting ? "连接中..." : "连接设备" }}
          </button>
          <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="handleDisconnect">断开连接</button>
          <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="handleStatus">刷新状态</button>
        </div>

        <div class="status">
          <span class="dot" :class="{ online: deviceStore.connected }"></span>
          <span>{{ deviceStore.connected ? "已连接" : "未连接" }}</span>
          <span v-if="deviceStore.mode" class="mode">模式：{{ deviceStore.mode }}</span>
        </div>
        <p v-if="message" class="msg">{{ message }}</p>
        <p v-if="deviceStore.error" class="msg error">{{ deviceStore.error }}</p>
      </div>

      <div class="grid">
        <div class="card">
          <h2 class="card-title">模式切换</h2>
          <div class="mode-group">
            <button class="mode-btn" :class="{ active: deviceStore.mode === 'clock' }" :disabled="!deviceStore.connected" @click="switchMode('clock')">静态时钟</button>
            <button class="mode-btn" :class="{ active: deviceStore.mode === 'animation' }" :disabled="!deviceStore.connected" @click="switchMode('animation')">动态时钟</button>
            <button class="mode-btn" :class="{ active: deviceStore.mode === 'canvas' }" :disabled="!deviceStore.connected" @click="switchMode('canvas')">画板模式</button>
          </div>
        </div>

        <div class="card">
          <h2 class="card-title">屏幕操作</h2>
          <div class="brightness-row">
            <label>亮度：{{ brightness }}</label>
            <input v-model.number="brightness" type="range" min="0" max="255" />
          </div>
          <div class="actions">
            <button class="btn btn-primary" :disabled="!deviceStore.connected" @click="handleBrightness">应用亮度</button>
            <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="handleClear">清屏</button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { onMounted, ref } from "vue";
import { useDeviceStore } from "@/stores/device.js";

const deviceStore = useDeviceStore();
const host = ref("");
const port = ref(80);
const secure = ref(false);
const brightness = ref(50);
const message = ref("");

onMounted(() => {
  deviceStore.init();
  host.value = deviceStore.host;
  port.value = deviceStore.port || 80;
  secure.value = deviceStore.secure;
});

async function handleConnect() {
  message.value = "";
  try {
    await deviceStore.connect({
      host: host.value,
      port: port.value,
      secure: secure.value,
    });
    message.value = "设备连接成功";
  } catch (error) {
    message.value = error.message || "连接失败";
  }
}

function handleDisconnect() {
  deviceStore.disconnect();
  message.value = "已断开设备连接";
}

async function handleStatus() {
  message.value = "";
  try {
    const resp = await deviceStore.requestStatus();
    message.value = resp?.message || "状态已刷新";
  } catch (error) {
    message.value = error.message || "刷新状态失败";
  }
}

async function switchMode(mode) {
  message.value = "";
  try {
    const resp = await deviceStore.setMode(mode);
    message.value = resp?.message || "模式切换成功";
  } catch (error) {
    message.value = error.message || "模式切换失败";
  }
}

async function handleBrightness() {
  message.value = "";
  try {
    const value = Number(brightness.value);
    await deviceStore.setBrightness(value);
    message.value = "亮度设置成功";
  } catch (error) {
    message.value = error.message || "亮度设置失败";
  }
}

async function handleClear() {
  message.value = "";
  try {
    await deviceStore.clearScreen();
    message.value = "已清屏";
  } catch (error) {
    message.value = error.message || "清屏失败";
  }
}
</script>

<style scoped>
.device-page {
  padding: 28px 0 72px;
}

.page-header {
  margin-bottom: 20px;
  padding: 24px 24px 20px;
  border: 3px solid var(--nb-ink);
  background: linear-gradient(180deg, var(--nb-paper) 0%, var(--tone-yellow-soft) 100%);
  box-shadow: var(--nb-shadow-card);
}

.page-title {
  font-size: 34px;
  font-weight: 900;
  line-height: 1.05;
  color: var(--nb-ink);
  letter-spacing: -0.03em;
}

.page-desc {
  max-width: 640px;
  margin-top: 10px;
  color: var(--text-secondary);
  font-size: 15px;
  line-height: 1.6;
}

.card {
  position: relative;
  margin-bottom: 16px;
  padding: 22px;
  border: 3px solid var(--nb-ink);
  background: var(--tone-paper-soft);
  box-shadow: var(--nb-shadow-card);
}

.card-title {
  display: inline-flex;
  align-items: center;
  min-height: 40px;
  margin-bottom: 16px;
  padding: 6px 14px;
  border: 3px solid var(--nb-ink);
  background: var(--nb-blue);
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 900;
  letter-spacing: 0.01em;
}

.connection-form {
  display: grid;
  grid-template-columns: minmax(0, 1.4fr) 120px auto;
  gap: 12px;
  margin-bottom: 14px;
  align-items: end;
}

.field {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.field span {
  font-size: 12px;
  font-weight: 800;
  letter-spacing: 0.04em;
  color: var(--text-secondary);
  text-transform: uppercase;
}

.field input {
  min-height: 46px;
  padding: 0 14px;
  font-size: 14px;
  font-weight: 700;
}

.field-port input {
  text-align: center;
}

.secure-switch {
  min-height: 46px;
  display: inline-flex;
  align-items: center;
  gap: 10px;
  padding: 0 14px;
  border: 3px solid var(--nb-ink);
  background: var(--tone-paper-soft);
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 800;
  white-space: nowrap;
}

.secure-switch input {
  width: 16px;
  height: 16px;
  accent-color: var(--nb-yellow);
}

.actions {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.status {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 16px;
  color: var(--text-secondary);
  font-size: 13px;
  font-weight: 700;
}

.dot {
  width: 14px;
  height: 14px;
  border: 3px solid var(--nb-ink);
  background: var(--text-tertiary);
  flex: 0 0 auto;
}

.dot.online {
  background: var(--nb-green);
}

.mode {
  display: inline-flex;
  align-items: center;
  min-height: 34px;
  padding: 6px 12px;
  border: 3px solid var(--nb-ink);
  background: var(--tone-paper-soft);
  color: var(--nb-ink);
  white-space: nowrap;
}

.msg {
  margin-top: 14px;
  padding: 10px 12px;
  border: 2px solid var(--nb-ink);
  background: var(--tone-paper-soft);
  font-size: 13px;
  font-weight: 700;
  color: var(--text-secondary);
}

.msg.error {
  background: var(--tone-coral-soft);
  color: var(--nb-ink);
}

.grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.mode-group {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px;
}

.mode-btn {
  min-height: 54px;
  padding: 10px 14px;
  border: 3px solid var(--nb-ink);
  background: var(--tone-paper-soft);
  cursor: pointer;
  font-size: 14px;
  font-weight: 900;
  line-height: 1.35;
  color: var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  transition: background-color 0.15s ease, color 0.15s ease, box-shadow 0.15s ease;
  white-space: nowrap;
}

.mode-btn:hover:not(:disabled) {
  background: var(--nb-muted);
}

.mode-btn.active {
  background: var(--nb-blue);
  color: var(--nb-ink);
}

.mode-btn:disabled {
  cursor: not-allowed;
  background: var(--nb-muted);
  color: #7a7a7a;
  box-shadow: none;
}

.brightness-row {
  margin-bottom: 14px;
  padding: 14px;
  border: 3px solid var(--nb-ink);
  background: var(--tone-paper-soft);
}

.brightness-row label {
  display: block;
  margin-bottom: 10px;
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 800;
}

.brightness-row input {
  width: 100%;
}

@media (max-width: 900px) {
  .grid {
    grid-template-columns: 1fr;
  }

  .mode-group {
    grid-template-columns: 1fr;
  }

  .connection-form {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 640px) {
  .device-page {
    padding: 20px 0 56px;
  }

  .page-header,
  .card {
    padding: 18px;
  }

  .page-title {
    font-size: 28px;
  }

  .actions .btn,
  .actions .mode-btn {
    width: 100%;
  }
}
</style>
