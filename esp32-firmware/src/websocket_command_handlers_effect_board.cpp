#include "websocket_effect_command_dispatch.h"

#include "board_native_effect.h"
#include "clock_font_renderer.h"
#include "runtime_command_bus.h"
#include "websocket_async_command_response.h"
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
  AsyncWebSocketClient* client,
  RuntimeCommandBus::RuntimeCommand* command,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (!RuntimeCommandBus::enqueue(command)) {
    RuntimeCommandBus::destroyCommand(command);
    wsSetErrorResponse(response, "device busy");
    return true;
  }
  return wsSendAcceptedResponse(client, response, responseSent);
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
  return enqueueWsCommand(client, command, response, responseSent);
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
      !doc.containsKey("colorSeed") ||
      !doc.containsKey("planetX") ||
      !doc.containsKey("planetY") ||
      !doc.containsKey("font") ||
      !doc.containsKey("showSeconds") ||
      !doc.containsKey("time")) {
    wsSetErrorResponse(response, "missing planet screensaver fields");
    return true;
  }

  const char* preset = doc["preset"];
  if (preset == nullptr ||
      (strcmp(preset, "earth") != 0 &&
       strcmp(preset, "terran_wet") != 0 &&
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
       strcmp(preset, "portal_green") != 0 &&
       strcmp(preset, "portal_blue") != 0 &&
       strcmp(preset, "portal_yellow") != 0 &&
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

  const char* fontName = doc["font"];
  uint8_t fontId = 0;
  if (fontName == nullptr || !clockFontIdFromString(fontName, fontId)) {
    wsSetErrorResponse(response, "invalid planet font");
    return true;
  }

  JsonObject time = doc["time"].as<JsonObject>();
  if (time.isNull() ||
      !time.containsKey("show") ||
      !time.containsKey("fontSize") ||
      !time.containsKey("x") ||
      !time.containsKey("y") ||
      !time.containsKey("color")) {
    wsSetErrorResponse(response, "missing planet time fields");
    return true;
  }

  JsonObject timeColor = time["color"].as<JsonObject>();
  if (timeColor.isNull() || !wsEnsureColorObject(timeColor)) {
    wsSetErrorResponse(response, "invalid planet time color");
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
  command->planetConfig.planetX =
    (uint8_t)wsClampInt(doc["planetX"].as<int>(), 0, 63);
  command->planetConfig.planetY =
    (uint8_t)wsClampInt(doc["planetY"].as<int>(), 0, 63);
  command->planetConfig.font = fontId;
  command->planetConfig.showSeconds = doc["showSeconds"].as<bool>();
  command->planetConfig.time.show = time["show"].as<bool>();
  command->planetConfig.time.fontSize =
    (uint8_t)wsClampInt(time["fontSize"].as<int>(), 1, 3);
  command->planetConfig.time.x =
    (uint8_t)wsClampInt(time["x"].as<int>(), 0, 63);
  command->planetConfig.time.y =
    (uint8_t)wsClampInt(time["y"].as<int>(), 0, 63);
  command->planetConfig.time.r = timeColor["r"].as<uint8_t>();
  command->planetConfig.time.g = timeColor["g"].as<uint8_t>();
  command->planetConfig.time.b = timeColor["b"].as<uint8_t>();
  return enqueueWsCommand(client, command, response, responseSent);
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

  if (cmd == "set_planet_screensaver") {
    return handlePlanetScreensaverCommand(client, doc, response, responseSent);
  }

  return false;
}
}
