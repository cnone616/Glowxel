#include "theme_renderer.h"

#include <math.h>
#include <pgmspace.h>
#include <string.h>
#include <WiFi.h>
#include "clock_font_renderer.h"
#include "config_manager.h"
#include "display_manager.h"
#include "theme_assets/common/clockwise_icons.h"
#include "theme_assets/common/picopixel.h"
#include "theme_assets/clockwise/mario/Super_Mario_Bros__24pt7b.h"
#include "theme_assets/clockwise/mario/assets.h"
#include "theme_assets/clockwise/pacman/hour_font.h"
#include "theme_assets/clockwise/castlevania_assets.h"
#include "theme_assets/clockwise/pokedex/PKMN_RBYGSC4pt7b.h"
#include "theme_assets/clockwise/pokedex/assets.h"
#include "theme_assets/clockwise/words/DateI18nEN.h"
#include "theme_assets/clockwise/words/atari.h"
#include "theme_assets/clockwise/words/hour8pt7b.h"
#include "theme_assets/clockwise/words/minute7pt7b.h"
#include "theme_assets/clockwise/words/small4pt7b.h"
#include "theme_assets/clockwise/worldmap/worldmap.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

enum PacmanMapBlock {
  PACMAN_BLOCK_EMPTY = 0,
  PACMAN_BLOCK_FOOD = 1,
  PACMAN_BLOCK_WALL = 2,
  PACMAN_BLOCK_GATE = 3,
  PACMAN_BLOCK_SUPER_FOOD = 4,
  PACMAN_BLOCK_CLOCK = 5,
  PACMAN_BLOCK_GHOST = 6,
  PACMAN_BLOCK_START = 7,
  PACMAN_BLOCK_OUT_OF_MAP = 99
};

enum PacmanDirection {
  PACMAN_DIRECTION_RIGHT = 0,
  PACMAN_DIRECTION_DOWN = 1,
  PACMAN_DIRECTION_LEFT = 2,
  PACMAN_DIRECTION_UP = 3
};

constexpr uint16_t kPacmanRouteCapacity = 384;

struct PacmanThemeState {
  bool initialized;
  uint8_t map[12][12];
  int x;
  int y;
  PacmanDirection direction;
  bool mouthOpen;
  bool showSeconds;
  bool invincible;
  unsigned long invincibleStartAt;
  unsigned long lastSecondBlinkAt;
  unsigned long stepCount;
  bool showingWin;
  uint8_t winPhase;
  unsigned long winPhaseStartedAt;
  uint8_t routeDirections[kPacmanRouteCapacity];
  uint16_t routeLength;
  uint16_t routeIndex;
};

struct PacmanTargetCell {
  int x;
  int y;
};

struct PacmanRoutePlan {
  uint8_t quadrantOrder[4];
  uint8_t variants[4];
};

struct PokedexThemeState {
  bool initialized;
  int lastMinute;
  int pokemonIndex;
  int blinkSecond;
  uint16_t blinkColor;
};

struct MarioThemeState {
  bool initialized;
  bool jumping;
  bool descending;
  int marioX;
  int marioY;
  int marioWidth;
  int marioHeight;
  int baseMarioY;
  int hourBlockY;
  int minuteBlockY;
  unsigned long jumpStartedAt;
  unsigned long lastFrameAt;
};

enum CanvasFontKind {
  CANVAS_FONT_DEFAULT = 0,
  CANVAS_FONT_PICOPIXEL = 1,
  CANVAS_FONT_SQUARE = 2,
  CANVAS_FONT_BIG = 3,
  CANVAS_FONT_MEDIUM = 4
};

enum CanvasElementType {
  CANVAS_ELEMENT_TEXT = 0,
  CANVAS_ELEMENT_DATETIME = 1,
  CANVAS_ELEMENT_FILLRECT = 2,
  CANVAS_ELEMENT_RECT = 3,
  CANVAS_ELEMENT_LINE = 4,
  CANVAS_ELEMENT_STORED_IMAGE = 5
};

struct CanvasElement {
  CanvasElementType type;
  const char* text;
  const char* format;
  int16_t x;
  int16_t y;
  int16_t x1;
  int16_t y1;
  uint8_t width;
  uint8_t height;
  uint16_t color;
  uint16_t fgColor;
  uint16_t bgColor;
  CanvasFontKind font;
};

struct CanvasSpriteFrame {
  const uint16_t* bitmap;
  uint8_t width;
  uint8_t height;
  uint16_t transparentColor;
};

struct CanvasSpriteDef {
  int8_t x;
  int8_t y;
  int8_t moveTargetX;
  int8_t moveTargetY;
  uint16_t frameDelayMs;
  uint16_t loopDelayMs;
  uint16_t moveStartTimeMs;
  uint16_t moveDurationMs;
  bool shouldReturnToOrigin;
  const CanvasSpriteFrame* frames;
  uint8_t frameCount;
};

const uint8_t kPacmanMapTemplate[12][12] = {
  {4, 1, 1, 1, 1, 1, 7, 1, 1, 1, 1, 4},
  {1, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 1},
  {1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1},
  {2, 1, 5, 5, 5, 5, 5, 5, 5, 5, 1, 2},
  {2, 1, 5, 5, 5, 5, 5, 5, 5, 5, 1, 2},
  {3, 1, 5, 5, 5, 5, 5, 5, 5, 5, 1, 3},
  {3, 1, 5, 5, 5, 5, 5, 5, 5, 5, 1, 3},
  {2, 1, 5, 5, 5, 5, 5, 5, 5, 5, 1, 2},
  {2, 1, 5, 5, 5, 5, 5, 5, 5, 5, 1, 2},
  {1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1},
  {1, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 1},
  {4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4}
};

constexpr unsigned long kPacmanWinDurationMs = 3000;
constexpr unsigned long kPacmanWinColorShiftMs = 120;
const uint8_t kPacmanWinPalette[][3] = {
  {255, 214, 64},
  {255, 124, 94},
  {255, 92, 214},
  {112, 214, 255},
  {128, 255, 118},
  {255, 255, 255}
};
const uint8_t kPacmanRecordedRoute[] = {
  PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_DOWN,
  PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT,
  PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_RIGHT,
  PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT,
  PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_DOWN,
  PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_DOWN,
  PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_DOWN,
  PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_RIGHT,
  PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_DOWN,
  PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_RIGHT,
  PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_DOWN, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_LEFT, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_UP, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_LEFT,
  PACMAN_DIRECTION_UP, PACMAN_DIRECTION_RIGHT, PACMAN_DIRECTION_UP, PACMAN_DIRECTION_UP
};
const PacmanRoutePlan kPacmanRoutePlans[] = {
  {{1, 0, 2, 3}, {0, 1, 1, 0}},
  {{0, 2, 1, 3}, {1, 1, 1, 0}},
  {{0, 1, 3, 2}, {1, 0, 0, 1}},
  {{1, 3, 0, 2}, {0, 1, 1, 1}},
  {{1, 3, 0, 2}, {0, 0, 0, 1}},
  {{1, 3, 0, 2}, {0, 0, 0, 0}},
  {{0, 2, 1, 3}, {1, 0, 0, 0}}
};

const char* kMonthNames[] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

const uint16_t kCanvasTransparent = 0xF81F;
const uint16_t kCanvasSparkFrame0[] PROGMEM = {
  kCanvasTransparent, kCanvasTransparent, 0xFFFF, kCanvasTransparent, kCanvasTransparent,
  kCanvasTransparent, 0x24FE, 0xFFFF, 0x24FE, kCanvasTransparent,
  0xFFE0, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFE0,
  kCanvasTransparent, 0x24FE, 0xFFFF, 0x24FE, kCanvasTransparent,
  kCanvasTransparent, kCanvasTransparent, 0xFFFF, kCanvasTransparent, kCanvasTransparent
};
const uint16_t kCanvasSparkFrame1[] PROGMEM = {
  kCanvasTransparent, kCanvasTransparent, 0x24FE, kCanvasTransparent, kCanvasTransparent,
  kCanvasTransparent, kCanvasTransparent, 0xFFFF, kCanvasTransparent, kCanvasTransparent,
  0x24FE, 0xFFFF, 0xFFE0, 0xFFFF, 0x24FE,
  kCanvasTransparent, kCanvasTransparent, 0xFFFF, kCanvasTransparent, kCanvasTransparent,
  kCanvasTransparent, kCanvasTransparent, 0x24FE, kCanvasTransparent, kCanvasTransparent
};
const uint16_t kCanvasScanFrame0[] PROGMEM = {
  kCanvasTransparent, 0x24FE, kCanvasTransparent,
  0x24FE, 0xFFFF, 0x24FE,
  kCanvasTransparent, 0x24FE, kCanvasTransparent
};
const uint16_t kCanvasScanFrame1[] PROGMEM = {
  kCanvasTransparent, 0x7E5F, kCanvasTransparent,
  0x24FE, 0xFFFF, 0x24FE,
  kCanvasTransparent, 0x7E5F, kCanvasTransparent
};
const CanvasSpriteFrame kCanvasSparkFrames[] = {
  {kCanvasSparkFrame0, 5, 5, kCanvasTransparent},
  {kCanvasSparkFrame1, 5, 5, kCanvasTransparent}
};
const CanvasSpriteFrame kCanvasScanFrames[] = {
  {kCanvasScanFrame0, 3, 3, kCanvasTransparent},
  {kCanvasScanFrame1, 3, 3, kCanvasTransparent}
};
const CanvasSpriteDef kCanvasSceneSprites[] = {
  {43, 5, 56, 5, 180, 4200, 900, 1300, true, kCanvasSparkFrames, 2},
  {58, 15, 58, 47, 120, 3600, 1400, 1500, true, kCanvasScanFrames, 2}
};
const CanvasElement kCanvasSceneElements[] = {
  {CANVAS_ELEMENT_FILLRECT, nullptr, nullptr, 0, 0, 0, 0, 64, 64, 0x0000, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_RECT, nullptr, nullptr, 1, 1, 0, 0, 62, 62, 0x1452, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_RECT, nullptr, nullptr, 4, 12, 0, 0, 25, 25, 0x24FE, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_FILLRECT, nullptr, nullptr, 5, 13, 0, 0, 23, 23, 0x0000, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_RECT, nullptr, nullptr, 33, 12, 0, 0, 27, 25, 0x24FE, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_FILLRECT, nullptr, nullptr, 34, 13, 0, 0, 25, 23, 0x10C4, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_LINE, nullptr, nullptr, 33, 39, 60, 39, 0, 0, 0x1452, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_LINE, nullptr, nullptr, 33, 50, 60, 50, 0, 0, 0x1452, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_FILLRECT, nullptr, nullptr, 5, 44, 0, 0, 8, 2, 0x24FE, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_FILLRECT, nullptr, nullptr, 5, 49, 0, 0, 12, 2, 0x7E5F, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_FILLRECT, nullptr, nullptr, 5, 54, 0, 0, 16, 2, 0xFFFF, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_TEXT, "CANVAS", nullptr, 5, 10, 0, 0, 0, 0, 0, 0x24FE, 0x0000, CANVAS_FONT_SQUARE},
  {CANVAS_ELEMENT_TEXT, "CW", nullptr, 52, 10, 0, 0, 0, 0, 0, 0x7E5F, 0x0000, CANVAS_FONT_PICOPIXEL},
  {CANVAS_ELEMENT_TEXT, "64x64", nullptr, 5, 62, 0, 0, 0, 0, 0, 0x7E5F, 0x0000, CANVAS_FONT_PICOPIXEL},
  {CANVAS_ELEMENT_STORED_IMAGE, nullptr, nullptr, 0, 0, 0, 0, 64, 64, 0, 0, 0, CANVAS_FONT_DEFAULT},
  {CANVAS_ELEMENT_DATETIME, nullptr, "%H", 36, 27, 0, 0, 0, 0, 0, 0xFFFF, 0x10C4, CANVAS_FONT_BIG},
  {CANVAS_ELEMENT_DATETIME, nullptr, "%M", 45, 37, 0, 0, 0, 0, 0, 0xFFFF, 0x10C4, CANVAS_FONT_MEDIUM},
  {CANVAS_ELEMENT_DATETIME, nullptr, "%b %d", 35, 47, 0, 0, 0, 0, 0, 0x7E5F, 0x0000, CANVAS_FONT_PICOPIXEL},
  {CANVAS_ELEMENT_DATETIME, nullptr, "%a", 35, 57, 0, 0, 0, 0, 0, 0x24FE, 0x0000, CANVAS_FONT_PICOPIXEL}
};

