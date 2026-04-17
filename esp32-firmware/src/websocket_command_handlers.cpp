#include "websocket_command_handlers.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "display_manager.h"
#include "game_screensaver_effect.h"
#include "mode_tags.h"
#include "tetris_effect.h"
#include "ambient_preset_codec.h"

namespace {
  void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
    response["status"] = "error";
    response["message"] = message;
  }

  bool parseRequiredColorObject(
    JsonDocument& doc,
    const char* fieldName,
    uint8_t& outR,
    uint8_t& outG,
    uint8_t& outB
  ) {
    if (!doc.containsKey(fieldName)) {
      return false;
    }
    JsonObject color = doc[fieldName].as<JsonObject>();
    if (!color.containsKey("r") || !color.containsKey("g") || !color.containsKey("b")) {
      return false;
    }
    outR = color["r"].as<uint8_t>();
    outG = color["g"].as<uint8_t>();
    outB = color["b"].as<uint8_t>();
    return true;
  }

  int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) {
      return minValue;
    }
    if (value > maxValue) {
      return maxValue;
    }
    return value;
  }

}

namespace {
  bool ensureColorObject(JsonObject color) {
    return color.containsKey("r") && color.containsKey("g") && color.containsKey("b");
  }
}

bool WebSocketCommandHandlers::handleEffectCommand(
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
      setErrorResponse(response, "missing ambient effect fields");
      return true;
    }

    String presetText = doc["preset"].as<String>();
    uint8_t preset = AMBIENT_PRESET_AURORA;
    if (!AmbientPresetCodec::fromString(presetText, preset)) {
      setErrorResponse(response, "invalid ambient preset");
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
        setErrorResponse(response, "missing rain scene fields");
        return true;
      }
      JsonObject color = doc["color"].as<JsonObject>();
      if (!ensureColorObject(color)) {
        setErrorResponse(response, "missing rain color fields");
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
        setErrorResponse(response, "missing ambient intensity field");
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

  if (cmd == "set_text_display") {
    if (!doc.containsKey("template") ||
        !doc.containsKey("text") ||
        !doc.containsKey("progress") ||
        !doc.containsKey("repeat") ||
        !doc.containsKey("pushIcon") ||
        !doc.containsKey("icon") ||
        !doc.containsKey("speed") ||
        !doc.containsKey("color") ||
        !doc.containsKey("bgColor")) {
      setErrorResponse(response, "missing text display fields");
      return true;
    }

    uint8_t colorR = 0;
    uint8_t colorG = 0;
    uint8_t colorB = 0;
    uint8_t bgColorR = 0;
    uint8_t bgColorG = 0;
    uint8_t bgColorB = 0;
    if (!parseRequiredColorObject(doc, "color", colorR, colorG, colorB) ||
        !parseRequiredColorObject(doc, "bgColor", bgColorR, bgColorG, bgColorB)) {
      setErrorResponse(response, "invalid text display color");
      return true;
    }

    TextDisplayNativeConfig config;
    config.templateName = doc["template"].as<String>();
    config.text = doc["text"].as<String>();
    config.progress = (uint8_t)clampInt(doc["progress"].as<int>(), 0, 100);
    config.repeat = (uint8_t)clampInt(doc["repeat"].as<int>(), 1, 8);
    config.pushIcon = doc["pushIcon"].as<bool>();
    config.icon = doc["icon"].as<String>();
    config.speed = (uint8_t)clampInt(doc["speed"].as<int>(), 1, 10);
    config.colorR = colorR;
    config.colorG = colorG;
    config.colorB = colorB;
    config.bgColorR = bgColorR;
    config.bgColorG = bgColorG;
    config.bgColorB = bgColorB;

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::TEXT_DISPLAY;
    DisplayManager::lastBusinessModeTag = ModeTags::TEXT_DISPLAY;
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    GameScreensaverEffect::init();
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    BoardNativeEffect::applyTextDisplayConfig(config);

    response["message"] = "text display applied";
    return true;
  }

  if (cmd == "set_weather_board") {
    if (!doc.containsKey("weatherType") ||
        !doc.containsKey("city") ||
        !doc.containsKey("temperature") ||
        !doc.containsKey("humidity") ||
        !doc.containsKey("unit")) {
      setErrorResponse(response, "missing weather board fields");
      return true;
    }

    WeatherBoardNativeConfig config;
    config.weatherType = doc["weatherType"].as<String>();
    config.city = doc["city"].as<String>();
    config.temperature = (int16_t)doc["temperature"].as<int>();
    config.humidity = (uint8_t)clampInt(doc["humidity"].as<int>(), 0, 100);
    config.unit = doc["unit"].as<String>();
    if (config.unit != "c" && config.unit != "f") {
      setErrorResponse(response, "invalid weather unit");
      return true;
    }

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::WEATHER;
    DisplayManager::lastBusinessModeTag = ModeTags::WEATHER;
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    GameScreensaverEffect::init();
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    BoardNativeEffect::applyWeatherConfig(config);

    response["message"] = "weather board applied";
    return true;
  }

  if (cmd == "set_countdown_board") {
    if (!doc.containsKey("hours") ||
        !doc.containsKey("minutes") ||
        !doc.containsKey("seconds") ||
        !doc.containsKey("progress")) {
      setErrorResponse(response, "missing countdown board fields");
      return true;
    }

    CountdownBoardNativeConfig config;
    config.hours = (uint8_t)clampInt(doc["hours"].as<int>(), 0, 99);
    config.minutes = (uint8_t)clampInt(doc["minutes"].as<int>(), 0, 59);
    config.seconds = (uint8_t)clampInt(doc["seconds"].as<int>(), 0, 59);
    config.progress = (uint8_t)clampInt(doc["progress"].as<int>(), 0, 100);

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::COUNTDOWN;
    DisplayManager::lastBusinessModeTag = ModeTags::COUNTDOWN;
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    GameScreensaverEffect::init();
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    BoardNativeEffect::applyCountdownConfig(config);

    response["message"] = "countdown board applied";
    return true;
  }

  if (cmd == "set_stopwatch_board") {
    if (!doc.containsKey("previewSeconds") ||
        !doc.containsKey("lapCount") ||
        !doc.containsKey("showMilliseconds")) {
      setErrorResponse(response, "missing stopwatch board fields");
      return true;
    }

    StopwatchBoardNativeConfig config;
    config.previewSeconds = (uint16_t)clampInt(doc["previewSeconds"].as<int>(), 0, 35999);
    config.lapCount = (uint8_t)clampInt(doc["lapCount"].as<int>(), 0, 99);
    config.showMilliseconds = doc["showMilliseconds"].as<bool>();

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::STOPWATCH;
    DisplayManager::lastBusinessModeTag = ModeTags::STOPWATCH;
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    GameScreensaverEffect::init();
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    BoardNativeEffect::applyStopwatchConfig(config);

    response["message"] = "stopwatch board applied";
    return true;
  }

  if (cmd == "set_notification_board") {
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
      setErrorResponse(response, "missing notification board fields");
      return true;
    }

    uint8_t accentR = 0;
    uint8_t accentG = 0;
    uint8_t accentB = 0;
    if (!parseRequiredColorObject(doc, "accentColor", accentR, accentG, accentB)) {
      setErrorResponse(response, "invalid notification accent color");
      return true;
    }

    NotificationBoardNativeConfig config;
    config.repeatMode = doc["repeatMode"].as<String>();
    config.text = doc["text"].as<String>();
    config.icon = doc["icon"].as<String>();
    config.accentR = accentR;
    config.accentG = accentG;
    config.accentB = accentB;
    config.contentType = doc["contentType"].as<String>();
    config.textTemplate = doc["textTemplate"].as<String>();
    config.staticTemplate = doc["staticTemplate"].as<String>();
    config.animationTemplate = doc["animationTemplate"].as<String>();
    config.hour = (uint8_t)clampInt(doc["hour"].as<int>(), 0, 23);
    config.minute = (uint8_t)clampInt(doc["minute"].as<int>(), 0, 59);

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::NOTIFICATION;
    DisplayManager::lastBusinessModeTag = ModeTags::NOTIFICATION;
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    GameScreensaverEffect::init();
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    BoardNativeEffect::applyNotificationConfig(config);

    response["message"] = "notification board applied";
    return true;
  }

  if (cmd == "set_planet_screensaver") {
    if (!doc.containsKey("preset") ||
        !doc.containsKey("size") ||
        !doc.containsKey("direction") ||
        !doc.containsKey("speed") ||
        !doc.containsKey("seed") ||
        !doc.containsKey("colorSeed")) {
      setErrorResponse(response, "missing planet screensaver fields");
      return true;
    }

    String preset = doc["preset"].as<String>();
    if (preset != "terran_wet" &&
        preset != "terran_dry" &&
        preset != "islands" &&
        preset != "no_atmosphere" &&
        preset != "gas_giant_1" &&
        preset != "gas_giant_2" &&
        preset != "ice_world" &&
        preset != "lava_world" &&
        preset != "asteroid" &&
        preset != "black_hole" &&
        preset != "galaxy" &&
        preset != "star") {
      setErrorResponse(response, "invalid planet preset");
      return true;
    }

    String size = doc["size"].as<String>();
    if (size != "small" && size != "medium" && size != "large") {
      setErrorResponse(response, "invalid planet size");
      return true;
    }

    String direction = doc["direction"].as<String>();
    if (direction != "left" && direction != "right") {
      setErrorResponse(response, "invalid planet direction");
      return true;
    }

    int speed = doc["speed"].as<int>();
    if (speed < 1 || speed > 10) {
      setErrorResponse(response, "invalid planet speed");
      return true;
    }

    unsigned long seed = doc["seed"].as<unsigned long>();
    if (seed > 999999999UL) {
      setErrorResponse(response, "invalid planet seed");
      return true;
    }

    unsigned long colorSeed = doc["colorSeed"].as<unsigned long>();
    if (colorSeed > 999999999UL) {
      setErrorResponse(response, "invalid planet colorSeed");
      return true;
    }

    PlanetScreensaverNativeConfig config;
    config.preset = preset;
    config.size = size;
    config.direction = direction;
    config.speed = (uint8_t)speed;
    config.seed = (uint32_t)seed;
    config.colorSeed = (uint32_t)colorSeed;

    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
    DisplayManager::currentBusinessModeTag = ModeTags::PLANET_SCREENSAVER;
    DisplayManager::lastBusinessModeTag = ModeTags::PLANET_SCREENSAVER;
    ConfigManager::saveClockConfig();
    TetrisEffect::isActive = false;
    GameScreensaverEffect::init();
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    BoardNativeEffect::applyPlanetScreensaverConfig(config);
    BoardNativeEffect::render();

    response["message"] = "planet screensaver applied";
    response["preset"] = config.preset;
    response["size"] = config.size;
    response["direction"] = config.direction;
    response["speed"] = config.speed;
    response["seed"] = config.seed;
    response["colorSeed"] = config.colorSeed;
    return true;
  }

  if (cmd == "set_game_screensaver") {
    if (!doc.containsKey("game") || !doc.containsKey("speed")) {
      setErrorResponse(response, "missing game screensaver fields");
      return true;
    }

    GameScreensaverConfig config = ConfigManager::gameScreensaverConfig;
    String game = doc["game"].as<String>();
    if (game == "maze") {
      config.game = GAME_SCREENSAVER_MAZE;
      if (!doc.containsKey("mazeSizeMode")) {
        setErrorResponse(response, "missing mazeSizeMode");
        return true;
      }
      String mazeSizeMode = doc["mazeSizeMode"].as<String>();
      if (mazeSizeMode == "wide") {
        config.mazeSizeMode = GAME_SCREENSAVER_MAZE_WIDE;
      } else if (mazeSizeMode == "dense") {
        config.mazeSizeMode = GAME_SCREENSAVER_MAZE_DENSE;
      } else {
        setErrorResponse(response, "invalid mazeSizeMode");
        return true;
      }
    } else if (game == "snake") {
      config.game = GAME_SCREENSAVER_SNAKE;
      if (!doc.containsKey("snakeWidth")) {
        setErrorResponse(response, "missing snakeWidth");
        return true;
      }
      int snakeWidth = doc["snakeWidth"].as<int>();
      if (snakeWidth < 1 || snakeWidth > 4) {
        setErrorResponse(response, "invalid snakeWidth");
        return true;
      }
      config.snakeWidth = (uint8_t)snakeWidth;
    } else if (game == "ping_pong") {
      config.game = GAME_SCREENSAVER_PING_PONG;
    } else if (game == "tetris_game") {
      config.game = GAME_SCREENSAVER_TETRIS_GAME;
      if (!doc.containsKey("cellSize") || !doc.containsKey("showClock")) {
        setErrorResponse(response, "missing tetris game fields");
        return true;
      }
      int cellSize = doc["cellSize"].as<int>();
      if (cellSize < 1 || cellSize > 3) {
        setErrorResponse(response, "invalid cellSize");
        return true;
      }
      config.cellSize = (uint8_t)cellSize;
      config.showClock = doc["showClock"].as<bool>();
    } else {
      setErrorResponse(response, "invalid game");
      return true;
    }

    int speed = doc["speed"].as<int>();
    if (speed < 1 || speed > 10) {
      setErrorResponse(response, "invalid speed");
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
      setErrorResponse(response, "game screensaver activation failed");
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
