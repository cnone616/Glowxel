#include "display_manager.h"
#include "clock_font_renderer.h"
#include "eyes_effect.h"
#include <time.h>
#include <math.h>

// 静态成员初始化
MatrixPanel_I2S_DMA* DisplayManager::dma_display = nullptr;
DeviceMode DisplayManager::currentMode = MODE_CANVAS;
DeviceMode DisplayManager::lastBusinessMode = MODE_CLOCK;
String DisplayManager::currentBusinessModeTag = "clock";
String DisplayManager::lastBusinessModeTag = "clock";
NativeEffectType DisplayManager::nativeEffectType = NATIVE_EFFECT_NONE;
BreathEffectConfig DisplayManager::breathEffectConfig = {5, true, 10, 100, 1800, BREATH_WAVE_SINE, 100, 200, 255};
RhythmEffectConfig DisplayManager::rhythmEffectConfig = {120, 5, true, RHYTHM_DIR_LEFT, 70, RHYTHM_MODE_PULSE, 100, 200, 255, 255, 100, 100};
unsigned long DisplayManager::nativeEffectStartTime = 0;
int DisplayManager::currentBrightness = 50;
bool DisplayManager::clientConnected = false;
uint8_t DisplayManager::canvasBuffer[64][64][3];
uint16_t DisplayManager::backgroundBuffer[64][64];
uint16_t DisplayManager::animationBuffer[64][64];
bool DisplayManager::canvasInitialized = false;
bool DisplayManager::backgroundValid = false;
bool DisplayManager::animationBufferValid = false;
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
static void clockTextBounds(const char* text, int x, int y, uint8_t fontId, int size, int& outX, int& outW, int& outH);
static void drawStaticClockOverlayDirty(const ClockConfig& c, const struct tm& timeinfo);
static void cacheLogoBackground(int x, int y);
static void formatTimeText(const ClockConfig& c, const struct tm& timeinfo, char* buffer, size_t bufferSize);
static void formatDateText(const struct tm& timeinfo, char* buffer, size_t bufferSize);
static void formatWeekText(const struct tm& timeinfo, char* buffer, size_t bufferSize);

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

static void clockTextBounds(const char* text, int x, int y, uint8_t fontId, int size, int& outX, int& outW, int& outH) {
  outX = x;
  outW = getClockTextWidth(text, fontId, size);
  outH = getClockTextHeight(fontId, size);
  (void)y;
}

static void formatTimeText(const ClockConfig& c, const struct tm& timeinfo, char* buffer, size_t bufferSize) {
  int hours = timeinfo.tm_hour;
  if (c.hourFormat == 12) {
    hours = hours % 12;
    if (hours == 0) {
      hours = 12;
    }
  }

  if (c.showSeconds) {
    snprintf(buffer, bufferSize, "%02d:%02d:%02d", hours, timeinfo.tm_min, timeinfo.tm_sec);
    return;
  }

  snprintf(buffer, bufferSize, "%02d:%02d", hours, timeinfo.tm_min);
}

static void formatDateText(const struct tm& timeinfo, char* buffer, size_t bufferSize) {
  snprintf(buffer, bufferSize, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
}

static void formatWeekText(const struct tm& timeinfo, char* buffer, size_t bufferSize) {
  const char* weekDays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  snprintf(buffer, bufferSize, "%s", weekDays[timeinfo.tm_wday]);
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
  if (clearFirst || !animationBufferValid) {
    memset(animationBuffer, 0, sizeof(animationBuffer));
    animationBufferValid = true;
  }

  if (pixels != nullptr && pixelCount > 0) {
    for (int i = 0; i < pixelCount; i++) {
      const PixelData& p = pixels[i];
      if (p.x < PANEL_RES_X && p.y < PANEL_RES_Y) {
        animationBuffer[p.y][p.x] = dma_display->color565(p.r, p.g, p.b);
      }
      if (i % 100 == 0) {
        yield();
      }
    }
  }

  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      uint16_t color = animationBuffer[y][x];
      dma_display->drawPixel(x, y, color);
      backgroundBuffer[y][x] = color;
    }
    if (y % 8 == 0) {
      yield();
    }
  }

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

  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      uint16_t color = dma_display->color565(
        blended[y][x][0],
        blended[y][x][1],
        blended[y][x][2]
      );
      dma_display->drawPixel(x, y, color);
      animationBuffer[y][x] = color;
      backgroundBuffer[y][x] = color;
    }
    if (y % 8 == 0) {
      yield();
    }
  }
  animationBufferValid = true;

  drawClockOverlay();
}

