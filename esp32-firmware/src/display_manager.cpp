#include "display_manager.h"
#include "clock_font_renderer.h"
#include "eyes_effect.h"
#include "theme_renderer.h"
#include "wifi_manager.h"
#include <time.h>
#include <math.h>

// 静态成员初始化
CaptureMatrixPanel* DisplayManager::dma_display = nullptr;
DeviceMode DisplayManager::currentMode = MODE_CANVAS;
DeviceMode DisplayManager::lastBusinessMode = MODE_CLOCK;
String DisplayManager::currentBusinessModeTag = "clock";
String DisplayManager::lastBusinessModeTag = "clock";
NativeEffectType DisplayManager::nativeEffectType = NATIVE_EFFECT_NONE;
BreathEffectConfig DisplayManager::breathEffectConfig = {
  5,
  true,
  BREATH_MOTION_CLOCKWISE,
  BREATH_SCOPE_SINGLE_RING,
  BREATH_COLOR_SOLID,
  100,
  200,
  255,
  255,
  255,
  255
};
RhythmEffectConfig DisplayManager::rhythmEffectConfig = {120, 5, true, RHYTHM_DIR_LEFT, 70, RHYTHM_MODE_PULSE, 100, 200, 255, 255, 100, 100};
AmbientEffectConfig DisplayManager::ambientEffectConfig = {AMBIENT_PRESET_AURORA, 6, 72, true};
unsigned long DisplayManager::nativeEffectStartTime = 0;
int DisplayManager::currentBrightness = 50;
bool DisplayManager::clientConnected = false;
uint8_t DisplayManager::canvasBuffer[64][64][3];
uint16_t DisplayManager::backgroundBuffer[64][64];
uint16_t DisplayManager::animationBuffer[64][64];
uint16_t DisplayManager::liveFrameBuffer[64][64];
bool DisplayManager::canvasInitialized = false;
bool DisplayManager::backgroundValid = false;
bool DisplayManager::animationBufferValid = false;
bool DisplayManager::liveFrameValid = false;
bool DisplayManager::doubleBufferEnabled = false;
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

static BufferMatrixPanel* s_offscreenDisplay = nullptr;

static void presentFrame() {
  if (DisplayManager::dma_display == nullptr) {
    return;
  }
  DisplayManager::dma_display->flipDMABuffer();
}

void CaptureMatrixPanel::drawPixel(int16_t x, int16_t y, uint16_t color) {
  MatrixPanel_I2S_DMA::drawPixel(x, y, color);
  DisplayManager::writeLiveFramePixel565(x, y, color);
}

void CaptureMatrixPanel::fillScreen(uint16_t color) {
  MatrixPanel_I2S_DMA::fillScreen(color);
  DisplayManager::clearLiveFrame(color);
}

void CaptureMatrixPanel::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
  MatrixPanel_I2S_DMA::drawPixelRGB888(x, y, r, g, b);
  DisplayManager::writeLiveFramePixelRGB888(x, y, r, g, b);
}

void CaptureMatrixPanel::fillScreenRGB888(uint8_t r, uint8_t g, uint8_t b) {
  MatrixPanel_I2S_DMA::fillScreenRGB888(r, g, b);
  DisplayManager::clearLiveFrame(MatrixPanel_I2S_DMA::color565(r, g, b));
}

void CaptureMatrixPanel::clearScreen() {
  MatrixPanel_I2S_DMA::clearScreen();
  DisplayManager::clearLiveFrame(0);
}

void BufferMatrixPanel::setTargetBuffer(uint16_t* buffer, int width, int height) {
  targetBuffer = buffer;
  targetWidth = width;
  targetHeight = height;
}

void BufferMatrixPanel::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (targetBuffer == nullptr) {
    return;
  }
  if (x < 0 || x >= targetWidth || y < 0 || y >= targetHeight) {
    return;
  }
  targetBuffer[y * targetWidth + x] = color;
}

void BufferMatrixPanel::fillScreen(uint16_t color) {
  if (targetBuffer == nullptr) {
    return;
  }
  for (int y = 0; y < targetHeight; y++) {
    for (int x = 0; x < targetWidth; x++) {
      targetBuffer[y * targetWidth + x] = color;
    }
  }
}

void BufferMatrixPanel::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
  drawPixel(x, y, MatrixPanel_I2S_DMA::color565(r, g, b));
}

void BufferMatrixPanel::clearScreen() {
  fillScreen(0);
}

void DisplayManager::clearLiveFrame(uint16_t color) {
  liveFrameValid = true;
  if (color == 0) {
    memset(liveFrameBuffer, 0, sizeof(liveFrameBuffer));
    return;
  }

  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      liveFrameBuffer[y][x] = color;
    }
  }
}

void DisplayManager::writeLiveFramePixel565(int x, int y, uint16_t color) {
  if (x < 0 || x >= PANEL_RES_X || y < 0 || y >= PANEL_RES_Y) {
    return;
  }
  liveFrameBuffer[y][x] = color;
  liveFrameValid = true;
}

void DisplayManager::writeLiveFramePixelRGB888(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  writeLiveFramePixel565(x, y, MatrixPanel_I2S_DMA::color565(r, g, b));
}

MatrixPanel_I2S_DMA* DisplayManager::beginOffscreenFrame(uint16_t* targetBuffer, uint16_t clearColor) {
  if (targetBuffer == nullptr) {
    return nullptr;
  }

  if (s_offscreenDisplay == nullptr) {
    HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
    mxconfig.gpio.e = 18;
    mxconfig.clkphase = false;
    s_offscreenDisplay = new BufferMatrixPanel(mxconfig);
  }

  if (s_offscreenDisplay == nullptr) {
    return nullptr;
  }

  s_offscreenDisplay->setTargetBuffer(targetBuffer, PANEL_RES_X, PANEL_RES_Y);
  s_offscreenDisplay->fillScreen(clearColor);
  s_offscreenDisplay->setTextWrap(true);
  s_offscreenDisplay->setCursor(0, 0);
  s_offscreenDisplay->setTextSize(1);
  s_offscreenDisplay->setFont(nullptr);
  return s_offscreenDisplay;
}

void DisplayManager::presentOffscreenFrame(const uint16_t* targetBuffer) {
  if (dma_display == nullptr || targetBuffer == nullptr) {
    return;
  }

  bool changed = false;
  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      uint16_t nextColor = targetBuffer[y * PANEL_RES_X + x];
      if (liveFrameValid && liveFrameBuffer[y][x] == nextColor) {
        continue;
      }
      dma_display->drawPixel(x, y, nextColor);
      changed = true;
    }
  }

  if (!liveFrameValid) {
    liveFrameValid = true;
  }
  if (changed) {
    presentFrame();
  }
}

void DisplayManager::init() {
  Serial.println("1. 初始化LED灯板...");
  setupMatrix();
}

void DisplayManager::setupMatrix() {
  rebuildMatrix(false, true);
}

