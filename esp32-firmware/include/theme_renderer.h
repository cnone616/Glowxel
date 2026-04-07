#ifndef THEME_RENDERER_H
#define THEME_RENDERER_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config_manager.h"

uint16_t getThemeRefreshIntervalMs(const char* themeId);
bool themeUsesStoredImage(const char* themeId);
void resetThemeRuntimeState(const char* themeId);
void renderThemeFrame(
  MatrixPanel_I2S_DMA* display,
  const char* themeId,
  const struct tm* timeinfo,
  const PixelData* imagePixels,
  int imagePixelCount
);

#endif
