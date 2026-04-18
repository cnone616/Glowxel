#include "websocket_effect_command_dispatch.h"
#include "websocket_effect_common.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "display_manager.h"
#include "game_screensaver_effect.h"
#include "mode_tags.h"
#include "tetris_effect.h"

namespace WebSocketEffectCommandDispatch {
bool handleGameEffectCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_game_screensaver") {
    if (!doc.containsKey("game") || !doc.containsKey("speed")) {
      wsSetErrorResponse(response, "missing game screensaver fields");
      return true;
    }

    GameScreensaverConfig config = ConfigManager::gameScreensaverConfig;
    String game = doc["game"].as<String>();
    if (game == "maze") {
      config.game = GAME_SCREENSAVER_MAZE;
      if (!doc.containsKey("mazeSizeMode")) {
        wsSetErrorResponse(response, "missing mazeSizeMode");
        return true;
      }
      String mazeSizeMode = doc["mazeSizeMode"].as<String>();
      if (mazeSizeMode == "wide") {
        config.mazeSizeMode = GAME_SCREENSAVER_MAZE_WIDE;
      } else if (mazeSizeMode == "dense") {
        config.mazeSizeMode = GAME_SCREENSAVER_MAZE_DENSE;
      } else {
        wsSetErrorResponse(response, "invalid mazeSizeMode");
        return true;
      }
    } else if (game == "snake") {
      config.game = GAME_SCREENSAVER_SNAKE;
      if (!doc.containsKey("snakeWidth")) {
        wsSetErrorResponse(response, "missing snakeWidth");
        return true;
      }
      int snakeWidth = doc["snakeWidth"].as<int>();
      if (snakeWidth < 1 || snakeWidth > 4) {
        wsSetErrorResponse(response, "invalid snakeWidth");
        return true;
      }
      config.snakeWidth = (uint8_t)snakeWidth;
    } else if (game == "ping_pong") {
      config.game = GAME_SCREENSAVER_PING_PONG;
    } else if (game == "tetris_game") {
      config.game = GAME_SCREENSAVER_TETRIS_GAME;
      if (!doc.containsKey("cellSize") || !doc.containsKey("showClock")) {
        wsSetErrorResponse(response, "missing tetris game fields");
        return true;
      }
      int cellSize = doc["cellSize"].as<int>();
      if (cellSize < 1 || cellSize > 3) {
        wsSetErrorResponse(response, "invalid cellSize");
        return true;
      }
      config.cellSize = (uint8_t)cellSize;
      config.showClock = doc["showClock"].as<bool>();
    } else {
      wsSetErrorResponse(response, "invalid game");
      return true;
    }

    int speed = doc["speed"].as<int>();
    if (speed < 1 || speed > 10) {
      wsSetErrorResponse(response, "invalid speed");
      return true;
    }
    config.speed = (uint8_t)speed;

    const DeviceMode previousMode = DisplayManager::currentMode;
    const String previousBusinessModeTag = DisplayManager::currentBusinessModeTag;
    const bool previousGameScreensaverActive = GameScreensaverEffect::isActive();
    const GameScreensaverConfig previousGameScreensaverConfig =
      GameScreensaverEffect::getConfig();

    GameScreensaverEffect::applyConfig(config);
    if (!GameScreensaverEffect::isActive()) {
      if (previousMode == MODE_ANIMATION &&
          previousBusinessModeTag == ModeTags::GAME_SCREENSAVER &&
          previousGameScreensaverActive) {
        GameScreensaverEffect::applyConfig(previousGameScreensaverConfig);
      }
      wsSetErrorResponse(response, "game screensaver activation failed");
      return true;
    }

    ConfigManager::gameScreensaverConfig = config;
    ConfigManager::saveGameScreensaverConfig();
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::GAME_SCREENSAVER;
    DisplayManager::lastBusinessModeTag = ModeTags::GAME_SCREENSAVER;
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    BoardNativeEffect::deactivate();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    DisplayManager::setNativeEffectNone();
    GameScreensaverEffect::render();

    response["message"] = "game screensaver applied";
    response["speed"] = config.speed;
    response["game"] = game;
    if (config.game == GAME_SCREENSAVER_SNAKE) {
      response["snakeWidth"] = config.snakeWidth;
    }
    if (config.game == GAME_SCREENSAVER_MAZE) {
      response["mazeSizeMode"] =
        config.mazeSizeMode == GAME_SCREENSAVER_MAZE_DENSE ? "dense" : "wide";
    }
    if (config.game == GAME_SCREENSAVER_TETRIS_GAME) {
      response["cellSize"] = config.cellSize;
      response["showClock"] = config.showClock;
    }
    return true;
  }

  (void)client;
  return false;
}
}
