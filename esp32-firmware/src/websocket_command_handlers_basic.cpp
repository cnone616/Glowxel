#include "websocket_command_handlers.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "display_manager.h"
#include "game_screensaver_effect.h"
#include "device_mode_tag_codec.h"
#include "mode_tags.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "wifi_manager.h"
#include "ambient_preset_codec.h"

bool WebSocketCommandHandlers::handleBasicCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  const char* currentMode
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "ping") {
    response["message"] = "pong";
    return true;
  }

  if (cmd == "status") {
    const char* runtimeMode = DeviceModeTagCodec::toTagOrUnknown(DisplayManager::currentMode);
    response["ip"] = WiFiManager::getDeviceIP();
    response["width"] = DisplayManager::PANEL_RES_X;
    response["height"] = DisplayManager::PANEL_RES_Y;
    response["brightness"] = DisplayManager::currentBrightness;
    response["displayBright"] = ConfigManager::deviceParamsConfig.displayBright;
    response["brightnessDay"] = ConfigManager::deviceParamsConfig.brightnessDay;
    response["brightnessNight"] = ConfigManager::deviceParamsConfig.brightnessNight;
    response["displayRotation"] = ConfigManager::deviceParamsConfig.displayRotation;
    response["swapBlueGreen"] = ConfigManager::deviceParamsConfig.swapBlueGreen;
    response["swapBlueRed"] = ConfigManager::deviceParamsConfig.swapBlueRed;
    response["clkphase"] = ConfigManager::deviceParamsConfig.clkphase;
    response["driver"] = ConfigManager::deviceParamsConfig.driver;
    response["i2cSpeed"] = ConfigManager::deviceParamsConfig.i2cSpeed;
    response["E_pin"] = ConfigManager::deviceParamsConfig.E_pin;
    response["nightStart"] = ConfigManager::deviceParamsConfig.nightStart;
    response["nightEnd"] = ConfigManager::deviceParamsConfig.nightEnd;
    response["ntpServer"] = ConfigManager::deviceParamsConfig.ntpServer;
    response["wifiSsid"] = WiFiManager::getConnectedSSID();
    response["uptime"] = millis() / 1000;
    response["mode"] = runtimeMode;
    response["businessMode"] = DisplayManager::currentBusinessModeTag;
    if (DisplayManager::nativeEffectType == NATIVE_EFFECT_BREATH) {
      response["effectMode"] = ModeTags::BREATH_EFFECT;
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_RHYTHM) {
      response["effectMode"] = ModeTags::RHYTHM_EFFECT;
    } else if (DisplayManager::currentBusinessModeTag == ModeTags::GAME_SCREENSAVER &&
               GameScreensaverEffect::isActive()) {
      response["effectMode"] = ModeTags::GAME_SCREENSAVER;
      const GameScreensaverConfig& config = GameScreensaverEffect::getConfig();
      if (config.game == GAME_SCREENSAVER_MAZE) {
        response["game"] = "maze";
      } else if (config.game == GAME_SCREENSAVER_SNAKE) {
        response["game"] = "snake";
        response["snakeWidth"] = config.snakeWidth;
      } else if (config.game == GAME_SCREENSAVER_PING_PONG) {
        response["game"] = "ping_pong";
      } else if (config.game == GAME_SCREENSAVER_TETRIS_GAME) {
        response["game"] = "tetris_game";
        response["cellSize"] = config.cellSize;
        response["showClock"] = config.showClock;
      }
      response["speed"] = config.speed;
      if (config.game == GAME_SCREENSAVER_MAZE) {
        response["mazeSizeMode"] =
          config.mazeSizeMode == GAME_SCREENSAVER_MAZE_DENSE ? "dense" : "wide";
      }
    } else if (BoardNativeEffect::isActive() &&
               DisplayManager::currentBusinessModeTag == ModeTags::TEXT_DISPLAY) {
      response["effectMode"] = ModeTags::TEXT_DISPLAY;
      const TextDisplayNativeConfig& config = BoardNativeEffect::getTextDisplayConfig();
      response["template"] = config.templateName;
      response["text"] = config.text;
      response["progress"] = config.progress;
      response["repeat"] = config.repeat;
      response["pushIcon"] = config.pushIcon;
      response["icon"] = config.icon;
      response["speed"] = config.speed;
      JsonObject color = response.createNestedObject("color");
      color["r"] = config.colorR;
      color["g"] = config.colorG;
      color["b"] = config.colorB;
      JsonObject bgColor = response.createNestedObject("bgColor");
      bgColor["r"] = config.bgColorR;
      bgColor["g"] = config.bgColorG;
      bgColor["b"] = config.bgColorB;
    } else if (BoardNativeEffect::isActive() &&
               DisplayManager::currentBusinessModeTag == ModeTags::WEATHER) {
      response["effectMode"] = ModeTags::WEATHER;
      const WeatherBoardNativeConfig& config = BoardNativeEffect::getWeatherConfig();
      response["weatherType"] = config.weatherType;
      response["city"] = config.city;
      response["temperature"] = config.temperature;
      response["humidity"] = config.humidity;
      response["unit"] = config.unit;
    } else if (BoardNativeEffect::isActive() &&
               DisplayManager::currentBusinessModeTag == ModeTags::COUNTDOWN) {
      response["effectMode"] = ModeTags::COUNTDOWN;
      const CountdownBoardNativeConfig& config = BoardNativeEffect::getCountdownConfig();
      response["hours"] = config.hours;
      response["minutes"] = config.minutes;
      response["seconds"] = config.seconds;
      response["progress"] = config.progress;
      response["remainingSeconds"] = BoardNativeEffect::getCountdownRemainingSeconds();
    } else if (BoardNativeEffect::isActive() &&
               DisplayManager::currentBusinessModeTag == ModeTags::STOPWATCH) {
      response["effectMode"] = ModeTags::STOPWATCH;
      const StopwatchBoardNativeConfig& config = BoardNativeEffect::getStopwatchConfig();
      response["previewSeconds"] = config.previewSeconds;
      response["lapCount"] = config.lapCount;
      response["showMilliseconds"] = config.showMilliseconds;
      response["elapsedMs"] = BoardNativeEffect::getStopwatchElapsedMs();
    } else if (BoardNativeEffect::isActive() &&
               DisplayManager::currentBusinessModeTag == ModeTags::NOTIFICATION) {
      response["effectMode"] = ModeTags::NOTIFICATION;
      const NotificationBoardNativeConfig& config = BoardNativeEffect::getNotificationConfig();
      response["repeatMode"] = config.repeatMode;
      response["text"] = config.text;
      response["icon"] = config.icon;
      response["contentType"] = config.contentType;
      response["textTemplate"] = config.textTemplate;
      response["staticTemplate"] = config.staticTemplate;
      response["animationTemplate"] = config.animationTemplate;
      response["hour"] = config.hour;
      response["minute"] = config.minute;
      JsonObject accentColor = response.createNestedObject("accentColor");
      accentColor["r"] = config.accentR;
      accentColor["g"] = config.accentG;
      accentColor["b"] = config.accentB;
    } else if (BoardNativeEffect::isActive() &&
               DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) {
      response["effectMode"] = ModeTags::PLANET_SCREENSAVER;
      const PlanetScreensaverNativeConfig& config =
        BoardNativeEffect::getPlanetScreensaverConfig();
      response["preset"] = config.preset;
      response["size"] = config.size;
      response["direction"] = config.direction;
      response["speed"] = config.speed;
      response["seed"] = config.seed;
      response["colorSeed"] = config.colorSeed;
    } else if (TetrisEffect::isActive) {
      response["effectMode"] = ModeTags::TETRIS;
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_EYES) {
      response["effectMode"] = ModeTags::EYES;
    } else if (DisplayManager::nativeEffectType == NATIVE_EFFECT_AMBIENT) {
      response["effectMode"] = ModeTags::AMBIENT_EFFECT;
      response["effectPreset"] = AmbientPresetCodec::toString(DisplayManager::ambientEffectConfig.preset);
      response["effectSpeed"] = DisplayManager::ambientEffectConfig.speed;
      response["effectIntensity"] = DisplayManager::ambientEffectConfig.intensity;
      response["effectDensity"] = DisplayManager::ambientEffectConfig.density;
      JsonObject effectColor = response.createNestedObject("effectColor");
      effectColor["r"] = DisplayManager::ambientEffectConfig.colorR;
      effectColor["g"] = DisplayManager::ambientEffectConfig.colorG;
      effectColor["b"] = DisplayManager::ambientEffectConfig.colorB;
      response["effectLoop"] = DisplayManager::ambientEffectConfig.loop;
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

  (void)client;
  (void)currentMode;
  return false;
}
