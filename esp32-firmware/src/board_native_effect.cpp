#include "board_native_effect.h"

#include <math.h>
#include "display_manager.h"

namespace {
BoardNativeMode s_mode = BOARD_NATIVE_NONE;
bool s_active = false;
unsigned long s_lastTickAt = 0;
unsigned long s_animTick = 0;
unsigned long s_lastRenderAt = 0;

TextDisplayNativeConfig s_textConfig = {
  "status_card",
  "HELLO GLOWXEL",
  72,
  2,
  true,
  "weather",
  6,
  100,
  200,
  255,
  10,
  18,
  36
};

WeatherBoardNativeConfig s_weatherConfig = {
  "sunny",
  "SH",
  26,
  58,
  "c"
};

CountdownBoardNativeConfig s_countdownConfig = {
  0,
  10,
  0,
  60
};

StopwatchBoardNativeConfig s_stopwatchConfig = {
  45,
  3,
  true
};

NotificationBoardNativeConfig s_notificationConfig = {
  "daily",
  "DRINK WATER",
  "drink",
  100,
  200,
  255,
  "text",
  "headline",
  "badge",
  "pulse",
  9,
  0
};

int s_countdownTotalSeconds = 600;
int s_countdownRemainingSeconds = 600;
unsigned long s_countdownLastSecondAt = 0;
unsigned long s_stopwatchStartAt = 0;
unsigned long s_stopwatchElapsedMs = 0;

struct HourglassPoint {
  int x;
  int y;
};

const HourglassPoint kTopHourglass[] = {
  {18, 8},
  {46, 8},
  {43, 20},
  {32, 29},
  {21, 20},
};

const HourglassPoint kBottomHourglass[] = {
  {21, 35},
  {32, 31},
  {43, 35},
  {46, 48},
  {18, 48},
};

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return MatrixPanel_I2S_DMA::color565(r, g, b);
}

int resolveCountdownRemainingSeconds(const CountdownBoardNativeConfig& config) {
  int totalSeconds = (int)config.hours * 3600 + (int)config.minutes * 60 + (int)config.seconds;
  if (totalSeconds <= 0) {
    return 0;
  }
  return clampInt((totalSeconds * (int)config.progress + 50) / 100, 0, totalSeconds);
}

void resetCountdownStateFromConfig() {
  s_countdownTotalSeconds = (int)s_countdownConfig.hours * 3600 + (int)s_countdownConfig.minutes * 60 + (int)s_countdownConfig.seconds;
  s_countdownRemainingSeconds = resolveCountdownRemainingSeconds(s_countdownConfig);
  s_countdownLastSecondAt = millis();
}

bool pointInPolygon(float px, float py, const HourglassPoint* points, size_t count) {
  bool inside = false;
  for (size_t currentIndex = 0, previousIndex = count - 1; currentIndex < count; previousIndex = currentIndex, currentIndex += 1) {
    const HourglassPoint& current = points[currentIndex];
    const HourglassPoint& previous = points[previousIndex];
    bool intersect =
      ((current.y > py) != (previous.y > py)) &&
      (px < (float)(previous.x - current.x) * (py - (float)current.y) / ((float)previous.y - (float)current.y) + (float)current.x);
    if (intersect) {
      inside = !inside;
    }
  }
  return inside;
}

void drawPolygonOutline(MatrixPanel_I2S_DMA* display, const HourglassPoint* points, size_t count, uint16_t color) {
  for (size_t index = 0; index < count; index += 1) {
    const HourglassPoint& current = points[index];
    const HourglassPoint& next = points[(index + 1) % count];
    display->drawLine(current.x, current.y, next.x, next.y, color);
  }
}

String toSafeAscii(const String& text, size_t maxLength) {
  String result = "";
  result.reserve(maxLength);
  for (size_t i = 0; i < text.length() && result.length() < maxLength; i++) {
    char ch = text.charAt(i);
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == ' ') {
      if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 32);
      }
      result += ch;
    }
  }
  if (result.length() == 0) {
    return "N/A";
  }
  return result;
}

