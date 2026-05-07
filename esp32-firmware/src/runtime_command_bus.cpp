#include "runtime_command_bus.h"

#include <new>
#include <string.h>

#include "ambient_preset_codec.h"
#include "animation_manager.h"
#include "clock_font_renderer.h"
#include "device_mode_tag_codec.h"
#include "eyes_effect.h"
#include "maze_effect.h"
#include "mode_tags.h"
#include "runtime_mode_coordinator.h"
#include "snake_effect.h"
#include "tetris_clock_effect.h"
#include "tetris_effect.h"
#include "web_server.h"
#include "websocket_effect_common.h"
#include "websocket_handler.h"

namespace {
constexpr size_t kRuntimeCommandQueueCapacity = 16;
RuntimeCommandBus::RuntimeCommand* gCommandQueue[kRuntimeCommandQueueCapacity] = {};
size_t gQueueHead = 0;
size_t gQueueTail = 0;
size_t gQueueCount = 0;
portMUX_TYPE gRuntimeCommandQueueMux = portMUX_INITIALIZER_UNLOCKED;
constexpr size_t kModeActivationErrorMessageSize = 160;
constexpr size_t kRuntimeCommandsPerTick = 2;
volatile uint8_t gPostTransactionYieldHint = 0;

bool isWaterWorldAmbientPreset(uint8_t preset) {
  return preset == AMBIENT_PRESET_WATER_SURFACE ||
         preset == AMBIENT_PRESET_WATER_CURRENT ||
         preset == AMBIENT_PRESET_WATER_CAUSTICS;
}

uint8_t waterWorldAmbientPresetIntensity(uint8_t preset) {
  if (preset == AMBIENT_PRESET_WATER_CAUSTICS) {
    return 74;
  }
  return 72;
}

struct PendingClientResponse {
  uint32_t clientId;
  char* payload;
  size_t len;
};

constexpr size_t kPendingClientResponseQueueCapacity = 16;
PendingClientResponse* gPendingClientResponseQueue[kPendingClientResponseQueueCapacity] = {};
size_t gPendingClientResponseQueueHead = 0;
size_t gPendingClientResponseQueueTail = 0;
size_t gPendingClientResponseQueueCount = 0;
portMUX_TYPE gPendingClientResponseQueueMux = portMUX_INITIALIZER_UNLOCKED;

enum class WebSocketTransactionBinaryType : uint8_t {
  NONE = 0,
  STATIC_PIXELS = 1,
  COMPACT_ANIMATION = 2
};

struct WebSocketTransactionSession {
  bool active = false;
  bool hasBinary = false;
  bool hasClockConfig = false;
  bool hasEyesConfig = false;
  bool preparedCommandValid = false;
  uint32_t clientId = 0;
  size_t binarySize = 0;
  size_t receivedBinarySize = 0;
  WebSocketTransactionBinaryType binaryType = WebSocketTransactionBinaryType::NONE;
  String txId;
  String mode;
  RuntimeModeCoordinator::ModeStateSnapshot restoreSnapshot = {};
  RuntimeCommandBus::RuntimeCommand preparedCommand = {};
  ClockConfig clockConfig = {};
  EyesConfig eyesConfig = {};
  PixelData* pixelBackup = nullptr;
  int pixelBackupCount = 0;
  DeviceMode pixelBackupMode = MODE_CLOCK;
  bool pixelBackupActive = false;
  uint8_t* canvasBackup = nullptr;
  DisplayManager::BlackPixel* canvasBlackPixelBackup = nullptr;
  int canvasBlackPixelBackupCount = 0;
  bool canvasInitializedBackup = false;
  bool canvasBackupActive = false;
  uint8_t* animationBinaryBuffer = nullptr;  // 复用为紧凑动画或 canvas 原始事务二进制缓冲
};

WebSocketTransactionSession gWebSocketTransactionSession = {};

bool executeCommand(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
);

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

void clearTransactionPixelBackup() {
  if (gWebSocketTransactionSession.pixelBackup != nullptr) {
    free(gWebSocketTransactionSession.pixelBackup);
    gWebSocketTransactionSession.pixelBackup = nullptr;
  }
  gWebSocketTransactionSession.pixelBackupCount = 0;
  gWebSocketTransactionSession.pixelBackupActive = false;
}

void clearTransactionAnimationBinaryBuffer() {
  if (gWebSocketTransactionSession.animationBinaryBuffer != nullptr) {
    free(gWebSocketTransactionSession.animationBinaryBuffer);
    gWebSocketTransactionSession.animationBinaryBuffer = nullptr;
  }
}

void clearTransactionCanvasBackup() {
  if (gWebSocketTransactionSession.canvasBackup != nullptr) {
    free(gWebSocketTransactionSession.canvasBackup);
    gWebSocketTransactionSession.canvasBackup = nullptr;
  }
  if (gWebSocketTransactionSession.canvasBlackPixelBackup != nullptr) {
    free(gWebSocketTransactionSession.canvasBlackPixelBackup);
    gWebSocketTransactionSession.canvasBlackPixelBackup = nullptr;
  }
  gWebSocketTransactionSession.canvasBlackPixelBackupCount = 0;
  gWebSocketTransactionSession.canvasInitializedBackup = false;
  gWebSocketTransactionSession.canvasBackupActive = false;
}

bool isCanvasStaticPixelTransaction() {
  return gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::STATIC_PIXELS &&
         gWebSocketTransactionSession.preparedCommand.targetMode == MODE_CANVAS;
}

bool isStaticPixelBinarySize(size_t binarySize) {
  return binarySize % 5 == 0;
}

bool isCompactAnimationBinarySize(size_t binarySize) {
  return binarySize >= 2 && binarySize % 5 == 2;
}

void clearWebSocketTransactionSession() {
  clearTransactionPixelBackup();
  clearTransactionCanvasBackup();
  clearTransactionAnimationBinaryBuffer();
  gWebSocketTransactionSession = WebSocketTransactionSession{};
}

const char* currentActiveModeTag() {
  if (DisplayManager::currentBusinessModeTag.length() > 0) {
    return DisplayManager::currentBusinessModeTag.c_str();
  }
  return DeviceModeTagCodec::toTagOrUnknown(DisplayManager::currentMode);
}

void fillTransactionAcceptedResponse(
  const String& txId,
  StaticJsonDocument<768>& response
) {
  response.clear();
  response["status"] = "accepted";
  response["txId"] = txId;
}

void fillTransactionFinalOkResponse(
  const String& txId,
  StaticJsonDocument<768>& response
) {
  response.clear();
  response["status"] = "final_ok";
  response["txId"] = txId;
  response["activeMode"] = String(currentActiveModeTag());
}

void fillTransactionFinalErrorResponse(
  const String& txId,
  const char* reason,
  StaticJsonDocument<768>& response
) {
  response.clear();
  response["status"] = "final_error";
  response["txId"] = txId;
  response["reason"] = String(reason == nullptr ? "" : reason);
  response["activeMode"] = String(currentActiveModeTag());
}

bool isHexColorText(const char* value) {
  if (value == nullptr) {
    return false;
  }
  if (strlen(value) != 7 || value[0] != '#') {
    return false;
  }

  for (int i = 1; i < 7; i++) {
    char ch = value[i];
    bool isDigit = ch >= '0' && ch <= '9';
    bool isLowerHex = ch >= 'a' && ch <= 'f';
    bool isUpperHex = ch >= 'A' && ch <= 'F';
    if (!isDigit && !isLowerHex && !isUpperHex) {
      return false;
    }
  }

  return true;
}

bool parseRequiredHexColorText(
  JsonObject params,
  const char* field,
  char* target,
  size_t targetSize
) {
  if (!params.containsKey(field) || !params[field].is<const char*>()) {
    return false;
  }

  const char* value = params[field];
  if (!isHexColorText(value)) {
    return false;
  }

  snprintf(target, targetSize, "%s", value);
  return true;
}

void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}

void copyModeActivationErrorMessage(
  const String& businessModeTag,
  char* buffer,
  size_t size
) {
  const char* message = "mode activation failed";
  if (businessModeTag == ModeTags::MAZE) {
    const char* detail = MazeEffect::getLastError();
    if (detail != nullptr && detail[0] != '\0') {
      message = detail;
    }
  } else if (businessModeTag == ModeTags::SNAKE) {
    const char* detail = SnakeEffect::getLastError();
    if (detail != nullptr && detail[0] != '\0') {
      message = detail;
    }
  }

  if (buffer == nullptr || size == 0) {
    return;
  }
  snprintf(buffer, size, "%s", message);
}

bool clientStillConnected(uint32_t clientId) {
  return clientId != 0 && WebSocketHandler::ws.hasClient(clientId);
}

PendingClientResponse* createPendingClientResponse(
  uint32_t clientId,
  const char* payload,
  size_t len
) {
  PendingClientResponse* response =
    static_cast<PendingClientResponse*>(malloc(sizeof(PendingClientResponse)));
  if (response == nullptr) {
    return nullptr;
  }

  response->payload = static_cast<char*>(malloc(len + 1));
  if (response->payload == nullptr) {
    free(response);
    return nullptr;
  }

  memcpy(response->payload, payload, len);
  response->payload[len] = '\0';
  response->clientId = clientId;
  response->len = len;
  return response;
}

void destroyPendingClientResponse(PendingClientResponse* response) {
  if (response == nullptr) {
    return;
  }

  if (response->payload != nullptr) {
    free(response->payload);
    response->payload = nullptr;
  }
  free(response);
}

bool enqueuePendingClientResponse(PendingClientResponse* response) {
  if (response == nullptr) {
    return false;
  }

  bool queued = false;
  portENTER_CRITICAL(&gPendingClientResponseQueueMux);
  if (gPendingClientResponseQueueCount < kPendingClientResponseQueueCapacity) {
    gPendingClientResponseQueue[gPendingClientResponseQueueTail] = response;
    gPendingClientResponseQueueTail =
      (gPendingClientResponseQueueTail + 1) % kPendingClientResponseQueueCapacity;
    gPendingClientResponseQueueCount += 1;
    queued = true;
  }
  portEXIT_CRITICAL(&gPendingClientResponseQueueMux);
  return queued;
}

PendingClientResponse* dequeuePendingClientResponse() {
  PendingClientResponse* response =
    gPendingClientResponseQueue[gPendingClientResponseQueueHead];
  gPendingClientResponseQueue[gPendingClientResponseQueueHead] = nullptr;
  gPendingClientResponseQueueHead =
    (gPendingClientResponseQueueHead + 1) % kPendingClientResponseQueueCapacity;
  if (gPendingClientResponseQueueCount > 0) {
    gPendingClientResponseQueueCount -= 1;
  }
  return response;
}

bool hasPendingClientResponse(uint32_t clientId) {
  bool found = false;
  portENTER_CRITICAL(&gPendingClientResponseQueueMux);
  for (size_t i = 0; i < gPendingClientResponseQueueCount; i++) {
    size_t index =
      (gPendingClientResponseQueueHead + i) % kPendingClientResponseQueueCapacity;
    PendingClientResponse* response = gPendingClientResponseQueue[index];
    if (response != nullptr && response->clientId == clientId) {
      found = true;
      break;
    }
  }
  portEXIT_CRITICAL(&gPendingClientResponseQueueMux);
  return found;
}

bool trySendClientResponseNow(
  uint32_t clientId,
  const char* payload,
  size_t len
) {
  if (!clientStillConnected(clientId)) {
    return false;
  }

  AsyncWebSocketClient* client = WebSocketHandler::ws.client(clientId);
  if (client == nullptr) {
    return false;
  }

  if (!WebSocketHandler::ws.availableForWrite(clientId)) {
    return false;
  }

  return WebSocketHandler::ws.text(clientId, payload, len);
}

bool shouldClearScreenBeforeBusinessModeEntry(const String& businessModeTag) {
  return DisplayManager::currentBusinessModeTag != businessModeTag &&
         RuntimeModeCoordinator::shouldClearScreenBeforeBusinessModeEntry(businessModeTag);
}

void flushPendingClientResponses() {
  while (true) {
    PendingClientResponse* response = nullptr;
    portENTER_CRITICAL(&gPendingClientResponseQueueMux);
    if (gPendingClientResponseQueueCount > 0) {
      response = gPendingClientResponseQueue[gPendingClientResponseQueueHead];
    }
    portEXIT_CRITICAL(&gPendingClientResponseQueueMux);

    if (response == nullptr) {
      return;
    }

    if (!clientStillConnected(response->clientId)) {
      portENTER_CRITICAL(&gPendingClientResponseQueueMux);
      PendingClientResponse* dropped = dequeuePendingClientResponse();
      portEXIT_CRITICAL(&gPendingClientResponseQueueMux);
      destroyPendingClientResponse(dropped);
      continue;
    }

    if (!trySendClientResponseNow(response->clientId, response->payload, response->len)) {
      return;
    }

    portENTER_CRITICAL(&gPendingClientResponseQueueMux);
    PendingClientResponse* sent = dequeuePendingClientResponse();
    portEXIT_CRITICAL(&gPendingClientResponseQueueMux);
    destroyPendingClientResponse(sent);
  }
}

void sendClientResponse(uint32_t clientId, StaticJsonDocument<768>& response) {
  if (!clientStillConnected(clientId)) {
    return;
  }

  String responseStr;
  serializeJson(response, responseStr);

  flushPendingClientResponses();
  if (!hasPendingClientResponse(clientId) &&
      trySendClientResponseNow(clientId, responseStr.c_str(), responseStr.length())) {
    return;
  }

  PendingClientResponse* pending =
    createPendingClientResponse(clientId, responseStr.c_str(), responseStr.length());
  if (pending == nullptr) {
    Serial.printf("[RuntimeBus] drop response due to OOM client=%u len=%u\n",
                  clientId,
                  static_cast<unsigned>(responseStr.length()));
    return;
  }

  if (!enqueuePendingClientResponse(pending)) {
    Serial.printf("[RuntimeBus] drop response due to full queue client=%u len=%u\n",
                  clientId,
                  static_cast<unsigned>(responseStr.length()));
    destroyPendingClientResponse(pending);
  }
}

void enterWebSocketTransactionLoadingState() {
  RuntimeModeCoordinator::deactivateRuntimeContent();
  if (DisplayManager::isLoadingActive) {
    DisplayManager::stopLoadingAnimation();
  }
  DisplayManager::clearScreen();
  DisplayManager::currentMode = MODE_TRANSFERRING;
  DisplayManager::startLoadingAnimation();
}

