#include "websocket_handler.h"
#include "display_manager.h"
#include "config_manager.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "wifi_manager.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "game_screensaver_effect.h"
#include "clock_font_renderer.h"
#include "device_mode_tag_codec.h"
#include "mode_tags.h"
#include "runtime_command_bus.h"
#include "runtime_status_builder.h"
#include "runtime_mode_coordinator.h"
#include "websocket_command_handlers.h"

namespace {
struct ClientTextMessageState {
  uint32_t clientId;
  String fragmentBuffer;
  bool inUse;
};

struct PendingTextMessage {
  uint32_t clientId;
  char* payload;
  size_t len;
};

struct ClientBinaryMessageState {
  uint32_t clientId;
  uint8_t* buffer;
  size_t expectedLen;
  bool inUse;
};

constexpr size_t kClientTextStateCount = 4;
constexpr size_t kClientBinaryStateCount = 4;
constexpr size_t kPendingTextQueueCapacity = 8;
ClientTextMessageState gClientTextStates[kClientTextStateCount] = {};
ClientBinaryMessageState gClientBinaryStates[kClientBinaryStateCount] = {};
PendingTextMessage* gPendingTextQueue[kPendingTextQueueCapacity] = {};
size_t gPendingTextQueueHead = 0;
size_t gPendingTextQueueTail = 0;
size_t gPendingTextQueueCount = 0;
portMUX_TYPE gPendingTextQueueMux = portMUX_INITIALIZER_UNLOCKED;
PixelData* gBinaryImageBackup = nullptr;
int gBinaryImageBackupCount = 0;
DeviceMode gBinaryImageBackupMode = MODE_CLOCK;
bool gBinaryImageBackupActive = false;

PixelData*& currentImagePixelsForMode(DeviceMode mode) {
  if (mode == MODE_ANIMATION) {
    return ConfigManager::animImagePixels;
  }
  return ConfigManager::staticImagePixels;
}

int& currentImagePixelCountForMode(DeviceMode mode) {
  if (mode == MODE_ANIMATION) {
    return ConfigManager::animImagePixelCount;
  }
  return ConfigManager::staticImagePixelCount;
}

void freeBinaryImageBackupStorage() {
  if (gBinaryImageBackup != nullptr) {
    free(gBinaryImageBackup);
    gBinaryImageBackup = nullptr;
  }
  gBinaryImageBackupCount = 0;
  gBinaryImageBackupActive = false;
}

bool snapshotBinaryImageState(DeviceMode mode) {
  freeBinaryImageBackupStorage();
  gBinaryImageBackupMode = mode;

  PixelData*& imagePixels = currentImagePixelsForMode(mode);
  int& imagePixelCount = currentImagePixelCountForMode(mode);
  if (imagePixels == nullptr || imagePixelCount <= 0) {
    gBinaryImageBackupActive = true;
    return true;
  }

  gBinaryImageBackup = static_cast<PixelData*>(malloc(sizeof(PixelData) * imagePixelCount));
  if (gBinaryImageBackup == nullptr) {
    return false;
  }

  memcpy(gBinaryImageBackup, imagePixels, sizeof(PixelData) * imagePixelCount);
  gBinaryImageBackupCount = imagePixelCount;
  gBinaryImageBackupActive = true;
  return true;
}

bool restoreBinaryImageBackupIfNeeded() {
  if (!gBinaryImageBackupActive) {
    return false;
  }

  PixelData*& imagePixels = currentImagePixelsForMode(gBinaryImageBackupMode);
  int& imagePixelCount = currentImagePixelCountForMode(gBinaryImageBackupMode);

  if (imagePixels != nullptr) {
    free(imagePixels);
    imagePixels = nullptr;
  }
  imagePixelCount = 0;

  if (gBinaryImageBackupCount > 0) {
    imagePixels = static_cast<PixelData*>(malloc(sizeof(PixelData) * gBinaryImageBackupCount));
    if (imagePixels == nullptr) {
      freeBinaryImageBackupStorage();
      return false;
    }
    memcpy(imagePixels, gBinaryImageBackup, sizeof(PixelData) * gBinaryImageBackupCount);
    imagePixelCount = gBinaryImageBackupCount;
  }

  freeBinaryImageBackupStorage();
  return true;
}

void beginBinaryImageBatch(DeviceMode mode) {
  if (!snapshotBinaryImageState(mode)) {
    Serial.println("静态像素批次快照失败，仍继续接收");
  }

  PixelData*& imagePixels = currentImagePixelsForMode(mode);
  int& imagePixelCount = currentImagePixelCountForMode(mode);
  if (imagePixels != nullptr) {
    free(imagePixels);
    imagePixels = nullptr;
  }
  imagePixelCount = 0;
  WebSocketHandler::binaryDataPending = false;
  WebSocketHandler::lastBinaryReceiveTime = 0;
  DisplayManager::receivingPixels = true;
}

PendingTextMessage* createPendingTextMessage(uint32_t clientId, const char* payload, size_t len) {
  PendingTextMessage* message = static_cast<PendingTextMessage*>(malloc(sizeof(PendingTextMessage)));
  if (message == nullptr) {
    return nullptr;
  }

  message->payload = static_cast<char*>(malloc(len + 1));
  if (message->payload == nullptr) {
    free(message);
    return nullptr;
  }

  memcpy(message->payload, payload, len);
  message->payload[len] = '\0';
  message->clientId = clientId;
  message->len = len;
  return message;
}

void destroyPendingTextMessage(PendingTextMessage* message) {
  if (message == nullptr) {
    return;
  }
  if (message->payload != nullptr) {
    free(message->payload);
    message->payload = nullptr;
  }
  free(message);
}

bool enqueuePendingTextMessage(PendingTextMessage* message) {
  if (message == nullptr) {
    return false;
  }

  bool queued = false;
  size_t pendingCount = 0;
  portENTER_CRITICAL(&gPendingTextQueueMux);
  if (gPendingTextQueueCount < kPendingTextQueueCapacity) {
    gPendingTextQueue[gPendingTextQueueTail] = message;
    gPendingTextQueueTail = (gPendingTextQueueTail + 1) % kPendingTextQueueCapacity;
    gPendingTextQueueCount += 1;
    pendingCount = gPendingTextQueueCount;
    queued = true;
  }
  portEXIT_CRITICAL(&gPendingTextQueueMux);

  if (queued) {
    Serial.printf("[WS] 文本消息已排队: client=%u len=%u pending=%u\n",
                  message->clientId,
                  static_cast<unsigned>(message->len),
                  static_cast<unsigned>(pendingCount));
  }
  return queued;
}

PendingTextMessage* dequeuePendingTextMessage() {
  PendingTextMessage* message = gPendingTextQueue[gPendingTextQueueHead];
  gPendingTextQueue[gPendingTextQueueHead] = nullptr;
  gPendingTextQueueHead = (gPendingTextQueueHead + 1) % kPendingTextQueueCapacity;
  if (gPendingTextQueueCount > 0) {
    gPendingTextQueueCount -= 1;
  }
  return message;
}

void clearAllPendingTextMessages() {
  portENTER_CRITICAL(&gPendingTextQueueMux);
  for (size_t i = 0; i < kPendingTextQueueCapacity; i++) {
    PendingTextMessage* message = gPendingTextQueue[i];
    gPendingTextQueue[i] = nullptr;
    if (message != nullptr) {
      portEXIT_CRITICAL(&gPendingTextQueueMux);
      destroyPendingTextMessage(message);
      portENTER_CRITICAL(&gPendingTextQueueMux);
    }
  }
  gPendingTextQueueHead = 0;
  gPendingTextQueueTail = 0;
  gPendingTextQueueCount = 0;
  portEXIT_CRITICAL(&gPendingTextQueueMux);
}

ClientTextMessageState* getClientTextState(uint32_t clientId, bool createIfMissing) {
  for (size_t i = 0; i < kClientTextStateCount; i++) {
    if (gClientTextStates[i].inUse && gClientTextStates[i].clientId == clientId) {
      return &gClientTextStates[i];
    }
  }

  if (!createIfMissing) {
    return nullptr;
  }

  for (size_t i = 0; i < kClientTextStateCount; i++) {
    if (!gClientTextStates[i].inUse) {
      gClientTextStates[i].clientId = clientId;
      gClientTextStates[i].fragmentBuffer = "";
      gClientTextStates[i].inUse = true;
      return &gClientTextStates[i];
    }
  }

  return nullptr;
}

void clearClientTextState(uint32_t clientId) {
  ClientTextMessageState* state = getClientTextState(clientId, false);
  if (state == nullptr) {
    return;
  }

  state->fragmentBuffer = "";
  state->clientId = 0;
  state->inUse = false;
}

void clearBinaryState(ClientBinaryMessageState& state) {
  if (state.buffer != nullptr) {
    free(state.buffer);
    state.buffer = nullptr;
  }
  state.clientId = 0;
  state.expectedLen = 0;
  state.inUse = false;
}

ClientBinaryMessageState* getClientBinaryState(uint32_t clientId, bool createIfMissing) {
  for (size_t i = 0; i < kClientBinaryStateCount; i++) {
    if (gClientBinaryStates[i].inUse && gClientBinaryStates[i].clientId == clientId) {
      return &gClientBinaryStates[i];
    }
  }

  if (!createIfMissing) {
    return nullptr;
  }

  for (size_t i = 0; i < kClientBinaryStateCount; i++) {
    if (!gClientBinaryStates[i].inUse) {
      gClientBinaryStates[i].clientId = clientId;
      gClientBinaryStates[i].buffer = nullptr;
      gClientBinaryStates[i].expectedLen = 0;
      gClientBinaryStates[i].inUse = true;
      return &gClientBinaryStates[i];
    }
  }

  return nullptr;
}

void clearClientBinaryState(uint32_t clientId) {
  ClientBinaryMessageState* state = getClientBinaryState(clientId, false);
  if (state == nullptr) {
    return;
  }
  clearBinaryState(*state);
}

void clearAllClientBinaryStates() {
  for (size_t i = 0; i < kClientBinaryStateCount; i++) {
    clearBinaryState(gClientBinaryStates[i]);
  }
}

const char* modeToString(DeviceMode mode) {
  return DeviceModeTagCodec::toTagOrUnknown(mode);
}

void logClientTextSendResult(const char* tag, AsyncWebSocketClient* client, size_t len, bool sent) {
  if (client == nullptr) {
    Serial.printf(
      "[WS SEND] %s client=0 sent=%d len=%u queue=0 available=0 millis=%lu\n",
      tag,
      sent ? 1 : 0,
      (unsigned int)len,
      millis()
    );
    return;
  }

  bool available = false;
  AsyncWebSocket* server = client->server();
  if (server != nullptr) {
    available = server->availableForWrite(client->id());
  }

  Serial.printf(
    "[WS SEND] %s client=%u sent=%d len=%u queue=%u available=%d millis=%lu\n",
    tag,
    client->id(),
    sent ? 1 : 0,
    (unsigned int)len,
    (unsigned int)client->queueLen(),
    available ? 1 : 0,
    millis()
  );
}

void logClientIdTextSendResult(const char* tag, uint32_t clientId, size_t len, bool sent) {
  AsyncWebSocketClient* client = WebSocketHandler::ws.client(clientId);
  if (client == nullptr) {
    Serial.printf(
      "[WS SEND] %s client=%u sent=%d len=%u connected=0 queue=0 available=0 millis=%lu\n",
      tag,
      clientId,
      sent ? 1 : 0,
      (unsigned int)len,
      millis()
    );
    return;
  }

  Serial.printf(
    "[WS SEND] %s client=%u sent=%d len=%u connected=1 queue=%u available=%d millis=%lu\n",
    tag,
    clientId,
    sent ? 1 : 0,
    (unsigned int)len,
    (unsigned int)client->queueLen(),
    WebSocketHandler::ws.availableForWrite(clientId) ? 1 : 0,
    millis()
  );
}
}