void DisplayManager::renderAnimationTransitionBuffers(
    const uint16_t* fromBuffer,
    const uint16_t* toBuffer,
    uint8_t mix) {
  if (fromBuffer == nullptr || toBuffer == nullptr) {
    return;
  }

  int fromWeight = 255 - mix;
  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      int pos = y * PANEL_RES_X + x;
      uint16_t fromColor = fromBuffer[pos];
      uint16_t toColor = toBuffer[pos];

      uint8_t fromR = (fromColor >> 11) & 0x1F;
      uint8_t fromG = (fromColor >> 5) & 0x3F;
      uint8_t fromB = fromColor & 0x1F;
      uint8_t toR = (toColor >> 11) & 0x1F;
      uint8_t toG = (toColor >> 5) & 0x3F;
      uint8_t toB = toColor & 0x1F;

      uint8_t blendR = ((uint16_t)fromR * fromWeight + (uint16_t)toR * mix) / 255;
      uint8_t blendG = ((uint16_t)fromG * fromWeight + (uint16_t)toG * mix) / 255;
      uint8_t blendB = ((uint16_t)fromB * fromWeight + (uint16_t)toB * mix) / 255;

      uint16_t color = (blendR << 11) | (blendG << 5) | blendB;
      dma_display->drawPixel(x, y, color);
      animationBuffer[y][x] = color;
      backgroundBuffer[y][x] = color;
    }
    if (y % 8 == 0) {
      yield();
    }
  }
  animationBufferValid = true;
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
    char placeholder[9];
    if (cfg.showSeconds) {
      snprintf(placeholder, sizeof(placeholder), "--:--:--");
    } else {
      snprintf(placeholder, sizeof(placeholder), "--:--");
    }
    dma_display->clearScreen();
    if (currentMode == MODE_CLOCK && !hasCustomImage) {
      drawLogo(12, 18);
    }
    if (cfg.time.show) {
      drawClockText(
        dma_display,
        placeholder,
        cfg.time.x,
        cfg.time.y,
        dma_display->color565(cfg.time.r, cfg.time.g, cfg.time.b),
        cfg.font,
        cfg.time.fontSize
      );
    }
    return;
  }

  static int s_lastMinuteOrSecond = -1;
  int tickValue = cfg.showSeconds ? timeinfo.tm_sec : timeinfo.tm_min;
  if (!force && tickValue == s_lastMinuteOrSecond) return;
  s_lastMinuteOrSecond = tickValue;

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
  if (c.time.show) {
    char timeStr[9];
    formatTimeText(c, timeinfo, timeStr, sizeof(timeStr));
    drawClockText(
      DisplayManager::dma_display,
      timeStr,
      c.time.x,
      c.time.y,
      DisplayManager::dma_display->color565(c.time.r, c.time.g, c.time.b),
      c.font,
      c.time.fontSize
    );
  }

  if (c.date.show) {
    char dateStr[6];
    formatDateText(timeinfo, dateStr, sizeof(dateStr));
    drawClockText(
      DisplayManager::dma_display,
      dateStr,
      c.date.x,
      c.date.y,
      DisplayManager::dma_display->color565(c.date.r, c.date.g, c.date.b),
      c.font,
      c.date.fontSize
    );
  }

  if (c.week.show) {
    char weekStr[4];
    formatWeekText(timeinfo, weekStr, sizeof(weekStr));
    drawClockText(
      DisplayManager::dma_display,
      weekStr,
      c.week.x,
      c.week.y,
      DisplayManager::dma_display->color565(c.week.r, c.week.g, c.week.b),
      c.font,
      1
    );
  }
}

