#include "tetris_effect.h"
#include "display_manager.h"

bool TetrisEffect::isActive = false;
bool TetrisEffect::doClearLines = true;
int TetrisEffect::cellSize = 2;
int TetrisEffect::dropSpeed = 150;
bool TetrisEffect::showClock = true;
uint8_t TetrisEffect::piecesMask = 0x7F;
uint8_t TetrisEffect::board[TETRIS_MAX][TETRIS_MAX];
int TetrisEffect::cols = 32;
int TetrisEffect::rows = 32;
int TetrisEffect::curType = 0;
int TetrisEffect::curRot = 0;
int TetrisEffect::targetRot = 0;
int TetrisEffect::curX = 0;
int TetrisEffect::curY = 0;
int TetrisEffect::targetX = 0;
uint8_t TetrisEffect::curColor = 1;
unsigned long TetrisEffect::lastDropTime = 0;
bool TetrisEffect::needsRender = true;

const uint16_t TetrisEffect::pieces[7][4] = {
  { 0x0F00, 0x2222, 0x00F0, 0x4444 },
  { 0x6600, 0x6600, 0x6600, 0x6600 },
  { 0x0E40, 0x4C40, 0x4E00, 0x4640 },
  { 0x06C0, 0x8C40, 0x6C00, 0x4620 },
  { 0x0C60, 0x4C80, 0xC600, 0x2640 },
  { 0x0E80, 0xC440, 0x2E00, 0x44C0 },
  { 0x0E20, 0x44C0, 0x8E00, 0xC880 }
};

const uint8_t TetrisEffect::colors[7][3] = {
  { 0, 240, 240 },
  { 240, 240, 0 },
  { 160, 0, 240 },
  { 0, 240, 0 },
  { 240, 0, 0 },
  { 0, 0, 240 },
  { 240, 160, 0 }
};

void TetrisEffect::init(bool clearMode, int cellSz, int speed, bool clock, uint8_t mask) {
  doClearLines = clearMode;
  cellSize = constrain(cellSz, 1, 3);
  dropSpeed = constrain(speed, 30, 1000);
  showClock = clock;
  piecesMask = mask ? mask : 0x7F;
  cols = 64 / cellSize;
  rows = 64 / cellSize;
  resetBoard();
  spawnPiece();
  lastDropTime = millis();
  needsRender = true;
  isActive = true;
}

void TetrisEffect::deactivate() {
  isActive = false;
  needsRender = false;
}

void TetrisEffect::resetBoard() {
  memset(board, 0, sizeof(board));
}

void TetrisEffect::getPieceCells(int type, int rot, int cells[4][2]) {
  uint16_t mask = pieces[type][rot];
  int idx = 0;
  for (int r = 0; r < 4 && idx < 4; r++) {
    for (int c = 0; c < 4 && idx < 4; c++) {
      if (mask & (0x8000 >> (r * 4 + c))) {
        cells[idx][0] = c;
        cells[idx][1] = r;
        idx++;
      }
    }
  }
}

bool TetrisEffect::canMove(int type, int rot, int x, int y) {
  int cells[4][2];
  getPieceCells(type, rot, cells);
  for (int i = 0; i < 4; i++) {
    int nx = x + cells[i][0];
    int ny = y + cells[i][1];
    if (nx < 0 || nx >= cols || ny >= rows) {
      return false;
    }
    if (ny >= 0 && board[ny][nx] != 0) {
      return false;
    }
  }
  return true;
}