bool snapshotStaticPixelTarget(DeviceMode mode) {
  clearTransactionPixelBackup();
  gWebSocketTransactionSession.pixelBackupMode = mode;

  PixelData*& imagePixels = currentImagePixelsForMode(mode);
  int& imagePixelCount = currentImagePixelCountForMode(mode);
  if (imagePixels == nullptr || imagePixelCount <= 0) {
    gWebSocketTransactionSession.pixelBackupActive = true;
    return true;
  }

  gWebSocketTransactionSession.pixelBackup =
    static_cast<PixelData*>(malloc(sizeof(PixelData) * imagePixelCount));
  if (gWebSocketTransactionSession.pixelBackup == nullptr) {
    return false;
  }

  memcpy(
    gWebSocketTransactionSession.pixelBackup,
    imagePixels,
    sizeof(PixelData) * imagePixelCount
  );
  gWebSocketTransactionSession.pixelBackupCount = imagePixelCount;
  gWebSocketTransactionSession.pixelBackupActive = true;
  return true;
}

void clearStaticPixelTarget(DeviceMode mode) {
  PixelData*& imagePixels = currentImagePixelsForMode(mode);
  int& imagePixelCount = currentImagePixelCountForMode(mode);
  if (imagePixels != nullptr) {
    free(imagePixels);
    imagePixels = nullptr;
  }
  imagePixelCount = 0;
}

bool restoreStaticPixelTarget() {
  if (!gWebSocketTransactionSession.pixelBackupActive) {
    return true;
  }

  DeviceMode mode = gWebSocketTransactionSession.pixelBackupMode;
  PixelData*& imagePixels = currentImagePixelsForMode(mode);
  int& imagePixelCount = currentImagePixelCountForMode(mode);
  clearStaticPixelTarget(mode);

  if (gWebSocketTransactionSession.pixelBackupCount > 0) {
    imagePixels = static_cast<PixelData*>(
      malloc(sizeof(PixelData) * gWebSocketTransactionSession.pixelBackupCount)
    );
    if (imagePixels == nullptr) {
      clearTransactionPixelBackup();
      return false;
    }
    memcpy(
      imagePixels,
      gWebSocketTransactionSession.pixelBackup,
      sizeof(PixelData) * gWebSocketTransactionSession.pixelBackupCount
    );
    imagePixelCount = gWebSocketTransactionSession.pixelBackupCount;
  }

  clearTransactionPixelBackup();
  return true;
}

bool snapshotCanvasTarget() {
  clearTransactionCanvasBackup();
  gWebSocketTransactionSession.canvasBackupActive = true;
  gWebSocketTransactionSession.canvasInitializedBackup = DisplayManager::canvasInitialized;

  if (DisplayManager::canvasInitialized) {
    gWebSocketTransactionSession.canvasBackup =
      static_cast<uint8_t*>(malloc(sizeof(DisplayManager::canvasBuffer)));
    if (gWebSocketTransactionSession.canvasBackup == nullptr) {
      clearTransactionCanvasBackup();
      return false;
    }

    memcpy(
      gWebSocketTransactionSession.canvasBackup,
      DisplayManager::canvasBuffer,
      sizeof(DisplayManager::canvasBuffer)
    );
  }

  if (DisplayManager::blackPixelCount > 0 && DisplayManager::blackPixels != nullptr) {
    gWebSocketTransactionSession.canvasBlackPixelBackup =
      static_cast<DisplayManager::BlackPixel*>(
        malloc(sizeof(DisplayManager::BlackPixel) * DisplayManager::blackPixelCount)
      );
    if (gWebSocketTransactionSession.canvasBlackPixelBackup == nullptr) {
      clearTransactionCanvasBackup();
      return false;
    }

    memcpy(
      gWebSocketTransactionSession.canvasBlackPixelBackup,
      DisplayManager::blackPixels,
      sizeof(DisplayManager::BlackPixel) * DisplayManager::blackPixelCount
    );
    gWebSocketTransactionSession.canvasBlackPixelBackupCount = DisplayManager::blackPixelCount;
  }

  return true;
}

bool restoreCanvasTarget() {
  if (!gWebSocketTransactionSession.canvasBackupActive) {
    return true;
  }

  DisplayManager::clearCanvas();

  if (gWebSocketTransactionSession.canvasInitializedBackup) {
    if (gWebSocketTransactionSession.canvasBackup == nullptr) {
      clearTransactionCanvasBackup();
      return false;
    }

    memcpy(
      DisplayManager::canvasBuffer,
      gWebSocketTransactionSession.canvasBackup,
      sizeof(DisplayManager::canvasBuffer)
    );
    DisplayManager::canvasInitialized = true;
  }

  if (gWebSocketTransactionSession.canvasBlackPixelBackupCount > 0) {
    DisplayManager::blackPixels = static_cast<DisplayManager::BlackPixel*>(
      malloc(
        sizeof(DisplayManager::BlackPixel) *
          gWebSocketTransactionSession.canvasBlackPixelBackupCount
      )
    );
    if (DisplayManager::blackPixels == nullptr) {
      clearTransactionCanvasBackup();
      return false;
    }

    memcpy(
      DisplayManager::blackPixels,
      gWebSocketTransactionSession.canvasBlackPixelBackup,
      sizeof(DisplayManager::BlackPixel) *
        gWebSocketTransactionSession.canvasBlackPixelBackupCount
    );
    DisplayManager::blackPixelCount = gWebSocketTransactionSession.canvasBlackPixelBackupCount;
  }

  clearTransactionCanvasBackup();
  return true;
}

void restoreAnimationAfterTransactionFailure() {
  AnimationManager::receivingFrameIndex = -1;
  AnimationManager::freeGIFAnimation();
  AnimationManager::loadAnimation();
}

bool shouldRestoreAnimationRuntimeOnRollback() {
  if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::COMPACT_ANIMATION) {
    return true;
  }

  return gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::STATIC_PIXELS &&
         gWebSocketTransactionSession.preparedCommand.targetMode == MODE_ANIMATION;
}

void restoreWebSocketTransactionRuntime() {
  if (DisplayManager::isLoadingActive) {
    DisplayManager::stopLoadingAnimation();
  }

  RuntimeModeCoordinator::deactivateRuntimeContent();
  DisplayManager::receivingPixels = false;
  RuntimeModeCoordinator::restoreModeState(gWebSocketTransactionSession.restoreSnapshot, false);
  DisplayManager::clearScreen();
  RuntimeModeCoordinator::restoreCurrentModeFrame(false);
}

void rollbackWebSocketTransactionSession() {
  if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::STATIC_PIXELS &&
      !isCanvasStaticPixelTransaction()) {
    restoreStaticPixelTarget();
  }
  if (isCanvasStaticPixelTransaction()) {
    restoreCanvasTarget();
  }
  if (shouldRestoreAnimationRuntimeOnRollback()) {
    restoreAnimationAfterTransactionFailure();
  }
  restoreWebSocketTransactionRuntime();
  clearWebSocketTransactionSession();
}

void finalizeSuccessfulWebSocketTransaction() {
  if (DisplayManager::isLoadingActive) {
    DisplayManager::stopLoadingAnimation();
  }
  DisplayManager::receivingPixels = false;
  clearWebSocketTransactionSession();
}

bool applyClockConfigForMode(
  const String& mode,
  const ClockConfig& clockConfig,
  const char*& reason
) {
  ClockConfig* target = nullptr;
  if (mode == ModeTags::CLOCK) {
    target = &ConfigManager::clockConfig;
  } else if (mode == ModeTags::ANIMATION || mode == ModeTags::GIF_PLAYER) {
    target = &ConfigManager::animClockConfig;
  } else if (mode == ModeTags::LED_MATRIX_SHOWCASE) {
    target = &ConfigManager::ledMatrixShowcaseClockConfig;
  } else if (mode == ModeTags::THEME) {
    target = &ConfigManager::themeClockConfig;
  } else if (mode == ModeTags::TETRIS || mode == ModeTags::TETRIS_CLOCK) {
    target = &ConfigManager::tetrisOverlayClockConfig;
  } else {
    reason = "clock config target mode invalid";
    return false;
  }

  *target = clockConfig;
  if (mode == ModeTags::ANIMATION || mode == ModeTags::GIF_PLAYER) {
    ConfigManager::saveAnimClockConfig();
  } else if (mode == ModeTags::LED_MATRIX_SHOWCASE) {
    ConfigManager::saveLedMatrixShowcaseClockConfig();
  } else if (mode == ModeTags::THEME) {
    ConfigManager::saveThemeClockConfig();
  } else if (mode == ModeTags::TETRIS || mode == ModeTags::TETRIS_CLOCK) {
    ConfigManager::saveTetrisOverlayClockConfig();
  } else {
    ConfigManager::saveClockConfig();
  }

  reason = nullptr;
  return true;
}

bool modeSupportsClockConfig(const String& mode) {
  return mode == ModeTags::CLOCK ||
         mode == ModeTags::ANIMATION ||
         mode == ModeTags::GIF_PLAYER ||
         mode == ModeTags::LED_MATRIX_SHOWCASE ||
         mode == ModeTags::THEME ||
         mode == ModeTags::TETRIS ||
         mode == ModeTags::TETRIS_CLOCK;
}

void resetPreparedCommand(RuntimeCommandBus::RuntimeCommand& command) {
  command = RuntimeCommandBus::RuntimeCommand{};
  command.source = RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET;
}

bool parseClockConfigPayload(
  JsonObject params,
  const String& mode,
  const char*& reason
) {
  if (!params.containsKey("config")) {
    return true;
  }

  if (!modeSupportsClockConfig(mode)) {
    reason = nullptr;
    return true;
  }

  JsonObject config = params["config"].as<JsonObject>();
  if (config.isNull()) {
    reason = "config invalid";
    return false;
  }

  if (!config.containsKey("font") ||
      !config.containsKey("showSeconds") ||
      !config.containsKey("hourFormat") ||
      !config.containsKey("time") ||
      !config.containsKey("date") ||
      !config.containsKey("week") ||
      !config.containsKey("image")) {
    reason = "clock config fields missing";
    return false;
  }

  const char* fontName = config["font"];
  uint8_t fontId = 0;
  if (fontName == nullptr || !clockFontIdFromString(fontName, fontId)) {
    reason = "clock font invalid";
    return false;
  }

  const int hourFormat = config["hourFormat"].as<int>();
  if (hourFormat != 12 && hourFormat != 24) {
    reason = "clock hourFormat invalid";
    return false;
  }

  JsonObject time = config["time"].as<JsonObject>();
  JsonObject date = config["date"].as<JsonObject>();
  JsonObject week = config["week"].as<JsonObject>();
  JsonObject image = config["image"].as<JsonObject>();
  if (time.isNull() || date.isNull() || week.isNull() || image.isNull()) {
    reason = "clock nested config invalid";
    return false;
  }

  if (!time.containsKey("show") ||
      !time.containsKey("fontSize") ||
      !time.containsKey("x") ||
      !time.containsKey("y") ||
      !time.containsKey("color") ||
      !date.containsKey("show") ||
      !date.containsKey("fontSize") ||
      !date.containsKey("x") ||
      !date.containsKey("y") ||
      !date.containsKey("color") ||
      !week.containsKey("show") ||
      !week.containsKey("x") ||
      !week.containsKey("y") ||
      !week.containsKey("color") ||
      !image.containsKey("show") ||
      !image.containsKey("x") ||
      !image.containsKey("y") ||
      !image.containsKey("width") ||
      !image.containsKey("height")) {
    reason = "clock nested config fields missing";
    return false;
  }

  JsonObject timeColor = time["color"].as<JsonObject>();
  JsonObject dateColor = date["color"].as<JsonObject>();
  JsonObject weekColor = week["color"].as<JsonObject>();
  if (!wsEnsureColorObject(timeColor) ||
      !wsEnsureColorObject(dateColor) ||
      !wsEnsureColorObject(weekColor)) {
    reason = "clock color invalid";
    return false;
  }

  ClockConfig nextConfig;
  nextConfig.font = fontId;
  nextConfig.showSeconds = config["showSeconds"].as<bool>();
  nextConfig.hourFormat = static_cast<uint8_t>(hourFormat);

  nextConfig.time.show = time["show"].as<bool>();
  nextConfig.time.fontSize = static_cast<uint8_t>(time["fontSize"].as<int>());
  nextConfig.time.x = static_cast<int8_t>(time["x"].as<int>());
  nextConfig.time.y = static_cast<int8_t>(time["y"].as<int>());
  nextConfig.time.r = timeColor["r"].as<uint8_t>();
  nextConfig.time.g = timeColor["g"].as<uint8_t>();
  nextConfig.time.b = timeColor["b"].as<uint8_t>();

  nextConfig.date.show = date["show"].as<bool>();
  nextConfig.date.fontSize = static_cast<uint8_t>(date["fontSize"].as<int>());
  nextConfig.date.x = static_cast<int8_t>(date["x"].as<int>());
  nextConfig.date.y = static_cast<int8_t>(date["y"].as<int>());
  nextConfig.date.r = dateColor["r"].as<uint8_t>();
  nextConfig.date.g = dateColor["g"].as<uint8_t>();
  nextConfig.date.b = dateColor["b"].as<uint8_t>();

  nextConfig.week.show = week["show"].as<bool>();
  nextConfig.week.x = static_cast<int8_t>(week["x"].as<int>());
  nextConfig.week.y = static_cast<int8_t>(week["y"].as<int>());
  nextConfig.week.r = weekColor["r"].as<uint8_t>();
  nextConfig.week.g = weekColor["g"].as<uint8_t>();
  nextConfig.week.b = weekColor["b"].as<uint8_t>();

  nextConfig.image.show = image["show"].as<bool>();
  nextConfig.image.x = static_cast<int8_t>(image["x"].as<int>());
  nextConfig.image.y = static_cast<int8_t>(image["y"].as<int>());
  nextConfig.image.width = static_cast<uint8_t>(image["width"].as<int>());
  nextConfig.image.height = static_cast<uint8_t>(image["height"].as<int>());

  gWebSocketTransactionSession.clockConfig = nextConfig;
  gWebSocketTransactionSession.hasClockConfig = true;
  reason = nullptr;
  return true;
}

