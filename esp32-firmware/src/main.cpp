#include <Arduino.h>
#include <time.h>
#include "config_manager.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "animation_manager.h"
#include "websocket_handler.h"
#include "web_server.h"
#include "ota_manager.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=================================");
  Serial.println("ESP32 LED图片显示系统启动...");
  Serial.println("=================================");
  
  // 打印内存信息
  Serial.printf("总内存: %d bytes (%.1f KB)\n", ESP.getHeapSize(), ESP.getHeapSize()/1024.0);
  Serial.printf("可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap()/1024.0);
  Serial.printf("最大可分配块: %d bytes (%.1f KB)\n", ESP.getMaxAllocHeap(), ESP.getMaxAllocHeap()/1024.0);
  Serial.printf("PSRAM 总量: %d bytes (%.1f KB)\n", ESP.getPsramSize(), ESP.getPsramSize()/1024.0);
  Serial.printf("PSRAM 可用: %d bytes (%.1f KB)\n", ESP.getFreePsram(), ESP.getFreePsram()/1024.0);
  Serial.println("=================================");
  
  // 初始化各个模块
  DisplayManager::init();
  ConfigManager::init();
  WiFiManager::init();
  AnimationManager::init();

  // 只有 WiFi 连接成功才启动 HTTP/WebSocket 服务
  if (!WiFiManager::isConfigMode()) {
    WebSocketHandler::init();
    WebServer::init();

    // OTA 检查更新
    OTAManager::init();
    OTAManager::checkUpdate();

    // 清除WiFi状态画面，进入正常显示
    DisplayManager::clearScreen();

    // 立即渲染首帧：画布模式画时钟，动画模式由 AnimationManager::init() 已处理
    if (DisplayManager::currentMode == MODE_CANVAS) {
      DisplayManager::displayClock();
    }
  }
  
  Serial.println("\n=================================");
  Serial.println("系统就绪！");
  if (WiFiManager::isConfigMode()) {
    Serial.println("BLE 配网模式，等待蓝牙配置...");
  } else {
    Serial.print("STA模式访问地址: http://");
    Serial.println(WiFiManager::getDeviceIP());
    Serial.println("WebSocket 路径: ws://" + WiFiManager::getDeviceIP() + "/ws");
  }
  Serial.println("=================================\n");
}

void loop() {
  // 配网模式下只等待 BLE 配网，不做其他事
  if (WiFiManager::isConfigMode()) {
    delay(100);
    return;
  }

  // 检查是否有待保存的二进制像素数据
  if (WebSocketHandler::binaryDataPending && millis() - WebSocketHandler::lastBinaryReceiveTime > 500) {
    WebSocketHandler::binaryDataPending = false;

    if (ConfigManager::imagePixels != nullptr && ConfigManager::imagePixelCount > 0) {
      Serial.printf("=== 开始保存像素数据到 Preferences: %d 个像素 ===\n", ConfigManager::imagePixelCount);

      ConfigManager::saveImagePixels();

      // 发送确认消息给客户端
      StaticJsonDocument<128> confirmDoc;
      confirmDoc["status"] = "ok";
      confirmDoc["message"] = "pixels_received";
      confirmDoc["count"] = ConfigManager::imagePixelCount;

      String confirmMsg;
      serializeJson(confirmDoc, confirmMsg);
      WebSocketHandler::ws.textAll(confirmMsg);

      // 保存完成后，如果是画布模式，刷新显示
      if (DisplayManager::currentMode == MODE_CANVAS) {
        DisplayManager::displayClock();
      }
    }
  }

  // 根据当前模式执行不同的逻辑
  if (DisplayManager::currentMode == MODE_CANVAS) {
    // 画布模式：每分钟更新一次时钟
    static unsigned long lastClockUpdate = 0;
    static int lastMinute = -1;
    static unsigned long lastNtpRetry = 0;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      if (timeinfo.tm_min != lastMinute) {
        lastMinute = timeinfo.tm_min;
        DisplayManager::displayClock();
      }
    } else {
      // 时间未同步：每60秒重试一次 NTP
      if (millis() - lastNtpRetry > 60000) {
        lastNtpRetry = millis();
        Serial.println("NTP 未同步，重试...");
        configTime(8 * 3600, 0, "pool.ntp.org");
      }
      // 每10秒刷新一次显示
      if (millis() - lastClockUpdate > 10000) {
        lastClockUpdate = millis();
        DisplayManager::displayClock();
      }
    }
  } else if (DisplayManager::currentMode == MODE_ANIMATION) {
    // 动画模式：持续更新 GIF 动画
    AnimationManager::updateGIFAnimation();
  }

  // 清理WebSocket，防止内存泄漏
  WebSocketHandler::ws.cleanupClients();

  yield();
}