#include "web_server.h"
#include "websocket_handler.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "config_manager.h"
#include "ota_manager.h"
#include "theme_renderer.h"

namespace {
struct ThemePageItem {
  const char* id;
  const char* name;
};

struct AmbientPageItem {
  const char* id;
  const char* name;
  const char* desc;
};

const ThemePageItem kThemePageItems[] = {
  {"clockwise_mario", "Mario"},
  {"clockwise_words", "文字时间"},
  {"clockwise_worldmap", "地图时钟"},
  {"clockwise_castlevania", "针塔时钟"},
  {"clockwise_pacman", "吃豆人"},
  {"clockwise_pokedex", "图鉴"},
  {"clockwise_canvas", "画布主题"}
};

const AmbientPageItem kAmbientPageItems[] = {
  {"digital_rain", "Digital Rain", "字符雨风格，适合做科技感屏保"},
  {"starfield", "Starfield", "低亮星空缓慢漂移，适合夜间待机"},
  {"neon_tunnel", "Neon Tunnel", "霓虹隧道纵深感更强，动态更集中"},
  {"metablob", "Metablob", "流体团块不断聚合分离，层次更自然"},
  {"falling_sand", "Falling Sand", "细颗粒往下流动，画面更安静"},
  {"sparks", "Sparks", "火花从底部向外迸发，暖色氛围更明显"},
  {"wave_pattern", "Wave Pattern", "图案化波纹持续流动，适合简洁桌搭"},
  {"rain_scene", "Rain Scene", "偏真实雨幕感，整体更克制"},
  {"watermelon_plasma", "Watermelon Plasma", "高彩红绿等离子流动，画面更活"},
  {"boids", "Boids", "粒子群游，像小生物一样在屏里穿梭"},
  {"bouncing_logo", "Bouncing Logo", "九宫格徽标在屏里反弹移动"},
  {"sorting_visualizer", "Sorting Visualizer", "排序柱条不断交换重排"},
  {"clock_scene", "Clock Scene", "场景化时钟读数，适合桌面待机"},
  {"countdown_scene", "Countdown Scene", "大字倒计时搭配底部进度条"},
  {"weather_scene", "Weather Scene", "天气图标与温度同屏展示"},
  {"game_of_life", "Game Of Life", "经典细胞自动机不断演化"},
  {"julia_set", "Julia Set", "分形纹理持续变化"},
  {"reaction_diffusion", "Reaction Diffusion", "类似细胞生长的纹理扩散"}
};

const char* ambientPresetToString(uint8_t preset) {
  if (preset == AMBIENT_PRESET_AURORA) {
    return "digital_rain";
  }
  if (preset == AMBIENT_PRESET_PLASMA) {
    return "starfield";
  }
  if (preset == AMBIENT_PRESET_MATRIX_RAIN) {
    return "neon_tunnel";
  }
  if (preset == AMBIENT_PRESET_FIREFLY_SWARM) {
    return "metablob";
  }
  if (preset == AMBIENT_PRESET_METEOR_SHOWER) {
    return "falling_sand";
  }
  if (preset == AMBIENT_PRESET_OCEAN_CURRENT) {
    return "sparks";
  }
  if (preset == AMBIENT_PRESET_NEON_GRID) {
    return "wave_pattern";
  }
  if (preset == AMBIENT_PRESET_SUNSET_BLUSH) {
    return "rain_scene";
  }
  if (preset == AMBIENT_PRESET_STARFIELD_DRIFT) {
    return "watermelon_plasma";
  }
  if (preset == AMBIENT_PRESET_BOIDS) {
    return "boids";
  }
  if (preset == AMBIENT_PRESET_BOUNCING_LOGO) {
    return "bouncing_logo";
  }
  if (preset == AMBIENT_PRESET_SORTING_VISUALIZER) {
    return "sorting_visualizer";
  }
  if (preset == AMBIENT_PRESET_CLOCK_SCENE) {
    return "clock_scene";
  }
  if (preset == AMBIENT_PRESET_COUNTDOWN_SCENE) {
    return "countdown_scene";
  }
  if (preset == AMBIENT_PRESET_WEATHER_SCENE) {
    return "weather_scene";
  }
  if (preset == AMBIENT_PRESET_GAME_OF_LIFE) {
    return "game_of_life";
  }
  if (preset == AMBIENT_PRESET_JULIA_SET) {
    return "julia_set";
  }
  if (preset == AMBIENT_PRESET_REACTION_DIFFUSION) {
    return "reaction_diffusion";
  }
  return "digital_rain";
}

String buildThemeBrowserPage() {
  String currentThemeId = ConfigManager::themeConfig.themeId;
  if (currentThemeId.length() == 0) {
    currentThemeId = kThemePageItems[0].id;
  }

  String themesJson = "[";
  for (size_t i = 0; i < sizeof(kThemePageItems) / sizeof(kThemePageItems[0]); i++) {
    if (i > 0) {
      themesJson += ",";
    }
    themesJson += "{\"id\":\"";
    themesJson += kThemePageItems[i].id;
    themesJson += "\",\"name\":\"";
    themesJson += kThemePageItems[i].name;
    themesJson += "\"}";
  }
  themesJson += "]";

  String html;
  html.reserve(12000);
  html += R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
  <title>Glowxel 主题模式</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #06080c;
      --panel: #101723;
      --panel-2: #151f2d;
      --line: #243244;
      --text: #eef4ff;
      --muted: #92a0b6;
      --accent: #62c4ff;
      --accent-2: #8affc1;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "SF Pro Display", "PingFang SC", "Microsoft YaHei", sans-serif;
      background:
        radial-gradient(circle at top, rgba(98, 196, 255, 0.16), transparent 32%),
        linear-gradient(180deg, #08111b 0%, var(--bg) 100%);
      color: var(--text);
    }
    .page {
      min-height: 100vh;
      padding: 18px;
      display: grid;
      gap: 18px;
      grid-template-columns: minmax(320px, 420px) minmax(320px, 1fr);
    }
    .panel {
      background: rgba(16, 23, 35, 0.94);
      border: 1px solid var(--line);
      border-radius: 18px;
      padding: 18px;
      backdrop-filter: blur(12px);
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.28);
    }
    .title {
      margin: 0 0 8px;
      font-size: 22px;
      font-weight: 700;
    }
    .sub {
      margin: 0;
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
    }
    .status {
      margin-top: 12px;
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 8px 12px;
      border-radius: 999px;
      background: rgba(255,255,255,0.04);
      color: var(--muted);
      font-size: 12px;
    }
    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #ffb347;
    }
    .status.connected .status-dot {
      background: var(--accent-2);
      box-shadow: 0 0 12px rgba(138,255,193,0.8);
    }
    .preview-wrap {
      display: flex;
      flex-direction: column;
      gap: 14px;
      align-items: center;
      justify-content: center;
    }
    .screen-shell {
      position: relative;
      width: min(72vw, 420px);
      aspect-ratio: 1;
      border-radius: 24px;
      background: #000;
      border: 1px solid rgba(255,255,255,0.08);
      overflow: hidden;
      box-shadow: inset 0 0 0 1px rgba(255,255,255,0.05);
    }
    #preview {
      width: 100%;
      height: 100%;
      display: block;
      image-rendering: pixelated;
      background: #000;
    }
    .screen-grid {
      position: absolute;
      inset: 0;
      pointer-events: none;
      background-image:
        linear-gradient(to right, rgba(255,255,255,0.10) 1px, transparent 1px),
        linear-gradient(to bottom, rgba(255,255,255,0.10) 1px, transparent 1px);
      background-size: calc(100% / 64) calc(100% / 64);
    }
    .preview-meta {
      width: 100%;
      display: flex;
      justify-content: space-between;
      gap: 12px;
      align-items: center;
      color: var(--muted);
      font-size: 12px;
    }
    .theme-name {
      color: var(--text);
      font-size: 18px;
      font-weight: 700;
    }
    .theme-grid {
      margin-top: 18px;
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 10px;
    }
    .theme-btn {
      width: 100%;
      border: 1px solid var(--line);
      border-radius: 14px;
      background: var(--panel-2);
      color: var(--text);
      padding: 14px 12px;
      text-align: left;
      font-size: 13px;
      font-weight: 600;
    }
    .theme-btn.active {
      border-color: var(--accent);
      box-shadow: inset 0 0 0 1px rgba(98, 196, 255, 0.25);
      background: linear-gradient(180deg, rgba(98,196,255,0.18), rgba(98,196,255,0.05));
    }
    .actions {
      margin-top: 14px;
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
    }
    .action-btn {
      border: 1px solid var(--line);
      border-radius: 12px;
      background: rgba(255,255,255,0.04);
      color: var(--text);
      padding: 10px 14px;
      font-size: 13px;
      font-weight: 600;
    }
    @media (max-width: 960px) {
      .page {
        grid-template-columns: 1fr;
      }
      .screen-shell {
        width: min(92vw, 420px);
      }
    }
  </style>