bool parseEyesConfigPayload(JsonObject params, const char*& reason) {
  if (!params.containsKey("config")) {
    return true;
  }

  JsonObject config = params["config"].as<JsonObject>();
  JsonObject layout = config["layout"].as<JsonObject>();
  JsonObject behavior = config["behavior"].as<JsonObject>();
  JsonObject interaction = config["interaction"].as<JsonObject>();
  JsonObject time = config["time"].as<JsonObject>();
  JsonObject style = config["style"].as<JsonObject>();
  if (layout.isNull() || behavior.isNull() || interaction.isNull() || time.isNull() || style.isNull()) {
    reason = "eyes config sections missing";
    return false;
  }

  if (!layout.containsKey("eyeY") ||
      !layout.containsKey("eyeSpacing") ||
      !layout.containsKey("eyeWidth") ||
      !layout.containsKey("eyeHeight") ||
      !layout.containsKey("timeX") ||
      !layout.containsKey("timeY") ||
      !behavior.containsKey("autoSwitch") ||
      !behavior.containsKey("blinkIntervalMs") ||
      !behavior.containsKey("lookIntervalMs") ||
      !behavior.containsKey("idleMove") ||
      !behavior.containsKey("sleepyAfterMs") ||
      !behavior.containsKey("expressionRhythm") ||
      !interaction.containsKey("lookHoldMs") ||
      !interaction.containsKey("moodHoldMs") ||
      !time.containsKey("show") ||
      !time.containsKey("showSeconds") ||
      !time.containsKey("font") ||
      !time.containsKey("fontSize") ||
      !style.containsKey("eyeColor") ||
      !style.containsKey("timeColor")) {
    reason = "eyes config fields missing";
    return false;
  }

  const char* eyeColor = style["eyeColor"];
  const char* timeColor = style["timeColor"];
  const char* timeFont = time["font"];
  const char* expressionRhythmText = behavior["expressionRhythm"];
  uint8_t timeFontId = 0;
  uint8_t expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_STANDARD;
  const int timeFontSize = time["fontSize"].as<int>();
  if (!isHexColorText(eyeColor) ||
      !isHexColorText(timeColor)) {
    reason = "eyes color invalid";
    return false;
  }
  if (timeFont == nullptr || !clockFontIdFromString(timeFont, timeFontId)) {
    reason = "eyes font invalid";
    return false;
  }
  if (expressionRhythmText == nullptr) {
    reason = "eyes expression rhythm invalid";
    return false;
  }
  if (strcmp(expressionRhythmText, "slow") == 0) {
    expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_SLOW;
  } else if (strcmp(expressionRhythmText, "standard") == 0) {
    expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_STANDARD;
  } else if (strcmp(expressionRhythmText, "lively") == 0) {
    expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_LIVELY;
  } else {
    reason = "eyes expression rhythm invalid";
    return false;
  }
  if (timeFontSize < 1 || timeFontSize > 3) {
    reason = "eyes time font size invalid";
    return false;
  }

  EyesConfig nextConfig;
  nextConfig.layout.eyeY = layout["eyeY"].as<int>();
  nextConfig.layout.eyeSpacing = layout["eyeSpacing"].as<int>();
  nextConfig.layout.eyeWidth = layout["eyeWidth"].as<int>();
  nextConfig.layout.eyeHeight = layout["eyeHeight"].as<int>();
  nextConfig.layout.timeX = layout["timeX"].as<int>();
  nextConfig.layout.timeY = layout["timeY"].as<int>();

  nextConfig.behavior.autoSwitch = behavior["autoSwitch"].as<bool>();
  nextConfig.behavior.blinkIntervalMs = behavior["blinkIntervalMs"].as<uint16_t>();
  nextConfig.behavior.lookIntervalMs = behavior["lookIntervalMs"].as<uint16_t>();
  nextConfig.behavior.idleMove = behavior["idleMove"].as<uint8_t>();
  nextConfig.behavior.sleepyAfterMs = behavior["sleepyAfterMs"].as<uint32_t>();
  nextConfig.behavior.expressionRhythm = expressionRhythm;

  nextConfig.interaction.lookHoldMs = interaction["lookHoldMs"].as<uint16_t>();
  nextConfig.interaction.moodHoldMs = interaction["moodHoldMs"].as<uint16_t>();

  nextConfig.time.show = time["show"].as<bool>();
  nextConfig.time.showSeconds = time["showSeconds"].as<bool>();
  nextConfig.time.font = timeFontId;
  nextConfig.time.fontSize = static_cast<uint8_t>(timeFontSize);

  memcpy(nextConfig.style.eyeColor, eyeColor, sizeof(nextConfig.style.eyeColor));
  memcpy(nextConfig.style.timeColor, timeColor, sizeof(nextConfig.style.timeColor));

  gWebSocketTransactionSession.eyesConfig = nextConfig;
  gWebSocketTransactionSession.hasEyesConfig = true;
  reason = nullptr;
  return true;
}

bool prepareSimpleModeSwitchCommand(
  const String& mode,
  RuntimeCommandBus::RuntimeCommand& command,
  const char*& reason
) {
  resetPreparedCommand(command);
  command.type = RuntimeCommandBus::RuntimeCommandType::MODE_SWITCH;
  command.flag1 = true;
  command.flag2 = true;

  if (mode == ModeTags::CLOCK) {
    command.targetMode = MODE_CLOCK;
    command.businessModeTag = ModeTags::CLOCK;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::CANVAS) {
    command.targetMode = MODE_CANVAS;
    command.businessModeTag = ModeTags::CANVAS;
    command.flag1 = false;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::ANIMATION) {
    command.targetMode = MODE_ANIMATION;
    command.businessModeTag = ModeTags::ANIMATION;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::GIF_PLAYER) {
    command.targetMode = MODE_ANIMATION;
    command.businessModeTag = ModeTags::GIF_PLAYER;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::THEME) {
    command.targetMode = MODE_THEME;
    command.businessModeTag = ModeTags::THEME;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::TEXT_DISPLAY) {
    command.targetMode = MODE_ANIMATION;
    command.businessModeTag = ModeTags::TEXT_DISPLAY;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::BREATH_EFFECT) {
    command.targetMode = MODE_ANIMATION;
    command.businessModeTag = ModeTags::BREATH_EFFECT;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::RHYTHM_EFFECT) {
    command.targetMode = MODE_ANIMATION;
    command.businessModeTag = ModeTags::RHYTHM_EFFECT;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::AMBIENT_EFFECT || mode == ModeTags::LED_MATRIX_SHOWCASE) {
    command.targetMode = MODE_ANIMATION;
    command.businessModeTag = mode;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::PLANET_SCREENSAVER) {
    command.targetMode = MODE_ANIMATION;
    command.businessModeTag = ModeTags::PLANET_SCREENSAVER;
    reason = nullptr;
    return true;
  }
  if (mode == ModeTags::EYES) {
    command.targetMode = MODE_ANIMATION;
    command.businessModeTag = ModeTags::EYES;
    reason = nullptr;
    return true;
  }

  reason = "mode unsupported";
  return false;
}

bool parseTetrisPiecesArray(JsonArray pieces, uint8_t& piecesMask) {
  if (pieces.isNull()) {
    return false;
  }

  uint8_t mask = 0;
  for (JsonVariant item : pieces) {
    if (!item.is<int>()) {
      return false;
    }
    const int index = item.as<int>();
    if (index < 0 || index >= 7) {
      return false;
    }
    mask |= static_cast<uint8_t>(1U << index);
  }

  if (mask == 0) {
    return false;
  }

  piecesMask = mask;
  return true;
}

bool prepareThemeTransaction(JsonObject params, const char*& reason) {
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  if (!params.containsKey("themeId")) {
    return prepareSimpleModeSwitchCommand(ModeTags::THEME, command, reason);
  }

  const char* themeId = params["themeId"];
  if (themeId == nullptr || themeId[0] == '\0') {
    reason = "themeId invalid";
    return false;
  }

  resetPreparedCommand(command);
  command.type = RuntimeCommandBus::RuntimeCommandType::THEME_CONFIG;
  snprintf(command.themeConfig.themeId, sizeof(command.themeConfig.themeId), "%s", themeId);
  gWebSocketTransactionSession.preparedCommandValid = true;
  reason = nullptr;
  return true;
}

