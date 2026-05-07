#include "ota_manager.h"
#include "display_manager.h"
#include "config_manager.h"
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// 静态成员初始化
String OTAManager::serverUrl = "";
String OTAManager::latestVersion = "";
String OTAManager::firmwareUrl = "";
String OTAManager::changelog = "";
String OTAManager::md5Hash = "";
int OTAManager::firmwareSize = 0;
bool OTAManager::updateAvailable = false;
bool OTAManager::isForce = false;

void OTAManager::init() {
  // 从 Preferences 读取服务器地址
  ConfigManager::preferences.begin("ota", true);
  serverUrl = ConfigManager::preferences.getString("server", "");
  ConfigManager::preferences.end();

  if (serverUrl.length() == 0) {
    return;
  }
}

void OTAManager::checkUpdate() {
  if (WiFi.status() != WL_CONNECTED || serverUrl.length() == 0) return;

  HTTPClient http;
  String url = serverUrl + "/api/firmware/check?version=" + FIRMWARE_VERSION + "&device_type=" + DEVICE_TYPE;
  http.begin(url);
  http.setTimeout(10000);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, payload);
    
    if (!err && doc["code"] == 0) {
      JsonObject data = doc["data"];
      updateAvailable = data["has_update"] | false;
      
      if (updateAvailable) {
        latestVersion = data["version"].as<String>();
        firmwareUrl = data["file_url"].as<String>();
        firmwareSize = data["file_size"] | 0;
        md5Hash = data["md5"].as<String>();
        changelog = data["changelog"].as<String>();
        isForce = data["is_force"] | false;

        if (isForce) {
          startUpdate();
        }
      }
    }
  } else {
    Serial.printf("[OTA] 检查更新失败: HTTP %d\n", httpCode);
  }
  
  http.end();
}

bool OTAManager::hasNewVersion() { return updateAvailable; }
String OTAManager::getLatestVersion() { return latestVersion; }
String OTAManager::getChangelog() { return changelog; }

bool OTAManager::startUpdate() {
  if (!updateAvailable || firmwareUrl.length() == 0) return false;
  
  // 显示更新提示
  auto* d = DisplayManager::dma_display;
  d->clearScreen();
  DisplayManager::drawTinyTextCentered("UPDATING", 20, d->color565(255, 200, 0));
  DisplayManager::drawTinyTextCentered(latestVersion.c_str(), 30, d->color565(150, 150, 150));
  DisplayManager::drawTinyTextCentered("DO NOT POWER OFF", 45, d->color565(255, 80, 80));

  // 构建下载 URL
  String downloadUrl = serverUrl + "/api/firmware/download/" + latestVersion;

  HTTPClient http;
  http.begin(downloadUrl);
  http.setTimeout(30000);
  
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("[OTA] 下载失败: HTTP %d\n", httpCode);
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("[OTA] 固件大小无效");
    return false;
  }

  if (!Update.begin(contentLength)) {
    Serial.println("[OTA] 空间不足，无法开始更新");
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written != contentLength) {
    Serial.printf("[OTA] 写入不完整: %d / %d\n", written, contentLength);
  }

  if (Update.end()) {
    if (Update.isFinished()) {
      d->clearScreen();
      DisplayManager::drawTinyTextCentered("UPDATE OK", 26, d->color565(100, 255, 100));
      DisplayManager::drawTinyTextCentered("REBOOTING", 36, d->color565(150, 150, 150));
      delay(2000);
      ESP.restart();
      return true;
    }
  }

  Serial.printf("[OTA] 更新失败: %u\n", Update.getError());
  http.end();
  return false;
}
