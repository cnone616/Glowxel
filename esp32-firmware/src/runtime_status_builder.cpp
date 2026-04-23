#include "runtime_status_builder.h"

#include "ambient_preset_codec.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "device_mode_tag_codec.h"
#include "display_manager.h"
#include "game_screensaver_effect.h"
#include "mode_tags.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "wifi_manager.h"

namespace {
void fillCommonStatus(JsonDocument& doc, bool includeSystemInfo, bool includeFreeHeap) {
  doc["status"] = "ok";
  doc["ip"] = WiFiManager::getDeviceIP();
  doc["width"] = DisplayManager::PANEL_RES_X;
  doc["height"] = DisplayManager::PANEL_RES_Y;
  doc["brightness"] = DisplayManager::currentBrightness;
  doc["displayBright"] = ConfigManager::deviceParamsConfig.displayBright;
  doc["brightnessDay"] = ConfigManager::deviceParamsConfig.brightnessDay;
  doc["brightnessNight"] = ConfigManager::deviceParamsConfig.brightnessNight;
  doc["displayRotation"] = ConfigManager::deviceParamsConfig.displayRotation;
  doc["swapBlueGreen"] = ConfigManager::deviceParamsConfig.swapBlueGreen;
  doc["swapBlueRed"] = ConfigManager::deviceParamsConfig.swapBlueRed;
  doc["clkphase"] = ConfigManager::deviceParamsConfig.clkphase;
  doc["driver"] = ConfigManager::deviceParamsConfig.driver;
  doc["i2cSpeed"] = ConfigManager::deviceParamsConfig.i2cSpeed;
  doc["E_pin"] = ConfigManager::deviceParamsConfig.E_pin;
  doc["nightStart"] = ConfigManager::deviceParamsConfig.nightStart;
  doc["nightEnd"] = ConfigManager::deviceParamsConfig.nightEnd;
  doc["ntpServer"] = ConfigManager::deviceParamsConfig.ntpServer;
  doc["wifiSsid"] = WiFiManager::getConnectedSSID();
  doc["uptime"] = millis() / 1000;
  doc["mode"] = DeviceModeTagCodec::toTagOrUnknown(DisplayManager::currentMode);
  doc["businessMode"] = DisplayManager::currentBusinessModeTag;
  doc["themeId"] = ConfigManager::themeConfig.themeId;

  if (includeSystemInfo) {
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["device_type"] = DEVICE_TYPE;
    doc["has_update"] = OTAManager::hasNewVersion();
  }

  if (includeFreeHeap) {
    doc["freeHeap"] = ESP.getFreeHeap();
  }
}

void fillAmbientEffectStatus(JsonDocument& doc) {
  doc["effectPreset"] = AmbientPresetCodec::toString(DisplayManager::ambientEffectConfig.preset);
  doc["effectSpeed"] = DisplayManager::ambientEffectConfig.speed;
  doc["effectIntensity"] = DisplayManager::ambientEffectConfig.intensity;
  doc["effectDensity"] = DisplayManager::ambientEffectConfig.density;
  doc["effectLoop"] = DisplayManager::ambientEffectConfig.loop;
  JsonObject effectColor = doc.createNestedObject("effectColor");
  effectColor["r"] = DisplayManager::ambientEffectConfig.colorR;
  effectColor["g"] = DisplayManager::ambientEffectConfig.colorG;
  effectColor["b"] = DisplayManager::ambientEffectConfig.colorB;
}

void fillGameScreensaverStatus(JsonDocument& doc) {
  const GameScreensaverConfig& config = GameScreensaverEffect::getConfig();
  doc["effectMode"] = ModeTags::GAME_SCREENSAVER;
  doc["speed"] = config.speed;

  if (config.game == GAME_SCREENSAVER_MAZE) {
    doc["game"] = "maze";
    doc["mazeSizeMode"] =
      config.mazeSizeMode == GAME_SCREENSAVER_MAZE_DENSE ? "dense" : "wide";
    return;
  }

  if (config.game == GAME_SCREENSAVER_SNAKE) {
    doc["game"] = "snake";
    doc["snakeWidth"] = config.snakeWidth;
    return;
  }

  if (config.game == GAME_SCREENSAVER_PING_PONG) {
    doc["game"] = "ping_pong";
    return;
  }

  if (config.game == GAME_SCREENSAVER_TETRIS_GAME) {
    doc["game"] = "tetris_game";
    doc["cellSize"] = config.cellSize;
    doc["showClock"] = config.showClock;
  }
}

void fillBoardNativeStatus(JsonDocument& doc) {
  if (DisplayManager::currentBusinessModeTag == ModeTags::TEXT_DISPLAY) {
    const TextDisplayNativeConfig& config = BoardNativeEffect::getTextDisplayConfig();
    doc["effectMode"] = ModeTags::TEXT_DISPLAY;
    doc["template"] = config.templateName;
    doc["text"] = config.text;
    doc["progress"] = config.progress;
    doc["repeat"] = config.repeat;
    doc["pushIcon"] = config.pushIcon;
    doc["icon"] = config.icon;
    doc["speed"] = config.speed;
    JsonObject color = doc.createNestedObject("color");
    color["r"] = config.colorR;
    color["g"] = config.colorG;
    color["b"] = config.colorB;
    JsonObject bgColor = doc.createNestedObject("bgColor");
    bgColor["r"] = config.bgColorR;
    bgColor["g"] = config.bgColorG;
    bgColor["b"] = config.bgColorB;
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::WEATHER) {
    const WeatherBoardNativeConfig& config = BoardNativeEffect::getWeatherConfig();
    doc["effectMode"] = ModeTags::WEATHER;
    doc["weatherType"] = config.weatherType;
    doc["city"] = config.city;
    doc["temperature"] = config.temperature;
    doc["humidity"] = config.humidity;
    doc["unit"] = config.unit;
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::COUNTDOWN) {
    const CountdownBoardNativeConfig& config = BoardNativeEffect::getCountdownConfig();
    doc["effectMode"] = ModeTags::COUNTDOWN;
    doc["hours"] = config.hours;
    doc["minutes"] = config.minutes;
    doc["seconds"] = config.seconds;
    doc["progress"] = config.progress;
    doc["remainingSeconds"] = BoardNativeEffect::getCountdownRemainingSeconds();
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::STOPWATCH) {
    const StopwatchBoardNativeConfig& config = BoardNativeEffect::getStopwatchConfig();
    doc["effectMode"] = ModeTags::STOPWATCH;
    doc["previewSeconds"] = config.previewSeconds;
    doc["lapCount"] = config.lapCount;
    doc["showMilliseconds"] = config.showMilliseconds;
    doc["elapsedMs"] = BoardNativeEffect::getStopwatchElapsedMs();
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::NOTIFICATION) {
    const NotificationBoardNativeConfig& config = BoardNativeEffect::getNotificationConfig();
    doc["effectMode"] = ModeTags::NOTIFICATION;
    doc["repeatMode"] = config.repeatMode;
    doc["text"] = config.text;
    doc["icon"] = config.icon;
    doc["contentType"] = config.contentType;
    doc["textTemplate"] = config.textTemplate;
    doc["staticTemplate"] = config.staticTemplate;
    doc["animationTemplate"] = config.animationTemplate;
    doc["hour"] = config.hour;
    doc["minute"] = config.minute;
    JsonObject accentColor = doc.createNestedObject("accentColor");
    accentColor["r"] = config.accentR;
    accentColor["g"] = config.accentG;
    accentColor["b"] = config.accentB;
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) {
    const PlanetScreensaverNativeConfig& config =
      BoardNativeEffect::getPlanetScreensaverConfig();
    doc["effectMode"] = ModeTags::PLANET_SCREENSAVER;
    doc["preset"] = config.preset;
    doc["size"] = config.size;
    doc["direction"] = config.direction;
    doc["speed"] = config.speed;
    doc["seed"] = config.seed;
    doc["colorSeed"] = config.colorSeed;
  }
}

void fillEffectStatus(JsonDocument& doc) {
  if (DisplayManager::nativeEffectType == NATIVE_EFFECT_BREATH) {
    doc["effectMode"] = ModeTags::BREATH_EFFECT;
    return;
  }

  if (DisplayManager::nativeEffectType == NATIVE_EFFECT_RHYTHM) {
    doc["effectMode"] = ModeTags::RHYTHM_EFFECT;
    return;
  }

  if (DisplayManager::nativeEffectType == NATIVE_EFFECT_EYES) {
    doc["effectMode"] = ModeTags::EYES;
    return;
  }

  if (DisplayManager::nativeEffectType == NATIVE_EFFECT_AMBIENT) {
    doc["effectMode"] = DisplayManager::currentBusinessModeTag;
    fillAmbientEffectStatus(doc);
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::GAME_SCREENSAVER &&
      GameScreensaverEffect::isActive()) {
    fillGameScreensaverStatus(doc);
    return;
  }

  if ((ModeTags::isBoardNativeModeTag(DisplayManager::currentBusinessModeTag) ||
       DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) &&
      BoardNativeEffect::isActive()) {
    fillBoardNativeStatus(doc);
    return;
  }

  if ((DisplayManager::currentBusinessModeTag == ModeTags::TETRIS ||
       DisplayManager::currentBusinessModeTag == ModeTags::TETRIS_CLOCK) &&
      TetrisEffect::isActive) {
    doc["effectMode"] = ModeTags::TETRIS;
    return;
  }

  if ((DisplayManager::currentBusinessModeTag == ModeTags::ANIMATION ||
       DisplayManager::currentBusinessModeTag == ModeTags::GIF_PLAYER) &&
      AnimationManager::currentGIF != nullptr) {
    doc["effectMode"] = DisplayManager::currentBusinessModeTag;
    doc["animationFrames"] = AnimationManager::currentGIF->frameCount;
    doc["animationPlaying"] = AnimationManager::currentGIF->isPlaying;
    doc["currentFrame"] = AnimationManager::currentGIF->currentFrame;
    return;
  }

  doc["effectMode"] = "none";
}
}

namespace RuntimeStatusBuilder {
void fillStatus(JsonDocument& doc, bool includeSystemInfo, bool includeFreeHeap) {
  fillCommonStatus(doc, includeSystemInfo, includeFreeHeap);
  fillEffectStatus(doc);
}
}
