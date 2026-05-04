#include "websocket_handler.h"
#include "display_manager.h"
#include "config_manager.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "wifi_manager.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "clock_font_renderer.h"
#include "device_mode_tag_codec.h"
#include "mode_tags.h"
#include "runtime_command_bus.h"
#include "runtime_status_builder.h"
#include "runtime_mode_coordinator.h"
#include "websocket_command_handlers.h"

namespace {
constexpr uint32_t kWebSocketTcpKeepAliveMs = 15000;
constexpr uint8_t kWebSocketTcpKeepAliveProbeCount = 3;
constexpr size_t kImmediateWsCommandMaxLen = 128;
constexpr size_t kImmediateWsCommandJsonCapacity = 192;
constexpr size_t kQueuedWsCommandProbeJsonCapacity = 128;
constexpr size_t kQueuedWsCommandBaseJsonCapacity = 2048;

struct ClientTextMessageState {
  uint32_t clientId;
  char* buffer;
  size_t expectedLen;
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
constexpr size_t kPendingTextQueueCapacity = 16;
constexpr size_t kQueuedWsCommandsPerTick = 2;
ClientTextMessageState gClientTextStates[kClientTextStateCount] = {};
ClientBinaryMessageState gClientBinaryStates[kClientBinaryStateCount] = {};
PendingTextMessage* gPendingTextQueue[kPendingTextQueueCapacity] = {};
size_t gPendingTextQueueHead = 0;
size_t gPendingTextQueueTail = 0;
size_t gPendingTextQueueCount = 0;
portMUX_TYPE gPendingTextQueueMux = portMUX_INITIALIZER_UNLOCKED;
uint32_t gPendingBinaryAnimationUploadClientId = 0;
uint32_t gStreamingAnimationUploadClientId = 0;
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

void clearTextState(ClientTextMessageState& state) {
  if (state.buffer != nullptr) {
    free(state.buffer);
    state.buffer = nullptr;
  }
  state.clientId = 0;
  state.expectedLen = 0;
  state.inUse = false;
}

bool isPendingBinaryAnimationUpload(uint32_t clientId) {
  return gPendingBinaryAnimationUploadClientId != 0 &&
         gPendingBinaryAnimationUploadClientId == clientId;
}

bool isStreamingAnimationUpload(uint32_t clientId) {
  return gStreamingAnimationUploadClientId != 0 &&
         gStreamingAnimationUploadClientId == clientId;
}

void restoreRuntimeAfterAnimationUploadFailure() {
  WebSocketHandler::clearPendingBinaryAnimationUpload();
  WebSocketHandler::clearStreamingAnimationUpload();
  AnimationManager::receivingFrameIndex = -1;
  AnimationManager::freeGIFAnimation();
  AnimationManager::loadAnimation();

  if (DisplayManager::isLoadingActive) {
    DisplayManager::stopLoadingAnimation();
  }

  if (RuntimeModeCoordinator::isTransientRuntimeMode(DisplayManager::currentMode)) {
    RuntimeModeCoordinator::restoreAfterTransientDisconnect(true, false);
    return;
  }

  RuntimeModeCoordinator::restoreCurrentModeFrame(false);
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
      gClientTextStates[i].buffer = nullptr;
      gClientTextStates[i].expectedLen = 0;
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

  clearTextState(*state);
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

bool tryHandleImmediateWsCommand(
  AsyncWebSocketClient* client,
  const char* payload,
  size_t len
) {
  if (client == nullptr || payload == nullptr || len == 0 || len > kImmediateWsCommandMaxLen) {
    return false;
  }

  StaticJsonDocument<kImmediateWsCommandJsonCapacity> doc;
  DeserializationError error = deserializeJson(doc, payload, len);
  if (error) {
    return false;
  }

  const char* cmd = doc["cmd"];
  if (cmd == nullptr) {
    return false;
  }

  if (strcmp(cmd, "ping") == 0) {
    StaticJsonDocument<64> response;
    response["status"] = "ok";
    response["message"] = "pong";

    String responseStr;
    serializeJson(response, responseStr);
    bool sent = client->text(responseStr);
    logClientTextSendResult("ping_response_fast", client, responseStr.length(), sent);
    return true;
  }

  return false;
}

bool queuedWsCommandNeedsExpandedJsonCapacity(const char* payload, size_t len) {
  if (payload == nullptr || len == 0) {
    return false;
  }

  StaticJsonDocument<32> filter;
  filter["cmd"] = true;

  StaticJsonDocument<kQueuedWsCommandProbeJsonCapacity> doc;
  DeserializationError error =
    deserializeJson(
      doc,
      static_cast<const char*>(payload),
      len,
      DeserializationOption::Filter(filter)
    );
  if (error) {
    return false;
  }

  const char* cmd = doc["cmd"];
  if (cmd == nullptr) {
    return false;
  }

  return strcmp(cmd, "image") == 0 ||
         strcmp(cmd, "image_sparse") == 0 ||
         strcmp(cmd, "image_chunk") == 0 ||
         strcmp(cmd, "set_gif_animation") == 0;
}

size_t resolveQueuedWsCommandJsonCapacity(
  const char* payload,
  size_t len,
  size_t freeHeap
) {
  size_t capacity = kQueuedWsCommandBaseJsonCapacity;
  if (queuedWsCommandNeedsExpandedJsonCapacity(payload, len)) {
    capacity = len * 8 + kQueuedWsCommandBaseJsonCapacity;
  }

  size_t heapLimit = static_cast<size_t>(freeHeap * 0.7);
  if (capacity > heapLimit) {
    capacity = heapLimit;
  }
  if (capacity < kQueuedWsCommandBaseJsonCapacity) {
    capacity = kQueuedWsCommandBaseJsonCapacity;
  }
  return capacity;
}
}

// 静态成员初始化
AsyncWebSocket WebSocketHandler::ws("/ws");
unsigned long WebSocketHandler::lastBinaryReceiveTime = 0;
bool WebSocketHandler::binaryDataPending = false;
uint32_t WebSocketHandler::lastBinaryClientId = 0;
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
    clearTextState(gClientTextStates[i]);
  }
  clearPendingBinaryAnimationUpload();
  clearStreamingAnimationUpload();
  clearAllClientBinaryStates();
  clearAllPendingTextMessages();
}

