#include "websocket_effect_command_dispatch.h"

#include "board_native_effect.h"
#include "runtime_command_bus.h"
#include "websocket_effect_common.h"

namespace {
RuntimeCommandBus::RuntimeCommand* createWsCommand(
  AsyncWebSocketClient* client,
  StaticJsonDocument<768>& response
) {
  RuntimeCommandBus::RuntimeCommand* command =
    RuntimeCommandBus::createCommand(
      RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
      client != nullptr ? client->id() : 0
    );
  if (command == nullptr) {
    wsSetErrorResponse(response, "out of memory");
  }
  return command;
}

bool enqueueWsCommand(
  RuntimeCommandBus::RuntimeCommand* command,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (!RuntimeCommandBus::enqueue(command)) {
    RuntimeCommandBus::destroyCommand(command);
    wsSetErrorResponse(response, "device busy");
    return true;
  }

  responseSent = true;
  return true;
}

bool handleTextDisplayCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (!doc.containsKey("template") ||
      !doc.containsKey("text") ||
      !doc.containsKey("progress") ||
      !doc.containsKey("repeat") ||
      !doc.containsKey("pushIcon") ||
      !doc.containsKey("icon") ||
      !doc.containsKey("speed") ||
      !doc.containsKey("color") ||
      !doc.containsKey("bgColor")) {
    wsSetErrorResponse(response, "missing text display fields");
    return true;
  }

  uint8_t colorR = 0;
  uint8_t colorG = 0;
  uint8_t colorB = 0;
  uint8_t bgColorR = 0;
  uint8_t bgColorG = 0;
  uint8_t bgColorB = 0;
  if (!wsParseRequiredColorObject(doc, "color", colorR, colorG, colorB) ||
      !wsParseRequiredColorObject(doc, "bgColor", bgColorR, bgColorG, bgColorB)) {
    wsSetErrorResponse(response, "invalid text display color");
    return true;
  }

  RuntimeCommandBus::RuntimeCommand* command = createWsCommand(client, response);
  if (command == nullptr) {
    return true;
  }

  command->type = RuntimeCommandBus::RuntimeCommandType::TEXT_DISPLAY;
  command->textDisplayConfig.templateName = doc["template"].as<const char*>();
  command->textDisplayConfig.text = doc["text"].as<const char*>();
  command->textDisplayConfig.progress = (uint8_t)wsClampInt(doc["progress"].as<int>(), 0, 100);
  command->textDisplayConfig.repeat = (uint8_t)wsClampInt(doc["repeat"].as<int>(), 1, 8);
  command->textDisplayConfig.pushIcon = doc["pushIcon"].as<bool>();
  command->textDisplayConfig.icon = doc["icon"].as<const char*>();
  command->textDisplayConfig.speed = (uint8_t)wsClampInt(doc["speed"].as<int>(), 1, 7);
  command->textDisplayConfig.colorR = colorR;
  command->textDisplayConfig.colorG = colorG;
  command->textDisplayConfig.colorB = colorB;
  command->textDisplayConfig.bgColorR = bgColorR;
  command->textDisplayConfig.bgColorG = bgColorG;
  command->textDisplayConfig.bgColorB = bgColorB;
  return enqueueWsCommand(command, response, responseSent);
}

bool handleWeatherBoardCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (!doc.containsKey("weatherType") ||
      !doc.containsKey("city") ||
      !doc.containsKey("temperature") ||
      !doc.containsKey("humidity") ||
      !doc.containsKey("unit")) {
    wsSetErrorResponse(response, "missing weather board fields");
    return true;
  }

  const char* unit = doc["unit"];
  if (unit == nullptr || (strcmp(unit, "c") != 0 && strcmp(unit, "f") != 0)) {
    wsSetErrorResponse(response, "invalid weather unit");
    return true;
  }

  RuntimeCommandBus::RuntimeCommand* command = createWsCommand(client, response);
  if (command == nullptr) {
    return true;
  }

  command->type = RuntimeCommandBus::RuntimeCommandType::WEATHER_BOARD;
  command->weatherConfig.weatherType = doc["weatherType"].as<const char*>();
  command->weatherConfig.city = doc["city"].as<const char*>();
  command->weatherConfig.temperature = (int16_t)doc["temperature"].as<int>();
  command->weatherConfig.humidity = (uint8_t)wsClampInt(doc["humidity"].as<int>(), 0, 100);
  command->weatherConfig.unit = unit;
  return enqueueWsCommand(command, response, responseSent);
}