</head>
<body>
  <div class="page">
    <section class="panel">
      <h1 class="title">板载主题模式</h1>
      <p class="sub">这里直接切换 ESP32 当前主题。右侧预览区读取的是板子当前真实 64x64 渲染帧，不是单独再画一套假预览。</p>
      <div id="status" class="status">
        <span class="status-dot"></span>
        <span id="status-text">正在连接设备 WebSocket...</span>
      </div>
      <div id="theme-grid" class="theme-grid"></div>
      <div class="actions">
        <button id="refresh-btn" class="action-btn" type="button">刷新当前预览</button>
        <button id="reapply-btn" class="action-btn" type="button">重新应用当前主题</button>
      </div>
    </section>

    <section class="panel preview-wrap">
      <div class="screen-shell">
        <canvas id="preview" width="64" height="64"></canvas>
        <div class="screen-grid"></div>
      </div>
      <div class="preview-meta">
        <div>
          <div class="theme-name" id="current-theme-name">--</div>
          <div>主题模式实时预览</div>
        </div>
        <div>64 x 64</div>
      </div>
    </section>
  </div>

  <script>
    const THEMES = )HTML";
  html += themesJson;
  html += R"HTML(;
    let activeThemeId = ")HTML";
  html += currentThemeId;
  html += R"HTML(";
    let ws = null;
    let frameTimer = null;

    const statusEl = document.getElementById("status");
    const statusTextEl = document.getElementById("status-text");
    const themeGridEl = document.getElementById("theme-grid");
    const currentThemeNameEl = document.getElementById("current-theme-name");
    const previewCanvas = document.getElementById("preview");
    const previewCtx = previewCanvas.getContext("2d", { alpha: false });

    function getThemeName(themeId) {
      const found = THEMES.find((item) => item.id === themeId);
      return found ? found.name : themeId;
    }

    function setStatus(text, connected) {
      statusTextEl.textContent = text;
      statusEl.classList.toggle("connected", !!connected);
    }

    function renderThemeButtons() {
      themeGridEl.innerHTML = "";
      THEMES.forEach((theme) => {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.className = "theme-btn" + (theme.id === activeThemeId ? " active" : "");
        btn.textContent = theme.name;
        btn.addEventListener("click", () => applyTheme(theme.id));
        themeGridEl.appendChild(btn);
      });
      currentThemeNameEl.textContent = getThemeName(activeThemeId);
    }

    function connectSocket() {
      const protocol = location.protocol === "https:" ? "wss://" : "ws://";
      ws = new WebSocket(protocol + location.host + "/ws");

      ws.addEventListener("open", () => {
        setStatus("WebSocket 已连接，主题切换可直接发送到板子", true);
        if (!activeThemeId && THEMES.length > 0) {
          activeThemeId = THEMES[0].id;
        }
        renderThemeButtons();
      });

      ws.addEventListener("close", () => {
        setStatus("连接已断开，正在重连...", false);
        window.setTimeout(connectSocket, 1500);
      });

      ws.addEventListener("error", () => {
        setStatus("WebSocket 连接异常", false);
      });
    }

    function sendJson(payload) {
      if (!ws || ws.readyState !== WebSocket.OPEN) {
        return;
      }
      ws.send(JSON.stringify(payload));
    }

    function applyTheme(themeId) {
      activeThemeId = themeId;
      renderThemeButtons();
      reapplyCurrentTheme();
    }

    function reapplyCurrentTheme() {
      if (!activeThemeId) {
        return;
      }
      sendJson({ cmd: "set_theme_config", themeId: activeThemeId });
    }

    function drawFrame(buffer) {
      const pixels = new Uint16Array(buffer);
      if (pixels.length < 64 * 64) {
        return;
      }

      const imageData = previewCtx.createImageData(64, 64);
      for (let i = 0; i < 64 * 64; i++) {
        const color = pixels[i];
        const r5 = (color >> 11) & 0x1f;
        const g6 = (color >> 5) & 0x3f;
        const b5 = color & 0x1f;
        const index = i * 4;
        imageData.data[index] = (r5 << 3) | (r5 >> 2);
        imageData.data[index + 1] = (g6 << 2) | (g6 >> 4);
        imageData.data[index + 2] = (b5 << 3) | (b5 >> 2);
        imageData.data[index + 3] = 255;
      }
      previewCtx.putImageData(imageData, 0, 0);
    }

    async function fetchFrame() {
      try {
        const response = await fetch("/theme-frame.bin?ts=" + Date.now(), { cache: "no-store" });
        const buffer = await response.arrayBuffer();
        drawFrame(buffer);
      } catch (error) {
        setStatus("预览帧读取失败", false);
      } finally {
        frameTimer = window.setTimeout(fetchFrame, 240);
      }
    }

    document.getElementById("refresh-btn").addEventListener("click", fetchFrame);
    document.getElementById("reapply-btn").addEventListener("click", reapplyCurrentTheme);

    renderThemeButtons();
    connectSocket();
    fetchFrame();
  </script>