bool prepareBreathTransaction(JsonObject params, const char*& reason) {
  if (params.size() == 0) {
    return prepareSimpleModeSwitchCommand(
      ModeTags::BREATH_EFFECT,
      gWebSocketTransactionSession.preparedCommand,
      reason
    );
  }

  if (!params.containsKey("speed") ||
      !params.containsKey("loop") ||
      !params.containsKey("motion") ||
      !params.containsKey("scope") ||
      !params.containsKey("colorMode") ||
      !params.containsKey("colorA") ||
      !params.containsKey("colorB")) {
    reason = "breath fields missing";
    return false;
  }

  JsonObject colorA = params["colorA"].as<JsonObject>();
  JsonObject colorB = params["colorB"].as<JsonObject>();
  if (!wsEnsureColorObject(colorA) || !wsEnsureColorObject(colorB)) {
    reason = "breath color invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::BREATH_EFFECT;
  command.breathEffectConfig.speed = params["speed"].as<uint8_t>();
  command.breathEffectConfig.loop = params["loop"].as<bool>();

  String motion = params["motion"].as<String>();
  if (motion == "clockwise") {
    command.breathEffectConfig.motion = BREATH_MOTION_CLOCKWISE;
  } else if (motion == "counterclockwise") {
    command.breathEffectConfig.motion = BREATH_MOTION_COUNTERCLOCKWISE;
  } else if (motion == "inward") {
    command.breathEffectConfig.motion = BREATH_MOTION_INWARD;
  } else if (motion == "outward") {
    command.breathEffectConfig.motion = BREATH_MOTION_OUTWARD;
  } else {
    reason = "breath motion invalid";
    return false;
  }

  String scope = params["scope"].as<String>();
  if (scope == "single_ring") {
    command.breathEffectConfig.scope = BREATH_SCOPE_SINGLE_RING;
  } else if (scope == "full_screen") {
    command.breathEffectConfig.scope = BREATH_SCOPE_FULL_SCREEN;
  } else {
    reason = "breath scope invalid";
    return false;
  }

  String colorMode = params["colorMode"].as<String>();
  if (colorMode == "solid") {
    command.breathEffectConfig.colorMode = BREATH_COLOR_SOLID;
  } else if (colorMode == "gradient") {
    command.breathEffectConfig.colorMode = BREATH_COLOR_GRADIENT;
  } else {
    reason = "breath colorMode invalid";
    return false;
  }

  command.breathEffectConfig.colorAR = colorA["r"].as<uint8_t>();
  command.breathEffectConfig.colorAG = colorA["g"].as<uint8_t>();
  command.breathEffectConfig.colorAB = colorA["b"].as<uint8_t>();
  command.breathEffectConfig.colorBR = colorB["r"].as<uint8_t>();
  command.breathEffectConfig.colorBG = colorB["g"].as<uint8_t>();
  command.breathEffectConfig.colorBB = colorB["b"].as<uint8_t>();
  reason = nullptr;
  return true;
}

bool prepareRhythmTransaction(JsonObject params, const char*& reason) {
  if (params.size() == 0) {
    return prepareSimpleModeSwitchCommand(
      ModeTags::RHYTHM_EFFECT,
      gWebSocketTransactionSession.preparedCommand,
      reason
    );
  }

  if (!params.containsKey("bpm") ||
      !params.containsKey("speed") ||
      !params.containsKey("loop") ||
      !params.containsKey("direction") ||
      !params.containsKey("strength") ||
      !params.containsKey("mode") ||
      !params.containsKey("colorA") ||
      !params.containsKey("colorB")) {
    reason = "rhythm fields missing";
    return false;
  }

  JsonObject colorA = params["colorA"].as<JsonObject>();
  JsonObject colorB = params["colorB"].as<JsonObject>();
  if (!wsEnsureColorObject(colorA) || !wsEnsureColorObject(colorB)) {
    reason = "rhythm color invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::RHYTHM_EFFECT;
  command.rhythmEffectConfig.bpm = params["bpm"].as<uint16_t>();
  command.rhythmEffectConfig.speed = params["speed"].as<uint8_t>();
  command.rhythmEffectConfig.loop = params["loop"].as<bool>();
  command.rhythmEffectConfig.strength = params["strength"].as<uint8_t>();

  String direction = params["direction"].as<String>();
  if (direction == "left") {
    command.rhythmEffectConfig.direction = RHYTHM_DIR_LEFT;
  } else if (direction == "right") {
    command.rhythmEffectConfig.direction = RHYTHM_DIR_RIGHT;
  } else if (direction == "up") {
    command.rhythmEffectConfig.direction = RHYTHM_DIR_UP;
  } else if (direction == "down") {
    command.rhythmEffectConfig.direction = RHYTHM_DIR_DOWN;
  } else {
    reason = "rhythm direction invalid";
    return false;
  }

  String effectMode = params["mode"].as<String>();
  if (effectMode == "pulse") {
    command.rhythmEffectConfig.mode = RHYTHM_MODE_PULSE;
  } else if (effectMode == "gradient") {
    command.rhythmEffectConfig.mode = RHYTHM_MODE_GRADIENT;
  } else if (effectMode == "jump") {
    command.rhythmEffectConfig.mode = RHYTHM_MODE_JUMP;
  } else {
    reason = "rhythm mode invalid";
    return false;
  }

  command.rhythmEffectConfig.colorAR = colorA["r"].as<uint8_t>();
  command.rhythmEffectConfig.colorAG = colorA["g"].as<uint8_t>();
  command.rhythmEffectConfig.colorAB = colorA["b"].as<uint8_t>();
  command.rhythmEffectConfig.colorBR = colorB["r"].as<uint8_t>();
  command.rhythmEffectConfig.colorBG = colorB["g"].as<uint8_t>();
  command.rhythmEffectConfig.colorBB = colorB["b"].as<uint8_t>();
  reason = nullptr;
  return true;
}

bool prepareAmbientTransaction(const String& mode, JsonObject params, const char*& reason) {
  if (params.size() == 0) {
    return prepareSimpleModeSwitchCommand(
      mode,
      gWebSocketTransactionSession.preparedCommand,
      reason
    );
  }

  if (!params.containsKey("preset") ||
      !params.containsKey("speed") ||
      !params.containsKey("loop")) {
    reason = "ambient fields missing";
    return false;
  }

  String presetText = params["preset"].as<String>();
  uint8_t preset = AMBIENT_PRESET_AURORA;
  if (!AmbientPresetCodec::fromString(presetText, preset)) {
    reason = "ambient preset invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::AMBIENT_EFFECT;
  command.targetMode = MODE_ANIMATION;
  command.businessModeTag = mode;
  command.ambientEffectConfig.preset = preset;
  command.ambientEffectConfig.speed = params["speed"].as<uint8_t>();
  command.ambientEffectConfig.loop = params["loop"].as<bool>();
  if (command.ambientEffectConfig.speed < 1) {
    command.ambientEffectConfig.speed = 1;
  }
  if (command.ambientEffectConfig.speed > 10) {
    command.ambientEffectConfig.speed = 10;
  }

  if (command.ambientEffectConfig.preset == AMBIENT_PRESET_SUNSET_BLUSH) {
    if (!params.containsKey("density") || !params.containsKey("color")) {
      reason = "ambient density fields missing";
      return false;
    }
    JsonObject color = params["color"].as<JsonObject>();
    if (!wsEnsureColorObject(color)) {
      reason = "ambient color invalid";
      return false;
    }
    command.ambientEffectConfig.intensity = DisplayManager::ambientEffectConfig.intensity;
    command.ambientEffectConfig.density = params["density"].as<uint8_t>();
    if (command.ambientEffectConfig.density < 10) {
      command.ambientEffectConfig.density = 10;
    }
    if (command.ambientEffectConfig.density > 100) {
      command.ambientEffectConfig.density = 100;
    }
    command.ambientEffectConfig.colorR = color["r"].as<uint8_t>();
    command.ambientEffectConfig.colorG = color["g"].as<uint8_t>();
    command.ambientEffectConfig.colorB = color["b"].as<uint8_t>();
  } else if (isWaterWorldAmbientPreset(command.ambientEffectConfig.preset)) {
    command.ambientEffectConfig.intensity =
      waterWorldAmbientPresetIntensity(command.ambientEffectConfig.preset);
    command.ambientEffectConfig.density = DisplayManager::ambientEffectConfig.density;
    command.ambientEffectConfig.colorR = DisplayManager::ambientEffectConfig.colorR;
    command.ambientEffectConfig.colorG = DisplayManager::ambientEffectConfig.colorG;
    command.ambientEffectConfig.colorB = DisplayManager::ambientEffectConfig.colorB;
  } else {
    if (!params.containsKey("intensity")) {
      reason = "ambient intensity missing";
      return false;
    }
    command.ambientEffectConfig.intensity = params["intensity"].as<uint8_t>();
    if (command.ambientEffectConfig.intensity < 10) {
      command.ambientEffectConfig.intensity = 10;
    }
    if (command.ambientEffectConfig.intensity > 100) {
      command.ambientEffectConfig.intensity = 100;
    }
    command.ambientEffectConfig.density = DisplayManager::ambientEffectConfig.density;
    command.ambientEffectConfig.colorR = DisplayManager::ambientEffectConfig.colorR;
    command.ambientEffectConfig.colorG = DisplayManager::ambientEffectConfig.colorG;
    command.ambientEffectConfig.colorB = DisplayManager::ambientEffectConfig.colorB;
  }

  command.stringValue1 = AmbientPresetCodec::toString(command.ambientEffectConfig.preset);
  reason = nullptr;
  return true;
}

bool prepareTextDisplayTransaction(JsonObject params, const char*& reason) {
  if (params.size() == 0) {
    return prepareSimpleModeSwitchCommand(
      ModeTags::TEXT_DISPLAY,
      gWebSocketTransactionSession.preparedCommand,
      reason
    );
  }

  if (!params.containsKey("template") ||
      !params.containsKey("text") ||
      !params.containsKey("progress") ||
      !params.containsKey("repeat") ||
      !params.containsKey("pushIcon") ||
      !params.containsKey("icon") ||
      !params.containsKey("speed") ||
      !params.containsKey("color") ||
      !params.containsKey("bgColor")) {
    reason = "text_display fields missing";
    return false;
  }

  JsonObject color = params["color"].as<JsonObject>();
  JsonObject bgColor = params["bgColor"].as<JsonObject>();
  if (!wsEnsureColorObject(color) || !wsEnsureColorObject(bgColor)) {
    reason = "text_display color invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::TEXT_DISPLAY;
  command.textDisplayConfig.templateName = params["template"].as<const char*>();
  command.textDisplayConfig.text = params["text"].as<const char*>();
  command.textDisplayConfig.progress =
    static_cast<uint8_t>(wsClampInt(params["progress"].as<int>(), 0, 100));
  command.textDisplayConfig.repeat =
    static_cast<uint8_t>(wsClampInt(params["repeat"].as<int>(), 1, 8));
  command.textDisplayConfig.pushIcon = params["pushIcon"].as<bool>();
  command.textDisplayConfig.icon = params["icon"].as<const char*>();
  command.textDisplayConfig.speed =
    static_cast<uint8_t>(wsClampInt(params["speed"].as<int>(), 1, 7));
  command.textDisplayConfig.colorR = color["r"].as<uint8_t>();
  command.textDisplayConfig.colorG = color["g"].as<uint8_t>();
  command.textDisplayConfig.colorB = color["b"].as<uint8_t>();
  command.textDisplayConfig.bgColorR = bgColor["r"].as<uint8_t>();
  command.textDisplayConfig.bgColorG = bgColor["g"].as<uint8_t>();
  command.textDisplayConfig.bgColorB = bgColor["b"].as<uint8_t>();
  reason = nullptr;
  return true;
}

bool preparePlanetTransaction(JsonObject params, const char*& reason) {
  if (params.size() == 0) {
    return prepareSimpleModeSwitchCommand(
      ModeTags::PLANET_SCREENSAVER,
      gWebSocketTransactionSession.preparedCommand,
      reason
    );
  }

  if (!params.containsKey("preset") ||
      !params.containsKey("size") ||
      !params.containsKey("direction") ||
      !params.containsKey("speed") ||
      !params.containsKey("seed") ||
      !params.containsKey("colorSeed") ||
      !params.containsKey("planetX") ||
      !params.containsKey("planetY") ||
      !params.containsKey("font") ||
      !params.containsKey("showSeconds") ||
      !params.containsKey("time")) {
    reason = "planet fields missing";
    return false;
  }

  const char* preset = params["preset"];
  const char* size = params["size"];
  const char* direction = params["direction"];
  const char* fontName = params["font"];
  uint8_t fontId = 0;
  if (preset == nullptr || size == nullptr || direction == nullptr || fontName == nullptr) {
    reason = "planet fields invalid";
    return false;
  }
  if (!clockFontIdFromString(fontName, fontId)) {
    reason = "planet font invalid";
    return false;
  }

  JsonObject time = params["time"].as<JsonObject>();
  JsonObject timeColor = time["color"].as<JsonObject>();
  if (time.isNull() || !time.containsKey("show") || !time.containsKey("fontSize") ||
      !time.containsKey("x") || !time.containsKey("y") || !wsEnsureColorObject(timeColor)) {
    reason = "planet time invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::PLANET_SCREENSAVER;
  snprintf(command.planetConfig.preset, sizeof(command.planetConfig.preset), "%s", preset);
  snprintf(command.planetConfig.size, sizeof(command.planetConfig.size), "%s", size);
  snprintf(command.planetConfig.direction, sizeof(command.planetConfig.direction), "%s", direction);
  command.planetConfig.speed = static_cast<uint8_t>(wsClampInt(params["speed"].as<int>(), 1, 7));
  command.planetConfig.seed = params["seed"].as<uint32_t>();
  command.planetConfig.colorSeed = params["colorSeed"].as<uint32_t>();
  command.planetConfig.planetX =
    static_cast<uint8_t>(wsClampInt(params["planetX"].as<int>(), 0, 63));
  command.planetConfig.planetY =
    static_cast<uint8_t>(wsClampInt(params["planetY"].as<int>(), 0, 63));
  command.planetConfig.font = fontId;
  command.planetConfig.showSeconds = params["showSeconds"].as<bool>();
  command.planetConfig.time.show = time["show"].as<bool>();
  command.planetConfig.time.fontSize =
    static_cast<uint8_t>(wsClampInt(time["fontSize"].as<int>(), 1, 3));
  command.planetConfig.time.x =
    static_cast<uint8_t>(wsClampInt(time["x"].as<int>(), 0, 63));
  command.planetConfig.time.y =
    static_cast<uint8_t>(wsClampInt(time["y"].as<int>(), 0, 63));
  command.planetConfig.time.r = timeColor["r"].as<uint8_t>();
  command.planetConfig.time.g = timeColor["g"].as<uint8_t>();
  command.planetConfig.time.b = timeColor["b"].as<uint8_t>();
  reason = nullptr;
  return true;
}

bool prepareMazeTransaction(JsonObject params, const char*& reason) {
  if (!params.containsKey("speed") ||
      !params.containsKey("mazeSizeMode") ||
      !params.containsKey("showClock") ||
      !params.containsKey("panelBgColor") ||
      !params.containsKey("borderColor") ||
      !params.containsKey("timeColor") ||
      !params.containsKey("dateColor") ||
      !params.containsKey("generationPathColor") ||
      !params.containsKey("searchVisitedColor") ||
      !params.containsKey("searchFrontierColor") ||
      !params.containsKey("solvedPathStartColor") ||
      !params.containsKey("solvedPathEndColor")) {
    reason = "maze fields missing";
    return false;
  }

  const int speed = params["speed"].as<int>();
  if (speed < 1 || speed > 10) {
    reason = "maze speed invalid";
    return false;
  }

  String mazeSizeMode = params["mazeSizeMode"].as<String>();
  int mazeSizeModeValue = -1;
  if (mazeSizeMode == "wide") {
    mazeSizeModeValue = MAZE_SIZE_WIDE;
  } else if (mazeSizeMode == "dense") {
    mazeSizeModeValue = MAZE_SIZE_DENSE;
  } else {
    reason = "mazeSizeMode invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::MODE_SWITCH;
  command.targetMode = MODE_ANIMATION;
  command.businessModeTag = ModeTags::MAZE;
  command.mazeConfig.speed = static_cast<uint8_t>(speed);
  command.mazeConfig.mazeSizeMode = static_cast<uint8_t>(mazeSizeModeValue);
  command.mazeConfig.showClock = params["showClock"].as<bool>();
  if (!parseRequiredHexColorText(params, "panelBgColor", command.mazeConfig.panelBgColor, sizeof(command.mazeConfig.panelBgColor)) ||
      !parseRequiredHexColorText(params, "borderColor", command.mazeConfig.borderColor, sizeof(command.mazeConfig.borderColor)) ||
      !parseRequiredHexColorText(params, "timeColor", command.mazeConfig.timeColor, sizeof(command.mazeConfig.timeColor)) ||
      !parseRequiredHexColorText(params, "dateColor", command.mazeConfig.dateColor, sizeof(command.mazeConfig.dateColor)) ||
      !parseRequiredHexColorText(params, "generationPathColor", command.mazeConfig.generationPathColor, sizeof(command.mazeConfig.generationPathColor)) ||
      !parseRequiredHexColorText(params, "searchVisitedColor", command.mazeConfig.searchVisitedColor, sizeof(command.mazeConfig.searchVisitedColor)) ||
      !parseRequiredHexColorText(params, "searchFrontierColor", command.mazeConfig.searchFrontierColor, sizeof(command.mazeConfig.searchFrontierColor)) ||
      !parseRequiredHexColorText(params, "solvedPathStartColor", command.mazeConfig.solvedPathStartColor, sizeof(command.mazeConfig.solvedPathStartColor)) ||
      !parseRequiredHexColorText(params, "solvedPathEndColor", command.mazeConfig.solvedPathEndColor, sizeof(command.mazeConfig.solvedPathEndColor))) {
    reason = "maze color invalid";
    return false;
  }
  reason = nullptr;
  return true;
}

bool prepareSnakeTransaction(JsonObject params, const char*& reason) {
  if (!params.containsKey("speed") ||
      !params.containsKey("snakeWidth") ||
      !params.containsKey("snakeColor") ||
      !params.containsKey("foodColor") ||
      !params.containsKey("font") ||
      !params.containsKey("showSeconds") ||
      !params.containsKey("snakeSkin")) {
    reason = "snake fields missing";
    return false;
  }

  const int speed = params["speed"].as<int>();
  const int snakeWidth = params["snakeWidth"].as<int>();
  if (speed < 1 || speed > 10 || snakeWidth < 2 || snakeWidth > 4) {
    reason = "snake config invalid";
    return false;
  }

  const char* fontText = params["font"];
  const char* snakeSkin = params["snakeSkin"];
  uint8_t fontId = 0;
  if (fontText == nullptr || snakeSkin == nullptr || !clockFontIdFromString(fontText, fontId)) {
    reason = "snake font invalid";
    return false;
  }
  if (strcmp(snakeSkin, "solid") != 0 &&
      strcmp(snakeSkin, "gradient") != 0 &&
      strcmp(snakeSkin, "spotted") != 0) {
    reason = "snake skin invalid";
    return false;
  }

  JsonObject snakeColor = params["snakeColor"].as<JsonObject>();
  JsonObject foodColor = params["foodColor"].as<JsonObject>();
  if (!wsEnsureColorObject(snakeColor) || !wsEnsureColorObject(foodColor)) {
    reason = "snake color invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::MODE_SWITCH;
  command.targetMode = MODE_ANIMATION;
  command.businessModeTag = ModeTags::SNAKE;
  command.snakeConfig.speed = static_cast<uint8_t>(speed);
  command.snakeConfig.snakeWidth = static_cast<uint8_t>(snakeWidth);
  command.snakeConfig.snakeColorR = snakeColor["r"].as<uint8_t>();
  command.snakeConfig.snakeColorG = snakeColor["g"].as<uint8_t>();
  command.snakeConfig.snakeColorB = snakeColor["b"].as<uint8_t>();
  command.snakeConfig.foodColorR = foodColor["r"].as<uint8_t>();
  command.snakeConfig.foodColorG = foodColor["g"].as<uint8_t>();
  command.snakeConfig.foodColorB = foodColor["b"].as<uint8_t>();
  command.snakeConfig.font = fontId;
  command.snakeConfig.showSeconds = params["showSeconds"].as<bool>();
  snprintf(command.snakeConfig.snakeSkin, sizeof(command.snakeConfig.snakeSkin), "%s", snakeSkin);
  reason = nullptr;
  return true;
}

bool prepareTetrisTransaction(JsonObject params, const char*& reason) {
  if (!params.containsKey("clearMode") ||
      !params.containsKey("cellSize") ||
      !params.containsKey("speed") ||
      !params.containsKey("showClock") ||
      !params.containsKey("pieces")) {
    reason = "tetris fields missing";
    return false;
  }

  uint8_t piecesMask = 0;
  if (!parseTetrisPiecesArray(params["pieces"].as<JsonArray>(), piecesMask)) {
    reason = "tetris pieces invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::MODE_SWITCH;
  command.targetMode = MODE_ANIMATION;
  command.businessModeTag = ModeTags::TETRIS;
  command.flag1 = params["clearMode"].as<bool>();
  command.flag2 = params["showClock"].as<bool>();
  command.intValue1 = params["cellSize"].as<int>();
  command.intValue2 = params["speed"].as<int>();
  command.byteValue1 = piecesMask;
  reason = nullptr;
  return true;
}

bool prepareTetrisClockTransaction(JsonObject params, const char*& reason) {
  if (!params.containsKey("cellSize") ||
      !params.containsKey("speed") ||
      !params.containsKey("hourFormat")) {
    reason = "tetris_clock fields missing";
    return false;
  }

  const int cellSize = params["cellSize"].as<int>();
  const int speed = params["speed"].as<int>();
  const int hourFormat = params["hourFormat"].as<int>();
  if ((cellSize != 1 && cellSize != 2) || speed <= 0 || (hourFormat != 12 && hourFormat != 24)) {
    reason = "tetris_clock config invalid";
    return false;
  }

  resetPreparedCommand(gWebSocketTransactionSession.preparedCommand);
  RuntimeCommandBus::RuntimeCommand& command = gWebSocketTransactionSession.preparedCommand;
  command.type = RuntimeCommandBus::RuntimeCommandType::MODE_SWITCH;
  command.targetMode = MODE_ANIMATION;
  command.businessModeTag = ModeTags::TETRIS_CLOCK;
  command.intValue1 = cellSize;
  command.intValue2 = speed;
  command.intValue3 = hourFormat;
  reason = nullptr;
  return true;
}

bool prepareEyesTransaction(JsonObject params, const char*& reason) {
  if (!parseEyesConfigPayload(params, reason)) {
    return false;
  }
  return prepareSimpleModeSwitchCommand(
    ModeTags::EYES,
    gWebSocketTransactionSession.preparedCommand,
    reason
  );
}

bool buildPreparedWebSocketTransaction(
  const String& mode,
  JsonObject params,
  const char*& reason
) {
  gWebSocketTransactionSession.preparedCommandValid = false;
  gWebSocketTransactionSession.hasClockConfig = false;
  gWebSocketTransactionSession.hasEyesConfig = false;

  if (!parseClockConfigPayload(params, mode, reason)) {
    return false;
  }

  bool prepared = false;
  if (mode == ModeTags::CLOCK ||
      mode == ModeTags::CANVAS ||
      mode == ModeTags::ANIMATION ||
      mode == ModeTags::GIF_PLAYER) {
    prepared = prepareSimpleModeSwitchCommand(mode, gWebSocketTransactionSession.preparedCommand, reason);
  } else if (mode == ModeTags::THEME) {
    prepared = prepareThemeTransaction(params, reason);
  } else if (mode == ModeTags::BREATH_EFFECT) {
    prepared = prepareBreathTransaction(params, reason);
  } else if (mode == ModeTags::RHYTHM_EFFECT) {
    prepared = prepareRhythmTransaction(params, reason);
  } else if (mode == ModeTags::AMBIENT_EFFECT || mode == ModeTags::LED_MATRIX_SHOWCASE) {
    prepared = prepareAmbientTransaction(mode, params, reason);
  } else if (mode == ModeTags::TEXT_DISPLAY) {
    prepared = prepareTextDisplayTransaction(params, reason);
  } else if (mode == ModeTags::PLANET_SCREENSAVER) {
    prepared = preparePlanetTransaction(params, reason);
  } else if (mode == ModeTags::EYES) {
    prepared = prepareEyesTransaction(params, reason);
  } else if (mode == ModeTags::MAZE) {
    prepared = prepareMazeTransaction(params, reason);
  } else if (mode == ModeTags::SNAKE) {
    prepared = prepareSnakeTransaction(params, reason);
  } else if (mode == ModeTags::TETRIS) {
    prepared = prepareTetrisTransaction(params, reason);
  } else if (mode == ModeTags::TETRIS_CLOCK) {
    prepared = prepareTetrisClockTransaction(params, reason);
  } else {
    reason = "mode unsupported";
    return false;
  }

  if (!prepared) {
    return false;
  }

  gWebSocketTransactionSession.preparedCommandValid = true;
  return true;
}

WebSocketTransactionBinaryType resolveBinaryTypeForMode(
  const String& mode,
  bool hasBinary,
  size_t binarySize
) {
  if (!hasBinary) {
    return WebSocketTransactionBinaryType::NONE;
  }
  if (mode == ModeTags::CLOCK) {
    if (!isStaticPixelBinarySize(binarySize)) {
      return WebSocketTransactionBinaryType::NONE;
    }
    return WebSocketTransactionBinaryType::STATIC_PIXELS;
  }
  if (mode == ModeTags::CANVAS) {
    if (!isStaticPixelBinarySize(binarySize)) {
      return WebSocketTransactionBinaryType::NONE;
    }
    return WebSocketTransactionBinaryType::STATIC_PIXELS;
  }
  if (mode == ModeTags::ANIMATION) {
    if (isStaticPixelBinarySize(binarySize)) {
      return WebSocketTransactionBinaryType::STATIC_PIXELS;
    }
    if (isCompactAnimationBinarySize(binarySize)) {
      return WebSocketTransactionBinaryType::COMPACT_ANIMATION;
    }
    return WebSocketTransactionBinaryType::NONE;
  }
  if (mode == ModeTags::GIF_PLAYER) {
    if (!isCompactAnimationBinarySize(binarySize)) {
      return WebSocketTransactionBinaryType::NONE;
    }
    return WebSocketTransactionBinaryType::COMPACT_ANIMATION;
  }
  return WebSocketTransactionBinaryType::NONE;
}

bool beginBinaryPayloadForTransaction(const char*& reason) {
  if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::STATIC_PIXELS) {
    if (isCanvasStaticPixelTransaction()) {
      if (!snapshotCanvasTarget()) {
        reason = "canvas backup failed";
        return false;
      }
      if (gWebSocketTransactionSession.binarySize == 0) {
        reason = nullptr;
        return true;
      }
      gWebSocketTransactionSession.animationBinaryBuffer =
        static_cast<uint8_t*>(malloc(gWebSocketTransactionSession.binarySize));
      if (gWebSocketTransactionSession.animationBinaryBuffer == nullptr) {
        reason = "canvas buffer alloc failed";
        return false;
      }
      reason = nullptr;
      return true;
    }

    if (!snapshotStaticPixelTarget(gWebSocketTransactionSession.preparedCommand.targetMode)) {
      reason = "pixel backup failed";
      return false;
    }
    clearStaticPixelTarget(gWebSocketTransactionSession.preparedCommand.targetMode);
    DisplayManager::receivingPixels = true;
    reason = nullptr;
    return true;
  }

  if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::COMPACT_ANIMATION) {
    gWebSocketTransactionSession.animationBinaryBuffer =
      static_cast<uint8_t*>(malloc(gWebSocketTransactionSession.binarySize));
    if (gWebSocketTransactionSession.animationBinaryBuffer == nullptr) {
      reason = "animation buffer alloc failed";
      return false;
    }
    memset(
      gWebSocketTransactionSession.animationBinaryBuffer,
      0,
      gWebSocketTransactionSession.binarySize
    );
    reason = nullptr;
    return true;
  }

  reason = "binary mode unsupported";
  return false;
}

const char* resolveExecuteFailureReason(
  StaticJsonDocument<768>& tempResponse,
  const char* fallbackReason
) {
  const char* reason = tempResponse["message"];
  if (reason == nullptr || reason[0] == '\0') {
    return fallbackReason;
  }
  return reason;
}

bool finalizeCommittedBinary(const char*& reason) {
  if (!gWebSocketTransactionSession.hasBinary) {
    reason = nullptr;
    return true;
  }

  if (gWebSocketTransactionSession.receivedBinarySize != gWebSocketTransactionSession.binarySize) {
    reason = "binary size mismatch";
    return false;
  }

  if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::STATIC_PIXELS) {
    if (isCanvasStaticPixelTransaction()) {
      if (gWebSocketTransactionSession.binarySize > 0 &&
          gWebSocketTransactionSession.animationBinaryBuffer == nullptr) {
        reason = "canvas buffer unavailable";
        return false;
      }

      DisplayManager::clearCanvas();
      DisplayManager::canvasInitialized = true;

      int newBlackCount = 0;
      for (size_t i = 0; i + 4 < gWebSocketTransactionSession.binarySize; i += 5) {
        uint8_t x = gWebSocketTransactionSession.animationBinaryBuffer[i];
        uint8_t y = gWebSocketTransactionSession.animationBinaryBuffer[i + 1];
        uint8_t r = gWebSocketTransactionSession.animationBinaryBuffer[i + 2];
        uint8_t g = gWebSocketTransactionSession.animationBinaryBuffer[i + 3];
        uint8_t b = gWebSocketTransactionSession.animationBinaryBuffer[i + 4];
        if (x < DisplayManager::PANEL_RES_X &&
            y < DisplayManager::PANEL_RES_Y &&
            r == 0 &&
            g == 0 &&
            b == 0) {
          newBlackCount += 1;
        }
      }

      if (newBlackCount > 0) {
        DisplayManager::blackPixels = static_cast<DisplayManager::BlackPixel*>(
          malloc(sizeof(DisplayManager::BlackPixel) * newBlackCount)
        );
        if (DisplayManager::blackPixels == nullptr) {
          reason = "canvas black pixel alloc failed";
          return false;
        }
      }

      for (size_t i = 0; i + 4 < gWebSocketTransactionSession.binarySize; i += 5) {
        uint8_t x = gWebSocketTransactionSession.animationBinaryBuffer[i];
        uint8_t y = gWebSocketTransactionSession.animationBinaryBuffer[i + 1];
        uint8_t r = gWebSocketTransactionSession.animationBinaryBuffer[i + 2];
        uint8_t g = gWebSocketTransactionSession.animationBinaryBuffer[i + 3];
        uint8_t b = gWebSocketTransactionSession.animationBinaryBuffer[i + 4];

        if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y) {
          DisplayManager::canvasBuffer[y][x][0] = r;
          DisplayManager::canvasBuffer[y][x][1] = g;
          DisplayManager::canvasBuffer[y][x][2] = b;
          if (r == 0 && g == 0 && b == 0 && DisplayManager::blackPixels != nullptr) {
            DisplayManager::blackPixels[DisplayManager::blackPixelCount].x = x;
            DisplayManager::blackPixels[DisplayManager::blackPixelCount].y = y;
            DisplayManager::blackPixelCount += 1;
          }
        }
      }

      clearTransactionAnimationBinaryBuffer();
      reason = nullptr;
      return true;
    }

    if (gWebSocketTransactionSession.preparedCommand.targetMode == MODE_ANIMATION) {
      AnimationManager::freeGIFAnimation();
    }
    clearTransactionPixelBackup();
    reason = nullptr;
    return true;
  }

  if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::COMPACT_ANIMATION) {
    if (gWebSocketTransactionSession.binarySize > 0 &&
        gWebSocketTransactionSession.animationBinaryBuffer == nullptr) {
      reason = "animation buffer unavailable";
      return false;
    }

    if (!AnimationManager::stageGIFAnimationBinaryUpload(
          gWebSocketTransactionSession.animationBinaryBuffer,
          gWebSocketTransactionSession.binarySize)) {
      reason = "animation stage failed";
      return false;
    }

    clearTransactionAnimationBinaryBuffer();

    if (!AnimationManager::loadStagedGIFAnimationUpload()) {
      reason = "animation load failed";
      return false;
    }
    reason = nullptr;
    return true;
  }

  reason = "binary mode unsupported";
  return false;
}

bool persistCommittedBinary(const char*& reason) {
  if (!gWebSocketTransactionSession.hasBinary) {
    reason = nullptr;
    return true;
  }

  if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::STATIC_PIXELS) {
    if (isCanvasStaticPixelTransaction()) {
      ConfigManager::saveCanvasPixels();
      reason = nullptr;
      return true;
    }

    if (gWebSocketTransactionSession.preparedCommand.targetMode == MODE_ANIMATION) {
      ConfigManager::saveAnimImagePixels();
      if (!AnimationManager::saveAnimation()) {
        reason = "animation persistence failed";
        return false;
      }
      reason = nullptr;
      return true;
    }

    ConfigManager::saveStaticImagePixels();
    reason = nullptr;
    return true;
  }

  if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::COMPACT_ANIMATION) {
    if (!AnimationManager::saveAnimation()) {
      reason = "animation persistence failed";
      return false;
    }
    reason = nullptr;
    return true;
  }

  reason = "binary mode unsupported";
  return false;
}

bool applyPreparedWebSocketTransaction(const char*& reason) {
  if (gWebSocketTransactionSession.hasClockConfig) {
    if (!applyClockConfigForMode(
          gWebSocketTransactionSession.mode,
          gWebSocketTransactionSession.clockConfig,
          reason)) {
      return false;
    }
  }

  if (gWebSocketTransactionSession.hasEyesConfig) {
    StaticJsonDocument<768> tempResponse;
    RuntimeCommandBus::RuntimeCommand command;
    resetPreparedCommand(command);
    command.type = RuntimeCommandBus::RuntimeCommandType::EYES_CONFIG;
    command.eyesConfig = gWebSocketTransactionSession.eyesConfig;
    if (!executeCommand(command, tempResponse)) {
      reason = resolveExecuteFailureReason(tempResponse, "eyes config apply failed");
      return false;
    }
  }

  if (gWebSocketTransactionSession.preparedCommandValid) {
    StaticJsonDocument<768> tempResponse;
    if (!executeCommand(gWebSocketTransactionSession.preparedCommand, tempResponse)) {
      reason = resolveExecuteFailureReason(tempResponse, "transaction apply failed");
      return false;
    }
  }

  reason = nullptr;
  return true;
}

bool switchToTargetMode(
  DeviceMode fromMode,
  DeviceMode targetMode,
  const String& businessModeTag,
  bool updateLastBusinessTarget,
  bool persistState,
  const char* successMessage,
  StaticJsonDocument<768>& response
) {
  (void)fromMode;
  RuntimeModeCoordinator::deactivateRuntimeContent();
  if (shouldClearScreenBeforeBusinessModeEntry(businessModeTag)) {
    DisplayManager::clearScreen();
  }

  if (!RuntimeModeCoordinator::switchToMode(
        targetMode,
        businessModeTag,
        updateLastBusinessTarget,
        persistState)) {
    setErrorResponse(response, "mode activation failed");
    return false;
  }

  response["message"] = successMessage;
  return true;
}

bool executeModeSwitch(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  DeviceMode fromMode = DisplayManager::currentMode;
  String fromBusinessModeTag = DisplayManager::currentBusinessModeTag;

  if (command.businessModeTag == ModeTags::TRANSFERRING) {
    RuntimeModeCoordinator::deactivateRuntimeContent();
    if (DisplayManager::isLoadingActive) {
      DisplayManager::stopLoadingAnimation();
    }
    DisplayManager::clearScreen();
    DisplayManager::currentMode = MODE_TRANSFERRING;
    DisplayManager::currentBusinessModeTag = fromBusinessModeTag;
    response["message"] = command.successMessage;
    return true;
  }

  if (command.businessModeTag == ModeTags::TETRIS) {
    const TetrisModeConfig previousConfig = ConfigManager::tetrisConfig;
    RuntimeModeCoordinator::deactivateRuntimeContent();
    if (shouldClearScreenBeforeBusinessModeEntry(command.businessModeTag)) {
      DisplayManager::clearScreen();
    }
    ConfigManager::tetrisConfig.clearMode = command.flag1;
    ConfigManager::tetrisConfig.cellSize = static_cast<uint8_t>(command.intValue1);
    ConfigManager::tetrisConfig.speed = static_cast<uint16_t>(command.intValue2);
    ConfigManager::tetrisConfig.showClock = command.flag2;
    ConfigManager::tetrisConfig.pieces = command.byteValue1;

    if (!RuntimeModeCoordinator::switchToMode(
          MODE_ANIMATION,
          command.businessModeTag,
          true,
          true)) {
      ConfigManager::tetrisConfig = previousConfig;
      RuntimeModeCoordinator::restoreCurrentModeFrame();
      setErrorResponse(response, "mode activation failed");
      return false;
    }

    ConfigManager::saveTetrisConfig();
    response["message"] = command.successMessage;
    return true;
  }

  if (command.businessModeTag == ModeTags::TETRIS_CLOCK) {
    const TetrisClockModeConfig previousConfig = ConfigManager::tetrisClockConfig;
    RuntimeModeCoordinator::deactivateRuntimeContent();
    if (shouldClearScreenBeforeBusinessModeEntry(command.businessModeTag)) {
      DisplayManager::clearScreen();
    }
    ConfigManager::tetrisClockConfig.cellSize = static_cast<uint8_t>(command.intValue1);
    ConfigManager::tetrisClockConfig.speed = static_cast<uint16_t>(command.intValue2);
    ConfigManager::tetrisClockConfig.hourFormat = static_cast<uint8_t>(command.intValue3);

    if (!RuntimeModeCoordinator::switchToMode(
          MODE_ANIMATION,
          command.businessModeTag,
          true,
          true)) {
      ConfigManager::tetrisClockConfig = previousConfig;
      RuntimeModeCoordinator::restoreCurrentModeFrame();
      setErrorResponse(response, "mode activation failed");
      return false;
    }

    ConfigManager::saveTetrisClockConfig();
    response["message"] = command.successMessage;
    return true;
  }

  if (command.businessModeTag == ModeTags::MAZE) {
    const MazeModeConfig previousConfig = ConfigManager::mazeConfig;
    char activationError[kModeActivationErrorMessageSize] = "mode activation failed";
    RuntimeModeCoordinator::deactivateRuntimeContent();
    if (shouldClearScreenBeforeBusinessModeEntry(command.businessModeTag)) {
      DisplayManager::clearScreen();
    }
    ConfigManager::mazeConfig = command.mazeConfig;

    if (!RuntimeModeCoordinator::switchToMode(
          MODE_ANIMATION,
          command.businessModeTag,
          true,
          true)) {
      copyModeActivationErrorMessage(
        command.businessModeTag,
        activationError,
        sizeof(activationError)
      );
      ConfigManager::mazeConfig = previousConfig;
      RuntimeModeCoordinator::restoreCurrentModeFrame();
      setErrorResponse(response, activationError);
      return false;
    }

    ConfigManager::saveMazeConfig();
    response["message"] = command.successMessage;
    response["speed"] = ConfigManager::mazeConfig.speed;
    response["mazeSizeMode"] =
      ConfigManager::mazeConfig.mazeSizeMode == MAZE_SIZE_DENSE ? "dense" : "wide";
    response["showClock"] = ConfigManager::mazeConfig.showClock;
    response["panelBgColor"] = ConfigManager::mazeConfig.panelBgColor;
    response["borderColor"] = ConfigManager::mazeConfig.borderColor;
    response["timeColor"] = ConfigManager::mazeConfig.timeColor;
    response["dateColor"] = ConfigManager::mazeConfig.dateColor;
    response["generationPathColor"] = ConfigManager::mazeConfig.generationPathColor;
    response["searchVisitedColor"] = ConfigManager::mazeConfig.searchVisitedColor;
    response["searchFrontierColor"] = ConfigManager::mazeConfig.searchFrontierColor;
    response["solvedPathStartColor"] = ConfigManager::mazeConfig.solvedPathStartColor;
    response["solvedPathEndColor"] = ConfigManager::mazeConfig.solvedPathEndColor;
    return true;
  }

  if (command.businessModeTag == ModeTags::SNAKE) {
    const SnakeModeConfig previousConfig = ConfigManager::snakeConfig;
    char activationError[kModeActivationErrorMessageSize] = "mode activation failed";
    RuntimeModeCoordinator::deactivateRuntimeContent();
    if (shouldClearScreenBeforeBusinessModeEntry(command.businessModeTag)) {
      DisplayManager::clearScreen();
    }
    ConfigManager::snakeConfig = command.snakeConfig;

    if (!RuntimeModeCoordinator::switchToMode(
          MODE_ANIMATION,
          command.businessModeTag,
          true,
          true)) {
      copyModeActivationErrorMessage(
        command.businessModeTag,
        activationError,
        sizeof(activationError)
      );
      ConfigManager::snakeConfig = previousConfig;
      RuntimeModeCoordinator::restoreCurrentModeFrame();
      setErrorResponse(response, activationError);
      return false;
    }

    ConfigManager::saveSnakeConfig();
    response["message"] = command.successMessage;
    response["speed"] = ConfigManager::snakeConfig.speed;
    response["snakeWidth"] = ConfigManager::snakeConfig.snakeWidth;
    response["font"] = clockFontNameFromId(ConfigManager::snakeConfig.font);
    response["showSeconds"] = ConfigManager::snakeConfig.showSeconds;
    response["snakeSkin"] = ConfigManager::snakeConfig.snakeSkin;
    JsonObject snakeColor = response.createNestedObject("snakeColor");
    snakeColor["r"] = ConfigManager::snakeConfig.snakeColorR;
    snakeColor["g"] = ConfigManager::snakeConfig.snakeColorG;
    snakeColor["b"] = ConfigManager::snakeConfig.snakeColorB;
    JsonObject foodColor = response.createNestedObject("foodColor");
    foodColor["r"] = ConfigManager::snakeConfig.foodColorR;
    foodColor["g"] = ConfigManager::snakeConfig.foodColorG;
    foodColor["b"] = ConfigManager::snakeConfig.foodColorB;
    return true;
  }

  return switchToTargetMode(
    fromMode,
    command.targetMode,
    command.businessModeTag,
    command.flag1,
    command.flag2,
    command.successMessage.c_str(),
    response
  );
}

bool executeThemeConfig(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  memcpy(
    ConfigManager::themeConfig.themeId,
    command.themeConfig.themeId,
    sizeof(ConfigManager::themeConfig.themeId)
  );
  ConfigManager::saveThemeConfig();

  DeviceMode fromMode = DisplayManager::currentMode;
  return switchToTargetMode(
    fromMode,
    MODE_THEME,
    ModeTags::THEME,
    true,
    true,
    "theme config updated",
    response
  );
}

bool executeBreathEffect(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  DisplayManager::breathEffectConfig = command.breathEffectConfig;
  DeviceMode fromMode = DisplayManager::currentMode;
  return switchToTargetMode(
    fromMode,
    MODE_ANIMATION,
    ModeTags::BREATH_EFFECT,
    true,
    true,
    "breath effect applied",
    response
  );
}

bool executeRhythmEffect(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  DisplayManager::rhythmEffectConfig = command.rhythmEffectConfig;
  DeviceMode fromMode = DisplayManager::currentMode;
  return switchToTargetMode(
    fromMode,
    MODE_ANIMATION,
    ModeTags::RHYTHM_EFFECT,
    true,
    true,
    "rhythm effect applied",
    response
  );
}

bool executeAmbientEffect(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  DisplayManager::ambientEffectConfig = command.ambientEffectConfig;
  ConfigManager::saveAmbientEffectConfig();

  DeviceMode fromMode = DisplayManager::currentMode;
  if (!switchToTargetMode(
        fromMode,
        MODE_ANIMATION,
        command.businessModeTag,
        true,
        true,
        "ambient effect applied",
        response)) {
    return false;
  }

  response["preset"] = command.stringValue1;
  return true;
}

bool executeTextDisplay(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  const TextDisplayNativeConfig previousConfig = BoardNativeEffect::getTextDisplayConfig();
  BoardNativeEffect::applyTextDisplayConfig(command.textDisplayConfig);

  DeviceMode fromMode = DisplayManager::currentMode;
  if (!switchToTargetMode(
        fromMode,
        MODE_ANIMATION,
        ModeTags::TEXT_DISPLAY,
        true,
        true,
        "text display applied",
        response)) {
    BoardNativeEffect::applyTextDisplayConfig(previousConfig);
    return false;
  }
  return true;
}

bool executePlanetScreensaver(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  const PlanetScreensaverNativeConfig previousConfig = BoardNativeEffect::getPlanetScreensaverConfig();
  BoardNativeEffect::applyPlanetScreensaverConfig(command.planetConfig);

  DeviceMode fromMode = DisplayManager::currentMode;
  if (!switchToTargetMode(
        fromMode,
        MODE_ANIMATION,
        ModeTags::PLANET_SCREENSAVER,
        true,
        true,
        "planet screensaver applied",
        response)) {
    BoardNativeEffect::setPlanetScreensaverConfig(previousConfig);
    return false;
  }
  ConfigManager::savePlanetScreensaverConfig();
  return true;
}

bool executeEyesConfig(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  ConfigManager::eyesConfig = command.eyesConfig;
  ConfigManager::saveEyesConfig();

  if (DisplayManager::currentBusinessModeTag == ModeTags::EYES) {
    DisplayManager::activateEyesEffect(ConfigManager::eyesConfig);
  }

  response["message"] = "eyes config updated";
  return true;
}

bool executeEyesAction(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  if (DisplayManager::currentBusinessModeTag != ModeTags::EYES) {
    setErrorResponse(response, "eyes mode required");
    return false;
  }

  if (!EyesEffect::triggerAction(command.stringValue1.c_str())) {
    setErrorResponse(response, "invalid eyes action");
    return false;
  }

  response["message"] = "eyes action applied";
  return true;
}

bool executeClear(StaticJsonDocument<768>& response) {
  DisplayManager::dma_display->clearScreen();
  if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
    DisplayManager::clearCanvas();
  }
  response["message"] = "cleared";
  return true;
}