void WebSocketHandler::resetTransientTransferState() {
  binaryDataPending = false;
  lastBinaryReceiveTime = 0;
  lastBinaryClientId = 0;
  clearPendingBinaryAnimationUpload();
  clearStreamingAnimationUpload();
  clearAllClientBinaryStates();
  clearBinaryImageTransferBackup();
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
  const bool wasBinaryAnimationUpload = isPendingBinaryAnimationUpload(clientId);
  const bool wasStreamingAnimationUpload = isStreamingAnimationUpload(clientId);
  clearClientTextState(clientId);
  clearClientBinaryState(clientId);
  clearPendingBinaryAnimationUpload(clientId);
  clearStreamingAnimationUpload(clientId);
  if (lastBinaryClientId == clientId) {
    lastBinaryClientId = 0;
  }

  if (wasBinaryAnimationUpload || wasStreamingAnimationUpload) {
    Serial.println("动画上传客户端已断开，恢复稳定态");
    restoreRuntimeAfterAnimationUploadFailure();
  }

  RuntimeCommandBus::handleWebSocketTransactionDisconnect(clientId);

  syncClientConnectionState();
  if (DisplayManager::clientConnected) {
    return;
  }

  if (RuntimeModeCoordinator::isTransientRuntimeMode(DisplayManager::currentMode) ||
      DisplayManager::receivingPixels) {
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

void WebSocketHandler::prepareBinaryAnimationUpload(uint32_t clientId) {
  gPendingBinaryAnimationUploadClientId = clientId;
}

void WebSocketHandler::clearPendingBinaryAnimationUpload(uint32_t clientId) {
  if (clientId != 0 && gPendingBinaryAnimationUploadClientId != clientId) {
    return;
  }
  gPendingBinaryAnimationUploadClientId = 0;
}

void WebSocketHandler::prepareStreamingAnimationUpload(uint32_t clientId) {
  gStreamingAnimationUploadClientId = clientId;
}

void WebSocketHandler::clearStreamingAnimationUpload(uint32_t clientId) {
  if (clientId != 0 && gStreamingAnimationUploadClientId != clientId) {
    return;
  }
  gStreamingAnimationUploadClientId = 0;
}

void WebSocketHandler::restoreAfterAnimationUploadFailure() {
  restoreRuntimeAfterAnimationUploadFailure();
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
  for (size_t processedCount = 0; processedCount < kQueuedWsCommandsPerTick; processedCount += 1) {
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
      continue;
    }

    size_t freeHeap = ESP.getFreeHeap();
    size_t maxJsonSize =
      resolveQueuedWsCommandJsonCapacity(message->payload, message->len, freeHeap);

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
      continue;
    }

    DynamicJsonDocument doc(maxJsonSize);
    if (doc.capacity() < maxJsonSize) {
      Serial.printf("[WS] JSON文档分配不足: requested=%u actual=%u free=%u\n",
                    static_cast<unsigned>(maxJsonSize),
                    static_cast<unsigned>(doc.capacity()),
                    static_cast<unsigned>(freeHeap));
    }
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
      continue;
    }

    Serial.println("JSON解析成功，处理命令");
    handleJsonCommand(client, doc);
    destroyPendingTextMessage(message);
  }
}