</body>
</html>)HTML";

  return html;
}

String buildAmbientEffectsPage() {
  String presetsJson = "[";
  for (size_t i = 0; i < sizeof(kAmbientPageItems) / sizeof(kAmbientPageItems[0]); i++) {
    if (i > 0) {
      presetsJson += ",";
    }
    presetsJson += "{\"id\":\"";
    presetsJson += kAmbientPageItems[i].id;
    presetsJson += "\",\"name\":\"";
    presetsJson += kAmbientPageItems[i].name;
    presetsJson += "\",\"desc\":\"";
    presetsJson += kAmbientPageItems[i].desc;
    presetsJson += "\"}";
  }
  presetsJson += "]";

  String html;
  html.reserve(16000);
  html += R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
  <title>Glowxel 板载特效</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #05070b;
      --panel: rgba(13, 18, 28, 0.94);
      --panel-2: rgba(18, 25, 37, 0.98);
      --line: #233146;
      --text: #eff5ff;
      --muted: #95a3ba;
      --accent: #66d9ff;
      --accent-2: #9dffb3;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "SF Pro Display", "PingFang SC", "Microsoft YaHei", sans-serif;
      background:
        radial-gradient(circle at top, rgba(102, 217, 255, 0.14), transparent 30%),
        linear-gradient(180deg, #07101a 0%, var(--bg) 100%);
      color: var(--text);
    }
    .page {
      min-height: 100vh;
      padding: 18px;
      display: grid;
      gap: 18px;
      grid-template-columns: minmax(320px, 430px) minmax(340px, 1fr);
    }
    .panel {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 18px;
      padding: 18px;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.28);
      backdrop-filter: blur(14px);
    }
    .title {
      margin: 0 0 8px;
      font-size: 22px;
      font-weight: 700;
    }
    .sub {
      margin: 0;
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
    }
    .status {
      margin-top: 12px;
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 8px 12px;
      border-radius: 999px;
      background: rgba(255,255,255,0.04);
      color: var(--muted);
      font-size: 12px;
    }
    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #ffb347;
    }
    .status.connected .status-dot {
      background: var(--accent-2);
      box-shadow: 0 0 12px rgba(157,255,179,0.8);
    }
    .preset-grid {
      margin-top: 18px;
      display: grid;
      grid-template-columns: 1fr;
      gap: 10px;
    }
    .preset-btn {
      width: 100%;
      border: 1px solid var(--line);
      border-radius: 14px;
      background: var(--panel-2);
      color: var(--text);
      padding: 14px;
      text-align: left;
    }
    .preset-btn.active {
      border-color: var(--accent);
      background: linear-gradient(180deg, rgba(102,217,255,0.18), rgba(102,217,255,0.05));
      box-shadow: inset 0 0 0 1px rgba(102,217,255,0.18);
    }
    .preset-btn strong {
      display: block;
      margin-bottom: 6px;
      font-size: 14px;
    }
    .preset-btn span {
      display: block;
      color: var(--muted);
      font-size: 12px;
      line-height: 1.5;
    }
    .control-group {
      margin-top: 18px;
      display: grid;
      gap: 14px;
    }
    .control-row {
      display: grid;
      gap: 6px;
    }
    .control-row label {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      color: var(--muted);
      font-size: 12px;
    }
    input[type="range"] {
      width: 100%;
    }
    .toggle-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 12px;
      border: 1px solid var(--line);
      border-radius: 12px;
      background: rgba(255,255,255,0.03);
      color: var(--muted);
      font-size: 13px;
    }
    .actions {
      margin-top: 16px;
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
    }
    .action-btn {
      border: 1px solid var(--line);
      border-radius: 12px;
      background: rgba(255,255,255,0.04);
      color: var(--text);
      padding: 10px 14px;
      font-size: 13px;
      font-weight: 600;
    }
    .preview-wrap {
      display: flex;
      flex-direction: column;
      gap: 14px;
      align-items: center;
      justify-content: center;
    }
    .screen-shell {
      position: relative;
      width: min(72vw, 420px);
      aspect-ratio: 1;
      border-radius: 24px;
      background: #000;
      border: 1px solid rgba(255,255,255,0.08);
      overflow: hidden;
      box-shadow: inset 0 0 0 1px rgba(255,255,255,0.05);
    }
    #preview {
      width: 100%;
      height: 100%;
      display: block;
      image-rendering: pixelated;
      background: #000;
    }
    .screen-grid {
      position: absolute;
      inset: 0;
      pointer-events: none;
      background-image:
        linear-gradient(to right, rgba(255,255,255,0.08) 1px, transparent 1px),
        linear-gradient(to bottom, rgba(255,255,255,0.08) 1px, transparent 1px);
      background-size: calc(100% / 64) calc(100% / 64);
    }
    .preview-meta {
      width: 100%;
      display: flex;
      justify-content: space-between;
      gap: 12px;
      color: var(--muted);
      font-size: 12px;
      align-items: center;
    }
    .current-name {
      color: var(--text);
      font-size: 18px;
      font-weight: 700;
    }
    @media (max-width: 960px) {
      .page { grid-template-columns: 1fr; }
      .screen-shell { width: min(92vw, 420px); }
    }
  </style>
