<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Device Params</span>
      <h1 class="glx-page-shell__title">设备参数</h1>
      <p class="glx-page-shell__desc">
        Web 端参数页直接读取设备本地 `/get` 和 `/set` 接口，只使用固件已经暴露的参数键。
      </p>
      <div class="glx-inline-actions">
        <router-link to="/device-control" class="glx-button glx-button--ghost">返回设备控制</router-link>
        <button type="button" class="glx-button glx-button--primary" @click="reloadParams">重新读取</button>
      </div>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">设备连接</h2>
          <span class="glx-section-meta">本地门户</span>
        </div>
        <label class="glx-field">
          <span class="glx-field__label">当前设备地址</span>
          <input v-model="deviceHost" class="glx-input" readonly />
        </label>
        <p class="glx-page-shell__desc">设备参数页只在已连接态下读取设备本地接口，不再作为离线浏览页。</p>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">只读信息</h2>
          <span class="glx-section-meta">设备回显</span>
        </div>
        <div class="glx-kv-grid">
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">固件版本</span>
            <strong class="glx-kv-card__value">{{ firmwareVersionText }}</strong>
          </div>
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">当前 WiFi</span>
            <strong class="glx-kv-card__value">{{ wifiSsidText }}</strong>
          </div>
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">运行时长</span>
            <strong class="glx-kv-card__value">{{ uptimeText }}</strong>
          </div>
        </div>
      </article>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">亮度</h2>
          <span class="glx-section-meta">0 - 178</span>
        </div>
        <label class="glx-field">
          <span class="glx-field__label">当前亮度</span>
          <input v-model.number="params.displayBright" type="number" min="0" max="178" class="glx-input" />
        </label>
        <label class="glx-field">
          <span class="glx-field__label">日间亮度</span>
          <input v-model.number="params.brightnessDay" type="number" min="0" max="178" class="glx-input" />
        </label>
        <label class="glx-field">
          <span class="glx-field__label">夜间亮度</span>
          <input v-model.number="params.brightnessNight" type="number" min="0" max="178" class="glx-input" />
        </label>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">显示方向与色彩</h2>
          <span class="glx-section-meta">LED 参数</span>
        </div>
        <label class="glx-field">
          <span class="glx-field__label">旋转</span>
          <select v-model.number="params.displayRotation" class="glx-select">
            <option :value="0">0°</option>
            <option :value="1">90°</option>
            <option :value="2">180°</option>
            <option :value="3">270°</option>
          </select>
        </label>
        <label class="glx-field">
          <span class="glx-field__label">色彩</span>
          <select v-model="colorOrderValue" class="glx-select">
            <option value="RGB">RGB</option>
            <option value="RBG">RBG</option>
            <option value="GBR">GBR</option>
          </select>
        </label>
        <label class="glx-field">
          <span class="glx-field__label">反转相位</span>
          <select v-model="clkPhaseValue" class="glx-select">
            <option value="0">关闭</option>
            <option value="1">开启</option>
          </select>
        </label>
      </article>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">驱动参数</h2>
          <span class="glx-section-meta">按固件合同回写</span>
        </div>
        <label class="glx-field">
          <span class="glx-field__label">驱动芯片</span>
          <select v-model.number="params.driver" class="glx-select">
            <option :value="0">SHIFTREG</option>
            <option :value="1">FM6124</option>
            <option :value="2">FM6126A</option>
            <option :value="3">ICN2038S</option>
            <option :value="4">MBI5124</option>
            <option :value="5">DP3246</option>
          </select>
        </label>
        <label class="glx-field">
          <span class="glx-field__label">传输速度</span>
          <select v-model.number="params.i2cSpeed" class="glx-select">
            <option :value="8000000">HZ_8M</option>
            <option :value="16000000">HZ_16M</option>
            <option :value="20000000">HZ_20M</option>
          </select>
        </label>
        <label class="glx-field">
          <span class="glx-field__label">E 引脚</span>
          <input v-model.number="params.E_pin" type="number" min="0" max="32" class="glx-input" />
        </label>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">网络参数</h2>
          <span class="glx-section-meta">时间同步</span>
        </div>
        <label class="glx-field">
          <span class="glx-field__label">夜间开始时间</span>
          <input v-model="params.nightStart" type="time" class="glx-input" />
        </label>
        <label class="glx-field">
          <span class="glx-field__label">夜间结束时间</span>
          <input v-model="params.nightEnd" type="time" class="glx-input" />
        </label>
        <label class="glx-field">
          <span class="glx-field__label">时间服务器</span>
          <input v-model="params.ntpServer" class="glx-input" maxlength="63" />
        </label>
      </article>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-inline-actions">
        <button type="button" class="glx-button glx-button--primary" :disabled="saving" @click="saveParams">
          {{ saving ? "保存中..." : "保存并应用" }}
        </button>
        <button type="button" class="glx-button glx-button--danger" :disabled="saving" @click="resetWifi">
          重置网络
        </button>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, reactive, ref, watch } from "vue";
