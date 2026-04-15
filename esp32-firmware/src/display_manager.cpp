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
AmbientEffectConfig DisplayManager::ambientEffectConfig = {AMBIENT_PRESET_AURORA, 6, 72, 72, 100, 200, 255, true};
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

namespace {
bool isValidDriver(uint8_t driver) {
  return driver <= 5;
}

bool isValidI2SSpeed(uint32_t speed) {
  return speed == 8000000 || speed == 16000000 || speed == 20000000;
}

bool parseHourMinuteText(const char* text, int& outMinutes) {
  if (text == nullptr || strlen(text) != 5 || text[2] != ':') {
    return false;
  }
  if (text[0] < '0' || text[0] > '9' ||
      text[1] < '0' || text[1] > '9' ||
      text[3] < '0' || text[3] > '9' ||
      text[4] < '0' || text[4] > '9') {
    return false;
  }

  int hour = (text[0] - '0') * 10 + (text[1] - '0');
  int minute = (text[3] - '0') * 10 + (text[4] - '0');
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return false;
  }

  outMinutes = hour * 60 + minute;
  return true;
}

bool isNightBrightnessWindowActive() {
  if (!WiFiManager::isTimeSynced()) {
    return false;
  }

  int nightStartMinutes = 0;
  int nightEndMinutes = 0;
  if (!parseHourMinuteText(ConfigManager::deviceParamsConfig.nightStart, nightStartMinutes) ||
      !parseHourMinuteText(ConfigManager::deviceParamsConfig.nightEnd, nightEndMinutes)) {
    return false;
  }

  if (nightStartMinutes == nightEndMinutes) {
    return false;
  }

  time_t now = time(nullptr);
  struct tm localTime = {};
  localtime_r(&now, &localTime);
  int currentMinutes = localTime.tm_hour * 60 + localTime.tm_min;

  if (nightStartMinutes < nightEndMinutes) {
    return currentMinutes >= nightStartMinutes && currentMinutes < nightEndMinutes;
  }
  return currentMinutes >= nightStartMinutes || currentMinutes < nightEndMinutes;
}

int resolveScheduledBrightness() {
  int dayBrightness = ConfigManager::deviceParamsConfig.brightnessDay;
  int nightBrightness = ConfigManager::deviceParamsConfig.brightnessNight;
  if (isNightBrightnessWindowActive()) {
    return nightBrightness;
  }
  return dayBrightness;
}

void applyColorSwapPins(HUB75_I2S_CFG& mxconfig) {
  if (ConfigManager::deviceParamsConfig.swapBlueGreen) {
    mxconfig.gpio.b1 = 26;
    mxconfig.gpio.b2 = 12;
    mxconfig.gpio.g1 = 27;
    mxconfig.gpio.g2 = 13;
  }

  if (ConfigManager::deviceParamsConfig.swapBlueRed) {
    mxconfig.gpio.b1 = 25;
    mxconfig.gpio.b2 = 14;
    mxconfig.gpio.r1 = 27;
    mxconfig.gpio.r2 = 13;
  }
}

void applyHardwareConfig(HUB75_I2S_CFG& mxconfig) {
  applyColorSwapPins(mxconfig);
  mxconfig.gpio.e = ConfigManager::deviceParamsConfig.E_pin;

  if (isValidDriver(ConfigManager::deviceParamsConfig.driver)) {
    mxconfig.driver = static_cast<HUB75_I2S_CFG::shift_driver>(ConfigManager::deviceParamsConfig.driver);
  }

  mxconfig.clkphase = ConfigManager::deviceParamsConfig.clkphase;

  if (isValidI2SSpeed(ConfigManager::deviceParamsConfig.i2cSpeed)) {
    mxconfig.i2sspeed = static_cast<HUB75_I2S_CFG::clk_speed>(ConfigManager::deviceParamsConfig.i2cSpeed);
  }
}

void destroyOffscreenDisplay() {
  if (s_offscreenDisplay != nullptr) {
    delete s_offscreenDisplay;
    s_offscreenDisplay = nullptr;
  }
}
}

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
    applyHardwareConfig(mxconfig);
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

void DisplayManager::applyDeviceParams(bool showBootLogo) {
  rebuildMatrix(doubleBufferEnabled, showBootLogo);
}

bool DisplayManager::rebuildMatrix(bool doubleBuffered, bool showBootLogo) {
  if (dma_display != nullptr) {
    delete dma_display;
    dma_display = nullptr;
  }

  destroyOffscreenDisplay();

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  applyHardwareConfig(mxconfig);
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
  dma_display->setBrightness8(ConfigManager::deviceParamsConfig.displayBright);
  dma_display->setRotation(ConfigManager::deviceParamsConfig.displayRotation);
  dma_display->setLatBlanking(2);
  dma_display->clearScreen();
  currentBrightness = ConfigManager::deviceParamsConfig.displayBright;

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
    ConfigManager::deviceParamsConfig.displayBright = static_cast<uint8_t>(brightness);
    dma_display->setBrightness8(brightness);
  }
}

