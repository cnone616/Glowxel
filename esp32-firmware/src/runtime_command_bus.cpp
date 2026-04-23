#include "runtime_command_bus.h"

#include <new>

#include "animation_manager.h"
#include "device_mode_tag_codec.h"
#include "eyes_effect.h"
#include "game_screensaver_effect.h"
#include "mode_tags.h"
#include "runtime_mode_coordinator.h"
#include "tetris_effect.h"
#include "web_server.h"
#include "websocket_handler.h"

namespace {
constexpr size_t kRuntimeCommandQueueCapacity = 8;
RuntimeCommandBus::RuntimeCommand* gCommandQueue[kRuntimeCommandQueueCapacity] = {};
size_t gQueueHead = 0;
size_t gQueueTail = 0;
size_t gQueueCount = 0;
portMUX_TYPE gRuntimeCommandQueueMux = portMUX_INITIALIZER_UNLOCKED;

struct PendingClientResponse {
  uint32_t clientId;
  char* payload;
  size_t len;
};

constexpr size_t kPendingClientResponseQueueCapacity = 8;
PendingClientResponse* gPendingClientResponseQueue[kPendingClientResponseQueueCapacity] = {};
size_t gPendingClientResponseQueueHead = 0;
size_t gPendingClientResponseQueueTail = 0;
size_t gPendingClientResponseQueueCount = 0;
portMUX_TYPE gPendingClientResponseQueueMux = portMUX_INITIALIZER_UNLOCKED;

void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
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
  size_t pendingCount = 0;
  portENTER_CRITICAL(&gPendingClientResponseQueueMux);
  if (gPendingClientResponseQueueCount < kPendingClientResponseQueueCapacity) {
    gPendingClientResponseQueue[gPendingClientResponseQueueTail] = response;
    gPendingClientResponseQueueTail =
      (gPendingClientResponseQueueTail + 1) % kPendingClientResponseQueueCapacity;
    gPendingClientResponseQueueCount += 1;
    pendingCount = gPendingClientResponseQueueCount;
    queued = true;
  }
  portEXIT_CRITICAL(&gPendingClientResponseQueueMux);

  if (queued) {
    Serial.printf("[RuntimeBus] queued delayed response client=%u len=%u pending=%u\n",
                  response->clientId,
                  static_cast<unsigned>(response->len),
                  static_cast<unsigned>(pendingCount));
  }
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
    Serial.printf("[RuntimeBus] response deferred client=%u queue=%u len=%u\n",
                  clientId,
                  static_cast<unsigned>(client->queueLen()),
                  static_cast<unsigned>(len));
    return false;
  }

  bool sent = WebSocketHandler::ws.text(clientId, payload, len);
  Serial.printf("[RuntimeBus] response send %s client=%u len=%u queue=%u\n",
                sent ? "ok" : "failed",
                clientId,
                static_cast<unsigned>(len),
                static_cast<unsigned>(client->queueLen()));
  return sent;
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
      Serial.printf("[RuntimeBus] drop delayed response for disconnected client=%u\n",
                    response->clientId);
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
    Serial.printf("[RuntimeBus] drop response for disconnected client=%u\n", clientId);
    return;
  }

  String responseStr;
  serializeJson(response, responseStr);

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

bool switchToTargetMode(
  DeviceMode fromMode,
  DeviceMode targetMode,
  const String& businessModeTag,
  bool updateLastBusinessTarget,
  bool persistState,
  const char* successMessage,
  StaticJsonDocument<768>& response
) {
  RuntimeModeCoordinator::ModeStateSnapshot previousState;
  RuntimeModeCoordinator::captureModeState(previousState);
  bool restoreToLastBusinessOnFailure =
    RuntimeModeCoordinator::isTransientRuntimeMode(previousState.currentMode);
  RuntimeModeCoordinator::deactivateRuntimeContent();

  bool shouldFinalizePendingUpload =
    businessModeTag != ModeTags::TRANSFERRING &&
    AnimationManager::hasPendingUploadReady();
  bool shouldPersistAfterFinalize = persistState && shouldFinalizePendingUpload;

  if (!RuntimeModeCoordinator::switchToMode(
        targetMode,
        businessModeTag,
        updateLastBusinessTarget,
        shouldPersistAfterFinalize ? false : persistState)) {
    setErrorResponse(response, "mode activation failed");
    if (shouldFinalizePendingUpload) {
      AnimationManager::abortUploadSession();
      DisplayManager::stopLoadingAnimation();
      if (restoreToLastBusinessOnFailure) {
        RuntimeModeCoordinator::restoreLastBusinessModeFrame(true);
      } else {
        RuntimeModeCoordinator::restoreModeState(previousState, true);
      }
    }
    return false;
  }

  if (shouldFinalizePendingUpload) {
    if (!AnimationManager::finalizeUploadSession()) {
      AnimationManager::abortUploadSession();
      DisplayManager::stopLoadingAnimation();
      if (restoreToLastBusinessOnFailure) {
        RuntimeModeCoordinator::restoreLastBusinessModeFrame(true);
      } else {
        RuntimeModeCoordinator::restoreModeState(previousState, true);
      }
      setErrorResponse(response, "animation commit failed");
      return false;
    }

    RuntimeModeCoordinator::restoreCurrentModeFrame(false);
    if (shouldPersistAfterFinalize) {
      ConfigManager::saveClockConfig();
    }
  }

  Serial.printf("模式切换: %s -> %s\n",
                DeviceModeTagCodec::toTagOrUnknown(fromMode),
                businessModeTag.c_str());
  response["message"] = successMessage;
  return true;
}

