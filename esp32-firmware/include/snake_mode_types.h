#ifndef SNAKE_MODE_TYPES_H
#define SNAKE_MODE_TYPES_H

#include <Arduino.h>

static constexpr size_t SNAKE_SKIN_TEXT_SIZE = 16;

struct SnakeModeConfig {
  uint8_t speed;
  uint8_t snakeWidth;
  uint8_t snakeColorR;
  uint8_t snakeColorG;
  uint8_t snakeColorB;
  uint8_t foodColorR;
  uint8_t foodColorG;
  uint8_t foodColorB;
  uint8_t font;
  bool showSeconds;
  char snakeSkin[SNAKE_SKIN_TEXT_SIZE];
};

#endif