bool handleCountdownBoardCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (!doc.containsKey("hours") ||
      !doc.containsKey("minutes") ||
      !doc.containsKey("seconds") ||
      !doc.containsKey("progress")) {
    wsSetErrorResponse(response, "missing countdown board fields");
    return true;
  }

  RuntimeCommandBus::RuntimeCommand* command = createWsCommand(client, response);
  if (command == nullptr) {
    return true;
  }

  command->type = RuntimeCommandBus::RuntimeCommandType::COUNTDOWN_BOARD;
  command->countdownConfig.hours = (uint8_t)wsClampInt(doc["hours"].as<int>(), 0, 99);
  command->countdownConfig.minutes = (uint8_t)wsClampInt(doc["minutes"].as<int>(), 0, 59);
  command->countdownConfig.seconds = (uint8_t)wsClampInt(doc["seconds"].as<int>(), 0, 59);
  command->countdownConfig.progress = (uint8_t)wsClampInt(doc["progress"].as<int>(), 0, 100);
  return enqueueWsCommand(command, response, responseSent);
}

bool handleStopwatchBoardCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (!doc.containsKey("previewSeconds") ||
      !doc.containsKey("lapCount") ||
      !doc.containsKey("showMilliseconds")) {
    wsSetErrorResponse(response, "missing stopwatch board fields");
    return true;
  }

  RuntimeCommandBus::RuntimeCommand* command = createWsCommand(client, response);
  if (command == nullptr) {
    return true;
  }

  command->type = RuntimeCommandBus::RuntimeCommandType::STOPWATCH_BOARD;
  command->stopwatchConfig.previewSeconds =
    (uint16_t)wsClampInt(doc["previewSeconds"].as<int>(), 0, 35999);
  command->stopwatchConfig.lapCount = (uint8_t)wsClampInt(doc["lapCount"].as<int>(), 0, 99);
  command->stopwatchConfig.showMilliseconds = doc["showMilliseconds"].as<bool>();
  return enqueueWsCommand(command, response, responseSent);
}

bool handleNotificationBoardCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (!doc.containsKey("repeatMode") ||
      !doc.containsKey("text") ||
      !doc.containsKey("icon") ||
      !doc.containsKey("accentColor") ||
      !doc.containsKey("contentType") ||
      !doc.containsKey("textTemplate") ||
      !doc.containsKey("staticTemplate") ||
      !doc.containsKey("animationTemplate") ||
      !doc.containsKey("hour") ||
      !doc.containsKey("minute")) {
    wsSetErrorResponse(response, "missing notification board fields");
    return true;
  }

  uint8_t accentR = 0;
  uint8_t accentG = 0;
  uint8_t accentB = 0;
  if (!wsParseRequiredColorObject(doc, "accentColor", accentR, accentG, accentB)) {
    wsSetErrorResponse(response, "invalid notification accent color");
    return true;
  }

  RuntimeCommandBus::RuntimeCommand* command = createWsCommand(client, response);
  if (command == nullptr) {
    return true;
  }

  command->type = RuntimeCommandBus::RuntimeCommandType::NOTIFICATION_BOARD;
  command->notificationConfig.repeatMode = doc["repeatMode"].as<const char*>();
  command->notificationConfig.text = doc["text"].as<const char*>();
  command->notificationConfig.icon = doc["icon"].as<const char*>();
  command->notificationConfig.accentR = accentR;
  command->notificationConfig.accentG = accentG;
  command->notificationConfig.accentB = accentB;
  command->notificationConfig.contentType = doc["contentType"].as<const char*>();
  command->notificationConfig.textTemplate = doc["textTemplate"].as<const char*>();
  command->notificationConfig.staticTemplate = doc["staticTemplate"].as<const char*>();
  command->notificationConfig.animationTemplate = doc["animationTemplate"].as<const char*>();
  command->notificationConfig.hour = (uint8_t)wsClampInt(doc["hour"].as<int>(), 0, 23);
  command->notificationConfig.minute = (uint8_t)wsClampInt(doc["minute"].as<int>(), 0, 59);
  return enqueueWsCommand(command, response, responseSent);
}

