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
  {14, 17, 18, 20, 24, 32, 60, 0, 0},
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

static const ClockFontDefinition CLOCK_FONTS[] = {
  {5, 7, 1, FONT_CLASSIC_5X7},
  {3, 5, 1, FONT_MINIMAL_3X5},
  {4, 6, 1, FONT_ROUNDED_4X6},
  {6, 8, 1, FONT_LCD_6X8},
  {5, 7, 1, FONT_HOLLOW_5X7},
  {5, 7, 1, FONT_SEVEN_SEG_5X7},
  {6, 7, 1, FONT_ITALIC_6X7},
  {5, 7, 1, FONT_RETRO_5X7},
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
