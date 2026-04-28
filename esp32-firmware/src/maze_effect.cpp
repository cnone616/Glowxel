#include "maze_effect.h"

#include <stdlib.h>
#include <string.h>

#include "clock_font_renderer.h"
#include "display_manager.h"

namespace {
constexpr int kPanelSize = 64;
constexpr int kMaxMazeCells = 63 * 63;
constexpr int kMazePanelPaddingX = 3;
constexpr int kMazePanelPaddingTop = 2;
constexpr int kMazePanelPaddingBottom = 2;
constexpr int kMazePanelSectionGapY = 1;

struct MazeRgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct MazePoint {
  int16_t x;
  int16_t y;
};

struct MazeInfoPanelState {
  bool active;
  int panelX;
  int panelY;
  int panelWidth;
  int panelHeight;
  int timeTextX;
  int timeTextY;
  int dateTextX;
  int dateTextY;
  int timeScale;
  int dateScale;
  int reservedLeft;
  int reservedTop;
  int reservedRight;
  int reservedBottom;
};

const int8_t kDirections[4][2] = {
  {0, -1},
  {1, 0},
  {0, 1},
  {-1, 0}
};

MazeModeConfig s_config = {
  3,
  MAZE_SIZE_WIDE,
  true,
  "#05070f",
  "#182c4c",
  "#ffd400",
  "#ff6464",
  "#4f4f55",
  "#ff4444",
  "#70ff9c",
  "#1a60ff",
  "#42bcff"
};

bool s_active = false;
bool s_needsRender = true;
unsigned long s_lastTickAt = 0;
char s_lastError[128] = "";

uint8_t* s_mazeGrid = nullptr;
uint16_t* s_mazeReveal = nullptr;
int s_mazeRevealCount = 0;
uint16_t* s_mazePath = nullptr;
int s_mazePathLength = 0;
int s_mazeDim = 31;
int s_mazeScale = 2;
int s_mazeOrigin = 1;
int s_mazeRenderStart[63] = {};
int s_mazeRenderSize[63] = {};
int s_mazeRenderTotal = 0;
int s_mazePhase = 0;
int s_mazeRevealIndex = 0;
int s_mazeSolveIndex = 0;
int s_mazeTravelIndex = 0;
int s_mazeHoldTicks = 0;
int s_mazeCapacity = 0;
int16_t* s_mazeParent = nullptr;
uint8_t* s_mazeVisited = nullptr;
uint16_t* s_mazeStack = nullptr;
uint16_t* s_mazeQueue = nullptr;
int s_mazeStackSize = 0;
int s_mazeQueueHead = 0;
int s_mazeQueueTail = 0;
int s_mazeClosedCount = 0;
uint32_t s_mazeSeed = 0;
int s_lastRenderDebugPhase = -1;
MazeInfoPanelState s_mazeInfoPanel = {};

const char* mazePhaseLabel(int phase) {
  switch (phase) {
    case 0:
      return "generation";
    case 1:
      return "generation_hold";
    case 2:
      return "solve";
    case 3:
      return "solve_hold";
    case 4:
      return "reveal";
    default:
      break;
  }
  switch (phase) {
    case 5:
      return "reveal_hold";
    case 6:
      return "travel";
    default:
      return "done_hold";
  }
}

void setMazePhase(int nextPhase) {
  if (s_mazePhase == nextPhase) {
    return;
  }
  Serial.printf(
    "[MAZE] phase %s -> %s\n",
    mazePhaseLabel(s_mazePhase),
    mazePhaseLabel(nextPhase)
  );
  s_mazePhase = nextPhase;
  s_lastRenderDebugPhase = -1;
}

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

bool parseHexColorText(const char* value, MazeRgbColor& color) {
  if (value == nullptr || strlen(value) != 7 || value[0] != '#') {
    return false;
  }

  char* end = nullptr;
  long red = strtol(value + 1, &end, 16);
  if (end != value + 7) {
    return false;
  }

  color.r = static_cast<uint8_t>((red >> 16) & 0xFF);
  color.g = static_cast<uint8_t>((red >> 8) & 0xFF);
  color.b = static_cast<uint8_t>(red & 0xFF);
  return true;
}

bool isValidMazeConfig(const MazeModeConfig& config) {
  MazeRgbColor color = {};
  return config.speed >= 1 &&
         config.speed <= 10 &&
         (config.mazeSizeMode == MAZE_SIZE_WIDE ||
          config.mazeSizeMode == MAZE_SIZE_DENSE) &&
         parseHexColorText(config.panelBgColor, color) &&
         parseHexColorText(config.borderColor, color) &&
         parseHexColorText(config.timeColor, color) &&
         parseHexColorText(config.dateColor, color) &&
         parseHexColorText(config.generationPathColor, color) &&
         parseHexColorText(config.searchVisitedColor, color) &&
         parseHexColorText(config.searchFrontierColor, color) &&
         parseHexColorText(config.solvedPathStartColor, color) &&
         parseHexColorText(config.solvedPathEndColor, color);
}

MazeRgbColor interpolateColor(
  const MazeRgbColor& startColor,
  const MazeRgbColor& endColor,
  int index,
  int total
) {
  if (total <= 1) {
    return startColor;
  }

  MazeRgbColor color = {};
  color.r = static_cast<uint8_t>(
    startColor.r +
    ((static_cast<int>(endColor.r) - static_cast<int>(startColor.r)) * index) /
      (total - 1)
  );
  color.g = static_cast<uint8_t>(
    startColor.g +
    ((static_cast<int>(endColor.g) - static_cast<int>(startColor.g)) * index) /
      (total - 1)
  );
  color.b = static_cast<uint8_t>(
    startColor.b +
    ((static_cast<int>(endColor.b) - static_cast<int>(startColor.b)) * index) /
      (total - 1)
  );
  return color;
}

void drawFilledRectPixels(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  int width,
  int height,
  uint16_t color
) {
  if (display == nullptr || width <= 0 || height <= 0) {
    return;
  }

  for (int py = 0; py < height; py++) {
    for (int px = 0; px < width; px++) {
      display->drawPixel(x + px, y + py, color);
    }
  }
}

void drawRectOutlinePixels(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  int width,
  int height,
  uint16_t color
) {
  if (display == nullptr || width <= 0 || height <= 0) {
    return;
  }

  for (int px = 0; px < width; px++) {
    display->drawPixel(x + px, y, color);
    display->drawPixel(x + px, y + height - 1, color);
  }
  for (int py = 1; py < height - 1; py++) {
    display->drawPixel(x, y + py, color);
    display->drawPixel(x + width - 1, y + py, color);
  }
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

void configureMazeRenderMetrics() {
  s_mazeRenderTotal = 0;
  for (int i = 0; i < s_mazeDim; i++) {
    s_mazeRenderStart[i] = s_mazeRenderTotal;
    if (s_config.mazeSizeMode == MAZE_SIZE_DENSE) {
      s_mazeRenderSize[i] = 1;
    } else {
      s_mazeRenderSize[i] = (i % 2 == 0) ? 1 : 2;
    }
    s_mazeRenderTotal += s_mazeRenderSize[i];
  }
}

int mazeRenderCoordStart(int coord) {
  if (coord < 0 || coord >= s_mazeDim) {
    return 0;
  }
  return s_mazeOrigin + s_mazeRenderStart[coord];
}

int mazeRenderCoordSize(int coord) {
  if (coord < 0 || coord >= s_mazeDim) {
    return 0;
  }
  return s_mazeRenderSize[coord];
}

bool screenRangeIntersectsCoord(int start, int end, int coord) {
  int coordStart = mazeRenderCoordStart(coord);
  int coordEnd = coordStart + mazeRenderCoordSize(coord) - 1;
  return !(coordEnd < start || coordStart > end);
}

int findReservedCoordStart(int screenStart, int screenEnd) {
  for (int coord = 0; coord < s_mazeDim; coord++) {
    if (screenRangeIntersectsCoord(screenStart, screenEnd, coord)) {
      return coord;
    }
  }
  return 0;
}

int findReservedCoordEnd(int screenStart, int screenEnd) {
  for (int coord = s_mazeDim - 1; coord >= 0; coord--) {
    if (screenRangeIntersectsCoord(screenStart, screenEnd, coord)) {
      return coord;
    }
  }
  return s_mazeDim - 1;
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

int resolveGenerationStepsPerTick() {
  int safeSpeed = clampInt((int)s_config.speed, 1, 6);
  if (s_config.mazeSizeMode == MAZE_SIZE_DENSE) {
    static const int denseValues[6] = {2, 2, 2, 2, 3, 3};
    return denseValues[safeSpeed - 1];
  }
  static const int wideValues[6] = {1, 1, 1, 2, 2, 2};
  return wideValues[safeSpeed - 1];
}

int resolveSolveStepsPerTick() {
  int safeSpeed = clampInt((int)s_config.speed, 1, 6);
  if (s_config.mazeSizeMode == MAZE_SIZE_DENSE) {
    static const int denseValues[6] = {1, 1, 1, 2, 2, 2};
    return denseValues[safeSpeed - 1];
  }
  static const int wideValues[6] = {1, 1, 1, 1, 2, 2};
  return wideValues[safeSpeed - 1];
}

int resolveRevealStepsPerTick() {
  int safeSpeed = clampInt((int)s_config.speed, 1, 6);
  if (s_config.mazeSizeMode == MAZE_SIZE_DENSE) {
    static const int denseValues[6] = {1, 1, 1, 1, 1, 2};
    return denseValues[safeSpeed - 1];
  }
  static const int wideValues[6] = {1, 1, 1, 1, 1, 1};
  return wideValues[safeSpeed - 1];
}

int resolveTravelStepsPerTick() {
  int safeSpeed = clampInt((int)s_config.speed, 1, 6);
  if (s_config.mazeSizeMode == MAZE_SIZE_DENSE) {
    static const int denseValues[6] = {1, 1, 1, 1, 2, 2};
    return denseValues[safeSpeed - 1];
  }
  static const int wideValues[6] = {1, 1, 1, 1, 1, 2};
  return wideValues[safeSpeed - 1];
}

uint32_t pseudoRandom(uint32_t seed) {
  return seed * 1664525UL + 1013904223UL;
}

uint32_t nextMazeRandom() {
  s_mazeSeed = pseudoRandom(s_mazeSeed);
  return s_mazeSeed;
}

void releaseMazeBuffers() {
  if (s_mazeGrid != nullptr) {
    free(s_mazeGrid);
    s_mazeGrid = nullptr;
  }
  if (s_mazeReveal != nullptr) {
    free(s_mazeReveal);
    s_mazeReveal = nullptr;
  }
  if (s_mazePath != nullptr) {
    free(s_mazePath);
    s_mazePath = nullptr;
  }
  if (s_mazeParent != nullptr) {
    free(s_mazeParent);
    s_mazeParent = nullptr;
  }
  if (s_mazeVisited != nullptr) {
    free(s_mazeVisited);
    s_mazeVisited = nullptr;
  }
  if (s_mazeStack != nullptr) {
    free(s_mazeStack);
    s_mazeStack = nullptr;
  }
  if (s_mazeQueue != nullptr) {
    free(s_mazeQueue);
    s_mazeQueue = nullptr;
  }
  s_mazeCapacity = 0;
}

bool ensureMazeBuffers(int totalCells) {
  if (totalCells <= 0 || totalCells > kMaxMazeCells) {
    setLastError("maze invalid grid cells");
    return false;
  }
  if (s_mazeCapacity == totalCells &&
      s_mazeGrid != nullptr &&
      s_mazeReveal != nullptr &&
      s_mazePath != nullptr &&
      s_mazeParent != nullptr &&
      s_mazeVisited != nullptr &&
      s_mazeStack != nullptr &&
      s_mazeQueue != nullptr) {
    clearLastError();
    return true;
  }

  releaseMazeBuffers();
  s_mazeGrid = static_cast<uint8_t*>(allocateEffectBuffer(totalCells));
  s_mazeReveal = static_cast<uint16_t*>(allocateEffectBuffer(sizeof(uint16_t) * totalCells));
  s_mazePath = static_cast<uint16_t*>(allocateEffectBuffer(sizeof(uint16_t) * totalCells));
  s_mazeParent = static_cast<int16_t*>(allocateEffectBuffer(sizeof(int16_t) * totalCells));
  s_mazeVisited = static_cast<uint8_t*>(allocateEffectBuffer(totalCells));
  s_mazeStack = static_cast<uint16_t*>(allocateEffectBuffer(sizeof(uint16_t) * totalCells));
  s_mazeQueue = static_cast<uint16_t*>(allocateEffectBuffer(sizeof(uint16_t) * totalCells));
  if (s_mazeGrid == nullptr ||
      s_mazeReveal == nullptr ||
      s_mazePath == nullptr ||
      s_mazeParent == nullptr ||
      s_mazeVisited == nullptr ||
      s_mazeStack == nullptr ||
      s_mazeQueue == nullptr) {
    snprintf(
      s_lastError,
      sizeof(s_lastError),
      "maze buffer allocation failed: cells=%d freeHeap=%u freePsram=%u",
      totalCells,
      ESP.getFreeHeap(),
      ESP.getFreePsram()
    );
    Serial.printf(
      "[MAZE] 缓冲分配失败, cells=%d, freeHeap=%u, freePsram=%u\n",
      totalCells,
      ESP.getFreeHeap(),
      ESP.getFreePsram()
    );
    releaseMazeBuffers();
    return false;
  }
  s_mazeCapacity = totalCells;
  clearLastError();
  return true;
}

void drawMazePixel(MatrixPanel_I2S_DMA* display, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (display == nullptr) {
    return;
  }
  int drawX = mazeRenderCoordStart(x);
  int drawY = mazeRenderCoordStart(y);
  int drawW = mazeRenderCoordSize(x);
  int drawH = mazeRenderCoordSize(y);
  drawFilledRectPixels(
    display,
    drawX,
    drawY,
    drawW,
    drawH,
    rgb565(r, g, b)
  );
}

void resetMazeInfoPanel() {
  s_mazeInfoPanel = {};
}

void configureMazeInfoPanel() {
  resetMazeInfoPanel();
  if (!s_config.showClock) {
    Serial.println("[MAZE] info panel disabled because showClock=false");
    return;
  }

  const char* timeSample = "88:88";
  const char* dateSample = "88-88";
  const int timeScale = 2;
  const int dateScale = 1;
  const int timeWidth = getClockTextWidth(timeSample, CLOCK_FONT_MINIMAL_3X5, timeScale);
  const int timeHeight = getClockTextHeight(CLOCK_FONT_MINIMAL_3X5, timeScale);
  const int dateWidth = getClockTextWidth(dateSample, CLOCK_FONT_MINIMAL_3X5, dateScale);
  const int dateHeight = getClockTextHeight(CLOCK_FONT_MINIMAL_3X5, dateScale);
  const int contentWidth = max(timeWidth, dateWidth);
  const int panelWidth = contentWidth + kMazePanelPaddingX * 2;
  const int panelHeight =
    kMazePanelPaddingTop +
    timeHeight +
    kMazePanelSectionGapY +
    dateHeight +
    kMazePanelPaddingBottom;
  const int panelX = (kPanelSize - panelWidth) / 2;
  const int panelY = (kPanelSize - panelHeight) / 2;
  const int reservedScreenLeft = panelX;
  const int reservedScreenTop = panelY;
  const int reservedScreenRight = panelX + panelWidth - 1;
  const int reservedScreenBottom = panelY + panelHeight - 1;

  s_mazeInfoPanel.active = true;
  s_mazeInfoPanel.panelX = panelX;
  s_mazeInfoPanel.panelY = panelY;
  s_mazeInfoPanel.panelWidth = panelWidth;
  s_mazeInfoPanel.panelHeight = panelHeight;
  s_mazeInfoPanel.timeTextX = panelX + (panelWidth - timeWidth) / 2;
  s_mazeInfoPanel.timeTextY = panelY + kMazePanelPaddingTop;
  s_mazeInfoPanel.dateTextX = panelX + (panelWidth - dateWidth) / 2;
  s_mazeInfoPanel.dateTextY = panelY + kMazePanelPaddingTop + timeHeight + kMazePanelSectionGapY;
  s_mazeInfoPanel.timeScale = timeScale;
  s_mazeInfoPanel.dateScale = dateScale;
  s_mazeInfoPanel.reservedLeft = clampInt(
    findReservedCoordStart(reservedScreenLeft, reservedScreenRight),
    0,
    s_mazeDim - 1
  );
  s_mazeInfoPanel.reservedTop = clampInt(
    findReservedCoordStart(reservedScreenTop, reservedScreenBottom),
    0,
    s_mazeDim - 1
  );
  s_mazeInfoPanel.reservedRight = clampInt(
    findReservedCoordEnd(reservedScreenLeft, reservedScreenRight),
    0,
    s_mazeDim - 1
  );
  s_mazeInfoPanel.reservedBottom = clampInt(
    findReservedCoordEnd(reservedScreenTop, reservedScreenBottom),
    0,
    s_mazeDim - 1
  );

  Serial.printf(
    "[MAZE] info panel active panel=(%d,%d %dx%d) reserved=(%d,%d)-(%d,%d)\n",
    s_mazeInfoPanel.panelX,
    s_mazeInfoPanel.panelY,
    s_mazeInfoPanel.panelWidth,
    s_mazeInfoPanel.panelHeight,
    s_mazeInfoPanel.reservedLeft,
    s_mazeInfoPanel.reservedTop,
    s_mazeInfoPanel.reservedRight,
    s_mazeInfoPanel.reservedBottom
  );
}

bool isMazeReservedCell(int x, int y) {
  if (!s_mazeInfoPanel.active) {
    return false;
  }
  return x >= s_mazeInfoPanel.reservedLeft &&
         x <= s_mazeInfoPanel.reservedRight &&
         y >= s_mazeInfoPanel.reservedTop &&
         y <= s_mazeInfoPanel.reservedBottom;
}

void drawMazeInfoPanel(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr || !s_mazeInfoPanel.active) {
    return;
  }

  struct tm timeinfo;
  char timeText[6] = "00:00";
  char dateText[6] = "00-00";
  if (getLocalTime(&timeinfo, 0)) {
    snprintf(timeText, sizeof(timeText), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    snprintf(dateText, sizeof(dateText), "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
  } else {
    memcpy(timeText, "--:--", sizeof(timeText));
    memcpy(dateText, "--.--", sizeof(dateText));
  }

  MazeRgbColor panelBgColor = {};
  MazeRgbColor borderColor = {};
  MazeRgbColor timeColor = {};
  MazeRgbColor dateColor = {};
  if (!parseHexColorText(s_config.panelBgColor, panelBgColor) ||
      !parseHexColorText(s_config.borderColor, borderColor) ||
      !parseHexColorText(s_config.timeColor, timeColor) ||
      !parseHexColorText(s_config.dateColor, dateColor)) {
    return;
  }

  drawFilledRectPixels(
    display,
    s_mazeInfoPanel.panelX,
    s_mazeInfoPanel.panelY,
    s_mazeInfoPanel.panelWidth,
    s_mazeInfoPanel.panelHeight,
    rgb565(panelBgColor.r, panelBgColor.g, panelBgColor.b)
  );
  drawRectOutlinePixels(
    display,
    s_mazeInfoPanel.panelX,
    s_mazeInfoPanel.panelY,
    s_mazeInfoPanel.panelWidth,
    s_mazeInfoPanel.panelHeight,
    rgb565(borderColor.r, borderColor.g, borderColor.b)
  );
  drawClockText(
    display,
    timeText,
    s_mazeInfoPanel.timeTextX,
    s_mazeInfoPanel.timeTextY,
    rgb565(timeColor.r, timeColor.g, timeColor.b),
    CLOCK_FONT_MINIMAL_3X5,
    s_mazeInfoPanel.timeScale
  );
  drawClockText(
    display,
    dateText,
    s_mazeInfoPanel.dateTextX,
    s_mazeInfoPanel.dateTextY,
    rgb565(dateColor.r, dateColor.g, dateColor.b),
    CLOCK_FONT_MINIMAL_3X5,
    s_mazeInfoPanel.dateScale
  );
}

void carveMazeCell(int x, int y) {
  if (x < 0 || x >= s_mazeDim || y < 0 || y >= s_mazeDim) {
    return;
  }
  if (isMazeReservedCell(x, y)) {
    return;
  }
  int index = y * s_mazeDim + x;
  if (s_mazeGrid[index] == 1) {
    return;
  }
  s_mazeGrid[index] = 1;
  s_mazeReveal[s_mazeRevealCount++] = (uint16_t)index;
}

int mazeStartIndex() {
  return 1 * s_mazeDim + 1;
}

int mazeEndIndex() {
  return (s_mazeDim - 2) * s_mazeDim + (s_mazeDim - 2);
}

void resetSolveState() {
  int totalCells = s_mazeDim * s_mazeDim;
  memset(s_mazeVisited, 0, totalCells);
  for (int i = 0; i < totalCells; i++) {
    s_mazeParent[i] = -1;
  }
  s_mazeQueueHead = 0;
  s_mazeQueueTail = 0;
  s_mazeClosedCount = 0;
  s_mazePathLength = 0;
  s_mazeSolveIndex = 0;
}

void beginSolvePhase() {
  resetSolveState();
  int startIndex = mazeStartIndex();
  s_mazeQueue[s_mazeQueueTail++] = (uint16_t)startIndex;
  s_mazeVisited[startIndex] = 1;
  s_mazeParent[startIndex] = -1;
}

void buildSolvedPath(int endIndex) {
  s_mazePathLength = 0;
  int cursor = endIndex;
  while (cursor >= 0) {
    s_mazePath[s_mazePathLength++] = (uint16_t)cursor;
    cursor = s_mazeParent[cursor];
  }
  for (int i = 0; i < s_mazePathLength / 2; i++) {
    uint16_t temp = s_mazePath[i];
    s_mazePath[i] = s_mazePath[s_mazePathLength - 1 - i];
    s_mazePath[s_mazePathLength - 1 - i] = temp;
  }
}

bool stepGeneration(int steps) {
  for (int step = 0; step < steps; step++) {
    if (s_mazeStackSize <= 0) {
      return true;
    }

    uint16_t currentIndex = s_mazeStack[s_mazeStackSize - 1];
    int currentX = currentIndex % s_mazeDim;
    int currentY = currentIndex / s_mazeDim;
    int dirs[4] = {0, 1, 2, 3};
    for (int i = 3; i > 0; i--) {
      int swapIndex = nextMazeRandom() % (uint32_t)(i + 1);
      int temp = dirs[i];
      dirs[i] = dirs[swapIndex];
      dirs[swapIndex] = temp;
    }

    bool advanced = false;
    for (int i = 0; i < 4; i++) {
      int nx = currentX + kDirections[dirs[i]][0] * 2;
      int ny = currentY + kDirections[dirs[i]][1] * 2;
      if (nx <= 0 || ny <= 0 || nx >= s_mazeDim - 1 || ny >= s_mazeDim - 1) {
        continue;
      }
      int wallX = currentX + kDirections[dirs[i]][0];
      int wallY = currentY + kDirections[dirs[i]][1];
      if (isMazeReservedCell(nx, ny) || isMazeReservedCell(wallX, wallY)) {
        continue;
      }
      int nextIndex = ny * s_mazeDim + nx;
      if (s_mazeVisited[nextIndex]) {
        continue;
      }
      carveMazeCell(wallX, wallY);
      carveMazeCell(nx, ny);
      s_mazeVisited[nextIndex] = 1;
      s_mazeStack[s_mazeStackSize++] = (uint16_t)nextIndex;
      advanced = true;
      break;
    }

    if (!advanced) {
      s_mazeStackSize -= 1;
    }
  }

  return s_mazeStackSize <= 0;
}

bool stepSolve(int steps) {
  int endIndex = mazeEndIndex();
  for (int step = 0; step < steps; step++) {
    if (s_mazeQueueHead >= s_mazeQueueTail) {
      return false;
    }

    int current = (int)s_mazeQueue[s_mazeQueueHead++];
    if (s_mazeVisited[current] == 2) {
      continue;
    }
    s_mazeVisited[current] = 2;
    s_mazePath[s_mazeClosedCount++] = (uint16_t)current;
    if (current == endIndex) {
      buildSolvedPath(endIndex);
      return true;
    }

    int currentX = current % s_mazeDim;
    int currentY = current / s_mazeDim;
    for (int dir = 0; dir < 4; dir++) {
      int nx = currentX + kDirections[dir][0];
      int ny = currentY + kDirections[dir][1];
      if (nx < 0 || ny < 0 || nx >= s_mazeDim || ny >= s_mazeDim) {
        continue;
      }
      int nextIndex = ny * s_mazeDim + nx;
      if (s_mazeGrid[nextIndex] == 0 || s_mazeVisited[nextIndex] != 0) {
        continue;
      }
      s_mazeVisited[nextIndex] = 1;
      s_mazeParent[nextIndex] = (int16_t)current;
      s_mazeQueue[s_mazeQueueTail++] = (uint16_t)nextIndex;
    }
  }

  return false;
}

void initMaze() {
  s_mazeDim = s_config.mazeSizeMode == MAZE_SIZE_DENSE ? 63 : 43;
  s_mazeScale = 1;
  s_mazeOrigin = 0;
  configureMazeRenderMetrics();
  configureMazeInfoPanel();
  int totalCells = s_mazeDim * s_mazeDim;
  if (!ensureMazeBuffers(totalCells)) {
    s_active = false;
    return;
  }

  memset(s_mazeGrid, 0, totalCells);
  memset(s_mazeVisited, 0, totalCells);
  for (int i = 0; i < totalCells; i++) {
    s_mazeParent[i] = -1;
  }
  s_mazeRevealCount = 0;
  s_mazePathLength = 0;
  s_mazePhase = 0;
  s_mazeRevealIndex = 0;
  s_mazeSolveIndex = 0;
  s_mazeTravelIndex = 0;
  s_mazeHoldTicks = 0;
  s_mazeStackSize = 0;
  s_mazeQueueHead = 0;
  s_mazeQueueTail = 0;
  s_mazeClosedCount = 0;
  s_mazeSeed = (uint32_t)(millis() ^ (s_config.speed * 131U) ^ (s_config.mazeSizeMode * 977U));
  s_lastRenderDebugPhase = -1;

  Serial.printf(
    "[MAZE] init dim=%d scale=%d origin=%d renderTotal=%d speed=%u sizeMode=%s showClock=%s\n",
    s_mazeDim,
    s_mazeScale,
    s_mazeOrigin,
    s_mazeRenderTotal,
    s_config.speed,
    s_config.mazeSizeMode == MAZE_SIZE_DENSE ? "dense" : "wide",
    s_config.showClock ? "true" : "false"
  );

  int startIndex = mazeStartIndex();
  s_mazeVisited[startIndex] = 1;
  s_mazeStack[s_mazeStackSize++] = (uint16_t)startIndex;
  carveMazeCell(1, 1);
  s_mazeRevealIndex = s_mazeRevealCount;

  Serial.printf(
    "[MAZE] init ready revealCount=%d stackSize=%d start=%d end=%d active=%s\n",
    s_mazeRevealCount,
    s_mazeStackSize,
    mazeStartIndex(),
    mazeEndIndex(),
    s_active ? "true" : "false"
  );
  s_needsRender = true;
}

void updateMaze() {
  if (s_mazePhase == 0) {
    int step = resolveGenerationStepsPerTick();
    bool completed = stepGeneration(step);
    s_mazeRevealIndex = s_mazeRevealCount;
    if (completed) {
      setMazePhase(1);
      s_mazeHoldTicks = 0;
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 1) {
    s_mazeHoldTicks += 1;
    if (s_mazeHoldTicks >= 8) {
      beginSolvePhase();
      setMazePhase(2);
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 2) {
    int step = resolveSolveStepsPerTick();
    bool found = stepSolve(step);
    if (found) {
      setMazePhase(3);
      s_mazeHoldTicks = 0;
    } else if (s_mazeQueueHead >= s_mazeQueueTail) {
      setMazePhase(7);
      s_mazeHoldTicks = 0;
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 3) {
    s_mazeHoldTicks += 1;
    if (s_mazeHoldTicks >= 8) {
      s_mazeSolveIndex = 0;
      setMazePhase(4);
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 4) {
    int step = resolveRevealStepsPerTick();
    s_mazeSolveIndex += step;
    if (s_mazeSolveIndex >= s_mazePathLength) {
      s_mazeSolveIndex = s_mazePathLength;
      setMazePhase(5);
      s_mazeHoldTicks = 0;
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 5) {
    s_mazeHoldTicks += 1;
    if (s_mazeHoldTicks >= 8) {
      s_mazeTravelIndex = 0;
      setMazePhase(6);
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 6) {
    s_mazeTravelIndex += resolveTravelStepsPerTick();
    if (s_mazeTravelIndex >= s_mazePathLength) {
      s_mazeTravelIndex = s_mazePathLength > 0 ? s_mazePathLength - 1 : 0;
      setMazePhase(7);
      s_mazeHoldTicks = 0;
    }
    s_needsRender = true;
    return;
  }

  s_mazeHoldTicks += 1;
  if (s_mazeHoldTicks >= 12) {
    Serial.println("[MAZE] reset after done hold");
    initMaze();
  }
  s_needsRender = true;
}

void renderMaze(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr) {
    return;
  }

  if (s_lastRenderDebugPhase != s_mazePhase) {
    s_lastRenderDebugPhase = s_mazePhase;
  }

  uint16_t wallColor = rgb565(12, 12, 14);
  display->fillScreen(wallColor);

  MazeRgbColor generationPathColor = {};
  MazeRgbColor searchVisitedColor = {};
  MazeRgbColor searchFrontierColor = {};
  MazeRgbColor solvedPathStartColor = {};
  MazeRgbColor solvedPathEndColor = {};
  if (!parseHexColorText(s_config.generationPathColor, generationPathColor) ||
      !parseHexColorText(s_config.searchVisitedColor, searchVisitedColor) ||
      !parseHexColorText(s_config.searchFrontierColor, searchFrontierColor) ||
      !parseHexColorText(s_config.solvedPathStartColor, solvedPathStartColor) ||
      !parseHexColorText(s_config.solvedPathEndColor, solvedPathEndColor)) {
    return;
  }

  for (int i = 0; i < s_mazeRevealIndex; i++) {
    int index = s_mazeReveal[i];
    drawMazePixel(
      display,
      index % s_mazeDim,
      index / s_mazeDim,
      generationPathColor.r,
      generationPathColor.g,
      generationPathColor.b
    );
  }

  if ((s_mazePhase == 0 || s_mazePhase == 1) && s_mazeStackSize > 0) {
    int generationCursor = s_mazeStack[s_mazeStackSize - 1];
    drawMazePixel(
      display,
      generationCursor % s_mazeDim,
      generationCursor / s_mazeDim,
      76,
      220,
      72
    );
  }

  if (s_mazePhase == 2 || s_mazePhase == 3) {
    for (int index = 0; index < s_mazeDim * s_mazeDim; index++) {
      if (s_mazeGrid[index] == 0) {
        continue;
      }
      if (s_mazeVisited[index] == 2) {
        drawMazePixel(
          display,
          index % s_mazeDim,
          index / s_mazeDim,
          searchVisitedColor.r,
          searchVisitedColor.g,
          searchVisitedColor.b
        );
      } else if (s_mazeVisited[index] == 1) {
        drawMazePixel(
          display,
          index % s_mazeDim,
          index / s_mazeDim,
          searchFrontierColor.r,
          searchFrontierColor.g,
          searchFrontierColor.b
        );
      }
    }

    if (s_mazeClosedCount > 0) {
      int solveCursor = s_mazePath[s_mazeClosedCount - 1];
      drawMazePixel(
        display,
        solveCursor % s_mazeDim,
        solveCursor / s_mazeDim,
        255,
        196,
        92
      );
    }
  }

  if (s_mazePathLength > 0 && s_mazePhase >= 4) {
    int visibleCount = s_mazePhase == 4 ? s_mazeSolveIndex : s_mazePathLength;
    for (int i = 0; i < visibleCount; i++) {
      int index = s_mazePath[i];
      MazeRgbColor pathColor = interpolateColor(
        solvedPathStartColor,
        solvedPathEndColor,
        i,
        s_mazePathLength
      );
      drawMazePixel(
        display,
        index % s_mazeDim,
        index / s_mazeDim,
        pathColor.r,
        pathColor.g,
        pathColor.b
      );
    }

    if (s_mazePhase == 4 && s_mazeSolveIndex > 0) {
      int currentIndex = s_mazePath[min(s_mazeSolveIndex - 1, s_mazePathLength - 1)];
      drawMazePixel(
        display,
        currentIndex % s_mazeDim,
        currentIndex / s_mazeDim,
        255,
        244,
        172
      );
    }
  }

  if (s_mazePathLength > 0) {
    int startIndex = s_mazePath[0];
    int endIndex = s_mazePath[s_mazePathLength - 1];
    drawMazePixel(display, startIndex % s_mazeDim, startIndex / s_mazeDim, 40, 255, 120);
    drawMazePixel(display, endIndex % s_mazeDim, endIndex / s_mazeDim, 90, 188, 255);
  } else {
    int startIndex = mazeStartIndex();
    int endIndex = mazeEndIndex();
    drawMazePixel(display, startIndex % s_mazeDim, startIndex / s_mazeDim, 40, 255, 120);
    drawMazePixel(display, endIndex % s_mazeDim, endIndex / s_mazeDim, 90, 188, 255);
  }

  if (s_mazePhase >= 6 && s_mazePathLength > 0) {
    int traveler = s_mazePath[s_mazeTravelIndex];
    drawMazePixel(display, traveler % s_mazeDim, traveler / s_mazeDim, 255, 244, 172);
  }

  drawMazeInfoPanel(display);
}
}

namespace MazeEffect {
void init() {
  clearLastError();
  s_active = false;
  s_needsRender = false;
  s_lastTickAt = 0;
  s_mazeRevealCount = 0;
  s_mazePathLength = 0;
  s_mazePhase = 0;
  s_mazeRevealIndex = 0;
  s_mazeSolveIndex = 0;
  s_mazeTravelIndex = 0;
  s_mazeHoldTicks = 0;
  s_mazeStackSize = 0;
  s_mazeQueueHead = 0;
  s_mazeQueueTail = 0;
  s_mazeClosedCount = 0;
  s_mazeSeed = 0;
  s_lastRenderDebugPhase = -1;
  resetMazeInfoPanel();
  releaseMazeBuffers();
}

void deactivate() {
  clearLastError();
  s_active = false;
  s_needsRender = false;
  s_lastTickAt = 0;
  s_mazeRevealCount = 0;
  s_mazePathLength = 0;
  s_mazePhase = 0;
  s_mazeRevealIndex = 0;
  s_mazeSolveIndex = 0;
  s_mazeTravelIndex = 0;
  s_mazeHoldTicks = 0;
  s_mazeStackSize = 0;
  s_mazeQueueHead = 0;
  s_mazeQueueTail = 0;
  s_mazeClosedCount = 0;
  s_mazeSeed = 0;
  s_lastRenderDebugPhase = -1;
  resetMazeInfoPanel();
  releaseMazeBuffers();
}

void applyConfig(const MazeModeConfig& config) {
  clearLastError();
  if (!isValidMazeConfig(config)) {
    s_active = false;
    s_needsRender = false;
    setLastError("invalid maze config");
    return;
  }
  s_config = config;
  s_active = true;
  s_lastTickAt = 0;
  Serial.printf(
    "[MAZE] applyConfig speed=%u sizeMode=%s showClock=%s\n",
    s_config.speed,
    s_config.mazeSizeMode == MAZE_SIZE_DENSE ? "dense" : "wide",
    s_config.showClock ? "true" : "false"
  );
  initMaze();
  if (!s_active) {
    Serial.printf("[MAZE] activation failed: %s\n", getLastError());
  }
}

void update() {
  if (!s_active) {
    return;
  }

  unsigned long now = millis();
  unsigned long interval = resolveTickInterval(s_config.speed, 130, 38);
  if (s_lastTickAt != 0 && now - s_lastTickAt < interval) {
    return;
  }
  s_lastTickAt = now;
  updateMaze();
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

  renderMaze(offscreen);
  DisplayManager::presentOffscreenFrame(frameBuffer);
  s_needsRender = false;
}

bool isActive() {
  return s_active;
}

const char* getLastError() {
  return s_lastError;
}

const MazeModeConfig& getConfig() {
  return s_config;
}
}
