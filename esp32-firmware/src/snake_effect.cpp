#include "snake_effect.h"

#include <string.h>
#include <time.h>

#include "clock_font_renderer.h"
#include "display_manager.h"

namespace {
constexpr int kPanelSize = 64;
constexpr int kMaxSnakeCells = kPanelSize * kPanelSize;
constexpr int kSnakeClockTextTop = 2;
constexpr int kSnakeClockPaddingX = 1;
constexpr int kSnakeClockPaddingY = 1;

struct SnakePoint {
  int8_t x;
  int8_t y;
};

struct SnakeClockLayout {
  bool active;
  int textX;
  int textY;
  int textWidth;
  int textHeight;
  int reservedLeft;
  int reservedTop;
  int reservedRight;
  int reservedBottom;
};

struct SnakeRgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

const int8_t kDirections[4][2] = {
  {0, -1},
  {1, 0},
  {0, 1},
  {-1, 0}
};

SnakeModeConfig s_config = {
  6,
  2,
  86,
  214,
  120,
  255,
  168,
  84,
  CLOCK_FONT_MINIMAL_3X5,
  false,
  "gradient"
};

bool s_active = false;
bool s_needsRender = true;
unsigned long s_lastTickAt = 0;
char s_lastError[128] = "";

SnakePoint* s_snake = nullptr;
int s_snakeLength = 0;
SnakePoint s_food = {0, 0};
int s_snakeGridWidth = 64;
int s_snakeGridHeight = 64;
int s_snakePixelWidth = 2;
int s_snakeOriginX = 0;
int s_snakeOriginY = 0;
int s_snakeDirection = 1;
bool s_snakeDead = false;
bool s_snakeWon = false;
int s_snakeHoldTicks = 0;
int s_snakeCapacity = 0;
int s_playableCellCount = 0;
uint8_t* s_snakeVisited = nullptr;
int16_t* s_snakeDistance = nullptr;
uint16_t* s_snakeQueue = nullptr;
SnakeClockLayout s_clockLayout = {};
char s_lastSnakeClockText[9] = "";
bool s_lastSnakeClockTextValid = false;
bool s_lastSnakeClockShowSeconds = false;

void clearLastError() {
  s_lastError[0] = '\0';
}

void setLastError(const char* message) {
  if (message == nullptr) {
    clearLastError();
    return;
  }
  snprintf(s_lastError, sizeof(s_lastError), "%s", message);
}

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return MatrixPanel_I2S_DMA::color565(r, g, b);
}

void* allocateEffectBuffer(size_t size) {
  if (size == 0) {
    return nullptr;
  }

  if (psramFound()) {
    void* buffer = ps_malloc(size);
    if (buffer != nullptr) {
      return buffer;
    }
  }

  return malloc(size);
}

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

int positiveModulo(int value, int base) {
  if (base <= 0) {
    return 0;
  }
  int result = value % base;
  if (result < 0) {
    result += base;
  }
  return result;
}

uint8_t clampByteInt(int value) {
  return static_cast<uint8_t>(clampInt(value, 0, 255));
}

unsigned long resolveTickInterval(uint8_t speed, unsigned long slowMs, unsigned long fastMs) {
  int safeSpeed = clampInt((int)speed, 1, 10);
  long span = (long)slowMs - (long)fastMs;
  long interval = (long)slowMs - ((long)(safeSpeed - 1) * span) / 9L;
  if (interval < (long)fastMs) {
    interval = (long)fastMs;
  }
  if (interval > (long)slowMs) {
    interval = (long)slowMs;
  }
  return (unsigned long)interval;
}

uint32_t pseudoRandom(uint32_t seed) {
  return seed * 1664525UL + 1013904223UL;
}

bool pointEquals(const SnakePoint& a, const SnakePoint& b) {
  return a.x == b.x && a.y == b.y;
}

bool isSupportedSnakeSkin(const char* snakeSkin) {
  if (snakeSkin == nullptr) {
    return false;
  }
  return strcmp(snakeSkin, "solid") == 0 ||
         strcmp(snakeSkin, "gradient") == 0 ||
         strcmp(snakeSkin, "spotted") == 0;
}

void fillBlock(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  int size,
  uint8_t r,
  uint8_t g,
  uint8_t b
) {
  if (display == nullptr) {
    return;
  }
  display->fillRect(x, y, size, size, rgb565(r, g, b));
}