void DisplayManager::refreshScheduledBrightness() {
  if (dma_display == nullptr) {
    return;
  }

  int targetBrightness = resolveScheduledBrightness();
  if (targetBrightness < 0 || targetBrightness > 255) {
    return;
  }

  if (currentBrightness == targetBrightness) {
    return;
  }

  currentBrightness = targetBrightness;
  dma_display->setBrightness8(targetBrightness);
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

static void fillAmbientRect(int x, int y, int w, int h, const NativeRgb& color) {
  if (DisplayManager::dma_display == nullptr) {
    return;
  }
  DisplayManager::dma_display->fillRect(x, y, w, h, DisplayManager::dma_display->color565(color.r, color.g, color.b));
}

static void drawAmbientRectOutline(int x, int y, int w, int h, const NativeRgb& color) {
  if (DisplayManager::dma_display == nullptr || w <= 0 || h <= 0) {
    return;
  }
  fillAmbientRect(x, y, w, 1, color);
  fillAmbientRect(x, y + h - 1, w, 1, color);
  fillAmbientRect(x, y, 1, h, color);
  fillAmbientRect(x + w - 1, y, 1, h, color);
}

static void drawAmbientGlowDot(int centerX, int centerY, const NativeRgb& color, float intensity, int radius) {
  if (DisplayManager::dma_display == nullptr) {
    return;
  }

  for (int offsetY = -radius; offsetY <= radius; offsetY++) {
    for (int offsetX = -radius; offsetX <= radius; offsetX++) {
      int distance = abs(offsetX) + abs(offsetY);
      float fade = clampUnit(1.0f - (float)distance / (float)(radius + 1));
      if (fade <= 0.0f) {
        continue;
      }
      float alpha = intensity * fade;
      DisplayManager::dma_display->drawPixelRGB888(
        centerX + offsetX,
        centerY + offsetY,
        clampByte((int)roundf((float)color.r * alpha)),
        clampByte((int)roundf((float)color.g * alpha)),
        clampByte((int)roundf((float)color.b * alpha))
      );
    }
  }
}

static int ambientTextWidth(const char* text, int size) {
  if (text == nullptr || size <= 0) {
    return 0;
  }
  int len = strlen(text);
  if (len <= 0) {
    return 0;
  }
  return len * 4 * size - size;
}

static void drawAmbientTextCenteredInBox(const char* text, int left, int width, int y, const NativeRgb& color, int size) {
  if (DisplayManager::dma_display == nullptr || text == nullptr) {
    return;
  }
  int textWidth = ambientTextWidth(text, size);
  int x = left + (width - textWidth) / 2;
  if (x < left) {
    x = left;
  }
  DisplayManager::drawTinyText(text, x, y, DisplayManager::dma_display->color565(color.r, color.g, color.b), size);
}

static void drawAmbientPanelFrame(int x, int y, int w, int h, const NativeRgb& bg, const NativeRgb& border, const NativeRgb& accent) {
  fillAmbientRect(x, y, w, h, bg);
  drawAmbientRectOutline(x, y, w, h, border);
  fillAmbientRect(x + 2, y + 2, w - 4, 1, accent);
  fillAmbientRect(x + 2, y + h - 3, w - 4, 1, accent);
}

static void drawAmbientChip(int x, int y, int w, const NativeRgb& color) {
  fillAmbientRect(x, y, w, 2, color);
}

static void drawAmbientSunIcon(int centerX, int centerY, const NativeRgb& color) {
  drawAmbientGlowDot(centerX, centerY, color, 0.95f, 4);
  NativeRgb core = addHighlightColor(color, 0.12f);
  fillAmbientRect(centerX - 3, centerY - 3, 7, 7, core);
  fillAmbientRect(centerX - 1, centerY - 7, 2, 3, color);
  fillAmbientRect(centerX - 1, centerY + 5, 2, 3, color);
  fillAmbientRect(centerX - 7, centerY - 1, 3, 2, color);
  fillAmbientRect(centerX + 5, centerY - 1, 3, 2, color);
}

static void drawAmbientCloudIcon(int x, int y, const NativeRgb& baseColor, const NativeRgb& glowColor) {
  drawAmbientGlowDot(x + 10, y + 8, glowColor, 0.45f, 3);
  fillAmbientRect(x + 3, y + 8, 14, 5, baseColor);
  fillAmbientRect(x + 1, y + 10, 18, 4, baseColor);
  fillAmbientRect(x + 5, y + 5, 5, 4, addHighlightColor(baseColor, 0.12f));
  fillAmbientRect(x + 9, y + 4, 6, 5, addHighlightColor(baseColor, 0.18f));
}

static void drawAmbientRainDrops(int originX, int originY, const NativeRgb& color) {
  for (int index = 0; index < 4; index++) {
    int x = originX + index * 4;
    fillAmbientRect(x, originY + (index % 2), 1, 4, color);
    fillAmbientRect(x + 1, originY + 3 + (index % 2), 1, 1, color);
  }
}

static void drawAmbientSnowFlakes(int originX, int originY, const NativeRgb& color) {
  for (int index = 0; index < 3; index++) {
    int x = originX + index * 5;
    int y = originY + (index % 2);
    fillAmbientRect(x, y + 1, 3, 1, color);
    fillAmbientRect(x + 1, y, 1, 3, color);
  }
}

static NativeRgb ambientHsvToRgb(float hue, float saturation, float value) {
  float h = fmodf(hue, 1.0f);
  if (h < 0.0f) {
    h += 1.0f;
  }
  float sector = h * 6.0f;
  int index = (int)floorf(sector);
  float fraction = sector - (float)index;
  float p = value * (1.0f - saturation);
  float q = value * (1.0f - saturation * fraction);
  float t = value * (1.0f - saturation * (1.0f - fraction));

  float r = value;
  float g = t;
  float b = p;

  if (index == 1) {
    r = q;
    g = value;
    b = p;
  } else if (index == 2) {
    r = p;
    g = value;
    b = t;
  } else if (index == 3) {
    r = p;
    g = q;
    b = value;
  } else if (index == 4) {
    r = t;
    g = p;
    b = value;
  } else if (index >= 5) {
    r = value;
    g = p;
    b = q;
  }

  return makeRgb(
    clampByte((int)roundf(r * 255.0f)),
    clampByte((int)roundf(g * 255.0f)),
    clampByte((int)roundf(b * 255.0f))
  );
}

static NativeRgb ambientHslToRgb(float hueDegrees, float saturation, float lightness) {
  float hue = fmodf(hueDegrees, 360.0f);
  if (hue < 0.0f) {
    hue += 360.0f;
  }
  float chroma = (1.0f - fabsf(2.0f * lightness - 1.0f)) * saturation;
  float huePrime = hue / 60.0f;
  float x = chroma * (1.0f - fabsf(fmodf(huePrime, 2.0f) - 1.0f));
  float match = lightness - chroma / 2.0f;

  float r1 = 0.0f;
  float g1 = 0.0f;
  float b1 = 0.0f;
  if (huePrime < 1.0f) {
    r1 = chroma;
    g1 = x;
  } else if (huePrime < 2.0f) {
    r1 = x;
    g1 = chroma;
  } else if (huePrime < 3.0f) {
    g1 = chroma;
    b1 = x;
  } else if (huePrime < 4.0f) {
    g1 = x;
    b1 = chroma;
  } else if (huePrime < 5.0f) {
    r1 = x;
    b1 = chroma;
  } else {
    r1 = chroma;
    b1 = x;
  }

  return makeRgb(
    clampByte((int)roundf((r1 + match) * 255.0f)),
    clampByte((int)roundf((g1 + match) * 255.0f)),
    clampByte((int)roundf((b1 + match) * 255.0f))
  );
}

struct AmbientBoidState {
  float x;
  float y;
  float vx;
  float vy;
  float ax;
  float ay;
  NativeRgb color;
};

static AmbientBoidState s_ambientBoids[18];
static bool s_ambientBoidsReady = false;
static unsigned long s_ambientBoidsElapsed = 0;

static void initAmbientBoids() {
  for (int index = 0; index < 18; index++) {
    float hue = fireflySeedf(index, 0.31f) * 360.0f;
    s_ambientBoids[index].x = fireflySeedf(index, 1.11f) * 63.0f;
    s_ambientBoids[index].y = fireflySeedf(index, 2.07f) * 63.0f;
    s_ambientBoids[index].vx = fireflySeedf(index, 3.11f) * 2.0f - 1.0f;
    s_ambientBoids[index].vy = fireflySeedf(index, 4.23f) * 2.0f - 1.0f;
    s_ambientBoids[index].ax = 0.0f;
    s_ambientBoids[index].ay = 0.0f;
    s_ambientBoids[index].color = ambientHslToRgb(hue, 1.0f, 0.5f);
  }
  s_ambientBoidsReady = true;
  s_ambientBoidsElapsed = 0;
}

static float ambientVectorLength(float x, float y) {
  return sqrtf(x * x + y * y);
}

static void limitAmbientVector(float& x, float& y, float maxLength) {
  float length = ambientVectorLength(x, y);
  if (length <= 0.0001f || length <= maxLength) {
    return;
  }
  float scale = maxLength / length;
  x *= scale;
  y *= scale;
}

static void normalizeAmbientVector(float& x, float& y) {
  float length = ambientVectorLength(x, y);
  if (length <= 0.0001f) {
    x = 0.0f;
    y = 0.0f;
    return;
  }
  x /= length;
  y /= length;
}

static void ambientSeparateBoid(
  int index,
  float desiredSeparation,
  float maxSpeed,
  float maxForce,
  float& outX,
  float& outY
) {
  outX = 0.0f;
  outY = 0.0f;
  int count = 0;

  for (int other = 0; other < 18; other++) {
    if (other == index) {
      continue;
    }
    float dx = s_ambientBoids[index].x - s_ambientBoids[other].x;
    float dy = s_ambientBoids[index].y - s_ambientBoids[other].y;
    float distance = ambientVectorLength(dx, dy);
    if (distance <= 0.0f || distance >= desiredSeparation) {
      continue;
    }
    normalizeAmbientVector(dx, dy);
    outX += dx / distance;
    outY += dy / distance;
    count++;
  }

  if (count <= 0) {
    outX = 0.0f;
    outY = 0.0f;
    return;
  }

  outX /= (float)count;
  outY /= (float)count;
  normalizeAmbientVector(outX, outY);
  outX *= maxSpeed;
  outY *= maxSpeed;
  outX -= s_ambientBoids[index].vx;
  outY -= s_ambientBoids[index].vy;
  limitAmbientVector(outX, outY, maxForce);
}

static void ambientAlignBoid(
  int index,
  float neighborDistance,
  float maxSpeed,
  float maxForce,
  float& outX,
  float& outY
) {
  outX = 0.0f;
  outY = 0.0f;
  int count = 0;

  for (int other = 0; other < 18; other++) {
    if (other == index) {
      continue;
    }
    float dx = s_ambientBoids[index].x - s_ambientBoids[other].x;
    float dy = s_ambientBoids[index].y - s_ambientBoids[other].y;
    float distance = ambientVectorLength(dx, dy);
    if (distance <= 0.0f || distance >= neighborDistance) {
      continue;
    }
    outX += s_ambientBoids[other].vx;
    outY += s_ambientBoids[other].vy;
    count++;
  }

  if (count <= 0) {
    outX = 0.0f;
    outY = 0.0f;
    return;
  }

  outX /= (float)count;
  outY /= (float)count;
  normalizeAmbientVector(outX, outY);
  outX *= maxSpeed;
  outY *= maxSpeed;
  outX -= s_ambientBoids[index].vx;
  outY -= s_ambientBoids[index].vy;
  limitAmbientVector(outX, outY, maxForce);
}

static void ambientCohesionBoid(
  int index,
  float neighborDistance,
  float maxSpeed,
  float maxForce,
  float& outX,
  float& outY
) {
  outX = 0.0f;
  outY = 0.0f;
  int count = 0;

  for (int other = 0; other < 18; other++) {
    if (other == index) {
      continue;
    }
    float dx = s_ambientBoids[index].x - s_ambientBoids[other].x;
    float dy = s_ambientBoids[index].y - s_ambientBoids[other].y;
    float distance = ambientVectorLength(dx, dy);
    if (distance <= 0.0f || distance >= neighborDistance) {
      continue;
    }
    outX += s_ambientBoids[other].x;
    outY += s_ambientBoids[other].y;
    count++;
  }

  if (count <= 0) {
    outX = 0.0f;
    outY = 0.0f;
    return;
  }

  outX /= (float)count;
  outY /= (float)count;
  outX -= s_ambientBoids[index].x;
  outY -= s_ambientBoids[index].y;
  normalizeAmbientVector(outX, outY);
  outX *= maxSpeed;
  outY *= maxSpeed;
  outX -= s_ambientBoids[index].vx;
  outY -= s_ambientBoids[index].vy;
  limitAmbientVector(outX, outY, maxForce);
}

static void stepAmbientBoids(float speedUnit, float intensityUnit) {
  if (!s_ambientBoidsReady) {
    initAmbientBoids();
  }

  float maxSpeed = 0.75f + speedUnit * 1.25f;
  float maxForce = 0.03f + intensityUnit * 0.05f;
  float separationDistance = 5.0f + intensityUnit * 7.0f;
  float alignmentDistance = 10.0f + intensityUnit * 12.0f;
  float cohesionDistance = 12.0f + intensityUnit * 16.0f;

  for (int index = 0; index < 18; index++) {
    float sepX = 0.0f;
    float sepY = 0.0f;
    float aliX = 0.0f;
    float aliY = 0.0f;
    float cohX = 0.0f;
    float cohY = 0.0f;
    ambientSeparateBoid(index, separationDistance, maxSpeed, maxForce, sepX, sepY);
    ambientAlignBoid(index, alignmentDistance, maxSpeed, maxForce, aliX, aliY);
    ambientCohesionBoid(index, cohesionDistance, maxSpeed, maxForce, cohX, cohY);
    s_ambientBoids[index].ax = sepX * 1.4f + aliX + cohX;
    s_ambientBoids[index].ay = sepY * 1.4f + aliY + cohY;
  }

  for (int index = 0; index < 18; index++) {
    s_ambientBoids[index].vx += s_ambientBoids[index].ax;
    s_ambientBoids[index].vy += s_ambientBoids[index].ay;
    limitAmbientVector(s_ambientBoids[index].vx, s_ambientBoids[index].vy, maxSpeed);
    s_ambientBoids[index].x += s_ambientBoids[index].vx;
    s_ambientBoids[index].y += s_ambientBoids[index].vy;
    if (s_ambientBoids[index].x > 64.0f) {
      s_ambientBoids[index].x = 0.0f;
    } else if (s_ambientBoids[index].x < 0.0f) {
      s_ambientBoids[index].x = 64.0f;
    }
    if (s_ambientBoids[index].y > 64.0f) {
      s_ambientBoids[index].y = 0.0f;
    } else if (s_ambientBoids[index].y < 0.0f) {
      s_ambientBoids[index].y = 64.0f;
    }
  }
}

static int s_sortValues[64];
static bool s_sortReady = false;
static unsigned long s_sortElapsed = 0;
static uint8_t s_sortAlgorithm = 0;
static uint8_t s_sortPhase = 1;
static uint8_t s_sortWait = 0;
static int s_sortAccessA = -1;
static int s_sortAccessB = -1;
static int s_sortI = 0;
static int s_sortJ = 0;
static int s_sortMinIndex = 0;
static int s_sortKey = 0;
static int s_sortStack[64][2];
static int s_sortStackSize = 0;
static bool s_sortPartitioning = false;
static int s_sortLow = 0;
static int s_sortHigh = 0;
static int s_sortPivot = 0;
static int s_sortPivotI = 0;
static int s_sortPivotJ = 0;

static void initAmbientSortingValues() {
  for (int index = 0; index < 64; index++) {
    s_sortValues[index] = index + 1;
  }
  for (int index = 63; index > 0; index--) {
    int swapIndex = (index * 13 + 17) % (index + 1);
    int temp = s_sortValues[index];
    s_sortValues[index] = s_sortValues[swapIndex];
    s_sortValues[swapIndex] = temp;
  }
}

static void resetAmbientSortingForAlgorithm() {
  initAmbientSortingValues();
  s_sortPhase = 1;
  s_sortWait = 0;
  s_sortAccessA = -1;
  s_sortAccessB = -1;
  if (s_sortAlgorithm == 0) {
    s_sortI = 0;
    s_sortJ = 0;
    return;
  }
  if (s_sortAlgorithm == 1) {
    s_sortI = 0;
    s_sortJ = 1;
    s_sortMinIndex = 0;
    return;
  }
  if (s_sortAlgorithm == 2) {
    s_sortI = 1;
    s_sortKey = s_sortValues[1];
    s_sortJ = 0;
    return;
  }
  s_sortStackSize = 1;
  s_sortStack[0][0] = 0;
  s_sortStack[0][1] = 63;
  s_sortPartitioning = false;
}

static void initAmbientSorting() {
  s_sortAlgorithm = 0;
  resetAmbientSortingForAlgorithm();
  s_sortReady = true;
  s_sortElapsed = 0;
}

static bool stepAmbientSortBubble() {
  if (s_sortI >= 63) {
    return true;
  }
  if (s_sortJ < 63 - s_sortI) {
    s_sortAccessA = s_sortJ;
    s_sortAccessB = s_sortJ + 1;
    if (s_sortValues[s_sortJ] > s_sortValues[s_sortJ + 1]) {
      int temp = s_sortValues[s_sortJ];
      s_sortValues[s_sortJ] = s_sortValues[s_sortJ + 1];
      s_sortValues[s_sortJ + 1] = temp;
    }
    s_sortJ++;
    return false;
  }
  s_sortJ = 0;
  s_sortI++;
  return false;
}

static bool stepAmbientSortSelection() {
  if (s_sortI >= 63) {
    return true;
  }
  if (s_sortJ < 64) {
    s_sortAccessA = s_sortMinIndex;
    s_sortAccessB = s_sortJ;
    if (s_sortValues[s_sortJ] < s_sortValues[s_sortMinIndex]) {
      s_sortMinIndex = s_sortJ;
    }
    s_sortJ++;
    return false;
  }
  s_sortAccessA = s_sortI;
  s_sortAccessB = s_sortMinIndex;
  int temp = s_sortValues[s_sortMinIndex];
  s_sortValues[s_sortMinIndex] = s_sortValues[s_sortI];
  s_sortValues[s_sortI] = temp;
  s_sortI++;
  s_sortMinIndex = s_sortI;
  s_sortJ = s_sortI + 1;
  return false;
}

static bool stepAmbientSortInsertion() {
  if (s_sortI >= 64) {
    return true;
  }
  s_sortAccessA = max(0, s_sortJ);
  s_sortAccessB = s_sortI;
  if (s_sortJ >= 0 && s_sortValues[s_sortJ] > s_sortKey) {
    s_sortValues[s_sortJ + 1] = s_sortValues[s_sortJ];
    s_sortJ--;
    return false;
  }
  s_sortValues[s_sortJ + 1] = s_sortKey;
  s_sortI++;
  if (s_sortI < 64) {
    s_sortKey = s_sortValues[s_sortI];
    s_sortJ = s_sortI - 1;
  }
  return false;
}

static bool stepAmbientSortQuick() {
  if (!s_sortPartitioning) {
    while (s_sortStackSize > 0) {
      s_sortStackSize--;
      s_sortLow = s_sortStack[s_sortStackSize][0];
      s_sortHigh = s_sortStack[s_sortStackSize][1];
      if (s_sortLow < s_sortHigh) {
        s_sortPivot = s_sortValues[s_sortHigh];
        s_sortPivotI = s_sortLow - 1;
        s_sortPivotJ = s_sortLow;
        s_sortPartitioning = true;
        break;
      }
    }
    if (!s_sortPartitioning) {
      return true;
    }
  }

  if (s_sortPivotJ <= s_sortHigh - 1) {
    s_sortAccessA = s_sortPivotJ;
    s_sortAccessB = s_sortHigh;
    if (s_sortValues[s_sortPivotJ] < s_sortPivot) {
      s_sortPivotI++;
      int temp = s_sortValues[s_sortPivotI];
      s_sortValues[s_sortPivotI] = s_sortValues[s_sortPivotJ];
      s_sortValues[s_sortPivotJ] = temp;
    }
    s_sortPivotJ++;
    return false;
  }

  int pivotIndex = s_sortPivotI + 1;
  int temp = s_sortValues[pivotIndex];
  s_sortValues[pivotIndex] = s_sortValues[s_sortHigh];
  s_sortValues[s_sortHigh] = temp;
  s_sortPartitioning = false;
  s_sortAccessA = pivotIndex;
  s_sortAccessB = s_sortHigh;
  s_sortStack[s_sortStackSize][0] = s_sortLow;
  s_sortStack[s_sortStackSize][1] = pivotIndex - 1;
  s_sortStackSize++;
  s_sortStack[s_sortStackSize][0] = pivotIndex + 1;
  s_sortStack[s_sortStackSize][1] = s_sortHigh;
  s_sortStackSize++;
  return false;
}

static void stepAmbientSorting() {
  if (!s_sortReady) {
    initAmbientSorting();
  }

  if (s_sortPhase == 2) {
    s_sortWait++;
    if (s_sortWait >= 18) {
      s_sortAlgorithm = (uint8_t)((s_sortAlgorithm + 1) % 4);
      resetAmbientSortingForAlgorithm();
    }
    return;
  }

  bool done = false;
  for (int step = 0; step < 8 && !done; step++) {
    if (s_sortAlgorithm == 0) {
      done = stepAmbientSortBubble();
    } else if (s_sortAlgorithm == 1) {
      done = stepAmbientSortSelection();
    } else if (s_sortAlgorithm == 2) {
      done = stepAmbientSortInsertion();
    } else {
      done = stepAmbientSortQuick();
    }
  }

  if (done) {
    s_sortPhase = 2;
    s_sortAccessA = -1;
    s_sortAccessB = -1;
  }
}

static void drawAmbientDigit3x5(int digit, int x, int y, const NativeRgb& color) {
  static const char glyphs[10][5][4] = {
    {"111", "101", "101", "101", "111"},
    {"010", "110", "010", "010", "111"},
    {"111", "001", "111", "100", "111"},
    {"111", "001", "111", "001", "111"},
    {"101", "101", "111", "001", "001"},
    {"111", "100", "111", "001", "111"},
    {"111", "100", "111", "101", "111"},
    {"111", "001", "001", "001", "001"},
    {"111", "101", "111", "101", "111"},
    {"111", "101", "111", "001", "111"}
  };

  int safeDigit = digit;
  if (safeDigit < 0 || safeDigit > 9) {
    safeDigit = 0;
  }

  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      if (glyphs[safeDigit][row][col] != '1') {
        continue;
      }
      fillAmbientRect(x + col, y + row, 1, 1, color);
    }
  }
}