</head>
<body>
  <div class="page">
    <section class="panel">
      <h1 class="title">板载氛围特效</h1>
      <p class="sub">这里直接调用 ESP32 固件里的内置特效。右侧预览读取的是设备当前真实显示帧，不依赖其他前端。</p>
      <div id="status" class="status">
        <span class="status-dot"></span>
        <span id="status-text">正在连接设备 WebSocket...</span>
      </div>
      <div id="preset-grid" class="preset-grid"></div>
      <div class="control-group">
        <div class="control-row">
          <label><span>速度</span><strong id="speed-value">6</strong></label>
          <input id="speed-input" type="range" min="1" max="10" step="1">
        </div>
        <div class="control-row">
          <label><span>强度</span><strong id="intensity-value">72</strong></label>
          <input id="intensity-input" type="range" min="10" max="100" step="1">
        </div>
        <div class="toggle-row">
          <span>持续循环</span>
          <input id="loop-input" type="checkbox">
        </div>
      </div>
      <div class="actions">
        <button id="apply-btn" class="action-btn" type="button">应用到设备</button>
        <button id="refresh-btn" class="action-btn" type="button">刷新预览</button>
      </div>
    </section>

    <section class="panel preview-wrap">
      <div class="screen-shell">
        <canvas id="preview" width="64" height="64"></canvas>
        <div class="screen-grid"></div>
      </div>
      <div class="preview-meta">
        <div>
          <div class="current-name" id="current-name">--</div>
          <div>板载特效实时预览</div>
        </div>
        <div>64 x 64</div>
      </div>
    </section>
  </div>

  <script>
    const PRESETS = )HTML";
  html += presetsJson;
  html += R"HTML(;
    let activePresetId = ")HTML";
  html += ambientPresetToString(DisplayManager::ambientEffectConfig.preset);
  html += R"HTML(";
    let ws = null;
    let frameTimer = null;

    const state = {
      speed: )HTML";
  html += String(DisplayManager::ambientEffectConfig.speed);
  html += R"HTML(,
      intensity: )HTML";
  html += String(DisplayManager::ambientEffectConfig.intensity);
  html += R"HTML(,
      loop: )HTML";
  html += (DisplayManager::ambientEffectConfig.loop ? "true" : "false");
  html += R"HTML(
    };

    const statusEl = document.getElementById("status");
    const statusTextEl = document.getElementById("status-text");
    const presetGridEl = document.getElementById("preset-grid");
    const currentNameEl = document.getElementById("current-name");
    const previewCanvas = document.getElementById("preview");
    const previewCtx = previewCanvas.getContext("2d", { alpha: false });
    const speedInput = document.getElementById("speed-input");
    const intensityInput = document.getElementById("intensity-input");
    const loopInput = document.getElementById("loop-input");
    const speedValueEl = document.getElementById("speed-value");
    const intensityValueEl = document.getElementById("intensity-value");

    function getPresetName(presetId) {
      const found = PRESETS.find((item) => item.id === presetId);
      return found ? found.name : presetId;
    }

    async function syncFromStatus() {
      try {
        const response = await fetch("/status?ts=" + Date.now(), { cache: "no-store" });
        const data = await response.json();
        if (data.effectPreset) {
          activePresetId = data.effectPreset;
        }
        if (typeof data.effectSpeed === "number") {
          state.speed = data.effectSpeed;
        }
        if (typeof data.effectIntensity === "number") {
          state.intensity = data.effectIntensity;
        }
        if (typeof data.effectLoop === "boolean") {
          state.loop = data.effectLoop;
        }
        renderPresetButtons();
        syncControls();
      } catch (error) {
        setStatus("状态读取失败，已使用本地默认值", false);
      }
    }

    function syncControls() {
      speedInput.value = state.speed;
      intensityInput.value = state.intensity;
      loopInput.checked = !!state.loop;
      speedValueEl.textContent = state.speed;
      intensityValueEl.textContent = state.intensity;
      currentNameEl.textContent = getPresetName(activePresetId);
    }

    function setStatus(text, connected) {
      statusTextEl.textContent = text;
      statusEl.classList.toggle("connected", !!connected);
    }

    function renderPresetButtons() {
      presetGridEl.innerHTML = "";
      PRESETS.forEach((preset) => {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.className = "preset-btn" + (preset.id === activePresetId ? " active" : "");
        btn.innerHTML = "<strong>" + preset.name + "</strong><span>" + preset.desc + "</span>";
        btn.addEventListener("click", () => {
          activePresetId = preset.id;
          renderPresetButtons();
          syncControls();
        });
        presetGridEl.appendChild(btn);
      });
    }

    function connectSocket() {
      const protocol = location.protocol === "https:" ? "wss://" : "ws://";
      ws = new WebSocket(protocol + location.host + "/ws");

      ws.addEventListener("open", () => {
        setStatus("WebSocket 已连接，可直接切换板载特效", true);
        renderPresetButtons();
        syncControls();
        syncFromStatus();
      });

      ws.addEventListener("close", () => {
        setStatus("连接已断开，正在重连...", false);
        window.setTimeout(connectSocket, 1500);
      });

      ws.addEventListener("error", () => {
        setStatus("WebSocket 连接异常", false);
      });
    }

    function sendJson(payload) {
      if (!ws || ws.readyState !== WebSocket.OPEN) {
        return;
      }
      ws.send(JSON.stringify(payload));
    }

    function applyPreset() {
      sendJson({
        cmd: "set_ambient_effect",
        preset: activePresetId,
        speed: Number(speedInput.value),
        intensity: Number(intensityInput.value),
        loop: !!loopInput.checked
      });
    }

    function drawFrame(buffer) {
      const pixels = new Uint16Array(buffer);
      if (pixels.length < 64 * 64) {
        return;
      }

      const imageData = previewCtx.createImageData(64, 64);
      for (let i = 0; i < 64 * 64; i++) {
        const color = pixels[i];
        const r5 = (color >> 11) & 0x1f;
        const g6 = (color >> 5) & 0x3f;
        const b5 = color & 0x1f;
        const index = i * 4;
        imageData.data[index] = (r5 << 3) | (r5 >> 2);
        imageData.data[index + 1] = (g6 << 2) | (g6 >> 4);
        imageData.data[index + 2] = (b5 << 3) | (b5 >> 2);
        imageData.data[index + 3] = 255;
      }
      previewCtx.putImageData(imageData, 0, 0);
    }

    async function fetchFrame() {
      try {
        const response = await fetch("/live-frame.bin?ts=" + Date.now(), { cache: "no-store" });
        const buffer = await response.arrayBuffer();
        drawFrame(buffer);
      } catch (error) {
        setStatus("预览帧读取失败", false);
      } finally {
        frameTimer = window.setTimeout(fetchFrame, 180);
      }
    }

    speedInput.addEventListener("input", () => {
      state.speed = Number(speedInput.value);
      syncControls();
    });
    intensityInput.addEventListener("input", () => {
      state.intensity = Number(intensityInput.value);
      syncControls();
    });
    loopInput.addEventListener("change", () => {
      state.loop = !!loopInput.checked;
    });

    document.getElementById("apply-btn").addEventListener("click", applyPreset);
    document.getElementById("refresh-btn").addEventListener("click", fetchFrame);

    renderPresetButtons();
    syncControls();
    connectSocket();
    syncFromStatus();
    fetchFrame();
  </script>
