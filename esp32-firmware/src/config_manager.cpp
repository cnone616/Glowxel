#include "config_manager.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "display_manager.h"
#include "device_mode_tag_codec.h"
#include "mode_tags.h"
#include "runtime_mode_coordinator.h"

// 静态成员初始化
Preferences ConfigManager::preferences;

namespace {
MazeModeConfig makeDefaultMazeModeConfig() {
  MazeModeConfig config = {};
  config.speed = 3;
  config.mazeSizeMode = MAZE_SIZE_WIDE;
  config.showClock = true;
  memcpy(config.panelBgColor, "#05070f", sizeof(config.panelBgColor));
  memcpy(config.borderColor, "#182c4c", sizeof(config.borderColor));
  memcpy(config.timeColor, "#ffd400", sizeof(config.timeColor));
  memcpy(config.dateColor, "#ff6464", sizeof(config.dateColor));
  memcpy(config.generationPathColor, "#4f4f55", sizeof(config.generationPathColor));
  memcpy(config.searchVisitedColor, "#ff4444", sizeof(config.searchVisitedColor));
  memcpy(config.searchFrontierColor, "#70ff9c", sizeof(config.searchFrontierColor));
  memcpy(config.solvedPathStartColor, "#1a60ff", sizeof(config.solvedPathStartColor));
  memcpy(config.solvedPathEndColor, "#42bcff", sizeof(config.solvedPathEndColor));
  return config;
}

SnakeModeConfig makeDefaultSnakeModeConfig() {
  SnakeModeConfig config = {};
  config.speed = 6;
  config.snakeWidth = 2;
  config.snakeColorR = 86;
  config.snakeColorG = 214;
  config.snakeColorB = 120;
  config.foodColorR = 255;
  config.foodColorG = 168;
  config.foodColorB = 84;
  config.font = CLOCK_FONT_MINIMAL_3X5;
  config.showSeconds = false;
  memcpy(config.snakeSkin, "gradient", sizeof("gradient"));
  return config;
}

TetrisModeConfig makeDefaultTetrisModeConfig(bool showClock) {
  TetrisModeConfig config = {};
  config.clearMode = true;
  config.cellSize = 2;
  config.speed = 150;
  config.showClock = showClock;
  config.pieces = 0x7F;
  return config;
}

TetrisClockModeConfig makeDefaultTetrisClockModeConfig() {
  TetrisClockModeConfig config = {};
  config.cellSize = 2;
  config.speed = 150;
  config.hourFormat = 24;
  return config;
}

DeviceParamsConfig makeDefaultDeviceParamsConfig() {
  DeviceParamsConfig config = {};
  config.swapBlueGreen = false;
  config.swapBlueRed = false;
  config.clkphase = false;
  config.displayBright = 50;
  config.brightnessDay = 50;
  config.brightnessNight = 50;
  config.displayRotation = 0;
  config.driver = 0;
  config.i2cSpeed = 8000000;
  config.E_pin = 18;
  memcpy(config.nightStart, "22:00", sizeof("22:00"));
  memcpy(config.nightEnd, "07:00", sizeof("07:00"));
  memcpy(config.ntpServer, "ntp2.aliyun.com", sizeof("ntp2.aliyun.com"));
  return config;
}

void readDeviceParamsConfigFromPreferences(DeviceParamsConfig& targetConfig) {
  ConfigManager::preferences.begin("device", true);

  size_t configSize = ConfigManager::preferences.getBytesLength("config");
  if (configSize > 0) {
    DeviceParamsConfig loadedConfig = makeDefaultDeviceParamsConfig();
    size_t bytesToRead = configSize;
    if (bytesToRead > sizeof(DeviceParamsConfig)) {
      bytesToRead = sizeof(DeviceParamsConfig);
    }
    ConfigManager::preferences.getBytes("config", &loadedConfig, bytesToRead);
    loadedConfig.nightStart[sizeof(loadedConfig.nightStart) - 1] = '\0';
    loadedConfig.nightEnd[sizeof(loadedConfig.nightEnd) - 1] = '\0';
    loadedConfig.ntpServer[sizeof(loadedConfig.ntpServer) - 1] = '\0';
    targetConfig = loadedConfig;
    Serial.printf("device params config loaded (%u bytes)\n", (unsigned int)bytesToRead);
  } else {
    targetConfig = makeDefaultDeviceParamsConfig();
    Serial.println("device params config: using default");
  }

  ConfigManager::preferences.end();
}

bool migrateLegacyDefaultClockLayout(ClockConfig& config) {
  if (config.font != CLOCK_FONT_CLASSIC_5X7) {
    return false;
  }
  if (config.showSeconds != false || config.hourFormat != 24) {
    return false;
  }
  if (config.time.show != true ||
      config.time.fontSize != 2 ||
      config.time.x != 22 ||
      config.time.y != 4 ||
      config.time.r != 255 ||
      config.time.g != 255 ||
      config.time.b != 255) {
    return false;
  }
  if (config.date.show != false ||
      config.date.fontSize != 1 ||
      config.date.x != 22 ||
      config.date.y != 10 ||
      config.date.r != 120 ||
      config.date.g != 120 ||
      config.date.b != 120) {
    return false;
  }
  if (config.week.show != false ||
      config.week.x != 26 ||
      config.week.y != 47 ||
      config.week.r != 100 ||
      config.week.g != 100 ||
      config.week.b != 100) {
    return false;
  }
  if (config.image.show != false ||
      config.image.x != 0 ||
      config.image.y != 0 ||
      config.image.width != 64 ||
      config.image.height != 64) {
    return false;
  }

  config.time.fontSize = 1;
  config.time.x = 17;
  config.time.y = 5;
  return true;
}

EyesConfig makeDefaultEyesConfig() {
  EyesConfig defaultEyesConfig = {};
  defaultEyesConfig.layout.eyeY = 24;
  defaultEyesConfig.layout.eyeSpacing = 14;
  defaultEyesConfig.layout.eyeWidth = 16;
  defaultEyesConfig.layout.eyeHeight = 10;
  defaultEyesConfig.layout.timeX = 18;
  defaultEyesConfig.layout.timeY = 5;
  defaultEyesConfig.behavior.autoSwitch = true;
  defaultEyesConfig.behavior.blinkIntervalMs = 3200;
  defaultEyesConfig.behavior.lookIntervalMs = 4200;
  defaultEyesConfig.behavior.idleMove = 2;
  defaultEyesConfig.behavior.sleepyAfterMs = 45000;
  defaultEyesConfig.behavior.expressionRhythm = EyesConfig::EXPRESSION_RHYTHM_STANDARD;
  defaultEyesConfig.interaction.lookHoldMs = 1200;
  defaultEyesConfig.interaction.moodHoldMs = 1800;
  defaultEyesConfig.time.show = true;
  defaultEyesConfig.time.showSeconds = false;
  defaultEyesConfig.time.font = CLOCK_FONT_CLASSIC_5X7;
  defaultEyesConfig.time.fontSize = 1;
  memcpy(defaultEyesConfig.style.eyeColor, "#9bdcff", sizeof(defaultEyesConfig.style.eyeColor));
  memcpy(defaultEyesConfig.style.timeColor, "#64c8ff", sizeof(defaultEyesConfig.style.timeColor));
  return defaultEyesConfig;
}

bool isValidHexColorString(const char* value) {
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

bool isValidMazeModeConfig(const MazeModeConfig& config) {
  return config.speed >= 1 &&
         config.speed <= 10 &&
         (config.mazeSizeMode == MAZE_SIZE_WIDE ||
          config.mazeSizeMode == MAZE_SIZE_DENSE) &&
         isValidHexColorString(config.panelBgColor) &&
         isValidHexColorString(config.borderColor) &&
         isValidHexColorString(config.timeColor) &&
         isValidHexColorString(config.dateColor) &&
         isValidHexColorString(config.generationPathColor) &&
         isValidHexColorString(config.searchVisitedColor) &&
         isValidHexColorString(config.searchFrontierColor) &&
         isValidHexColorString(config.solvedPathStartColor) &&
         isValidHexColorString(config.solvedPathEndColor);
}

bool sanitizeEyesConfig(EyesConfig& config) {
  bool changed = false;
  EyesConfig defaults = makeDefaultEyesConfig();

  if (config.layout.eyeWidth < 6 || config.layout.eyeWidth > 28) {
    config.layout.eyeWidth = defaults.layout.eyeWidth;
    changed = true;
  }
  if (config.layout.eyeHeight < 4 || config.layout.eyeHeight > 24) {
    config.layout.eyeHeight = defaults.layout.eyeHeight;
    changed = true;
  }
  if (config.layout.eyeSpacing < 4 || config.layout.eyeSpacing > 24) {
    config.layout.eyeSpacing = defaults.layout.eyeSpacing;
    changed = true;
  }
  if (config.layout.eyeY < 8 || config.layout.eyeY > 56) {
    config.layout.eyeY = defaults.layout.eyeY;
    changed = true;
  }
  if (config.layout.timeX < 0 || config.layout.timeX > 63) {
    config.layout.timeX = defaults.layout.timeX;
    changed = true;
  }
  if (config.layout.timeY < 0 || config.layout.timeY > 63) {
    config.layout.timeY = defaults.layout.timeY;
    changed = true;
  }

  if (config.behavior.blinkIntervalMs < 500 || config.behavior.blinkIntervalMs > 30000) {
    config.behavior.blinkIntervalMs = defaults.behavior.blinkIntervalMs;
    changed = true;
  }
  if (config.behavior.lookIntervalMs < 500 || config.behavior.lookIntervalMs > 30000) {
    config.behavior.lookIntervalMs = defaults.behavior.lookIntervalMs;
    changed = true;
  }
  if (config.behavior.idleMove > 6) {
    config.behavior.idleMove = defaults.behavior.idleMove;
    changed = true;
  }
  if (config.behavior.sleepyAfterMs < 1000 || config.behavior.sleepyAfterMs > 300000) {
    config.behavior.sleepyAfterMs = defaults.behavior.sleepyAfterMs;
    changed = true;
  }
  if (config.behavior.expressionRhythm > EyesConfig::EXPRESSION_RHYTHM_LIVELY) {
    config.behavior.expressionRhythm = defaults.behavior.expressionRhythm;
    changed = true;
  }

  if (config.interaction.lookHoldMs < 200 || config.interaction.lookHoldMs > 10000) {
    config.interaction.lookHoldMs = defaults.interaction.lookHoldMs;
    changed = true;
  }
  if (config.interaction.moodHoldMs < 200 || config.interaction.moodHoldMs > 10000) {
    config.interaction.moodHoldMs = defaults.interaction.moodHoldMs;
    changed = true;
  }

  if (config.time.font > CLOCK_FONT_PIXEL_RAIL_5X8) {
    config.time.font = defaults.time.font;
    changed = true;
  }
  if (config.time.fontSize < 1 || config.time.fontSize > 3) {
    config.time.fontSize = defaults.time.fontSize;
    changed = true;
  }

  if (!isValidHexColorString(config.style.eyeColor)) {
    memcpy(config.style.eyeColor, defaults.style.eyeColor, sizeof(config.style.eyeColor));
    changed = true;
  }
  if (!isValidHexColorString(config.style.timeColor)) {
    memcpy(config.style.timeColor, defaults.style.timeColor, sizeof(config.style.timeColor));
    changed = true;
  }

  return changed;
}

bool isStaticallyRecoverableBusinessModeTag(const String& businessModeTag) {
  return businessModeTag == ModeTags::CLOCK ||
         businessModeTag == ModeTags::THEME ||
         businessModeTag == ModeTags::CANVAS ||
         businessModeTag == ModeTags::ANIMATION ||
         businessModeTag == ModeTags::TETRIS ||
         businessModeTag == ModeTags::TETRIS_CLOCK ||
         businessModeTag == ModeTags::GIF_PLAYER ||
         businessModeTag == ModeTags::EYES ||
         businessModeTag == ModeTags::AMBIENT_EFFECT ||
         businessModeTag == ModeTags::LED_MATRIX_SHOWCASE ||
         businessModeTag == ModeTags::MAZE ||
         businessModeTag == ModeTags::SNAKE ||
         businessModeTag == ModeTags::PLANET_SCREENSAVER;
}

bool canPersistCurrentBusinessModeTag(const String& businessModeTag) {
  if (businessModeTag == ModeTags::GIF_PLAYER) {
    return AnimationManager::currentGIF != nullptr &&
           AnimationManager::currentGIF->frameCount > 0;
  }

  return isStaticallyRecoverableBusinessModeTag(businessModeTag);
}

bool isReturnTargetBusinessModeTag(const String& businessModeTag) {
  return RuntimeModeCoordinator::isReturnTargetBusinessModeTag(businessModeTag);
}

DeviceMode resolveTopLevelModeFromBusinessModeTag(const String& businessModeTag) {
  return RuntimeModeCoordinator::resolveTopLevelMode(businessModeTag);
}

String resolveBusinessModeTagFromMode(DeviceMode mode) {
  const char* restoredModeTag = DeviceModeTagCodec::toTagOrEmpty(mode);
  if (restoredModeTag[0] == '\0') {
    return ModeTags::CLOCK;
  }
  return String(restoredModeTag);
}

String sanitizeRecoverableBusinessModeTag(const String& primary, const String& fallback) {
  if (isStaticallyRecoverableBusinessModeTag(primary)) {
    return primary;
  }
  if (isStaticallyRecoverableBusinessModeTag(fallback)) {
    return fallback;
  }
  return ModeTags::CLOCK;
}

String sanitizeLastBusinessTargetTag(const String& primary, const String& fallback) {
  if (isReturnTargetBusinessModeTag(primary)) {
    return primary;
  }
  if (isReturnTargetBusinessModeTag(fallback)) {
    return fallback;
  }
  return ModeTags::CLOCK;
}
}

ClockConfig ConfigManager::clockConfig = {
  .font = CLOCK_FONT_CLASSIC_5X7,
  .showSeconds = false,
  .hourFormat = 24,
  .time = {true, 1, 17, 5, 255, 255, 255},
  .date = {false, 1, 22, 10, 120, 120, 120},
  .week = {false, 26, 47, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
ClockConfig ConfigManager::animClockConfig = {
  .font = CLOCK_FONT_CLASSIC_5X7,
  .showSeconds = false,
  .hourFormat = 24,
  .time = {true, 1, 17, 5, 255, 255, 255},
  .date = {false, 1, 22, 10, 120, 120, 120},
  .week = {false, 26, 47, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
ClockConfig ConfigManager::ledMatrixShowcaseClockConfig = {
  .font = CLOCK_FONT_CLASSIC_5X7,
  .showSeconds = false,
  .hourFormat = 24,
  .time = {true, 1, 17, 5, 255, 255, 255},
  .date = {false, 1, 22, 10, 120, 120, 120},
  .week = {false, 26, 47, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
ClockConfig ConfigManager::tetrisOverlayClockConfig = {
  .font = CLOCK_FONT_CLASSIC_5X7,
  .showSeconds = false,
  .hourFormat = 24,
  .time = {true, 1, 18, 2, 255, 255, 255},
  .date = {false, 1, 0, 0, 120, 120, 120},
  .week = {false, 0, 0, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
EyesConfig ConfigManager::eyesConfig = makeDefaultEyesConfig();
ThemeConfig ConfigManager::themeConfig = {};
ClockConfig ConfigManager::themeClockConfig = {
  .font = CLOCK_FONT_CLASSIC_5X7,
  .showSeconds = false,
  .hourFormat = 24,
  .time = {true, 1, 17, 5, 255, 255, 255},
  .date = {false, 1, 22, 10, 120, 120, 120},
  .week = {false, 26, 47, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
TetrisModeConfig ConfigManager::tetrisConfig = makeDefaultTetrisModeConfig(false);
TetrisClockModeConfig ConfigManager::tetrisClockConfig = makeDefaultTetrisClockModeConfig();
MazeModeConfig ConfigManager::mazeConfig = makeDefaultMazeModeConfig();
SnakeModeConfig ConfigManager::snakeConfig = makeDefaultSnakeModeConfig();
DeviceParamsConfig ConfigManager::deviceParamsConfig = makeDefaultDeviceParamsConfig();
PixelData* ConfigManager::staticImagePixels = nullptr;
int ConfigManager::staticImagePixelCount = 0;
PixelData* ConfigManager::animImagePixels = nullptr;
int ConfigManager::animImagePixelCount = 0;
uint8_t* ConfigManager::pacmanRouteData = nullptr;
uint16_t ConfigManager::pacmanRouteLength = 0;

void ConfigManager::init() {
  Serial.println("2. 加载闹钟配置...");

  // 检查配置版本，版本不匹配说明固件更新了默认配置，清除旧的
  preferences.begin("clock", true);
  int savedVersion = preferences.getInt("cfgVer", 0);
  preferences.end();

  if (savedVersion != CONFIG_VERSION) {
    Serial.println("配置版本不匹配，清除旧配置");
    resetToDefault();
    // 保存新版本号
    preferences.begin("clock", false);
    preferences.putInt("cfgVer", CONFIG_VERSION);
    preferences.end();
  } else {
    loadDeviceParamsConfig();
    loadClockConfig();
    loadAnimClockConfig();
    loadLedMatrixShowcaseClockConfig();
    loadTetrisOverlayClockConfig();
    loadStaticImagePixels();
    loadAnimImagePixels();
    loadEyesConfig();
    loadAmbientEffectConfig();
    loadThemeConfig();
    loadThemeClockConfig();
    loadTetrisConfig();
    loadTetrisClockConfig();
    loadMazeConfig();
    loadSnakeConfig();
    loadPacmanRoute();
    loadCanvasPixels();
  }
}

void ConfigManager::preloadDeviceParamsConfig() {
  preferences.begin("clock", true);
  int savedVersion = preferences.getInt("cfgVer", 0);
  preferences.end();

  if (savedVersion != CONFIG_VERSION) {
    deviceParamsConfig = makeDefaultDeviceParamsConfig();
    DisplayManager::currentBrightness = deviceParamsConfig.displayBright;
    Serial.println("device params preload: using default because config version changed");
    return;
  }

  readDeviceParamsConfigFromPreferences(deviceParamsConfig);
  DisplayManager::currentBrightness = deviceParamsConfig.displayBright;
}

void ConfigManager::loadDeviceParamsConfig() {
  readDeviceParamsConfigFromPreferences(deviceParamsConfig);
  DisplayManager::currentBrightness = deviceParamsConfig.displayBright;
  Serial.println("device params config loaded for runtime state");
}

void ConfigManager::saveDeviceParamsConfig() {
  preferences.begin("device", false);
  preferences.putBytes("config", &deviceParamsConfig, sizeof(DeviceParamsConfig));
  preferences.end();
  Serial.println("device params config saved");
}

void ConfigManager::loadClockConfig() {
  preferences.begin("clock", true);

  // 加载配置
  size_t configSize = preferences.getBytesLength("config");
  Serial.printf("配置大小: %d bytes (期望: %d bytes)\n", configSize, sizeof(ClockConfig));

  if (configSize == sizeof(ClockConfig)) {
    preferences.getBytes("config", &clockConfig, sizeof(ClockConfig));
    if (migrateLegacyDefaultClockLayout(clockConfig)) {
      Serial.println("已迁移静态时钟默认布局");
      saveClockConfig();
    }
    Serial.println("已加载保存的闹钟配置");
    Serial.printf("  image.show = %d\n", clockConfig.image.show);
  } else {
    Serial.println("使用默认闹钟配置");
  }

  // 恢复上次的显示模式
  DeviceMode savedMode = static_cast<DeviceMode>(preferences.getInt("dispMode", (int)MODE_CLOCK));
  String savedBusinessMode = preferences.getString("bizMode", "");
  if (savedBusinessMode.length() == 0) {
    savedBusinessMode = resolveBusinessModeTagFromMode(savedMode);
  }
  savedBusinessMode = sanitizeRecoverableBusinessModeTag(savedBusinessMode, "");

  String savedLastBusinessMode = preferences.getString("lastBizMode", "");
  savedLastBusinessMode = sanitizeLastBusinessTargetTag(savedLastBusinessMode, savedBusinessMode);

  savedMode = resolveTopLevelModeFromBusinessModeTag(savedBusinessMode);

  DisplayManager::currentMode = savedMode;
  DisplayManager::currentBusinessModeTag = savedBusinessMode;
  DisplayManager::lastBusinessModeTag = savedLastBusinessMode;
  DisplayManager::lastBusinessMode =
    resolveTopLevelModeFromBusinessModeTag(savedLastBusinessMode);

  const char* modeStr =
    savedMode == MODE_ANIMATION
      ? "ANIMATION"
      : (savedMode == MODE_CLOCK
          ? "CLOCK"
          : (savedMode == MODE_THEME ? "THEME" : "CANVAS"));
  Serial.printf("恢复显示模式: %s\n", modeStr);
  Serial.printf("恢复业务模式: %s\n", DisplayManager::currentBusinessModeTag.c_str());

  preferences.end();
}

void ConfigManager::saveClockConfig() {
  preferences.begin("clock", false);
  preferences.putBytes("config", &clockConfig, sizeof(ClockConfig));

  DeviceMode storedMode = static_cast<DeviceMode>(preferences.getInt("dispMode", (int)MODE_CLOCK));
  String storedBusinessMode = preferences.getString("bizMode", "");
  if (storedBusinessMode.length() == 0) {
    storedBusinessMode = resolveBusinessModeTagFromMode(storedMode);
  }
  storedBusinessMode = sanitizeRecoverableBusinessModeTag(storedBusinessMode, "");

  String bizModeToSave = storedBusinessMode;
  if (canPersistCurrentBusinessModeTag(DisplayManager::currentBusinessModeTag)) {
    bizModeToSave = DisplayManager::currentBusinessModeTag;
  }

  DeviceMode modeToSave = resolveTopLevelModeFromBusinessModeTag(bizModeToSave);
  String lastBusinessModeToSave =
    sanitizeLastBusinessTargetTag(DisplayManager::lastBusinessModeTag, bizModeToSave);

  preferences.putInt("dispMode", (int)modeToSave);
  preferences.putString("bizMode", bizModeToSave);
  preferences.putString("lastBizMode", lastBusinessModeToSave);
  preferences.end();
  Serial.println("static clock config saved");
}

void ConfigManager::loadAnimClockConfig() {
  preferences.begin("anim", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(ClockConfig)) {
    preferences.getBytes("config", &animClockConfig, sizeof(ClockConfig));
    if (migrateLegacyDefaultClockLayout(animClockConfig)) {
      Serial.println("已迁移动态时钟默认布局");
      saveAnimClockConfig();
    }
    Serial.println("anim clock config loaded");
  } else {
    Serial.println("anim clock config: using default");
  }
  preferences.end();
}

void ConfigManager::saveAnimClockConfig() {
  preferences.begin("anim", false);
  preferences.putBytes("config", &animClockConfig, sizeof(ClockConfig));
  preferences.end();
  Serial.println("anim clock config saved");
}

void ConfigManager::loadLedMatrixShowcaseClockConfig() {
  preferences.begin("ledmx_clk", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(ClockConfig)) {
    preferences.getBytes("config", &ledMatrixShowcaseClockConfig, sizeof(ClockConfig));
    if (migrateLegacyDefaultClockLayout(ledMatrixShowcaseClockConfig)) {
      Serial.println("已迁移像素场景集时钟默认布局");
      saveLedMatrixShowcaseClockConfig();
    }
    Serial.println("led matrix showcase clock config loaded");
  } else {
    Serial.println("led matrix showcase clock config: using default");
  }
  preferences.end();
}

void ConfigManager::saveLedMatrixShowcaseClockConfig() {
  preferences.begin("ledmx_clk", false);
  preferences.putBytes("config", &ledMatrixShowcaseClockConfig, sizeof(ClockConfig));
  preferences.end();
  Serial.println("led matrix showcase clock config saved");
}

void ConfigManager::loadTetrisOverlayClockConfig() {
  preferences.begin("tetris_ovr", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(ClockConfig)) {
    preferences.getBytes("config", &tetrisOverlayClockConfig, sizeof(ClockConfig));
    if (migrateLegacyDefaultClockLayout(tetrisOverlayClockConfig)) {
      Serial.println("已迁移俄罗斯方块屏保时钟默认布局");
      saveTetrisOverlayClockConfig();
    }
    Serial.println("tetris overlay clock config loaded");
  } else {
    Serial.println("tetris overlay clock config: using default");
  }
  preferences.end();
}

void ConfigManager::saveTetrisOverlayClockConfig() {
  preferences.begin("tetris_ovr", false);
  preferences.putBytes("config", &tetrisOverlayClockConfig, sizeof(ClockConfig));
  preferences.end();
  Serial.println("tetris overlay clock config saved");
}

void ConfigManager::loadStaticImagePixels() {
  preferences.begin("clock", true);

  staticImagePixelCount = preferences.getInt("pixelCount", 0);
  Serial.printf("静态时钟像素数量: %d\n", staticImagePixelCount);

  if (staticImagePixelCount > 0) {
    size_t pixelDataSize = preferences.getBytesLength("pixels");
    Serial.printf("静态时钟像素数据大小: %d bytes (期望: %d bytes)\n",
      pixelDataSize, sizeof(PixelData) * staticImagePixelCount);

    if (pixelDataSize == sizeof(PixelData) * staticImagePixelCount) {
      staticImagePixels = (PixelData*)malloc(pixelDataSize);
      if (staticImagePixels != nullptr) {
        preferences.getBytes("pixels", staticImagePixels, pixelDataSize);
        Serial.printf("✓ 已加载静态时钟像素数据: %d 个像素\n", staticImagePixelCount);

        // 如果有像素数据，自动启用图片显示
        clockConfig.image.show = true;
        Serial.println("✓ 已自动启用静态时钟图片显示");
      } else {
        Serial.println("✗ 静态时钟像素数据内存分配失败");
        staticImagePixelCount = 0;
      }
    } else {
      Serial.println("✗ 静态时钟像素数据大小不匹配");
      staticImagePixelCount = 0;
    }
  } else {
    Serial.println("无保存的静态时钟像素数据");
  }

  preferences.end();
}

void ConfigManager::saveStaticImagePixels() {
  preferences.begin("clock", false);

  if (staticImagePixels == nullptr || staticImagePixelCount <= 0) {
    preferences.remove("pixels");
    preferences.putInt("pixelCount", 0);
    preferences.end();
    Serial.println("静态时钟像素数据已清空");
    return;
  }

  size_t dataSize = staticImagePixelCount * sizeof(PixelData);
  const size_t maxDataSize = 64 * 64 * sizeof(PixelData);

  if (dataSize <= maxDataSize) {
    preferences.putBytes("pixels", staticImagePixels, dataSize);
    preferences.putInt("pixelCount", staticImagePixelCount);
    Serial.printf("静态时钟像素数据已保存: %d 个像素（%d bytes）\n", staticImagePixelCount, dataSize);
  } else {
    Serial.printf("静态时钟像素数据太大（%d bytes），无法保存\n", dataSize);
  }

  preferences.end();
}

void ConfigManager::loadAnimImagePixels() {
  preferences.begin("anim", true);

  animImagePixelCount = preferences.getInt("pixelCount", 0);
  Serial.printf("动态时钟像素数量: %d\n", animImagePixelCount);

  if (animImagePixelCount > 0) {
    size_t pixelDataSize = preferences.getBytesLength("pixels");
    Serial.printf("动态时钟像素数据大小: %d bytes (期望: %d bytes)\n",
      pixelDataSize, sizeof(PixelData) * animImagePixelCount);

    if (pixelDataSize == sizeof(PixelData) * animImagePixelCount) {
      animImagePixels = (PixelData*)malloc(pixelDataSize);
      if (animImagePixels != nullptr) {
        preferences.getBytes("pixels", animImagePixels, pixelDataSize);
        Serial.printf("✓ 已加载动态时钟像素数据: %d 个像素\n", animImagePixelCount);

        // 如果有像素数据，自动启用图片显示
        animClockConfig.image.show = true;
        Serial.println("✓ 已自动启用动态时钟图片显示");
      } else {
        Serial.println("✗ 动态时钟像素数据内存分配失败");
        animImagePixelCount = 0;
      }
    } else {
      Serial.println("✗ 动态时钟像素数据大小不匹配");
      animImagePixelCount = 0;
    }
  } else {
    Serial.println("无保存的动态时钟像素数据");
  }

  preferences.end();
}

void ConfigManager::saveAnimImagePixels() {
  preferences.begin("anim", false);

  if (animImagePixels == nullptr || animImagePixelCount <= 0) {
    preferences.remove("pixels");
    preferences.putInt("pixelCount", 0);
    preferences.end();
    Serial.println("动态时钟像素数据已清空");
    return;
  }

  size_t dataSize = animImagePixelCount * sizeof(PixelData);
  const size_t maxDataSize = 64 * 64 * sizeof(PixelData);

  if (dataSize <= maxDataSize) {
    preferences.putBytes("pixels", animImagePixels, dataSize);
    preferences.putInt("pixelCount", animImagePixelCount);
    Serial.printf("动态时钟像素数据已保存: %d 个像素（%d bytes）\n", animImagePixelCount, dataSize);
  } else {
    Serial.printf("动态时钟像素数据太大（%d bytes），无法保存\n", dataSize);
  }

  preferences.end();
}

void ConfigManager::loadEyesConfig() {
  preferences.begin("eyes", true);

  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(EyesConfig)) {
    preferences.getBytes("config", &eyesConfig, sizeof(EyesConfig));
    if (sanitizeEyesConfig(eyesConfig)) {
      Serial.println("eyes config loaded and sanitized");
    } else {
      Serial.println("eyes config loaded");
    }
  } else {
    eyesConfig = makeDefaultEyesConfig();
    Serial.println("eyes config: using default");
  }

  preferences.end();
}

void ConfigManager::saveEyesConfig() {
  preferences.begin("eyes", false);
  preferences.putBytes("config", &eyesConfig, sizeof(EyesConfig));
  preferences.end();
  Serial.println("eyes config saved");
}

void ConfigManager::loadAmbientEffectConfig() {
  preferences.begin("ambient", true);

  size_t configSize = preferences.getBytesLength("config");
  bool removedPresetMigrated = false;
  if (configSize == sizeof(AmbientEffectConfig)) {
    preferences.getBytes("config", &DisplayManager::ambientEffectConfig, sizeof(AmbientEffectConfig));
    if (DisplayManager::ambientEffectConfig.preset == 13 ||
        DisplayManager::ambientEffectConfig.preset == 14 ||
        DisplayManager::ambientEffectConfig.preset == 24) {
      DisplayManager::ambientEffectConfig = {AMBIENT_PRESET_AURORA, 6, 72, 72, 100, 200, 255, true};
      removedPresetMigrated = true;
      Serial.println("ambient effect config migrated from removed preset");
    }
    Serial.println("ambient effect config loaded");
  } else {
    DisplayManager::ambientEffectConfig = {AMBIENT_PRESET_AURORA, 6, 72, 72, 100, 200, 255, true};
    Serial.println("ambient effect config: using default");
  }

  preferences.end();

  if (removedPresetMigrated) {
    saveAmbientEffectConfig();
  }
}

void ConfigManager::saveAmbientEffectConfig() {
  preferences.begin("ambient", false);
  preferences.putBytes("config", &DisplayManager::ambientEffectConfig, sizeof(AmbientEffectConfig));
  preferences.end();
  Serial.println("ambient effect config saved");
}

void ConfigManager::loadThemeConfig() {
  preferences.begin("theme", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(ThemeConfig)) {
    preferences.getBytes("config", &themeConfig, sizeof(ThemeConfig));
    themeConfig.themeId[sizeof(themeConfig.themeId) - 1] = '\0';
    Serial.printf("theme config loaded: %s\n", themeConfig.themeId);
  } else {
    themeConfig.themeId[0] = '\0';
    Serial.println("theme config: using default");
  }
  preferences.end();
}

void ConfigManager::saveThemeConfig() {
  preferences.begin("theme", false);
  preferences.putBytes("config", &themeConfig, sizeof(ThemeConfig));
  preferences.end();
  Serial.printf("theme config saved: %s\n", themeConfig.themeId);
}

void ConfigManager::loadThemeClockConfig() {
  preferences.begin("theme_clk", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(ClockConfig)) {
    preferences.getBytes("config", &themeClockConfig, sizeof(ClockConfig));
    if (migrateLegacyDefaultClockLayout(themeClockConfig)) {
      Serial.println("已迁移主题模式默认布局");
      saveThemeClockConfig();
    }
    Serial.println("theme clock config loaded");
  } else {
    Serial.println("theme clock config: using default");
  }
  preferences.end();
}

void ConfigManager::saveThemeClockConfig() {
  preferences.begin("theme_clk", false);
  preferences.putBytes("config", &themeClockConfig, sizeof(ClockConfig));
  preferences.end();
  Serial.println("theme clock config saved");
}

void ConfigManager::loadTetrisConfig() {
  preferences.begin("tetris", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(TetrisModeConfig)) {
    preferences.getBytes("config", &tetrisConfig, sizeof(TetrisModeConfig));
    Serial.println("tetris config loaded");
  } else {
    tetrisConfig = makeDefaultTetrisModeConfig(false);
    Serial.println("tetris config: using default");
  }
  preferences.end();
}

void ConfigManager::saveTetrisConfig() {
  preferences.begin("tetris", false);
  preferences.putBytes("config", &tetrisConfig, sizeof(TetrisModeConfig));
  preferences.end();
  Serial.println("tetris config saved");
}

void ConfigManager::loadTetrisClockConfig() {
  preferences.begin("tetris_clk", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(TetrisClockModeConfig)) {
    preferences.getBytes("config", &tetrisClockConfig, sizeof(TetrisClockModeConfig));
    Serial.println("tetris clock config loaded");
  } else {
    tetrisClockConfig = makeDefaultTetrisClockModeConfig();
    Serial.println("tetris clock config: using default");
  }
  preferences.end();
}

void ConfigManager::saveTetrisClockConfig() {
  preferences.begin("tetris_clk", false);
  preferences.putBytes("config", &tetrisClockConfig, sizeof(TetrisClockModeConfig));
  preferences.end();
  Serial.println("tetris clock config saved");
}

void ConfigManager::loadMazeConfig() {
  preferences.begin("maze", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(MazeModeConfig)) {
    preferences.getBytes("config", &mazeConfig, sizeof(MazeModeConfig));
    mazeConfig.panelBgColor[sizeof(mazeConfig.panelBgColor) - 1] = '\0';
    mazeConfig.borderColor[sizeof(mazeConfig.borderColor) - 1] = '\0';
    mazeConfig.timeColor[sizeof(mazeConfig.timeColor) - 1] = '\0';
    mazeConfig.dateColor[sizeof(mazeConfig.dateColor) - 1] = '\0';
    mazeConfig.generationPathColor[sizeof(mazeConfig.generationPathColor) - 1] = '\0';
    mazeConfig.searchVisitedColor[sizeof(mazeConfig.searchVisitedColor) - 1] = '\0';
    mazeConfig.searchFrontierColor[sizeof(mazeConfig.searchFrontierColor) - 1] = '\0';
    mazeConfig.solvedPathStartColor[sizeof(mazeConfig.solvedPathStartColor) - 1] = '\0';
    mazeConfig.solvedPathEndColor[sizeof(mazeConfig.solvedPathEndColor) - 1] = '\0';
    if (isValidMazeModeConfig(mazeConfig)) {
      Serial.println("maze config loaded");
    } else {
      mazeConfig = makeDefaultMazeModeConfig();
      Serial.println("maze config invalid: using default");
    }
  } else {
    mazeConfig = makeDefaultMazeModeConfig();
    Serial.println("maze config: using default");
  }
  preferences.end();
}

void ConfigManager::saveMazeConfig() {
  preferences.begin("maze", false);
  preferences.putBytes("config", &mazeConfig, sizeof(MazeModeConfig));
  preferences.end();
  Serial.println("maze config saved");
}

void ConfigManager::loadSnakeConfig() {
  preferences.begin("snake", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(SnakeModeConfig)) {
    preferences.getBytes("config", &snakeConfig, sizeof(SnakeModeConfig));
    snakeConfig.snakeSkin[sizeof(snakeConfig.snakeSkin) - 1] = '\0';
    Serial.println("snake config loaded");
  } else {
    snakeConfig = makeDefaultSnakeModeConfig();
    Serial.println("snake config: using default");
  }
  preferences.end();
}

void ConfigManager::saveSnakeConfig() {
  preferences.begin("snake", false);
  preferences.putBytes("config", &snakeConfig, sizeof(SnakeModeConfig));
  preferences.end();
  Serial.println("snake config saved");
}

void ConfigManager::loadPlanetScreensaverConfig() {
  preferences.begin("planet_ss", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(PlanetScreensaverNativeConfig)) {
    PlanetScreensaverNativeConfig config = {};
    preferences.getBytes("config", &config, sizeof(PlanetScreensaverNativeConfig));
    config.preset[sizeof(config.preset) - 1] = '\0';
    config.size[sizeof(config.size) - 1] = '\0';
    config.direction[sizeof(config.direction) - 1] = '\0';
    BoardNativeEffect::setPlanetScreensaverConfig(config);
    Serial.println("planet screensaver config loaded");
  } else {
    Serial.println("planet screensaver config: keep current");
  }
  preferences.end();
}

void ConfigManager::savePlanetScreensaverConfig() {
  const PlanetScreensaverNativeConfig& config =
    BoardNativeEffect::getPlanetScreensaverConfig();
  preferences.begin("planet_ss", false);
  preferences.putBytes("config", &config, sizeof(PlanetScreensaverNativeConfig));
  preferences.end();
  Serial.println("planet screensaver config saved");
}

void ConfigManager::loadPacmanRoute() {
  preferences.begin("theme", true);

  uint32_t savedLength = preferences.getUInt("pcRouteLen", 0);
  size_t savedBytes = preferences.getBytesLength("pcRouteData");

  if (pacmanRouteData != nullptr) {
    free(pacmanRouteData);
    pacmanRouteData = nullptr;
  }
  pacmanRouteLength = 0;

  if (savedLength > 0 && savedBytes == savedLength) {
    pacmanRouteData = static_cast<uint8_t*>(malloc(savedLength));
    if (pacmanRouteData != nullptr) {
      preferences.getBytes("pcRouteData", pacmanRouteData, savedLength);
      pacmanRouteLength = static_cast<uint16_t>(savedLength);
      Serial.printf("pacman route loaded: %u steps\n", pacmanRouteLength);
    } else {
      Serial.println("pacman route load failed: out of memory");
    }
  } else {
    Serial.println("pacman route: using generated route");
  }

  preferences.end();
}

bool ConfigManager::savePacmanRoute(const uint8_t* routeData, uint16_t routeLength) {
  if (routeData == nullptr || routeLength == 0) {
    return false;
  }

  preferences.begin("theme", false);
  preferences.putUInt("pcRouteLen", routeLength);
  size_t written = preferences.putBytes("pcRouteData", routeData, routeLength);
  preferences.end();

  if (written != routeLength) {
    Serial.println("pacman route save failed");
    return false;
  }

  if (pacmanRouteData != nullptr) {
    free(pacmanRouteData);
    pacmanRouteData = nullptr;
  }
  pacmanRouteData = static_cast<uint8_t*>(malloc(routeLength));
  if (pacmanRouteData == nullptr) {
    pacmanRouteLength = 0;
    Serial.println("pacman route cache update failed");
    return false;
  }

  memcpy(pacmanRouteData, routeData, routeLength);
  pacmanRouteLength = routeLength;
  Serial.printf("pacman route saved: %u steps\n", pacmanRouteLength);
  return true;
}

void ConfigManager::clearPacmanRoute() {
  preferences.begin("theme", false);
  preferences.remove("pcRouteLen");
  preferences.remove("pcRouteData");
  preferences.end();

  if (pacmanRouteData != nullptr) {
    free(pacmanRouteData);
    pacmanRouteData = nullptr;
  }
  pacmanRouteLength = 0;
  Serial.println("pacman route cleared");
}

void ConfigManager::loadCanvasPixels() {
  DisplayManager::clearCanvas();
  preferences.begin("canvas", true);

  bool initialized = preferences.getBool("initialized", false);
  size_t dataSize = preferences.getBytesLength("buffer");

  if (initialized && dataSize == sizeof(DisplayManager::canvasBuffer)) {
    preferences.getBytes("buffer", DisplayManager::canvasBuffer, sizeof(DisplayManager::canvasBuffer));
    DisplayManager::canvasInitialized = true;
    Serial.println("画板像素数据已加载");
  } else {
    Serial.println("无可用画板像素数据，使用空画板");
  }

  preferences.end();
}

void ConfigManager::saveCanvasPixels() {
  preferences.begin("canvas", false);

  preferences.putBool("initialized", DisplayManager::canvasInitialized);
  if (DisplayManager::canvasInitialized) {
    preferences.putBytes("buffer", DisplayManager::canvasBuffer, sizeof(DisplayManager::canvasBuffer));
  } else {
    preferences.remove("buffer");
  }

  preferences.end();
  Serial.println("画板像素数据已保存");
}

void ConfigManager::clearCanvasPixels() {
  preferences.begin("canvas", false);
  preferences.clear();
  preferences.end();
  Serial.println("画板像素持久化已清空");
}

void ConfigManager::resetToDefault() {
  Serial.println("清除所有配置，恢复默认...");

  preferences.begin("clock", false);
  preferences.clear();
  preferences.end();

  preferences.begin("anim", false);
  preferences.clear();
  preferences.end();

  preferences.begin("canvas", false);
  preferences.clear();
  preferences.end();

  preferences.begin("eyes", false);
  preferences.clear();
  preferences.end();

  preferences.begin("ambient", false);
  preferences.clear();
  preferences.end();

  preferences.begin("theme", false);
  preferences.clear();
  preferences.end();

  preferences.begin("tetris", false);
  preferences.clear();
  preferences.end();

  preferences.begin("tetris_clk", false);
  preferences.clear();
  preferences.end();

  preferences.begin("game_ss", false);
  preferences.clear();
  preferences.end();

  preferences.begin("maze", false);
  preferences.clear();
  preferences.end();

  preferences.begin("snake", false);
  preferences.clear();
  preferences.end();

  preferences.begin("tetris_ovr", false);
  preferences.clear();
  preferences.end();

  preferences.begin("planet_ss", false);
  preferences.clear();
  preferences.end();

  preferences.begin("device", false);
  preferences.clear();
  preferences.end();
  if (pacmanRouteData != nullptr) {
    free(pacmanRouteData);
    pacmanRouteData = nullptr;
  }
  pacmanRouteLength = 0;

  if (staticImagePixels != nullptr) {
    free(staticImagePixels);
    staticImagePixels = nullptr;
  }
  staticImagePixelCount = 0;

  if (animImagePixels != nullptr) {
    free(animImagePixels);
    animImagePixels = nullptr;
  }
  animImagePixelCount = 0;

  clockConfig = {
    .font = CLOCK_FONT_CLASSIC_5X7,
    .showSeconds = false,
    .hourFormat = 24,
    .time = {true, 1, 17, 5, 255, 255, 255},
    .date = {false, 1, 22, 10, 120, 120, 120},
    .week = {false, 26, 47, 100, 100, 100},
    .image = {false, 0, 0, 64, 64}
  };

  animClockConfig = {
    .font = CLOCK_FONT_CLASSIC_5X7,
    .showSeconds = false,
    .hourFormat = 24,
    .time = {true, 1, 17, 5, 255, 255, 255},
    .date = {false, 1, 22, 10, 120, 120, 120},
    .week = {false, 26, 47, 100, 100, 100},
    .image = {false, 0, 0, 64, 64}
  };

  ledMatrixShowcaseClockConfig = {
    .font = CLOCK_FONT_CLASSIC_5X7,
    .showSeconds = false,
    .hourFormat = 24,
    .time = {true, 1, 17, 5, 255, 255, 255},
    .date = {false, 1, 22, 10, 120, 120, 120},
    .week = {false, 26, 47, 100, 100, 100},
    .image = {false, 0, 0, 64, 64}
  };

  tetrisOverlayClockConfig = {
    .font = CLOCK_FONT_CLASSIC_5X7,
    .showSeconds = false,
    .hourFormat = 24,
    .time = {true, 1, 18, 2, 255, 255, 255},
    .date = {false, 1, 0, 0, 120, 120, 120},
    .week = {false, 0, 0, 100, 100, 100},
    .image = {false, 0, 0, 64, 64}
  };

  eyesConfig = makeDefaultEyesConfig();
  DisplayManager::ambientEffectConfig = {AMBIENT_PRESET_AURORA, 6, 72, 72, 100, 200, 255, true};
  themeConfig.themeId[0] = '\0';
  themeClockConfig = {
    .font = CLOCK_FONT_CLASSIC_5X7,
    .showSeconds = false,
    .hourFormat = 24,
    .time = {true, 1, 17, 5, 255, 255, 255},
    .date = {false, 1, 22, 10, 120, 120, 120},
    .week = {false, 26, 47, 100, 100, 100},
    .image = {false, 0, 0, 64, 64}
  };
  tetrisConfig = makeDefaultTetrisModeConfig(false);
  tetrisClockConfig = makeDefaultTetrisClockModeConfig();
  mazeConfig = makeDefaultMazeModeConfig();
  snakeConfig = makeDefaultSnakeModeConfig();
  deviceParamsConfig = makeDefaultDeviceParamsConfig();
  DisplayManager::currentBrightness = deviceParamsConfig.displayBright;

  Serial.println("配置已恢复默认");
}
