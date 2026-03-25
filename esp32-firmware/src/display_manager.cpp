#include "display_manager.h"
#include <time.h>

// 静态成员初始化
MatrixPanel_I2S_DMA* DisplayManager::dma_display = nullptr;
DeviceMode DisplayManager::currentMode = MODE_CANVAS;
int DisplayManager::currentBrightness = 50;
bool DisplayManager::clientConnected = false;
uint8_t DisplayManager::canvasBuffer[64][64][3];
uint16_t DisplayManager::backgroundBuffer[64][64];
bool DisplayManager::canvasInitialized = false;
bool DisplayManager::backgroundValid = false;
bool DisplayManager::receivingPixels = false;
DisplayManager::BlackPixel* DisplayManager::blackPixels = nullptr;
int DisplayManager::blackPixelCount = 0;

// Loading 动画状态
bool DisplayManager::isLoadingActive = false;
int DisplayManager::loadingStep = 0;
unsigned long DisplayManager::lastLoadingUpdate = 0;

// 常量定义
const int DisplayManager::PANEL_RES_X = 64;
const int DisplayManager::PANEL_RES_Y = 64;
const int DisplayManager::PANEL_CHAIN = 1;
const int DisplayManager::MAX_PIXELS = PANEL_RES_X * PANEL_RES_Y;

void DisplayManager::init() {
  Serial.println("1. 初始化LED灯板...");
  setupMatrix();
}

void DisplayManager::setupMatrix() {
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  mxconfig.gpio.e = 18;
  mxconfig.clkphase = false;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(51);
  dma_display->clearScreen();

  Serial.println("显示开机Logo...");
  drawLogo(12, 7);  // 开机 logo 偏上
  // 开机 logo 额外显示品牌名
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(220, 220, 220));
  dma_display->setCursor(11, 52);
  dma_display->print("RenLight");
  delay(2000);
  Serial.println("LED灯板初始化完成");
}

void DisplayManager::drawLogo(int x, int y) {
  // RenLight 品牌色：orange → red → yellow → blue
  uint16_t orange = dma_display->color565(249, 115, 22);
  uint16_t red    = dma_display->color565(239, 68, 68);
  uint16_t yellow = dma_display->color565(251, 191, 36);
  uint16_t blue   = dma_display->color565(59, 130, 246);

  int bs = 11, gap = 3, step = bs + gap;

  uint16_t grid[3][3] = {
    { orange, orange, red    },
    { orange, yellow, yellow },
    { orange, blue,   blue   }
  };

  for (int row = 0; row < 3; row++)
    for (int col = 0; col < 3; col++)
      dma_display->fillRect(x + col * step, y + row * step, bs, bs, grid[row][col]);
}

// Loading 动画控制
void DisplayManager::startLoadingAnimation() {
  isLoadingActive = true;
  loadingStep = 0;
  lastLoadingUpdate = millis();
  Serial.println("Loading 动画已启动");
}

void DisplayManager::stopLoadingAnimation() {
  isLoadingActive = false;
  Serial.println("Loading 动画已停止");
}

void DisplayManager::updateLoadingAnimation() {
  if (!isLoadingActive) return;

  unsigned long now = millis();
  if (now - lastLoadingUpdate < 200) return;  // 每 200ms 更新一次

  lastLoadingUpdate = now;
  loadingStep = (loadingStep + 1) % 8;

  // 九宫格 Logo 参数（与 drawLogo 一致）
  int x = 12;  // 水平居中
  int y = 12;  // 上下居中
  int bs = 11, gap = 3, step = bs + gap;

  // 品牌色
  uint16_t orange = dma_display->color565(249, 115, 22);
  uint16_t red    = dma_display->color565(239, 68, 68);
  uint16_t yellow = dma_display->color565(251, 191, 36);
  uint16_t blue   = dma_display->color565(59, 130, 246);

  uint16_t grid[3][3] = {
    { orange, orange, red    },
    { orange, yellow, yellow },
    { orange, blue,   blue   }
  };

  // 清屏
  dma_display->clearScreen();

  // 绘制暗色底（所有格子 30% 亮度）
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      uint8_t r = ((grid[row][col] >> 11) & 0x1F) * 255 / 31 * 0.3;
      uint8_t g = ((grid[row][col] >> 5) & 0x3F) * 255 / 63 * 0.3;
      uint8_t b = (grid[row][col] & 0x1F) * 255 / 31 * 0.3;
      uint16_t dimColor = dma_display->color565(r, g, b);
      dma_display->fillRect(x + col * step, y + row * step, bs, bs, dimColor);
    }
  }

  // 外圈顺序：1→2→3→6→9→8→7→4
  int outer[8][2] = {
    {0, 0}, {0, 1}, {0, 2}, {1, 2},
    {2, 2}, {2, 1}, {2, 0}, {1, 0}
  };

  int row = outer[loadingStep][0];
  int col = outer[loadingStep][1];

  // 高亮当前格子
  dma_display->fillRect(x + col * step, y + row * step, bs, bs, grid[row][col]);
}

