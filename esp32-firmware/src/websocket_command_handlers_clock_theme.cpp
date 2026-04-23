#include "websocket_command_handlers.h"
#include "clock_font_renderer.h"
#include "config_manager.h"
#include "display_manager.h"
#include "mode_tags.h"
#include "runtime_command_bus.h"
#include "runtime_mode_coordinator.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}
}

bool WebSocketCommandHandlers::handleClockCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  String cmd = doc["cmd"].as<String>();
  if (cmd != "set_clock_config") {
    return false;
  }

  if (!doc.containsKey("config") || !doc.containsKey("clockMode")) {
    setErrorResponse(response, "missing config or clockMode");
    return true;
  }

  JsonObject config = doc["config"];
  const char* clockMode = doc["clockMode"];

  if (strcmp(clockMode, ModeTags::CLOCK) != 0 && strcmp(clockMode, ModeTags::ANIMATION) != 0) {
    setErrorResponse(response, "invalid clockMode");
    return true;
  }

  if (!config.containsKey("font") ||
      !config.containsKey("showSeconds") ||
      !config.containsKey("hourFormat") ||
      !config.containsKey("time") ||
      !config.containsKey("date") ||
      !config.containsKey("week") ||
      !config.containsKey("image")) {
    setErrorResponse(response, "missing required clock config fields");
    return true;
  }

  ClockConfig& target = (strcmp(clockMode, ModeTags::ANIMATION) == 0)
    ? ConfigManager::animClockConfig
    : ConfigManager::clockConfig;

  const char* fontName = config["font"];
  uint8_t fontId = 0;
  if (!clockFontIdFromString(fontName, fontId)) {
    setErrorResponse(response, "invalid font");
    return true;
  }

  int hourFormat = config["hourFormat"];
  if (hourFormat != 12 && hourFormat != 24) {
    setErrorResponse(response, "invalid hourFormat");
    return true;
  }

  JsonObject time = config["time"];
  JsonObject date = config["date"];
  JsonObject week = config["week"];
  JsonObject image = config["image"];

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
    setErrorResponse(response, "missing nested clock config fields");
    return true;
  }

  JsonObject timeColor = time["color"];
  JsonObject dateColor = date["color"];
  JsonObject weekColor = week["color"];

  if (!timeColor.containsKey("r") ||
      !timeColor.containsKey("g") ||
      !timeColor.containsKey("b") ||
      !dateColor.containsKey("r") ||
      !dateColor.containsKey("g") ||
      !dateColor.containsKey("b") ||
      !weekColor.containsKey("r") ||
      !weekColor.containsKey("g") ||
      !weekColor.containsKey("b")) {
    setErrorResponse(response, "missing color fields");
    return true;
  }

  target.font = fontId;
  target.showSeconds = config["showSeconds"];
  target.hourFormat = hourFormat;

  target.time.show = time["show"];
  target.time.fontSize = time["fontSize"];
  target.time.x = time["x"];
  target.time.y = time["y"];
  target.time.r = timeColor["r"];
  target.time.g = timeColor["g"];
  target.time.b = timeColor["b"];

  target.date.show = date["show"];
  target.date.fontSize = date["fontSize"];
  target.date.x = date["x"];
  target.date.y = date["y"];
  target.date.r = dateColor["r"];
  target.date.g = dateColor["g"];
  target.date.b = dateColor["b"];

  target.week.show = week["show"];
  target.week.x = week["x"];
  target.week.y = week["y"];
  target.week.r = weekColor["r"];
  target.week.g = weekColor["g"];
  target.week.b = weekColor["b"];

  target.image.show = image["show"];
  target.image.x = image["x"];
  target.image.y = image["y"];
  target.image.width = image["width"];
  target.image.height = image["height"];

  if (strcmp(clockMode, ModeTags::ANIMATION) == 0) {
    ConfigManager::saveAnimClockConfig();
  } else {
    ConfigManager::saveClockConfig();
  }

  response["message"] = "clock config updated";
  return true;
}

bool WebSocketCommandHandlers::handleThemeCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();
  if (cmd != "set_theme_config") {
    return false;
  }

  if (!doc.containsKey("themeId")) {
    setErrorResponse(response, "missing themeId");
    return true;
  }

  const char* themeId = doc["themeId"];
  if (themeId == nullptr || themeId[0] == '\0') {
    setErrorResponse(response, "invalid themeId");
    return true;
  }

  RuntimeCommandBus::RuntimeCommand* command =
    RuntimeCommandBus::createCommand(
      RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
      client != nullptr ? client->id() : 0
    );
  if (command == nullptr) {
    setErrorResponse(response, "out of memory");
    return true;
  }
  snprintf(
    command->themeConfig.themeId,
    sizeof(command->themeConfig.themeId),
    "%s",
    themeId
  );
  command->type = RuntimeCommandBus::RuntimeCommandType::THEME_CONFIG;
  if (!RuntimeCommandBus::enqueue(command)) {
    RuntimeCommandBus::destroyCommand(command);
    setErrorResponse(response, "device busy");
    return true;
  }
  responseSent = true;
  return true;
}