import { useRouter } from "vue-router";
import { useDeviceStore } from "@/stores/device.js";
import { useFeedback } from "@/composables/useFeedback.js";

const router = useRouter();
const deviceStore = useDeviceStore();
const feedback = useFeedback();

const saving = ref(false);
const info = ref({
  firmwareVersion: "",
  wifiSsid: "",
  uptime: 0,
});

const params = reactive({
  displayBright: 50,
  brightnessDay: 50,
  brightnessNight: 50,
  displayRotation: 0,
  swapBlueGreen: false,
  swapBlueRed: false,
  clkphase: false,
  driver: 0,
  i2cSpeed: 8000000,
  E_pin: 18,
  nightStart: "22:00",
  nightEnd: "07:00",
  ntpServer: "ntp2.aliyun.com",
});

const snapshot = ref(null);
const deviceHost = ref("");

const colorOrderValue = computed({
  get() {
    if (params.swapBlueGreen === false && params.swapBlueRed === false) {
      return "RGB";
    }
    if (params.swapBlueGreen === true && params.swapBlueRed === false) {
      return "RBG";
    }
    return "GBR";
  },
  set(value) {
    if (value === "RGB") {
      params.swapBlueGreen = false;
      params.swapBlueRed = false;
      return;
    }
    if (value === "RBG") {
      params.swapBlueGreen = true;
      params.swapBlueRed = false;
      return;
    }
    params.swapBlueGreen = false;
    params.swapBlueRed = true;
  },
});

const clkPhaseValue = computed({
  get() {
    return params.clkphase ? "1" : "0";
  },
  set(value) {
    params.clkphase = value === "1";
  },
});

const firmwareVersionText = computed(() => {
  if (typeof info.value.firmwareVersion === "string" && info.value.firmwareVersion.length > 0) {
    return info.value.firmwareVersion;
  }
  return "--";
});

const wifiSsidText = computed(() => {
  if (typeof info.value.wifiSsid === "string" && info.value.wifiSsid.length > 0) {
    return info.value.wifiSsid;
  }
  return "--";
});

