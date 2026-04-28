#ifndef BOARD_NATIVE_EFFECT_H
#define BOARD_NATIVE_EFFECT_H

#include <Arduino.h>

enum BoardNativeMode : uint8_t {
  BOARD_NATIVE_NONE = 0,
  BOARD_NATIVE_TEXT_DISPLAY = 1,
  BOARD_NATIVE_PLANET = 2
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

struct PlanetScreensaverNativeConfig {
  char preset[20];
  char size[12];
  char direction[12];
  uint8_t speed;
  uint32_t seed;
  uint32_t colorSeed;
  uint8_t planetX;
  uint8_t planetY;
  uint8_t font;
  bool showSeconds;
  struct {
    bool show;
    uint8_t fontSize;
    uint8_t x;
    uint8_t y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } time;
};

namespace BoardNativeEffect {
  void init();
  void deactivate();
  bool isActive();
  BoardNativeMode getMode();

  void applyTextDisplayConfig(const TextDisplayNativeConfig& config);
  void setPlanetScreensaverConfig(const PlanetScreensaverNativeConfig& config);
  void applyPlanetScreensaverConfig(const PlanetScreensaverNativeConfig& config);

  void update();
  void render();

  const TextDisplayNativeConfig& getTextDisplayConfig();
  const PlanetScreensaverNativeConfig& getPlanetScreensaverConfig();
}

#endif
