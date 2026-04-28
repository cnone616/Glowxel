#include "websocket_mode_command_dispatch.h"

#include <string.h>

#include "clock_font_renderer.h"
#include "maze_mode_types.h"
#include "mode_tags.h"
#include "runtime_command_bus.h"
#include "websocket_async_command_response.h"
#include "websocket_effect_common.h"

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

bool parseRequiredMazeColorText(
  JsonDocument& doc,
  const char* field,
  char* target,
  size_t targetSize
) {
  if (!doc.containsKey(field) || !doc[field].is<const char*>()) {
    return false;
  }

  const char* value = doc[field];
  if (!isHexColorText(value)) {
    return false;
  }

  snprintf(target, targetSize, "%s", value);
  return true;
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
  const char* successMessage
) {
  if (!hasRequiredTetrisFields(doc)) {
    return false;
  }

  if (!doc["showClock"].is<bool>()) {
    return false;
  }

  const bool showClock = doc["showClock"].as<bool>();

  uint8_t piecesMask = 0;
  if (!parseTetrisPiecesMask(doc, piecesMask)) {
    return false;
  }

  command->targetMode = MODE_ANIMATION;
  command->businessModeTag = businessModeTag;
  command->successMessage = successMessage;
  command->flag1 = doc["clearMode"].as<bool>();
  command->flag2 = showClock;
  command->intValue1 = doc["cellSize"].as<int>();
  command->intValue2 = doc["speed"].as<int>();
  command->byteValue1 = piecesMask;
  return true;
}

bool fillTetrisClockModeCommand(
  RuntimeCommandBus::RuntimeCommand* command,
  JsonDocument& doc
) {
  if (!doc.containsKey("cellSize") ||
      !doc.containsKey("speed") ||
      !doc.containsKey("hourFormat")) {
    return false;
  }

  if (!doc["cellSize"].is<int>() ||
      !doc["speed"].is<int>() ||
      !doc["hourFormat"].is<int>()) {
    return false;
  }

  const int cellSize = doc["cellSize"].as<int>();
  const int speed = doc["speed"].as<int>();
  const int hourFormat = doc["hourFormat"].as<int>();

  if ((cellSize != 1 && cellSize != 2) ||
      speed <= 0 ||
      (hourFormat != 12 && hourFormat != 24)) {
    return false;
  }

  command->targetMode = MODE_ANIMATION;
  command->businessModeTag = ModeTags::TETRIS_CLOCK;
  command->successMessage = "tetris clock started";
  command->intValue1 = cellSize;
  command->intValue2 = speed;
  command->intValue3 = hourFormat;
  return true;
}

bool fillMazeModeCommand(
  RuntimeCommandBus::RuntimeCommand* command,
  JsonDocument& doc
) {
  if (!doc.containsKey("speed") ||
      !doc.containsKey("mazeSizeMode") ||
      !doc.containsKey("showClock") ||
      !doc.containsKey("panelBgColor") ||
      !doc.containsKey("borderColor") ||
      !doc.containsKey("timeColor") ||
      !doc.containsKey("dateColor") ||
      !doc.containsKey("generationPathColor") ||
      !doc.containsKey("searchVisitedColor") ||
      !doc.containsKey("searchFrontierColor") ||
      !doc.containsKey("solvedPathStartColor") ||
      !doc.containsKey("solvedPathEndColor")) {
    return false;
  }

  if (!doc["speed"].is<int>() || !doc["showClock"].is<bool>()) {
    return false;
  }

  const int speed = doc["speed"].as<int>();
  if (speed < 1 || speed > 10) {
    return false;
  }

  String mazeSizeMode = doc["mazeSizeMode"].as<String>();
  int mazeSizeModeValue = -1;
  if (mazeSizeMode == "wide") {
    mazeSizeModeValue = MAZE_SIZE_WIDE;
  } else if (mazeSizeMode == "dense") {
    mazeSizeModeValue = MAZE_SIZE_DENSE;
  } else {
    return false;
  }

  command->targetMode = MODE_ANIMATION;
  command->businessModeTag = ModeTags::MAZE;
  command->successMessage = "maze mode started";
  command->mazeConfig.speed = static_cast<uint8_t>(speed);
  command->mazeConfig.mazeSizeMode = static_cast<uint8_t>(mazeSizeModeValue);
  command->mazeConfig.showClock = doc["showClock"].as<bool>();
  if (!parseRequiredMazeColorText(
        doc,
        "panelBgColor",
        command->mazeConfig.panelBgColor,
        sizeof(command->mazeConfig.panelBgColor)
      ) ||
      !parseRequiredMazeColorText(
        doc,
        "borderColor",
        command->mazeConfig.borderColor,
        sizeof(command->mazeConfig.borderColor)
      ) ||
      !parseRequiredMazeColorText(
        doc,
        "timeColor",
        command->mazeConfig.timeColor,
        sizeof(command->mazeConfig.timeColor)
      ) ||
      !parseRequiredMazeColorText(
        doc,
        "dateColor",
        command->mazeConfig.dateColor,
        sizeof(command->mazeConfig.dateColor)
      ) ||
      !parseRequiredMazeColorText(
        doc,
        "generationPathColor",
        command->mazeConfig.generationPathColor,
        sizeof(command->mazeConfig.generationPathColor)
      ) ||
      !parseRequiredMazeColorText(
        doc,
        "searchVisitedColor",
        command->mazeConfig.searchVisitedColor,
        sizeof(command->mazeConfig.searchVisitedColor)
      ) ||
      !parseRequiredMazeColorText(
        doc,
        "searchFrontierColor",
        command->mazeConfig.searchFrontierColor,
        sizeof(command->mazeConfig.searchFrontierColor)
      ) ||
      !parseRequiredMazeColorText(
        doc,
        "solvedPathStartColor",
        command->mazeConfig.solvedPathStartColor,
        sizeof(command->mazeConfig.solvedPathStartColor)
      ) ||
      !parseRequiredMazeColorText(
        doc,
        "solvedPathEndColor",
        command->mazeConfig.solvedPathEndColor,
        sizeof(command->mazeConfig.solvedPathEndColor)
      )) {
    return false;
  }
  return true;
}