static void drawAmbientLine(int x0, int y0, int x1, int y1, const NativeRgb& color) {
  int startX = x0;
  int startY = y0;
  int endX = x1;
  int endY = y1;
  int dx = abs(endX - startX);
  int sx = startX < endX ? 1 : -1;
  int dy = -abs(endY - startY);
  int sy = startY < endY ? 1 : -1;
  int err = dx + dy;

  while (true) {
    DisplayManager::dma_display->drawPixelRGB888(startX, startY, color.r, color.g, color.b);
    if (startX == endX && startY == endY) {
      break;
    }
    int twice = err * 2;
    if (twice >= dy) {
      err += dy;
      startX += sx;
    }
    if (twice <= dx) {
      err += dx;
      startY += sy;
    }
  }
}

static void renderAmbientDigitalRain(unsigned long elapsed, float speedUnit) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);

  const int width = 64;
  const int height = 64;
  const int numDrops = 40;
  const float baseSpeed = 0.5f + speedUnit * 1.5f;

  for (int index = 0; index < numDrops; index++) {
    int x = (index * 37 + 11) % width;
    float lengthSeed = fireflySeedf(index, 0.91f);
    float speedSeed = fireflySeedf(index, 1.61f);
    float positionSeed = fireflySeedf(index, 2.31f);
    int length = 5 + (int)roundf(lengthSeed * ((float)height / 2.0f - 4.0f));
    float speed = baseSpeed * (0.5f + speedSeed);
    float y = -((float)height) + fractf32(positionSeed + (float)elapsed * 0.00055f * speed) * ((float)height * 2.0f);
    int headY = (int)floorf(y);

    for (int segment = 0; segment < length; segment++) {
      int py = headY - segment;
      if (py < 0 || py >= height) {
        continue;
      }
      float brightness = 1.0f - (float)segment / (float)max(1, length - 1);
      uint8_t green = clampByte((int)roundf(255.0f * brightness));
      DisplayManager::dma_display->drawPixelRGB888(x, py, 0, green, 0);
    }

    if (headY >= 0 && headY < height) {
      DisplayManager::dma_display->drawPixelRGB888(x, headY, 255, 255, 255);
    }
  }
}