void drawGlowPixel(MatrixPanel_I2S_DMA* display, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (display == nullptr) {
    return;
  }
  if (x < 0 || x >= kPanelSize || y < 0 || y >= kPanelSize) {
    return;
  }
  display->drawPixel(x, y, rgb565(r, g, b));
}

void resetSnakeClockLayout() {
  s_clockLayout = {};
}

void configureSnakeClockLayout() {
  resetSnakeClockLayout();

  const char* sampleText = s_config.showSeconds ? "88:88:88" : "88:88";
  const int textWidth = getClockTextWidth(sampleText, s_config.font, 1);
  const int textHeight = getClockTextHeight(s_config.font, 1);
  const int textX = (kPanelSize - textWidth) / 2;
  const int textY = kSnakeClockTextTop;

  s_clockLayout.active = true;
  s_clockLayout.textX = textX;
  s_clockLayout.textY = textY;
  s_clockLayout.textWidth = textWidth;
  s_clockLayout.textHeight = textHeight;
  s_clockLayout.reservedLeft = clampInt(textX - kSnakeClockPaddingX, 0, kPanelSize - 1);
  s_clockLayout.reservedTop = clampInt(textY - kSnakeClockPaddingY, 0, kPanelSize - 1);
  s_clockLayout.reservedRight = clampInt(
    textX + textWidth - 1 + kSnakeClockPaddingX,
    0,
    kPanelSize - 1
  );
  s_clockLayout.reservedBottom = clampInt(
    textY + textHeight - 1 + kSnakeClockPaddingY,
    0,
    kPanelSize - 1
  );
}

bool isSnakeReservedCell(int x, int y) {
  if (!s_clockLayout.active) {
    return false;
  }

  const int cellLeft = s_snakeOriginX + x * s_snakePixelWidth;
  const int cellTop = s_snakeOriginY + y * s_snakePixelWidth;
  const int cellRight = cellLeft + s_snakePixelWidth - 1;
  const int cellBottom = cellTop + s_snakePixelWidth - 1;

  return !(cellRight < s_clockLayout.reservedLeft ||
           cellLeft > s_clockLayout.reservedRight ||
           cellBottom < s_clockLayout.reservedTop ||
           cellTop > s_clockLayout.reservedBottom);
}

int countSnakePlayableCells() {
  int count = 0;
  for (int y = 0; y < s_snakeGridHeight; y++) {
    for (int x = 0; x < s_snakeGridWidth; x++) {
      if (!isSnakeReservedCell(x, y)) {
        count += 1;
      }
    }
  }
  return count;
}

SnakeRgb resolveSnakeSegmentColor(int segmentIndex) {
  if (s_snakeDead) {
    return {255, 80, 80};
  }
  if (s_snakeWon) {
    return {120, 210, 255};
  }

  SnakeRgb color = {
    s_config.snakeColorR,
    s_config.snakeColorG,
    s_config.snakeColorB
  };

  if (strcmp(s_config.snakeSkin, "gradient") == 0 && s_snakeLength > 1) {
    const int gradientSpan = max(1, s_snakeLength - 1);
    const int factorPercent = 120 - (segmentIndex * 55) / gradientSpan;
    color.r = clampByteInt(((int)color.r * factorPercent) / 100);
    color.g = clampByteInt(((int)color.g * factorPercent) / 100);
    color.b = clampByteInt(((int)color.b * factorPercent) / 100);
  }

  if (segmentIndex == 0) {
    color.r = clampByteInt((int)color.r + ((255 - (int)color.r) * 35) / 100);
    color.g = clampByteInt((int)color.g + ((255 - (int)color.g) * 35) / 100);
    color.b = clampByteInt((int)color.b + ((255 - (int)color.b) * 35) / 100);
  }

  return color;
}

void drawSnakeCellBorder(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  int size,
  const SnakeRgb& color,
  bool emphasize
) {
  if (display == nullptr || size < 2) {
    return;
  }

  const SnakeRgb light = {
    clampByteInt((int)color.r + (emphasize ? 56 : 28)),
    clampByteInt((int)color.g + (emphasize ? 56 : 28)),
    clampByteInt((int)color.b + (emphasize ? 56 : 28))
  };
  const SnakeRgb dark = {
    clampByteInt((int)color.r - (emphasize ? 34 : 18)),
    clampByteInt((int)color.g - (emphasize ? 34 : 18)),
    clampByteInt((int)color.b - (emphasize ? 34 : 18))
  };

  for (int offset = 0; offset < size; offset++) {
    drawGlowPixel(display, x + offset, y, light.r, light.g, light.b);
    drawGlowPixel(display, x, y + offset, light.r, light.g, light.b);
    drawGlowPixel(display, x + offset, y + size - 1, dark.r, dark.g, dark.b);
    drawGlowPixel(display, x + size - 1, y + offset, dark.r, dark.g, dark.b);
  }
}