bool fillSnakeModeCommand(
  RuntimeCommandBus::RuntimeCommand* command,
  JsonDocument& doc
) {
  if (!doc.containsKey("speed") ||
      !doc.containsKey("snakeWidth") ||
      !doc.containsKey("snakeColor") ||
      !doc.containsKey("foodColor") ||
      !doc.containsKey("font") ||
      !doc.containsKey("showSeconds") ||
      !doc.containsKey("snakeSkin")) {
    return false;
  }

  if (!doc["speed"].is<int>() ||
      !doc["snakeWidth"].is<int>() ||
      !doc["showSeconds"].is<bool>()) {
    return false;
  }

  const int speed = doc["speed"].as<int>();
  const int snakeWidth = doc["snakeWidth"].as<int>();
  if (speed < 1 || speed > 10 || snakeWidth < 2 || snakeWidth > 4) {
    return false;
  }

  const char* fontText = doc["font"];
  const char* snakeSkin = doc["snakeSkin"];
  if (fontText == nullptr || snakeSkin == nullptr) {
    return false;
  }

  uint8_t fontId = 0;
  if (!clockFontIdFromString(fontText, fontId)) {
    return false;
  }

  if (strcmp(snakeSkin, "solid") != 0 &&
      strcmp(snakeSkin, "gradient") != 0 &&
      strcmp(snakeSkin, "spotted") != 0) {
    return false;
  }

  uint8_t snakeColorR = 0;
  uint8_t snakeColorG = 0;
  uint8_t snakeColorB = 0;
  uint8_t foodColorR = 0;
  uint8_t foodColorG = 0;
  uint8_t foodColorB = 0;
  if (!wsParseRequiredColorObject(doc, "snakeColor", snakeColorR, snakeColorG, snakeColorB) ||
      !wsParseRequiredColorObject(doc, "foodColor", foodColorR, foodColorG, foodColorB)) {
    return false;
  }

  command->targetMode = MODE_ANIMATION;
  command->businessModeTag = ModeTags::SNAKE;
  command->successMessage = "snake mode started";
  command->snakeConfig.speed = static_cast<uint8_t>(speed);
  command->snakeConfig.snakeWidth = static_cast<uint8_t>(snakeWidth);
  command->snakeConfig.snakeColorR = snakeColorR;
  command->snakeConfig.snakeColorG = snakeColorG;
  command->snakeConfig.snakeColorB = snakeColorB;
  command->snakeConfig.foodColorR = foodColorR;
  command->snakeConfig.foodColorG = foodColorG;
  command->snakeConfig.foodColorB = foodColorB;
  command->snakeConfig.font = fontId;
  command->snakeConfig.showSeconds = doc["showSeconds"].as<bool>();
  snprintf(
    command->snakeConfig.snakeSkin,
    sizeof(command->snakeConfig.snakeSkin),
    "%s",
    snakeSkin
  );
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
  } else if (mode == ModeTags::TRANSFERRING) {
    command->targetMode = MODE_TRANSFERRING;
    command->businessModeTag = ModeTags::TRANSFERRING;
    command->flag1 = false;
    command->flag2 = false;
    command->successMessage = "entered transferring mode";
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
  } else if (mode == ModeTags::MAZE) {
    if (!fillMazeModeCommand(command, doc)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "invalid maze mode fields");
      return true;
    }
  } else if (mode == ModeTags::SNAKE) {
    if (!fillSnakeModeCommand(command, doc)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "invalid snake mode fields");
      return true;
    }
  } else if (mode == ModeTags::TETRIS) {
    if (!fillTetrisModeCommand(
          command,
          doc,
          ModeTags::TETRIS,
          "tetris started")) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "invalid tetris mode fields");
      return true;
    }
  } else if (mode == ModeTags::TETRIS_CLOCK) {
    if (!fillTetrisClockModeCommand(command, doc)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "invalid tetris clock mode fields");
      return true;
    }
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
  return wsSendAcceptedResponse(client, response, responseSent);
}
}
