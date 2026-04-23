#ifndef TETRIS_EFFECT_H
#define TETRIS_EFFECT_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define TETRIS_MAX 64

class TetrisEffect {
public:
  static void init(bool clearMode = true, int cellSize = 2, int speed = 150, bool showClock = true, uint8_t piecesMask = 0x7F);
  static void deactivate();
  static void update();
  static void render(MatrixPanel_I2S_DMA* display);
  static bool isActive;
  static bool doClearLines;
  static int cellSize;
  static int dropSpeed;
  static bool showClock;
  static uint8_t piecesMask;

private:
  static uint8_t board[64][64];
  static int cols, rows;
  static int curType;
  static int curRot;
  static int targetRot;
  static int curX, curY;
  static int targetX;
  static uint8_t curColor;
  static unsigned long lastDropTime;
  static bool needsRender;

  static const uint16_t pieces[7][4];
  static const uint8_t colors[7][3];

  static void spawnPiece();
  static bool canMove(int type, int rot, int x, int y);
  static void lockPiece();
  static int clearLines();
  static void resetBoard();
  static void getPieceCells(int type, int rot, int cells[4][2]);
};

#endif
