#ifndef TETRIS_EFFECT_H
#define TETRIS_EFFECT_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// 俄罗斯方块屏保效果
// 动态格子大小，铺满 64x64 面板

#define TETRIS_MAX 64

class TetrisEffect {
public:
  static void init(bool clearMode = true, int cellSize = 2, int speed = 150, bool showClock = true, uint8_t piecesMask = 0x7F);
  static void update();
  static void render(MatrixPanel_I2S_DMA* display);
  static bool isActive;
  static bool doClearLines;
  static int cellSize;       // 每格像素大小 1/2/3
  static int dropSpeed;      // 下落间隔 ms
  static bool showClock;     // 是否显示时钟
  static uint8_t piecesMask; // 7bit，每bit对应一种方块是否启用

private:
  static uint8_t (*board)[64]; // 最大64x64格子（cellSize=1时）
  static int cols, rows;        // 实际列数行数（64/cellSize）
  
  // 当前方块
  static int curType;    // 方块类型 0-6
  static int curRot;     // 旋转状态 0-3
  static int curX, curY; // 位置（格子坐标）
  static int targetX;    // 目标列，落块过程中会逐步横向靠拢
  static uint8_t curColor; // 颜色索引
  
  // 时间控制
  static unsigned long lastDropTime;
  static bool needsRender;  // update() 后标记需要渲染

  // 增量渲染：上一帧的合成状态（board + 当前方块）
  static uint8_t (*prevDisplay)[TETRIS_MAX];
  static uint8_t (*targetMap)[TETRIS_MAX / 8];
  static int lastClockMinute;
  static int lastClockHour;
  static bool holdClockFrame;
  static unsigned long spawnCounter;
  static bool spawnBiasLeft;
  static unsigned long holdPulseAt;
  static bool holdPulseBright;
  
  // 方块定义：7种标准俄罗斯方块，4种旋转
  static const uint16_t pieces[7][4]; // 每个旋转状态用 16bit 编码 4x4 格子
  
  // 颜色表
  static const uint8_t colors[7][3]; // RGB
  
  static void spawnPiece();
  static bool canMove(int type, int rot, int x, int y);
  static void lockPiece();
  static int clearLines();
  static void resetBoard();
  static void getPieceCells(int type, int rot, int cells[4][2]);
  static int pieceMinX(int type, int rot);
  static int pieceMaxX(int type, int rot);
  static uint32_t buildSequenceSeed();
  static int computeSpawnDrift(int type);
  static int computeSpawnX(int type, int rot, int finalX);
  static void rebuildClockTarget();
  static bool buildClockPlacement(int& outRot, int& outX);
  static bool getTargetCell(int x, int y);
  static void setTargetCell(int x, int y);
  static bool ensureBuffers();
};

#endif