bool executeBrightness(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  if (command.intValue1 < 0 || command.intValue1 > 255) {
    setErrorResponse(response, "brightness must be 0-255");
    return false;
  }

  DisplayManager::setBrightness(command.intValue1);
  ConfigManager::saveDeviceParamsConfig();
  response["brightness"] = command.intValue1;
  response["message"] = "brightness updated";
  return true;
}

bool executeShowLoading(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  DisplayManager::dma_display->clearScreen();

  int x = 12;
  int y = 18;
  int bs = 11;
  int gap = 3;
  int step = bs + gap;

  uint16_t orange = DisplayManager::dma_display->color565(249, 115, 22);
  uint16_t red = DisplayManager::dma_display->color565(239, 68, 68);
  uint16_t yellow = DisplayManager::dma_display->color565(251, 191, 36);
  uint16_t blue = DisplayManager::dma_display->color565(59, 130, 246);

  uint16_t grid[3][3] = {
    { orange, orange, red },
    { orange, yellow, yellow },
    { orange, blue, blue }
  };

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      uint8_t r = ((grid[row][col] >> 11) & 0x1F) * 255 / 31 * 0.3f;
      uint8_t g = ((grid[row][col] >> 5) & 0x3F) * 255 / 63 * 0.3f;
      uint8_t b = (grid[row][col] & 0x1F) * 255 / 31 * 0.3f;
      uint16_t dimColor = DisplayManager::dma_display->color565(r, g, b);
      DisplayManager::dma_display->fillRect(x + col * step, y + row * step, bs, bs, dimColor);
    }
  }

  int stepIndex = command.intValue1 % 8;
  if (stepIndex < 0) {
    stepIndex += 8;
  }
  int outer[8][2] = {
    {0, 0}, {0, 1}, {0, 2}, {1, 2},
    {2, 2}, {2, 1}, {2, 0}, {1, 0}
  };

  int row = outer[stepIndex][0];
  int col = outer[stepIndex][1];
  DisplayManager::dma_display->fillRect(x + col * step, y + row * step, bs, bs, grid[row][col]);

  response["message"] = "loading displayed";
  return true;
}

