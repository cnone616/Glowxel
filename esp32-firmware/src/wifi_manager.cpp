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
const unsigned long kStaConnectTimeoutMs = 12000;
const unsigned long kNtpRetryIntervalMs = 15000;
const unsigned long kNtpFastRetryIntervalsMs[] = {1500, 3000, 5000};
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

  size_t fastRetryIndex = static_cast<size_t>(s_ntpRequestAttemptCount - 1);
  if (fastRetryIndex < (sizeof(kNtpFastRetryIntervalsMs) / sizeof(kNtpFastRetryIntervalsMs[0]))) {
    return kNtpFastRetryIntervalsMs[fastRetryIndex];
  }

  return kNtpRetryIntervalMs;
}

String buildConfigPortalSSID() {
  uint64_t chipId = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06llX", static_cast<unsigned long long>(chipId & 0xFFFFFFULL));
  return "Glowxel-" + String(suffix);
}

void copyScanItem(WiFiScanResultItem& item, const String& ssid, int32_t rssi, bool secure) {
  memset(item.ssid, 0, sizeof(item.ssid));
  ssid.toCharArray(item.ssid, sizeof(item.ssid));
  item.rssi = rssi;
  item.secure = secure;
}

void stopWifiScanDriverIfNeeded(const char* reason) {
  esp_err_t err = esp_wifi_scan_stop();
  if (err == ESP_OK) {
    Serial.printf("[WiFi] %s，已请求停止底层扫描\n", reason);
    return;
  }
  if (err == ESP_ERR_WIFI_STATE || err == ESP_ERR_WIFI_NOT_STARTED || err == ESP_ERR_WIFI_NOT_INIT) {
    return;
  }
  Serial.printf("[WiFi] %s，停止底层扫描失败: %d\n", reason, (int)err);
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
String WiFiManager::saved_ssid = "";
String WiFiManager::saved_password = "";
String WiFiManager::config_portal_ssid = "";
unsigned long WiFiManager::last_ntp_retry_at = 0;
unsigned long WiFiManager::portal_restart_at = 0;
unsigned long WiFiManager::sta_connect_started_at = 0;
unsigned long WiFiManager::sta_connect_deadline_at = 0;
bool WiFiManager::ntp_sync_logged = false;
bool WiFiManager::time_synced_once = false;
WifiScanPhase WiFiManager::scan_phase = WifiScanPhase::IDLE;
unsigned long WiFiManager::scan_started_at = 0;
volatile bool WiFiManager::scan_result_ready = false;
volatile int WiFiManager::scan_result_code = WIFI_SCAN_FAILED;
uint32_t WiFiManager::scan_attempt_counter = 0;
uint32_t WiFiManager::active_scan_attempt_id = 0;

void WiFiManager::init() {
  Serial.println("3. 连接WiFi...");
  Serial.printf("[WiFi] init: heap=%u bytes, mode=%d\n", ESP.getFreeHeap(), (int)WiFi.getMode());
  Serial.println("[WiFi] init: 调用 WiFi.persistent(false)");
  WiFi.persistent(false);
  Serial.println("[WiFi] init: WiFi.persistent(false) 完成");
  Serial.println("[WiFi] init: 注册 WiFi 事件回调");
  WiFi.onEvent(WiFiManager::handleWiFiEvent);
  Serial.println("[WiFi] init: WiFi.onEvent 完成，开始执行 setupWiFi()");
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
  if (scan_phase == nextPhase) {
    return;
  }

  Serial.printf("[WiFi] ScanPhase %s -> %s (%s)\n",
                scanPhaseLabel(scan_phase),
                scanPhaseLabel(nextPhase),
                reason == nullptr ? "no reason" : reason);
  scan_phase = nextPhase;
}

void WiFiManager::beginStationConnect() {
  config_mode = false;
  sta_connecting = true;
  portal_restart_at = 0;
  sta_connect_started_at = millis();
  sta_connect_deadline_at = sta_connect_started_at + kStaConnectTimeoutMs;
  last_ntp_retry_at = 0;
  ntp_sync_logged = false;
  time_synced_once = false;
  config_portal_ssid = "";
  resetNtpRetrySchedule();

  Serial.printf("[WiFi] STA 冷启动前: heap=%u bytes, mode=%d\n", ESP.getFreeHeap(), (int)WiFi.getMode());
  Serial.println("尝试连接到保存的WiFi: " + saved_ssid);
  Serial.printf("[WiFi] 进入非阻塞 STA 连接，超时窗口=%lu ms\n", kStaConnectTimeoutMs);
  // 对齐旧稳定路径：STA 冷启动时不做额外 radio 清理和熄屏动作。
  bool mode_ok = WiFi.mode(WIFI_STA);
  Serial.printf("[WiFi] WiFi.mode(WIFI_STA)=%d, 当前模式=%d, heap=%u bytes\n",
                mode_ok ? 1 : 0,
                (int)WiFi.getMode(),
                ESP.getFreeHeap());
  wl_status_t begin_status = WiFi.begin(saved_ssid.c_str(), saved_password.c_str());
  Serial.printf("[WiFi] WiFi.begin 已调用，返回=%d\n", (int)begin_status);
}

void WiFiManager::finalizeStationConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  sta_connecting = false;
  config_mode = false;
  portal_restart_at = 0;
  sta_connect_started_at = 0;
  sta_connect_deadline_at = 0;

  requestTimeSync();
  resetNtpRetrySchedule();
  noteNtpSyncRequest();
  last_ntp_retry_at = millis();
  ntp_sync_logged = false;
  time_synced_once = false;

  String ip = WiFi.localIP().toString();
  Serial.println("WiFi已连接");
  Serial.print("IP地址: ");
  Serial.println(ip);
  Serial.println("已发起网络时间同步，后台更新中");
  showWiFiConnectedScreen(ip);
}