void drawSnakeCellSpots(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  int size,
  const SnakeRgb& color,
  int patternSeed
) {
  if (display == nullptr || size < 2) {
    return;
  }

  const SnakeRgb spot = {
    clampByteInt((int)color.r - 46),
    clampByteInt((int)color.g - 46),
    clampByteInt((int)color.b - 46)
  };
  const int spotCount = size >= 4 ? 3 : 2;

  for (int index = 0; index < spotCount; index++) {
    const int seed = patternSeed + index * 19;
    const int localX = positiveModulo(seed * 3 + 1, size);
    const int localY = positiveModulo(seed * 5 + 2, size);
    drawGlowPixel(display, x + localX, y + localY, spot.r, spot.g, spot.b);
    if (size >= 4 && index == 0) {
      const int mirrorX = positiveModulo(localX + 1, size);
      drawGlowPixel(display, x + mirrorX, y + localY, spot.r, spot.g, spot.b);
    }
  }
}

void drawSnakeCell(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  int size,
  const SnakeRgb& color,
  bool emphasize,
  bool drawSpots,
  int patternSeed
) {
  fillBlock(display, x, y, size, color.r, color.g, color.b);

  if (size == 1) {
    const uint8_t haloR = emphasize
      ? static_cast<uint8_t>(min(255, color.r / 2 + 48))
      : static_cast<uint8_t>(color.r / 3);
    const uint8_t haloG = emphasize
      ? static_cast<uint8_t>(min(255, color.g / 2 + 48))
      : static_cast<uint8_t>(color.g / 3);
    const uint8_t haloB = emphasize
      ? static_cast<uint8_t>(min(255, color.b / 2 + 48))
      : static_cast<uint8_t>(color.b / 3);
    const int orthogonalOffsets[4][2] = {
      {-1, 0},
      {1, 0},
      {0, -1},
      {0, 1}
    };
    for (int i = 0; i < 4; i++) {
      drawGlowPixel(display, x + orthogonalOffsets[i][0], y + orthogonalOffsets[i][1], haloR, haloG, haloB);
    }

    if (!emphasize) {
      return;
    }

    const uint8_t cornerR = static_cast<uint8_t>(haloR / 2);
    const uint8_t cornerG = static_cast<uint8_t>(haloG / 2);
    const uint8_t cornerB = static_cast<uint8_t>(haloB / 2);
    const int diagonalOffsets[4][2] = {
      {-1, -1},
      {1, -1},
      {-1, 1},
      {1, 1}
    };
    for (int i = 0; i < 4; i++) {
      drawGlowPixel(display, x + diagonalOffsets[i][0], y + diagonalOffsets[i][1], cornerR, cornerG, cornerB);
    }
    return;
  }

  drawSnakeCellBorder(display, x, y, size, color, emphasize);
  if (drawSpots) {
    drawSnakeCellSpots(display, x, y, size, color, patternSeed);
  }
}

void releaseSnakeBuffers() {
  if (s_snake != nullptr) {
    free(s_snake);
    s_snake = nullptr;
  }
  if (s_snakeVisited != nullptr) {
    free(s_snakeVisited);
    s_snakeVisited = nullptr;
  }
  if (s_snakeDistance != nullptr) {
    free(s_snakeDistance);
    s_snakeDistance = nullptr;
  }
  if (s_snakeQueue != nullptr) {
    free(s_snakeQueue);
    s_snakeQueue = nullptr;
  }
  s_snakeCapacity = 0;
}

