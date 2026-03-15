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
  dma_display->setBrightness8(51); // 默认亮度约20% (51/255)
  dma_display->clearScreen();
  
  Serial.println("显示开机Logo...");

  // Glowxel logo 颜色
  uint16_t orange = dma_display->color565(249, 115, 22);   // #F97316
  uint16_t blue   = dma_display->color565(59, 130, 246);    // #3B82F6
  uint16_t pink   = dma_display->color565(236, 72, 153);    // #EC4899

  // 3x3 方块网格 logo，每块 8x8，间距 2px
  // 居中：总宽 = 8*3 + 2*2 = 28，起始x = (64-28)/2 = 18
  // 上半部分：起始y = 10
  int bs = 8;   // block size
  int gap = 2;  // gap between blocks
  int sx = 18;  // start x
  int sy = 10;  // start y
  int step = bs + gap; // 10

  // 3x3 网格颜色映射 (行优先)
  // [橙, 橙, 蓝]
  // [橙, 蓝, 粉]
  // [橙, 橙, 蓝]
  uint16_t grid[3][3] = {
    { orange, orange, blue },
    { orange, blue,   pink },
    { orange, orange, blue }
  };

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      dma_display->fillRect(sx + col * step, sy + row * step, bs, bs, grid[row][col]);
    }
  }

  // "Glowxel" 文字居中显示在下半部分
  // 内置字体每字符 6px 宽，7 个字符 = 42px，起始x = (64-42)/2 = 11
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(220, 220, 220)); // 浅白色
  dma_display->setCursor(11, 46);
  dma_display->print("Glowxel");

  delay(2000);
  Serial.println("LED灯板初始化完成");
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
    Serial.println("不绘制像素画");
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