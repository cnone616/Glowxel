#include "wifi_manager.h"
#include "config_manager.h"
#include "display_manager.h"
#include "websocket_handler.h"
#include <esp_wifi.h>
#include <time.h>
#include <math.h>

namespace {
const long kGmtOffsetSec = 8 * 3600;
const int kDaylightOffsetSec = 0;
const unsigned long kStaConnectTimeoutMs = 20000;
const unsigned long kStaReconnectDelayMs = 1500;
const unsigned long kRuntimeSettingsWindowDefaultHoldMs = 120000;
const unsigned long kNtpFirstRetryIntervalMs = 20000;
const unsigned long kNtpRetryIntervalMs = 20000;
const unsigned long kWifiScanTimeoutMs = 18000;
const uint32_t kWifiScanMinMsPerChannel = 80;
const uint32_t kWifiScanMaxMsPerChannel = 280;
const uint8_t kWifiScanHomeChannelDwellMs = 60;
const time_t kUnixTimeSyncedThreshold = 1700000000;
const IPAddress kConfigPortalIp(192, 168, 4, 1);
const IPAddress kConfigPortalGateway(192, 168, 4, 1);
const IPAddress kConfigPortalSubnet(255, 255, 255, 0);
uint8_t s_ntpRequestAttemptCount = 0;

void requestTimeSync() {
  configTime(
    kGmtOffsetSec,
    kDaylightOffsetSec,
    ConfigManager::deviceParamsConfig.ntpServer
  );
}

bool hasValidSystemTime() {
  time_t now = time(nullptr);
  return now >= kUnixTimeSyncedThreshold;
}

void resetNtpRetrySchedule() {
  s_ntpRequestAttemptCount = 0;
}

void noteNtpSyncRequest() {
  if (s_ntpRequestAttemptCount < 255) {
    s_ntpRequestAttemptCount++;
  }
}

unsigned long currentNtpRetryIntervalMs() {
  if (s_ntpRequestAttemptCount == 0) {
    return 0;
  }

  if (s_ntpRequestAttemptCount == 1) {
    return kNtpFirstRetryIntervalMs;
  }

  return kNtpRetryIntervalMs;
}

void disableStationPowerSave(const char* stage) {
  (void)stage;
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);
}

void applyStationRadioTuning(const char* stage, bool logLinkMetrics) {
  (void)stage;
  (void)logLinkMetrics;
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
}

String buildConfigPortalSSID() {
  uint64_t chipId = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06llX", static_cast<unsigned long long>(chipId & 0xFFFFFFULL));
  return "Glowxel PixelBoard-" + String(suffix);
}

void copyScanItem(WiFiScanResultItem& item, const String& ssid, int32_t rssi, bool secure) {
  memset(item.ssid, 0, sizeof(item.ssid));
  ssid.toCharArray(item.ssid, sizeof(item.ssid));
  item.rssi = rssi;
  item.secure = secure;
}

void stopWifiScanDriverIfNeeded(const char* reason) {
  (void)reason;
  esp_err_t err = esp_wifi_scan_stop();
  if (err == ESP_OK) {
    return;
  }
  if (err == ESP_ERR_WIFI_STATE || err == ESP_ERR_WIFI_NOT_STARTED || err == ESP_ERR_WIFI_NOT_INIT) {
    return;
  }
}

const char* wifiEventLogLabel(arduino_event_id_t eventId) {
  switch (eventId) {
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      return "SCAN_DONE";
    case ARDUINO_EVENT_WIFI_AP_START:
      return "AP_START";
    case ARDUINO_EVENT_WIFI_AP_STOP:
      return "AP_STOP";
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      return "AP_STACONNECTED";
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      return "AP_STADISCONNECTED";
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      return "STA_DISCONNECTED";
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      return "STA_CONNECTED";
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      return "STA_GOT_IP";
    default:
      return nullptr;
  }
}

