<template>
  <div class="glx-page-shell device-params-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Device Params</span>
      <h1 class="glx-page-shell__title">设备参数</h1>
      <p class="glx-page-shell__desc">
        这里通过统一设备通信底座读取和写回设备参数。字段分组、边界值和保存链路跟小程序设备参数页保持同一条主线，亮度上限固定为 178。
      </p>
      <div class="glx-hero-metrics">
        <div class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前设备地址</span>
          <strong class="glx-hero-metric__value device-params-page__metric">{{ deviceHostText }}</strong>
        </div>
        <div class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前 WiFi</span>
          <strong class="glx-hero-metric__value device-params-page__metric">{{ wifiSsidText }}</strong>
        </div>
        <div class="glx-hero-metric">
          <span class="glx-hero-metric__label">固件版本</span>
          <strong class="glx-hero-metric__value device-params-page__metric">{{ firmwareVersionText }}</strong>
        </div>
        <div class="glx-hero-metric">
          <span class="glx-hero-metric__label">运行时长</span>
          <strong class="glx-hero-metric__value device-params-page__metric">{{ uptimeText }}</strong>
        </div>
      </div>
      <div class="glx-inline-actions">
        <router-link to="/device-control" class="glx-button glx-button--ghost">返回设备控制</router-link>
        <button
          type="button"
          class="glx-button glx-button--primary"
          :disabled="loadingParams || saving"
          @click="reloadParams"
        >
          {{ loadingParams ? "读取中..." : "重新读取" }}
        </button>
      </div>
    </section>

    <section class="glx-grid glx-grid--two">
      <DeviceParamsSection
        title="特殊LED"
        meta="旋转、色彩、反转相位"
        description="画面侧着、颜色不对、残影分裂或左右错位时，优先检查这一组。"
      >
        <DeviceParamsField label="旋转" description="画面侧着时在这里切换。">
          <select v-model.number="params.displayRotation" class="glx-select">
            <option
              v-for="option in rotationOptions"
              :key="option.value"
              :value="option.value"
            >
              {{ option.label }}
            </option>
          </select>
        </DeviceParamsField>
        <DeviceParamsField label="色彩" description="颜色不对时在这里切换。">
          <select v-model="colorOrderValue" class="glx-select">
            <option
              v-for="option in colorOrderOptions"
              :key="option.label"
              :value="option.label"
            >
              {{ option.label }}
            </option>
          </select>
        </DeviceParamsField>
        <DeviceParamsField label="反转相位" description="残影、分裂、左右错位时切换。">
          <select v-model="clkPhaseValue" class="glx-select">
            <option value="0">关闭</option>
            <option value="1">开启</option>
          </select>
        </DeviceParamsField>
      </DeviceParamsSection>

      <DeviceParamsSection
        title="设备信息"
        meta="只读"
        description="只展示设备当前回读值，不在这里做本地猜测或缓存改写。"
      >
        <DeviceParamsField label="当前设备地址" description="当前已连接设备主机地址。">
          <span class="device-params-page__value-box">{{ deviceHostText }}</span>
        </DeviceParamsField>
        <DeviceParamsField label="固件版本" description="当前固件版本回显。">
          <span class="device-params-page__value-box">{{ firmwareVersionText }}</span>
        </DeviceParamsField>
        <DeviceParamsField label="运行时长" description="设备已运行时长。">
          <span class="device-params-page__value-box">{{ uptimeText }}</span>
        </DeviceParamsField>
        <DeviceParamsField label="当前 WiFi" description="设备当前已连接 WiFi。">
          <span class="device-params-page__value-box">{{ wifiSsidText }}</span>
        </DeviceParamsField>
      </DeviceParamsSection>
    </section>

    <section class="glx-grid glx-grid--two">
      <DeviceParamsSection
        title="日夜亮度"
        meta="0 - 178"
        description="设备常规亮度、日间亮度、夜间亮度都按 0 到 178 下发。"
      >
        <DeviceBrightnessField
          v-model="params.displayBright"
          label="当前亮度"
          description="设备常规显示亮度，范围 0-178。"
          :max="UI_BRIGHTNESS_MAX"
        />
        <DeviceBrightnessField
          v-model="params.brightnessDay"
          label="日间亮度"
          description="白天自动亮度，范围 0-178。"
          :max="UI_BRIGHTNESS_MAX"
        />
        <DeviceBrightnessField
          v-model="params.brightnessNight"
          label="夜间亮度"
          description="夜间自动亮度，范围 0-178。"
          :max="UI_BRIGHTNESS_MAX"
        />
        <DeviceParamsField label="夜间开始时间" description="夜间开始时间。">
          <input v-model="params.nightStart" class="glx-input" type="time" />
        </DeviceParamsField>
        <DeviceParamsField label="夜间结束时间" description="夜间结束时间。">
          <input v-model="params.nightEnd" class="glx-input" type="time" />
        </DeviceParamsField>
        <div class="device-params-page__tip-box">
          只有当日间亮度和夜间亮度设置成不同数值时，设备才会按时间自动切换。
        </div>
      </DeviceParamsSection>

      <DeviceParamsSection
        title="驱动参数"
        meta="面板驱动相关配置"
        description="这组值按固件合同逐项回写，适合做驱动芯片、I2S 速度和 E 引脚校准。"
      >
        <DeviceParamsField label="驱动芯片" description="按设备当前 driver 参数回显。">
          <select v-model.number="params.driver" class="glx-select">
            <option
              v-for="option in driverOptions"
              :key="option.value"
              :value="option.value"
            >
              {{ option.label }}
            </option>
          </select>
        </DeviceParamsField>
        <DeviceParamsField label="I2S 速度" description="按设备当前 i2cSpeed 参数回显。">
          <select v-model.number="params.i2cSpeed" class="glx-select">
            <option
              v-for="option in speedOptions"
              :key="option.value"
              :value="option.value"
            >
              {{ option.label }}
            </option>
          </select>
        </DeviceParamsField>
        <DeviceParamsField label="E 引脚" description="按设备当前 E_pin 参数回显。">
          <input
            v-model.number="params.E_pin"
            class="glx-input"
            type="number"
            min="0"
            max="32"
            step="1"
          />
        </DeviceParamsField>
      </DeviceParamsSection>
    </section>

    <section class="glx-grid glx-grid--two">
      <DeviceParamsSection
        title="网络参数"
        meta="国内优先 NTP"
        description="默认优先国内可用时间服务器；如果默认地址慢或不可用，可以手动填写。"
      >
        <DeviceParamsField label="时间服务器预设" description="默认优先国内可用地址。">
          <select v-model="ntpPresetValue" class="glx-select">
            <option
              v-for="option in ntpPresetOptions"
              :key="option.value"
              :value="option.value"
            >
              {{ option.label }}
            </option>
          </select>
        </DeviceParamsField>
        <DeviceParamsField
          label="自定义时间服务器"
          description="如果默认地址慢或不可用，可以手动填写。"
          :stack="true"
        >
          <input
            v-model.trim="params.ntpServer"
            class="glx-input"
            type="text"
            maxlength="63"
            placeholder="例如 ntp2.aliyun.com"
          />
        </DeviceParamsField>
      </DeviceParamsSection>

      <DeviceParamsSection
        title="应用操作"
        meta="逐项下发到设备"
        description="保存时会按参数变更逐项调用统一参数写入链路，写完后重新回读一遍确认设备实际值。"
      >
        <div class="device-params-page__action-stack">
          <DeviceActionCard
            mark="AP"
            title="保存并应用"
            description="把当前参数写入设备并立即生效。"
            tone="primary"
            :disabled="saving || loadingParams"
            @trigger="saveParams"
          />
          <DeviceActionCard
            mark="RD"
            title="重新读取"
            description="从设备拉取当前实际参数。"
            tone="accent"
            :disabled="saving || loadingParams"
            @trigger="reloadParams"
          />
        </div>
      </DeviceParamsSection>
    </section>

    <DeviceParamsSection
      title="高级"
      meta="危险操作请谨慎"
      description="清除 WiFi 配置后设备会自动重启，随后需要回到热点配网页重新配网。"
    >
      <div class="device-params-page__action-stack">
        <DeviceActionCard
          mark="RST"
          title="重置网络"
          description="清除 WiFi 配置并自动重启设备。"
          tone="danger"
          :disabled="saving || loadingParams"
          @trigger="openResetConfirm"
        />
      </div>
      <div v-if="showResetConfirm" class="device-params-page__confirm-box">
        <div class="device-params-page__confirm-copy">
          <strong class="device-params-page__confirm-title">确认要重置网络吗？</strong>
          <p class="device-params-page__confirm-desc">
            设备会清除当前 WiFi 配置并自动重启。重启后热点会重新出现，你需要回到热点配网页重新配网。
          </p>
        </div>
        <div class="glx-inline-actions">
          <button
            type="button"
            class="glx-button glx-button--ghost"
            :disabled="saving || loadingParams"
            @click="cancelResetConfirm"
          >
            先不重置
          </button>
          <button
            type="button"
            class="glx-button glx-button--danger"
            :disabled="saving || loadingParams"
            @click="resetWifi"
          >
            确认重置
          </button>
        </div>
      </div>
    </DeviceParamsSection>
  </div>
