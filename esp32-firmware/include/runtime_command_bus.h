#ifndef RUNTIME_COMMAND_BUS_H
#define RUNTIME_COMMAND_BUS_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "board_native_effect.h"
#include "config_manager.h"
#include "display_manager.h"

class AsyncWebSocketClient;

namespace RuntimeCommandBus {
enum class RuntimeCommandSource : uint8_t {
  WEBSOCKET = 0,
  HTTP = 1,
  SYSTEM = 2
};

enum class RuntimeCommandType : uint8_t {
  NONE = 0,
  MODE_SWITCH = 1,
  THEME_CONFIG = 2,
  BREATH_EFFECT = 3,
  RHYTHM_EFFECT = 4,
  AMBIENT_EFFECT = 5,
  TEXT_DISPLAY = 7,
  PLANET_SCREENSAVER = 8,
  EYES_CONFIG = 9,
  EYES_ACTION = 10,
  CLEAR = 11,
  BRIGHTNESS = 12,
  START_LOADING = 13,
  STOP_LOADING = 14,
  SHOW_LOADING = 15,
  LOAD_CANVAS = 16,
  CLEAR_CANVAS = 17,
  HIGHLIGHT_COLOR = 18,
  HIGHLIGHT_ROW = 19,
  TEXT = 20,
  PIXEL = 21,
  IMAGE = 22,
  IMAGE_SPARSE = 23,
  IMAGE_CHUNK = 24,
  APPLY_DEVICE_PARAMS = 25,
  RESTORE_CURRENT_MODE_FRAME = 26,
  RESTORE_AFTER_TRANSIENT_DISCONNECT = 27,
  ABORT_TRANSIENT_TRANSFER_AND_RESTORE = 28,
  HTTP_UPLOAD_IMAGE = 29,
  WEBSOCKET_TRANSACTION_COMMIT = 30,
  WEBSOCKET_TRANSACTION_ABORT = 31
};

struct RuntimeCommand {
  RuntimeCommandType type = RuntimeCommandType::NONE;
  RuntimeCommandSource source = RuntimeCommandSource::SYSTEM;
  uint32_t clientId = 0;

  DeviceMode targetMode = MODE_CLOCK;
  String businessModeTag;
  String successMessage;
  String stringValue1;
  String rawJsonPayload;
  String restoreReason;

  bool flag1 = false;
  bool flag2 = false;
  bool flag3 = false;
  int intValue1 = 0;
  int intValue2 = 0;
  int intValue3 = 0;
  int intValue4 = 0;
  int intValue5 = 0;
  uint8_t byteValue1 = 0;

  ThemeConfig themeConfig = {};
  EyesConfig eyesConfig = {};
  BreathEffectConfig breathEffectConfig = {};
  RhythmEffectConfig rhythmEffectConfig = {};
  AmbientEffectConfig ambientEffectConfig = {};
  TextDisplayNativeConfig textDisplayConfig = {};
  PlanetScreensaverNativeConfig planetConfig = {};
  MazeModeConfig mazeConfig = {};
  SnakeModeConfig snakeConfig = {};
};

RuntimeCommand* createCommand(RuntimeCommandSource source, uint32_t clientId);
void destroyCommand(RuntimeCommand* command);
bool enqueue(RuntimeCommand* command);
bool enqueueRestoreCurrentModeFrame();
bool enqueueRestoreAfterTransientDisconnect(bool persistState);
bool enqueueAbortTransientTransferAndRestore(const char* reason);
void queueOrSendClientResponse(uint32_t clientId, StaticJsonDocument<768>& response);
bool beginWebSocketTransaction(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
);
bool commitWebSocketTransaction(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
);
bool abortWebSocketTransaction(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
);
bool appendWebSocketTransactionBinary(
  AsyncWebSocketClient* client,
  const uint8_t* data,
  size_t len,
  StaticJsonDocument<768>& response,
  bool& responseReady
);
bool hasActiveWebSocketTransaction(uint32_t clientId);
void handleWebSocketTransactionDisconnect(uint32_t clientId);

void tick();
bool consumePostTransactionYieldHint();
bool shouldPauseMazeForOutboundDrain();
const char* commandTypeLabel(RuntimeCommandType type);
}

#endif