const char* scanPhaseLabel(WifiScanPhase phase) {
  switch (phase) {
    case WifiScanPhase::IDLE:
      return "IDLE";
    case WifiScanPhase::REQUESTED:
      return "REQUESTED";
    case WifiScanPhase::RUNNING:
      return "RUNNING";
    default:
      return "UNKNOWN";
  }
}
}

// 画WiFi扇形图标，弧线两端圆头
static void drawWifiIcon(int cx, int cy, int maxR, uint16_t color) {
  auto* d = DisplayManager::dma_display;
  const int gap = 7;
  const float spread = 0.66f; // ~52° 扇形宽度
  const int thickness = 3;

  // 底部小圆点
  d->fillCircle(cx, cy, 2, color);

  // 3层弧线，从外到内
  for (int i = 0; i < 3; i++) {
    int radius = maxR - i * gap;
    if (radius <= 2) break;
    float midR = radius - thickness * 0.5f; // 弧线中心半径

    for (int dy = -radius; dy <= 0; dy++) {
      for (int dx = -radius; dx <= radius; dx++) {
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist >= radius - thickness && dist <= radius) {
          float angle = atan2f(-dy, abs(dx));
          if (angle > spread) {
            int px = cx + dx, py = cy + dy;
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
              d->drawPixel(px, py, color);
            }
          }
        }
      }
    }

    // 两端圆头：在扇形边界角度处画小圆
    float capR = thickness * 0.5f;
    // 左端和右端的圆头
    for (int side = -1; side <= 1; side += 2) {
      float capCx = cx + side * midR * cosf(spread);
      float capCy = cy - midR * sinf(spread);
      for (int dy2 = -(int)capR - 1; dy2 <= (int)capR + 1; dy2++) {
        for (int dx2 = -(int)capR - 1; dx2 <= (int)capR + 1; dx2++) {
          float dd = sqrtf(dx2 * dx2 + dy2 * dy2);
          if (dd <= capR) {
            int px = (int)(capCx + dx2 + 0.5f);
            int py = (int)(capCy + dy2 + 0.5f);
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
              d->drawPixel(px, py, color);
            }
          }
        }
      }
    }
  }
}

// 在WiFi图标右下角画状态徽章
static void drawStatusBadge(int cx, int cy, bool success) {
  auto* d = DisplayManager::dma_display;
  const int maxR = 23, gap = 8;
  // 和 HTML 调试页一致: bx = cx + offsetX, by = cy - (maxR - gap*2) + offsetY
  int bx = cx + 5;
  int by = cy - (maxR - gap * 2) + 9;

  if (success) {
    uint16_t green = d->color565(34, 197, 94);
    uint16_t white = d->color565(255, 255, 255);
    d->fillCircle(bx, by, 4, green);
    // 白色勾 ✓
    d->drawPixel(bx - 2, by,     white);
    d->drawPixel(bx - 1, by + 1, white);
    d->drawPixel(bx,     by + 2, white);
    d->drawPixel(bx + 1, by + 1, white);
    d->drawPixel(bx + 2, by,     white);
    d->drawPixel(bx + 3, by - 1, white);
  } else {
    uint16_t red = d->color565(239, 68, 68);
    uint16_t white = d->color565(255, 255, 255);
    d->fillCircle(bx, by, 4, red);
    // 白色 X
    for (int i = -2; i <= 2; i++) {
      d->drawPixel(bx + i, by + i, white);
      d->drawPixel(bx + i, by - i, white);
    }
  }
}

static void showWiFiConnectingScreen() {
  // 保留接口占位，但恢复到“连接时不主动切换显示”的旧稳定策略。
}

static void showWiFiConnectedScreen(const String& ip) {
  if (DisplayManager::dma_display == nullptr) {
    return;
  }

  DisplayManager::dma_display->clearScreen();
  drawWifiIcon(32, 32, 23, DisplayManager::dma_display->color565(59, 130, 246));
  drawStatusBadge(32, 32, true);
  DisplayManager::drawTinyTextCentered(ip.c_str(), 46, DisplayManager::dma_display->color565(150, 150, 150));
  DisplayManager::dma_display->flipDMABuffer();
}