bool DisplayManager::rebuildMatrix(bool doubleBuffered, bool showBootLogo) {
  if (dma_display != nullptr) {
    delete dma_display;
    dma_display = nullptr;
  }

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  mxconfig.gpio.e = 18;
  mxconfig.clkphase = false;
  mxconfig.double_buff = doubleBuffered;

  CaptureMatrixPanel* nextDisplay = new CaptureMatrixPanel(mxconfig);
  if (nextDisplay == nullptr) {
    Serial.println("显示实例创建失败");
    return false;
  }
  if (!nextDisplay->begin()) {
    Serial.printf("显示初始化失败，double buffer=%d\n", doubleBuffered ? 1 : 0);
    delete nextDisplay;
    return false;
  }

  dma_display = nextDisplay;
  doubleBufferEnabled = doubleBuffered;
  dma_display->setBrightness8(51);
  dma_display->setLatBlanking(2);
  dma_display->clearScreen();

  if (!showBootLogo) {
    return true;
  }

  Serial.println("显示开机Logo...");
  drawLogo(12, 7);  // 开机 logo 偏上
  // 开机 logo 额外显示品牌名
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(220, 220, 220));
  dma_display->setCursor(11, 52);
  dma_display->print("RenLight");
  presentFrame();
  delay(2000);
  Serial.println("LED灯板初始化完成");
  return true;
}

bool DisplayManager::enableDoubleBuffer() {
  if (doubleBufferEnabled) {
    return true;
  }

  Serial.println("尝试启用双缓冲显示...");
  bool ok = rebuildMatrix(true, false);
  if (ok) {
    Serial.println("双缓冲已启用");
    clearLiveFrame();
    backgroundValid = false;
    animationBufferValid = false;
    return true;
  }

  Serial.println("双缓冲启用失败，恢复单缓冲");
  bool fallbackOk = rebuildMatrix(false, false);
  if (fallbackOk) {
    clearLiveFrame();
    backgroundValid = false;
    animationBufferValid = false;
  }
  return false;
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
  presentFrame();
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
  presentFrame();
}

