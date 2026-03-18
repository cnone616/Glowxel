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
DisplayManager::BlackPixel* DisplayManager::blackPixels = nullptr;
int DisplayManager::blackPixelCount = 0;

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
  dma_display->print("Glowxel");
  delay(2000);
  Serial.println("LED灯板初始化完成");
}

void DisplayManager::drawLogo(int x, int y) {
  uint16_t orange = dma_display->color565(249, 115, 22);
  uint16_t blue   = dma_display->color565(59, 130, 246);
  uint16_t pink   = dma_display->color565(236, 72, 153);

  int bs = 11, gap = 3, step = bs + gap;

  uint16_t grid[3][3] = {
    { orange, orange, blue },
    { orange, blue,   pink },
    { orange, orange, blue }
  };

  for (int row = 0; row < 3; row++)
    for (int col = 0; col < 3; col++)
      dma_display->fillRect(x + col * step, y + row * step, bs, bs, grid[row][col]);
}

// 前向声明（定义在后面）
static void _eraseClockRegion();
static int clockConfig_timeY();

void DisplayManager::displayClock(bool force) {
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

  bool hasCustomImage = cfg.image.show &&
                        imagePixels != nullptr &&
                        imagePixelCount > 0;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // 时间未同步：清屏后画默认背景，显示白色 "--:--"
    dma_display->clearScreen();
    if (currentMode == MODE_CANVAS && !hasCustomImage) {
      drawLogo(12, 18);  // 时钟背景，给时间留空间
    }
    drawTinyTextCentered("--:--", clockConfig_timeY(), dma_display->color565(255, 255, 255));
    return;
  }

  // 只在分钟变化时刷新，避免不必要的重绘（force=true 时跳过）
  static int s_lastMin = -1;
  if (!force && timeinfo.tm_min == s_lastMin) return;
  s_lastMin = timeinfo.tm_min;

  // 强制模式：先清屏再重绘全部
  if (force) {
    dma_display->clearScreen();
  } else {
    // 普通模式：只擦除时钟文字区域
    _eraseClockRegion();
  }

  // 绘制背景
  if (currentMode == MODE_CANVAS) {
    if (hasCustomImage) {
      // 有自定义图片，画图片
      for (int i = 0; i < imagePixelCount; i++) {
        PixelData& pixel = imagePixels[i];
        if (pixel.x < PANEL_RES_X && pixel.y < PANEL_RES_Y) {
          dma_display->drawPixelRGB888(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
        }
        if (i % 100 == 0) yield();
      }
    } else {
      // 没有自定义图片，画Logo九宫格作为时钟背景（偏上，给时间留空间）
      drawLogo(12, 18);
    }
  }

  // 叠加时钟文字（白色，由 clockConfig 控制）
  drawClockOverlay();
}

// 擦除时钟文字区域，然后恢复该区域的背景像素（避免黑条覆盖背景图）
static void _eraseClockRegion() {
  auto* d = DisplayManager::dma_display;
  auto& c = (DisplayManager::currentMode == MODE_ANIMATION)
    ? ConfigManager::animClockConfig
    : ConfigManager::clockConfig;

  PixelData* imagePixels = (DisplayManager::currentMode == MODE_ANIMATION)
    ? ConfigManager::animImagePixels
    : ConfigManager::staticImagePixels;

  int imagePixelCount = (DisplayManager::currentMode == MODE_ANIMATION)
    ? ConfigManager::animImagePixelCount
    : ConfigManager::staticImagePixelCount;

  bool hasImage = c.image.show && imagePixels != nullptr && imagePixelCount > 0;

  auto eraseRow = [&](int y, int fontSize) {
    int h = fontSize * 6;
    int startY = y - 1;
    if (startY < 0) startY = 0;
    int endY = startY + h;
    if (endY > 64) endY = 64;
    for (int py = startY; py < endY; py++) {
      for (int px = 0; px < 64; px++) {
        d->drawPixelRGB888(px, py, 0, 0, 0);
      }
    }
    if (hasImage) {
      for (int i = 0; i < imagePixelCount; i++) {
        PixelData& pixel = imagePixels[i];
        if (pixel.y >= startY && pixel.y < endY && pixel.x < 64) {
          d->drawPixelRGB888(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
        }
      }
    }
  };
  eraseRow(c.time.y, c.time.fontSize > 0 ? c.time.fontSize : 1);
  if (c.date.show) eraseRow(c.date.y, c.date.fontSize > 0 ? c.date.fontSize : 1);
  if (c.week.show) eraseRow(c.week.y, 1);
}

static int clockConfig_timeY() {
  return (DisplayManager::currentMode == MODE_ANIMATION)
    ? ConfigManager::animClockConfig.time.y
    : ConfigManager::clockConfig.time.y;
}

void DisplayManager::drawClockOverlay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  auto& c = (currentMode == MODE_ANIMATION)
    ? ConfigManager::animClockConfig
    : ConfigManager::clockConfig;

  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  drawTinyTextCentered(timeStr, c.time.y,
    dma_display->color565(c.time.r, c.time.g, c.time.b),
    c.time.fontSize);

  if (c.date.show) {
    char dateStr[6];
    sprintf(dateStr, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    drawTinyTextCentered(dateStr, c.date.y,
      dma_display->color565(c.date.r, c.date.g, c.date.b),
      c.date.fontSize);
  }

  if (c.week.show) {
    const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    drawTinyTextCentered(weekDays[timeinfo.tm_wday], c.week.y,
      dma_display->color565(c.week.r, c.week.g, c.week.b));
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