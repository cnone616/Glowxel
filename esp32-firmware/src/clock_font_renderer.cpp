#include "clock_font_renderer.h"

namespace {

struct ClockFontDefinition {
  uint8_t width;
  uint8_t height;
  uint8_t spacing;
  const uint8_t (*glyphs)[9];
};

static const uint8_t FONT_CLASSIC_5X7[11][9] = {
  {14, 17, 19, 21, 25, 17, 14, 0, 0},
  {4, 12, 4, 4, 4, 4, 14, 0, 0},
  {14, 17, 1, 2, 4, 8, 31, 0, 0},
  {30, 1, 1, 14, 1, 1, 30, 0, 0},
  {2, 6, 10, 18, 31, 2, 2, 0, 0},
  {31, 16, 30, 1, 1, 17, 14, 0, 0},
  {6, 8, 16, 30, 17, 17, 14, 0, 0},
  {31, 1, 2, 4, 8, 8, 8, 0, 0},
  {14, 17, 17, 14, 17, 17, 14, 0, 0},
  {14, 17, 17, 15, 1, 2, 12, 0, 0},
  {0, 12, 12, 0, 12, 12, 0, 0, 0},
};

static const uint8_t FONT_MINIMAL_3X5[11][9] = {
  {7, 5, 5, 5, 7, 0, 0, 0, 0},
  {2, 6, 2, 2, 7, 0, 0, 0, 0},
  {7, 1, 7, 4, 7, 0, 0, 0, 0},
  {7, 1, 7, 1, 7, 0, 0, 0, 0},
  {5, 5, 7, 1, 1, 0, 0, 0, 0},
  {7, 4, 7, 1, 7, 0, 0, 0, 0},
  {7, 4, 7, 5, 7, 0, 0, 0, 0},
  {7, 1, 2, 2, 2, 0, 0, 0, 0},
  {7, 5, 7, 5, 7, 0, 0, 0, 0},
  {7, 5, 7, 1, 7, 0, 0, 0, 0},
  {0, 2, 0, 2, 0, 0, 0, 0, 0},
};

static const uint8_t FONT_ROUNDED_4X6[11][9] = {
  {6, 9, 9, 9, 9, 6, 0, 0, 0},
  {4, 12, 4, 4, 4, 14, 0, 0, 0},
  {6, 9, 1, 2, 4, 15, 0, 0, 0},
  {14, 1, 6, 1, 1, 14, 0, 0, 0},
  {2, 6, 10, 15, 2, 2, 0, 0, 0},
  {15, 8, 14, 1, 9, 6, 0, 0, 0},
  {6, 8, 14, 9, 9, 6, 0, 0, 0},
  {15, 1, 2, 4, 4, 4, 0, 0, 0},
  {6, 9, 6, 9, 9, 6, 0, 0, 0},
  {6, 9, 9, 7, 1, 6, 0, 0, 0},
  {0, 4, 0, 0, 4, 0, 0, 0, 0},
};

static const uint8_t FONT_LCD_6X8[11][9] = {
  {30, 51, 33, 33, 33, 33, 51, 30, 0},
  {12, 28, 12, 12, 12, 12, 12, 63, 0},
  {30, 51, 3, 6, 12, 24, 48, 63, 0},
  {62, 3, 3, 14, 3, 3, 3, 62, 0},
  {6, 14, 22, 38, 63, 6, 6, 6, 0},
  {63, 48, 48, 62, 3, 3, 51, 30, 0},
  {14, 24, 48, 62, 51, 51, 51, 30, 0},
  {63, 3, 6, 12, 24, 24, 24, 24, 0},
  {30, 51, 51, 30, 51, 51, 51, 30, 0},
  {30, 51, 51, 31, 3, 6, 12, 56, 0},
  {0, 12, 12, 0, 0, 12, 12, 0, 0},
};

static const uint8_t FONT_HOLLOW_5X7[11][9] = {
  {14, 17, 17, 17, 17, 17, 14, 0, 0},
  {4, 12, 4, 4, 4, 4, 4, 0, 0},
  {14, 17, 1, 2, 4, 8, 31, 0, 0},
  {30, 1, 1, 14, 1, 1, 30, 0, 0},
  {2, 6, 10, 18, 31, 2, 2, 0, 0},
  {31, 16, 30, 1, 1, 17, 14, 0, 0},
  {6, 8, 16, 30, 17, 17, 14, 0, 0},
  {31, 1, 2, 4, 8, 8, 8, 0, 0},
  {14, 17, 17, 14, 17, 17, 14, 0, 0},
  {14, 17, 17, 15, 1, 2, 12, 0, 0},
  {0, 8, 0, 0, 0, 8, 0, 0, 0},
};

static const uint8_t FONT_SEVEN_SEG_5X7[11][9] = {
  {31, 17, 17, 17, 17, 17, 31, 0, 0},
  {1, 1, 1, 1, 1, 1, 1, 0, 0},
  {31, 1, 1, 31, 16, 16, 31, 0, 0},
  {31, 1, 1, 31, 1, 1, 31, 0, 0},
  {17, 17, 17, 31, 1, 1, 1, 0, 0},
  {31, 16, 16, 31, 1, 1, 31, 0, 0},
  {31, 16, 16, 31, 17, 17, 31, 0, 0},
  {31, 1, 1, 1, 1, 1, 1, 0, 0},
  {31, 17, 17, 31, 17, 17, 31, 0, 0},
  {31, 17, 17, 31, 1, 1, 31, 0, 0},
  {0, 4, 0, 0, 0, 4, 0, 0, 0},
};

static const uint8_t FONT_ITALIC_6X7[11][9] = {
  {15, 17, 18, 18, 34, 60, 0, 0, 0},
  {4, 12, 4, 4, 8, 8, 28, 0, 0},
  {14, 17, 2, 4, 8, 16, 62, 0, 0},
  {30, 1, 2, 12, 2, 1, 30, 0, 0},
  {2, 6, 10, 18, 31, 4, 4, 0, 0},
  {31, 16, 30, 1, 1, 18, 12, 0, 0},
  {6, 8, 16, 30, 17, 18, 12, 0, 0},
  {31, 1, 2, 4, 8, 8, 16, 0, 0},
  {14, 17, 18, 12, 18, 33, 30, 0, 0},
  {14, 17, 18, 15, 2, 4, 24, 0, 0},
  {0, 8, 0, 0, 16, 0, 0, 0, 0},
};

static const uint8_t FONT_RETRO_5X7[11][9] = {
  {31, 17, 19, 21, 25, 17, 31, 0, 0},
  {6, 14, 6, 6, 6, 6, 31, 0, 0},
  {31, 1, 1, 31, 16, 16, 31, 0, 0},
  {31, 1, 1, 31, 1, 1, 31, 0, 0},
  {17, 17, 17, 31, 1, 1, 1, 0, 0},
  {31, 16, 16, 31, 1, 1, 31, 0, 0},
  {31, 16, 16, 31, 17, 17, 31, 0, 0},
  {31, 1, 1, 1, 1, 1, 1, 0, 0},
  {31, 17, 17, 31, 17, 17, 31, 0, 0},
  {31, 17, 17, 31, 1, 1, 31, 0, 0},
  {0, 12, 0, 0, 0, 12, 0, 0, 0},
};

static const uint8_t FONT_NEO_BOLD_6X8[11][9] = {
  {30, 63, 55, 63, 59, 51, 30, 28, 0},
  {12, 28, 12, 12, 12, 12, 30, 28, 0},
  {30, 63, 3, 6, 12, 24, 63, 62, 0},
  {62, 63, 3, 30, 3, 3, 62, 60, 0},
  {6, 14, 30, 54, 63, 62, 6, 4, 0},
  {63, 62, 62, 63, 3, 51, 30, 28, 0},
  {14, 28, 48, 62, 51, 51, 30, 28, 0},
  {63, 63, 6, 12, 24, 24, 24, 16, 0},
  {30, 63, 51, 62, 51, 51, 30, 28, 0},
  {30, 63, 51, 63, 3, 6, 28, 24, 0},
  {0, 12, 0, 0, 0, 0, 12, 0, 0},
};

static const uint8_t FONT_STENCIL_CLASSIC_6X8[11][9] = {
  {30, 35, 51, 63, 59, 51, 26, 0, 0},
  {12, 24, 12, 8, 12, 8, 30, 0, 0},
  {30, 51, 3, 6, 8, 24, 59, 0, 0},
  {62, 3, 3, 26, 3, 3, 62, 0, 0},
  {6, 14, 26, 54, 59, 6, 2, 0, 0},
  {63, 48, 62, 3, 3, 51, 30, 0, 0},
  {14, 8, 48, 62, 51, 51, 26, 0, 0},
  {63, 3, 6, 8, 24, 24, 24, 0, 0},
  {30, 51, 51, 30, 51, 51, 26, 0, 0},
  {30, 35, 51, 27, 3, 2, 28, 0, 0},
  {0, 12, 0, 0, 0, 0, 12, 0, 0},
};

static const uint8_t FONT_WIRE_LCD_6X8[11][9] = {
  {30, 51, 33, 33, 33, 33, 51, 30, 0},
  {12, 20, 12, 12, 12, 12, 12, 63, 0},
  {30, 51, 3, 6, 12, 24, 48, 63, 0},
  {62, 3, 3, 14, 3, 3, 3, 62, 0},
  {6, 10, 22, 38, 57, 6, 6, 6, 0},
  {63, 48, 48, 62, 3, 3, 51, 30, 0},
  {14, 24, 48, 46, 51, 51, 51, 30, 0},
  {63, 3, 6, 12, 24, 24, 24, 24, 0},
  {30, 51, 51, 30, 51, 51, 51, 30, 0},
  {30, 51, 51, 29, 3, 6, 12, 56, 0},
  {0, 12, 0, 0, 0, 0, 12, 0, 0},
};

static const uint8_t FONT_FRAME_LCD_7X9[11][9] = {
  {30, 30, 115, 97, 117, 97, 97, 115, 30},
  {12, 12, 20, 12, 46, 12, 12, 12, 127},
  {30, 30, 115, 3, 22, 12, 24, 112, 127},
  {126, 126, 3, 3, 46, 3, 3, 3, 126},
  {6, 6, 10, 22, 118, 121, 6, 6, 6},
  {127, 79, 80, 80, 126, 3, 3, 115, 30},
  {14, 14, 24, 112, 94, 83, 83, 115, 30},
  {127, 125, 3, 6, 46, 24, 24, 24, 24},
  {30, 30, 115, 115, 30, 115, 83, 115, 30},
  {30, 30, 115, 115, 63, 3, 6, 12, 120},
  {0, 24, 0, 0, 0, 0, 0, 24, 0},
};

static const uint8_t FONT_SLANT_RETRO_7X7[11][9] = {
  {31, 34, 38, 42, 100, 68, 124, 0, 0},
  {6, 28, 12, 12, 24, 24, 124, 0, 0},
  {31, 2, 2, 62, 64, 64, 124, 0, 0},
  {31, 2, 2, 62, 4, 4, 124, 0, 0},
  {17, 34, 34, 62, 4, 4, 4, 0, 0},
  {31, 32, 32, 62, 4, 4, 124, 0, 0},
  {31, 32, 32, 62, 68, 68, 124, 0, 0},
  {31, 2, 2, 2, 4, 4, 4, 0, 0},
  {31, 34, 34, 62, 68, 68, 124, 0, 0},
  {31, 34, 34, 62, 4, 4, 124, 0, 0},
  {0, 24, 0, 0, 0, 24, 0, 0, 0},
};

static const uint8_t FONT_PRISM_SEG_6X8[11][9] = {
  {31, 31, 17, 34, 42, 34, 62, 28, 0},
  {1, 1, 1, 2, 10, 2, 2, 0, 0},
  {31, 15, 1, 62, 56, 32, 62, 28, 0},
  {31, 15, 1, 54, 26, 2, 62, 28, 0},
  {17, 17, 17, 62, 26, 2, 2, 0, 0},
  {31, 30, 16, 54, 26, 2, 62, 28, 0},
  {31, 30, 16, 62, 58, 34, 62, 28, 0},
  {31, 15, 1, 2, 10, 2, 2, 0, 0},
  {31, 31, 17, 62, 58, 34, 62, 28, 0},
  {31, 31, 17, 54, 26, 2, 62, 28, 0},
  {0, 12, 0, 0, 0, 0, 12, 0, 0},
};

static const uint8_t FONT_ARC_ROUND_5X7[11][9] = {
  {14, 18, 18, 18, 18, 30, 12, 0, 0},
  {12, 24, 24, 8, 8, 30, 28, 0, 0},
  {14, 18, 18, 6, 8, 31, 30, 0, 0},
  {30, 18, 14, 2, 2, 30, 28, 0, 0},
  {6, 12, 20, 30, 22, 6, 4, 0, 0},
  {31, 18, 28, 18, 18, 30, 12, 0, 0},
  {14, 16, 28, 18, 18, 30, 12, 0, 0},
  {31, 18, 6, 8, 8, 12, 8, 0, 0},
  {14, 18, 30, 18, 18, 30, 12, 0, 0},
  {14, 18, 18, 30, 2, 14, 12, 0, 0},
  {0, 4, 0, 0, 0, 4, 0, 0, 0},
};

static const uint8_t FONT_COMPACT_MINIMAL_4X6[11][9] = {
  {15, 15, 13, 13, 13, 15, 0, 0, 0},
  {2, 2, 14, 2, 2, 15, 0, 0, 0},
  {15, 15, 1, 15, 12, 15, 0, 0, 0},
  {15, 15, 1, 15, 1, 15, 0, 0, 0},
  {13, 13, 13, 15, 1, 1, 0, 0, 0},
  {15, 15, 12, 15, 1, 15, 0, 0, 0},
  {15, 15, 12, 15, 13, 15, 0, 0, 0},
  {15, 15, 1, 2, 2, 2, 0, 0, 0},
  {15, 15, 13, 15, 13, 15, 0, 0, 0},
  {15, 15, 13, 15, 1, 15, 0, 0, 0},
  {0, 4, 0, 0, 4, 0, 0, 0, 0},
};

static const uint8_t FONT_PIXEL_RAIL_5X8[11][9] = {
  {31, 31, 31, 25, 25, 25, 25, 31, 0},
  {6, 6, 30, 30, 22, 6, 6, 31, 0},
  {31, 31, 17, 1, 31, 25, 24, 31, 0},
  {31, 31, 31, 1, 31, 17, 1, 31, 0},
  {25, 25, 25, 31, 31, 17, 1, 1, 0},
  {31, 31, 25, 24, 31, 17, 1, 31, 0},
  {31, 31, 31, 24, 31, 25, 25, 31, 0},
  {31, 31, 17, 15, 7, 6, 6, 6, 0},
  {31, 31, 25, 25, 31, 25, 25, 31, 0},
  {31, 31, 31, 25, 31, 17, 1, 31, 0},
  {0, 4, 0, 0, 0, 0, 4, 0, 0},
};

static const ClockFontDefinition CLOCK_FONTS[] = {
  {5, 7, 1, FONT_CLASSIC_5X7},
  {3, 5, 1, FONT_MINIMAL_3X5},
  {4, 6, 1, FONT_ROUNDED_4X6},
  {6, 8, 1, FONT_LCD_6X8},
  {5, 7, 1, FONT_HOLLOW_5X7},
  {5, 7, 1, FONT_SEVEN_SEG_5X7},
  {6, 7, 1, FONT_ITALIC_6X7},
  {5, 7, 1, FONT_RETRO_5X7},
  {6, 8, 1, FONT_NEO_BOLD_6X8},
  {6, 8, 1, FONT_STENCIL_CLASSIC_6X8},
  {6, 8, 1, FONT_WIRE_LCD_6X8},
  {7, 9, 1, FONT_FRAME_LCD_7X9},
  {7, 7, 1, FONT_SLANT_RETRO_7X7},
  {6, 8, 1, FONT_PRISM_SEG_6X8},
  {5, 7, 1, FONT_ARC_ROUND_5X7},
  {4, 6, 1, FONT_COMPACT_MINIMAL_4X6},
  {5, 8, 1, FONT_PIXEL_RAIL_5X8},
};

static const char* CLOCK_FONT_NAMES[] = {
  "classic_5x7",
  "minimal_3x5",
  "rounded_4x6",
  "lcd_6x8",
  "hollow_5x7",
  "seven_seg_5x7",
  "italic_6x7",
  "retro_5x7",
  "neo_bold_6x8",
  "stencil_classic_6x8",
  "wire_lcd_6x8",
  "frame_lcd_7x9",
  "slant_retro_7x7",
  "prism_seg_6x8",
  "arc_round_5x7",
  "compact_minimal_4x6",
  "pixel_rail_5x8",
};

static const uint8_t BASE_ALPHA_3X5[26][5] = {
  {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {7, 4, 4, 4, 7}, {6, 5, 5, 5, 6},
  {7, 4, 7, 4, 7}, {7, 4, 7, 4, 4}, {7, 4, 5, 5, 7}, {5, 5, 7, 5, 5},
  {7, 2, 2, 2, 7}, {3, 1, 1, 5, 7}, {5, 6, 4, 6, 5}, {4, 4, 4, 4, 7},
  {5, 7, 5, 5, 5}, {5, 7, 7, 5, 5}, {7, 5, 5, 5, 7}, {7, 5, 7, 4, 4},
  {7, 5, 5, 7, 1}, {7, 5, 7, 6, 5}, {7, 4, 7, 1, 7}, {7, 2, 2, 2, 2},
  {5, 5, 5, 5, 7}, {5, 5, 5, 2, 2}, {5, 5, 5, 7, 5}, {5, 5, 2, 5, 5},
  {5, 5, 2, 2, 2}, {7, 1, 2, 4, 7},
};

static const ClockFontDefinition& getClockFontDefinition(uint8_t fontId) {
  return CLOCK_FONTS[fontId];
}

static int glyphIndex(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c == ':') {
    return 10;
  }
  return -1;
}

static void drawScaledBlock(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  uint16_t color,
  int scale
) {
  for (int sy = 0; sy < scale; sy++) {
    for (int sx = 0; sx < scale; sx++) {
      display->drawPixel(x + sx, y + sy, color);
    }
  }
}

static void drawDigitGlyph(
  MatrixPanel_I2S_DMA* display,
  const ClockFontDefinition& font,
  const uint8_t* rows,
  int x,
  int y,
  uint16_t color,
  int scale
) {
  for (int row = 0; row < font.height; row++) {
    uint8_t bits = rows[row];
    for (int col = 0; col < font.width; col++) {
      uint8_t mask = 1 << (font.width - col - 1);
      if (bits & mask) {
        drawScaledBlock(display, x + col * scale, y + row * scale, color, scale);
      }
    }
  }
}

static void drawDashGlyph(
  MatrixPanel_I2S_DMA* display,
  const ClockFontDefinition& font,
  int x,
  int y,
  uint16_t color,
  int scale
) {
  int row = font.height / 2;
  for (int col = 0; col < font.width; col++) {
    drawScaledBlock(display, x + col * scale, y + row * scale, color, scale);
  }
}

static void drawScaled3x5Letter(
  MatrixPanel_I2S_DMA* display,
  const uint8_t* glyph,
  const ClockFontDefinition& font,
  int x,
  int y,
  uint16_t color,
  int scale
) {
  for (int row = 0; row < font.height; row++) {
    int sourceRow = (row * 5) / font.height;
    for (int col = 0; col < font.width; col++) {
      int sourceCol = (col * 3) / font.width;
      uint8_t mask = 1 << (2 - sourceCol);
      if (glyph[sourceRow] & mask) {
        drawScaledBlock(display, x + col * scale, y + row * scale, color, scale);
      }
    }
  }
}

}  // namespace

