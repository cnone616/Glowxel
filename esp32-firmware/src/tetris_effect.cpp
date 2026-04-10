#include "tetris_effect.h"
#include "display_manager.h"
#include <time.h>

// 静态成员初始化
bool TetrisEffect::isActive = false;
bool TetrisEffect::doClearLines = true;
int TetrisEffect::cellSize = 2;
int TetrisEffect::dropSpeed = 150;
bool TetrisEffect::showClock = true;
uint8_t TetrisEffect::piecesMask = 0x7F;
uint8_t (*TetrisEffect::board)[TETRIS_MAX] = nullptr;
int TetrisEffect::cols = 32;
int TetrisEffect::rows = 32;
int TetrisEffect::curType = 0;
int TetrisEffect::curRot = 0;
int TetrisEffect::curX = 0;
int TetrisEffect::curY = 0;
int TetrisEffect::targetX = 0;
uint8_t TetrisEffect::curColor = 1;
unsigned long TetrisEffect::lastDropTime = 0;
bool TetrisEffect::needsRender = true;
uint8_t (*TetrisEffect::prevDisplay)[TETRIS_MAX] = nullptr;
uint8_t (*TetrisEffect::targetMap)[TETRIS_MAX / 8] = nullptr;
int TetrisEffect::lastClockMinute = -1;
int TetrisEffect::lastClockHour = -1;
bool TetrisEffect::holdClockFrame = false;
unsigned long TetrisEffect::spawnCounter = 0;
bool TetrisEffect::spawnBiasLeft = true;
unsigned long TetrisEffect::holdPulseAt = 0;
bool TetrisEffect::holdPulseBright = false;

namespace {
constexpr size_t kTetrisBoardBytes = TETRIS_MAX * TETRIS_MAX * sizeof(uint8_t);
constexpr size_t kTetrisTargetMapBytes = TETRIS_MAX * (TETRIS_MAX / 8) * sizeof(uint8_t);
}

// 7种方块的4种旋转，用16bit编码4x4网格（从左上到右下，行优先）
const uint16_t TetrisEffect::pieces[7][4] = {
  // I
  { 0x0F00, 0x2222, 0x00F0, 0x4444 },
  // O
  { 0x6600, 0x6600, 0x6600, 0x6600 },
  // T
  { 0x0E40, 0x4C40, 0x4E00, 0x4640 },
  // S
  { 0x06C0, 0x8C40, 0x6C00, 0x4620 },
  // Z
  { 0x0C60, 0x4C80, 0xC600, 0x2640 },
  // J
  { 0x0E80, 0xC440, 0x2E00, 0x44C0 },
  // L
  { 0x0E20, 0x44C0, 0x8E00, 0xC880 }
};

// 方块颜色 (柔和的霓虹色)
const uint8_t TetrisEffect::colors[7][3] = {
  { 0, 240, 240 },   // I - 青色
  { 240, 240, 0 },   // O - 黄色
  { 160, 0, 240 },   // T - 紫色
  { 0, 240, 0 },     // S - 绿色
  { 240, 0, 0 },     // Z - 红色
  { 0, 0, 240 },     // J - 蓝色
  { 240, 160, 0 }    // L - 橙色
};

bool TetrisEffect::ensureBuffers() {
  if (board == nullptr) {
    board = static_cast<uint8_t (*)[TETRIS_MAX]>(malloc(kTetrisBoardBytes));
    if (board == nullptr) {
      return false;
    }
  }

  if (prevDisplay == nullptr) {
    prevDisplay = static_cast<uint8_t (*)[TETRIS_MAX]>(malloc(kTetrisBoardBytes));
    if (prevDisplay == nullptr) {
      free(board);
      board = nullptr;
      return false;
    }
  }

  if (targetMap == nullptr) {
    targetMap = static_cast<uint8_t (*)[TETRIS_MAX / 8]>(malloc(kTetrisTargetMapBytes));
    if (targetMap == nullptr) {
      free(prevDisplay);
      prevDisplay = nullptr;
      free(board);
      board = nullptr;
      return false;
    }
  }

  return true;
}

