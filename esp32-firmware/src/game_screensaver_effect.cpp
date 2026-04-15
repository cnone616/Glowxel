#include "game_screensaver_effect.h"

#include <string.h>
#include "clock_font_renderer.h"
#include "display_manager.h"

namespace {
constexpr int kPanelSize = 64;
constexpr int kMaxSnakeCells = kPanelSize * kPanelSize;
constexpr int kMaxMazeCells = 63 * 63;
constexpr int kMazeQueueSize = 63 * 63;
constexpr int kTetrisCols = 10;
constexpr int kTetrisRows = 20;
constexpr uint16_t kTetrisTimePanelColor = 0x0843;
constexpr uint16_t kTetrisTimeColor = 0x763F;
constexpr uint16_t kTetrisTimeBorderColor = 0x21CA;

struct Point {
  int16_t x;
  int16_t y;
};

GameScreensaverConfig s_config = {
  GAME_SCREENSAVER_MAZE,
  6,
  1,
  GAME_SCREENSAVER_MAZE_WIDE,
  2,
  true
};

bool s_active = false;
bool s_needsRender = true;
unsigned long s_lastTickAt = 0;

Point* s_snake = nullptr;
int s_snakeLength = 0;
Point s_food = {0, 0};
int s_snakeGridWidth = 64;
int s_snakeGridHeight = 64;
int s_snakeCellSize = 1;
int s_snakeOriginX = 0;
int s_snakeOriginY = 0;
int s_snakeDirection = 1;
bool s_snakeDead = false;
bool s_snakeWon = false;
int s_snakeHoldTicks = 0;
int s_snakeCapacity = 0;
uint8_t* s_snakeVisited = nullptr;
int16_t* s_snakeDistance = nullptr;
uint16_t* s_snakeQueue = nullptr;

uint8_t* s_mazeGrid = nullptr;
uint16_t* s_mazeReveal = nullptr;
int s_mazeRevealCount = 0;
uint16_t* s_mazePath = nullptr;
int s_mazePathLength = 0;
int s_mazeDim = 31;
int s_mazeScale = 2;
int s_mazeOrigin = 1;
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

uint8_t s_tetrisBoard[kTetrisRows][kTetrisCols];
int s_tetrisPiece = 0;
int s_tetrisRotation = 0;
int s_tetrisX = 3;
int s_tetrisY = 0;
int s_tetrisTargetRotation = 0;
int s_tetrisTargetX = 3;
bool s_tetrisGameOver = false;
int s_tetrisHoldTicks = 0;
int s_tetrisCellSize = 2;
int s_tetrisOriginX = 0;
int s_tetrisOriginY = 0;

int s_pingBallX = 32;
int s_pingBallY = 32;
int s_pingBallVX = 1;
int s_pingBallVY = 1;
int s_pingPaddleLeftY = 24;
int s_pingPaddleRightY = 24;
int s_pingScoreLeft = 0;
int s_pingScoreRight = 0;
int s_pingHoldTicks = 0;

const int8_t kDirections[4][2] = {
  {0, -1},
  {1, 0},
  {0, 1},
  {-1, 0}
};

const uint16_t kTetrisPieces[7][4] = {
  {0x0F00, 0x2222, 0x00F0, 0x4444},
  {0x8E00, 0x6440, 0x0E20, 0x44C0},
  {0x2E00, 0x4460, 0x0E80, 0xC440},
  {0x6600, 0x6600, 0x6600, 0x6600},
  {0x6C00, 0x4620, 0x06C0, 0x8C40},
  {0x4E00, 0x4640, 0x0E40, 0x4C40},
  {0xC600, 0x2640, 0x0C60, 0x4C80}
};

const uint8_t kTetrisColors[7][3] = {
  {82, 196, 255},
  {59, 130, 246},
  {249, 115, 22},
  {250, 204, 21},
  {34, 197, 94},
  {168, 85, 247},
  {239, 68, 68}
};

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return MatrixPanel_I2S_DMA::color565(r, g, b);
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
  if (totalCells <= 0) {
    return false;
  }
  if (s_snakeCapacity == totalCells &&
      s_snake != nullptr &&
      s_snakeVisited != nullptr &&
      s_snakeDistance != nullptr &&
      s_snakeQueue != nullptr) {
    return true;
  }

