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
  {"clockwise_pokedex", "图鉴"}
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
  if (preset == AMBIENT_PRESET_COSMIC_KALE) {
    return "cosmic_kale";
  }
  if (preset == AMBIENT_PRESET_VOID_FIRE) {
    return "void_fire";
  }
  if (preset == AMBIENT_PRESET_DEEP_SPACE_NEBULA) {
    return "deep_space_nebula";
  }
  return "";
}

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

void sendDeviceParamsResponse(AsyncWebServerRequest* request) {
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
  request->send(200, "application/json", response);
}

bool applyDeviceParamValue(const String& key, const String& value, String& errorMessage) {
  bool requiresDisplayRebuild = false;
  uint32_t parsedNumber = 0;
  bool parsedBool = false;

  if (key == "displayBright") {
    if (!parseUnsignedValue(value, 0, 255, parsedNumber)) {
      errorMessage = "displayBright must be 0-255";
      return false;
    }
    DisplayManager::setBrightness(static_cast<int>(parsedNumber));
  } else if (key == "brightnessDay") {
    if (!parseUnsignedValue(value, 0, 255, parsedNumber)) {
      errorMessage = "brightnessDay must be 0-255";
      return false;
    }
    ConfigManager::deviceParamsConfig.brightnessDay = static_cast<uint8_t>(parsedNumber);
  } else if (key == "brightnessNight") {
    if (!parseUnsignedValue(value, 0, 255, parsedNumber)) {
      errorMessage = "brightnessNight must be 0-255";
      return false;
    }
    ConfigManager::deviceParamsConfig.brightnessNight = static_cast<uint8_t>(parsedNumber);
  } else if (key == "displayRotation") {
    if (!parseUnsignedValue(value, 0, 3, parsedNumber)) {
      errorMessage = "displayRotation must be 0-3";
      return false;
    }
    ConfigManager::deviceParamsConfig.displayRotation = static_cast<uint8_t>(parsedNumber);
    requiresDisplayRebuild = true;
  } else if (key == "swapBlueGreen") {
    if (!parseBooleanSwitch(value, parsedBool)) {
      errorMessage = "swapBlueGreen must be 0 or 1";
      return false;
    }
    ConfigManager::deviceParamsConfig.swapBlueGreen = parsedBool;
    requiresDisplayRebuild = true;
  } else if (key == "swapBlueRed") {
    if (!parseBooleanSwitch(value, parsedBool)) {
      errorMessage = "swapBlueRed must be 0 or 1";
      return false;
    }
    ConfigManager::deviceParamsConfig.swapBlueRed = parsedBool;
    requiresDisplayRebuild = true;
  } else if (key == "clkphase") {
    if (!parseBooleanSwitch(value, parsedBool)) {
      errorMessage = "clkphase must be 0 or 1";
      return false;
    }
    ConfigManager::deviceParamsConfig.clkphase = parsedBool;
    requiresDisplayRebuild = true;
  } else if (key == "driver") {
    if (!parseUnsignedValue(value, 0, 5, parsedNumber)) {
      errorMessage = "driver must be 0-5";
      return false;
    }
    ConfigManager::deviceParamsConfig.driver = static_cast<uint8_t>(parsedNumber);
    requiresDisplayRebuild = true;
  } else if (key == "i2cSpeed") {
    if (!parseUnsignedValue(value, 8000000, 20000000, parsedNumber) ||
        (parsedNumber != 8000000 && parsedNumber != 16000000 && parsedNumber != 20000000)) {
      errorMessage = "i2cSpeed must be 8000000, 16000000 or 20000000";
      return false;
    }
    ConfigManager::deviceParamsConfig.i2cSpeed = parsedNumber;
    requiresDisplayRebuild = true;
  } else if (key == "E_pin") {
    if (!parseUnsignedValue(value, 0, 32, parsedNumber)) {
      errorMessage = "E_pin must be 0-32";
      return false;
    }
    ConfigManager::deviceParamsConfig.E_pin = static_cast<uint8_t>(parsedNumber);
    requiresDisplayRebuild = true;
  } else if (key == "nightStart") {
    if (!isValidHourMinuteText(value)) {
      errorMessage = "nightStart must be HH:MM";
      return false;
    }
    memset(ConfigManager::deviceParamsConfig.nightStart, 0, sizeof(ConfigManager::deviceParamsConfig.nightStart));
    value.toCharArray(ConfigManager::deviceParamsConfig.nightStart, sizeof(ConfigManager::deviceParamsConfig.nightStart));
  } else if (key == "nightEnd") {
    if (!isValidHourMinuteText(value)) {
      errorMessage = "nightEnd must be HH:MM";
      return false;
    }
    memset(ConfigManager::deviceParamsConfig.nightEnd, 0, sizeof(ConfigManager::deviceParamsConfig.nightEnd));
    value.toCharArray(ConfigManager::deviceParamsConfig.nightEnd, sizeof(ConfigManager::deviceParamsConfig.nightEnd));
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

  if (requiresDisplayRebuild) {
    DisplayManager::applyDeviceParams(false);
    WebSocketHandler::restoreDisplayForCurrentMode();
  }

  if (key == "ntpServer") {
    WiFiManager::refreshTimeSync();
  }
  DisplayManager::refreshScheduledBrightness();

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

String buildWifiNetworkListHtml() {
  String listHtml;
  size_t count = WiFiManager::getScannedNetworkCount();

  if (count == 0) {
    if (WiFiManager::isNetworkScanRunning()) {
      listHtml += R"HTML(
        <div class="empty-state">
          <strong>正在扫描附近 WiFi</strong>
          <span>设备正在后台扫描热点，页面会自动刷新。请稍候 1-3 秒。</span>
        </div>
      )HTML";
      return listHtml;
    }

    listHtml += R"HTML(
      <div class="empty-state">
        <strong>页面已经打开成功</strong>
        <span>为避免热点门户卡死，设备不会在热点刚启动时立刻扫描。请点一次“开始扫描 WiFi”，也可以直接手动输入 SSID。</span>
      </div>
    )HTML";
    return listHtml;
  }

  for (size_t i = 0; i < count; i++) {
    WiFiScanResultItem item = WiFiManager::getScannedNetwork(i);
    String ssid = item.ssid;
    if (ssid.length() == 0) {
      continue;
    }

    listHtml += "<button type=\"button\" class=\"wifi-item\" data-ssid=\"";
    listHtml += escapeHtml(ssid);
    listHtml += "\">";
    listHtml += "<span class=\"wifi-name\">";
    listHtml += escapeHtml(ssid);
    listHtml += "</span>";
    listHtml += "<span class=\"wifi-meta\">";
    listHtml += item.secure ? "加密网络" : "开放网络";
    listHtml += " · ";
    listHtml += String(wifiSignalPercentFromRssi(item.rssi));
    listHtml += "%";
    listHtml += "</span>";
    listHtml += "</button>";
  }

  return listHtml;
}

