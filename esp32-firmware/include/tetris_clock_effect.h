#ifndef TETRIS_CLOCK_EFFECT_H
#define TETRIS_CLOCK_EFFECT_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

class TetrisClockEffect {
public:
  static void init(int speed = 150, int cellSize = 2, int hourFormat = 24);
  static void deactivate();
  static void update();
  static void render(MatrixPanel_I2S_DMA* display);

  static bool isActive;
  static int dropSpeed;
  static int cellSize;
  static int hourFormat;
};

#endif