</body>
</html>)HTML";

  return html;
}

String buildControlHubPage() {
  String html;
  html.reserve(13000);
  html += R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
  <title>Glowxel 板载控制</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #06080d;
      --panel: rgba(14, 20, 31, 0.94);
      --line: #243142;
      --text: #eef5ff;
      --muted: #91a1b8;
      --accent: #67d9ff;
      --accent-2: #9effc4;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "SF Pro Display", "PingFang SC", "Microsoft YaHei", sans-serif;
      background:
        radial-gradient(circle at top, rgba(103, 217, 255, 0.14), transparent 32%),
        linear-gradient(180deg, #08111b 0%, var(--bg) 100%);
      color: var(--text);
    }
    .page {
      min-height: 100vh;
      padding: 18px;
      display: grid;
      gap: 18px;
      grid-template-columns: minmax(320px, 420px) minmax(320px, 1fr);
    }
    .panel {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 18px;
      padding: 18px;
      backdrop-filter: blur(14px);
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.28);
    }
    .title {
      margin: 0 0 8px;
      font-size: 22px;
      font-weight: 700;
    }
    .sub {
      margin: 0;
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
    }
    .status {
      margin-top: 12px;
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 8px 12px;
      border-radius: 999px;
      background: rgba(255,255,255,0.04);
      color: var(--muted);
      font-size: 12px;
    }
    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #ffb347;
    }
    .status.connected .status-dot {
      background: var(--accent-2);
      box-shadow: 0 0 12px rgba(158,255,196,0.8);
    }
    .nav-grid {
      margin-top: 18px;
      display: grid;
      gap: 12px;
    }
    .nav-card {
      display: block;
      border: 1px solid var(--line);
      border-radius: 16px;
      padding: 16px;
      text-decoration: none;
      color: inherit;
      background: rgba(255,255,255,0.03);
    }
    .nav-card strong {
      display: block;
      margin-bottom: 6px;
      font-size: 16px;
    }
    .nav-card span {
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
    }
    .control-box {
      margin-top: 18px;
      display: grid;
      gap: 14px;
    }
    .section-label {
      color: var(--muted);
      font-size: 12px;
      letter-spacing: 0.04em;
      text-transform: uppercase;
    }
    .mode-grid {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 10px;
    }
    .mode-btn {
      border: 1px solid var(--line);
      border-radius: 12px;
      background: rgba(255,255,255,0.04);
      color: var(--text);
      padding: 10px 12px;
      font-size: 13px;
      font-weight: 600;
    }
    .mode-btn.active {
      border-color: var(--accent);
      box-shadow: inset 0 0 0 1px rgba(103,217,255,0.18);
      background: linear-gradient(180deg, rgba(103,217,255,0.18), rgba(103,217,255,0.05));
    }
    .brightness-box {
      display: grid;
      gap: 8px;
      padding: 12px;
      border: 1px solid var(--line);
      border-radius: 14px;
      background: rgba(255,255,255,0.03);
    }
    .brightness-head {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      color: var(--muted);
      font-size: 13px;
    }
    .brightness-head strong {
      color: var(--text);
    }
    .chip-grid {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 10px;
    }
    .chip-btn {
      border: 1px solid var(--line);
      border-radius: 12px;
      background: rgba(255,255,255,0.04);
      color: var(--text);
      padding: 10px 12px;
      text-align: left;
      font-size: 13px;
      font-weight: 600;
    }
    .chip-btn.active {
      border-color: var(--accent);
      box-shadow: inset 0 0 0 1px rgba(103,217,255,0.18);
      background: linear-gradient(180deg, rgba(103,217,255,0.18), rgba(103,217,255,0.05));
    }
    .preview-wrap {
      display: flex;
      flex-direction: column;
      gap: 14px;
      align-items: center;
      justify-content: center;
    }
    .screen-shell {
      position: relative;
      width: min(72vw, 420px);
      aspect-ratio: 1;
      border-radius: 24px;
      background: #000;
      border: 1px solid rgba(255,255,255,0.08);
      overflow: hidden;
    }
    #preview {
      width: 100%;
      height: 100%;
      display: block;
      image-rendering: pixelated;
      background: #000;
    }
    .screen-grid {
      position: absolute;
      inset: 0;
      pointer-events: none;
      background-image:
        linear-gradient(to right, rgba(255,255,255,0.08) 1px, transparent 1px),
        linear-gradient(to bottom, rgba(255,255,255,0.08) 1px, transparent 1px);
      background-size: calc(100% / 64) calc(100% / 64);
    }
    .meta {
      width: 100%;
      display: grid;
      gap: 10px;
    }
    .meta-item {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      padding: 10px 12px;
      border: 1px solid var(--line);
      border-radius: 12px;
      background: rgba(255,255,255,0.03);
      color: var(--muted);
      font-size: 13px;
    }
    .meta-item strong {
      color: var(--text);
    }
    @media (max-width: 960px) {
      .page { grid-template-columns: 1fr; }
      .screen-shell { width: min(92vw, 420px); }
    }
  </style>
