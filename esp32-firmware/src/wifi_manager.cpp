#include "wifi_manager.h"
#include "config_manager.h"
#include "display_manager.h"
#include <time.h>
#include <math.h>

// 画WiFi扇形图标 (cx,cy)=底部圆点中心, r=最大弧半径
static void drawWifiIcon(int cx, int cy, int r, uint16_t color) {
  auto* d = DisplayManager::dma_display;

  // 底部小圆点 (2x2)
  d->fillRect(cx - 1, cy - 1, 3, 3, color);

  // 3层弧线，从内到外
  for (int arc = 0; arc < 3; arc++) {
    int radius = 4 + arc * 4;
    if (radius > r) break;
    // 画上半圆弧 (约 -60° 到 -120°，即上方扇形)
    for (float angle = -2.35; angle <= -0.78; angle += 0.04) {
      int px = cx + (int)(radius * cos(angle));
      int py = cy + (int)(radius * sin(angle));
      if (px >= 0 && px < 64 && py >= 0 && py < 64) {
        d->drawPixel(px, py, color);
      }
    }
  }
}

// 静态成员初始化
const char* WiFiManager::ap_ssid = "Glowxel-Setup";
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
      
      // 在LED上显示WiFi图标 + 连接成功
      DisplayManager::dma_display->clearScreen();
      drawWifiIcon(32, 18, 14, DisplayManager::dma_display->color565(59, 130, 246));

      DisplayManager::dma_display->setTextSize(1);
      DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(100, 255, 100));
      // "WiFi OK" 居中: 7字符*6px=42, (64-42)/2=11
      DisplayManager::dma_display->setCursor(11, 38);
      DisplayManager::dma_display->print("WiFi OK");

      // IP 居中
      String ip = WiFi.localIP().toString();
      int ipWidth = ip.length() * 6;
      DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(200, 200, 200));
      DisplayManager::dma_display->setCursor((64 - ipWidth) / 2, 50);
      DisplayManager::dma_display->print(ip);
      
      delay(2000);
      
      // 进入画布模式（默认黑底+时钟）
      DisplayManager::currentMode = MODE_CANVAS;
      
      return;
    }
    
    Serial.println("\n连接失败，进入配置模式");
  } else {
    Serial.println("未找到WiFi配置，进入配置模式");
  }
  
  config_mode = true;
  WiFi.mode(WIFI_AP);
  
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  WiFi.softAP(ap_ssid, ap_password, 1, 0, 4);
  delay(500);
  
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  Serial.println("\n=================================");
  Serial.println("配置模式已启动");
  Serial.println("热点名称: " + String(ap_ssid));
  Serial.println("热点密码: " + String(ap_password));
  Serial.print("配置地址: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("DNS服务器已启动");
  Serial.println("=================================");
  
  DisplayManager::dma_display->clearScreen();

  // 蓝色WiFi扇形图标居中显示在上方
  drawWifiIcon(32, 14, 12, DisplayManager::dma_display->color565(59, 130, 246));

  DisplayManager::dma_display->setTextSize(1);

  // SSID 居中
  int ssidLen = strlen(ap_ssid);
  DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(200, 200, 200));
  DisplayManager::dma_display->setCursor((64 - ssidLen * 6) / 2, 32);
  DisplayManager::dma_display->print(ap_ssid);

  // "Pass:" + 密码 居中
  String passStr = String("P:") + ap_password;
  int passWidth = passStr.length() * 6;
  DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(150, 150, 150));
  DisplayManager::dma_display->setCursor((64 - passWidth) / 2, 44);
  DisplayManager::dma_display->print(passStr);

  // "192.168.4.1" 居中
  String apIp = WiFi.softAPIP().toString();
  int apIpWidth = apIp.length() * 6;
  DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(100, 100, 100));
  DisplayManager::dma_display->setCursor((64 - apIpWidth) / 2, 55);
  DisplayManager::dma_display->print(apIp);
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