#include "tetris_effect.h"
#include "display_manager.h"

// 静态成员初始化
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
int TetrisEffect::curX = 0;
int TetrisEffect::curY = 0;
uint8_t TetrisEffect::curColor = 1;
unsigned long TetrisEffect::lastDropTime = 0;
bool TetrisEffect::needsRender = true;
uint8_t TetrisEffect::prevDisplay[TETRIS_MAX][TETRIS_MAX];

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

void TetrisEffect::init(bool clearMode, int cellSz, int speed, bool clock, uint8_t mask) {
  doClearLines = clearMode;
  cellSize = constrain(cellSz, 1, 3);
  dropSpeed = constrain(speed, 30, 1000);
  showClock = clock;
  piecesMask = mask ? mask : 0x7F; // 至少一种
  cols = 64 / cellSize;
  rows = 64 / cellSize;
  resetBoard();
  memset(prevDisplay, 0, sizeof(prevDisplay));
  spawnPiece();
  lastDropTime = millis();
  needsRender = true;
  isActive = true;
}

void TetrisEffect::resetBoard() {
  memset(board, 0, sizeof(board));
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
  curType = enabled[random(count)];
  curColor = curType + 1;

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
  curX = bestX;
  curY = -2;
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

  unsigned long now = millis();
  if (now - lastDropTime < (unsigned long)dropSpeed) return;
  lastDropTime = now;

  // 尝试下落
  if (canMove(curType, curRot, curX, curY + 1)) {
    curY++;
    needsRender = true;
  } else {
    // 贴合锁定
    lockPiece();

    if (doClearLines) {
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
  needsRender = false;

  // 构建当前帧的合成状态（board + 当前下落方块）
  uint8_t curDisplay[TETRIS_MAX][TETRIS_MAX];
  memcpy(curDisplay, board, sizeof(board));

  int cells[4][2];
  getPieceCells(curType, curRot, cells);
  for (int i = 0; i < 4; i++) {
    int px = curX + cells[i][0];
    int py = curY + cells[i][1];
    if (px >= 0 && px < cols && py >= 0 && py < rows) {
      curDisplay[py][px] = curColor; // 和 lockPiece 一致
    }
  }

  // 增量渲染：只重绘变化的格子
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      if (curDisplay[y][x] == prevDisplay[y][x]) continue;

      uint8_t c = curDisplay[y][x];
      if (c > 0) {
        const uint8_t* rgb = colors[c - 1];
        for (int dy = 0; dy < cellSize; dy++) {
          for (int dx = 0; dx < cellSize; dx++) {
            display->drawPixelRGB888(x * cellSize + dx, y * cellSize + dy, rgb[0], rgb[1], rgb[2]);
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
  memcpy(prevDisplay, curDisplay, sizeof(prevDisplay));

  // 时钟在方块上层叠加
  if (showClock) {
    DisplayManager::drawClockOverlay();
  }
}