static float metaBlobRandSin(int index) {
  return sinf((float)index * 1.64f);
}

static void renderAmbientMetaBlob(unsigned long elapsed) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);

  const int numBlobs = 10;
  const float threshold = 0.0003f;
  const float timeValue = (float)elapsed / 1000.0f;
  const float hue = fmodf(timeValue * 0.033f, 1.0f);

  struct MetaBlob {
    float x;
    float y;
    float radius;
  };

  MetaBlob blobs[numBlobs];
  for (int index = 0; index < numBlobs; index++) {
    float x = 0.5f + 0.1f * metaBlobRandSin(index);
    float y = 0.5f + 0.1f * metaBlobRandSin(index + 42);
    x += 0.5f * sinf(0.25f * timeValue * metaBlobRandSin(index + 2)) * metaBlobRandSin(index + 56);
    y += 0.5f * -sinf(0.25f * timeValue) * metaBlobRandSin(index * 9);
    float radius = 0.1f * fabsf(metaBlobRandSin(index + 3));
    blobs[index] = {
      x * 64.0f,
      y * 64.0f,
      radius * 64.0f
    };
  }

  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      float field = 0.0f;
      for (int index = 0; index < numBlobs; index++) {
        float dx = (float)x - blobs[index].x;
        float dy = (float)y - blobs[index].y;
        float distance = max(sqrtf(dx * dx + dy * dy) + blobs[index].radius / 2.0f, 0.0f);
        float temp = distance * distance;
        field += 1.0f / (temp * temp);
      }

      if (field <= threshold) {
        continue;
      }

      float t = min(1.0f, (field - threshold) / threshold);
      t = t * t * (3.0f - 2.0f * t);
      NativeRgb center = ambientHsvToRgb(hue, 1.0f, 1.0f);
      NativeRgb edge = ambientHsvToRgb(hue + 0.25f, 1.0f, 1.0f);
      NativeRgb color = blendRgbColor(edge, center, t);
      DisplayManager::dma_display->drawPixelRGB888(x, y, color.r, color.g, color.b);
    }
  }
}

