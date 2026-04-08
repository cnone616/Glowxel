#include "websocket_command_handlers.h"
#include "animation_manager.h"
#include "clock_font_renderer.h"
#include "config_manager.h"
#include "display_manager.h"
#include "eyes_effect.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "wifi_manager.h"
#include <string.h>

namespace {
  void setErrorResponse(StaticJsonDocument<512>& response, const char* message) {
    response["status"] = "error";
    response["message"] = message;
  }

  void sendResponse(AsyncWebSocketClient* client, StaticJsonDocument<512>& response) {
    String responseStr;
    serializeJson(response, responseStr);
    client->text(responseStr);
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

  const char* ambientPresetToString(uint8_t preset) {
    if (preset == AMBIENT_PRESET_AURORA) {
      return "digital_rain";
    }
    if (preset == AMBIENT_PRESET_PLASMA) {
      return "starfield";
    }
    if (preset == AMBIENT_PRESET_MATRIX_RAIN) {
      return "neon_tunnel";
    }
    if (preset == AMBIENT_PRESET_FIREFLY_SWARM) {
      return "metablob";
    }
    if (preset == AMBIENT_PRESET_METEOR_SHOWER) {
      return "falling_sand";
    }
    if (preset == AMBIENT_PRESET_OCEAN_CURRENT) {
      return "sparks";
    }
    if (preset == AMBIENT_PRESET_NEON_GRID) {
      return "wave_pattern";
    }
    if (preset == AMBIENT_PRESET_SUNSET_BLUSH) {
      return "rain_scene";
    }
    if (preset == AMBIENT_PRESET_STARFIELD_DRIFT) {
      return "watermelon_plasma";
    }
    if (preset == AMBIENT_PRESET_BOIDS) {
      return "boids";
    }
    if (preset == AMBIENT_PRESET_BOUNCING_LOGO) {
      return "bouncing_logo";
    }
    if (preset == AMBIENT_PRESET_SORTING_VISUALIZER) {
      return "sorting_visualizer";
    }
    if (preset == AMBIENT_PRESET_CLOCK_SCENE) {
      return "clock_scene";
    }
    if (preset == AMBIENT_PRESET_COUNTDOWN_SCENE) {
      return "countdown_scene";
    }
    if (preset == AMBIENT_PRESET_WEATHER_SCENE) {
      return "weather_scene";
    }
    if (preset == AMBIENT_PRESET_GAME_OF_LIFE) {
      return "game_of_life";
    }
    if (preset == AMBIENT_PRESET_JULIA_SET) {
      return "julia_set";
    }
    if (preset == AMBIENT_PRESET_REACTION_DIFFUSION) {
      return "reaction_diffusion";
    }
    return "";
  }

  bool ambientPresetFromString(const String& value, uint8_t& preset) {
    if (value == "digital_rain") {
      preset = AMBIENT_PRESET_AURORA;
      return true;
    }
    if (value == "starfield") {
      preset = AMBIENT_PRESET_PLASMA;
      return true;
    }
    if (value == "neon_tunnel") {
      preset = AMBIENT_PRESET_MATRIX_RAIN;
      return true;
    }
    if (value == "metablob") {
      preset = AMBIENT_PRESET_FIREFLY_SWARM;
      return true;
    }
    if (value == "falling_sand") {
      preset = AMBIENT_PRESET_METEOR_SHOWER;
      return true;
    }
    if (value == "sparks") {
      preset = AMBIENT_PRESET_OCEAN_CURRENT;
      return true;
    }
    if (value == "wave_pattern") {
      preset = AMBIENT_PRESET_NEON_GRID;
      return true;
    }
    if (value == "rain_scene") {
      preset = AMBIENT_PRESET_SUNSET_BLUSH;
      return true;
    }
    if (value == "watermelon_plasma") {
      preset = AMBIENT_PRESET_STARFIELD_DRIFT;
      return true;
    }
    if (value == "boids") {
      preset = AMBIENT_PRESET_BOIDS;
      return true;
    }
    if (value == "bouncing_logo") {
      preset = AMBIENT_PRESET_BOUNCING_LOGO;
      return true;
    }
    if (value == "sorting_visualizer") {
      preset = AMBIENT_PRESET_SORTING_VISUALIZER;
      return true;
    }
    if (value == "clock_scene") {
      preset = AMBIENT_PRESET_CLOCK_SCENE;
      return true;
    }
    if (value == "countdown_scene") {
      preset = AMBIENT_PRESET_COUNTDOWN_SCENE;
      return true;
    }
    if (value == "weather_scene") {
      preset = AMBIENT_PRESET_WEATHER_SCENE;
      return true;
    }
    if (value == "game_of_life") {
      preset = AMBIENT_PRESET_GAME_OF_LIFE;
      return true;
    }
    if (value == "julia_set") {
      preset = AMBIENT_PRESET_JULIA_SET;
      return true;
    }
    if (value == "reaction_diffusion") {
      preset = AMBIENT_PRESET_REACTION_DIFFUSION;
      return true;
    }
    return false;
  }
}

bool WebSocketCommandHandlers::handleBasicCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response,
  const char* currentMode
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "ping") {
    response["message"] = "pong";
    return true;
  }

  if (cmd == "status") {
    response["ip"] = WiFiManager::getDeviceIP();
    response["width"] = DisplayManager::PANEL_RES_X;
    response["height"] = DisplayManager::PANEL_RES_Y;
    response["brightness"] = DisplayManager::currentBrightness;
    response["mode"] = currentMode;
    if (DisplayManager::nativeEffectType == NATIVE_EFFECT_BREATH) {
      response["effectMode"] = "breath_effect";
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_RHYTHM) {
      response["effectMode"] = "rhythm_effect";
    } else if (TetrisEffect::isActive) {
      response["effectMode"] = "tetris";
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_EYES) {
      response["effectMode"] = "eyes";
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_AMBIENT) {
      response["effectMode"] = "ambient_effect";
      response["effectPreset"] = ambientPresetToString(DisplayManager::ambientEffectConfig.preset);
    } else {
      response["effectMode"] = "none";
    }
    if (DisplayManager::currentMode == MODE_ANIMATION && AnimationManager::currentGIF != nullptr) {
      response["animationFrames"] = AnimationManager::currentGIF->frameCount;
      response["animationPlaying"] = AnimationManager::currentGIF->isPlaying;
      response["currentFrame"] = AnimationManager::currentGIF->currentFrame;
    }
    return true;
  }

  if (cmd == "get_info") {
    response["firmware_version"] = FIRMWARE_VERSION;
    response["device_type"] = DEVICE_TYPE;
    response["ip"] = WiFiManager::getDeviceIP();
    response["free_heap"] = ESP.getFreeHeap();
    response["uptime"] = millis() / 1000;
    response["has_update"] = OTAManager::hasNewVersion();
    return true;
  }

  return false;
}

