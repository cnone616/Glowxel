<template>
  <div class="clock-page">
    <div class="container">
      <div class="page-header">
        <h1 class="page-title">时钟编辑器（Web）</h1>
        <p class="page-desc">网页直接连接设备，调整时钟参数并下发到板载程序</p>
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
            {{ deviceStore.connecting ? "连接中..." : deviceStore.connected ? "重新连接" : "连接设备" }}
          </button>
          <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="handleDisconnect">断开连接</button>
          <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="refreshStatus">刷新状态</button>
        </div>
        <div class="status">
          <span class="dot" :class="{ online: deviceStore.connected }"></span>
          <span>{{ deviceStore.connected ? "已连接" : "未连接" }}</span>
          <span v-if="deviceStore.mode" class="badge">设备模式：{{ deviceStore.mode }}</span>
        </div>
      </div>

      <div class="grid">
        <div class="card">
          <h2 class="card-title">模式与字体</h2>
          <div class="field-row">
            <label class="field">
              <span>时钟模式</span>
              <select v-model="clockMode">
                <option value="clock">静态时钟</option>
                <option value="animation">动态时钟</option>
              </select>
            </label>
            <label class="field">
              <span>字体</span>
              <select v-model="config.font">
                <option v-for="font in fontOptions" :key="font.id" :value="font.id">{{ font.name }}</option>
              </select>
            </label>
          </div>
          <div class="switch-row">
            <label><input v-model="config.showSeconds" type="checkbox" /> 显示秒钟</label>
            <label>
              小时制：
              <select v-model.number="config.hourFormat">
                <option :value="24">24</option>
                <option :value="12">12</option>
              </select>
            </label>
          </div>
        </div>

        <div class="card">
          <h2 class="card-title">设备操作</h2>
          <div class="actions">
            <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="switchMode('clock')">切到静态时钟</button>
            <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="switchMode('animation')">切到动态时钟</button>
          </div>
          <div class="brightness-row">
            <label>亮度：{{ brightness }}</label>
            <input v-model.number="brightness" type="range" min="0" max="255" />
          </div>
          <div class="actions">
            <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="applyBrightness">应用亮度</button>
            <button class="btn btn-outline" :disabled="!deviceStore.connected" @click="clearScreen">清屏</button>
          </div>
        </div>
      </div>

      <div class="grid sections">
        <div class="card section-card">
          <h3>时间</h3>
          <div class="switch-row">
            <label><input v-model="config.time.show" type="checkbox" /> 显示</label>
            <label>
              对齐：
              <select v-model="config.time.align">
                <option value="left">左</option>
                <option value="center">中</option>
                <option value="right">右</option>
              </select>
            </label>
          </div>
          <div class="field-row triple">
            <label class="field"><span>X</span><input v-model.number="config.time.x" type="number" min="0" max="63" /></label>
            <label class="field"><span>Y</span><input v-model.number="config.time.y" type="number" min="0" max="63" /></label>
            <label class="field"><span>字号</span><input v-model.number="config.time.fontSize" type="number" min="1" max="3" /></label>
          </div>
          <label class="field"><span>颜色</span><input v-model="config.time.color" type="color" /></label>
        </div>

        <div class="card section-card">
          <h3>日期</h3>
          <div class="switch-row">
            <label><input v-model="config.date.show" type="checkbox" /> 显示</label>
            <label>
              对齐：
              <select v-model="config.date.align">
                <option value="left">左</option>
                <option value="center">中</option>
                <option value="right">右</option>
              </select>
            </label>
          </div>
          <div class="field-row triple">
            <label class="field"><span>X</span><input v-model.number="config.date.x" type="number" min="0" max="63" /></label>
            <label class="field"><span>Y</span><input v-model.number="config.date.y" type="number" min="0" max="63" /></label>
            <label class="field"><span>字号</span><input v-model.number="config.date.fontSize" type="number" min="1" max="2" /></label>
          </div>
          <label class="field"><span>颜色</span><input v-model="config.date.color" type="color" /></label>
        </div>

        <div class="card section-card">
          <h3>星期</h3>
          <div class="switch-row">
            <label><input v-model="config.week.show" type="checkbox" /> 显示</label>
            <label>
              对齐：
              <select v-model="config.week.align">
                <option value="left">左</option>
                <option value="center">中</option>
                <option value="right">右</option>
              </select>
            </label>
          </div>
          <div class="field-row">
            <label class="field"><span>X</span><input v-model.number="config.week.x" type="number" min="0" max="63" /></label>
            <label class="field"><span>Y</span><input v-model.number="config.week.y" type="number" min="0" max="63" /></label>
          </div>
          <label class="field"><span>颜色</span><input v-model="config.week.color" type="color" /></label>
        </div>

        <div class="card section-card">
          <h3>背景图配置</h3>
          <div class="switch-row">
            <label><input v-model="config.image.show" type="checkbox" /> 显示</label>
          </div>
          <div class="field-row quad">
            <label class="field"><span>X</span><input v-model.number="config.image.x" type="number" min="-128" max="128" /></label>
            <label class="field"><span>Y</span><input v-model.number="config.image.y" type="number" min="-128" max="128" /></label>
            <label class="field"><span>宽</span><input v-model.number="config.image.width" type="number" min="1" max="64" /></label>
            <label class="field"><span>高</span><input v-model.number="config.image.height" type="number" min="1" max="64" /></label>
          </div>
          <div class="actions">
            <button class="btn btn-outline" type="button" @click="triggerMediaUpload">上传图片/GIF</button>
            <button class="btn btn-outline" type="button" :disabled="mediaType === ''" @click="removeMedia">移除素材</button>
          </div>
          <p class="media-tip">动画模式上传 GIF 可下发为设备动画；静态模式会按当前宽高转成背景像素</p>
          <p v-if="mediaFileName" class="msg">已选择：{{ mediaFileName }}</p>
          <p v-if="mediaType === 'gif' && gifFrameCount > 0" class="msg">GIF 帧数：{{ gifFrameCount }}</p>
          <input ref="mediaInputRef" type="file" accept="image/*,.gif" class="hidden-input" @change="handleMediaFileChange" />
        </div>
      </div>

      <div class="card">
        <h2 class="card-title">下发配置</h2>
        <div class="actions">
          <button class="btn btn-outline" @click="saveLocal">保存到浏览器</button>
          <button class="btn btn-outline" @click="loadLocal">读取本地配置</button>
          <button class="btn btn-primary" :disabled="!deviceStore.connected || sending" @click="sendConfig">
            {{ sending ? "发送中..." : "发送到设备" }}
          </button>
        </div>
        <p v-if="message" class="msg">{{ message }}</p>
        <p v-if="deviceStore.error" class="msg error">{{ deviceStore.error }}</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch } from "vue";
