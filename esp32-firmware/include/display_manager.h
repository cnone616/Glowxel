#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config_manager.h"

// 设备模式（枚举值不能改，NVS 里存的是数字）
enum DeviceMode {
  MODE_CLOCK,        // 0: 静态时钟背景模式（默认）
  MODE_ANIMATION,    // 1: 动态壁纸模式
  MODE_CANVAS,       // 2: 画板模式
  MODE_TRANSFERRING  // 3: 传输模式（临时状态，不保存到 NVS）
};

class DisplayManager {
public:
  static void init();
  static void setupMatrix();
  static void drawBackground();       // 独立：清屏+画像素背景或Logo
  static void displayClock(bool force = false);
  static void drawClockOverlay();  // 不清屏，只叠加时钟文字
  static void drawLogo(int x, int y);  // 画九宫格 logo 到指定坐标
  static void displayImage(uint8_t* data, size_t len, int width, int height);
  static void clearScreen();
  static void setBrightness(int brightness);

  // 3x5 微型字体渲染
  static void drawTinyText(const char* text, int x, int y, uint16_t color, int size = 1);
  static void drawTinyTextCentered(const char* text, int y, uint16_t color, int size = 1);

  // Loading 动画控制
  static void startLoadingAnimation();
  static void stopLoadingAnimation();
  static void updateLoadingAnimation();

  static MatrixPanel_I2S_DMA* dma_display;
  static DeviceMode currentMode;
  static int currentBrightness;
  static bool clientConnected;
  
  // 画板模式相关
  static uint8_t canvasBuffer[64][64][3]; // [y][x][rgb]
  static bool canvasInitialized;
  static bool isCanvasMode;
  static bool receivingPixels;  // true=纯画板（不画时钟），false=静态时钟
  
  // 黑色像素坐标存储
  struct BlackPixel {
    uint8_t x;
    uint8_t y;
  };
  static BlackPixel* blackPixels;
  static int blackPixelCount;

  // Loading 动画状态
  static bool isLoadingActive;
  static int loadingStep;
  static unsigned long lastLoadingUpdate;

  // 常量定义
  static const int PANEL_RES_X;
  static const int PANEL_RES_Y;
  static const int PANEL_CHAIN;
  static const int MAX_PIXELS;

private:
};

#endif