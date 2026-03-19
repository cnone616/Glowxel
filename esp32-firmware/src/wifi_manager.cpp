#include "wifi_manager.h"
#include "config_manager.h"
#include "display_manager.h"
#include "ble_config.h"
#include <time.h>
#include <math.h>

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

// 静态成员初始化
const char* WiFiManager::ap_ssid = "RenLight-Setup";
const char* WiFiManager::ap_password = "12345678";
bool WiFiManager::config_mode = false;
String WiFiManager::saved_ssid = "";
String WiFiManager::saved_password = "";
DNSServer WiFiManager::dnsServer;

void WiFiManager::init() {
  Serial.println("3. 连接WiFi...");
  setupWiFi();
}

void WiFiManager::setupWiFi() {
  // NTP 时间服务器
  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = 8 * 3600; // 中国时区 UTC+8
  const int daylightOffset_sec = 0;
  
  ConfigManager::preferences.begin("wifi", false);
  saved_ssid = ConfigManager::preferences.getString("ssid", "");
  saved_password = ConfigManager::preferences.getString("password", "");
  ConfigManager::preferences.end();
  
  if (saved_ssid.length() > 0) {
    Serial.println("尝试连接到保存的WiFi: " + saved_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(saved_ssid.c_str(), saved_password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi已连接");
      Serial.print("IP地址: ");
      Serial.println(WiFi.localIP());
      config_mode = false;
      
      // 同步NTP时间
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      Serial.println("正在同步网络时间...");

      // 等待NTP同步完成，最多10秒
      struct tm timeinfo;
      int ntpRetry = 0;
      while (!getLocalTime(&timeinfo) && ntpRetry < 20) {
        delay(500);
        ntpRetry++;
        Serial.print(".");
      }
      if (ntpRetry < 20) {
        Serial.println("\nNTP时间同步成功");
      } else {
        Serial.println("\nNTP同步超时，继续启动");
      }
      
      // 在LED上显示WiFi大图标 + 右下角绿色勾徽章
      DisplayManager::dma_display->clearScreen();
      drawWifiIcon(32, 32, 23, DisplayManager::dma_display->color565(59, 130, 246));
      drawStatusBadge(32, 32, true);

      // IP 3x5小字居中
      String ip = WiFi.localIP().toString();
      DisplayManager::drawTinyTextCentered(ip.c_str(), 46, DisplayManager::dma_display->color565(150, 150, 150));
      
      delay(2000);

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
  if (config_mode) {
    return WiFi.softAPIP().toString();
  } else {
    return WiFi.localIP().toString();
  }
}