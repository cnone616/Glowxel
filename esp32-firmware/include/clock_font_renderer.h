#ifndef CLOCK_FONT_RENDERER_H
#define CLOCK_FONT_RENDERER_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

enum ClockFontId : uint8_t {
  CLOCK_FONT_CLASSIC_5X7 = 0,
  CLOCK_FONT_MINIMAL_3X5 = 1,
  CLOCK_FONT_ROUNDED_4X6 = 2,
  CLOCK_FONT_LCD_6X8 = 3,
  CLOCK_FONT_HOLLOW_5X7 = 4,
  CLOCK_FONT_SEVEN_SEG_5X7 = 5,
  CLOCK_FONT_ITALIC_6X7 = 6,
  CLOCK_FONT_RETRO_5X7 = 7,
};

bool clockFontIdFromString(const char* fontName, uint8_t& outFontId);
int getClockFontWidth(uint8_t fontId);
int getClockFontHeight(uint8_t fontId);
int getClockTextWidth(const char* text, uint8_t fontId, int scale);
int getClockTextHeight(uint8_t fontId, int scale);
void drawClockText(
  MatrixPanel_I2S_DMA* display,
  const char* text,
  int x,
  int y,
  uint16_t color,
  uint8_t fontId,
  int scale
);

#endif
