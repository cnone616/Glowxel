#include "display_manager.h"
#include "clock_font_renderer.h"
#include "eyes_effect.h"
#include "mode_tags.h"
#include "theme_renderer.h"
#include "wifi_manager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <time.h>
#include <math.h>

// 静态成员初始化
CaptureMatrixPanel* DisplayManager::dma_display = nullptr;
DeviceMode DisplayManager::currentMode = MODE_CLOCK;
DeviceMode DisplayManager::lastBusinessMode = MODE_CLOCK;
String DisplayManager::currentBusinessModeTag = ModeTags::CLOCK;
String DisplayManager::lastBusinessModeTag = ModeTags::CLOCK;
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
static uint16_t* s_redirectFrameBuffer = nullptr;
static bool s_redirectFrameActive = false;
static SemaphoreHandle_t s_runtimeAccessMutex = nullptr;
static bool s_forceStaticClockOverlayRefresh = false;
static unsigned long s_lastPresentDurationUs = 0;
static uint16_t s_lastPresentChangedPixels = 0;
static uint32_t s_lastPresentFrameHash = 0;
struct AmbientWaterSurfacePerfStats {
  unsigned long windowStartedAt;
  unsigned long frameCount;
  uint64_t drawUsTotal;
  uint64_t presentUsTotal;
  uint32_t changedPixelsTotal;
  unsigned long maxDrawUs;
  unsigned long maxPresentUs;
};
static AmbientWaterSurfacePerfStats s_ambientWaterSurfacePerfStats = {};
static unsigned long s_pendingAmbientWaterSurfaceDrawUs = 0;

namespace {
void ensureRuntimeAccessMutex() {
  if (s_runtimeAccessMutex == nullptr) {
    s_runtimeAccessMutex = xSemaphoreCreateRecursiveMutex();
  }
}

bool isValidDriver(uint8_t driver) {
  return driver <= 5;
}

bool shouldTrackAmbientWaterSurfacePerf() {
  return DisplayManager::nativeEffectType == NATIVE_EFFECT_AMBIENT &&
         DisplayManager::ambientEffectConfig.preset == AMBIENT_PRESET_WATER_SURFACE &&
         DisplayManager::currentBusinessModeTag == ModeTags::LED_MATRIX_SHOWCASE;
}

void resetAmbientWaterSurfacePerfWindow(unsigned long now) {
  s_ambientWaterSurfacePerfStats.windowStartedAt = now;
  s_ambientWaterSurfacePerfStats.frameCount = 0;
  s_ambientWaterSurfacePerfStats.drawUsTotal = 0;
  s_ambientWaterSurfacePerfStats.presentUsTotal = 0;
  s_ambientWaterSurfacePerfStats.changedPixelsTotal = 0;
  s_ambientWaterSurfacePerfStats.maxDrawUs = 0;
  s_ambientWaterSurfacePerfStats.maxPresentUs = 0;
}

void maybeLogAmbientWaterSurfacePerf(unsigned long now) {
  if (!shouldTrackAmbientWaterSurfacePerf()) {
    resetAmbientWaterSurfacePerfWindow(now);
    return;
  }

  if (s_ambientWaterSurfacePerfStats.windowStartedAt == 0) {
    resetAmbientWaterSurfacePerfWindow(now);
    return;
  }

  unsigned long windowMs = now - s_ambientWaterSurfacePerfStats.windowStartedAt;
  if (windowMs < 1000UL || s_ambientWaterSurfacePerfStats.frameCount == 0) {
    return;
  }

  unsigned long avgDrawUs =
    (unsigned long)(s_ambientWaterSurfacePerfStats.drawUsTotal /
                    s_ambientWaterSurfacePerfStats.frameCount);
  unsigned long avgPresentUs =
    (unsigned long)(s_ambientWaterSurfacePerfStats.presentUsTotal /
                    s_ambientWaterSurfacePerfStats.frameCount);
  unsigned long avgChangedPixels =
    (unsigned long)(s_ambientWaterSurfacePerfStats.changedPixelsTotal /
                    s_ambientWaterSurfacePerfStats.frameCount);

  Serial.printf(
    "[WATER SURFACE PERF] window=%lums frames=%lu avgDrawUs=%lu maxDrawUs=%lu avgPresentUs=%lu maxPresentUs=%lu avgChangedPixels=%lu\n",
    windowMs,
    s_ambientWaterSurfacePerfStats.frameCount,
    avgDrawUs,
    s_ambientWaterSurfacePerfStats.maxDrawUs,
    avgPresentUs,
    s_ambientWaterSurfacePerfStats.maxPresentUs,
    avgChangedPixels
  );

  resetAmbientWaterSurfacePerfWindow(now);
}

void trackAmbientWaterSurfacePerfFrame(
  unsigned long now,
  unsigned long drawUs,
  unsigned long presentUs,
  uint16_t changedPixels
) {
  if (!shouldTrackAmbientWaterSurfacePerf()) {
    resetAmbientWaterSurfacePerfWindow(now);
    return;
  }

  if (s_ambientWaterSurfacePerfStats.windowStartedAt == 0) {
    resetAmbientWaterSurfacePerfWindow(now);
  }

  s_ambientWaterSurfacePerfStats.frameCount += 1UL;
  s_ambientWaterSurfacePerfStats.drawUsTotal += (uint64_t)drawUs;
  s_ambientWaterSurfacePerfStats.presentUsTotal += (uint64_t)presentUs;
  s_ambientWaterSurfacePerfStats.changedPixelsTotal += (uint32_t)changedPixels;
  if (drawUs > s_ambientWaterSurfacePerfStats.maxDrawUs) {
    s_ambientWaterSurfacePerfStats.maxDrawUs = drawUs;
  }
  if (presentUs > s_ambientWaterSurfacePerfStats.maxPresentUs) {
    s_ambientWaterSurfacePerfStats.maxPresentUs = presentUs;
  }

  maybeLogAmbientWaterSurfacePerf(now);
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

bool isRedirectFrameReady() {
  return s_redirectFrameActive && s_redirectFrameBuffer != nullptr;
}

bool modePrefersDoubleBuffer() {
  return false;
}

void writeRedirectPixel(int16_t x, int16_t y, uint16_t color) {
  if (!isRedirectFrameReady()) {
    return;
  }
  if (x < 0 || x >= DisplayManager::PANEL_RES_X || y < 0 || y >= DisplayManager::PANEL_RES_Y) {
    return;
  }
  s_redirectFrameBuffer[y * DisplayManager::PANEL_RES_X + x] = color;
}

void fillRedirectFrame(uint16_t color) {
  if (!isRedirectFrameReady()) {
    return;
  }
  for (int y = 0; y < DisplayManager::PANEL_RES_Y; y++) {
    for (int x = 0; x < DisplayManager::PANEL_RES_X; x++) {
      s_redirectFrameBuffer[y * DisplayManager::PANEL_RES_X + x] = color;
    }
  }
}
}

static void presentFrame() {
  if (DisplayManager::dma_display == nullptr) {
    return;
  }
  // 单缓冲模式下 drawPixel 已经即时生效，强制 flip 会造成整屏闪烁。
  if (!DisplayManager::doubleBufferEnabled) {
    return;
  }
  DisplayManager::dma_display->flipDMABuffer();
}

void CaptureMatrixPanel::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (isRedirectFrameReady()) {
    writeRedirectPixel(x, y, color);
    return;
  }
  MatrixPanel_I2S_DMA::drawPixel(x, y, color);
  DisplayManager::writeLiveFramePixel565(x, y, color);
}

void CaptureMatrixPanel::fillScreen(uint16_t color) {
  if (isRedirectFrameReady()) {
    fillRedirectFrame(color);
    return;
  }
  MatrixPanel_I2S_DMA::fillScreen(color);
  DisplayManager::clearLiveFrame(color);
}

void CaptureMatrixPanel::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
  if (isRedirectFrameReady()) {
    writeRedirectPixel(x, y, MatrixPanel_I2S_DMA::color565(r, g, b));
    return;
  }
  MatrixPanel_I2S_DMA::drawPixelRGB888(x, y, r, g, b);
  DisplayManager::writeLiveFramePixelRGB888(x, y, r, g, b);
}

void CaptureMatrixPanel::fillScreenRGB888(uint8_t r, uint8_t g, uint8_t b) {
  if (isRedirectFrameReady()) {
    fillRedirectFrame(MatrixPanel_I2S_DMA::color565(r, g, b));
    return;
  }
  MatrixPanel_I2S_DMA::fillScreenRGB888(r, g, b);
  DisplayManager::clearLiveFrame(MatrixPanel_I2S_DMA::color565(r, g, b));
}

void CaptureMatrixPanel::clearScreen() {
  if (isRedirectFrameReady()) {
    fillRedirectFrame(0);
    return;
  }
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

void BufferMatrixPanel::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (targetBuffer == nullptr || w <= 0 || h <= 0) {
    return;
  }

  int startX = x;
  int startY = y;
  int endX = x + w;
  int endY = y + h;

  if (startX < 0) {
    startX = 0;
  }
  if (startY < 0) {
    startY = 0;
  }
  if (endX > targetWidth) {
    endX = targetWidth;
  }
  if (endY > targetHeight) {
    endY = targetHeight;
  }

  if (startX >= endX || startY >= endY) {
    return;
  }

  for (int py = startY; py < endY; py++) {
    for (int px = startX; px < endX; px++) {
      targetBuffer[py * targetWidth + px] = color;
    }
  }
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

bool DisplayManager::beginRedirectedFrame(
  uint16_t* targetBuffer,
  uint16_t clearColor,
  bool clearBuffer
) {
  if (targetBuffer == nullptr || dma_display == nullptr) {
    return false;
  }
  s_redirectFrameBuffer = targetBuffer;
  s_redirectFrameActive = true;
  if (clearBuffer) {
    fillRedirectFrame(clearColor);
  }
  return true;
}

void DisplayManager::endRedirectedFrame(bool present) {
  if (!s_redirectFrameActive || s_redirectFrameBuffer == nullptr) {
    s_redirectFrameActive = false;
    s_redirectFrameBuffer = nullptr;
    return;
  }

  const uint16_t* targetBuffer = s_redirectFrameBuffer;
  s_redirectFrameActive = false;
  s_redirectFrameBuffer = nullptr;

  if (present) {
    presentOffscreenFrame(targetBuffer);
  }
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

  unsigned long presentStartedUs = micros();
  if (nativeEffectType == NATIVE_EFFECT_AMBIENT &&
      ambientEffectConfig.preset == AMBIENT_PRESET_WATER_SURFACE &&
      currentBusinessModeTag == ModeTags::LED_MATRIX_SHOWCASE) {
    for (int y = 0; y < PANEL_RES_Y; y++) {
      for (int x = 0; x < PANEL_RES_X; x++) {
        dma_display->drawPixel(x, y, targetBuffer[y * PANEL_RES_X + x]);
      }
    }
    memcpy(liveFrameBuffer, targetBuffer, sizeof(liveFrameBuffer));
    liveFrameValid = true;
    presentFrame();
    s_lastPresentChangedPixels = PANEL_RES_X * PANEL_RES_Y;
    s_lastPresentDurationUs = micros() - presentStartedUs;
    s_lastPresentFrameHash = 0;
    if (s_pendingAmbientWaterSurfaceDrawUs > 0UL) {
      trackAmbientWaterSurfacePerfFrame(
        millis(),
        s_pendingAmbientWaterSurfaceDrawUs,
        s_lastPresentDurationUs,
        s_lastPresentChangedPixels
      );
      s_pendingAmbientWaterSurfaceDrawUs = 0;
    }
    return;
  }

  bool changed = false;
  uint16_t changedPixels = 0;
  uint32_t frameHash = 2166136261UL;
  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      uint16_t nextColor = targetBuffer[y * PANEL_RES_X + x];
      frameHash ^= (uint32_t)nextColor;
      frameHash *= 16777619UL;
      if (liveFrameValid && liveFrameBuffer[y][x] == nextColor) {
        continue;
      }
      dma_display->drawPixel(x, y, nextColor);
      changed = true;
      if (changedPixels < 0xffffU) {
        changedPixels += 1U;
      }
    }
  }

  memcpy(liveFrameBuffer, targetBuffer, sizeof(liveFrameBuffer));
  liveFrameValid = true;
  if (changed) {
    presentFrame();
  }
  s_lastPresentChangedPixels = changedPixels;
  s_lastPresentDurationUs = micros() - presentStartedUs;
  s_lastPresentFrameHash = frameHash;
}

unsigned long DisplayManager::getLastPresentDurationUs() {
  return s_lastPresentDurationUs;
}

uint16_t DisplayManager::getLastPresentChangedPixels() {
  return s_lastPresentChangedPixels;
}

uint32_t DisplayManager::getLastPresentFrameHash() {
  return s_lastPresentFrameHash;
}

void DisplayManager::init() {
  Serial.println("1. 初始化LED灯板...");
  ensureRuntimeAccessMutex();
  setupMatrix();
}

void DisplayManager::lockRuntimeAccess() {
  ensureRuntimeAccessMutex();
  if (s_runtimeAccessMutex != nullptr) {
    xSemaphoreTakeRecursive(s_runtimeAccessMutex, portMAX_DELAY);
  }
}

void DisplayManager::unlockRuntimeAccess() {
  if (s_runtimeAccessMutex != nullptr) {
    xSemaphoreGiveRecursive(s_runtimeAccessMutex);
  }
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
  dma_display->clearScreen();
  currentBrightness = ConfigManager::deviceParamsConfig.displayBright;

  if (!showBootLogo) {
    return true;
  }

  Serial.println("显示开机Logo...");
  drawLogo(12, 7);  // 开机 logo 偏上
  // 开机 logo 额外显示品牌名
  const char* brandText = "Glowxel";
  int16_t textBoundsX = 0;
  int16_t textBoundsY = 0;
  uint16_t textWidth = 0;
  uint16_t textHeight = 0;
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(220, 220, 220));
  dma_display->getTextBounds(brandText, 0, 52, &textBoundsX, &textBoundsY, &textWidth, &textHeight);
  int textX = (PANEL_RES_X - (int)textWidth) / 2;
  if (textX < 0) {
    textX = 0;
  }
  dma_display->setCursor(textX, 52);
  dma_display->print(brandText);
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
  return fallbackOk;
}