bool executeCanvasPayload(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  if (command.rawJsonPayload.length() == 0) {
    setErrorResponse(response, "missing payload");
    return false;
  }

  size_t totalLen = command.rawJsonPayload.length();
  size_t capacity = totalLen * 8 + 2048;
  size_t heapLimit = (size_t)(ESP.getFreeHeap() * 0.7);
  if (capacity > heapLimit) {
    capacity = heapLimit;
  }
  if (capacity < 2048) {
    capacity = 2048;
  }

  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, command.rawJsonPayload.c_str(), totalLen);
  if (error) {
    setErrorResponse(response, "invalid canvas payload");
    return false;
  }

  if (command.type == RuntimeCommandBus::RuntimeCommandType::IMAGE) {
    JsonArray pixels = doc["pixels"].as<JsonArray>();
    int width = doc["width"] | DisplayManager::PANEL_RES_X;
    int height = doc["height"] | DisplayManager::PANEL_RES_Y;

    DisplayManager::dma_display->clearScreen();
    int idx = 0;
    for (int y = 0; y < height && y < DisplayManager::PANEL_RES_Y; y++) {
      for (int x = 0; x < width && x < DisplayManager::PANEL_RES_X; x++) {
        if (idx + 2 < pixels.size()) {
          uint8_t r = pixels[idx++];
          uint8_t g = pixels[idx++];
          uint8_t b = pixels[idx++];
          DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
        }
      }
      if (y % 8 == 0) {
        yield();
      }
    }
    response["message"] = "image displayed";
    return true;
  }

  if (command.type == RuntimeCommandBus::RuntimeCommandType::IMAGE_SPARSE) {
    JsonArray pixels = doc["pixels"].as<JsonArray>();
    for (JsonVariant pixel : pixels) {
      int x = pixel["x"];
      int y = pixel["y"];
      uint8_t r = pixel["r"];
      uint8_t g = pixel["g"];
      uint8_t b = pixel["b"];
      if (x >= 0 && x < DisplayManager::PANEL_RES_X &&
          y >= 0 && y < DisplayManager::PANEL_RES_Y) {
        DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }
    response["message"] = "sparse pixels drawn";
    response["count"] = pixels.size();
    return true;
  }

  if (command.type == RuntimeCommandBus::RuntimeCommandType::IMAGE_CHUNK) {
    JsonArray pixels = doc["pixels"].as<JsonArray>();
    int width = doc["width"];
    int offsetX = doc["offsetX"] | 0;
    int offsetY = doc["offsetY"] | 0;
    int height = doc["height"];
    int chunk = doc["chunk"] | 0;
    int total = doc["total"] | 1;

    if (chunk == 0) {
      DisplayManager::dma_display->clearScreen();
    }

    int idx = 0;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        if (idx + 2 < pixels.size()) {
          uint8_t r = pixels[idx++];
          uint8_t g = pixels[idx++];
          uint8_t b = pixels[idx++];
          int drawX = x + offsetX;
          int drawY = y + offsetY;
          if (drawX >= 0 && drawX < DisplayManager::PANEL_RES_X &&
              drawY >= 0 && drawY < DisplayManager::PANEL_RES_Y) {
            DisplayManager::dma_display->drawPixelRGB888(drawX, drawY, r, g, b);
          }
        }
      }
      if (y % 8 == 0) {
        yield();
      }
    }
    response["message"] = "chunk displayed";
    response["chunk"] = chunk;
    response["total"] = total;
    return true;
  }

  setErrorResponse(response, "unsupported canvas payload");
  return false;
}