PacmanThemeState gPacmanThemeState = {};
uint8_t gPacmanAutoRoute[kPacmanRouteCapacity] = {};
PokedexThemeState gPokedexThemeState = {};
MarioThemeState gMarioThemeState = {};
char gLastThemeId[64] = "";
DateI18nEN gClockwiseWordsI18n;

bool isThemeId(const char* themeId, const char* value) {
  if (themeId == nullptr || value == nullptr) {
    return false;
  }
  return strcmp(themeId, value) == 0;
}

uint16_t toColor(MatrixPanel_I2S_DMA* display, uint8_t r, uint8_t g, uint8_t b) {
  return display->color565(r, g, b);
}

void setPixel(MatrixPanel_I2S_DMA* display, int x, int y, uint16_t color) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }
  display->drawPixel(x, y, color);
}

void fillRect(MatrixPanel_I2S_DMA* display, int x, int y, int width, int height, uint16_t color) {
  for (int py = y; py < y + height; py++) {
    for (int px = x; px < x + width; px++) {
      setPixel(display, px, py, color);
    }
  }
}

void drawRect(MatrixPanel_I2S_DMA* display, int x, int y, int width, int height, uint16_t color) {
  for (int px = x; px < x + width; px++) {
    setPixel(display, px, y, color);
    setPixel(display, px, y + height - 1, color);
  }
  for (int py = y; py < y + height; py++) {
    setPixel(display, x, py, color);
    setPixel(display, x + width - 1, py, color);
  }
}

