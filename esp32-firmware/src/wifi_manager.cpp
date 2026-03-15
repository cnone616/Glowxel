#include "wifi_manager.h"
#include "config_manager.h"
#include "display_manager.h"
#include <time.h>

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
      
      // 在LED上显示连接成功和IP
      DisplayManager::dma_display->clearScreen();
      DisplayManager::dma_display->setTextSize(1);
      DisplayManager::dma_display->setCursor(2, 2);
      DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(0, 255, 0));
      DisplayManager::dma_display->println("WiFi OK");
      DisplayManager::dma_display->setCursor(2, 12);
      DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
      DisplayManager::dma_display->println(WiFi.localIP().toString());
      
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
  DisplayManager::dma_display->setTextSize(1);
  
  DisplayManager::dma_display->setCursor(2, 10);
  DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(0, 255, 255));
  DisplayManager::dma_display->print("WiFi:");
  DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
  DisplayManager::dma_display->println(ap_ssid);
  
  DisplayManager::dma_display->setCursor(2, 40);
  DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(0, 255, 255));
  DisplayManager::dma_display->print("Pass:");
  DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
  DisplayManager::dma_display->println(ap_password);
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