bool handlePlanetScreensaverCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (!doc.containsKey("preset") ||
      !doc.containsKey("size") ||
      !doc.containsKey("direction") ||
      !doc.containsKey("speed") ||
      !doc.containsKey("seed") ||
      !doc.containsKey("colorSeed")) {
    wsSetErrorResponse(response, "missing planet screensaver fields");
    return true;
  }

  const char* preset = doc["preset"];
  if (preset == nullptr ||
      (strcmp(preset, "terran_wet") != 0 &&
       strcmp(preset, "terran_dry") != 0 &&
       strcmp(preset, "islands") != 0 &&
       strcmp(preset, "no_atmosphere") != 0 &&
       strcmp(preset, "gas_giant_1") != 0 &&
       strcmp(preset, "gas_giant_2") != 0 &&
       strcmp(preset, "ice_world") != 0 &&
       strcmp(preset, "lava_world") != 0 &&
       strcmp(preset, "asteroid") != 0 &&
       strcmp(preset, "black_hole") != 0 &&
       strcmp(preset, "galaxy") != 0 &&
       strcmp(preset, "star") != 0)) {
    wsSetErrorResponse(response, "invalid planet preset");
    return true;
  }

  const char* size = doc["size"];
  if (size == nullptr ||
      (strcmp(size, "small") != 0 &&
       strcmp(size, "medium") != 0 &&
       strcmp(size, "large") != 0)) {
    wsSetErrorResponse(response, "invalid planet size");
    return true;
  }

  const char* direction = doc["direction"];
  if (direction == nullptr ||
      (strcmp(direction, "left") != 0 &&
       strcmp(direction, "right") != 0)) {
    wsSetErrorResponse(response, "invalid planet direction");
    return true;
  }

  RuntimeCommandBus::RuntimeCommand* command = createWsCommand(client, response);
  if (command == nullptr) {
    return true;
  }

  command->type = RuntimeCommandBus::RuntimeCommandType::PLANET_SCREENSAVER;
  snprintf(command->planetConfig.preset, sizeof(command->planetConfig.preset), "%s", preset);
  snprintf(command->planetConfig.size, sizeof(command->planetConfig.size), "%s", size);
  snprintf(command->planetConfig.direction, sizeof(command->planetConfig.direction), "%s", direction);
  command->planetConfig.speed = (uint8_t)wsClampInt(doc["speed"].as<int>(), 1, 7);
  command->planetConfig.seed = doc["seed"].as<uint32_t>();
  command->planetConfig.colorSeed = doc["colorSeed"].as<uint32_t>();
  return enqueueWsCommand(command, response, responseSent);
}
}

namespace WebSocketEffectCommandDispatch {
bool handleBoardEffectCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_text_display") {
    return handleTextDisplayCommand(client, doc, response, responseSent);
  }

  if (cmd == "set_weather_board") {
    return handleWeatherBoardCommand(client, doc, response, responseSent);
  }

  if (cmd == "set_countdown_board") {
    return handleCountdownBoardCommand(client, doc, response, responseSent);
  }

  if (cmd == "set_stopwatch_board") {
    return handleStopwatchBoardCommand(client, doc, response, responseSent);
  }

  if (cmd == "set_notification_board") {
    return handleNotificationBoardCommand(client, doc, response, responseSent);
  }

  if (cmd == "set_planet_screensaver") {
    return handlePlanetScreensaverCommand(client, doc, response, responseSent);
  }

  return false;
}
}