bool WebSocketCommandHandlers::handleModeCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response
) {
  auto modeToString = [](DeviceMode mode) {
    if (mode == MODE_CLOCK) {
      return "clock";
    }
    if (mode == MODE_CANVAS) {
      return "canvas";
    }
    if (mode == MODE_ANIMATION) {
      return "animation";
    }
    if (mode == MODE_TRANSFERRING) {
      return "transferring";
    }
    if (mode == MODE_THEME) {
      return "theme";
    }
    return "unknown";
  };

  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_mode") {
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

      if (!playLoadedAnimation()) {
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

    TetrisEffect::isActive = false;
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    DisplayManager::receivingPixels = false;
    DisplayManager::dma_display->clearScreen();

    if (mode == "clock") {
      DisplayManager::currentMode = MODE_CLOCK;
      DisplayManager::lastBusinessMode = MODE_CLOCK;
      DisplayManager::currentBusinessModeTag = "clock";
      DisplayManager::lastBusinessModeTag = "clock";
      ConfigManager::saveClockConfig();
      DisplayManager::displayClock(true);
      Serial.printf("模式切换: %s -> clock\n", modeToString(fromMode));
      response["message"] = "switched to static clock mode";
      return true;
    }

    if (mode == "canvas") {
      DisplayManager::currentMode = MODE_CANVAS;
      DisplayManager::currentBusinessModeTag = "canvas";
      ConfigManager::saveClockConfig();
      DisplayManager::renderCanvas();
      Serial.printf("模式切换: %s -> canvas\n", modeToString(fromMode));
      response["message"] = "switched to canvas mode";
      return true;
    }

    if (mode == "animation") {
      return setAnimationBusinessMode("animation", "switched to animation mode", true);
    }

    if (mode == "theme") {
      DisplayManager::currentMode = MODE_THEME;
      DisplayManager::lastBusinessMode = MODE_THEME;
      DisplayManager::currentBusinessModeTag = "theme";
      DisplayManager::lastBusinessModeTag = "theme";
      ConfigManager::saveClockConfig();
      DisplayManager::displayTheme(true);
      Serial.printf("模式切换: %s -> theme\n", modeToString(fromMode));
      response["message"] = "switched to theme mode";
      return true;
    }

    if (mode == "text_display") {
      return setAnimationBusinessMode("text_display", "switched to text display mode", true);
    }

    if (mode == "breath_effect") {
      DisplayManager::currentMode = MODE_ANIMATION;
      DisplayManager::lastBusinessMode = MODE_ANIMATION;
      DisplayManager::currentBusinessModeTag = "breath_effect";
      DisplayManager::lastBusinessModeTag = "breath_effect";
      ConfigManager::saveClockConfig();
      DisplayManager::activateBreathEffect(DisplayManager::breathEffectConfig);

      Serial.printf("模式切换: %s -> breath_effect\n", modeToString(fromMode));
      response["message"] = "switched to breath effect mode";
      return true;
    }

    if (mode == "rhythm_effect") {
      DisplayManager::currentMode = MODE_ANIMATION;
      DisplayManager::lastBusinessMode = MODE_ANIMATION;
      DisplayManager::currentBusinessModeTag = "rhythm_effect";
      DisplayManager::lastBusinessModeTag = "rhythm_effect";
      ConfigManager::saveClockConfig();
      DisplayManager::activateRhythmEffect(DisplayManager::rhythmEffectConfig);

      Serial.printf("模式切换: %s -> rhythm_effect\n", modeToString(fromMode));
      response["message"] = "switched to rhythm effect mode";
      return true;
    }

    if (mode == "ambient_effect") {
      DisplayManager::currentMode = MODE_ANIMATION;
      DisplayManager::lastBusinessMode = MODE_ANIMATION;
      DisplayManager::currentBusinessModeTag = "ambient_effect";
      DisplayManager::lastBusinessModeTag = "ambient_effect";
      ConfigManager::saveClockConfig();
      DisplayManager::activateAmbientEffect(DisplayManager::ambientEffectConfig);

      Serial.printf("模式切换: %s -> ambient_effect\n", modeToString(fromMode));
      response["message"] = "switched to ambient effect mode";
      return true;
    }

    if (mode == "transferring") {
      DisplayManager::currentMode = MODE_TRANSFERRING;
      DisplayManager::currentBusinessModeTag = "transferring";
      DisplayManager::dma_display->clearScreen();
      Serial.printf("模式切换: %s -> transferring\n", modeToString(fromMode));
      Serial.println("进入传输模式，准备接收动画数据");
      response["message"] = "entered transferring mode";
      return true;
    }

    if (mode == "tetris") {
      DisplayManager::currentMode = MODE_ANIMATION;
      DisplayManager::lastBusinessMode = MODE_ANIMATION;
      DisplayManager::currentBusinessModeTag = "tetris";
      DisplayManager::lastBusinessModeTag = "tetris";
      bool clearMode = doc["clearMode"] | true;
      int cellSz = doc["cellSize"] | 2;
      int speed = doc["speed"] | 150;
      bool clock = doc["showClock"] | true;
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

      TetrisEffect::init(clearMode, cellSz, speed, clock, mask);
      Serial.printf("模式切换: %s -> tetris\n", modeToString(fromMode));
      response["message"] = "tetris started";
      return true;
    }

    if (mode == "weather") {
      return setAnimationBusinessMode("weather", "switched to weather mode", false);
    }

    if (mode == "countdown") {
      return setAnimationBusinessMode("countdown", "switched to countdown mode", false);
    }

    if (mode == "stopwatch") {
      return setAnimationBusinessMode("stopwatch", "switched to stopwatch mode", false);
    }

    if (mode == "notification") {
      return setAnimationBusinessMode("notification", "switched to notification mode", false);
    }

    if (mode == "eyes") {
      DisplayManager::currentMode = MODE_ANIMATION;
      DisplayManager::lastBusinessMode = MODE_ANIMATION;
      DisplayManager::currentBusinessModeTag = "eyes";
      DisplayManager::lastBusinessModeTag = "eyes";
      ConfigManager::saveClockConfig();
      DisplayManager::activateEyesEffect(ConfigManager::eyesConfig);

      Serial.printf("模式切换: %s -> eyes\n", modeToString(fromMode));
      response["message"] = "switched to eyes mode";
      return true;
    }

    setErrorResponse(response, "invalid mode");
    return true;
  }

  if (cmd == "clear") {
    DisplayManager::dma_display->clearScreen();

    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      Serial.println("清空画布缓冲区");
      DisplayManager::clearCanvas();
    }

    response["message"] = "cleared";
    return true;
  }

  if (cmd == "brightness") {
    int value = doc["value"];
    if (value >= 0 && value <= 255) {
      DisplayManager::setBrightness(value);
      response["brightness"] = value;
    } else {
      setErrorResponse(response, "brightness must be 0-255");
    }
    return true;
  }

  if (cmd == "start_loading") {
    DisplayManager::startLoadingAnimation();
    response["status"] = "ok";
    response["message"] = "loading started";
    return true;
  }

  if (cmd == "stop_loading") {
    DisplayManager::stopLoadingAnimation();
    DisplayManager::dma_display->clearScreen();
    response["status"] = "ok";
    response["message"] = "loading stopped";
    return true;
  }

  if (cmd == "show_loading") {
    DisplayManager::dma_display->clearScreen();

    int x = 12;
    int y = 18;
    int bs = 11;
    int gap = 3;
    int step = bs + gap;

    uint16_t orange = DisplayManager::dma_display->color565(249, 115, 22);
    uint16_t red = DisplayManager::dma_display->color565(239, 68, 68);
    uint16_t yellow = DisplayManager::dma_display->color565(251, 191, 36);
    uint16_t blue = DisplayManager::dma_display->color565(59, 130, 246);

    uint16_t grid[3][3] = {
      { orange, orange, red },
      { orange, yellow, yellow },
      { orange, blue, blue }
    };

    for (int row = 0; row < 3; row++) {
      for (int col = 0; col < 3; col++) {
        uint8_t r = ((grid[row][col] >> 11) & 0x1F) * 255 / 31 * 0.3;
        uint8_t g = ((grid[row][col] >> 5) & 0x3F) * 255 / 63 * 0.3;
        uint8_t b = (grid[row][col] & 0x1F) * 255 / 31 * 0.3;
        uint16_t dimColor = DisplayManager::dma_display->color565(r, g, b);
        DisplayManager::dma_display->fillRect(x + col * step, y + row * step, bs, bs, dimColor);
      }
    }

    int stepIndex = doc["step"] | 0;
    stepIndex = stepIndex % 8;
    int outer[8][2] = {
      {0, 0}, {0, 1}, {0, 2}, {1, 2},
      {2, 2}, {2, 1}, {2, 0}, {1, 0}
    };

    int row = outer[stepIndex][0];
    int col = outer[stepIndex][1];
    DisplayManager::dma_display->fillRect(x + col * step, y + row * step, bs, bs, grid[row][col]);

    response["message"] = "loading displayed";
    return true;
  }

  return false;
}