// 静态成员初始化
AsyncWebSocket WebSocketHandler::ws("/ws");
unsigned long WebSocketHandler::lastBinaryReceiveTime = 0;
bool WebSocketHandler::binaryDataPending = false;
uint32_t WebSocketHandler::lastBinaryClientId = 0;
String WebSocketHandler::fragmentBuffer = "";
bool WebSocketHandler::isReceivingFragments = false;
unsigned long WebSocketHandler::lastMessageTime = 0;

const char* WebSocketHandler::getCurrentModeString() {
  if (DisplayManager::currentBusinessModeTag.length() > 0) {
    return DisplayManager::currentBusinessModeTag.c_str();
  }
  return DeviceModeTagCodec::toTagOrEmpty(DisplayManager::currentMode);
}

void WebSocketHandler::init() {
  ws.onEvent(onWsEvent);
  Serial.println("WebSocket 已初始化");
}

bool WebSocketHandler::hasConnectedClients() {
  for (AsyncWebSocketClient& client : ws.getClients()) {
    if (client.status() == WS_CONNECTED) {
      return true;
    }
  }
  return false;
}

void WebSocketHandler::syncClientConnectionState() {
  DisplayManager::clientConnected = hasConnectedClients();
}

void WebSocketHandler::clearAllClientTextStates() {
  for (size_t i = 0; i < kClientTextStateCount; i++) {
    gClientTextStates[i].fragmentBuffer = "";
    gClientTextStates[i].clientId = 0;
    gClientTextStates[i].inUse = false;
  }
  clearAllClientBinaryStates();
  clearAllPendingTextMessages();
}