bool clockFontIdFromString(const char* fontName, uint8_t& outFontId) {
  for (uint8_t i = 0; i < sizeof(CLOCK_FONT_NAMES) / sizeof(CLOCK_FONT_NAMES[0]); i++) {
    if (strcmp(fontName, CLOCK_FONT_NAMES[i]) == 0) {
      outFontId = i;
      return true;
    }
  }
  return false;
}

int getClockFontWidth(uint8_t fontId) {
  return getClockFontDefinition(fontId).width;
}

int getClockFontHeight(uint8_t fontId) {
  return getClockFontDefinition(fontId).height;
}

int getClockTextWidth(const char* text, uint8_t fontId, int scale) {
  if (text == nullptr || text[0] == '\0') {
    return 0;
  }

  const ClockFontDefinition& font = getClockFontDefinition(fontId);
  int length = strlen(text);
  return length * (font.width + font.spacing) * scale - font.spacing * scale;
}

int getClockTextHeight(uint8_t fontId, int scale) {
  return getClockFontDefinition(fontId).height * scale;
}

void drawClockText(
  MatrixPanel_I2S_DMA* display,
  const char* text,
  int x,
  int y,
  uint16_t color,
  uint8_t fontId,
  int scale
) {
  const ClockFontDefinition& font = getClockFontDefinition(fontId);
  int cursorX = x;

  for (int i = 0; text[i]; i++) {
    int idx = glyphIndex(text[i]);
    if (idx >= 0) {
      drawDigitGlyph(display, font, font.glyphs[idx], cursorX, y, color, scale);
    } else if (text[i] == '-') {
      drawDashGlyph(display, font, cursorX, y, color, scale);
    } else if (text[i] != ' ') {
      char normalized = text[i];
      if (normalized >= 'a' && normalized <= 'z') {
        normalized = normalized - 'a' + 'A';
      }
      if (normalized >= 'A' && normalized <= 'Z') {
        drawScaled3x5Letter(
          display,
          BASE_ALPHA_3X5[normalized - 'A'],
          font,
          cursorX,
          y,
          color,
          scale
        );
      }
    }

    cursorX += (font.width + font.spacing) * scale;
  }
}