static void drawStaticClockOverlayDirty(const ClockConfig& c, const struct tm& timeinfo) {
  static char s_prevTime[9] = "";
  static int s_prevTimeX = -1, s_prevTimeY = -1, s_prevTimeSize = 0, s_prevTimeFont = -1;
  static char s_prevDate[6] = "";
  static int s_prevDateX = -1, s_prevDateY = -1, s_prevDateSize = 0, s_prevDateFont = -1;
  static char s_prevWeek[4] = "";
  static int s_prevWeekX = -1, s_prevWeekY = -1, s_prevWeekFont = -1;
  static DeviceMode s_prevMode = MODE_CANVAS;

  if (DisplayManager::currentMode != s_prevMode) {
    s_prevTime[0] = '\0';
    s_prevDate[0] = '\0';
    s_prevWeek[0] = '\0';
    s_prevTimeX = -1;
    s_prevTimeY = -1;
    s_prevDateX = -1;
    s_prevDateY = -1;
    s_prevWeekX = -1;
    s_prevWeekY = -1;
    s_prevMode = DisplayManager::currentMode;
  }

  if (c.time.show) {
    char timeStr[9];
    formatTimeText(c, timeinfo, timeStr, sizeof(timeStr));
    if (strcmp(s_prevTime, timeStr) != 0 ||
        s_prevTimeX != c.time.x ||
        s_prevTimeY != c.time.y ||
        s_prevTimeSize != c.time.fontSize ||
        s_prevTimeFont != c.font) {
      int oldX = 0, oldW = 0, oldH = 0;
      int newX = 0, newW = 0, newH = 0;
      clockTextBounds(s_prevTime, s_prevTimeX, s_prevTimeY, s_prevTimeFont < 0 ? c.font : s_prevTimeFont, s_prevTimeSize, oldX, oldW, oldH);
      clockTextBounds(timeStr, c.time.x, c.time.y, c.font, c.time.fontSize, newX, newW, newH);
      restoreBackgroundRect(oldX, s_prevTimeY, oldW, oldH);
      restoreBackgroundRect(newX, c.time.y, newW, newH);
      drawClockText(
        DisplayManager::dma_display,
        timeStr,
        c.time.x,
        c.time.y,
        DisplayManager::dma_display->color565(c.time.r, c.time.g, c.time.b),
        c.font,
        c.time.fontSize
      );
      strcpy(s_prevTime, timeStr);
      s_prevTimeX = c.time.x;
      s_prevTimeY = c.time.y;
      s_prevTimeSize = c.time.fontSize;
      s_prevTimeFont = c.font;
    }
  } else if (s_prevTime[0]) {
    int oldX = 0, oldW = 0, oldH = 0;
    clockTextBounds(s_prevTime, s_prevTimeX, s_prevTimeY, s_prevTimeFont < 0 ? c.font : s_prevTimeFont, s_prevTimeSize, oldX, oldW, oldH);
    restoreBackgroundRect(oldX, s_prevTimeY, oldW, oldH);
    s_prevTime[0] = '\0';
    s_prevTimeX = -1;
    s_prevTimeY = -1;
    s_prevTimeSize = 0;
    s_prevTimeFont = -1;
  }

  if (c.date.show) {
    char dateStr[6];
    formatDateText(timeinfo, dateStr, sizeof(dateStr));
    if (strcmp(s_prevDate, dateStr) != 0 ||
        s_prevDateX != c.date.x ||
        s_prevDateY != c.date.y ||
        s_prevDateSize != c.date.fontSize ||
        s_prevDateFont != c.font) {
      int oldX = 0, oldW = 0, oldH = 0;
      int newX = 0, newW = 0, newH = 0;
      clockTextBounds(s_prevDate, s_prevDateX, s_prevDateY, s_prevDateFont < 0 ? c.font : s_prevDateFont, s_prevDateSize, oldX, oldW, oldH);
      clockTextBounds(dateStr, c.date.x, c.date.y, c.font, c.date.fontSize, newX, newW, newH);
      restoreBackgroundRect(oldX, s_prevDateY, oldW, oldH);
      restoreBackgroundRect(newX, c.date.y, newW, newH);
      drawClockText(
        DisplayManager::dma_display,
        dateStr,
        c.date.x,
        c.date.y,
        DisplayManager::dma_display->color565(c.date.r, c.date.g, c.date.b),
        c.font,
        c.date.fontSize
      );
      strcpy(s_prevDate, dateStr);
      s_prevDateX = c.date.x;
      s_prevDateY = c.date.y;
      s_prevDateSize = c.date.fontSize;
      s_prevDateFont = c.font;
    }
  } else if (s_prevDate[0]) {
    int oldX = 0, oldW = 0, oldH = 0;
    clockTextBounds(s_prevDate, s_prevDateX, s_prevDateY, s_prevDateFont < 0 ? c.font : s_prevDateFont, s_prevDateSize, oldX, oldW, oldH);
    restoreBackgroundRect(oldX, s_prevDateY, oldW, oldH);
    s_prevDate[0] = '\0';
    s_prevDateX = -1;
    s_prevDateY = -1;
    s_prevDateSize = 0;
    s_prevDateFont = -1;
  }

  if (c.week.show) {
    char weekStr[4];
    formatWeekText(timeinfo, weekStr, sizeof(weekStr));
    if (strcmp(s_prevWeek, weekStr) != 0 ||
        s_prevWeekX != c.week.x ||
        s_prevWeekY != c.week.y ||
        s_prevWeekFont != c.font) {
      int oldX = 0, oldW = 0, oldH = 0;
      int newX = 0, newW = 0, newH = 0;
      clockTextBounds(s_prevWeek, s_prevWeekX, s_prevWeekY, s_prevWeekFont < 0 ? c.font : s_prevWeekFont, 1, oldX, oldW, oldH);
      clockTextBounds(weekStr, c.week.x, c.week.y, c.font, 1, newX, newW, newH);
      restoreBackgroundRect(oldX, s_prevWeekY, oldW, oldH);
      restoreBackgroundRect(newX, c.week.y, newW, newH);
      drawClockText(
        DisplayManager::dma_display,
        weekStr,
        c.week.x,
        c.week.y,
        DisplayManager::dma_display->color565(c.week.r, c.week.g, c.week.b),
        c.font,
        1
      );
      strncpy(s_prevWeek, weekStr, 3);
      s_prevWeek[3] = '\0';
      s_prevWeekX = c.week.x;
      s_prevWeekY = c.week.y;
      s_prevWeekFont = c.font;
    }
  } else if (s_prevWeek[0]) {
    int oldX = 0, oldW = 0, oldH = 0;
    clockTextBounds(s_prevWeek, s_prevWeekX, s_prevWeekY, s_prevWeekFont < 0 ? c.font : s_prevWeekFont, 1, oldX, oldW, oldH);
    restoreBackgroundRect(oldX, s_prevWeekY, oldW, oldH);
    s_prevWeek[0] = '\0';
    s_prevWeekX = -1;
    s_prevWeekY = -1;
    s_prevWeekFont = -1;
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

void DisplayManager::setNativeEffectNone() {
  nativeEffectType = NATIVE_EFFECT_NONE;
}

void DisplayManager::activateBreathEffect(const BreathEffectConfig& config) {
  breathEffectConfig = config;
  nativeEffectType = NATIVE_EFFECT_BREATH;
  nativeEffectStartTime = millis();
}

void DisplayManager::activateRhythmEffect(const RhythmEffectConfig& config) {
  rhythmEffectConfig = config;
  nativeEffectType = NATIVE_EFFECT_RHYTHM;
  nativeEffectStartTime = millis();
}

void DisplayManager::activateEyesEffect(const EyesConfig& config) {
  EyesEffect::applyConfig(config);
  nativeEffectType = NATIVE_EFFECT_EYES;
  nativeEffectStartTime = millis();
}

static uint8_t clampByte(int value) {
  if (value < 0) return 0;
  if (value > 255) return 255;
  return (uint8_t)value;
}

void DisplayManager::renderNativeEffect() {
  if (nativeEffectType == NATIVE_EFFECT_NONE) {
    return;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - nativeEffectStartTime;

  if (nativeEffectType == NATIVE_EFFECT_BREATH) {
    if (!breathEffectConfig.loop && elapsed >= breathEffectConfig.periodMs) {
      setNativeEffectNone();
      return;
    }

    float phase = 0.0f;
    if (breathEffectConfig.periodMs > 0) {
      phase = (float)(elapsed % breathEffectConfig.periodMs) / (float)breathEffectConfig.periodMs;
    }

    float level = 0.0f;
    if (breathEffectConfig.waveform == BREATH_WAVE_SINE) {
      level = (sinf(phase * 2.0f * 3.1415926f - 3.1415926f / 2.0f) + 1.0f) * 0.5f;
    } else if (breathEffectConfig.waveform == BREATH_WAVE_TRIANGLE) {
      level = phase < 0.5f ? (phase * 2.0f) : (2.0f - phase * 2.0f);
    } else {
      level = phase < 0.5f ? 1.0f : 0.0f;
    }

    int minB = breathEffectConfig.minBrightness;
    int maxB = breathEffectConfig.maxBrightness;
    int brightness = minB + (int)((maxB - minB) * level);

    uint8_t r = clampByte((breathEffectConfig.colorR * brightness) / 100);
    uint8_t g = clampByte((breathEffectConfig.colorG * brightness) / 100);
    uint8_t b = clampByte((breathEffectConfig.colorB * brightness) / 100);

    dma_display->fillRect(0, 0, PANEL_RES_X, PANEL_RES_Y, dma_display->color565(r, g, b));
    return;
  }

  if (nativeEffectType == NATIVE_EFFECT_RHYTHM) {
    if (!rhythmEffectConfig.loop && elapsed >= 60000UL) {
      setNativeEffectNone();
      return;
    }

    int bpm = rhythmEffectConfig.bpm <= 0 ? 120 : rhythmEffectConfig.bpm;
    unsigned long beatInterval = 60000UL / (unsigned long)bpm;
    if (beatInterval == 0) {
      beatInterval = 1;
    }

    float beatPhase = (float)(elapsed % beatInterval) / (float)beatInterval;
    float pulse = 1.0f - beatPhase;
    if (pulse < 0.0f) pulse = 0.0f;

    int strength = rhythmEffectConfig.strength;
    if (strength < 0) strength = 0;
    if (strength > 100) strength = 100;

    if (rhythmEffectConfig.mode == RHYTHM_MODE_JUMP) {
      bool useA = ((elapsed / beatInterval) % 2) == 0;
      uint8_t r = useA ? rhythmEffectConfig.colorAR : rhythmEffectConfig.colorBR;
      uint8_t g = useA ? rhythmEffectConfig.colorAG : rhythmEffectConfig.colorBG;
      uint8_t b = useA ? rhythmEffectConfig.colorAB : rhythmEffectConfig.colorBB;
      int scaled = (strength * 255) / 100;
      dma_display->fillRect(
        0,
        0,
        PANEL_RES_X,
        PANEL_RES_Y,
        dma_display->color565((r * scaled) / 255, (g * scaled) / 255, (b * scaled) / 255)
      );
      return;
    }

    int speed = rhythmEffectConfig.speed;
    if (speed < 1) speed = 1;
    if (speed > 10) speed = 10;

    for (int y = 0; y < PANEL_RES_Y; y++) {
      for (int x = 0; x < PANEL_RES_X; x++) {
        float position = 0.0f;
        if (rhythmEffectConfig.direction == RHYTHM_DIR_LEFT) {
          position = (float)(x + (elapsed / (12UL - speed))) / (float)PANEL_RES_X;
        } else if (rhythmEffectConfig.direction == RHYTHM_DIR_RIGHT) {
          position = (float)((PANEL_RES_X - 1 - x) + (elapsed / (12UL - speed))) / (float)PANEL_RES_X;
        } else if (rhythmEffectConfig.direction == RHYTHM_DIR_UP) {
          position = (float)(y + (elapsed / (12UL - speed))) / (float)PANEL_RES_Y;
        } else {
          position = (float)((PANEL_RES_Y - 1 - y) + (elapsed / (12UL - speed))) / (float)PANEL_RES_Y;
        }

        float frac = position - floorf(position);
        float pulseWeight = rhythmEffectConfig.mode == RHYTHM_MODE_PULSE ? pulse : 1.0f;
        float mix = frac;

        int r = (int)(rhythmEffectConfig.colorAR * (1.0f - mix) + rhythmEffectConfig.colorBR * mix);
        int g = (int)(rhythmEffectConfig.colorAG * (1.0f - mix) + rhythmEffectConfig.colorBG * mix);
        int b = (int)(rhythmEffectConfig.colorAB * (1.0f - mix) + rhythmEffectConfig.colorBB * mix);

        int scaled = (int)(strength * pulseWeight * 2.55f);
        if (scaled > 255) scaled = 255;

        dma_display->drawPixelRGB888(
          x,
          y,
          clampByte((r * scaled) / 255),
          clampByte((g * scaled) / 255),
          clampByte((b * scaled) / 255)
        );
      }
    }
    return;
  }

  if (nativeEffectType == NATIVE_EFFECT_EYES) {
    EyesEffect::render();
  }
}