void TetrisEffect::init(bool clearMode, int cellSz, int speed, bool clock, uint8_t mask) {
  if (!ensureBuffers()) {
    isActive = false;
    return;
  }

  doClearLines = clearMode;
  cellSize = constrain(cellSz, 1, 3);
  dropSpeed = constrain(speed, 30, 1000);
  showClock = clock;
  piecesMask = mask ? mask : 0x7F; // 至少一种
  cols = 64 / cellSize;
  rows = 64 / cellSize;
  resetBoard();
  memset(targetMap, 0, kTetrisTargetMapBytes);
  memset(prevDisplay, 0, kTetrisBoardBytes);
  lastClockMinute = -1;
  lastClockHour = -1;
  holdClockFrame = false;
  spawnCounter = 0;
  spawnBiasLeft = true;
  holdPulseAt = 0;
  holdPulseBright = false;
  if (showClock) {
    rebuildClockTarget();
  }
  spawnPiece();
  lastDropTime = millis();
  needsRender = true;
  isActive = true;
}

void TetrisEffect::resetBoard() {
  if (board == nullptr) {
    return;
  }
  memset(board, 0, kTetrisBoardBytes);
}

bool TetrisEffect::getTargetCell(int x, int y) {
  if (x < 0 || x >= TETRIS_MAX || y < 0 || y >= TETRIS_MAX) {
    return false;
  }
  return (targetMap[y][x / 8] & (1 << (x % 8))) != 0;
}

void TetrisEffect::setTargetCell(int x, int y) {
  if (x < 0 || x >= TETRIS_MAX || y < 0 || y >= TETRIS_MAX) {
    return;
  }
  targetMap[y][x / 8] |= (1 << (x % 8));
}

// 从16bit编码中提取4x4格子中的4个有效格子坐标
void TetrisEffect::getPieceCells(int type, int rot, int cells[4][2]) {
  uint16_t mask = pieces[type][rot];
  int idx = 0;
  for (int r = 0; r < 4 && idx < 4; r++) {
    for (int c = 0; c < 4 && idx < 4; c++) {
      if (mask & (0x8000 >> (r * 4 + c))) {
        cells[idx][0] = c; // x
        cells[idx][1] = r; // y
        idx++;
      }
    }
  }
}

int TetrisEffect::pieceMinX(int type, int rot) {
  int cells[4][2];
  getPieceCells(type, rot, cells);
  int minValue = 4;
  for (int i = 0; i < 4; i++) {
    if (cells[i][0] < minValue) {
      minValue = cells[i][0];
    }
  }
  return minValue;
}

int TetrisEffect::pieceMaxX(int type, int rot) {
  int cells[4][2];
  getPieceCells(type, rot, cells);
  int maxValue = 0;
  for (int i = 0; i < 4; i++) {
    if (cells[i][0] > maxValue) {
      maxValue = cells[i][0];
    }
  }
  return maxValue;
}

uint32_t TetrisEffect::buildSequenceSeed() {
  int minuteValue = lastClockHour * 60 + lastClockMinute;
  if (minuteValue < 0) {
    minuteValue = 0;
  }

  uint32_t seed = static_cast<uint32_t>(minuteValue);
  seed = seed * 131U + static_cast<uint32_t>(cellSize * 17);
  seed = seed * 131U + static_cast<uint32_t>(dropSpeed);
  seed = seed * 131U + static_cast<uint32_t>(showClock ? 1 : 0);
  seed = seed * 131U + static_cast<uint32_t>(doClearLines ? 1 : 0);
  seed = seed * 131U + static_cast<uint32_t>(piecesMask);
  return seed;
}

int TetrisEffect::computeSpawnDrift(int type) {
  uint32_t seed = buildSequenceSeed();
  uint32_t mixed = seed + spawnCounter * 29U + static_cast<uint32_t>(type) * 41U;
  return 4 + static_cast<int>(mixed % 5U);
}

int TetrisEffect::computeSpawnX(int type, int rot, int finalX) {
  const int minOffset = pieceMinX(type, rot);
  const int maxOffset = pieceMaxX(type, rot);
  const int minSpawnX = -minOffset;
  const int maxSpawnX = cols - 1 - maxOffset;
  int drift = computeSpawnDrift(type);
  int startX = finalX + (spawnBiasLeft ? -drift : drift);
  spawnBiasLeft = !spawnBiasLeft;
  if (startX < minSpawnX) {
    startX = minSpawnX;
  }
  if (startX > maxSpawnX) {
    startX = maxSpawnX;
  }
  return startX;
}

