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
bool handleBoardEffectCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  String cmd = doc["cmd"].as<String>();

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

    TextDisplayNativeConfig config;
    config.templateName = doc["template"].as<String>();
    config.text = doc["text"].as<String>();
    config.progress = (uint8_t)wsClampInt(doc["progress"].as<int>(), 0, 100);
    config.repeat = (uint8_t)wsClampInt(doc["repeat"].as<int>(), 1, 8);
    config.pushIcon = doc["pushIcon"].as<bool>();
    config.icon = doc["icon"].as<String>();
    config.speed = (uint8_t)wsClampInt(doc["speed"].as<int>(), 1, 10);
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
      wsSetErrorResponse(response, "missing weather board fields");
      return true;
    }

    WeatherBoardNativeConfig config;
    config.weatherType = doc["weatherType"].as<String>();
    config.city = doc["city"].as<String>();
    config.temperature = (int16_t)doc["temperature"].as<int>();
    config.humidity = (uint8_t)wsClampInt(doc["humidity"].as<int>(), 0, 100);
    config.unit = doc["unit"].as<String>();
    if (config.unit != "c" && config.unit != "f") {
      wsSetErrorResponse(response, "invalid weather unit");
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
      wsSetErrorResponse(response, "missing countdown board fields");
      return true;
    }

    CountdownBoardNativeConfig config;
    config.hours = (uint8_t)wsClampInt(doc["hours"].as<int>(), 0, 99);
    config.minutes = (uint8_t)wsClampInt(doc["minutes"].as<int>(), 0, 59);
    config.seconds = (uint8_t)wsClampInt(doc["seconds"].as<int>(), 0, 59);
    config.progress = (uint8_t)wsClampInt(doc["progress"].as<int>(), 0, 100);

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
      wsSetErrorResponse(response, "missing stopwatch board fields");
      return true;
    }

    StopwatchBoardNativeConfig config;
    config.previewSeconds = (uint16_t)wsClampInt(doc["previewSeconds"].as<int>(), 0, 35999);
    config.lapCount = (uint8_t)wsClampInt(doc["lapCount"].as<int>(), 0, 99);
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
      wsSetErrorResponse(response, "missing notification board fields");
      return true;
    }

    uint8_t accentR = 0;
    uint8_t accentG = 0;
    uint8_t accentB = 0;
    if (!wsParseRequiredColorObject(doc, "accentColor", accentR, accentG, accentB)) {
      wsSetErrorResponse(response, "invalid notification accent color");
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
    config.hour = (uint8_t)wsClampInt(doc["hour"].as<int>(), 0, 23);
    config.minute = (uint8_t)wsClampInt(doc["minute"].as<int>(), 0, 59);

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
      wsSetErrorResponse(response, "missing planet screensaver fields");
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
      wsSetErrorResponse(response, "invalid planet preset");
      return true;
    }

    String size = doc["size"].as<String>();
    if (size != "small" && size != "medium" && size != "large") {
      wsSetErrorResponse(response, "invalid planet size");
      return true;
    }

    String direction = doc["direction"].as<String>();
    if (direction != "left" && direction != "right") {
      wsSetErrorResponse(response, "invalid planet direction");
      return true;
    }

    int speed = doc["speed"].as<int>();
    if (speed < 1 || speed > 10) {
      wsSetErrorResponse(response, "invalid planet speed");
      return true;
    }

    unsigned long seed = doc["seed"].as<unsigned long>();
    if (seed > 999999999UL) {
      wsSetErrorResponse(response, "invalid planet seed");
      return true;
    }

    unsigned long colorSeed = doc["colorSeed"].as<unsigned long>();
    if (colorSeed > 999999999UL) {
      wsSetErrorResponse(response, "invalid planet colorSeed");
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

  (void)client;
  return false;
}
}