void drawWeatherIcon(const String& weatherType) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  int phase = (int)(s_animTick % 12);
  if (weatherType == "sunny") {
    display->fillCircle(13, 16, 5, color565(250, 191, 36));
    for (int i = 0; i < 8; i++) {
      int dx = (i % 2 == 0 ? 8 : 7);
      int x = 13 + (i < 4 ? dx : -dx);
      int y = 16 + ((i % 4 < 2) ? dx : -dx);
      display->drawPixel(x, y, color565(255, 220, 96));
    }
    return;
  }

  if (weatherType == "cloudy") {
    display->fillRoundRect(5, 12, 18, 8, 3, color565(180, 196, 214));
    display->fillRoundRect(9, 8, 12, 8, 3, color565(210, 220, 232));
    return;
  }

  if (weatherType == "rainy") {
    display->fillRoundRect(6, 9, 18, 8, 3, color565(180, 198, 220));
    for (int i = 0; i < 4; i++) {
      int y = 20 + ((phase + i * 3) % 8);
      display->drawPixel(8 + i * 4, y, color565(96, 170, 255));
    }
    return;
  }

  if (weatherType == "snow") {
    display->fillRoundRect(6, 9, 18, 8, 3, color565(196, 212, 232));
    for (int i = 0; i < 3; i++) {
      int cx = 10 + i * 5;
      int cy = 21 + ((phase + i * 2) % 5);
      display->drawPixel(cx, cy, color565(240, 248, 255));
      display->drawPixel(cx - 1, cy, color565(240, 248, 255));
      display->drawPixel(cx + 1, cy, color565(240, 248, 255));
      display->drawPixel(cx, cy - 1, color565(240, 248, 255));
      display->drawPixel(cx, cy + 1, color565(240, 248, 255));
    }
    return;
  }

  display->fillRoundRect(6, 9, 18, 8, 3, color565(170, 184, 206));
  int boltShift = phase % 2;
  display->fillRect(12, 18 + boltShift, 2, 4, color565(255, 220, 64));
  display->fillRect(10, 21 + boltShift, 2, 4, color565(255, 220, 64));
}

void drawNotificationIcon(const String& icon) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  uint16_t accent = color565(s_notificationConfig.accentR, s_notificationConfig.accentG, s_notificationConfig.accentB);
  if (icon == "drink") {
    display->drawRect(6, 10, 8, 12, accent);
    display->fillRect(7, 14, 6, 7, accent);
    display->drawPixel(14, 13, accent);
    display->drawPixel(15, 12, accent);
    return;
  }
  if (icon == "moon") {
    display->fillCircle(10, 16, 6, accent);
    display->fillCircle(13, 14, 5, color565(8, 14, 24));
    return;
  }
  if (icon == "fireworks") {
    int pulse = (int)(s_animTick % 3);
    display->drawPixel(10, 16, accent);
    display->drawPixel(10 - pulse, 16, accent);
    display->drawPixel(10 + pulse, 16, accent);
    display->drawPixel(10, 16 - pulse, accent);
    display->drawPixel(10, 16 + pulse, accent);
    display->drawPixel(10 - pulse, 16 - pulse, accent);
    display->drawPixel(10 + pulse, 16 + pulse, accent);
    return;
  }
  display->fillRect(7, 12, 7, 7, accent);
}

void drawTextDisplay() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  display->fillScreen(color565(s_textConfig.bgColorR, s_textConfig.bgColorG, s_textConfig.bgColorB));

  int iconOffset = 0;
  if (s_textConfig.pushIcon) {
    int cycle = clampInt((int)s_textConfig.speed, 1, 10);
    iconOffset = ((int)s_animTick / (12 - cycle)) % 3;
  }
  display->fillRect(4 + iconOffset, 6, 8, 8, color565(s_textConfig.colorR, s_textConfig.colorG, s_textConfig.colorB));
  display->fillRect(6 + iconOffset, 8, 4, 4, color565(8, 14, 24));

  String text = toSafeAscii(s_textConfig.text, 16);
  int textX = 16 - ((int)(s_animTick % 64) / (12 - clampInt((int)s_textConfig.speed, 1, 10)));
  if (textX < -60) {
    textX = 16;
  }
  DisplayManager::drawTinyText(text.c_str(), textX, 8, color565(s_textConfig.colorR, s_textConfig.colorG, s_textConfig.colorB), 1);

  int progress = clampInt((int)s_textConfig.progress, 0, 100);
  int width = (progress * 56) / 100;
  display->drawRect(4, 56, 56, 4, color565(48, 68, 96));
  display->fillRect(4, 56, width, 4, color565(s_textConfig.colorR, s_textConfig.colorG, s_textConfig.colorB));
}