static void renderAmbientStarfield(unsigned long elapsed, float speedUnit) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);

  const int centerX = 32;
  const int centerY = 32;
  const float maxDepth = 3.0f;
  const int starCount = 50;
  const float travelRate = 0.00022f + speedUnit * 0.00022f;

  for (int index = 0; index < starCount; index++) {
    float seedX = fireflySeedf(index, 0.11f);
    float seedY = fireflySeedf(index, 1.73f);
    float seedZ = fireflySeedf(index, 2.91f);
    float seedT = fireflySeedf(index, 3.71f);
    float x3d = seedX * 2.0f - 1.0f;
    float y3d = seedY * 2.0f - 1.0f;
    float travel = fractf32(seedZ + (float)elapsed * travelRate * (0.65f + seedT * 0.7f));
    float z = max(0.08f, maxDepth - travel * maxDepth);
    float perspective = 0.5f / z;
    int x = (int)roundf(x3d * perspective * (float)centerX + (float)centerX);
    int y = (int)roundf(y3d * perspective * (float)centerY + (float)centerY);

    if (x < 0 || x >= 64 || y < 0 || y >= 64) {
      continue;
    }

    float depthFactor = clampUnit((maxDepth - z) / maxDepth);
    float twinkle =
      0.8f +
      0.2f * (0.5f + 0.5f * sinf((float)elapsed * 0.0014f * (0.7f + seedT) + seedX * 6.2831852f));
    uint8_t brightness = clampByte((int)roundf(255.0f * sqrtf(depthFactor) * twinkle));

    DisplayManager::dma_display->drawPixelRGB888(x, y, brightness, brightness, brightness);

    if (z < maxDepth * 0.3f) {
      NativeRgb glow = makeRgb(brightness / 4, brightness / 4, brightness / 4);
      drawAmbientGlowDot(x, y, glow, 0.5f, 1);
    }
  }
}

static void renderAmbientNeonTunnel(unsigned long elapsed, float speedUnit) {
  float timeCounter = ((float)elapsed / 120.0f) * 0.05f;
  float centerX = 32.0f;
  float centerY = 32.0f;
  float oscX = centerX + sinf(timeCounter * 0.5f) * 16.0f;
  float oscY = centerY + cosf(timeCounter * 0.7f) * 16.0f;
  float distanceFactor = 1.25f + speedUnit * 0.8f;
  float angleFactor = 2.4f;
  float hueShiftSpeed = 0.2f + speedUnit * 0.35f;

  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      float dx = (float)x - oscX;
      float dy = (float)y - oscY;
      float distance = max(0.001f, sqrtf(dx * dx + dy * dy));
      float angle = atan2f(dy, dx);
      float uvX = distanceFactor / distance + speedUnit * timeCounter;
      float uvY = (angle * angleFactor / 3.1415926f) + sinf(timeCounter) * 2.0f;
      int texX = ((int)roundf(fabsf(uvX * 32.0f))) % 256;
      int texY = ((int)roundf(fabsf(uvY * 32.0f))) % 256;
      int pattern = texX ^ texY;
      float depthShade = clampUnit(1.0f - distance / 96.0f);
      float hue = fmodf(timeCounter * hueShiftSpeed * 50.0f + distance * 2.0f, 360.0f);
      float lightness = pattern > 128 ? 0.5f * depthShade : 0.0f;
      if (lightness <= 0.0f) {
        DisplayManager::dma_display->drawPixelRGB888(x, y, 0, 0, 0);
        continue;
      }
      NativeRgb color = ambientHslToRgb(hue, 1.0f, lightness);
      DisplayManager::dma_display->drawPixelRGB888(x, y, color.r, color.g, color.b);
    }
  }
}

static void renderAmbientRainbowRain(unsigned long elapsed, float speedUnit, uint8_t density, const NativeRgb& color) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);

  int safeDensity = density;
  if (safeDensity < 10) safeDensity = 10;
  if (safeDensity > 100) safeDensity = 100;
  float densityUnit = (float)safeDensity / 100.0f;
  int dropletCount = (int)roundf(96.0f + densityUnit * 560.0f);
  float driftRate = 0.00018f + speedUnit * 0.00042f;

  for (int index = 0; index < dropletCount; index++) {
    float seedA = fireflySeedf(index, 0.27f);
    float seedB = fireflySeedf(index, 1.11f);
    float seedC = fireflySeedf(index, 2.43f);
    float seedD = fireflySeedf(index, 3.92f);
    float sway = sinf((float)elapsed * (0.0007f + seedB * 0.0009f) + seedD * 6.2831852f) * 0.35f;
    int lane = (int)floorf(seedA * 64.0f);
    int x = max(0, min(63, (int)roundf((float)lane + sway)));
    float velocity = 0.45f + seedC * 1.65f;
    float travel = fractf32(seedD + (float)elapsed * driftRate * velocity);
    int y = (int)floorf(travel * 68.0f) - 2;
    if (y < 0 || y >= 64) {
      continue;
    }

    float brightness = 0.28f + seedB * 0.72f;
    NativeRgb dropColor = scaleRgbColor(color, brightness);
    DisplayManager::dma_display->drawPixelRGB888(x, y, dropColor.r, dropColor.g, dropColor.b);
  }
}

static void renderAmbientBoids(unsigned long elapsed, float speedUnit, float intensityUnit) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);
  if (!s_ambientBoidsReady || elapsed < s_ambientBoidsElapsed) {
    initAmbientBoids();
  }
  int steps = max(1, (int)((elapsed - s_ambientBoidsElapsed) / 40UL));
  for (int step = 0; step < steps; step++) {
    stepAmbientBoids(speedUnit, intensityUnit);
  }
  s_ambientBoidsElapsed = elapsed;

  for (int index = 0; index < 18; index++) {
    int x = (int)roundf(s_ambientBoids[index].x);
    int y = (int)roundf(s_ambientBoids[index].y);
    DisplayManager::dma_display->drawPixelRGB888(
      x,
      y,
      s_ambientBoids[index].color.r,
      s_ambientBoids[index].color.g,
      s_ambientBoids[index].color.b
    );
  }
}

static void renderAmbientBouncingLogo(unsigned long elapsed, float speedUnit) {
  DisplayManager::dma_display->fillScreenRGB888(8, 11, 18);
  int travelX = 8 + abs((int)fmodf((float)elapsed * (0.02f + speedUnit * 0.02f), 96.0f) - 48);
  int travelY = 8 + abs((int)fmodf((float)elapsed * (0.016f + speedUnit * 0.018f) + 12.0f, 84.0f) - 42);
  int x = min(46, travelX);
  int y = min(46, travelY);
  DisplayManager::drawLogo(x, y);
  drawAmbientGlowDot(x + 16, y + 16, makeRgb(110, 180, 255), 0.32f, 5);
}

static void renderAmbientSortingVisualizer(unsigned long elapsed, float intensityUnit) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);
  if (!s_sortReady || elapsed < s_sortElapsed) {
    initAmbientSorting();
  }
  int frames = max(1, (int)((elapsed - s_sortElapsed) / 80UL));
  for (int step = 0; step < frames; step++) {
    stepAmbientSorting();
  }
  s_sortElapsed = elapsed;

  for (int index = 0; index < 64; index++) {
    int barHeight = max(1, (s_sortValues[index] * 64) / 64);
    NativeRgb color;
    if (s_sortPhase == 1 && (index == s_sortAccessA || index == s_sortAccessB)) {
      color = makeRgb(255, 0, 0);
    } else if (s_sortPhase == 2) {
      color = makeRgb(0, 255, 0);
    } else {
      color = ambientHslToRgb(((float)s_sortValues[index] / 64.0f) * 360.0f, 1.0f, 0.5f);
    }
    color = addHighlightColor(color, intensityUnit * 0.08f);
    for (int y = 63; y >= 64 - barHeight; y--) {
      DisplayManager::dma_display->drawPixelRGB888(index, y, color.r, color.g, color.b);
    }
  }
}

