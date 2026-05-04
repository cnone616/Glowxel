#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config_manager.h"

class CaptureMatrixPanel : public MatrixPanel_I2S_DMA {
public:
  using MatrixPanel_I2S_DMA::MatrixPanel_I2S_DMA;

  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
  void fillScreenRGB888(uint8_t r, uint8_t g, uint8_t b);
  void clearScreen();
};

class BufferMatrixPanel : public MatrixPanel_I2S_DMA {
public:
  using MatrixPanel_I2S_DMA::MatrixPanel_I2S_DMA;

  void setTargetBuffer(uint16_t* buffer, int width, int height);
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
  void clearScreen();

private:
  uint16_t* targetBuffer = nullptr;
  int targetWidth = 0;
  int targetHeight = 0;
};

// 设备模式（枚举值不能改，NVS 里存的是数字）
enum DeviceMode {
  MODE_CLOCK = 0,     // 0: 静态时钟背景模式（默认）
  MODE_ANIMATION = 1, // 1: 动态壁纸模式
  MODE_CANVAS = 2,    // 2: 画板模式
  MODE_TRANSFERRING = 3,  // 3: 临时传输模式（不持久化）
  MODE_THEME = 4      // 4: 主题模式；值 3 保留空洞，避免影响已存 NVS 数字
};

enum NativeEffectType : uint8_t {
  NATIVE_EFFECT_NONE = 0,
  NATIVE_EFFECT_BREATH = 1,
  NATIVE_EFFECT_RHYTHM = 2,
  NATIVE_EFFECT_EYES = 3,
  NATIVE_EFFECT_AMBIENT = 4
};

enum BreathMotion : uint8_t {
  BREATH_MOTION_CLOCKWISE = 0,
  BREATH_MOTION_COUNTERCLOCKWISE = 1,
  BREATH_MOTION_INWARD = 2,
  BREATH_MOTION_OUTWARD = 3
};

enum BreathScope : uint8_t {
  BREATH_SCOPE_SINGLE_RING = 0,
  BREATH_SCOPE_FULL_SCREEN = 1
};

enum BreathColorMode : uint8_t {
  BREATH_COLOR_SOLID = 0,
  BREATH_COLOR_GRADIENT = 1
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

enum AmbientEffectPreset : uint8_t {
  AMBIENT_PRESET_AURORA = 0,
  AMBIENT_PRESET_PLASMA = 1,
  AMBIENT_PRESET_MATRIX_RAIN = 2,
  AMBIENT_PRESET_FIREFLY_SWARM = 3,
  AMBIENT_PRESET_METEOR_SHOWER = 4,
  AMBIENT_PRESET_OCEAN_CURRENT = 5,
  AMBIENT_PRESET_NEON_GRID = 6,
  AMBIENT_PRESET_SUNSET_BLUSH = 7,
  AMBIENT_PRESET_STARFIELD_DRIFT = 8,
  AMBIENT_PRESET_BOIDS = 9,
  AMBIENT_PRESET_BOUNCING_LOGO = 10,
  AMBIENT_PRESET_SORTING_VISUALIZER = 11,
  AMBIENT_PRESET_CLOCK_SCENE = 12,
  AMBIENT_PRESET_GAME_OF_LIFE = 15,
  AMBIENT_PRESET_JULIA_SET = 16,
  AMBIENT_PRESET_REACTION_DIFFUSION = 17,
  AMBIENT_PRESET_COSMIC_KALE = 18,
  AMBIENT_PRESET_VOID_FIRE = 19,
  AMBIENT_PRESET_DEEP_SPACE_NEBULA = 20,
  AMBIENT_PRESET_WATER_SURFACE = 21,
  AMBIENT_PRESET_WATER_CURRENT = 22,
  AMBIENT_PRESET_WATER_CAUSTICS = 23
};

struct BreathEffectConfig {
  uint8_t speed;
  bool loop;
  uint8_t motion;
  uint8_t scope;
  uint8_t colorMode;
  uint8_t colorAR;
  uint8_t colorAG;
  uint8_t colorAB;
  uint8_t colorBR;
  uint8_t colorBG;
  uint8_t colorBB;
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

struct AmbientEffectConfig {
  uint8_t preset;
  uint8_t speed;
  uint8_t intensity;
  uint8_t density;
  uint8_t colorR;
  uint8_t colorG;
  uint8_t colorB;
  bool loop;
};

class DisplayManager {
public:
  static void init();
  static void setupMatrix();
  static void applyDeviceParams(bool showBootLogo = false);
  static bool rebuildMatrix(bool doubleBuffered, bool showBootLogo);
  static bool enableDoubleBuffer();
  static bool disableDoubleBuffer();
  static bool syncBufferStrategyForCurrentMode();
  static void drawBackground();       // 独立：清屏+画像素背景或Logo
  static void displayClock(bool force = false);
  static void displayTheme(bool force = false);
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
  static void refreshScheduledBrightness();
  static void lockRuntimeAccess();
  static void unlockRuntimeAccess();

