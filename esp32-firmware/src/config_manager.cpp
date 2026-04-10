#include "config_manager.h"
#include "display_manager.h"

// 静态成员初始化
Preferences ConfigManager::preferences;

namespace {
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
  defaultEyesConfig.interaction.lookHoldMs = 1200;
  defaultEyesConfig.interaction.moodHoldMs = 1800;
  defaultEyesConfig.time.show = true;
  defaultEyesConfig.time.showSeconds = false;
  defaultEyesConfig.time.font = CLOCK_FONT_CLASSIC_5X7;
  defaultEyesConfig.time.fontSize = 1;
  memcpy(defaultEyesConfig.style.eyeColor, "#9bdcff", sizeof(defaultEyesConfig.style.eyeColor));
  memcpy(defaultEyesConfig.style.pupilColor, "#1b6dff", sizeof(defaultEyesConfig.style.pupilColor));
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

  if (config.interaction.lookHoldMs < 200 || config.interaction.lookHoldMs > 10000) {
    config.interaction.lookHoldMs = defaults.interaction.lookHoldMs;
    changed = true;
  }
  if (config.interaction.moodHoldMs < 200 || config.interaction.moodHoldMs > 10000) {
    config.interaction.moodHoldMs = defaults.interaction.moodHoldMs;
    changed = true;
  }

  if (config.time.font > CLOCK_FONT_CLASSIC_5X7) {
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
  if (!isValidHexColorString(config.style.pupilColor)) {
    memcpy(config.style.pupilColor, defaults.style.pupilColor, sizeof(config.style.pupilColor));
    changed = true;
  }
  if (!isValidHexColorString(config.style.timeColor)) {
    memcpy(config.style.timeColor, defaults.style.timeColor, sizeof(config.style.timeColor));
    changed = true;
  }

  return changed;
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
EyesConfig ConfigManager::eyesConfig = {
  .layout = {24, 14, 16, 10, 22, 4},
  .behavior = {true, 3200, 4200, 2, 45000},
  .interaction = {1200, 1800},
  .time = {true, false, CLOCK_FONT_MINIMAL_3X5, 1},
  .style = {"#9bdcff", "#1b6dff", "#d8f3ff"}
};
ThemeConfig ConfigManager::themeConfig = {};
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
    loadClockConfig();
    loadAnimClockConfig();
    loadStaticImagePixels();
    loadAnimImagePixels();
    loadEyesConfig();
    loadAmbientEffectConfig();
    loadThemeConfig();
    loadPacmanRoute();
    loadCanvasPixels();
  }
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
  int savedMode = preferences.getInt("dispMode", (int)MODE_CLOCK);
  String savedBusinessMode = preferences.getString("bizMode", "");
  if (savedBusinessMode.length() == 0) {
    if (savedMode == MODE_CLOCK) {
      savedBusinessMode = "clock";
    } else if (savedMode == MODE_CANVAS) {
      savedBusinessMode = "canvas";
    } else if (savedMode == MODE_ANIMATION) {
      savedBusinessMode = "animation";
    } else if (savedMode == MODE_THEME) {
      savedBusinessMode = "theme";
    } else {
      savedBusinessMode = "clock";
    }
  }
  String savedLastBusinessMode = preferences.getString("lastBizMode", "");
  if (savedLastBusinessMode.length() == 0) {
    if (savedBusinessMode == "clock") {
      savedLastBusinessMode = "clock";
    } else if (savedBusinessMode == "canvas") {
      savedLastBusinessMode = "clock";
    } else {
      savedLastBusinessMode = savedBusinessMode;
    }
  }

  // 传输模式是临时态，重启后不应停留在该模式
  if (savedMode == MODE_TRANSFERRING) {
    if (savedLastBusinessMode == "clock") {
      savedMode = MODE_CLOCK;
      savedBusinessMode = "clock";
    } else if (savedLastBusinessMode == "theme") {
      savedMode = MODE_THEME;
      savedBusinessMode = "theme";
    } else if (savedLastBusinessMode == "canvas") {
      savedMode = MODE_CANVAS;
      savedBusinessMode = "canvas";
    } else {
      savedMode = MODE_ANIMATION;
      savedBusinessMode = savedLastBusinessMode;
    }
    Serial.println("检测到传输临时模式，已恢复到上次业务模式");
  }

  DisplayManager::currentMode = (DeviceMode)savedMode;
  DisplayManager::currentBusinessModeTag = savedBusinessMode;
  DisplayManager::lastBusinessModeTag = savedLastBusinessMode;
  if (savedLastBusinessMode == "clock") {
    DisplayManager::lastBusinessMode = MODE_CLOCK;
  } else if (savedLastBusinessMode == "theme") {
    DisplayManager::lastBusinessMode = MODE_THEME;
  } else {
    DisplayManager::lastBusinessMode = MODE_ANIMATION;
  }
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
  DeviceMode modeToSave = DisplayManager::currentMode;
  String bizModeToSave = DisplayManager::currentBusinessModeTag;
  if (modeToSave == MODE_TRANSFERRING) {
    modeToSave = DisplayManager::lastBusinessMode;
    bizModeToSave = DisplayManager::lastBusinessModeTag;
  }
  preferences.putInt("dispMode", (int)modeToSave);
  preferences.putString("bizMode", bizModeToSave);
  preferences.putString("lastBizMode", DisplayManager::lastBusinessModeTag);
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
  if (staticImagePixels == nullptr || staticImagePixelCount <= 0) return;

  preferences.begin("clock", false);

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
  if (animImagePixels == nullptr || animImagePixelCount <= 0) return;

  preferences.begin("anim", false);

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
  if (configSize == sizeof(AmbientEffectConfig)) {
    preferences.getBytes("config", &DisplayManager::ambientEffectConfig, sizeof(AmbientEffectConfig));
    Serial.println("ambient effect config loaded");
  } else {
    DisplayManager::ambientEffectConfig = {AMBIENT_PRESET_AURORA, 6, 72, true};
    Serial.println("ambient effect config: using default");
  }

  preferences.end();
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

  eyesConfig = makeDefaultEyesConfig();
  DisplayManager::ambientEffectConfig = {AMBIENT_PRESET_AURORA, 6, 72, true};
  themeConfig.themeId[0] = '\0';

  Serial.println("配置已恢复默认");
}