static void renderAmbientClockScene(unsigned long elapsed, float intensityUnit) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);

  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  int second = timeinfo != nullptr ? timeinfo->tm_sec : 0;
  int minuteValue = timeinfo != nullptr ? timeinfo->tm_min : 34;
  int hourValue = timeinfo != nullptr ? timeinfo->tm_hour : 12;

  float secondProgress = (float)((second + (int)(elapsed / 120UL)) % 60);
  float minute = (float)minuteValue + secondProgress / 60.0f;
  float hour = (float)(hourValue % 12) + minute / 60.0f;

  int centerX = 32;
  int centerY = 21;
  int radius = 18;

  for (int index = 0; index < 12; index++) {
    float angle = (float)index * 3.1415926f / 6.0f;
    int x = centerX + (int)roundf(sinf(angle) * (float)radius);
    int y = centerY - (int)roundf(cosf(angle) * (float)radius);
    if (index % 3 == 0) {
      DisplayManager::dma_display->drawPixelRGB888(x, y, 200, 200, 200);
      DisplayManager::dma_display->drawPixelRGB888(x - 1, y, 150, 150, 150);
      DisplayManager::dma_display->drawPixelRGB888(x + 1, y, 150, 150, 150);
      DisplayManager::dma_display->drawPixelRGB888(x, y - 1, 150, 150, 150);
      DisplayManager::dma_display->drawPixelRGB888(x, y + 1, 150, 150, 150);
    } else {
      DisplayManager::dma_display->drawPixelRGB888(x, y, 100, 100, 100);
    }
  }

  float hourAngle = hour / 12.0f * 6.2831852f - 1.5707963f;
  float minuteAngle = minute / 60.0f * 6.2831852f - 1.5707963f;
  float secondAngle = secondProgress / 60.0f * 6.2831852f - 1.5707963f;
  drawAmbientLine(centerX, centerY, centerX + (int)roundf(cosf(hourAngle) * 9.0f), centerY + (int)roundf(sinf(hourAngle) * 9.0f), makeRgb(84, 196, 255));
  drawAmbientLine(centerX, centerY, centerX + (int)roundf(cosf(minuteAngle) * 13.0f), centerY + (int)roundf(sinf(minuteAngle) * 13.0f), makeRgb(220, 220, 220));
  drawAmbientLine(centerX, centerY, centerX + (int)roundf(cosf(secondAngle) * 15.0f), centerY + (int)roundf(sinf(secondAngle) * 15.0f), makeRgb(255, 100, 130));
  fillAmbientRect(centerX - 1, centerY - 1, 3, 3, addHighlightColor(makeRgb(255, 255, 255), intensityUnit * 0.08f));

  int displayHour = hourValue % 12;
  if (displayHour == 0) {
    displayHour = 12;
  }
  bool isPm = hourValue >= 12;
  int xStart = 20;
  int yStart = 48;
  NativeRgb hourColor = makeRgb(84, 196, 255);
  NativeRgb minuteColor = makeRgb(220, 220, 220);
  drawAmbientDigit3x5(displayHour / 10, xStart, yStart, hourColor);
  drawAmbientDigit3x5(displayHour % 10, xStart + 5, yStart, hourColor);
  fillAmbientRect(xStart + 9, yStart + 1, 1, 1, makeRgb(255, 255, 255));
  fillAmbientRect(xStart + 9, yStart + 3, 1, 1, makeRgb(255, 255, 255));
  drawAmbientDigit3x5(minuteValue / 10, xStart + 11, yStart, minuteColor);
  drawAmbientDigit3x5(minuteValue % 10, xStart + 16, yStart, minuteColor);
  if (isPm) {
    fillAmbientRect(xStart + 22, yStart, 1, 1, makeRgb(255, 255, 255));
    fillAmbientRect(xStart + 22, yStart + 1, 1, 1, makeRgb(255, 255, 255));
    fillAmbientRect(xStart + 23, yStart, 1, 1, makeRgb(255, 255, 255));
    fillAmbientRect(xStart + 23, yStart + 1, 1, 1, makeRgb(255, 255, 255));
  } else {
    fillAmbientRect(xStart + 22, yStart + 3, 1, 1, makeRgb(255, 255, 255));
    fillAmbientRect(xStart + 22, yStart + 4, 1, 1, makeRgb(255, 255, 255));
    fillAmbientRect(xStart + 23, yStart + 3, 1, 1, makeRgb(255, 255, 255));
    fillAmbientRect(xStart + 23, yStart + 4, 1, 1, makeRgb(255, 255, 255));
  }
}

static void renderAmbientCountdownScene(unsigned long elapsed, float speedUnit) {
  DisplayManager::dma_display->fillScreenRGB888(8, 10, 18);
  int totalSeconds = 300;
  int spent = ((int)floorf(((float)elapsed / 1000.0f) * (0.4f + speedUnit * 1.2f))) % totalSeconds;
  int remaining = totalSeconds - spent;
  int minutes = remaining / 60;
  int seconds = remaining % 60;
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
  float progress = (float)remaining / (float)totalSeconds;
  bool warning = progress < 0.25f;
  NativeRgb accent = warning ? makeRgb(255, 108, 82) : makeRgb(255, 212, 82);
  NativeRgb border = warning ? makeRgb(124, 48, 44) : makeRgb(108, 84, 40);
  drawAmbientPanelFrame(5, 13, 54, 38, makeRgb(10, 16, 28), border, scaleRgbColor(accent, 0.55f));
  DisplayManager::drawTinyText("TIMER", 10, 17, DisplayManager::dma_display->color565(accent.r, accent.g, accent.b), 1);
  drawAmbientChip(42, 17, 10, scaleRgbColor(accent, 0.7f));
  drawAmbientTextCenteredInBox(buffer, 5, 54, 26, accent, 2);
  drawAmbientRectOutline(10, 43, 44, 5, border);
  int fillWidth = max(2, (int)roundf(progress * 42.0f));
  fillAmbientRect(11, 44, fillWidth, 3, accent);
  drawAmbientGlowDot(11 + fillWidth, 45, accent, 0.28f, 1);
}

static void renderAmbientWeatherScene(unsigned long elapsed, float speedUnit) {
  DisplayManager::dma_display->fillScreenRGB888(6, 10, 18);
  int phase = ((int)floorf(((float)elapsed / 1000.0f) * (0.28f + speedUnit * 0.2f))) % 3;
  int temp = phase == 0 ? 26 : (phase == 1 ? 18 : 8);
  char buffer[5];
  snprintf(buffer, sizeof(buffer), "%02dC", temp);
  NativeRgb panelBg = makeRgb(10, 16, 28);
  NativeRgb border = makeRgb(30, 64, 94);
  NativeRgb accent = makeRgb(224, 244, 255);
  drawAmbientPanelFrame(6, 10, 52, 40, panelBg, border, makeRgb(92, 150, 198));

  if (phase == 0) {
    drawAmbientSunIcon(18, 24, makeRgb(255, 206, 84));
    drawAmbientCloudIcon(10, 23, makeRgb(228, 236, 244), makeRgb(126, 184, 232));
    DisplayManager::drawTinyText("SUN", 10, 41, DisplayManager::dma_display->color565(255, 214, 102), 1);
  } else if (phase == 1) {
    drawAmbientCloudIcon(8, 18, makeRgb(196, 214, 232), makeRgb(120, 176, 228));
    drawAmbientRainDrops(13, 32, makeRgb(88, 176, 255));
    DisplayManager::drawTinyText("RAIN", 8, 41, DisplayManager::dma_display->color565(146, 204, 255), 1);
  } else {
    drawAmbientCloudIcon(8, 18, makeRgb(214, 226, 236), makeRgb(156, 198, 236));
    drawAmbientSnowFlakes(12, 33, makeRgb(240, 248, 255));
    DisplayManager::drawTinyText("SNOW", 8, 41, DisplayManager::dma_display->color565(214, 234, 255), 1);
  }

  DisplayManager::drawTinyText(buffer, 32, 24, DisplayManager::dma_display->color565(accent.r, accent.g, accent.b), 2);
  fillAmbientRect(35, 18, 18, 1, makeRgb(80, 132, 176));
}