// 前向声明
static int clockConfig_timeY();
static void writeBackgroundPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
static void restoreBackgroundRect(int x, int y, int w, int h);
static int centeredTextWidth(const char* text, int size);
static void centeredTextBounds(const char* text, int y, int size, int& x, int& w, int& h);
static void drawStaticClockOverlayDirty(const ClockConfig& c, const struct tm& timeinfo);
static void cacheLogoBackground(int x, int y);

static void writeBackgroundPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (x < 0 || x >= DisplayManager::PANEL_RES_X || y < 0 || y >= DisplayManager::PANEL_RES_Y) {
    return;
  }
  DisplayManager::backgroundBuffer[y][x] = DisplayManager::dma_display->color565(r, g, b);
}

static void restoreBackgroundRect(int x, int y, int w, int h) {
  if (!DisplayManager::backgroundValid || w <= 0 || h <= 0) {
    return;
  }

  int startX = x < 0 ? 0 : x;
  int startY = y < 0 ? 0 : y;
  int endX = x + w;
  int endY = y + h;
  if (endX > DisplayManager::PANEL_RES_X) endX = DisplayManager::PANEL_RES_X;
  if (endY > DisplayManager::PANEL_RES_Y) endY = DisplayManager::PANEL_RES_Y;

  for (int py = startY; py < endY; py++) {
    for (int px = startX; px < endX; px++) {
      DisplayManager::dma_display->drawPixel(px, py, DisplayManager::backgroundBuffer[py][px]);
    }
  }
}

static void cacheLogoBackground(int x, int y) {
  uint8_t grid[3][3][3] = {
    { {249, 115, 22}, {249, 115, 22}, {239, 68, 68} },
    { {249, 115, 22}, {251, 191, 36}, {251, 191, 36} },
    { {249, 115, 22}, {59, 130, 246}, {59, 130, 246} }
  };

  int bs = 11;
  int gap = 3;
  int step = bs + gap;

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      for (int dy = 0; dy < bs; dy++) {
        for (int dx = 0; dx < bs; dx++) {
          writeBackgroundPixel(
            x + col * step + dx,
            y + row * step + dy,
            grid[row][col][0],
            grid[row][col][1],
            grid[row][col][2]
          );
        }
      }
    }
  }
}

static int centeredTextWidth(const char* text, int size) {
  int len = strlen(text);
  return len > 0 ? (len * 4 * size - size) : 0;
}

static void centeredTextBounds(const char* text, int y, int size, int& x, int& w, int& h) {
  w = centeredTextWidth(text, size);
  h = 5 * size;
  x = (DisplayManager::PANEL_RES_X - w) / 2;
  if (x < 0) x = 0;
}

void DisplayManager::drawPixels(const PixelData* pixels, int pixelCount, bool clearFirst) {
  if (clearFirst) {
    dma_display->clearScreen();
  }

  if (pixels == nullptr || pixelCount <= 0) {
    return;
  }

  for (int i = 0; i < pixelCount; i++) {
    const PixelData& p = pixels[i];
    if (p.x < PANEL_RES_X && p.y < PANEL_RES_Y) {
      dma_display->drawPixelRGB888(p.x, p.y, p.r, p.g, p.b);
      writeBackgroundPixel(p.x, p.y, p.r, p.g, p.b);
    }
    if (i % 100 == 0) {
      yield();
    }
  }
}