void drawLine(MatrixPanel_I2S_DMA* display, int x0, int y0, int x1, int y1, uint16_t color) {
  int dx = abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;

  while (true) {
    setPixel(display, x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    int doubleError = error * 2;
    if (doubleError >= dy) {
      error += dy;
      x0 += sx;
    }
    if (doubleError <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

void drawCircle(MatrixPanel_I2S_DMA* display, int cx, int cy, int radius, uint16_t color) {
  int x = radius;
  int y = 0;
  int error = 0;

  while (x >= y) {
    setPixel(display, cx + x, cy + y, color);
    setPixel(display, cx + y, cy + x, color);
    setPixel(display, cx - y, cy + x, color);
    setPixel(display, cx - x, cy + y, color);
    setPixel(display, cx - x, cy - y, color);
    setPixel(display, cx - y, cy - x, color);
    setPixel(display, cx + y, cy - x, color);
    setPixel(display, cx + x, cy - y, color);

    if (error <= 0) {
      y += 1;
      error += 2 * y + 1;
    }
    if (error > 0) {
      x -= 1;
      error -= 2 * x + 1;
    }
  }
}

void drawFilledCircle(MatrixPanel_I2S_DMA* display, int cx, int cy, int radius, uint16_t color) {
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x * x + y * y <= radius * radius) {
        setPixel(display, cx + x, cy + y, color);
      }
    }
  }
}

void drawClockTextAligned(
  MatrixPanel_I2S_DMA* display,
  const char* text,
  int x,
  int y,
  uint16_t color,
  uint8_t fontId,
  int scale,
  const char* align
) {
  int drawX = x;
  int width = getClockTextWidth(text, fontId, scale);
  if (strcmp(align, "center") == 0) {
    drawX = x - width / 2;
  } else if (strcmp(align, "right") == 0) {
    drawX = x - width;
  }
  drawClockText(display, text, drawX, y, color, fontId, scale);
}

int getTinyTextWidth(const char* text, int size) {
  int len = strlen(text);
  if (len <= 0) {
    return 0;
  }
  return len * 4 * size - size;
}

void drawTinyTextAligned(
  MatrixPanel_I2S_DMA* display,
  const char* text,
  int x,
  int y,
  uint16_t color,
  int size,
  const char* align
) {
  int drawX = x;
  int width = getTinyTextWidth(text, size);
  if (strcmp(align, "center") == 0) {
    drawX = x - width / 2;
  } else if (strcmp(align, "right") == 0) {
    drawX = x - width;
  }
  (void)display;
  DisplayManager::drawTinyText(text, drawX, y, color, size);
}

void drawStoredImage(
  MatrixPanel_I2S_DMA* display,
  const PixelData* imagePixels,
  int imagePixelCount
) {
  if (imagePixels == nullptr || imagePixelCount <= 0) {
    return;
  }

  for (int i = 0; i < imagePixelCount; i++) {
    const PixelData& pixel = imagePixels[i];
    if (pixel.x < 64 && pixel.y < 64) {
      setPixel(display, pixel.x, pixel.y, display->color565(pixel.r, pixel.g, pixel.b));
    }
  }
}

uint16_t readProgmemColor565(const uint16_t* bitmap, int index) {
  return pgm_read_word(&bitmap[index]);
}

uint16_t scaleColor565(uint16_t color, uint8_t scale) {
  uint8_t red = static_cast<uint8_t>(((color >> 11) & 0x1F) * scale / 255);
  uint8_t green = static_cast<uint8_t>(((color >> 5) & 0x3F) * scale / 255);
  uint8_t blue = static_cast<uint8_t>((color & 0x1F) * scale / 255);
  return static_cast<uint16_t>((red << 11) | (green << 5) | blue);
}

void drawBitmap565(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  const uint16_t* bitmap,
  int width,
  int height
) {
  for (int py = 0; py < height; py++) {
    for (int px = 0; px < width; px++) {
      uint16_t color = readProgmemColor565(bitmap, py * width + px);
      setPixel(display, x + px, y + py, color);
    }
  }
}

void drawBitmap565Scaled(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  const uint16_t* bitmap,
  int width,
  int height,
  uint8_t scale
) {
  for (int py = 0; py < height; py++) {
    for (int px = 0; px < width; px++) {
      uint16_t color = readProgmemColor565(bitmap, py * width + px);
      setPixel(display, x + px, y + py, scaleColor565(color, scale));
    }
  }
}

void drawBitmap565RamTransparent(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  const uint16_t* bitmap,
  int width,
  int height,
  uint16_t transparentColor
) {
  for (int py = 0; py < height; py++) {
    for (int px = 0; px < width; px++) {
      uint16_t color = bitmap[py * width + px];
      if (color == transparentColor) {
        continue;
      }
      setPixel(display, x + px, y + py, color);
    }
  }
}

void drawBitmap565Transparent(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  const uint16_t* bitmap,
  int width,
  int height,
  uint16_t transparentColor
) {
  for (int py = 0; py < height; py++) {
    for (int px = 0; px < width; px++) {
      uint16_t color = readProgmemColor565(bitmap, py * width + px);
      if (color == transparentColor) {
        continue;
      }
      setPixel(display, x + px, y + py, color);
    }
  }
}

void drawBitmap565CropTransparent(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  const uint16_t* bitmap,
  int bitmapWidth,
  int srcX,
  int srcY,
  int cropWidth,
  int cropHeight,
  uint16_t transparentColor
) {
  for (int py = 0; py < cropHeight; py++) {
    for (int px = 0; px < cropWidth; px++) {
      int sourceIndex = (srcY + py) * bitmapWidth + srcX + px;
      uint16_t color = readProgmemColor565(bitmap, sourceIndex);
      if (color == transparentColor) {
        continue;
      }
      setPixel(display, x + px, y + py, color);
    }
  }
}

void formatTimeText(const struct tm* timeinfo, char* buffer, size_t bufferSize) {
  if (timeinfo == nullptr) {
    snprintf(buffer, bufferSize, "--:--");
    return;
  }
  snprintf(buffer, bufferSize, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
}

void formatDateText(const struct tm* timeinfo, char* buffer, size_t bufferSize) {
  if (timeinfo == nullptr) {
    snprintf(buffer, bufferSize, "----");
    return;
  }
  snprintf(buffer, bufferSize, "%d-%d", timeinfo->tm_mon + 1, timeinfo->tm_mday);
}

void formatWeekText(const struct tm* timeinfo, char* buffer, size_t bufferSize) {
  static const char* kWeekDays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  if (timeinfo == nullptr) {
    snprintf(buffer, bufferSize, "---");
    return;
  }
  snprintf(buffer, bufferSize, "%s", kWeekDays[timeinfo->tm_wday]);
}

void formatMonthText(const struct tm* timeinfo, char* buffer, size_t bufferSize) {
  if (timeinfo == nullptr) {
    snprintf(buffer, bufferSize, "---");
    return;
  }
  snprintf(buffer, bufferSize, "%s", kMonthNames[timeinfo->tm_mon]);
}

int getThemeDisplayHour(const struct tm* timeinfo) {
  if (timeinfo == nullptr) {
    return 0;
  }

  int hour = timeinfo->tm_hour;
  if (ConfigManager::themeClockConfig.hourFormat == 12) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }
  return hour;
}

bool themeUses12HourFormat() {
  return ConfigManager::themeClockConfig.hourFormat == 12;
}

bool isThemeAM(const struct tm* timeinfo) {
  if (timeinfo == nullptr) {
    return true;
  }
  return timeinfo->tm_hour < 12;
}

void uppercaseAscii(char* text) {
  if (text == nullptr) {
    return;
  }
  for (size_t i = 0; text[i] != '\0'; i++) {
    if (text[i] >= 'a' && text[i] <= 'z') {
      text[i] = (char)(text[i] - 'a' + 'A');
    }
  }
}

void setCanvasFont(MatrixPanel_I2S_DMA* display, CanvasFontKind font) {
  if (font == CANVAS_FONT_PICOPIXEL) {
    display->setFont(&Picopixel);
  } else if (font == CANVAS_FONT_SQUARE) {
    display->setFont(&atariFont);
  } else if (font == CANVAS_FONT_BIG) {
    display->setFont(&hour8pt7b);
  } else if (font == CANVAS_FONT_MEDIUM) {
    display->setFont(&minute7pt7b);
  } else {
    display->setFont(nullptr);
  }
}

void drawCanvasTextElement(MatrixPanel_I2S_DMA* display, const char* text, const CanvasElement& element) {
  if (text == nullptr || text[0] == '\0') {
    return;
  }

  setCanvasFont(display, element.font);

  int16_t boundX = 0;
  int16_t boundY = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  display->getTextBounds(text, 0, 0, &boundX, &boundY, &width, &height);
  display->fillRect(element.x + boundX, element.y + boundY, width, height, element.bgColor);
  display->setTextColor(element.fgColor);
  display->setCursor(element.x, element.y);
  display->print(text);
}

void formatCanvasDateTimeText(
  const char* format,
  const struct tm* timeinfo,
  char* buffer,
  size_t bufferSize
) {
  if (bufferSize == 0) {
    return;
  }

  if (format == nullptr || format[0] == '\0' || timeinfo == nullptr) {
    snprintf(buffer, bufferSize, "--");
    return;
  }

  size_t written = strftime(buffer, bufferSize, format, timeinfo);
  if (written == 0) {
    snprintf(buffer, bufferSize, "--");
    return;
  }
  uppercaseAscii(buffer);
}

void drawCanvasPlaceholder(MatrixPanel_I2S_DMA* display) {
  fillRect(display, 11, 17, 10, 12, 0xFFFF);
  drawRect(display, 10, 16, 12, 14, 0x7E5F);
  drawLine(display, 11, 29, 9, 35, 0x1452);
  drawLine(display, 20, 29, 22, 35, 0x1452);
  drawLine(display, 8, 35, 23, 35, 0x1452);
  drawLine(display, 12, 20, 19, 27, 0x24FE);
  drawLine(display, 19, 20, 12, 27, 0x24FE);
}

void drawCanvasElement(
  MatrixPanel_I2S_DMA* display,
  const CanvasElement& element,
  const struct tm* timeinfo,
  const PixelData* imagePixels,
  int imagePixelCount
) {
  if (element.type == CANVAS_ELEMENT_FILLRECT) {
    fillRect(display, element.x, element.y, element.width, element.height, element.color);
    return;
  }
  if (element.type == CANVAS_ELEMENT_RECT) {
    drawRect(display, element.x, element.y, element.width, element.height, element.color);
    return;
  }
  if (element.type == CANVAS_ELEMENT_LINE) {
    drawLine(display, element.x, element.y, element.x1, element.y1, element.color);
    return;
  }
  if (element.type == CANVAS_ELEMENT_STORED_IMAGE) {
    if (imagePixels != nullptr && imagePixelCount > 0) {
      drawStoredImage(display, imagePixels, imagePixelCount);
    } else {
      drawCanvasPlaceholder(display);
    }
    return;
  }
  if (element.type == CANVAS_ELEMENT_TEXT) {
    drawCanvasTextElement(display, element.text, element);
    return;
  }
  if (element.type == CANVAS_ELEMENT_DATETIME) {
    char buffer[24];
    formatCanvasDateTimeText(element.format, timeinfo, buffer, sizeof(buffer));
    drawCanvasTextElement(display, buffer, element);
  }
}

int lerpCanvasPosition(int start, int end, unsigned long elapsed, unsigned long duration) {
  if (duration == 0 || elapsed >= duration) {
    return end;
  }
  return start + (int)((long)(end - start) * (long)elapsed / (long)duration);
}

unsigned long getCanvasSpriteFrameSequenceMs(const CanvasSpriteDef& sprite) {
  if (sprite.frameCount == 0 || sprite.frameDelayMs == 0) {
    return 0;
  }
  return (unsigned long)sprite.frameDelayMs * (unsigned long)sprite.frameCount;
}

unsigned long getCanvasSpriteMovementSequenceMs(const CanvasSpriteDef& sprite) {
  if (sprite.moveDurationMs == 0) {
    return 0;
  }
  unsigned long total = (unsigned long)sprite.moveStartTimeMs + (unsigned long)sprite.moveDurationMs;
  if (sprite.shouldReturnToOrigin) {
    total += (unsigned long)sprite.moveDurationMs;
  }
  return total;
}

unsigned long getCanvasSpriteCycleMs(const CanvasSpriteDef& sprite) {
  unsigned long cycle = sprite.loopDelayMs;
  unsigned long frameSequence = getCanvasSpriteFrameSequenceMs(sprite);
  unsigned long movementSequence = getCanvasSpriteMovementSequenceMs(sprite);

  if (frameSequence > cycle) {
    cycle = frameSequence;
  }
  if (movementSequence > cycle) {
    cycle = movementSequence;
  }
  if (cycle == 0) {
    cycle = 1;
  }
  return cycle;
}

uint8_t resolveCanvasSpriteFrameIndex(const CanvasSpriteDef& sprite, unsigned long tick) {
  if (sprite.frameCount == 0) {
    return 0;
  }

  if (sprite.frameDelayMs == 0) {
    return sprite.frameCount - 1;
  }

  unsigned long cycle = getCanvasSpriteCycleMs(sprite);
  unsigned long phase = tick % cycle;
  unsigned long frameIndex = phase / sprite.frameDelayMs;
  if (frameIndex >= sprite.frameCount) {
    frameIndex = sprite.frameCount - 1;
  }
  return (uint8_t)frameIndex;
}

void resolveCanvasSpritePosition(const CanvasSpriteDef& sprite, unsigned long tick, int& outX, int& outY) {
  outX = sprite.x;
  outY = sprite.y;

  if (sprite.moveDurationMs == 0) {
    return;
  }

  unsigned long cycle = getCanvasSpriteCycleMs(sprite);
  unsigned long phase = tick % cycle;
  if (phase < sprite.moveStartTimeMs) {
    return;
  }

  unsigned long movePhase = phase - sprite.moveStartTimeMs;
  if (movePhase <= sprite.moveDurationMs) {
    outX = lerpCanvasPosition(sprite.x, sprite.moveTargetX, movePhase, sprite.moveDurationMs);
    outY = lerpCanvasPosition(sprite.y, sprite.moveTargetY, movePhase, sprite.moveDurationMs);
    return;
  }

  if (sprite.shouldReturnToOrigin && movePhase <= sprite.moveDurationMs * 2UL) {
    unsigned long returnPhase = movePhase - sprite.moveDurationMs;
    outX = lerpCanvasPosition(sprite.moveTargetX, sprite.x, returnPhase, sprite.moveDurationMs);
    outY = lerpCanvasPosition(sprite.moveTargetY, sprite.y, returnPhase, sprite.moveDurationMs);
    return;
  }

  outX = sprite.shouldReturnToOrigin ? sprite.x : sprite.moveTargetX;
  outY = sprite.shouldReturnToOrigin ? sprite.y : sprite.moveTargetY;
}

void drawCanvasSprite(MatrixPanel_I2S_DMA* display, const CanvasSpriteDef& sprite, unsigned long tick) {
  if (sprite.frames == nullptr || sprite.frameCount == 0) {
    return;
  }

  uint8_t frameIndex = resolveCanvasSpriteFrameIndex(sprite, tick);
  int drawX = sprite.x;
  int drawY = sprite.y;
  resolveCanvasSpritePosition(sprite, tick, drawX, drawY);
  const CanvasSpriteFrame& frame = sprite.frames[frameIndex];
  drawBitmap565Transparent(
    display,
    drawX,
    drawY,
    frame.bitmap,
    frame.width,
    frame.height,
    frame.transparentColor
  );
}

void handleThemeStateReset(const char* themeId) {
  if (themeId == nullptr) {
    gLastThemeId[0] = '\0';
    gPacmanThemeState.initialized = false;
    gPokedexThemeState.initialized = false;
    gMarioThemeState.initialized = false;
    return;
  }

  if (strcmp(gLastThemeId, themeId) == 0) {
    return;
  }

  strncpy(gLastThemeId, themeId, sizeof(gLastThemeId) - 1);
  gLastThemeId[sizeof(gLastThemeId) - 1] = '\0';
  gPacmanThemeState.initialized = false;
  gPokedexThemeState.initialized = false;
  gMarioThemeState.initialized = false;
}

const char* wordsForHour(int hour) {
  static const char* kWords[] = {
    "twelve", "one", "two", "three", "four", "five",
    "six", "seven", "eight", "nine", "ten", "eleven"
  };
  return kWords[hour % 12];
}

void wordsForMinute(int minute, char* lineA, size_t lineASize, char* lineB, size_t lineBSize) {
  lineA[0] = '\0';
  lineB[0] = '\0';

  static const char* kSpecial[] = {
    "oclock", "one", "two", "three", "four", "five", "six", "seven",
    "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen",
    "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"
  };
  if (minute >= 0 && minute < 20) {
    snprintf(lineA, lineASize, "%s", kSpecial[minute]);
    return;
  }

  static const char* kTens[] = {"", "", "twenty", "thirty", "forty", "fifty"};
  int tens = minute / 10;
  int ones = minute % 10;
  snprintf(lineA, lineASize, "%s", kTens[tens]);
  if (ones > 0) {
    snprintf(lineB, lineBSize, "%s", kSpecial[ones]);
  }
}

void drawClockBlock(MatrixPanel_I2S_DMA* display, int x, int y, const char* text) {
  uint16_t yellow = toColor(display, 247, 202, 74);
  uint16_t border = toColor(display, 99, 49, 0);
  uint16_t textColor = toColor(display, 26, 17, 0);
  fillRect(display, x, y, 18, 12, yellow);
  drawRect(display, x, y, 18, 12, border);
  setPixel(display, x + 3, y + 3, border);
  setPixel(display, x + 14, y + 3, border);
  setPixel(display, x + 3, y + 8, border);
  setPixel(display, x + 14, y + 8, border);
  drawClockText(display, text, x + 4, y + 2, textColor, CLOCK_FONT_CLASSIC_5X7, 1);
}

void drawCloud(MatrixPanel_I2S_DMA* display, int x, int y) {
  uint16_t white = toColor(display, 255, 255, 255);
  drawFilledCircle(display, x + 3, y + 5, 4, white);
  drawFilledCircle(display, x + 8, y + 4, 5, white);
  drawFilledCircle(display, x + 13, y + 5, 4, white);
  fillRect(display, x + 2, y + 5, 12, 4, white);
}

void drawHill(MatrixPanel_I2S_DMA* display, int x, int y) {
  drawFilledCircle(display, x + 8, y + 14, 10, toColor(display, 12, 154, 45));
  drawFilledCircle(display, x + 20, y + 12, 12, toColor(display, 38, 193, 70));
  drawFilledCircle(display, x + 14, y + 10, 9, toColor(display, 50, 212, 90));
  fillRect(display, x + 6, y + 12, 16, 10, toColor(display, 38, 193, 70));
}

void drawBush(MatrixPanel_I2S_DMA* display, int x, int y) {
  drawFilledCircle(display, x + 3, y + 5, 4, toColor(display, 30, 177, 59));
  drawFilledCircle(display, x + 8, y + 4, 5, toColor(display, 46, 204, 75));
  drawFilledCircle(display, x + 13, y + 5, 4, toColor(display, 30, 177, 59));
  fillRect(display, x + 1, y + 6, 14, 5, toColor(display, 22, 145, 47));
}

void drawGround(MatrixPanel_I2S_DMA* display) {
  uint16_t top = toColor(display, 243, 214, 168);
  uint16_t dirt = toColor(display, 159, 93, 45);
  uint16_t dark = toColor(display, 111, 57, 21);
  uint16_t grass = toColor(display, 52, 190, 69);
  fillRect(display, 0, 53, 64, 11, dirt);
  fillRect(display, 0, 51, 64, 2, grass);
  for (int x = 0; x < 64; x += 4) {
    fillRect(display, x, 53, 2, 2, top);
    fillRect(display, x + 2, 56, 2, 2, dark);
    fillRect(display, x + ((x / 4) % 2), 59, 2, 2, top);
  }
}

void drawMario(MatrixPanel_I2S_DMA* display, int x, int y) {
  const char* sprite[] = {
    "....RRR.....",
    "...RRSRR....",
    "...SSSSS....",
    "..SSKSK.....",
    "..SSSSSS....",
    "..RBBR......",
    ".RBBBBR.....",
    ".RRBBRR.....",
    "..BB..BB....",
    ".BB....BB...",
    ".WW....WW..."
  };

  for (int row = 0; row < 11; row++) {
    for (int col = 0; col < 12; col++) {
      char token = sprite[row][col];
      if (token == '.') {
        continue;
      }
      uint16_t color = 0;
      if (token == 'R') color = toColor(display, 217, 74, 58);
      if (token == 'S') color = toColor(display, 245, 195, 155);
      if (token == 'K') color = toColor(display, 36, 20, 14);
      if (token == 'B') color = toColor(display, 37, 103, 232);
      if (token == 'W') color = toColor(display, 247, 216, 93);
      setPixel(display, x + col, y + row, color);
    }
  }
}

void applySparkles(MatrixPanel_I2S_DMA* display, unsigned long tick, uint16_t colorA, uint16_t colorB, uint16_t colorC) {
  const int points[][2] = {
    {8, 8}, {24, 10}, {41, 7}, {56, 11}, {10, 20}, {53, 18}
  };
  const uint16_t palette[] = {colorA, colorB, colorC};
  for (int i = 0; i < 6; i++) {
    int frame = (tick / 250 + i) % 3;
    int x = points[i][0];
    int y = points[i][1];
    uint16_t color = palette[frame];
    setPixel(display, x, y, color);
    if (((tick / 250) + i) % 2 == 0) {
      setPixel(display, x - 1, y, color);
      setPixel(display, x + 1, y, color);
      setPixel(display, x, y - 1, color);
      setPixel(display, x, y + 1, color);
    }
  }
}

void drawFireworkBurst(
  MatrixPanel_I2S_DMA* display,
  int cx,
  int cy,
  int radius,
  uint16_t innerColor,
  uint16_t outerColor
) {
  drawFilledCircle(display, cx, cy, 1, innerColor);
  for (int i = 0; i < 8; i++) {
    float angle = (float)i * (float)M_PI / 4.0f;
    int x = (int)roundf(cx + cosf(angle) * radius);
    int y = (int)roundf(cy + sinf(angle) * radius);
    setPixel(display, x, y, outerColor);
    if (radius > 2) {
      int midX = (int)roundf(cx + cosf(angle) * (radius - 2));
      int midY = (int)roundf(cy + sinf(angle) * (radius - 2));
      setPixel(display, midX, midY, innerColor);
    }
  }
}

void resetMarioThemeState(MarioThemeState* state) {
  state->initialized = true;
  state->jumping = false;
  state->descending = false;
  state->marioX = 23;
  state->marioY = 40;
  state->marioWidth = MARIO_IDLE_SIZE[0];
  state->marioHeight = MARIO_IDLE_SIZE[1];
  state->baseMarioY = 40;
  state->hourBlockY = 8;
  state->minuteBlockY = 8;
  state->jumpStartedAt = 0;
  state->lastFrameAt = millis();
}

void startMarioJump(MarioThemeState* state) {
  if (state->jumping) {
    return;
  }
  state->jumping = true;
  state->descending = false;
  state->marioWidth = MARIO_JUMP_SIZE[0];
  state->marioHeight = MARIO_JUMP_SIZE[1];
  state->jumpStartedAt = millis();
  state->lastFrameAt = millis();
}

void updateMarioThemeState(MarioThemeState* state, const struct tm* timeinfo) {
  if (timeinfo != nullptr &&
      timeinfo->tm_sec == 0 &&
      (!state->jumping && millis() - state->jumpStartedAt > 1000)) {
    startMarioJump(state);
  }

  if (!state->jumping || millis() - state->lastFrameAt < 50) {
    return;
  }

  state->marioY += state->descending ? 3 : -3;
  if (!state->descending && state->baseMarioY - state->marioY >= 14) {
    state->descending = true;
  }

  if (!state->descending) {
    if (state->marioY <= 28) {
      state->hourBlockY = 6;
      state->minuteBlockY = 6;
    }
  } else {
    if (state->hourBlockY < 8) {
      state->hourBlockY += 2;
    }
    if (state->minuteBlockY < 8) {
      state->minuteBlockY += 2;
    }
  }

  if (state->marioY + state->marioHeight >= 56) {
    state->jumping = false;
    state->descending = false;
    state->marioY = state->baseMarioY;
    state->marioWidth = MARIO_IDLE_SIZE[0];
    state->marioHeight = MARIO_IDLE_SIZE[1];
    state->hourBlockY = 8;
    state->minuteBlockY = 8;
  }

  state->lastFrameAt = millis();
}

void drawMarioThemeScene(MatrixPanel_I2S_DMA* display) {
  fillRect(display, 0, 0, 64, 64, SKY_COLOR);
  for (int x = 0; x < 64; x += 8) {
    drawBitmap565RamTransparent(display, x, 56, GROUND, 8, 8, _MASK);
  }
  drawBitmap565RamTransparent(display, 43, 47, BUSH, 21, 9, _MASK);
  drawBitmap565RamTransparent(display, 0, 34, HILL, 20, 22, _MASK);
  drawBitmap565RamTransparent(display, 0, 21, CLOUD1, 13, 12, _MASK);
  drawBitmap565RamTransparent(display, 51, 7, CLOUD2, 13, 12, _MASK);
}

void drawMarioBlockText(MatrixPanel_I2S_DMA* display, int x, int y, const char* text) {
  display->setFont(&Super_Mario_Bros__24pt7b);
  display->setTextColor(0x0000);
  if (strlen(text) == 1) {
    display->setCursor(x + 6, y + 12);
  } else {
    display->setCursor(x + 2, y + 12);
  }
  display->print(text);
  display->setFont(nullptr);
}

void renderMarioTheme(
  MatrixPanel_I2S_DMA* display,
  const struct tm* timeinfo
) {
  if (!gMarioThemeState.initialized) {
    resetMarioThemeState(&gMarioThemeState);
  }
  updateMarioThemeState(&gMarioThemeState, timeinfo);
  drawMarioThemeScene(display);

  char hourText[4];
  char minuteText[3];
  if (timeinfo == nullptr) {
    snprintf(hourText, sizeof(hourText), "0");
    snprintf(minuteText, sizeof(minuteText), "00");
  } else {
    snprintf(hourText, sizeof(hourText), "%d", timeinfo->tm_hour);
    snprintf(minuteText, sizeof(minuteText), "%02d", timeinfo->tm_min);
  }

  drawBitmap565RamTransparent(display, 13, gMarioThemeState.hourBlockY, BLOCK, 19, 19, _MASK);
  drawBitmap565RamTransparent(display, 32, gMarioThemeState.minuteBlockY, BLOCK, 19, 19, _MASK);
  drawMarioBlockText(display, 13, gMarioThemeState.hourBlockY, hourText);
  drawMarioBlockText(display, 32, gMarioThemeState.minuteBlockY, minuteText);

  const uint16_t* marioSprite = gMarioThemeState.jumping ? MARIO_JUMP : MARIO_IDLE;
  drawBitmap565RamTransparent(
    display,
    gMarioThemeState.marioX,
    gMarioThemeState.marioY,
    marioSprite,
    gMarioThemeState.marioWidth,
    gMarioThemeState.marioHeight,
    _MASK
  );
}

void renderWordsTheme(
  MatrixPanel_I2S_DMA* display,
  const struct tm* timeinfo,
  const char* dateText,
  const char* weekText
) {
  (void)dateText;
  (void)weekText;
  const uint16_t kWordsAccent = 0x2589;

  char hourWords[20] = "";
  char minuteWords[20] = "";
  char formattedDate[8] = "";
  char weekDay[4] = "";
  int hour = timeinfo == nullptr ? 0 : timeinfo->tm_hour;
  int minute = timeinfo == nullptr ? 0 : timeinfo->tm_min;
  int day = timeinfo == nullptr ? 1 : timeinfo->tm_mday;
  int month = timeinfo == nullptr ? 1 : timeinfo->tm_mon + 1;
  int weekDayIndex = timeinfo == nullptr ? 0 : timeinfo->tm_wday;

  gClockwiseWordsI18n.timeInWords(hour, minute, hourWords, minuteWords);
  snprintf(formattedDate, sizeof(formattedDate), "%s", gClockwiseWordsI18n.formatDate(day, month));
  snprintf(weekDay, sizeof(weekDay), "%s", gClockwiseWordsI18n.weekDayName(weekDayIndex));

  fillRect(display, 0, 0, 64, 64, 0x0000);

  display->setTextWrap(true);
  display->setTextColor(kWordsAccent);
  display->setFont(&hour8pt7b);
  display->setCursor(1, 14);
  display->print(hourWords);

  display->setTextColor(0xFFFF);
  display->setFont(&minute7pt7b);
  display->setCursor(0, 25);
  display->print(minuteWords);

  display->drawFastHLine(1, 40, 62, 0xFFFF);

  fillRect(display, 0, 41, 46, 13, 0x0000);
  display->setTextColor(kWordsAccent);
  display->setFont(&minute7pt7b);
  display->setCursor(0, 52);
  display->print(formattedDate);

  uint16_t dateWidth = 0;
  uint16_t dateHeight = 0;
  int16_t boundX = 0;
  int16_t boundY = 0;
  display->getTextBounds(formattedDate, 0, 0, &boundX, &boundY, &dateWidth, &dateHeight);

  display->setTextColor(0xFFFF);
  display->setFont(&small4pt7b);
  display->setCursor(dateWidth + 2, 52);
  display->print(weekDay);

  if (WiFi.status() == WL_CONNECTED) {
    drawBitmap565(display, 1, 55, WIFI, 8, 8);
  } else {
    fillRect(display, 1, 55, 8, 8, 0x0000);
  }

  display->setFont(nullptr);
}

void renderWorldMapTheme(
  MatrixPanel_I2S_DMA* display,
  const struct tm* timeinfo
) {
  const int kBrazilTimezoneOffset = 32;
  const int kTimezoneWidth = 5;
  const int kMapWidth = 120;
  const int kMapHeight = 56;
  const uint16_t kTransparentMask = 0xF81F;
  const int hour = timeinfo == nullptr ? 0 : timeinfo->tm_hour;

  fillRect(display, 0, 0, 64, 64, toColor(display, 1, 8, 16));

  if (hour < 12) {
    int position = kBrazilTimezoneOffset + ((12 - hour) * kTimezoneWidth);
    int firstWidth = 32 + (kMapWidth - position);
    int secondWidth = 64 - firstWidth;
    drawBitmap565CropTransparent(display, 0, 0, _WORLD_MAP, kMapWidth, position - 32, 0, firstWidth, kMapHeight, kTransparentMask);
    if (secondWidth > 0) {
      drawBitmap565CropTransparent(display, firstWidth, 0, _WORLD_MAP, kMapWidth, 0, 0, secondWidth, kMapHeight, kTransparentMask);
    }
  } else {
    int position = kBrazilTimezoneOffset - ((hour % 12) * kTimezoneWidth);
    int offset = 32 - position;
    int firstWidth = 64 - offset;
    drawBitmap565CropTransparent(display, offset, 0, _WORLD_MAP, kMapWidth, 0, 0, firstWidth, kMapHeight, kTransparentMask);
    if (offset > 0) {
      drawBitmap565CropTransparent(display, 0, 0, _WORLD_MAP, kMapWidth, kMapWidth - offset, 0, offset, kMapHeight, kTransparentMask);
    }
  }

  fillRect(display, 0, 55, 31, 9, 0x0000);
  fillRect(display, 32, 0, 1, 64, 0xF000);

  display->setFont(&small4pt7b);
  display->setTextColor(0xFFFF);
  display->setCursor(1, 62);
  if (timeinfo == nullptr) {
    display->print("0:00");
  } else {
    display->print(String(timeinfo->tm_hour));
    display->print(":");
    if (timeinfo->tm_min < 10) {
      display->print("0");
    }
    display->print(String(timeinfo->tm_min));
  }
  display->setFont(nullptr);
}

void renderCastlevaniaTheme(
  MatrixPanel_I2S_DMA* display,
  const struct tm* timeinfo
) {
  const float kHourOffset = -30.0f;
  const float kMinuteOffset = -6.0f;
  const uint16_t kPointerColor = 0xB58C;
  const int centerX = 32;
  const int centerY = 28;
  int hour = timeinfo == nullptr ? 0 : timeinfo->tm_hour;
  int minute = timeinfo == nullptr ? 0 : timeinfo->tm_min;

  drawBitmap565(display, 0, 0, _CLOCK_TOWER, 64, 64);

  float additionalOffset = (kHourOffset * minute) / 60.0f;
  float hourAngle = ((hour * kHourOffset) + 180.0f + additionalOffset) * (float)M_PI / 180.0f;
  float minuteAngle = ((minute * kMinuteOffset) + 180.0f) * (float)M_PI / 180.0f;

  drawLine(
    display,
    centerX,
    centerY,
    (int)roundf(centerX + sinf(minuteAngle) * 10.0f),
    (int)roundf(centerY + cosf(minuteAngle) * 10.0f),
    kPointerColor
  );
  drawLine(
    display,
    centerX,
    centerY,
    (int)roundf(centerX + sinf(hourAngle) * 7.0f),
    (int)roundf(centerY + cosf(hourAngle) * 7.0f),
    kPointerColor
  );
}

PacmanMapBlock getPacmanNextBlock(const PacmanThemeState& state, PacmanDirection direction) {
  int gridX = (state.x - 2) / 5;
  int gridY = (state.y - 2) / 5;

  if (direction == PACMAN_DIRECTION_RIGHT) {
    if (state.x + 5 < 62) {
      return static_cast<PacmanMapBlock>(state.map[gridY][gridX + 1]);
    }
  } else if (direction == PACMAN_DIRECTION_DOWN) {
    if (state.y + 5 < 62) {
      return static_cast<PacmanMapBlock>(state.map[gridY + 1][gridX]);
    }
  } else if (direction == PACMAN_DIRECTION_LEFT) {
    if (state.x - 2 > 0) {
      return static_cast<PacmanMapBlock>(state.map[gridY][gridX - 1]);
    }
  } else if (direction == PACMAN_DIRECTION_UP) {
    if (state.y - 2 > 0) {
      return static_cast<PacmanMapBlock>(state.map[gridY - 1][gridX]);
    }
  }

  return PACMAN_BLOCK_OUT_OF_MAP;
}

bool pacmanAllowsMovement(PacmanMapBlock block) {
  return block == PACMAN_BLOCK_EMPTY ||
         block == PACMAN_BLOCK_FOOD ||
         block == PACMAN_BLOCK_GATE ||
         block == PACMAN_BLOCK_SUPER_FOOD ||
         block == PACMAN_BLOCK_START;
}

bool pacmanBlocksMovement(PacmanMapBlock block) {
  return block == PACMAN_BLOCK_OUT_OF_MAP ||
         block == PACMAN_BLOCK_WALL ||
         block == PACMAN_BLOCK_CLOCK;
}

bool isPacmanVisibleTarget(uint8_t block) {
  return block == PACMAN_BLOCK_FOOD ||
         block == PACMAN_BLOCK_GATE ||
         block == PACMAN_BLOCK_SUPER_FOOD;
}

uint8_t getPacmanQuadrantIndex(int x, int y) {
  uint8_t quadrant = 0;
  if (x >= 6) {
    quadrant += 1;
  }
  if (y >= 6) {
    quadrant += 2;
  }
  return quadrant;
}

bool pacmanTargetComesBefore(
  const PacmanTargetCell& a,
  const PacmanTargetCell& b,
  uint8_t quadrant,
  uint8_t variant
) {
  int baseX = (quadrant % 2) * 6;
  int baseY = (quadrant / 2) * 6;
  int ax = a.x - baseX;
  int ay = a.y - baseY;
  int bx = b.x - baseX;
  int by = b.y - baseY;

  if (variant == 0) {
    if (ay != by) {
      return ay < by;
    }
    return (ay % 2 == 0) ? (ax < bx) : (ax > bx);
  }

  if (variant == 1) {
    if (ay != by) {
      return ay < by;
    }
    return (ay % 2 == 0) ? (ax > bx) : (ax < bx);
  }

  if (variant == 2) {
    if (ax != bx) {
      return ax < bx;
    }
    return (ax % 2 == 0) ? (ay < by) : (ay > by);
  }

  if (ax != bx) {
    return ax > bx;
  }
  return (ax % 2 == 0) ? (ay < by) : (ay > by);
}

void sortPacmanTargets(
  PacmanTargetCell* targets,
  int count,
  uint8_t quadrant,
  uint8_t variant
) {
  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (!pacmanTargetComesBefore(targets[i], targets[j], quadrant, variant)) {
        PacmanTargetCell temp = targets[i];
        targets[i] = targets[j];
        targets[j] = temp;
      }
    }
  }
}

