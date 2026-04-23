#ifndef GAME_SCREENSAVER_EFFECT_H
#define GAME_SCREENSAVER_EFFECT_H

#include <Arduino.h>
#include "game_screensaver_types.h"

namespace GameScreensaverEffect {
  void init();
  void deactivate();
  void applyConfig(const GameScreensaverConfig& config);
  void update();
  void render();
  bool isActive();
  const GameScreensaverConfig& getConfig();
}

#endif