import { useDeviceStore } from "@/stores/device.js";
import { GIFParser } from "@/utils/gifParser.js";

const deviceStore = useDeviceStore();

const fontOptions = [
  { id: "classic_5x7", name: "5x7 经典", width: 5, spacing: 1 },
  { id: "minimal_3x5", name: "3x5 极简", width: 3, spacing: 1 },
  { id: "rounded_4x6", name: "4x6 圆润", width: 4, spacing: 1 },
  { id: "lcd_6x8", name: "6x8 LCD", width: 6, spacing: 1 },
  { id: "hollow_5x7", name: "5x7 空心", width: 5, spacing: 1 },
  { id: "seven_seg_5x7", name: "5x7 七段", width: 5, spacing: 1 },
  { id: "italic_6x7", name: "6x7 斜体", width: 6, spacing: 1 },
  { id: "retro_5x7", name: "5x7 终端", width: 5, spacing: 1 },
];

const host = ref("");
const port = ref(80);
const secure = ref(false);
const brightness = ref(50);
const clockMode = ref("clock");
const sending = ref(false);
const message = ref("");
const mediaInputRef = ref(null);
const mediaFileName = ref("");
const mediaType = ref("");
const gifFrameCount = ref(0);
const staticImagePixels = ref([]);
const gifAnimationData = ref(null);

const STORAGE_PREFIX = "clock_editor_web_config_";
let staticImageSource = null;
let gifParser = null;

function createDefaultConfig() {
  return {
    font: "classic_5x7",
    showSeconds: false,
    hourFormat: 24,
    time: {
      show: true,
      fontSize: 1,
      x: 32,
      y: 20,
      color: "#64c8ff",
      align: "center",
    },
    date: {
      show: true,
      fontSize: 1,
      x: 32,
      y: 29,
      color: "#787878",
      align: "center",
    },
    week: {
      show: true,
      x: 32,
      y: 38,
      color: "#646464",
      align: "center",
    },
    image: {
      show: false,
      x: 0,
      y: 0,
      width: 64,
      height: 64,
    },
  };
}

