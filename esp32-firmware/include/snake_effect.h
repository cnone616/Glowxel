#ifndef SNAKE_EFFECT_H
#define SNAKE_EFFECT_H

#include <Arduino.h>

#include "snake_mode_types.h"

namespace SnakeEffect {
void init();
void deactivate();
void applyConfig(const SnakeModeConfig& config);
void update();
void render();
bool isActive();
const char* getLastError();
const SnakeModeConfig& getConfig();
}

#endif
