#include "runtime_status_builder.h"

#include "ambient_preset_codec.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "clock_font_renderer.h"
#include "config_manager.h"
#include "device_mode_tag_codec.h"
#include "display_manager.h"
#include "maze_effect.h"
#include "mode_tags.h"
#include "ota_manager.h"
#include "snake_effect.h"
#include "tetris_clock_effect.h"
#include "tetris_effect.h"
#include "wifi_manager.h"

namespace {
bool isWaterWorldAmbientPreset(uint8_t preset) {
  return preset == AMBIENT_PRESET_WATER_SURFACE ||
         preset == AMBIENT_PRESET_WATER_CURRENT ||
         preset == AMBIENT_PRESET_WATER_CAUSTICS;
}

void fillCompactCommonStatus(JsonDocument& doc) {
  doc["status"] = "ok";
  doc["ip"] = WiFiManager::getDeviceIP();
  doc["width"] = DisplayManager::PANEL_RES_X;
  doc["height"] = DisplayManager::PANEL_RES_Y;
  doc["brightness"] = DisplayManager::currentBrightness;
  doc["mode"] = DeviceModeTagCodec::toTagOrUnknown(DisplayManager::currentMode);
  doc["businessMode"] = DisplayManager::currentBusinessModeTag;
}

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
  doc["effectLoop"] = DisplayManager::ambientEffectConfig.loop;
  if (!isWaterWorldAmbientPreset(DisplayManager::ambientEffectConfig.preset)) {
    doc["effectIntensity"] = DisplayManager::ambientEffectConfig.intensity;
    doc["effectDensity"] = DisplayManager::ambientEffectConfig.density;
  }
  JsonObject effectColor = doc.createNestedObject("effectColor");
  effectColor["r"] = DisplayManager::ambientEffectConfig.colorR;
  effectColor["g"] = DisplayManager::ambientEffectConfig.colorG;
  effectColor["b"] = DisplayManager::ambientEffectConfig.colorB;
}

void fillMazeStatus(JsonDocument& doc) {
  const MazeModeConfig& config = MazeEffect::getConfig();
  doc["effectMode"] = ModeTags::MAZE;
  doc["speed"] = config.speed;
  doc["mazeSizeMode"] = config.mazeSizeMode == MAZE_SIZE_DENSE ? "dense" : "wide";
  doc["showClock"] = config.showClock;
  doc["panelBgColor"] = config.panelBgColor;
  doc["borderColor"] = config.borderColor;
  doc["timeColor"] = config.timeColor;
  doc["dateColor"] = config.dateColor;
  doc["generationPathColor"] = config.generationPathColor;
  doc["searchVisitedColor"] = config.searchVisitedColor;
  doc["searchFrontierColor"] = config.searchFrontierColor;
  doc["solvedPathStartColor"] = config.solvedPathStartColor;
  doc["solvedPathEndColor"] = config.solvedPathEndColor;
}

void fillSnakeStatus(JsonDocument& doc) {
  const SnakeModeConfig& config = SnakeEffect::getConfig();
  doc["effectMode"] = ModeTags::SNAKE;
  doc["speed"] = config.speed;
  doc["snakeWidth"] = config.snakeWidth;
  doc["font"] = clockFontNameFromId(config.font);
  doc["showSeconds"] = config.showSeconds;
  doc["snakeSkin"] = config.snakeSkin;
  JsonObject snakeColor = doc.createNestedObject("snakeColor");
  snakeColor["r"] = config.snakeColorR;
  snakeColor["g"] = config.snakeColorG;
  snakeColor["b"] = config.snakeColorB;
  JsonObject foodColor = doc.createNestedObject("foodColor");
  foodColor["r"] = config.foodColorR;
  foodColor["g"] = config.foodColorG;
  foodColor["b"] = config.foodColorB;
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
    doc["planetX"] = config.planetX;
    doc["planetY"] = config.planetY;
    doc["font"] = clockFontNameFromId(config.font);
    doc["showSeconds"] = config.showSeconds;
    JsonObject time = doc["time"].to<JsonObject>();
    time["show"] = config.time.show;
    time["fontSize"] = config.time.fontSize;
    time["x"] = config.time.x;
    time["y"] = config.time.y;
    JsonObject color = time["color"].to<JsonObject>();
    color["r"] = config.time.r;
    color["g"] = config.time.g;
    color["b"] = config.time.b;
  }
}