namespace {
  bool ensureColorObject(JsonObject color) {
    return color.containsKey("r") && color.containsKey("g") && color.containsKey("b");
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
           behavior.containsKey("sleepyAfterMs");
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
           style.containsKey("pupilColor") &&
           style.containsKey("timeColor");
  }
}

bool WebSocketCommandHandlers::handleEffectCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response
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
      setErrorResponse(response, "missing breath effect fields");
      return true;
    }

    JsonObject colorA = doc["colorA"].as<JsonObject>();
    JsonObject colorB = doc["colorB"].as<JsonObject>();
    if (!ensureColorObject(colorA) || !ensureColorObject(colorB)) {
      setErrorResponse(response, "missing breath color fields");
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
      setErrorResponse(response, "invalid breath motion");
      return true;
    }

    String scope = doc["scope"].as<String>();
    if (scope == "single_ring") {
      config.scope = BREATH_SCOPE_SINGLE_RING;
    } else if (scope == "full_screen") {
      config.scope = BREATH_SCOPE_FULL_SCREEN;
    } else {
      setErrorResponse(response, "invalid breath scope");
      return true;
    }

    String colorMode = doc["colorMode"].as<String>();
    if (colorMode == "solid") {
      config.colorMode = BREATH_COLOR_SOLID;
    } else if (colorMode == "gradient") {
      config.colorMode = BREATH_COLOR_GRADIENT;
    } else {
      setErrorResponse(response, "invalid breath color mode");
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
    DisplayManager::currentBusinessModeTag = "breath_effect";
    DisplayManager::lastBusinessModeTag = "breath_effect";
    TetrisEffect::isActive = false;
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
      setErrorResponse(response, "missing rhythm effect fields");
      return true;
    }

    JsonObject colorA = doc["colorA"].as<JsonObject>();
    JsonObject colorB = doc["colorB"].as<JsonObject>();
    if (!ensureColorObject(colorA) || !ensureColorObject(colorB)) {
      setErrorResponse(response, "missing rhythm color fields");
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
      setErrorResponse(response, "invalid rhythm direction");
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
      setErrorResponse(response, "invalid rhythm mode");
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
    DisplayManager::currentBusinessModeTag = "rhythm_effect";
    DisplayManager::lastBusinessModeTag = "rhythm_effect";
    TetrisEffect::isActive = false;
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
        !doc.containsKey("intensity") ||
        !doc.containsKey("loop")) {
      setErrorResponse(response, "missing ambient effect fields");
      return true;
    }

    String presetText = doc["preset"].as<String>();
    uint8_t preset = AMBIENT_PRESET_AURORA;
    if (!ambientPresetFromString(presetText, preset)) {
      setErrorResponse(response, "invalid ambient preset");
      return true;
    }

    AmbientEffectConfig config;
    config.preset = preset;
    config.speed = doc["speed"].as<uint8_t>();
    config.intensity = doc["intensity"].as<uint8_t>();
    config.loop = doc["loop"].as<bool>();
    if (config.speed < 1) config.speed = 1;
    if (config.speed > 10) config.speed = 10;
    if (config.intensity < 10) config.intensity = 10;
    if (config.intensity > 100) config.intensity = 100;

    DisplayManager::ambientEffectConfig = config;
    ConfigManager::saveAmbientEffectConfig();
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = "ambient_effect";
    DisplayManager::lastBusinessModeTag = "ambient_effect";
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    DisplayManager::activateAmbientEffect(config);

    response["message"] = "ambient effect applied";
    response["preset"] = ambientPresetToString(config.preset);
    return true;
  }

  (void)client;
  return false;
}