void WebSocketHandler::resetTransientTransferState() {
  binaryDataPending = false;
  lastBinaryReceiveTime = 0;
  lastBinaryClientId = 0;
  clearAllClientBinaryStates();
  clearBinaryImageTransferBackup();
  AnimationManager::abortUploadSession();
  AnimationManager::receivingFrameIndex = -1;
  DisplayManager::receivingPixels = false;
  if (DisplayManager::isLoadingActive) {
    DisplayManager::stopLoadingAnimation();
  }
}

WebSocketHandler::TransientTransferAbortState WebSocketHandler::abortTransientTransfer(const char* reason) {
  if (reason != nullptr && reason[0] != '\0') {
    Serial.println(reason);
  }

  TransientTransferAbortState state;
  state.wasTransientMode = RuntimeModeCoordinator::isTransientRuntimeMode(DisplayManager::currentMode);
  state.hadBinaryPixels = DisplayManager::receivingPixels || binaryDataPending;
  restoreBinaryImageBackupIfNeeded();
  resetTransientTransferState();
  return state;
}

void WebSocketHandler::clearBinaryImageTransferBackup() {
  freeBinaryImageBackupStorage();
}

void WebSocketHandler::finalizeClientDisconnect(uint32_t clientId, bool fromHeartbeatTimeout) {
  clearClientTextState(clientId);
  clearClientBinaryState(clientId);
  if (lastBinaryClientId == clientId) {
    lastBinaryClientId = 0;
  }

  syncClientConnectionState();
  if (DisplayManager::clientConnected) {
    return;
  }

  if (RuntimeModeCoordinator::isTransientRuntimeMode(DisplayManager::currentMode) ||
      DisplayManager::receivingPixels ||
      AnimationManager::isUploadSessionActive()) {
    DeviceMode fromMode = DisplayManager::currentMode;
    Serial.printf(
      "%s自动恢复模式: %s -> %s\n",
      fromHeartbeatTimeout ? "心跳超时后" : "客户端断开时",
      modeToString(fromMode),
      DisplayManager::lastBusinessModeTag.c_str()
    );
    RuntimeCommandBus::enqueueAbortTransientTransferAndRestore("客户端断开，终止当前传输并恢复稳定态");
    return;
  }

  if (fromHeartbeatTimeout) {
    RuntimeCommandBus::enqueueRestoreAfterTransientDisconnect(true);
  }
}