DNSServer WiFiManager::dns_server;
WiFiScanResultItem* WiFiManager::scanned_networks = nullptr;
size_t WiFiManager::scanned_network_count = 0;
size_t WiFiManager::scanned_network_capacity = 0;
bool WiFiManager::config_mode = false;
bool WiFiManager::sta_connecting = false;
bool WiFiManager::sta_reconnect_pending = false;
String WiFiManager::saved_ssid = "";
String WiFiManager::saved_password = "";
String WiFiManager::config_portal_ssid = "";
unsigned long WiFiManager::last_ntp_retry_at = 0;
unsigned long WiFiManager::portal_restart_at = 0;
unsigned long WiFiManager::sta_connect_started_at = 0;
unsigned long WiFiManager::sta_connect_deadline_at = 0;
unsigned long WiFiManager::sta_reconnect_due_at = 0;
unsigned long WiFiManager::runtime_settings_window_until = 0;
bool WiFiManager::ntp_sync_logged = false;
bool WiFiManager::time_synced_once = false;
bool WiFiManager::runtime_access_ap_started = false;
WifiScanPhase WiFiManager::scan_phase = WifiScanPhase::IDLE;
unsigned long WiFiManager::scan_started_at = 0;
volatile bool WiFiManager::scan_result_ready = false;
volatile int WiFiManager::scan_result_code = WIFI_SCAN_FAILED;
uint32_t WiFiManager::scan_attempt_counter = 0;
uint32_t WiFiManager::active_scan_attempt_id = 0;

void WiFiManager::init() {
  WiFi.persistent(false);
  WiFi.onEvent(WiFiManager::handleWiFiEvent);
  setupWiFi();
}

void WiFiManager::resetScanState() {
  clearScannedNetworkCache();
  scan_started_at = 0;
  scan_result_ready = false;
  scan_result_code = WIFI_SCAN_FAILED;
  active_scan_attempt_id = 0;
  setScanPhase(WifiScanPhase::IDLE, "扫描状态重置");
}

void WiFiManager::clearScannedNetworkCache() {
  if (scanned_networks != nullptr) {
    free(scanned_networks);
    scanned_networks = nullptr;
  }
  scanned_network_count = 0;
  scanned_network_capacity = 0;
}

void WiFiManager::setScanPhase(WifiScanPhase nextPhase, const char* reason) {
  (void)reason;
  if (scan_phase == nextPhase) {
    return;
  }
  scan_phase = nextPhase;
}

void WiFiManager::beginStationConnect() {
  config_mode = false;
  sta_connecting = true;
  sta_reconnect_pending = false;
  portal_restart_at = 0;
  sta_connect_started_at = millis();
  sta_connect_deadline_at = sta_connect_started_at + kStaConnectTimeoutMs;
  sta_reconnect_due_at = 0;
  last_ntp_retry_at = 0;
  ntp_sync_logged = false;
  time_synced_once = false;
  runtime_settings_window_until = 0;
  runtime_access_ap_started = false;
  config_portal_ssid = "";
  resetNtpRetrySchedule();

  // 对齐旧稳定路径：STA 冷启动时不做额外 radio 清理和熄屏动作。
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);
  disableStationPowerSave("beginStationConnect()");
  applyStationRadioTuning("beginStationConnect()", false);
  WiFi.begin(saved_ssid.c_str(), saved_password.c_str());
}

void WiFiManager::finalizeStationConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  sta_connecting = false;
  sta_reconnect_pending = false;
  config_mode = false;
  portal_restart_at = 0;
  sta_connect_started_at = 0;
  sta_connect_deadline_at = 0;
  sta_reconnect_due_at = 0;

  requestTimeSync();
  resetNtpRetrySchedule();
  noteNtpSyncRequest();
  last_ntp_retry_at = millis();
  ntp_sync_logged = false;
  time_synced_once = false;

  String ip = WiFi.localIP().toString();
  disableStationPowerSave("finalizeStationConnected()");
  applyStationRadioTuning("finalizeStationConnected()", true);
  showWiFiConnectedScreen(ip);
}