bool DisplayManager::disableDoubleBuffer() {
  if (!doubleBufferEnabled) {
    return true;
  }

  Serial.println("切换为单缓冲显示...");
  bool ok = rebuildMatrix(false, false);
  if (ok) {
    clearLiveFrame();
    backgroundValid = false;
    animationBufferValid = false;
    return true;
  }

  Serial.println("单缓冲恢复失败，尝试保留双缓冲");
  bool fallbackOk = rebuildMatrix(true, false);
  if (fallbackOk) {
    clearLiveFrame();
    backgroundValid = false;
    animationBufferValid = false;
  }
  return fallbackOk;
}

bool DisplayManager::syncBufferStrategyForCurrentMode() {
  if (dma_display == nullptr) {
    return false;
  }

  bool shouldUseDoubleBuffer = modePrefersDoubleBuffer();
  if (shouldUseDoubleBuffer == doubleBufferEnabled) {
    return true;
  }

  Serial.printf("[Display] 当前模式=%d，目标缓冲=%s\n",
                currentMode,
                shouldUseDoubleBuffer ? "double" : "single");
  if (shouldUseDoubleBuffer) {
    return enableDoubleBuffer();
  }
  return disableDoubleBuffer();
}

void DisplayManager::drawLogo(int x, int y) {
  // Glowxel PixelBoard 品牌色：orange → red → yellow → blue
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
static ClockConfig& resolveActiveClockConfig();
static void writeBackgroundPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
static void restoreBackgroundRect(int x, int y, int w, int h);
static void clockTextBounds(const char* text, int x, int y, uint8_t fontId, int size, int& outX, int& outW, int& outH);
static void drawClockOverlayAnimationMode(const ClockConfig& c, const struct tm& timeinfo);
static void drawStaticClockOverlayDirty(const ClockConfig& c, const struct tm& timeinfo);
static void cacheLogoBackground(int x, int y);
static void formatTimeText(const ClockConfig& c, const struct tm& timeinfo, char* buffer, size_t bufferSize);
static void formatDateText(const struct tm& timeinfo, char* buffer, size_t bufferSize);
static void formatWeekText(const struct tm& timeinfo, char* buffer, size_t bufferSize);
static bool isAmbientWaterWorldPreset(uint8_t preset);
static void drawAmbientWaterWorldTimeOverlayIfNeeded();
static bool rebuildStaticClockBackgroundFrame(
  const PixelData* imagePixels,
  int imagePixelCount,
  bool hasCustomImage
);
static bool presentStaticClockFrame(const ClockConfig& c, const struct tm& timeinfo);

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

static bool isAmbientWaterWorldPreset(uint8_t preset) {
  return preset == AMBIENT_PRESET_WATER_SURFACE ||
         preset == AMBIENT_PRESET_WATER_CURRENT ||
         preset == AMBIENT_PRESET_WATER_CAUSTICS;
}

struct AmbientWaterOverlayRect {
  int x;
  int y;
  int w;
  int h;
};

static unsigned long s_ambientWaterMotionLastTickAt = 0;
static unsigned long s_ambientWaterMotionElapsed = 0;
static uint8_t s_ambientWaterMotionPreset = 0xFF;
static bool s_ambientWaterDirty = false;
static AmbientWaterOverlayRect s_ambientWaterOverlayRects[3];
static int s_ambientWaterOverlayRectCount = 0;
static bool s_ambientWaterOverlayFrameActive = false;

static unsigned long ambientWaterMotionTickMs(uint8_t preset) {
  if (preset == AMBIENT_PRESET_WATER_SURFACE) {
    return 16UL;
  }
  if (preset == AMBIENT_PRESET_WATER_CURRENT) {
    return 16UL;
  }
  if (preset == AMBIENT_PRESET_WATER_CAUSTICS) {
    return 16UL;
  }
  return 16UL;
}

static void resetAmbientWaterOverlayState() {
  s_ambientWaterOverlayRectCount = 0;
  s_ambientWaterOverlayFrameActive = false;
  s_forceStaticClockOverlayRefresh = true;
}

static void resetAmbientWaterMotionTimeline() {
  s_ambientWaterMotionLastTickAt = 0;
  s_ambientWaterMotionElapsed = 0;
  s_ambientWaterMotionPreset = 0xFF;
  s_ambientWaterDirty = false;
  resetAmbientWaterOverlayState();
}

static void advanceAmbientWaterMotionTimeline(unsigned long now, uint8_t preset) {
  if (s_ambientWaterMotionLastTickAt == 0 || s_ambientWaterMotionPreset != preset) {
    s_ambientWaterMotionLastTickAt = now;
    s_ambientWaterMotionElapsed = 0;
    s_ambientWaterMotionPreset = preset;
    s_ambientWaterDirty = true;
    return;
  }

  unsigned long tickMs = ambientWaterMotionTickMs(preset);
  unsigned long delta = now - s_ambientWaterMotionLastTickAt;
  if (delta < tickMs) {
    return;
  }

  s_ambientWaterMotionLastTickAt = now;
  s_ambientWaterMotionElapsed += delta;
  s_ambientWaterDirty = true;
}

static void pushAmbientWaterOverlayRect(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0 || s_ambientWaterOverlayRectCount >= 3) {
    return;
  }
  s_ambientWaterOverlayRects[s_ambientWaterOverlayRectCount++] = {x, y, w, h};
}

static bool ambientWaterOverlayRectContainsPixel(int x, int y) {
  for (int index = 0; index < s_ambientWaterOverlayRectCount; index++) {
    const AmbientWaterOverlayRect& rect = s_ambientWaterOverlayRects[index];
    if (x >= rect.x && x < rect.x + rect.w &&
        y >= rect.y && y < rect.y + rect.h) {
      return true;
    }
  }
  return false;
}

static void prepareAmbientWaterOverlayFrame(const ClockConfig& clockConfig, const struct tm& timeinfo) {
  s_ambientWaterOverlayRectCount = 0;
  s_ambientWaterOverlayFrameActive = false;

  if (!clockConfig.time.show && !clockConfig.date.show && !clockConfig.week.show) {
    return;
  }

  if (clockConfig.time.show) {
    char timeText[9];
    int x = 0;
    int w = 0;
    int h = 0;
    formatTimeText(clockConfig, timeinfo, timeText, sizeof(timeText));
    clockTextBounds(timeText, clockConfig.time.x, clockConfig.time.y, clockConfig.font, clockConfig.time.fontSize, x, w, h);
    pushAmbientWaterOverlayRect(x, clockConfig.time.y, w, h);
  }

  if (clockConfig.date.show) {
    char dateText[6];
    int x = 0;
    int w = 0;
    int h = 0;
    formatDateText(timeinfo, dateText, sizeof(dateText));
    clockTextBounds(dateText, clockConfig.date.x, clockConfig.date.y, clockConfig.font, clockConfig.date.fontSize, x, w, h);
    pushAmbientWaterOverlayRect(x, clockConfig.date.y, w, h);
  }

  if (clockConfig.week.show) {
    char weekText[4];
    int x = 0;
    int w = 0;
    int h = 0;
    formatWeekText(timeinfo, weekText, sizeof(weekText));
    clockTextBounds(weekText, clockConfig.week.x, clockConfig.week.y, clockConfig.font, 1, x, w, h);
    pushAmbientWaterOverlayRect(x, clockConfig.week.y, w, h);
  }

  s_ambientWaterOverlayFrameActive = s_ambientWaterOverlayRectCount > 0;
}

static void drawAmbientWaterPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
}