bool TetrisEffect::canMove(int type, int rot, int x, int y) {
  int cells[4][2];
  getPieceCells(type, rot, cells);
  for (int i = 0; i < 4; i++) {
    int nx = x + cells[i][0];
    int ny = y + cells[i][1];
    if (nx < 0 || nx >= cols || ny >= rows) return false;
    if (ny >= 0 && board[ny][nx] != 0) return false;
  }
  return true;
}

void TetrisEffect::spawnPiece() {
  // 从启用的方块中随机选一种
  int enabled[7], count = 0;
  for (int i = 0; i < 7; i++) {
    if (piecesMask & (1 << i)) enabled[count++] = i;
  }
  if (count == 0) { enabled[0] = 0; count = 1; }
  uint32_t seed = buildSequenceSeed();
  uint32_t pieceIndex = (seed + spawnCounter * 73U + spawnCounter * spawnCounter * 11U) % static_cast<uint32_t>(count);
  curType = enabled[pieceIndex];
  curColor = curType + 1;

  if (showClock) {
    int clockRot = 0;
    int clockX = 0;
    if (buildClockPlacement(clockRot, clockX)) {
      holdClockFrame = false;
      curRot = clockRot;
      targetX = clockX;
      curX = computeSpawnX(curType, curRot, targetX);
      if (!canMove(curType, curRot, curX, -2)) {
        curX = targetX;
      }
      curY = -2;
      spawnCounter++;
      needsRender = true;
      return;
    }
    holdClockFrame = true;
    curRot = 0;
    curX = 0;
    curY = -100;
    needsRender = true;
    return;
  }

  // AI 选位：遍历所有旋转和水平位置，找最佳放置点
  int bestRot = 0, bestX = 0;
  int bestScore = -99999;

  for (int rot = 0; rot < 4; rot++) {
    for (int x = -2; x < cols; x++) {
      // 从顶部往下模拟掉落
      int y = -2;
      if (!canMove(curType, rot, x, y)) continue;
      while (canMove(curType, rot, x, y + 1)) y++;

      // 模拟放置，计算分数
      // 临时锁定
      int cells[4][2];
      getPieceCells(curType, rot, cells);
      bool valid = true;
      for (int i = 0; i < 4; i++) {
        int ny = y + cells[i][1];
        if (ny < 0) { valid = false; break; }
      }
      if (!valid) continue;

      // 临时写入
      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        if (ny >= 0 && ny < rows && nx >= 0 && nx < cols)
          board[ny][nx] = curColor;
      }

      // 评分：尽量填满每一行，不留空洞
      int score = 0;

      // 1. 放得越低越好
      score += y * 20;

      // 2. 完整行大加分
      for (int row = 0; row < rows; row++) {
        bool full = true;
        int filled = 0;
        for (int col = 0; col < cols; col++) {
          if (board[row][col] != 0) filled++;
          else full = false;
        }
        if (full) score += 5000;
        // 接近满行也加分（鼓励填满）
        score += filled * 2;
      }

      // 3. 空洞重罚（上面有方块但自己是空的）
      for (int col = 0; col < cols; col++) {
        bool blocked = false;
        for (int row = 0; row < rows; row++) {
          if (board[row][col] != 0) blocked = true;
          else if (blocked) score -= 500;
        }
      }

      // 4. 紧贴已有方块或墙壁加分
      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        if (ny + 1 >= rows) score += 10;
        else if (ny + 1 >= 0 && board[ny + 1][nx] != 0 && board[ny + 1][nx] != curColor) score += 10;
        if (nx - 1 < 0 || (board[ny][nx - 1] != 0 && board[ny][nx - 1] != curColor)) score += 5;
        if (nx + 1 >= cols || (board[ny][nx + 1] != 0 && board[ny][nx + 1] != curColor)) score += 5;
      }

      // 5. 高度差惩罚（表面越平越好）
      int maxH[TETRIS_MAX];
      for (int col = 0; col < cols; col++) {
        maxH[col] = rows;
        for (int row = 0; row < rows; row++) {
          if (board[row][col] != 0) { maxH[col] = row; break; }
        }
      }
      for (int col = 1; col < cols; col++) {
        int diff = abs(maxH[col] - maxH[col - 1]);
        score -= diff * 30;
      }

      // 撤销临时写入
      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        if (ny >= 0 && ny < rows && nx >= 0 && nx < cols)
          board[ny][nx] = 0;
      }

      if (score > bestScore) {
        bestScore = score;
        bestRot = rot;
        bestX = x;
      }
    }
  }

  curRot = bestRot;
  targetX = bestX;
  curX = computeSpawnX(curType, curRot, targetX);
  if (!canMove(curType, curRot, curX, -2)) {
    curX = targetX;
  }
  curY = -2;
  spawnCounter++;
  needsRender = true;
}

