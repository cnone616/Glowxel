#include "tetris_clock_effect.h"

#include <string.h>
#include <time.h>

#include "display_manager.h"

namespace {
constexpr int kPanelSize = 64;
constexpr uint8_t kDigitCount = 4;
constexpr uint8_t kColonCount = 2;
constexpr int8_t kFallStartY = -5;
constexpr int8_t kOriginalDigitXShifts[kDigitCount] = {1, 8, 18, 25};

enum TetrisClockColorIndex : uint8_t {
  kColorRed = 0,
  kColorGreen = 1,
  kColorBlue = 2,
  kColorWhite = 3,
  kColorYellow = 4,
  kColorCyan = 5,
  kColorMagenta = 6,
  kColorOrange = 7,
};

struct BlockInstruction {
  uint8_t block;
  uint8_t colorIndex;
  int8_t x;
  int8_t y;
  uint8_t rot;
};

struct DigitState {
  uint8_t digit;
  uint8_t blockIndex;
  uint8_t fallIndex;
  uint8_t colorShift;
  int8_t xShift;
};

struct ColonState {
  uint8_t blockIndex;
  uint8_t fallIndex;
  uint8_t colorShift;
  bool started;
};

struct Bounds {
  int minX;
  int maxX;
  int minY;
  int maxY;
};

constexpr BlockInstruction makeInstruction(
  uint8_t block,
  uint8_t colorIndex,
  int8_t x,
  int8_t y,
  uint8_t rot
) {
  return {block, colorIndex, x, y, rot};
}

constexpr uint8_t kColorSequence[] = {
  kColorRed,
  kColorGreen,
  kColorBlue,
  kColorWhite,
  kColorYellow,
  kColorCyan,
  kColorMagenta,
  kColorOrange,
};

constexpr uint8_t kBlockColors[8][3] = {
  {255, 0, 0},
  {0, 255, 0},
  {48, 73, 255},
  {255, 255, 255},
  {255, 255, 0},
  {0, 255, 255},
  {255, 0, 255},
  {255, 96, 0},
};

constexpr BlockInstruction kDigitFalls0[] = {
  makeInstruction(2, kColorCyan, 4, 16, 0),
  makeInstruction(4, kColorOrange, 2, 16, 1),
  makeInstruction(3, kColorYellow, 0, 16, 1),
  makeInstruction(6, kColorMagenta, 1, 16, 1),
  makeInstruction(5, kColorGreen, 4, 14, 0),
  makeInstruction(6, kColorMagenta, 0, 13, 3),
  makeInstruction(5, kColorGreen, 4, 12, 0),
  makeInstruction(5, kColorGreen, 0, 11, 0),
  makeInstruction(6, kColorMagenta, 4, 10, 1),
  makeInstruction(6, kColorMagenta, 0, 9, 1),
  makeInstruction(5, kColorGreen, 1, 8, 1),
  makeInstruction(2, kColorCyan, 3, 8, 3),
};

constexpr BlockInstruction kDigitFalls1[] = {
  makeInstruction(2, kColorCyan, 4, 16, 0),
  makeInstruction(3, kColorYellow, 4, 15, 1),
  makeInstruction(3, kColorYellow, 5, 13, 3),
  makeInstruction(2, kColorCyan, 4, 11, 2),
  makeInstruction(0, kColorRed, 4, 8, 0),
};

constexpr BlockInstruction kDigitFalls2[] = {
  makeInstruction(0, kColorRed, 4, 16, 0),
  makeInstruction(3, kColorYellow, 0, 16, 1),
  makeInstruction(1, kColorBlue, 1, 16, 3),
  makeInstruction(1, kColorBlue, 1, 15, 0),
  makeInstruction(3, kColorYellow, 1, 12, 2),
  makeInstruction(1, kColorBlue, 0, 12, 1),
  makeInstruction(2, kColorCyan, 3, 12, 3),
  makeInstruction(0, kColorRed, 4, 10, 0),
  makeInstruction(3, kColorYellow, 1, 8, 0),
  makeInstruction(2, kColorCyan, 3, 8, 3),
  makeInstruction(1, kColorBlue, 0, 8, 1),
};

constexpr BlockInstruction kDigitFalls3[] = {
  makeInstruction(1, kColorBlue, 3, 16, 3),
  makeInstruction(2, kColorCyan, 0, 16, 1),
  makeInstruction(3, kColorYellow, 1, 15, 2),
  makeInstruction(0, kColorRed, 4, 14, 0),
  makeInstruction(3, kColorYellow, 1, 12, 2),
  makeInstruction(1, kColorBlue, 0, 12, 1),
  makeInstruction(3, kColorYellow, 5, 12, 3),
  makeInstruction(2, kColorCyan, 3, 11, 0),
  makeInstruction(3, kColorYellow, 1, 8, 0),
  makeInstruction(1, kColorBlue, 0, 8, 1),
  makeInstruction(2, kColorCyan, 3, 8, 3),
};

constexpr BlockInstruction kDigitFalls4[] = {
  makeInstruction(0, kColorRed, 4, 16, 0),
  makeInstruction(0, kColorRed, 4, 14, 0),
  makeInstruction(3, kColorYellow, 1, 12, 0),
  makeInstruction(1, kColorBlue, 0, 12, 1),
  makeInstruction(2, kColorCyan, 0, 10, 0),
  makeInstruction(2, kColorCyan, 3, 12, 3),
  makeInstruction(3, kColorYellow, 4, 10, 3),
  makeInstruction(2, kColorCyan, 0, 9, 2),
  makeInstruction(3, kColorYellow, 5, 10, 1),
};

constexpr BlockInstruction kDigitFalls5[] = {
  makeInstruction(0, kColorRed, 0, 16, 0),
  makeInstruction(2, kColorCyan, 2, 16, 1),
  makeInstruction(2, kColorCyan, 3, 15, 0),
  makeInstruction(3, kColorYellow, 5, 16, 1),
  makeInstruction(3, kColorYellow, 1, 12, 0),
  makeInstruction(1, kColorBlue, 0, 12, 1),
  makeInstruction(2, kColorCyan, 3, 12, 3),
  makeInstruction(0, kColorRed, 0, 10, 0),
  makeInstruction(3, kColorYellow, 1, 8, 2),
  makeInstruction(1, kColorBlue, 0, 8, 1),
  makeInstruction(2, kColorCyan, 3, 8, 3),
};

constexpr BlockInstruction kDigitFalls6[] = {
  makeInstruction(2, kColorCyan, 0, 16, 1),
  makeInstruction(5, kColorGreen, 2, 16, 1),
  makeInstruction(6, kColorMagenta, 0, 15, 3),
  makeInstruction(6, kColorMagenta, 4, 16, 3),
  makeInstruction(5, kColorGreen, 4, 14, 0),
  makeInstruction(3, kColorYellow, 1, 12, 2),
  makeInstruction(2, kColorCyan, 0, 13, 2),
  makeInstruction(3, kColorYellow, 2, 11, 0),
  makeInstruction(0, kColorRed, 0, 10, 0),
  makeInstruction(3, kColorYellow, 1, 8, 0),
  makeInstruction(1, kColorBlue, 0, 8, 1),
  makeInstruction(2, kColorCyan, 3, 8, 3),
};

constexpr BlockInstruction kDigitFalls7[] = {
  makeInstruction(0, kColorRed, 4, 16, 0),
  makeInstruction(1, kColorBlue, 4, 14, 0),
  makeInstruction(3, kColorYellow, 5, 13, 1),
  makeInstruction(2, kColorCyan, 4, 11, 2),
  makeInstruction(3, kColorYellow, 1, 8, 2),
  makeInstruction(2, kColorCyan, 3, 8, 3),
  makeInstruction(1, kColorBlue, 0, 8, 1),
};

constexpr BlockInstruction kDigitFalls8[] = {
  makeInstruction(3, kColorYellow, 1, 16, 0),
  makeInstruction(6, kColorMagenta, 0, 16, 1),
  makeInstruction(3, kColorYellow, 5, 16, 1),
  makeInstruction(1, kColorBlue, 2, 15, 3),
  makeInstruction(4, kColorOrange, 0, 14, 0),
  makeInstruction(1, kColorBlue, 1, 12, 3),
  makeInstruction(6, kColorMagenta, 4, 13, 1),
  makeInstruction(2, kColorCyan, 0, 11, 1),
  makeInstruction(4, kColorOrange, 0, 10, 0),
  makeInstruction(4, kColorOrange, 4, 11, 0),
  makeInstruction(5, kColorGreen, 0, 8, 1),
  makeInstruction(5, kColorGreen, 2, 8, 1),
  makeInstruction(1, kColorBlue, 4, 9, 2),
};

constexpr BlockInstruction kDigitFalls9[] = {
  makeInstruction(0, kColorRed, 0, 16, 0),
  makeInstruction(3, kColorYellow, 2, 16, 0),
  makeInstruction(1, kColorBlue, 2, 15, 3),
  makeInstruction(1, kColorBlue, 4, 15, 2),
  makeInstruction(3, kColorYellow, 1, 12, 2),
  makeInstruction(3, kColorYellow, 5, 12, 3),
  makeInstruction(5, kColorGreen, 0, 12, 0),
  makeInstruction(1, kColorBlue, 2, 11, 3),
  makeInstruction(5, kColorGreen, 4, 9, 0),
  makeInstruction(6, kColorMagenta, 0, 10, 1),
  makeInstruction(5, kColorGreen, 0, 8, 1),
  makeInstruction(6, kColorMagenta, 2, 8, 2),
};

DigitState gDigitStates[kDigitCount] = {};
ColonState gColonStates[kColonCount] = {};
BlockInstruction gColonFalls[kColonCount] = {};
char gCurrentDigits[5] = {'0', '0', '0', '0', '\0'};
bool gNeedsRender = false;
bool gSequenceComplete = false;
unsigned long gLastStepTime = 0;
unsigned long gHoldUntilMillis = 0;
int gRenderOffsetX = 0;
int gRenderOffsetY = 12;
bool gSuppressRenderUntilVisible = false;

const BlockInstruction* getDigitInstructions(uint8_t digit, size_t& count) {
  switch (digit) {
    case 0:
      count = sizeof(kDigitFalls0) / sizeof(kDigitFalls0[0]);
      return kDigitFalls0;
    case 1:
      count = sizeof(kDigitFalls1) / sizeof(kDigitFalls1[0]);
      return kDigitFalls1;
    case 2:
      count = sizeof(kDigitFalls2) / sizeof(kDigitFalls2[0]);
      return kDigitFalls2;
    case 3:
      count = sizeof(kDigitFalls3) / sizeof(kDigitFalls3[0]);
      return kDigitFalls3;
    case 4:
      count = sizeof(kDigitFalls4) / sizeof(kDigitFalls4[0]);
      return kDigitFalls4;
    case 5:
      count = sizeof(kDigitFalls5) / sizeof(kDigitFalls5[0]);
      return kDigitFalls5;
    case 6:
      count = sizeof(kDigitFalls6) / sizeof(kDigitFalls6[0]);
      return kDigitFalls6;
    case 7:
      count = sizeof(kDigitFalls7) / sizeof(kDigitFalls7[0]);
      return kDigitFalls7;
    case 8:
      count = sizeof(kDigitFalls8) / sizeof(kDigitFalls8[0]);
      return kDigitFalls8;
    case 9:
      count = sizeof(kDigitFalls9) / sizeof(kDigitFalls9[0]);
      return kDigitFalls9;
    default:
      count = 0;
      return nullptr;
  }
}

void getCurrentTimeDigits(char digits[5]) {
  struct tm timeinfo = {};
  const time_t now = time(nullptr);
  if (now > 0) {
    localtime_r(&now, &timeinfo);
  }

  int hours = constrain(timeinfo.tm_hour, 0, 23);
  if (TetrisClockEffect::hourFormat == 12) {
    hours %= 12;
    if (hours == 0) {
      hours = 12;
    }
  }
  const int minutes = constrain(timeinfo.tm_min, 0, 59);
  snprintf(digits, 5, "%02d%02d", hours, minutes);
}

void getOriginalShapeCells(uint8_t blockType, uint8_t rotation, int cells[4][2]) {
  if (blockType == 0) {
    cells[0][0] = 0; cells[0][1] = 0;
    cells[1][0] = 1; cells[1][1] = 0;
    cells[2][0] = 0; cells[2][1] = -1;
    cells[3][0] = 1; cells[3][1] = -1;
    return;
  }

  if (blockType == 1) {
    if (rotation == 0) {
      cells[0][0] = 0; cells[0][1] = 0;
      cells[1][0] = 1; cells[1][1] = 0;
      cells[2][0] = 0; cells[2][1] = -1;
      cells[3][0] = 0; cells[3][1] = -2;
      return;
    }
    if (rotation == 1) {
      cells[0][0] = 0; cells[0][1] = 0;
      cells[1][0] = 0; cells[1][1] = -1;
      cells[2][0] = 1; cells[2][1] = -1;
      cells[3][0] = 2; cells[3][1] = -1;
      return;
    }
    if (rotation == 2) {
      cells[0][0] = 1; cells[0][1] = 0;
      cells[1][0] = 1; cells[1][1] = -1;
      cells[2][0] = 1; cells[2][1] = -2;
      cells[3][0] = 0; cells[3][1] = -2;
      return;
    }
    cells[0][0] = 0; cells[0][1] = 0;
    cells[1][0] = 1; cells[1][1] = 0;
    cells[2][0] = 2; cells[2][1] = 0;
    cells[3][0] = 2; cells[3][1] = -1;
    return;
  }

  if (blockType == 2) {
    if (rotation == 0) {
      cells[0][0] = 0; cells[0][1] = 0;
      cells[1][0] = 1; cells[1][1] = 0;
      cells[2][0] = 1; cells[2][1] = -1;
      cells[3][0] = 1; cells[3][1] = -2;
      return;
    }
    if (rotation == 1) {
      cells[0][0] = 0; cells[0][1] = 0;
      cells[1][0] = 1; cells[1][1] = 0;
      cells[2][0] = 2; cells[2][1] = 0;
      cells[3][0] = 0; cells[3][1] = -1;
      return;
    }
    if (rotation == 2) {
      cells[0][0] = 0; cells[0][1] = 0;
      cells[1][0] = 0; cells[1][1] = -1;
      cells[2][0] = 0; cells[2][1] = -2;
      cells[3][0] = 1; cells[3][1] = -2;
      return;
    }
    cells[0][0] = 0; cells[0][1] = -1;
    cells[1][0] = 1; cells[1][1] = -1;
    cells[2][0] = 2; cells[2][1] = -1;
    cells[3][0] = 2; cells[3][1] = 0;
    return;
  }

  if (blockType == 3) {
    if (rotation == 1 || rotation == 3) {
      cells[0][0] = 0; cells[0][1] = 0;
      cells[1][0] = 0; cells[1][1] = -1;
      cells[2][0] = 0; cells[2][1] = -2;
      cells[3][0] = 0; cells[3][1] = -3;
      return;
    }
    cells[0][0] = 0; cells[0][1] = 0;
    cells[1][0] = 1; cells[1][1] = 0;
    cells[2][0] = 2; cells[2][1] = 0;
    cells[3][0] = 3; cells[3][1] = 0;
    return;
  }

  if (blockType == 4) {
    if (rotation == 1 || rotation == 3) {
      cells[0][0] = 0; cells[0][1] = 0;
      cells[1][0] = 1; cells[1][1] = 0;
      cells[2][0] = 1; cells[2][1] = -1;
      cells[3][0] = 2; cells[3][1] = -1;
      return;
    }
    cells[0][0] = 1; cells[0][1] = 0;
    cells[1][0] = 0; cells[1][1] = -1;
    cells[2][0] = 1; cells[2][1] = -1;
    cells[3][0] = 0; cells[3][1] = -2;
    return;
  }

  if (blockType == 5) {
    if (rotation == 1 || rotation == 3) {
      cells[0][0] = 1; cells[0][1] = 0;
      cells[1][0] = 2; cells[1][1] = 0;
      cells[2][0] = 0; cells[2][1] = -1;
      cells[3][0] = 1; cells[3][1] = -1;
      return;
    }
    cells[0][0] = 0; cells[0][1] = 0;
    cells[1][0] = 0; cells[1][1] = -1;
    cells[2][0] = 1; cells[2][1] = -1;
    cells[3][0] = 1; cells[3][1] = -2;
    return;
  }

  if (rotation == 0) {
    cells[0][0] = 0; cells[0][1] = 0;
    cells[1][0] = 1; cells[1][1] = 0;
    cells[2][0] = 2; cells[2][1] = 0;
    cells[3][0] = 1; cells[3][1] = -1;
    return;
  }
  if (rotation == 1) {
    cells[0][0] = 0; cells[0][1] = 0;
    cells[1][0] = 0; cells[1][1] = -1;
    cells[2][0] = 0; cells[2][1] = -2;
    cells[3][0] = 1; cells[3][1] = -1;
    return;
  }
  if (rotation == 2) {
    cells[0][0] = 1; cells[0][1] = 0;
    cells[1][0] = 0; cells[1][1] = -1;
    cells[2][0] = 1; cells[2][1] = -1;
    cells[3][0] = 2; cells[3][1] = -1;
    return;
  }

  cells[0][0] = 1; cells[0][1] = 0;
  cells[1][0] = 0; cells[1][1] = -1;
  cells[2][0] = 1; cells[2][1] = -1;
  cells[3][0] = 1; cells[3][1] = -2;
}

Bounds getPlacedBlockBounds(const BlockInstruction& item, int shiftX, int shiftY) {
  int cells[4][2];
  getOriginalShapeCells(item.block, item.rot, cells);
  Bounds bounds = {999, -999, 999, -999};
  for (uint8_t index = 0; index < 4; index += 1) {
    const int x = shiftX + item.x + cells[index][0];
    const int y = shiftY + item.y + cells[index][1];
    if (x < bounds.minX) bounds.minX = x;
    if (x > bounds.maxX) bounds.maxX = x;
    if (y < bounds.minY) bounds.minY = y;
    if (y > bounds.maxY) bounds.maxY = y;
  }
  return bounds;
}

Bounds getDigitGroupBounds(const char digits[5]) {
  Bounds result = {999, -999, 999, -999};
  for (uint8_t index = 0; index < kDigitCount; index += 1) {
    size_t count = 0;
    const uint8_t digit = static_cast<uint8_t>(digits[index] - '0');
    const BlockInstruction* instructions = getDigitInstructions(digit, count);
    for (size_t itemIndex = 0; itemIndex < count; itemIndex += 1) {
      const Bounds bounds =
        getPlacedBlockBounds(instructions[itemIndex], kOriginalDigitXShifts[index], -1);
      if (bounds.minX < result.minX) result.minX = bounds.minX;
      if (bounds.maxX > result.maxX) result.maxX = bounds.maxX;
      if (bounds.minY < result.minY) result.minY = bounds.minY;
      if (bounds.maxY > result.maxY) result.maxY = bounds.maxY;
    }
  }
  if (result.minX == 999) {
    result.minX = 0;
    result.maxX = 0;
    result.minY = 0;
    result.maxY = 0;
  }
  return result;
}

void mergeBounds(Bounds& target, const Bounds& source) {
  if (source.minX < target.minX) target.minX = source.minX;
  if (source.maxX > target.maxX) target.maxX = source.maxX;
  if (source.minY < target.minY) target.minY = source.minY;
  if (source.maxY > target.maxY) target.maxY = source.maxY;
}

uint32_t buildColorShuffleSeed(const char digits[5]) {
  uint32_t seed = millis() ^ 0x9e3779b9UL;
  for (uint8_t index = 0; index < 4; index += 1) {
    seed ^= (uint32_t)(uint8_t)digits[index];
    seed *= 16777619UL;
  }
  return seed == 0UL ? 0x6d2b79f5UL : seed;
}

uint32_t nextColorShuffleValue(uint32_t& state) {
  if (state == 0UL) {
    state = 0x6d2b79f5UL;
  }
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

uint8_t nextColorShift(uint32_t& state, uint8_t avoidShift) {
  const uint8_t colorCount = sizeof(kBlockColors) / sizeof(kBlockColors[0]);
  for (uint8_t attempt = 0; attempt < colorCount; attempt += 1) {
    const uint8_t shift = static_cast<uint8_t>(nextColorShuffleValue(state) % colorCount);
    if (shift != avoidShift) {
      return shift;
    }
  }
  return static_cast<uint8_t>(nextColorShuffleValue(state) % colorCount);
}

void buildColonFallsForDigits(const char digits[5]) {
  const Bounds digitBounds = getDigitGroupBounds(digits);
  const Bounds colonBounds = getPlacedBlockBounds(
    makeInstruction(0, kColorWhite, 0, 0, 0),
    0,
    0
  );
  const int digitHeight = digitBounds.maxY - digitBounds.minY + 1;
  const int colonBlockHeight = colonBounds.maxY - colonBounds.minY + 1;
  const int maxGap = max(0, digitHeight - colonBlockHeight * 2);
  const int colonGap = constrain((digitHeight + 2) / 5, 0, maxGap);
  const int colonGroupHeight = colonBlockHeight * 2 + colonGap;
  const int topPadding = max(0, (digitHeight - colonGroupHeight) / 2);
  const int topRow = digitBounds.minY + topPadding;
  const int bottomTopRow = topRow + colonBlockHeight + colonGap;
  const int topY = topRow - colonBounds.minY + 1;
  const int bottomY = bottomTopRow - colonBounds.minY + 1;

  const uint8_t topDigit = static_cast<uint8_t>(digits[1] - '0');
  const uint8_t bottomDigit = static_cast<uint8_t>(digits[2] - '0');
  const uint8_t topColor =
    kColorSequence[(topDigit + 2) % (sizeof(kColorSequence) / sizeof(kColorSequence[0]))];
  const uint8_t bottomColor =
    kColorSequence[(bottomDigit + 5) % (sizeof(kColorSequence) / sizeof(kColorSequence[0]))];

  gColonFalls[0] = makeInstruction(0, bottomColor, 15, bottomY, 0);
  gColonFalls[1] = makeInstruction(0, topColor, 15, topY, 0);
}

void updateRenderLayout(const char digits[5]) {
  Bounds bounds = getDigitGroupBounds(digits);
  for (uint8_t index = 0; index < kColonCount; index += 1) {
    mergeBounds(bounds, getPlacedBlockBounds(gColonFalls[index], 0, -1));
  }

  const int scale = TetrisClockEffect::cellSize;
  const int width = (bounds.maxX - bounds.minX + 1) * scale;
  const int height = (bounds.maxY - bounds.minY + 1) * scale;
  gRenderOffsetX = (kPanelSize - width) / 2 - bounds.minX * scale;
  gRenderOffsetY = (kPanelSize - height) / 2 - bounds.minY * scale;
}

int getCurrentFallY(uint8_t fallIndex) {
  return kFallStartY + static_cast<int>(fallIndex) - 1;
}

uint8_t resolveFallingRotation(uint8_t targetRotation, uint8_t fallIndex, int stopY) {
  const int travelDistance = max(1, stopY - kFallStartY);
  uint8_t rotation = targetRotation;
  if (rotation == 1) {
    if (fallIndex < static_cast<uint8_t>(travelDistance / 2)) {
      rotation = 0;
    }
  } else if (rotation == 2) {
    if (fallIndex < static_cast<uint8_t>(travelDistance / 3)) {
      rotation = 0;
    } else if (fallIndex < static_cast<uint8_t>((travelDistance / 3) * 2)) {
      rotation = 1;
    }
  } else if (rotation == 3) {
    if (fallIndex < static_cast<uint8_t>(travelDistance / 4)) {
      rotation = 0;
    } else if (fallIndex < static_cast<uint8_t>((travelDistance / 4) * 2)) {
      rotation = 1;
    } else if (fallIndex < static_cast<uint8_t>((travelDistance / 4) * 3)) {
      rotation = 2;
    }
  }
  return rotation;
}

void clearSettledFrame() {
  memset(DisplayManager::backgroundBuffer, 0, sizeof(DisplayManager::backgroundBuffer));
}

void paintBlockShapeToBuffer(
  uint16_t* targetBuffer,
  const BlockInstruction& item,
  int xPos,
  int yPos,
  uint8_t rotation,
  uint8_t colorShift
) {
  if (targetBuffer == nullptr) {
    return;
  }
  int cells[4][2];
  getOriginalShapeCells(item.block, rotation, cells);
  const uint8_t resolvedColorIndex =
    static_cast<uint8_t>((item.colorIndex + colorShift) %
                         (sizeof(kBlockColors) / sizeof(kBlockColors[0])));
  const uint8_t* rgb = kBlockColors[resolvedColorIndex];
  const uint16_t color565 = MatrixPanel_I2S_DMA::color565(rgb[0], rgb[1], rgb[2]);
  const int scale = TetrisClockEffect::cellSize;

  for (uint8_t cellIndex = 0; cellIndex < 4; cellIndex += 1) {
    const int baseX =
      gRenderOffsetX + (xPos + cells[cellIndex][0]) * scale;
    const int baseY =
      gRenderOffsetY + (yPos + cells[cellIndex][1]) * scale;

    for (int dy = 0; dy < scale; dy += 1) {
      for (int dx = 0; dx < scale; dx += 1) {
        const int px = baseX + dx;
        const int py = baseY + dy;
        if (px < 0 || px >= kPanelSize || py < 0 || py >= kPanelSize) {
          continue;
        }
        targetBuffer[py * kPanelSize + px] = color565;
      }
    }
  }
}

bool blockTouchesPanel(
  const BlockInstruction& item,
  int xPos,
  int yPos,
  uint8_t rotation
) {
  int cells[4][2];
  getOriginalShapeCells(item.block, rotation, cells);
  const int scale = TetrisClockEffect::cellSize;

  for (uint8_t cellIndex = 0; cellIndex < 4; cellIndex += 1) {
    const int baseX = gRenderOffsetX + (xPos + cells[cellIndex][0]) * scale;
    const int baseY = gRenderOffsetY + (yPos + cells[cellIndex][1]) * scale;
    const int maxX = baseX + scale - 1;
    const int maxY = baseY + scale - 1;
    if (maxX < 0 || maxY < 0 || baseX >= kPanelSize || baseY >= kPanelSize) {
      continue;
    }
    return true;
  }
  return false;
}

uint8_t findFirstVisibleFallIndex(const BlockInstruction& item, int xPos) {
  const int travelDistance = max(0, item.y - kFallStartY);
  for (int fallIndex = 0; fallIndex <= travelDistance; fallIndex += 1) {
    const uint8_t resolvedRotation =
      resolveFallingRotation(item.rot, static_cast<uint8_t>(fallIndex), item.y);
    if (blockTouchesPanel(
          item,
          xPos,
          getCurrentFallY(static_cast<uint8_t>(fallIndex)),
          resolvedRotation)) {
      return static_cast<uint8_t>(fallIndex);
    }
  }
  return 0;
}

bool hasVisibleActiveBlocks() {
  for (uint8_t stateIndex = 0; stateIndex < kDigitCount; stateIndex += 1) {
    const DigitState& state = gDigitStates[stateIndex];
    size_t count = 0;
    const BlockInstruction* instructions = getDigitInstructions(state.digit, count);
    if (state.blockIndex >= count) {
      continue;
    }
    const BlockInstruction& current = instructions[state.blockIndex];
    if (blockTouchesPanel(
          current,
          current.x + state.xShift,
          getCurrentFallY(state.fallIndex),
          resolveFallingRotation(current.rot, state.fallIndex, current.y))) {
      return true;
    }
  }

  for (uint8_t stateIndex = 0; stateIndex < kColonCount; stateIndex += 1) {
    const ColonState& state = gColonStates[stateIndex];
    if (!state.started || state.blockIndex > 0) {
      continue;
    }
    const BlockInstruction& current = gColonFalls[stateIndex];
    if (blockTouchesPanel(current, current.x, getCurrentFallY(state.fallIndex), current.rot)) {
      return true;
    }
  }

  return false;
}

bool areDigitsDone() {
  for (uint8_t index = 0; index < kDigitCount; index += 1) {
    size_t count = 0;
    const BlockInstruction* instructions = getDigitInstructions(gDigitStates[index].digit, count);
    (void)instructions;
    if (gDigitStates[index].blockIndex < count) {
      return false;
    }
  }
  return true;
}

bool areColonsDone() {
  for (uint8_t index = 0; index < kColonCount; index += 1) {
    if (gColonStates[index].blockIndex == 0) {
      return false;
    }
  }
  return true;
}

void restartSequenceForDigits(const char digits[5], bool preserveCurrentFrameUntilVisible) {
  uint32_t colorSeed = buildColorShuffleSeed(digits);
  uint8_t lastShift = 0xff;
  memcpy(gCurrentDigits, digits, sizeof(gCurrentDigits));
  for (uint8_t index = 0; index < kDigitCount; index += 1) {
    gDigitStates[index].digit = static_cast<uint8_t>(digits[index] - '0');
    gDigitStates[index].blockIndex = 0;
    gDigitStates[index].fallIndex = 0;
    gDigitStates[index].colorShift = nextColorShift(colorSeed, lastShift);
    gDigitStates[index].xShift = kOriginalDigitXShifts[index];
    lastShift = gDigitStates[index].colorShift;
  }
  buildColonFallsForDigits(digits);
  updateRenderLayout(digits);
  clearSettledFrame();
  for (uint8_t index = 0; index < kColonCount; index += 1) {
    gColonStates[index].blockIndex = 0;
    gColonStates[index].fallIndex = 0;
    gColonStates[index].colorShift = nextColorShift(colorSeed, lastShift);
    gColonStates[index].started = index == 0;
    lastShift = gColonStates[index].colorShift;
  }
  gSequenceComplete = false;
  gHoldUntilMillis = 0;
  gLastStepTime = millis();
  gSuppressRenderUntilVisible = preserveCurrentFrameUntilVisible;
  gNeedsRender = !preserveCurrentFrameUntilVisible;
}

unsigned long computeHoldDuration() {
  const int holdFrames =
    TetrisClockEffect::dropSpeed <= 90 ? 40 :
    (TetrisClockEffect::dropSpeed <= 170 ? 56 : 68);
  return static_cast<unsigned long>(holdFrames) *
         static_cast<unsigned long>(TetrisClockEffect::dropSpeed);
}

bool stepDigitStates() {
  for (uint8_t index = 0; index < kDigitCount; index += 1) {
    size_t count = 0;
    DigitState& state = gDigitStates[index];
    const BlockInstruction* instructions = getDigitInstructions(state.digit, count);
    if (state.blockIndex >= count) {
      continue;
    }

    const BlockInstruction& current = instructions[state.blockIndex];
    state.fallIndex += 1;
    const int travelDistance = current.y - kFallStartY;
    if (state.fallIndex > travelDistance) {
      paintBlockShapeToBuffer(
        &DisplayManager::backgroundBuffer[0][0],
        current,
        current.x + state.xShift,
        current.y - 1,
        current.rot,
        state.colorShift
      );
      state.fallIndex = 0;
      state.blockIndex += 1;
      if (state.blockIndex < count) {
        const BlockInstruction& next = instructions[state.blockIndex];
        state.fallIndex = findFirstVisibleFallIndex(next, next.x + state.xShift);
      }
    }
  }

  return areDigitsDone();
}

bool stepColonStates() {
  for (uint8_t index = 0; index < kColonCount; index += 1) {
    ColonState& state = gColonStates[index];
    if (!state.started || state.blockIndex > 0) {
      continue;
    }

    state.fallIndex += 1;
    const BlockInstruction& current = gColonFalls[index];
    const int travelDistance = current.y - kFallStartY;
    if (state.fallIndex > travelDistance) {
      paintBlockShapeToBuffer(
        &DisplayManager::backgroundBuffer[0][0],
        current,
        current.x,
        current.y - 1,
        current.rot,
        state.colorShift
      );
      state.fallIndex = 0;
      state.blockIndex = 1;
      if (index + 1 < kColonCount) {
        gColonStates[index + 1].started = true;
        gColonStates[index + 1].fallIndex =
          findFirstVisibleFallIndex(gColonFalls[index + 1], gColonFalls[index + 1].x);
      }
    }
    return false;
  }

  return true;
}
}

bool TetrisClockEffect::isActive = false;
int TetrisClockEffect::dropSpeed = 150;
int TetrisClockEffect::cellSize = 2;
int TetrisClockEffect::hourFormat = 24;

void TetrisClockEffect::init(int speed, int cellSizeValue, int hourFormatValue) {
  dropSpeed = constrain(speed, 30, 1000);
  cellSize = constrain(cellSizeValue, 1, 2);
  hourFormat = hourFormatValue == 12 ? 12 : 24;
  char digits[5] = {'0', '0', '0', '0', '\0'};
  getCurrentTimeDigits(digits);
  restartSequenceForDigits(digits, false);
  isActive = true;
}

void TetrisClockEffect::deactivate() {
  isActive = false;
  gNeedsRender = false;
  gSequenceComplete = false;
  gHoldUntilMillis = 0;
  gSuppressRenderUntilVisible = false;
}

void TetrisClockEffect::update() {
  if (!isActive) {
    return;
  }

  char digits[5] = {'0', '0', '0', '0', '\0'};
  getCurrentTimeDigits(digits);
  if (strncmp(digits, gCurrentDigits, 4) != 0) {
    restartSequenceForDigits(digits, true);
    return;
  }

  const unsigned long now = millis();
  if (gSequenceComplete) {
    return;
  }

  if (now - gLastStepTime < static_cast<unsigned long>(dropSpeed)) {
    return;
  }
  gLastStepTime = now;

  const bool digitsDone = stepDigitStates();
  const bool colonDone = stepColonStates();
  if (digitsDone && colonDone) {
    gSequenceComplete = true;
    gHoldUntilMillis = now + computeHoldDuration();
  }
  if (gSuppressRenderUntilVisible && !hasVisibleActiveBlocks()) {
    return;
  }
  gSuppressRenderUntilVisible = false;
  gNeedsRender = true;
}

void TetrisClockEffect::render(MatrixPanel_I2S_DMA* display) {
  if (!isActive || !gNeedsRender || display == nullptr) {
    return;
  }
  gNeedsRender = false;

  uint16_t* frameBuffer = &DisplayManager::animationBuffer[0][0];
  MatrixPanel_I2S_DMA* offscreen = DisplayManager::beginOffscreenFrame(frameBuffer, 0);
  if (offscreen == nullptr) {
    return;
  }
  (void)offscreen;

  memcpy(frameBuffer, DisplayManager::backgroundBuffer, sizeof(DisplayManager::backgroundBuffer));

  for (uint8_t stateIndex = 0; stateIndex < kDigitCount; stateIndex += 1) {
    const DigitState& state = gDigitStates[stateIndex];
    size_t count = 0;
    const BlockInstruction* instructions = getDigitInstructions(state.digit, count);
    if (state.blockIndex < count) {
      const BlockInstruction& current = instructions[state.blockIndex];
      paintBlockShapeToBuffer(
        frameBuffer,
        current,
        current.x + state.xShift,
        getCurrentFallY(state.fallIndex),
        resolveFallingRotation(current.rot, state.fallIndex, current.y),
        state.colorShift
      );
    }
  }

  for (uint8_t stateIndex = 0; stateIndex < kColonCount; stateIndex += 1) {
    const ColonState& state = gColonStates[stateIndex];
    if (!state.started) {
      continue;
    }
    const BlockInstruction& item = gColonFalls[stateIndex];
    if (state.blockIndex > 0) {
      continue;
    }
    paintBlockShapeToBuffer(
      frameBuffer,
      item,
      item.x,
      getCurrentFallY(state.fallIndex),
      item.rot,
      state.colorShift
    );
  }

  DisplayManager::presentOffscreenFrame(frameBuffer);
}