String buildWifiConfigPortalPage() {
  String portalSsid = WiFiManager::getConfigPortalSSID();
  String portalIp = WiFiManager::getConfigPortalIP();
  String networkListHtml = buildWifiNetworkListHtml();
  bool scanRunning = WiFiManager::isNetworkScanRunning();
  String scanButtonText = scanRunning
    ? "正在扫描..."
    : (WiFiManager::getScannedNetworkCount() > 0
    ? "重新扫描 WiFi"
    : "开始扫描 WiFi");

  String html;
  html.reserve(9000);
  html += R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Glowxel WiFi 配网</title>
)HTML";
  if (scanRunning) {
    html += R"HTML(<meta http-equiv="refresh" content="2">)HTML";
  }
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
    .wifi-list { display: block; margin-bottom: 14px; }
    .wifi-item { width: 100%; display: block; margin-bottom: 10px; padding: 12px 14px; text-align: left; border: 1px solid #dce7f5; border-radius: 12px; background: #f7fafe; color: #142033; }
    .wifi-name { display: block; font-size: 14px; font-weight: 700; margin-bottom: 4px; word-break: break-all; }
    .wifi-meta { display: block; font-size: 12px; color: #5c6c84; }
    .empty-state { padding: 14px; border-radius: 12px; border: 1px dashed #c6d6ea; background: #f8fbff; color: #5c6c84; font-size: 13px; line-height: 1.6; }
    .empty-state strong { display: block; color: #142033; margin-bottom: 6px; }
    label { display: block; margin: 12px 0 8px; color: #50627d; font-size: 13px; font-weight: 600; }
    input { width: 100%; box-sizing: border-box; padding: 13px 14px; border: 1px solid #cfdced; border-radius: 12px; background: #fff; color: #142033; font-size: 14px; }
    .actions { margin-top: 14px; }
    .btn { width: 100%; box-sizing: border-box; display: block; padding: 13px 16px; border-radius: 12px; border: 1px solid #1e5eff; background: #1e5eff; color: #fff; text-align: center; text-decoration: none; font-size: 14px; font-weight: 700; }
    .btn-secondary { margin-bottom: 10px; background: #eef4ff; color: #1e5eff; }
    .hint { margin-top: 14px; color: #5c6c84; font-size: 12px; line-height: 1.6; }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>Glowxel WiFi 配网</h1>
      <p class="sub">连上设备热点后，在这里选择你的 2.4GHz WiFi 并保存。</p>
      <div class="meta">设备热点：<strong>)HTML";
  html += escapeHtml(portalSsid);
  html += R"HTML(</strong><br>配网页地址：<strong>)HTML";
  html += escapeHtml(portalIp);
  html += R"HTML(</strong></div>
      <div class="notice">仅支持 2.4GHz WiFi。为保证系统登录页稳定打开，设备不会在热点刚启动时立刻扫描，请手动点一次扫描。</div>
      <div class="section-title">附近 WiFi</div>
      <div class="wifi-list">
)HTML";
  html += networkListHtml;
  html += R"HTML(
      </div>
      <form method="post" action="/configure-wifi">
        <label for="ssid-input">WiFi 名称 (SSID)</label>
        <input id="ssid-input" name="ssid" maxlength="32" autocomplete="off" required placeholder="输入 2.4GHz WiFi 名称">
        <label for="password-input">WiFi 密码</label>
        <input id="password-input" name="password" maxlength="64" type="password" autocomplete="off" placeholder="输入 WiFi 密码，开放网络可留空">
        <div class="actions">
          <a class="btn btn-secondary" href="/scan-wifi">)HTML";
  html += scanButtonText;
  html += R"HTML(</a>
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
      var items = document.querySelectorAll(".wifi-item[data-ssid]");
      for (var i = 0; i < items.length; i++) {
        items[i].addEventListener("click", function () {
          var ssid = this.getAttribute("data-ssid") || "";
          var ssidInput = document.getElementById("ssid-input");
          if (ssidInput) {
            ssidInput.value = ssid;
            ssidInput.focus();
          }
        });
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
  <title>Glowxel WiFi 配置已保存</title>
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

String buildConfigPortalRootUrl() {
  return "http://" + WiFiManager::getConfigPortalIP() + "/";
}

bool isConfigPortalHostRequest(AsyncWebServerRequest* request) {
  String requestHost = request->host();
  String portalHost = WiFiManager::getConfigPortalIP();
  return requestHost == portalHost || requestHost == (portalHost + ":80");
}

void addConfigPortalNoCacheHeaders(AsyncWebServerResponse* response) {
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
}

void redirectToConfigPortalRoot(AsyncWebServerRequest* request) {
  String location = buildConfigPortalRootUrl();
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
      { id: "clockwise_pokedex", name: "图鉴" }
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
      if (effectId === "rain_scene") {
        sendJson({
          cmd: "set_ambient_effect",
          preset: effectId,
          speed: 6,
          density: 72,
          color: { r: 100, g: 200, b: 255 },
          loop: true
        });
        currentMode = "ambient_effect";
        currentEffectId = effectId;
        setModeButtons(currentMode);
        renderEffectChips();
        setStatus("特效已切换", true);
        window.setTimeout(syncStatus, 220);
        return;
      }

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

  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request){
    if (WiFiManager::isConfigMode()) {
      if (!isConfigPortalHostRequest(request)) {
        redirectToConfigPortalRoot(request);
        return;
      }
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

    WiFiManager::scanNearbyNetworks();
    redirectToConfigPortalRoot(request);
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

  server.on("/themes", HTTP_GET, [](AsyncWebServerRequest *request){
    if (WiFiManager::isConfigMode()) {
      sendConfigPortalPage(request);
      return;
    }
    request->send(200, "text/html; charset=utf-8", buildThemeBrowserPage());
  });

  server.on("/effects", HTTP_GET, [](AsyncWebServerRequest *request){
    if (WiFiManager::isConfigMode()) {
      sendConfigPortalPage(request);
      return;
    }
    request->send(200, "text/html; charset=utf-8", buildAmbientEffectsPage());
  });

  auto captiveProbeHandler = [](AsyncWebServerRequest *request) {
    if (WiFiManager::isConfigMode()) {
      redirectToConfigPortalRoot(request);
      return;
    }
    request->send(204);
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
    if (WiFiManager::isConfigMode()) {
      redirectToConfigPortalRoot(request);
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
    Serial.println("收到状态查询请求");
    StaticJsonDocument<640> doc;
    const char* mode = "unknown";
    if (DisplayManager::currentMode == MODE_CLOCK) {
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
    doc["effectDensity"] = DisplayManager::ambientEffectConfig.density;
    JsonObject effectColor = doc.createNestedObject("effectColor");
    effectColor["r"] = DisplayManager::ambientEffectConfig.colorR;
    effectColor["g"] = DisplayManager::ambientEffectConfig.colorG;
    effectColor["b"] = DisplayManager::ambientEffectConfig.colorB;
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
        ConfigManager::saveDeviceParamsConfig();
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
