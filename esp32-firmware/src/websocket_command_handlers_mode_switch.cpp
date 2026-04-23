#include "websocket_mode_command_dispatch.h"

#include "mode_tags.h"
#include "runtime_command_bus.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}

bool hasRequiredTetrisFields(JsonDocument& doc) {
  return doc.containsKey("clearMode") &&
         doc.containsKey("cellSize") &&
         doc.containsKey("speed") &&
         doc.containsKey("showClock") &&
         doc.containsKey("pieces");
}

bool parseTetrisPiecesMask(JsonDocument& doc, uint8_t& piecesMask) {
  JsonArray pieces = doc["pieces"].as<JsonArray>();
  if (pieces.isNull()) {
    return false;
  }

  uint8_t mask = 0;
  for (JsonVariant item : pieces) {
    if (!item.is<int>()) {
      return false;
    }
    int index = item.as<int>();
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

bool fillTetrisModeCommand(
  RuntimeCommandBus::RuntimeCommand* command,
  JsonDocument& doc,
  const char* businessModeTag,
  bool expectedShowClock,
  const char* successMessage
) {
  if (!hasRequiredTetrisFields(doc)) {
    return false;
  }

  if (!doc["showClock"].is<bool>()) {
    return false;
  }

  const bool showClock = doc["showClock"].as<bool>();
  if (showClock != expectedShowClock) {
    return false;
  }

  uint8_t piecesMask = 0;
  if (!parseTetrisPiecesMask(doc, piecesMask)) {
    return false;
  }

  command->targetMode = MODE_ANIMATION;
  command->businessModeTag = businessModeTag;
  command->successMessage = successMessage;
  command->flag1 = doc["clearMode"].as<bool>();
  command->flag2 = expectedShowClock;
  command->intValue1 = doc["cellSize"].as<int>();
  command->intValue2 = doc["speed"].as<int>();
  command->byteValue1 = piecesMask;
  return true;
}
}

namespace WebSocketModeCommandDispatch {
bool handleModeSwitchCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();
  if (cmd != "set_mode") {
    return false;
  }

  String mode = doc["mode"].as<String>();
  RuntimeCommandBus::RuntimeCommand* command =
    RuntimeCommandBus::createCommand(
      RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
      client != nullptr ? client->id() : 0
    );
  if (command == nullptr) {
    setErrorResponse(response, "out of memory");
    return true;
  }
  command->type = RuntimeCommandBus::RuntimeCommandType::MODE_SWITCH;
  command->flag1 = true;
  command->flag2 = true;

  if (mode == ModeTags::CLOCK) {
    command->targetMode = MODE_CLOCK;
    command->businessModeTag = ModeTags::CLOCK;
    command->successMessage = "switched to static clock mode";
  } else if (mode == ModeTags::CANVAS) {
    command->targetMode = MODE_CANVAS;
    command->businessModeTag = ModeTags::CANVAS;
    command->flag1 = false;
    command->successMessage = "switched to canvas mode";
  } else if (mode == ModeTags::ANIMATION) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::ANIMATION;
    command->successMessage = "switched to animation mode";
  } else if (mode == ModeTags::GIF_PLAYER) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::GIF_PLAYER;
    command->successMessage = "switched to gif player mode";
  } else if (mode == ModeTags::THEME) {
    command->targetMode = MODE_THEME;
    command->businessModeTag = ModeTags::THEME;
    command->successMessage = "switched to theme mode";
  } else if (mode == ModeTags::TEXT_DISPLAY) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::TEXT_DISPLAY;
    command->successMessage = "switched to text display mode";
  } else if (mode == ModeTags::BREATH_EFFECT) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::BREATH_EFFECT;
    command->successMessage = "switched to breath effect mode";
  } else if (mode == ModeTags::RHYTHM_EFFECT) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::RHYTHM_EFFECT;
    command->successMessage = "switched to rhythm effect mode";
  } else if (mode == ModeTags::AMBIENT_EFFECT) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::AMBIENT_EFFECT;
    command->successMessage = "switched to ambient effect mode";
  } else if (mode == ModeTags::LED_MATRIX_SHOWCASE) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::LED_MATRIX_SHOWCASE;
    command->successMessage = "switched to led matrix showcase mode";
  } else if (mode == ModeTags::TRANSFERRING) {
    command->targetMode = MODE_TRANSFERRING;
    command->businessModeTag = ModeTags::TRANSFERRING;
    command->flag1 = false;
    command->flag2 = false;
    command->successMessage = "entered transferring mode";
  } else if (mode == ModeTags::TETRIS) {
    if (!fillTetrisModeCommand(
          command,
          doc,
          ModeTags::TETRIS,
          false,
          "tetris started")) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "invalid tetris mode fields");
      return true;
    }
  } else if (mode == ModeTags::TETRIS_CLOCK) {
    if (!fillTetrisModeCommand(
          command,
          doc,
          ModeTags::TETRIS_CLOCK,
          true,
          "tetris clock started")) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "invalid tetris clock mode fields");
      return true;
    }
  } else if (mode == ModeTags::WEATHER) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::WEATHER;
    command->successMessage = "switched to weather mode";
  } else if (mode == ModeTags::COUNTDOWN) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::COUNTDOWN;
    command->successMessage = "switched to countdown mode";
  } else if (mode == ModeTags::STOPWATCH) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::STOPWATCH;
    command->successMessage = "switched to stopwatch mode";
  } else if (mode == ModeTags::NOTIFICATION) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::NOTIFICATION;
    command->successMessage = "switched to notification mode";
  } else if (mode == ModeTags::PLANET_SCREENSAVER) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::PLANET_SCREENSAVER;
    command->successMessage = "switched to planet screensaver mode";
  } else if (mode == ModeTags::EYES) {
    command->targetMode = MODE_ANIMATION;
    command->businessModeTag = ModeTags::EYES;
    command->successMessage = "switched to eyes mode";
  } else {
    RuntimeCommandBus::destroyCommand(command);
    setErrorResponse(response, "invalid mode");
    return true;
  }

  if (!RuntimeCommandBus::enqueue(command)) {
    RuntimeCommandBus::destroyCommand(command);
    setErrorResponse(response, "device busy");
    return true;
  }

  if (mode == ModeTags::TRANSFERRING) {
    Serial.println("进入传输模式，准备接收动画数据");
  }
  responseSent = true;
  return true;
}
}
