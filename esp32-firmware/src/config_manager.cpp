#include "config_manager.h"
#include "display_manager.h"

// 静态成员初始化
Preferences ConfigManager::preferences;
ClockConfig ConfigManager::clockConfig = {
  .time = {2, 22, 4, 255, 255, 255},
  .date = {false, 1, 22, 10, 120, 120, 120},
  .week = {false, 26, 47, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
ClockConfig ConfigManager::animClockConfig = {
  .time = {2, 22, 4, 255, 255, 255},
  .date = {false, 1, 22, 10, 120, 120, 120},
  .week = {false, 26, 47, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
PixelData* ConfigManager::staticImagePixels = nullptr;
int ConfigManager::staticImagePixelCount = 0;
PixelData* ConfigManager::animImagePixels = nullptr;
int ConfigManager::animImagePixelCount = 0;

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
  }
}

void ConfigManager::loadClockConfig() {
  preferences.begin("clock", true);

  // 加载配置
  size_t configSize = preferences.getBytesLength("config");
  Serial.printf("配置大小: %d bytes (期望: %d bytes)\n", configSize, sizeof(ClockConfig));

  if (configSize == sizeof(ClockConfig)) {
    preferences.getBytes("config", &clockConfig, sizeof(ClockConfig));
    Serial.println("已加载保存的闹钟配置");
    Serial.printf("  image.show = %d\n", clockConfig.image.show);
  } else {
    Serial.println("使用默认闹钟配置");
  }

  // 恢复上次的显示模式
  int savedMode = preferences.getInt("dispMode", (int)MODE_CLOCK);
  DisplayManager::currentMode = (DeviceMode)savedMode;
  const char* modeStr = savedMode == MODE_ANIMATION ? "ANIMATION" : (savedMode == MODE_CLOCK ? "CLOCK" : "CANVAS");
  Serial.printf("恢复显示模式: %s\n", modeStr);

  preferences.end();
}

void ConfigManager::saveClockConfig() {
  preferences.begin("clock", false);
  preferences.putBytes("config", &clockConfig, sizeof(ClockConfig));
  preferences.putInt("dispMode", (int)DisplayManager::currentMode);
  preferences.end();
  Serial.println("static clock config saved");
}

void ConfigManager::loadAnimClockConfig() {
  preferences.begin("anim", true);
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == sizeof(ClockConfig)) {
    preferences.getBytes("config", &animClockConfig, sizeof(ClockConfig));
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

void ConfigManager::resetToDefault() {
  Serial.println("清除所有配置，恢复默认...");

  preferences.begin("clock", false);
  preferences.clear();
  preferences.end();

  preferences.begin("anim", false);
  preferences.clear();
  preferences.end();

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
    .time = {2, 22, 4, 255, 255, 255},
    .date = {false, 1, 22, 10, 120, 120, 120},
    .week = {false, 26, 47, 100, 100, 100},
    .image = {false, 0, 0, 64, 64}
  };

  animClockConfig = {
    .time = {2, 22, 4, 255, 255, 255},
    .date = {false, 1, 22, 10, 120, 120, 120},
    .week = {false, 26, 47, 100, 100, 100},
    .image = {false, 0, 0, 64, 64}
  };

  Serial.println("配置已恢复默认");
}