bool ensureSnakeBuffers(int totalCells) {
  if (totalCells <= 0 || totalCells > kMaxSnakeCells) {
    setLastError("snake invalid grid cells");
    return false;
  }
  if (s_snakeCapacity == totalCells &&
      s_snake != nullptr &&
      s_snakeVisited != nullptr &&
      s_snakeDistance != nullptr &&
      s_snakeQueue != nullptr) {
    clearLastError();
    return true;
  }

  releaseSnakeBuffers();
  s_snake = static_cast<SnakePoint*>(allocateEffectBuffer(sizeof(SnakePoint) * totalCells));
  s_snakeVisited = static_cast<uint8_t*>(allocateEffectBuffer(totalCells));
  s_snakeDistance = static_cast<int16_t*>(allocateEffectBuffer(sizeof(int16_t) * totalCells));
  s_snakeQueue = static_cast<uint16_t*>(allocateEffectBuffer(sizeof(uint16_t) * totalCells));
  if (s_snake == nullptr || s_snakeVisited == nullptr || s_snakeDistance == nullptr || s_snakeQueue == nullptr) {
    snprintf(
      s_lastError,
      sizeof(s_lastError),
      "snake buffer allocation failed: cells=%d freeHeap=%u freePsram=%u",
      totalCells,
      ESP.getFreeHeap(),
      ESP.getFreePsram()
    );
    Serial.printf(
      "[SNAKE] 缓冲分配失败, cells=%d, freeHeap=%u, freePsram=%u\n",
      totalCells,
      ESP.getFreeHeap(),
      ESP.getFreePsram()
    );
    releaseSnakeBuffers();
    return false;
  }
  s_snakeCapacity = totalCells;
  clearLastError();
  return true;
}

int snakeIndex(int x, int y) {
  return y * s_snakeGridWidth + x;
}

bool isSnakeCellOccupied(int x, int y, int ignoreTailCount) {
  int length = s_snakeLength - ignoreTailCount;
  for (int i = 0; i < length; i++) {
    if (s_snake[i].x == x && s_snake[i].y == y) {
      return true;
    }
  }
  return false;
}

void placeSnakeFood() {
  uint32_t seed = (uint32_t)millis() ^ (uint32_t)(s_snakeLength * 2654435761UL);
  int available = s_playableCellCount - s_snakeLength;
  if (available <= 0) {
    s_food = {-1, -1};
    return;
  }

  int target = (int)(pseudoRandom(seed) % (uint32_t)available);
  int current = 0;
  for (int y = 0; y < s_snakeGridHeight; y++) {
    for (int x = 0; x < s_snakeGridWidth; x++) {
      if (isSnakeReservedCell(x, y) || isSnakeCellOccupied(x, y, 0)) {
        continue;
      }
      if (current == target) {
        s_food = {(int8_t)x, (int8_t)y};
        return;
      }
      current += 1;
    }
  }
}

void initSnake() {
  s_snakePixelWidth = s_config.snakeWidth;
  s_snakeGridWidth = kPanelSize / s_snakePixelWidth;
  s_snakeGridHeight = kPanelSize / s_snakePixelWidth;
  s_snakeOriginX = (kPanelSize - s_snakeGridWidth * s_snakePixelWidth) / 2;
  s_snakeOriginY = (kPanelSize - s_snakeGridHeight * s_snakePixelWidth) / 2;
  configureSnakeClockLayout();
  s_playableCellCount = countSnakePlayableCells();

  if (s_playableCellCount <= 4) {
    setLastError("snake playable cells too few");
    s_active = false;
    return;
  }

  if (!ensureSnakeBuffers(s_snakeGridWidth * s_snakeGridHeight)) {
    s_active = false;
    return;
  }

  s_snakeLength = 4;
  int startX = s_snakeGridWidth / 2;
  int startY = s_snakeGridHeight / 2;
  s_snake[0] = {(int8_t)startX, (int8_t)startY};
  s_snake[1] = {(int8_t)(startX - 1), (int8_t)startY};
  s_snake[2] = {(int8_t)(startX - 2), (int8_t)startY};
  s_snake[3] = {(int8_t)(startX - 3), (int8_t)startY};
  s_snakeDirection = 1;
  s_snakeDead = false;
  s_snakeWon = false;
  s_snakeHoldTicks = 0;
  placeSnakeFood();
  s_needsRender = true;
}

bool buildSnakeDistanceMap() {
  int total = s_snakeGridWidth * s_snakeGridHeight;
  memset(s_snakeVisited, 0, total);
  for (int i = 0; i < total; i++) {
    s_snakeDistance[i] = -1;
  }

  if (s_food.x < 0 || s_food.y < 0) {
    return false;
  }

  int head = 0;
  int tail = 0;
  int foodIdx = snakeIndex(s_food.x, s_food.y);
  s_snakeQueue[tail++] = (uint16_t)foodIdx;
  s_snakeVisited[foodIdx] = 1;
  s_snakeDistance[foodIdx] = 0;

  while (head < tail) {
    int current = (int)s_snakeQueue[head++];
    int cx = current % s_snakeGridWidth;
    int cy = current / s_snakeGridWidth;

    for (int dir = 0; dir < 4; dir++) {
      int nx = cx + kDirections[dir][0];
      int ny = cy + kDirections[dir][1];
      if (nx < 0 || nx >= s_snakeGridWidth || ny < 0 || ny >= s_snakeGridHeight) {
        continue;
      }
      if (isSnakeReservedCell(nx, ny) || isSnakeCellOccupied(nx, ny, 1)) {
        continue;
      }
      int nextIdx = snakeIndex(nx, ny);
      if (s_snakeVisited[nextIdx]) {
        continue;
      }
      s_snakeVisited[nextIdx] = 1;
      s_snakeDistance[nextIdx] = s_snakeDistance[current] + 1;
      s_snakeQueue[tail++] = (uint16_t)nextIdx;
    }
  }

  return true;
}

