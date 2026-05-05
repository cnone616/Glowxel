#include "maze_effect.h"

#include <stdlib.h>
#include <string.h>

#include "clock_font_renderer.h"
#include "display_manager.h"

namespace {
constexpr int kPanelSize = 64;
constexpr int kMaxMazeCells = 63 * 63;
constexpr int kMazeDirtyCellCapacity = 64;
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

struct MazeRenderPassResult {
  bool usedFullRefresh;
  bool panelChanged;
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
int s_mazeRevealCount = 0;
uint16_t* s_mazeTraversal = nullptr;
int s_mazePathLength = 0;
int s_mazeDim = 31;
int s_mazeScale = 2;
int s_mazeOrigin = 1;
int s_mazeRenderStart[63] = {};
int s_mazeRenderSize[63] = {};
int s_mazeRenderTotal = 0;
int s_mazePhase = 0;
int s_mazeSolveIndex = 0;
int s_mazeTravelIndex = 0;
int s_mazeHoldTicks = 0;
int s_mazeCapacity = 0;
int16_t* s_mazeParent = nullptr;
uint8_t* s_mazeVisited = nullptr;
int s_mazeStackSize = 0;
int s_mazeQueueHead = 0;
int s_mazeQueueTail = 0;
int s_mazeSolveCursorIndex = -1;
uint32_t s_mazeSeed = 0;
int s_lastRenderDebugPhase = -1;
MazeInfoPanelState s_mazeInfoPanel = {};
bool s_mazeFrameInitialized = false;
bool s_mazeInfoPanelDrawn = false;
char s_lastInfoPanelTimeText[6] = "";
char s_lastInfoPanelDateText[6] = "";
uint16_t s_generationPathColor565 = 0;
uint16_t s_searchVisitedColor565 = 0;
uint16_t s_searchFrontierColor565 = 0;
MazeRgbColor s_solvedPathStartColor = {};
MazeRgbColor s_solvedPathEndColor = {};
uint16_t s_panelBgColor565 = 0;
uint16_t s_borderColor565 = 0;
uint16_t s_timeColor565 = 0;
uint16_t s_dateColor565 = 0;
bool s_mazeDirtyAllCells = true;
uint16_t s_mazeDirtyCells[kMazeDirtyCellCapacity] = {};
int s_mazeDirtyCellCount = 0;

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

void resetMazeFrameState() {
  s_mazeFrameInitialized = false;
  s_mazeInfoPanelDrawn = false;
  s_lastInfoPanelTimeText[0] = '\0';
  s_lastInfoPanelDateText[0] = '\0';
  s_mazeDirtyAllCells = true;
  s_mazeDirtyCellCount = 0;
}

void clearMazeDirtyState() {
  s_mazeDirtyAllCells = false;
  s_mazeDirtyCellCount = 0;
}

void requestFullMazeRender() {
  s_mazeDirtyAllCells = true;
  s_mazeDirtyCellCount = 0;
  s_needsRender = true;
}

void markMazeCellDirtyIndex(int index) {
  if (index < 0 || index >= s_mazeDim * s_mazeDim) {
    return;
  }
  if (s_mazeDirtyAllCells) {
    s_needsRender = true;
    return;
  }
  for (int i = 0; i < s_mazeDirtyCellCount; i++) {
    if (s_mazeDirtyCells[i] == static_cast<uint16_t>(index)) {
      s_needsRender = true;
      return;
    }
  }
  if (s_mazeDirtyCellCount >= kMazeDirtyCellCapacity) {
    requestFullMazeRender();
    return;
  }
  s_mazeDirtyCells[s_mazeDirtyCellCount++] = static_cast<uint16_t>(index);
  s_needsRender = true;
}

void markMazeCellDirtyCoord(int x, int y) {
  if (x < 0 || x >= s_mazeDim || y < 0 || y >= s_mazeDim) {
    return;
  }
  markMazeCellDirtyIndex(y * s_mazeDim + x);
}

int currentGenerationCursorIndex() {
  if ((s_mazePhase == 0 || s_mazePhase == 1) && s_mazeStackSize > 0) {
    return s_mazeTraversal[s_mazeStackSize - 1];
  }
  return -1;
}

int currentRevealCursorIndex() {
  if (s_mazePhase == 4 && s_mazeSolveIndex > 0 && s_mazePathLength > 0) {
    return s_mazeTraversal[min(s_mazeSolveIndex - 1, s_mazePathLength - 1)];
  }
  return -1;
}

int currentTravelerIndex() {
  if (s_mazePhase >= 6 && s_mazePathLength > 0) {
    return s_mazeTraversal[s_mazeTravelIndex];
  }
  return -1;
}

int resolveMazeSolveCursorIndex();
bool isMazeReservedCell(int x, int y);

void markMazeCursorDirtyPair(int beforeIndex, int afterIndex) {
  if (beforeIndex >= 0) {
    markMazeCellDirtyIndex(beforeIndex);
  }
  if (afterIndex >= 0 && afterIndex != beforeIndex) {
    markMazeCellDirtyIndex(afterIndex);
  }
}

inline void yieldMazeWork(uint32_t& counter, uint32_t mask = 0x3FU) {
  counter += 1U;
  if ((counter & mask) == 0U) {
    yield();
  }
}

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return MatrixPanel_I2S_DMA::color565(r, g, b);
}

uint16_t mazeWallColor565() {
  return rgb565(12, 12, 14);
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

bool cacheMazeConfigColors() {
  MazeRgbColor generationPathColor = {};
  MazeRgbColor searchVisitedColor = {};
  MazeRgbColor searchFrontierColor = {};
  MazeRgbColor panelBgColor = {};
  MazeRgbColor borderColor = {};
  MazeRgbColor timeColor = {};
  MazeRgbColor dateColor = {};

  if (!parseHexColorText(s_config.generationPathColor, generationPathColor) ||
      !parseHexColorText(s_config.searchVisitedColor, searchVisitedColor) ||
      !parseHexColorText(s_config.searchFrontierColor, searchFrontierColor) ||
      !parseHexColorText(s_config.solvedPathStartColor, s_solvedPathStartColor) ||
      !parseHexColorText(s_config.solvedPathEndColor, s_solvedPathEndColor) ||
      !parseHexColorText(s_config.panelBgColor, panelBgColor) ||
      !parseHexColorText(s_config.borderColor, borderColor) ||
      !parseHexColorText(s_config.timeColor, timeColor) ||
      !parseHexColorText(s_config.dateColor, dateColor)) {
    return false;
  }

  s_generationPathColor565 = rgb565(
    generationPathColor.r,
    generationPathColor.g,
    generationPathColor.b
  );
  s_searchVisitedColor565 = rgb565(
    searchVisitedColor.r,
    searchVisitedColor.g,
    searchVisitedColor.b
  );
  s_searchFrontierColor565 = rgb565(
    searchFrontierColor.r,
    searchFrontierColor.g,
    searchFrontierColor.b
  );
  s_panelBgColor565 = rgb565(panelBgColor.r, panelBgColor.g, panelBgColor.b);
  s_borderColor565 = rgb565(borderColor.r, borderColor.g, borderColor.b);
  s_timeColor565 = rgb565(timeColor.r, timeColor.g, timeColor.b);
  s_dateColor565 = rgb565(dateColor.r, dateColor.g, dateColor.b);
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
    if ((py & 0x03) == 0x03) {
      yield();
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
    if ((py & 0x03) == 0x03) {
      yield();
    }
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

int resolveMazeDisplayOffsetY() {
  return s_config.mazeSizeMode == MAZE_SIZE_WIDE ? 1 : 0;
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

int mazeRenderCoordStartWithOffset(int coord, int offset) {
  return mazeRenderCoordStart(coord) + offset;
}

int mazeRenderCoordSize(int coord) {
  if (coord < 0 || coord >= s_mazeDim) {
    return 0;
  }
  return s_mazeRenderSize[coord];
}

bool screenRangeIntersectsCoord(int start, int end, int coord, int offset = 0) {
  int coordStart = mazeRenderCoordStartWithOffset(coord, offset);
  int coordEnd = coordStart + mazeRenderCoordSize(coord) - 1;
  return !(coordEnd < start || coordStart > end);
}

int findReservedCoordStart(int screenStart, int screenEnd, int offset = 0) {
  for (int coord = 0; coord < s_mazeDim; coord++) {
    if (screenRangeIntersectsCoord(screenStart, screenEnd, coord, offset)) {
      return coord;
    }
  }
  return 0;
}

int findReservedCoordEnd(int screenStart, int screenEnd, int offset = 0) {
  for (int coord = s_mazeDim - 1; coord >= 0; coord--) {
    if (screenRangeIntersectsCoord(screenStart, screenEnd, coord, offset)) {
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
  if (s_mazeTraversal != nullptr) {
    free(s_mazeTraversal);
    s_mazeTraversal = nullptr;
  }
  if (s_mazeParent != nullptr) {
    free(s_mazeParent);
    s_mazeParent = nullptr;
  }
  if (s_mazeVisited != nullptr) {
    free(s_mazeVisited);
    s_mazeVisited = nullptr;
  }
  s_mazeGrid = nullptr;
  s_mazeTraversal = nullptr;
  s_mazeParent = nullptr;
  s_mazeVisited = nullptr;
  s_mazeCapacity = 0;
}

bool ensureMazeBuffers(int totalCells) {
  if (totalCells <= 0 || totalCells > kMaxMazeCells) {
    setLastError("maze invalid grid cells");
    return false;
  }
  if (s_mazeCapacity == totalCells &&
      s_mazeGrid != nullptr &&
      s_mazeTraversal != nullptr &&
      s_mazeParent != nullptr &&
      s_mazeVisited != nullptr) {
    clearLastError();
    return true;
  }

  releaseMazeBuffers();
  s_mazeGrid = static_cast<uint8_t*>(allocateEffectBuffer(totalCells));
  s_mazeTraversal = static_cast<uint16_t*>(allocateEffectBuffer(sizeof(uint16_t) * totalCells));
  s_mazeParent = static_cast<int16_t*>(allocateEffectBuffer(sizeof(int16_t) * totalCells));
  s_mazeVisited = static_cast<uint8_t*>(allocateEffectBuffer(totalCells));
  if (s_mazeGrid == nullptr ||
      s_mazeTraversal == nullptr ||
      s_mazeParent == nullptr ||
      s_mazeVisited == nullptr) {
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
  int drawX = mazeRenderCoordStartWithOffset(x, 0);
  int drawY = mazeRenderCoordStartWithOffset(y, resolveMazeDisplayOffsetY());
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

void fillMazeFrameBuffer(uint16_t* frameBuffer, uint16_t color) {
  if (frameBuffer == nullptr) {
    return;
  }
  for (int index = 0; index < kPanelSize * kPanelSize; index++) {
    frameBuffer[index] = color;
  }
}

void writeMazeFrameRect(
  uint16_t* frameBuffer,
  int x,
  int y,
  int width,
  int height,
  uint16_t color
) {
  if (frameBuffer == nullptr || width <= 0 || height <= 0) {
    return;
  }

  if (x < 0) {
    width += x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    y = 0;
  }
  if (x + width > kPanelSize) {
    width = kPanelSize - x;
  }
  if (y + height > kPanelSize) {
    height = kPanelSize - y;
  }
  if (width <= 0 || height <= 0) {
    return;
  }

  for (int py = 0; py < height; py++) {
    uint16_t* row = frameBuffer + ((y + py) * kPanelSize + x);
    for (int px = 0; px < width; px++) {
      row[px] = color;
    }
  }
}

void writeMazeFrameCell(uint16_t* frameBuffer, int x, int y, uint16_t color) {
  int drawX = mazeRenderCoordStartWithOffset(x, 0);
  int drawY = mazeRenderCoordStartWithOffset(y, resolveMazeDisplayOffsetY());
  int drawW = mazeRenderCoordSize(x);
  int drawH = mazeRenderCoordSize(y);
  writeMazeFrameRect(frameBuffer, drawX, drawY, drawW, drawH, color);
}

uint16_t readMazeFrameCellColor(const uint16_t* frameBuffer, int x, int y) {
  if (frameBuffer == nullptr) {
    return 0;
  }
  int drawX = mazeRenderCoordStartWithOffset(x, 0);
  int drawY = mazeRenderCoordStartWithOffset(y, resolveMazeDisplayOffsetY());
  if (drawX < 0 || drawX >= kPanelSize || drawY < 0 || drawY >= kPanelSize) {
    return 0;
  }
  return frameBuffer[drawY * kPanelSize + drawX];
}

void buildMazeInfoPanelTexts(char timeText[6], char dateText[6]) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    snprintf(timeText, 6, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    snprintf(dateText, 6, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    return;
  }

  if (s_mazeInfoPanelDrawn &&
      s_lastInfoPanelTimeText[0] != '\0' &&
      s_lastInfoPanelDateText[0] != '\0') {
    memcpy(timeText, s_lastInfoPanelTimeText, sizeof(s_lastInfoPanelTimeText));
    memcpy(dateText, s_lastInfoPanelDateText, sizeof(s_lastInfoPanelDateText));
    return;
  }

  memcpy(timeText, "--:--", 6);
  memcpy(dateText, "--.--", 6);
}

bool isMazeInfoPanelRefreshNeeded() {
  if (!s_mazeInfoPanel.active) {
    return false;
  }
  char timeText[6] = "";
  char dateText[6] = "";
  buildMazeInfoPanelTexts(timeText, dateText);
  return !s_mazeInfoPanelDrawn ||
         strcmp(timeText, s_lastInfoPanelTimeText) != 0 ||
         strcmp(dateText, s_lastInfoPanelDateText) != 0;
}

bool canDirectPresentMazeCells() {
  return !DisplayManager::doubleBufferEnabled &&
         DisplayManager::dma_display != nullptr &&
         DisplayManager::liveFrameValid;
}

bool tryMergeMazeRectUpdate(
  DisplayManager::SolidRectUpdate* updates,
  size_t updateCount,
  const DisplayManager::SolidRectUpdate& candidate
) {
  for (size_t i = 0; i < updateCount; i++) {
    DisplayManager::SolidRectUpdate& current = updates[i];
    if (current.color != candidate.color) {
      continue;
    }

    if (current.y == candidate.y &&
        current.height == candidate.height &&
        candidate.x <= current.x + current.width &&
        current.x <= candidate.x + candidate.width) {
      int mergedStart = min((int)current.x, (int)candidate.x);
      int mergedEnd = max(
        (int)current.x + (int)current.width,
        (int)candidate.x + (int)candidate.width
      );
      current.x = static_cast<uint8_t>(mergedStart);
      current.width = static_cast<uint8_t>(mergedEnd - mergedStart);
      return true;
    }

    if (current.x == candidate.x &&
        current.width == candidate.width &&
        candidate.y <= current.y + current.height &&
        current.y <= candidate.y + candidate.height) {
      int mergedStart = min((int)current.y, (int)candidate.y);
      int mergedEnd = max(
        (int)current.y + (int)current.height,
        (int)candidate.y + (int)candidate.height
      );
      current.y = static_cast<uint8_t>(mergedStart);
      current.height = static_cast<uint8_t>(mergedEnd - mergedStart);
      return true;
    }
  }

  return false;
}

size_t buildMazePresentUpdates(
  const uint16_t* frameBuffer,
  DisplayManager::SolidRectUpdate* updates,
  size_t capacity
) {
  if (frameBuffer == nullptr || updates == nullptr || capacity == 0) {
    return 0;
  }

  size_t updateCount = 0;
  for (int i = 0; i < s_mazeDirtyCellCount; i++) {
    int index = s_mazeDirtyCells[i];
    int x = index % s_mazeDim;
    int y = index / s_mazeDim;
    if (isMazeReservedCell(x, y)) {
      continue;
    }

    int drawX = mazeRenderCoordStartWithOffset(x, 0);
    int drawY = mazeRenderCoordStartWithOffset(y, resolveMazeDisplayOffsetY());
    int drawW = mazeRenderCoordSize(x);
    int drawH = mazeRenderCoordSize(y);
    if (drawW <= 0 || drawH <= 0 ||
        drawX < 0 || drawY < 0 ||
        drawX >= kPanelSize || drawY >= kPanelSize) {
      continue;
    }

    uint16_t nextColor = readMazeFrameCellColor(frameBuffer, x, y);
    if (DisplayManager::liveFrameBuffer[drawY][drawX] == nextColor) {
      continue;
    }

    DisplayManager::SolidRectUpdate candidate = {
      static_cast<uint8_t>(drawX),
      static_cast<uint8_t>(drawY),
      static_cast<uint8_t>(drawW),
      static_cast<uint8_t>(drawH),
      nextColor
    };
    if (tryMergeMazeRectUpdate(updates, updateCount, candidate)) {
      continue;
    }
    if (updateCount >= capacity) {
      return 0;
    }
    updates[updateCount++] = candidate;
  }

  return updateCount;
}

void drawMazeInfoPanelFrame(MatrixPanel_I2S_DMA* display, const char* timeText, const char* dateText) {
  if (display == nullptr || !s_mazeInfoPanel.active) {
    return;
  }

  drawFilledRectPixels(
    display,
    s_mazeInfoPanel.panelX,
    s_mazeInfoPanel.panelY,
    s_mazeInfoPanel.panelWidth,
    s_mazeInfoPanel.panelHeight,
    s_panelBgColor565
  );
  drawRectOutlinePixels(
    display,
    s_mazeInfoPanel.panelX,
    s_mazeInfoPanel.panelY,
    s_mazeInfoPanel.panelWidth,
    s_mazeInfoPanel.panelHeight,
    s_borderColor565
  );
  drawClockText(
    display,
    timeText,
    s_mazeInfoPanel.timeTextX,
    s_mazeInfoPanel.timeTextY,
    s_timeColor565,
    CLOCK_FONT_MINIMAL_3X5,
    s_mazeInfoPanel.timeScale
  );
  drawClockText(
    display,
    dateText,
    s_mazeInfoPanel.dateTextX,
    s_mazeInfoPanel.dateTextY,
    s_dateColor565,
    CLOCK_FONT_MINIMAL_3X5,
    s_mazeInfoPanel.dateScale
  );
}

void resetMazeInfoPanel() {
  s_mazeInfoPanel = {};
}

void configureMazeInfoPanel() {
  resetMazeInfoPanel();
  if (!s_config.showClock) {
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
  const int panelY = (kPanelSize - panelHeight) / 2 + resolveMazeDisplayOffsetY();
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
    findReservedCoordStart(reservedScreenLeft, reservedScreenRight, 0),
    0,
    s_mazeDim - 1
  );
  s_mazeInfoPanel.reservedTop = clampInt(
    findReservedCoordStart(reservedScreenTop, reservedScreenBottom, resolveMazeDisplayOffsetY()),
    0,
    s_mazeDim - 1
  );
  s_mazeInfoPanel.reservedRight = clampInt(
    findReservedCoordEnd(reservedScreenLeft, reservedScreenRight, 0),
    0,
    s_mazeDim - 1
  );
  s_mazeInfoPanel.reservedBottom = clampInt(
    findReservedCoordEnd(reservedScreenTop, reservedScreenBottom, resolveMazeDisplayOffsetY()),
    0,
    s_mazeDim - 1
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

  char timeText[6] = "";
  char dateText[6] = "";
  buildMazeInfoPanelTexts(timeText, dateText);
  drawMazeInfoPanelFrame(display, timeText, dateText);
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
  s_mazeRevealCount += 1;
  markMazeCellDirtyIndex(index);
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
  memset(s_mazeParent, 0xFF, sizeof(int16_t) * totalCells);
  s_mazeQueueHead = 0;
  s_mazeQueueTail = 0;
  s_mazeSolveCursorIndex = -1;
  s_mazePathLength = 0;
  s_mazeSolveIndex = 0;
}

void beginSolvePhase() {
  resetSolveState();
  int startIndex = mazeStartIndex();
  s_mazeTraversal[s_mazeQueueTail++] = (uint16_t)startIndex;
  s_mazeVisited[startIndex] = 1;
  s_mazeParent[startIndex] = -1;
}

void buildSolvedPath(int endIndex) {
  s_mazePathLength = 0;
  int cursor = endIndex;
  uint32_t workCounter = 0;
  while (cursor >= 0) {
    s_mazeTraversal[s_mazePathLength++] = (uint16_t)cursor;
    cursor = s_mazeParent[cursor];
    yieldMazeWork(workCounter);
  }
  for (int i = 0; i < s_mazePathLength / 2; i++) {
    uint16_t temp = s_mazeTraversal[i];
    s_mazeTraversal[i] = s_mazeTraversal[s_mazePathLength - 1 - i];
    s_mazeTraversal[s_mazePathLength - 1 - i] = temp;
    yieldMazeWork(workCounter);
  }

  int totalCells = s_mazeDim * s_mazeDim;
  memset(s_mazeParent, 0xFF, sizeof(int16_t) * totalCells);
  for (int i = 0; i < s_mazePathLength; i++) {
    s_mazeParent[s_mazeTraversal[i]] = static_cast<int16_t>(i);
    yieldMazeWork(workCounter);
  }
}

bool stepGeneration(int steps) {
  for (int step = 0; step < steps; step++) {
    if (s_mazeStackSize <= 0) {
      return true;
    }

    uint16_t currentIndex = s_mazeTraversal[s_mazeStackSize - 1];
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
      s_mazeTraversal[s_mazeStackSize++] = (uint16_t)nextIndex;
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

    int current = (int)s_mazeTraversal[s_mazeQueueHead++];
    if (s_mazeVisited[current] == 2) {
      continue;
    }
    s_mazeVisited[current] = 2;
    s_mazeSolveCursorIndex = current;
    markMazeCellDirtyIndex(current);
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
      s_mazeTraversal[s_mazeQueueTail++] = (uint16_t)nextIndex;
      markMazeCellDirtyIndex(nextIndex);
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
  if (!cacheMazeConfigColors()) {
    setLastError("invalid maze colors");
    s_active = false;
    return;
  }
  int totalCells = s_mazeDim * s_mazeDim;
  if (!ensureMazeBuffers(totalCells)) {
    s_active = false;
    return;
  }

  resetMazeFrameState();
  memset(s_mazeGrid, 0, totalCells);
  memset(s_mazeVisited, 0, totalCells);
  s_mazeRevealCount = 0;
  s_mazePathLength = 0;
  s_mazePhase = 0;
  s_mazeSolveIndex = 0;
  s_mazeTravelIndex = 0;
  s_mazeHoldTicks = 0;
  s_mazeStackSize = 0;
  s_mazeQueueHead = 0;
  s_mazeQueueTail = 0;
  s_mazeSolveCursorIndex = -1;
  s_mazeSeed = (uint32_t)(millis() ^ (s_config.speed * 131U) ^ (s_config.mazeSizeMode * 977U));
  s_lastRenderDebugPhase = -1;

  int startIndex = mazeStartIndex();
  s_mazeVisited[startIndex] = 1;
  s_mazeTraversal[s_mazeStackSize++] = (uint16_t)startIndex;
  carveMazeCell(1, 1);
  s_lastTickAt = 0;

  s_needsRender = true;
}

void updateMaze() {
  s_needsRender = false;
  const int generationCursorBefore = currentGenerationCursorIndex();
  const int solveCursorBefore = resolveMazeSolveCursorIndex();
  const int revealCursorBefore = currentRevealCursorIndex();
  const int travelerBefore = currentTravelerIndex();

  if (s_mazePhase == 0) {
    int step = resolveGenerationStepsPerTick();
    bool completed = stepGeneration(step);
    if (completed) {
      setMazePhase(1);
      s_mazeHoldTicks = 0;
    }
    markMazeCursorDirtyPair(generationCursorBefore, currentGenerationCursorIndex());
    if (!s_needsRender) {
      s_needsRender = isMazeInfoPanelRefreshNeeded();
    }
    return;
  }

  if (s_mazePhase == 1) {
    s_mazeHoldTicks += 1;
    if (s_mazeHoldTicks >= 8) {
      beginSolvePhase();
      setMazePhase(2);
      markMazeCursorDirtyPair(generationCursorBefore, currentGenerationCursorIndex());
      markMazeCursorDirtyPair(solveCursorBefore, resolveMazeSolveCursorIndex());
    }
    if (!s_needsRender) {
      s_needsRender = isMazeInfoPanelRefreshNeeded();
    }
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
      requestFullMazeRender();
    }
    markMazeCursorDirtyPair(solveCursorBefore, resolveMazeSolveCursorIndex());
    if (!s_needsRender) {
      s_needsRender = isMazeInfoPanelRefreshNeeded();
    }
    return;
  }

  if (s_mazePhase == 3) {
    s_mazeHoldTicks += 1;
    if (s_mazeHoldTicks >= 8) {
      s_mazeSolveIndex = 0;
      setMazePhase(4);
      requestFullMazeRender();
    }
    if (!s_needsRender) {
      s_needsRender = isMazeInfoPanelRefreshNeeded();
    }
    return;
  }

  if (s_mazePhase == 4) {
    int step = resolveRevealStepsPerTick();
    int visibleBefore = s_mazeSolveIndex;
    s_mazeSolveIndex += step;
    if (s_mazeSolveIndex >= s_mazePathLength) {
      s_mazeSolveIndex = s_mazePathLength;
      setMazePhase(5);
      s_mazeHoldTicks = 0;
    }
    for (int i = visibleBefore; i < s_mazeSolveIndex; i++) {
      markMazeCellDirtyIndex(s_mazeTraversal[i]);
    }
    markMazeCursorDirtyPair(revealCursorBefore, currentRevealCursorIndex());
    if (!s_needsRender) {
      s_needsRender = isMazeInfoPanelRefreshNeeded();
    }
    return;
  }

  if (s_mazePhase == 5) {
    s_mazeHoldTicks += 1;
    if (s_mazeHoldTicks >= 8) {
      s_mazeTravelIndex = 0;
      setMazePhase(6);
      markMazeCursorDirtyPair(travelerBefore, currentTravelerIndex());
    }
    if (!s_needsRender) {
      s_needsRender = isMazeInfoPanelRefreshNeeded();
    }
    return;
  }

  if (s_mazePhase == 6) {
    s_mazeTravelIndex += resolveTravelStepsPerTick();
    if (s_mazeTravelIndex >= s_mazePathLength) {
      s_mazeTravelIndex = s_mazePathLength > 0 ? s_mazePathLength - 1 : 0;
      setMazePhase(7);
      s_mazeHoldTicks = 0;
    }
    markMazeCursorDirtyPair(travelerBefore, currentTravelerIndex());
    if (!s_needsRender) {
      s_needsRender = isMazeInfoPanelRefreshNeeded();
    }
    return;
  }

  s_mazeHoldTicks += 1;
  if (s_mazeHoldTicks >= 12) {
    initMaze();
    requestFullMazeRender();
    return;
  }
  if (!s_needsRender) {
    s_needsRender = isMazeInfoPanelRefreshNeeded();
  }
}

uint16_t resolveSolvedPathColor565(int pathOrder) {
  MazeRgbColor pathColor = interpolateColor(
    s_solvedPathStartColor,
    s_solvedPathEndColor,
    pathOrder,
    s_mazePathLength
  );
  return rgb565(pathColor.r, pathColor.g, pathColor.b);
}

int resolveMazeSolveCursorIndex() {
  if (s_mazePhase == 2 && s_mazeSolveCursorIndex >= 0) {
    return s_mazeSolveCursorIndex;
  }
  if (s_mazePhase == 3 && s_mazePathLength > 0) {
    return s_mazeTraversal[s_mazePathLength - 1];
  }
  return -1;
}

uint16_t resolveMazeCellColor(int index) {
  uint16_t color = mazeWallColor565();
  if (index < 0 || index >= s_mazeDim * s_mazeDim) {
    return color;
  }

  if (s_mazeGrid[index] != 0) {
    color = s_generationPathColor565;
  }

  if ((s_mazePhase == 2 || s_mazePhase == 3) && s_mazeGrid[index] != 0) {
    if (s_mazeVisited[index] == 2) {
      color = s_searchVisitedColor565;
    } else if (s_mazeVisited[index] == 1) {
      color = s_searchFrontierColor565;
    }
  }

  if (s_mazePathLength > 0 && s_mazePhase >= 4) {
    int pathOrder = s_mazeParent[index];
    int visibleCount = s_mazePhase == 4 ? s_mazeSolveIndex : s_mazePathLength;
    if (pathOrder >= 0 && pathOrder < visibleCount) {
      color = resolveSolvedPathColor565(pathOrder);
    }
  }

  if ((s_mazePhase == 0 || s_mazePhase == 1) &&
      s_mazeStackSize > 0 &&
      s_mazeTraversal[s_mazeStackSize - 1] == index) {
    color = rgb565(76, 220, 72);
  }

  int solveCursorIndex = resolveMazeSolveCursorIndex();
  if (solveCursorIndex == index) {
    color = rgb565(255, 196, 92);
  }

  if (s_mazePhase == 4 && s_mazeSolveIndex > 0 && s_mazePathLength > 0) {
    int revealCursorIndex = s_mazeTraversal[min(s_mazeSolveIndex - 1, s_mazePathLength - 1)];
    if (revealCursorIndex == index) {
      color = rgb565(255, 244, 172);
    }
  }

  int startIndex = mazeStartIndex();
  int endIndex = mazeEndIndex();
  if (s_mazePathLength > 0) {
    startIndex = s_mazeTraversal[0];
    endIndex = s_mazeTraversal[s_mazePathLength - 1];
  }
  if (startIndex == index) {
    color = rgb565(40, 255, 120);
  }
  if (endIndex == index) {
    color = rgb565(90, 188, 255);
  }

  if (s_mazePhase >= 6 && s_mazePathLength > 0) {
    int travelerIndex = s_mazeTraversal[s_mazeTravelIndex];
    if (travelerIndex == index) {
      color = rgb565(255, 244, 172);
    }
  }

  return color;
}

MazeRenderPassResult renderMazeFrame(uint16_t* frameBuffer, MatrixPanel_I2S_DMA* display) {
  MazeRenderPassResult result = {};
  if (frameBuffer == nullptr || display == nullptr) {
    return result;
  }

  uint32_t workCounter = 0;
  result.usedFullRefresh = !s_mazeFrameInitialized || s_mazeDirtyAllCells;
  if (result.usedFullRefresh) {
    for (int y = 0; y < s_mazeDim; y++) {
      for (int x = 0; x < s_mazeDim; x++) {
        if (isMazeReservedCell(x, y)) {
          yieldMazeWork(workCounter, 0x7FU);
          continue;
        }

        int index = y * s_mazeDim + x;
        uint16_t nextColor = resolveMazeCellColor(index);
        if (readMazeFrameCellColor(frameBuffer, x, y) != nextColor) {
          writeMazeFrameCell(frameBuffer, x, y, nextColor);
        }
        yieldMazeWork(workCounter, 0x7FU);
      }
    }
  } else {
    for (int i = 0; i < s_mazeDirtyCellCount; i++) {
      int index = s_mazeDirtyCells[i];
      int x = index % s_mazeDim;
      int y = index / s_mazeDim;
      if (!isMazeReservedCell(x, y)) {
        uint16_t nextColor = resolveMazeCellColor(index);
        if (readMazeFrameCellColor(frameBuffer, x, y) != nextColor) {
          writeMazeFrameCell(frameBuffer, x, y, nextColor);
        }
      }
      yieldMazeWork(workCounter, 0x7FU);
    }
  }

  if (s_mazeInfoPanel.active) {
    char timeText[6] = "";
    char dateText[6] = "";
    buildMazeInfoPanelTexts(timeText, dateText);
    result.panelChanged =
      !s_mazeInfoPanelDrawn ||
      strcmp(timeText, s_lastInfoPanelTimeText) != 0 ||
      strcmp(dateText, s_lastInfoPanelDateText) != 0;
    if (result.panelChanged) {
      drawMazeInfoPanelFrame(display, timeText, dateText);
      memcpy(s_lastInfoPanelTimeText, timeText, sizeof(s_lastInfoPanelTimeText));
      memcpy(s_lastInfoPanelDateText, dateText, sizeof(s_lastInfoPanelDateText));
      s_mazeInfoPanelDrawn = true;
    }
  }

  s_mazeFrameInitialized = true;
  yield();
  return result;
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
  s_mazeSolveIndex = 0;
  s_mazeTravelIndex = 0;
  s_mazeHoldTicks = 0;
  s_mazeStackSize = 0;
  s_mazeQueueHead = 0;
  s_mazeQueueTail = 0;
  s_mazeSolveCursorIndex = -1;
  s_mazeSeed = 0;
  s_lastRenderDebugPhase = -1;
  resetMazeFrameState();
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
  s_mazeSolveIndex = 0;
  s_mazeTravelIndex = 0;
  s_mazeHoldTicks = 0;
  s_mazeStackSize = 0;
  s_mazeQueueHead = 0;
  s_mazeQueueTail = 0;
  s_mazeSolveCursorIndex = -1;
  s_mazeSeed = 0;
  s_lastRenderDebugPhase = -1;
  resetMazeFrameState();
  resetMazeInfoPanel();
  releaseMazeBuffers();
}

void applyConfig(const MazeModeConfig& config) {
  clearLastError();
  if (!isValidMazeConfig(config)) {
    s_active = false;
    s_needsRender = false;
    resetMazeFrameState();
    setLastError("invalid maze config");
    return;
  }
  s_config = config;
  s_active = true;
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
  MatrixPanel_I2S_DMA* offscreen = DisplayManager::beginOffscreenFrame(
    frameBuffer,
    mazeWallColor565(),
    !s_mazeFrameInitialized
  );
  if (offscreen == nullptr) {
    return;
  }

  MazeRenderPassResult renderResult = renderMazeFrame(frameBuffer, offscreen);
  bool usedDirectPresent =
    !renderResult.usedFullRefresh &&
    !renderResult.panelChanged &&
    canDirectPresentMazeCells();
  if (usedDirectPresent) {
    DisplayManager::SolidRectUpdate updates[kMazeDirtyCellCapacity] = {};
    size_t updateCount = buildMazePresentUpdates(frameBuffer, updates, kMazeDirtyCellCapacity);
    if (updateCount > 0) {
      DisplayManager::presentSolidRectUpdates(updates, updateCount);
    }
  } else {
    DisplayManager::presentOffscreenFrame(frameBuffer);
  }
  clearMazeDirtyState();
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