void WebSocketHandler::sendBinaryReceiveConfirmation(int pixelCount) {
  StaticJsonDocument<128> confirmDoc;
  confirmDoc["status"] = "ok";
  confirmDoc["message"] = "pixels_received";
  confirmDoc["count"] = pixelCount;

  String confirmMsg;
  serializeJson(confirmDoc, confirmMsg);

  if (lastBinaryClientId != 0 && ws.hasClient(lastBinaryClientId)) {
    bool sent = ws.text(lastBinaryClientId, confirmMsg);
    logClientIdTextSendResult("binary_confirm", lastBinaryClientId, confirmMsg.length(), sent);
  }

  lastBinaryClientId = 0;
}

void WebSocketHandler::tick() {
  PendingTextMessage* message = nullptr;
  portENTER_CRITICAL(&gPendingTextQueueMux);
  if (gPendingTextQueueCount > 0) {
    message = dequeuePendingTextMessage();
  }
  portEXIT_CRITICAL(&gPendingTextQueueMux);

  if (message == nullptr) {
    return;
  }

  AsyncWebSocketClient* client = ws.client(message->clientId);
  if (client == nullptr || client->status() != WS_CONNECTED) {
    Serial.printf("[WS] 丢弃已断开客户端的文本消息: client=%u\n", message->clientId);
    destroyPendingTextMessage(message);
    return;
  }

  size_t freeHeap = ESP.getFreeHeap();
  bool isChunkCmd = strstr(message->payload, "\"cmd\":\"frame_chunk\"") != nullptr;
  size_t multiplier = isChunkCmd ? 4 : 8;
  size_t maxJsonSize = message->len * multiplier + 2048;
  size_t heapLimit = (size_t)(freeHeap * 0.7);
  if (maxJsonSize > heapLimit) {
    maxJsonSize = heapLimit;
  }
  if (maxJsonSize < 2048) {
    maxJsonSize = 2048;
  }

  Serial.printf("主循环处理 JSON: client=%u data=%u bytes json=%u bytes\n",
                message->clientId,
                static_cast<unsigned>(message->len),
                static_cast<unsigned>(maxJsonSize));

  if (maxJsonSize > freeHeap * 0.85) {
    ws.text(
      message->clientId,
      String("{\"error\":\"insufficient memory\",\"needed\":") +
      String(maxJsonSize) +
      ",\"available\":" +
      String(freeHeap) +
      "}"
    );
    destroyPendingTextMessage(message);
    return;
  }

  DynamicJsonDocument doc(maxJsonSize);
  DeserializationError error = deserializeJson(doc, message->payload, message->len);
  if (error) {
    Serial.print("JSON解析失败: ");
    Serial.println(error.c_str());
    ws.text(
      message->clientId,
      String("{\"error\":\"invalid json\",\"details\":\"") +
      String(error.c_str()) +
      "\"}"
    );
    destroyPendingTextMessage(message);
    return;
  }

  Serial.println("JSON解析成功，处理命令");
  handleJsonCommand(client, doc);
  destroyPendingTextMessage(message);
}