bool canSnakeMoveTo(int x, int y) {
  if (x < 0 || x >= s_snakeGridWidth || y < 0 || y >= s_snakeGridHeight) {
    return false;
  }
  if (isSnakeReservedCell(x, y)) {
    return false;
  }
  bool eating = s_food.x == x && s_food.y == y;
  return !isSnakeCellOccupied(x, y, eating ? 0 : 1);
}

int pickSnakeDirection() {
  bool hasDistanceMap = buildSnakeDistanceMap();
  int bestDir = -1;
  int bestDistance = 32767;
  for (int dir = 0; dir < 4; dir++) {
    int nx = s_snake[0].x + kDirections[dir][0];
    int ny = s_snake[0].y + kDirections[dir][1];
    if (!canSnakeMoveTo(nx, ny)) {
      continue;
    }
    int nextIdx = snakeIndex(nx, ny);
    int distance = hasDistanceMap ? s_snakeDistance[nextIdx] : -1;
    if (distance >= 0 && distance < bestDistance) {
      bestDistance = distance;
      bestDir = dir;
    }
  }

  if (bestDir >= 0) {
    return bestDir;
  }

  for (int dir = 0; dir < 4; dir++) {
    int nx = s_snake[0].x + kDirections[dir][0];
    int ny = s_snake[0].y + kDirections[dir][1];
    if (canSnakeMoveTo(nx, ny)) {
      return dir;
    }
  }
  return -1;
}

void updateSnake() {
  if (s_snakeWon || s_snakeDead) {
    s_snakeHoldTicks += 1;
    if (s_snakeHoldTicks >= 20) {
      initSnake();
    }
    s_needsRender = true;
    return;
  }

  int nextDirection = pickSnakeDirection();
  if (nextDirection < 0) {
    s_snakeDead = true;
    s_snakeHoldTicks = 0;
    s_needsRender = true;
    return;
  }

  s_snakeDirection = nextDirection;
  SnakePoint nextHead = {
    (int8_t)(s_snake[0].x + kDirections[s_snakeDirection][0]),
    (int8_t)(s_snake[0].y + kDirections[s_snakeDirection][1])
  };
  bool eating = pointEquals(nextHead, s_food);

  for (int i = s_snakeLength; i > 0; i--) {
    s_snake[i] = s_snake[i - 1];
  }
  s_snake[0] = nextHead;
  if (eating) {
    s_snakeLength += 1;
    if (s_snakeLength >= s_playableCellCount) {
      s_snakeWon = true;
      s_snakeHoldTicks = 0;
    } else {
      placeSnakeFood();
    }
  }

  s_needsRender = true;
}

