#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "clock_font_renderer.h"
#include "maze_mode_types.h"
#include "snake_mode_types.h"

#define CONFIG_VERSION 12  // 改默认配置时递增此版本号

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

struct EyesConfig {
  enum ExpressionRhythm : uint8_t {
    EXPRESSION_RHYTHM_SLOW = 0,
    EXPRESSION_RHYTHM_STANDARD = 1,
    EXPRESSION_RHYTHM_LIVELY = 2,
  };

  struct {
    int eyeY;
    int eyeSpacing;
    int eyeWidth;
    int eyeHeight;
    int timeX;
    int timeY;
  } layout;

  struct {
    bool autoSwitch;
    uint16_t blinkIntervalMs;
    uint16_t lookIntervalMs;
    uint8_t idleMove;
    uint32_t sleepyAfterMs;
    uint8_t expressionRhythm;
  } behavior;

  struct {
    uint16_t lookHoldMs;
    uint16_t moodHoldMs;
  } interaction;

  struct {
    bool show;
    bool showSeconds;
    uint8_t font;
    uint8_t fontSize;
  } time;

  struct {
    char eyeColor[8];
    char timeColor[8];
  } style;
};

struct ThemeConfig {
  char themeId[48];
};

struct TetrisModeConfig {
  bool clearMode;
  uint8_t cellSize;
  uint16_t speed;
  bool showClock;
  uint8_t pieces;
};

struct TetrisClockModeConfig {
  uint8_t cellSize;
  uint16_t speed;
  uint8_t hourFormat;
};

struct DeviceParamsConfig {
  bool swapBlueGreen;
  bool swapBlueRed;
  bool clkphase;
  uint8_t displayBright;
  uint8_t brightnessDay;
  uint8_t brightnessNight;
  uint8_t displayRotation;
  uint8_t driver;
  uint32_t i2cSpeed;
  uint8_t E_pin;
  char nightStart[6];
  char nightEnd[6];
  char ntpServer[64];
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
  static void preloadDeviceParamsConfig();
  static void loadClockConfig();
  static void saveClockConfig();
  static void loadAnimClockConfig();
  static void saveAnimClockConfig();
  static void loadTetrisOverlayClockConfig();
  static void saveTetrisOverlayClockConfig();
  static void loadStaticImagePixels();
  static void saveStaticImagePixels();
  static void loadAnimImagePixels();
  static void saveAnimImagePixels();
  static void loadEyesConfig();
  static void saveEyesConfig();
  static void loadAmbientEffectConfig();
  static void saveAmbientEffectConfig();
  static void loadThemeConfig();
  static void saveThemeConfig();
  static void loadThemeClockConfig();
  static void saveThemeClockConfig();
  static void loadTetrisConfig();
  static void saveTetrisConfig();
  static void loadTetrisClockConfig();
  static void saveTetrisClockConfig();
  static void loadMazeConfig();
  static void saveMazeConfig();
  static void loadSnakeConfig();
  static void saveSnakeConfig();
  static void loadPlanetScreensaverConfig();
  static void savePlanetScreensaverConfig();
  static void loadDeviceParamsConfig();
  static void saveDeviceParamsConfig();
  static void loadPacmanRoute();
  static bool savePacmanRoute(const uint8_t* routeData, uint16_t routeLength);
  static void clearPacmanRoute();
  static void loadCanvasPixels();
  static void saveCanvasPixels();
  static void clearCanvasPixels();
  static void resetToDefault();

  static ClockConfig clockConfig;
  static ClockConfig animClockConfig;
  static ClockConfig tetrisOverlayClockConfig;
  static EyesConfig eyesConfig;
  static ThemeConfig themeConfig;
  static ClockConfig themeClockConfig;
  static TetrisModeConfig tetrisConfig;
  static TetrisClockModeConfig tetrisClockConfig;
  static MazeModeConfig mazeConfig;
  static SnakeModeConfig snakeConfig;
  static DeviceParamsConfig deviceParamsConfig;

  // 静态时钟的背景图片像素
  static PixelData* staticImagePixels;
  static int staticImagePixelCount;

  // 动态时钟的背景图片像素（如果不用 GIF 动画，可以用静态图）
  static PixelData* animImagePixels;
  static int animImagePixelCount;

  static uint8_t* pacmanRouteData;
  static uint16_t pacmanRouteLength;

  static Preferences preferences;

private:
};

#endif
