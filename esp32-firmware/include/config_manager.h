#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// 闹钟配置结构
struct ClockConfig {
  struct {
    int fontSize;
    int x;
    int y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } time;
  
  struct {
    bool show;
    int fontSize;
    int x;
    int y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } date;
  
  struct {
    bool show;
    int x;
    int y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } week;
  
  struct {
    bool show;
    int x;
    int y;
    int width;
    int height;
  } image;
};

// 像素数据结构
struct PixelData {
  uint8_t x;
  uint8_t y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

class ConfigManager {
public:
  static void init();
  static void loadClockConfig();
  static void saveClockConfig();
  static void loadImagePixels();
  static void saveImagePixels();
  static void resetToDefault();  // 清除所有配置，恢复默认
  
  static ClockConfig clockConfig;
  static PixelData* imagePixels;
  static int imagePixelCount;
  static Preferences preferences;

private:
};

#endif