bool executeCommand(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  response["status"] = "ok";

  switch (command.type) {
    case RuntimeCommandBus::RuntimeCommandType::MODE_SWITCH:
      return executeModeSwitch(command, response);
    case RuntimeCommandBus::RuntimeCommandType::THEME_CONFIG:
      return executeThemeConfig(command, response);
    case RuntimeCommandBus::RuntimeCommandType::BREATH_EFFECT:
      return executeBreathEffect(command, response);
    case RuntimeCommandBus::RuntimeCommandType::RHYTHM_EFFECT:
      return executeRhythmEffect(command, response);
    case RuntimeCommandBus::RuntimeCommandType::AMBIENT_EFFECT:
      return executeAmbientEffect(command, response);
    case RuntimeCommandBus::RuntimeCommandType::TEXT_DISPLAY:
      return executeTextDisplay(command, response);
    case RuntimeCommandBus::RuntimeCommandType::PLANET_SCREENSAVER:
      return executePlanetScreensaver(command, response);
    case RuntimeCommandBus::RuntimeCommandType::EYES_CONFIG:
      return executeEyesConfig(command, response);
    case RuntimeCommandBus::RuntimeCommandType::EYES_ACTION:
      return executeEyesAction(command, response);
    case RuntimeCommandBus::RuntimeCommandType::CLEAR:
      return executeClear(response);
    case RuntimeCommandBus::RuntimeCommandType::BRIGHTNESS:
      return executeBrightness(command, response);
    case RuntimeCommandBus::RuntimeCommandType::START_LOADING:
      DisplayManager::startLoadingAnimation();
      response["message"] = "loading started";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::STOP_LOADING:
      DisplayManager::stopLoadingAnimation();
      if (DisplayManager::currentMode == MODE_TRANSFERRING) {
        DisplayManager::clearScreen();
      } else if (RuntimeModeCoordinator::isTransientRuntimeMode(DisplayManager::currentMode)) {
        RuntimeModeCoordinator::restoreAfterTransientDisconnect(true, false);
      } else {
        RuntimeModeCoordinator::restoreCurrentModeFrame(false);
      }
      response["message"] = "loading stopped";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::SHOW_LOADING:
      return executeShowLoading(command, response);
    case RuntimeCommandBus::RuntimeCommandType::LOAD_CANVAS:
      ConfigManager::loadCanvasPixels();
      if (DisplayManager::currentMode == MODE_CANVAS) {
        DisplayManager::renderCanvas();
      }
      response["message"] = "canvas loaded";
      response["initialized"] = DisplayManager::canvasInitialized;
      return true;
    case RuntimeCommandBus::RuntimeCommandType::CLEAR_CANVAS:
      DisplayManager::clearCanvas();
      ConfigManager::clearCanvasPixels();
      if (DisplayManager::currentMode == MODE_CANVAS) {
        DisplayManager::dma_display->clearScreen();
      }
      response["message"] = "canvas cleared";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::HIGHLIGHT_COLOR:
      if (DisplayManager::currentMode != MODE_CANVAS || !DisplayManager::canvasInitialized) {
        setErrorResponse(response, "not in canvas mode or canvas not initialized");
        return false;
      }
      if (command.flag1) {
        DisplayManager::highlightCanvasColor(command.intValue1, command.intValue2, command.intValue3);
        response["message"] = "color highlighted";
      } else {
        DisplayManager::renderCanvas();
        response["message"] = "highlight cleared";
      }
      return true;
    case RuntimeCommandBus::RuntimeCommandType::HIGHLIGHT_ROW:
      if (DisplayManager::currentMode != MODE_CANVAS || !DisplayManager::canvasInitialized) {
        setErrorResponse(response, "not in canvas mode or canvas not initialized");
        return false;
      }
      DisplayManager::highlightCanvasRow(command.intValue1);
      response["message"] = command.intValue1 >= 0 ? "row highlighted" : "highlight cleared";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::TEXT:
      DisplayManager::dma_display->clearScreen();
      DisplayManager::dma_display->setCursor(command.intValue1, command.intValue2);
      DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
      DisplayManager::dma_display->print(command.stringValue1);
      response["message"] = "text displayed";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::PIXEL:
      if (command.intValue1 < 0 || command.intValue1 >= DisplayManager::PANEL_RES_X ||
          command.intValue2 < 0 || command.intValue2 >= DisplayManager::PANEL_RES_Y) {
        setErrorResponse(response, "pixel out of range");
        return false;
      }
      DisplayManager::dma_display->drawPixelRGB888(
        command.intValue1,
        command.intValue2,
        command.intValue3,
        command.intValue4,
        command.intValue5
      );
      response["message"] = "pixel set";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::IMAGE:
    case RuntimeCommandBus::RuntimeCommandType::IMAGE_SPARSE:
    case RuntimeCommandBus::RuntimeCommandType::IMAGE_CHUNK:
      return executeCanvasPayload(command, response);
    case RuntimeCommandBus::RuntimeCommandType::APPLY_DEVICE_PARAMS:
      DisplayManager::applyDeviceParams(false);
      DisplayManager::refreshScheduledBrightness();
      RuntimeModeCoordinator::restoreCurrentModeFrame();
      response["message"] = "device params applied";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::RESTORE_CURRENT_MODE_FRAME:
      RuntimeModeCoordinator::restoreCurrentModeFrame();
      response["message"] = "runtime restored";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::RESTORE_AFTER_TRANSIENT_DISCONNECT:
      RuntimeModeCoordinator::restoreAfterTransientDisconnect(command.flag1);
      response["message"] = "transient runtime restored";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::ABORT_TRANSIENT_TRANSFER_AND_RESTORE:
      {
        WebSocketHandler::TransientTransferAbortState abortState =
          WebSocketHandler::abortTransientTransfer(command.restoreReason.c_str());
        if (abortState.wasTransientMode) {
          RuntimeModeCoordinator::restoreAfterTransientDisconnect(true, false);
        } else if (abortState.hadBinaryPixels) {
          RuntimeModeCoordinator::restoreCurrentModeFrame(false);
        }
      }
      response["message"] = "transient transfer aborted";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::HTTP_UPLOAD_IMAGE:
      if (WebServer::imageBuffer == nullptr || WebServer::imageSize == 0) {
        setErrorResponse(response, "missing upload image");
        return false;
      }
      DisplayManager::displayImage(
        WebServer::imageBuffer,
        WebServer::imageSize,
        command.intValue1,
        command.intValue2
      );
      free(WebServer::imageBuffer);
      WebServer::imageBuffer = nullptr;
      WebServer::imageSize = 0;
      response["message"] = "image displayed";
      return true;
    case RuntimeCommandBus::RuntimeCommandType::WEBSOCKET_TRANSACTION_COMMIT:
      {
        if (!gWebSocketTransactionSession.active ||
            gWebSocketTransactionSession.txId != command.stringValue1) {
          fillTransactionFinalErrorResponse(command.stringValue1, "transaction not active", response);
          return false;
        }

        const char* reason = nullptr;
        if (!finalizeCommittedBinary(reason)) {
          fillTransactionFinalErrorResponse(gWebSocketTransactionSession.txId, reason, response);
          rollbackWebSocketTransactionSession();
          return false;
        }

        DisplayManager::receivingPixels = false;

        if (!applyPreparedWebSocketTransaction(reason) ||
            !persistCommittedBinary(reason)) {
          fillTransactionFinalErrorResponse(gWebSocketTransactionSession.txId, reason, response);
          rollbackWebSocketTransactionSession();
          return false;
        }

        fillTransactionFinalOkResponse(gWebSocketTransactionSession.txId, response);
        finalizeSuccessfulWebSocketTransaction();
      }
      return true;
    case RuntimeCommandBus::RuntimeCommandType::WEBSOCKET_TRANSACTION_ABORT:
      {
        if (!gWebSocketTransactionSession.active ||
            gWebSocketTransactionSession.txId != command.stringValue1) {
          fillTransactionFinalErrorResponse(command.stringValue1, "transaction not active", response);
          return false;
        }

        const String txId = gWebSocketTransactionSession.txId;
        rollbackWebSocketTransactionSession();
        fillTransactionFinalOkResponse(txId, response);
      }
      return true;
    default:
      setErrorResponse(response, "unknown runtime command");
      return false;
  }
}

RuntimeCommandBus::RuntimeCommand* dequeueCommand() {
  RuntimeCommandBus::RuntimeCommand* command = gCommandQueue[gQueueHead];
  gCommandQueue[gQueueHead] = nullptr;
  gQueueHead = (gQueueHead + 1) % kRuntimeCommandQueueCapacity;
  if (gQueueCount > 0) {
    gQueueCount -= 1;
  }
  return command;
}
}