</head>
<body>
  <div class="page">
    <section class="panel">
      <h1 class="title">板载控制中心</h1>
      <p class="sub">这里是设备上的快捷入口。可以直接跳到主题模式和氛围特效页，右侧展示当前设备的真实 64x64 帧。</p>
      <div id="status" class="status">
        <span class="status-dot"></span>
        <span id="status-text">正在读取设备状态...</span>
      </div>
      <div class="nav-grid">
        <a class="nav-card" href="/themes">
          <strong>主题模式</strong>
          <span>切换板载主题，查看当前真实预览。</span>
        </a>
        <a class="nav-card" href="/effects">
          <strong>氛围特效</strong>
          <span>切换内置特效，调整速度与强度。</span>
        </a>
        <a class="nav-card" href="/status">
          <strong>设备状态 JSON</strong>
          <span>查看当前模式、亮度、主题、特效与运行状态。</span>
        </a>
      </div>
      <div class="control-box">
        <div class="section-label">快捷切换</div>
        <div class="mode-grid">
          <button id="mode-clock" class="mode-btn" type="button">静态时钟</button>
          <button id="mode-theme" class="mode-btn" type="button">主题模式</button>
          <button id="mode-ambient" class="mode-btn" type="button">氛围特效</button>
        </div>
        <div class="section-label">常用主题</div>
        <div id="theme-chip-grid" class="chip-grid"></div>
        <div class="section-label">常用特效</div>
        <div id="effect-chip-grid" class="chip-grid"></div>
        <div class="section-label">亮度</div>
        <div class="brightness-box">
          <div class="brightness-head">
            <span>设备亮度</span>
            <strong id="brightness-slider-value">--</strong>
          </div>
          <input id="brightness-slider" type="range" min="0" max="255" step="1">
        </div>
      </div>
    </section>

    <section class="panel preview-wrap">
      <div class="screen-shell">
        <canvas id="preview" width="64" height="64"></canvas>
        <div class="screen-grid"></div>
      </div>
      <div class="meta">
        <div class="meta-item"><span>当前模式</span><strong id="mode-text">--</strong></div>
        <div class="meta-item"><span>当前主题</span><strong id="theme-text">--</strong></div>
        <div class="meta-item"><span>当前特效</span><strong id="effect-text">--</strong></div>
        <div class="meta-item"><span>亮度</span><strong id="brightness-text">--</strong></div>
      </div>
    </section>
  </div>

  <script>
    const QUICK_THEMES = [
      { id: "clockwise_mario", name: "Mario" },
      { id: "clockwise_words", name: "文字时间" },
      { id: "clockwise_worldmap", name: "地图时钟" },
      { id: "clockwise_castlevania", name: "针塔时钟" },
      { id: "clockwise_pacman", name: "吃豆人" },
      { id: "clockwise_pokedex", name: "图鉴" },
      { id: "clockwise_canvas", name: "画布主题" }
    ];
    const QUICK_EFFECTS = [
      { id: "digital_rain", name: "数字雨" },
      { id: "starfield", name: "星空" },
      { id: "neon_tunnel", name: "霓虹隧道" },
      { id: "metablob", name: "流体团块" },
      { id: "falling_sand", name: "流沙" },
      { id: "watermelon_plasma", name: "西瓜等离子" },
      { id: "boids", name: "群游粒子" },
      { id: "bouncing_logo", name: "弹跳徽标" },
      { id: "sorting_visualizer", name: "排序柱阵" },
      { id: "clock_scene", name: "场景时钟" },
      { id: "countdown_scene", name: "倒计时" },
      { id: "weather_scene", name: "天气场景" },
      { id: "game_of_life", name: "生命游戏" },
      { id: "julia_set", name: "朱莉亚集" },
      { id: "reaction_diffusion", name: "反应扩散" }
    ];
    const statusEl = document.getElementById("status");
    const statusTextEl = document.getElementById("status-text");
    const previewCanvas = document.getElementById("preview");
    const previewCtx = previewCanvas.getContext("2d", { alpha: false });
    const brightnessSlider = document.getElementById("brightness-slider");
    const brightnessSliderValue = document.getElementById("brightness-slider-value");
    const themeChipGrid = document.getElementById("theme-chip-grid");
    const effectChipGrid = document.getElementById("effect-chip-grid");
    let ws = null;
    let currentMode = "";
    let currentThemeId = "";
    let currentEffectId = "";

    function setStatus(text, connected) {
      statusTextEl.textContent = text;
      statusEl.classList.toggle("connected", !!connected);
    }

    function setModeButtons(mode) {
      document.getElementById("mode-clock").classList.toggle("active", mode === "clock");
      document.getElementById("mode-theme").classList.toggle("active", mode === "theme");
      document.getElementById("mode-ambient").classList.toggle("active", mode === "ambient_effect");
    }

    function updateBrightnessValue(value) {
      brightnessSlider.value = value;
      brightnessSliderValue.textContent = String(value);
    }

    function renderThemeChips() {
      themeChipGrid.innerHTML = "";
      QUICK_THEMES.forEach((item) => {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.className = "chip-btn" + (item.id === currentThemeId ? " active" : "");
        btn.textContent = item.name;
        btn.addEventListener("click", () => {
          applyThemeQuick(item.id);
        });
        themeChipGrid.appendChild(btn);
      });
    }

    function renderEffectChips() {
      effectChipGrid.innerHTML = "";
      QUICK_EFFECTS.forEach((item) => {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.className = "chip-btn" + (item.id === currentEffectId ? " active" : "");
        btn.textContent = item.name;
        btn.addEventListener("click", () => {
          applyEffectQuick(item.id);
        });
        effectChipGrid.appendChild(btn);
      });
    }

    function connectSocket() {
      const protocol = location.protocol === "https:" ? "wss://" : "ws://";
      ws = new WebSocket(protocol + location.host + "/ws");

      ws.addEventListener("open", () => {
        setStatus("设备在线，可直接切换模式", true);
      });

      ws.addEventListener("close", () => {
        setStatus("WebSocket 已断开，正在重连...", false);
        window.setTimeout(connectSocket, 1500);
      });

      ws.addEventListener("error", () => {
        setStatus("WebSocket 连接异常", false);
      });
    }

    function sendJson(payload) {
      if (!ws || ws.readyState !== WebSocket.OPEN) {
        return;
      }
      ws.send(JSON.stringify(payload));
    }

    async function applyBrightness(value) {
      try {
        await fetch("/brightness", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
          body: "value=" + encodeURIComponent(String(value))
        });
        updateBrightnessValue(value);
        setStatus("亮度已更新", true);
      } catch (error) {
        setStatus("亮度更新失败", false);
      }
    }

    function applyThemeQuick(themeId) {
      sendJson({ cmd: "set_theme_config", themeId });
      currentMode = "theme";
      currentThemeId = themeId;
      setModeButtons(currentMode);
      renderThemeChips();
      setStatus("主题已切换", true);
      window.setTimeout(syncStatus, 220);
    }

    function applyEffectQuick(effectId) {
      sendJson({
        cmd: "set_ambient_effect",
        preset: effectId,
        speed: 6,
        intensity: 72,
        loop: true
      });
      currentMode = "ambient_effect";
      currentEffectId = effectId;
      setModeButtons(currentMode);
      renderEffectChips();
      setStatus("氛围特效已切换", true);
      window.setTimeout(syncStatus, 220);
    }

    function drawFrame(buffer) {
      const pixels = new Uint16Array(buffer);
      if (pixels.length < 64 * 64) {
        return;
      }

      const imageData = previewCtx.createImageData(64, 64);
      for (let i = 0; i < 64 * 64; i++) {
        const color = pixels[i];
        const r5 = (color >> 11) & 0x1f;
        const g6 = (color >> 5) & 0x3f;
        const b5 = color & 0x1f;
        const index = i * 4;
        imageData.data[index] = (r5 << 3) | (r5 >> 2);
        imageData.data[index + 1] = (g6 << 2) | (g6 >> 4);
        imageData.data[index + 2] = (b5 << 3) | (b5 >> 2);
        imageData.data[index + 3] = 255;
      }
      previewCtx.putImageData(imageData, 0, 0);
    }

    async function syncStatus() {
      try {
        const response = await fetch("/status?ts=" + Date.now(), { cache: "no-store" });
        const data = await response.json();
        currentMode = data.mode || "";
        currentThemeId = data.themeId || "";
        currentEffectId = data.effectPreset || "";
        document.getElementById("mode-text").textContent = data.mode || "--";
        document.getElementById("theme-text").textContent = data.themeId || "--";
        document.getElementById("effect-text").textContent = data.effectPreset || data.effectMode || "--";
        document.getElementById("brightness-text").textContent = typeof data.brightness === "number" ? String(data.brightness) : "--";
        if (typeof data.brightness === "number") {
          updateBrightnessValue(data.brightness);
        }
        setModeButtons(currentMode);
        renderThemeChips();
        renderEffectChips();
        setStatus("设备在线，状态已同步", true);
      } catch (error) {
        setStatus("状态读取失败", false);
      }
    }

    async function syncFrame() {
      try {
        const response = await fetch("/live-frame.bin?ts=" + Date.now(), { cache: "no-store" });
        const buffer = await response.arrayBuffer();
        drawFrame(buffer);
      } catch (error) {
        setStatus("实时帧读取失败", false);
      } finally {
        window.setTimeout(syncFrame, 220);
      }
    }

    document.getElementById("mode-clock").addEventListener("click", () => {
      sendJson({ cmd: "set_mode", mode: "clock" });
      currentMode = "clock";
      setModeButtons(currentMode);
      setStatus("已切到静态时钟", true);
      window.setTimeout(syncStatus, 200);
    });

    document.getElementById("mode-theme").addEventListener("click", () => {
      sendJson({ cmd: "set_mode", mode: "theme" });
      currentMode = "theme";
      setModeButtons(currentMode);
      setStatus("已切到主题模式", true);
      window.setTimeout(syncStatus, 200);
    });

    document.getElementById("mode-ambient").addEventListener("click", () => {
      sendJson({ cmd: "set_mode", mode: "ambient_effect" });
      currentMode = "ambient_effect";
      setModeButtons(currentMode);
      setStatus("已切到氛围特效", true);
      window.setTimeout(syncStatus, 200);
    });

    brightnessSlider.addEventListener("input", () => {
      brightnessSliderValue.textContent = brightnessSlider.value;
    });

    brightnessSlider.addEventListener("change", () => {
      applyBrightness(Number(brightnessSlider.value));
    });

    connectSocket();
    renderThemeChips();
    renderEffectChips();
    syncStatus();
    syncFrame();
    window.setInterval(syncStatus, 2000);
  </script>