void fillCompactPlanetScreensaverStatus(JsonDocument& doc) {
  const PlanetScreensaverNativeConfig& config =
    BoardNativeEffect::getPlanetScreensaverConfig();
  doc["effectMode"] = ModeTags::PLANET_SCREENSAVER;
  doc["preset"] = config.preset;
  doc["size"] = config.size;
  doc["direction"] = config.direction;
  doc["speed"] = config.speed;
  doc["seed"] = config.seed;
  doc["colorSeed"] = config.colorSeed;
  doc["planetX"] = config.planetX;
  doc["planetY"] = config.planetY;
  doc["font"] = clockFontNameFromId(config.font);
  doc["showSeconds"] = config.showSeconds;
  JsonObject time = doc.createNestedObject("time");
  time["show"] = config.time.show;
  time["fontSize"] = config.time.fontSize;
  time["x"] = config.time.x;
  time["y"] = config.time.y;
  JsonObject color = time.createNestedObject("color");
  color["r"] = config.time.r;
  color["g"] = config.time.g;
  color["b"] = config.time.b;
}

void fillCompactEffectStatus(JsonDocument& doc) {
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

  if ((ModeTags::isBoardNativeModeTag(DisplayManager::currentBusinessModeTag) ||
       DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) &&
      BoardNativeEffect::isActive()) {
    if (DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) {
      fillCompactPlanetScreensaverStatus(doc);
      return;
    }

    doc["effectMode"] = DisplayManager::currentBusinessModeTag;
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::MAZE &&
      MazeEffect::isActive()) {
    fillMazeStatus(doc);
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::SNAKE &&
      SnakeEffect::isActive()) {
    fillSnakeStatus(doc);
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::TETRIS &&
      TetrisEffect::isActive) {
    doc["effectMode"] = ModeTags::TETRIS;
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::TETRIS_CLOCK &&
      TetrisClockEffect::isActive) {
    doc["effectMode"] = ModeTags::TETRIS_CLOCK;
    doc["cellSize"] = ConfigManager::tetrisClockConfig.cellSize;
    doc["speed"] = ConfigManager::tetrisClockConfig.speed;
    doc["hourFormat"] = ConfigManager::tetrisClockConfig.hourFormat;
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

  if (DisplayManager::currentBusinessModeTag == ModeTags::MAZE &&
      MazeEffect::isActive()) {
    fillMazeStatus(doc);
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::SNAKE &&
      SnakeEffect::isActive()) {
    fillSnakeStatus(doc);
    return;
  }

  if ((ModeTags::isBoardNativeModeTag(DisplayManager::currentBusinessModeTag) ||
       DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) &&
      BoardNativeEffect::isActive()) {
    fillBoardNativeStatus(doc);
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::TETRIS &&
      TetrisEffect::isActive) {
    doc["effectMode"] = ModeTags::TETRIS;
    return;
  }

  if (DisplayManager::currentBusinessModeTag == ModeTags::TETRIS_CLOCK &&
      TetrisClockEffect::isActive) {
    doc["effectMode"] = ModeTags::TETRIS_CLOCK;
    doc["cellSize"] = ConfigManager::tetrisClockConfig.cellSize;
    doc["speed"] = ConfigManager::tetrisClockConfig.speed;
    doc["hourFormat"] = ConfigManager::tetrisClockConfig.hourFormat;
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

void fillCompactStatus(JsonDocument& doc) {
  fillCompactCommonStatus(doc);
  fillCompactEffectStatus(doc);
}
}
