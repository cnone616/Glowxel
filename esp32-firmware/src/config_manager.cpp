#include "config_manager.h"
#include "display_manager.h"

// 静态成员初始化
Preferences ConfigManager::preferences;
ClockConfig ConfigManager::clockConfig = {
  // 默认：时间居中，size=2，白色
  .time = {2, 22, 4, 255, 255, 255},
  .date = {false, 1, 22, 10, 120, 120, 120},
  .week = {false, 26, 47, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
PixelData* ConfigManager::imagePixels = nullptr;
int ConfigManager::imagePixelCount = 0;

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
    loadImagePixels();
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
  int savedMode = preferences.getInt("dispMode", 0);
  DisplayManager::currentMode = (DeviceMode)savedMode;
  Serial.printf("恢复显示模式: %s\n", savedMode == MODE_ANIMATION ? "ANIMATION" : "CANVAS");

  preferences.end();
}

void ConfigManager::saveClockConfig() {
  preferences.begin("clock", false);
  preferences.putBytes("config", &clockConfig, sizeof(ClockConfig));
  // 持久化当前显示模式
  preferences.putInt("dispMode", (int)DisplayManager::currentMode);
  preferences.end();
  Serial.println("闹钟配置已保存");
}

void ConfigManager::loadImagePixels() {
  preferences.begin("clock", true);

  imagePixelCount = preferences.getInt("pixelCount", 0);
  Serial.printf("像素数量: %d\n", imagePixelCount);

  if (imagePixelCount > 0) {
    size_t pixelDataSize = preferences.getBytesLength("pixels");
    Serial.printf("像素数据大小: %d bytes (期望: %d bytes)\n",
      pixelDataSize, sizeof(PixelData) * imagePixelCount);

    if (pixelDataSize == sizeof(PixelData) * imagePixelCount) {
      imagePixels = (PixelData*)malloc(pixelDataSize);
      if (imagePixels != nullptr) {
        preferences.getBytes("pixels", imagePixels, pixelDataSize);
        Serial.printf("✓ 已加载保存的像素数据: %d 个像素\n", imagePixelCount);

        // 如果有像素数据，自动启用图片显示
        clockConfig.image.show = true;
        Serial.println("✓ 已自动启用图片显示");
      } else {
        Serial.println("✗ 像素数据内存分配失败");
        imagePixelCount = 0;
      }
    } else {
      Serial.println("✗ 像素数据大小不匹配");
      imagePixelCount = 0;
    }
  } else {
    Serial.println("没有保存的像素数据");
  }

  preferences.end();
}

void ConfigManager::saveImagePixels() {
  if (imagePixels == nullptr || imagePixelCount <= 0) return;

  preferences.begin("clock", false);

  size_t dataSize = imagePixelCount * sizeof(PixelData);
  const size_t maxDataSize = 64 * 64 * sizeof(PixelData); // 最大64x64像素

  if (dataSize <= maxDataSize) {
    preferences.putBytes("pixels", imagePixels, dataSize);
    preferences.putInt("pixelCount", imagePixelCount);
    Serial.printf("像素数据已保存: %d 个像素（%d bytes）\n", imagePixelCount, dataSize);
  } else {
    Serial.printf("像素数据太大（%d bytes），无法保存到 Preferences，最大允许 %d bytes\n",
      dataSize, maxDataSize);
  }

  preferences.end();
}

void ConfigManager::resetToDefault() {
  Serial.println("清除所有配置，恢复默认...");

  // 清除闹钟配置
  preferences.begin("clock", false);
  preferences.clear();
  preferences.end();

  // 释放像素内存
  if (imagePixels != nullptr) {
    free(imagePixels);
    imagePixels = nullptr;
  }
  imagePixelCount = 0;

  // 恢复默认闹钟配置
  clockConfig = {
    .time = {2, 22, 4, 255, 255, 255},
    .date = {false, 1, 22, 10, 120, 120, 120},
    .week = {false, 26, 47, 100, 100, 100},
    .image = {false, 0, 0, 64, 64}
  };

  Serial.println("配置已恢复默认");
}