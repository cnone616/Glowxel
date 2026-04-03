#ifndef EYES_EFFECT_H
#define EYES_EFFECT_H

#include <Arduino.h>
#include "config_manager.h"

namespace EyesEffect {
  void init();
  void applyConfig(const EyesConfig& config);
  void render();
  bool triggerAction(const char* action);
}

#endif
