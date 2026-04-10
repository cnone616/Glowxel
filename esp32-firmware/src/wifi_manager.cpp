#include "wifi_manager.h"
#include "config_manager.h"
#include "display_manager.h"
#include "ble_config.h"
#include <time.h>
#include <math.h>

namespace {
const char* kNtpServerPrimary = "ntp.aliyun.com";
const char* kNtpServerSecondary = "ntp.tencent.com";
const char* kNtpServerFallback = "pool.ntp.org";
const long kGmtOffsetSec = 8 * 3600;
const int kDaylightOffsetSec = 0;
const unsigned long kNtpRetryIntervalMs = 15000;
const time_t kUnixTimeSyncedThreshold = 1700000000;

void requestTimeSync() {
  configTime(
    kGmtOffsetSec,
    kDaylightOffsetSec,
    kNtpServerPrimary,
    kNtpServerSecondary,
    kNtpServerFallback
  );
}

bool hasValidSystemTime() {
  time_t now = time(nullptr);
  return now >= kUnixTimeSyncedThreshold;
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

bool WiFiManager::config_mode = false;
String WiFiManager::saved_ssid = "";
String WiFiManager::saved_password = "";
unsigned long WiFiManager::last_ntp_retry_at = 0;
bool WiFiManager::ntp_sync_logged = false;
bool WiFiManager::time_synced_once = false;

void WiFiManager::init() {
  Serial.println("3. 连接WiFi...");
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
    
    Serial.println("\n连接失败，进入配置模式");
  } else {
    Serial.println("未找到WiFi配置，进入配置模式");
  }
  
  config_mode = true;

  // 只用 BLE 配网，不开 WiFi AP（内存不够同时开）
  BLEConfig::init();

  Serial.println("\n=================================");
  Serial.println("BLE 配网模式已启动");
  Serial.println("设备名称: RenLight-Setup");
  Serial.println("请使用小程序蓝牙配网");
  Serial.println("=================================");

  DisplayManager::dma_display->clearScreen();

  // 蓝色WiFi大图标 + 右下角红色X徽章
  drawWifiIcon(32, 32, 23, DisplayManager::dma_display->color565(59, 130, 246));
  drawStatusBadge(32, 32, false);

  // 提示文字
  DisplayManager::drawTinyTextCentered("NO WIFI", 46, DisplayManager::dma_display->color565(200, 200, 200));
  DisplayManager::drawTinyTextCentered("BLE SETUP", 53, DisplayManager::dma_display->color565(150, 150, 150));
}

bool WiFiManager::isConfigMode() {
  return config_mode;
}

String WiFiManager::getDeviceIP() {
  return WiFi.localIP().toString();
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
  if (config_mode || WiFi.status() != WL_CONNECTED) {
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