void WiFiManager::setupWiFi() {
  ConfigManager::preferences.begin("wifi", false);
  saved_ssid = ConfigManager::preferences.getString("ssid", "");
  saved_password = ConfigManager::preferences.getString("password", "");
  ConfigManager::preferences.end();

  sta_connecting = false;
  sta_reconnect_pending = false;
  sta_connect_started_at = 0;
  sta_connect_deadline_at = 0;
  sta_reconnect_due_at = 0;
  portal_restart_at = 0;
  last_ntp_retry_at = 0;
  ntp_sync_logged = false;
  time_synced_once = false;
  config_portal_ssid = "";
  resetNtpRetrySchedule();
  resetScanState();

  if (saved_ssid.length() == 0) {
    startConfigPortal();
    return;
  }

  beginStationConnect();
}

void WiFiManager::startConfigPortal() {
  config_mode = true;
  sta_connecting = false;
  sta_reconnect_pending = false;
  portal_restart_at = 0;
  last_ntp_retry_at = 0;
  ntp_sync_logged = false;
  time_synced_once = false;
  sta_connect_started_at = 0;
  sta_connect_deadline_at = 0;
  sta_reconnect_due_at = 0;
  runtime_settings_window_until = 0;
  runtime_access_ap_started = false;
  config_portal_ssid = buildConfigPortalSSID();
  resetNtpRetrySchedule();

  stopWifiScanDriverIfNeeded("启动配网页前");
  resetScanState();
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(kConfigPortalIp, kConfigPortalGateway, kConfigPortalSubnet);
  bool started = WiFi.softAP(config_portal_ssid.c_str());
  if (!started) {
    Serial.println("[WiFi] 热点启动失败");
  }

  dns_server.stop();
  dns_server.start(53, "*", kConfigPortalIp);
  WiFi.scanDelete();

  if (DisplayManager::dma_display != nullptr) {
    DisplayManager::dma_display->clearScreen();
    drawWifiIcon(32, 24, 18, DisplayManager::dma_display->color565(59, 130, 246));
    drawStatusBadge(32, 24, false);
    DisplayManager::drawTinyTextCentered("WIFI SETUP", 42, DisplayManager::dma_display->color565(220, 220, 220));
    DisplayManager::drawTinyTextCentered(config_portal_ssid.c_str(), 50, DisplayManager::dma_display->color565(150, 210, 255));
    DisplayManager::drawTinyTextCentered("192.168.4.1", 58, DisplayManager::dma_display->color565(150, 150, 150));
  }
}

void WiFiManager::stopConfigPortal() {
  config_mode = false;
  portal_restart_at = 0;
  dns_server.stop();
  resetScanState();
  stopWifiScanDriverIfNeeded("关闭配网页前");
  WiFi.scanDelete();
  WiFi.softAPdisconnect(true);
  runtime_access_ap_started = false;
}

void WiFiManager::ensureRuntimeSettingsAccessPoint() {
  if (config_mode || runtime_access_ap_started) {
    return;
  }

  if (config_portal_ssid.length() == 0) {
    config_portal_ssid = buildConfigPortalSSID();
  }

  WiFi.softAPConfig(kConfigPortalIp, kConfigPortalGateway, kConfigPortalSubnet);
  bool started = WiFi.softAP(config_portal_ssid.c_str());
  if (!started) {
    Serial.println("[WiFi] 运行态设置热点启动失败");
    return;
  }

  runtime_access_ap_started = true;
}

void WiFiManager::scanNearbyNetworks() {
  if (!config_mode) {
    return;
  }

  if (scan_phase != WifiScanPhase::IDLE) {
    return;
  }

  setScanPhase(WifiScanPhase::REQUESTED, "收到门户扫描请求");
}