int buildPacmanPathBetweenCells(
  const PacmanThemeState& state,
  int startX,
  int startY,
  int targetX,
  int targetY,
  PacmanDirection* directions,
  int maxDirections
) {
  struct PacmanNode {
    int x;
    int y;
  };

  bool visited[12][12];
  int prevX[12][12];
  int prevY[12][12];
  PacmanDirection prevDirection[12][12];
  PacmanNode queue[144];
  memset(visited, 0, sizeof(visited));
  memset(prevX, -1, sizeof(prevX));
  memset(prevY, -1, sizeof(prevY));

  static const int kDx[4] = {1, 0, -1, 0};
  static const int kDy[4] = {0, 1, 0, -1};
  static const PacmanDirection kDirections[4] = {
    PACMAN_DIRECTION_RIGHT,
    PACMAN_DIRECTION_DOWN,
    PACMAN_DIRECTION_LEFT,
    PACMAN_DIRECTION_UP
  };

  int head = 0;
  int tail = 0;
  queue[tail++] = {startX, startY};
  visited[startY][startX] = true;

  while (head < tail) {
    PacmanNode current = queue[head++];
    if (current.x == targetX && current.y == targetY) {
      break;
    }

    for (int i = 0; i < 4; i++) {
      int nextX = current.x + kDx[i];
      int nextY = current.y + kDy[i];
      if (nextX < 0 || nextX >= 12 || nextY < 0 || nextY >= 12) {
        continue;
      }
      if (visited[nextY][nextX]) {
        continue;
      }
      if (!pacmanAllowsMovement(static_cast<PacmanMapBlock>(state.map[nextY][nextX]))) {
        continue;
      }

      visited[nextY][nextX] = true;
      prevX[nextY][nextX] = current.x;
      prevY[nextY][nextX] = current.y;
      prevDirection[nextY][nextX] = kDirections[i];
      queue[tail++] = {nextX, nextY};
    }
  }

  if (!visited[targetY][targetX]) {
    return -1;
  }

  PacmanDirection reversed[144];
  int length = 0;
  int cursorX = targetX;
  int cursorY = targetY;

  while (!(cursorX == startX && cursorY == startY)) {
    if (length >= 144) {
      return -1;
    }
    reversed[length++] = prevDirection[cursorY][cursorX];
    int nextCursorX = prevX[cursorY][cursorX];
    int nextCursorY = prevY[cursorY][cursorX];
    cursorX = nextCursorX;
    cursorY = nextCursorY;
  }

  if (length > maxDirections) {
    return -1;
  }

  for (int i = 0; i < length; i++) {
    directions[i] = reversed[length - 1 - i];
  }
  return length;
}

