#include "websocket_effect_command_dispatch.h"
#include "websocket_effect_common.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "display_manager.h"
#include "game_screensaver_effect.h"
#include "mode_tags.h"
#include "tetris_effect.h"
#include "ambient_preset_codec.h"

namespace WebSocketEffectCommandDispatch {
bool handleVisualEffectCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_breath_effect") {
    if (!doc.containsKey("speed") ||
        !doc.containsKey("loop") ||
        !doc.containsKey("motion") ||
        !doc.containsKey("scope") ||
        !doc.containsKey("colorMode") ||
        !doc.containsKey("colorA") ||
        !doc.containsKey("colorB")) {
      wsSetErrorResponse(response, "missing breath effect fields");
      return true;
    }

    JsonObject colorA = doc["colorA"].as<JsonObject>();
    JsonObject colorB = doc["colorB"].as<JsonObject>();
    if (!wsEnsureColorObject(colorA) || !wsEnsureColorObject(colorB)) {
      wsSetErrorResponse(response, "missing breath color fields");
      return true;
    }

    BreathEffectConfig config;
    config.speed = doc["speed"].as<uint8_t>();
    config.loop = doc["loop"].as<bool>();
    String motion = doc["motion"].as<String>();
    if (motion == "clockwise") {
      config.motion = BREATH_MOTION_CLOCKWISE;
    } else if (motion == "counterclockwise") {
      config.motion = BREATH_MOTION_COUNTERCLOCKWISE;
    } else if (motion == "inward") {
      config.motion = BREATH_MOTION_INWARD;
    } else if (motion == "outward") {
      config.motion = BREATH_MOTION_OUTWARD;
    } else {
      wsSetErrorResponse(response, "invalid breath motion");
      return true;
    }

    String scope = doc["scope"].as<String>();
    if (scope == "single_ring") {
      config.scope = BREATH_SCOPE_SINGLE_RING;
    } else if (scope == "full_screen") {
      config.scope = BREATH_SCOPE_FULL_SCREEN;
    } else {
      wsSetErrorResponse(response, "invalid breath scope");
      return true;
    }

    String colorMode = doc["colorMode"].as<String>();
    if (colorMode == "solid") {
      config.colorMode = BREATH_COLOR_SOLID;
    } else if (colorMode == "gradient") {
      config.colorMode = BREATH_COLOR_GRADIENT;
    } else {
      wsSetErrorResponse(response, "invalid breath color mode");
      return true;
    }

    config.colorAR = colorA["r"].as<uint8_t>();
    config.colorAG = colorA["g"].as<uint8_t>();
    config.colorAB = colorA["b"].as<uint8_t>();
    config.colorBR = colorB["r"].as<uint8_t>();
    config.colorBG = colorB["g"].as<uint8_t>();
    config.colorBB = colorB["b"].as<uint8_t>();

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::BREATH_EFFECT;
    DisplayManager::lastBusinessModeTag = ModeTags::BREATH_EFFECT;
    TetrisEffect::isActive = false;
    BoardNativeEffect::deactivate();
    GameScreensaverEffect::init();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    DisplayManager::activateBreathEffect(config);