void WiFiManager::finalizeNetworkScan(int networkCount) {
  if (networkCount < 0) {
    Serial.printf("[WiFi] 扫描失败，scan#%lu，结果码=%d\n", (unsigned long)active_scan_attempt_id, networkCount);
    stopWifiScanDriverIfNeeded("扫描失败收尾");
    WiFi.scanDelete();
    active_scan_attempt_id = 0;
    setScanPhase(WifiScanPhase::IDLE, "扫描失败");
    return;
  }

  size_t nextCapacity = static_cast<size_t>(networkCount);
  WiFiScanResultItem* nextResults = nullptr;
  size_t nextCount = 0;

  if (nextCapacity > 0) {
    nextResults = static_cast<WiFiScanResultItem*>(malloc(sizeof(WiFiScanResultItem) * nextCapacity));
    if (nextResults == nullptr) {
      Serial.printf("[WiFi] 扫描结果缓存分配失败，保留上一次结果，所需条目=%u\n",
                    (unsigned int)nextCapacity);
      WiFi.scanDelete();
      active_scan_attempt_id = 0;
      setScanPhase(WifiScanPhase::IDLE, "扫描结果缓存分配失败");
      return;
    }
  }

  for (int i = 0; i < networkCount; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) {
      continue;
    }

    int32_t rssi = WiFi.RSSI(i);
    bool secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    int targetIndex = -1;

    for (size_t j = 0; j < nextCount; j++) {
      if (strcmp(nextResults[j].ssid, ssid.c_str()) == 0) {
        targetIndex = static_cast<int>(j);
        break;
      }
    }

    if (targetIndex >= 0) {
      if (rssi > nextResults[targetIndex].rssi) {
        copyScanItem(nextResults[targetIndex], ssid, rssi, secure);
      }
      continue;
    }

    if (nextCount < nextCapacity) {
      copyScanItem(nextResults[nextCount], ssid, rssi, secure);
      nextCount++;
      continue;
    }
  }

  for (size_t i = 0; i < nextCount; i++) {
    for (size_t j = i + 1; j < nextCount; j++) {
      if (nextResults[j].rssi > nextResults[i].rssi) {
        WiFiScanResultItem temp = nextResults[i];
        nextResults[i] = nextResults[j];
        nextResults[j] = temp;
      }
    }
  }

  clearScannedNetworkCache();
  scanned_networks = nextResults;
  scanned_network_count = nextCount;
  scanned_network_capacity = nextCapacity;

  WiFi.scanDelete();
  active_scan_attempt_id = 0;
  setScanPhase(WifiScanPhase::IDLE, "扫描结果已整理完成");
}

void WiFiManager::handleWiFiEvent(arduino_event_t* event) {
  if (event == nullptr) {
    return;
  }

  if (event->event_id == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
    if (!config_mode) {
      startRuntimeSettingsWindow(kRuntimeSettingsWindowDefaultHoldMs);
    }
    return;
  }

  if (event->event_id == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
    return;
  }

  if (event->event_id == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    if (!config_mode && saved_ssid.length() > 0) {
      if (sta_connecting) {
        return;
      }
      sta_connecting = false;
      sta_reconnect_pending = true;
      sta_reconnect_due_at = millis() + kStaReconnectDelayMs;
    }
    return;
  }

  if (event->event_id == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    return;
  }

  if (event->event_id != ARDUINO_EVENT_WIFI_SCAN_DONE) {
    return;
  }

  int result = event->event_info.wifi_scan_done.status == 0
    ? (int)event->event_info.wifi_scan_done.number
    : WIFI_SCAN_FAILED;

  if (!config_mode || scan_phase != WifiScanPhase::RUNNING) {
    WiFi.scanDelete();
    return;
  }

  scan_result_code = result;
  scan_result_ready = true;
}

void WiFiManager::saveConfigPortalCredentials(const String& ssid, const String& password) {
  ConfigManager::preferences.begin("wifi", false);
  ConfigManager::preferences.putString("ssid", ssid);
  ConfigManager::preferences.putString("password", password);
  ConfigManager::preferences.end();

  saved_ssid = ssid;
  saved_password = password;
}