bool buildPacmanVariedRoute(PacmanThemeState* state) {
  PacmanTargetCell quadrantTargets[4][36];
  uint8_t quadrantCounts[4] = {0, 0, 0, 0};
  state->routeLength = 0;
  state->routeIndex = 0;

  for (int row = 0; row < 12; row++) {
    for (int col = 0; col < 12; col++) {
      if (!isPacmanVisibleTarget(state->map[row][col])) {
        continue;
      }

      uint8_t quadrant = getPacmanQuadrantIndex(col, row);
      if (quadrantCounts[quadrant] >= 36) {
        return false;
      }
      quadrantTargets[quadrant][quadrantCounts[quadrant]++] = {col, row};
    }
  }

  int currentX = (state->x - 2) / 5;
  int currentY = (state->y - 2) / 5;
  PacmanDirection tempDirections[144];
  int planChoice = random((int)(sizeof(kPacmanRoutePlans) / sizeof(kPacmanRoutePlans[0])) + 1);

  if (planChoice == 0) {
    state->routeLength = sizeof(kPacmanRecordedRoute);
    memcpy(state->routeDirections, kPacmanRecordedRoute, state->routeLength);
    return true;
  }

  const PacmanRoutePlan& routePlan = kPacmanRoutePlans[planChoice - 1];

  for (int quadrantIndex = 0; quadrantIndex < 4; quadrantIndex++) {
    uint8_t quadrant = routePlan.quadrantOrder[quadrantIndex];
    uint8_t variant = routePlan.variants[quadrantIndex];
    int targetCount = quadrantCounts[quadrant];

    if (targetCount <= 0) {
      continue;
    }

    sortPacmanTargets(quadrantTargets[quadrant], targetCount, quadrant, variant);

    for (int i = 0; i < targetCount; i++) {
      int pathLength = buildPacmanPathBetweenCells(
        *state,
        currentX,
        currentY,
        quadrantTargets[quadrant][i].x,
        quadrantTargets[quadrant][i].y,
        tempDirections,
        144
      );
      if (pathLength < 0) {
        return false;
      }
      if ((state->routeLength + pathLength) > kPacmanRouteCapacity) {
        return false;
      }

      for (int step = 0; step < pathLength; step++) {
        state->routeDirections[state->routeLength++] = static_cast<uint8_t>(tempDirections[step]);
      }

      currentX = quadrantTargets[quadrant][i].x;
      currentY = quadrantTargets[quadrant][i].y;
    }
  }

  return state->routeLength > 0;
}