namespace RuntimeCommandBus {
RuntimeCommand* createCommand(RuntimeCommandSource source, uint32_t clientId) {
  RuntimeCommand* command = new (std::nothrow) RuntimeCommand();
  if (command == nullptr) {
    return nullptr;
  }

  command->source = source;
  command->clientId = clientId;
  return command;
}

void destroyCommand(RuntimeCommand* command) {
  if (command == nullptr) {
    return;
  }

  delete command;
}

bool enqueue(RuntimeCommand* command) {
  if (command == nullptr) {
    return false;
  }

  bool queued = false;
  portENTER_CRITICAL(&gRuntimeCommandQueueMux);
  if (gQueueCount < kRuntimeCommandQueueCapacity) {
    gCommandQueue[gQueueTail] = command;
    gQueueTail = (gQueueTail + 1) % kRuntimeCommandQueueCapacity;
    gQueueCount += 1;
    queued = true;
  }
  portEXIT_CRITICAL(&gRuntimeCommandQueueMux);

  if (!queued) {
    return false;
  }

  return true;
}

bool enqueueRestoreCurrentModeFrame() {
  RuntimeCommand* command = createCommand(RuntimeCommandSource::SYSTEM, 0);
  if (command == nullptr) {
    return false;
  }

  command->type = RuntimeCommandType::RESTORE_CURRENT_MODE_FRAME;
  if (!enqueue(command)) {
    destroyCommand(command);
    return false;
  }
  return true;
}

bool enqueueRestoreAfterTransientDisconnect(bool persistState) {
  RuntimeCommand* command = createCommand(RuntimeCommandSource::SYSTEM, 0);
  if (command == nullptr) {
    return false;
  }

  command->type = RuntimeCommandType::RESTORE_AFTER_TRANSIENT_DISCONNECT;
  command->flag1 = persistState;
  if (!enqueue(command)) {
    destroyCommand(command);
    return false;
  }
  return true;
}

bool enqueueAbortTransientTransferAndRestore(const char* reason) {
  RuntimeCommand* command = createCommand(RuntimeCommandSource::SYSTEM, 0);
  if (command == nullptr) {
    return false;
  }

  command->type = RuntimeCommandType::ABORT_TRANSIENT_TRANSFER_AND_RESTORE;
  if (reason != nullptr) {
    command->restoreReason = reason;
  }
  if (!enqueue(command)) {
    destroyCommand(command);
    return false;
  }
  return true;
}

void queueOrSendClientResponse(uint32_t clientId, StaticJsonDocument<768>& response) {
  sendClientResponse(clientId, response);
}

bool beginWebSocketTransaction(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (client == nullptr) {
    fillTransactionFinalErrorResponse("", "client unavailable", response);
    return true;
  }

  if (!doc.containsKey("txId") ||
      !doc.containsKey("mode") ||
      !doc.containsKey("params") ||
      !doc.containsKey("hasBinary") ||
      !doc.containsKey("binarySize")) {
    fillTransactionFinalErrorResponse("", "tx_begin fields missing", response);
    return true;
  }

  const char* txId = doc["txId"];
  const char* mode = doc["mode"];
  JsonObject params = doc["params"].as<JsonObject>();
  if (txId == nullptr || txId[0] == '\0' || mode == nullptr || mode[0] == '\0' || params.isNull()) {
    fillTransactionFinalErrorResponse("", "tx_begin fields invalid", response);
    return true;
  }

  if (gWebSocketTransactionSession.active) {
    fillTransactionFinalErrorResponse(txId, "transaction already active", response);
    return true;
  }

  clearWebSocketTransactionSession();
  gWebSocketTransactionSession.active = true;
  gWebSocketTransactionSession.clientId = client->id();
  gWebSocketTransactionSession.txId = txId;
  gWebSocketTransactionSession.mode = mode;
  gWebSocketTransactionSession.hasBinary = doc["hasBinary"].as<bool>();
  gWebSocketTransactionSession.binarySize = static_cast<size_t>(doc["binarySize"].as<unsigned long>());
  RuntimeModeCoordinator::captureModeState(gWebSocketTransactionSession.restoreSnapshot);

  const char* reason = nullptr;
  if (!buildPreparedWebSocketTransaction(gWebSocketTransactionSession.mode, params, reason)) {
    fillTransactionFinalErrorResponse(gWebSocketTransactionSession.txId, reason, response);
    clearWebSocketTransactionSession();
    return true;
  }

  gWebSocketTransactionSession.binaryType =
    resolveBinaryTypeForMode(
      gWebSocketTransactionSession.mode,
      gWebSocketTransactionSession.hasBinary,
      gWebSocketTransactionSession.binarySize
    );
  if (gWebSocketTransactionSession.hasBinary &&
      gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::NONE) {
    fillTransactionFinalErrorResponse(gWebSocketTransactionSession.txId, "binary mode unsupported", response);
    clearWebSocketTransactionSession();
    return true;
  }

  if (gWebSocketTransactionSession.hasBinary) {
    if (gWebSocketTransactionSession.binarySize == 0 &&
        gWebSocketTransactionSession.binaryType != WebSocketTransactionBinaryType::STATIC_PIXELS) {
      fillTransactionFinalErrorResponse(gWebSocketTransactionSession.txId, "binarySize invalid", response);
      clearWebSocketTransactionSession();
      return true;
    }
  }

  bool shouldEnterLoadingState =
    (gWebSocketTransactionSession.hasBinary && !isCanvasStaticPixelTransaction()) ||
    (!gWebSocketTransactionSession.hasBinary &&
     (gWebSocketTransactionSession.mode == ModeTags::AMBIENT_EFFECT ||
      gWebSocketTransactionSession.mode == ModeTags::LED_MATRIX_SHOWCASE));

  if (shouldEnterLoadingState) {
    enterWebSocketTransactionLoadingState();
  }

  if (gWebSocketTransactionSession.hasBinary) {
    if (!beginBinaryPayloadForTransaction(reason)) {
      fillTransactionFinalErrorResponse(gWebSocketTransactionSession.txId, reason, response);
      rollbackWebSocketTransactionSession();
      return true;
    }
  }

  fillTransactionAcceptedResponse(gWebSocketTransactionSession.txId, response);
  queueOrSendClientResponse(client->id(), response);
  responseSent = true;
  return true;
}

bool commitWebSocketTransaction(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  if (client == nullptr || !doc.containsKey("txId")) {
    fillTransactionFinalErrorResponse("", "tx_commit invalid", response);
    return true;
  }

  const char* txId = doc["txId"];
  if (txId == nullptr || txId[0] == '\0') {
    fillTransactionFinalErrorResponse("", "tx_commit invalid", response);
    return true;
  }

  if (!gWebSocketTransactionSession.active ||
      gWebSocketTransactionSession.clientId != client->id() ||
      gWebSocketTransactionSession.txId != txId) {
    fillTransactionFinalErrorResponse(txId, "transaction not active", response);
    return true;
  }

  RuntimeCommand* command = createCommand(RuntimeCommandSource::WEBSOCKET, client->id());
  if (command == nullptr) {
    fillTransactionFinalErrorResponse(txId, "out of memory", response);
    rollbackWebSocketTransactionSession();
    return true;
  }
  command->type = RuntimeCommandType::WEBSOCKET_TRANSACTION_COMMIT;
  command->stringValue1 = txId;
  if (!enqueue(command)) {
    destroyCommand(command);
    fillTransactionFinalErrorResponse(txId, "device busy", response);
    rollbackWebSocketTransactionSession();
    return true;
  }
  return true;
}

bool abortWebSocketTransaction(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  if (client == nullptr || !doc.containsKey("txId")) {
    fillTransactionFinalErrorResponse("", "tx_abort invalid", response);
    return true;
  }

  const char* txId = doc["txId"];
  if (txId == nullptr || txId[0] == '\0') {
    fillTransactionFinalErrorResponse("", "tx_abort invalid", response);
    return true;
  }

  if (!gWebSocketTransactionSession.active ||
      gWebSocketTransactionSession.clientId != client->id() ||
      gWebSocketTransactionSession.txId != txId) {
    fillTransactionFinalErrorResponse(txId, "transaction not active", response);
    return true;
  }

  RuntimeCommand* command = createCommand(RuntimeCommandSource::WEBSOCKET, client->id());
  if (command == nullptr) {
    fillTransactionFinalErrorResponse(txId, "out of memory", response);
    rollbackWebSocketTransactionSession();
    return true;
  }
  command->type = RuntimeCommandType::WEBSOCKET_TRANSACTION_ABORT;
  command->stringValue1 = txId;
  if (!enqueue(command)) {
    destroyCommand(command);
    fillTransactionFinalErrorResponse(txId, "device busy", response);
    rollbackWebSocketTransactionSession();
    return true;
  }
  return true;
}

bool appendWebSocketTransactionBinary(
  AsyncWebSocketClient* client,
  const uint8_t* data,
  size_t len,
  StaticJsonDocument<768>& response,
  bool& responseReady
) {
  responseReady = false;
  if (client == nullptr ||
      !gWebSocketTransactionSession.active ||
      !gWebSocketTransactionSession.hasBinary ||
      gWebSocketTransactionSession.clientId != client->id()) {
    return false;
  }

  const char* reason = nullptr;
  if (gWebSocketTransactionSession.receivedBinarySize + len > gWebSocketTransactionSession.binarySize) {
    reason = "binary exceeds expected size";
  } else if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::STATIC_PIXELS) {
    if (len == 0 || len % 5 != 0) {
      reason = "invalid static binary length";
    } else if (isCanvasStaticPixelTransaction()) {
      if (gWebSocketTransactionSession.animationBinaryBuffer == nullptr) {
        reason = "canvas buffer unavailable";
      } else {
        memcpy(
          gWebSocketTransactionSession.animationBinaryBuffer + gWebSocketTransactionSession.receivedBinarySize,
          data,
          len
        );
        gWebSocketTransactionSession.receivedBinarySize += len;
      }
    } else {
      PixelData*& imagePixels =
        currentImagePixelsForMode(gWebSocketTransactionSession.preparedCommand.targetMode);
      int& imagePixelCount =
        currentImagePixelCountForMode(gWebSocketTransactionSession.preparedCommand.targetMode);
      const size_t pixelCount = len / 5;
      if (imagePixelCount + static_cast<int>(pixelCount) > DisplayManager::MAX_PIXELS) {
        reason = "static pixels exceed limit";
      } else {
        PixelData* resized = static_cast<PixelData*>(
          realloc(imagePixels, sizeof(PixelData) * (imagePixelCount + pixelCount))
        );
        if (resized == nullptr) {
          reason = "pixel buffer alloc failed";
        } else {
          imagePixels = resized;
          for (size_t i = 0; i + 4 < len; i += 5) {
            uint8_t x = data[i];
            uint8_t y = data[i + 1];
            uint8_t r = data[i + 2];
            uint8_t g = data[i + 3];
            uint8_t b = data[i + 4];
            if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y) {
              imagePixels[imagePixelCount].x = x;
              imagePixels[imagePixelCount].y = y;
              imagePixels[imagePixelCount].r = r;
              imagePixels[imagePixelCount].g = g;
              imagePixels[imagePixelCount].b = b;
              imagePixelCount += 1;
            }
          }
          gWebSocketTransactionSession.receivedBinarySize += len;
        }
      }
    }
  } else if (gWebSocketTransactionSession.binaryType == WebSocketTransactionBinaryType::COMPACT_ANIMATION) {
    if (gWebSocketTransactionSession.animationBinaryBuffer == nullptr) {
      reason = "animation buffer unavailable";
    } else {
      memcpy(
        gWebSocketTransactionSession.animationBinaryBuffer + gWebSocketTransactionSession.receivedBinarySize,
        data,
        len
      );
      gWebSocketTransactionSession.receivedBinarySize += len;
    }
  } else {
    reason = "binary mode unsupported";
  }

  if (reason != nullptr) {
    fillTransactionFinalErrorResponse(gWebSocketTransactionSession.txId, reason, response);
    rollbackWebSocketTransactionSession();
    responseReady = true;
  }
  return true;
}

bool hasActiveWebSocketTransaction(uint32_t clientId) {
  return gWebSocketTransactionSession.active &&
         gWebSocketTransactionSession.hasBinary &&
         gWebSocketTransactionSession.clientId == clientId;
}

void handleWebSocketTransactionDisconnect(uint32_t clientId) {
  if (!gWebSocketTransactionSession.active ||
      gWebSocketTransactionSession.clientId != clientId) {
    return;
  }

  RuntimeCommand* command = createCommand(RuntimeCommandSource::SYSTEM, 0);
  if (command == nullptr) {
    rollbackWebSocketTransactionSession();
    return;
  }
  command->type = RuntimeCommandType::WEBSOCKET_TRANSACTION_ABORT;
  command->stringValue1 = gWebSocketTransactionSession.txId;
  command->restoreReason = "client disconnected";
  if (!enqueue(command)) {
    destroyCommand(command);
    rollbackWebSocketTransactionSession();
  }
}

void tick() {
  flushPendingClientResponses();
  for (size_t processedCount = 0; processedCount < kRuntimeCommandsPerTick; processedCount += 1) {
    RuntimeCommand* command = nullptr;
    portENTER_CRITICAL(&gRuntimeCommandQueueMux);
    if (gQueueCount > 0) {
      command = dequeueCommand();
    }
    portEXIT_CRITICAL(&gRuntimeCommandQueueMux);

    if (command == nullptr) {
      return;
    }

    if (command->type == RuntimeCommandType::NONE) {
      destroyCommand(command);
      continue;
    }

    if (command->source == RuntimeCommandSource::WEBSOCKET &&
        !clientStillConnected(command->clientId)) {
      destroyCommand(command);
      continue;
    }

    StaticJsonDocument<768> response;
    bool success = false;
    DisplayManager::lockRuntimeAccess();
    success = executeCommand(*command, response);
    DisplayManager::unlockRuntimeAccess();

    if (command->source == RuntimeCommandSource::WEBSOCKET) {
      sendClientResponse(command->clientId, response);
      if (command->type == RuntimeCommandType::WEBSOCKET_TRANSACTION_COMMIT ||
          command->type == RuntimeCommandType::WEBSOCKET_TRANSACTION_ABORT) {
        gPostTransactionYieldHint = 2;
      }
    }

    destroyCommand(command);
    flushPendingClientResponses();
  }
}

bool consumePostTransactionYieldHint() {
  if (gPostTransactionYieldHint == 0) {
    return false;
  }
  gPostTransactionYieldHint -= 1;
  return true;
}

const char* commandTypeLabel(RuntimeCommandType type) {
  switch (type) {
    case RuntimeCommandType::MODE_SWITCH:
      return "MODE_SWITCH";
    case RuntimeCommandType::THEME_CONFIG:
      return "THEME_CONFIG";
    case RuntimeCommandType::BREATH_EFFECT:
      return "BREATH_EFFECT";
    case RuntimeCommandType::RHYTHM_EFFECT:
      return "RHYTHM_EFFECT";
    case RuntimeCommandType::AMBIENT_EFFECT:
      return "AMBIENT_EFFECT";
    case RuntimeCommandType::TEXT_DISPLAY:
      return "TEXT_DISPLAY";
    case RuntimeCommandType::PLANET_SCREENSAVER:
      return "PLANET_SCREENSAVER";
    case RuntimeCommandType::EYES_CONFIG:
      return "EYES_CONFIG";
    case RuntimeCommandType::EYES_ACTION:
      return "EYES_ACTION";
    case RuntimeCommandType::CLEAR:
      return "CLEAR";
    case RuntimeCommandType::BRIGHTNESS:
      return "BRIGHTNESS";
    case RuntimeCommandType::START_LOADING:
      return "START_LOADING";
    case RuntimeCommandType::STOP_LOADING:
      return "STOP_LOADING";
    case RuntimeCommandType::SHOW_LOADING:
      return "SHOW_LOADING";
    case RuntimeCommandType::LOAD_CANVAS:
      return "LOAD_CANVAS";
    case RuntimeCommandType::CLEAR_CANVAS:
      return "CLEAR_CANVAS";
    case RuntimeCommandType::HIGHLIGHT_COLOR:
      return "HIGHLIGHT_COLOR";
    case RuntimeCommandType::HIGHLIGHT_ROW:
      return "HIGHLIGHT_ROW";
    case RuntimeCommandType::TEXT:
      return "TEXT";
    case RuntimeCommandType::PIXEL:
      return "PIXEL";
    case RuntimeCommandType::IMAGE:
      return "IMAGE";
    case RuntimeCommandType::IMAGE_SPARSE:
      return "IMAGE_SPARSE";
    case RuntimeCommandType::IMAGE_CHUNK:
      return "IMAGE_CHUNK";
    case RuntimeCommandType::APPLY_DEVICE_PARAMS:
      return "APPLY_DEVICE_PARAMS";
    case RuntimeCommandType::RESTORE_CURRENT_MODE_FRAME:
      return "RESTORE_CURRENT_MODE_FRAME";
    case RuntimeCommandType::RESTORE_AFTER_TRANSIENT_DISCONNECT:
      return "RESTORE_AFTER_TRANSIENT_DISCONNECT";
    case RuntimeCommandType::ABORT_TRANSIENT_TRANSFER_AND_RESTORE:
      return "ABORT_TRANSIENT_TRANSFER_AND_RESTORE";
    case RuntimeCommandType::HTTP_UPLOAD_IMAGE:
      return "HTTP_UPLOAD_IMAGE";
    case RuntimeCommandType::WEBSOCKET_TRANSACTION_COMMIT:
      return "WEBSOCKET_TRANSACTION_COMMIT";
    case RuntimeCommandType::WEBSOCKET_TRANSACTION_ABORT:
      return "WEBSOCKET_TRANSACTION_ABORT";
    default:
      return "NONE";
  }
}
}