void WiFiManager::schedulePortalRestart(unsigned long delayMs) {
  portal_restart_at = millis() + delayMs;

  if (DisplayManager::dma_display == nullptr) {
    return;
  }

  DisplayManager::dma_display->clearScreen();
  DisplayManager::drawTinyTextCentered("WIFI SAVED", 24, DisplayManager::dma_display->color565(100, 255, 100));
  DisplayManager::drawTinyTextCentered("REBOOTING..", 34, DisplayManager::dma_display->color565(150, 150, 150));
  DisplayManager::drawTinyTextCentered(saved_ssid.c_str(), 46, DisplayManager::dma_display->color565(150, 210, 255));
  DisplayManager::drawTinyTextCentered("2.4GHz ONLY", 56, DisplayManager::dma_display->color565(255, 210, 120));
}

void WiFiManager::refreshTimeSync() {
  if (config_mode || WiFi.status() != WL_CONNECTED) {
    return;
  }

  requestTimeSync();
  resetNtpRetrySchedule();
  noteNtpSyncRequest();
  last_ntp_retry_at = millis();
  ntp_sync_logged = false;
  time_synced_once = false;
}

bool WiFiManager::hasSavedCredentials() {
  return saved_ssid.length() > 0;
}

bool WiFiManager::isConfigMode() {
  return config_mode;
}

bool WiFiManager::isStationConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::isStationConnecting() {
  return sta_connecting;
}

bool WiFiManager::isPortalRestartScheduled() {
  return portal_restart_at != 0;
}

void WiFiManager::startRuntimeSettingsWindow(unsigned long holdMs) {
  if (holdMs == 0) {
    return;
  }
  runtime_settings_window_until = millis() + holdMs;
}

bool WiFiManager::isRuntimeSettingsWindowActive() {
  return runtime_settings_window_until != 0 &&
         static_cast<long>(runtime_settings_window_until - millis()) > 0;
}

WifiScanPhase WiFiManager::getScanPhase() {
  return scan_phase;
}

String WiFiManager::getDeviceIP() {
  return WiFi.localIP().toString();
}

String WiFiManager::getConnectedSSID() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.SSID();
  }
  return "";
}

String WiFiManager::getConfigPortalSSID() {
  return config_portal_ssid;
}

String WiFiManager::getConfigPortalIP() {
  return kConfigPortalIp.toString();
}

size_t WiFiManager::getScannedNetworkCount() {
  return scanned_network_count;
}

WiFiScanResultItem WiFiManager::getScannedNetwork(size_t index) {
  WiFiScanResultItem emptyItem = {};
  if (index >= scanned_network_count || scanned_networks == nullptr) {
    return emptyItem;
  }
  return scanned_networks[index];
}

bool WiFiManager::isNetworkScanRunning() {
  return scan_phase != WifiScanPhase::IDLE;
}

bool WiFiManager::isTimeSynced() {
  if (time_synced_once) {
    return true;
  }
  if (hasValidSystemTime()) {
    time_synced_once = true;
    return true;
  }
  return false;
}

void WiFiManager::showTimeSyncScreen() {
  if (WiFi.status() == WL_CONNECTED) {
    showWiFiConnectedScreen(WiFi.localIP().toString());
    return;
  }

  if (DisplayManager::dma_display == nullptr) {
    return;
  }

  DisplayManager::dma_display->clearScreen();
  drawWifiIcon(32, 28, 23, DisplayManager::dma_display->color565(59, 130, 246));
  drawStatusBadge(32, 28, true);
  DisplayManager::drawTinyTextCentered("SYNC TIME", 46, DisplayManager::dma_display->color565(220, 220, 220));
  DisplayManager::drawTinyTextCentered("WAIT", 53, DisplayManager::dma_display->color565(150, 150, 150));
  DisplayManager::dma_display->flipDMABuffer();
}

void WiFiManager::showStationConnectedScreen() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  showWiFiConnectedScreen(WiFi.localIP().toString());
}