const uint16_t kPacmanFrameOpen[25] = {
  0x0000, 0xFE40, 0xFE40, 0xFE40, 0x0000,
  0xFE40, 0xFE40, 0x0000, 0xFE40, 0xFE40,
  0xFE40, 0xFE40, 0xFE40, 0xFE40, 0xFE40,
  0xFE40, 0xFE40, 0xFE40, 0xFE40, 0xFE40,
  0x0000, 0xFE40, 0xFE40, 0xFE40, 0x0000
};

const uint16_t kPacmanFrameClosed[25] = {
  0x0000, 0xFE40, 0xFE40, 0xFE40, 0xFE40,
  0xFE40, 0xFE40, 0x0000, 0xFE40, 0x0000,
  0xFE40, 0xFE40, 0xFE40, 0x0000, 0x0000,
  0xFE40, 0xFE40, 0xFE40, 0xFE40, 0x0000,
  0x0000, 0xFE40, 0xFE40, 0xFE40, 0xFE40
};

void copyPacmanFrame(uint16_t* target, const uint16_t* source) {
  memcpy(target, source, sizeof(uint16_t) * 25);
}

void recolorPacmanFrame(uint16_t* frame, uint16_t color) {
  for (int i = 0; i < 25; i++) {
    if (frame[i] != 0x0000) {
      frame[i] = color;
    }
  }
}

void flipPacmanFrame(uint16_t* frame) {
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      uint16_t temp = frame[row * 5 + col];
      frame[row * 5 + col] = frame[row * 5 + (4 - col)];
      frame[row * 5 + (4 - col)] = temp;
    }
  }
}

void rotatePacmanFrame(uint16_t* frame) {
  uint16_t temp[25];
  memcpy(temp, frame, sizeof(temp));
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 5; col++) {
      frame[row * 5 + col] = temp[(4 - row) + (col * 5)];
    }
  }
}

int countPacmanEdibleBlocks(const PacmanThemeState& state) {
  int count = 0;
  for (int row = 0; row < 12; row++) {
    for (int col = 0; col < 12; col++) {
      if (isPacmanVisibleTarget(state.map[row][col])) {
        count++;
      }
    }
  }
  return count;
}

void resetPacmanThemeState(PacmanThemeState* state) {
  memcpy(state->map, kPacmanMapTemplate, sizeof(kPacmanMapTemplate));
  state->x = 32;
  state->y = 2;
  state->direction = PACMAN_DIRECTION_RIGHT;
  state->mouthOpen = false;
  state->showSeconds = true;
  state->invincible = false;
  state->invincibleStartAt = 0;
  state->lastSecondBlinkAt = millis();
  state->stepCount = 0;
  state->showingWin = false;
  state->winPhase = 0;
  state->winPhaseStartedAt = 0;
  state->routeLength = 0;
  state->routeIndex = 0;
  if (!buildPacmanVariedRoute(state)) {
    state->routeLength = sizeof(kPacmanRecordedRoute);
    memcpy(state->routeDirections, kPacmanRecordedRoute, state->routeLength);
  }
  state->initialized = true;
}

void stepPacmanTheme(PacmanThemeState* state) {
  if (state->showingWin) {
    if (millis() - state->winPhaseStartedAt >= kPacmanWinDurationMs) {
      resetPacmanThemeState(state);
    }
    return;
  }

  bool movingAlongX = state->direction == PACMAN_DIRECTION_LEFT || state->direction == PACMAN_DIRECTION_RIGHT;
  bool alignedToBlock = movingAlongX ? ((state->x - 2) % 5 == 0) : ((state->y - 2) % 5 == 0);

  if (alignedToBlock) {
    int gridX = (state->x - 2) / 5;
    int gridY = (state->y - 2) / 5;
    uint8_t currentBlock = state->map[gridY][gridX];

    if (currentBlock == PACMAN_BLOCK_SUPER_FOOD) {
      state->invincible = true;
      state->invincibleStartAt = millis();
    }
    if (isPacmanVisibleTarget(currentBlock) || currentBlock == PACMAN_BLOCK_START) {
      state->map[gridY][gridX] = PACMAN_BLOCK_EMPTY;
    }

    if (countPacmanEdibleBlocks(*state) == 0) {
      state->showingWin = true;
      state->winPhase = 0;
      state->winPhaseStartedAt = millis();
      return;
    }

    if (state->routeIndex >= state->routeLength) {
      resetPacmanThemeState(state);
      return;
    }

    if (state->routeIndex < state->routeLength) {
      state->direction = static_cast<PacmanDirection>(state->routeDirections[state->routeIndex++]);
    }
  }

  if (!alignedToBlock) {
    if (state->direction == PACMAN_DIRECTION_RIGHT) {
      state->x += 1;
    } else if (state->direction == PACMAN_DIRECTION_LEFT) {
      state->x -= 1;
    } else if (state->direction == PACMAN_DIRECTION_DOWN) {
      state->y += 1;
    } else if (state->direction == PACMAN_DIRECTION_UP) {
      state->y -= 1;
    }
  } else {
    PacmanMapBlock moveTarget = getPacmanNextBlock(*state, state->direction);
    if (pacmanAllowsMovement(moveTarget)) {
      if (state->direction == PACMAN_DIRECTION_RIGHT) {
        state->x += 1;
      } else if (state->direction == PACMAN_DIRECTION_LEFT) {
        state->x -= 1;
      } else if (state->direction == PACMAN_DIRECTION_DOWN) {
        state->y += 1;
      } else if (state->direction == PACMAN_DIRECTION_UP) {
        state->y -= 1;
      }
    }
  }

  if (state->invincible && millis() - state->invincibleStartAt >= 7000) {
    state->invincible = false;
  }

  if (state->stepCount % 3 == 0) {
    state->mouthOpen = !state->mouthOpen;
  }
  state->stepCount++;
}

void drawPacmanSprite(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  PacmanDirection direction,
  bool mouthOpen,
  uint16_t color
) {
  uint16_t frame[25];
  copyPacmanFrame(frame, mouthOpen ? kPacmanFrameOpen : kPacmanFrameClosed);

  if (direction == PACMAN_DIRECTION_LEFT) {
    flipPacmanFrame(frame);
  } else if (direction == PACMAN_DIRECTION_DOWN) {
    flipPacmanFrame(frame);
    rotatePacmanFrame(frame);
    flipPacmanFrame(frame);
  } else if (direction == PACMAN_DIRECTION_UP) {
    rotatePacmanFrame(frame);
  }

  recolorPacmanFrame(frame, color);

  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 5; col++) {
      uint16_t pixel = frame[row * 5 + col];
      if (pixel != 0x0000) {
        setPixel(display, x + col, y + row, pixel);
      }
    }
  }
}

void drawPacmanMap(MatrixPanel_I2S_DMA* display, const PacmanThemeState& state) {
  const uint16_t wall = 0x0016;
  const uint16_t food = 0xB58C;
  const uint16_t superFood = 0xFBE0;

  fillRect(display, 0, 0, 64, 64, toColor(display, 0, 0, 0));

  drawRect(display, 0, 0, 64, 64, wall);
  drawRect(display, 1, 1, 62, 62, wall);

  for (int row = 0; row < 12; row++) {
    for (int col = 0; col < 12; col++) {
      uint8_t block = state.map[row][col];
      int px = col * 5 + 2;
      int py = row * 5 + 2;

      if (block == PACMAN_BLOCK_FOOD) {
        fillRect(display, px + 1, py + 2, 3, 1, food);
      } else if (block == PACMAN_BLOCK_WALL || block == PACMAN_BLOCK_CLOCK) {
        fillRect(display, px, py, 5, 5, wall);
      } else if (block == PACMAN_BLOCK_GATE) {
        fillRect(display, px + 1, py + 2, 3, 1, food);
      } else if (block == PACMAN_BLOCK_SUPER_FOOD) {
        fillRect(display, px + 1, py + 1, 3, 3, superFood);
      }
    }
  }
}