void DisplayManager::renderAnimationFrame(const PixelData* pixels, int pixelCount, bool clearFirst) {
  drawPixels(pixels, pixelCount, clearFirst);
  drawClockOverlay();
}

void DisplayManager::renderCanvas() {
  dma_display->clearScreen();

  if (!canvasInitialized) {
    drawLogo(12, 12);
    return;
  }

  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      uint8_t r = canvasBuffer[y][x][0];
      uint8_t g = canvasBuffer[y][x][1];
      uint8_t b = canvasBuffer[y][x][2];
      if (r > 0 || g > 0 || b > 0) {
        dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }
    if (y % 8 == 0) {
      yield();
    }
  }
}

void DisplayManager::clearCanvas() {
  memset(canvasBuffer, 0, sizeof(canvasBuffer));
  canvasInitialized = false;

  if (blackPixels != nullptr) {
    free(blackPixels);
    blackPixels = nullptr;
  }
  blackPixelCount = 0;
}

void DisplayManager::highlightCanvasColor(int r, int g, int b) {
  if (!canvasInitialized) {
    return;
  }

  dma_display->clearScreen();

  if (r == 0 && g == 0 && b == 0) {
    for (int i = 0; i < blackPixelCount; i++) {
      dma_display->drawPixelRGB888(blackPixels[i].x, blackPixels[i].y, 255, 255, 255);
    }
    return;
  }

  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      uint8_t cr = canvasBuffer[y][x][0];
      uint8_t cg = canvasBuffer[y][x][1];
      uint8_t cb = canvasBuffer[y][x][2];
      if (cr > 0 || cg > 0 || cb > 0) {
        bool match = (abs(cr - r) <= 2 && abs(cg - g) <= 2 && abs(cb - b) <= 2);
        if (match) {
          dma_display->drawPixelRGB888(x, y, 255, 255, 255);
        }
      }
    }
    if (y % 8 == 0) {
      yield();
    }
  }
}

void DisplayManager::highlightCanvasRow(int row) {
  if (!canvasInitialized) {
    return;
  }

  dma_display->clearScreen();

  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      uint8_t r = canvasBuffer[y][x][0];
      uint8_t g = canvasBuffer[y][x][1];
      uint8_t b = canvasBuffer[y][x][2];

      if (r > 0 || g > 0 || b > 0) {
        uint8_t displayR = r;
        uint8_t displayG = g;
        uint8_t displayB = b;

        if (row >= 0 && y != row) {
          displayR = r * 0.2;
          displayG = g * 0.2;
          displayB = b * 0.2;
        }

        dma_display->drawPixelRGB888(x, y, displayR, displayG, displayB);
      }
    }
    if (y % 8 == 0) {
      yield();
    }
  }
}

void DisplayManager::renderAnimationTransition(
    const PixelData* fromPixels,
    int fromPixelCount,
    const PixelData* toPixels,
    int toPixelCount,
    uint8_t mix) {
  static uint8_t blended[PANEL_RES_Y][PANEL_RES_X][3];
  memset(blended, 0, sizeof(blended));

  int fromWeight = 255 - mix;
  if (fromPixels != nullptr && fromPixelCount > 0) {
    for (int i = 0; i < fromPixelCount; i++) {
      const PixelData& p = fromPixels[i];
      if (p.x < PANEL_RES_X && p.y < PANEL_RES_Y) {
        blended[p.y][p.x][0] = (uint16_t)p.r * fromWeight / 255;
        blended[p.y][p.x][1] = (uint16_t)p.g * fromWeight / 255;
        blended[p.y][p.x][2] = (uint16_t)p.b * fromWeight / 255;
      }
    }
  }

  if (toPixels != nullptr && toPixelCount > 0) {
    for (int i = 0; i < toPixelCount; i++) {
      const PixelData& p = toPixels[i];
      if (p.x < PANEL_RES_X && p.y < PANEL_RES_Y) {
        blended[p.y][p.x][0] = blended[p.y][p.x][0] + ((uint16_t)p.r * mix / 255);
        blended[p.y][p.x][1] = blended[p.y][p.x][1] + ((uint16_t)p.g * mix / 255);
        blended[p.y][p.x][2] = blended[p.y][p.x][2] + ((uint16_t)p.b * mix / 255);
      }
    }
  }

  dma_display->clearScreen();
  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      dma_display->drawPixelRGB888(
        x, y,
        blended[y][x][0],
        blended[y][x][1],
        blended[y][x][2]
      );
    }
    if (y % 8 == 0) {
      yield();
    }
  }

  drawClockOverlay();
}

