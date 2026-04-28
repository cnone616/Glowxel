#ifndef MAZE_EFFECT_H
#define MAZE_EFFECT_H

#include <Arduino.h>

#include "maze_mode_types.h"

namespace MazeEffect {
void init();
void deactivate();
void applyConfig(const MazeModeConfig& config);
void update();
void render();
bool isActive();
const char* getLastError();
const MazeModeConfig& getConfig();
}

#endif