void renderPacmanTheme(
  MatrixPanel_I2S_DMA* display,
  const struct tm* timeinfo
) {
  if (!gPacmanThemeState.initialized) {
    resetPacmanThemeState(&gPacmanThemeState);
  }

  stepPacmanTheme(&gPacmanThemeState);
  drawPacmanMap(display, gPacmanThemeState);

  uint16_t pacmanColor = 0xFE40;
  if (gPacmanThemeState.invincible && (gPacmanThemeState.stepCount % 2 == 0)) {
    pacmanColor = static_cast<uint16_t>(random(0xFFFF));
  }
  drawPacmanSprite(
    display,
    gPacmanThemeState.x,
    gPacmanThemeState.y,
    gPacmanThemeState.direction,
    gPacmanThemeState.mouthOpen,
    pacmanColor
  );

  fillRect(display, 14, 19, 36, 26, toColor(display, 0, 0, 0));

  char hourText[3];
  char minuteText[3];
  char monthText[4];
  char weekText[4];
  if (timeinfo == nullptr) {
    snprintf(hourText, sizeof(hourText), "00");
    snprintf(minuteText, sizeof(minuteText), "00");
  } else {
    snprintf(hourText, sizeof(hourText), "%02d", getThemeDisplayHour(timeinfo));
    snprintf(minuteText, sizeof(minuteText), "%02d", timeinfo->tm_min);
  }
  formatMonthText(timeinfo, monthText, sizeof(monthText));
  formatWeekText(timeinfo, weekText, sizeof(weekText));

  display->setFont(&hourFont);
  display->setTextColor(0xFE40);
  display->setCursor(15, 28);
  display->print(hourText);
  display->print(" ");
  display->print(minuteText);

  if (!gPacmanThemeState.showingWin && gPacmanThemeState.showSeconds) {
    fillRect(display, 31, 24, 2, 2, 0xFE40);
    fillRect(display, 31, 29, 2, 2, 0xFE40);
  } else {
    fillRect(display, 31, 24, 2, 2, 0x0000);
    fillRect(display, 31, 29, 2, 2, 0x0000);
  }

  fillRect(display, 14, 34, 36, 12, toColor(display, 0, 0, 0));

  if (gPacmanThemeState.showingWin) {
    uint8_t colorIndex =
      ((millis() - gPacmanThemeState.winPhaseStartedAt) / kPacmanWinColorShiftMs) %
      (sizeof(kPacmanWinPalette) / sizeof(kPacmanWinPalette[0]));
    int16_t textX = 0;
    int16_t textY = 0;
    uint16_t textWidth = 0;
    uint16_t textHeight = 0;
    display->setFont(&Picopixel);
    display->setTextColor(
      toColor(
        display,
        kPacmanWinPalette[colorIndex][0],
        kPacmanWinPalette[colorIndex][1],
        kPacmanWinPalette[colorIndex][2]
      )
    );
    display->getTextBounds("GAME WIN", 0, 0, &textX, &textY, &textWidth, &textHeight);
    int16_t centeredX = 14 + static_cast<int16_t>((36 - textWidth) / 2) - textX;
    display->setCursor(centeredX, 41);
    display->print("GAME WIN");
  } else {
    display->setFont(&Picopixel);
    display->setTextColor(0xAD55);
    display->setCursor(15, 41);
    display->print(monthText);
    display->print(" ");
    if (timeinfo == nullptr) {
      display->print("--");
    } else {
      display->print(timeinfo->tm_mday);
    }
    display->print(" ");
    display->print(weekText);
  }
  display->setFont(nullptr);
}

void renderPokedexTheme(
  MatrixPanel_I2S_DMA* display,
  const struct tm* timeinfo
) {
  constexpr uint8_t kPokedexBackgroundScale = 144;
  constexpr uint8_t kPokedexPokemonScale = 156;
  constexpr uint8_t kPokedexAccentScale = 176;
  const uint16_t kLightGreen = scaleColor565(0x754d, kPokedexAccentScale);
  const uint16_t kDarkGreen = scaleColor565(0x0264, 196);
  const uint16_t kDarkBlue = scaleColor565(0x016D, 184);
  const uint16_t kLightBlack = scaleColor565(0x10c4, 172);
  const uint16_t kStatusLedOn = scaleColor565(display->color565(164, 244, 255), 160);
  const uint16_t kStatusLedOff = scaleColor565(display->color565(78, 116, 122), 170);
  const uint16_t kClockTextColor = scaleColor565(0xFFFF, 208);
  const uint16_t* const pokemons[] = {
    pokemon1, pokemon2, pokemon3, pokemon4, pokemon5, pokemon6, pokemon7
  };
  int minute = timeinfo == nullptr ? -1 : timeinfo->tm_min;
  int second = timeinfo == nullptr ? 0 : timeinfo->tm_sec;

  if (!gPokedexThemeState.initialized) {
    gPokedexThemeState.initialized = true;
    gPokedexThemeState.lastMinute = -1;
    gPokedexThemeState.pokemonIndex = 0;
    gPokedexThemeState.blinkSecond = -1;
    gPokedexThemeState.blinkColor = 0xFFFF;
    randomSeed(millis());
  }

  if (minute != gPokedexThemeState.lastMinute) {
    gPokedexThemeState.lastMinute = minute;
    gPokedexThemeState.pokemonIndex = random(7);
  }

  if (second != gPokedexThemeState.blinkSecond) {
    gPokedexThemeState.blinkSecond = second;
    gPokedexThemeState.blinkColor = (second % 2 == 0) ? kStatusLedOn : kStatusLedOff;
  }

  // 图鉴主题原图亮面占比很高，板载上会明显放大供电压力。
  // 这里仅收这个主题的像素亮度，不改其他主题和全局亮度语义。
  drawBitmap565Scaled(display, 0, 0, POKEDEX_BG, 64, 64, kPokedexBackgroundScale);

  drawBitmap565Scaled(
    display,
    8,
    21,
    pokemons[gPokedexThemeState.pokemonIndex],
    16,
    16,
    kPokedexPokemonScale
  );

  fillRect(display, 35, 17, 26, 14, kLightBlack);

  char hourText[3];
  char minuteText[3];
  if (timeinfo == nullptr) {
    snprintf(hourText, sizeof(hourText), "0");
    snprintf(minuteText, sizeof(minuteText), "00");
  } else {
    snprintf(hourText, sizeof(hourText), "%d", getThemeDisplayHour(timeinfo));
    snprintf(minuteText, sizeof(minuteText), "%02d", timeinfo->tm_min);
  }

  display->setFont(&PKMN_RBYGSC4pt7b);
  display->setTextColor(kClockTextColor);
  display->setCursor(35, 22);
  display->print(hourText);
  display->setCursor(46, 30);
  display->print(minuteText);
  if (themeUses12HourFormat()) {
    display->drawBitmap(55, 18, isThemeAM(timeinfo) ? AM_SIGN : PM_SIGN, 4, 4, kClockTextColor);
  }

  if (timeinfo != nullptr) {
    int weekday = timeinfo->tm_wday;
    int weekdayX = 36 + ((weekday > 3 ? (weekday - 4) : weekday) * 6);
    int weekdayY = 35 + (weekday > 3 ? 5 : 0);
    fillRect(display, weekdayX, weekdayY, 5, 4, kDarkBlue);
  }

  if (second == 0) {
    fillRect(display, 9, 53, 11, 5, kLightGreen);
  } else {
    fillRect(display, 9, 53, ((10 * second) / 59) + 1, 5, kDarkGreen);
  }

  fillRect(display, 5, 4, 2, 4, gPokedexThemeState.blinkColor);
  fillRect(display, 4, 5, 4, 2, gPokedexThemeState.blinkColor);
  display->setFont(nullptr);
}

void renderMinimalDigitalTheme(
  MatrixPanel_I2S_DMA* display,
  const char* timeText,
  const char* dateText,
  const char* weekText
) {
  fillRect(display, 0, 0, 64, 64, toColor(display, 7, 17, 14));
  for (int x = 5; x <= 59; x += 9) {
    fillRect(display, x, 50, 4, 1, toColor(display, 21, 54, 46));
  }
  drawClockTextAligned(display, timeText, 32, 16, toColor(display, 123, 242, 208), CLOCK_FONT_LCD_6X8, 1, "center");
  drawTinyTextAligned(display, dateText, 32, 37, toColor(display, 210, 255, 243), 1, "center");
  drawTinyTextAligned(display, weekText, 32, 46, toColor(display, 76, 199, 165), 1, "center");
  fillRect(display, 18, 56, 28, 1, toColor(display, 30, 91, 74));
}

void renderFrameClockTheme(
  MatrixPanel_I2S_DMA* display,
  const char* timeText,
  const char* dateText,
  const char* weekText
) {
  fillRect(display, 0, 0, 64, 64, toColor(display, 27, 18, 8));
  drawRect(display, 2, 2, 60, 60, toColor(display, 255, 207, 90));
  drawRect(display, 6, 6, 52, 52, toColor(display, 123, 75, 20));
  fillRect(display, 2, 2, 4, 4, toColor(display, 255, 243, 180));
  fillRect(display, 58, 2, 4, 4, toColor(display, 255, 243, 180));
  fillRect(display, 2, 58, 4, 4, toColor(display, 255, 243, 180));
  fillRect(display, 58, 58, 4, 4, toColor(display, 255, 243, 180));
  drawClockTextAligned(display, timeText, 32, 18, toColor(display, 255, 207, 90), CLOCK_FONT_CLASSIC_5X7, 1, "center");
  drawTinyTextAligned(display, dateText, 32, 35, toColor(display, 255, 245, 203), 1, "center");
  drawTinyTextAligned(display, weekText, 32, 44, toColor(display, 255, 159, 67), 1, "center");
}

void renderCenterpieceBadgeTheme(
  MatrixPanel_I2S_DMA* display,
  const char* timeText,
  const char* dateText,
  const char* weekText
) {
  fillRect(display, 0, 0, 64, 64, toColor(display, 24, 13, 18));
  drawFilledCircle(display, 27, 33, 15, toColor(display, 255, 86, 120));
  drawFilledCircle(display, 27, 33, 10, toColor(display, 255, 143, 165));
  drawRect(display, 12, 18, 30, 30, toColor(display, 255, 209, 95));
  drawLine(display, 17, 23, 37, 43, toColor(display, 255, 241, 210));
  drawLine(display, 37, 23, 17, 43, toColor(display, 255, 241, 210));
  drawClockTextAligned(display, timeText, 60, 6, toColor(display, 255, 224, 231), CLOCK_FONT_ROUNDED_4X6, 1, "right");
  drawTinyTextAligned(display, dateText, 60, 15, toColor(display, 255, 157, 175), 1, "right");
  drawTinyTextAligned(display, weekText, 60, 23, toColor(display, 255, 209, 95), 1, "right");
}

void renderTopbarMotionTheme(
  MatrixPanel_I2S_DMA* display,
  const char* timeText,
  const char* dateText,
  const char* weekText
) {
  fillRect(display, 0, 0, 64, 64, toColor(display, 6, 16, 25));
  fillRect(display, 0, 0, 64, 14, toColor(display, 16, 40, 58));
  drawClockText(display, timeText, 4, 4, toColor(display, 94, 203, 255), CLOCK_FONT_MINIMAL_3X5, 2);
  for (int x = 0; x < 64; x += 4) {
    int waveHeight = 28 + ((x / 4) % 4) * 3;
    fillRect(display, x, waveHeight, 2, 20, toColor(display, 26, 66, 96));
    fillRect(display, x + 2, waveHeight + 4, 2, 16, toColor(display, 46, 134, 183));
  }
  fillRect(display, 0, 42, 64, 1, toColor(display, 94, 203, 255));
  drawTinyTextAligned(display, dateText, 4, 46, toColor(display, 216, 245, 255), 1, "left");
  drawTinyTextAligned(display, weekText, 60, 46, toColor(display, 127, 240, 255), 1, "right");
  drawFilledCircle(display, 56, 18, 2, toColor(display, 127, 240, 255));
  drawFilledCircle(display, 49, 24, 1, toColor(display, 94, 203, 255));
}