void WiFiManager::tick() {
  if (config_mode) {
    dns_server.processNextRequest();
    last_ntp_retry_at = 0;
    ntp_sync_logged = false;
    time_synced_once = false;
    resetNtpRetrySchedule();

    if (scan_phase == WifiScanPhase::REQUESTED) {
      setScanPhase(WifiScanPhase::RUNNING, "开始底层异步扫描");
      scan_started_at = millis();
      scan_attempt_counter += 1;
      active_scan_attempt_id = scan_attempt_counter;
      scan_result_ready = false;
      scan_result_code = WIFI_SCAN_FAILED;
      stopWifiScanDriverIfNeeded("启动新扫描前");
      WiFi.scanDelete();
      wifi_scan_config_t config = {};
      config.ssid = nullptr;
      config.bssid = nullptr;
      config.channel = 0;
      config.show_hidden = false;
      config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
      config.scan_time.active.min = kWifiScanMinMsPerChannel;
      config.scan_time.active.max = kWifiScanMaxMsPerChannel;
      config.home_chan_dwell_time = kWifiScanHomeChannelDwellMs;
      esp_err_t err = esp_wifi_scan_start(&config, false);
      if (err != ESP_OK) {
        setScanPhase(WifiScanPhase::IDLE, "底层异步扫描启动失败");
        scan_started_at = 0;
        active_scan_attempt_id = 0;
        Serial.printf("[WiFi] 异步扫描启动失败，错误码=%d\n", (int)err);
      }
    }

    if (scan_phase == WifiScanPhase::RUNNING) {
      if (scan_result_ready) {
        int result = scan_result_code;
        scan_result_ready = false;
        scan_started_at = 0;
        finalizeNetworkScan(result);
      } else if (scan_started_at != 0 && millis() - scan_started_at > kWifiScanTimeoutMs) {
        scan_started_at = 0;
        scan_result_ready = false;
        scan_result_code = WIFI_SCAN_FAILED;
        stopWifiScanDriverIfNeeded("等待扫描完成超时");
        WiFi.scanDelete();
        Serial.printf("[WiFi] 等待扫描完成超时，scan#%lu，本次扫描已终止\n",
                      (unsigned long)active_scan_attempt_id);
        active_scan_attempt_id = 0;
        setScanPhase(WifiScanPhase::IDLE, "等待扫描完成超时");
      }
    }

    if (portal_restart_at != 0 && millis() >= portal_restart_at) {
      stopConfigPortal();
      ESP.restart();
    }
    return;
  }

  if (sta_connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      finalizeStationConnected();
      return;
    }

    const unsigned long now = millis();
    if (sta_connect_deadline_at != 0 && now >= sta_connect_deadline_at) {
      Serial.printf("[WiFi] STA 连接超时，已等待 %lu ms，切换到热点配网模式\n",
                    now - sta_connect_started_at);
      startConfigPortal();
    }
    return;
  }

  if (sta_reconnect_pending) {
    if (WiFi.status() == WL_CONNECTED) {
      finalizeStationConnected();
      return;
    }

    if (sta_reconnect_due_at != 0 && millis() >= sta_reconnect_due_at) {
      beginStationConnect();
    }
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    last_ntp_retry_at = 0;
    ntp_sync_logged = false;
    time_synced_once = false;
    resetNtpRetrySchedule();
    if (saved_ssid.length() > 0) {
      sta_reconnect_pending = true;
      sta_reconnect_due_at = millis() + kStaReconnectDelayMs;
    }
    return;
  }

  if (hasValidSystemTime()) {
    time_synced_once = true;
    if (!ntp_sync_logged) {
      ntp_sync_logged = true;
    }
    return;
  }

  const unsigned long now = millis();
  const unsigned long retryIntervalMs = currentNtpRetryIntervalMs();
  if (now - last_ntp_retry_at < retryIntervalMs) {
    return;
  }

  last_ntp_retry_at = now;
  noteNtpSyncRequest();
  ntp_sync_logged = false;
  // configTime() 会停止并重新初始化 SNTP，频繁调用会打断正在进行的同步流程。
  requestTimeSync();
}
