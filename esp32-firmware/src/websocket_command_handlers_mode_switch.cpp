#include "websocket_mode_command_dispatch.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "device_mode_tag_codec.h"
#include "display_manager.h"
#include "game_screensaver_effect.h"
#include "mode_tags.h"
#include "tetris_effect.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}
}

namespace WebSocketModeCommandDispatch {
bool handleModeSwitchCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  auto modeToString = [](DeviceMode mode) {
    return DeviceModeTagCodec::toTagOrUnknown(mode);
  };

  String cmd = doc["cmd"].as<String>();
  if (cmd != "set_mode") {
    return false;
  }

  String mode = doc["mode"].as<String>();
  DeviceMode fromMode = DisplayManager::currentMode;
  auto playLoadedAnimation = []() {
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = true;
      AnimationManager::currentGIF->currentFrame = 0;
      AnimationManager::currentGIF->lastFrameTime = millis();
      AnimationManager::renderGIFFrame(0);
      return true;
    }
    return false;
  };
  auto setAnimationBusinessMode = [&](const String& modeTag, const char* message, bool fallbackToClock) {
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = modeTag;
    DisplayManager::lastBusinessModeTag = modeTag;
    ConfigManager::saveClockConfig();

    bool boardMode = modeTag == ModeTags::TEXT_DISPLAY ||
                     modeTag == ModeTags::WEATHER ||
                     modeTag == ModeTags::COUNTDOWN ||
                     modeTag == ModeTags::STOPWATCH ||
                     modeTag == ModeTags::NOTIFICATION ||
                     modeTag == ModeTags::PLANET_SCREENSAVER;
    if (boardMode && BoardNativeEffect::isActive()) {
      BoardNativeEffect::render();
    } else if (!playLoadedAnimation()) {
      if (fallbackToClock) {
        DisplayManager::displayClock(true);
      } else {
        DisplayManager::dma_display->clearScreen();
      }
    }

    Serial.printf("模式切换: %s -> %s\n", modeToString(fromMode), modeTag.c_str());
    response["message"] = message;
    return true;
  };

  bool keepBoardState = mode == ModeTags::TEXT_DISPLAY ||
                        mode == ModeTags::WEATHER ||
                        mode == ModeTags::COUNTDOWN ||
                        mode == ModeTags::STOPWATCH ||
                        mode == ModeTags::NOTIFICATION ||
                        mode == ModeTags::PLANET_SCREENSAVER;

  TetrisEffect::isActive = false;
  GameScreensaverEffect::init();
  if (!keepBoardState) {
    BoardNativeEffect::deactivate();
  }
  DisplayManager::setNativeEffectNone();
  if (AnimationManager::currentGIF != nullptr) {
    AnimationManager::currentGIF->isPlaying = false;
  }
  DisplayManager::receivingPixels = false;
  DisplayManager::dma_display->clearScreen();

  if (mode == ModeTags::CLOCK) {
    DisplayManager::currentMode = MODE_CLOCK;
    DisplayManager::lastBusinessMode = MODE_CLOCK;
    DisplayManager::currentBusinessModeTag = ModeTags::CLOCK;
    DisplayManager::lastBusinessModeTag = ModeTags::CLOCK;
    ConfigManager::saveClockConfig();
    DisplayManager::displayClock(true);
    Serial.printf("模式切换: %s -> clock\n", modeToString(fromMode));
    response["message"] = "switched to static clock mode";
    return true;
  }

  if (mode == ModeTags::CANVAS) {
    DisplayManager::currentMode = MODE_CANVAS;
    DisplayManager::currentBusinessModeTag = ModeTags::CANVAS;
    ConfigManager::saveClockConfig();
    DisplayManager::renderCanvas();
    Serial.printf("模式切换: %s -> canvas\n", modeToString(fromMode));
    response["message"] = "switched to canvas mode";
    return true;
  }

  if (mode == ModeTags::ANIMATION) {
    return setAnimationBusinessMode(ModeTags::ANIMATION, "switched to animation mode", true);
  }

  if (mode == ModeTags::GIF_PLAYER) {
    return setAnimationBusinessMode(ModeTags::GIF_PLAYER, "switched to gif player mode", true);
  }

  if (mode == ModeTags::THEME) {
    DisplayManager::currentMode = MODE_THEME;
    DisplayManager::lastBusinessMode = MODE_THEME;
    DisplayManager::currentBusinessModeTag = ModeTags::THEME;
    DisplayManager::lastBusinessModeTag = ModeTags::THEME;
    ConfigManager::saveClockConfig();
    DisplayManager::displayTheme(true);
    Serial.printf("模式切换: %s -> theme\n", modeToString(fromMode));
    response["message"] = "switched to theme mode";
    return true;
  }

  if (mode == ModeTags::TEXT_DISPLAY) {
    return setAnimationBusinessMode(ModeTags::TEXT_DISPLAY, "switched to text display mode", true);
  }

  if (mode == ModeTags::BREATH_EFFECT) {
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::BREATH_EFFECT;
    DisplayManager::lastBusinessModeTag = ModeTags::BREATH_EFFECT;
    ConfigManager::saveClockConfig();
    DisplayManager::activateBreathEffect(DisplayManager::breathEffectConfig);

    Serial.printf("模式切换: %s -> breath_effect\n", modeToString(fromMode));
    response["message"] = "switched to breath effect mode";
    return true;
  }

  if (mode == ModeTags::RHYTHM_EFFECT) {
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::RHYTHM_EFFECT;
    DisplayManager::lastBusinessModeTag = ModeTags::RHYTHM_EFFECT;
    ConfigManager::saveClockConfig();
    DisplayManager::activateRhythmEffect(DisplayManager::rhythmEffectConfig);

    Serial.printf("模式切换: %s -> rhythm_effect\n", modeToString(fromMode));
    response["message"] = "switched to rhythm effect mode";
    return true;
  }

  if (mode == ModeTags::AMBIENT_EFFECT) {
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::AMBIENT_EFFECT;
    DisplayManager::lastBusinessModeTag = ModeTags::AMBIENT_EFFECT;
    ConfigManager::saveClockConfig();
    DisplayManager::activateAmbientEffect(DisplayManager::ambientEffectConfig);

    Serial.printf("模式切换: %s -> ambient_effect\n", modeToString(fromMode));
    response["message"] = "switched to ambient effect mode";
    return true;
  }

  if (mode == ModeTags::LED_MATRIX_SHOWCASE) {
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::LED_MATRIX_SHOWCASE;
    DisplayManager::lastBusinessModeTag = ModeTags::LED_MATRIX_SHOWCASE;
    ConfigManager::saveClockConfig();
    DisplayManager::activateAmbientEffect(DisplayManager::ambientEffectConfig);

    Serial.printf("模式切换: %s -> led_matrix_showcase\n", modeToString(fromMode));
    response["message"] = "switched to led matrix showcase mode";
    return true;
  }

  if (mode == ModeTags::TRANSFERRING) {
    DisplayManager::currentMode = MODE_TRANSFERRING;
    DisplayManager::currentBusinessModeTag = ModeTags::TRANSFERRING;
    DisplayManager::dma_display->clearScreen();
    Serial.printf("模式切换: %s -> transferring\n", modeToString(fromMode));
    Serial.println("进入传输模式，准备接收动画数据");
    response["message"] = "entered transferring mode";
    return true;
  }

  if (mode == ModeTags::TETRIS) {
    if (!doc.containsKey("clearMode") ||
        !doc.containsKey("cellSize") ||
        !doc.containsKey("speed") ||
        !doc.containsKey("showClock")) {
      setErrorResponse(response, "missing tetris mode fields");
      return true;
    }

    bool clearMode = doc["clearMode"].as<bool>();
    int cellSz = doc["cellSize"].as<int>();
    int speed = doc["speed"].as<int>();
    bool clock = doc["showClock"].as<bool>();
    uint8_t mask = 0x7F;

    if (doc.containsKey("pieces")) {
      mask = 0;
      JsonArray arr = doc["pieces"].as<JsonArray>();
      for (JsonVariant v : arr) {
        int idx = v.as<int>();
        if (idx >= 0 && idx < 7) {
          mask |= (1 << idx);
        }
      }
      if (mask == 0) {
        mask = 0x7F;
      }
    }

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::TETRIS;
    DisplayManager::lastBusinessModeTag = ModeTags::TETRIS;
    TetrisEffect::init(clearMode, cellSz, speed, clock, mask);
    Serial.printf("模式切换: %s -> tetris\n", modeToString(fromMode));
    response["message"] = "tetris started";
    return true;
  }

  if (mode == ModeTags::WEATHER) {
    return setAnimationBusinessMode(ModeTags::WEATHER, "switched to weather mode", false);
  }

  if (mode == ModeTags::COUNTDOWN) {
    return setAnimationBusinessMode(ModeTags::COUNTDOWN, "switched to countdown mode", false);
  }

  if (mode == ModeTags::STOPWATCH) {
    return setAnimationBusinessMode(ModeTags::STOPWATCH, "switched to stopwatch mode", false);
  }

  if (mode == ModeTags::NOTIFICATION) {
    return setAnimationBusinessMode(ModeTags::NOTIFICATION, "switched to notification mode", false);
  }

  if (mode == ModeTags::PLANET_SCREENSAVER) {
    return setAnimationBusinessMode(ModeTags::PLANET_SCREENSAVER, "switched to planet screensaver mode", false);
  }

  if (mode == ModeTags::EYES) {
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::EYES;
    DisplayManager::lastBusinessModeTag = ModeTags::EYES;
    ConfigManager::saveClockConfig();
    DisplayManager::activateEyesEffect(ConfigManager::eyesConfig);

    Serial.printf("模式切换: %s -> eyes\n", modeToString(fromMode));
    response["message"] = "switched to eyes mode";
    return true;
  }

  setErrorResponse(response, "invalid mode");
  return true;
}
}