void renderFestivalPosterTheme(
  MatrixPanel_I2S_DMA* display,
  const char* timeText,
  const char* dateText,
  const char* weekText
) {
  unsigned long tick = millis();
  fillRect(display, 0, 0, 64, 64, toColor(display, 42, 9, 9));
  drawFilledCircle(display, 14, 12, 6, toColor(display, 255, 155, 61));
  drawFilledCircle(display, 50, 14, 5, toColor(display, 255, 207, 90));
  drawLine(display, 14, 0, 14, 6, toColor(display, 255, 207, 90));
  drawLine(display, 50, 0, 50, 8, toColor(display, 255, 207, 90));
  fillRect(display, 8, 48, 48, 8, toColor(display, 92, 16, 16));
  fillRect(display, 12, 50, 40, 4, toColor(display, 122, 21, 21));
  drawClockTextAligned(display, timeText, 32, 17, toColor(display, 255, 240, 214), CLOCK_FONT_RETRO_5X7, 1, "center");
  drawTinyTextAligned(display, dateText, 32, 35, toColor(display, 255, 208, 89), 1, "center");
  drawTinyTextAligned(display, weekText, 32, 44, toColor(display, 255, 158, 99), 1, "center");
  applySparkles(display, tick, toColor(display, 255, 207, 90), toColor(display, 255, 247, 207), toColor(display, 255, 155, 61));
}

void renderPixelArcadeTheme(
  MatrixPanel_I2S_DMA* display,
  const char* timeText,
  const char* dateText,
  const char* weekText
) {
  fillRect(display, 0, 0, 64, 64, toColor(display, 9, 17, 10));
  drawRect(display, 3, 3, 58, 58, toColor(display, 54, 93, 40));
  fillRect(display, 6, 6, 52, 52, toColor(display, 16, 27, 18));
  drawClockTextAligned(display, timeText, 32, 17, toColor(display, 243, 255, 240), CLOCK_FONT_SEVEN_SEG_5X7, 1, "center");
  fillRect(display, 11, 14, 3, 3, toColor(display, 255, 100, 100));
  fillRect(display, 16, 14, 3, 3, toColor(display, 255, 100, 100));
  fillRect(display, 21, 14, 3, 3, toColor(display, 255, 100, 100));
  drawTinyTextAligned(display, dateText, 6, 40, toColor(display, 141, 255, 106), 1, "left");
  drawTinyTextAligned(display, weekText, 58, 40, toColor(display, 94, 210, 255), 1, "right");
  drawRect(display, 8, 48, 48, 7, toColor(display, 36, 63, 25));
  for (int i = 0; i < 6; i++) {
    fillRect(display, 11 + i * 7, 50 - (i % 3), 4, 3 + (i % 3), toColor(display, 141, 255, 106));
  }
}

void renderFireworksBloomTheme(
  MatrixPanel_I2S_DMA* display,
  const char* timeText,
  const char* dateText,
  const char* weekText
) {
  unsigned long tick = millis();
  int phase = (tick / 220) % 6;
  fillRect(display, 0, 0, 64, 64, toColor(display, 16, 7, 11));
  fillRect(display, 0, 46, 64, 18, toColor(display, 33, 13, 13));
  drawLine(display, 0, 46, 63, 46, toColor(display, 90, 28, 28));
  drawClockTextAligned(display, timeText, 32, 10, toColor(display, 255, 246, 210), CLOCK_FONT_RETRO_5X7, 1, "center");
  drawTinyTextAligned(display, dateText, 32, 49, toColor(display, 255, 207, 106), 1, "center");
  drawTinyTextAligned(display, weekText, 32, 57, toColor(display, 255, 143, 102), 1, "center");
  drawFireworkBurst(display, 18, 18, 3 + phase, toColor(display, 255, 179, 71), toColor(display, 255, 217, 95));
  drawFireworkBurst(display, 46, 14, 2 + ((phase + 2) % 4), toColor(display, 255, 247, 207), toColor(display, 255, 155, 61));
}

void renderFireworksRiseTheme(
  MatrixPanel_I2S_DMA* display,
  const char* timeText,
  const char* dateText,
  const char* weekText
) {
  unsigned long tick = millis();
  int phase = (tick / 200) % 8;
  fillRect(display, 0, 0, 64, 64, toColor(display, 4, 9, 21));
  fillRect(display, 0, 52, 64, 12, toColor(display, 8, 17, 28));
  for (int x = 0; x < 64; x += 8) {
    fillRect(display, x, 54 + ((x / 8) % 2), 4, 8, toColor(display, 13, 32, 50));
  }
  int rocketY = 46 - phase * 4;
  drawLine(display, 20, 52, 20, rocketY, toColor(display, 95, 182, 255));
  drawLine(display, 44, 52, 44, rocketY + 4, toColor(display, 137, 216, 255));
  drawFireworkBurst(display, 20, rocketY, 2 + (phase % 4), toColor(display, 223, 244, 255), toColor(display, 95, 182, 255));
  drawFireworkBurst(display, 44, rocketY + 4, 2 + ((phase + 2) % 4), toColor(display, 137, 216, 255), toColor(display, 223, 244, 255));
  drawClockTextAligned(display, timeText, 60, 6, toColor(display, 242, 251, 255), CLOCK_FONT_ROUNDED_4X6, 1, "right");
  drawTinyTextAligned(display, dateText, 60, 15, toColor(display, 156, 215, 255), 1, "right");
  drawTinyTextAligned(display, weekText, 60, 23, toColor(display, 107, 192, 255), 1, "right");
}

void renderCanvasTheme(
  MatrixPanel_I2S_DMA* display,
  const struct tm* timeinfo,
  const PixelData* imagePixels,
  int imagePixelCount
) {
  for (size_t i = 0; i < sizeof(kCanvasSceneElements) / sizeof(kCanvasSceneElements[0]); i++) {
    drawCanvasElement(display, kCanvasSceneElements[i], timeinfo, imagePixels, imagePixelCount);
  }

  unsigned long tick = millis();
  for (size_t i = 0; i < sizeof(kCanvasSceneSprites) / sizeof(kCanvasSceneSprites[0]); i++) {
    drawCanvasSprite(display, kCanvasSceneSprites[i], tick);
  }

  display->setFont(nullptr);
}

void renderFallbackTheme(MatrixPanel_I2S_DMA* display, const char* timeText, const char* themeId) {
  fillRect(display, 0, 0, 64, 64, toColor(display, 8, 12, 20));
  drawRect(display, 2, 2, 60, 60, toColor(display, 79, 127, 255));
  drawClockTextAligned(display, timeText, 32, 20, toColor(display, 255, 255, 255), CLOCK_FONT_CLASSIC_5X7, 1, "center");
  drawTinyTextAligned(display, "THEME", 32, 38, toColor(display, 127, 240, 255), 1, "center");
  if (themeId != nullptr && themeId[0] != '\0') {
    drawTinyTextAligned(display, themeId, 32, 48, toColor(display, 160, 180, 220), 1, "center");
  }
}

}  // namespace

uint16_t getThemeRefreshIntervalMs(const char* themeId) {
  if (isThemeId(themeId, "glowxel_festival_poster") ||
      isThemeId(themeId, "glowxel_fireworks_bloom") ||
      isThemeId(themeId, "glowxel_fireworks_rise")) {
    return 220;
  }
  if (isThemeId(themeId, "clockwise_mario")) {
    return 50;
  }
  if (isThemeId(themeId, "clockwise_words")) {
    return 240;
  }
  if (isThemeId(themeId, "clockwise_worldmap")) {
    return 240;
  }
  if (isThemeId(themeId, "clockwise_castlevania")) {
    return 140;
  }
  if (isThemeId(themeId, "clockwise_pacman")) {
    return 75;
  }
  if (isThemeId(themeId, "clockwise_pokedex")) {
    return 280;
  }
  if (isThemeId(themeId, "clockwise_canvas")) {
    return 120;
  }
  return 1000;
}

bool themeUsesStoredImage(const char* themeId) {
  return isThemeId(themeId, "clockwise_canvas");
}

void resetThemeRuntimeState(const char* themeId) {
  if (isThemeId(themeId, "clockwise_pacman")) {
    gPacmanThemeState.initialized = false;
    gLastThemeId[0] = '\0';
    return;
  }
  handleThemeStateReset(nullptr);
}

void renderThemeFrame(
  MatrixPanel_I2S_DMA* display,
  const char* themeId,
  const struct tm* timeinfo,
  const PixelData* imagePixels,
  int imagePixelCount
) {
  handleThemeStateReset(themeId);

  char timeText[6];
  char dateText[8];
  char weekText[4];
  formatTimeText(timeinfo, timeText, sizeof(timeText));
  formatDateText(timeinfo, dateText, sizeof(dateText));
  formatWeekText(timeinfo, weekText, sizeof(weekText));

  if (isThemeId(themeId, "glowxel_minimal_digital")) {
    renderMinimalDigitalTheme(display, timeText, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "glowxel_frame_clock")) {
    renderFrameClockTheme(display, timeText, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "glowxel_centerpiece_badge")) {
    renderCenterpieceBadgeTheme(display, timeText, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "glowxel_topbar_motion")) {
    renderTopbarMotionTheme(display, timeText, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "glowxel_festival_poster")) {
    renderFestivalPosterTheme(display, timeText, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "glowxel_pixel_arcade")) {
    renderPixelArcadeTheme(display, timeText, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "glowxel_fireworks_bloom")) {
    renderFireworksBloomTheme(display, timeText, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "glowxel_fireworks_rise")) {
    renderFireworksRiseTheme(display, timeText, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "clockwise_mario")) {
    renderMarioTheme(display, timeinfo);
    return;
  }
  if (isThemeId(themeId, "clockwise_words")) {
    renderWordsTheme(display, timeinfo, dateText, weekText);
    return;
  }
  if (isThemeId(themeId, "clockwise_worldmap")) {
    renderWorldMapTheme(display, timeinfo);
    return;
  }
  if (isThemeId(themeId, "clockwise_castlevania")) {
    renderCastlevaniaTheme(display, timeinfo);
    return;
  }
  if (isThemeId(themeId, "clockwise_pacman")) {
    renderPacmanTheme(display, timeinfo);
    return;
  }
  if (isThemeId(themeId, "clockwise_pokedex")) {
    renderPokedexTheme(display, timeinfo);
    return;
  }
  if (isThemeId(themeId, "clockwise_canvas")) {
    renderCanvasTheme(display, timeinfo, imagePixels, imagePixelCount);
    return;
  }

  renderFallbackTheme(display, timeText, themeId);
}
