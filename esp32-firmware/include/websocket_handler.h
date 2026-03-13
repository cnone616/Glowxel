#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include <Arduino.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>

// 前向声明
class WiFiManager;
class DisplayManager;
class ConfigManager;
class AnimationManager;

class WebSocketHandler {
public:
  static void init();
  static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                       AwsEventType type, void *arg, uint8_t *data, size_t len);
  
  static AsyncWebSocket ws;
  
  // 二进制数据接收状态
  static unsigned long lastBinaryReceiveTime;
  static bool binaryDataPending;
  
  // WebSocket 分片数据缓存
  static String fragmentBuffer;
  static bool isReceivingFragments;
  
private:
  static void handleJsonCommand(AsyncWebSocketClient *client, JsonDocument& doc);
  static void handleJsonCommand(AsyncWebSocketClient *client, uint8_t *data, size_t len, AwsFrameInfo *info);
  static void handleBinaryData(AsyncWebSocketClient *client, uint8_t *data, size_t len);
};

#endif