static void renderAmbientGameOfLife(unsigned long elapsed, float intensityUnit) {
  DisplayManager::dma_display->fillScreenRGB888(4, 9, 13);
  uint8_t grid[16][16];
  uint8_t next[16][16];
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      float seed = fireflySeedf(y * 16 + x, 0.42f);
      grid[y][x] = seed > 0.58f ? 1 : 0;
    }
  }

  int steps = (int)(elapsed / 120UL) + 2;
  for (int step = 0; step < steps; step++) {
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        int neighbors = 0;
        for (int oy = -1; oy <= 1; oy++) {
          for (int ox = -1; ox <= 1; ox++) {
            if (ox == 0 && oy == 0) {
              continue;
            }
            int px = (x + ox + 16) % 16;
            int py = (y + oy + 16) % 16;
            neighbors += grid[py][px];
          }
        }
        bool alive = grid[y][x] == 1;
        next[y][x] = alive ? ((neighbors == 2 || neighbors == 3) ? 1 : 0) : (neighbors == 3 ? 1 : 0);
      }
    }
    memcpy(grid, next, sizeof(grid));
  }

  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      if (grid[y][x] != 1) {
        continue;
      }
      NativeRgb color = addHighlightColor(
        blendRgbColor(makeRgb(68, 255, 144), makeRgb(82, 178, 255), (float)(x + y) / 30.0f),
        intensityUnit * 0.16f
      );
      fillAmbientRect(x * 4, y * 4, 3, 3, color);
    }
  }
}

static void renderAmbientJuliaSet(unsigned long elapsed, float intensityUnit) {
  float timeBase = (float)elapsed * 0.0012f;
  float cx = -0.7f + sinf(timeBase * 0.6f) * 0.12f;
  float cy = 0.27015f + cosf(timeBase * 0.8f) * 0.08f;

  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      float zx = ((float)x - 32.0f) / 22.0f;
      float zy = ((float)y - 32.0f) / 22.0f;
      int iteration = 0;
      while (zx * zx + zy * zy < 4.0f && iteration < 18) {
        float nextX = zx * zx - zy * zy + cx;
        zy = 2.0f * zx * zy + cy;
        zx = nextX;
        iteration++;
      }
      float ratio = (float)iteration / 18.0f;
      NativeRgb color = addHighlightColor(
        blendRgbColor(makeRgb(34, 18, 92), makeRgb(82, 212, 255), ratio),
        intensityUnit * 0.12f
      );
      DisplayManager::dma_display->drawPixelRGB888(
        x,
        y,
        clampByte((int)roundf((float)color.r * ratio)),
        clampByte((int)roundf((float)color.g * ratio)),
        clampByte((int)roundf((float)color.b * ratio))
      );
    }
  }
}

static void renderAmbientReactionDiffusion(unsigned long elapsed, float speedUnit, float intensityUnit) {
  float timeBase = (float)elapsed * (0.0011f + speedUnit * 0.0011f);
  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      float nx = ((float)x - 31.5f) / 31.5f;
      float ny = ((float)y - 31.5f) / 31.5f;
      float fieldA = 0.5f + 0.5f * sinf(nx * 8.2f + timeBase * 1.6f + sinf(ny * 6.4f));
      float fieldB = 0.5f + 0.5f * cosf(ny * 8.8f - timeBase * 1.2f + sinf(nx * 5.6f));
      float contour = smoothstepf(0.46f, 0.62f + intensityUnit * 0.08f, fieldA * 0.58f + fieldB * 0.42f);
      NativeRgb color = blendRgbColor(makeRgb(22, 38, 88), makeRgb(96, 255, 204), contour);
      DisplayManager::dma_display->drawPixelRGB888(
        x,
        y,
        clampByte((int)roundf((float)color.r * contour)),
        clampByte((int)roundf((float)color.g * contour)),
        clampByte((int)roundf((float)color.b * contour))
      );
    }
  }
}

