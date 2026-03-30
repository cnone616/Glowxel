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

enum NativeEffectType : uint8_t {
  NATIVE_EFFECT_NONE = 0,
  NATIVE_EFFECT_BREATH = 1,
  NATIVE_EFFECT_RHYTHM = 2
};

enum BreathWaveform : uint8_t {
  BREATH_WAVE_SINE = 0,
  BREATH_WAVE_TRIANGLE = 1,
  BREATH_WAVE_SQUARE = 2
};

enum RhythmDirection : uint8_t {
  RHYTHM_DIR_LEFT = 0,
  RHYTHM_DIR_RIGHT = 1,
  RHYTHM_DIR_UP = 2,
  RHYTHM_DIR_DOWN = 3
};

enum RhythmMode : uint8_t {
  RHYTHM_MODE_PULSE = 0,
  RHYTHM_MODE_GRADIENT = 1,
  RHYTHM_MODE_JUMP = 2
};

struct BreathEffectConfig {
  uint8_t speed;
  bool loop;
  uint8_t minBrightness;
  uint8_t maxBrightness;
  uint16_t periodMs;
  uint8_t waveform;
  uint8_t colorR;
  uint8_t colorG;
  uint8_t colorB;
};

struct RhythmEffectConfig {
  uint16_t bpm;
  uint8_t speed;
  bool loop;
  uint8_t direction;
  uint8_t strength;
  uint8_t mode;
  uint8_t colorAR;
  uint8_t colorAG;
  uint8_t colorAB;
  uint8_t colorBR;
  uint8_t colorBG;
  uint8_t colorBB;
};

class DisplayManager {
public:
  static void init();
  static void setupMatrix();
  static void drawBackground();       // 独立：清屏+画像素背景或Logo
  static void displayClock(bool force = false);
  static void drawClockOverlay();  // 不清屏，只叠加时钟文字
  static void drawPixels(const PixelData* pixels, int pixelCount, bool clearFirst = false);
  static void renderCanvas();
  static void clearCanvas();
  static void highlightCanvasColor(int r, int g, int b);
  static void highlightCanvasRow(int row);
  static void renderAnimationFrame(const PixelData* pixels, int pixelCount, bool clearFirst);
  static void renderAnimationTransition(
    const PixelData* fromPixels,
    int fromPixelCount,
    const PixelData* toPixels,
    int toPixelCount,
    uint8_t mix
  );
  static void renderAnimationTransitionBuffers(
    const uint16_t* fromBuffer,
    const uint16_t* toBuffer,
    uint8_t mix
  );
  static void drawLogo(int x, int y);  // 画九宫格 logo 到指定坐标
  static void displayImage(uint8_t* data, size_t len, int width, int height);
  static void clearScreen();
  static void setBrightness(int brightness);

  // 3x5 微型字体渲染
  static void drawTinyText(const char* text, int x, int y, uint16_t color, int size = 1);
  static void drawTinyTextCentered(const char* text, int y, uint16_t color, int size = 1);
  static void setNativeEffectNone();
  static void activateBreathEffect(const BreathEffectConfig& config);
  static void activateRhythmEffect(const RhythmEffectConfig& config);
  static void renderNativeEffect();

  // Loading 动画控制
  static void startLoadingAnimation();
  static void stopLoadingAnimation();
  static void updateLoadingAnimation();

  static MatrixPanel_I2S_DMA* dma_display;
  static DeviceMode currentMode;
  static DeviceMode lastBusinessMode;
  static NativeEffectType nativeEffectType;
  static BreathEffectConfig breathEffectConfig;
  static RhythmEffectConfig rhythmEffectConfig;
  static unsigned long nativeEffectStartTime;
  static int currentBrightness;
  static bool clientConnected;
  
  // 画板模式相关
  static uint8_t canvasBuffer[64][64][3]; // [y][x][rgb]
  static uint16_t backgroundBuffer[64][64]; // 静态背景缓存，用于时钟脏区恢复
  static uint16_t animationBuffer[64][64];  // 动画帧缓存，避免清屏闪烁
  static bool canvasInitialized;
  static bool backgroundValid;
  static bool animationBufferValid;
  static bool receivingPixels;  // 正在接收背景像素，暂不刷新时钟
  
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