const config = ref(createDefaultConfig());

const fontMetaMap = computed(() => {
  const map = new Map();
  fontOptions.forEach((item) => map.set(item.id, item));
  return map;
});

function getClockTextWidth(text, fontId, size = 1) {
  const meta = fontMetaMap.value.get(fontId) || fontOptions[0];
  if (!text) {
    return 0;
  }
  return text.length * (meta.width + meta.spacing) * size - meta.spacing * size;
}

function getCurrentTimeText(showSeconds, hourFormat) {
  const now = new Date();
  let hours = now.getHours();
  if (hourFormat === 12) {
    hours = hours % 12;
    if (hours === 0) {
      hours = 12;
    }
  }
  const hourText = String(hours).padStart(2, "0");
  const minuteText = String(now.getMinutes()).padStart(2, "0");
  if (showSeconds) {
    const secondText = String(now.getSeconds()).padStart(2, "0");
    return `${hourText}:${minuteText}:${secondText}`;
  }
  return `${hourText}:${minuteText}`;
}

function getCurrentDateText() {
  const now = new Date();
  const month = String(now.getMonth() + 1).padStart(2, "0");
  const date = String(now.getDate()).padStart(2, "0");
  return `${month}-${date}`;
}

function getCurrentWeekText() {
  const weekDays = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"];
  return weekDays[new Date().getDay()];
}

function sleep(ms) {
  return new Promise((resolve) => {
    window.setTimeout(resolve, ms);
  });
}

function alignX(baseX, align, text, fontId, size) {
  if (align === "center") {
    return baseX - Math.floor(getClockTextWidth(text, fontId, size) / 2);
  }
  if (align === "right") {
    return baseX - getClockTextWidth(text, fontId, size);
  }
  return baseX;
}

function hexToRgb(hex) {
  const raw = String(hex || "").replace("#", "");
  if (raw.length !== 6) {
    return { r: 255, g: 255, b: 255 };
  }
  return {
    r: Number.parseInt(raw.slice(0, 2), 16),
    g: Number.parseInt(raw.slice(2, 4), 16),
    b: Number.parseInt(raw.slice(4, 6), 16),
  };
}

function normalizeInt(value, min, max) {
  const num = Number(value);
  if (!Number.isFinite(num)) {
    return min;
  }
  if (num < min) {
    return min;
  }
  if (num > max) {
    return max;
  }
  return Math.round(num);
}

function isGifFile(file) {
  if (typeof file.type === "string" && file.type === "image/gif") {
    return true;
  }
  if (typeof file.name === "string") {
    return file.name.toLowerCase().endsWith(".gif");
  }
  return false;
}

function triggerMediaUpload() {
  if (mediaInputRef.value) {
    mediaInputRef.value.click();
  }
}

function clearMediaState() {
  mediaType.value = "";
  mediaFileName.value = "";
  gifFrameCount.value = 0;
  staticImagePixels.value = [];
  gifAnimationData.value = null;
  staticImageSource = null;
  gifParser = null;
}

function removeMedia() {
  clearMediaState();
  message.value = "素材已移除";
}

function loadImageFromFile(file) {
  return new Promise((resolve, reject) => {
    const objectUrl = URL.createObjectURL(file);
    const image = new Image();
    image.onload = () => {
      URL.revokeObjectURL(objectUrl);
      resolve(image);
    };
    image.onerror = () => {
      URL.revokeObjectURL(objectUrl);
      reject(new Error("图片加载失败"));
    };
    image.src = objectUrl;
  });
}

function rgbaToAbsolutePixels(rgba, width, height, offsetX, offsetY, skipBlack) {
  const pixels = [];
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const idx = (y * width + x) * 4;
      const r = rgba[idx];
      const g = rgba[idx + 1];
      const b = rgba[idx + 2];
      const a = rgba[idx + 3];
      if (a <= 10) {
        continue;
      }
      if (skipBlack && r === 0 && g === 0 && b === 0) {
        continue;
      }
      const fx = x + offsetX;
      const fy = y + offsetY;
      if (fx < 0 || fx >= 64 || fy < 0 || fy >= 64) {
        continue;
      }
      pixels.push({ x: fx, y: fy, r, g, b });
    }
  }
  return pixels;
}