void drawWeatherBoard() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  display->fillScreen(color565(8, 14, 24));

  drawWeatherIcon(s_weatherConfig.weatherType);

  char tempText[20];
  snprintf(tempText, sizeof(tempText), "%d%c", (int)s_weatherConfig.temperature, s_weatherConfig.unit == "f" ? 'F' : 'C');
  DisplayManager::drawTinyText(tempText, 28, 10, color565(132, 212, 255), 2);

  char humText[20];
  snprintf(humText, sizeof(humText), "H%u%%", (unsigned int)s_weatherConfig.humidity);
  DisplayManager::drawTinyText(humText, 28, 28, color565(125, 223, 138), 1);

  String city = toSafeAscii(s_weatherConfig.city, 8);
  DisplayManager::drawTinyText(city.c_str(), 4, 54, color565(188, 204, 225), 1);
}

void drawCountdownBoard() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  uint16_t background = color565(7, 12, 20);
  uint16_t capColor = color565(123, 143, 187);
  uint16_t capHighlight = color565(220, 233, 255);
  uint16_t outlineColor = color565(196, 210, 244);
  uint16_t outlineSoft = color565(123, 143, 187);
  uint16_t sandColor = color565(255, 207, 74);
  uint16_t sandHighlight = color565(255, 241, 166);
  uint16_t sandShadow = color565(244, 169, 53);
  uint16_t textColor = color565(255, 231, 164);
  display->fillScreen(background);

  display->fillRect(22, 4, 20, 2, capColor);
  display->fillRect(20, 5, 24, 1, capHighlight);
  display->fillRect(22, 50, 20, 2, capColor);
  display->fillRect(20, 52, 24, 1, capHighlight);

  float ratio = (float)clampInt((int)s_countdownConfig.progress, 0, 100) / 100.0f;
  if (s_countdownTotalSeconds > 0) {
    ratio = (float)s_countdownRemainingSeconds / (float)s_countdownTotalSeconds;
  }
  if (ratio < 0.0f) {
    ratio = 0.0f;
  }
  if (ratio > 1.0f) {
    ratio = 1.0f;
  }

  int topMinY = 8;
  int topMaxY = 29;
  int bottomMinY = 31;
  int bottomMaxY = 48;
  int topCutoff = ratio <= 0.0f
    ? topMaxY + 1
    : (int)floorf((float)topMinY + (1.0f - ratio) * (float)(topMaxY - topMinY + 1));
  float bottomRatio = 1.0f - ratio;
  int bottomCutoff = bottomRatio <= 0.0f
    ? bottomMaxY + 1
    : (int)floorf((float)bottomMaxY - bottomRatio * (float)(bottomMaxY - bottomMinY + 1) + 1.0f);

  for (int y = topMinY; y <= topMaxY; y += 1) {
    for (int x = 17; x <= 47; x += 1) {
      if (!pointInPolygon((float)x + 0.5f, (float)y + 0.5f, kTopHourglass, sizeof(kTopHourglass) / sizeof(kTopHourglass[0]))) {
        continue;
      }
      if (y < topCutoff) {
        continue;
      }
      bool sparkle = ((x * 7 + y * 11 + (int)s_animTick * 3) % 19) == 0;
      bool isSurface = y == topCutoff;
      display->drawPixel(x, y, isSurface || sparkle ? sandHighlight : sandColor);
    }
  }

  for (int y = bottomMinY; y <= bottomMaxY; y += 1) {
    for (int x = 17; x <= 47; x += 1) {
      if (!pointInPolygon((float)x + 0.5f, (float)y + 0.5f, kBottomHourglass, sizeof(kBottomHourglass) / sizeof(kBottomHourglass[0]))) {
        continue;
      }
      if (y < bottomCutoff) {
        continue;
      }
      bool sparkle = ((x * 5 + y * 13 + (int)s_animTick * 4) % 17) == 0;
      bool isSurface = y == bottomCutoff;
      display->drawPixel(x, y, isSurface || sparkle ? sandHighlight : sandShadow);
    }
  }

  if (s_countdownRemainingSeconds > 0 && ratio > 0.0f) {
    int streamPhase = (int)(s_animTick % 12);
    for (int grain = 0; grain < 6; grain += 1) {
      int y = 30 + ((streamPhase + grain * 2) % 10);
      int x = 31 + ((streamPhase + grain) % 2);
      display->drawPixel(x, y, grain % 2 == 0 ? sandHighlight : sandColor);
    }
    int splashPhase = (int)(s_animTick % 8);
    display->drawPixel(30 + (splashPhase % 3), 39 + splashPhase / 3, sandHighlight);
  }

  drawPolygonOutline(display, kTopHourglass, sizeof(kTopHourglass) / sizeof(kTopHourglass[0]), outlineColor);
  drawPolygonOutline(display, kBottomHourglass, sizeof(kBottomHourglass) / sizeof(kBottomHourglass[0]), outlineColor);
  display->drawLine(31, 29, 31, 31, outlineSoft);
  display->drawLine(33, 29, 33, 31, outlineSoft);

  int total = s_countdownRemainingSeconds;
  int hh = total / 3600;
  int mm = (total % 3600) / 60;
  int ss = total % 60;
  char timeText[24];
  if (s_countdownConfig.hours > 0) {
    snprintf(timeText, sizeof(timeText), "%02d:%02d:%02d", hh, mm, ss);
    DisplayManager::drawTinyTextCentered(timeText, 55, textColor, 1);
    return;
  }

  snprintf(timeText, sizeof(timeText), "%02d:%02d", mm, ss);
  DisplayManager::drawTinyTextCentered(timeText, 53, textColor, 2);
}

