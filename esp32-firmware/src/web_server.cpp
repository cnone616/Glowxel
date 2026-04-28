#include "web_server.h"
#include "websocket_handler.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "config_manager.h"
#include "ota_manager.h"
#include "theme_renderer.h"
#include "device_mode_tag_codec.h"
#include "ambient_preset_codec.h"
#include "mode_tags.h"
#include "runtime_command_bus.h"
#include "runtime_mode_coordinator.h"
#include "runtime_status_builder.h"
#include <ctype.h>
namespace {
struct SyncHttpRequest {
  String method;
  String path;
  String query;
  String body;
};

WiFiServer gRuntimeSyncServer(80);
bool gRuntimeSyncServerStarted = false;
bool gRuntimeAsyncServerStarted = false;
portMUX_TYPE gSettingsSessionMux = portMUX_INITIALIZER_UNLOCKED;
unsigned long gSettingsSessionActiveUntil = 0;
constexpr unsigned long kSettingsSessionHoldMs = 120000;

const char* kRuntimeDeviceParamKeys[] = {
  "displayBright",
  "brightnessDay",
  "brightnessNight",
  "displayRotation",
  "swapBlueGreen",
  "swapBlueRed",
  "clkphase",
  "driver",
  "i2cSpeed",
  "E_pin",
  "nightStart",
  "nightEnd",
  "ntpServer"
};

void markSettingsSessionActivity() {
  unsigned long until = millis() + kSettingsSessionHoldMs;
  portENTER_CRITICAL(&gSettingsSessionMux);
  gSettingsSessionActiveUntil = until;
  portEXIT_CRITICAL(&gSettingsSessionMux);
}

bool isSettingsSessionActiveInternal() {
  unsigned long until = 0;
  portENTER_CRITICAL(&gSettingsSessionMux);
  until = gSettingsSessionActiveUntil;
  portEXIT_CRITICAL(&gSettingsSessionMux);
  return until != 0 && static_cast<long>(until - millis()) > 0;
}

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
  {"clockwise_pokedex", "图鉴"}
};

const AmbientPageItem kAmbientPageItems[] = {
  {"digital_rain", "Digital Rain", "字符雨风格，适合做科技感屏保"},
  {"starfield", "Starfield", "低亮星空缓慢漂移，适合夜间待机"},
  {"neon_tunnel", "Neon Tunnel", "霓虹隧道纵深感更强，动态更集中"},
  {"metablob", "Liquid Sim", "粒子液体会随重力摇摆，更接近水在容器里晃动"},
  {"falling_sand", "Falling Sand", "细颗粒往下流动，画面更安静"},
  {"sparks", "Sparks", "火花从底部向外迸发，暖色氛围更明显"},
  {"wave_pattern", "Wave Pattern", "图案化波纹持续流动，适合简洁桌搭"},
  {"surface", "Water Surface", "水面长浪和亮脊持续流动，作为水世界第一阶段板载路线"},
  {"current", "Ocean Current", "深海海流和冷色光束持续穿行，作为水世界第一阶段板载路线"},
  {"caustics", "Caustics", "海底焦散光网缓慢扫动，作为水世界第一阶段板载路线"},
  {"rain_scene", "Rain Scene", "偏真实雨幕感，整体更克制"},
  {"watermelon_plasma", "Watermelon Plasma", "高彩红绿等离子流动，画面更活"},
  {"boids", "Boids", "粒子群游，像小生物一样在屏里穿梭"},
  {"bouncing_logo", "Bouncing Logo", "九宫格徽标在屏里反弹移动"},
  {"sorting_visualizer", "Sorting Visualizer", "排序柱条不断交换重排"},
  {"clock_scene", "Clock Scene", "场景化时钟读数，适合桌面待机"},
  {"game_of_life", "Game Of Life", "经典细胞自动机不断演化"},
  {"julia_set", "Julia Set", "分形纹理持续变化"},
  {"reaction_diffusion", "Reaction Diffusion", "类似细胞生长的纹理扩散"}
};

bool parseUnsignedValue(const String& text, uint32_t minValue, uint32_t maxValue, uint32_t& outValue) {
  if (text.length() == 0) {
    return false;
  }

  for (size_t i = 0; i < text.length(); i++) {
    char ch = text[i];
    if (ch < '0' || ch > '9') {
      return false;
    }
  }

  unsigned long parsed = strtoul(text.c_str(), nullptr, 10);
  if (parsed < minValue || parsed > maxValue) {
    return false;
  }

  outValue = static_cast<uint32_t>(parsed);
  return true;
}

bool parseBooleanSwitch(const String& text, bool& outValue) {
  if (text == "0") {
    outValue = false;
    return true;
  }
  if (text == "1") {
    outValue = true;
    return true;
  }
  return false;
}

int gUploadResponseStatusCode = 200;
String gUploadResponseBody = "{\"status\":\"ok\"}";
bool gIgnoreCurrentUpload = false;

void resetUploadResponseState() {
  gUploadResponseStatusCode = 200;
  gUploadResponseBody = "{\"status\":\"ok\"}";
  gIgnoreCurrentUpload = false;
}

void setUploadErrorResponse(int statusCode, const char* message) {
  gUploadResponseStatusCode = statusCode;
  gUploadResponseBody = "{\"error\":\"" + String(message) + "\"}";
  gIgnoreCurrentUpload = true;
}

void releaseUploadBuffer() {
  if (WebServer::imageBuffer != nullptr) {
    free(WebServer::imageBuffer);
    WebServer::imageBuffer = nullptr;
  }
  WebServer::imageSize = 0;
}

bool isValidHourMinuteText(const String& text) {
  if (text.length() != 5 || text[2] != ':') {
    return false;
  }
  if (text[0] < '0' || text[0] > '9' ||
      text[1] < '0' || text[1] > '9' ||
      text[3] < '0' || text[3] > '9' ||
      text[4] < '0' || text[4] > '9') {
    return false;
  }

  int hour = (text[0] - '0') * 10 + (text[1] - '0');
  int minute = (text[3] - '0') * 10 + (text[4] - '0');
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return false;
  }
  return true;
}

const AsyncWebParameter* findDeviceParam(AsyncWebServerRequest* request, const char* key) {
  if (request->hasParam(key)) {
    return request->getParam(key);
  }
  if (request->hasParam(key, true)) {
    return request->getParam(key, true);
  }
  return nullptr;
}

String buildDeviceParamsJson() {
  StaticJsonDocument<640> doc;
  doc["displayBright"] = ConfigManager::deviceParamsConfig.displayBright;
  doc["brightnessDay"] = ConfigManager::deviceParamsConfig.brightnessDay;
  doc["brightnessNight"] = ConfigManager::deviceParamsConfig.brightnessNight;
  doc["displayRotation"] = ConfigManager::deviceParamsConfig.displayRotation;
  doc["swapBlueGreen"] = ConfigManager::deviceParamsConfig.swapBlueGreen;
  doc["swapBlueRed"] = ConfigManager::deviceParamsConfig.swapBlueRed;
  doc["clkphase"] = ConfigManager::deviceParamsConfig.clkphase;
  doc["driver"] = ConfigManager::deviceParamsConfig.driver;
  doc["i2cSpeed"] = ConfigManager::deviceParamsConfig.i2cSpeed;
  doc["E_pin"] = ConfigManager::deviceParamsConfig.E_pin;
  doc["nightStart"] = ConfigManager::deviceParamsConfig.nightStart;
  doc["nightEnd"] = ConfigManager::deviceParamsConfig.nightEnd;
  doc["ntpServer"] = ConfigManager::deviceParamsConfig.ntpServer;
  doc["wifiSsid"] = WiFiManager::getConnectedSSID();
  doc["uptime"] = millis() / 1000;
  doc["firmwareVersion"] = FIRMWARE_VERSION;

  String response;
  serializeJson(doc, response);
  return response;
}

String buildRuntimeStatusJson() {
  StaticJsonDocument<RuntimeStatusBuilder::kStatusJsonCapacity> doc;
  RuntimeStatusBuilder::fillStatus(doc, true, true);

  if (doc.overflowed()) {
    Serial.printf(
      "/status 响应 JSON 溢出: capacity=%u\n",
      RuntimeStatusBuilder::kStatusJsonCapacity
    );
  }

  String response;
  serializeJson(doc, response);
  return response;
}

void sendDeviceParamsResponse(AsyncWebServerRequest* request) {
  request->send(200, "application/json", buildDeviceParamsJson());
}

bool applyDeviceParamValue(const String& key, const String& value, String& errorMessage) {
  bool requiresRuntimeApply = false;
  uint32_t parsedNumber = 0;
  bool parsedBool = false;

  if (key == "displayBright") {
    if (!parseUnsignedValue(value, 0, 255, parsedNumber)) {
      errorMessage = "displayBright must be 0-255";
      return false;
    }
    ConfigManager::deviceParamsConfig.displayBright = static_cast<uint8_t>(parsedNumber);
    requiresRuntimeApply = true;
  } else if (key == "brightnessDay") {
    if (!parseUnsignedValue(value, 0, 255, parsedNumber)) {
      errorMessage = "brightnessDay must be 0-255";
      return false;
    }
    ConfigManager::deviceParamsConfig.brightnessDay = static_cast<uint8_t>(parsedNumber);
    requiresRuntimeApply = true;
  } else if (key == "brightnessNight") {
    if (!parseUnsignedValue(value, 0, 255, parsedNumber)) {
      errorMessage = "brightnessNight must be 0-255";
      return false;
    }
    ConfigManager::deviceParamsConfig.brightnessNight = static_cast<uint8_t>(parsedNumber);
    requiresRuntimeApply = true;
  } else if (key == "displayRotation") {
    if (!parseUnsignedValue(value, 0, 3, parsedNumber)) {
      errorMessage = "displayRotation must be 0-3";
      return false;
    }
    ConfigManager::deviceParamsConfig.displayRotation = static_cast<uint8_t>(parsedNumber);
    requiresRuntimeApply = true;
  } else if (key == "swapBlueGreen") {
    if (!parseBooleanSwitch(value, parsedBool)) {
      errorMessage = "swapBlueGreen must be 0 or 1";
      return false;
    }
    ConfigManager::deviceParamsConfig.swapBlueGreen = parsedBool;
    requiresRuntimeApply = true;
  } else if (key == "swapBlueRed") {
    if (!parseBooleanSwitch(value, parsedBool)) {
      errorMessage = "swapBlueRed must be 0 or 1";
      return false;
    }
    ConfigManager::deviceParamsConfig.swapBlueRed = parsedBool;
    requiresRuntimeApply = true;
  } else if (key == "clkphase") {
    if (!parseBooleanSwitch(value, parsedBool)) {
      errorMessage = "clkphase must be 0 or 1";
      return false;
    }
    ConfigManager::deviceParamsConfig.clkphase = parsedBool;
    requiresRuntimeApply = true;
  } else if (key == "driver") {
    if (!parseUnsignedValue(value, 0, 5, parsedNumber)) {
      errorMessage = "driver must be 0-5";
      return false;
    }
    ConfigManager::deviceParamsConfig.driver = static_cast<uint8_t>(parsedNumber);
    requiresRuntimeApply = true;
  } else if (key == "i2cSpeed") {
    if (!parseUnsignedValue(value, 8000000, 20000000, parsedNumber) ||
        (parsedNumber != 8000000 && parsedNumber != 16000000 && parsedNumber != 20000000)) {
      errorMessage = "i2cSpeed must be 8000000, 16000000 or 20000000";
      return false;
    }
    ConfigManager::deviceParamsConfig.i2cSpeed = parsedNumber;
    requiresRuntimeApply = true;
  } else if (key == "E_pin") {
    if (!parseUnsignedValue(value, 0, 32, parsedNumber)) {
      errorMessage = "E_pin must be 0-32";
      return false;
    }
    ConfigManager::deviceParamsConfig.E_pin = static_cast<uint8_t>(parsedNumber);
    requiresRuntimeApply = true;
  } else if (key == "nightStart") {
    if (!isValidHourMinuteText(value)) {
      errorMessage = "nightStart must be HH:MM";
      return false;
    }
    memset(ConfigManager::deviceParamsConfig.nightStart, 0, sizeof(ConfigManager::deviceParamsConfig.nightStart));
    value.toCharArray(ConfigManager::deviceParamsConfig.nightStart, sizeof(ConfigManager::deviceParamsConfig.nightStart));
    requiresRuntimeApply = true;
  } else if (key == "nightEnd") {
    if (!isValidHourMinuteText(value)) {
      errorMessage = "nightEnd must be HH:MM";
      return false;
    }
    memset(ConfigManager::deviceParamsConfig.nightEnd, 0, sizeof(ConfigManager::deviceParamsConfig.nightEnd));
    value.toCharArray(ConfigManager::deviceParamsConfig.nightEnd, sizeof(ConfigManager::deviceParamsConfig.nightEnd));
    requiresRuntimeApply = true;
  } else if (key == "ntpServer") {
    if (value.length() == 0 || value.length() >= (int)sizeof(ConfigManager::deviceParamsConfig.ntpServer)) {
      errorMessage = "ntpServer length is invalid";
      return false;
    }
    memset(ConfigManager::deviceParamsConfig.ntpServer, 0, sizeof(ConfigManager::deviceParamsConfig.ntpServer));
    value.toCharArray(ConfigManager::deviceParamsConfig.ntpServer, sizeof(ConfigManager::deviceParamsConfig.ntpServer));
  } else {
    errorMessage = "unsupported device param key";
    return false;
  }

  ConfigManager::saveDeviceParamsConfig();

  if (requiresRuntimeApply) {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(RuntimeCommandBus::RuntimeCommandSource::HTTP, 0);
    if (command == nullptr) {
      errorMessage = "out of memory";
      return false;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::APPLY_DEVICE_PARAMS;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      errorMessage = "device busy";
      return false;
    }
  }

  if (key == "ntpServer") {
    WiFiManager::refreshTimeSync();
  }

  return true;
}