// 独立背景绘制：清屏 + 画像素背景或 Logo，不涉及时钟文字
void DisplayManager::drawBackground() {
  PixelData* imagePixels = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animImagePixels
    : ConfigManager::staticImagePixels;
  int imagePixelCount = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animImagePixelCount
    : ConfigManager::staticImagePixelCount;

  dma_display->clearScreen();
  memset(backgroundBuffer, 0, sizeof(backgroundBuffer));
  backgroundValid = true;

  if (imagePixels != nullptr && imagePixelCount > 0) {
    drawPixels(imagePixels, imagePixelCount, false);
  } else {
    drawLogo(12, 18);
    cacheLogoBackground(12, 18);
  }
}

void DisplayManager::displayClock(bool force) {
  // 正在接收像素，不碰屏幕
  if (receivingPixels) return;
  // 根据当前模式选择对应的时钟配置和像素数据
  ClockConfig& cfg = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animClockConfig
    : ConfigManager::clockConfig;

  PixelData* imagePixels = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animImagePixels
    : ConfigManager::staticImagePixels;

  int imagePixelCount = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animImagePixelCount
    : ConfigManager::staticImagePixelCount;

  bool hasCustomImage = imagePixels != nullptr && imagePixelCount > 0;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    dma_display->clearScreen();
    if (currentMode == MODE_CLOCK && !hasCustomImage) {
      drawLogo(12, 18);
    }
    drawTinyTextCentered("--:--", clockConfig_timeY(), dma_display->color565(255, 255, 255));
    return;
  }

  static int s_lastMin = -1;
  if (!force && timeinfo.tm_min == s_lastMin) return;
  s_lastMin = timeinfo.tm_min;

  bool rebuildBackground = force;
  if (currentMode == MODE_CLOCK && !backgroundValid) {
    rebuildBackground = true;
  }

  if (rebuildBackground) {
    dma_display->clearScreen();
    memset(backgroundBuffer, 0, sizeof(backgroundBuffer));
    backgroundValid = true;
  }

  if (currentMode == MODE_CLOCK || currentMode == MODE_ANIMATION) {
    if (hasCustomImage) {
      drawPixels(imagePixels, imagePixelCount, false);
    } else if (currentMode == MODE_CLOCK) {
      drawLogo(12, 18);
      cacheLogoBackground(12, 18);
    }
  }

  // 叠加时钟文字（白色，由 clockConfig 控制）
  drawClockOverlay();
}


static int clockConfig_timeY() {
  return (DisplayManager::currentMode == MODE_ANIMATION)
    ? ConfigManager::animClockConfig.time.y
    : ConfigManager::clockConfig.time.y;
}