</body>
</html>)HTML";

  return html;
}
}  // namespace

// 静态成员初始化
AsyncWebServer WebServer::server(80);
uint8_t* WebServer::imageBuffer = nullptr;
size_t WebServer::imageSize = 0;

void WebServer::init() {
  Serial.println("4. 启动HTTP和WebSocket服务器...");
  setupServer();
}

void WebServer::setupServer() {
  // 设置 WebSocket
  server.addHandler(&WebSocketHandler::ws);
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Private-Network", "true");
  
  setupRoutes();
  setupAPIRoutes();
  
  server.begin();
  Serial.println("HTTP服务器已启动在端口80");
  Serial.println("等待客户端连接...");
}

void WebServer::setupRoutes() {
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到测试请求");
    request->send(200, "text/plain", "ESP32 is working!");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html; charset=utf-8", buildControlHubPage());
  });

  server.on("/clear-wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("清除WiFi配置");
    ConfigManager::preferences.begin("wifi", false);
    ConfigManager::preferences.clear();
    ConfigManager::preferences.end();
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi配置已清除，3秒后重启\"}");
    Serial.println("3秒后重启...");
    delay(3000);
    ESP.restart();
  });

  server.on("/themes", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html; charset=utf-8", buildThemeBrowserPage());
  });

  server.on("/effects", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html; charset=utf-8", buildAmbientEffectsPage());
  });

  auto sendLiveFrameBinary = [](AsyncWebServerRequest *request) {
    const uint8_t* frameData =
      reinterpret_cast<const uint8_t*>(DisplayManager::liveFrameBuffer);
    const size_t frameSize = sizeof(DisplayManager::liveFrameBuffer);
    request->send(
      "application/octet-stream",
      frameSize,
      [frameData, frameSize](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
        if (index >= frameSize) {
          return 0;
        }
        size_t remaining = frameSize - index;
        size_t chunkSize = remaining < maxLen ? remaining : maxLen;
        memcpy(buffer, frameData + index, chunkSize);
        return chunkSize;
      }
    );
  };

  server.on("/theme-frame.bin", HTTP_GET, sendLiveFrameBinary);
  server.on("/live-frame.bin", HTTP_GET, sendLiveFrameBinary);

  auto sendLiveFrameText = [](AsyncWebServerRequest *request) {
    const uint8_t* frameData =
      reinterpret_cast<const uint8_t*>(DisplayManager::liveFrameBuffer);
    AsyncWebServerResponse *response = request->beginChunkedResponse(
      "text/plain",
      [frameData](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
        static const char kHexChars[] = "0123456789abcdef";
        const size_t totalHexLength = sizeof(DisplayManager::liveFrameBuffer) * 2;
        if (index >= totalHexLength) {
          return 0;
        }

        size_t remaining = totalHexLength - index;
        size_t chunkSize = remaining < maxLen ? remaining : maxLen;
        for (size_t i = 0; i < chunkSize; i++) {
          size_t hexIndex = index + i;
          uint8_t value = frameData[hexIndex / 2];
          buffer[i] = (hexIndex % 2 == 0)
            ? kHexChars[(value >> 4) & 0x0F]
            : kHexChars[value & 0x0F];
        }
        return chunkSize;
      }
    );
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
  };

  server.on("/theme-frame.txt", HTTP_GET, sendLiveFrameText);
  server.on("/live-frame.txt", HTTP_GET, sendLiveFrameText);

  server.onNotFound([](AsyncWebServerRequest *request){
    if (request->method() == HTTP_OPTIONS) {
      request->send(204);
      return;
    }
    Serial.printf("404: %s %s\n", request->methodToString(), request->url().c_str());
    request->send(404, "text/plain", "Not Found");
  });
}

