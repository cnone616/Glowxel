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
  static uint32_t lastBinaryClientId;

  // WebSocket 分片数据缓存
  static String fragmentBuffer;
  static bool isReceivingFragments;

  // 心跳超时检测
  static unsigned long lastMessageTime;
  static void handleHeartbeatTimeout();
  static void sendBinaryReceiveConfirmation(int pixelCount);
  static void restoreDisplayForCurrentMode();
  
private:
  static const char* getCurrentModeString();
  static bool hasConnectedClients();
  static void syncClientConnectionState();
  static void clearAllClientTextStates();
  static void resetTransientTransferState();
  static void finalizeClientDisconnect(uint32_t clientId, bool fromHeartbeatTimeout);
  static void handleJsonCommand(AsyncWebSocketClient *client, JsonDocument& doc);
  static void handleJsonCommand(AsyncWebSocketClient *client, uint8_t *data, size_t len, AwsFrameInfo *info);
  static void handleBinaryData(AsyncWebSocketClient *client, uint8_t *data, size_t len);
};

#endif
