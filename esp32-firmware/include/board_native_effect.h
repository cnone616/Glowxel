#ifndef BOARD_NATIVE_EFFECT_H
#define BOARD_NATIVE_EFFECT_H

#include <Arduino.h>

enum BoardNativeMode : uint8_t {
  BOARD_NATIVE_NONE = 0,
  BOARD_NATIVE_TEXT_DISPLAY = 1,
  BOARD_NATIVE_WEATHER = 2,
  BOARD_NATIVE_COUNTDOWN = 3,
  BOARD_NATIVE_STOPWATCH = 4,
  BOARD_NATIVE_NOTIFICATION = 5
};

struct TextDisplayNativeConfig {
  String templateName;
  String text;
  uint8_t progress;
  uint8_t repeat;
  bool pushIcon;
  String icon;
  uint8_t speed;
  uint8_t colorR;
  uint8_t colorG;
  uint8_t colorB;
  uint8_t bgColorR;
  uint8_t bgColorG;
  uint8_t bgColorB;
};

struct WeatherBoardNativeConfig {
  String weatherType;
  String city;
  int16_t temperature;
  uint8_t humidity;
  String unit;
};

struct CountdownBoardNativeConfig {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t progress;
};

struct StopwatchBoardNativeConfig {
  uint16_t previewSeconds;
  uint8_t lapCount;
  bool showMilliseconds;
};

struct NotificationBoardNativeConfig {
  String repeatMode;
  String text;
  String icon;
  uint8_t accentR;
  uint8_t accentG;
  uint8_t accentB;
  String contentType;
  String textTemplate;
  String staticTemplate;
  String animationTemplate;
  uint8_t hour;
  uint8_t minute;
};

namespace BoardNativeEffect {
  void init();
  void deactivate();
  bool isActive();
  BoardNativeMode getMode();

  void applyTextDisplayConfig(const TextDisplayNativeConfig& config);
  void applyWeatherConfig(const WeatherBoardNativeConfig& config);
  void applyCountdownConfig(const CountdownBoardNativeConfig& config);
  void applyStopwatchConfig(const StopwatchBoardNativeConfig& config);
  void applyNotificationConfig(const NotificationBoardNativeConfig& config);

  void update();
  void render();

  const TextDisplayNativeConfig& getTextDisplayConfig();
  const WeatherBoardNativeConfig& getWeatherConfig();
  const CountdownBoardNativeConfig& getCountdownConfig();
  const StopwatchBoardNativeConfig& getStopwatchConfig();
  const NotificationBoardNativeConfig& getNotificationConfig();
  int getCountdownRemainingSeconds();
  unsigned long getStopwatchElapsedMs();
}

#endif