  releaseSnakeBuffers();
  s_snake = static_cast<Point*>(malloc(sizeof(Point) * totalCells));
  s_snakeVisited = static_cast<uint8_t*>(malloc(totalCells));
  s_snakeDistance = static_cast<int16_t*>(malloc(sizeof(int16_t) * totalCells));
  s_snakeQueue = static_cast<uint16_t*>(malloc(sizeof(uint16_t) * totalCells));
  if (s_snake == nullptr || s_snakeVisited == nullptr || s_snakeDistance == nullptr || s_snakeQueue == nullptr) {
    releaseSnakeBuffers();
    return false;
  }
  s_snakeCapacity = totalCells;
  return true;
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
  if (totalCells <= 0) {
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
    return true;
  }

  releaseMazeBuffers();
  s_mazeGrid = static_cast<uint8_t*>(malloc(totalCells));
  s_mazeReveal = static_cast<uint16_t*>(malloc(sizeof(uint16_t) * totalCells));
  s_mazePath = static_cast<uint16_t*>(malloc(sizeof(uint16_t) * totalCells));
  s_mazeParent = static_cast<int16_t*>(malloc(sizeof(int16_t) * totalCells));
  s_mazeVisited = static_cast<uint8_t*>(malloc(totalCells));
  s_mazeStack = static_cast<uint16_t*>(malloc(sizeof(uint16_t) * totalCells));
  s_mazeQueue = static_cast<uint16_t*>(malloc(sizeof(uint16_t) * totalCells));
  if (s_mazeGrid == nullptr ||
      s_mazeReveal == nullptr ||
      s_mazePath == nullptr ||
      s_mazeParent == nullptr ||
      s_mazeVisited == nullptr ||
      s_mazeStack == nullptr ||
      s_mazeQueue == nullptr) {
    releaseMazeBuffers();
    return false;
  }
  s_mazeCapacity = totalCells;
  return true;
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

int resolveTetrisDropSpeed(uint8_t speed) {
  int safeSpeed = clampInt((int)speed, 1, 10);
  if (safeSpeed <= 3) {
    return 300;
  }
  if (safeSpeed >= 8) {
    return 80;
  }
  return 150;
}

uint32_t pseudoRandom(uint32_t seed) {
  return seed * 1664525UL + 1013904223UL;
}

void fillBlock(MatrixPanel_I2S_DMA* display, int x, int y, int size, uint8_t r, uint8_t g, uint8_t b) {
  if (display == nullptr) {
    return;
  }
  display->fillRect(x, y, size, size, rgb565(r, g, b));
}

void drawMazePixel(MatrixPanel_I2S_DMA* display, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (display == nullptr) {
    return;
  }
  if (s_mazeScale == 1) {
    display->drawPixel(s_mazeOrigin + x, s_mazeOrigin + y, rgb565(r, g, b));
    return;
  }
  display->fillRect(
    s_mazeOrigin + x * s_mazeScale,
    s_mazeOrigin + y * s_mazeScale,
    s_mazeScale,
    s_mazeScale,
    rgb565(r, g, b)
  );
}

bool pointEquals(const Point& a, const Point& b) {
  return a.x == b.x && a.y == b.y;
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
  int available = s_snakeGridWidth * s_snakeGridHeight - s_snakeLength;
  if (available <= 0) {
    s_food = {-1, -1};
    return;
  }

  int target = (int)(pseudoRandom(seed) % (uint32_t)available);
  int current = 0;
  for (int y = 0; y < s_snakeGridHeight; y++) {
    for (int x = 0; x < s_snakeGridWidth; x++) {
      if (isSnakeCellOccupied(x, y, 0)) {
        continue;
      }
      if (current == target) {
        s_food = {(int16_t)x, (int16_t)y};
        return;
      }
      current += 1;
    }
  }
}

void initSnake() {
  s_snakeCellSize = clampInt((int)s_config.snakeWidth, 1, 4);
  s_snakeGridWidth = kPanelSize / s_snakeCellSize;
  s_snakeGridHeight = kPanelSize / s_snakeCellSize;
  if (!ensureSnakeBuffers(s_snakeGridWidth * s_snakeGridHeight)) {
    s_active = false;
    return;
  }
  s_snakeOriginX = (kPanelSize - s_snakeGridWidth * s_snakeCellSize) / 2;
  s_snakeOriginY = (kPanelSize - s_snakeGridHeight * s_snakeCellSize) / 2;
  s_snakeLength = 4;
  int startX = s_snakeGridWidth / 2;
  int startY = s_snakeGridHeight / 2;
  s_snake[0] = {(int16_t)startX, (int16_t)startY};
  s_snake[1] = {(int16_t)(startX - 1), (int16_t)startY};
  s_snake[2] = {(int16_t)(startX - 2), (int16_t)startY};
  s_snake[3] = {(int16_t)(startX - 3), (int16_t)startY};
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
      if (isSnakeCellOccupied(nx, ny, 1)) {
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
  bool eating = s_food.x == x && s_food.y == y;
  return !isSnakeCellOccupied(x, y, eating ? 0 : 1);
}

int pickSnakeDirection() {
  buildSnakeDistanceMap();
  int bestDir = -1;
  int bestDistance = 32767;
  for (int dir = 0; dir < 4; dir++) {
    int nx = s_snake[0].x + kDirections[dir][0];
    int ny = s_snake[0].y + kDirections[dir][1];
    if (!canSnakeMoveTo(nx, ny)) {
      continue;
    }
    int nextIdx = snakeIndex(nx, ny);
    int distance = s_snakeDistance[nextIdx];
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
  Point nextHead = {
    (int16_t)(s_snake[0].x + kDirections[s_snakeDirection][0]),
    (int16_t)(s_snake[0].y + kDirections[s_snakeDirection][1])
  };
  bool eating = pointEquals(nextHead, s_food);

  for (int i = s_snakeLength; i > 0; i--) {
    s_snake[i] = s_snake[i - 1];
  }
  s_snake[0] = nextHead;
  if (eating) {
    s_snakeLength += 1;
    if (s_snakeLength >= s_snakeGridWidth * s_snakeGridHeight) {
      s_snakeWon = true;
      s_snakeHoldTicks = 0;
    } else {
      placeSnakeFood();
    }
  }

  s_needsRender = true;
}

void renderSnake(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr) {
    return;
  }

  for (int i = 0; i < s_snakeLength; i++) {
    uint8_t r = 86;
    uint8_t g = 214;
    uint8_t b = 120;
    if (i == 0) {
      r = 180;
      g = 255;
      b = 208;
    }
    if (s_snakeDead) {
      r = 255;
      g = 80;
      b = 80;
    }
    if (s_snakeWon) {
      r = 120;
      g = 210;
      b = 255;
    }
    fillBlock(
      display,
      s_snakeOriginX + s_snake[i].x * s_snakeCellSize,
      s_snakeOriginY + s_snake[i].y * s_snakeCellSize,
      s_snakeCellSize,
      r,
      g,
      b
    );
  }

  if (!s_snakeWon && s_food.x >= 0 && s_food.y >= 0) {
    fillBlock(
      display,
      s_snakeOriginX + s_food.x * s_snakeCellSize,
      s_snakeOriginY + s_food.y * s_snakeCellSize,
      s_snakeCellSize,
      255,
      168,
      84
    );
  }
}

void initPingPong() {
  s_pingBallX = 32;
  s_pingBallY = 32;
  s_pingBallVX = 1;
  s_pingBallVY = 1;
  s_pingPaddleLeftY = 24;
  s_pingPaddleRightY = 24;
  s_pingHoldTicks = 0;
  s_needsRender = true;
}

void updatePingPong() {
  if (s_pingHoldTicks > 0) {
    s_pingHoldTicks -= 1;
    if (s_pingHoldTicks == 0) {
      initPingPong();
    }
    s_needsRender = true;
    return;
  }

  if (s_pingBallY < s_pingPaddleLeftY + 2) {
    s_pingPaddleLeftY -= 1;
  } else if (s_pingBallY > s_pingPaddleLeftY + 3) {
    s_pingPaddleLeftY += 1;
  }
  if (s_pingBallY < s_pingPaddleRightY + 2) {
    s_pingPaddleRightY -= 1;
  } else if (s_pingBallY > s_pingPaddleRightY + 3) {
    s_pingPaddleRightY += 1;
  }
  s_pingPaddleLeftY = clampInt(s_pingPaddleLeftY, 1, 57);
  s_pingPaddleRightY = clampInt(s_pingPaddleRightY, 1, 57);

  s_pingBallX += s_pingBallVX;
  s_pingBallY += s_pingBallVY;

  if (s_pingBallY <= 1 || s_pingBallY >= 62) {
    s_pingBallVY = -s_pingBallVY;
    s_pingBallY = clampInt(s_pingBallY, 1, 62);
  }

  if (s_pingBallX <= 3) {
    if (s_pingBallY >= s_pingPaddleLeftY && s_pingBallY <= s_pingPaddleLeftY + 6) {
      s_pingBallVX = 1;
      if (s_pingBallY <= s_pingPaddleLeftY + 1) {
        s_pingBallVY = -1;
      } else if (s_pingBallY >= s_pingPaddleLeftY + 5) {
        s_pingBallVY = 1;
      }
      s_pingBallX = 3;
    } else {
      s_pingScoreRight = (s_pingScoreRight + 1) % 10;
      s_pingHoldTicks = 14;
    }
  }

  if (s_pingBallX >= 60) {
    if (s_pingBallY >= s_pingPaddleRightY && s_pingBallY <= s_pingPaddleRightY + 6) {
      s_pingBallVX = -1;
      if (s_pingBallY <= s_pingPaddleRightY + 1) {
        s_pingBallVY = -1;
      } else if (s_pingBallY >= s_pingPaddleRightY + 5) {
        s_pingBallVY = 1;
      }
      s_pingBallX = 60;
    } else {
      s_pingScoreLeft = (s_pingScoreLeft + 1) % 10;
      s_pingHoldTicks = 14;
    }
  }

  s_needsRender = true;
}

void renderPingPong(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr) {
    return;
  }

  uint16_t netColor = rgb565(68, 84, 110);
  for (int y = 1; y < 63; y += 4) {
    display->drawPixel(32, y, netColor);
    display->drawPixel(32, y + 1, netColor);
  }

  uint16_t paddleColor = rgb565(130, 204, 255);
  display->fillRect(1, s_pingPaddleLeftY, 2, 7, paddleColor);
  display->fillRect(61, s_pingPaddleRightY, 2, 7, paddleColor);

  uint16_t ballColor = rgb565(255, 218, 120);
  display->fillRect(s_pingBallX - 1, s_pingBallY - 1, 3, 3, ballColor);

  uint16_t scoreColor = rgb565(190, 214, 238);
  display->fillRect(6, 2, clampInt(s_pingScoreLeft, 0, 9) * 2, 2, scoreColor);
  display->fillRect(64 - 6 - clampInt(s_pingScoreRight, 0, 9) * 2, 2, clampInt(s_pingScoreRight, 0, 9) * 2, 2, scoreColor);
}

void carveMazeCell(int x, int y) {
  if (x < 0 || x >= s_mazeDim || y < 0 || y >= s_mazeDim) {
    return;
  }
  int index = y * s_mazeDim + x;
  if (s_mazeGrid[index] == 1) {
    return;
  }
  s_mazeGrid[index] = 1;
  s_mazeReveal[s_mazeRevealCount++] = (uint16_t)index;
}

void initMaze() {
  s_mazeDim = s_config.mazeSizeMode == GAME_SCREENSAVER_MAZE_DENSE ? 63 : 31;
  s_mazeScale = s_config.mazeSizeMode == GAME_SCREENSAVER_MAZE_DENSE ? 1 : 2;
  s_mazeOrigin = (kPanelSize - s_mazeDim * s_mazeScale) / 2;
  int totalCells = s_mazeDim * s_mazeDim;
  if (!ensureMazeBuffers(totalCells)) {
    s_active = false;
    return;
  }
  memset(s_mazeGrid, 0, totalCells);
  memset(s_mazeVisited, 0, totalCells);
  s_mazeRevealCount = 0;
  s_mazePathLength = 0;
  s_mazePhase = 0;
  s_mazeRevealIndex = 0;
  s_mazeSolveIndex = 0;
  s_mazeTravelIndex = 0;
  s_mazeHoldTicks = 0;

  int stackSize = 0;
  s_mazeStack[stackSize++] = (uint16_t)(1 * s_mazeDim + 1);
  s_mazeVisited[1 * s_mazeDim + 1] = 1;
  carveMazeCell(1, 1);

  while (stackSize > 0) {
    uint16_t currentIndex = s_mazeStack[stackSize - 1];
    int currentX = currentIndex % s_mazeDim;
    int currentY = currentIndex / s_mazeDim;
    int dirs[4] = {0, 1, 2, 3};
    for (int i = 3; i > 0; i--) {
      uint32_t seed = pseudoRandom((uint32_t)(currentX * 131 + currentY * 197 + i * 17 + stackSize * 29));
      int swapIndex = seed % (uint32_t)(i + 1);
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
      int nextIndex = ny * s_mazeDim + nx;
      if (s_mazeVisited[nextIndex]) {
        continue;
      }
      int wallX = currentX + kDirections[dirs[i]][0];
      int wallY = currentY + kDirections[dirs[i]][1];
      carveMazeCell(wallX, wallY);
      carveMazeCell(nx, ny);
      s_mazeVisited[nextIndex] = 1;
      s_mazeStack[stackSize++] = (uint16_t)nextIndex;
      advanced = true;
      break;
    }

    if (!advanced) {
      stackSize -= 1;
    }
  }

  bool found = false;
  int head = 0;
  int tail = 0;
  memset(s_mazeVisited, 0, totalCells);
  int startIndex = 1 * s_mazeDim + 1;
  int endIndex = (s_mazeDim - 2) * s_mazeDim + (s_mazeDim - 2);
  s_mazeQueue[tail++] = (uint16_t)startIndex;
  s_mazeVisited[startIndex] = 1;
  s_mazeParent[startIndex] = -1;

  while (head < tail) {
    int current = (int)s_mazeQueue[head++];
    if (current == endIndex) {
      found = true;
      break;
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
      if (s_mazeVisited[nextIndex] || s_mazeGrid[nextIndex] == 0) {
        continue;
      }
      s_mazeVisited[nextIndex] = 1;
      s_mazeParent[nextIndex] = (int16_t)current;
      s_mazeQueue[tail++] = (uint16_t)nextIndex;
    }
  }

  if (found) {
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

  s_needsRender = true;
}

void updateMaze() {
  if (s_mazePhase == 0) {
    int step = s_config.mazeSizeMode == GAME_SCREENSAVER_MAZE_DENSE ? 18 : 8;
    s_mazeRevealIndex += step;
    if (s_mazeRevealIndex >= s_mazeRevealCount) {
      s_mazeRevealIndex = s_mazeRevealCount;
      s_mazePhase = 1;
      s_mazeHoldTicks = 0;
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 1) {
    s_mazeHoldTicks += 1;
    if (s_mazeHoldTicks >= 8) {
      s_mazePhase = 2;
      s_mazeSolveIndex = 0;
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 2) {
    s_mazeSolveIndex += 6;
    if (s_mazeSolveIndex >= s_mazePathLength) {
      s_mazeSolveIndex = s_mazePathLength;
      s_mazePhase = 3;
      s_mazeHoldTicks = 0;
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 3) {
    s_mazeHoldTicks += 1;
    if (s_mazeHoldTicks >= 8) {
      s_mazePhase = 4;
      s_mazeTravelIndex = 0;
    }
    s_needsRender = true;
    return;
  }

  if (s_mazePhase == 4) {
    s_mazeTravelIndex += 1;
    if (s_mazeTravelIndex >= s_mazePathLength) {
      s_mazeTravelIndex = s_mazePathLength - 1;
      s_mazePhase = 5;
      s_mazeHoldTicks = 0;
    }
    s_needsRender = true;
    return;
  }

  s_mazeHoldTicks += 1;
  if (s_mazeHoldTicks >= 12) {
    initMaze();
  }
  s_needsRender = true;
}

void renderMaze(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr) {
    return;
  }

  int revealLimit = s_mazePhase == 0 ? s_mazeRevealIndex : s_mazeRevealCount;
  for (int i = 0; i < revealLimit; i++) {
    int index = s_mazeReveal[i];
    drawMazePixel(display, index % s_mazeDim, index / s_mazeDim, 210, 240, 255);
  }

  if (s_mazePhase >= 2) {
    int solveLimit = s_mazePhase == 2 ? s_mazeSolveIndex : s_mazePathLength;
    for (int i = 0; i < solveLimit; i++) {
      int index = s_mazePath[i];
      drawMazePixel(display, index % s_mazeDim, index / s_mazeDim, 96, 170, 255);
    }
  }

  if (s_mazePathLength > 0) {
    int startIndex = s_mazePath[0];
    int endIndex = s_mazePath[s_mazePathLength - 1];
    drawMazePixel(display, startIndex % s_mazeDim, startIndex / s_mazeDim, 94, 234, 145);
    drawMazePixel(display, endIndex % s_mazeDim, endIndex / s_mazeDim, 255, 131, 131);
  }

  if (s_mazePhase >= 4 && s_mazePathLength > 0) {
    int traveler = s_mazePath[s_mazeTravelIndex];
    drawMazePixel(display, traveler % s_mazeDim, traveler / s_mazeDim, 255, 220, 120);
  }
}

void clearTetrisBoard() {
  memset(s_tetrisBoard, 0, sizeof(s_tetrisBoard));
}

void getPieceCells(int piece, int rotation, Point out[4]) {
  uint16_t bits = kTetrisPieces[piece][rotation];
  int count = 0;
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (bits & (0x8000 >> (y * 4 + x))) {
        out[count++] = {(int16_t)x, (int16_t)y};
      }
    }
  }
}

bool canPlaceTetrisPiece(int piece, int rotation, int px, int py) {
  Point cells[4];
  getPieceCells(piece, rotation, cells);
  for (int i = 0; i < 4; i++) {
    int x = px + cells[i].x;
    int y = py + cells[i].y;
    if (x < 0 || x >= kTetrisCols || y < 0 || y >= kTetrisRows) {
      return false;
    }
    if (s_tetrisBoard[y][x] != 0) {
      return false;
    }
  }
  return true;
}

int evaluateTetrisBoard() {
  int columnHeights[kTetrisCols] = {};
  int holes = 0;
  for (int x = 0; x < kTetrisCols; x++) {
    bool found = false;
    for (int y = 0; y < kTetrisRows; y++) {
      if (s_tetrisBoard[y][x] != 0) {
        if (!found) {
          columnHeights[x] = kTetrisRows - y;
          found = true;
        }
      } else if (found) {
        holes += 1;
      }
    }
  }

  int aggregateHeight = 0;
  int bumpiness = 0;
  for (int x = 0; x < kTetrisCols; x++) {
    aggregateHeight += columnHeights[x];
    if (x > 0) {
      bumpiness += abs(columnHeights[x] - columnHeights[x - 1]);
    }
  }

  return -(aggregateHeight * 5 + holes * 28 + bumpiness * 7);
}

void clearCompletedTetrisLines() {
  for (int y = kTetrisRows - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < kTetrisCols; x++) {
      if (s_tetrisBoard[y][x] == 0) {
        full = false;
        break;
      }
    }
    if (!full) {
      continue;
    }
    for (int yy = y; yy > 0; yy--) {
      memcpy(s_tetrisBoard[yy], s_tetrisBoard[yy - 1], sizeof(s_tetrisBoard[yy]));
    }
    memset(s_tetrisBoard[0], 0, sizeof(s_tetrisBoard[0]));
    y += 1;
  }
}

void chooseTetrisTarget() {
  int bestScore = -32768;
  int bestRotation = 0;
  int bestX = 3;
  uint8_t snapshot[kTetrisRows][kTetrisCols];
  memcpy(snapshot, s_tetrisBoard, sizeof(snapshot));

  for (int rotation = 0; rotation < 4; rotation++) {
    for (int x = -2; x < kTetrisCols; x++) {
      memcpy(s_tetrisBoard, snapshot, sizeof(snapshot));
      int y = 0;
      while (canPlaceTetrisPiece(s_tetrisPiece, rotation, x, y)) {
        y += 1;
      }
      y -= 1;
      if (y < 0 || !canPlaceTetrisPiece(s_tetrisPiece, rotation, x, y)) {
        continue;
      }

      Point cells[4];
      getPieceCells(s_tetrisPiece, rotation, cells);
      for (int i = 0; i < 4; i++) {
        int cx = x + cells[i].x;
        int cy = y + cells[i].y;
        s_tetrisBoard[cy][cx] = (uint8_t)(s_tetrisPiece + 1);
      }

      int lineCount = 0;
      for (int row = 0; row < kTetrisRows; row++) {
        bool full = true;
        for (int col = 0; col < kTetrisCols; col++) {
          if (s_tetrisBoard[row][col] == 0) {
            full = false;
            break;
          }
        }
        if (full) {
          lineCount += 1;
        }
      }

      int score = evaluateTetrisBoard() + lineCount * 800;
      if (score > bestScore) {
        bestScore = score;
        bestRotation = rotation;
        bestX = x;
      }
    }
  }

  memcpy(s_tetrisBoard, snapshot, sizeof(snapshot));
  s_tetrisTargetRotation = bestRotation;
  s_tetrisTargetX = bestX;
}

void spawnTetrisPiece() {
  s_tetrisPiece = (millis() / 157UL) % 7;
  s_tetrisRotation = 0;
  s_tetrisX = 3;
  s_tetrisY = 0;
  chooseTetrisTarget();
  if (!canPlaceTetrisPiece(s_tetrisPiece, s_tetrisRotation, s_tetrisX, s_tetrisY)) {
    s_tetrisGameOver = true;
    s_tetrisHoldTicks = 0;
  }
}

void drawTetrisBlock(MatrixPanel_I2S_DMA* display, int cellX, int cellY, const uint8_t* color) {
  if (display == nullptr || color == nullptr) {
    return;
  }

  int pixelX = s_tetrisOriginX + cellX * s_tetrisCellSize;
  int pixelY = s_tetrisOriginY + cellY * s_tetrisCellSize;
  fillBlock(display, pixelX, pixelY, s_tetrisCellSize, color[0], color[1], color[2]);

  if (s_tetrisCellSize <= 1) {
    return;
  }

  uint16_t highlight = rgb565(
    (uint8_t)min(255, color[0] + 42),
    (uint8_t)min(255, color[1] + 42),
    (uint8_t)min(255, color[2] + 42)
  );
  uint16_t shadow = rgb565(color[0] / 2, color[1] / 2, color[2] / 2);

  display->drawFastHLine(pixelX, pixelY, s_tetrisCellSize, highlight);
  display->drawFastVLine(pixelX, pixelY, s_tetrisCellSize, highlight);
  display->drawFastHLine(pixelX, pixelY + s_tetrisCellSize - 1, s_tetrisCellSize, shadow);
  display->drawFastVLine(pixelX + s_tetrisCellSize - 1, pixelY, s_tetrisCellSize, shadow);
}

void drawTetrisTimeOverlay(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr || !s_config.showClock) {
    return;
  }

