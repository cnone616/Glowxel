#include "websocket_command_handlers.h"
#include "clock_font_renderer.h"
#include "config_manager.h"
#include "display_manager.h"
#include "eyes_effect.h"
#include "mode_tags.h"
#include "runtime_command_bus.h"
#include "websocket_async_command_response.h"
#include <string.h>

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
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

bool ensureEyesLayoutObject(JsonObject layout) {
  return layout.containsKey("eyeY") &&
         layout.containsKey("eyeSpacing") &&
         layout.containsKey("eyeWidth") &&
         layout.containsKey("eyeHeight") &&
         layout.containsKey("timeX") &&
         layout.containsKey("timeY");
}

bool ensureEyesBehaviorObject(JsonObject behavior) {
  return behavior.containsKey("autoSwitch") &&
         behavior.containsKey("blinkIntervalMs") &&
         behavior.containsKey("lookIntervalMs") &&
         behavior.containsKey("idleMove") &&
         behavior.containsKey("sleepyAfterMs") &&
         behavior.containsKey("expressionRhythm");
}

bool ensureEyesInteractionObject(JsonObject interaction) {
  return interaction.containsKey("lookHoldMs") &&
         interaction.containsKey("moodHoldMs");
}

bool ensureEyesTimeObject(JsonObject time) {
  return time.containsKey("show") &&
         time.containsKey("showSeconds") &&
         time.containsKey("font") &&
         time.containsKey("fontSize");
}

bool ensureEyesStyleObject(JsonObject style) {
  return style.containsKey("eyeColor") &&
         style.containsKey("timeColor");
}

bool parseEyesExpressionRhythm(const char* value, uint8_t& expressionRhythm) {
  if (value == nullptr) {
    return false;
  }
  if (strcmp(value, "slow") == 0) {
    expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_SLOW;
    return true;
  }
  if (strcmp(value, "standard") == 0) {
    expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_STANDARD;
    return true;
  }
  if (strcmp(value, "lively") == 0) {
    expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_LIVELY;
    return true;
  }
  return false;
}
}

bool WebSocketCommandHandlers::handleEyesCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_eyes_config") {
    if (!doc.containsKey("config")) {
      setErrorResponse(response, "missing config");
      return true;
    }

    JsonObject config = doc["config"].as<JsonObject>();
    if (!config.containsKey("layout") ||
        !config.containsKey("behavior") ||
        !config.containsKey("interaction") ||
        !config.containsKey("time") ||
        !config.containsKey("style")) {
      setErrorResponse(response, "missing eyes config sections");
      return true;
    }

    JsonObject layout = config["layout"].as<JsonObject>();
    JsonObject behavior = config["behavior"].as<JsonObject>();
    JsonObject interaction = config["interaction"].as<JsonObject>();
    JsonObject time = config["time"].as<JsonObject>();
    JsonObject style = config["style"].as<JsonObject>();

    if (!ensureEyesLayoutObject(layout) ||
        !ensureEyesBehaviorObject(behavior) ||
        !ensureEyesInteractionObject(interaction) ||
        !ensureEyesTimeObject(time) ||
        !ensureEyesStyleObject(style)) {
      setErrorResponse(response, "missing eyes config fields");
      return true;
    }

    const char* eyeColor = style["eyeColor"];
    const char* timeColor = style["timeColor"];
    const char* timeFont = time["font"];
    const char* expressionRhythmText = behavior["expressionRhythm"];
    uint8_t timeFontId = 0;
    uint8_t expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_STANDARD;
    int timeFontSize = time["fontSize"].as<int>();
    if (!isHexColorText(eyeColor) ||
        !isHexColorText(timeColor)) {
      setErrorResponse(response, "invalid eyes color");
      return true;
    }
    if (timeFont == nullptr || !clockFontIdFromString(timeFont, timeFontId)) {
      setErrorResponse(response, "invalid eyes time font");
      return true;
    }
    if (!parseEyesExpressionRhythm(expressionRhythmText, expressionRhythm)) {
      setErrorResponse(response, "invalid eyes expression rhythm");
      return true;
    }
    if (timeFontSize < 1 || timeFontSize > 3) {
      setErrorResponse(response, "invalid eyes time font size");
      return true;
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

    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::EYES_CONFIG;
    command->eyesConfig = nextConfig;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    return wsSendAcceptedResponse(client, response, responseSent);
  }

  if (cmd == "eyes_interact") {
    if (!doc.containsKey("action")) {
      setErrorResponse(response, "missing action");
      return true;
    }

    const char* action = doc["action"];
    if (action == nullptr || action[0] == '\0') {
      setErrorResponse(response, "invalid eyes action");
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
    command->type = RuntimeCommandBus::RuntimeCommandType::EYES_ACTION;
    command->stringValue1 = action;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    return wsSendAcceptedResponse(client, response, responseSent);
  }

  return false;
}