static void drawAmbientWaterWorldTimeOverlayIfNeeded() {
  if (!isAmbientWaterWorldPreset(DisplayManager::ambientEffectConfig.preset)) {
    return;
  }

  const ClockConfig& clockConfig = ConfigManager::ledMatrixShowcaseClockConfig;
  if (!clockConfig.time.show && !clockConfig.date.show && !clockConfig.week.show) {
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  drawClockOverlayAnimationMode(clockConfig, timeinfo);
}

static bool rebuildStaticClockBackgroundFrame(
  const PixelData* imagePixels,
  int imagePixelCount,
  bool hasCustomImage
) {
  if (!DisplayManager::beginRedirectedFrame(&DisplayManager::backgroundBuffer[0][0], 0, true)) {
    return false;
  }

  if (hasCustomImage) {
    DisplayManager::drawPixels(imagePixels, imagePixelCount, false);
  } else {
    DisplayManager::drawLogo(12, 18);
    cacheLogoBackground(12, 18);
  }

  DisplayManager::endRedirectedFrame(false);
  DisplayManager::backgroundValid = true;
  return true;
}

static bool presentStaticClockFrame(const ClockConfig& c, const struct tm& timeinfo) {
  memcpy(DisplayManager::animationBuffer, DisplayManager::backgroundBuffer, sizeof(DisplayManager::backgroundBuffer));
  if (!DisplayManager::beginRedirectedFrame(&DisplayManager::animationBuffer[0][0], 0, false)) {
    return false;
  }

  drawClockOverlayAnimationMode(c, timeinfo);
  DisplayManager::endRedirectedFrame(true);
  return true;
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
  uint16_t* frameBuffer = &animationBuffer[0][0];
  if (!beginRedirectedFrame(frameBuffer, 0, false)) {
    return;
  }

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
  endRedirectedFrame(true);
  memcpy(animationBuffer, backgroundBuffer, sizeof(animationBuffer));
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
  uint16_t* frameBuffer = &animationBuffer[0][0];
  if (!beginRedirectedFrame(frameBuffer, 0)) {
    return;
  }

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
  endRedirectedFrame(true);
  memcpy(animationBuffer, backgroundBuffer, sizeof(animationBuffer));
}

void DisplayManager::renderAnimationTransitionBuffers(
    const uint16_t* fromBuffer,
    const uint16_t* toBuffer,
    uint8_t mix) {
  if (fromBuffer == nullptr || toBuffer == nullptr) {
    return;
  }

  uint16_t* frameBuffer = &animationBuffer[0][0];
  if (!beginRedirectedFrame(frameBuffer, 0)) {
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
  endRedirectedFrame(true);
  memcpy(animationBuffer, backgroundBuffer, sizeof(animationBuffer));
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
  ClockConfig& cfg = resolveActiveClockConfig();

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
    if (currentMode == MODE_CLOCK) {
      if (!rebuildStaticClockBackgroundFrame(imagePixels, imagePixelCount, hasCustomImage)) {
        return;
      }
      s_forceStaticClockOverlayRefresh = true;
    } else {
      dma_display->clearScreen();
      memset(backgroundBuffer, 0, sizeof(backgroundBuffer));
      backgroundValid = true;
    }
  }

  if (currentMode == MODE_CLOCK) {
    presentStaticClockFrame(cfg, timeinfo);
    return;
  }

  if (currentMode == MODE_ANIMATION) {
    if (hasCustomImage) {
      drawPixels(imagePixels, imagePixelCount, false);
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
  return resolveActiveClockConfig().time.y;
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

  if (s_forceStaticClockOverlayRefresh || DisplayManager::currentMode != s_prevMode) {
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
    s_forceStaticClockOverlayRefresh = false;
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

  auto& c = resolveActiveClockConfig();

  // 动态背景和俄罗斯方块屏保都会按帧合成完整画面，
  // 这里直接补画前景层，避免静态脏区逻辑把时钟漏掉。
  if (currentMode == MODE_ANIMATION ||
      currentBusinessModeTag == ModeTags::TETRIS) {
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
  resetAmbientWaterMotionTimeline();
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
  resetAmbientWaterMotionTimeline();
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

static float mixf(float start, float end, float alpha) {
  float safe = clampUnit(alpha);
  return start + (end - start) * safe;
}

static float pow4fExact(float value) {
  float squared = value * value;
  return squared * squared;
}

static float pow5fExact(float value) {
  return pow4fExact(value) * value;
}

static float pow7fExact(float value) {
  float squared = value * value;
  float fourth = squared * squared;
  return fourth * squared * value;
}

static constexpr float kAmbientTrigTau = 6.28318530718f;
static constexpr float kAmbientTrigHalfPi = 1.57079632679f;
static constexpr int kAmbientWaterTrigLutSize = 128;
static constexpr int kAmbientWaterPowLutSize = 128;
static float s_ambientWaterTrigLut[kAmbientWaterTrigLutSize + 1];
static bool s_ambientWaterTrigLutReady = false;

static void ensureAmbientWaterTrigLut() {
  if (s_ambientWaterTrigLutReady) {
    return;
  }

  for (int index = 0; index <= kAmbientWaterTrigLutSize; index++) {
    float angle = ((float)index / (float)kAmbientWaterTrigLutSize) * kAmbientTrigTau;
    s_ambientWaterTrigLut[index] = sinf(angle);
  }
  s_ambientWaterTrigLutReady = true;
}

static float wrapAmbientTrigAngle(float angle) {
  float wrapped = angle - floorf(angle / kAmbientTrigTau) * kAmbientTrigTau;
  if (wrapped < 0.0f) {
    wrapped += kAmbientTrigTau;
  }
  return wrapped;
}

static float ambientWaterSin(float angle) {
  ensureAmbientWaterTrigLut();
  float wrapped = wrapAmbientTrigAngle(angle);
  float position = wrapped * ((float)kAmbientWaterTrigLutSize / kAmbientTrigTau);
  int baseIndex = (int)position;
  float alpha = position - (float)baseIndex;
  return mixf(
    s_ambientWaterTrigLut[baseIndex],
    s_ambientWaterTrigLut[baseIndex + 1],
    alpha
  );
}

static float ambientWaterCos(float angle) {
  return ambientWaterSin(angle + kAmbientTrigHalfPi);
}

static void buildAmbientWaterUnitPowLut(float* lut, float exponent) {
  for (int index = 0; index <= kAmbientWaterPowLutSize; index++) {
    float x = (float)index / (float)kAmbientWaterPowLutSize;
    lut[index] = powf(x, exponent);
  }
}

static inline float sampleAmbientWaterUnitPowLut(
  const float* lut,
  float value
) {
  float clamped = clampUnit(value) * (float)kAmbientWaterPowLutSize;
  int index0 = (int)clamped;
  int index1 = index0 < kAmbientWaterPowLutSize ? (index0 + 1) : index0;
  return mixf(lut[index0], lut[index1], clamped - (float)index0);
}

static unsigned long ambientPresetMinRenderIntervalMs(uint8_t preset) {
  if (preset == AMBIENT_PRESET_WATER_SURFACE) {
    return 16UL;
  }
  if (preset == AMBIENT_PRESET_WATER_CURRENT) {
    return 16UL;
  }
  if (preset == AMBIENT_PRESET_WATER_CAUSTICS) {
    return 42UL;
  }
  return 33UL;
}

static unsigned long ambientPresetQuantizedElapsedMs(
  uint8_t preset,
  unsigned long elapsed
) {
  if (preset == AMBIENT_PRESET_WATER_CAUSTICS) {
    const unsigned long stepMs = 42UL;
    return elapsed - (elapsed % stepMs);
  }
  return elapsed;
}

static void fillAmbientWaterWaveSequence(
  float* values,
  float startPhase,
  float deltaPhase,
  bool cosine
) {
  float sinValue = ambientWaterSin(startPhase);
  float cosValue = ambientWaterCos(startPhase);
  float deltaSin = ambientWaterSin(deltaPhase);
  float deltaCos = ambientWaterCos(deltaPhase);

  for (int index = 0; index < 64; index++) {
    values[index] = cosine ? cosValue : sinValue;
    if (index == 63) {
      break;
    }

    if ((index & 15) == 15) {
      float nextPhase = startPhase + deltaPhase * (float)(index + 1);
      sinValue = ambientWaterSin(nextPhase);
      cosValue = ambientWaterCos(nextPhase);
      continue;
    }

    float nextSin = sinValue * deltaCos + cosValue * deltaSin;
    float nextCos = cosValue * deltaCos - sinValue * deltaSin;
    sinValue = nextSin;
    cosValue = nextCos;
  }
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

struct SpaceGeneratorNebulaLayer {
  const NativeRgb* palette;
  uint8_t paletteSize;
  float threshold;
  float density;
  float alpha;
  bool ditherEnabled;
  bool modulationEnabled;
  NativeRgb modulationColor;
  float modulationIntensity;
  float modulationAlphaIntensity;
  float modulationDensity;
  uint8_t modulationSteps;
  bool oscillate;
  float oscillationIntensity;
  float oscillationRate;
  float oscillationOffset;
  float speed;
  float seed;
  float modulationSeed;
};

struct SpaceGeneratorStarLayer {
  uint16_t sourceCount;
  float speed;
  float flickerRate;
  float flickerDepth;
  float seedOffset;
  NativeRgb tint;
};

static const uint8_t kSpaceGeneratorBayer4x4[16] = {
  0, 8, 2, 10,
  12, 4, 14, 6,
  3, 11, 1, 9,
  15, 7, 13, 5
};

static const NativeRgb kCosmicKaleBackgroundPalette[] = {
  {4, 12, 6},
  {17, 35, 24},
  {30, 58, 41},
  {48, 93, 66},
  {77, 128, 97},
  {137, 162, 87},
  {190, 220, 127},
  {238, 255, 204}
};

static const NativeRgb kCosmicKaleForegroundPalette[] = {
  {30, 58, 41},
  {30, 58, 41},
  {30, 58, 41},
  {30, 58, 41},
  {77, 128, 97},
  {77, 128, 97},
  {77, 128, 97},
  {190, 220, 127},
  {77, 128, 97},
  {77, 128, 97},
  {77, 128, 97},
  {30, 58, 41},
  {30, 58, 41},
  {30, 58, 41},
  {30, 58, 41}
};

static const NativeRgb kVoidFireDensePalette[] = {
  {19, 2, 8},
  {19, 2, 8},
  {19, 2, 8},
  {19, 2, 8},
  {19, 2, 8},
  {31, 5, 16},
  {31, 5, 16},
  {31, 5, 16},
  {49, 5, 30},
  {49, 5, 30},
  {70, 14, 43},
  {124, 24, 60},
  {213, 60, 106},
  {255, 130, 116}
};

static const NativeRgb kVoidFireCoarsePalette[] = {
  {19, 2, 8},
  {31, 5, 16},
  {49, 5, 30},
  {70, 14, 43},
  {124, 24, 60},
  {213, 60, 106},
  {255, 130, 116}
};

static const SpaceGeneratorNebulaLayer kCosmicKaleBackgroundLayer = {
  kCosmicKaleBackgroundPalette,
  static_cast<uint8_t>(sizeof(kCosmicKaleBackgroundPalette) / sizeof(kCosmicKaleBackgroundPalette[0])),
  0.36f,
  0.01f,
  0.63f,
  false,
  true,
  {186, 165, 26},
  0.5f,
  0.38f,
  0.0027598312f,
  13,
  false,
  0.24f,
  0.2f,
  0.2f,
  11.5f,
  1.31f,
  3.67f
};

static const SpaceGeneratorStarLayer kCosmicKaleStarsNear = {
  256,
  6.0f,
  1.12f,
  0.16f,
  4.11f,
  {245, 255, 213}
};

static const SpaceGeneratorStarLayer kCosmicKaleStarsFar = {
  256,
  6.0f,
  1.12f,
  0.16f,
  7.39f,
  {229, 255, 216}
};

static const SpaceGeneratorNebulaLayer kCosmicKaleForegroundLayer = {
  kCosmicKaleForegroundPalette,
  static_cast<uint8_t>(sizeof(kCosmicKaleForegroundPalette) / sizeof(kCosmicKaleForegroundPalette[0])),
  0.0f,
  0.01f,
  0.69f,
  false,
  true,
  {66, 255, 28},
  0.73f,
  0.6f,
  0.0050061825f,
  12,
  true,
  0.08545377f,
  1.0859375f,
  0.5f,
  29.2f,
  9.27f,
  12.81f
};

static const SpaceGeneratorStarLayer kVoidFireStarsFar = {
  360,
  6.0f,
  1.12f,
  0.16f,
  2.17f,
  {255, 242, 210}
};

static const SpaceGeneratorNebulaLayer kVoidFireDenseLayer = {
  kVoidFireDensePalette,
  static_cast<uint8_t>(sizeof(kVoidFireDensePalette) / sizeof(kVoidFireDensePalette[0])),
  0.25f,
  0.0053201809f,
  1.0f,
  true,
  true,
  {173, 24, 24},
  0.56f,
  0.22f,
  0.0052951898f,
  10,
  true,
  0.02611940f,
  0.8f,
  0.5f,
  6.0f,
  14.37f,
  15.91f
};

static const SpaceGeneratorNebulaLayer kVoidFireGlowLayer = {
  kVoidFireDensePalette,
  static_cast<uint8_t>(sizeof(kVoidFireDensePalette) / sizeof(kVoidFireDensePalette[0])),
  0.42f,
  0.0054803207f,
  0.89f,
  false,
  true,
  {255, 187, 0},
  1.0f,
  0.39f,
  0.0054803379f,
  16,
  true,
  0.01865672f,
  0.6268657f,
  0.235f,
  6.0f,
  17.23f,
  18.49f
};

static const SpaceGeneratorStarLayer kVoidFireStarsNear = {
  360,
  6.0f,
  1.12f,
  0.16f,
  6.83f,
  {255, 224, 191}
};

static const SpaceGeneratorNebulaLayer kVoidFireDriftLayer = {
  kVoidFireCoarsePalette,
  static_cast<uint8_t>(sizeof(kVoidFireCoarsePalette) / sizeof(kVoidFireCoarsePalette[0])),
  0.0f,
  0.11278186f,
  0.56f,
  false,
  true,
  {255, 197, 36},
  0.26f,
  1.0f,
  0.0024861244f,
  16,
  false,
  0.04f,
  0.8f,
  0.5f,
  19.6f,
  20.17f,
  22.31f
};

static const SpaceGeneratorNebulaLayer kVoidFireEmberLayer = {
  kVoidFireCoarsePalette,
  static_cast<uint8_t>(sizeof(kVoidFireCoarsePalette) / sizeof(kVoidFireCoarsePalette[0])),
  0.03f,
  0.03244237f,
  0.78f,
  true,
  true,
  {252, 0, 4},
  0.44f,
  1.0f,
  0.0026630207f,
  10,
  true,
  0.04f,
  0.8f,
  0.5f,
  29.6f,
  24.11f,
  26.73f
};

static const NativeRgb kDeepSpaceNebulaBodyPalette[] = {
  {5, 9, 18},
  {8, 16, 32},
  {14, 25, 51},
  {21, 38, 74},
  {33, 59, 104},
  {48, 89, 142},
  {68, 120, 173},
  {105, 162, 216}
};

static const NativeRgb kDeepSpaceNebulaCorePalette[] = {
  {12, 23, 48},
  {22, 48, 86},
  {33, 74, 122},
  {47, 107, 163},
  {79, 151, 202},
  {142, 200, 239},
  {205, 239, 255}
};

static const SpaceGeneratorStarLayer kDeepSpaceNebulaStarLayer = {
  54,
  2.8f,
  0.74f,
  0.08f,
  1.37f,
  {183, 202, 238}
};

static NativeRgb rgb565ToNativeRgb(uint16_t color) {
  NativeRgb rgb = {
    (uint8_t)((((color >> 11) & 0x1F) * 255 + 15) / 31),
    (uint8_t)((((color >> 5) & 0x3F) * 255 + 31) / 63),
    (uint8_t)(((color & 0x1F) * 255 + 15) / 31)
  };
  return rgb;
}

static void blendSpaceGeneratorPixel(int x, int y, const NativeRgb& src, float alpha) {
  if (DisplayManager::dma_display == nullptr) {
    return;
  }
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }

  uint16_t currentColor565 = DisplayManager::liveFrameBuffer[y][x];
  if (isRedirectFrameReady()) {
    currentColor565 = s_redirectFrameBuffer[y * DisplayManager::PANEL_RES_X + x];
  }
  NativeRgb current = rgb565ToNativeRgb(currentColor565);
  blendIntoPixel(current.r, current.g, current.b, src, alpha);
  DisplayManager::dma_display->drawPixelRGB888(x, y, current.r, current.g, current.b);
}

static float spaceGeneratorHash(int x, int y, float seed) {
  return fractf32(
    sinf((float)(x + 1) * 12.9898f + (float)(y + 1) * 78.233f + seed * 37.719f) * 43758.5453f
  );
}

static float spaceGeneratorBayerThreshold(int x, int y) {
  return (float)kSpaceGeneratorBayer4x4[(x & 3) + ((y & 3) << 2)] / 16.0f;
}

static float sampleSpaceGeneratorLayerField(
  int x,
  int y,
  unsigned long elapsed,
  float speedUnit,
  float density,
  float seed,
  float speed
) {
  float nx = (float)x / 63.0f;
  float scroll = (float)elapsed * 0.000011f * speed * (0.45f + speedUnit * 0.85f);
  float ny = fractf32((float)y / 63.0f + scroll);
  float densityScale = 2.6f + density * 120.0f;
  float warpX =
    nx +
    sinf((ny + seed * 0.07f) * (6.4f + densityScale * 0.32f) + scroll * 3.1f) * 0.12f +
    cosf((nx + seed * 0.11f) * (3.8f + densityScale * 0.18f) - scroll * 2.4f) * 0.05f;
  float warpY =
    ny +
    cosf((nx + seed * 0.13f) * (5.8f + densityScale * 0.28f) - scroll * 2.2f) * 0.11f +
    sinf((ny + seed * 0.19f) * (4.2f + densityScale * 0.14f) + scroll * 1.6f) * 0.04f;
  float fieldA =
    0.5f +
    0.5f *
      sinf(
        warpX * (8.4f + densityScale) +
        seed * 0.91f +
        sinf(warpY * (5.8f + densityScale * 0.46f) - scroll * 4.1f)
      );
  float fieldB =
    0.5f +
    0.5f *
      cosf(
        warpY * (9.1f + densityScale * 0.82f) -
        seed * 0.63f +
        sinf(warpX * (4.9f + densityScale * 0.28f) + scroll * 3.4f)
      );
  float fieldC =
    0.5f +
    0.5f *
      sinf((warpX + warpY) * (6.7f + densityScale * 0.58f) + seed * 1.37f - scroll * 2.1f);
  float grain = spaceGeneratorHash(x, y, seed * 1.71f);
  return clampUnit(fieldA * 0.42f + fieldB * 0.34f + fieldC * 0.18f + grain * 0.06f);
}

static bool resolveSpaceGeneratorPaletteColor(
  const SpaceGeneratorNebulaLayer& layer,
  float value,
  int x,
  int y,
  NativeRgb& outColor
) {
  if (value <= 0.0f || layer.paletteSize == 0) {
    return false;
  }

  float scaled = clampUnit(value) * (float)(layer.paletteSize - 1);
  int lowIndex = (int)floorf(scaled);
  int highIndex = min((int)layer.paletteSize - 1, lowIndex + 1);
  float paletteMix = scaled - (float)lowIndex;
  int index =
    layer.ditherEnabled && paletteMix > spaceGeneratorBayerThreshold(x, y)
      ? highIndex
      : lowIndex;
  outColor = layer.palette[index];
  return true;
}

static void blendSpaceGeneratorNebulaLayer(
  unsigned long elapsed,
  float speedUnit,
  float intensityUnit,
  const SpaceGeneratorNebulaLayer& layer
) {
  float thresholdShift = layer.oscillate
    ? sinf(
        (float)elapsed * 0.001f * layer.oscillationRate +
        layer.oscillationOffset * 6.2831853f
      ) * layer.oscillationIntensity
    : 0.0f;
  float threshold = max(0.0f, min(0.92f, layer.threshold + thresholdShift));

  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      float field = sampleSpaceGeneratorLayerField(
        x,
        y,
        elapsed,
        speedUnit,
        layer.density,
        layer.seed,
        layer.speed
      );
      float value = (field - threshold) / max(0.001f, 1.0f - threshold);
      NativeRgb paletteColor;
      if (!resolveSpaceGeneratorPaletteColor(layer, value, x, y, paletteColor)) {
        continue;
      }

      NativeRgb finalColor = paletteColor;
      float alpha = layer.alpha * (0.58f + intensityUnit * 0.48f);
      if (layer.modulationEnabled) {
        float modulationField = sampleSpaceGeneratorLayerField(
          x,
          y,
          elapsed,
          speedUnit,
          layer.modulationDensity,
          layer.modulationSeed,
          layer.speed * 0.62f + 2.4f
        );
        float stepCount = max(1, (int)layer.modulationSteps);
        float quantizedLum =
          floorf(min(modulationField, 0.999f) * stepCount) / stepCount;
        NativeRgb modulatedColor = scaleRgbColor(layer.modulationColor, quantizedLum);
        finalColor = blendRgbColor(finalColor, modulatedColor, layer.modulationIntensity);
        alpha *= 1.0f - (1.0f - quantizedLum) * layer.modulationAlphaIntensity;
      }

      blendSpaceGeneratorPixel(x, y, finalColor, clampUnit(alpha));
    }
  }
}

static void drawSpaceGeneratorStarShape(
  int centerX,
  int centerY,
  uint8_t sizeTier,
  const NativeRgb& color,
  float alpha
) {
  if (sizeTier == 0) {
    blendSpaceGeneratorPixel(centerX, centerY, color, alpha);
    return;
  }

  NativeRgb core = addHighlightColor(color, 0.16f);
  blendSpaceGeneratorPixel(centerX, centerY, core, alpha);
  blendSpaceGeneratorPixel(centerX - 1, centerY, color, alpha * 0.56f);
  blendSpaceGeneratorPixel(centerX + 1, centerY, color, alpha * 0.56f);
  blendSpaceGeneratorPixel(centerX, centerY - 1, color, alpha * 0.56f);
  blendSpaceGeneratorPixel(centerX, centerY + 1, color, alpha * 0.56f);

  if (sizeTier == 2) {
    blendSpaceGeneratorPixel(centerX - 2, centerY, color, alpha * 0.24f);
    blendSpaceGeneratorPixel(centerX + 2, centerY, color, alpha * 0.24f);
    blendSpaceGeneratorPixel(centerX, centerY - 2, color, alpha * 0.24f);
    blendSpaceGeneratorPixel(centerX, centerY + 2, color, alpha * 0.24f);
  }
}

static void paintSpaceGeneratorStarLayer(
  unsigned long elapsed,
  float speedUnit,
  float intensityUnit,
  const SpaceGeneratorStarLayer& layer
) {
  float densityRatio = (64.0f * 64.0f) / (360.0f * 240.0f);
  int starCount = max(8, (int)roundf((float)layer.sourceCount * densityRatio));
  float travel = (float)elapsed * 0.000011f * layer.speed * (0.45f + speedUnit * 0.85f);

  for (int index = 0; index < starCount; index++) {
    float seedOffset = layer.seedOffset + (float)index * 0.73f;
    float baseX = spaceGeneratorHash(index, 2, seedOffset);
    float baseY = spaceGeneratorHash(index, 7, seedOffset);
    float sizeSeed = spaceGeneratorHash(index, 13, seedOffset);
    float twinkleSeed = spaceGeneratorHash(index, 29, seedOffset);
    float swaySeed = spaceGeneratorHash(index, 37, seedOffset);
    uint8_t sizeTier = sizeSeed > 0.86f ? 2 : (sizeSeed > 0.62f ? 1 : 0);
    int x = (int)roundf(
      baseX * 63.0f +
      sinf((float)elapsed * 0.00062f * (0.4f + swaySeed * 0.7f) + seedOffset) *
        (sizeTier == 2 ? 1.2f : 0.7f)
    );
    int y = (int)roundf(fractf32(baseY + travel + sizeSeed * 0.12f) * 63.0f);
    float twinkle =
      0.66f +
      0.34f *
        sinf(
          (float)elapsed * 0.0012f * layer.flickerRate * (0.78f + twinkleSeed * 0.44f) +
          seedOffset * 4.1f
        );
    float brightness = clampUnit((0.55f + intensityUnit * 0.35f) * twinkle);
    NativeRgb color = sizeTier == 2 ? addHighlightColor(layer.tint, 0.22f) : layer.tint;
    drawSpaceGeneratorStarShape(
      x,
      y,
      sizeTier,
      color,
      brightness * (1.0f - layer.flickerDepth * 0.45f)
    );

    if (sizeTier >= 1) {
      NativeRgb glow = scaleRgbColor(color, 0.32f + intensityUnit * 0.12f);
      blendSpaceGeneratorPixel(x - 1, y - 1, glow, brightness * 0.16f);
      blendSpaceGeneratorPixel(x + 1, y - 1, glow, brightness * 0.16f);
      blendSpaceGeneratorPixel(x - 1, y + 1, glow, brightness * 0.16f);
      blendSpaceGeneratorPixel(x + 1, y + 1, glow, brightness * 0.16f);
    }
  }
}

static float ellipseMaskf(float nx, float ny, float cx, float cy, float rx, float ry) {
  float dx = (nx - cx) / max(rx, 0.001f);
  float dy = (ny - cy) / max(ry, 0.001f);
  return clampUnit(1.0f - dx * dx - dy * dy);
}

static NativeRgb resolveDeepSpacePaletteColor(const NativeRgb* palette, uint8_t paletteSize, float value) {
  if (palette == nullptr || paletteSize == 0) {
    return makeRgb(0, 0, 0);
  }
  float scaled = clampUnit(value) * (float)(paletteSize - 1);
  int lowIndex = (int)floorf(scaled);
  int highIndex = min((int)paletteSize - 1, lowIndex + 1);
  return blendRgbColor(palette[lowIndex], palette[highIndex], scaled - (float)lowIndex);
}

static float sampleDeepSpaceNoiseField(float nx, float ny, float timeBase) {
  float warpX =
    nx +
    sinf(ny * 7.1f + timeBase * 0.46f + cosf(nx * 3.8f - timeBase * 0.18f)) * 0.085f +
    cosf(nx * 5.2f - timeBase * 0.22f) * 0.032f;
  float warpY =
    ny +
    cosf(nx * 6.4f - timeBase * 0.28f + sinf(ny * 4.2f + timeBase * 0.2f)) * 0.082f +
    sinf(ny * 5.7f + timeBase * 0.16f) * 0.028f;
  float bandA = 0.5f + 0.5f * sinf((warpX * 6.8f + warpY * 3.2f) + timeBase * 0.92f);
  float bandB = 0.5f + 0.5f * cosf((warpY * 7.4f - warpX * 4.3f) - timeBase * 0.68f);
  float detail = 0.5f + 0.5f * sinf((warpX + warpY) * 12.4f - timeBase * 1.34f);
  return clampUnit(bandA * 0.48f + bandB * 0.34f + detail * 0.18f);
}

static void renderAmbientDeepSpaceNebulaBase(unsigned long elapsed, float speedUnit, float intensityUnit) {
  NativeRgb backgroundA = makeRgb(2, 4, 10);
  NativeRgb backgroundB = makeRgb(5, 10, 21);
  NativeRgb clusterColor = makeRgb(217, 244, 255);
  float timeBase = (float)elapsed * (0.00038f + speedUnit * 0.00072f);

  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      float nx = (float)x / 63.0f;
      float ny = (float)y / 63.0f;
      float centerX = ((float)x - 31.5f) / 31.5f;
      float centerY = ((float)y - 31.5f) / 31.5f;
      float vignette = clampUnit(1.0f - sqrtf(centerX * centerX + centerY * centerY) * 0.58f);

      float ridgeCenter = 0.8f - nx * 0.47f + sinf(timeBase * 0.34f + nx * 4.6f) * 0.032f;
      float ridge = clampUnit(1.0f - fabsf(ny - ridgeCenter) / 0.17f);
      float cloudA = ellipseMaskf(nx, ny, 0.3f, 0.67f, 0.62f, 0.34f);
      float cloudB = ellipseMaskf(nx, ny, 0.66f, 0.34f, 0.34f, 0.26f);
      float coreMask = ellipseMaskf(nx, ny, 0.68f, 0.3f, 0.16f, 0.12f);
      float structure = clampUnit(cloudA * 0.84f + cloudB * 0.9f + ridge * 0.44f);

      float largeNoise = sampleDeepSpaceNoiseField(nx, ny, timeBase);
      float fineNoise = sampleDeepSpaceNoiseField(nx + 0.19f, ny - 0.13f, timeBase * 1.62f + 2.4f);
      float bodyField = clampUnit(
        (largeNoise * 0.7f + fineNoise * 0.3f) * structure - 0.21f + cloudA * 0.08f
      );
      float coreField = clampUnit(
        (largeNoise * 0.38f + fineNoise * 0.62f) * (coreMask * 1.34f + cloudB * 0.12f) -
        0.48f +
        coreMask * 0.42f
      );

      float backgroundMix = clampUnit(0.18f + ridge * 0.18f + cloudA * 0.07f);
      NativeRgb baseBackground = blendRgbColor(backgroundA, backgroundB, backgroundMix);
      NativeRgb background = scaleRgbColor(baseBackground, 0.68f + vignette * 0.32f);
      uint8_t r = background.r;
      uint8_t g = background.g;
      uint8_t b = background.b;

      if (bodyField > 0.01f) {
        NativeRgb bodyColor = resolveDeepSpacePaletteColor(
          kDeepSpaceNebulaBodyPalette,
          static_cast<uint8_t>(sizeof(kDeepSpaceNebulaBodyPalette) / sizeof(kDeepSpaceNebulaBodyPalette[0])),
          bodyField
        );
        float bodyAlpha = clampUnit((0.16f + structure * 0.58f) * (0.52f + intensityUnit * 0.34f));
        blendIntoPixel(r, g, b, bodyColor, bodyAlpha);
      }

      if (coreField > 0.01f) {
        NativeRgb coreColor = resolveDeepSpacePaletteColor(
          kDeepSpaceNebulaCorePalette,
          static_cast<uint8_t>(sizeof(kDeepSpaceNebulaCorePalette) / sizeof(kDeepSpaceNebulaCorePalette[0])),
          coreField
        );
        float coreAlpha = clampUnit((0.12f + coreMask * 0.62f) * (0.48f + intensityUnit * 0.38f));
        blendIntoPixel(r, g, b, coreColor, coreAlpha);
      }

      float clusterGlow = smoothstepf(
        0.34f,
        0.02f,
        fabsf(ny - (0.31f + sinf(nx * 8.0f + timeBase) * 0.012f))
      );
      if (clusterGlow > 0.24f && cloudB > 0.18f) {
        float glowAlpha = clampUnit(clusterGlow * cloudB * 0.14f);
        blendIntoPixel(r, g, b, clusterColor, glowAlpha);
      }

      DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
    }
  }
}

static void paintDeepSpaceNebulaCluster(unsigned long elapsed, float intensityUnit) {
  const int centerX = 44;
  const int centerY = 20;
  NativeRgb tint = makeRgb(245, 251, 255);

  for (int index = 0; index < 16; index++) {
    float angle = spaceGeneratorHash(index, 5, 0.37f) * 6.2831853f;
    float radius = powf(spaceGeneratorHash(index, 9, 0.91f), 1.65f) * 10.0f;
    float orbitX = (float)centerX + cosf(angle) * radius;
    float orbitY = (float)centerY + sinf(angle) * radius * 0.58f;
    float twinkle =
      0.64f +
      0.36f * sinf((float)elapsed * 0.0011f * (0.8f + spaceGeneratorHash(index, 3, 0.61f) * 0.4f) + index * 1.7f);
    float alpha = clampUnit((0.36f + intensityUnit * 0.24f) * twinkle);
    uint8_t sizeTier = spaceGeneratorHash(index, 7, 0.44f) > 0.82f ? 1 : 0;
    drawSpaceGeneratorStarShape((int)roundf(orbitX), (int)roundf(orbitY), sizeTier, tint, alpha);
  }
}

static void renderAmbientCosmicKale(unsigned long elapsed, float speedUnit, float intensityUnit) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);
  blendSpaceGeneratorNebulaLayer(elapsed, speedUnit, intensityUnit, kCosmicKaleBackgroundLayer);
  paintSpaceGeneratorStarLayer(elapsed, speedUnit, intensityUnit, kCosmicKaleStarsNear);
  paintSpaceGeneratorStarLayer(elapsed, speedUnit, intensityUnit, kCosmicKaleStarsFar);
  blendSpaceGeneratorNebulaLayer(elapsed, speedUnit, intensityUnit, kCosmicKaleForegroundLayer);
}