  struct tm timeinfo;
  char timeText[6];
  if (getLocalTime(&timeinfo, 0)) {
    snprintf(timeText, sizeof(timeText), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  } else {
    memcpy(timeText, "--:--", sizeof(timeText));
  }

  int textScale = s_tetrisCellSize >= 3 ? 2 : 1;
  int textWidth = getClockTextWidth(timeText, CLOCK_FONT_MINIMAL_3X5, textScale);
  int textHeight = getClockTextHeight(CLOCK_FONT_MINIMAL_3X5, textScale);
  int panelWidth = textWidth + 6;
  int panelHeight = textHeight + 4;
  int panelX = (kPanelSize - panelWidth) / 2;
  int panelY = (kPanelSize - panelHeight) / 2;

  display->fillRect(panelX, panelY, panelWidth, panelHeight, kTetrisTimePanelColor);
  display->drawRect(panelX, panelY, panelWidth, panelHeight, kTetrisTimeBorderColor);
  drawClockText(
    display,
    timeText,
    panelX + 3,
    panelY + 2,
    kTetrisTimeColor,
    CLOCK_FONT_MINIMAL_3X5,
    textScale
  );
}

void initTetrisGame() {
  clearTetrisBoard();
  s_tetrisCellSize = clampInt((int)s_config.cellSize, 1, 3);
  s_tetrisOriginX = (kPanelSize - kTetrisCols * s_tetrisCellSize) / 2;
  s_tetrisOriginY = (kPanelSize - kTetrisRows * s_tetrisCellSize) / 2;
  s_tetrisGameOver = false;
  s_tetrisHoldTicks = 0;
  spawnTetrisPiece();
  s_needsRender = true;
}

void lockTetrisPiece() {
  Point cells[4];
  getPieceCells(s_tetrisPiece, s_tetrisRotation, cells);
  for (int i = 0; i < 4; i++) {
    int x = s_tetrisX + cells[i].x;
    int y = s_tetrisY + cells[i].y;
    if (x >= 0 && x < kTetrisCols && y >= 0 && y < kTetrisRows) {
      s_tetrisBoard[y][x] = (uint8_t)(s_tetrisPiece + 1);
    }
  }
  clearCompletedTetrisLines();
  spawnTetrisPiece();
}

void updateTetrisGame() {
  if (s_tetrisGameOver) {
    s_tetrisHoldTicks += 1;
    if (s_tetrisHoldTicks >= 18) {
      initTetrisGame();
    }
    s_needsRender = true;
    return;
  }

  if (s_tetrisRotation != s_tetrisTargetRotation) {
    int nextRotation = (s_tetrisRotation + 1) % 4;
    if (canPlaceTetrisPiece(s_tetrisPiece, nextRotation, s_tetrisX, s_tetrisY)) {
      s_tetrisRotation = nextRotation;
      s_needsRender = true;
      return;
    }
    s_tetrisTargetRotation = s_tetrisRotation;
  }

  if (s_tetrisX < s_tetrisTargetX && canPlaceTetrisPiece(s_tetrisPiece, s_tetrisRotation, s_tetrisX + 1, s_tetrisY)) {
    s_tetrisX += 1;
    s_needsRender = true;
    return;
  }
  if (s_tetrisX > s_tetrisTargetX && canPlaceTetrisPiece(s_tetrisPiece, s_tetrisRotation, s_tetrisX - 1, s_tetrisY)) {
    s_tetrisX -= 1;
    s_needsRender = true;
    return;
  }

  if (canPlaceTetrisPiece(s_tetrisPiece, s_tetrisRotation, s_tetrisX, s_tetrisY + 1)) {
    s_tetrisY += 1;
  } else {
    lockTetrisPiece();
  }
  s_needsRender = true;
}

void renderTetrisGame(MatrixPanel_I2S_DMA* display) {
  if (display == nullptr) {
    return;
  }

  int boardWidth = kTetrisCols * s_tetrisCellSize;
  int boardHeight = kTetrisRows * s_tetrisCellSize;
  display->drawRect(
    s_tetrisOriginX - 1,
    s_tetrisOriginY - 1,
    boardWidth + 2,
    boardHeight + 2,
    rgb565(42, 52, 72)
  );
  for (int y = 0; y < kTetrisRows; y++) {
    for (int x = 0; x < kTetrisCols; x++) {
      uint8_t cell = s_tetrisBoard[y][x];
      if (cell == 0) {
        continue;
      }
      const uint8_t* color = kTetrisColors[cell - 1];
      drawTetrisBlock(display, x, y, color);
    }
  }

  if (!s_tetrisGameOver) {
    Point cells[4];
    getPieceCells(s_tetrisPiece, s_tetrisRotation, cells);
    const uint8_t* color = kTetrisColors[s_tetrisPiece];
    for (int i = 0; i < 4; i++) {
      drawTetrisBlock(display, s_tetrisX + cells[i].x, s_tetrisY + cells[i].y, color);
    }
  }

  drawTetrisTimeOverlay(display);

  if (s_tetrisGameOver) {
    display->fillRect(16, 24, 32, 16, rgb565(48, 16, 16));
    DisplayManager::drawTinyTextCentered("RE", 28, rgb565(255, 120, 120), 2);
  }
}

void resetForCurrentGame() {
  if (s_config.game == GAME_SCREENSAVER_SNAKE) {
    releaseMazeBuffers();
    initSnake();
    return;
  }
  if (s_config.game == GAME_SCREENSAVER_PING_PONG) {
    releaseSnakeBuffers();
    releaseMazeBuffers();
    initPingPong();
    return;
  }
  if (s_config.game == GAME_SCREENSAVER_TETRIS_GAME) {
    releaseSnakeBuffers();
    releaseMazeBuffers();
    initTetrisGame();
    return;
  }
  releaseSnakeBuffers();
  initMaze();
}
}