void TetrisEffect::spawnPiece() {
  int enabled[7];
  int count = 0;
  for (int i = 0; i < 7; i++) {
    if (piecesMask & (1 << i)) {
      enabled[count++] = i;
    }
  }
  if (count == 0) {
    enabled[0] = 0;
    count = 1;
  }

  curType = enabled[random(count)];
  curColor = curType + 1;

  int bestRot = 0;
  int bestX = 0;
  int bestScore = -99999;

  for (int rot = 0; rot < 4; rot++) {
    for (int x = -2; x < cols; x++) {
      int y = -2;
      if (!canMove(curType, rot, x, y)) {
        continue;
      }
      while (canMove(curType, rot, x, y + 1)) {
        y++;
      }

      int cells[4][2];
      getPieceCells(curType, rot, cells);
      bool valid = true;
      for (int i = 0; i < 4; i++) {
        int ny = y + cells[i][1];
        if (ny < 0) {
          valid = false;
          break;
        }
      }
      if (!valid) {
        continue;
      }

      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        if (ny >= 0 && ny < rows && nx >= 0 && nx < cols) {
          board[ny][nx] = curColor;
        }
      }

      int score = 0;
      score += y * 20;

      for (int row = 0; row < rows; row++) {
        bool full = true;
        int filled = 0;
        for (int col = 0; col < cols; col++) {
          if (board[row][col] != 0) {
            filled++;
          } else {
            full = false;
          }
        }
        if (full) {
          score += 5000;
        }
        score += filled * 2;
      }

      for (int col = 0; col < cols; col++) {
        bool blocked = false;
        for (int row = 0; row < rows; row++) {
          if (board[row][col] != 0) {
            blocked = true;
          } else if (blocked) {
            score -= 500;
          }
        }
      }

      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        if (ny + 1 >= rows) {
          score += 10;
        } else if (ny + 1 >= 0 && board[ny + 1][nx] != 0 && board[ny + 1][nx] != curColor) {
          score += 10;
        }
        if (nx - 1 < 0 || (board[ny][nx - 1] != 0 && board[ny][nx - 1] != curColor)) {
          score += 5;
        }
        if (nx + 1 >= cols || (board[ny][nx + 1] != 0 && board[ny][nx + 1] != curColor)) {
          score += 5;
        }
      }

      int maxH[TETRIS_MAX];
      for (int col = 0; col < cols; col++) {
        maxH[col] = rows;
        for (int row = 0; row < rows; row++) {
          if (board[row][col] != 0) {
            maxH[col] = row;
            break;
          }
        }
      }

      for (int col = 1; col < cols; col++) {
        int diff = abs(maxH[col] - maxH[col - 1]);
        score -= diff * 30;
      }

      for (int i = 0; i < 4; i++) {
        int nx = x + cells[i][0];
        int ny = y + cells[i][1];
        if (ny >= 0 && ny < rows && nx >= 0 && nx < cols) {
          board[ny][nx] = 0;
        }
      }

      if (score > bestScore) {
        bestScore = score;
        bestRot = rot;
        bestX = x;
      }
    }
  }

  targetRot = bestRot;
  targetX = bestX;
  curRot = 0;
  curY = 0;

  const int preferredSpawnX = constrain(cols / 2 - 2, 0, max(0, cols - 1));
  curX = preferredSpawnX;

  if (!canMove(curType, curRot, curX, curY)) {
    bool foundSpawn = false;
    const int maxSpawnX = max(0, cols - 1);
    for (int delta = 1; delta <= cols; delta++) {
      const int leftX = preferredSpawnX - delta;
      if (leftX >= 0 && leftX <= maxSpawnX &&
          canMove(curType, curRot, leftX, curY)) {
        curX = leftX;
        foundSpawn = true;
        break;
      }

      const int rightX = preferredSpawnX + delta;
      if (rightX >= 0 && rightX <= maxSpawnX &&
          canMove(curType, curRot, rightX, curY)) {
        curX = rightX;
        foundSpawn = true;
        break;
      }
    }

    if (!foundSpawn) {
      resetBoard();
      spawnPiece();
      return;
    }
  }

  needsRender = true;
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
      if (board[y][x] == 0) {
        full = false;
        break;
      }
    }
    if (full) {
      for (int yy = y; yy > 0; yy--) {
        memcpy(board[yy], board[yy - 1], cols);
      }
      memset(board[0], 0, cols);
      cleared++;
      y++;
    }
  }
  return cleared;
}

void TetrisEffect::update() {
  if (!isActive) {
    return;
  }

  unsigned long now = millis();
  if (now - lastDropTime < (unsigned long)dropSpeed) {
    return;
  }
  lastDropTime = now;

  if (curRot != targetRot) {
    const int nextRot = (curRot + 1) % 4;
    if (canMove(curType, nextRot, curX, curY)) {
      curRot = nextRot;
      needsRender = true;
      return;
    }
    targetRot = curRot;
  }

  if (curX < targetX && canMove(curType, curRot, curX + 1, curY)) {
    curX++;
    needsRender = true;
    return;
  }

  if (curX > targetX && canMove(curType, curRot, curX - 1, curY)) {
    curX--;
    needsRender = true;
    return;
  }

  if (canMove(curType, curRot, curX, curY + 1)) {
    curY++;
    needsRender = true;
    return;
  }

  lockPiece();

  if (doClearLines) {
    clearLines();
  }

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

void TetrisEffect::render(MatrixPanel_I2S_DMA* display) {
  if (!isActive || !needsRender) {
    return;
  }
  needsRender = false;

  uint16_t* frameBuffer = &DisplayManager::animationBuffer[0][0];
  if (!DisplayManager::beginRedirectedFrame(frameBuffer, 0)) {
    return;
  }

  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      uint8_t c = board[y][x];
      uint16_t blockColor565 = 0;
      if (c > 0) {
        const uint8_t* rgb = colors[c - 1];
        blockColor565 = display->color565(rgb[0], rgb[1], rgb[2]);
        for (int dy = 0; dy < cellSize; dy++) {
          for (int dx = 0; dx < cellSize; dx++) {
            int px = x * cellSize + dx;
            int py = y * cellSize + dy;
            display->drawPixelRGB888(px, py, rgb[0], rgb[1], rgb[2]);
            DisplayManager::backgroundBuffer[py][px] = blockColor565;
          }
        }
      } else {
        for (int dy = 0; dy < cellSize; dy++) {
          for (int dx = 0; dx < cellSize; dx++) {
            int px = x * cellSize + dx;
            int py = y * cellSize + dy;
            display->drawPixelRGB888(px, py, 0, 0, 0);
            DisplayManager::backgroundBuffer[py][px] = 0;
          }
        }
      }
    }
  }

  int cells[4][2];
  getPieceCells(curType, curRot, cells);
  const uint8_t* rgb = colors[curType];
  for (int i = 0; i < 4; i++) {
    int px = curX + cells[i][0];
    int py = curY + cells[i][1];
    if (px >= 0 && px < cols && py >= 0 && py < rows) {
      uint16_t pieceColor565 = display->color565(rgb[0], rgb[1], rgb[2]);
      for (int dy = 0; dy < cellSize; dy++) {
        for (int dx = 0; dx < cellSize; dx++) {
          int drawX = px * cellSize + dx;
          int drawY = py * cellSize + dy;
          display->drawPixelRGB888(drawX, drawY, rgb[0], rgb[1], rgb[2]);
          DisplayManager::backgroundBuffer[drawY][drawX] = pieceColor565;
        }
      }
    }
  }

  if (showClock) {
    DisplayManager::drawClockOverlay();
  }

  DisplayManager::endRedirectedFrame(true);
}