</template>

<script setup>
import { computed, onMounted, reactive, ref, watch } from "vue";
import { useRouter } from "vue-router";
import DeviceActionCard from "@/components/device/params/DeviceActionCard.vue";
import DeviceBrightnessField from "@/components/device/params/DeviceBrightnessField.vue";
import DeviceParamsField from "@/components/device/params/DeviceParamsField.vue";
import DeviceParamsSection from "@/components/device/params/DeviceParamsSection.vue";
import { useFeedback } from "@/composables/useFeedback.js";
import { useDeviceStore } from "@/stores/device.js";

const UI_BRIGHTNESS_MAX = 178;

const driverOptions = [
  { label: "SHIFTREG", value: 0 },
  { label: "FM6124", value: 1 },
  { label: "FM6126A", value: 2 },
  { label: "ICN2038S", value: 3 },
  { label: "MBI5124", value: 4 },
  { label: "DP3246", value: 5 },
];

const speedOptions = [
  { label: "HZ_8M", value: 8000000 },
  { label: "HZ_16M", value: 16000000 },
  { label: "HZ_20M", value: 20000000 },
];

const rotationOptions = [
  { label: "0°", value: 0 },
  { label: "90°", value: 1 },
  { label: "180°", value: 2 },
  { label: "270°", value: 3 },
];

