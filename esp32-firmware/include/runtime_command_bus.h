#ifndef RUNTIME_COMMAND_BUS_H
#define RUNTIME_COMMAND_BUS_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "board_native_effect.h"
#include "config_manager.h"
#include "display_manager.h"
#include "game_screensaver_types.h"

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
  GAME_SCREENSAVER = 6,
  TEXT_DISPLAY = 7,
  WEATHER_BOARD = 8,
  COUNTDOWN_BOARD = 9,
  STOPWATCH_BOARD = 10,
  NOTIFICATION_BOARD = 11,
  PLANET_SCREENSAVER = 12,
  EYES_CONFIG = 13,
  EYES_ACTION = 14,
  CLEAR = 15,
  BRIGHTNESS = 16,
  START_LOADING = 17,
  STOP_LOADING = 18,
  SHOW_LOADING = 19,
  LOAD_CANVAS = 20,
  CLEAR_CANVAS = 21,
  HIGHLIGHT_COLOR = 22,
  HIGHLIGHT_ROW = 23,
  TEXT = 24,
  PIXEL = 25,
  IMAGE = 26,
  IMAGE_SPARSE = 27,
  IMAGE_CHUNK = 28,
  APPLY_DEVICE_PARAMS = 29,
  RESTORE_CURRENT_MODE_FRAME = 30,
  RESTORE_AFTER_TRANSIENT_DISCONNECT = 31,
  ABORT_TRANSIENT_TRANSFER_AND_RESTORE = 32,
  HTTP_UPLOAD_IMAGE = 33
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
  GameScreensaverConfig gameScreensaverConfig = {};
  TextDisplayNativeConfig textDisplayConfig = {};
  WeatherBoardNativeConfig weatherConfig = {};
  CountdownBoardNativeConfig countdownConfig = {};
  StopwatchBoardNativeConfig stopwatchConfig = {};
  NotificationBoardNativeConfig notificationConfig = {};
  PlanetScreensaverNativeConfig planetConfig = {};
};

RuntimeCommand* createCommand(RuntimeCommandSource source, uint32_t clientId);
void destroyCommand(RuntimeCommand* command);
bool enqueue(RuntimeCommand* command);
bool enqueueRestoreCurrentModeFrame();
bool enqueueRestoreAfterTransientDisconnect(bool persistState);
bool enqueueAbortTransientTransferAndRestore(const char* reason);

void tick();
const char* commandTypeLabel(RuntimeCommandType type);
}

#endif