void WebSocketHandler::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket 客户端已连接: %u\n", client->id());
    client->setCloseClientOnQueueFull(false);
    syncClientConnectionState();
    lastMessageTime = millis();
    clearClientTextState(client->id());
    clearClientBinaryState(client->id());
    
    // 连接时不切换模式，保持当前状态

    // 返回当前实际模式
    String modeStr = modeToString(DisplayManager::currentMode);
    String businessModeStr = getCurrentModeString();
    String response =
      "{\"status\":\"connected\",\"device\":\"RenLight\",\"mode\":\"" +
      modeStr +
      "\",\"businessMode\":\"" +
      businessModeStr +
      "\"}";
    bool sent = client->text(response);
    logClientTextSendResult("connect_ack", client, response.length(), sent);
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket 客户端已断开: %u\n", client->id());
    finalizeClientDisconnect(client->id(), false);
  }
  else if (type == WS_EVT_PING || type == WS_EVT_PONG) {
    lastMessageTime = millis();
  }
  else if (type == WS_EVT_ERROR) {
    uint16_t errorCode = 0;
    if (arg != nullptr) {
      errorCode = *reinterpret_cast<uint16_t*>(arg);
    }
    Serial.printf(
      "WebSocket 客户端错误: client=%u code=%u len=%u reason=%.*s\n",
      client->id(),
      (unsigned int)errorCode,
      (unsigned int)len,
      (int)len,
      data == nullptr ? "" : reinterpret_cast<const char*>(data)
    );
  }
  else if (type == WS_EVT_DATA) {
    lastMessageTime = millis();
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    // 处理二进制数据
    if (info->opcode == WS_BINARY) {
      handleBinaryMessage(client, data, len, info);
      return;
    }

    // 处理JSON命令（支持分片重组）
    handleJsonCommand(client, data, len, info);
  }
}