const uptimeText = computed(() => {
  if (typeof info.value.uptime !== "number") {
    return "--";
  }
  const totalSeconds = info.value.uptime;
  if (totalSeconds <= 0) {
    return "--";
  }
  const days = Math.floor(totalSeconds / 86400);
  const hours = Math.floor((totalSeconds % 86400) / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  if (days > 0) {
    return `${days}天 ${hours}小时 ${minutes}分`;
  }
  if (hours > 0) {
    return `${hours}小时 ${minutes}分`;
  }
  return `${minutes}分`;
});

function getBaseUrl() {
  const host = deviceHost.value.trim();
  if (host.length === 0) {
    throw new Error("请先填写设备地址");
  }
  return `http://${host}`;
}

async function requestJson(path, init) {
  const baseUrl = getBaseUrl();
  const response = await fetch(`${baseUrl}${path}`, init);
  const data = await response.json();
  if (!response.ok) {
    if (typeof data.error === "string" && data.error.length > 0) {
      throw new Error(data.error);
    }
    throw new Error("设备请求失败");
  }
  if (data != null && typeof data.error === "string" && data.error.length > 0) {
    throw new Error(data.error);
  }
  return data;
}

function clampBrightness(value) {
  const numberValue = Number(value);
  if (!Number.isFinite(numberValue)) {
    return 0;
  }
  if (numberValue < 0) {
    return 0;
  }
  if (numberValue > 178) {
    return 178;
  }
  return Math.round(numberValue);
}

function applyResponse(data) {
  params.displayBright = clampBrightness(data.displayBright);
  params.brightnessDay = clampBrightness(data.brightnessDay);
  params.brightnessNight = clampBrightness(data.brightnessNight);
  params.displayRotation = Number(data.displayRotation);
  params.swapBlueGreen = data.swapBlueGreen === true;
  params.swapBlueRed = data.swapBlueRed === true;
  params.clkphase = data.clkphase === true;
  params.driver = Number(data.driver);
  params.i2cSpeed = Number(data.i2cSpeed);
  params.E_pin = Number(data.E_pin);
  params.nightStart = String(data.nightStart);
  params.nightEnd = String(data.nightEnd);
  params.ntpServer = String(data.ntpServer);
  info.value = {
    firmwareVersion: typeof data.firmwareVersion === "string" ? data.firmwareVersion : "",
    wifiSsid: typeof data.wifiSsid === "string" ? data.wifiSsid : "",
    uptime: typeof data.uptime === "number" ? data.uptime : 0,
  };
  snapshot.value = JSON.parse(JSON.stringify(params));
}

async function reloadParams() {
  try {
    feedback.showBlocking("读取设备参数", "正在从设备本地接口读取当前实际参数。");
    const data = await requestJson("/get", { method: "GET" });
    applyResponse(data);
    feedback.success("读取完成", "设备参数已经按当前值回显。");
  } catch (error) {
    feedback.error("读取失败", error.message);
  } finally {
    feedback.hideBlocking();
  }
}

async function updateDeviceParam(key, value) {
  const body = new URLSearchParams();
  body.set(key, String(value));
  await requestJson("/set", {
    method: "POST",
    headers: {
      "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8",
    },
    body: body.toString(),
  });
}

async function saveParams() {
  if (snapshot.value == null) {
    feedback.warning("还没有快照", "请先重新读取一次设备参数。");
    return;
  }

  if (!Number.isInteger(params.E_pin) || params.E_pin < 0 || params.E_pin > 32) {
    feedback.error("参数无效", "E_pin 必须是 0 到 32 之间的整数。");
    return;
  }

  if (params.ntpServer.trim().length === 0) {
    feedback.error("参数无效", "ntpServer 不能为空。");
    return;
  }

  const updates = [];

  if (snapshot.value.displayBright !== params.displayBright) {
    updates.push(["displayBright", params.displayBright]);
  }
  if (snapshot.value.brightnessDay !== params.brightnessDay) {
    updates.push(["brightnessDay", params.brightnessDay]);
  }
  if (snapshot.value.brightnessNight !== params.brightnessNight) {
    updates.push(["brightnessNight", params.brightnessNight]);
  }
  if (snapshot.value.displayRotation !== params.displayRotation) {
    updates.push(["displayRotation", params.displayRotation]);
  }
  if (snapshot.value.swapBlueGreen !== params.swapBlueGreen) {
    updates.push(["swapBlueGreen", params.swapBlueGreen ? 1 : 0]);
  }
  if (snapshot.value.swapBlueRed !== params.swapBlueRed) {
    updates.push(["swapBlueRed", params.swapBlueRed ? 1 : 0]);
  }
  if (snapshot.value.clkphase !== params.clkphase) {
    updates.push(["clkphase", params.clkphase ? 1 : 0]);
  }
  if (snapshot.value.driver !== params.driver) {
    updates.push(["driver", params.driver]);
  }
  if (snapshot.value.i2cSpeed !== params.i2cSpeed) {
    updates.push(["i2cSpeed", params.i2cSpeed]);
  }
  if (snapshot.value.E_pin !== params.E_pin) {
    updates.push(["E_pin", params.E_pin]);
  }
  if (snapshot.value.nightStart !== params.nightStart) {
    updates.push(["nightStart", params.nightStart]);
  }
  if (snapshot.value.nightEnd !== params.nightEnd) {
    updates.push(["nightEnd", params.nightEnd]);
  }
  if (snapshot.value.ntpServer !== params.ntpServer) {
    updates.push(["ntpServer", params.ntpServer]);
  }

  if (updates.length === 0) {
    feedback.info("没有变更", "当前参数没有新的修改。");
    return;
  }

  saving.value = true;
  try {
    feedback.showBlocking("保存设备参数", "正在逐项把参数写入设备并等待生效。");
    for (const entry of updates) {
      await updateDeviceParam(entry[0], entry[1]);
    }
    await reloadParams();
    feedback.success("保存成功", "设备参数已经写入并重新回读。");
  } catch (error) {
    feedback.error("保存失败", error.message);
  } finally {
    saving.value = false;
    feedback.hideBlocking();
  }
}

async function resetWifi() {
  try {
    feedback.showBlocking("重置网络", "设备会清除 WiFi 配置并自动重启。");
    await requestJson("/clear-wifi", { method: "GET" });
    feedback.success("网络已重置", "设备热点会重新出现，请重新配网。");
  } catch (error) {
    feedback.error("重置失败", error.message);
  } finally {
    feedback.hideBlocking();
  }
}

onMounted(async () => {
  deviceStore.init();
  if (!deviceStore.connected) {
    await deviceStore.restoreConnection();
  }

  if (!deviceStore.connected) {
    feedback.warning("请先连接设备", "设备参数页只能在已连接态进入，正在返回设备控制页。");
    router.push("/device-control");
    return;
  }

  if (typeof deviceStore.host === "string" && deviceStore.host.length > 0) {
    deviceHost.value = deviceStore.host;
    await reloadParams();
  }
});

watch(
  () => deviceStore.host,
  (nextHost) => {
    if (typeof nextHost === "string" && nextHost.length > 0 && deviceHost.value.length === 0) {
      deviceHost.value = nextHost;
    }
  },
);

watch(
  () => deviceStore.connected,
  (connected) => {
    if (!connected) {
      feedback.warning("设备已断开", "设备参数页只支持已连接状态，正在返回设备控制页。");
      router.push("/device-control");
    }
  },
);
</script>