void drawStopwatchBoard() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  display->fillScreen(color565(9, 12, 20));

  unsigned long totalMs = s_stopwatchElapsedMs;
  unsigned long totalSeconds = totalMs / 1000UL;
  unsigned long mm = (totalSeconds / 60UL) % 100UL;
  unsigned long ss = totalSeconds % 60UL;
  unsigned long cs = (totalMs % 1000UL) / 10UL;

  char text[32];
  if (s_stopwatchConfig.showMilliseconds) {
    snprintf(text, sizeof(text), "%02lu:%02lu.%02lu", mm, ss, cs);
  } else {
    snprintf(text, sizeof(text), "%02lu:%02lu", mm, ss);
  }
  DisplayManager::drawTinyText(text, 4, 18, color565(132, 212, 255), 2);

  char lapText[24];
  snprintf(lapText, sizeof(lapText), "LAP %u", (unsigned int)s_stopwatchConfig.lapCount);
  DisplayManager::drawTinyText(lapText, 4, 52, color565(166, 184, 206), 1);
}

void drawNotificationBoard() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  display->fillScreen(color565(8, 12, 20));

  uint16_t accent = color565(s_notificationConfig.accentR, s_notificationConfig.accentG, s_notificationConfig.accentB);
  display->fillRect(0, 0, 64, 3, accent);
  drawNotificationIcon(s_notificationConfig.icon);

  String text = toSafeAscii(s_notificationConfig.text, 18);
  DisplayManager::drawTinyText(text.c_str(), 18, 12, color565(236, 244, 255), 1);

  String repeatMode = toSafeAscii(s_notificationConfig.repeatMode, 8);
  DisplayManager::drawTinyText(repeatMode.c_str(), 18, 24, color565(156, 172, 194), 1);

  char timeText[16];
  snprintf(timeText, sizeof(timeText), "%02u:%02u", (unsigned int)s_notificationConfig.hour, (unsigned int)s_notificationConfig.minute);
  DisplayManager::drawTinyText(timeText, 18, 36, accent, 2);

  int pulse = (int)(2.0f + 2.0f * sinf((float)s_animTick * 0.2f));
  display->drawRect(2 + pulse, 50, 60 - pulse * 2, 10, accent);
}

void setActiveMode(BoardNativeMode mode) {
  s_mode = mode;
  s_active = true;
  s_lastTickAt = millis();
  s_lastRenderAt = 0;
  s_animTick = 0;
}
}