void WebSocketHandler::handleBinaryMessage(
  AsyncWebSocketClient *client,
  uint8_t *data,
  size_t len,
  AwsFrameInfo *info
) {
  if (client == nullptr || info == nullptr) {
    return;
  }

  if (info->index == 0 && info->final && info->len == len) {
    handleBinaryData(client, data, len);
    return;
  }

  ClientBinaryMessageState* state = getClientBinaryState(client->id(), true);
  if (state == nullptr) {
    client->text("{\"status\":\"error\",\"message\":\"binary message state unavailable\"}");
    return;
  }

  if (info->index == 0) {
    clearBinaryState(*state);
    state->clientId = client->id();
    state->buffer = static_cast<uint8_t*>(malloc(info->len));
    if (state->buffer == nullptr) {
      state->inUse = false;
      client->text("{\"status\":\"error\",\"message\":\"out of memory\"}");
      return;
    }
    state->expectedLen = info->len;
    state->inUse = true;
    Serial.printf("开始接收二进制分片，总大小: %llu bytes\n", info->len);
  }

  if (!state->inUse || state->buffer == nullptr || state->expectedLen != info->len) {
    clearBinaryState(*state);
    client->text("{\"status\":\"error\",\"message\":\"binary fragment state mismatch\"}");
    return;
  }

  if (info->index + len > state->expectedLen) {
    clearBinaryState(*state);
    client->text("{\"status\":\"error\",\"message\":\"binary fragment overflow\"}");
    return;
  }

  memcpy(state->buffer + info->index, data, len);

  if (!info->final || (info->index + len) < info->len) {
    Serial.printf("二进制分片接收中: %llu/%llu bytes\n", info->index + len, info->len);
    return;
  }

  Serial.printf("完整二进制消息已收齐: %u bytes\n", static_cast<unsigned>(state->expectedLen));
  handleBinaryData(client, state->buffer, state->expectedLen);
  clearBinaryState(*state);
}

