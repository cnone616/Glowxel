#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config_manager.h"

// 设备模式
enum DeviceMode {
  MODE_CANVAS,     // 画板/静态背景模式（默认，时钟叠加）
  MODE_ANIMATION   // 动态壁纸模式（时钟叠加）
};

class DisplayManager {
public:
  static void init();
  static void setupMatrix();
  static void displayClock(bool force = false);
  static void drawClockOverlay();  // 不清屏，只叠加时钟文字
  static void drawLogo(int x, int y);  // 画九宫格 logo 到指定坐标
  static void displayImage(uint8_t* data, size_t len, int width, int height);
  static void clearScreen();
  static void setBrightness(int brightness);

  // 3x5 微型字体渲染
  static void drawTinyText(const char* text, int x, int y, uint16_t color, int size = 1);
  static void drawTinyTextCentered(const char* text, int y, uint16_t color, int size = 1);

  static MatrixPanel_I2S_DMA* dma_display;
  static DeviceMode currentMode;
  static int currentBrightness;
  static bool clientConnected;
  
  // 画板模式相关
  static uint8_t canvasBuffer[64][64][3]; // [y][x][rgb]
  static bool canvasInitialized;
  static bool isCanvasMode;  // true=纯画板（不画时钟），false=静态时钟
  
  // 黑色像素坐标存储
  struct BlackPixel {
    uint8_t x;
    uint8_t y;
  };
  static BlackPixel* blackPixels;
  static int blackPixelCount;
  
  // 常量定义
  static const int PANEL_RES_X;
  static const int PANEL_RES_Y;
  static const int PANEL_CHAIN;
  static const int MAX_PIXELS;

private:
};

#endif