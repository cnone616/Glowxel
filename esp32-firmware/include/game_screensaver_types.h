#ifndef GAME_SCREENSAVER_TYPES_H
#define GAME_SCREENSAVER_TYPES_H

#include <Arduino.h>

enum GameScreensaverId : uint8_t {
  GAME_SCREENSAVER_MAZE = 0,
  GAME_SCREENSAVER_SNAKE = 1,
  GAME_SCREENSAVER_TETRIS_GAME = 2,
  GAME_SCREENSAVER_PING_PONG = 3
};

enum GameScreensaverMazeSizeMode : uint8_t {
  GAME_SCREENSAVER_MAZE_WIDE = 0,
  GAME_SCREENSAVER_MAZE_DENSE = 1
};

struct GameScreensaverConfig {
  uint8_t game;
  uint8_t speed;
  uint8_t snakeWidth;
  uint8_t mazeSizeMode;
  uint8_t cellSize;
  bool showClock;
};

#endif
