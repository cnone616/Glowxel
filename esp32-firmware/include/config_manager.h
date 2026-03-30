#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "clock_font_renderer.h"

#define CONFIG_VERSION 9  // 改默认配置时递增此版本号

// 闹钟配置结构
struct ClockConfig {
  uint8_t font;
  bool showSeconds;
  uint8_t hourFormat;

  struct {
    bool show;
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
  static void loadAnimClockConfig();
  static void saveAnimClockConfig();
  static void loadStaticImagePixels();
  static void saveStaticImagePixels();
  static void loadAnimImagePixels();
  static void saveAnimImagePixels();
  static void loadCanvasPixels();
  static void saveCanvasPixels();
  static void clearCanvasPixels();
  static void resetToDefault();

  static ClockConfig clockConfig;
  static ClockConfig animClockConfig;

  // 静态时钟的背景图片像素
  static PixelData* staticImagePixels;
  static int staticImagePixelCount;

  // 动态时钟的背景图片像素（如果不用 GIF 动画，可以用静态图）
  static PixelData* animImagePixels;
  static int animImagePixelCount;

  static Preferences preferences;

private:
};

#endif