function buildStaticImagePixelsFromSource() {
  if (staticImageSource === null) {
    staticImagePixels.value = [];
    return;
  }
  const width = normalizeInt(config.value.image.width, 1, 64);
  const height = normalizeInt(config.value.image.height, 1, 64);
  const offsetX = normalizeInt(config.value.image.x, -128, 128);
  const offsetY = normalizeInt(config.value.image.y, -128, 128);
  const canvas = document.createElement("canvas");
  canvas.width = width;
  canvas.height = height;
  const ctx = canvas.getContext("2d");
  if (ctx === null) {
    staticImagePixels.value = [];
    return;
  }
  ctx.imageSmoothingEnabled = false;
  ctx.clearRect(0, 0, width, height);
  ctx.drawImage(staticImageSource, 0, 0, width, height);
  const imageData = ctx.getImageData(0, 0, width, height);
  staticImagePixels.value = rgbaToAbsolutePixels(
    imageData.data,
    width,
    height,
    offsetX,
    offsetY,
    false,
  );
}

function buildGifFirstFramePixels() {
  if (gifParser === null) {
    staticImagePixels.value = [];
    return;
  }
  const width = normalizeInt(config.value.image.width, 1, 64);
  const height = normalizeInt(config.value.image.height, 1, 64);
  const offsetX = normalizeInt(config.value.image.x, -128, 128);
  const offsetY = normalizeInt(config.value.image.y, -128, 128);
  const frame = gifParser.renderFrame(0, width, height);
  if (frame === null) {
    staticImagePixels.value = [];
    return;
  }
  staticImagePixels.value = rgbaToAbsolutePixels(
    frame.rgba,
    width,
    height,
    offsetX,
    offsetY,
    true,
  );
}

function buildGifAnimationData() {
  if (gifParser === null) {
    gifAnimationData.value = null;
    gifFrameCount.value = 0;
    return;
  }
  const width = normalizeInt(config.value.image.width, 1, 64);
  const height = normalizeInt(config.value.image.height, 1, 64);
  const offsetX = normalizeInt(config.value.image.x, -128, 128);
  const offsetY = normalizeInt(config.value.image.y, -128, 128);
  const renderedFrames = gifParser.renderFrames(width, height);
  gifAnimationData.value = gifParser.generateESP32Data(
    width,
    height,
    20,
    null,
    offsetX,
    offsetY,
    renderedFrames,
  );
  gifFrameCount.value = gifAnimationData.value.frameCount;
}

async function rebuildMediaData() {
  if (mediaType.value === "image") {
    buildStaticImagePixelsFromSource();
    gifAnimationData.value = null;
    gifFrameCount.value = 0;
    return;
  }

  if (mediaType.value === "gif") {
    if (clockMode.value === "animation") {
      buildGifAnimationData();
      staticImagePixels.value = [];
      return;
    }
    buildGifFirstFramePixels();
    gifAnimationData.value = null;
    if (gifParser !== null) {
      gifFrameCount.value = gifParser.frames.length;
    }
    return;
  }

  gifAnimationData.value = null;
  gifFrameCount.value = 0;
  staticImagePixels.value = [];
}

async function handleMediaFileChange(event) {
  const target = event.target;
  if (!target || !target.files || target.files.length === 0) {
    return;
  }

  const file = target.files[0];
  target.value = "";
  message.value = "";

  try {
    mediaFileName.value = file.name;
    if (isGifFile(file) && clockMode.value === "animation") {
      const buffer = await file.arrayBuffer();
      gifParser = new GIFParser();
      gifParser.parse(buffer);
      mediaType.value = "gif";
      staticImageSource = null;
      await rebuildMediaData();
      message.value = `GIF 已加载，共 ${gifFrameCount.value} 帧`;
      return;
    }

    if (isGifFile(file) && clockMode.value === "clock") {
      message.value = "静态模式下 GIF 将按首帧作为背景图处理";
    }

    const image = await loadImageFromFile(file);
    mediaType.value = "image";
    staticImageSource = image;
    gifParser = null;
    gifAnimationData.value = null;
    gifFrameCount.value = 0;
    buildStaticImagePixelsFromSource();
    if (message.value === "") {
      message.value = "图片已加载";
    }
  } catch (error) {
    clearMediaState();
    if (error instanceof Error && typeof error.message === "string") {
      message.value = error.message;
    } else {
      message.value = "素材加载失败";
    }
  }
}

function getStorageKey(mode) {
  return `${STORAGE_PREFIX}${mode}`;
}