static void renderAmbientVoidFire(unsigned long elapsed, float speedUnit, float intensityUnit) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);
  paintSpaceGeneratorStarLayer(elapsed, speedUnit, intensityUnit, kVoidFireStarsFar);
  blendSpaceGeneratorNebulaLayer(elapsed, speedUnit, intensityUnit, kVoidFireDenseLayer);
  blendSpaceGeneratorNebulaLayer(elapsed, speedUnit, intensityUnit, kVoidFireGlowLayer);
  paintSpaceGeneratorStarLayer(elapsed, speedUnit, intensityUnit, kVoidFireStarsNear);
  blendSpaceGeneratorNebulaLayer(elapsed, speedUnit, intensityUnit, kVoidFireDriftLayer);
  blendSpaceGeneratorNebulaLayer(elapsed, speedUnit, intensityUnit, kVoidFireEmberLayer);
}

static void renderAmbientDeepSpaceNebula(unsigned long elapsed, float speedUnit, float intensityUnit) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);
  renderAmbientDeepSpaceNebulaBase(elapsed, speedUnit, intensityUnit);
  paintSpaceGeneratorStarLayer(elapsed, speedUnit, intensityUnit, kDeepSpaceNebulaStarLayer);
  paintDeepSpaceNebulaCluster(elapsed, intensityUnit);
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