static void drawClockOverlayAnimationMode(const ClockConfig& c, const struct tm& timeinfo) {
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  DisplayManager::drawTinyTextCentered(
    timeStr,
    c.time.y,
    DisplayManager::dma_display->color565(c.time.r, c.time.g, c.time.b),
    c.time.fontSize
  );

  if (c.date.show) {
    char dateStr[6];
    sprintf(dateStr, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    DisplayManager::drawTinyTextCentered(
      dateStr,
      c.date.y,
      DisplayManager::dma_display->color565(c.date.r, c.date.g, c.date.b),
      c.date.fontSize
    );
  }

  if (c.week.show) {
    const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* weekStr = weekDays[timeinfo.tm_wday];
    DisplayManager::drawTinyTextCentered(
      weekStr,
      c.week.y,
      DisplayManager::dma_display->color565(c.week.r, c.week.g, c.week.b),
      1
    );
  }
}

static void drawStaticClockOverlayDirty(const ClockConfig& c, const struct tm& timeinfo) {
  static char s_prevTime[6] = "";
  static int s_prevTimeY = -1, s_prevTimeSize = 0;
  static char s_prevDate[6] = "";
  static int s_prevDateY = -1, s_prevDateSize = 0;
  static char s_prevWeek[4] = "";
  static int s_prevWeekY = -1;
  static DeviceMode s_prevMode = MODE_CANVAS;

  if (DisplayManager::currentMode != s_prevMode) {
    s_prevTime[0] = '\0';
    s_prevDate[0] = '\0';
    s_prevWeek[0] = '\0';
    s_prevTimeY = -1;
    s_prevDateY = -1;
    s_prevWeekY = -1;
    s_prevMode = DisplayManager::currentMode;
  }

  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  if (strcmp(s_prevTime, timeStr) != 0 || s_prevTimeY != c.time.y || s_prevTimeSize != c.time.fontSize) {
    int oldX = 0, oldW = 0, oldH = 0;
    int newX = 0, newW = 0, newH = 0;
    centeredTextBounds(s_prevTime, s_prevTimeY, s_prevTimeSize, oldX, oldW, oldH);
    centeredTextBounds(timeStr, c.time.y, c.time.fontSize, newX, newW, newH);
    restoreBackgroundRect(oldX, s_prevTimeY, oldW, oldH);
    restoreBackgroundRect(newX, c.time.y, newW, newH);
    DisplayManager::drawTinyTextCentered(timeStr, c.time.y,
      DisplayManager::dma_display->color565(c.time.r, c.time.g, c.time.b), c.time.fontSize);
    strcpy(s_prevTime, timeStr);
    s_prevTimeY = c.time.y;
    s_prevTimeSize = c.time.fontSize;
  }

  if (c.date.show) {
    char dateStr[6];
    sprintf(dateStr, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    if (strcmp(s_prevDate, dateStr) != 0 || s_prevDateY != c.date.y || s_prevDateSize != c.date.fontSize) {
      int oldX = 0, oldW = 0, oldH = 0;
      int newX = 0, newW = 0, newH = 0;
      centeredTextBounds(s_prevDate, s_prevDateY, s_prevDateSize, oldX, oldW, oldH);
      centeredTextBounds(dateStr, c.date.y, c.date.fontSize, newX, newW, newH);
      restoreBackgroundRect(oldX, s_prevDateY, oldW, oldH);
      restoreBackgroundRect(newX, c.date.y, newW, newH);
      DisplayManager::drawTinyTextCentered(dateStr, c.date.y,
        DisplayManager::dma_display->color565(c.date.r, c.date.g, c.date.b), c.date.fontSize);
      strcpy(s_prevDate, dateStr);
      s_prevDateY = c.date.y;
      s_prevDateSize = c.date.fontSize;
    }
  } else if (s_prevDate[0]) {
    int oldX = 0, oldW = 0, oldH = 0;
    centeredTextBounds(s_prevDate, s_prevDateY, s_prevDateSize, oldX, oldW, oldH);
    restoreBackgroundRect(oldX, s_prevDateY, oldW, oldH);
    s_prevDate[0] = '\0';
  }

  if (c.week.show) {
    const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* weekStr = weekDays[timeinfo.tm_wday];
    if (strcmp(s_prevWeek, weekStr) != 0 || s_prevWeekY != c.week.y) {
      int oldX = 0, oldW = 0, oldH = 0;
      int newX = 0, newW = 0, newH = 0;
      centeredTextBounds(s_prevWeek, s_prevWeekY, 1, oldX, oldW, oldH);
      centeredTextBounds(weekStr, c.week.y, 1, newX, newW, newH);
      restoreBackgroundRect(oldX, s_prevWeekY, oldW, oldH);
      restoreBackgroundRect(newX, c.week.y, newW, newH);
      DisplayManager::drawTinyTextCentered(weekStr, c.week.y,
        DisplayManager::dma_display->color565(c.week.r, c.week.g, c.week.b), 1);
      strncpy(s_prevWeek, weekStr, 3);
      s_prevWeek[3] = '\0';
      s_prevWeekY = c.week.y;
    }
  } else if (s_prevWeek[0]) {
    int oldX = 0, oldW = 0, oldH = 0;
    centeredTextBounds(s_prevWeek, s_prevWeekY, 1, oldX, oldW, oldH);
    restoreBackgroundRect(oldX, s_prevWeekY, oldW, oldH);
    s_prevWeek[0] = '\0';
  }
}

// 3x5 微型字体 (每字符5行，每行3bit: bit2=左, bit1=中, bit0=右)
static const uint8_t FONT3X5[][5] = {
  // 0-9 (index 0-9)
  {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
  {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7},
  // . (10), : (11), - (12), space (13)
  {0,0,0,0,2},{0,2,0,2,0},{0,0,7,0,0},{0,0,0,0,0},
  // A-Z (index 14-39)
  {2,5,7,5,5},{6,5,6,5,6},{7,4,4,4,7},{6,5,5,5,6},{7,4,7,4,7},
  {7,4,7,4,4},{7,4,5,5,7},{5,5,7,5,5},{7,2,2,2,7},{3,1,1,5,7},
  {5,6,4,6,5},{4,4,4,4,7},{5,7,5,5,5},{5,7,7,5,5},{7,5,5,5,7},
  {7,5,7,4,4},{7,5,5,7,1},{7,5,7,6,5},{7,4,7,1,7},{7,2,2,2,2},
  {5,5,5,5,7},{5,5,5,2,2},{5,5,5,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7},
};

static int fontIndex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c == '.') return 10;
  if (c == ':') return 11;
  if (c == '-') return 12;
  if (c == ' ') return 13;
  if (c >= 'A' && c <= 'Z') return 14 + (c - 'A');
  if (c >= 'a' && c <= 'z') return 14 + (c - 'a');
  return 13;
}

void DisplayManager::drawClockOverlay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  auto& c = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animClockConfig
    : ConfigManager::clockConfig;

  // 动态背景每帧都会重画底图，所以这里直接补画前景层，
  // 不做基于上一帧的擦除，避免文字和动态背景互相打架。
  if (currentMode == MODE_ANIMATION) {
    drawClockOverlayAnimationMode(c, timeinfo);
    return;
  }

  drawStaticClockOverlayDirty(c, timeinfo);
}