static bool renderAmbientExtendedScene(uint8_t preset, unsigned long elapsed, float speedUnit, float intensityUnit) {
  if (preset == AMBIENT_PRESET_AURORA) {
    renderAmbientDigitalRain(elapsed, speedUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_PLASMA) {
    renderAmbientStarfield(elapsed, speedUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_MATRIX_RAIN) {
    renderAmbientNeonTunnel(elapsed, speedUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_BOIDS) {
    renderAmbientBoids(elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_FIREFLY_SWARM) {
    renderAmbientMetaBlob(elapsed);
    return true;
  }
  if (preset == AMBIENT_PRESET_BOUNCING_LOGO) {
    renderAmbientBouncingLogo(elapsed, speedUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_SORTING_VISUALIZER) {
    renderAmbientSortingVisualizer(elapsed, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_CLOCK_SCENE) {
    renderAmbientClockScene(elapsed, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_COUNTDOWN_SCENE) {
    renderAmbientCountdownScene(elapsed, speedUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_WEATHER_SCENE) {
    renderAmbientWeatherScene(elapsed, speedUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_GAME_OF_LIFE) {
    renderAmbientGameOfLife(elapsed, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_JULIA_SET) {
    renderAmbientJuliaSet(elapsed, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_REACTION_DIFFUSION) {
    renderAmbientReactionDiffusion(elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_SUNSET_BLUSH) {
    renderAmbientRainbowRain(
      elapsed,
      speedUnit,
      DisplayManager::ambientEffectConfig.density,
      makeRgb(
        DisplayManager::ambientEffectConfig.colorR,
        DisplayManager::ambientEffectConfig.colorG,
        DisplayManager::ambientEffectConfig.colorB
      )
    );
    return true;
  }
  return false;
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

    if (renderAmbientExtendedScene(ambientEffectConfig.preset, elapsed, speedUnit, intensityUnit)) {
      return;
    }

    for (int y = 0; y < PANEL_RES_Y; y++) {
      for (int x = 0; x < PANEL_RES_X; x++) {
        float nx = ((float)x - 31.5f) / 31.5f;
        float ny = ((float)y - 31.5f) / 31.5f;
        float dist = sqrtf(nx * nx + ny * ny);

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        if (ambientEffectConfig.preset == AMBIENT_PRESET_AURORA) {
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
          g = clampByte((int)roundf(26.0f + 220.0f * brightness));
          b = clampByte((int)roundf(8.0f + 92.0f * (tail * 0.55f + headGlow * 0.35f + tailSecondary * 0.22f)));
          if (((x + (int)(elapsed / 42UL)) % 9) == 0 && tail > 0.16f) {
            addHighlight(r, g, b, 0.1f + intensityUnit * 0.12f);
          }
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_MATRIX_RAIN) {
          float angle = atan2f(ny, nx) / (3.1415926f * 2.0f);
          float tunnelDepth = fractf32(dist * 4.4f - timeBase * (1.8f + speedUnit * 1.1f));
          float ringGlow = clampUnit(1.0f - fabsf(tunnelDepth - 0.5f) * 6.5f);
          float laneGlow = clampUnit(1.0f - fabsf(fractf32(angle * 6.0f + timeBase * 0.22f) - 0.5f) * 4.6f);
          float rimGlow = clampUnit(1.0f - fabsf(dist - 0.88f) * 8.5f);
          float centerFade = clampUnit(1.0f - dist * 0.92f);
          float pulse = 0.5f + 0.5f * sinf(timeBase * 3.2f + angle * 8.0f);
          float glow = clampUnit(
            0.04f +
            ringGlow * (0.46f + intensityUnit * 0.2f) +
            laneGlow * 0.24f +
            rimGlow * 0.18f +
            centerFade * 0.08f
          );

          r = clampByte((int)roundf((20.0f + 180.0f * laneGlow + 45.0f * pulse) * glow));
          g = clampByte((int)roundf((10.0f + 70.0f * pulse + 30.0f * rimGlow) * glow));
          b = clampByte((int)roundf((40.0f + 210.0f * ringGlow + 80.0f * laneGlow) * glow));
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_FIREFLY_SWARM) {
          float field = 0.0f;
          for (int index = 0; index < 4; index++) {
            float seedA = fireflySeedf(index, 0.3f);
            float seedB = fireflySeedf(index, 1.6f);
            float blobX = sinf(timeBase * (0.72f + seedA * 0.65f) + seedB * 6.2831852f) * (0.24f + seedA * 0.38f);
            float blobY = cosf(timeBase * (0.94f + seedB * 0.52f) + seedA * 6.2831852f) * (0.22f + seedB * 0.36f);
            float dx = nx - blobX;
            float dy = ny - blobY;
            field += (0.065f + intensityUnit * 0.032f) / (dx * dx + dy * dy + 0.026f);
          }
          float iso = clampUnit((field - 0.82f) / (1.9f + intensityUnit * 0.4f));
          float edge = smoothstepf(1.8f, 2.45f + intensityUnit * 0.3f, field);
          float shimmer = 0.5f + 0.5f * sinf(field * 4.2f - timeBase * 2.1f);
          float glow = clampUnit(iso * 0.78f + edge * 0.22f);

          r = clampByte((int)roundf((8.0f + 46.0f * shimmer + 22.0f * edge) * glow));
          g = clampByte((int)roundf((24.0f + 150.0f * iso + 55.0f * shimmer) * glow));
          b = clampByte((int)roundf((42.0f + 150.0f * edge + 48.0f * (1.0f - shimmer)) * glow));
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_METEOR_SHOWER) {
          float depth = (float)y / 63.0f;
          float duneBase =
            0.74f +
            sinf(nx * 5.4f + timeBase * 0.22f) * 0.045f +
            sinf(nx * 10.8f - timeBase * 0.14f) * 0.018f;
          float duneGlow = clampUnit((depth - duneBase) / 0.18f);
          float driftA = fractf32(timeBase * (0.62f + speedUnit * 0.68f) + (float)x * 0.071f);
          float driftB = fractf32(timeBase * (0.48f + speedUnit * 0.44f) + (float)x * 0.127f + 0.31f);
          float grainA =
            smoothstepf(0.018f, 0.0f, fabsf(depth - driftA * 0.76f)) *
            smoothstepf(0.08f, 0.01f, fabsf(fractf32((float)x * 0.19f) - 0.5f));
          float grainB =
            smoothstepf(0.016f, 0.0f, fabsf(depth - driftB * 0.7f)) *
            smoothstepf(0.12f, 0.02f, fabsf(fractf32((float)x * 0.11f + 0.4f) - 0.5f));
          float sparkle = clampUnit((grainA + grainB) * (0.8f + intensityUnit * 0.4f));
          float haze = clampUnit(0.04f + duneGlow * 0.36f + sparkle * 0.32f);

          r = clampByte((int)roundf((18.0f + 170.0f * duneGlow + 120.0f * sparkle) * haze));
          g = clampByte((int)roundf((10.0f + 104.0f * duneGlow + 76.0f * sparkle) * haze));
          b = clampByte((int)roundf((4.0f + 34.0f * duneGlow + 16.0f * sparkle) * haze));
          if (sparkle > 0.48f) {
            addHighlight(r, g, b, 0.08f + intensityUnit * 0.12f);
          }
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_OCEAN_CURRENT) {
          float ember = clampUnit(1.0f - dist * 1.1f) * 0.08f;
          r = clampByte((int)roundf(2.0f + 24.0f * ember));
          g = clampByte((int)roundf(1.0f + 12.0f * ember));
          b = clampByte((int)roundf(1.0f + 6.0f * ember));

          const float launchX = 0.0f;
          const float launchY = 0.55f;
          for (int index = 0; index < 8; index++) {
            float seedA = fireflySeedf(index, 0.4f);
            float seedB = fireflySeedf(index, 2.1f);
            float angle = -1.9f + seedA * 0.95f;
            float progress = fractf32(timeBase * (0.34f + speedUnit * 0.64f) + seedB);
            float radius = progress * (0.65f + seedA * 0.35f);
            float sx = launchX + cosf(angle) * radius;
            float sy = launchY + sinf(angle) * radius * 1.2f;
            float dx = nx - sx;
            float dy = ny - sy;
            float tip = clampUnit(1.0f - sqrtf(dx * dx + dy * dy) / 0.085f);
            float tail = clampUnit(
              1.0f - sqrtf((nx - (sx * 0.78f)) * (nx - (sx * 0.78f)) + (ny - (sy * 0.78f + 0.12f)) * (ny - (sy * 0.78f + 0.12f))) / 0.15f
            );
            r = clampByte((int)r + (int)roundf(180.0f * tip + 96.0f * tail));
            g = clampByte((int)g + (int)roundf(110.0f * tip + 54.0f * tail));
            b = clampByte((int)b + (int)roundf(26.0f * tip + 10.0f * tail));
          }

          if (ny > 0.55f) {
            float floorGlow = clampUnit((ny - 0.55f) / 0.45f) * 0.12f;
            r = clampByte((int)r + (int)roundf(38.0f * floorGlow));
            g = clampByte((int)g + (int)roundf(12.0f * floorGlow));
          }
          if (intensityUnit > 0.4f && fabsf(nx) < 0.04f && ny > 0.18f) {
            r = clampByte((int)r + 30);
            g = clampByte((int)g + 10);
          }
        } else if (ambientEffectConfig.preset == AMBIENT_PRESET_NEON_GRID) {
          float waveA = 0.5f + 0.5f * sinf(nx * 7.2f - timeBase * 1.6f + sinf(ny * 4.6f));
          float waveB = 0.5f + 0.5f * sinf(ny * 11.4f + timeBase * 1.1f + nx * 3.6f);
          float stripe = clampUnit(1.0f - fabsf(fractf32(waveA * 1.8f + waveB * 1.2f) - 0.5f) * 5.8f);
          float glow = clampUnit(0.06f + stripe * (0.58f + intensityUnit * 0.14f) + waveB * 0.16f);

          r = clampByte((int)roundf((8.0f + 44.0f * waveB + 16.0f * stripe) * glow));
          g = clampByte((int)roundf((18.0f + 110.0f * stripe + 40.0f * waveA) * glow));
          b = clampByte((int)roundf((40.0f + 170.0f * waveA + 72.0f * stripe) * glow));
        } else {
          float plasmaA = 0.5f + 0.5f * sinf(nx * 5.8f + timeBase * 1.2f + sinf(ny * 4.2f));
          float plasmaB = 0.5f + 0.5f * sinf((nx + ny) * 6.6f - timeBase * 1.9f);
          float rind = 0.5f + 0.5f * sinf(dist * 10.4f - timeBase * 1.5f);
          float field = clampUnit(plasmaA * 0.48f + plasmaB * 0.34f + rind * 0.18f);
          float flesh = clampUnit(0.3f + plasmaA * 0.7f);
          float glow = clampUnit(0.16f + field * (0.52f + intensityUnit * 0.2f));

          r = clampByte((int)roundf((22.0f + 215.0f * flesh) * glow));
          g = clampByte((int)roundf((14.0f + 145.0f * (1.0f - flesh) + 42.0f * rind) * glow));
          b = clampByte((int)roundf((12.0f + 54.0f * (1.0f - field)) * glow));

          if (((int)floorf((nx + 1.0f) * 18.0f) + (int)floorf((ny + 1.0f) * 18.0f)) % 19 == 0) {
            r = clampByte((int)roundf((float)r * 0.62f));
            g = clampByte((int)roundf((float)g * 0.42f));
            b = clampByte((int)roundf((float)b * 0.38f));
          }
          addHighlight(r, g, b, field * 0.08f);
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
