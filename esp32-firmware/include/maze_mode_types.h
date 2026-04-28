#ifndef MAZE_MODE_TYPES_H
#define MAZE_MODE_TYPES_H

#include <Arduino.h>

static constexpr size_t MAZE_HEX_COLOR_TEXT_SIZE = 8;

enum MazeSizeMode : uint8_t {
  MAZE_SIZE_WIDE = 0,
  MAZE_SIZE_DENSE = 1
};

struct MazeModeConfig {
  uint8_t speed;
  uint8_t mazeSizeMode;
  bool showClock;
  char panelBgColor[MAZE_HEX_COLOR_TEXT_SIZE];
  char borderColor[MAZE_HEX_COLOR_TEXT_SIZE];
  char timeColor[MAZE_HEX_COLOR_TEXT_SIZE];
  char dateColor[MAZE_HEX_COLOR_TEXT_SIZE];
  char generationPathColor[MAZE_HEX_COLOR_TEXT_SIZE];
  char searchVisitedColor[MAZE_HEX_COLOR_TEXT_SIZE];
  char searchFrontierColor[MAZE_HEX_COLOR_TEXT_SIZE];
  char solvedPathStartColor[MAZE_HEX_COLOR_TEXT_SIZE];
  char solvedPathEndColor[MAZE_HEX_COLOR_TEXT_SIZE];
};

#endif