namespace GameScreensaverEffect {
void init() {
  s_lastTickAt = millis();
  resetForCurrentGame();
  s_active = false;
}

void applyConfig(const GameScreensaverConfig& config) {
  s_config = config;
  s_active = true;
  s_lastTickAt = 0;
  resetForCurrentGame();
}

void update() {
  if (!s_active) {
    return;
  }

  unsigned long now = millis();
  unsigned long interval = 120;
  if (s_config.game == GAME_SCREENSAVER_SNAKE) {
    interval = resolveTickInterval(s_config.speed, 180, 55);
  } else if (s_config.game == GAME_SCREENSAVER_PING_PONG) {
    interval = resolveTickInterval(s_config.speed, 120, 28);
  } else if (s_config.game == GAME_SCREENSAVER_TETRIS_GAME) {
    interval = resolveTickInterval(s_config.speed, 150, 42);
  } else {
    interval = resolveTickInterval(s_config.speed, 130, 38);
  }

  if (s_lastTickAt != 0 && now - s_lastTickAt < interval) {
    return;
  }
  s_lastTickAt = now;

  if (s_config.game == GAME_SCREENSAVER_SNAKE) {
    updateSnake();
    return;
  }
  if (s_config.game == GAME_SCREENSAVER_PING_PONG) {
    updatePingPong();
    return;
  }
  if (s_config.game == GAME_SCREENSAVER_TETRIS_GAME) {
    updateTetrisGame();
    return;
  }
  updateMaze();
}

void render() {
  if (!s_active) {
    return;
  }

  if (!s_needsRender) {
    return;
  }

  uint16_t* frameBuffer = &DisplayManager::animationBuffer[0][0];
  MatrixPanel_I2S_DMA* offscreen = DisplayManager::beginOffscreenFrame(frameBuffer, 0);
  if (offscreen == nullptr) {
    return;
  }

  if (s_config.game == GAME_SCREENSAVER_SNAKE) {
    renderSnake(offscreen);
  } else if (s_config.game == GAME_SCREENSAVER_PING_PONG) {
    renderPingPong(offscreen);
  } else if (s_config.game == GAME_SCREENSAVER_TETRIS_GAME) {
    renderTetrisGame(offscreen);
  } else {
    renderMaze(offscreen);
  }

  DisplayManager::presentOffscreenFrame(frameBuffer);
  s_needsRender = false;
}

bool isActive() {
  return s_active;
}

const GameScreensaverConfig& getConfig() {
  return s_config;
}
}