const colorOrderOptions = [
  { label: "RGB", swapBlueGreen: false, swapBlueRed: false },
  { label: "RBG", swapBlueGreen: true, swapBlueRed: false },
  { label: "GBR", swapBlueGreen: false, swapBlueRed: true },
];

const ntpPresetOptions = [
  { label: "阿里云 2", value: "ntp2.aliyun.com" },
  { label: "阿里云", value: "ntp.aliyun.com" },
  { label: "腾讯云", value: "ntp.tencent.com" },
  { label: "国家授时中心", value: "ntp.ntsc.ac.cn" },
  { label: "自定义", value: "__custom__" },
];

const router = useRouter();
const deviceStore = useDeviceStore();
const feedback = useFeedback();

const saving = ref(false);
const loadingParams = ref(false);
const showResetConfirm = ref(false);
const snapshot = ref(null);
const deviceHost = ref("");
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
  ntpServer: "ntp2.aliyun.com",
  nightStart: "22:00",
  nightEnd: "07:00",
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
  return "未连接";
});

const deviceHostText = computed(() => {
  if (typeof deviceHost.value === "string" && deviceHost.value.length > 0) {
    return deviceHost.value;
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

const colorOrderValue = computed({
  get() {
    for (const option of colorOrderOptions) {
      if (
        option.swapBlueGreen === params.swapBlueGreen &&
        option.swapBlueRed === params.swapBlueRed
      ) {
        return option.label;
      }
    }
    return colorOrderOptions[0].label;
  },
  set(value) {
    for (const option of colorOrderOptions) {
      if (option.label !== value) {
        continue;
      }
      params.swapBlueGreen = option.swapBlueGreen;
      params.swapBlueRed = option.swapBlueRed;
      return;
    }
  },
});

const clkPhaseValue = computed({
  get() {
    if (params.clkphase) {
      return "1";
    }
    return "0";
  },
  set(value) {
    params.clkphase = value === "1";
  },
});

const ntpPresetValue = computed({
  get() {
    for (const option of ntpPresetOptions) {
      if (option.value === "__custom__") {
        continue;
      }
      if (option.value === params.ntpServer) {
        return option.value;
      }
    }
    return "__custom__";
  },
  set(value) {
    if (value === "__custom__") {
      return;
    }
    params.ntpServer = value;
  },
});

function clampBrightness(value) {
  const numericValue = Number(value);
  if (!Number.isFinite(numericValue)) {
    return 0;
  }
  if (numericValue < 0) {
    return 0;
  }
  if (numericValue > UI_BRIGHTNESS_MAX) {
    return UI_BRIGHTNESS_MAX;
  }
  return Math.round(numericValue);
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
  params.ntpServer = String(data.ntpServer);
  params.nightStart = String(data.nightStart);
  params.nightEnd = String(data.nightEnd);
  info.value = {
    firmwareVersion: typeof data.firmwareVersion === "string" ? data.firmwareVersion : "",
    wifiSsid: typeof data.wifiSsid === "string" ? data.wifiSsid : "",
    uptime: typeof data.uptime === "number" ? data.uptime : 0,
  };
  snapshot.value = JSON.parse(JSON.stringify(params));
}

async function reloadParams() {
  if (loadingParams.value) {
    return;
  }
  showResetConfirm.value = false;
  loadingParams.value = true;
  try {
    feedback.showBlocking("读取设备参数", "正在从设备本地接口读取当前实际参数。");
    const data = await deviceStore.getDeviceParams();
    applyResponse(data);
    feedback.success("读取完成", "设备参数已经按当前值回显。");
  } catch (error) {
    feedback.error("读取失败", error.message);
  } finally {
    loadingParams.value = false;
    feedback.hideBlocking();
  }
}

async function updateDeviceParam(key, value) {
  await deviceStore.setDeviceParam(key, value);
}

function buildUpdates() {
  const updates = [];

  const nextDisplayBright = clampBrightness(params.displayBright);
  const nextBrightnessDay = clampBrightness(params.brightnessDay);
  const nextBrightnessNight = clampBrightness(params.brightnessNight);
  params.displayBright = nextDisplayBright;
  params.brightnessDay = nextBrightnessDay;
  params.brightnessNight = nextBrightnessNight;

  if (snapshot.value.displayBright !== nextDisplayBright) {
    updates.push(["displayBright", nextDisplayBright]);
  }
  if (snapshot.value.brightnessDay !== nextBrightnessDay) {
    updates.push(["brightnessDay", nextBrightnessDay]);
  }
  if (snapshot.value.brightnessNight !== nextBrightnessNight) {
    updates.push(["brightnessNight", nextBrightnessNight]);
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

  return updates;
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

  const updates = buildUpdates();
  if (updates.length === 0) {
    feedback.info("参数没有变化", "当前参数没有新的修改。");
    return;
  }

  saving.value = true;
  showResetConfirm.value = false;
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

function openResetConfirm() {
  showResetConfirm.value = true;
}

function cancelResetConfirm() {
  showResetConfirm.value = false;
}

async function resetWifi() {
  showResetConfirm.value = false;
  try {
    feedback.showBlocking("重置网络", "设备会清除 WiFi 配置并自动重启。");
    await deviceStore.clearWifiConfig();
    feedback.success("设备已开始重置网络", "设备热点会重新出现，请回到热点配网页重新配网。");
  } catch (error) {
    feedback.error("重置网络失败", error.message);
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
    if (typeof nextHost !== "string") {
      return;
    }
    if (nextHost.length === 0) {
      return;
    }
    deviceHost.value = nextHost;
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

<style scoped>
.device-params-page {
  gap: 20px;
}

.device-params-page__metric {
  font-size: clamp(18px, 2.6vw, 24px);
  line-height: 1.2;
  word-break: break-all;
}

.device-params-page code {
  padding: 2px 6px;
  border: var(--glx-shell-border);
  background: #ffffff;
  color: var(--nb-ink);
  font-size: 0.92em;
  font-weight: 800;
}

.device-params-page__value-box {
  min-height: 46px;
  display: inline-flex;
  align-items: center;
  justify-content: flex-start;
  padding: 11px 12px;
  border: var(--glx-shell-border);
  background: #ffffff;
  color: var(--nb-ink);
  font-size: 14px;
  font-weight: 800;
  word-break: break-all;
}

.device-params-page__tip-box {
  padding: 14px 16px;
  border: var(--glx-shell-border);
  background: #fff6d1;
  color: var(--nb-ink);
  font-size: 13px;
  line-height: 1.7;
  font-weight: 700;
}

.device-params-page__action-stack {
  display: grid;
  gap: 12px;
}

.device-params-page__confirm-box {
  display: grid;
  gap: 14px;
  padding: 16px;
  border: var(--glx-shell-border);
  background: #fff1ef;
}

.device-params-page__confirm-copy {
  display: grid;
  gap: 6px;
}

.device-params-page__confirm-title {
  color: var(--nb-ink);
  font-size: 16px;
  font-weight: 900;
}

.device-params-page__confirm-desc {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.7;
}

@media (max-width: 980px) {
  .device-params-page :deep(.glx-grid--two) {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