void TetrisEffect::rebuildClockTarget() {
  if (targetMap == nullptr) {
    return;
  }
  memset(targetMap, 0, kTetrisTargetMapBytes);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) {
    return;
  }

  lastClockHour = timeinfo.tm_hour;
  lastClockMinute = timeinfo.tm_min;

  static const uint8_t digitPatterns[10][5][3] = {
    {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}},
    {{0,1,0},{1,1,0},{0,1,0},{0,1,0},{1,1,1}},
    {{1,1,1},{0,0,1},{1,1,1},{1,0,0},{1,1,1}},
    {{1,1,1},{0,0,1},{0,1,1},{0,0,1},{1,1,1}},
    {{1,0,1},{1,0,1},{1,1,1},{0,0,1},{0,0,1}},
    {{1,1,1},{1,0,0},{1,1,1},{0,0,1},{1,1,1}},
    {{1,1,1},{1,0,0},{1,1,1},{1,0,1},{1,1,1}},
    {{1,1,1},{0,0,1},{0,1,0},{0,1,0},{0,1,0}},
    {{1,1,1},{1,0,1},{1,1,1},{1,0,1},{1,1,1}},
    {{1,1,1},{1,0,1},{1,1,1},{0,0,1},{1,1,1}},
  };

  const int glyphUnits = 17;
  int scale = 1;
  for (int tryScale = 1; tryScale <= 6; tryScale++) {
    if (glyphUnits * tryScale <= cols - 2 && 5 * tryScale <= rows - 2) {
      scale = tryScale;
    }
  }

  const int totalWidth = glyphUnits * scale;
  const int totalHeight = 5 * scale;
  const int startX = (cols - totalWidth) / 2;
  const int startY = (rows - totalHeight) / 2;
  char timeBuffer[5];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d%02d", timeinfo.tm_hour, timeinfo.tm_min);

  int cursorX = startX;
  for (int digitIndex = 0; digitIndex < 4; digitIndex++) {
    int value = timeBuffer[digitIndex] - '0';
    for (int row = 0; row < 5; row++) {
      for (int col = 0; col < 3; col++) {
        if (digitPatterns[value][row][col] == 0) continue;
        for (int sy = 0; sy < scale; sy++) {
          for (int sx = 0; sx < scale; sx++) {
            int px = cursorX + col * scale + sx;
            int py = startY + row * scale + sy;
            if (px >= 0 && px < cols && py >= 0 && py < rows) {
              setTargetCell(px, py);
            }
          }
        }
      }
    }
    cursorX += 3 * scale + scale;
    if (digitIndex == 1) {
      for (int sy = 0; sy < scale; sy++) {
        int topY = startY + scale + sy;
        int bottomY = startY + scale * 3 + sy;
        for (int sx = 0; sx < scale; sx++) {
          int px = cursorX + sx;
          if (px >= 0 && px < cols) {
            if (topY >= 0 && topY < rows) {
              setTargetCell(px, topY);
            }
            if (bottomY >= 0 && bottomY < rows) {
              setTargetCell(px, bottomY);
            }
          }
        }
      }
      cursorX += scale + scale;
    }
  }
}