void WiFiManager::setupWiFi() {
  Serial.println("[WiFi] setupWiFi: 打开 Preferences(wifi)");
  ConfigManager::preferences.begin("wifi", false);
  saved_ssid = ConfigManager::preferences.getString("ssid", "");
  saved_password = ConfigManager::preferences.getString("password", "");
  ConfigManager::preferences.end();
  Serial.printf("[WiFi] setupWiFi: 已读取凭据，ssid_length=%u, has_password=%d\n",
                (unsigned int)saved_ssid.length(),
                saved_password.length() > 0 ? 1 : 0);

  sta_connecting = false;
  sta_connect_started_at = 0;
  sta_connect_deadline_at = 0;
  portal_restart_at = 0;
  last_ntp_retry_at = 0;
  ntp_sync_logged = false;
  time_synced_once = false;
  config_portal_ssid = "";
  resetNtpRetrySchedule();
  resetScanState();

  if (saved_ssid.length() == 0) {
    Serial.println("未找到WiFi配置，进入热点配网模式");
    startConfigPortal();
    return;
  }

  beginStationConnect();
}

void WiFiManager::startConfigPortal() {
  config_mode = true;
  sta_connecting = false;
  portal_restart_at = 0;
  last_ntp_retry_at = 0;
  ntp_sync_logged = false;
  time_synced_once = false;
  sta_connect_started_at = 0;
  sta_connect_deadline_at = 0;
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

  Serial.println("\n=================================");
  Serial.println("WiFi 热点配网模式已启动");
  Serial.print("热点名称: ");
  Serial.println(config_portal_ssid);
  Serial.print("配网页地址: http://");
  Serial.println(getConfigPortalIP());
  Serial.printf("[WiFi] 当前模式=%d, heap=%u bytes\n", (int)WiFi.getMode(), ESP.getFreeHeap());
  Serial.println("仅支持 2.4GHz WiFi");
  Serial.println("=================================");

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
}

void WiFiManager::scanNearbyNetworks() {
  if (!config_mode) {
    return;
  }

  if (scan_phase != WifiScanPhase::IDLE) {
    Serial.println("[WiFi] 扫描正在进行中，忽略重复请求");
    return;
  }

  setScanPhase(WifiScanPhase::REQUESTED, "收到门户扫描请求");
  Serial.println("[WiFi] 已收到扫描请求，等待主循环启动异步扫描");
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

  Serial.printf("[WiFi] 扫描完成，scan#%lu，共发现 %d 个热点\n", (unsigned long)active_scan_attempt_id, networkCount);

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
  Serial.printf("[WiFi] 热点列表已整理，缓存热点=%u，当前可用堆内存: %u bytes\n",
                (unsigned int)scanned_network_count,
                ESP.getFreeHeap());
}