bool WebSocketCommandHandlers::handleEyesCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response
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
    const char* pupilColor = style["pupilColor"];
    const char* timeColor = style["timeColor"];
    const char* timeFont = time["font"];
    uint8_t timeFontId = 0;
    int timeFontSize = time["fontSize"].as<int>();
    if (!isHexColorText(eyeColor) ||
        !isHexColorText(pupilColor) ||
        !isHexColorText(timeColor)) {
      setErrorResponse(response, "invalid eyes color");
      return true;
    }
    if (timeFont == nullptr || !clockFontIdFromString(timeFont, timeFontId)) {
      setErrorResponse(response, "invalid eyes time font");
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

    nextConfig.interaction.lookHoldMs = interaction["lookHoldMs"].as<uint16_t>();
    nextConfig.interaction.moodHoldMs = interaction["moodHoldMs"].as<uint16_t>();

    nextConfig.time.show = time["show"].as<bool>();
    nextConfig.time.showSeconds = time["showSeconds"].as<bool>();
    nextConfig.time.font = timeFontId;
    nextConfig.time.fontSize = static_cast<uint8_t>(timeFontSize);

    memcpy(nextConfig.style.eyeColor, eyeColor, sizeof(nextConfig.style.eyeColor));
    memcpy(nextConfig.style.pupilColor, pupilColor, sizeof(nextConfig.style.pupilColor));
    memcpy(nextConfig.style.timeColor, timeColor, sizeof(nextConfig.style.timeColor));

    ConfigManager::eyesConfig = nextConfig;
    ConfigManager::saveEyesConfig();

    if (DisplayManager::currentBusinessModeTag == "eyes") {
      DisplayManager::activateEyesEffect(ConfigManager::eyesConfig);
    }

    response["message"] = "eyes config updated";
    return true;
  }

  if (cmd == "eyes_interact") {
    if (!doc.containsKey("action")) {
      setErrorResponse(response, "missing action");
      return true;
    }

    if (DisplayManager::currentBusinessModeTag != "eyes") {
      setErrorResponse(response, "eyes mode required");
      return true;
    }

    const char* action = doc["action"];
    if (!EyesEffect::triggerAction(action)) {
      setErrorResponse(response, "invalid eyes action");
      return true;
    }

    response["message"] = "eyes action applied";
    return true;
  }

  (void)client;
  return false;
}

