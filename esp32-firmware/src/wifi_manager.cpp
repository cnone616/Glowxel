#include "wifi_manager.h"
#include "config_manager.h"
#include "display_manager.h"
#include <time.h>
#include <math.h>

namespace {
const char* kNtpServerSecondary = "ntp.tencent.com";
const char* kNtpServerFallback = "pool.ntp.org";
const long kGmtOffsetSec = 8 * 3600;
const int kDaylightOffsetSec = 0;
const unsigned long kNtpRetryIntervalMs = 15000;
const unsigned long kWifiScanTimeoutMs = 15000;
const time_t kUnixTimeSyncedThreshold = 1700000000;
const IPAddress kConfigPortalIp(192, 168, 4, 1);
const IPAddress kConfigPortalGateway(192, 168, 4, 1);
const IPAddress kConfigPortalSubnet(255, 255, 255, 0);

void requestTimeSync() {
  configTime(
    kGmtOffsetSec,
    kDaylightOffsetSec,
    ConfigManager::deviceParamsConfig.ntpServer,
    kNtpServerSecondary,
    kNtpServerFallback
  );
}

bool hasValidSystemTime() {
  time_t now = time(nullptr);
  return now >= kUnixTimeSyncedThreshold;
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

DNSServer WiFiManager::dns_server;
WiFiScanResultItem WiFiManager::scanned_networks[12] = {};
size_t WiFiManager::scanned_network_count = 0;
bool WiFiManager::config_mode = false;
String WiFiManager::saved_ssid = "";
String WiFiManager::saved_password = "";
String WiFiManager::config_portal_ssid = "";
unsigned long WiFiManager::last_ntp_retry_at = 0;
unsigned long WiFiManager::portal_restart_at = 0;
bool WiFiManager::ntp_sync_logged = false;
bool WiFiManager::time_synced_once = false;
bool WiFiManager::scan_requested = false;
bool WiFiManager::scan_in_progress = false;
unsigned long WiFiManager::scan_started_at = 0;

void WiFiManager::init() {
  Serial.println("3. 连接WiFi...");
  WiFi.persistent(false);
  setupWiFi();
}

void WiFiManager::setupWiFi() {
  ConfigManager::preferences.begin("wifi", false);
  saved_ssid = ConfigManager::preferences.getString("ssid", "");
  saved_password = ConfigManager::preferences.getString("password", "");
  ConfigManager::preferences.end();
  
  if (saved_ssid.length() > 0) {
    Serial.println("尝试连接到保存的WiFi: " + saved_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(saved_ssid.c_str(), saved_password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi已连接");
      Serial.print("IP地址: ");
      Serial.println(WiFi.localIP());
      config_mode = false;
      
      // 发起 NTP 同步但不阻塞启动，后续由主循环继续重试。
      requestTimeSync();
      last_ntp_retry_at = millis();
      ntp_sync_logged = false;
      time_synced_once = false;
      Serial.println("已发起网络时间同步，后台更新中");
      
      // 在LED上显示WiFi大图标 + 右下角绿色勾徽章
      DisplayManager::dma_display->clearScreen();
      drawWifiIcon(32, 32, 23, DisplayManager::dma_display->color565(59, 130, 246));
      drawStatusBadge(32, 32, true);

      // IP 3x5小字居中
      String ip = WiFi.localIP().toString();
      DisplayManager::drawTinyTextCentered(ip.c_str(), 46, DisplayManager::dma_display->color565(150, 150, 150));
      
      delay(600);

      // 不强制覆盖模式，ConfigManager::loadClockConfig() 已从 Preferences 恢复
      // AnimationManager::init() 会从 LittleFS 加载动画数据并自动播放

      return;
    }
    
    Serial.println("\n连接失败，进入热点配网模式");
  } else {
    Serial.println("未找到WiFi配置，进入热点配网模式");
  }

  startConfigPortal();
}

void WiFiManager::startConfigPortal() {
  config_mode = true;
  portal_restart_at = 0;
  ntp_sync_logged = false;
  time_synced_once = false;
  config_portal_ssid = buildConfigPortalSSID();
  scanned_network_count = 0;
  scan_requested = false;
  scan_in_progress = false;
  scan_started_at = 0;

  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPdisconnect(true);
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
  dns_server.stop();
  scan_requested = false;
  scan_in_progress = false;
  scan_started_at = 0;
  WiFi.scanDelete();
  WiFi.softAPdisconnect(true);
}

void WiFiManager::scanNearbyNetworks() {
  if (!config_mode) {
    return;
  }

  if (scan_requested || scan_in_progress) {
    Serial.println("[WiFi] 扫描正在进行中，忽略重复请求");
    return;
  }

  scanned_network_count = 0;
  scan_requested = true;
  Serial.println("[WiFi] 已收到扫描请求，等待主循环启动异步扫描");
}

void WiFiManager::finalizeNetworkScan(int networkCount) {
  scanned_network_count = 0;

  if (networkCount < 0) {
    Serial.printf("[WiFi] 扫描失败，结果码=%d\n", networkCount);
    WiFi.scanDelete();
    return;
  }

  Serial.printf("[WiFi] 扫描完成，共发现 %d 个热点\n", networkCount);

  for (int i = 0; i < networkCount; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) {
      continue;
    }

    int32_t rssi = WiFi.RSSI(i);
    bool secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    int targetIndex = -1;

    for (size_t j = 0; j < scanned_network_count; j++) {
      if (ssid == String(scanned_networks[j].ssid)) {
        targetIndex = static_cast<int>(j);
        break;
      }
    }

    if (targetIndex >= 0) {
      if (rssi > scanned_networks[targetIndex].rssi) {
        copyScanItem(scanned_networks[targetIndex], ssid, rssi, secure);
      }
      continue;
    }

    if (scanned_network_count < 12) {
      copyScanItem(scanned_networks[scanned_network_count], ssid, rssi, secure);
      scanned_network_count++;
      continue;
    }

    size_t weakestIndex = 0;
    for (size_t j = 1; j < scanned_network_count; j++) {
      if (scanned_networks[j].rssi < scanned_networks[weakestIndex].rssi) {
        weakestIndex = j;
      }
    }

    if (rssi > scanned_networks[weakestIndex].rssi) {
      copyScanItem(scanned_networks[weakestIndex], ssid, rssi, secure);
    }
  }

  for (size_t i = 0; i < scanned_network_count; i++) {
    for (size_t j = i + 1; j < scanned_network_count; j++) {
      if (scanned_networks[j].rssi > scanned_networks[i].rssi) {
        WiFiScanResultItem temp = scanned_networks[i];
        scanned_networks[i] = scanned_networks[j];
        scanned_networks[j] = temp;
      }
    }
  }

  WiFi.scanDelete();
  Serial.printf("[WiFi] 热点列表已整理，当前可用堆内存: %u bytes\n", ESP.getFreeHeap());
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
  last_ntp_retry_at = millis();
  ntp_sync_logged = false;
  time_synced_once = false;
  Serial.printf("已更新 NTP 服务器并重新发起同步: %s\n", ConfigManager::deviceParamsConfig.ntpServer);
}

bool WiFiManager::isConfigMode() {
  return config_mode;
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
  if (index >= scanned_network_count) {
    return emptyItem;
  }
  return scanned_networks[index];
}

bool WiFiManager::isNetworkScanRunning() {
  return scan_requested || scan_in_progress;
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

void WiFiManager::tick() {
  if (config_mode) {
    dns_server.processNextRequest();
    ntp_sync_logged = false;
    time_synced_once = false;

    if (scan_requested && !scan_in_progress) {
      scan_requested = false;
      scan_in_progress = true;
      scan_started_at = millis();
      scanned_network_count = 0;
      WiFi.scanDelete();
      Serial.printf("[WiFi] 开始异步扫描附近热点，当前可用堆内存: %u bytes\n", ESP.getFreeHeap());
      int result = WiFi.scanNetworks(true);
      if (result == WIFI_SCAN_FAILED) {
        scan_in_progress = false;
        scan_started_at = 0;
        Serial.println("[WiFi] 异步扫描启动失败");
      } else if (result >= 0 && result != WIFI_SCAN_RUNNING) {
        scan_in_progress = false;
        scan_started_at = 0;
        finalizeNetworkScan(result);
      }
    }

    if (scan_in_progress) {
      int result = WiFi.scanComplete();
      if (result != WIFI_SCAN_RUNNING) {
        scan_in_progress = false;
        scan_started_at = 0;
        finalizeNetworkScan(result);
      } else if (scan_started_at != 0 && millis() - scan_started_at > kWifiScanTimeoutMs) {
        scan_in_progress = false;
        scan_started_at = 0;
        scanned_network_count = 0;
        WiFi.scanDelete();
        Serial.println("[WiFi] 扫描超时，已中止本次扫描请求");
      }
    }

    if (portal_restart_at != 0 && millis() >= portal_restart_at) {
      stopConfigPortal();
      ESP.restart();
    }
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    ntp_sync_logged = false;
    time_synced_once = false;
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
  if (now - last_ntp_retry_at < kNtpRetryIntervalMs) {
    return;
  }

  last_ntp_retry_at = now;
  ntp_sync_logged = false;
  Serial.println("NTP 未同步，后台重试...");
  requestTimeSync();
}