struct AmbientLiquidParticle {
  float x;
  float y;
  float vx;
  float vy;
};

static constexpr int kAmbientLiquidMaxParticles = 72;
static constexpr int kAmbientLiquidLeft = 9;
static constexpr int kAmbientLiquidRight = 54;
static constexpr int kAmbientLiquidTop = 9;
static constexpr int kAmbientLiquidBottom = 57;

static AmbientLiquidParticle s_ambientLiquidParticles[kAmbientLiquidMaxParticles];
static int s_ambientLiquidParticleCount = 0;
static unsigned long s_ambientLiquidLastElapsed = 0;
static float s_ambientLiquidSimTimeMs = 0.0f;
static uint8_t s_ambientLiquidLastSpeed = 0;
static uint8_t s_ambientLiquidLastIntensity = 0;
static bool s_ambientLiquidInitialized = false;

static float ambientLiquidSeedf(int index, float offset) {
  return fractf32(
    sinf(((float)index + 1.0f) * 12.9898f + offset * 78.233f) * 43758.5453f
  );
}

static int resolveAmbientLiquidParticleCount(float intensityUnit) {
  int particleCount = 28 + (int)roundf(intensityUnit * 36.0f);
  if (particleCount < 28) {
    return 28;
  }
  if (particleCount > kAmbientLiquidMaxParticles) {
    return kAmbientLiquidMaxParticles;
  }
  return particleCount;
}

static void resetAmbientLiquidState(float speedUnit, float intensityUnit) {
  s_ambientLiquidParticleCount = resolveAmbientLiquidParticleCount(intensityUnit);
  s_ambientLiquidLastElapsed = 0;
  s_ambientLiquidSimTimeMs = 0.0f;

  const float usableWidth = (float)(kAmbientLiquidRight - kAmbientLiquidLeft - 4);
  const float usableHeight = (float)(kAmbientLiquidBottom - kAmbientLiquidTop - 6);
  for (int index = 0; index < s_ambientLiquidParticleCount; index++) {
    s_ambientLiquidParticles[index].x =
      (float)kAmbientLiquidLeft + 2.0f + ambientLiquidSeedf(index, 0.17f) * usableWidth;
    s_ambientLiquidParticles[index].y =
      (float)kAmbientLiquidTop +
      usableHeight * (0.54f + ambientLiquidSeedf(index, 0.41f) * 0.38f);
    s_ambientLiquidParticles[index].vx =
      (ambientLiquidSeedf(index, 0.63f) - 0.5f) * (0.8f + speedUnit * 0.6f);
    s_ambientLiquidParticles[index].vy =
      (ambientLiquidSeedf(index, 0.89f) - 0.5f) * 0.2f;
  }
}

static void stepAmbientLiquidState(float stepMs, float speedUnit, float intensityUnit) {
  s_ambientLiquidSimTimeMs += stepMs;

  const float dt = stepMs / 16.0f;
  const float timeSeconds = s_ambientLiquidSimTimeMs * 0.001f;
  const float swayA = sinf(timeSeconds * (1.6f + speedUnit * 1.5f));
  const float swayB = sinf(timeSeconds * (0.64f + speedUnit * 0.44f) + 1.7f);
  const float swayC = sinf(timeSeconds * 0.31f + 0.9f);
  const float gravityX =
    (swayA * 0.08f + swayB * 0.05f + swayC * 0.03f) *
    (0.48f + intensityUnit * 0.52f);
  const float gravityY =
    0.085f + 0.01f * (0.5f + 0.5f * sinf(timeSeconds * 0.73f + 0.35f));
  const float minDist = 2.35f + intensityUnit * 0.65f;
  const float minDistSq = minDist * minDist;

  for (int index = 0; index < s_ambientLiquidParticleCount; index++) {
    AmbientLiquidParticle& particle = s_ambientLiquidParticles[index];
    particle.vx += gravityX * dt;
    particle.vy += gravityY * dt;
    particle.vx *= 0.992f;
    particle.vy *= 0.992f;
    particle.x += particle.vx * dt;
    particle.y += particle.vy * dt;

    if (particle.x < (float)kAmbientLiquidLeft + 1.0f) {
      particle.x = (float)kAmbientLiquidLeft + 1.0f;
      particle.vx *= -0.72f;
    }
    if (particle.x > (float)kAmbientLiquidRight - 1.0f) {
      particle.x = (float)kAmbientLiquidRight - 1.0f;
      particle.vx *= -0.72f;
    }
    if (particle.y < (float)kAmbientLiquidTop + 1.0f) {
      particle.y = (float)kAmbientLiquidTop + 1.0f;
      particle.vy *= -0.58f;
    }
    if (particle.y > (float)kAmbientLiquidBottom - 1.0f) {
      particle.y = (float)kAmbientLiquidBottom - 1.0f;
      particle.vy *= -0.46f;
      particle.vx *= 0.985f;
    }
  }

  for (int leftIndex = 0; leftIndex < s_ambientLiquidParticleCount; leftIndex++) {
    for (int rightIndex = leftIndex + 1;
         rightIndex < s_ambientLiquidParticleCount;
         rightIndex++) {
      AmbientLiquidParticle& left = s_ambientLiquidParticles[leftIndex];
      AmbientLiquidParticle& right = s_ambientLiquidParticles[rightIndex];
      float dx = right.x - left.x;
      float dy = right.y - left.y;
      float distSq = dx * dx + dy * dy;
      if (distSq >= minDistSq) {
        continue;
      }

      if (distSq <= 0.0001f) {
        dx = ambientLiquidSeedf(leftIndex + rightIndex, 0.27f) * 0.2f - 0.1f;
        dy = ambientLiquidSeedf(leftIndex + rightIndex, 0.61f) * 0.2f - 0.1f;
        distSq = dx * dx + dy * dy;
        if (distSq <= 0.0001f) {
          continue;
        }
      }

      float dist = sqrtf(distSq);
      float nx = dx / dist;
      float ny = dy / dist;
      float overlap = (minDist - dist) * 0.5f;
      left.x -= nx * overlap;
      left.y -= ny * overlap;
      right.x += nx * overlap;
      right.y += ny * overlap;

      float leftNormal = left.vx * nx + left.vy * ny;
      float rightNormal = right.vx * nx + right.vy * ny;
      float sharedNormal = (leftNormal + rightNormal) * 0.5f;
      left.vx += (sharedNormal - leftNormal) * nx;
      left.vy += (sharedNormal - leftNormal) * ny;
      right.vx += (sharedNormal - rightNormal) * nx;
      right.vy += (sharedNormal - rightNormal) * ny;
    }
  }
}

static void ensureAmbientLiquidState(
  unsigned long elapsed,
  uint8_t speedValue,
  uint8_t intensityValue
) {
  float speedUnit = (float)speedValue / 10.0f;
  float intensityUnit = (float)intensityValue / 100.0f;
  if (!s_ambientLiquidInitialized ||
      elapsed < s_ambientLiquidLastElapsed ||
      s_ambientLiquidLastSpeed != speedValue ||
      s_ambientLiquidLastIntensity != intensityValue) {
    resetAmbientLiquidState(speedUnit, intensityUnit);
    s_ambientLiquidInitialized = true;
    s_ambientLiquidLastSpeed = speedValue;
    s_ambientLiquidLastIntensity = intensityValue;
  }

  while (s_ambientLiquidLastElapsed < elapsed) {
    unsigned long remaining = elapsed - s_ambientLiquidLastElapsed;
    unsigned long stepMs = remaining > 16UL ? 16UL : remaining;
    stepAmbientLiquidState((float)stepMs, speedUnit, intensityUnit);
    s_ambientLiquidLastElapsed += stepMs;
  }
}