void WiFiManager::handleWiFiEvent(arduino_event_t* event) {
  if (event == nullptr) {
    return;
  }

  const char* label = wifiEventLogLabel(event->event_id);
  if (label != nullptr) {
    Serial.printf("[WiFi Event] %s, mode=%d, config_mode=%d, sta_connecting=%d, scan_phase=%s, active_scan=%lu\n",
                  label,
                  (int)WiFi.getMode(),
                  config_mode ? 1 : 0,
                  sta_connecting ? 1 : 0,
                  scanPhaseLabel(scan_phase),
                  (unsigned long)active_scan_attempt_id);
  }

  if (event->event_id == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
    Serial.printf("[WiFi Event] AP client connected, aid=%d\n",
                  (int)event->event_info.wifi_ap_staconnected.aid);
    return;
  }

  if (event->event_id == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
    Serial.printf("[WiFi Event] AP client disconnected, aid=%d\n",
                  (int)event->event_info.wifi_ap_stadisconnected.aid);
    return;
  }

  if (event->event_id == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    Serial.printf(
      "[WiFi Event] STA disconnected, millis=%lu, reason=%u, mode=%d, ws_clients=%u, current_mode=%d, business_mode=%s\n",
      millis(),
      (unsigned int)event->event_info.wifi_sta_disconnected.reason,
      (int)WiFi.getMode(),
      (unsigned int)WebSocketHandler::ws.count(),
      (int)DisplayManager::currentMode,
      DisplayManager::currentBusinessModeTag.c_str()
    );
    return;
  }

  if (event->event_id == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    Serial.printf("[WiFi Event] STA got IP: %s\n", WiFi.localIP().toString().c_str());
    return;
  }

  if (event->event_id != ARDUINO_EVENT_WIFI_SCAN_DONE) {
    return;
  }

  int result = event->event_info.wifi_scan_done.status == 0
    ? (int)event->event_info.wifi_scan_done.number
    : WIFI_SCAN_FAILED;

  if (!config_mode || scan_phase != WifiScanPhase::RUNNING) {
    Serial.printf("[WiFi] 收到过期扫描完成事件，状态=%d，数量=%d，释放扫描缓存\n",
                  (int)event->event_info.wifi_scan_done.status,
                  (int)event->event_info.wifi_scan_done.number);
    WiFi.scanDelete();
    return;
  }

  scan_result_code = result;
  scan_result_ready = true;
  Serial.printf("[WiFi] 收到扫描完成事件，状态=%d，数量=%d\n",
                (int)event->event_info.wifi_scan_done.status,
                (int)event->event_info.wifi_scan_done.number);
}

void WiFiManager::saveConfigPortalCredentials(const String& ssid, const String& password) {
  ConfigManager::preferences.begin("wifi", false);
  ConfigManager::preferences.putString("ssid", ssid);
  ConfigManager::preferences.putString("password", password);
  ConfigManager::preferences.end();

  saved_ssid = ssid;
  saved_password = password;
  Serial.println("WiFi 配置已保存");
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
  Serial.printf("已更新 NTP 服务器并重新发起同步: %s\n", ConfigManager::deviceParamsConfig.ntpServer);
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
      Serial.printf("[WiFi] 开始异步扫描附近热点，scan#%lu，当前可用堆内存: %u bytes\n",
                    (unsigned long)active_scan_attempt_id,
                    ESP.getFreeHeap());
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

  if (WiFi.status() != WL_CONNECTED) {
    last_ntp_retry_at = 0;
    ntp_sync_logged = false;
    time_synced_once = false;
    resetNtpRetrySchedule();
    return;
  }

  if (hasValidSystemTime()) {
    time_synced_once = true;
    if (!ntp_sync_logged) {
      ntp_sync_logged = true;
      Serial.println("NTP时间同步成功");
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
  Serial.printf("NTP 未同步，后台重试... request#%u, interval=%lu ms\n",
                (unsigned int)s_ntpRequestAttemptCount,
                retryIntervalMs);
  requestTimeSync();
}
