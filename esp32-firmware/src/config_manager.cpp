#include "config_manager.h"

// 静态成员初始化
Preferences ConfigManager::preferences;
ClockConfig ConfigManager::clockConfig = {
  .time = {1, 17, 18, 100, 200, 255},
  .date = {true, 1, 3, 30, 120, 120, 120},
  .week = {true, 23, 44, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};
PixelData* ConfigManager::imagePixels = nullptr;
int ConfigManager::imagePixelCount = 0;

void ConfigManager::init() {
  Serial.println("2. 加载闹钟配置...");
  loadClockConfig();
  loadImagePixels();
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
  
  preferences.end();
}

void ConfigManager::saveClockConfig() {
  preferences.begin("clock", false);
  preferences.putBytes("config", &clockConfig, sizeof(ClockConfig));
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