bool executeModeSwitch(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  DeviceMode fromMode = DisplayManager::currentMode;
  String fromBusinessModeTag = DisplayManager::currentBusinessModeTag;

  if (command.businessModeTag == ModeTags::TETRIS ||
      command.businessModeTag == ModeTags::TETRIS_CLOCK) {
    RuntimeModeCoordinator::deactivateRuntimeContent();
    TetrisEffect::init(
      command.flag1,
      command.intValue1,
      command.intValue2,
      command.flag2,
      command.byteValue1
    );

    if (!RuntimeModeCoordinator::switchToMode(
          MODE_ANIMATION,
          command.businessModeTag,
          true,
          true)) {
      RuntimeModeCoordinator::restoreCurrentModeFrame();
      setErrorResponse(response, "mode activation failed");
      return false;
    }

    Serial.printf("模式切换: %s -> %s\n",
                  fromBusinessModeTag.c_str(),
                  command.businessModeTag.c_str());
    response["message"] = command.successMessage;
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
        ModeTags::LED_MATRIX_SHOWCASE,
        true,
        true,
        "ambient effect applied",
        response)) {
    return false;
  }

  response["preset"] = command.stringValue1;
  return true;
}

bool executeGameScreensaver(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  const bool previousActive =
    DisplayManager::currentBusinessModeTag == ModeTags::GAME_SCREENSAVER &&
    GameScreensaverEffect::isActive();
  const GameScreensaverConfig previousConfig = GameScreensaverEffect::getConfig();

  RuntimeModeCoordinator::deactivateRuntimeContent();
  GameScreensaverEffect::applyConfig(command.gameScreensaverConfig);
  if (!GameScreensaverEffect::isActive()) {
    if (previousActive) {
      GameScreensaverEffect::applyConfig(previousConfig);
    }
    RuntimeModeCoordinator::restoreCurrentModeFrame();
    setErrorResponse(response, "game screensaver activation failed");
    return false;
  }

  ConfigManager::gameScreensaverConfig = command.gameScreensaverConfig;
  ConfigManager::saveGameScreensaverConfig();

  DeviceMode fromMode = DisplayManager::currentMode;
  if (!switchToTargetMode(
        fromMode,
        MODE_ANIMATION,
        ModeTags::GAME_SCREENSAVER,
        true,
        true,
        "game screensaver applied",
        response)) {
    if (previousActive) {
      GameScreensaverEffect::applyConfig(previousConfig);
    }
    return false;
  }

  response["speed"] = command.gameScreensaverConfig.speed;
  response["game"] = command.stringValue1;
  if (command.gameScreensaverConfig.game == GAME_SCREENSAVER_SNAKE) {
    response["snakeWidth"] = command.gameScreensaverConfig.snakeWidth;
  }
  if (command.gameScreensaverConfig.game == GAME_SCREENSAVER_MAZE) {
    response["mazeSizeMode"] =
      command.gameScreensaverConfig.mazeSizeMode == GAME_SCREENSAVER_MAZE_DENSE ? "dense" : "wide";
  }
  if (command.gameScreensaverConfig.game == GAME_SCREENSAVER_TETRIS_GAME) {
    response["cellSize"] = command.gameScreensaverConfig.cellSize;
    response["showClock"] = command.gameScreensaverConfig.showClock;
  }
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

bool executeWeatherBoard(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  const WeatherBoardNativeConfig previousConfig = BoardNativeEffect::getWeatherConfig();
  BoardNativeEffect::applyWeatherConfig(command.weatherConfig);

  DeviceMode fromMode = DisplayManager::currentMode;
  if (!switchToTargetMode(
        fromMode,
        MODE_ANIMATION,
        ModeTags::WEATHER,
        true,
        true,
        "weather board applied",
        response)) {
    BoardNativeEffect::applyWeatherConfig(previousConfig);
    return false;
  }
  return true;
}

bool executeCountdownBoard(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  const CountdownBoardNativeConfig previousConfig = BoardNativeEffect::getCountdownConfig();
  BoardNativeEffect::applyCountdownConfig(command.countdownConfig);

  DeviceMode fromMode = DisplayManager::currentMode;
  if (!switchToTargetMode(
        fromMode,
        MODE_ANIMATION,
        ModeTags::COUNTDOWN,
        true,
        true,
        "countdown board applied",
        response)) {
    BoardNativeEffect::applyCountdownConfig(previousConfig);
    return false;
  }
  return true;
}

bool executeStopwatchBoard(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  const StopwatchBoardNativeConfig previousConfig = BoardNativeEffect::getStopwatchConfig();
  BoardNativeEffect::applyStopwatchConfig(command.stopwatchConfig);

  DeviceMode fromMode = DisplayManager::currentMode;
  if (!switchToTargetMode(
        fromMode,
        MODE_ANIMATION,
        ModeTags::STOPWATCH,
        true,
        true,
        "stopwatch board applied",
        response)) {
    BoardNativeEffect::applyStopwatchConfig(previousConfig);
    return false;
  }
  return true;
}

bool executeNotificationBoard(
  const RuntimeCommandBus::RuntimeCommand& command,
  StaticJsonDocument<768>& response
) {
  const NotificationBoardNativeConfig previousConfig = BoardNativeEffect::getNotificationConfig();
  BoardNativeEffect::applyNotificationConfig(command.notificationConfig);

  DeviceMode fromMode = DisplayManager::currentMode;
  if (!switchToTargetMode(
        fromMode,
        MODE_ANIMATION,
        ModeTags::NOTIFICATION,
        true,
        true,
        "notification board applied",
        response)) {
    BoardNativeEffect::applyNotificationConfig(previousConfig);
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
    BoardNativeEffect::applyPlanetScreensaverConfig(previousConfig);
    return false;
  }
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
    Serial.println("清空画布缓冲区");
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
    case RuntimeCommandBus::RuntimeCommandType::GAME_SCREENSAVER:
      return executeGameScreensaver(command, response);
    case RuntimeCommandBus::RuntimeCommandType::TEXT_DISPLAY:
      return executeTextDisplay(command, response);
    case RuntimeCommandBus::RuntimeCommandType::WEATHER_BOARD:
      return executeWeatherBoard(command, response);
    case RuntimeCommandBus::RuntimeCommandType::COUNTDOWN_BOARD:
      return executeCountdownBoard(command, response);
    case RuntimeCommandBus::RuntimeCommandType::STOPWATCH_BOARD:
      return executeStopwatchBoard(command, response);
    case RuntimeCommandBus::RuntimeCommandType::NOTIFICATION_BOARD:
      return executeNotificationBoard(command, response);
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
      if (RuntimeModeCoordinator::isTransientRuntimeMode(DisplayManager::currentMode)) {
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

  size_t pendingCount = 0;
  bool queued = false;
  portENTER_CRITICAL(&gRuntimeCommandQueueMux);
  if (gQueueCount < kRuntimeCommandQueueCapacity) {
    gCommandQueue[gQueueTail] = command;
    gQueueTail = (gQueueTail + 1) % kRuntimeCommandQueueCapacity;
    gQueueCount += 1;
    pendingCount = gQueueCount;
    queued = true;
  }
  portEXIT_CRITICAL(&gRuntimeCommandQueueMux);

  if (!queued) {
    return false;
  }

  Serial.printf("[RuntimeBus] queued type=%s source=%d client=%u pending=%u\n",
                commandTypeLabel(command->type),
                static_cast<int>(command->source),
                command->clientId,
                static_cast<unsigned>(pendingCount));
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

void tick() {
  flushPendingClientResponses();

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
    return;
  }

  if (command->source == RuntimeCommandSource::WEBSOCKET &&
      !clientStillConnected(command->clientId)) {
    Serial.printf("[RuntimeBus] drop disconnected client command=%s client=%u\n",
                  commandTypeLabel(command->type),
                  command->clientId);
    destroyCommand(command);
    return;
  }

  Serial.printf("[RuntimeBus] executing on main loop type=%s client=%u\n",
                commandTypeLabel(command->type),
                command->clientId);

  StaticJsonDocument<768> response;
  bool success = false;
  DisplayManager::lockRuntimeAccess();
  success = executeCommand(*command, response);
  DisplayManager::unlockRuntimeAccess();

  if (!success) {
    Serial.printf("[RuntimeBus] rollback type=%s client=%u\n",
                  commandTypeLabel(command->type),
                  command->clientId);
  }

  Serial.printf("[RuntimeBus] finished type=%s client=%u status=%s\n",
                commandTypeLabel(command->type),
                command->clientId,
                success ? "ok" : "error");

  if (command->source == RuntimeCommandSource::WEBSOCKET) {
    sendClientResponse(command->clientId, response);
  }

  destroyCommand(command);
  flushPendingClientResponses();
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
    case RuntimeCommandType::GAME_SCREENSAVER:
      return "GAME_SCREENSAVER";
    case RuntimeCommandType::TEXT_DISPLAY:
      return "TEXT_DISPLAY";
    case RuntimeCommandType::WEATHER_BOARD:
      return "WEATHER_BOARD";
    case RuntimeCommandType::COUNTDOWN_BOARD:
      return "COUNTDOWN_BOARD";
    case RuntimeCommandType::STOPWATCH_BOARD:
      return "STOPWATCH_BOARD";
    case RuntimeCommandType::NOTIFICATION_BOARD:
      return "NOTIFICATION_BOARD";
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
    default:
      return "NONE";
  }
}
}