  // 3x5 微型字体渲染
  static void drawTinyText(const char* text, int x, int y, uint16_t color, int size = 1);
  static void drawTinyTextCentered(const char* text, int y, uint16_t color, int size = 1);
  static void setNativeEffectNone();
  static void activateBreathEffect(const BreathEffectConfig& config);
  static void activateRhythmEffect(const RhythmEffectConfig& config);
  static void activateEyesEffect(const EyesConfig& config);
  static void activateAmbientEffect(const AmbientEffectConfig& config);
  static void renderNativeEffect();

  // Loading 动画控制
  static void startLoadingAnimation();
  static void stopLoadingAnimation();
  static void updateLoadingAnimation();
  static void clearLiveFrame(uint16_t color = 0);
  static void writeLiveFramePixel565(int x, int y, uint16_t color);
  static void writeLiveFramePixelRGB888(int x, int y, uint8_t r, uint8_t g, uint8_t b);
  static bool beginRedirectedFrame(
    uint16_t* targetBuffer,
    uint16_t clearColor = 0,
    bool clearBuffer = true
  );
  static void endRedirectedFrame(bool present = true);
  static MatrixPanel_I2S_DMA* beginOffscreenFrame(
    uint16_t* targetBuffer,
    uint16_t clearColor = 0,
    bool clearBuffer = true
  );
  struct SolidRectUpdate {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    uint16_t color;
  };
  static void presentOffscreenFrame(const uint16_t* targetBuffer);
  static void presentSolidRectUpdates(const SolidRectUpdate* updates, size_t updateCount);
  static unsigned long getLastPresentDurationUs();
  static uint16_t getLastPresentChangedPixels();
  static uint32_t getLastPresentFrameHash();

  static CaptureMatrixPanel* dma_display;
  static DeviceMode currentMode;
  static DeviceMode lastBusinessMode;
  static String currentBusinessModeTag;
  static String lastBusinessModeTag;
  static NativeEffectType nativeEffectType;
  static BreathEffectConfig breathEffectConfig;
  static RhythmEffectConfig rhythmEffectConfig;
  static AmbientEffectConfig ambientEffectConfig;
  static unsigned long nativeEffectStartTime;
  static int currentBrightness;
  static bool clientConnected;
  
  // 画板模式相关
  static uint8_t canvasBuffer[64][64][3]; // [y][x][rgb]
  static uint16_t backgroundBuffer[64][64]; // 静态背景缓存，用于时钟脏区恢复
  static uint16_t animationBuffer[64][64];  // 动画帧缓存，避免清屏闪烁
  static uint16_t liveFrameBuffer[64][64];  // 当前真实显示帧，用于板载网页预览
  static bool canvasInitialized;
  static bool backgroundValid;
  static bool animationBufferValid;
  static bool liveFrameValid;
  static bool doubleBufferEnabled;
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
