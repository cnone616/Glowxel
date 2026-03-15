#include "display_manager.h"
#include <time.h>

// 静态成员初始化
MatrixPanel_I2S_DMA* DisplayManager::dma_display = nullptr;
DeviceMode DisplayManager::currentMode = MODE_CANVAS;
int DisplayManager::currentBrightness = 50;
bool DisplayManager::clientConnected = false;
uint8_t DisplayManager::canvasBuffer[64][64][3];
bool DisplayManager::canvasInitialized = false;
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
  drawLogo(false);  // 开机 logo 在上半部分
  // 开机 logo 额外显示品牌名
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(220, 220, 220));
  dma_display->setCursor(11, 46);
  dma_display->print("Glowxel");
  delay(2000);
  Serial.println("LED灯板初始化完成");
}

void DisplayManager::drawLogo(bool centered) {
  uint16_t orange = dma_display->color565(249, 115, 22);
  uint16_t blue   = dma_display->color565(59, 130, 246);
  uint16_t pink   = dma_display->color565(236, 72, 153);

  int bs = 11, gap = 3, step = bs + gap;
  int totalW = bs * 3 + gap * 2;
  int sx = (64 - totalW) / 2;
  int sy = centered ? 18 : 10;  // 闹钟背景=18, 开机=10

  uint16_t grid[3][3] = {
    { orange, orange, blue },
    { orange, blue,   pink },
    { orange, orange, blue }
  };

  for (int row = 0; row < 3; row++)
    for (int col = 0; col < 3; col++)
      dma_display->fillRect(sx + col * step, sy + row * step, bs, bs, grid[row][col]);
}

void DisplayManager::displayClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // 时间未同步，显示 "--:--"
    dma_display->clearScreen();
    dma_display->setTextSize(1);
    dma_display->setCursor(17, 18);
    dma_display->setTextColor(dma_display->color565(100, 200, 255));
    dma_display->print("--:--");
    return;
  }
  
  dma_display->clearScreen();
  
  // 先绘制背景图片（如果有）
  Serial.printf("displayClock: image.show=%d, imagePixels=%p, imagePixelCount=%d\n", 
    ConfigManager::clockConfig.image.show, ConfigManager::imagePixels, ConfigManager::imagePixelCount);
  
  if (ConfigManager::clockConfig.image.show && ConfigManager::imagePixels != nullptr && ConfigManager::imagePixelCount > 0) {
    Serial.printf("开始绘制 %d 个像素\n", ConfigManager::imagePixelCount);
    // 绘制所有保存的像素
    for (int i = 0; i < ConfigManager::imagePixelCount; i++) {
      PixelData& pixel = ConfigManager::imagePixels[i];
      if (pixel.x < PANEL_RES_X && pixel.y < PANEL_RES_Y) {
        dma_display->drawPixelRGB888(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
      }
      // 每100个像素让出CPU
      if (i % 100 == 0) yield();
    }
    Serial.println("像素绘制完成");
  } else {
    // 没有背景图，画默认 logo 作为背景
    drawLogo();
  }
  
  // 显示时间 HH:MM
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  
  dma_display->setTextSize(ConfigManager::clockConfig.time.fontSize);
  dma_display->setCursor(ConfigManager::clockConfig.time.x, ConfigManager::clockConfig.time.y);
  dma_display->setTextColor(dma_display->color565(
    ConfigManager::clockConfig.time.r, 
    ConfigManager::clockConfig.time.g, 
    ConfigManager::clockConfig.time.b
  ));
  dma_display->print(timeStr);
  
  // 显示日期 MM-DD
  if (ConfigManager::clockConfig.date.show) {
    char dateStr[6];
    sprintf(dateStr, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    dma_display->setTextSize(ConfigManager::clockConfig.date.fontSize);
    dma_display->setCursor(ConfigManager::clockConfig.date.x, ConfigManager::clockConfig.date.y);
    dma_display->setTextColor(dma_display->color565(
      ConfigManager::clockConfig.date.r, 
      ConfigManager::clockConfig.date.g, 
      ConfigManager::clockConfig.date.b
    ));
    dma_display->print(dateStr);
  }
  
  // 显示星期
  if (ConfigManager::clockConfig.week.show) {
    const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    dma_display->setTextSize(1);
    dma_display->setCursor(ConfigManager::clockConfig.week.x, ConfigManager::clockConfig.week.y);
    dma_display->setTextColor(dma_display->color565(
      ConfigManager::clockConfig.week.r, 
      ConfigManager::clockConfig.week.g, 
      ConfigManager::clockConfig.week.b
    ));
    dma_display->print(weekDays[timeinfo.tm_wday]);
  }
}

void DisplayManager::drawClockOverlay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  // 显示时间 HH:MM
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  dma_display->setTextSize(ConfigManager::clockConfig.time.fontSize);
  dma_display->setCursor(ConfigManager::clockConfig.time.x, ConfigManager::clockConfig.time.y);
  dma_display->setTextColor(dma_display->color565(
    ConfigManager::clockConfig.time.r,
    ConfigManager::clockConfig.time.g,
    ConfigManager::clockConfig.time.b
  ));
  dma_display->print(timeStr);

  // 显示日期 MM-DD
  if (ConfigManager::clockConfig.date.show) {
    char dateStr[6];
    sprintf(dateStr, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    dma_display->setTextSize(ConfigManager::clockConfig.date.fontSize);
    dma_display->setCursor(ConfigManager::clockConfig.date.x, ConfigManager::clockConfig.date.y);
    dma_display->setTextColor(dma_display->color565(
      ConfigManager::clockConfig.date.r,
      ConfigManager::clockConfig.date.g,
      ConfigManager::clockConfig.date.b
    ));
    dma_display->print(dateStr);
  }

  // 显示星期
  if (ConfigManager::clockConfig.week.show) {
    const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    dma_display->setTextSize(1);
    dma_display->setCursor(ConfigManager::clockConfig.week.x, ConfigManager::clockConfig.week.y);
    dma_display->setTextColor(dma_display->color565(
      ConfigManager::clockConfig.week.r,
      ConfigManager::clockConfig.week.g,
      ConfigManager::clockConfig.week.b
    ));
    dma_display->print(weekDays[timeinfo.tm_wday]);
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

void DisplayManager::drawTinyText(const char* text, int x, int y, uint16_t color) {
  int cx = x;
  for (int i = 0; text[i]; i++) {
    int idx = fontIndex(text[i]);
    const uint8_t* glyph = FONT3X5[idx];
    for (int row = 0; row < 5; row++) {
      uint8_t bits = glyph[row];
      if (bits & 4) dma_display->drawPixel(cx,     y + row, color);
      if (bits & 2) dma_display->drawPixel(cx + 1, y + row, color);
      if (bits & 1) dma_display->drawPixel(cx + 2, y + row, color);
    }
    cx += 4;
  }
}

void DisplayManager::drawTinyTextCentered(const char* text, int y, uint16_t color) {
  int len = strlen(text);
  int width = len * 4 - 1;
  int x = (64 - width) / 2;
  if (x < 0) x = 0;
  drawTinyText(text, x, y, color);
}