    response["message"] = "breath effect applied";
    return true;
  }

  if (cmd == "set_rhythm_effect") {
    if (!doc.containsKey("bpm") ||
        !doc.containsKey("speed") ||
        !doc.containsKey("loop") ||
        !doc.containsKey("direction") ||
        !doc.containsKey("strength") ||
        !doc.containsKey("mode") ||
        !doc.containsKey("colorA") ||
        !doc.containsKey("colorB")) {
      wsSetErrorResponse(response, "missing rhythm effect fields");
      return true;
    }

    JsonObject colorA = doc["colorA"].as<JsonObject>();
    JsonObject colorB = doc["colorB"].as<JsonObject>();
    if (!wsEnsureColorObject(colorA) || !wsEnsureColorObject(colorB)) {
      wsSetErrorResponse(response, "missing rhythm color fields");
      return true;
    }

    RhythmEffectConfig config;
    config.bpm = doc["bpm"].as<uint16_t>();
    config.speed = doc["speed"].as<uint8_t>();
    config.loop = doc["loop"].as<bool>();
    config.strength = doc["strength"].as<uint8_t>();

    String direction = doc["direction"].as<String>();
    if (direction == "left") {
      config.direction = RHYTHM_DIR_LEFT;
    } else if (direction == "right") {
      config.direction = RHYTHM_DIR_RIGHT;
    } else if (direction == "up") {
      config.direction = RHYTHM_DIR_UP;
    } else if (direction == "down") {
      config.direction = RHYTHM_DIR_DOWN;
    } else {
      wsSetErrorResponse(response, "invalid rhythm direction");
      return true;
    }

    String mode = doc["mode"].as<String>();
    if (mode == "pulse") {
      config.mode = RHYTHM_MODE_PULSE;
    } else if (mode == "gradient") {
      config.mode = RHYTHM_MODE_GRADIENT;
    } else if (mode == "jump") {
      config.mode = RHYTHM_MODE_JUMP;
    } else {
      wsSetErrorResponse(response, "invalid rhythm mode");
      return true;
    }

    config.colorAR = colorA["r"].as<uint8_t>();
    config.colorAG = colorA["g"].as<uint8_t>();
    config.colorAB = colorA["b"].as<uint8_t>();
    config.colorBR = colorB["r"].as<uint8_t>();
    config.colorBG = colorB["g"].as<uint8_t>();
    config.colorBB = colorB["b"].as<uint8_t>();

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::RHYTHM_EFFECT;
    DisplayManager::lastBusinessModeTag = ModeTags::RHYTHM_EFFECT;
    TetrisEffect::isActive = false;
    BoardNativeEffect::deactivate();
    GameScreensaverEffect::init();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    DisplayManager::activateRhythmEffect(config);

    response["message"] = "rhythm effect applied";
    return true;
  }

  if (cmd == "set_ambient_effect") {
    if (!doc.containsKey("preset") ||
        !doc.containsKey("speed") ||
        !doc.containsKey("loop")) {
      wsSetErrorResponse(response, "missing ambient effect fields");
      return true;
    }

    String presetText = doc["preset"].as<String>();
    uint8_t preset = AMBIENT_PRESET_AURORA;
    if (!AmbientPresetCodec::fromString(presetText, preset)) {
      wsSetErrorResponse(response, "invalid ambient preset");
      return true;
    }

    AmbientEffectConfig config;
    config.preset = preset;
    config.speed = doc["speed"].as<uint8_t>();
    config.loop = doc["loop"].as<bool>();
    if (config.speed < 1) config.speed = 1;
    if (config.speed > 10) config.speed = 10;

    if (config.preset == AMBIENT_PRESET_SUNSET_BLUSH) {
      if (!doc.containsKey("density") || !doc.containsKey("color")) {
        wsSetErrorResponse(response, "missing rain scene fields");
        return true;
      }
      JsonObject color = doc["color"].as<JsonObject>();
      if (!wsEnsureColorObject(color)) {
        wsSetErrorResponse(response, "missing rain color fields");
        return true;
      }
      config.intensity = DisplayManager::ambientEffectConfig.intensity;
      config.density = doc["density"].as<uint8_t>();
      if (config.density < 10) config.density = 10;
      if (config.density > 100) config.density = 100;
      config.colorR = color["r"].as<uint8_t>();
      config.colorG = color["g"].as<uint8_t>();
      config.colorB = color["b"].as<uint8_t>();
    } else {
      if (!doc.containsKey("intensity")) {
        wsSetErrorResponse(response, "missing ambient intensity field");
        return true;
      }
      config.intensity = doc["intensity"].as<uint8_t>();
      if (config.intensity < 10) config.intensity = 10;
      if (config.intensity > 100) config.intensity = 100;
      config.density = DisplayManager::ambientEffectConfig.density;
      config.colorR = DisplayManager::ambientEffectConfig.colorR;
      config.colorG = DisplayManager::ambientEffectConfig.colorG;
      config.colorB = DisplayManager::ambientEffectConfig.colorB;
    }

    DisplayManager::ambientEffectConfig = config;
    ConfigManager::saveAmbientEffectConfig();
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::AMBIENT_EFFECT;
    DisplayManager::lastBusinessModeTag = ModeTags::AMBIENT_EFFECT;
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    BoardNativeEffect::deactivate();
    GameScreensaverEffect::init();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    DisplayManager::activateAmbientEffect(config);

    response["message"] = "ambient effect applied";
    response["preset"] = AmbientPresetCodec::toString(config.preset);
    return true;
  }

  (void)client;
  return false;
}
}