void DisplayManager::displayImage(uint8_t* data, size_t len, int width, int height) {
  dma_display->clearScreen();
  
  for (int y = 0; y < height && y < PANEL_RES_Y; y++) {
    for (int x = 0; x < width && x < PANEL_RES_X; x++) {
      int idx = (y * width + x) * 3;
      if (idx + 2 < len) {
        uint8_t r = data[idx];
        uint8_t g = data[idx + 1];
        uint8_t b = data[idx + 2];
        dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }
  }
}

void DisplayManager::clearScreen() {
  dma_display->clearScreen();
}

void DisplayManager::setBrightness(int brightness) {
  if (brightness >= 0 && brightness <= 255) {
    currentBrightness = brightness;
    dma_display->setBrightness8(brightness);
  }
}

void DisplayManager::drawTinyText(const char* text, int x, int y, uint16_t color, int size) {
  int cx = x;
  for (int i = 0; text[i]; i++) {
    int idx = fontIndex(text[i]);
    const uint8_t* glyph = FONT3X5[idx];
    for (int row = 0; row < 5; row++) {
      uint8_t bits = glyph[row];
      for (int sy = 0; sy < size; sy++) {
        for (int sx = 0; sx < size; sx++) {
          if (bits & 4) dma_display->drawPixel(cx + sx,          y + row * size + sy, color);
          if (bits & 2) dma_display->drawPixel(cx + size + sx,   y + row * size + sy, color);
          if (bits & 1) dma_display->drawPixel(cx + size*2 + sx, y + row * size + sy, color);
        }
      }
    }
    cx += 4 * size;
  }
}

void DisplayManager::drawTinyTextCentered(const char* text, int y, uint16_t color, int size) {
  int len = strlen(text);
  int width = len * 4 * size - size;
  int x = (64 - width) / 2;
  if (x < 0) x = 0;
  drawTinyText(text, x, y, color, size);
}