namespace BoardNativeEffect {
void init() {
  s_mode = BOARD_NATIVE_NONE;
  s_active = false;
  s_lastTickAt = millis();
  s_animTick = 0;
  resetCountdownStateFromConfig();
  s_stopwatchStartAt = millis();
  s_stopwatchElapsedMs = (unsigned long)s_stopwatchConfig.previewSeconds * 1000UL;
}

void deactivate() {
  s_active = false;
  s_mode = BOARD_NATIVE_NONE;
}

bool isActive() {
  return s_active;
}

BoardNativeMode getMode() {
  return s_mode;
}

void applyTextDisplayConfig(const TextDisplayNativeConfig& config) {
  s_textConfig = config;
  setActiveMode(BOARD_NATIVE_TEXT_DISPLAY);
}

void applyWeatherConfig(const WeatherBoardNativeConfig& config) {
  s_weatherConfig = config;
  setActiveMode(BOARD_NATIVE_WEATHER);
}

void applyCountdownConfig(const CountdownBoardNativeConfig& config) {
  s_countdownConfig = config;
  resetCountdownStateFromConfig();
  setActiveMode(BOARD_NATIVE_COUNTDOWN);
}

void applyStopwatchConfig(const StopwatchBoardNativeConfig& config) {
  s_stopwatchConfig = config;
  s_stopwatchStartAt = millis();
  s_stopwatchElapsedMs = (unsigned long)s_stopwatchConfig.previewSeconds * 1000UL;
  setActiveMode(BOARD_NATIVE_STOPWATCH);
}

void applyNotificationConfig(const NotificationBoardNativeConfig& config) {
  s_notificationConfig = config;
  setActiveMode(BOARD_NATIVE_NOTIFICATION);
}

void update() {
  if (!s_active) {
    return;
  }

  unsigned long now = millis();
  if (s_lastTickAt == 0 || now - s_lastTickAt >= 33) {
    s_lastTickAt = now;
    s_animTick += 1;
  }

  if (s_mode == BOARD_NATIVE_COUNTDOWN) {
    if (now - s_countdownLastSecondAt >= 1000) {
      s_countdownLastSecondAt = now;
      if (s_countdownRemainingSeconds > 0) {
        s_countdownRemainingSeconds -= 1;
      }
    }
    return;
  }

  if (s_mode == BOARD_NATIVE_STOPWATCH) {
    unsigned long base = (unsigned long)s_stopwatchConfig.previewSeconds * 1000UL;
    s_stopwatchElapsedMs = base + (now - s_stopwatchStartAt);
  }
}

void render() {
  if (!s_active || DisplayManager::dma_display == nullptr) {
    return;
  }

  unsigned long now = millis();
  if (s_lastRenderAt != 0 && now - s_lastRenderAt < 33) {
    return;
  }
  s_lastRenderAt = now;

  if (s_mode == BOARD_NATIVE_TEXT_DISPLAY) {
    drawTextDisplay();
    return;
  }
  if (s_mode == BOARD_NATIVE_WEATHER) {
    drawWeatherBoard();
    return;
  }
  if (s_mode == BOARD_NATIVE_COUNTDOWN) {
    drawCountdownBoard();
    return;
  }
  if (s_mode == BOARD_NATIVE_STOPWATCH) {
    drawStopwatchBoard();
    return;
  }
  if (s_mode == BOARD_NATIVE_NOTIFICATION) {
    drawNotificationBoard();
  }
}

const TextDisplayNativeConfig& getTextDisplayConfig() {
  return s_textConfig;
}

const WeatherBoardNativeConfig& getWeatherConfig() {
  return s_weatherConfig;
}

const CountdownBoardNativeConfig& getCountdownConfig() {
  return s_countdownConfig;
}

const StopwatchBoardNativeConfig& getStopwatchConfig() {
  return s_stopwatchConfig;
}

const NotificationBoardNativeConfig& getNotificationConfig() {
  return s_notificationConfig;
}

int getCountdownRemainingSeconds() {
  return s_countdownRemainingSeconds;
}

unsigned long getStopwatchElapsedMs() {
  return s_stopwatchElapsedMs;
}
}