bool TetrisEffect::buildClockPlacement(int& outRot, int& outX) {
  int bestRot = 0;
  int bestX = 0;
  int bestScore = -999999;

  for (int rot = 0; rot < 4; rot++) {
    for (int x = -2; x < cols; x++) {
      int y = -2;
      if (!canMove(curType, rot, x, y)) continue;
      while (canMove(curType, rot, x, y + 1)) y++;

      int cells[4][2];
      getPieceCells(curType, rot, cells);
      int score = 0;
      bool valid = true;

      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) {
          valid = false;
          break;
        }
        if (!getTargetCell(nx, ny)) {
          valid = false;
          break;
        }
        score += 2000;
        if (ny + 1 >= rows || board[ny + 1][nx] != 0 || !getTargetCell(nx, ny + 1)) {
          score += 40;
        }
        if (nx > 0 && board[ny][nx - 1] != 0) score += 18;
        if (nx + 1 < cols && board[ny][nx + 1] != 0) score += 18;
      }
      if (!valid) {
        continue;
      }

      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        board[ny][nx] = curColor;
      }

      int holes = 0;
      for (int col = 0; col < cols; col++) {
        bool seenTargetBlock = false;
        for (int row = 0; row < rows; row++) {
          if (getTargetCell(col, row) && board[row][col] != 0) {
            seenTargetBlock = true;
          } else if (seenTargetBlock && getTargetCell(col, row) && board[row][col] == 0) {
            holes++;
          }
        }
      }
      score -= holes * 180;

      for (int row = 0; row < rows; row++) {
        int filledInTarget = 0;
        int targetCount = 0;
        for (int col = 0; col < cols; col++) {
          if (getTargetCell(col, row)) {
            targetCount++;
            if (board[row][col] != 0) {
              filledInTarget++;
            }
          }
        }
        score += filledInTarget * 8;
        if (targetCount > 0 && filledInTarget == targetCount) {
          score += 120;
        }
      }

      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        board[ny][nx] = 0;
      }

      score += y * 4;

      if (score > bestScore) {
        bestScore = score;
        bestRot = rot;
        bestX = x;
      }
    }
  }

  if (bestScore <= -999999) {
    return false;
  }

  outRot = bestRot;
  outX = bestX;
  return true;
}

void TetrisEffect::lockPiece() {
  int cells[4][2];
  getPieceCells(curType, curRot, cells);
  for (int i = 0; i < 4; i++) {
    int nx = curX + cells[i][0];
    int ny = curY + cells[i][1];
    if (ny >= 0 && ny < rows && nx >= 0 && nx < cols) {
      board[ny][nx] = curColor;
    }
  }
}

int TetrisEffect::clearLines() {
  int cleared = 0;
  for (int y = rows - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < cols; x++) {
      if (board[y][x] == 0) { full = false; break; }
    }
    if (full) {
      // 整行下移
      for (int yy = y; yy > 0; yy--) {
        memcpy(board[yy], board[yy - 1], cols);
      }
      memset(board[0], 0, cols);
      cleared++;
      y++; // 重新检查当前行
    }
  }
  return cleared;
}

void TetrisEffect::update() {
  if (!isActive) return;
  if (board == nullptr || prevDisplay == nullptr || targetMap == nullptr) return;
  unsigned long now = millis();

  if (showClock) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
      if (timeinfo.tm_min != lastClockMinute || timeinfo.tm_hour != lastClockHour) {
        resetBoard();
        memset(prevDisplay, 0, kTetrisBoardBytes);
        rebuildClockTarget();
        holdClockFrame = false;
        spawnCounter = 0;
        spawnBiasLeft = true;
        spawnPiece();
        needsRender = true;
      }
    }
    if (holdClockFrame) {
      if (now - holdPulseAt >= 220) {
        holdPulseAt = now;
        holdPulseBright = !holdPulseBright;
        needsRender = true;
      }
      return;
    }
  }
  if (now - lastDropTime < (unsigned long)dropSpeed) return;
  lastDropTime = now;

  if (curX != targetX) {
    int step = curX < targetX ? 1 : -1;
    if (canMove(curType, curRot, curX + step, curY)) {
      curX += step;
      needsRender = true;
    } else {
      targetX = curX;
    }
  }

  // 尝试下落
  if (canMove(curType, curRot, curX, curY + 1)) {
    curY++;
    needsRender = true;
  } else {
    // 贴合锁定
    lockPiece();

    if (doClearLines && !showClock) {
      // 消除模式：填满一行就消除
      clearLines();
    }

    // 检查是否堆到顶部（前2行有方块就重置）
    bool topFull = false;
    for (int x = 0; x < cols; x++) {
      if (board[0][x] != 0 || board[1][x] != 0) {
        topFull = true;
        break;
      }
    }
    if (topFull) {
      resetBoard();
    }

    spawnPiece();
    needsRender = true;
  }
}