String escapeHtml(const String& text) {
  String escaped;
  escaped.reserve(text.length() + 16);

  for (size_t i = 0; i < text.length(); i++) {
    char ch = text[i];
    if (ch == '&') {
      escaped += "&amp;";
    } else if (ch == '<') {
      escaped += "&lt;";
    } else if (ch == '>') {
      escaped += "&gt;";
    } else if (ch == '"') {
      escaped += "&quot;";
    } else if (ch == '\'') {
      escaped += "&#39;";
    } else {
      escaped += ch;
    }
  }

  return escaped;
}

int wifiSignalPercentFromRssi(int32_t rssi) {
  if (rssi <= -100) {
    return 0;
  }
  if (rssi >= -50) {
    return 100;
  }
  return (rssi + 100) * 2;
}

const char* wifiScanPhaseApiValue(WifiScanPhase phase) {
  switch (phase) {
    case WifiScanPhase::IDLE:
      return "idle";
    case WifiScanPhase::REQUESTED:
      return "requested";
    case WifiScanPhase::RUNNING:
      return "running";
    default:
      return "idle";
  }
}

void addConfigPortalNoCacheHeaders(AsyncWebServerResponse* response);
String buildWifiScanSnapshotJson();

String buildWifiScanSelectOptionsHtml() {
  String optionsHtml;
  size_t count = WiFiManager::getScannedNetworkCount();

  if (count == 0) {
    optionsHtml += R"HTML(<option value="">请先点击扫描 WiFi</option>)HTML";
    return optionsHtml;
  }

  optionsHtml += R"HTML(<option value="">请选择扫描到的 WiFi</option>)HTML";

  for (size_t i = 0; i < count; i++) {
    WiFiScanResultItem item = WiFiManager::getScannedNetwork(i);
    String ssid = item.ssid;
    if (ssid.length() == 0) {
      continue;
    }

    optionsHtml += "<option value=\"";
    optionsHtml += escapeHtml(ssid);
    optionsHtml += "\">";
    optionsHtml += escapeHtml(ssid);
    optionsHtml += " · ";
    optionsHtml += item.secure ? "加密网络" : "开放网络";
    optionsHtml += " · ";
    optionsHtml += String(wifiSignalPercentFromRssi(item.rssi));
    optionsHtml += "%</option>";
  }

  return optionsHtml;
}

String buildWifiScanStatusText() {
  WifiScanPhase phase = WiFiManager::getScanPhase();
  size_t count = WiFiManager::getScannedNetworkCount();

  if (phase == WifiScanPhase::REQUESTED || phase == WifiScanPhase::RUNNING) {
    return "正在扫描附近 WiFi，请稍候 3-6 秒。";
  }

  if (count == 0) {
    return "尚未扫描到 WiFi。请点击“扫描 WiFi”，也可以直接手动输入 SSID。";
  }

  return "已扫描到 " + String(count) + " 个 WiFi，请从下拉框选择，也可以直接手动输入。";
}

const char* buildWifiScanStatusClassName() {
  WifiScanPhase phase = WiFiManager::getScanPhase();
  if (phase == WifiScanPhase::REQUESTED || phase == WifiScanPhase::RUNNING) {
    return "scan-status status-busy";
  }
  if (WiFiManager::getScannedNetworkCount() > 0) {
    return "scan-status status-ready";
  }
  return "scan-status status-empty";
}

void sendWifiScanSnapshotResponse(AsyncWebServerRequest* request) {
  String responseBody = buildWifiScanSnapshotJson();

  AsyncWebServerResponse* response = request->beginResponse(
    200,
    "application/json; charset=utf-8",
    responseBody
  );
  addConfigPortalNoCacheHeaders(response);
  request->send(response);
}

String buildWifiScanSnapshotJson() {
  size_t count = WiFiManager::getScannedNetworkCount();
  size_t jsonCapacity = JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(count) + 24;

  for (size_t i = 0; i < count; i++) {
    WiFiScanResultItem item = WiFiManager::getScannedNetwork(i);
    jsonCapacity += JSON_OBJECT_SIZE(3);
    jsonCapacity += strlen(item.ssid) + 1;
  }

  DynamicJsonDocument doc(jsonCapacity);
  doc["scanPhase"] = wifiScanPhaseApiValue(WiFiManager::getScanPhase());
  JsonArray networks = doc.createNestedArray("networks");

  for (size_t i = 0; i < count; i++) {
    WiFiScanResultItem item = WiFiManager::getScannedNetwork(i);
    JsonObject network = networks.createNestedObject();
    network["ssid"] = item.ssid;
    network["rssi"] = item.rssi;
    network["secure"] = item.secure;
  }

  String responseBody;
  serializeJson(doc, responseBody);
  return responseBody;
}