bool WebSocketCommandHandlers::handleClockCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response
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

  if (strcmp(clockMode, "clock") != 0 && strcmp(clockMode, "animation") != 0) {
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

  ClockConfig& target = (strcmp(clockMode, "animation") == 0)
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

  if (strcmp(clockMode, "animation") == 0) {
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
  StaticJsonDocument<512>& response
) {
  (void)client;

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

  snprintf(
    ConfigManager::themeConfig.themeId,
    sizeof(ConfigManager::themeConfig.themeId),
    "%s",
    themeId
  );
  ConfigManager::saveThemeConfig();
  DisplayManager::currentMode = MODE_THEME;
  DisplayManager::lastBusinessMode = MODE_THEME;
  DisplayManager::currentBusinessModeTag = "theme";
  DisplayManager::lastBusinessModeTag = "theme";
  ConfigManager::saveClockConfig();
  DisplayManager::displayTheme(true);

  response["message"] = "theme config updated";
  return true;
}

bool WebSocketCommandHandlers::handleAnimationCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_gif_animation") {
    JsonVariant animData = doc["animationData"];

    if (animData.isNull()) {
      AnimationManager::freeGIFAnimation();
      response["status"] = "success";
      response["message"] = "animation cleared";
    } else if (AnimationManager::loadGIFAnimation(animData)) {
      response["status"] = "success";
      response["message"] = "compact animation loaded";
      response["frameCount"] = AnimationManager::currentGIF->frameCount;
    } else {
      setErrorResponse(response, "animation load failed");
    }
    return true;
  }

  if (cmd == "animation_begin") {
    int frameCount = doc["frameCount"] | 0;
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::beginAnimation(frameCount)) {
      response["status"] = "success";
      response["message"] = "ready to receive frames";
      response["frameCount"] = frameCount;
    } else {
      setErrorResponse(response, "animation init failed");
    }
    return true;
  }

  if (cmd == "animation_frame") {
    int index = doc["index"] | -1;
    JsonVariant frameData = doc["frame"];
    if (AnimationManager::addFrame(index, frameData)) {
      response["status"] = "success";
      response["message"] = "frame received";
      response["index"] = index;
    } else {
      setErrorResponse(response, "frame load failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_begin") {
    int index = doc["index"] | -1;
    int type = doc["type"] | 1;
    int delay = doc["delay"] | 100;
    int totalPixels = doc["totalPixels"] | 0;
    if (AnimationManager::beginFrame(index, type, delay, totalPixels)) {
      response["status"] = "success";
      response["message"] = "frame initialized";
      response["index"] = index;
    } else {
      setErrorResponse(response, "frame init failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_status") {
    int index = doc["index"] | -1;
    if (AnimationManager::currentGIF != nullptr &&
        index >= 0 &&
        index < AnimationManager::currentGIF->frameCount) {
      AnimationFrame& frame = AnimationManager::currentGIF->frames[index];
      response["status"] = "success";
      response["message"] = "frame status";
      response["index"] = index;
      response["count"] = frame.pixelCount;
      Serial.printf("帧 %d 状态查询: %d 像素已接收\n", index, frame.pixelCount);
    } else {
      setErrorResponse(response, "frame not found");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_chunk") {
    int index = doc["index"] | -1;
    JsonArray pixels = doc["pixels"];
    if (AnimationManager::addFrameChunk(index, pixels)) {
      response["status"] = "success";
      response["message"] = "chunk received";
      response["index"] = index;
    } else {
      setErrorResponse(response, "chunk failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "animation_end") {
    AnimationManager::receivingFrameIndex = -1;
    if (AnimationManager::endAnimation()) {
      DisplayManager::currentMode = MODE_ANIMATION;
      AnimationManager::currentGIF->isPlaying = true;
      AnimationManager::currentGIF->currentFrame = 0;
      AnimationManager::currentGIF->lastFrameTime = millis();
      DisplayManager::dma_display->clearScreen();
      AnimationManager::renderGIFFrame(0);
      AnimationManager::saveAnimation();
      ConfigManager::saveClockConfig();

      response["status"] = "success";
      response["message"] = "animation loaded and playing";
      response["frameCount"] = AnimationManager::currentGIF->frameCount;
    } else {
      setErrorResponse(response, "animation finalize failed");
    }
    return true;
  }

  if (cmd == "control_gif") {
    String action = doc["action"];

    if (AnimationManager::currentGIF == nullptr) {
      setErrorResponse(response, "no animation loaded");
      return true;
    }

    if (action == "play") {
      AnimationManager::playAnimation();
      response["status"] = "success";
      response["message"] = "animation playing";
      return true;
    }

    if (action == "pause") {
      AnimationManager::pauseAnimation();
      response["status"] = "success";
      response["message"] = "animation paused";
      return true;
    }

    if (action == "stop") {
      AnimationManager::stopAnimation();
      response["status"] = "success";
      response["message"] = "animation stopped";
      return true;
    }

    setErrorResponse(response, "unknown action");
    return true;
  }

  (void)client;
  (void)responseSent;
  return false;
}

bool WebSocketCommandHandlers::handleCanvasCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "save_canvas") {
    ConfigManager::saveCanvasPixels();
    response["message"] = "canvas saved";
    return true;
  }

  if (cmd == "load_canvas") {
    ConfigManager::loadCanvasPixels();
    if (DisplayManager::currentMode == MODE_CANVAS) {
      DisplayManager::renderCanvas();
    }
    response["message"] = "canvas loaded";
    response["initialized"] = DisplayManager::canvasInitialized;
    return true;
  }

  if (cmd == "clear_canvas") {
    DisplayManager::clearCanvas();
    ConfigManager::clearCanvasPixels();
    if (DisplayManager::currentMode == MODE_CANVAS) {
      DisplayManager::dma_display->clearScreen();
    }
    response["message"] = "canvas cleared";
    return true;
  }

  if (cmd == "highlight_color") {
    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      bool hasHighlight = doc.containsKey("color") && !doc["color"].isNull();
      uint8_t highlightR = 0;
      uint8_t highlightG = 0;
      uint8_t highlightB = 0;

      if (hasHighlight) {
        JsonObject color = doc["color"];
        highlightR = color["r"] | 0;
        highlightG = color["g"] | 0;
        highlightB = color["b"] | 0;

        if (highlightR == 0 && highlightG == 0 && highlightB == 0) {
          DisplayManager::highlightCanvasColor(0, 0, 0);
        } else {
          DisplayManager::highlightCanvasColor(highlightR, highlightG, highlightB);
        }
      } else {
        DisplayManager::renderCanvas();
      }

      response["message"] = hasHighlight ? "color highlighted" : "highlight cleared";
    } else {
      setErrorResponse(response, "not in canvas mode or canvas not initialized");
    }
    return true;
  }

  if (cmd == "highlight_row") {
    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      int highlightRow = doc["row"] | -1;
      DisplayManager::highlightCanvasRow(highlightRow);
      response["message"] = (highlightRow >= 0) ? "row highlighted" : "highlight cleared";
    } else {
      setErrorResponse(response, "not in canvas mode or canvas not initialized");
    }
    return true;
  }

  if (cmd == "text") {
    String text = doc["text"].as<String>();
    int x = doc["x"] | 0;
    int y = doc["y"] | 0;
    DisplayManager::dma_display->clearScreen();
    DisplayManager::dma_display->setCursor(x, y);
    DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
    DisplayManager::dma_display->print(text);
    response["message"] = "text displayed";
    return true;
  }

  if (cmd == "pixel") {
    int x = doc["x"];
    int y = doc["y"];
    int r = doc["r"] | 255;
    int g = doc["g"] | 255;
    int b = doc["b"] | 255;
    if (x >= 0 && x < DisplayManager::PANEL_RES_X && y >= 0 && y < DisplayManager::PANEL_RES_Y) {
      DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
      response["message"] = "pixel set";
    } else {
      setErrorResponse(response, "pixel out of range");
    }
    return true;
  }

  if (cmd == "image") {
    JsonArray pixels = doc["pixels"];
    int width = doc["width"] | DisplayManager::PANEL_RES_X;
    int height = doc["height"] | DisplayManager::PANEL_RES_Y;

    DisplayManager::dma_display->clearScreen();
    int idx = 0;
    for (int y = 0; y < height && y < DisplayManager::PANEL_RES_Y; y++) {
      for (int x = 0; x < width && x < DisplayManager::PANEL_RES_X; x++) {
        if (idx + 2 < pixels.size()) {
          uint8_t r = pixels[idx++];
          uint8_t g = pixels[idx++];
          uint8_t b = pixels[idx++];
          DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
        }
      }
      if (y % 8 == 0) {
        yield();
      }
    }
    response["message"] = "image displayed";
    return true;
  }

  if (cmd == "image_sparse") {
    JsonArray pixels = doc["pixels"];

    for (JsonVariant pixel : pixels) {
      int x = pixel["x"];
      int y = pixel["y"];
      uint8_t r = pixel["r"];
      uint8_t g = pixel["g"];
      uint8_t b = pixel["b"];

      if (x >= 0 && x < DisplayManager::PANEL_RES_X && y >= 0 && y < DisplayManager::PANEL_RES_Y) {
        DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }

    response["message"] = "sparse pixels drawn";
    response["count"] = pixels.size();
    return true;
  }

  if (cmd == "image_chunk") {
    JsonArray pixels = doc["pixels"];
    int width = doc["width"];
    int offsetX = doc["offsetX"] | 0;
    int offsetY = doc["offsetY"] | 0;
    int height = doc["height"];
    int chunk = doc["chunk"] | 0;
    int total = doc["total"] | 1;

    if (chunk == 0) {
      DisplayManager::dma_display->clearScreen();
    }

    int idx = 0;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        if (idx + 2 < pixels.size()) {
          uint8_t r = pixels[idx++];
          uint8_t g = pixels[idx++];
          uint8_t b = pixels[idx++];

          int drawX = x + offsetX;
          int drawY = y + offsetY;

          if (drawX >= 0 && drawX < DisplayManager::PANEL_RES_X &&
              drawY >= 0 && drawY < DisplayManager::PANEL_RES_Y) {
            DisplayManager::dma_display->drawPixelRGB888(drawX, drawY, r, g, b);
          }
        }
      }
      if (y % 8 == 0) {
        yield();
      }
    }

    response["message"] = "chunk displayed";
    response["chunk"] = chunk;
    response["total"] = total;
    return true;
  }

  (void)client;
  return false;
}

bool WebSocketCommandHandlers::handleOtaCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "ota_check") {
    OTAManager::checkUpdate();
    response["firmware_version"] = FIRMWARE_VERSION;
    response["has_update"] = OTAManager::hasNewVersion();
    if (OTAManager::hasNewVersion()) {
      response["latest_version"] = OTAManager::getLatestVersion();
      response["changelog"] = OTAManager::getChangelog();
      response["is_force"] = OTAManager::isForce;
    }
    return true;
  }

  if (cmd == "ota_update") {
    if (OTAManager::hasNewVersion()) {
      response["message"] = "starting update";
      sendResponse(client, response);
      responseSent = true;
      delay(500);
      OTAManager::startUpdate();
      return true;
    }

    setErrorResponse(response, "no update available");
    return true;
  }

  if (cmd == "ota_set_server") {
    String url = doc["url"].as<String>();
    if (url.length() > 0) {
      OTAManager::serverUrl = url;
      ConfigManager::preferences.begin("ota", false);
      ConfigManager::preferences.putString("server", url);
      ConfigManager::preferences.end();
      response["message"] = "OTA server set";
      response["server"] = url;
    } else {
      setErrorResponse(response, "missing url");
    }
    return true;
  }

  return false;
}
