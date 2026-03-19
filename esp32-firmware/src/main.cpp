#include <Arduino.h>
#include <time.h>
#include "config_manager.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "animation_manager.h"
#include "websocket_handler.h"
#include "web_server.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "ble_config.h"

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
    if (DisplayManager::currentMode == MODE_CLOCK) {
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
    // WiFi 配置已收到，延迟后重启
    if (BLEConfig::wifiConfigReceived) {
      delay(2000);
      ESP.restart();
    }
    delay(100);
    return;
  }

  // 检查是否有待保存的二进制像素数据
  if (WebSocketHandler::binaryDataPending && millis() - WebSocketHandler::lastBinaryReceiveTime > 500) {
    WebSocketHandler::binaryDataPending = false;

    // 根据当前模式选择对应的像素数据
    PixelData*& imagePixels = (DisplayManager::currentMode == MODE_ANIMATION)
      ? ConfigManager::animImagePixels
      : ConfigManager::staticImagePixels;
    int& imagePixelCount = (DisplayManager::currentMode == MODE_ANIMATION)
      ? ConfigManager::animImagePixelCount
      : ConfigManager::staticImagePixelCount;

    if (imagePixels != nullptr && imagePixelCount > 0) {
      Serial.printf("=== 开始保存像素数据到 Preferences: %d 个像素 ===\n", imagePixelCount);

      if (DisplayManager::currentMode == MODE_ANIMATION) {
        ConfigManager::saveAnimImagePixels();
      } else {
        ConfigManager::saveStaticImagePixels();
      }

      // 发送确认消息给客户端
      StaticJsonDocument<128> confirmDoc;
      confirmDoc["status"] = "ok";
      confirmDoc["message"] = "pixels_received";
      confirmDoc["count"] = imagePixelCount;

      String confirmMsg;
      serializeJson(confirmDoc, confirmMsg);
      WebSocketHandler::ws.textAll(confirmMsg);

      // 保存完成后刷新显示
      if (DisplayManager::currentMode == MODE_CLOCK) {
        DisplayManager::receivingPixels = false;
        DisplayManager::drawBackground();
        DisplayManager::drawClockOverlay();
      }
    }
  }

  // 心跳超时检测：15秒没收到消息认为客户端已断开
  if (DisplayManager::clientConnected &&
      millis() - WebSocketHandler::lastMessageTime > 15000) {
    Serial.println("心跳超时，客户端已断开");
    DisplayManager::clientConnected = false;
  }

  // 根据当前模式执行不同的逻辑
  if (DisplayManager::currentMode == MODE_CANVAS || DisplayManager::currentMode == MODE_CLOCK) {
    if (DisplayManager::currentMode == MODE_CANVAS) {
      // 纯画板模式：不画时钟，不干预显示
    } else if (DisplayManager::clientConnected) {
      // 有客户端连接时，不自动刷新时钟
    } else {
      // 静态时钟模式，无客户端：每分钟更新一次时钟
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
        if (millis() - lastNtpRetry > 60000) {
          lastNtpRetry = millis();
          Serial.println("NTP 未同步，重试...");
          configTime(8 * 3600, 0, "pool.ntp.org");
        }
        if (millis() - lastClockUpdate > 10000) {
          lastClockUpdate = millis();
          DisplayManager::displayClock();
        }
      }
    }
  } else if (DisplayManager::currentMode == MODE_ANIMATION) {
    // 动画模式
    if (TetrisEffect::isActive) {
      // 俄罗斯方块屏保
      TetrisEffect::update();
      TetrisEffect::render(DisplayManager::dma_display);
    } else {
      // GIF 动画
      AnimationManager::updateGIFAnimation();
    }
  }

  // 清理WebSocket，防止内存泄漏
  WebSocketHandler::ws.cleanupClients();

  yield();
}