String buildWifiConfigPortalPage() {
  String portalSsid = WiFiManager::getConfigPortalSSID();
  String portalIp = WiFiManager::getConfigPortalIP();
  String networkOptionsHtml = buildWifiScanSelectOptionsHtml();
  String scanStatusText = buildWifiScanStatusText();
  const char* scanStatusClassName = buildWifiScanStatusClassName();
  bool scanRunning = WiFiManager::isNetworkScanRunning();
  String scanButtonText = scanRunning
    ? "正在扫描..."
    : (WiFiManager::getScannedNetworkCount() > 0
    ? "重新扫描 WiFi"
    : "扫描 WiFi");

  String html;
  html.reserve(15000);
  html += R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Glowxel PixelBoard WiFi 配网</title>
)HTML";
  html += R"HTML(
  <style>
    body { margin: 0; padding: 18px; background: #f4f7fb; color: #142033; font-family: -apple-system, BlinkMacSystemFont, "PingFang SC", "Microsoft YaHei", sans-serif; }
    .wrap { max-width: 420px; margin: 0 auto; }
    .card { background: #fff; border: 1px solid #d7e3f2; border-radius: 16px; padding: 20px; box-shadow: 0 10px 24px rgba(20, 32, 51, 0.08); }
    h1 { margin: 0 0 10px; font-size: 24px; text-align: center; }
    .sub { margin: 0 0 16px; color: #5c6c84; font-size: 14px; text-align: center; line-height: 1.6; }
    .meta { margin-bottom: 16px; padding: 12px 14px; border-radius: 12px; background: #f8fbff; border: 1px solid #dce7f5; font-size: 13px; color: #50627d; }
    .meta strong { color: #142033; word-break: break-all; }
    .notice { margin-bottom: 16px; padding: 12px 14px; border-radius: 12px; background: #fff7e8; border: 1px solid #ffd89a; color: #7d5412; font-size: 13px; line-height: 1.6; }
    .section-title { margin: 18px 0 10px; font-size: 15px; font-weight: 700; }
    .scan-box { margin-bottom: 14px; }
    .scan-status { margin: 10px 0 0; padding: 12px 14px; border-radius: 12px; font-size: 13px; line-height: 1.6; border: 1px solid #dce7f5; background: #f8fbff; color: #50627d; }
    .status-busy { border-color: #bfd5ff; background: #eef4ff; color: #184fb6; }
    .status-ready { border-color: #cde9d2; background: #f3fcf5; color: #1e6b3a; }
    .status-empty { border-color: #dce7f5; background: #f8fbff; color: #50627d; }
    label { display: block; margin: 12px 0 8px; color: #50627d; font-size: 13px; font-weight: 600; }
    input, select { width: 100%; box-sizing: border-box; padding: 13px 14px; border: 1px solid #cfdced; border-radius: 12px; background: #fff; color: #142033; font-size: 14px; }
    .actions { margin-top: 14px; }
    .btn { width: 100%; box-sizing: border-box; display: block; padding: 13px 16px; border-radius: 12px; border: 1px solid #1e5eff; background: #1e5eff; color: #fff; text-align: center; text-decoration: none; font-size: 14px; font-weight: 700; }
    .btn-secondary { margin-bottom: 10px; background: #eef4ff; color: #1e5eff; }
    .btn-disabled { border-color: #d8e1ef; background: #f3f6fb; color: #7e8ea5; pointer-events: none; }
    .hint { margin-top: 14px; color: #5c6c84; font-size: 12px; line-height: 1.6; }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>Glowxel PixelBoard WiFi 配网</h1>
      <p class="sub">连上设备热点后，在这里选择你的 2.4GHz WiFi 并保存。</p>
      <div class="meta">设备热点：<strong>)HTML";
  html += escapeHtml(portalSsid);
  html += R"HTML(</strong><br>配网页地址：<strong>)HTML";
  html += escapeHtml(portalIp);
  html += R"HTML(</strong></div>
      <div class="notice">仅支持 2.4GHz WiFi。为保证系统登录页稳定打开，设备不会在热点刚启动时立刻扫描，请手动点一次扫描。</div>
      <div class="section-title">附近 WiFi</div>
      <div class="scan-box">
        <label for="ssid-select">扫描结果</label>
        <select id="ssid-select">)HTML";
  html += networkOptionsHtml;
  html += R"HTML(</select>
        <p class=")HTML";
  html += scanStatusClassName;
  html += R"HTML(" id="scan-status">)HTML";
  html += escapeHtml(scanStatusText);
  html += R"HTML(</p>
      </div>
      <form method="post" action="/configure-wifi">
        <label for="ssid-input">WiFi 名称 (SSID)</label>
        <input id="ssid-input" name="ssid" maxlength="32" autocomplete="off" required placeholder="输入 2.4GHz WiFi 名称">
        <label for="password-input">WiFi 密码</label>
        <input id="password-input" name="password" maxlength="64" type="password" autocomplete="off" placeholder="输入 WiFi 密码，开放网络可留空">
        <div class="actions">
          <button class="btn btn-secondary )HTML";
  if (scanRunning) {
    html += R"HTML(btn-disabled)HTML";
  }
  html += R"HTML(" id="scan-btn" type="button")HTML";
  if (scanRunning) {
    html += R"HTML( disabled)HTML";
  }
  html += R"HTML(>)HTML";
  html += scanButtonText;
  html += R"HTML(</button>
          <button class="btn" type="submit">保存并连接</button>
        </div>
      </form>
      <p class="hint">保存后设备会自动重启。如果密码错误，设备会重新回到热点配网模式，你可以再次连接 <code>)HTML";
  html += escapeHtml(portalSsid);
  html += R"HTML(</code> 重新配置。</p>
    </div>
  </div>
  <script>
    (function () {
      var scanBtn = document.getElementById("scan-btn");
      var scanStatus = document.getElementById("scan-status");
      var ssidSelect = document.getElementById("ssid-select");
      var ssidInput = document.getElementById("ssid-input");
      var scanPollTimer = 0;

      if (scanBtn == null || scanStatus == null || ssidSelect == null || ssidInput == null) {
        return;
      }

      function clearScanPollTimer() {
        if (scanPollTimer !== 0) {
          window.clearTimeout(scanPollTimer);
          scanPollTimer = 0;
        }
      }

      function setScanButtonState(scanPhase, networkCount) {
        var busy = scanPhase === "requested" || scanPhase === "running";
        scanBtn.disabled = busy;
        if (busy) {
          scanBtn.classList.add("btn-disabled");
          scanBtn.textContent = "正在扫描...";
          return;
        }

        scanBtn.classList.remove("btn-disabled");
        if (networkCount > 0) {
          scanBtn.textContent = "重新扫描 WiFi";
          return;
        }

        scanBtn.textContent = "扫描 WiFi";
      }

      function setScanStatus(scanPhase, networkCount) {
        scanStatus.className = "scan-status";

        if (scanPhase === "requested" || scanPhase === "running") {
          scanStatus.classList.add("status-busy");
          scanStatus.textContent = "正在扫描附近 WiFi，请稍候 3-6 秒。";
          return;
        }

        if (networkCount > 0) {
          scanStatus.classList.add("status-ready");
          scanStatus.textContent = "已扫描到 " + networkCount + " 个 WiFi，请从下拉框选择，也可以直接手动输入。";
          return;
        }

        scanStatus.classList.add("status-empty");
        scanStatus.textContent = "尚未扫描到 WiFi。请点击“扫描 WiFi”，也可以直接手动输入 SSID。";
      }

      function renderNetworkOptions(networks) {
        var currentInputValue = ssidInput.value;
        var placeholderText = "请先点击扫描 WiFi";

        if (networks.length > 0) {
          placeholderText = "请选择扫描到的 WiFi";
        }

        while (ssidSelect.options.length > 0) {
          ssidSelect.remove(0);
        }

        var placeholderOption = document.createElement("option");
        placeholderOption.value = "";
        placeholderOption.textContent = placeholderText;
        ssidSelect.appendChild(placeholderOption);

        for (var i = 0; i < networks.length; i++) {
          var network = networks[i];
          var option = document.createElement("option");
          option.value = network.ssid;
          option.textContent = network.ssid + " · " + (network.secure ? "加密网络" : "开放网络") + " · " + network.rssi + " dBm";
          if (currentInputValue === network.ssid) {
            option.selected = true;
          }
          ssidSelect.appendChild(option);
        }
      }

      function scheduleScanPoll() {
        clearScanPollTimer();
        scanPollTimer = window.setTimeout(function () {
          fetchScanSnapshot("GET");
        }, 1000);
      }

      function applyScanSnapshot(snapshot) {
        if (snapshot == null || typeof snapshot !== "object") {
          return;
        }
        if (snapshot.scanPhase !== "idle" &&
            snapshot.scanPhase !== "requested" &&
            snapshot.scanPhase !== "running") {
          return;
        }
        if (!Array.isArray(snapshot.networks)) {
          return;
        }

        renderNetworkOptions(snapshot.networks);
        setScanStatus(snapshot.scanPhase, snapshot.networks.length);
        setScanButtonState(snapshot.scanPhase, snapshot.networks.length);

        if (snapshot.scanPhase === "requested" || snapshot.scanPhase === "running") {
          scheduleScanPoll();
          return;
        }

        clearScanPollTimer();
      }

      function fetchScanSnapshot(method) {
        fetch("/scan-wifi", {
          method: method,
          cache: "no-store"
        })
          .then(function (response) {
            if (!response.ok) {
              throw new Error("scan request failed");
            }
            return response.json();
          })
          .then(function (snapshot) {
            applyScanSnapshot(snapshot);
          })
          .catch(function () {
            clearScanPollTimer();
            scanStatus.className = "scan-status status-empty";
            scanStatus.textContent = "扫描状态获取失败，请重试。";
            setScanButtonState("idle", ssidSelect.options.length > 1 ? ssidSelect.options.length - 1 : 0);
          });
      }

      ssidSelect.addEventListener("change", function () {
        if (ssidSelect.value.length > 0) {
          ssidInput.value = ssidSelect.value;
          ssidInput.focus();
        }
      });

      ssidInput.addEventListener("input", function () {
        var matchedIndex = 0;
        for (var i = 1; i < ssidSelect.options.length; i++) {
          if (ssidSelect.options[i].value === ssidInput.value) {
            matchedIndex = i;
            break;
          }
        }
        ssidSelect.selectedIndex = matchedIndex;
      });

      scanBtn.addEventListener("click", function () {
        if (scanBtn.disabled) {
          return;
        }
        fetchScanSnapshot("POST");
      });

      fetchScanSnapshot("GET");
      if (scanBtn.disabled) {
        scheduleScanPoll();
      }
    })();
  </script>
</body>
</html>)HTML";

  return html;
}

String buildWifiConfigSuccessPage(const String& ssid) {
  String html;
  html.reserve(5200);
  html += R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
  <title>Glowxel PixelBoard WiFi 配置已保存</title>
  <style>
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 18px;
      font-family: "PingFang SC", "Microsoft YaHei", sans-serif;
      background: #eef4fb;
      color: #142033;
    }
    .card {
      width: min(100%, 420px);
      padding: 24px 22px;
      border-radius: 20px;
      background: #ffffff;
      border: 1px solid #d7e3f2;
      box-shadow: 0 16px 40px rgba(25, 45, 76, 0.10);
      text-align: center;
    }
    .badge {
      width: 62px;
      height: 62px;
      border-radius: 18px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      margin: 0 auto 16px;
      background: #e9fbef;
      color: #1f9f4a;
      font-size: 28px;
      font-weight: 700;
    }
    h1 {
      margin: 0 0 10px;
      font-size: 22px;
    }
    p {
      margin: 0;
      color: #50627d;
      font-size: 14px;
      line-height: 1.7;
    }
    .meta {
      margin-top: 16px;
      padding: 14px;
      border-radius: 16px;
      border: 1px solid #dce7f5;
      background: #f6f9fd;
      color: #50627d;
      line-height: 1.8;
      font-size: 14px;
    }
    .meta strong {
      color: #142033;
      word-break: break-all;
    }
  </style>
</head>
<body>
  <div class="card">
    <div class="badge">✓</div>
    <h1>WiFi 配置已保存</h1>
    <p>设备正在重启并连接你刚才填写的网络。重启完成后，请回到设备控制页并输入新的设备 IP 地址进行连接。</p>
    <div class="meta">目标网络：<strong>)HTML";
  html += escapeHtml(ssid);
  html += R"HTML(</strong><br>注意：设备只支持 2.4GHz WiFi。如果网络不可用或密码错误，设备会重新回到热点配网模式。</div>
  </div>
</body>
</html>)HTML";

  return html;
}

void addConfigPortalNoCacheHeaders(AsyncWebServerResponse* response) {
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  response->addHeader("Connection", "close");
}

void redirectToConfigPortalRoot(AsyncWebServerRequest* request) {
  String location = "http://" + WiFiManager::getConfigPortalIP() + "/";
  Serial.printf("[WiFi Portal] 重定向到根页: %s %s host=%s -> %s\n",
                request->methodToString(),
                request->url().c_str(),
                request->host().c_str(),
                location.c_str());
  AsyncWebServerResponse* response = request->beginResponse(
    302,
    "text/plain; charset=utf-8",
    ""
  );
  response->addHeader("Location", location);
  addConfigPortalNoCacheHeaders(response);
  request->send(response);
}

void sendConfigPortalPage(AsyncWebServerRequest* request) {
  AsyncWebServerResponse* response = request->beginResponse(
    200,
    "text/html; charset=utf-8",
    buildWifiConfigPortalPage()
  );
  addConfigPortalNoCacheHeaders(response);
  request->send(response);
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
  <title>Glowxel PixelBoard 主题模式</title>
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
  <title>Glowxel PixelBoard 板载特效</title>
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
        <div id="intensity-row" class="control-row">
          <label><span>强度</span><strong id="intensity-value">72</strong></label>
          <input id="intensity-input" type="range" min="10" max="100" step="1">
        </div>
        <div id="density-row" class="control-row">
          <label><span>密度</span><strong id="density-value">72</strong></label>
          <input id="density-input" type="range" min="10" max="100" step="1">
        </div>
        <div id="color-row" class="control-row">
          <label><span>颜色</span><strong id="color-value">#64C8FF</strong></label>
          <input id="color-input" type="color">
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
  html += AmbientPresetCodec::toString(DisplayManager::ambientEffectConfig.preset);
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
      density: )HTML";
  html += String(DisplayManager::ambientEffectConfig.density);
  html += R"HTML(,
      color: "#)HTML";
  if (DisplayManager::ambientEffectConfig.colorR < 16) html += "0";
  html += String(DisplayManager::ambientEffectConfig.colorR, HEX);
  if (DisplayManager::ambientEffectConfig.colorG < 16) html += "0";
  html += String(DisplayManager::ambientEffectConfig.colorG, HEX);
  if (DisplayManager::ambientEffectConfig.colorB < 16) html += "0";
  html += String(DisplayManager::ambientEffectConfig.colorB, HEX);
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
    const densityInput = document.getElementById("density-input");
    const colorInput = document.getElementById("color-input");
    const loopInput = document.getElementById("loop-input");
    const speedValueEl = document.getElementById("speed-value");
    const intensityValueEl = document.getElementById("intensity-value");
    const densityValueEl = document.getElementById("density-value");
    const colorValueEl = document.getElementById("color-value");
    const intensityRow = document.getElementById("intensity-row");
    const densityRow = document.getElementById("density-row");
    const colorRow = document.getElementById("color-row");

    function isRainPreset(presetId) {
      return presetId === "rain_scene";
    }

    function rgbToHex(color) {
      const toHex = (value) => {
        const safe = Math.max(0, Math.min(255, Number(value) || 0));
        return safe.toString(16).padStart(2, "0");
      };
      return "#" + toHex(color.r) + toHex(color.g) + toHex(color.b);
    }

    function hexToRgb(hex) {
      const match = /^#?([0-9a-fA-F]{6})$/.exec(hex);
      if (!match) {
        return { r: 100, g: 200, b: 255 };
      }
      return {
        r: parseInt(match[1].slice(0, 2), 16),
        g: parseInt(match[1].slice(2, 4), 16),
        b: parseInt(match[1].slice(4, 6), 16)
      };
    }

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
        if (typeof data.effectDensity === "number") {
          state.density = data.effectDensity;
        }
        if (data.effectColor && typeof data.effectColor === "object") {
          state.color = rgbToHex(data.effectColor);
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
      densityInput.value = state.density;
      colorInput.value = state.color;
      loopInput.checked = !!state.loop;
      speedValueEl.textContent = state.speed;
      intensityValueEl.textContent = state.intensity;
      densityValueEl.textContent = state.density;
      colorValueEl.textContent = String(state.color || "").toUpperCase();
      currentNameEl.textContent = getPresetName(activePresetId);
      const rainPreset = isRainPreset(activePresetId);
      intensityRow.style.display = rainPreset ? "none" : "";
      densityRow.style.display = rainPreset ? "" : "none";
      colorRow.style.display = rainPreset ? "" : "none";
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
      if (isRainPreset(activePresetId)) {
        sendJson({
          cmd: "set_ambient_effect",
          preset: activePresetId,
          speed: Number(speedInput.value),
          density: Number(densityInput.value),
          color: hexToRgb(colorInput.value),
          loop: !!loopInput.checked
        });
        return;
      }

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
    densityInput.addEventListener("input", () => {
      state.density = Number(densityInput.value);
      syncControls();
    });
    colorInput.addEventListener("input", () => {
      state.color = colorInput.value;
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
  html.reserve(30000);
  html += R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
  <title>Glowxel PixelBoard 运行态板载设置</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #ffffff;
      --ink: #111111;
      --muted: #4f4f4f;
      --line: #111111;
      --panel: #ffffff;
      --panel-soft: #f6f7fb;
      --yellow: #ffd23f;
      --blue: #74b9ff;
      --mint: #88d498;
      --peach: #ffa552;
      --rose: #ff8fa3;
      --shadow: 6px 6px 0 #111111;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "SF Pro Display", "PingFang SC", "Microsoft YaHei", sans-serif;
      background:
        linear-gradient(180deg, rgba(255, 210, 63, 0.16) 0, rgba(255, 210, 63, 0.16) 120px, transparent 120px),
        linear-gradient(90deg, rgba(17, 17, 17, 0.04) 1px, transparent 1px),
        linear-gradient(180deg, rgba(17, 17, 17, 0.04) 1px, transparent 1px),
        var(--bg);
      background-size: auto, 20px 20px, 20px 20px, auto;
      color: var(--ink);
    }
    a, button, input, select { font: inherit; }
    button {
      cursor: pointer;
      border: 3px solid var(--line);
      background: var(--panel);
      color: var(--ink);
      box-shadow: 4px 4px 0 #111111;
      border-radius: 0;
      transition: background 0.15s ease;
    }
    button:hover { background: #f5f5f5; }
    .page {
      min-height: 100vh;
      padding: 18px;
      display: grid;
      gap: 18px;
      grid-template-columns: minmax(0, 1.45fr) minmax(320px, 430px);
    }
    .main-column,
    .preview-column {
      display: grid;
      gap: 18px;
      align-content: start;
    }
    .preview-column {
      position: sticky;
      top: 18px;
      height: max-content;
    }
    .hero-card,
    .section-card,
    .preview-card {
      background: var(--panel);
      border: 3px solid var(--line);
      box-shadow: var(--shadow);
      padding: 18px;
    }
    .hero-top,
    .section-head {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      gap: 14px;
      flex-wrap: wrap;
    }
    .hero-title,
    .section-title {
      margin: 0;
      font-size: 28px;
      font-weight: 700;
    }
    .section-title {
      font-size: 22px;
    }
    .hero-sub,
    .section-meta,
    .field-desc,
    .mode-desc,
    .legend-note {
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
    }
    .hero-sub {
      margin: 10px 0 0;
      max-width: 58ch;
      font-size: 14px;
    }
    .hero-actions,
    .inline-actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      align-items: center;
    }
    .ghost-link {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-height: 44px;
      padding: 0 14px;
      border: 3px solid var(--line);
      background: var(--panel);
      color: var(--ink);
      text-decoration: none;
      box-shadow: 4px 4px 0 #111111;
    }
    .status-pill {
      margin-top: 14px;
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 9px 12px;
      border: 3px solid var(--line);
      background: #fff3c4;
      font-size: 13px;
      font-weight: 700;
    }
    .status-pill.connected { background: #eaf9ee; }
    .status-dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: #ffa552;
    }
    .status-pill.connected .status-dot { background: #2fbf71; }
    .feedback-card {
      margin-top: 16px;
      padding: 14px;
      border: 3px solid var(--line);
      background: #f5f6fa;
      font-size: 13px;
      line-height: 1.6;
    }
    .feedback-card[data-level="ok"] { background: #eaf9ee; }
    .feedback-card[data-level="warn"] { background: #fff2d7; }
    .feedback-card[data-level="error"] { background: #ffe4e4; }
    .summary-grid,
    .mode-grid,
    .theme-grid,
    .preset-grid,
    .layout-two {
      display: grid;
      gap: 12px;
    }
    .summary-grid { margin-top: 18px; grid-template-columns: repeat(4, minmax(0, 1fr)); }
    .mode-grid { grid-template-columns: repeat(3, minmax(0, 1fr)); }
    .theme-grid,
    .preset-grid,
    .layout-two { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    .summary-card,
    .field-card,
    .switch-row,
    .meta-item {
      border: 3px solid var(--line);
      background: var(--panel-soft);
      padding: 12px;
    }
    .summary-label {
      display: block;
      margin-bottom: 6px;
      font-size: 12px;
      color: var(--muted);
    }
    .summary-value,
    .field-title,
    .mode-name {
      font-size: 15px;
      font-weight: 700;
      line-height: 1.5;
    }
    .field-title { font-size: 15px; }
    .field-head {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      align-items: flex-start;
      margin-bottom: 10px;
    }
    .field-value {
      min-width: 54px;
      padding: 4px 8px;
      border: 3px solid var(--line);
      background: #ffffff;
      text-align: center;
      font-size: 13px;
      font-weight: 700;
    }
    .select-input,
    .text-input,
    .number-input,
    .time-input,
    .color-input {
      width: 100%;
      min-height: 44px;
      padding: 10px 12px;
      border: 3px solid var(--line);
      background: #ffffff;
      color: var(--ink);
      border-radius: 0;
    }
    input[type="color"] { padding: 6px; }
    input[type="range"] {
      width: 100%;
      appearance: none;
      background: transparent;
      margin: 0;
    }
    input[type="range"]::-webkit-slider-runnable-track {
      height: 10px;
      border: 3px solid var(--line);
      background: #ffffff;
    }
    input[type="range"]::-webkit-slider-thumb {
      appearance: none;
      width: 18px;
      height: 18px;
      margin-top: -7px;
      border: 3px solid var(--line);
      background: var(--yellow);
    }
    .switch-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 12px;
      background: #ffffff;
    }
    .toggle-input { width: 20px; height: 20px; }
    .mode-card,
    .theme-card,
    .preset-card {
      padding: 14px;
      text-align: left;
      background: #ffffff;
    }
    .mode-card.active,
    .theme-card.active,
    .preset-card.active { background: #fff2b0; }
    .mode-accent {
      width: 40px;
      height: 12px;
      margin-bottom: 10px;
      border: 3px solid var(--line);
    }
    .tone-yellow { background: var(--yellow); }
    .tone-blue { background: var(--blue); }
    .tone-mint { background: var(--mint); }
    .tone-peach { background: var(--peach); }
    .tone-rose { background: var(--rose); }
    .legend-note { background: #f5f6fa; }
    .mini-btn,
    .apply-btn,
    .refresh-btn {
      min-height: 44px;
      padding: 0 14px;
      font-size: 13px;
      font-weight: 700;
    }
    .apply-btn { background: var(--yellow); }
    .refresh-btn { background: var(--blue); }
    .preview-stage {
      position: relative;
      width: 100%;
      max-width: 394px;
      aspect-ratio: 1;
      margin: 0 auto;
      background: #000000;
      border: 3px solid var(--line);
      box-shadow: var(--shadow);
      overflow: hidden;
    }
    #preview {
      width: 100%;
      height: 100%;
      display: block;
      image-rendering: pixelated;
      background: #000000;
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
    .preview-label {
      margin-top: 16px;
      font-size: 18px;
      font-weight: 700;
      text-align: center;
    }
    .preview-sub {
      margin-top: 6px;
      color: var(--muted);
      font-size: 13px;
      text-align: center;
    }
    .meta-list {
      margin-top: 16px;
      display: grid;
      gap: 10px;
    }
    .meta-item {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      background: #ffffff;
      font-size: 13px;
      line-height: 1.5;
      color: var(--muted);
    }
    .meta-item strong {
      color: var(--ink);
      text-align: right;
      word-break: break-word;
    }
    @media (max-width: 1180px) {
      .page { grid-template-columns: 1fr; }
      .preview-column { position: static; }
    }
    @media (max-width: 860px) {
      .summary-grid,
      .mode-grid,
      .theme-grid,
      .preset-grid,
      .layout-two { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    }
    @media (max-width: 640px) {
      .page { padding: 14px; }
      .summary-grid,
      .mode-grid,
      .theme-grid,
      .preset-grid,
      .layout-two { grid-template-columns: 1fr; }
      .hero-title { font-size: 26px; }
    }
  </style>
</head>
<body>
  <div class="page">
    <div class="main-column">
      <section class="hero-card">
        <div class="hero-top">
          <div>
            <h1 class="hero-title">运行态板载设置</h1>
            <p class="hero-sub">热点配网仍然走 <strong>192.168.4.1</strong>；这里是设备联网成功后的运行态设置页。当前页只复用 <code>/get</code>、<code>/set</code>、<code>/status</code>、<code>/brightness</code> 和 <code>/ws</code>，不新增协议字段。</p>
          </div>
          <div class="hero-actions">
            <a class="ghost-link" href="/themes">主题总览</a>
            <a class="ghost-link" href="/effects">场景总览</a>
            <a class="ghost-link" href="/status" target="_blank" rel="noopener">状态 JSON</a>
          </div>
        </div>
        <div id="runtime-status" class="status-pill">
          <span class="status-dot"></span>
          <span id="runtime-status-text">正在连接设备并同步运行状态...</span>
        </div>
        <div id="feedback-card" class="feedback-card" data-level="neutral">正在准备板载设置页...</div>
        <div class="summary-grid">
          <div class="summary-card"><span class="summary-label">设备 IP</span><span id="summary-ip" class="summary-value">--</span></div>
          <div class="summary-card"><span class="summary-label">已连 WiFi</span><span id="summary-ssid" class="summary-value">--</span></div>
          <div class="summary-card"><span class="summary-label">固件版本</span><span id="summary-firmware" class="summary-value">--</span></div>
          <div class="summary-card"><span class="summary-label">运行时长</span><span id="summary-uptime" class="summary-value">--</span></div>
        </div>
      </section>

      <section class="section-card">
        <div class="section-head">
          <div>
            <h2 class="section-title">模式切换</h2>
            <div class="section-meta">这里收口当前可直接通过 <code>set_mode</code> 进入的运行态模式。</div>
          </div>
          <button id="refresh-all-btn" class="refresh-btn" type="button">刷新状态</button>
        </div>
        <div id="mode-grid" class="mode-grid"></div>
      </section>

      <section class="section-card">
        <div class="section-head">
          <div>
            <h2 class="section-title">设备参数</h2>
            <div class="section-meta">严格只使用既有字段。亮度上限继续按 178 控制，避免供电过高。</div>
          </div>
        </div>
        <div class="layout-two">
          <div class="field-card">
            <div class="field-head">
              <div>
                <div class="field-title">显示亮度</div>
                <div class="field-desc">立即生效，同时更新设备当前显示亮度。</div>
              </div>
              <div id="display-bright-value" class="field-value">--</div>
            </div>
            <input id="display-bright-input" type="range" min="0" max="178" step="1">
          </div>
          <div class="field-card">
            <div class="field-head">
              <div>
                <div class="field-title">显示方向</div>
                <div class="field-desc">屏幕旋转方向。</div>
              </div>
            </div>
            <select id="rotation-select" class="select-input">
              <option value="0">0°</option>
              <option value="1">90°</option>
              <option value="2">180°</option>
              <option value="3">270°</option>
            </select>
          </div>
          <div class="field-card">
            <div class="field-head">
              <div>
                <div class="field-title">日间亮度</div>
                <div class="field-desc">白天自动亮度。</div>
              </div>
              <div id="brightness-day-value" class="field-value">--</div>
            </div>
            <input id="brightness-day-input" type="range" min="0" max="178" step="1">
          </div>
          <div class="field-card">
            <div class="field-head">
              <div>
                <div class="field-title">夜间亮度</div>
                <div class="field-desc">夜间自动亮度。</div>
              </div>
              <div id="brightness-night-value" class="field-value">--</div>
            </div>
            <input id="brightness-night-input" type="range" min="0" max="178" step="1">
          </div>
          <div class="field-card">
            <div class="field-title">夜间时段</div>
            <div class="field-desc">当日夜亮度不同，设备会按时间自动切换。</div>
            <div class="inline-actions">
              <input id="night-start-input" class="time-input" type="time">
              <input id="night-end-input" class="time-input" type="time">
            </div>
          </div>
          <div class="field-card">
            <div class="field-title">驱动芯片</div>
            <div class="field-desc">按设备当前 driver 参数回显。</div>
            <select id="driver-select" class="select-input">
              <option value="0">SHIFTREG</option>
              <option value="1">FM6124</option>
              <option value="2">FM6126A</option>
              <option value="3">ICN2038S</option>
              <option value="4">MBI5124</option>
              <option value="5">DP3246</option>
            </select>
          </div>
          <div class="switch-row">
            <div>
              <div class="field-title">swapBlueGreen</div>
              <div class="field-desc">蓝绿通道互换。</div>
            </div>
            <input id="swap-blue-green-input" class="toggle-input" type="checkbox">
          </div>
          <div class="switch-row">
            <div>
              <div class="field-title">swapBlueRed</div>
              <div class="field-desc">蓝红通道互换。</div>
            </div>
            <input id="swap-blue-red-input" class="toggle-input" type="checkbox">
          </div>
          <div class="switch-row">
            <div>
              <div class="field-title">clkphase</div>
              <div class="field-desc">残影、分裂、左右错位时切换。</div>
            </div>
            <input id="clkphase-input" class="toggle-input" type="checkbox">
          </div>
          <div class="field-card">
            <div class="field-title">I2S 速度</div>
            <div class="field-desc">按设备当前 i2cSpeed 参数回显。</div>
            <select id="speed-select" class="select-input">
              <option value="8000000">HZ_8M</option>
              <option value="16000000">HZ_16M</option>
              <option value="20000000">HZ_20M</option>
            </select>
          </div>
          <div class="field-card">
            <div class="field-title">E_pin</div>
            <div class="field-desc">面板 E 引脚值。</div>
            <div class="inline-actions">
              <input id="e-pin-input" class="number-input" type="number" min="0" max="32">
              <button id="save-e-pin-btn" class="mini-btn" type="button">保存</button>
            </div>
          </div>
          <div class="field-card">
            <div class="field-title">时间服务器</div>
            <div class="field-desc">可直接选国内预设，也可以写自定义地址。</div>
            <select id="ntp-preset-select" class="select-input">
              <option value="ntp2.aliyun.com">阿里云 2</option>
              <option value="ntp.aliyun.com">阿里云</option>
              <option value="ntp.tencent.com">腾讯云</option>
              <option value="ntp.ntsc.ac.cn">国家授时中心</option>
              <option value="__custom__">自定义</option>
            </select>
            <div class="inline-actions">
              <input id="ntp-server-input" class="text-input" type="text" maxlength="63" placeholder="例如 ntp2.aliyun.com">
              <button id="save-ntp-btn" class="mini-btn" type="button">保存</button>
            </div>
          </div>
        </div>
      </section>

      <section class="section-card">
        <div class="section-head">
          <div>
            <h2 class="section-title">主题设置</h2>
            <div class="section-meta">点击即下发 <code>set_theme_config</code>，并切到主题模式。</div>
          </div>
        </div>
        <div id="theme-grid" class="theme-grid"></div>
      </section>

      <section class="section-card">
        <div class="section-head">
          <div>
            <h2 class="section-title">像素场景集</h2>
            <div class="section-meta">这里保留的是 <code>scene preset</code>，和已下线的独立看板模式不是一回事。</div>
          </div>
          <button id="apply-ambient-btn" class="apply-btn" type="button">应用当前场景</button>
        </div>
        <div id="preset-grid" class="preset-grid"></div>
        <div class="layout-two">
          <div class="field-card">
            <div class="field-head">
              <div>
                <div class="field-title">速度</div>
                <div class="field-desc">所有场景都会使用这个速度值。</div>
              </div>
              <div id="effect-speed-value" class="field-value">--</div>
            </div>
            <input id="effect-speed-input" type="range" min="1" max="10" step="1">
          </div>
          <div class="field-card" id="effect-intensity-row">
            <div class="field-head">
              <div>
                <div class="field-title">强度</div>
                <div class="field-desc">非雨幕场景使用。</div>
              </div>
              <div id="effect-intensity-value" class="field-value">--</div>
            </div>
            <input id="effect-intensity-input" type="range" min="10" max="100" step="1">
          </div>
          <div class="field-card" id="effect-density-row">
            <div class="field-head">
              <div>
                <div class="field-title">密度</div>
                <div class="field-desc">雨幕场景使用。</div>
              </div>
              <div id="effect-density-value" class="field-value">--</div>
            </div>
            <input id="effect-density-input" type="range" min="10" max="100" step="1">
          </div>
          <div class="field-card" id="effect-color-row">
            <div class="field-head">
              <div>
                <div class="field-title">颜色</div>
                <div class="field-desc">雨幕场景使用。</div>
              </div>
              <div id="effect-color-value" class="field-value">--</div>
            </div>
            <input id="effect-color-input" class="color-input" type="color">
          </div>
        </div>
        <div class="switch-row">
          <div>
            <div class="field-title">loop</div>
            <div class="field-desc">持续循环当前场景。</div>
          </div>
          <input id="effect-loop-input" class="toggle-input" type="checkbox">
        </div>
      </section>
    </div>

    <aside class="preview-column">
      <section class="preview-card">
        <div class="section-head">
          <div>
            <h2 class="section-title">实时预览</h2>
            <div class="section-meta">读取设备当前真实 64×64 帧，不另外画假预览。</div>
          </div>
          <button id="refresh-frame-btn" class="refresh-btn" type="button">刷新预览</button>
        </div>
        <div class="preview-stage">
          <canvas id="preview" width="64" height="64"></canvas>
          <div class="screen-grid"></div>
        </div>
        <div id="preview-business-mode" class="preview-label">--</div>
        <div class="preview-sub">当前业务模式</div>
        <div class="meta-list">
          <div class="meta-item"><span>顶层 mode</span><strong id="meta-mode">--</strong></div>
          <div class="meta-item"><span>businessMode</span><strong id="meta-business-mode">--</strong></div>
          <div class="meta-item"><span>themeId</span><strong id="meta-theme-id">--</strong></div>
          <div class="meta-item"><span>effectMode</span><strong id="meta-effect-mode">--</strong></div>
          <div class="meta-item"><span>effectPreset</span><strong id="meta-effect-preset">--</strong></div>
          <div class="meta-item"><span>当前亮度</span><strong id="meta-brightness">--</strong></div>
        </div>
      </section>
    </aside>
  </div>

  <script>
    const THEMES = )HTML";
  html += themesJson;
  html += R"HTML(;
    const PRESETS = )HTML";
  html += presetsJson;
  html += R"HTML(;
    const MODE_OPTIONS = [
      { id: "clock", name: "静态时钟", desc: "回到基础时钟显示。", tone: "yellow" },
      { id: "theme", name: "主题模式", desc: "应用板载主题库。", tone: "blue" },
      { id: "animation", name: "动态时钟", desc: "继续当前动画时钟。", tone: "mint" },
      { id: "eyes", name: "桌面宠物", desc: "进入桌面宠物运行态。", tone: "rose" },
      { id: "ambient_effect", name: "氛围特效", desc: "切到氛围特效底座。", tone: "peach" },
      { id: "led_matrix_showcase", name: "像素场景集", desc: "切到场景 preset 底座。", tone: "mint" },
      { id: "text_display", name: "文本看板", desc: "使用当前文本看板配置。", tone: "blue" },
      { id: "planet_screensaver", name: "星球屏保", desc: "使用当前星球屏保配置。", tone: "yellow" },
      { id: "breath_effect", name: "呼吸效果", desc: "低频氛围呼吸。", tone: "yellow" },
      { id: "rhythm_effect", name: "节奏效果", desc: "进入节奏灯效。", tone: "blue" },
      { id: "gif_player", name: "GIF 播放器", desc: "继续当前 GIF 播放。", tone: "mint" }
    ];

    const state = {
      ws: null,
      wsConnected: false,
      deviceParams: null,
      runtimeStatus: null,
      ambientDirty: false,
      ambientDraft: {
        preset: ")HTML";
  html += "\"";
  html += AmbientPresetCodec::toString(DisplayManager::ambientEffectConfig.preset);
  html += "\"";
  html += R"HTML(,
        speed: )HTML";
  html += String(DisplayManager::ambientEffectConfig.speed);
  html += R"HTML(,
        intensity: )HTML";
  html += String(DisplayManager::ambientEffectConfig.intensity);
  html += R"HTML(,
        density: )HTML";
  html += String(DisplayManager::ambientEffectConfig.density);
  html += R"HTML(,
        color: "#)HTML";
  if (DisplayManager::ambientEffectConfig.colorR < 16) html += "0";
  html += String(DisplayManager::ambientEffectConfig.colorR, HEX);
  if (DisplayManager::ambientEffectConfig.colorG < 16) html += "0";
  html += String(DisplayManager::ambientEffectConfig.colorG, HEX);
  if (DisplayManager::ambientEffectConfig.colorB < 16) html += "0";
  html += String(DisplayManager::ambientEffectConfig.colorB, HEX);
  html += R"HTML(",
        loop: )HTML";
  html += DisplayManager::ambientEffectConfig.loop ? "true" : "false";
  html += R"HTML(
      }
    };

    const els = {
      runtimeStatus: document.getElementById("runtime-status"),
      runtimeStatusText: document.getElementById("runtime-status-text"),
      feedbackCard: document.getElementById("feedback-card"),
      summaryIp: document.getElementById("summary-ip"),
      summarySsid: document.getElementById("summary-ssid"),
      summaryFirmware: document.getElementById("summary-firmware"),
      summaryUptime: document.getElementById("summary-uptime"),
      modeGrid: document.getElementById("mode-grid"),
      displayBrightInput: document.getElementById("display-bright-input"),
      displayBrightValue: document.getElementById("display-bright-value"),
      brightnessDayInput: document.getElementById("brightness-day-input"),
      brightnessDayValue: document.getElementById("brightness-day-value"),
      brightnessNightInput: document.getElementById("brightness-night-input"),
      brightnessNightValue: document.getElementById("brightness-night-value"),
      nightStartInput: document.getElementById("night-start-input"),
      nightEndInput: document.getElementById("night-end-input"),
      rotationSelect: document.getElementById("rotation-select"),
      swapBlueGreenInput: document.getElementById("swap-blue-green-input"),
      swapBlueRedInput: document.getElementById("swap-blue-red-input"),
      clkphaseInput: document.getElementById("clkphase-input"),
      driverSelect: document.getElementById("driver-select"),
      speedSelect: document.getElementById("speed-select"),
      ePinInput: document.getElementById("e-pin-input"),
      saveEPinBtn: document.getElementById("save-e-pin-btn"),
      ntpPresetSelect: document.getElementById("ntp-preset-select"),
      ntpServerInput: document.getElementById("ntp-server-input"),
      saveNtpBtn: document.getElementById("save-ntp-btn"),
      themeGrid: document.getElementById("theme-grid"),
      presetGrid: document.getElementById("preset-grid"),
      effectSpeedInput: document.getElementById("effect-speed-input"),
      effectSpeedValue: document.getElementById("effect-speed-value"),
      effectIntensityInput: document.getElementById("effect-intensity-input"),
      effectIntensityValue: document.getElementById("effect-intensity-value"),
      effectDensityInput: document.getElementById("effect-density-input"),
      effectDensityValue: document.getElementById("effect-density-value"),
      effectColorInput: document.getElementById("effect-color-input"),
      effectColorValue: document.getElementById("effect-color-value"),
      effectLoopInput: document.getElementById("effect-loop-input"),
      effectIntensityRow: document.getElementById("effect-intensity-row"),
      effectDensityRow: document.getElementById("effect-density-row"),
      effectColorRow: document.getElementById("effect-color-row"),
      applyAmbientBtn: document.getElementById("apply-ambient-btn"),
      previewBusinessMode: document.getElementById("preview-business-mode"),
      metaMode: document.getElementById("meta-mode"),
      metaBusinessMode: document.getElementById("meta-business-mode"),
      metaThemeId: document.getElementById("meta-theme-id"),
      metaEffectMode: document.getElementById("meta-effect-mode"),
      metaEffectPreset: document.getElementById("meta-effect-preset"),
      metaBrightness: document.getElementById("meta-brightness"),
      refreshAllBtn: document.getElementById("refresh-all-btn"),
      refreshFrameBtn: document.getElementById("refresh-frame-btn"),
      previewCanvas: document.getElementById("preview")
    };
    const previewCtx = els.previewCanvas.getContext("2d", { alpha: false });

    function textValue(value) {
      if (typeof value === "string") {
        if (value.length > 0) {
          return value;
        }
        return "--";
      }
      if (typeof value === "number" && Number.isFinite(value)) {
        return String(value);
      }
      return "--";
    }

    function formatUptime(seconds) {
      if (typeof seconds !== "number" || !Number.isFinite(seconds) || seconds < 0) {
        return "--";
      }
      const day = Math.floor(seconds / 86400);
      const hour = Math.floor((seconds % 86400) / 3600);
      const minute = Math.floor((seconds % 3600) / 60);
      if (day > 0) {
        return String(day) + " 天 " + String(hour) + " 小时";
      }
      if (hour > 0) {
        return String(hour) + " 小时 " + String(minute) + " 分";
      }
      return String(minute) + " 分";
    }

    function setFeedback(text, level) {
      els.feedbackCard.textContent = text;
      els.feedbackCard.setAttribute("data-level", level);
    }

    function updateStatusPill() {
      let text = state.wsConnected ? "WebSocket 已连接，可直接下发设置" : "WebSocket 未连接";
      if (state.runtimeStatus !== null && typeof state.runtimeStatus.ip === "string" && state.runtimeStatus.ip.length > 0) {
        text += " · " + state.runtimeStatus.ip;
      }
      els.runtimeStatusText.textContent = text;
      els.runtimeStatus.classList.toggle("connected", state.wsConnected);
    }

    function rgbToHex(color) {
      const red = Number(color.r);
      const green = Number(color.g);
      const blue = Number(color.b);
      const toHex = (value) => {
        const safe = Math.max(0, Math.min(255, value));
        return safe.toString(16).padStart(2, "0");
      };
      return "#" + toHex(red) + toHex(green) + toHex(blue);
    }

    function hexToRgb(hex) {
      const match = /^#?([0-9a-fA-F]{6})$/.exec(hex);
      if (match == null) {
        return { r: 100, g: 200, b: 255 };
      }
      return {
        r: parseInt(match[1].slice(0, 2), 16),
        g: parseInt(match[1].slice(2, 4), 16),
        b: parseInt(match[1].slice(4, 6), 16)
      };
    }

    function isRainPreset(presetId) {
      return presetId === "rain_scene";
    }

    function syncAmbientInputs() {
      els.effectSpeedInput.value = String(state.ambientDraft.speed);
      els.effectSpeedValue.textContent = String(state.ambientDraft.speed);
      els.effectIntensityInput.value = String(state.ambientDraft.intensity);
      els.effectIntensityValue.textContent = String(state.ambientDraft.intensity);
      els.effectDensityInput.value = String(state.ambientDraft.density);
      els.effectDensityValue.textContent = String(state.ambientDraft.density);
      els.effectColorInput.value = state.ambientDraft.color;
      els.effectColorValue.textContent = state.ambientDraft.color.toUpperCase();
      els.effectLoopInput.checked = state.ambientDraft.loop;
      const rain = isRainPreset(state.ambientDraft.preset);
      els.effectIntensityRow.style.display = rain ? "none" : "block";
      els.effectDensityRow.style.display = rain ? "block" : "none";
      els.effectColorRow.style.display = rain ? "block" : "none";
    }

    function renderSummary() {
      if (state.runtimeStatus !== null) {
        els.summaryIp.textContent = textValue(state.runtimeStatus.ip);
        els.previewBusinessMode.textContent = textValue(state.runtimeStatus.businessMode);
        els.metaMode.textContent = textValue(state.runtimeStatus.mode);
        els.metaBusinessMode.textContent = textValue(state.runtimeStatus.businessMode);
        els.metaThemeId.textContent = textValue(state.runtimeStatus.themeId);
        els.metaEffectMode.textContent = textValue(state.runtimeStatus.effectMode);
        els.metaEffectPreset.textContent = textValue(state.runtimeStatus.effectPreset);
        els.metaBrightness.textContent = textValue(state.runtimeStatus.brightness);
      }
      if (state.deviceParams !== null) {
        els.summarySsid.textContent = textValue(state.deviceParams.wifiSsid);
        els.summaryFirmware.textContent = textValue(state.deviceParams.firmwareVersion);
        els.summaryUptime.textContent = formatUptime(state.deviceParams.uptime);
      }
    }

    function renderModeGrid() {
      const active = state.runtimeStatus !== null && typeof state.runtimeStatus.businessMode === "string"
        ? state.runtimeStatus.businessMode
        : "";
      els.modeGrid.innerHTML = "";
      MODE_OPTIONS.forEach((item) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "mode-card";
        if (item.id === active) {
          button.classList.add("active");
        }
        const accent = document.createElement("div");
        accent.className = "mode-accent tone-" + item.tone;
        const name = document.createElement("div");
        name.className = "mode-name";
        name.textContent = item.name;
        const desc = document.createElement("div");
        desc.className = "mode-desc";
        desc.textContent = item.desc;
        button.appendChild(accent);
        button.appendChild(name);
        button.appendChild(desc);
        button.addEventListener("click", () => {
          applyMode(item.id);
        });
        els.modeGrid.appendChild(button);
      });
    }

    function renderThemeGrid() {
      const active = state.runtimeStatus !== null && typeof state.runtimeStatus.themeId === "string"
        ? state.runtimeStatus.themeId
        : "";
      els.themeGrid.innerHTML = "";
      THEMES.forEach((item) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "theme-card";
        if (item.id === active) {
          button.classList.add("active");
        }
        const name = document.createElement("div");
        name.className = "mode-name";
        name.textContent = item.name;
        const desc = document.createElement("div");
        desc.className = "mode-desc";
        desc.textContent = item.id;
        button.appendChild(name);
        button.appendChild(desc);
        button.addEventListener("click", () => {
          applyTheme(item.id);
        });
        els.themeGrid.appendChild(button);
      });
    }

    function renderPresetGrid() {
      els.presetGrid.innerHTML = "";
      PRESETS.forEach((item) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "preset-card";
        if (item.id === state.ambientDraft.preset) {
          button.classList.add("active");
        }
        const name = document.createElement("div");
        name.className = "mode-name";
        name.textContent = item.name;
        const desc = document.createElement("div");
        desc.className = "mode-desc";
        desc.textContent = item.desc;
        button.appendChild(name);
        button.appendChild(desc);
        button.addEventListener("click", () => {
          state.ambientDirty = true;
          state.ambientDraft.preset = item.id;
          renderPresetGrid();
          syncAmbientInputs();
        });
        els.presetGrid.appendChild(button);
      });
    }

    function renderDeviceParams() {
      if (state.deviceParams == null) {
        return;
      }
      const params = state.deviceParams;
      if (typeof params.displayBright === "number") {
        els.displayBrightInput.value = String(params.displayBright);
        els.displayBrightValue.textContent = String(params.displayBright);
      }
      if (typeof params.brightnessDay === "number") {
        els.brightnessDayInput.value = String(params.brightnessDay);
        els.brightnessDayValue.textContent = String(params.brightnessDay);
      }
      if (typeof params.brightnessNight === "number") {
        els.brightnessNightInput.value = String(params.brightnessNight);
        els.brightnessNightValue.textContent = String(params.brightnessNight);
      }
      if (typeof params.nightStart === "string") {
        els.nightStartInput.value = params.nightStart;
      }
      if (typeof params.nightEnd === "string") {
        els.nightEndInput.value = params.nightEnd;
      }
      if (typeof params.displayRotation === "number") {
        els.rotationSelect.value = String(params.displayRotation);
      }
      if (typeof params.swapBlueGreen === "boolean") {
        els.swapBlueGreenInput.checked = params.swapBlueGreen;
      }
      if (typeof params.swapBlueRed === "boolean") {
        els.swapBlueRedInput.checked = params.swapBlueRed;
      }
      if (typeof params.clkphase === "boolean") {
        els.clkphaseInput.checked = params.clkphase;
      }
      if (typeof params.driver === "number") {
        els.driverSelect.value = String(params.driver);
      }
      if (typeof params.i2cSpeed === "number") {
        els.speedSelect.value = String(params.i2cSpeed);
      }
      if (typeof params.E_pin === "number") {
        els.ePinInput.value = String(params.E_pin);
      }
      if (typeof params.ntpServer === "string") {
        els.ntpServerInput.value = params.ntpServer;
        let preset = "__custom__";
        if (params.ntpServer === "ntp2.aliyun.com" ||
            params.ntpServer === "ntp.aliyun.com" ||
            params.ntpServer === "ntp.tencent.com" ||
            params.ntpServer === "ntp.ntsc.ac.cn") {
          preset = params.ntpServer;
        }
        els.ntpPresetSelect.value = preset;
      }
    }

    async function parseJsonSafe(response) {
      try {
        return await response.json();
      } catch (error) {
        return null;
      }
    }

    async function postForm(url, payload) {
      const form = new URLSearchParams();
      for (const key in payload) {
        if (Object.prototype.hasOwnProperty.call(payload, key)) {
          form.append(key, String(payload[key]));
        }
      }
      const response = await fetch(url, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
        body: form.toString()
      });
      const result = await parseJsonSafe(response);
      if (!response.ok) {
        if (result !== null && typeof result.error === "string" && result.error.length > 0) {
          throw new Error(result.error);
        }
        throw new Error("请求失败");
      }
      return result;
    }

    async function syncDeviceParams() {
      const response = await fetch("/get?ts=" + Date.now(), { cache: "no-store" });
      if (!response.ok) {
        throw new Error("设备参数读取失败");
      }
      state.deviceParams = await response.json();
      renderDeviceParams();
      renderSummary();
    }

    async function syncRuntimeStatus() {
      const response = await fetch("/status?ts=" + Date.now(), { cache: "no-store" });
      if (!response.ok) {
        throw new Error("运行状态读取失败");
      }
      const data = await response.json();
      state.runtimeStatus = data;
      if (!state.ambientDirty) {
        if (typeof data.effectPreset === "string" && data.effectPreset.length > 0) {
          state.ambientDraft.preset = data.effectPreset;
        }
        if (typeof data.effectSpeed === "number") {
          state.ambientDraft.speed = data.effectSpeed;
        }
        if (typeof data.effectIntensity === "number") {
          state.ambientDraft.intensity = data.effectIntensity;
        }
        if (typeof data.effectDensity === "number") {
          state.ambientDraft.density = data.effectDensity;
        }
        if (data.effectColor !== null && typeof data.effectColor === "object") {
          state.ambientDraft.color = rgbToHex(data.effectColor);
        }
        if (typeof data.effectLoop === "boolean") {
          state.ambientDraft.loop = data.effectLoop;
        }
      }
      renderSummary();
      renderModeGrid();
      renderThemeGrid();
      renderPresetGrid();
      syncAmbientInputs();
      updateStatusPill();
    }

    function connectSocket() {
      const protocol = location.protocol === "https:" ? "wss://" : "ws://";
      state.ws = new WebSocket(protocol + location.host + "/ws");
      state.ws.addEventListener("open", () => {
        state.wsConnected = true;
        updateStatusPill();
        setFeedback("设备在线，可直接切换模式和保存参数。", "ok");
      });
      state.ws.addEventListener("close", () => {
        state.wsConnected = false;
        updateStatusPill();
        setFeedback("WebSocket 已断开，正在重连。", "warn");
        window.setTimeout(connectSocket, 1500);
      });
      state.ws.addEventListener("error", () => {
        state.wsConnected = false;
        updateStatusPill();
        setFeedback("WebSocket 连接异常。", "error");
      });
      state.ws.addEventListener("message", async (event) => {
        let data = null;
        try {
          data = JSON.parse(event.data);
        } catch (error) {
          return;
        }
        if (data !== null && typeof data.message === "string" && data.message.length > 0) {
          setFeedback(data.message, data.status === "ok" ? "ok" : "error");
        }
        await syncRuntimeStatus().catch(() => {});
      });
    }

    function sendJson(payload) {
      if (state.ws == null || state.ws.readyState !== WebSocket.OPEN) {
        setFeedback("WebSocket 未连接，暂时无法下发模式设置。", "error");
        return false;
      }
      state.ws.send(JSON.stringify(payload));
      return true;
    }

    async function saveBrightness(value) {
      try {
        await postForm("/brightness", { value: value });
        setFeedback("显示亮度已更新。", "ok");
        await syncDeviceParams();
        await syncRuntimeStatus();
      } catch (error) {
        setFeedback(error.message, "error");
      }
    }

    async function saveParam(key, value, successText) {
      const payload = {};
      payload[key] = value;
      try {
        await postForm("/set", payload);
        setFeedback(successText, "ok");
        await syncDeviceParams();
        await syncRuntimeStatus();
      } catch (error) {
        setFeedback(error.message, "error");
      }
    }

    function applyMode(modeId) {
      if (!sendJson({ cmd: "set_mode", mode: modeId })) {
        return;
      }
      setFeedback("模式切换指令已发送。", "ok");
      window.setTimeout(() => { syncRuntimeStatus().catch(() => {}); }, 260);
    }

    function applyTheme(themeId) {
      if (!sendJson({ cmd: "set_theme_config", themeId: themeId })) {
        return;
      }
      setFeedback("主题切换指令已发送。", "ok");
      window.setTimeout(() => { syncRuntimeStatus().catch(() => {}); }, 260);
    }

    function applyAmbientDraft() {
      const payload = {
        cmd: "set_ambient_effect",
        preset: state.ambientDraft.preset,
        speed: Number(els.effectSpeedInput.value),
        loop: els.effectLoopInput.checked
      };
      if (isRainPreset(state.ambientDraft.preset)) {
        payload.density = Number(els.effectDensityInput.value);
        payload.color = hexToRgb(els.effectColorInput.value);
      } else {
        payload.intensity = Number(els.effectIntensityInput.value);
      }
      state.ambientDirty = false;
      if (!sendJson(payload)) {
        return;
      }
      setFeedback("像素场景集指令已发送。", "ok");
      window.setTimeout(() => { syncRuntimeStatus().catch(() => {}); }, 260);
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

    async function pollFrame() {
      try {
        const response = await fetch("/live-frame.bin?ts=" + Date.now(), { cache: "no-store" });
        if (!response.ok) {
          throw new Error("实时帧读取失败");
        }
        const buffer = await response.arrayBuffer();
        drawFrame(buffer);
      } catch (error) {
        setFeedback(error.message, "error");
      } finally {
        window.setTimeout(pollFrame, 240);
      }
    }

    els.displayBrightInput.addEventListener("input", () => {
      els.displayBrightValue.textContent = els.displayBrightInput.value;
    });
    els.displayBrightInput.addEventListener("change", () => {
      saveBrightness(Number(els.displayBrightInput.value));
    });
    els.brightnessDayInput.addEventListener("input", () => {
      els.brightnessDayValue.textContent = els.brightnessDayInput.value;
    });
    els.brightnessDayInput.addEventListener("change", () => {
      saveParam("brightnessDay", els.brightnessDayInput.value, "日间亮度已保存。");
    });
    els.brightnessNightInput.addEventListener("input", () => {
      els.brightnessNightValue.textContent = els.brightnessNightInput.value;
    });
    els.brightnessNightInput.addEventListener("change", () => {
      saveParam("brightnessNight", els.brightnessNightInput.value, "夜间亮度已保存。");
    });
    els.nightStartInput.addEventListener("change", () => {
      saveParam("nightStart", els.nightStartInput.value, "夜间开始时间已保存。");
    });
    els.nightEndInput.addEventListener("change", () => {
      saveParam("nightEnd", els.nightEndInput.value, "夜间结束时间已保存。");
    });
    els.rotationSelect.addEventListener("change", () => {
      saveParam("displayRotation", els.rotationSelect.value, "显示方向已保存。");
    });
    els.swapBlueGreenInput.addEventListener("change", () => {
      saveParam("swapBlueGreen", els.swapBlueGreenInput.checked ? "1" : "0", "swapBlueGreen 已保存。");
    });
    els.swapBlueRedInput.addEventListener("change", () => {
      saveParam("swapBlueRed", els.swapBlueRedInput.checked ? "1" : "0", "swapBlueRed 已保存。");
    });
    els.clkphaseInput.addEventListener("change", () => {
      saveParam("clkphase", els.clkphaseInput.checked ? "1" : "0", "clkphase 已保存。");
    });
    els.driverSelect.addEventListener("change", () => {
      saveParam("driver", els.driverSelect.value, "驱动芯片已保存。");
    });
    els.speedSelect.addEventListener("change", () => {
      saveParam("i2cSpeed", els.speedSelect.value, "I2S 速度已保存。");
    });
    els.saveEPinBtn.addEventListener("click", () => {
      saveParam("E_pin", els.ePinInput.value, "E_pin 已保存。");
    });
    els.ntpPresetSelect.addEventListener("change", () => {
      if (els.ntpPresetSelect.value === "__custom__") {
        els.ntpServerInput.focus();
        return;
      }
      els.ntpServerInput.value = els.ntpPresetSelect.value;
      saveParam("ntpServer", els.ntpPresetSelect.value, "时间服务器已保存。");
    });
    els.saveNtpBtn.addEventListener("click", () => {
      saveParam("ntpServer", els.ntpServerInput.value, "时间服务器已保存。");
    });
    els.effectSpeedInput.addEventListener("input", () => {
      state.ambientDirty = true;
      state.ambientDraft.speed = Number(els.effectSpeedInput.value);
      syncAmbientInputs();
    });
    els.effectIntensityInput.addEventListener("input", () => {
      state.ambientDirty = true;
      state.ambientDraft.intensity = Number(els.effectIntensityInput.value);
      syncAmbientInputs();
    });
    els.effectDensityInput.addEventListener("input", () => {
      state.ambientDirty = true;
      state.ambientDraft.density = Number(els.effectDensityInput.value);
      syncAmbientInputs();
    });
    els.effectColorInput.addEventListener("input", () => {
      state.ambientDirty = true;
      state.ambientDraft.color = els.effectColorInput.value;
      syncAmbientInputs();
    });
    els.effectLoopInput.addEventListener("change", () => {
      state.ambientDirty = true;
      state.ambientDraft.loop = els.effectLoopInput.checked;
    });
    els.applyAmbientBtn.addEventListener("click", applyAmbientDraft);
    els.refreshAllBtn.addEventListener("click", async () => {
      try {
        await syncDeviceParams();
        await syncRuntimeStatus();
        setFeedback("运行态信息已刷新。", "ok");
      } catch (error) {
        setFeedback(error.message, "error");
      }
    });
    els.refreshFrameBtn.addEventListener("click", async () => {
      try {
        const response = await fetch("/live-frame.bin?ts=" + Date.now(), { cache: "no-store" });
        if (!response.ok) {
          throw new Error("实时帧读取失败");
        }
        const buffer = await response.arrayBuffer();
        drawFrame(buffer);
        setFeedback("实时预览已刷新。", "ok");
      } catch (error) {
        setFeedback(error.message, "error");
      }
    });

    renderModeGrid();
    renderThemeGrid();
    renderPresetGrid();
    syncAmbientInputs();
    updateStatusPill();
    connectSocket();
    syncDeviceParams().catch((error) => { setFeedback(error.message, "error"); });
    syncRuntimeStatus().catch((error) => { setFeedback(error.message, "error"); });
    pollFrame();
    window.setInterval(() => { syncRuntimeStatus().catch(() => {}); }, 2400);
  </script>
</body>
</html>)HTML";

  return html;
}

String buildRuntimeSettingsLitePage() {
  return R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Glowxel 设置</title>
  <style>
    body{margin:0;padding:14px;background:#08111d;color:#e6edf7;font:14px/1.45 -apple-system,BlinkMacSystemFont,"PingFang SC","Microsoft YaHei",sans-serif}
    .wrap{max-width:760px;margin:0 auto}
    .card{background:#102033;border:1px solid #22354b;border-radius:12px;padding:14px;margin-bottom:12px}
    h1,h2{margin:0 0 10px} h1{font-size:20px} h2{font-size:14px}
    .meta{display:grid;grid-template-columns:90px 1fr;gap:6px 10px;margin:0}
    .meta dt{color:#8da2ba}.meta dd{margin:0;word-break:break-all}
    .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:12px}
    label{display:block;margin:0 0 10px;color:#aab9cc;font-size:12px}
    input,select,button{width:100%;box-sizing:border-box;padding:9px 10px;border-radius:10px;border:1px solid #29405a;background:#15283f;color:#e6edf7;font:inherit}
    input[type=checkbox]{width:auto;margin-right:6px}
    input[type=range]{padding:0}
    .check{display:flex;align-items:center;color:#e6edf7}
    .row{display:grid;grid-template-columns:1fr 42px;gap:8px;align-items:center}
    .actions{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:8px;margin-top:8px}
    .danger{background:#3a1b24;border-color:#6d3044}
    .msg{padding:11px 12px;border:1px solid #22354b;border-radius:12px;background:#0d1928;color:#9fb0c4;min-height:20px}
    .ok{color:#b8f2c8}.err{color:#ffb4b4}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>Glowxel 运行态设置</h1>
      <div class="actions">
        <button id="refreshBtn" type="button">刷新状态</button>
        <button id="clearWifiBtn" class="danger" type="button">清除 WiFi 并重启</button>
      </div>
    </div>

    <div class="card">
      <h2>设备状态</h2>
      <dl class="meta">
        <dt>WiFi</dt><dd id="wifiSsid">-</dd>
        <dt>固件</dt><dd id="firmwareVersion">-</dd>
        <dt>运行时长</dt><dd id="uptime">-</dd>
        <dt>当前模式</dt><dd id="modeName">-</dd>
        <dt>业务模式</dt><dd id="businessMode">-</dd>
      </dl>
    </div>

    <div class="grid">
      <div class="card">
        <h2>亮度</h2>
        <label>显示亮度<div class="row"><input id="displayBright" type="range" min="0" max="255" value="50"><span id="displayBrightText">50</span></div></label>
        <label>日间亮度<div class="row"><input id="brightnessDay" type="range" min="0" max="255" value="50"><span id="brightnessDayText">50</span></div></label>
        <label>夜间亮度<div class="row"><input id="brightnessNight" type="range" min="0" max="255" value="50"><span id="brightnessNightText">50</span></div></label>
        <button id="saveBrightnessBtn" type="button">保存亮度</button>
      </div>

      <div class="card">
        <h2>硬件参数</h2>
        <label>显示旋转<select id="displayRotation"><option value="0">0</option><option value="1">90</option><option value="2">180</option><option value="3">270</option></select></label>
        <label class="check"><input id="swapBlueGreen" type="checkbox">交换蓝绿</label>
        <label class="check"><input id="swapBlueRed" type="checkbox">交换蓝红</label>
        <label class="check"><input id="clkphase" type="checkbox">启用 clkphase</label>
        <label>驱动芯片<select id="driver"><option value="0">SHIFTREG</option><option value="1">FM6124</option><option value="2">FM6126A</option><option value="3">ICN2038S</option><option value="4">MBI5124</option><option value="5">DP3246</option></select></label>
        <label>I2S 速度<select id="i2cSpeed"><option value="8000000">8000000</option><option value="16000000">16000000</option><option value="20000000">20000000</option></select></label>
        <label>E_pin<input id="E_pin" type="number" min="0" max="32" step="1"></label>
        <button id="saveHardwareBtn" type="button">保存硬件参数</button>
      </div>

      <div class="card">
        <h2>时间参数</h2>
        <label>夜间开始<input id="nightStart" type="time"></label>
        <label>夜间结束<input id="nightEnd" type="time"></label>
        <label>NTP 服务器<input id="ntpServer" type="text" maxlength="63"></label>
        <button id="saveTimeBtn" type="button">保存时间参数</button>
      </div>
    </div>

    <div id="feedback" class="msg">准备就绪。</div>
  </div>

  <script>
    const $=id=>document.getElementById(id);
    const els={
      wifiSsid:$("wifiSsid"),firmwareVersion:$("firmwareVersion"),uptime:$("uptime"),
      modeName:$("modeName"),businessMode:$("businessMode"),
      displayBright:$("displayBright"),displayBrightText:$("displayBrightText"),
      brightnessDay:$("brightnessDay"),brightnessDayText:$("brightnessDayText"),
      brightnessNight:$("brightnessNight"),brightnessNightText:$("brightnessNightText"),
      displayRotation:$("displayRotation"),swapBlueGreen:$("swapBlueGreen"),
      swapBlueRed:$("swapBlueRed"),clkphase:$("clkphase"),driver:$("driver"),
      i2cSpeed:$("i2cSpeed"),E_pin:$("E_pin"),nightStart:$("nightStart"),
      nightEnd:$("nightEnd"),ntpServer:$("ntpServer"),feedback:$("feedback")
    };

    function setFeedback(msg,type){els.feedback.textContent=msg;els.feedback.className="msg"+(type==="ok"?" ok":type==="err"?" err":"");}
    function syncRangeText(){els.displayBrightText.textContent=els.displayBright.value;els.brightnessDayText.textContent=els.brightnessDay.value;els.brightnessNightText.textContent=els.brightnessNight.value;}
    function formatUptime(seconds){const t=Number(seconds||0);const h=Math.floor(t/3600);const m=Math.floor((t%3600)/60);const s=t%60;return h+"h "+m+"m "+s+"s";}
    function decodeErrorText(text){try{const parsed=JSON.parse(text);if(parsed&&typeof parsed.error==="string")return parsed.error;}catch(_){ }return text||"请求失败";}
    async function fetchJson(url){const response=await fetch(url,{cache:"no-store"});const text=await response.text();if(!response.ok)throw new Error(decodeErrorText(text));return JSON.parse(text);}
    async function postForm(url,payload){const response=await fetch(url,{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded;charset=UTF-8"},body:new URLSearchParams(payload).toString()});const text=await response.text();if(!response.ok)throw new Error(decodeErrorText(text));return text?JSON.parse(text):{};}
    async function saveParam(key,value){const payload={};payload[key]=value;return postForm("/set",payload);}

    function applyStatus(status){
      els.wifiSsid.textContent=typeof status.wifiSsid==="string"?status.wifiSsid:"-";
      els.firmwareVersion.textContent=typeof status.firmware_version==="string"?status.firmware_version:"-";
      els.uptime.textContent=formatUptime(status.uptime);
      els.modeName.textContent=typeof status.mode==="string"?status.mode:"-";
      els.businessMode.textContent=typeof status.businessMode==="string"?status.businessMode:"-";
      els.displayBright.value=String(status.displayBright??50);
      els.brightnessDay.value=String(status.brightnessDay??50);
      els.brightnessNight.value=String(status.brightnessNight??50);
      els.displayRotation.value=String(status.displayRotation??0);
      els.swapBlueGreen.checked=Boolean(status.swapBlueGreen);
      els.swapBlueRed.checked=Boolean(status.swapBlueRed);
      els.clkphase.checked=Boolean(status.clkphase);
      els.driver.value=String(status.driver??0);
      els.i2cSpeed.value=String(status.i2cSpeed??8000000);
      els.E_pin.value=String(status.E_pin??18);
      els.nightStart.value=typeof status.nightStart==="string"?status.nightStart:"22:00";
      els.nightEnd.value=typeof status.nightEnd==="string"?status.nightEnd:"07:00";
      els.ntpServer.value=typeof status.ntpServer==="string"?status.ntpServer:"";
      syncRangeText();
    }

    async function refreshAll(){applyStatus(await fetchJson("/status"));}
    async function saveBrightnessGroup(){await saveParam("displayBright",els.displayBright.value);await saveParam("brightnessDay",els.brightnessDay.value);await saveParam("brightnessNight",els.brightnessNight.value);}
    async function saveHardwareGroup(){await saveParam("displayRotation",els.displayRotation.value);await saveParam("swapBlueGreen",els.swapBlueGreen.checked?"1":"0");await saveParam("swapBlueRed",els.swapBlueRed.checked?"1":"0");await saveParam("clkphase",els.clkphase.checked?"1":"0");await saveParam("driver",els.driver.value);await saveParam("i2cSpeed",els.i2cSpeed.value);await saveParam("E_pin",els.E_pin.value);}
    async function saveTimeGroup(){await saveParam("nightStart",els.nightStart.value);await saveParam("nightEnd",els.nightEnd.value);await saveParam("ntpServer",els.ntpServer.value.trim());}

    els.displayBright.addEventListener("input",syncRangeText);
    els.brightnessDay.addEventListener("input",syncRangeText);
    els.brightnessNight.addEventListener("input",syncRangeText);
    $("refreshBtn").addEventListener("click",async()=>{try{await refreshAll();setFeedback("设备状态已刷新。","ok");}catch(error){setFeedback(error.message,"err");}});
    $("saveBrightnessBtn").addEventListener("click",async()=>{try{await saveBrightnessGroup();await refreshAll();setFeedback("亮度参数已保存。","ok");}catch(error){setFeedback(error.message,"err");}});
    $("saveHardwareBtn").addEventListener("click",async()=>{try{await saveHardwareGroup();await refreshAll();setFeedback("硬件参数已保存。","ok");}catch(error){setFeedback(error.message,"err");}});
    $("saveTimeBtn").addEventListener("click",async()=>{try{await saveTimeGroup();await refreshAll();setFeedback("时间参数已保存。","ok");}catch(error){setFeedback(error.message,"err");}});
    $("clearWifiBtn").addEventListener("click",async()=>{if(!window.confirm("确认清除 WiFi 配置并重启吗？"))return;try{const response=await fetch("/clear-wifi",{cache:"no-store"});const text=await response.text();if(!response.ok)throw new Error(decodeErrorText(text));setFeedback("WiFi 配置已清除，设备即将重启。","ok");}catch(error){setFeedback(error.message,"err");}});

    refreshAll().then(()=>setFeedback("设备参数已同步。","ok")).catch(error=>setFeedback(error.message,"err"));
  </script>
</body>
</html>)HTML";
}

int hexValue(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return 10 + (ch - 'a');
  }
  if (ch >= 'A' && ch <= 'F') {
    return 10 + (ch - 'A');
  }
  return -1;
}

String urlDecode(const String& text) {
  String decoded;
  decoded.reserve(text.length());

  for (size_t i = 0; i < text.length(); i++) {
    char ch = text[i];
    if (ch == '+') {
      decoded += ' ';
      continue;
    }
    if (ch == '%' && i + 2 < text.length()) {
      int hi = hexValue(text[i + 1]);
      int lo = hexValue(text[i + 2]);
      if (hi >= 0 && lo >= 0) {
        decoded += static_cast<char>((hi << 4) | lo);
        i += 2;
        continue;
      }
    }
    decoded += ch;
  }

  return decoded;
}

bool tryExtractFormValue(const String& encoded, const char* key, String& outValue) {
  size_t start = 0;
  while (start <= encoded.length()) {
    int end = encoded.indexOf('&', start);
    if (end < 0) {
      end = encoded.length();
    }

    String entry = encoded.substring(start, end);
    if (entry.length() > 0) {
      int equalsIndex = entry.indexOf('=');
      String entryKey = equalsIndex >= 0 ? entry.substring(0, equalsIndex) : entry;
      if (urlDecode(entryKey) == key) {
        outValue = equalsIndex >= 0 ? urlDecode(entry.substring(equalsIndex + 1)) : "";
        return true;
      }
    }

    if (end >= static_cast<int>(encoded.length())) {
      break;
    }
    start = static_cast<size_t>(end + 1);
  }

  return false;
}

bool findSyncRequestParam(const SyncHttpRequest& request, const char* key, String& outValue) {
  if (tryExtractFormValue(request.query, key, outValue)) {
    return true;
  }
  if (tryExtractFormValue(request.body, key, outValue)) {
    return true;
  }
  return false;
}

int readContentLength(const String& headers) {
  int start = headers.indexOf("Content-Length:");
  if (start < 0) {
    return 0;
  }

  int lineStart = start + static_cast<int>(strlen("Content-Length:"));
  while (lineStart < headers.length() && headers[lineStart] == ' ') {
    lineStart++;
  }
  int lineEnd = headers.indexOf("\r\n", lineStart);
  String value = lineEnd >= 0 ? headers.substring(lineStart, lineEnd) : headers.substring(lineStart);
  return value.toInt();
}

bool readSyncHttpRequest(WiFiClient& client, SyncHttpRequest& request) {
  String buffer;
  int headerEnd = -1;
  unsigned long deadline = millis() + 500;

  while (client.connected() && millis() < deadline) {
    while (client.available()) {
      buffer += static_cast<char>(client.read());
      if (buffer.length() > 4096) {
        return false;
      }
      headerEnd = buffer.indexOf("\r\n\r\n");
      if (headerEnd >= 0) {
        break;
      }
    }
    if (headerEnd >= 0) {
      break;
    }
    delay(1);
  }

  if (headerEnd < 0) {
    return false;
  }

  String headers = buffer.substring(0, headerEnd);
  request.body = buffer.substring(headerEnd + 4);

  int contentLength = readContentLength(headers);
  while (request.body.length() < contentLength && client.connected() && millis() < deadline) {
    while (client.available()) {
      request.body += static_cast<char>(client.read());
      if (request.body.length() > 2048) {
        return false;
      }
    }
    delay(1);
  }

  int requestLineEnd = headers.indexOf("\r\n");
  String requestLine = requestLineEnd >= 0 ? headers.substring(0, requestLineEnd) : headers;
  int methodEnd = requestLine.indexOf(' ');
  int targetEnd = requestLine.indexOf(' ', methodEnd + 1);
  if (methodEnd <= 0 || targetEnd <= methodEnd) {
    return false;
  }

  request.method = requestLine.substring(0, methodEnd);
  String target = requestLine.substring(methodEnd + 1, targetEnd);
  int queryStart = target.indexOf('?');
  if (queryStart >= 0) {
    request.path = target.substring(0, queryStart);
    request.query = target.substring(queryStart + 1);
  } else {
    request.path = target;
    request.query = "";
  }

  return true;
}

void sendSyncResponse(
  WiFiClient& client,
  int statusCode,
  const char* statusText,
  const char* contentType,
  const String& body
) {
  client.printf("HTTP/1.1 %d %s\r\n", statusCode, statusText);
  client.printf("Content-Type: %s\r\n", contentType);
  client.printf("Content-Length: %u\r\n", static_cast<unsigned>(body.length()));
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
  client.println("Access-Control-Allow-Headers: Content-Type");
  client.println("Access-Control-Allow-Private-Network: true");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
  client.print(body);
}

void sendSyncRedirect(WiFiClient& client, const String& location) {
  client.println("HTTP/1.1 302 Found");
  client.printf("Location: %s\r\n", location.c_str());
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
  client.println("Access-Control-Allow-Headers: Content-Type");
  client.println("Access-Control-Allow-Private-Network: true");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
}

String buildErrorJson(const char* message) {
  String body = "{\"error\":\"";
  body += message == nullptr ? "unknown error" : message;
  body += "\"}";
  return body;
}

void handlePortalSyncRequest(WiFiClient& client, const SyncHttpRequest& request) {
  if (request.method == "OPTIONS") {
    sendSyncResponse(client, 204, "No Content", "text/plain; charset=utf-8", "");
    return;
  }

  if (request.method == "GET" && request.path == "/favicon.ico") {
    sendSyncResponse(client, 204, "No Content", "text/plain; charset=utf-8", "");
    return;
  }

  if (request.method == "GET" && request.path == "/test") {
    Serial.println("收到测试请求");
    sendSyncResponse(client, 200, "OK", "text/plain; charset=utf-8", "ESP32 is working!");
    return;
  }

  if (request.method == "GET" && request.path == "/") {
    sendSyncResponse(client, 200, "OK", "text/html; charset=utf-8", buildWifiConfigPortalPage());
    return;
  }

  if (request.path == "/scan-wifi") {
    if (request.method == "POST") {
      WiFiManager::scanNearbyNetworks();
    }
    if (request.method == "GET" || request.method == "POST") {
      sendSyncResponse(
        client,
        200,
        "OK",
        "application/json; charset=utf-8",
        buildWifiScanSnapshotJson()
      );
      return;
    }
  }

  if (request.method == "POST" && request.path == "/configure-wifi") {
    String ssid;
    String password;
    bool hasSsid = findSyncRequestParam(request, "ssid", ssid);
    bool hasPassword = findSyncRequestParam(request, "password", password);
    if (!hasSsid || !hasPassword) {
      sendSyncResponse(
        client,
        400,
        "Bad Request",
        "text/plain; charset=utf-8",
        "missing ssid or password field"
      );
      return;
    }

    if (ssid.length() == 0) {
      sendSyncResponse(client, 400, "Bad Request", "text/plain; charset=utf-8", "ssid cannot be empty");
      return;
    }
    if (ssid.length() > 32 || password.length() > 64) {
      sendSyncResponse(client, 400, "Bad Request", "text/plain; charset=utf-8", "ssid or password is too long");
      return;
    }

    WiFiManager::saveConfigPortalCredentials(ssid, password);
    WiFiManager::schedulePortalRestart(2500);
    sendSyncResponse(client, 200, "OK", "text/html; charset=utf-8", buildWifiConfigSuccessPage(ssid));
    return;
  }

  if (request.method == "GET" && request.path == "/clear-wifi") {
    Serial.println("清除WiFi配置");
    ConfigManager::preferences.begin("wifi", false);
    ConfigManager::preferences.clear();
    ConfigManager::preferences.end();
    sendSyncResponse(
      client,
      200,
      "OK",
      "application/json; charset=utf-8",
      "{\"status\":\"ok\",\"message\":\"WiFi配置已清除，3秒后重启\"}"
    );
    client.flush();
    delay(3000);
    ESP.restart();
    return;
  }

  String location = "http://" + WiFiManager::getConfigPortalIP() + "/";
  sendSyncRedirect(client, location);
}

void handleSyncServerClient(WiFiClient& client) {
  SyncHttpRequest request;
  if (!readSyncHttpRequest(client, request)) {
    sendSyncResponse(client, 400, "Bad Request", "application/json; charset=utf-8", buildErrorJson("invalid request"));
    return;
  }

  if (WiFiManager::isConfigMode()) {
    handlePortalSyncRequest(client, request);
    return;
  }

  if (request.method == "OPTIONS") {
    sendSyncResponse(client, 204, "No Content", "text/plain; charset=utf-8", "");
    return;
  }

  if (request.method == "GET" && request.path == "/test") {
    Serial.println("收到测试请求");
    sendSyncResponse(client, 200, "OK", "text/plain; charset=utf-8", "ESP32 is working!");
    return;
  }

  if (request.method == "GET" && request.path == "/get") {
    sendSyncResponse(client, 200, "OK", "application/json; charset=utf-8", buildDeviceParamsJson());
    return;
  }

  if (request.method == "GET" && request.path == "/status") {
    sendSyncResponse(client, 200, "OK", "application/json; charset=utf-8", buildRuntimeStatusJson());
    return;
  }

  if (request.method == "GET" && request.path == "/") {
    sendSyncResponse(client, 200, "OK", "text/html; charset=utf-8", buildRuntimeSettingsLitePage());
    return;
  }

  if (request.method == "GET" && request.path == "/favicon.ico") {
    sendSyncResponse(client, 204, "No Content", "text/plain; charset=utf-8", "");
    return;
  }

  if (request.method == "POST" && request.path == "/set") {
    String selectedKey = "";
    String selectedValue = "";

    for (size_t i = 0; i < sizeof(kRuntimeDeviceParamKeys) / sizeof(kRuntimeDeviceParamKeys[0]); i++) {
      String value;
      if (!findSyncRequestParam(request, kRuntimeDeviceParamKeys[i], value)) {
        continue;
      }
      if (selectedKey.length() > 0) {
        sendSyncResponse(client, 400, "Bad Request", "application/json; charset=utf-8", "{\"error\":\"only one device param can be updated per request\"}");
        return;
      }
      selectedKey = kRuntimeDeviceParamKeys[i];
      selectedValue = value;
    }

    if (selectedKey.length() == 0) {
      sendSyncResponse(client, 400, "Bad Request", "application/json; charset=utf-8", "{\"error\":\"missing device param\"}");
      return;
    }

    String errorMessage;
    if (!applyDeviceParamValue(selectedKey, selectedValue, errorMessage)) {
      sendSyncResponse(
        client,
        400,
        "Bad Request",
        "application/json; charset=utf-8",
        String("{\"error\":\"") + errorMessage + "\"}"
      );
      return;
    }

    StaticJsonDocument<128> doc;
    doc["status"] = "ok";
    doc["key"] = selectedKey;
    doc["value"] = selectedValue;
    String response;
    serializeJson(doc, response);
    sendSyncResponse(client, 200, "OK", "application/json; charset=utf-8", response);
    return;
  }

  if (request.method == "POST" && request.path == "/brightness") {
    String valueText;
    if (!findSyncRequestParam(request, "value", valueText)) {
      sendSyncResponse(client, 400, "Bad Request", "application/json; charset=utf-8", "{\"error\":\"missing value parameter\"}");
      return;
    }

    int brightness = valueText.toInt();
    if (brightness < 0 || brightness > 255) {
      sendSyncResponse(client, 400, "Bad Request", "application/json; charset=utf-8", "{\"error\":\"brightness must be 0-255\"}");
      return;
    }

    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(RuntimeCommandBus::RuntimeCommandSource::HTTP, 0);
    if (command == nullptr) {
      sendSyncResponse(client, 500, "Internal Server Error", "application/json; charset=utf-8", "{\"error\":\"out of memory\"}");
      return;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::BRIGHTNESS;
    command->intValue1 = brightness;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      sendSyncResponse(client, 503, "Service Unavailable", "application/json; charset=utf-8", "{\"error\":\"device busy\"}");
      return;
    }

    sendSyncResponse(client, 200, "OK", "application/json; charset=utf-8", String("{\"status\":\"ok\",\"brightness\":") + String(brightness) + "}");
    return;
  }

  if (request.method == "GET" && request.path == "/clear-wifi") {
    Serial.println("清除WiFi配置");
    ConfigManager::preferences.begin("wifi", false);
    ConfigManager::preferences.clear();
    ConfigManager::preferences.end();
    sendSyncResponse(client, 200, "OK", "application/json; charset=utf-8", "{\"status\":\"ok\",\"message\":\"WiFi配置已清除，3秒后重启\"}");
    client.flush();
    delay(3000);
    ESP.restart();
    return;
  }

  sendSyncResponse(client, 404, "Not Found", "text/plain; charset=utf-8", "Not Found");
}
}  // namespace

// 静态成员初始化
AsyncWebServer WebServer::server(80);
uint8_t* WebServer::imageBuffer = nullptr;
size_t WebServer::imageSize = 0;

bool WebServer::isSettingsSessionActive() {
  return isSettingsSessionActiveInternal();
}

void WebServer::initConfigPortal() {
  Serial.println("4. 启动热点配网页轻量HTTP服务器...");
  if (!gRuntimeSyncServerStarted) {
    gRuntimeSyncServer.begin();
    gRuntimeSyncServerStarted = true;
  }
  Serial.println("配网页轻量HTTP服务器已启动在端口80");
}

void WebServer::initRuntime() {
  Serial.println("4. 启动运行态 HTTP / WebSocket 服务器...");
  if (gRuntimeSyncServerStarted) {
    gRuntimeSyncServer.end();
    gRuntimeSyncServerStarted = false;
    Serial.println("[HTTP] 已关闭配网页同步 80 端口监听，运行态仅保留 Async HTTP/WS");
  }
  if (!gRuntimeAsyncServerStarted) {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Private-Network", "true");

    WebSocketHandler::init();
    server.addHandler(&WebSocketHandler::ws);
    setupRuntimeRoutes();
    setupAPIRoutes();
    server.begin();
    gRuntimeAsyncServerStarted = true;
  }
  Serial.println("运行态 HTTP / WebSocket 服务器已启动在端口80");
  Serial.println("等待客户端连接...");
}

void WebServer::setupPortalRoutes() {
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到测试请求");
    request->send(200, "text/plain", "ESP32 is working!");
  });

  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request){
    if (WiFiManager::isConfigMode()) {
      sendConfigPortalPage(request);
      return;
    }
    request->send(200, "text/html; charset=utf-8", buildControlHubPage());
  });

  server.on("/scan-wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!WiFiManager::isConfigMode()) {
      request->send(409, "text/plain; charset=utf-8", "device is not in config portal mode");
      return;
    }

    sendWifiScanSnapshotResponse(request);
  });

  server.on("/scan-wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!WiFiManager::isConfigMode()) {
      request->send(409, "text/plain; charset=utf-8", "device is not in config portal mode");
      return;
    }

    WiFiManager::scanNearbyNetworks();
    sendWifiScanSnapshotResponse(request);
  });

  server.on("/configure-wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!WiFiManager::isConfigMode()) {
      request->send(409, "text/plain; charset=utf-8", "device is not in config portal mode");
      return;
    }
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
      request->send(400, "text/plain; charset=utf-8", "missing ssid or password field");
      return;
    }

    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();

    if (ssid.length() == 0) {
      request->send(400, "text/plain; charset=utf-8", "ssid cannot be empty");
      return;
    }
    if (ssid.length() > 32 || password.length() > 64) {
      request->send(400, "text/plain; charset=utf-8", "ssid or password is too long");
      return;
    }

    WiFiManager::saveConfigPortalCredentials(ssid, password);
    WiFiManager::schedulePortalRestart(2500);

    AsyncWebServerResponse* response = request->beginResponse(
      200,
      "text/html; charset=utf-8",
      buildWifiConfigSuccessPage(ssid)
    );
    addConfigPortalNoCacheHeaders(response);
    request->send(response);
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

  auto captiveProbeHandler = [](AsyncWebServerRequest *request) {
    redirectToConfigPortalRoot(request);
  };

  server.on("/generate_204", HTTP_ANY, captiveProbeHandler);
  server.on("/gen_204", HTTP_ANY, captiveProbeHandler);
  server.on("/hotspot-detect.html", HTTP_ANY, captiveProbeHandler);
  server.on("/library/test/success.html", HTTP_ANY, captiveProbeHandler);
  server.on("/connecttest.txt", HTTP_ANY, captiveProbeHandler);
  server.on("/ncsi.txt", HTTP_ANY, captiveProbeHandler);
  server.on("/success.txt", HTTP_ANY, captiveProbeHandler);
  server.on("/canonical.html", HTTP_ANY, captiveProbeHandler);
  server.on("/connectivity-check.html", HTTP_ANY, captiveProbeHandler);
  server.on("/redirect", HTTP_ANY, captiveProbeHandler);
  server.on("/fwlink", HTTP_ANY, captiveProbeHandler);
  server.on("/favicon.ico", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(204);
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    if (request->method() == HTTP_OPTIONS) {
      request->send(204);
      return;
    }
    redirectToConfigPortalRoot(request);
  });
}

void WebServer::setupRuntimeRoutes() {
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到测试请求");
    request->send(200, "text/plain", "ESP32 is working!");
  });

  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(200, "text/html; charset=utf-8", buildRuntimeSettingsLitePage());
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
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendDeviceParamsResponse(request);
  });

  server.on("/set", HTTP_POST, [](AsyncWebServerRequest *request) {
    static const char* kDeviceParamKeys[] = {
      "displayBright",
      "brightnessDay",
      "brightnessNight",
      "displayRotation",
      "swapBlueGreen",
      "swapBlueRed",
      "clkphase",
      "driver",
      "i2cSpeed",
      "E_pin",
      "nightStart",
      "nightEnd",
      "ntpServer"
    };

    String selectedKey = "";
    String selectedValue = "";

    for (size_t i = 0; i < sizeof(kDeviceParamKeys) / sizeof(kDeviceParamKeys[0]); i++) {
      const AsyncWebParameter* param = findDeviceParam(request, kDeviceParamKeys[i]);
      if (param == nullptr) {
        continue;
      }
      if (selectedKey.length() > 0) {
        request->send(400, "application/json", "{\"error\":\"only one device param can be updated per request\"}");
        return;
      }
      selectedKey = kDeviceParamKeys[i];
      selectedValue = param->value();
    }

    if (selectedKey.length() == 0) {
      request->send(400, "application/json", "{\"error\":\"missing device param\"}");
      return;
    }

    String errorMessage;
    if (!applyDeviceParamValue(selectedKey, selectedValue, errorMessage)) {
      String response = "{\"error\":\"" + errorMessage + "\"}";
      request->send(400, "application/json", response);
      return;
    }

    StaticJsonDocument<128> doc;
    doc["status"] = "ok";
    doc["key"] = selectedKey;
    doc["value"] = selectedValue;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", buildRuntimeStatusJson());
  });
  
  server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("value", true)) {
      int brightness = request->getParam("value", true)->value().toInt();
      if (brightness >= 0 && brightness <= 255) {
        RuntimeCommandBus::RuntimeCommand* command =
          RuntimeCommandBus::createCommand(RuntimeCommandBus::RuntimeCommandSource::HTTP, 0);
        if (command == nullptr) {
          request->send(500, "application/json", "{\"error\":\"out of memory\"}");
          return;
        }
        command->type = RuntimeCommandBus::RuntimeCommandType::BRIGHTNESS;
        command->intValue1 = brightness;
        if (!RuntimeCommandBus::enqueue(command)) {
          RuntimeCommandBus::destroyCommand(command);
          request->send(503, "application/json", "{\"error\":\"device busy\"}");
          return;
        }
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

      RuntimeCommandBus::RuntimeCommand* command =
        RuntimeCommandBus::createCommand(RuntimeCommandBus::RuntimeCommandSource::HTTP, 0);
      if (command == nullptr) {
        request->send(500, "application/json", "{\"error\":\"out of memory\"}");
        return;
      }
      command->type = RuntimeCommandBus::RuntimeCommandType::TEXT;
      command->stringValue1 = text;
      command->intValue1 = x;
      command->intValue2 = y;
      if (!RuntimeCommandBus::enqueue(command)) {
        RuntimeCommandBus::destroyCommand(command);
        request->send(503, "application/json", "{\"error\":\"device busy\"}");
        return;
      }

      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing content parameter\"}");
    }
  });
}

void WebServer::handleLoop() {
  if (!gRuntimeSyncServerStarted || !WiFiManager::isConfigMode()) {
    return;
  }

  WiFiClient client = gRuntimeSyncServer.available();
  if (!client) {
    return;
  }

  handleSyncServerClient(client);
  delay(1);
  client.stop();
}
