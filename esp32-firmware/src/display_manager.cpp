#include "display_manager.h"
#include <time.h>

// 静态成员初始化
MatrixPanel_I2S_DMA* DisplayManager::dma_display = nullptr;
DeviceMode DisplayManager::currentMode = MODE_CANVAS;
int DisplayManager::currentBrightness = 50;
bool DisplayManager::clientConnected = false;
uint8_t DisplayManager::canvasBuffer[64][64][3];
bool DisplayManager::canvasInitialized = false;
bool DisplayManager::isCanvasMode = false;
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

// 独立背景绘制：清屏 + 画像素背景或 Logo，不涉及时钟文字
void DisplayManager::drawBackground() {
  PixelData* imagePixels = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animImagePixels
    : ConfigManager::staticImagePixels;
  int imagePixelCount = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animImagePixelCount
    : ConfigManager::staticImagePixelCount;

  dma_display->clearScreen();

  if (imagePixels != nullptr && imagePixelCount > 0) {
    for (int i = 0; i < imagePixelCount; i++) {
      PixelData& p = imagePixels[i];
      if (p.x < PANEL_RES_X && p.y < PANEL_RES_Y)
        dma_display->drawPixelRGB888(p.x, p.y, p.r, p.g, p.b);
      if (i % 100 == 0) yield();
    }
  } else {
    drawLogo(12, 18);
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

  if (force) dma_display->clearScreen();

  if (currentMode == MODE_CLOCK || currentMode == MODE_ANIMATION) {
    if (hasCustomImage) {
      for (int i = 0; i < imagePixelCount; i++) {
        PixelData& pixel = imagePixels[i];
        if (pixel.x < PANEL_RES_X && pixel.y < PANEL_RES_Y)
          dma_display->drawPixelRGB888(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
        if (i % 100 == 0) yield();
      }
    } else if (currentMode == MODE_CLOCK) {
      drawLogo(12, 18);
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

// 擦除文字中亮像素（只擦亮的点，不影响背景）
static void eraseTinyTextCentered(const char* text, int y, int size) {
  int len = strlen(text);
  int w = len * 4 * size - size;
  int x = (64 - w) / 2;
  if (x < 0) x = 0;
  int cx = x;
  for (int i = 0; text[i]; i++) {
    int idx = fontIndex(text[i]);
    const uint8_t* glyph = FONT3X5[idx];
    for (int row = 0; row < 5; row++) {
      uint8_t bits = glyph[row];
      for (int sy = 0; sy < size; sy++) {
        for (int sx = 0; sx < size; sx++) {
          int py = y + row * size + sy;
          if (bits & 4) DisplayManager::dma_display->drawPixel(cx + sx, py, 0);
          if (bits & 2) DisplayManager::dma_display->drawPixel(cx + size + sx, py, 0);
          if (bits & 1) DisplayManager::dma_display->drawPixel(cx + size*2 + sx, py, 0);
        }
      }
    }
    cx += 4 * size;
  }
}

// Diff 绘制：比较新旧字模，只清除"旧有新无"的像素，始终重绘新文字像素
static void drawTinyTextCenteredDiff(
    const char* oldText, int oldY, int oldSize,
    const char* newText, int newY, int newSize,
    uint16_t color) {
  int newLen = strlen(newText);
  int newW = newLen * 4 * newSize - newSize;
  int newX = (64 - newW) / 2;
  if (newX < 0) newX = 0;

  // 位置或大小变了：擦旧位置亮像素，画新位置
  if (oldText[0] && (oldY != newY || oldSize != newSize)) {
    eraseTinyTextCentered(oldText, oldY, oldSize);
    DisplayManager::drawTinyText(newText, newX, newY, color, newSize);
    return;
  }

  // 逐字符 diff
  int oldLen = strlen(oldText);
  int cx = newX;
  for (int i = 0; i < newLen; i++) {
    char oldCh = (i < oldLen) ? oldText[i] : ' ';
    char newCh = newText[i];
    const uint8_t* oldGlyph = FONT3X5[fontIndex(oldCh)];
    const uint8_t* newGlyph = FONT3X5[fontIndex(newCh)];

    for (int row = 0; row < 5; row++) {
      uint8_t oldBits = oldGlyph[row];
      uint8_t newBits = newGlyph[row];
      uint8_t clearBits = oldBits & ~newBits; // 旧有新无，需清除

      for (int sy = 0; sy < newSize; sy++) {
        for (int sx = 0; sx < newSize; sx++) {
          int py = newY + row * newSize + sy;
          // 清除旧有新无的像素
          if (clearBits & 4) DisplayManager::dma_display->drawPixel(cx + sx, py, 0);
          if (clearBits & 2) DisplayManager::dma_display->drawPixel(cx + newSize + sx, py, 0);
          if (clearBits & 1) DisplayManager::dma_display->drawPixel(cx + newSize*2 + sx, py, 0);
          // 始终重绘新文字像素（GIF 帧可能覆盖了文字）
          if (newBits & 4) DisplayManager::dma_display->drawPixel(cx + sx, py, color);
          if (newBits & 2) DisplayManager::dma_display->drawPixel(cx + newSize + sx, py, color);
          if (newBits & 1) DisplayManager::dma_display->drawPixel(cx + newSize*2 + sx, py, color);
        }
      }
    }
    cx += 4 * newSize;
  }
}

void DisplayManager::drawClockOverlay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  auto& c = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animClockConfig
    : ConfigManager::clockConfig;

  static char s_prevTime[6] = "";
  static int s_prevTimeY = -1, s_prevTimeSize = 0;
  static char s_prevDate[6] = "";
  static int s_prevDateY = -1, s_prevDateSize = 0;
  static char s_prevWeek[4] = "";
  static int s_prevWeekY = -1;
  static DeviceMode s_prevMode = MODE_CANVAS;

  // 模式切换时重置缓存，避免跨模式擦除错误位置
  if (currentMode != s_prevMode) {
    s_prevTime[0] = '\0';
    s_prevDate[0] = '\0';
    s_prevWeek[0] = '\0';
    s_prevTimeY = -1;
    s_prevDateY = -1;
    s_prevWeekY = -1;
    s_prevMode = currentMode;
  }

  // 时间
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  drawTinyTextCenteredDiff(s_prevTime, s_prevTimeY, s_prevTimeSize,
    timeStr, c.time.y, c.time.fontSize,
    dma_display->color565(c.time.r, c.time.g, c.time.b));
  strcpy(s_prevTime, timeStr);
  s_prevTimeY = c.time.y;
  s_prevTimeSize = c.time.fontSize;

  // 日期
  if (c.date.show) {
    char dateStr[6];
    sprintf(dateStr, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    drawTinyTextCenteredDiff(s_prevDate, s_prevDateY, s_prevDateSize,
      dateStr, c.date.y, c.date.fontSize,
      dma_display->color565(c.date.r, c.date.g, c.date.b));
    strcpy(s_prevDate, dateStr);
    s_prevDateY = c.date.y;
    s_prevDateSize = c.date.fontSize;
  } else if (s_prevDate[0]) {
    eraseTinyTextCentered(s_prevDate, s_prevDateY, s_prevDateSize);
    s_prevDate[0] = '\0';
  }

  // 星期
  if (c.week.show) {
    const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* weekStr = weekDays[timeinfo.tm_wday];
    drawTinyTextCenteredDiff(s_prevWeek, s_prevWeekY, 1,
      weekStr, c.week.y, 1,
      dma_display->color565(c.week.r, c.week.g, c.week.b));
    strncpy(s_prevWeek, weekStr, 3);
    s_prevWeek[3] = '\0';
    s_prevWeekY = c.week.y;
  } else if (s_prevWeek[0]) {
    eraseTinyTextCentered(s_prevWeek, s_prevWeekY, 1);
    s_prevWeek[0] = '\0';
  }
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