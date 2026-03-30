<template>
  <div class="device-page">
    <div class="container">
      <div class="page-header">
        <h1 class="page-title">设备控制</h1>
        <p class="page-desc">在网页直接连接 RenLight 设备并进行基础控制</p>
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
  padding: 36px 0 56px;
}

.page-header {
  margin-bottom: 16px;
}

.page-title {
  font-size: 30px;
  font-weight: 700;
  color: var(--color-text-primary);
}

.page-desc {
  margin-top: 6px;
  color: var(--color-text-muted);
  font-size: 14px;
}

.card {
  background: #fff;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: 20px;
  box-shadow: var(--shadow-sm);
  margin-bottom: 14px;
}

.card-title {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 14px;
}

.connection-form {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 110px auto;
  gap: 10px;
  margin-bottom: 14px;
  align-items: end;
}

.field {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.field span {
  font-size: 12px;
  color: var(--color-text-muted);
}

.field input {
  height: 36px;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-sm);
  padding: 0 10px;
  font-size: 14px;
}

.field-port input {
  text-align: center;
}

.secure-switch {
  height: 36px;
  display: inline-flex;
  align-items: center;
  gap: 8px;
  color: var(--color-text-secondary);
  font-size: 13px;
}

.actions {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.status {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 12px;
  color: var(--color-text-secondary);
  font-size: 13px;
}

.dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #bbb;
}

.dot.online {
  background: #22c55e;
}

.mode {
  padding: 2px 8px;
  border: 1px solid var(--color-border);
  border-radius: 999px;
}

.msg {
  margin-top: 10px;
  font-size: 13px;
  color: var(--color-text-secondary);
}

.msg.error {
  color: var(--color-danger);
}

.grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.mode-group {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
}

.mode-btn {
  height: 36px;
  border-radius: var(--radius-sm);
  border: 1px solid var(--color-border);
  background: #fff;
  cursor: pointer;
  font-size: 13px;
}

.mode-btn.active {
  border-color: var(--color-primary);
  color: var(--color-primary);
  background: rgba(79, 127, 255, 0.08);
}

.brightness-row {
  margin-bottom: 12px;
}

.brightness-row label {
  display: block;
  font-size: 13px;
  color: var(--color-text-secondary);
  margin-bottom: 8px;
}

.brightness-row input {
  width: 100%;
}

@media (max-width: 900px) {
  .grid {
    grid-template-columns: 1fr;
  }

  .connection-form {
    grid-template-columns: 1fr;
  }
}
</style>