void TetrisEffect::render(MatrixPanel_I2S_DMA* display) {
  if (!isActive || !needsRender) return;
  if (board == nullptr || prevDisplay == nullptr) return;
  needsRender = false;

  // 构建当前帧的合成状态（board + 当前下落方块）
  uint8_t curDisplay[TETRIS_MAX][TETRIS_MAX];
  memcpy(curDisplay, board, kTetrisBoardBytes);

  int cells[4][2];
  getPieceCells(curType, curRot, cells);
  for (int i = 0; i < 4; i++) {
    int px = curX + cells[i][0];
    int py = curY + cells[i][1];
    if (px >= 0 && px < cols && py >= 0 && py < rows) {
      curDisplay[py][px] = curColor; // 和 lockPiece 一致
    }
  }

  if (showClock) {
    for (int y = 0; y < rows; y++) {
      for (int x = 0; x < cols; x++) {
        if (curDisplay[y][x] == 0 && getTargetCell(x, y)) {
          curDisplay[y][x] = 8;
        }
      }
    }
  }

  // 增量渲染：只重绘变化的格子
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      if (curDisplay[y][x] == prevDisplay[y][x]) continue;

      uint8_t c = curDisplay[y][x];
      if (c > 0 && c <= 7) {
        const uint8_t* rgb = colors[c - 1];
        uint8_t baseBoost = (showClock && holdClockFrame && holdPulseBright) ? 20 : 0;
        uint8_t highlightBoost = (showClock && holdClockFrame && holdPulseBright) ? 60 : 40;
        uint8_t baseR = min(255, rgb[0] + baseBoost);
        uint8_t baseG = min(255, rgb[1] + baseBoost);
        uint8_t baseB = min(255, rgb[2] + baseBoost);
        uint8_t highlightR = min(255, rgb[0] + highlightBoost);
        uint8_t highlightG = min(255, rgb[1] + highlightBoost);
        uint8_t highlightB = min(255, rgb[2] + highlightBoost);
        uint8_t shadowR = rgb[0] / 2;
        uint8_t shadowG = rgb[1] / 2;
        uint8_t shadowB = rgb[2] / 2;
        for (int dy = 0; dy < cellSize; dy++) {
          for (int dx = 0; dx < cellSize; dx++) {
            uint8_t drawR = baseR;
            uint8_t drawG = baseG;
            uint8_t drawB = baseB;

            if (cellSize >= 2) {
              if (dy == 0 || dx == 0) {
                drawR = highlightR;
                drawG = highlightG;
                drawB = highlightB;
              } else if (dy == cellSize - 1 || dx == cellSize - 1) {
                drawR = shadowR;
                drawG = shadowG;
                drawB = shadowB;
              }
            }

            display->drawPixelRGB888(x * cellSize + dx, y * cellSize + dy, drawR, drawG, drawB);
          }
        }
      } else if (c == 8) {
        for (int dy = 0; dy < cellSize; dy++) {
          for (int dx = 0; dx < cellSize; dx++) {
            uint8_t drawR = 16;
            uint8_t drawG = 32;
            uint8_t drawB = 56;
            if (cellSize >= 2 && (dy == 0 || dx == 0)) {
              drawR = 28;
              drawG = 58;
              drawB = 92;
            }
            display->drawPixelRGB888(x * cellSize + dx, y * cellSize + dy, drawR, drawG, drawB);
          }
        }
      } else {
        for (int dy = 0; dy < cellSize; dy++) {
          for (int dx = 0; dx < cellSize; dx++) {
            display->drawPixelRGB888(x * cellSize + dx, y * cellSize + dy, 0, 0, 0);
          }
        }
      }
    }
  }

  // 保存当前帧状态
  memcpy(prevDisplay, curDisplay, kTetrisBoardBytes);

  // 时钟在方块上层叠加
}