function saveLocal(showMessage = true) {
  localStorage.setItem(getStorageKey(clockMode.value), JSON.stringify(config.value));
  if (showMessage) {
    message.value = "本地配置已保存";
  }
}

function loadLocal() {
  const raw = localStorage.getItem(getStorageKey(clockMode.value));
  if (!raw) {
    config.value = createDefaultConfig();
    message.value = "未找到本地配置，已使用默认值";
    return;
  }
  try {
    const parsed = JSON.parse(raw);
    config.value = {
      ...createDefaultConfig(),
      ...parsed,
      time: { ...createDefaultConfig().time, ...(parsed.time || {}) },
      date: { ...createDefaultConfig().date, ...(parsed.date || {}) },
      week: { ...createDefaultConfig().week, ...(parsed.week || {}) },
      image: { ...createDefaultConfig().image, ...(parsed.image || {}) },
    };
    message.value = "本地配置已载入";
  } catch (error) {
    message.value = "本地配置解析失败";
  }
}

function buildPayload() {
  const cfg = config.value;
  const timeText = getCurrentTimeText(cfg.showSeconds, cfg.hourFormat);
  const dateText = getCurrentDateText();
  const weekText = getCurrentWeekText();

  const timeX = alignX(cfg.time.x, cfg.time.align, timeText, cfg.font, cfg.time.fontSize);
  const dateX = alignX(cfg.date.x, cfg.date.align, dateText, cfg.font, cfg.date.fontSize);
  const weekX = alignX(cfg.week.x, cfg.week.align, weekText, cfg.font, 1);

  return {
    font: cfg.font,
    showSeconds: !!cfg.showSeconds,
    hourFormat: cfg.hourFormat === 12 ? 12 : 24,
    time: {
      show: !!cfg.time.show,
      fontSize: normalizeInt(cfg.time.fontSize, 1, 3),
      x: normalizeInt(timeX, 0, 63),
      y: normalizeInt(cfg.time.y, 0, 63),
      color: hexToRgb(cfg.time.color),
    },
    date: {
      show: !!cfg.date.show,
      fontSize: normalizeInt(cfg.date.fontSize, 1, 2),
      x: normalizeInt(dateX, 0, 63),
      y: normalizeInt(cfg.date.y, 0, 63),
      color: hexToRgb(cfg.date.color),
    },
    week: {
      show: !!cfg.week.show,
      x: normalizeInt(weekX, 0, 63),
      y: normalizeInt(cfg.week.y, 0, 63),
      color: hexToRgb(cfg.week.color),
    },
    image: {
      show: !!cfg.image.show,
      x: normalizeInt(cfg.image.x, -128, 128),
      y: normalizeInt(cfg.image.y, -128, 128),
      width: normalizeInt(cfg.image.width, 1, 64),
      height: normalizeInt(cfg.image.height, 1, 64),
    },
  };
}

onMounted(() => {
  deviceStore.init();
  host.value = deviceStore.host;
  port.value = deviceStore.port || 80;
  secure.value = deviceStore.secure;
  loadLocal();
});

watch(clockMode, () => {
  loadLocal();
  if (mediaType.value !== "") {
    rebuildMediaData().catch(() => {});
  }
});

watch(
  () => [config.value.image.x, config.value.image.y, config.value.image.width, config.value.image.height],
  () => {
    if (mediaType.value !== "") {
      rebuildMediaData().catch(() => {});
    }
  },
);

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