void WebSocketHandler::handleBinaryData(AsyncWebSocketClient *client, uint8_t *data, size_t len) {
  // 二进制格式：每5字节一个像素 (x, y, r, g, b)
  lastBinaryClientId = client->id();

  // 数据完整性检查
  if (len % 5 != 0) {
    Serial.printf("⚠️ 数据长度错误: %d 字节不能被5整除！\n", len);
    client->text("{\"status\":\"error\",\"message\":\"invalid data length\"}");
    return;
  }

  int pixelCount = len / 5;

  if (pixelCount > 0) {
    // 动画帧二进制数据：如果正在接收动画帧，追加到对应帧
    if (AnimationManager::receivingFrameIndex >= 0) {
      int idx = AnimationManager::receivingFrameIndex;

      // 调试：打印接收到的数据详情
      static int chunkCount = 0;
      static int lastFrameIndex = -1;

      // 检测帧切换，重置计数器
      if (idx != lastFrameIndex) {
        chunkCount = 0;
        lastFrameIndex = idx;
      }

      chunkCount++;
      Serial.printf("[ESP32] 帧 %d chunk %d: 接收 %d 字节, %d 像素\n", idx, chunkCount, len, pixelCount);

      // 打印前3个像素用于对比
      if (chunkCount == 1 && len >= 15) {
        Serial.printf("[ESP32] 前3像素: [%d,%d,%d,%d,%d] [%d,%d,%d,%d,%d] [%d,%d,%d,%d,%d]\n",
          data[0], data[1], data[2], data[3], data[4],
          data[5], data[6], data[7], data[8], data[9],
          data[10], data[11], data[12], data[13], data[14]);
      }

      bool ok = AnimationManager::addFrameChunkBinary(idx, data, pixelCount);
      if (!ok) {
        client->text("{\"status\":\"error\",\"message\":\"binary chunk failed\",\"index\":" + String(idx) + "}");
        RuntimeCommandBus::enqueueAbortTransientTransferAndRestore("动画二进制 chunk 追加失败，恢复稳定态");
      }

      return;
    }

    // 画板模式：保存到 canvasBuffer（64x64完整画布）
    if (DisplayManager::currentMode == MODE_CANVAS) {
      
      // 第一次接收时初始化画布（清空）
      if (!DisplayManager::canvasInitialized) {
        DisplayManager::clearCanvas();
        DisplayManager::canvasInitialized = true;
      }
      
      // 先统计本批有效的黑色像素数量（在范围内的）
      int newBlackCount = 0;
      for (size_t i = 0; i + 4 < len; i += 5) {
        uint8_t x = data[i];
        uint8_t y = data[i + 1];
        uint8_t r = data[i + 2];
        uint8_t g = data[i + 3];
        uint8_t b = data[i + 4];
        if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y && r == 0 && g == 0 && b == 0) {
          newBlackCount++;
        }
      }
      
      // 扩展黑色像素数组（累加，不是替换）
      if (newBlackCount > 0) {
        DisplayManager::BlackPixel* newBuffer = (DisplayManager::BlackPixel*)realloc(DisplayManager::blackPixels, sizeof(DisplayManager::BlackPixel) * (DisplayManager::blackPixelCount + newBlackCount));
        if (newBuffer == nullptr) {
          return;
        }
        DisplayManager::blackPixels = newBuffer;
      }
      
      int validPixels = 0;
      for (size_t i = 0; i + 4 < len; i += 5) {
        uint8_t x = data[i];
        uint8_t y = data[i + 1];
        uint8_t r = data[i + 2];
        uint8_t g = data[i + 3];
        uint8_t b = data[i + 4];
        
        if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y) {
          // 存储到画布缓冲区
          DisplayManager::canvasBuffer[y][x][0] = r;
          DisplayManager::canvasBuffer[y][x][1] = g;
          DisplayManager::canvasBuffer[y][x][2] = b;
          validPixels++;
          
          // 如果是黑色，记录坐标
          if (r == 0 && g == 0 && b == 0 && DisplayManager::blackPixels != nullptr) {
            DisplayManager::blackPixels[DisplayManager::blackPixelCount].x = x;
            DisplayManager::blackPixels[DisplayManager::blackPixelCount].y = y;
            DisplayManager::blackPixelCount++;
          }
          
          // 立即绘制
          DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
        }
        
        if (i % 500 == 0) yield();
      }
      
      return;
    }
    
    // 静态时钟背景模式：保存到 staticImagePixels
    PixelData*& imagePixels = (DisplayManager::currentMode == MODE_ANIMATION)
      ? ConfigManager::animImagePixels
      : ConfigManager::staticImagePixels;

    int& imagePixelCount = (DisplayManager::currentMode == MODE_ANIMATION)
      ? ConfigManager::animImagePixelCount
      : ConfigManager::staticImagePixelCount;

    DeviceMode imageMode = DisplayManager::currentMode;

    bool needStartNewBatch = !DisplayManager::receivingPixels && !binaryDataPending;

    // 如果是新的一批数据（距离上次超过1秒），终止上一次未完成批次并恢复稳定画面
    if (!needStartNewBatch && millis() - lastBinaryReceiveTime > 1000) {
      if (DisplayManager::receivingPixels || binaryDataPending) {
        Serial.println("静态像素接收超时，恢复上一稳定画面");
        RuntimeCommandBus::enqueueAbortTransientTransferAndRestore("静态像素接收超时，恢复上一稳定画面");
        return;
      }
      needStartNewBatch = true;
    }

    if (needStartNewBatch) {
      beginBinaryImageBatch(imageMode);
    }

    // 追加数据
    PixelData* newBuffer = (PixelData*)realloc(imagePixels, sizeof(PixelData) * (imagePixelCount + pixelCount));
    if (newBuffer != nullptr) {
      imagePixels = newBuffer;

      int validPixels = 0;
      // 复制像素数据
      for (size_t i = 0; i + 4 < len; i += 5) {
        // 达到最大像素数量后，忽略后续数据，避免占用过多 NVS 空间
        if (imagePixelCount >= DisplayManager::MAX_PIXELS) {
          break;
        }

        uint8_t x = data[i];
        uint8_t y = data[i + 1];
        uint8_t r = data[i + 2];
        uint8_t g = data[i + 3];
        uint8_t b = data[i + 4];

        if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y) {
          // 保存到内存
          imagePixels[imagePixelCount].x = x;
          imagePixels[imagePixelCount].y = y;
          imagePixels[imagePixelCount].r = r;
          imagePixels[imagePixelCount].g = g;
          imagePixels[imagePixelCount].b = b;
          imagePixelCount++;
          validPixels++;

          // 立即绘制
          DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
        }

        // 每处理100个像素让出CPU，防止看门狗超时
        if (i % 500 == 0) {
          yield();
        }
      }
      
      // 更新接收时间，标记有待保存的数据
      lastBinaryReceiveTime = millis();
      binaryDataPending = true;
      
    } else {
      Serial.println("✗ 内存分配失败！");
    }
  }
}