void WebServer::setupAPIRoutes() {
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到状态查询请求");
    StaticJsonDocument<300> doc;
    const char* mode = "";
    if (DisplayManager::currentBusinessModeTag.length() > 0) {
      mode = DisplayManager::currentBusinessModeTag.c_str();
    } else if (DisplayManager::currentMode == MODE_CLOCK) {
      mode = "clock";
    } else if (DisplayManager::currentMode == MODE_CANVAS) {
      mode = "canvas";
    } else if (DisplayManager::currentMode == MODE_ANIMATION) {
      mode = "animation";
    } else if (DisplayManager::currentMode == MODE_THEME) {
      mode = "theme";
    } else if (DisplayManager::currentMode == MODE_TRANSFERRING) {
      mode = "transferring";
    }

    doc["status"] = "ok";
    doc["ip"] = WiFiManager::getDeviceIP();
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["device_type"] = DEVICE_TYPE;
    doc["width"] = DisplayManager::PANEL_RES_X;
    doc["height"] = DisplayManager::PANEL_RES_Y;
    doc["brightness"] = DisplayManager::currentBrightness;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["mode"] = mode;
    doc["businessMode"] = DisplayManager::currentBusinessModeTag;
    if (DisplayManager::nativeEffectType == NATIVE_EFFECT_BREATH) {
      doc["effectMode"] = "breath_effect";
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_RHYTHM) {
      doc["effectMode"] = "rhythm_effect";
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_EYES) {
      doc["effectMode"] = "eyes";
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_AMBIENT) {
      doc["effectMode"] = "ambient_effect";
    } else {
      doc["effectMode"] = "none";
    }
    doc["effectPreset"] = ambientPresetToString(DisplayManager::ambientEffectConfig.preset);
    doc["effectSpeed"] = DisplayManager::ambientEffectConfig.speed;
    doc["effectIntensity"] = DisplayManager::ambientEffectConfig.intensity;
    doc["effectLoop"] = DisplayManager::ambientEffectConfig.loop;
    doc["themeId"] = ConfigManager::themeConfig.themeId;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/upload", HTTP_POST, 
    [](AsyncWebServerRequest *request){
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if (index == 0) {
        Serial.println("开始接收图片...");
        if (imageBuffer) free(imageBuffer);
        imageBuffer = nullptr;
        imageSize = 0;
      }
      
      uint8_t* newBuffer = (uint8_t*)realloc(imageBuffer, imageSize + len);
      if (newBuffer) {
        imageBuffer = newBuffer;
        memcpy(imageBuffer + imageSize, data, len);
        imageSize += len;
      }
      
      if (final) {
        Serial.printf("接收完成，大小: %d bytes\n", imageSize);
        if (imageBuffer != nullptr && imageSize > 0) {
          int width = request->hasParam("width") ? request->getParam("width")->value().toInt() : DisplayManager::PANEL_RES_X;
          int height = request->hasParam("height") ? request->getParam("height")->value().toInt() : DisplayManager::PANEL_RES_Y;
          DisplayManager::displayImage(imageBuffer, imageSize, width, height);
        } else {
          Serial.println("✗ 图片缓冲区为空，无法显示");
        }
        
        if (imageBuffer) {
          free(imageBuffer);
          imageBuffer = nullptr;
        }
        imageSize = 0;
      }
    }
  );
  
  server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("value", true)) {
      int brightness = request->getParam("value", true)->value().toInt();
      if (brightness >= 0 && brightness <= 255) {
        DisplayManager::setBrightness(brightness);
        request->send(200, "application/json", "{\"status\":\"ok\",\"brightness\":" + String(brightness) + "}");
      } else {
        request->send(400, "application/json", "{\"error\":\"brightness must be 0-255\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing value parameter\"}");
    }
  });
  
  server.on("/text", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("content", true)) {
      String text = request->getParam("content", true)->value();
      int x = request->hasParam("x", true) ? request->getParam("x", true)->value().toInt() : 0;
      int y = request->hasParam("y", true) ? request->getParam("y", true)->value().toInt() : 0;
      
      DisplayManager::dma_display->clearScreen();
      DisplayManager::dma_display->setCursor(x, y);
      DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
      DisplayManager::dma_display->print(text);
      
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing content parameter\"}");
    }
  });
}

void WebServer::handleLoop() {
  // Web服务器的循环处理（如果需要）
}