void DisplayManager::renderCanvas() {
  dma_display->clearScreen();

  if (!canvasInitialized) {
    drawLogo(12, 12);
    presentFrame();
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
  presentFrame();
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
    presentFrame();
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
  presentFrame();
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
  presentFrame();
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
  presentFrame();
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
  presentFrame();
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
  presentFrame();
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
    WiFiManager::showTimeSyncScreen();
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
  presentFrame();
}

void DisplayManager::displayTheme(bool force) {
  if (receivingPixels) {
    return;
  }

  static unsigned long s_lastThemeRenderAt = 0;
  unsigned long now = millis();
  uint16_t refreshInterval = getThemeRefreshIntervalMs(ConfigManager::themeConfig.themeId);
  if (!force && now - s_lastThemeRenderAt < refreshInterval) {
    return;
  }
  s_lastThemeRenderAt = now;

  struct tm timeinfo;
  struct tm* timePtr = getLocalTime(&timeinfo) ? &timeinfo : nullptr;

  const PixelData* imagePixels = nullptr;
  int imagePixelCount = 0;
  if (themeUsesStoredImage(ConfigManager::themeConfig.themeId)) {
    imagePixels = ConfigManager::staticImagePixels;
    imagePixelCount = ConfigManager::staticImagePixelCount;
  }

  backgroundValid = false;
  animationBufferValid = false;
  MatrixPanel_I2S_DMA* offscreen = beginOffscreenFrame(&animationBuffer[0][0], 0);
  if (offscreen == nullptr) {
    return;
  }
  renderThemeFrame(
    offscreen,
    ConfigManager::themeConfig.themeId,
    timePtr,
    imagePixels,
    imagePixelCount
  );
  presentOffscreenFrame(&animationBuffer[0][0]);
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

void DisplayManager::activateAmbientEffect(const AmbientEffectConfig& config) {
  ambientEffectConfig = config;
  nativeEffectType = NATIVE_EFFECT_AMBIENT;
  nativeEffectStartTime = millis();
}

static uint8_t clampByte(int value) {
  if (value < 0) return 0;
  if (value > 255) return 255;
  return (uint8_t)value;
}

static float clampUnit(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

static float fractf32(float value) {
  return value - floorf(value);
}

static float smoothstepf(float edge0, float edge1, float value) {
  if (edge0 == edge1) {
    return value < edge0 ? 0.0f : 1.0f;
  }
  float x = clampUnit((value - edge0) / (edge1 - edge0));
  return x * x * (3.0f - 2.0f * x);
}

static float colorLuminance255(uint8_t r, uint8_t g, uint8_t b) {
  return ((float)r * 0.2126f + (float)g * 0.7152f + (float)b * 0.0722f) / 255.0f;
}

static bool isWarmRhythmPalette(const RhythmEffectConfig& config) {
  float warmA = (float)config.colorAR + (float)config.colorAG * 0.4f - (float)config.colorAB * 0.2f;
  float warmB = (float)config.colorBR + (float)config.colorBG * 0.4f - (float)config.colorBB * 0.2f;
  return warmA + warmB > 290.0f;
}

static bool isWarmBreathPalette(const BreathEffectConfig& config) {
  float warmA = (float)config.colorAR + (float)config.colorAG * 0.4f - (float)config.colorAB * 0.2f;
  float warmB = (float)config.colorBR + (float)config.colorBG * 0.4f - (float)config.colorBB * 0.2f;
  return warmA + warmB > 290.0f;
}

static bool isMatrixRhythmPalette(const RhythmEffectConfig& config) {
  int greenLeadA = (int)config.colorAG - max((int)config.colorAR, (int)config.colorAB);
  int greenLeadB = (int)config.colorBG - max((int)config.colorBR, (int)config.colorBB);
  return greenLeadA > 20 || greenLeadB > 20;
}

static bool isMatrixBreathPalette(const BreathEffectConfig& config) {
  int greenLeadA = (int)config.colorAG - max((int)config.colorAR, (int)config.colorAB);
  int greenLeadB = (int)config.colorBG - max((int)config.colorBR, (int)config.colorBB);
  return greenLeadA > 20 || greenLeadB > 20;
}

static float columnSeedf(int index) {
  return fractf32(sinf((float)(index + 1) * 12.9898f) * 43758.5453f);
}

static int ringPathLength(int layer) {
  int left = layer;
  int top = layer;
  int right = 63 - layer;
  int bottom = 63 - layer;
  if (left > right || top > bottom) {
    return 0;
  }
  if (left == right && top == bottom) {
    return 1;
  }
  return ((right - left + 1) * 2 + (bottom - top + 1) * 2) - 4;
}

static bool ringPathIndex(int layer, int x, int y, int& outIndex) {
  int left = layer;
  int top = layer;
  int right = 63 - layer;
  int bottom = 63 - layer;
  if (x < left || x > right || y < top || y > bottom) {
    return false;
  }
  if (y == top) {
    outIndex = x - left;
    return true;
  }
  if (x == right && y > top) {
    outIndex = (right - left + 1) + (y - top) - 1;
    return true;
  }
  if (y == bottom && x < right) {
    outIndex = (right - left + 1) + (bottom - top) + (right - x) - 1;
    return true;
  }
  if (x == left && y > top && y < bottom) {
    outIndex = (right - left + 1) * 2 + (bottom - top) + (bottom - y) - 2;
    return true;
  }
  return false;
}

static unsigned long breathDurationMs(const BreathEffectConfig& config) {
  int base = config.motion == BREATH_MOTION_INWARD || config.motion == BREATH_MOTION_OUTWARD ? 2000 : 2400;
  int duration = base - (int)config.speed * 90;
  if (config.scope == BREATH_SCOPE_FULL_SCREEN) {
    duration += 220;
  }
  if (duration < 900) {
    duration = 900;
  }
  return (unsigned long)duration;
}

static void mixRgb(
  uint8_t ar,
  uint8_t ag,
  uint8_t ab,
  uint8_t br,
  uint8_t bg,
  uint8_t bb,
  float mix,
  float scale,
  uint8_t& outR,
  uint8_t& outG,
  uint8_t& outB
) {
  float safeMix = clampUnit(mix);
  float safeScale = clampUnit(scale);
  float r = ((float)ar * (1.0f - safeMix) + (float)br * safeMix) * safeScale;
  float g = ((float)ag * (1.0f - safeMix) + (float)bg * safeMix) * safeScale;
  float b = ((float)ab * (1.0f - safeMix) + (float)bb * safeMix) * safeScale;
  outR = clampByte((int)roundf(r));
  outG = clampByte((int)roundf(g));
  outB = clampByte((int)roundf(b));
}

static void addHighlight(uint8_t& r, uint8_t& g, uint8_t& b, float amount) {
  float safeAmount = clampUnit(amount);
  r = clampByte((int)roundf((float)r + (255.0f - (float)r) * safeAmount));
  g = clampByte((int)roundf((float)g + (255.0f - (float)g) * safeAmount));
  b = clampByte((int)roundf((float)b + (255.0f - (float)b) * safeAmount));
}

struct NativeRgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

static NativeRgb makeRgb(uint8_t r, uint8_t g, uint8_t b) {
  NativeRgb color = {r, g, b};
  return color;
}

static NativeRgb blendRgbColor(const NativeRgb& colorA, const NativeRgb& colorB, float alpha) {
  float safe = clampUnit(alpha);
  NativeRgb out = {
    clampByte((int)roundf((float)colorA.r * (1.0f - safe) + (float)colorB.r * safe)),
    clampByte((int)roundf((float)colorA.g * (1.0f - safe) + (float)colorB.g * safe)),
    clampByte((int)roundf((float)colorA.b * (1.0f - safe) + (float)colorB.b * safe))
  };
  return out;
}

static NativeRgb scaleRgbColor(const NativeRgb& color, float scale) {
  float safe = clampUnit(scale);
  NativeRgb out = {
    clampByte((int)roundf((float)color.r * safe)),
    clampByte((int)roundf((float)color.g * safe)),
    clampByte((int)roundf((float)color.b * safe))
  };
  return out;
}

static NativeRgb addHighlightColor(const NativeRgb& color, float amount) {
  NativeRgb out = color;
  addHighlight(out.r, out.g, out.b, amount);
  return out;
}

static void blendIntoPixel(uint8_t& dstR, uint8_t& dstG, uint8_t& dstB, const NativeRgb& src, float alpha) {
  float safe = clampUnit(alpha);
  if (safe <= 0.0f) {
    return;
  }
  dstR = clampByte((int)roundf((float)dstR * (1.0f - safe) + (float)src.r * safe));
  dstG = clampByte((int)roundf((float)dstG * (1.0f - safe) + (float)src.g * safe));
  dstB = clampByte((int)roundf((float)dstB * (1.0f - safe) + (float)src.b * safe));
}

static float rhythmAmplitudeAt(const RhythmEffectConfig& config, float phase, int speed, float strengthUnit, int band) {
  float seed = columnSeedf(band);
  float beatFlash = 0.5f + 0.5f * sinf(phase * 3.1415926f * (2.1f + (float)speed * 0.08f));
  float spectrumWave =
    0.5f +
    0.5f *
      sinf(
        phase * 3.1415926f * (2.3f + (float)speed * 0.07f) +
        (float)band * 0.44f +
        seed * 3.1f
      );
  float detailWave =
    0.5f +
    0.5f *
      cosf(
        phase * 3.1415926f * (4.7f + (float)speed * 0.05f) +
        (float)band * 0.7f +
        seed * 4.4f
      );

  float amplitude = 0.0f;
  if (config.mode == RHYTHM_MODE_PULSE) {
    amplitude = 0.16f + spectrumWave * 0.48f + detailWave * 0.16f + beatFlash * 0.22f;
  } else if (config.mode == RHYTHM_MODE_GRADIENT) {
    bool matrixPalette = isMatrixRhythmPalette(config);
    amplitude =
      0.14f +
      spectrumWave * 0.32f +
      detailWave * 0.18f +
      (matrixPalette ? beatFlash * 0.14f : beatFlash * 0.08f);
  } else {
    float gate = fractf32(phase * (1.25f + (float)speed * 0.05f) + seed);
    amplitude = gate > 0.48f ? 0.78f + detailWave * 0.16f : 0.08f + spectrumWave * 0.12f;
  }

  return clampUnit(amplitude * (0.38f + strengthUnit * 0.92f));
}

static int rhythmHeightFromAmplitude(float amplitude) {
  return max(4, (int)roundf(7.0f + amplitude * 43.0f));
}

static bool resolveRhythmLane(
  uint8_t direction,
  int x,
  int y,
  int& bandIndex,
  int& thickness,
  int& depth,
  bool& axisVertical,
  bool& reverseAxis
) {
  axisVertical = direction == RHYTHM_DIR_LEFT || direction == RHYTHM_DIR_RIGHT;
  reverseAxis = direction == RHYTHM_DIR_RIGHT || direction == RHYTHM_DIR_DOWN;

  if (axisVertical) {
    if (x < 2 || x > 61 || y < 2 || y > 61) {
      return false;
    }
    int lane = x - 2;
    bandIndex = lane / 4;
    thickness = lane % 4;
    depth = reverseAxis ? y - 2 : 61 - y;
  } else {
    if (x < 2 || x > 61 || y < 2 || y > 61) {
      return false;
    }
    int lane = y - 2;
    bandIndex = lane / 4;
    thickness = lane % 4;
    depth = reverseAxis ? 61 - x : x - 2;
  }

  if (bandIndex < 0 || bandIndex >= 16 || thickness < 0 || thickness > 2 || depth < 0 || depth > 59) {
    return false;
  }
  return true;
}

static float directionalPosition(uint8_t direction, int x, int y) {
  if (direction == RHYTHM_DIR_LEFT) {
    return (float)x / 63.0f;
  }
  if (direction == RHYTHM_DIR_RIGHT) {
    return 1.0f - (float)x / 63.0f;
  }
  if (direction == RHYTHM_DIR_UP) {
    return (float)y / 63.0f;
  }
  return 1.0f - (float)y / 63.0f;
}

static float glowPulsef(float timeValue, float offset) {
  return 0.5f + 0.5f * sinf(timeValue + offset);
}

static float fireflySeedf(int index, float shift) {
  return fractf32(sinf((float)(index + 1) * 17.371f + shift * 3.147f) * 24634.6345f);
}

void DisplayManager::renderNativeEffect() {
  if (nativeEffectType == NATIVE_EFFECT_NONE) {
    return;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - nativeEffectStartTime;

  if (nativeEffectType == NATIVE_EFFECT_BREATH) {
    unsigned long durationMs = breathDurationMs(breathEffectConfig);
    if (!breathEffectConfig.loop && elapsed >= durationMs) {
      setNativeEffectNone();
      return;
    }

    float phase = 0.0f;
    if (durationMs > 0) {
      phase = (float)(elapsed % durationMs) / (float)durationMs;
    }

    int speed = breathEffectConfig.speed;
    if (speed < 1) speed = 1;
    if (speed > 10) speed = 10;
    float breath = 0.45f + 0.55f * sinf(phase * 2.0f * 3.1415926f - 3.1415926f / 2.0f) * 0.5f + 0.275f;
    float timeBase = phase * 2.0f * 3.1415926f;
    bool warmPalette = isWarmBreathPalette(breathEffectConfig);
    bool matrixPalette = isMatrixBreathPalette(breathEffectConfig);
    bool gradientMode = breathEffectConfig.colorMode == BREATH_COLOR_GRADIENT;
    bool fullScreen = breathEffectConfig.scope == BREATH_SCOPE_FULL_SCREEN;

    for (int y = 0; y < PANEL_RES_Y; y++) {
      for (int x = 0; x < PANEL_RES_X; x++) {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        if (fullScreen && warmPalette) {
          float waveA = 0.5f + 0.5f * sinf((float)x * 0.22f + phase * 3.1415926f * (1.8f + (float)speed * 0.06f));
          float waveB = 0.5f + 0.5f * sinf((float)x * 0.11f - phase * 3.1415926f * (2.4f + (float)speed * 0.08f));
          float flameHeight = roundf(10.0f + waveA * 12.0f + waveB * 18.0f + breath * 8.0f);
          float depth = 63.0f - (float)y;
          if (depth >= 0.0f && depth < flameHeight) {
            float mix = clampUnit(depth / max(flameHeight - 1.0f, 1.0f));
            float alpha = clampUnit(0.22f + mix * 0.7f);
            uint8_t baseR = 0;
            uint8_t baseG = 0;
            uint8_t baseB = 0;
            mixRgb(
              breathEffectConfig.colorAR,
              breathEffectConfig.colorAG,
              breathEffectConfig.colorAB,
              breathEffectConfig.colorBR,
              breathEffectConfig.colorBG,
              breathEffectConfig.colorBB,
              clampUnit(mix * 0.85f),
              alpha,
              baseR,
              baseG,
              baseB
            );
            r = baseR;
            g = baseG;
            b = baseB;
            addHighlight(r, g, b, clampUnit(mix * 0.3f));
          }
          float sparkGate = fractf32(phase * (1.3f + (float)speed * 0.05f) + (float)x * 0.17f);
          if (sparkGate > 0.82f) {
            float sparkY = 40.0f - roundf((sparkGate - 0.82f) * 70.0f);
            float sparkGlow = clampUnit(1.0f - fabsf((float)y - sparkY) / 2.4f);
            if (sparkGlow > 0.0f) {
              addHighlight(r, g, b, sparkGlow * 0.42f);
            }
          }
        } else if (fullScreen && matrixPalette) {
          uint8_t accumR = 0;
          uint8_t accumG = 0;
          uint8_t accumB = 0;
          for (int column = 0; column < 16; column++) {
            float seed = columnSeedf(column);
            int laneX = column * 4 + 1;
            if (x < laneX || x > laneX + 2) {
              continue;
            }
            float travel = fractf32(phase * (0.75f + (float)speed * 0.08f) + seed);
            float headY = roundf(travel * 76.0f) - 6.0f;
            for (int trail = 0; trail < 14; trail++) {
              float py = headY - (float)trail;
              if (fabsf((float)y - py) > 0.6f) {
                continue;
              }
              float alpha = clampUnit((1.0f - (float)trail / 14.0f) * (0.4f + breath * 0.45f));
              if (x == laneX + 1) {
                alpha *= 0.72f;
              } else if (x == laneX + 2) {
                alpha *= trail < 4 ? 0.2f : 0.0f;
              }
              if (alpha <= 0.0f) {
                continue;
              }
              uint8_t laneR = 0;
              uint8_t laneG = 0;
              uint8_t laneB = 0;
              mixRgb(
                breathEffectConfig.colorAR,
                breathEffectConfig.colorAG,
                breathEffectConfig.colorAB,
                breathEffectConfig.colorBR,
                breathEffectConfig.colorBG,
                breathEffectConfig.colorBB,
                trail < 2 ? 0.3f : 0.65f,
                alpha,
                laneR,
                laneG,
                laneB
              );
              if (trail < 2) {
                addHighlight(laneR, laneG, laneB, 0.32f);
              }
              accumR = clampByte((int)accumR + (int)laneR);
              accumG = clampByte((int)accumG + (int)laneG);
              accumB = clampByte((int)accumB + (int)laneB);
            }
          }
          r = accumR;
          g = accumG;
          b = accumB;
          if (y >= 6 && (y - 6) % 8 == 0) {
            uint8_t guideR = 0;
            uint8_t guideG = 0;
            uint8_t guideB = 0;
            mixRgb(
              breathEffectConfig.colorAR,
              breathEffectConfig.colorAG,
              breathEffectConfig.colorAB,
              breathEffectConfig.colorBR,
              breathEffectConfig.colorBG,
              breathEffectConfig.colorBB,
              0.3f,
              0.06f,
              guideR,
              guideG,
              guideB
            );
            r = clampByte((int)r + (int)guideR);
            g = clampByte((int)g + (int)guideG);
            b = clampByte((int)b + (int)guideB);
          }
        } else {
          int displayLayers[16];
          int displayCount = 0;
          if (breathEffectConfig.scope == BREATH_SCOPE_SINGLE_RING) {
            displayLayers[0] = 0;
            displayLayers[1] = 1;
            displayLayers[2] = 2;
            displayCount = 3;
          } else {
            for (int i = 0; i < 16; i++) {
              displayLayers[i] = i * 2;
            }
            displayCount = 16;
          }

          for (int layerIndex = 0; layerIndex < displayCount; layerIndex++) {
            int layer = displayLayers[layerIndex];
            int pathIndex = 0;
            if (!ringPathIndex(layer, x, y, pathIndex)) {
              continue;
            }
            int pathLength = ringPathLength(layer);
            if (pathLength <= 0) {
              continue;
            }
            float ringMix = gradientMode
              ? (float)layerIndex / (float)max(displayCount - 1, 1)
              : 0.0f;
            uint8_t ringR = 0;
            uint8_t ringG = 0;
            uint8_t ringB = 0;
            mixRgb(
              breathEffectConfig.colorAR,
              breathEffectConfig.colorAG,
              breathEffectConfig.colorAB,
              breathEffectConfig.colorBR,
              breathEffectConfig.colorBG,
              breathEffectConfig.colorBB,
              ringMix,
              1.0f,
              ringR,
              ringG,
              ringB
            );

            float alpha = 0.0f;
            if (breathEffectConfig.motion == BREATH_MOTION_CLOCKWISE ||
                breathEffectConfig.motion == BREATH_MOTION_COUNTERCLOCKWISE) {
              int direction = breathEffectConfig.motion == BREATH_MOTION_CLOCKWISE ? 1 : -1;
              float travel = phase * (1.2f + (float)speed * 0.18f) * (float)pathLength;
              int headIndex = ((int)roundf(travel + (float)layerIndex * 11.0f)) % pathLength;
              int rawDelta = headIndex - pathIndex;
              if (direction < 0) {
                rawDelta = pathIndex - headIndex;
              }
              while (rawDelta < 0) {
                rawDelta += pathLength;
              }
              rawDelta %= pathLength;
              int trailLength = breathEffectConfig.scope == BREATH_SCOPE_SINGLE_RING
                ? max(18, 34 - layerIndex * 6 + speed * 2)
                : max(10, 24 - layerIndex + speed);
              if (rawDelta < trailLength) {
                float trailRatio = trailLength <= 1 ? 0.0f : (float)rawDelta / (float)(trailLength - 1);
                float tail = powf(1.0f - trailRatio, 1.18f);
                alpha = (0.46f + breath * 0.34f) * tail;
                if (gradientMode) {
                  float accentMix = clampUnit(0.2f + trailRatio * 0.8f);
                  mixRgb(
                    ringR,
                    ringG,
                    ringB,
                    breathEffectConfig.colorBR,
                    breathEffectConfig.colorBG,
                    breathEffectConfig.colorBB,
                    accentMix,
                    alpha,
                    r,
                    g,
                    b
                  );
                } else {
                  mixRgb(ringR, ringG, ringB, ringR, ringG, ringB, 0.0f, alpha, r, g, b);
                }
                if (rawDelta < 2) {
                  addHighlight(r, g, b, 0.18f + breath * 0.16f);
                }
              }
              if (breathEffectConfig.scope == BREATH_SCOPE_FULL_SCREEN && layerIndex < 8) {
                uint8_t baseR = 0;
                uint8_t baseG = 0;
                uint8_t baseB = 0;
                mixRgb(
                  ringR,
                  ringG,
                  ringB,
                  ringR,
                  ringG,
                  ringB,
                  0.0f,
                  0.04f + ((float)(displayCount - layerIndex) / (float)displayCount) * 0.05f,
                  baseR,
                  baseG,
                  baseB
                );
                r = clampByte((int)r + (int)baseR);
                g = clampByte((int)g + (int)baseG);
                b = clampByte((int)b + (int)baseB);
              }
            } else {
              float travel = phase * (float)(displayCount + 4);
              float frontIndex = breathEffectConfig.motion == BREATH_MOTION_INWARD
                ? travel - (float)layerIndex * 0.85f
                : travel - (float)(displayCount - 1 - layerIndex) * 0.85f;
              float shell = clampUnit(1.0f - fabsf(frontIndex) * 0.42f);
              float residual = clampUnit(1.0f - fabsf(frontIndex - 2.6f) * 0.18f);
              alpha = shell * (0.42f + breath * 0.24f) + residual * 0.08f;
              if (alpha > 0.01f) {
                mixRgb(ringR, ringG, ringB, ringR, ringG, ringB, 0.0f, alpha, r, g, b);
                int headIndex =
                  ((int)roundf((phase * (0.9f + (float)speed * 0.16f) + (float)layerIndex * 0.09f) * (float)pathLength)) %
                  pathLength;
                int rawDelta = headIndex - pathIndex;
                while (rawDelta < 0) {
                  rawDelta += pathLength;
                }
                rawDelta %= pathLength;
                int trailLength = max(8, 14 - abs((int)roundf(frontIndex)));
                if (rawDelta < trailLength) {
                  addHighlight(r, g, b, alpha * 0.28f);
                }
              }
            }
          }

          bool cornerLight =
            (x <= 5 && y <= 5) ||
            (x >= 58 && y <= 5) ||
            (x <= 5 && y >= 58) ||
            (x >= 58 && y >= 58);
          if (cornerLight) {
            addHighlight(r, g, b, 0.18f + breath * 0.24f);
          }
        }
        dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }
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

    int speed = rhythmEffectConfig.speed;
    if (speed < 1) speed = 1;
    if (speed > 10) speed = 10;
    float strengthUnit = (float)strength / 100.0f;
    float beatFlash = 0.45f + 0.55f * sinf(((float)(elapsed % beatInterval) / (float)beatInterval) * 3.1415926f * (2.0f + (float)speed * 0.08f));
    float phase = fractf32((float)elapsed / max(beatInterval * 2.0f, 1.0f));
    bool matrixPalette = isMatrixRhythmPalette(rhythmEffectConfig);
    bool axisVertical =
      rhythmEffectConfig.direction == RHYTHM_DIR_LEFT ||
      rhythmEffectConfig.direction == RHYTHM_DIR_RIGHT;
    bool reverseAxis =
      rhythmEffectConfig.direction == RHYTHM_DIR_RIGHT ||
      rhythmEffectConfig.direction == RHYTHM_DIR_DOWN;
    NativeRgb colorA = makeRgb(
      rhythmEffectConfig.colorAR,
      rhythmEffectConfig.colorAG,
      rhythmEffectConfig.colorAB
    );
    NativeRgb colorB = makeRgb(
      rhythmEffectConfig.colorBR,
      rhythmEffectConfig.colorBG,
      rhythmEffectConfig.colorBB
    );
    NativeRgb backgroundColor = blendRgbColor(colorA, colorB, 0.14f + strengthUnit * 0.08f);
    NativeRgb gridColor = scaleRgbColor(blendRgbColor(colorA, colorB, 0.5f), 0.12f);
    int bandHeights[16];
    int peakHeights[16];
    int waterfallHistory[18][16];

    for (int band = 0; band < 16; band++) {
      float amplitude = rhythmAmplitudeAt(rhythmEffectConfig, phase, speed, strengthUnit, band);
      bandHeights[band] = rhythmHeightFromAmplitude(amplitude);
      peakHeights[band] = bandHeights[band];
      for (int lookback = 1; lookback <= 4; lookback++) {
        float historyPhase = fractf32(phase - (float)lookback / 20.0f);
        int historyHeight = rhythmHeightFromAmplitude(
          rhythmAmplitudeAt(rhythmEffectConfig, historyPhase, speed, strengthUnit, band)
        );
        peakHeights[band] = max(peakHeights[band], historyHeight - lookback * 2);
      }
      for (int row = 0; row < 18; row++) {
        float historyPhase = fractf32(phase - (float)row / 20.0f);
        waterfallHistory[row][band] = rhythmHeightFromAmplitude(
          rhythmAmplitudeAt(rhythmEffectConfig, historyPhase, speed, strengthUnit, band)
        );
      }
    }

    for (int y = 0; y < PANEL_RES_Y; y++) {
      for (int x = 0; x < PANEL_RES_X; x++) {
        float diagonal = (float)(x + y) / 126.0f;
        float backgroundPulse =
          0.5f +
          0.5f *
            sinf(
              diagonal * 3.1415926f * 2.8f -
              phase * 3.1415926f * (2.4f + (float)speed * 0.08f)
            );
        NativeRgb background = blendRgbColor(backgroundColor, colorA, backgroundPulse * 0.08f);
        uint8_t r = background.r;
        uint8_t g = background.g;
        uint8_t b = background.b;

        if (x % 8 == 0) {
          blendIntoPixel(r, g, b, gridColor, 0.34f);
        }
        if (y % 8 == 0) {
          blendIntoPixel(r, g, b, gridColor, 0.22f);
        }

        int bandIndex = 0;
        int thickness = 0;
        int depth = 0;
        bool laneVertical = false;
        bool laneReverse = false;
        bool onLane = resolveRhythmLane(
          rhythmEffectConfig.direction,
          x,
          y,
          bandIndex,
          thickness,
          depth,
          laneVertical,
          laneReverse
        );

        if (onLane) {
          int bandHeight = bandHeights[bandIndex];
          int peakHeight = max(0, peakHeights[bandIndex]);
          float bandMix =
            rhythmEffectConfig.mode == RHYTHM_MODE_GRADIENT
              ? (float)bandIndex / 15.0f
              : 0.18f + rhythmAmplitudeAt(rhythmEffectConfig, phase, speed, strengthUnit, bandIndex) * 0.72f;
          NativeRgb barColor =
            rhythmEffectConfig.mode == RHYTHM_MODE_JUMP
              ? blendRgbColor(colorA, colorB, (bandIndex % 2) == 0 ? 0.0f : 0.6f)
              : blendRgbColor(colorA, colorB, clampUnit(bandMix));
          NativeRgb glowColor = addHighlightColor(blendRgbColor(barColor, colorB, 0.35f), 0.22f);

          if (rhythmEffectConfig.mode == RHYTHM_MODE_PULSE) {
            if (depth < max(1, bandHeight)) {
              float fillRatio = bandHeight <= 1 ? 1.0f : (float)depth / (float)(bandHeight - 1);
              NativeRgb bodyColor = blendRgbColor(
                scaleRgbColor(barColor, 0.46f + fillRatio * 0.18f),
                addHighlightColor(barColor, 0.2f + fillRatio * 0.34f),
                fillRatio
              );
              float alpha = clampUnit(0.18f + fillRatio * 0.72f);
              blendIntoPixel(r, g, b, bodyColor, alpha);
              if (thickness == 1 && depth >= bandHeight - 2) {
                blendIntoPixel(r, g, b, glowColor, 0.32f + fillRatio * 0.16f);
              }
            }
            if (depth == peakHeight) {
              blendIntoPixel(r, g, b, glowColor, thickness == 1 ? 0.9f : 0.64f);
            }
            if (depth < min(6, bandHeight)) {
              float fade = clampUnit(0.22f - (float)depth * 0.03f);
              blendIntoPixel(r, g, b, glowColor, fade);
            }
          } else if (rhythmEffectConfig.mode == RHYTHM_MODE_GRADIENT) {
            if (depth >= 6 && ((depth - 6) % 3) == 0) {
              int row = (depth - 6) / 3;
              if (row >= 0 && row < 18) {
                int historyHeight = waterfallHistory[row][bandIndex];
                float ratio = clampUnit(((float)historyHeight - 6.0f) / 44.0f);
                NativeRgb rowColor = blendRgbColor(colorA, colorB, ratio);
                NativeRgb rowGlow = addHighlightColor(rowColor, 0.18f + ratio * 0.24f);
                blendIntoPixel(r, g, b, rowGlow, 0.2f + ratio * 0.7f);
                if (row == 0) {
                  blendIntoPixel(r, g, b, rowGlow, 0.16f + ratio * 0.18f);
                }
              }
            }
            if (matrixPalette && thickness == 1) {
              float seed = columnSeedf(bandIndex);
              float travel = fractf32(phase * (0.8f + (float)speed * 0.06f) + seed);
              int head = (int)roundf(travel * 82.0f) - 9;
              for (int trail = 0; trail < 10; trail++) {
                if (depth == head - trail) {
                  float fade = clampUnit(1.0f - (float)trail / 10.0f);
                  blendIntoPixel(r, g, b, colorB, fade * 0.55f);
                }
              }
            }
          } else {
            int safeHeight = max(2, (int)roundf((float)bandHeight * 0.72f));
            if (depth < safeHeight) {
              float alpha = depth > safeHeight - 3 ? 0.9f : 0.46f;
              blendIntoPixel(r, g, b, barColor, alpha);
              if (depth > safeHeight - 2) {
                blendIntoPixel(r, g, b, glowColor, 0.3f);
              }
            }
          }
        }

        NativeRgb hubColor = blendRgbColor(colorA, colorB, beatFlash * 0.7f);
        int dx = abs(x - 32);
        int dy = abs(y - 32);
        if (dx <= 2 && dy <= 2) {
          blendIntoPixel(r, g, b, hubColor, clampUnit(0.16f + strengthUnit * 0.2f));
        }
        if (rhythmEffectConfig.mode == RHYTHM_MODE_JUMP) {
          if ((x == 32 && dy <= 3) || (y == 32 && dx <= 3)) {
            blendIntoPixel(r, g, b, scaleRgbColor(hubColor, 0.52f), 0.72f);
          }
          if (dx + dy <= 2) {
            blendIntoPixel(r, g, b, scaleRgbColor(hubColor, 0.3f), 0.46f);
          }
          for (int pulseIndex = 0; pulseIndex < 4; pulseIndex++) {
            int radius = 4 + pulseIndex * 4 + (int)roundf(beatFlash * 2.0f);
            NativeRgb pulseColor = scaleRgbColor(hubColor, 0.22f - (float)pulseIndex * 0.04f);
            if (pulseColor.r == 0 && pulseColor.g == 0 && pulseColor.b == 0) {
              continue;
            }
            bool onPulse =
              (abs(y - (32 - radius)) == 0 && dx <= radius) ||
              (abs(y - (32 + radius)) == 0 && dx <= radius) ||
              (abs(x - (32 - radius)) == 0 && dy <= radius) ||
              (abs(x - (32 + radius)) == 0 && dy <= radius);
            if (onPulse) {
              blendIntoPixel(r, g, b, pulseColor, 0.36f);
            }
          }
        } else if (rhythmEffectConfig.mode == RHYTHM_MODE_GRADIENT) {
          float sweepAlpha = smoothstepf(0.0f, 1.0f, 0.2f + beatFlash * 0.8f) * 0.22f;
          NativeRgb sweepColor = blendRgbColor(colorA, colorB, 0.5f);
          int scan = (int)roundf(fractf32(phase + (float)y / 96.0f) * 63.0f);
          if (x == scan) {
            blendIntoPixel(r, g, b, sweepColor, sweepAlpha * 0.6f);
          }
        } else if (rhythmEffectConfig.mode == RHYTHM_MODE_PULSE) {
          static const int orbit[12][2] = {
            {32, 8}, {44, 12}, {52, 20}, {56, 32}, {52, 44}, {44, 52},
            {32, 56}, {20, 52}, {12, 44}, {8, 32}, {12, 20}, {20, 12}
          };
          int frameIndex = (int)roundf(phase * 20.0f);
          for (int orbitIndex = 0; orbitIndex < 12; orbitIndex++) {
            int pointIndex = (orbitIndex + frameIndex) % 12;
            int px = orbit[pointIndex][0];
            int py = orbit[pointIndex][1];
            if ((x == px || x == px + 1) && y == py) {
              NativeRgb orbitColor =
                orbitIndex % 2 == 0
                  ? scaleRgbColor(colorA, 0.5f + beatFlash * 0.18f)
                  : scaleRgbColor(colorB, 0.42f + beatFlash * 0.16f);
              blendIntoPixel(r, g, b, orbitColor, 0.9f);
            }
          }
        }

        float luminance = colorLuminance255(r, g, b);
        addHighlight(r, g, b, clampUnit((1.0f - luminance) * 0.08f + beatFlash * 0.12f));
        dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }
    return;
  }

  if (nativeEffectType == NATIVE_EFFECT_AMBIENT) {
    if (!ambientEffectConfig.loop && elapsed >= 90000UL) {
      setNativeEffectNone();
      return;
    }

    int speed = ambientEffectConfig.speed;
    if (speed < 1) speed = 1;
    if (speed > 10) speed = 10;

    int intensity = ambientEffectConfig.intensity;
    if (intensity < 0) intensity = 0;
    if (intensity > 100) intensity = 100;

    float speedUnit = (float)speed / 10.0f;
    float intensityUnit = (float)intensity / 100.0f;
    float timeBase = (float)elapsed * (0.00045f + speedUnit * 0.00135f);

    for (int y = 0; y < PANEL_RES_Y; y++) {
      for (int x = 0; x < PANEL_RES_X; x++) {
        float nx = ((float)x - 31.5f) / 31.5f;
        float ny = ((float)y - 31.5f) / 31.5f;
        float dist = sqrtf(nx * nx + ny * ny);

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        if (ambientEffectConfig.preset == AMBIENT_PRESET_AURORA) {
          float curtainA = 0.5f + 0.5f * sinf(nx * 5.2f + timeBase * 1.9f + sinf(ny * 3.6f - timeBase * 1.1f));
          float curtainB = 0.5f + 0.5f * sinf(nx * 7.4f - timeBase * 1.5f + ny * 5.0f);
          float horizon = clampUnit(1.0f - ((float)y / 63.0f) * 1.18f);
          float veil = clampUnit(curtainA * 0.55f + curtainB * 0.45f);
          float glow = clampUnit(horizon * (0.35f + veil * (0.5f + intensityUnit * 0.35f)));
          float accent = clampUnit(0.24f + 0.76f * glowPulsef(timeBase * 1.1f, nx * 2.5f + ny * 1.7f));

          r = clampByte((int)roundf((10.0f + 70.0f * veil + 24.0f * accent) * glow));
          g = clampByte((int)roundf((30.0f + 180.0f * veil + 28.0f * accent) * glow));
          b = clampByte((int)roundf((50.0f + 210.0f * accent + 18.0f * veil) * glow));

          if (((x + y + (int)(elapsed / 180UL)) % 29) == 0) {
            addHighlight(r, g, b, 0.18f + intensityUnit * 0.16f);
          }
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_PLASMA) {
          float plasmaA = sinf((nx * 5.8f + timeBase * 1.4f) + sinf(ny * 4.0f - timeBase * 0.9f));
          float plasmaB = sinf((nx + ny) * 6.4f - timeBase * 1.8f);
          float plasmaC = sinf(dist * 12.0f - timeBase * 3.0f);
          float field = clampUnit((plasmaA + plasmaB + plasmaC + 3.0f) / 6.0f);
          float heat = clampUnit(0.5f + 0.5f * sinf(field * 8.0f + timeBase * 2.2f));
          float cool = clampUnit(0.5f + 0.5f * cosf(field * 7.0f - timeBase * 1.3f));
          float glow = 0.18f + field * (0.56f + intensityUnit * 0.22f);

          r = clampByte((int)roundf((40.0f + 205.0f * heat) * glow));
          g = clampByte((int)roundf((20.0f + 150.0f * field + 45.0f * cool) * glow));
          b = clampByte((int)roundf((60.0f + 185.0f * cool) * glow));
          addHighlight(r, g, b, field * 0.12f);
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_MATRIX_RAIN) {
          float seed = columnSeedf(x);
          float companionSeed = columnSeedf(x + 19);
          float travel = fractf32(timeBase * (0.34f + speedUnit * 0.32f) + seed);
          float headY = travel * (64.0f + 22.0f) - 11.0f;
          float headYSecondary =
            fractf32(timeBase * (0.22f + speedUnit * 0.18f) + companionSeed * 0.83f) * (64.0f + 16.0f) - 8.0f;
          float tailLength = 10.0f + intensityUnit * 26.0f;
          float tailDistance = headY - (float)y;
          float secondaryDistance = headYSecondary - (float)y;
          float tail = 0.0f;
          float tailSecondary = 0.0f;
          if (tailDistance >= 0.0f && tailDistance <= tailLength) {
            tail = 1.0f - tailDistance / max(tailLength, 1.0f);
          }
          if (secondaryDistance >= 0.0f && secondaryDistance <= tailLength * 0.56f) {
            tailSecondary = 1.0f - secondaryDistance / max(tailLength * 0.56f, 1.0f);
          }
          float headGlow = smoothstepf(4.5f, 0.0f, fabsf(headY - (float)y));
          float secondaryHeadGlow = smoothstepf(3.2f, 0.0f, fabsf(headYSecondary - (float)y));
          float glyph = (((y + (int)(seed * 11.0f) + (int)(elapsed / 65UL)) % 6) == 0) ? 1.0f : 0.0f;
          float ambient = 0.03f + glowPulsef(timeBase * 0.9f, seed * 6.0f) * 0.04f;
          float brightness = clampUnit(
            ambient +
            tail * (0.52f + intensityUnit * 0.28f) +
            tailSecondary * 0.28f +
            headGlow * 0.55f +
            secondaryHeadGlow * 0.26f +
            glyph * tail * 0.18f
          );

          r = clampByte((int)roundf(8.0f + 38.0f * headGlow + 16.0f * secondaryHeadGlow));
          g = clampByte((int)roundf((26.0f + 220.0f * brightness)));
          b = clampByte((int)roundf((8.0f + 92.0f * (tail * 0.55f + headGlow * 0.35f + tailSecondary * 0.22f))));
          if (((x + (int)(elapsed / 42UL)) % 9) == 0 && tail > 0.16f) {
            addHighlight(r, g, b, 0.1f + intensityUnit * 0.12f);
          }
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_FIREFLY_SWARM) {
          float dusk = clampUnit(1.0f - ((float)y / 63.0f) * 0.78f);
          float backgroundWave = 0.5f + 0.5f * sinf(ny * 6.0f - timeBase * 0.85f);
          r = clampByte((int)roundf(2.0f + 6.0f * backgroundWave));
          g = clampByte((int)roundf(10.0f + 18.0f * dusk));
          b = clampByte((int)roundf(16.0f + 42.0f * dusk));

          const int kFireflyCount = 9;
          for (int i = 0; i < kFireflyCount; i++) {
            float seedX = fireflySeedf(i, 0.0f);
            float seedY = fireflySeedf(i, 1.3f);
            float orbit = fireflySeedf(i, 2.7f);
            float fx = 6.0f + seedX * 52.0f + sinf(timeBase * (0.7f + orbit * 0.8f) + seedY * 6.2831852f) * (5.0f + orbit * 10.0f);
            float fy = 8.0f + seedY * 44.0f + cosf(timeBase * (0.9f + seedX * 0.7f) + orbit * 4.1f) * (4.0f + seedX * 9.0f);
            float dx = (float)x - fx;
            float dy = (float)y - fy;
            float glow = clampUnit(1.0f - sqrtf(dx * dx + dy * dy) / (4.5f + intensityUnit * 4.0f));
            if (glow > 0.0f) {
              float twinkle = glowPulsef(timeBase * (1.5f + seedX), seedY * 8.0f);
              int addR = (int)roundf(50.0f * glow * twinkle);
              int addG = (int)roundf((140.0f + 70.0f * intensityUnit) * glow * twinkle);
              int addB = (int)roundf(30.0f * glow * twinkle);
              r = clampByte((int)r + addR);
              g = clampByte((int)g + addG);
              b = clampByte((int)b + addB);
            }
          }
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_OCEAN_CURRENT) {
          float tideA = 0.5f + 0.5f * sinf(nx * 4.6f - timeBase * 1.1f + sinf(ny * 5.0f + timeBase * 0.7f));
          float tideB = 0.5f + 0.5f * sinf((nx - ny) * 6.2f + timeBase * 1.6f);
          float ripple = 0.5f + 0.5f * sinf(dist * 14.0f - timeBase * 2.4f);
          float depth = clampUnit(0.18f + ((float)y / 63.0f) * 0.52f);
          float foam = clampUnit(1.0f - fabsf(ripple - 0.82f) * 5.5f);
          float glow = clampUnit(0.16f + depth * 0.34f + tideA * 0.24f + tideB * 0.2f);

          r = clampByte((int)roundf((6.0f + 28.0f * tideB + 42.0f * foam) * glow));
          g = clampByte((int)roundf((36.0f + 110.0f * tideA + 40.0f * foam) * glow));
          b = clampByte((int)roundf((70.0f + 150.0f * tideA + 36.0f * ripple) * glow));
          addHighlight(r, g, b, foam * (0.1f + intensityUnit * 0.12f));
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_NEON_GRID) {
          float scanX = clampUnit(1.0f - fabsf(fractf32((float)x / 64.0f + timeBase * (0.28f + speedUnit * 0.18f)) - 0.5f) * 5.6f);
          float scanY = clampUnit(1.0f - fabsf(fractf32((float)y / 64.0f - timeBase * (0.22f + speedUnit * 0.14f)) - 0.5f) * 5.6f);
          float gridLine = (x % 8 == 0 || y % 8 == 0) ? 1.0f : 0.0f;
          float junction = (x % 8 == 0 && y % 8 == 0) ? 1.0f : 0.0f;
          float cellPulse = 0.5f + 0.5f * sinf((float)(x + y) * 0.25f + timeBase * 3.2f);
          float brightness = clampUnit(
            0.06f +
            gridLine * 0.24f +
            junction * 0.12f +
            scanX * 0.32f +
            scanY * 0.26f +
            cellPulse * 0.14f
          );

          r = clampByte((int)roundf((18.0f + 120.0f * scanX + 40.0f * gridLine + 40.0f * junction) * brightness));
          g = clampByte((int)roundf((10.0f + 70.0f * cellPulse + 30.0f * gridLine + 12.0f * junction) * brightness));
          b = clampByte((int)roundf((30.0f + 180.0f * scanY + 100.0f * gridLine + 60.0f * junction) * brightness));
          if (gridLine > 0.5f) {
            addHighlight(r, g, b, 0.08f + intensityUnit * 0.1f);
          }
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_SUNSET_BLUSH) {
          float sky = 1.0f - ((float)y / 63.0f);
          float warmBand = 0.5f + 0.5f * sinf(nx * 4.2f + timeBase * 0.9f + ny * 2.0f);
          float cloud = 0.5f + 0.5f * sinf(nx * 8.0f - timeBase * 0.6f + sinf(ny * 6.0f + timeBase * 0.5f));
          float sunGlow = clampUnit(1.0f - sqrtf((nx * 0.92f) * (nx * 0.92f) + ((ny + 0.18f) * 1.25f) * ((ny + 0.18f) * 1.25f)) * 1.55f);
          float haze = clampUnit(0.1f + sky * 0.35f + warmBand * 0.22f + cloud * 0.12f);

          r = clampByte((int)roundf((40.0f + 185.0f * sky + 55.0f * sunGlow) * haze));
          g = clampByte((int)roundf((16.0f + 88.0f * warmBand + 76.0f * sunGlow) * haze));
          b = clampByte((int)roundf((20.0f + 105.0f * cloud + 70.0f * (1.0f - sky)) * haze));
          addHighlight(r, g, b, sunGlow * (0.16f + intensityUnit * 0.14f));
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_STARFIELD_DRIFT) {
          float nebulaA = 0.5f + 0.5f * sinf(nx * 5.0f + timeBase * 0.45f + ny * 4.0f);
          float nebulaB = 0.5f + 0.5f * sinf((nx - ny) * 7.2f - timeBase * 0.62f);
          float backdrop = clampUnit(0.03f + nebulaA * 0.08f + nebulaB * 0.06f);
          r = clampByte((int)roundf(4.0f + 16.0f * nebulaB * backdrop));
          g = clampByte((int)roundf(6.0f + 22.0f * nebulaA * backdrop));
          b = clampByte((int)roundf(12.0f + 46.0f * (nebulaA * 0.55f + nebulaB * 0.45f) * backdrop));

          int starIndex = (x * 31 + y * 17) % 97;
          if (starIndex < 3) {
            float twinkle = glowPulsef(timeBase * (1.1f + (float)starIndex * 0.35f), (float)(x + y) * 0.21f);
            float shine = 0.28f + twinkle * (0.42f + intensityUnit * 0.18f);
            r = clampByte((int)roundf((float)r + 140.0f * shine));
            g = clampByte((int)roundf((float)g + 150.0f * shine));
            b = clampByte((int)roundf((float)b + 185.0f * shine));
            if (x > 0 && starIndex == 0 && twinkle > 0.72f) {
              r = clampByte((int)r + 32);
              g = clampByte((int)g + 34);
              b = clampByte((int)b + 40);
            }
          }
        } else {
          float diag = fractf32(((float)(x + y) / 96.0f) - timeBase * (0.7f + speedUnit * 0.9f));
          float diagB =
            fractf32(((float)x * 0.84f + (float)y * 1.18f) / 88.0f - timeBase * (0.48f + speedUnit * 0.66f) + 0.37f);
          float streak = clampUnit(1.0f - fabsf(diag - 0.5f) * 7.6f);
          float streakB = clampUnit(1.0f - fabsf(diagB - 0.5f) * 8.4f);
          float sparkle = (((x * 13 + y * 7 + (int)(elapsed / 60UL)) % 43) == 0) ? 1.0f : 0.0f;
          float haze = clampUnit(0.08f + (1.0f - dist) * 0.14f);
          float brightness = clampUnit(haze + streak * (0.42f + intensityUnit * 0.2f) + streakB * 0.28f + sparkle * 0.32f);

          r = clampByte((int)roundf((10.0f + 80.0f * streak + 52.0f * streakB + 45.0f * sparkle + 20.0f * haze) * brightness));
          g = clampByte((int)roundf((18.0f + 120.0f * streak + 78.0f * streakB + 60.0f * sparkle + 35.0f * haze) * brightness));
          b = clampByte((int)roundf((28.0f + 180.0f * streak + 110.0f * streakB + 90.0f * sparkle + 60.0f * haze) * brightness));
          if (sparkle > 0.0f) {
            addHighlight(r, g, b, 0.2f + intensityUnit * 0.15f);
          }
        }

        dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }
    return;
  }

  if (nativeEffectType == NATIVE_EFFECT_EYES) {
    EyesEffect::render();
  }
}