async function refreshStatus() {
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

async function applyBrightness() {
  message.value = "";
  try {
    await deviceStore.setBrightness(normalizeInt(brightness.value, 0, 255));
    message.value = "亮度设置成功";
  } catch (error) {
    message.value = error.message || "亮度设置失败";
  }
}

async function clearScreen() {
  message.value = "";
  try {
    await deviceStore.clearScreen();
    message.value = "清屏成功";
  } catch (error) {
    message.value = error.message || "清屏失败";
  }
}

function buildBinaryBuffer(pixels) {
  const buffer = new Uint8Array(pixels.length * 5);
  for (let i = 0; i < pixels.length; i += 1) {
    const base = i * 5;
    const pixel = pixels[i];
    buffer[base] = pixel.x;
    buffer[base + 1] = pixel.y;
    buffer[base + 2] = pixel.r;
    buffer[base + 3] = pixel.g;
    buffer[base + 4] = pixel.b;
  }
  return buffer;
}

async function sendStaticImagePixels() {
  if (staticImagePixels.value.length === 0) {
    return null;
  }

  const batchSize = 64;
  for (let start = 0; start < staticImagePixels.value.length; start += batchSize) {
    const end = Math.min(start + batchSize, staticImagePixels.value.length);
    const chunkPixels = staticImagePixels.value.slice(start, end);
    const buffer = buildBinaryBuffer(chunkPixels);
    await deviceStore.sendBinary(buffer.buffer);
    if (end < staticImagePixels.value.length) {
      await sleep(120);
    }
  }

  return deviceStore.waitForMessage(
    (msg) => typeof msg === "object" && msg !== null && msg.message === "pixels_received",
    { timeoutMs: 15000 },
  );
}

async function sendGifAnimation() {
  if (gifAnimationData.value === null) {
    throw new Error("GIF 动画数据未准备好");
  }
  if (gifAnimationData.value.frameCount <= 0) {
    throw new Error("GIF 动画帧为空");
  }

  await deviceStore.setMode("transferring");
  await deviceStore.sendAndWait({
    cmd: "animation_begin",
    frameCount: gifAnimationData.value.frameCount,
  });

  for (let index = 0; index < gifAnimationData.value.frames.length; index += 1) {
    const frame = gifAnimationData.value.frames[index];
    const type = frame[0];
    const delay = frame[1];
    const totalPixels = frame[2];
    const pixels = frame[3] instanceof Uint8Array ? frame[3] : new Uint8Array(frame[3]);

    await deviceStore.sendAndWait({
      cmd: "frame_begin",
      index,
      type,
      delay,
      totalPixels,
    });

    let chunkPixels = 200;
    if (totalPixels > 0 && totalPixels <= 100) {
      chunkPixels = totalPixels;
    }
    if (chunkPixels <= 0) {
      chunkPixels = 1;
    }

    for (let start = 0; start < totalPixels; start += chunkPixels) {
      const end = Math.min(start + chunkPixels, totalPixels);
      const startByte = start * 5;
      const endByte = end * 5;
      const chunk = pixels.subarray(startByte, endByte);
      const payload = chunk.buffer.slice(chunk.byteOffset, chunk.byteOffset + chunk.byteLength);
      await deviceStore.sendBinary(payload);
      if (end < totalPixels) {
        await sleep(80);
      }
    }

    await sleep(120);
    const frameStatus = await deviceStore.sendAndWait({ cmd: "frame_status", index });
    if (typeof frameStatus === "object" && frameStatus !== null) {
      if (frameStatus.status === "error") {
        throw new Error(frameStatus.message || "帧状态校验失败");
      }
      if (typeof frameStatus.count === "number" && frameStatus.count < totalPixels) {
        throw new Error(`帧 ${index} 接收不完整`);
      }
    }
  }

  return deviceStore.sendAndWait(
    { cmd: "animation_end" },
    { timeoutMs: 20000 },
  );
}

async function sendConfig() {
  if (!deviceStore.connected) {
    message.value = "请先连接设备";
    return;
  }

  sending.value = true;
  message.value = "";
  try {
    const payload = buildPayload();
    await deviceStore.setMode(clockMode.value);
    const resp = await deviceStore.setClockConfig(clockMode.value, payload);

    if (config.value.image.show) {
      await rebuildMediaData();
      if (mediaType.value === "gif" && clockMode.value === "animation") {
        const animResp = await sendGifAnimation();
        if (typeof animResp === "object" && animResp !== null && typeof animResp.message === "string") {
          message.value = animResp.message;
        }
      } else if (mediaType.value !== "") {
        const imageResp = await sendStaticImagePixels();
        if (typeof imageResp === "object" && imageResp !== null && typeof imageResp.count === "number") {
          message.value = `像素背景已下发，共 ${imageResp.count} 点`;
        }
      }
    }

    saveLocal(false);
    if (message.value === "") {
      if (typeof resp === "object" && resp !== null && typeof resp.message === "string") {
        message.value = resp.message;
      } else {
        message.value = "时钟配置已发送";
      }
    }
  } catch (error) {
    message.value = error.message || "发送失败";
  } finally {
    sending.value = false;
  }
}
</script>

<style scoped>
.clock-page {
  padding: 28px 0 72px;
}

.page-header {
  margin-bottom: 20px;
  padding: 24px 24px 20px;
  border: 3px solid var(--nb-ink);
  background: linear-gradient(180deg, #ffffff 0%, #eef6ff 100%);
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
  max-width: 700px;
  margin-top: 10px;
  color: var(--nb-text-secondary);
  font-size: 15px;
  line-height: 1.6;
}

.card {
  margin-bottom: 16px;
  padding: 22px;
  border: 3px solid var(--nb-ink);
  background: var(--nb-paper);
  box-shadow: var(--nb-shadow-card);
}

.card-title {
  display: inline-flex;
  align-items: center;
  min-height: 40px;
  margin-bottom: 16px;
  padding: 6px 14px;
  border: 3px solid var(--nb-ink);
  background: var(--nb-yellow);
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 900;
  letter-spacing: 0.01em;
}

.connection-form {
  display: grid;
  grid-template-columns: minmax(0, 1.4fr) 120px auto;
  gap: 12px;
  align-items: end;
  margin-bottom: 14px;
}

.field-row {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-bottom: 12px;
}

.field-row.triple {
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.field-row.quad {
  grid-template-columns: repeat(4, minmax(0, 1fr));
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
  color: var(--nb-text-secondary);
  text-transform: uppercase;
}

.field input,
.field select {
  min-height: 46px;
  padding: 0 14px;
  font-size: 14px;
  font-weight: 700;
}

.field input[type="color"] {
  width: 100%;
  min-height: 52px;
  padding: 6px;
  cursor: pointer;
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
  background: var(--nb-surface-soft);
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

.section-card {
  background:
    linear-gradient(180deg, #ffffff 0%, #fffdf6 100%);
}

.section-card h3 {
  display: inline-flex;
  align-items: center;
  min-height: 36px;
  margin-bottom: 16px;
  padding: 4px 12px;
  border: 3px solid var(--nb-ink);
  background: var(--nb-blue);
  color: var(--nb-ink);
  font-size: 16px;
  font-weight: 900;
  letter-spacing: 0.01em;
}

.switch-row {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  margin-bottom: 14px;
  align-items: center;
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 700;
}

.switch-row label {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  min-height: 42px;
  padding: 0 12px;
  border: 3px solid var(--nb-ink);
  background: var(--nb-muted);
}

.switch-row input[type="checkbox"] {
  width: 16px;
  height: 16px;
  accent-color: var(--nb-yellow);
}

.switch-row select {
  min-height: 34px;
  padding: 0 36px 0 10px;
  margin-left: 0;
  font-size: 13px;
  font-weight: 700;
}

.actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.status {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 16px;
  color: var(--nb-text-secondary);
  font-size: 13px;
  font-weight: 700;
}

.dot {
  width: 14px;
  height: 14px;
  border: 3px solid var(--nb-ink);
  background: #d0d0d0;
  flex: 0 0 auto;
}

.dot.online {
  background: var(--nb-green);
}

.badge {
  display: inline-flex;
  align-items: center;
  min-height: 34px;
  padding: 6px 12px;
  border: 3px solid var(--nb-ink);
  background: var(--nb-muted);
  color: var(--nb-ink);
  white-space: nowrap;
}

.grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.sections {
  align-items: start;
}

.brightness-row {
  margin: 14px 0;
  padding: 14px;
  border: 3px solid var(--nb-ink);
  background: var(--nb-surface-soft);
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

.msg {
  margin-top: 14px;
  padding: 10px 12px;
  border: 2px solid var(--nb-ink);
  background: var(--nb-muted);
  font-size: 13px;
  font-weight: 700;
  color: var(--nb-text-secondary);
}

.msg.error {
  background: #ffe1df;
  color: #8a1f1f;
}

.media-tip {
  margin-top: 14px;
  padding: 12px 14px;
  border: 3px solid var(--nb-ink);
  background: #eef6ff;
  color: var(--nb-text-secondary);
  font-size: 12px;
  font-weight: 700;
  line-height: 1.6;
}

.hidden-input {
  display: none;
}

@media (max-width: 1000px) {
  .grid {
    grid-template-columns: 1fr;
  }

  .field-row.quad {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .connection-form {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 680px) {
  .clock-page {
    padding: 20px 0 56px;
  }

  .page-header,
  .card {
    padding: 18px;
  }

  .page-title {
    font-size: 28px;
  }

  .field-row.triple,
  .field-row {
    grid-template-columns: 1fr;
  }

  .actions .btn {
    width: 100%;
  }
}
</style>