void WebSocketHandler::handleJsonCommand(AsyncWebSocketClient *client, uint8_t *data, size_t len, AwsFrameInfo *info) {
  Serial.printf("收到 WebSocket 数据: %d bytes, final=%d, index=%llu, total=%llu\n",
    len, info->final, info->index, info->len);

  ClientTextMessageState* clientState = getClientTextState(client->id(), true);
  if (clientState == nullptr) {
    client->text("{\"error\":\"message state unavailable\"}");
    return;
  }

  // === 分片重组逻辑 ===
  if (info->index == 0) {
    // 第一个分片，清空缓冲区
    clientState->fragmentBuffer = "";
    clientState->fragmentBuffer.reserve(info->len);
    bool isFragmentedMessage = !info->final || (info->len > len);
    if (isFragmentedMessage) {
      Serial.printf("开始接收分片数据，总大小: %llu bytes\n", info->len);
    }
  }

  // 追加当前分片数据到缓冲区
  clientState->fragmentBuffer += String((char*)data, len);

  // 如果还没收完，等待后续分片
  if (!info->final || (info->index + len) < info->len) {
    Serial.printf("分片接收中: %llu/%llu bytes\n", info->index + len, info->len);
    return;
  }

  // === 完整消息已收齐，开始解析 ===
  size_t totalLen = clientState->fragmentBuffer.length();
  Serial.printf("完整消息已收齐: %d bytes\n", totalLen);
  PendingTextMessage* message = createPendingTextMessage(client->id(), clientState->fragmentBuffer.c_str(), totalLen);
  clientState->fragmentBuffer = "";
  if (message == nullptr) {
    client->text("{\"status\":\"error\",\"message\":\"out of memory\"}");
    return;
  }

  if (!enqueuePendingTextMessage(message)) {
    destroyPendingTextMessage(message);
    client->text("{\"status\":\"error\",\"message\":\"device busy\"}");
  }
}

void WebSocketHandler::handleJsonCommand(AsyncWebSocketClient *client, JsonDocument& doc) {
  String cmd = doc["cmd"].as<String>();
  Serial.println("命令: " + cmd);

  if (cmd == "status") {
    StaticJsonDocument<RuntimeStatusBuilder::kStatusJsonCapacity> statusResponse;
    RuntimeStatusBuilder::fillStatus(statusResponse, false, false);

    if (statusResponse.overflowed()) {
      Serial.printf(
        "status 响应 JSON 溢出: capacity=%u\n",
        RuntimeStatusBuilder::kStatusJsonCapacity
      );
    }

    String responseStr;
    serializeJson(statusResponse, responseStr);
    bool sent = client->text(responseStr);
    logClientTextSendResult("status_response", client, responseStr.length(), sent);
    return;
  }
  
  StaticJsonDocument<768> response;
  response["status"] = "ok";

  bool responseSent = false;
  bool handled =
    WebSocketCommandHandlers::handleBasicCommand(client, doc, response, getCurrentModeString()) ||
    WebSocketCommandHandlers::handleModeCommand(client, doc, response, responseSent) ||
    WebSocketCommandHandlers::handleClockCommand(client, doc, response) ||
    WebSocketCommandHandlers::handleThemeCommand(client, doc, response, responseSent) ||
    WebSocketCommandHandlers::handleEffectCommand(client, doc, response, responseSent) ||
    WebSocketCommandHandlers::handleEyesCommand(client, doc, response, responseSent) ||
    WebSocketCommandHandlers::handleAnimationCommand(client, doc, response, responseSent) ||
    WebSocketCommandHandlers::handleCanvasCommand(client, doc, response, responseSent) ||
    WebSocketCommandHandlers::handleOtaCommand(client, doc, response, responseSent);

  if (!handled) {
    response["status"] = "error";
    response["message"] = "unknown command";
  }

  if (responseSent) {
    return;
  }

  String responseStr;
  serializeJson(response, responseStr);
  bool sent = client->text(responseStr);
  logClientTextSendResult("json_response", client, responseStr.length(), sent);
}