void WebSocketHandler::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket 客户端已连接: %u\n", client->id());
    client->setCloseClientOnQueueFull(false);
    AsyncClient* rawClient = client->client();
    if (rawClient != nullptr) {
      rawClient->setNoDelay(true);
      rawClient->setKeepAlive(
        kWebSocketTcpKeepAliveMs,
        kWebSocketTcpKeepAliveProbeCount
      );
      client->keepAlivePeriod(15);
      Serial.printf(
        "[WS] client=%u transport tuned ack_timeout=%lu keepalive_ms=%lu keepalive_cnt=%u ws_ping_s=%u nodelay=%u\n",
                    client->id(),
                    static_cast<unsigned long>(rawClient->getAckTimeout()),
                    static_cast<unsigned long>(kWebSocketTcpKeepAliveMs),
                    static_cast<unsigned int>(kWebSocketTcpKeepAliveProbeCount),
                    static_cast<unsigned int>(client->keepAlivePeriod()),
                    rawClient->getNoDelay() ? 1U : 0U
      );
    }
    syncClientConnectionState();
    lastMessageTime = millis();
    clearClientTextState(client->id());
    clearClientBinaryState(client->id());
    
    // 连接时不切换模式，保持当前状态

    // 返回当前实际模式
    String modeStr = modeToString(DisplayManager::currentMode);
    String businessModeStr = getCurrentModeString();
    String response =
      "{\"status\":\"connected\",\"device\":\"Glowxel PixelBoard\",\"mode\":\"" +
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
  StaticJsonDocument<768> txResponse;
  bool txResponseReady = false;
  if (RuntimeCommandBus::appendWebSocketTransactionBinary(
        client,
        data,
        len,
        txResponse,
        txResponseReady)) {
    if (txResponseReady && client != nullptr) {
      RuntimeCommandBus::queueOrSendClientResponse(client->id(), txResponse);
    }
    return;
  }

  if (client != nullptr && isPendingBinaryAnimationUpload(client->id())) {
    clearPendingBinaryAnimationUpload(client->id());

    StaticJsonDocument<256> response;
    if (AnimationManager::loadGIFAnimationBinary(data, len)) {
      if (!AnimationManager::saveAnimation()) {
        response["status"] = "error";
        response["message"] = "animation persistence failed";
        restoreRuntimeAfterAnimationUploadFailure();
      } else {
        response["status"] = "success";
        response["message"] = "compact animation loaded";
        response["frameCount"] =
          AnimationManager::currentGIF != nullptr ? AnimationManager::currentGIF->frameCount : 0;
      }
    } else {
      response["status"] = "error";
      response["message"] = "animation load failed";
      restoreRuntimeAfterAnimationUploadFailure();
    }

    String responseStr;
    serializeJson(response, responseStr);
    bool sent = client->text(responseStr);
    logClientTextSendResult("animation_binary_response", client, responseStr.length(), sent);
    return;
  }

  if (client != nullptr &&
      isStreamingAnimationUpload(client->id()) &&
      AnimationManager::receivingFrameIndex >= 0) {
    if (len % 5 != 0) {
      client->text("{\"status\":\"error\",\"message\":\"invalid data length\"}");
      restoreRuntimeAfterAnimationUploadFailure();
      return;
    }

    const int pixelCount = len / 5;
    if (!AnimationManager::addFrameChunkBinary(AnimationManager::receivingFrameIndex, data, pixelCount)) {
      client->text("{\"status\":\"error\",\"message\":\"frame chunk failed\"}");
      restoreRuntimeAfterAnimationUploadFailure();
    }
    return;
  }

  if (client != nullptr) {
    client->text("{\"status\":\"error\",\"message\":\"binary requires active transaction\"}");
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

  if (info->index == 0) {
    clearTextState(*clientState);
    clientState->clientId = client->id();
    clientState->buffer = static_cast<char*>(malloc(info->len + 1));
    if (clientState->buffer == nullptr) {
      clearTextState(*clientState);
      client->text("{\"status\":\"error\",\"message\":\"out of memory\"}");
      return;
    }
    clientState->expectedLen = info->len;
    clientState->inUse = true;

    bool isFragmentedMessage = !info->final || (info->len > len);
    if (isFragmentedMessage) {
      Serial.printf("开始接收分片数据，总大小: %llu bytes\n", info->len);
    }
  }

  if (!clientState->inUse || clientState->buffer == nullptr || clientState->expectedLen != info->len) {
    clearTextState(*clientState);
    client->text("{\"status\":\"error\",\"message\":\"text fragment state mismatch\"}");
    return;
  }

  if (info->index + len > clientState->expectedLen) {
    clearTextState(*clientState);
    client->text("{\"status\":\"error\",\"message\":\"text fragment overflow\"}");
    return;
  }

  memcpy(clientState->buffer + info->index, data, len);

  if (!info->final || (info->index + len) < info->len) {
    Serial.printf("分片接收中: %llu/%llu bytes\n", info->index + len, info->len);
    return;
  }

  clientState->buffer[clientState->expectedLen] = '\0';
  size_t totalLen = clientState->expectedLen;
  Serial.printf("完整消息已收齐: %d bytes\n", totalLen);
  if (tryHandleImmediateWsCommand(client, clientState->buffer, totalLen)) {
    clearTextState(*clientState);
    return;
  }
  PendingTextMessage* message = createPendingTextMessage(client->id(), clientState->buffer, totalLen);
  clearTextState(*clientState);
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

  StaticJsonDocument<768> response;
  bool responseSent = false;

  if (cmd == "tx_begin") {
    if (RuntimeCommandBus::beginWebSocketTransaction(client, doc, response, responseSent)) {
      if (!responseSent && client != nullptr) {
        RuntimeCommandBus::queueOrSendClientResponse(client->id(), response);
      }
      return;
    }
  }

  if (cmd == "tx_commit") {
    if (RuntimeCommandBus::commitWebSocketTransaction(client, doc, response)) {
      const char* status = response["status"];
      if (status != nullptr && client != nullptr) {
        RuntimeCommandBus::queueOrSendClientResponse(client->id(), response);
      }
      return;
    }
  }

  if (cmd == "tx_abort") {
    if (RuntimeCommandBus::abortWebSocketTransaction(client, doc, response)) {
      const char* status = response["status"];
      if (status != nullptr && client != nullptr) {
        RuntimeCommandBus::queueOrSendClientResponse(client->id(), response);
      }
      return;
    }
  }

  if (cmd == "status") {
    StaticJsonDocument<RuntimeStatusBuilder::kCompactStatusJsonCapacity> statusResponse;
    RuntimeStatusBuilder::fillCompactStatus(statusResponse);

    if (statusResponse.overflowed()) {
      Serial.printf(
        "status 响应 JSON 溢出: capacity=%u\n",
        RuntimeStatusBuilder::kCompactStatusJsonCapacity
      );
    }

    char responseBuffer[RuntimeStatusBuilder::kCompactStatusMessageBufferSize] = {};
    size_t responseLen = measureJson(statusResponse);
    if (responseLen >= sizeof(responseBuffer)) {
      Serial.printf(
        "status 响应缓冲不足: len=%u capacity=%u\n",
        static_cast<unsigned>(responseLen),
        static_cast<unsigned>(sizeof(responseBuffer))
      );
      return;
    }

    serializeJson(statusResponse, responseBuffer, sizeof(responseBuffer));

    bool sent = false;
    try {
      sent = client->text(responseBuffer, responseLen);
    } catch (...) {
      Serial.printf(
        "status 响应发送异常: len=%u freeHeap=%u client=%u\n",
        static_cast<unsigned>(responseLen),
        static_cast<unsigned>(ESP.getFreeHeap()),
        client == nullptr ? 0U : client->id()
      );
      return;
    }

    logClientTextSendResult("status_response", client, responseLen, sent);
    return;
  }

  response["status"] = "ok";
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

  if (client != nullptr) {
    RuntimeCommandBus::queueOrSendClientResponse(client->id(), response);
    return;
  }

  String responseStr;
  serializeJson(response, responseStr);
  bool sent = client->text(responseStr);
  logClientTextSendResult("json_response", client, responseStr.length(), sent);
}