static void renderAmbientMetaBlob(
  unsigned long elapsed,
  uint8_t speedValue,
  uint8_t intensityValue
) {
  DisplayManager::dma_display->fillScreenRGB888(0, 0, 0);
  ensureAmbientLiquidState(elapsed, speedValue, intensityValue);

  uint8_t density[64 * 64];
  memset(density, 0, sizeof(density));
  const float radius = 4.0f;
  const float radiusSq = radius * radius;
  for (int index = 0; index < s_ambientLiquidParticleCount; index++) {
    const AmbientLiquidParticle& particle = s_ambientLiquidParticles[index];
    int minX = max(0, (int)floorf(particle.x - radius));
    int maxX = min(63, (int)ceilf(particle.x + radius));
    int minY = max(0, (int)floorf(particle.y - radius));
    int maxY = min(63, (int)ceilf(particle.y + radius));
    for (int y = minY; y <= maxY; y++) {
      for (int x = minX; x <= maxX; x++) {
        float dx = (float)x + 0.5f - particle.x;
        float dy = (float)y + 0.5f - particle.y;
        float distSq = dx * dx + dy * dy;
        if (distSq > radiusSq) {
          continue;
        }
        float weight = 1.0f - distSq / radiusSq;
        int pixelIndex = y * 64 + x;
        int nextValue =
          (int)density[pixelIndex] + (int)roundf(weight * weight * 48.0f);
        density[pixelIndex] = nextValue > 255 ? 255 : (uint8_t)nextValue;
      }
    }
  }

  float intensityUnit = (float)intensityValue / 100.0f;
  for (int y = kAmbientLiquidTop; y <= kAmbientLiquidBottom; y++) {
    for (int x = kAmbientLiquidLeft; x <= kAmbientLiquidRight; x++) {
      int pixelIndex = y * 64 + x;
      float value = (float)density[pixelIndex] / 255.0f;
      if (value <= 0.02f) {
        continue;
      }

      float glow = clampUnit((value - 0.05f) / 0.18f);
      float body = clampUnit((value - 0.18f) / 0.5f);
      float above = y > 0 ? (float)density[(y - 1) * 64 + x] / 255.0f : 0.0f;
      float below = y < 63 ? (float)density[(y + 1) * 64 + x] / 255.0f : 0.0f;
      float surface = clampUnit((value - above) * 5.0f);
      float depth =
        clampUnit((float)(y - kAmbientLiquidTop) /
                  (float)(kAmbientLiquidBottom - kAmbientLiquidTop));

      float rf = 2.0f + (18.0f - 2.0f) * glow;
      float gf = 10.0f + (86.0f - 10.0f) * glow;
      float bf = 28.0f + (170.0f - 28.0f) * glow;
      rf = rf * (1.0f - body) + (22.0f + depth * 10.0f) * body;
      gf = gf * (1.0f - body) + (110.0f + depth * 18.0f) * body;
      bf = bf * (1.0f - body) + (205.0f + depth * 26.0f) * body;

      float whitecap = surface * surface * (0.38f + intensityUnit * 0.34f);
      uint8_t r = clampByte((int)roundf(rf + 90.0f * whitecap + below * 18.0f));
      uint8_t g = clampByte((int)roundf(gf + 110.0f * whitecap + below * 24.0f));
      uint8_t b = clampByte((int)roundf(bf + 120.0f * whitecap + below * 30.0f));
      addHighlight(r, g, b, whitecap * 0.25f);
      DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
    }
  }

  const NativeRgb shell = makeRgb(10, 18, 30);
  const NativeRgb floor = makeRgb(18, 32, 44);
  for (int y = kAmbientLiquidTop; y <= kAmbientLiquidBottom; y++) {
    DisplayManager::dma_display->drawPixelRGB888(
      kAmbientLiquidLeft - 1,
      y,
      shell.r,
      shell.g,
      shell.b
    );
    DisplayManager::dma_display->drawPixelRGB888(
      kAmbientLiquidRight + 1,
      y,
      shell.r,
      shell.g,
      shell.b
    );
  }
  for (int x = kAmbientLiquidLeft - 1; x <= kAmbientLiquidRight + 1; x++) {
    DisplayManager::dma_display->drawPixelRGB888(
      x,
      kAmbientLiquidBottom + 1,
      floor.r,
      floor.g,
      floor.b
    );
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

struct AmbientWaterSurfacePulse {
  float x;
  float y;
  float strength;
  unsigned long bornAt;
};

struct AmbientWaterSurfacePulseFrameState {
  float x;
  float y;
  float strength;
  float phaseOffset;
  float ageDecay;
};

struct AmbientWaterSurfaceAxisSample {
  uint8_t index0;
  uint8_t index1;
  float alpha;
};

static constexpr int kAmbientWaterPanelSize = 64;
static constexpr int kAmbientWaterPanelPixels = kAmbientWaterPanelSize * kAmbientWaterPanelSize;
static constexpr int kAmbientWaterSurfaceMaxPulses = 6;
static constexpr float kAmbientWaterSurfacePulseDistanceCutoffSq = 0.18f;
static constexpr float kAmbientWaterSurfacePulseDistanceCutoff = 0.42426407f;
static constexpr float kAmbientWaterUnitCoordStep = 1.0f / 63.0f;
static AmbientWaterSurfacePulse s_ambientWaterSurfacePulses[kAmbientWaterSurfaceMaxPulses];
static AmbientWaterSurfacePulseFrameState s_ambientWaterSurfacePulseFrameStates[kAmbientWaterSurfaceMaxPulses];
static int s_ambientWaterSurfacePulseCount = 0;
static unsigned long s_ambientWaterSurfaceLastElapsed = 0;
static unsigned long s_ambientWaterSurfaceLastSpawnAt = 0;
static float s_ambientUnitCoordLut[kAmbientWaterPanelSize];
static bool s_ambientUnitCoordLutReady = false;
static AmbientWaterSurfaceAxisSample
  s_ambientWaterSurfaceNegativeOffsetSamples[kAmbientWaterPanelSize];
static AmbientWaterSurfaceAxisSample
  s_ambientWaterSurfacePositiveOffsetSamples[kAmbientWaterPanelSize];
static bool s_ambientWaterSurfaceAxisSamplesReady = false;
static uint16_t s_ambientWaterSurfaceFieldCache[kAmbientWaterPanelPixels];

static float ambientWaterSeedf(int index, float offset) {
  return ambientLiquidSeedf(index, offset);
}

static void ensureAmbientUnitCoordLut() {
  if (s_ambientUnitCoordLutReady) {
    return;
  }

  for (int index = 0; index < kAmbientWaterPanelSize; index++) {
    s_ambientUnitCoordLut[index] = (float)index / (float)(kAmbientWaterPanelSize - 1);
  }
  s_ambientUnitCoordLutReady = true;
}

static AmbientWaterSurfaceAxisSample buildAmbientWaterSurfaceAxisSample(
  int index,
  float offsetPixels
) {
  float samplePosition = (float)index + offsetPixels;
  if (samplePosition < 0.0f) {
    samplePosition = 0.0f;
  } else if (samplePosition > (float)(kAmbientWaterPanelSize - 1)) {
    samplePosition = (float)(kAmbientWaterPanelSize - 1);
  }

  int index0 = (int)samplePosition;
  int index1 = index0 < (kAmbientWaterPanelSize - 1) ? (index0 + 1) : index0;
  return {
    (uint8_t)index0,
    (uint8_t)index1,
    samplePosition - (float)index0
  };
}

static void ensureAmbientWaterSurfaceAxisSamples() {
  if (s_ambientWaterSurfaceAxisSamplesReady) {
    return;
  }

  for (int index = 0; index < kAmbientWaterPanelSize; index++) {
    s_ambientWaterSurfaceNegativeOffsetSamples[index] =
      buildAmbientWaterSurfaceAxisSample(index, -1.4f);
    s_ambientWaterSurfacePositiveOffsetSamples[index] =
      buildAmbientWaterSurfaceAxisSample(index, 1.4f);
  }

  s_ambientWaterSurfaceAxisSamplesReady = true;
}

static uint16_t encodeAmbientUnitField(float value) {
  return (uint16_t)roundf(clampUnit(value) * 65535.0f);
}

static float decodeAmbientUnitField(uint16_t value) {
  return (float)value / 65535.0f;
}

static void resetAmbientWaterSurfaceState() {
  s_ambientWaterSurfacePulseCount = 0;
  s_ambientWaterSurfaceLastElapsed = 0;
  s_ambientWaterSurfaceLastSpawnAt = 0;
}

static void pushAmbientWaterSurfacePulse(
  float x,
  float y,
  float strength,
  unsigned long bornAt
) {
  if (s_ambientWaterSurfacePulseCount >= kAmbientWaterSurfaceMaxPulses) {
    for (int index = 1; index < s_ambientWaterSurfacePulseCount; index++) {
      s_ambientWaterSurfacePulses[index - 1] = s_ambientWaterSurfacePulses[index];
    }
    s_ambientWaterSurfacePulseCount = kAmbientWaterSurfaceMaxPulses - 1;
  }

  s_ambientWaterSurfacePulses[s_ambientWaterSurfacePulseCount++] = {
    x,
    y,
    strength,
    bornAt
  };
}

static void updateAmbientWaterSurfaceState(
  unsigned long elapsed,
  float densityUnit,
  float frequencyUnit,
  float strengthUnit
) {
  if (elapsed < s_ambientWaterSurfaceLastElapsed) {
    resetAmbientWaterSurfaceState();
  }

  int nextCount = 0;
  for (int index = 0; index < s_ambientWaterSurfacePulseCount; index++) {
    const AmbientWaterSurfacePulse& pulse = s_ambientWaterSurfacePulses[index];
    if (elapsed - pulse.bornAt >= 6200UL) {
      continue;
    }
    s_ambientWaterSurfacePulses[nextCount++] = pulse;
  }
  s_ambientWaterSurfacePulseCount = nextCount;

  float spawnBlend = clampUnit(frequencyUnit * 0.68f + densityUnit * 0.32f);
  unsigned long interval = (unsigned long)roundf(mixf(3600.0f, 1850.0f, spawnBlend));
  if (interval < 1UL) {
    interval = 1UL;
  }
  if (elapsed - s_ambientWaterSurfaceLastSpawnAt >= interval) {
    s_ambientWaterSurfaceLastSpawnAt = elapsed;
    int seedIndex =
      s_ambientWaterSurfacePulseCount + (int)floorf((float)elapsed / (float)interval);
    pushAmbientWaterSurfacePulse(
      0.18f + ambientWaterSeedf(seedIndex, 0.31f) * 0.64f,
      0.2f + ambientWaterSeedf(seedIndex, 0.63f) * 0.46f,
      0.14f + strengthUnit * 0.16f + ambientWaterSeedf(seedIndex, 0.87f) * 0.16f,
      elapsed
    );
  }

  s_ambientWaterSurfaceLastElapsed = elapsed;
}

static float sampleAmbientWaterSurfaceBaseField(
  float driftX,
  float driftY,
  float bandWarpA,
  float bandWarpB,
  float timeBase,
  float speedUnit,
  float densityUnit
) {
  float longSwell =
    ambientWaterSin(
      (driftY * 1.72f + bandWarpA * 0.2f + driftX * 0.09f) *
      (10.4f + densityUnit * 3.8f) -
      timeBase * (1.42f + speedUnit * 0.78f)
    );
  float secondarySwell =
    ambientWaterSin(
      (driftY * 1.18f - driftX * 0.26f + bandWarpB * 0.16f) *
      (14.2f + densityUnit * 4.6f) -
      timeBase * (1.84f + speedUnit * 0.96f)
    );
  float diagonalSwell =
    ambientWaterSin(
      (driftY * 0.82f + driftX * 0.42f + bandWarpA * 0.08f) * 8.6f -
      timeBase * (1.08f + speedUnit * 0.54f)
    );
  float backwash =
    ambientWaterCos(
      (driftY * 1.36f - driftX * 0.92f + bandWarpB * 0.06f) * 9.2f +
      timeBase * (1.18f + densityUnit * 0.64f)
    );
  float capillary =
    ambientWaterSin(
      (driftY * 2.84f + driftX * 0.62f + bandWarpA * 0.04f) * 22.8f -
      timeBase * (2.46f + speedUnit * 0.84f)
    );
  float trough =
    ambientWaterCos(
      (driftY * 0.66f - driftX * 0.14f) * 6.4f -
      timeBase * (0.82f + speedUnit * 0.3f)
    );

  return clampUnit(
    0.5f +
    0.5f *
      (longSwell * 0.34f +
       secondarySwell * 0.24f +
       diagonalSwell * 0.16f +
       backwash * 0.1f +
       capillary * 0.08f +
       trough * 0.08f)
  );
}

static void buildAmbientWaterSurfaceFieldCache(
  unsigned long elapsed,
  float timeBase,
  float speedUnit,
  float densityUnit
) {
  ensureAmbientUnitCoordLut();
  const float rippleScale = 0.045f + densityUnit * 0.018f;
  const float scaledMaxIndex = (float)(kAmbientWaterPanelSize - 1);

  for (int y = 0; y < kAmbientWaterPanelSize; y++) {
    float ny = s_ambientUnitCoordLut[y];
    float driftY = ny - 0.5f;
    float rowBandWarpSeedA = ambientWaterSin(driftY * 4.6f - timeBase * 0.2f);
    float rowBandWarpSeedB = ambientWaterSin(driftY * 7.1f + timeBase * 0.28f);
    int rowOffset = y * kAmbientWaterPanelSize;
    float baseRow[kAmbientWaterPanelSize];
    float rippleRow[kAmbientWaterPanelSize] = {};
    const float driftXStart = -0.5f;
    const float bandWarpAPhaseStart =
      driftXStart * 5.8f + timeBase * 0.58f + rowBandWarpSeedA * 0.86f;
    const float bandWarpBPhaseStart =
      driftXStart * 3.2f - timeBase * 0.36f + rowBandWarpSeedB * 0.58f;
    const float bandWarpADelta = 5.8f * kAmbientWaterUnitCoordStep;
    const float bandWarpBDelta = 3.2f * kAmbientWaterUnitCoordStep;
    float bandWarpASin = ambientWaterSin(bandWarpAPhaseStart);
    float bandWarpACos = ambientWaterCos(bandWarpAPhaseStart);
    float bandWarpADeltaSin = ambientWaterSin(bandWarpADelta);
    float bandWarpADeltaCos = ambientWaterCos(bandWarpADelta);
    float bandWarpBSin = ambientWaterSin(bandWarpBPhaseStart);
    float bandWarpBCos = ambientWaterCos(bandWarpBPhaseStart);
    float bandWarpBDeltaSin = ambientWaterSin(bandWarpBDelta);
    float bandWarpBDeltaCos = ambientWaterCos(bandWarpBDelta);

    for (int x = 0; x < kAmbientWaterPanelSize; x++) {
      float driftX = s_ambientUnitCoordLut[x] - 0.5f;
      baseRow[x] =
        sampleAmbientWaterSurfaceBaseField(
          driftX,
          driftY,
          bandWarpASin,
          bandWarpBSin,
          timeBase,
          speedUnit,
          densityUnit
        );

      if (x == (kAmbientWaterPanelSize - 1)) {
        continue;
      }

      if ((x & 15) == 15) {
        float nextDriftX = s_ambientUnitCoordLut[x + 1] - 0.5f;
        float nextBandWarpAPhase =
          nextDriftX * 5.8f + timeBase * 0.58f + rowBandWarpSeedA * 0.86f;
        bandWarpASin = ambientWaterSin(nextBandWarpAPhase);
        bandWarpACos = ambientWaterCos(nextBandWarpAPhase);
        float nextBandWarpBPhase =
          nextDriftX * 3.2f - timeBase * 0.36f + rowBandWarpSeedB * 0.58f;
        bandWarpBSin = ambientWaterSin(nextBandWarpBPhase);
        bandWarpBCos = ambientWaterCos(nextBandWarpBPhase);
        continue;
      }

      float nextBandWarpASin =
        bandWarpASin * bandWarpADeltaCos + bandWarpACos * bandWarpADeltaSin;
      float nextBandWarpACos =
        bandWarpACos * bandWarpADeltaCos - bandWarpASin * bandWarpADeltaSin;
      bandWarpASin = nextBandWarpASin;
      bandWarpACos = nextBandWarpACos;

      float nextBandWarpBSin =
        bandWarpBSin * bandWarpBDeltaCos + bandWarpBCos * bandWarpBDeltaSin;
      float nextBandWarpBCos =
        bandWarpBCos * bandWarpBDeltaCos - bandWarpBSin * bandWarpBDeltaSin;
      bandWarpBSin = nextBandWarpBSin;
      bandWarpBCos = nextBandWarpBCos;
    }

    for (int pulseIndex = 0; pulseIndex < s_ambientWaterSurfacePulseCount; pulseIndex++) {
      const AmbientWaterSurfacePulseFrameState& pulse =
        s_ambientWaterSurfacePulseFrameStates[pulseIndex];
      float dy = ny - pulse.y;
      float dySq = dy * dy;
      if (dySq > kAmbientWaterSurfacePulseDistanceCutoffSq) {
        continue;
      }

      float spanX = sqrtf(kAmbientWaterSurfacePulseDistanceCutoffSq - dySq);
      int minX =
        (int)floorf((pulse.x - kAmbientWaterSurfacePulseDistanceCutoff) * scaledMaxIndex);
      int maxX =
        (int)ceilf((pulse.x + kAmbientWaterSurfacePulseDistanceCutoff) * scaledMaxIndex);
      if (minX < 0) {
        minX = 0;
      }
      if (maxX > (kAmbientWaterPanelSize - 1)) {
        maxX = kAmbientWaterPanelSize - 1;
      }

      int tighterMinX = (int)floorf((pulse.x - spanX) * scaledMaxIndex);
      int tighterMaxX = (int)ceilf((pulse.x + spanX) * scaledMaxIndex);
      if (tighterMinX > minX) {
        minX = tighterMinX;
      }
      if (tighterMaxX < maxX) {
        maxX = tighterMaxX;
      }
      if (minX > maxX) {
        continue;
      }

      float pulseScale = pulse.ageDecay * pulse.strength;
      for (int x = minX; x <= maxX; x++) {
        float dx = s_ambientUnitCoordLut[x] - pulse.x;
        float distSq = dx * dx + dySq;
        if (distSq > kAmbientWaterSurfacePulseDistanceCutoffSq) {
          continue;
        }
        float dist = sqrtf(distSq);
        rippleRow[x] +=
          ambientWaterSin(dist * 54.0f - pulse.phaseOffset) *
          expf(-dist * 11.4f) *
          pulseScale;
      }
    }

    for (int x = 0; x < kAmbientWaterPanelSize; x++) {
      s_ambientWaterSurfaceFieldCache[rowOffset + x] =
        encodeAmbientUnitField(clampUnit(baseRow[x] + rippleRow[x] * rippleScale));
    }
  }
}

static inline float readAmbientWaterSurfaceFieldCacheValue(int offset) {
  return decodeAmbientUnitField(s_ambientWaterSurfaceFieldCache[offset]);
}

static inline float sampleAmbientWaterSurfaceFieldCacheHorizontal(
  int rowOffset,
  const AmbientWaterSurfaceAxisSample& sample
) {
  return mixf(
    readAmbientWaterSurfaceFieldCacheValue(rowOffset + (int)sample.index0),
    readAmbientWaterSurfaceFieldCacheValue(rowOffset + (int)sample.index1),
    sample.alpha
  );
}

static inline float sampleAmbientWaterSurfaceFieldCacheVertical(
  int x,
  int rowOffset0,
  int rowOffset1,
  float alpha
) {
  return mixf(
    readAmbientWaterSurfaceFieldCacheValue(rowOffset0 + x),
    readAmbientWaterSurfaceFieldCacheValue(rowOffset1 + x),
    alpha
  );
}

static void prepareAmbientWaterSurfacePulseFrameStates(
  unsigned long elapsed,
  float speedUnit
) {
  const float pulsePhaseSpeed = 7.4f + speedUnit * 2.8f;
  for (int index = 0; index < s_ambientWaterSurfacePulseCount; index++) {
    const AmbientWaterSurfacePulse& pulse = s_ambientWaterSurfacePulses[index];
    float age = (float)(elapsed - pulse.bornAt) * 0.001f;
    s_ambientWaterSurfacePulseFrameStates[index] = {
      pulse.x,
      pulse.y,
      pulse.strength,
      age * pulsePhaseSpeed,
      expf(-age * 0.68f)
    };
  }
}

static void shadeAmbientWaterSurfaceFrame(
  float timeBase,
  float speedUnit,
  float intensityUnit,
  float frequencyUnit,
  float strengthUnit,
  float waterLevelUnit,
  float surfaceDensityUnit
) {
  const float sampleStep = 1.4f * kAmbientWaterUnitCoordStep;
  const NativeRgb deep = makeRgb(7, 33, 93);
  const NativeRgb mid = makeRgb(20, 90, 176);
  const NativeRgb shallow = makeRgb(49, 129, 199);
  const NativeRgb bright = makeRgb(119, 209, 245);
  const NativeRgb foam = makeRgb(228, 246, 255);
  const NativeRgb troughColor = makeRgb(3, 16, 48);
  const NativeRgb edgeColor = makeRgb(2, 11, 33);
  const float lightDirX = -0.46f;
  const float lightDirY = 0.36f;
  const float lightDirZ = 0.82f;
  const float invGradientSpan = 1.0f / (sampleStep * 2.0f);
  const float normalXScale =
    0.2f + surfaceDensityUnit * 0.08f + strengthUnit * 0.06f;
  const float normalYScale =
    0.88f + surfaceDensityUnit * 0.18f + strengthUnit * 0.12f;
  const float specularPower = 14.0f - intensityUnit * 5.5f;
  const float ridgeHighlightScale =
    0.05f + intensityUnit * 0.09f + strengthUnit * 0.05f;
  const float foamAmountScale =
    0.08f + intensityUnit * 0.14f + strengthUnit * 0.1f;
  const float highlightScale = 0.12f + intensityUnit * 0.1f;
  const float sparkleScale = 0.03f + intensityUnit * 0.05f;
  const float foamGrainScale = 0.02f + strengthUnit * 0.05f;
  const float subsurfaceRowScale = 0.04f + waterLevelUnit * 0.08f;
  const float depthMixRowScale = 0.5f + (1.0f - waterLevelUnit) * 0.18f;
  const float depthMixFieldScale = 0.16f + waterLevelUnit * 0.08f;
  const float ridgeXScale = 0.16f * 15.6f;
  const float ridgeTimeScale = timeBase * (1.12f + speedUnit * 0.22f);
  const float foamXScale = 26.0f + frequencyUnit * 12.0f;
  const float foamTimeScale = timeBase * (5.2f + frequencyUnit * 1.7f);
  const float undercurrentScale = 5.8f + frequencyUnit * 2.2f;
  const float undercurrentXScale =
    (0.72f + frequencyUnit * 0.24f) * undercurrentScale;
  const float undercurrentTimeScale =
    timeBase * (1.1f + surfaceDensityUnit * 0.36f);
  const float ridgeDelta = ridgeXScale * kAmbientWaterUnitCoordStep;
  const float foamDelta = foamXScale * kAmbientWaterUnitCoordStep;
  const float undercurrentDelta =
    undercurrentXScale * kAmbientWaterUnitCoordStep;
  float pow32Lut[kAmbientWaterPowLutSize + 1];
  float pow46Lut[kAmbientWaterPowLutSize + 1];
  float specularLut[kAmbientWaterPowLutSize + 1];
  float waveScratchA[64];
  float waveScratchB[64];
  float waveScratchC[64];

  ensureAmbientUnitCoordLut();
  ensureAmbientWaterSurfaceAxisSamples();
  buildAmbientWaterUnitPowLut(pow32Lut, 3.2f);
  buildAmbientWaterUnitPowLut(pow46Lut, 4.6f);
  for (int index = 0; index <= kAmbientWaterPowLutSize; index++) {
    float x = (float)index / (float)kAmbientWaterPowLutSize;
    specularLut[index] = powf(x, specularPower);
  }

  for (int y = 0; y < 64; y++) {
    float ny = s_ambientUnitCoordLut[y];
    float edgeShadeY = fabsf(ny - 0.48f) * 0.54f;
    float subsurfaceBase = 0.04f + (1.0f - ny) * subsurfaceRowScale;
    float depthMixBase = 0.04f + ny * depthMixRowScale;
    int rowOffset = y * kAmbientWaterPanelSize;
    const AmbientWaterSurfaceAxisSample& upSample =
      s_ambientWaterSurfaceNegativeOffsetSamples[y];
    const AmbientWaterSurfaceAxisSample& downSample =
      s_ambientWaterSurfacePositiveOffsetSamples[y];
    int upRowOffset0 = (int)upSample.index0 * kAmbientWaterPanelSize;
    int upRowOffset1 = (int)upSample.index1 * kAmbientWaterPanelSize;
    int downRowOffset0 = (int)downSample.index0 * kAmbientWaterPanelSize;
    int downRowOffset1 = (int)downSample.index1 * kAmbientWaterPanelSize;
    const float ridgeRowPhase = ny * 2.04f * 15.6f - ridgeTimeScale;
    const float foamRowPhase =
      ny * 17.0f -
      foamTimeScale +
      ambientWaterSin(ny * 11.0f + timeBase * 1.2f) * 0.6f;
    const float undercurrentRowPhase =
      (-ny * 1.14f) * undercurrentScale - undercurrentTimeScale;
    fillAmbientWaterWaveSequence(waveScratchA, ridgeRowPhase, ridgeDelta, false);
    fillAmbientWaterWaveSequence(waveScratchB, foamRowPhase, foamDelta, false);
    fillAmbientWaterWaveSequence(
      waveScratchC,
      undercurrentRowPhase,
      undercurrentDelta,
      false
    );
    for (int x = 0; x < 64; x++) {
      float edgeShadeX = fabsf(s_ambientUnitCoordLut[x] - 0.5f) * 1.18f;
      const AmbientWaterSurfaceAxisSample& leftSample =
        s_ambientWaterSurfaceNegativeOffsetSamples[x];
      const AmbientWaterSurfaceAxisSample& rightSample =
        s_ambientWaterSurfacePositiveOffsetSamples[x];
      float field = readAmbientWaterSurfaceFieldCacheValue(rowOffset + x);
      float fieldL =
        sampleAmbientWaterSurfaceFieldCacheHorizontal(rowOffset, leftSample);
      float fieldR =
        sampleAmbientWaterSurfaceFieldCacheHorizontal(rowOffset, rightSample);
      float fieldU =
        sampleAmbientWaterSurfaceFieldCacheVertical(
          x,
          upRowOffset0,
          upRowOffset1,
          upSample.alpha
        );
      float fieldD =
        sampleAmbientWaterSurfaceFieldCacheVertical(
          x,
          downRowOffset0,
          downRowOffset1,
          downSample.alpha
        );

      float fieldSmooth =
        clampUnit(
          field * 0.44f +
          (fieldL + fieldR + fieldU + fieldD) * 0.14f
        );
      float gradientX = (fieldR - fieldL) * invGradientSpan;
      float gradientY = (fieldD - fieldU) * invGradientSpan;
      float curvature =
        fieldR + fieldL + fieldD + fieldU - fieldSmooth * 4.0f;
      float slope = fabsf(gradientX) * 0.44f + fabsf(gradientY) * 0.96f;
      float normalX = -gradientX * normalXScale;
      float normalY = -gradientY * normalYScale;
      float normalLength = sqrtf(normalX * normalX + normalY * normalY + 1.0f);
      float lightAmount =
        clampUnit(
          (normalX * lightDirX + normalY * lightDirY + lightDirZ) /
          normalLength
        );
      float specular =
        sampleAmbientWaterUnitPowLut(specularLut, lightAmount);
      float longGlint =
        pow7fExact(
          clampUnit(1.0f - fabsf(gradientX * 2.6f - gradientY * 0.22f - 0.12f))
        );
      float undercurrent =
        0.5f +
        0.5f * waveScratchC[x];
      float crestAmount =
        smoothstepf(0.58f - strengthUnit * 0.05f, 0.86f, fieldSmooth) *
        smoothstepf(0.05f, 0.34f + strengthUnit * 0.04f, slope);
      float ridgeHighlight =
        crestAmount *
        sampleAmbientWaterUnitPowLut(
          pow32Lut,
          0.5f + 0.5f * waveScratchA[x]
        ) *
        ridgeHighlightScale;
      float foamAmount =
        crestAmount *
        smoothstepf(-0.2f, 0.05f, -curvature) *
        foamAmountScale;
      float highlightAmount =
        crestAmount * longGlint * highlightScale;
      float sparkle =
        smoothstepf(0.52f, 0.9f, fieldSmooth) *
        pow4fExact(
          clampUnit(1.0f - fabsf(gradientY * 0.7f + gradientX * 1.34f - 0.1f))
        ) *
        sparkleScale;
      float foamGrain =
        smoothstepf(0.8f - strengthUnit * 0.06f, 1.0f, fieldSmooth) *
        sampleAmbientWaterUnitPowLut(
          pow46Lut,
          0.5f + 0.5f * waveScratchB[x]
        ) *
        foamGrainScale;
      float subsurface =
        smoothstepf(0.36f, 0.8f, fieldSmooth) *
        (subsurfaceBase + undercurrent * 0.035f);
      float troughShadow =
        smoothstepf(0.08f, 0.52f, 1.0f - fieldSmooth) *
        smoothstepf(0.035f, 0.28f, slope) *
        0.24f;

      float depthMix =
        clampUnit(
          depthMixBase +
          fieldSmooth * depthMixFieldScale
        );
      NativeRgb color = blendRgbColor(deep, mid, depthMix);
      color =
        blendRgbColor(
          color,
          shallow,
          smoothstepf(0.5f, 0.88f, fieldSmooth) * 0.32f + highlightAmount * 0.08f
        );
      color = blendRgbColor(color, bright, subsurface);
      color =
        blendRgbColor(
          color,
          bright,
          clampUnit(
            highlightAmount * 0.78f +
            ridgeHighlight * 0.74f +
            specular * 0.28f +
            sparkle
          )
        );
      color = blendRgbColor(color, foam, foamAmount * 0.74f);
      color = blendRgbColor(color, foam, foamGrain);
      color = blendRgbColor(color, troughColor, troughShadow);

      float edgeShade =
        smoothstepf(
          1.06f,
          0.22f,
          edgeShadeX + edgeShadeY
        );
      color = blendRgbColor(
        edgeColor,
        color,
        0.42f + edgeShade * 0.58f
      );

      drawAmbientWaterPixel(
        x,
        y,
        color.r,
        color.g,
        color.b
      );
    }
  }
}

static void renderAmbientWaterSurface(
  unsigned long elapsed,
  float speedUnit,
  float intensityUnit
) {
  const float waterLevelUnit = 0.72f;
  const float densityUnit = 0.58f;
  const float frequencyUnit = 0.56f;
  const float strengthUnit = 0.48f;
  const float surfaceDensityUnit =
    clampUnit(densityUnit * 0.44f + frequencyUnit * 0.56f);
  const float timeBase =
    (float)elapsed *
    (0.00038f + speedUnit * 0.0011f + frequencyUnit * 0.00072f);
  updateAmbientWaterSurfaceState(
    elapsed,
    densityUnit,
    frequencyUnit,
    strengthUnit
  );
  prepareAmbientWaterSurfacePulseFrameStates(elapsed, speedUnit);
  buildAmbientWaterSurfaceFieldCache(
    elapsed,
    timeBase,
    speedUnit,
    surfaceDensityUnit
  );
  shadeAmbientWaterSurfaceFrame(
    timeBase,
    speedUnit,
    intensityUnit,
    frequencyUnit,
    strengthUnit,
    waterLevelUnit,
    surfaceDensityUnit
  );
}

static float sampleAmbientWaterCausticField(
  float nx,
  float ny,
  float timeBase,
  float speedUnit,
  float intensityUnit
) {
  float px = (nx - 0.5f) * (11.6f + intensityUnit * 2.2f);
  float py = (ny - 0.46f) * (13.4f + intensityUnit * 2.8f);
  float ix = px;
  float iy = py;
  float energy = 0.0f;

  for (int index = 0; index < 3; index++) {
    float phase = timeBase * (1.2f + (float)index * 0.46f + speedUnit * 0.3f) + (float)index * 1.7f;
    float warp =
      ambientWaterSin(iy * (1.7f + (float)index * 0.22f) + phase) *
      (0.78f - (float)index * 0.12f);
    float twist =
      ambientWaterCos(ix * (1.4f + (float)index * 0.28f) - phase * 0.86f) *
      (0.66f - (float)index * 0.1f);
    float nextX =
      px + warp + ambientWaterSin(phase + ix * 0.72f - iy * 0.38f) * 0.34f;
    iy =
      py + twist + ambientWaterCos(phase * 0.92f + iy * 0.66f + ix * 0.24f) * 0.3f;
    ix = nextX;
    float ridge =
      1.0f -
      fabsf(
        ambientWaterSin(
          ix * (2.2f + (float)index * 0.42f) +
          iy * (1.8f + (float)index * 0.36f) +
          phase * 1.6f
        )
      );
    energy += powf(clampUnit(ridge), 2.3f - (float)index * 0.3f);
  }

  float swell =
    0.5f +
    0.5f *
      ambientWaterSin(
        (nx * 1.12f - ny * 0.84f) * 6.6f -
        timeBase * (0.56f + speedUnit * 0.18f)
      );
  float focus = smoothstepf(0.16f, 0.98f, ny);
  return clampUnit(
    powf(clampUnit(energy / 2.55f), 1.42f) * (0.72f + swell * 0.28f) * focus
  );
}

static void renderAmbientWaterCurrent(
  unsigned long elapsed,
  float speedUnit,
  float intensityUnit
) {
  ensureAmbientUnitCoordLut();

  const float timeBase = (float)elapsed * (0.00032f + speedUnit * 0.00108f);
  const float flowATime = timeBase * (2.2f + speedUnit * 1.4f);
  const float flowANestedTime = timeBase * 0.8f;
  const float flowBTime = timeBase * (1.7f + speedUnit * 1.1f);
  const float flowCTime = timeBase * (1.3f + intensityUnit * 0.7f);
  const float flowADelta = 8.1f * kAmbientWaterUnitCoordStep;
  const float flowBDelta = 7.4f * kAmbientWaterUnitCoordStep;
  const float flowCDelta = -4.7f * kAmbientWaterUnitCoordStep;
  float waveScratchA[64];
  float waveScratchB[64];
  float waveScratchC[64];
  const NativeRgb abyss = makeRgb(1, 10, 26);
  const NativeRgb deep = makeRgb(4, 38, 86);
  const NativeRgb bright = makeRgb(18, 98, 176);
  const NativeRgb glow = makeRgb(118, 198, 246);
  const NativeRgb vein = makeRgb(1, 7, 20);

  for (int y = 0; y < 64; y++) {
    float ny = s_ambientUnitCoordLut[y];
    float rowFlowAWarp = ambientWaterSin(ny * 5.2f + flowANestedTime) * 1.35f;
    float rowFlowB = ny * 7.4f - flowBTime;
    float rowFlowC = ny * 9.2f + flowCTime;
    fillAmbientWaterWaveSequence(
      waveScratchA,
      flowATime + rowFlowAWarp,
      flowADelta,
      false
    );
    fillAmbientWaterWaveSequence(waveScratchB, rowFlowB, flowBDelta, false);
    fillAmbientWaterWaveSequence(waveScratchC, rowFlowC, flowCDelta, true);
    for (int x = 0; x < 64; x++) {
      float nx = s_ambientUnitCoordLut[x];
      float flowA = waveScratchA[x];
      float flowB = waveScratchB[x];
      float flowC = waveScratchC[x];
      float currentField =
        clampUnit(0.5f + 0.5f * (flowA * 0.42f + flowB * 0.34f + flowC * 0.24f));
      float caustic =
        sampleAmbientWaterCausticField(nx, ny, timeBase * 0.9f, speedUnit, intensityUnit);
      float depthMix = clampUnit(0.1f + ny * 0.68f + currentField * 0.18f);
      NativeRgb color = blendRgbColor(abyss, deep, depthMix);
      color =
        blendRgbColor(
          color,
          bright,
          smoothstepf(0.48f, 0.94f, currentField) * 0.72f
        );
      color =
        blendRgbColor(
          color,
          glow,
          caustic * (0.44f + intensityUnit * 0.28f)
        );

      float darkVein =
        smoothstepf(0.68f, 0.9f, currentField) * 0.18f * (1.0f - caustic);
      color = blendRgbColor(color, vein, darkVein);

      drawAmbientWaterPixel(
        x,
        y,
        color.r,
        color.g,
        color.b
      );
    }
  }
}

static void renderAmbientWaterCaustics(
  unsigned long elapsed,
  float speedUnit,
  float intensityUnit
) {
  ensureAmbientUnitCoordLut();

  const float waterLevelUnit = 0.72f;
  const float frequencyUnit = clampUnit(0.16f + speedUnit * 0.58f);
  const float strengthUnit = clampUnit(0.18f + intensityUnit * 0.54f);
  const float causticDensityUnit =
    clampUnit(intensityUnit * 0.42f + frequencyUnit * 0.58f);
  const float timeBase =
    (float)elapsed *
    (0.00028f + speedUnit * 0.00082f + frequencyUnit * 0.00056f);
  const float lowWaveScale = 4.6f + frequencyUnit * 1.6f;
  const float lowWaveTime = timeBase * (0.72f + speedUnit * 0.2f);
  const float crossWaveScale = 6.4f + causticDensityUnit * 2.2f;
  const float crossWaveTime = timeBase * (0.94f + causticDensityUnit * 0.24f);
  const float hazeTime = timeBase * (0.48f + intensityUnit * 0.16f);
  const float hazeNestedTime = timeBase * 0.64f;
  const float lowWaveDelta = 0.82f * lowWaveScale * kAmbientWaterUnitCoordStep;
  const float crossWaveDelta = 1.08f * crossWaveScale * kAmbientWaterUnitCoordStep;
  const float hazeDelta = 1.3f * 3.8f * kAmbientWaterUnitCoordStep;
  const float brightBandDelta = 1.7f * 9.4f * kAmbientWaterUnitCoordStep;
  const float dustFlashDelta =
    (26.0f + frequencyUnit * 12.0f) * kAmbientWaterUnitCoordStep;
  float waveScratchA[64];
  float waveScratchB[64];
  float waveScratchC[64];
  float waveScratchD[64];
  float waveScratchE[64];

  const NativeRgb abyss = makeRgb(2, 14, 34);
  const NativeRgb deep = makeRgb(4, 36, 78);
  const NativeRgb teal = makeRgb(10, 82, 150);
  const NativeRgb aqua = makeRgb(80, 172, 236);
  const NativeRgb glint = makeRgb(208, 234, 250);
  const NativeRgb shade = makeRgb(1, 10, 22);
  const NativeRgb vignetteColor = makeRgb(2, 11, 24);

  for (int y = 0; y < 64; y++) {
    float ny = s_ambientUnitCoordLut[y];
    float rowLowWave =
      ny * (1.0f + waterLevelUnit * 0.3f) * lowWaveScale - lowWaveTime;
    float rowCrossWave =
      -(ny * 0.48f) * crossWaveScale + crossWaveTime;
    float rowHaze =
      ny * 0.64f * 3.8f +
      hazeTime +
      ambientWaterSin(ny * 5.2f - hazeNestedTime) * 0.42f;
    float brightBandRowPhase =
      (-ny * 1.18f) * 9.4f +
      timeBase * (1.18f + speedUnit * 0.24f);
    float dustFlashRowPhase =
      ny * 18.0f -
      timeBase * (4.4f + frequencyUnit * 1.2f);
    fillAmbientWaterWaveSequence(waveScratchA, rowLowWave, lowWaveDelta, false);
    fillAmbientWaterWaveSequence(
      waveScratchB,
      rowCrossWave,
      crossWaveDelta,
      true
    );
    fillAmbientWaterWaveSequence(waveScratchC, rowHaze, hazeDelta, false);
    fillAmbientWaterWaveSequence(
      waveScratchD,
      brightBandRowPhase,
      brightBandDelta,
      false
    );
    fillAmbientWaterWaveSequence(
      waveScratchE,
      dustFlashRowPhase,
      dustFlashDelta,
      false
    );
    for (int x = 0; x < 64; x++) {
      float nx = s_ambientUnitCoordLut[x];

      float caustic =
        sampleAmbientWaterCausticField(
          nx,
          ny,
          timeBase,
          speedUnit,
          causticDensityUnit
        );
      float lowWave =
        0.5f +
        0.5f * waveScratchA[x];
      float crossWave =
        0.5f +
        0.5f * waveScratchB[x];
      float haze =
        0.5f +
        0.5f * waveScratchC[x];
      caustic = clampUnit(caustic * 0.76f + lowWave * 0.12f + crossWave * 0.12f);
      float bottomMix = smoothstepf(0.18f, 1.0f, ny);
      float brightBand =
        caustic *
        powf(
          clampUnit(
            1.0f -
            fabsf(waveScratchD[x])
          ),
          3.4f
        );
      float dustFlash =
        pow5fExact(
          clampUnit(
            0.5f +
            0.5f * waveScratchE[x]
          )
        ) *
        (0.012f + strengthUnit * 0.04f) *
        smoothstepf(0.18f, 0.84f, ny);
      float darkShade = smoothstepf(0.46f, 0.96f, 1.0f - caustic) * 0.2f;

      NativeRgb color =
        blendRgbColor(
          abyss,
          deep,
          0.18f + bottomMix * 0.44f + lowWave * 0.1f
        );
      color =
        blendRgbColor(
          color,
          teal,
          0.16f + haze * 0.22f + crossWave * 0.14f
        );
      color =
        blendRgbColor(
          color,
          aqua,
          clampUnit(
            caustic * (0.22f + intensityUnit * 0.16f + strengthUnit * 0.12f) +
            brightBand * 0.42f
          )
        );
      color =
        blendRgbColor(
          color,
          glint,
          clampUnit(
            brightBand * (0.1f + intensityUnit * 0.12f + strengthUnit * 0.08f) +
            dustFlash
          )
        );
      color = blendRgbColor(color, shade, darkShade);

      float vignette =
        smoothstepf(
          1.12f,
          0.16f,
          fabsf(nx - 0.5f) * 1.08f + fabsf(ny - 0.62f) * 0.78f
        );
      color = blendRgbColor(vignetteColor, color, 0.46f + vignette * 0.54f);

      drawAmbientWaterPixel(
        x,
        y,
        color.r,
        color.g,
        color.b
      );
    }
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
    renderAmbientMetaBlob(
      elapsed,
      DisplayManager::ambientEffectConfig.speed,
      DisplayManager::ambientEffectConfig.intensity
    );
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
  if (preset == AMBIENT_PRESET_COSMIC_KALE) {
    renderAmbientCosmicKale(elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_VOID_FIRE) {
    renderAmbientVoidFire(elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_DEEP_SPACE_NEBULA) {
    renderAmbientDeepSpaceNebula(elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_WATER_SURFACE) {
    renderAmbientWaterSurface(elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_WATER_CURRENT) {
    renderAmbientWaterCurrent(elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset == AMBIENT_PRESET_WATER_CAUSTICS) {
    renderAmbientWaterCaustics(elapsed, speedUnit, intensityUnit);
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

  // Eyes 自己维护离屏绘制与提交，避免和通用 native guard 叠成两套帧链。
  if (nativeEffectType == NATIVE_EFFECT_EYES) {
    EyesEffect::render();
    return;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - nativeEffectStartTime;
  static unsigned long s_lastBreathRenderAt = 0;
  static unsigned long s_lastRhythmRenderAt = 0;
  static unsigned long s_lastAmbientRenderAt = 0;

  struct NativeRenderGuard {
    bool active;
    bool shouldPresent;
    bool bypassRedirect;

    NativeRenderGuard(bool useRedirect = true)
      : active(false),
        shouldPresent(true),
        bypassRedirect(!useRedirect) {
      if (!useRedirect || DisplayManager::doubleBufferEnabled) {
        return;
      }
      active = DisplayManager::beginRedirectedFrame(&DisplayManager::animationBuffer[0][0], 0);
    }

    bool ready() const {
      if (bypassRedirect || DisplayManager::doubleBufferEnabled) {
        return true;
      }
      return active;
    }

    void cancelPresent() {
      shouldPresent = false;
    }

    ~NativeRenderGuard() {
      if (!active) {
        return;
      }
      DisplayManager::endRedirectedFrame(shouldPresent);
    }
  };

  if (nativeEffectType == NATIVE_EFFECT_BREATH) {
    if (s_lastBreathRenderAt != 0 && now - s_lastBreathRenderAt < 33UL) {
      return;
    }
    s_lastBreathRenderAt = now;

    NativeRenderGuard breathRenderGuard;
    if (!breathRenderGuard.ready()) {
      return;
    }

    unsigned long durationMs = breathDurationMs(breathEffectConfig);
    if (!breathEffectConfig.loop && elapsed >= durationMs) {
      setNativeEffectNone();
      breathRenderGuard.cancelPresent();
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
    if (s_lastRhythmRenderAt != 0 && now - s_lastRhythmRenderAt < 33UL) {
      return;
    }
    s_lastRhythmRenderAt = now;

    NativeRenderGuard rhythmRenderGuard;
    if (!rhythmRenderGuard.ready()) {
      return;
    }

    if (!rhythmEffectConfig.loop && elapsed >= 60000UL) {
      setNativeEffectNone();
      rhythmRenderGuard.cancelPresent();
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
    unsigned long minRenderIntervalMs =
      ambientPresetMinRenderIntervalMs(ambientEffectConfig.preset);
    if (s_lastAmbientRenderAt != 0 &&
        now - s_lastAmbientRenderAt < minRenderIntervalMs) {
      return;
    }
    s_lastAmbientRenderAt = now;

    NativeRenderGuard ambientRenderGuard;

    if (!ambientRenderGuard.ready()) {
      return;
    }

    if (!ambientEffectConfig.loop && elapsed >= 90000UL) {
      setNativeEffectNone();
      ambientRenderGuard.cancelPresent();
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
    unsigned long visualElapsed =
      ambientPresetQuantizedElapsedMs(ambientEffectConfig.preset, elapsed);
    float timeBase = (float)visualElapsed * (0.00045f + speedUnit * 0.00135f);
    s_pendingAmbientWaterSurfaceDrawUs = 0;
    unsigned long extendedSceneDrawStartedUs = micros();

    if (renderAmbientExtendedScene(
          ambientEffectConfig.preset,
          visualElapsed,
          speedUnit,
          intensityUnit)) {
      drawAmbientWaterWorldTimeOverlayIfNeeded();
      if (shouldTrackAmbientWaterSurfacePerf()) {
        s_pendingAmbientWaterSurfaceDrawUs =
          micros() - extendedSceneDrawStartedUs;
      }
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
    drawAmbientWaterWorldTimeOverlayIfNeeded();
    return;
  }

}
static ClockConfig& resolveActiveClockConfig() {
  if (DisplayManager::currentBusinessModeTag == ModeTags::TETRIS) {
    return ConfigManager::tetrisOverlayClockConfig;
  }
  if (DisplayManager::currentMode == MODE_ANIMATION) {
    return ConfigManager::animClockConfig;
  }
  return ConfigManager::clockConfig;
}
