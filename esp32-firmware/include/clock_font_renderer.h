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
  CLOCK_FONT_NEO_BOLD_6X8 = 8,
  CLOCK_FONT_STENCIL_CLASSIC_6X8 = 9,
  CLOCK_FONT_WIRE_LCD_6X8 = 10,
  CLOCK_FONT_FRAME_LCD_7X9 = 11,
  CLOCK_FONT_SLANT_RETRO_7X7 = 12,
  CLOCK_FONT_PRISM_SEG_6X8 = 13,
  CLOCK_FONT_ARC_ROUND_5X7 = 14,
  CLOCK_FONT_COMPACT_MINIMAL_4X6 = 15,
  CLOCK_FONT_PIXEL_RAIL_5X8 = 16,
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