void formatSnakeClockText(char* text, size_t textSize) {
  if (text == nullptr || textSize == 0) {
    return;
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    if (s_config.showSeconds) {
      snprintf(text, textSize, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
      snprintf(text, textSize, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    }
    snprintf(s_lastSnakeClockText, sizeof(s_lastSnakeClockText), "%s", text);
    s_lastSnakeClockTextValid = true;
    s_lastSnakeClockShowSeconds = s_config.showSeconds;
    return;
  }

  if (s_lastSnakeClockTextValid && s_lastSnakeClockShowSeconds == s_config.showSeconds) {
    snprintf(text, textSize, "%s", s_lastSnakeClockText);
    return;
  }

  if (s_config.showSeconds) {
    memcpy(text, "--:--:--", sizeof("--:--:--"));
    return;
  }
  memcpy(text, "--:--", sizeof("--:--"));
}

void drawSnakeClock(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr || !s_clockLayout.active) {
    return;
  }

  char timeText[9] = "";
  formatSnakeClockText(timeText, sizeof(timeText));
  drawClockText(
    display,
    timeText,
    s_clockLayout.textX,
    s_clockLayout.textY,
    rgb565(255, 255, 255),
    s_config.font,
    1
  );
}

void renderSnake(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr) {
    return;
  }

  const bool drawSpots = strcmp(s_config.snakeSkin, "spotted") == 0 &&
                         !s_snakeDead &&
                         !s_snakeWon;

  for (int i = 0; i < s_snakeLength; i++) {
    const SnakeRgb color = resolveSnakeSegmentColor(i);
    const int patternSeed = s_snake[i].x * 31 + s_snake[i].y * 17 + i * 13;
    drawSnakeCell(
      display,
      s_snakeOriginX + s_snake[i].x * s_snakePixelWidth,
      s_snakeOriginY + s_snake[i].y * s_snakePixelWidth,
      s_snakePixelWidth,
      color,
      i == 0,
      drawSpots,
      patternSeed
    );
  }

  if (!s_snakeWon && s_food.x >= 0 && s_food.y >= 0) {
    const SnakeRgb foodColor = {
      s_config.foodColorR,
      s_config.foodColorG,
      s_config.foodColorB
    };
    drawSnakeCell(
      display,
      s_snakeOriginX + s_food.x * s_snakePixelWidth,
      s_snakeOriginY + s_food.y * s_snakePixelWidth,
      s_snakePixelWidth,
      foodColor,
      true,
      false,
      0
    );
  }

  drawSnakeClock(display);
}
}

namespace SnakeEffect {
void init() {
  clearLastError();
  s_active = false;
  s_needsRender = false;
  s_lastTickAt = 0;
  s_snakeLength = 0;
  s_food = {-1, -1};
  s_snakeDead = false;
  s_snakeWon = false;
  s_snakeHoldTicks = 0;
  s_playableCellCount = 0;
  s_lastSnakeClockText[0] = '\0';
  s_lastSnakeClockTextValid = false;
  s_lastSnakeClockShowSeconds = false;
  resetSnakeClockLayout();
  releaseSnakeBuffers();
}

void deactivate() {
  clearLastError();
  s_active = false;
  s_needsRender = false;
  s_lastTickAt = 0;
  s_snakeLength = 0;
  s_food = {-1, -1};
  s_snakeDead = false;
  s_snakeWon = false;
  s_snakeHoldTicks = 0;
  s_playableCellCount = 0;
  s_lastSnakeClockText[0] = '\0';
  s_lastSnakeClockTextValid = false;
  s_lastSnakeClockShowSeconds = false;
  resetSnakeClockLayout();
  releaseSnakeBuffers();
}

void applyConfig(const SnakeModeConfig& config) {
  clearLastError();
  s_config = config;
  s_config.snakeSkin[sizeof(s_config.snakeSkin) - 1] = '\0';
  s_lastSnakeClockText[0] = '\0';
  s_lastSnakeClockTextValid = false;
  s_lastSnakeClockShowSeconds = false;

  if (s_config.snakeWidth < 2 || s_config.snakeWidth > 4) {
    setLastError("snake invalid width");
    s_active = false;
    return;
  }
  if (s_config.font > CLOCK_FONT_PIXEL_RAIL_5X8) {
    setLastError("snake invalid font");
    s_active = false;
    return;
  }
  if (!isSupportedSnakeSkin(s_config.snakeSkin)) {
    setLastError("snake invalid skin");
    s_active = false;
    return;
  }

  s_active = true;
  s_lastTickAt = 0;
  initSnake();
}

void update() {
  if (!s_active) {
    return;
  }

  unsigned long now = millis();
  unsigned long interval = resolveTickInterval(s_config.speed, 180, 55);
  if (s_lastTickAt != 0 && now - s_lastTickAt < interval) {
    return;
  }
  s_lastTickAt = now;
  updateSnake();
}

void render() {
  if (!s_active || !s_needsRender) {
    return;
  }

  uint16_t* frameBuffer = &DisplayManager::animationBuffer[0][0];
  MatrixPanel_I2S_DMA* offscreen = DisplayManager::beginOffscreenFrame(frameBuffer, 0);
  if (offscreen == nullptr) {
    return;
  }

  renderSnake(offscreen);
  DisplayManager::presentOffscreenFrame(frameBuffer);
  s_needsRender = false;
}

bool isActive() {
  return s_active;
}

const char* getLastError() {
  return s_lastError;
}

const SnakeModeConfig& getConfig() {
  return s_config;
}
}
