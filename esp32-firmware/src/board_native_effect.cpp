#include "board_native_effect.h"

#include <math.h>
#include <string.h>
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

PlanetScreensaverNativeConfig s_planetConfig = {
  "gas_giant_1",
  "medium",
  "right",
  3,
  20260415UL,
  20260415UL
};

int s_countdownTotalSeconds = 600;
int s_countdownRemainingSeconds = 600;
unsigned long s_countdownLastSecondAt = 0;
unsigned long s_countdownLastPhysicsAt = 0;
unsigned long s_stopwatchStartAt = 0;
unsigned long s_stopwatchElapsedMs = 0;
float s_countdownReleaseAccumulator = 0.0f;
float s_countdownReleasePerMs = 0.0f;
uint32_t s_countdownRandomState = 0x9e3779b9UL;
float s_planetPhase = 0.0f;
bool s_planetDirty = false;

struct PlanetRgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

PlanetRgb s_planetTintColor = { 210, 184, 168 };
float s_planetTintMix = 0.12f;
float s_planetHueShift = 0.0f;
float s_planetSaturationScale = 1.0f;
float s_planetTintBrightness = 1.0f;

struct HourglassPoint {
  int x;
  int y;
};

struct CountdownParticle {
  int x;
  int y;
  bool active;
};

struct CountdownSplash {
  int x;
  int y;
  uint8_t ttl;
  bool active;
};

constexpr int kCountdownSandStackDepth = 6;
constexpr float kCountdownSandFillRatio = 0.84f;
constexpr int kCountdownMaxParticles = 6;
constexpr int kCountdownMaxSplashes = 12;
constexpr unsigned long kCountdownPhysicsStepMs = 55;
constexpr unsigned long kCountdownTailSettleMs = 450;
constexpr int kCountdownTopMinX = 20;
constexpr int kCountdownTopMaxX = 44;
constexpr int kCountdownTopMinY = 8;
constexpr int kCountdownTopMaxY = 28;
constexpr int kCountdownBottomMinX = 20;
constexpr int kCountdownBottomMaxX = 44;
constexpr int kCountdownBottomMinY = 31;
constexpr int kCountdownBottomMaxY = 47;
constexpr int kCountdownNeckX = 32;
constexpr int kCountdownNeckY = 29;
constexpr int kCountdownStreamEntryY = 30;
constexpr int kCountdownTopWidth = kCountdownTopMaxX - kCountdownTopMinX + 1;
constexpr int kCountdownTopHeight = kCountdownTopMaxY - kCountdownTopMinY + 1;
constexpr int kCountdownTopArea = kCountdownTopWidth * kCountdownTopHeight;
constexpr int kCountdownBottomWidth = kCountdownBottomMaxX - kCountdownBottomMinX + 1;
constexpr int kCountdownBottomHeight = kCountdownBottomMaxY - kCountdownBottomMinY + 1;
constexpr int kCountdownBottomArea = kCountdownBottomWidth * kCountdownBottomHeight;

struct CountdownRegion {
  int minX;
  int maxX;
  int minY;
  int maxY;
  int width;
  int height;
};

constexpr CountdownRegion kCountdownTopRegion = {
  kCountdownTopMinX,
  kCountdownTopMaxX,
  kCountdownTopMinY,
  kCountdownTopMaxY,
  kCountdownTopWidth,
  kCountdownTopHeight
};

constexpr CountdownRegion kCountdownBottomRegion = {
  kCountdownBottomMinX,
  kCountdownBottomMaxX,
  kCountdownBottomMinY,
  kCountdownBottomMaxY,
  kCountdownBottomWidth,
  kCountdownBottomHeight
};

uint8_t s_countdownTopMask[kCountdownTopArea] = {0};
uint8_t s_countdownBottomMask[kCountdownBottomArea] = {0};
uint8_t s_countdownTopDepth[kCountdownTopArea] = {0};
uint8_t s_countdownBottomDepth[kCountdownBottomArea] = {0};
int s_countdownTopCellCount = 0;
int s_countdownBottomCellCount = 0;
bool s_countdownGeometryReady = false;
CountdownParticle s_countdownParticles[kCountdownMaxParticles] = {};
CountdownSplash s_countdownSplashes[kCountdownMaxSplashes] = {};

const HourglassPoint kTopHourglass[] = {
  {20, 8},
  {44, 8},
  {32, 28},
};

const HourglassPoint kBottomHourglass[] = {
  {32, 31},
  {20, 47},
  {44, 47},
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

float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

float fractFloat(float value) {
  return value - floorf(value);
}

float wrapPlanetUnit(float value) {
  float wrapped = fmodf(value, 1.0f);
  if (wrapped < 0.0f) {
    wrapped += 1.0f;
  }
  return wrapped;
}

float mixFloat(float left, float right, float amount) {
  return left * (1.0f - amount) + right * amount;
}

float smoothstepFloat(float edge0, float edge1, float value) {
  if (edge0 == edge1) {
    return value < edge0 ? 0.0f : 1.0f;
  }
  float t = clampFloat((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

PlanetRgb makePlanetRgb(uint8_t r, uint8_t g, uint8_t b) {
  PlanetRgb color = { r, g, b };
  return color;
}

PlanetRgb mixPlanetRgb(const PlanetRgb& left, const PlanetRgb& right, float amount) {
  float t = clampFloat(amount, 0.0f, 1.0f);
  return makePlanetRgb(
    (uint8_t)roundf(mixFloat((float)left.r, (float)right.r, t)),
    (uint8_t)roundf(mixFloat((float)left.g, (float)right.g, t)),
    (uint8_t)roundf(mixFloat((float)left.b, (float)right.b, t))
  );
}

PlanetRgb scalePlanetRgb(const PlanetRgb& color, float factor) {
  float clamped = clampFloat(factor, 0.0f, 2.0f);
  return makePlanetRgb(
    (uint8_t)roundf(clampFloat((float)color.r * clamped, 0.0f, 255.0f)),
    (uint8_t)roundf(clampFloat((float)color.g * clamped, 0.0f, 255.0f)),
    (uint8_t)roundf(clampFloat((float)color.b * clamped, 0.0f, 255.0f))
  );
}

void rgbToPlanetHsv(const PlanetRgb& color, float& hue, float& saturation, float& value) {
  float red = (float)color.r / 255.0f;
  float green = (float)color.g / 255.0f;
  float blue = (float)color.b / 255.0f;
  float maxValue = fmaxf(red, fmaxf(green, blue));
  float minValue = fminf(red, fminf(green, blue));
  float delta = maxValue - minValue;

  hue = 0.0f;
  if (delta > 0.0f) {
    if (maxValue == red) {
      hue = wrapPlanetUnit(((green - blue) / delta) / 6.0f);
    } else if (maxValue == green) {
      hue = (((blue - red) / delta) + 2.0f) / 6.0f;
    } else {
      hue = (((red - green) / delta) + 4.0f) / 6.0f;
    }
  }

  saturation = maxValue <= 0.0f ? 0.0f : delta / maxValue;
  value = maxValue;
}

PlanetRgb hsvToPlanetRgb(float hue, float saturation, float value) {
  float normalizedHue = wrapPlanetUnit(hue);
  float clampedSaturation = clampFloat(saturation, 0.0f, 1.0f);
  float clampedValue = clampFloat(value, 0.0f, 1.0f);
  float segment = normalizedHue * 6.0f;
  float chroma = clampedValue * clampedSaturation;
  float xValue = chroma * (1.0f - fabsf(fmodf(segment, 2.0f) - 1.0f));
  float match = clampedValue - chroma;
  float red = 0.0f;
  float green = 0.0f;
  float blue = 0.0f;

  if (segment < 1.0f) {
    red = chroma;
    green = xValue;
  } else if (segment < 2.0f) {
    red = xValue;
    green = chroma;
  } else if (segment < 3.0f) {
    green = chroma;
    blue = xValue;
  } else if (segment < 4.0f) {
    green = xValue;
    blue = chroma;
  } else if (segment < 5.0f) {
    red = chroma;
    blue = xValue;
  } else {
    red = chroma;
    blue = xValue;
  }

  return makePlanetRgb(
    (uint8_t)roundf(clampFloat((red + match) * 255.0f, 0.0f, 255.0f)),
    (uint8_t)roundf(clampFloat((green + match) * 255.0f, 0.0f, 255.0f)),
    (uint8_t)roundf(clampFloat((blue + match) * 255.0f, 0.0f, 255.0f))
  );
}

PlanetRgb brightenPlanetRgb(const PlanetRgb& color, float amount) {
  return mixPlanetRgb(color, makePlanetRgb(255, 255, 255), clampFloat(amount, 0.0f, 1.0f));
}

PlanetRgb darkenPlanetRgb(const PlanetRgb& color, float amount) {
  return mixPlanetRgb(color, makePlanetRgb(0, 0, 0), clampFloat(amount, 0.0f, 1.0f));
}

uint16_t planetColor565(const PlanetRgb& color) {
  return color565(color.r, color.g, color.b);
}

uint32_t hashPlanet(uint32_t value) {
  value ^= value >> 16;
  value *= 0x7feb352dUL;
  value ^= value >> 15;
  value *= 0x846ca68bUL;
  value ^= value >> 16;
  return value;
}

uint32_t hashPlanetString(const char* value) {
  uint32_t hash = 2166136261UL;
  if (value == nullptr) {
    return hash;
  }
  while (*value != '\0') {
    hash ^= (uint8_t)(*value);
    hash *= 16777619UL;
    value += 1;
  }
  return hash;
}

float nextPlanetSeededRandom(uint32_t& state) {
  state = state * 1664525UL + 1013904223UL;
  return (float)state / 4294967296.0f;
}

float seededPlanetRange(uint32_t seed, const char* key, float minValue, float maxValue);
float seededPlanetRange(uint32_t seed, uint32_t salt, float minValue, float maxValue);

float hashPlanetFloat(int x, int y, uint32_t seed) {
  uint32_t value = seed;
  value ^= (uint32_t)(x * 374761393);
  value ^= (uint32_t)(y * 668265263);
  return (float)(hashPlanet(value) & 0x00ffffffUL) / 16777215.0f;
}

float planetVariantUnit(uint32_t seed, uint32_t salt) {
  uint32_t state = seed ^ salt;
  state = state * 1664525UL + 1013904223UL;
  return (float)(state & 0x00ffffffUL) / 16777215.0f;
}

void refreshPlanetColorVariant() {
  uint32_t seed = s_planetConfig.colorSeed;
  char key[48];
  snprintf(key, sizeof(key), "%s_color_variant_hue", s_planetConfig.preset);
  s_planetHueShift = seededPlanetRange(seed, key, -0.32f, 0.32f);
  snprintf(key, sizeof(key), "%s_color_variant_sat", s_planetConfig.preset);
  s_planetSaturationScale = seededPlanetRange(seed, key, 0.68f, 1.42f);
  snprintf(key, sizeof(key), "%s_color_variant_val", s_planetConfig.preset);
  s_planetTintBrightness = seededPlanetRange(seed, key, 0.78f, 1.26f);
  snprintf(key, sizeof(key), "%s_color_variant_r", s_planetConfig.preset);
  float tintR = seededPlanetRange(seed, key, 0.2f, 1.0f);
  snprintf(key, sizeof(key), "%s_color_variant_g", s_planetConfig.preset);
  float tintG = seededPlanetRange(seed, key, 0.2f, 1.0f);
  snprintf(key, sizeof(key), "%s_color_variant_b", s_planetConfig.preset);
  float tintB = seededPlanetRange(seed, key, 0.2f, 1.0f);
  s_planetTintColor = makePlanetRgb(
    (uint8_t)roundf(tintR * 255.0f),
    (uint8_t)roundf(tintG * 255.0f),
    (uint8_t)roundf(tintB * 255.0f)
  );
  snprintf(key, sizeof(key), "%s_color_variant_mix", s_planetConfig.preset);
  s_planetTintMix = seededPlanetRange(seed, key, 0.04f, 0.26f);
}

float noisePlanet(float x, float y, uint32_t seed) {
  int xi = (int)floorf(x);
  int yi = (int)floorf(y);
  float tx = fractFloat(x);
  float ty = fractFloat(y);
  float u = smoothstepFloat(0.0f, 1.0f, tx);
  float v = smoothstepFloat(0.0f, 1.0f, ty);

  float n00 = hashPlanetFloat(xi, yi, seed);
  float n10 = hashPlanetFloat(xi + 1, yi, seed);
  float n01 = hashPlanetFloat(xi, yi + 1, seed);
  float n11 = hashPlanetFloat(xi + 1, yi + 1, seed);

  float nx0 = mixFloat(n00, n10, u);
  float nx1 = mixFloat(n01, n11, u);
  return mixFloat(nx0, nx1, v);
}

float fbmPlanet(float x, float y, uint32_t seed, int octaves) {
  float value = 0.0f;
  float amplitude = 0.5f;
  float frequency = 1.0f;
  for (int index = 0; index < octaves; index += 1) {
    value += noisePlanet(x * frequency, y * frequency, seed + (uint32_t)index * 97UL) * amplitude;
    frequency *= 2.0f;
    amplitude *= 0.5f;
  }
  return value;
}

float resolvePlanetSizeScale(const char* size) {
  if (strcmp(size, "small") == 0) {
    return 0.82f;
  }
  if (strcmp(size, "large") == 0) {
    return 1.15f;
  }
  return 1.0f;
}

float resolvePlanetDirectionFactor(const char* direction) {
  if (strcmp(direction, "left") == 0) {
    return -1.0f;
  }
  return 1.0f;
}

unsigned long resolvePlanetFrameDelay(uint8_t speed) {
  static const unsigned long kPlanetDelayBySpeed[10] = {
    840UL, 720UL, 620UL, 530UL, 450UL, 380UL, 320UL, 270UL, 220UL, 180UL
  };
  int index = clampInt((int)speed, 1, 7) - 1;
  return kPlanetDelayBySpeed[index];
}

unsigned long resolvePlanetTickMs() {
  return 100UL;
}

float resolvePlanetPhaseStep(uint8_t speed) {
  unsigned long frameDelay = resolvePlanetFrameDelay(speed);
  return (float)resolvePlanetTickMs() / (float)frameDelay;
}

float resolvePlanetFlow() {
  return s_planetPhase * 0.11f;
}

void drawPlanetPixel(MatrixPanel_I2S_DMA* display, int x, int y, const PlanetRgb& color) {
  float hue = 0.0f;
  float saturation = 0.0f;
  float value = 0.0f;
  rgbToPlanetHsv(color, hue, saturation, value);
  PlanetRgb remapped = hsvToPlanetRgb(
    hue + s_planetHueShift,
    clampFloat(saturation * s_planetSaturationScale, 0.0f, 1.0f),
    clampFloat(value * s_planetTintBrightness, 0.0f, 1.0f)
  );
  PlanetRgb mixed = mixPlanetRgb(remapped, s_planetTintColor, s_planetTintMix);
  display->drawPixel(x, y, planetColor565(mixed));
}

int countdownRegionIndex(const CountdownRegion& region, int x, int y) {
  if (x < region.minX || x > region.maxX || y < region.minY || y > region.maxY) {
    return -1;
  }
  return (y - region.minY) * region.width + (x - region.minX);
}

uint32_t nextCountdownRandom() {
  s_countdownRandomState = s_countdownRandomState * 1664525UL + 1013904223UL;
  return s_countdownRandomState;
}

bool countdownRandomBool() {
  return (nextCountdownRandom() & 1UL) == 0;
}

int resolveCountdownRemainingSeconds(const CountdownBoardNativeConfig& config) {
  int totalSeconds = (int)config.hours * 3600 + (int)config.minutes * 60 + (int)config.seconds;
  if (totalSeconds <= 0) {
    return 0;
  }
  return clampInt((totalSeconds * (int)config.progress + 50) / 100, 0, totalSeconds);
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

void clearCountdownParticles() {
  for (int index = 0; index < kCountdownMaxParticles; index += 1) {
    s_countdownParticles[index].active = false;
  }
}

void clearCountdownSplashes() {
  for (int index = 0; index < kCountdownMaxSplashes; index += 1) {
    s_countdownSplashes[index].active = false;
    s_countdownSplashes[index].ttl = 0;
  }
}

void buildCountdownMask(
  const HourglassPoint* polygon,
  size_t polygonCount,
  const CountdownRegion& region,
  uint8_t* mask,
  int& cellCount
) {
  memset(mask, 0, (size_t)region.width * (size_t)region.height);
  cellCount = 0;
  for (int y = region.minY; y <= region.maxY; y += 1) {
    for (int x = region.minX; x <= region.maxX; x += 1) {
      if (!pointInPolygon((float)x + 0.5f, (float)y + 0.5f, polygon, polygonCount)) {
        continue;
      }
      int index = countdownRegionIndex(region, x, y);
      mask[index] = 1;
      cellCount += 1;
    }
  }
}

void ensureCountdownGeometry() {
  if (s_countdownGeometryReady) {
    return;
  }
  buildCountdownMask(
    kTopHourglass,
    sizeof(kTopHourglass) / sizeof(kTopHourglass[0]),
    kCountdownTopRegion,
    s_countdownTopMask,
    s_countdownTopCellCount
  );
  buildCountdownMask(
    kBottomHourglass,
    sizeof(kBottomHourglass) / sizeof(kBottomHourglass[0]),
    kCountdownBottomRegion,
    s_countdownBottomMask,
    s_countdownBottomCellCount
  );
  s_countdownGeometryReady = true;
}

bool countdownHasSand(const uint8_t* depthGrid, const CountdownRegion& region, int x, int y) {
  int index = countdownRegionIndex(region, x, y);
  if (index < 0) {
    return false;
  }
  return depthGrid[index] > 0;
}

bool countdownCanReceive(const uint8_t* mask, const uint8_t* depthGrid, const CountdownRegion& region, int x, int y) {
  int index = countdownRegionIndex(region, x, y);
  if (index < 0) {
    return false;
  }
  if (mask[index] != 1) {
    return false;
  }
  return depthGrid[index] < kCountdownSandStackDepth;
}

void countdownMoveDepth(uint8_t* depthGrid, const CountdownRegion& region, int fromX, int fromY, int toX, int toY) {
  int fromIndex = countdownRegionIndex(region, fromX, fromY);
  int toIndex = countdownRegionIndex(region, toX, toY);
  if (fromIndex < 0 || toIndex < 0) {
    return;
  }
  if (depthGrid[fromIndex] == 0) {
    return;
  }
  depthGrid[fromIndex] -= 1;
  depthGrid[toIndex] += 1;
}

void countdownSettleOne(uint8_t* depthGrid, const uint8_t* mask, const CountdownRegion& region, int x, int y) {
  int currentIndex = countdownRegionIndex(region, x, y);
  if (currentIndex < 0) {
    return;
  }
  if (depthGrid[currentIndex] == 0) {
    return;
  }

  int midX = x;
  int midY = y + 1;
  int leftX = x - 1;
  int leftY = y + 1;
  int rightX = x + 1;
  int rightY = y + 1;

  bool midOpen = countdownCanReceive(mask, depthGrid, region, midX, midY);
  bool leftOpen = countdownCanReceive(mask, depthGrid, region, leftX, leftY);
  bool rightOpen = countdownCanReceive(mask, depthGrid, region, rightX, rightY);

  if (midOpen) {
    countdownMoveDepth(depthGrid, region, x, y, midX, midY);
    return;
  }
  if (leftOpen && !rightOpen) {
    countdownMoveDepth(depthGrid, region, x, y, leftX, leftY);
    return;
  }
  if (!leftOpen && rightOpen) {
    countdownMoveDepth(depthGrid, region, x, y, rightX, rightY);
    return;
  }
  if (leftOpen && rightOpen) {
    if (countdownRandomBool()) {
      countdownMoveDepth(depthGrid, region, x, y, leftX, leftY);
      return;
    }
    countdownMoveDepth(depthGrid, region, x, y, rightX, rightY);
  }
}

void countdownSettleChamber(uint8_t* depthGrid, const uint8_t* mask, const CountdownRegion& region) {
  for (int y = region.maxY - 1; y >= region.minY; y -= 1) {
    if (countdownRandomBool()) {
      for (int x = region.minX; x <= region.maxX; x += 1) {
        countdownSettleOne(depthGrid, mask, region, x, y);
      }
      continue;
    }
    for (int x = region.maxX; x >= region.minX; x -= 1) {
      countdownSettleOne(depthGrid, mask, region, x, y);
    }
  }
}

void fillCountdownDepthGrid(
  uint8_t* depthGrid,
  const uint8_t* mask,
  const CountdownRegion& region,
  int totalCount
) {
  memset(depthGrid, 0, (size_t)region.width * (size_t)region.height);
  int remaining = totalCount;
  for (int y = region.maxY; y >= region.minY && remaining > 0; y -= 1) {
    for (int offset = 0; offset <= region.maxX - region.minX && remaining > 0; offset += 1) {
      int leftX = kCountdownNeckX - offset;
      if (leftX >= region.minX) {
        int leftIndex = countdownRegionIndex(region, leftX, y);
        if (mask[leftIndex] == 1) {
          int fill = remaining < kCountdownSandStackDepth ? remaining : kCountdownSandStackDepth;
          depthGrid[leftIndex] = (uint8_t)fill;
          remaining -= fill;
        }
      }
      if (offset == 0 || remaining <= 0) {
        continue;
      }
      int rightX = kCountdownNeckX + offset;
      if (rightX <= region.maxX) {
        int rightIndex = countdownRegionIndex(region, rightX, y);
        if (mask[rightIndex] == 1) {
          int fill = remaining < kCountdownSandStackDepth ? remaining : kCountdownSandStackDepth;
          depthGrid[rightIndex] = (uint8_t)fill;
          remaining -= fill;
        }
      }
    }
  }
}

bool findCountdownTopReleaseSource(HourglassPoint& source) {
  for (int y = kCountdownTopMaxY; y >= kCountdownTopMinY; y -= 1) {
    for (int offset = 0; offset <= kCountdownTopMaxX - kCountdownTopMinX; offset += 1) {
      int leftX = kCountdownNeckX - offset;
      if (leftX >= kCountdownTopMinX && countdownHasSand(s_countdownTopDepth, kCountdownTopRegion, leftX, y)) {
        source.x = leftX;
        source.y = y;
        return true;
      }
      if (offset == 0) {
        continue;
      }
      int rightX = kCountdownNeckX + offset;
      if (rightX <= kCountdownTopMaxX && countdownHasSand(s_countdownTopDepth, kCountdownTopRegion, rightX, y)) {
        source.x = rightX;
        source.y = y;
        return true;
      }
    }
  }
  return false;
}

int countActiveCountdownParticles() {
  int count = 0;
  for (int index = 0; index < kCountdownMaxParticles; index += 1) {
    if (s_countdownParticles[index].active) {
      count += 1;
    }
  }
  return count;
}

bool countdownParticleOccupied(int x, int y, int ignoreIndex) {
  for (int index = 0; index < kCountdownMaxParticles; index += 1) {
    if (index == ignoreIndex) {
      continue;
    }
    if (!s_countdownParticles[index].active) {
      continue;
    }
    if (s_countdownParticles[index].x == x && s_countdownParticles[index].y == y) {
      return true;
    }
  }
  return false;
}

bool countdownCanMoveParticleTo(int x, int y, int ignoreIndex) {
  if (countdownParticleOccupied(x, y, ignoreIndex)) {
    return false;
  }
  if (y < kCountdownBottomMinY) {
    return x >= kCountdownNeckX - 1 && x <= kCountdownNeckX + 1 && y <= kCountdownBottomMinY;
  }
  return countdownCanReceive(s_countdownBottomMask, s_countdownBottomDepth, kCountdownBottomRegion, x, y);
}

void pushCountdownSplash(int x, int y) {
  const int offsets[] = {-1, 1};
  for (int offsetIndex = 0; offsetIndex < 2; offsetIndex += 1) {
    int splashX = x + offsets[offsetIndex];
    if (splashX < kCountdownBottomMinX || splashX > kCountdownBottomMaxX) {
      continue;
    }
    for (int slot = 0; slot < kCountdownMaxSplashes; slot += 1) {
      if (s_countdownSplashes[slot].active) {
        continue;
      }
      s_countdownSplashes[slot].active = true;
      s_countdownSplashes[slot].x = splashX;
      s_countdownSplashes[slot].y = y;
      s_countdownSplashes[slot].ttl = 2;
      break;
    }
  }
}

void countdownAdvanceParticles() {
  int order[kCountdownMaxParticles];
  int count = 0;
  for (int index = 0; index < kCountdownMaxParticles; index += 1) {
    if (!s_countdownParticles[index].active) {
      continue;
    }
    order[count] = index;
    count += 1;
  }

  for (int i = 0; i < count; i += 1) {
    for (int j = i + 1; j < count; j += 1) {
      int leftIndex = order[i];
      int rightIndex = order[j];
      if (s_countdownParticles[rightIndex].y > s_countdownParticles[leftIndex].y) {
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
      }
    }
  }

  for (int orderIndex = 0; orderIndex < count; orderIndex += 1) {
    int particleIndex = order[orderIndex];
    CountdownParticle& particle = s_countdownParticles[particleIndex];
    if (!particle.active) {
      continue;
    }

    const int targetXs[3] = {particle.x, particle.x - 1, particle.x + 1};
    const int targetYs[3] = {particle.y + 1, particle.y + 1, particle.y + 1};

    int targetX = particle.x;
    int targetY = particle.y;
    bool moved = false;

    bool midOpen = countdownCanMoveParticleTo(targetXs[0], targetYs[0], particleIndex);
    bool leftOpen = countdownCanMoveParticleTo(targetXs[1], targetYs[1], particleIndex);
    bool rightOpen = countdownCanMoveParticleTo(targetXs[2], targetYs[2], particleIndex);

    if (midOpen) {
      targetX = targetXs[0];
      targetY = targetYs[0];
      moved = true;
    } else if (leftOpen && !rightOpen) {
      targetX = targetXs[1];
      targetY = targetYs[1];
      moved = true;
    } else if (!leftOpen && rightOpen) {
      targetX = targetXs[2];
      targetY = targetYs[2];
      moved = true;
    } else if (leftOpen && rightOpen) {
      if (countdownRandomBool()) {
        targetX = targetXs[1];
        targetY = targetYs[1];
      } else {
        targetX = targetXs[2];
        targetY = targetYs[2];
      }
      moved = true;
    }

    if (moved) {
      particle.x = targetX;
      particle.y = targetY;
      continue;
    }

    if (
      particle.y >= kCountdownBottomMinY &&
      particle.y <= kCountdownBottomMaxY &&
      countdownCanReceive(s_countdownBottomMask, s_countdownBottomDepth, kCountdownBottomRegion, particle.x, particle.y)
    ) {
      int depositIndex = countdownRegionIndex(kCountdownBottomRegion, particle.x, particle.y);
      if (depositIndex < 0) {
        continue;
      }
      s_countdownBottomDepth[depositIndex] += 1;
      pushCountdownSplash(particle.x, particle.y);
      particle.active = false;
    }
  }
}

void countdownUpdateSplashes() {
  for (int index = 0; index < kCountdownMaxSplashes; index += 1) {
    if (!s_countdownSplashes[index].active) {
      continue;
    }
    if (s_countdownSplashes[index].ttl > 0) {
      s_countdownSplashes[index].ttl -= 1;
    }
    if (s_countdownSplashes[index].ttl == 0) {
      s_countdownSplashes[index].active = false;
    }
  }
}

void countdownScheduleRelease() {
  while (
    s_countdownReleaseAccumulator >= 1.0f &&
    countActiveCountdownParticles() < kCountdownMaxParticles
  ) {
    HourglassPoint source = {kCountdownNeckX, kCountdownTopMaxY};
    if (!findCountdownTopReleaseSource(source)) {
      s_countdownReleaseAccumulator = 0.0f;
      return;
    }

    int sourceIndex = countdownRegionIndex(kCountdownTopRegion, source.x, source.y);
    if (s_countdownTopDepth[sourceIndex] == 0) {
      s_countdownReleaseAccumulator = 0.0f;
      return;
    }

    s_countdownTopDepth[sourceIndex] -= 1;
    for (int index = 0; index < kCountdownMaxParticles; index += 1) {
      if (s_countdownParticles[index].active) {
        continue;
      }
      s_countdownParticles[index].active = true;
      s_countdownParticles[index].x = kCountdownNeckX;
      s_countdownParticles[index].y = kCountdownNeckY;
      break;
    }
    s_countdownReleaseAccumulator -= 1.0f;
  }
}

bool countdownIsSurface(const uint8_t* depthGrid, const uint8_t* mask, const CountdownRegion& region, int x, int y) {
  if (!countdownHasSand(depthGrid, region, x, y)) {
    return false;
  }
  if (y <= region.minY) {
    return true;
  }
  int aboveIndex = countdownRegionIndex(region, x, y - 1);
  if (aboveIndex < 0) {
    return true;
  }
  if (mask[aboveIndex] != 1) {
    return true;
  }
  return depthGrid[aboveIndex] == 0;
}

void drawCountdownDepthGrid(
  MatrixPanel_I2S_DMA* display,
  const uint8_t* depthGrid,
  const uint8_t* mask,
  const CountdownRegion& region
) {
  uint16_t sandColor = color565(255, 209, 90);
  uint16_t sandBright = color565(255, 240, 166);
  for (int y = region.minY; y <= region.maxY; y += 1) {
    for (int x = region.minX; x <= region.maxX; x += 1) {
      int index = countdownRegionIndex(region, x, y);
      if (index < 0 || mask[index] != 1 || !countdownHasSand(depthGrid, region, x, y)) {
        continue;
      }
      bool bright =
        countdownIsSurface(depthGrid, mask, region, x, y) ||
        (((x * 11 + y * 7 + (int)s_animTick) % 17) == 0);
      display->drawPixel(x, y, bright ? sandBright : sandColor);
    }
  }
}

void drawCountdownParticles(MatrixPanel_I2S_DMA* display) {
  uint16_t sandColor = color565(255, 209, 90);
  uint16_t sandBright = color565(255, 240, 166);
  for (int index = 0; index < kCountdownMaxParticles; index += 1) {
    if (!s_countdownParticles[index].active) {
      continue;
    }
    display->drawPixel(
      s_countdownParticles[index].x,
      s_countdownParticles[index].y,
      (index % 2 == 0) ? sandBright : sandColor
    );
  }
  for (int index = 0; index < kCountdownMaxSplashes; index += 1) {
    if (!s_countdownSplashes[index].active) {
      continue;
    }
    display->drawPixel(
      s_countdownSplashes[index].x,
      s_countdownSplashes[index].y,
      sandBright
    );
  }
}

void resetCountdownStateFromConfig() {
  ensureCountdownGeometry();
  s_countdownTotalSeconds = (int)s_countdownConfig.hours * 3600 + (int)s_countdownConfig.minutes * 60 + (int)s_countdownConfig.seconds;
  s_countdownRemainingSeconds = resolveCountdownRemainingSeconds(s_countdownConfig);
  s_countdownLastSecondAt = millis();
  s_countdownLastPhysicsAt = millis();
  s_countdownReleaseAccumulator = 0.0f;
  s_countdownRandomState =
    0x9e3779b9UL ^
    ((uint32_t)s_countdownConfig.hours << 24) ^
    ((uint32_t)s_countdownConfig.minutes << 16) ^
    ((uint32_t)s_countdownConfig.seconds << 8) ^
    (uint32_t)s_countdownConfig.progress;

  int totalCapacity = (s_countdownTopCellCount + s_countdownBottomCellCount) * kCountdownSandStackDepth;
  int minimumGrains = totalCapacity < 180 ? totalCapacity : 180;
  int grainTotal = clampInt((int)roundf((float)totalCapacity * kCountdownSandFillRatio), minimumGrains, totalCapacity);
  float ratio = 0.0f;
  if (s_countdownTotalSeconds > 0) {
    ratio = (float)s_countdownRemainingSeconds / (float)s_countdownTotalSeconds;
  }
  if (ratio < 0.0f) {
    ratio = 0.0f;
  }
  if (ratio > 1.0f) {
    ratio = 1.0f;
  }

  int topCount = clampInt((int)roundf((float)grainTotal * ratio), 0, grainTotal);
  int bottomCount = grainTotal - topCount;
  fillCountdownDepthGrid(
    s_countdownTopDepth,
    s_countdownTopMask,
    kCountdownTopRegion,
    topCount
  );
  fillCountdownDepthGrid(
    s_countdownBottomDepth,
    s_countdownBottomMask,
    kCountdownBottomRegion,
    bottomCount
  );

  clearCountdownParticles();
  clearCountdownSplashes();

  unsigned long remainingMs = (unsigned long)s_countdownRemainingSeconds * 1000UL;
  if (remainingMs > kCountdownTailSettleMs && topCount > 0) {
    s_countdownReleasePerMs = (float)topCount / (float)(remainingMs - kCountdownTailSettleMs);
  } else if (remainingMs > 0 && topCount > 0) {
    s_countdownReleasePerMs = (float)topCount / (float)remainingMs;
  } else {
    s_countdownReleasePerMs = 0.0f;
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
  uint16_t outlineColor = color565(200, 214, 246);
  uint16_t textColor = color565(255, 233, 171);
  display->fillScreen(background);

  drawCountdownDepthGrid(
    display,
    s_countdownTopDepth,
    s_countdownTopMask,
    kCountdownTopRegion
  );
  drawCountdownDepthGrid(
    display,
    s_countdownBottomDepth,
    s_countdownBottomMask,
    kCountdownBottomRegion
  );
  drawCountdownParticles(display);

  drawPolygonOutline(display, kTopHourglass, sizeof(kTopHourglass) / sizeof(kTopHourglass[0]), outlineColor);
  drawPolygonOutline(display, kBottomHourglass, sizeof(kBottomHourglass) / sizeof(kBottomHourglass[0]), outlineColor);
  display->drawLine(31, 28, 31, 30, outlineColor);
  display->drawLine(33, 28, 33, 30, outlineColor);

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
  DisplayManager::drawTinyTextCentered(timeText, 55, textColor, 1);
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

float planetRadius() {
  return 22.0f * resolvePlanetSizeScale(s_planetConfig.size);
}

bool planetPresetEquals(const char* preset) {
  return strcmp(s_planetConfig.preset, preset) == 0;
}

float planetFlowSign() {
  return resolvePlanetDirectionFactor(s_planetConfig.direction);
}

constexpr float kPlanetPreviewDiameter = 52.0f;
constexpr float kPlanetTwoPi = 6.2831853f;

float distancePlanet(float x1, float y1, float x2, float y2) {
  float dx = x1 - x2;
  float dy = y1 - y2;
  return sqrtf(dx * dx + dy * dy);
}

void rotatePlanetUv(float x, float y, float angle, float& outX, float& outY) {
  float cosValue = cosf(angle);
  float sinValue = sinf(angle);
  float dx = x - 0.5f;
  float dy = y - 0.5f;
  outX = dx * cosValue - dy * sinValue + 0.5f;
  outY = dx * sinValue + dy * cosValue + 0.5f;
}

float resolvePlanetPresetRelativeScale() {
  if (planetPresetEquals("gas_giant_2")) {
    return 3.0f;
  }
  if (planetPresetEquals("black_hole") || planetPresetEquals("star")) {
    return 2.0f;
  }
  return 1.0f;
}

float resolvePlanetPreviewPlaneSize(float planeScale) {
  return kPlanetPreviewDiameter * resolvePlanetSizeScale(s_planetConfig.size) * planeScale /
         resolvePlanetPresetRelativeScale();
}

bool mapPlanetPixelToUv(int x, int y, float planeSize, float& u, float& v) {
  float inversePlaneSize = 1.0f / planeSize;
  u = ((float)x + 0.5f - 32.0f) * inversePlaneSize + 0.5f;
  v = ((float)y + 0.5f - 32.0f) * inversePlaneSize + 0.5f;
  return u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f;
}

float resolvePlanetRotationTime() {
  return resolvePlanetFlow() * kPlanetTwoPi;
}

float resolvePlanetBackgroundStarExclusionRadius() {
  if (planetPresetEquals("galaxy")) {
    return 30.0f;
  }
  if (planetPresetEquals("star")) {
    return 28.0f;
  }
  if (planetPresetEquals("black_hole")) {
    return 29.0f;
  }

  float radius = planetRadius() + 5.0f;
  if (planetPresetEquals("gas_giant_2")) {
    radius += 4.0f;
  }
  return clampFloat(radius, 22.0f, 30.0f);
}

PlanetRgb planetShade(const PlanetRgb& base, float light) {
  float factor = clampFloat(0.24f + light * 0.72f, 0.0f, 1.0f);
  return scalePlanetRgb(base, factor);
}

PlanetRgb selectPlanetPalette3(float value, const PlanetRgb& color0, const PlanetRgb& color1, const PlanetRgb& color2) {
  float clamped = clampFloat(value, 0.0f, 1.0f);
  if (clamped > 0.66f) {
    return color2;
  }
  if (clamped > 0.33f) {
    return color1;
  }
  return color0;
}

PlanetRgb selectPlanetPalette4(float value, const PlanetRgb& color0, const PlanetRgb& color1, const PlanetRgb& color2, const PlanetRgb& color3) {
  float clamped = clampFloat(value, 0.0f, 1.0f);
  if (clamped > 0.75f) {
    return color3;
  }
  if (clamped > 0.5f) {
    return color2;
  }
  if (clamped > 0.25f) {
    return color1;
  }
  return color0;
}

float seededPlanetRange(uint32_t seed, const char* key, float minValue, float maxValue) {
  uint32_t state = seed ^ hashPlanetString(key);
  return minValue + (maxValue - minValue) * nextPlanetSeededRandom(state);
}

float seededPlanetRange(uint32_t seed, uint32_t salt, float minValue, float maxValue) {
  uint32_t state = seed ^ salt;
  return minValue + (maxValue - minValue) * nextPlanetSeededRandom(state);
}

float getWetTerranCloudCover(uint32_t seed) {
  return seededPlanetRange(seed, "wet_terran_cloud_cover", 0.35f, 0.60f);
}

float getIslandCloudCover(uint32_t seed) {
  return seededPlanetRange(seed, "islands_cloud_cover", 0.35f, 0.60f);
}

float getGasGiantCloudCover(uint32_t seed) {
  return seededPlanetRange(seed, "gas_giant_one_cloud_cover", 0.28f, 0.50f);
}

float samplePlanetCloudMask(float longitude, float latitude, float dx, float flow, uint32_t seed, float stretch, float scale, int octaves) {
  float cloudLatitude = latitude * stretch + smoothstepFloat(0.0f, 0.55f, fabsf(dx));
  float base = fbmPlanet(longitude * scale - flow * 1.8f + 31.0f, cloudLatitude * scale + 17.0f, seed + 43U, octaves);
  float detail = fbmPlanet(
    longitude * (scale + 2.4f) - flow * 0.95f + 59.0f,
    cloudLatitude * (scale * 0.6f) + 73.0f,
    seed + 59U,
    octaves > 2 ? octaves - 1 : octaves
  );
  return clampFloat(base * 0.72f + detail * 0.28f, 0.0f, 1.0f);
}

float samplePlanetRiverMask(float longitude, float latitude, float flow, uint32_t seed, float scale) {
  float base = fbmPlanet(longitude * scale + flow * 0.2f + 7.0f, latitude * scale + 13.0f, seed + 71U, 3);
  float branch = fbmPlanet(
    longitude * scale + base * 6.0f + 23.0f,
    latitude * scale + base * 6.0f + 29.0f,
    seed + 89U,
    2
  );
  return clampFloat(branch, 0.0f, 1.0f);
}

float samplePlanetCraterMask(float longitude, float latitude, float flow, uint32_t seed, float scale) {
  float field = fbmPlanet(longitude * scale + flow * 0.02f + 11.0f, latitude * scale + 7.0f, seed + 101U, 3);
  float rim = fbmPlanet(longitude * (scale * 1.6f) + 37.0f, latitude * (scale * 1.6f) + 19.0f, seed + 113U, 2);
  float cavity = smoothstepFloat(0.58f, 0.84f, field);
  float rimShadow = 1.0f - smoothstepFloat(0.18f, 0.46f, rim);
  return clampFloat(cavity * rimShadow, 0.0f, 1.0f);
}

float samplePlanetLakeMask(float longitude, float latitude, float flow, uint32_t seed, float scale) {
  float lake = fbmPlanet(longitude * scale + flow * 0.25f + 41.0f, latitude * scale + 53.0f, seed + 131U, 3);
  float detail = fbmPlanet(longitude * (scale * 1.4f) + 67.0f, latitude * (scale * 1.4f) + 79.0f, seed + 149U, 2);
  return clampFloat(lake * 0.65f + detail * 0.35f, 0.0f, 1.0f);
}

float samplePlanetGasMask(float longitude, float latitude, float flow, uint32_t seed, float scale, float bandScale) {
  float band = fbmPlanet(0.0f, latitude * scale * bandScale + 17.0f, seed + 163U, 4);
  float turbulence = fbmPlanet(
    longitude * scale * 0.35f - flow * 1.4f + 11.0f,
    latitude * scale * 0.55f + 23.0f,
    seed + 173U,
    2
  );
  float gas = fbmPlanet(
    longitude * scale - flow * 1.6f + turbulence * 2.4f + 31.0f,
    latitude * scale * 1.2f,
    seed + 181U,
    3
  );
  return clampFloat(gas * 0.72f + band * 0.28f, 0.0f, 1.0f);
}

void drawPlanetBackground() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  display->fillScreen(color565(0, 0, 0));

  uint32_t seed = s_planetConfig.seed;
  float exclusionRadius = resolvePlanetBackgroundStarExclusionRadius();
  uint32_t state = seed ^ hashPlanetString("planet_starfield_sparse");
  int attempts = 0;
  int placed = 0;
  while (placed < 10 && attempts < 400) {
    attempts += 1;
    int sx = (int)floorf(nextPlanetSeededRandom(state) * 64.0f);
    int sy = (int)floorf(nextPlanetSeededRandom(state) * 64.0f);
    int dx = sx - 32;
    int dy = sy - 32;
    if (sqrtf((float)(dx * dx + dy * dy)) < exclusionRadius) {
      continue;
    }
    PlanetRgb starColor = nextPlanetSeededRandom(state) > 0.45f
      ? makePlanetRgb(188, 204, 255)
      : makePlanetRgb(255, 214, 132);
    float brightness = 0.44f + nextPlanetSeededRandom(state) * 0.24f;
    PlanetRgb adjusted = scalePlanetRgb(starColor, brightness);
    display->drawPixel(sx, sy, planetColor565(adjusted));
    placed += 1;
  }
}

void drawAccretionDiskHalf(bool frontHalf) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  float flow = resolvePlanetFlow();
  uint32_t seed = s_planetConfig.seed;
  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float dx = ((float)x + 0.5f - 32.0f) / 22.0f;
      float dy = (((float)y + 0.5f - 32.0f) / 22.0f) * 4.6f;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist < 0.72f || dist > 1.32f) {
        continue;
      }
      bool isFront = dy >= 0.0f;
      if (isFront != frontHalf) {
        continue;
      }
      float angle = atan2f(dy, dx) + flow * 2.1f;
      float noiseValue = fbmPlanet(cosf(angle) * 2.8f + flow * 3.0f, sinf(angle) * 2.8f + dist * 4.0f, seed + 137U, 3);
      float rim = 1.0f - fabsf(dist - 1.0f) / 0.32f;
      float alpha = clampFloat(rim * (0.45f + noiseValue * 0.75f), 0.0f, 1.0f);
      if (alpha < 0.18f) {
        continue;
      }
      PlanetRgb band = mixPlanetRgb(
        makePlanetRgb(255, 241, 188),
        makePlanetRgb(238, 122, 52),
        clampFloat((noiseValue + dist * 0.6f) * 0.7f, 0.0f, 1.0f)
      );
      if (!frontHalf) {
        band = darkenPlanetRgb(band, 0.28f);
      }
      drawPlanetPixel(display, x, y, scalePlanetRgb(band, alpha));
    }
  }
}

void drawRingHalf(float ringRadius, float ringWidth, float perspective, uint32_t seed, bool frontHalf) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  float flow = resolvePlanetFlow();
  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float dx = ((float)x + 0.5f - 32.0f) / ringRadius;
      float dy = (((float)y + 0.5f - 32.0f) / ringRadius) * perspective;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist < 1.0f - ringWidth || dist > 1.0f + ringWidth) {
        continue;
      }
      bool isFront = dy >= 0.0f;
      if (isFront != frontHalf) {
        continue;
      }
      float bandNoise = fbmPlanet(dx * 5.0f + flow * 1.8f, dy * 4.0f, seed + 59U, 3);
      float edge = 1.0f - fabsf(dist - 1.0f) / ringWidth;
      float alpha = clampFloat(edge * (0.35f + bandNoise * 0.85f), 0.0f, 1.0f);
      if (alpha < 0.12f) {
        continue;
      }
      PlanetRgb ring = mixPlanetRgb(
        makePlanetRgb(246, 224, 162),
        makePlanetRgb(164, 120, 82),
        clampFloat(bandNoise, 0.0f, 1.0f)
      );
      if (!frontHalf) {
        ring = darkenPlanetRgb(ring, 0.25f);
      }
      drawPlanetPixel(display, x, y, scalePlanetRgb(ring, alpha));
    }
  }
}

void drawAsteroidPlanet() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  uint32_t seed = s_planetConfig.seed;
  float radius = planetRadius() * 0.9f;
  float angleOffset = resolvePlanetFlow() * 0.9f;

  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float dx = ((float)x + 0.5f - 32.0f) / radius;
      float dy = ((float)y + 0.5f - 32.0f) / radius;
      float rx = cosf(angleOffset) * dx - sinf(angleOffset) * dy;
      float ry = sinf(angleOffset) * dx + cosf(angleOffset) * dy;
      float angle = atan2f(ry, rx);
      float radial = 0.86f + noisePlanet(cosf(angle) * 2.6f + 4.0f, sinf(angle) * 2.6f + 4.0f, seed + 7U) * 0.28f;
      float dist = sqrtf(rx * rx + ry * ry);
      if (dist > radial) {
        continue;
      }
      float nz = sqrtf(clampFloat(radial * radial - dist * dist, 0.0f, 1.0f));
      float light = clampFloat((-rx * 0.58f) + (-ry * 0.42f) + nz * 0.95f, 0.0f, 1.0f);
      float surface = fbmPlanet(rx * 4.0f + 10.0f, ry * 4.0f + 10.0f, seed + 101U, 3);
      PlanetRgb color = mixPlanetRgb(
        makePlanetRgb(118, 108, 102),
        makePlanetRgb(176, 150, 120),
        clampFloat(surface * 1.1f, 0.0f, 1.0f)
      );
      if (noisePlanet(rx * 8.0f + 30.0f, ry * 8.0f + 12.0f, seed + 211U) > 0.77f) {
        color = darkenPlanetRgb(color, 0.35f);
      }
      drawPlanetPixel(display, x, y, planetShade(color, light));
    }
  }
}

void drawGalaxyPlanet() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  uint32_t seed = s_planetConfig.seed;
  float planeSize = resolvePlanetPreviewPlaneSize(1.0f);
  float time = resolvePlanetRotationTime();
  const PlanetRgb palette[] = {
    makePlanetRgb(255, 255, 235),
    makePlanetRgb(255, 233, 141),
    makePlanetRgb(181, 224, 102),
    makePlanetRgb(101, 165, 102),
    makePlanetRgb(57, 93, 100),
    makePlanetRgb(50, 57, 77),
    makePlanetRgb(50, 41, 71)
  };
  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPixelToUv(x, y, planeSize, u, v)) {
        continue;
      }

      float zoomedU = u * 1.375f - 0.1875f;
      float zoomedV = v * 1.375f - 0.1875f;
      float rotatedU = 0.0f;
      float rotatedV = 0.0f;
      rotatePlanetUv(zoomedU, zoomedV, 0.674f, rotatedU, rotatedV);

      float layerU = rotatedU;
      float layerV = rotatedV * 3.0f - 1.0f;
      float distanceOne = distancePlanet(layerU, layerV, 0.5f, 0.5f);
      float swirlOne = -9.0f * powf(distanceOne, 0.4f);
      float rotatedLayerU = 0.0f;
      float rotatedLayerV = 0.0f;
      rotatePlanetUv(layerU, layerV, swirlOne + time, rotatedLayerU, rotatedLayerV);
      float f1 = fbmPlanet(rotatedLayerU * 7.0f, rotatedLayerV * 7.0f, seed + 433U, 1);
      f1 = floorf(f1 * 4.0f) / 4.0f;

      float galaxyU = rotatedU;
      float galaxyV = rotatedV * 3.0f - 1.0f - f1 * 0.4f;
      float distanceTwo = distancePlanet(galaxyU, galaxyV, 0.5f, 0.5f);
      float swirlTwo = -9.0f * powf(distanceTwo, 0.4f);
      float rotatedGalaxyU = 0.0f;
      float rotatedGalaxyV = 0.0f;
      rotatePlanetUv(galaxyU, galaxyV, swirlTwo + time, rotatedGalaxyU, rotatedGalaxyV);
      float f2 = fbmPlanet(
        rotatedGalaxyU * 7.0f + f1 * 10.0f,
        rotatedGalaxyV * 7.0f + f1 * 10.0f,
        seed + 467U,
        1
      );
      if (f2 + distanceTwo > 0.7f) {
        continue;
      }

      int colorIndex = clampInt((int)floorf(f2 * 2.3f * 6.0f), 0, 6);
      drawPlanetPixel(display, x, y, palette[colorIndex]);
    }
  }
}

void drawStarPlanet() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  uint32_t seed = s_planetConfig.seed;
  bool evenSeed = (s_planetConfig.colorSeed & 1UL) == 0;
  PlanetRgb starPalette[4];
  PlanetRgb flarePalette[2];
  if (evenSeed) {
    starPalette[0] = makePlanetRgb(245, 255, 232);
    starPalette[1] = makePlanetRgb(255, 216, 50);
    starPalette[2] = makePlanetRgb(255, 130, 59);
    starPalette[3] = makePlanetRgb(124, 25, 26);
    flarePalette[0] = makePlanetRgb(255, 216, 50);
    flarePalette[1] = makePlanetRgb(245, 255, 232);
  } else {
    starPalette[0] = makePlanetRgb(245, 255, 232);
    starPalette[1] = makePlanetRgb(119, 214, 193);
    starPalette[2] = makePlanetRgb(28, 146, 167);
    starPalette[3] = makePlanetRgb(3, 62, 94);
    flarePalette[0] = makePlanetRgb(119, 214, 193);
    flarePalette[1] = makePlanetRgb(245, 255, 232);
  }

  float outerPlaneSize = resolvePlanetPreviewPlaneSize(2.0f);
  float corePlaneSize = resolvePlanetPreviewPlaneSize(1.0f);
  float flow = resolvePlanetRotationTime();

  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float outerU = 0.0f;
      float outerV = 0.0f;
      if (mapPlanetPixelToUv(x, y, outerPlaneSize, outerU, outerV)) {
        float outerDx = outerU - 0.5f;
        float outerDy = outerV - 0.5f;
        float outerDist = sqrtf(outerDx * outerDx + outerDy * outerDy);
        if (outerDist < 0.37f) {
          float outerAngle = atan2f(outerDx, outerDy);
          float blobNoise = fbmPlanet(
            outerDist * 4.93f - flow * 0.05f + 11.0f,
            outerAngle * 4.93f + 31.0f,
            seed + 271U,
            3
          );
          float halo = clampFloat((0.37f - outerDist) * 3.6f, 0.0f, 1.0f);
          float blobAlpha = clampFloat((blobNoise - 0.54f) * 2.2f, 0.0f, 1.0f) * halo;
          if (blobAlpha > 0.08f) {
            PlanetRgb blobColor = makePlanetRgb(255, 255, 228);
            drawPlanetPixel(display, x, y, scalePlanetRgb(blobColor, blobAlpha));
          }
        }
      }

      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPixelToUv(x, y, corePlaneSize, u, v)) {
        continue;
      }

      float dx = u - 0.5f;
      float dy = v - 0.5f;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist > 0.5f) {
        continue;
      }

      float cellNoise = fbmPlanet(u * 10.0f - flow * 0.12f + 17.0f, v * 10.0f + 23.0f, seed + 307U, 3);
      int colorIndex = clampInt((int)floorf(cellNoise * 4.0f), 0, 3);
      PlanetRgb color = starPalette[colorIndex];

      float angle = atan2f(dx, dy);
      float flareNoise = fbmPlanet(dist * 1.6f - flow * 0.05f + 43.0f, angle * 1.6f + 79.0f, seed + 331U, 4);
      float flareAlpha = clampFloat((flareNoise - 0.66f) * 2.0f, 0.0f, 1.0f);
      if (flareAlpha > 0.0f && dist > 0.18f) {
        PlanetRgb flareColor = flareNoise > 0.82f ? flarePalette[1] : flarePalette[0];
        color = mixPlanetRgb(color, flareColor, clampFloat(flareAlpha * 0.65f, 0.0f, 0.72f));
      }

      float light = clampFloat((1.0f - dist * 1.35f) + (1.0f - dist) * 0.18f, 0.0f, 1.0f);
      drawPlanetPixel(
        display,
        x,
        y,
        scalePlanetRgb(color, clampFloat(0.78f + light * 0.38f, 0.0f, 1.0f))
      );
    }
  }
}

PlanetRgb sampleSphereColor(float dx, float dy, float dz, float light, uint32_t seed) {
  float flow = resolvePlanetFlow();
  float longitude = atan2f(dx, dz) / 6.2831853f + 0.5f + flow;
  float latitude = dy * 0.5f + 0.5f;
  float n1 = fbmPlanet(longitude * 4.0f + 3.0f, latitude * 4.0f + 7.0f, seed + 11U, 3);
  float n2 = fbmPlanet(longitude * 8.0f + 13.0f, latitude * 8.0f + 19.0f, seed + 23U, 2);
  float n3 = fbmPlanet(longitude * 6.0f + flow * 0.45f + 41.0f, latitude * 6.0f + 29.0f, seed + 31U, 2);

  if (planetPresetEquals("terran_wet")) {
    float landMask = clampFloat(n1 - n2 * 0.18f + n3 * 0.12f, 0.0f, 1.0f);
    float riverMask = samplePlanetRiverMask(longitude, latitude, flow, seed, 4.6f);
    float cloudMask = samplePlanetCloudMask(longitude, latitude, dx, flow, seed, 2.0f, 7.3f, 2);
    PlanetRgb ocean = selectPlanetPalette3(
      clampFloat(n2 * 0.75f + (1.0f - light) * 0.35f, 0.0f, 1.0f),
      makePlanetRgb(79, 164, 184),
      makePlanetRgb(64, 73, 115),
      makePlanetRgb(40, 53, 64)
    );
    if (landMask < 0.43f && landMask > 0.37f) {
      ocean = mixPlanetRgb(ocean, makePlanetRgb(92, 192, 220), 0.45f);
    }
    PlanetRgb land = selectPlanetPalette4(
      clampFloat(n2 * 0.7f + n3 * 0.3f + (1.0f - light) * 0.2f, 0.0f, 1.0f),
      makePlanetRgb(99, 171, 63),
      makePlanetRgb(59, 125, 79),
      makePlanetRgb(47, 87, 83),
      makePlanetRgb(40, 53, 64)
    );
    PlanetRgb base = landMask >= 0.41f ? land : ocean;
    if (landMask >= 0.41f && riverMask > 0.58f) {
      PlanetRgb river = mixPlanetRgb(makePlanetRgb(79, 164, 184), makePlanetRgb(64, 73, 115), clampFloat((riverMask - 0.58f) * 2.0f, 0.0f, 1.0f));
      base = mixPlanetRgb(base, river, clampFloat((riverMask - 0.58f) * 1.35f, 0.0f, 0.82f));
    }
    float cloudCover = getWetTerranCloudCover(seed);
    if (cloudMask > cloudCover) {
      PlanetRgb cloudColor = selectPlanetPalette4(
        1.0f - light,
        makePlanetRgb(245, 255, 232),
        makePlanetRgb(223, 224, 232),
        makePlanetRgb(104, 111, 153),
        makePlanetRgb(64, 73, 115)
      );
      float cloudAlpha = clampFloat((cloudMask - cloudCover) * 2.2f, 0.0f, 0.72f);
      base = mixPlanetRgb(base, cloudColor, cloudAlpha);
    }
    return planetShade(base, light);
  }

  if (planetPresetEquals("terran_dry")) {
    PlanetRgb base = mixPlanetRgb(makePlanetRgb(228, 154, 72), makePlanetRgb(112, 54, 44), clampFloat(n1 + n2 * 0.25f, 0.0f, 1.0f));
    return planetShade(base, light);
  }

  if (planetPresetEquals("islands")) {
    float islandMask = clampFloat(n1 + n3 * 0.12f - n2 * 0.08f, 0.0f, 1.0f);
    float cloudMask = samplePlanetCloudMask(longitude, latitude, dx, flow * 1.8f, seed, 2.0f, 7.7f, 2);
    PlanetRgb ocean = selectPlanetPalette3(
      clampFloat(n2 * 0.85f + (1.0f - light) * 0.3f, 0.0f, 1.0f),
      makePlanetRgb(146, 232, 192),
      makePlanetRgb(79, 164, 184),
      makePlanetRgb(44, 53, 77)
    );
    PlanetRgb land = selectPlanetPalette4(
      clampFloat(n2 * 0.65f + n3 * 0.35f + (1.0f - light) * 0.18f, 0.0f, 1.0f),
      makePlanetRgb(200, 212, 93),
      makePlanetRgb(99, 171, 63),
      makePlanetRgb(47, 87, 83),
      makePlanetRgb(40, 53, 64)
    );
    PlanetRgb base = islandMask > 0.58f ? land : ocean;
    if (islandMask > 0.52f && islandMask <= 0.58f) {
      base = mixPlanetRgb(ocean, makePlanetRgb(146, 232, 192), 0.38f);
    }
    float cloudCover = getIslandCloudCover(seed);
    if (cloudMask > cloudCover) {
      PlanetRgb cloudColor = selectPlanetPalette4(
        1.0f - light,
        makePlanetRgb(223, 224, 232),
        makePlanetRgb(163, 167, 194),
        makePlanetRgb(104, 111, 153),
        makePlanetRgb(64, 73, 115)
      );
      float cloudAlpha = clampFloat((cloudMask - cloudCover) * 2.1f, 0.0f, 0.68f);
      base = mixPlanetRgb(base, cloudColor, cloudAlpha);
    }
    return planetShade(base, light);
  }

  if (planetPresetEquals("no_atmosphere")) {
    float craterMask = samplePlanetCraterMask(longitude, latitude, flow, seed, 5.0f);
    PlanetRgb base = selectPlanetPalette3(
      clampFloat(n1 * 0.78f + n2 * 0.22f + (1.0f - light) * 0.28f, 0.0f, 1.0f),
      makePlanetRgb(163, 167, 194),
      makePlanetRgb(76, 104, 133),
      makePlanetRgb(58, 63, 94)
    );
    if (craterMask > 0.42f) {
      PlanetRgb crater = mixPlanetRgb(makePlanetRgb(76, 104, 133), makePlanetRgb(58, 63, 94), clampFloat((craterMask - 0.42f) * 1.7f, 0.0f, 1.0f));
      base = mixPlanetRgb(base, crater, clampFloat((craterMask - 0.42f) * 1.6f, 0.0f, 0.74f));
    }
    return planetShade(base, light);
  }

  if (planetPresetEquals("gas_giant_1")) {
    float darkMask = samplePlanetGasMask(longitude, latitude, flow * 1.45f, seed, 9.0f, 1.0f);
    float warmMask = samplePlanetGasMask(longitude, latitude, flow, seed + 17U, 9.0f, 1.0f);
    PlanetRgb base = selectPlanetPalette4(
      clampFloat(darkMask + (1.0f - light) * 0.2f, 0.0f, 1.0f),
      makePlanetRgb(59, 32, 39),
      makePlanetRgb(59, 32, 39),
      makePlanetRgb(0, 0, 0),
      makePlanetRgb(33, 24, 27)
    );
    float cloudCover = getGasGiantCloudCover(seed);
    if (warmMask > cloudCover) {
      PlanetRgb cloudColor = selectPlanetPalette4(
        clampFloat(warmMask * 0.8f + (1.0f - light) * 0.2f, 0.0f, 1.0f),
        makePlanetRgb(240, 181, 65),
        makePlanetRgb(207, 117, 43),
        makePlanetRgb(171, 81, 48),
        makePlanetRgb(125, 56, 51)
      );
      base = mixPlanetRgb(base, cloudColor, clampFloat((warmMask - cloudCover) * 1.85f, 0.0f, 0.88f));
    }
    return planetShade(base, light);
  }

  if (planetPresetEquals("gas_giant_2")) {
    float bandMask = samplePlanetGasMask(longitude, latitude, flow * 0.55f, seed, 10.1f, 0.89f);
    PlanetRgb base = selectPlanetPalette3(
      clampFloat(bandMask + (1.0f - light) * 0.24f, 0.0f, 1.0f),
      makePlanetRgb(238, 195, 154),
      makePlanetRgb(217, 160, 102),
      makePlanetRgb(143, 86, 59)
    );
    if (bandMask > 0.63f) {
      PlanetRgb darkBand = selectPlanetPalette3(
        clampFloat((bandMask - 0.63f) * 2.7f, 0.0f, 1.0f),
        makePlanetRgb(102, 57, 49),
        makePlanetRgb(69, 40, 60),
        makePlanetRgb(34, 32, 52)
      );
      base = mixPlanetRgb(base, darkBand, clampFloat((bandMask - 0.63f) * 1.5f, 0.0f, 0.72f));
    }
    return planetShade(base, light);
  }

  if (planetPresetEquals("ice_world")) {
    float lakeMask = samplePlanetLakeMask(longitude, latitude, flow, seed, 10.0f);
    float cloudMask = samplePlanetCloudMask(longitude, latitude, dx, flow, seed, 2.5f, 4.0f, 4);
    PlanetRgb base = selectPlanetPalette3(
      clampFloat(n1 * 0.72f + n2 * 0.28f + (1.0f - light) * 0.22f, 0.0f, 1.0f),
      makePlanetRgb(250, 255, 255),
      makePlanetRgb(199, 212, 225),
      makePlanetRgb(146, 143, 184)
    );
    if (lakeMask > 0.55f) {
      PlanetRgb lake = selectPlanetPalette3(
        clampFloat(lakeMask * 0.9f + (1.0f - light) * 0.22f, 0.0f, 1.0f),
        makePlanetRgb(79, 164, 184),
        makePlanetRgb(76, 104, 133),
        makePlanetRgb(58, 63, 94)
      );
      base = mixPlanetRgb(base, lake, clampFloat((lakeMask - 0.55f) * 1.7f, 0.0f, 0.86f));
    }
    if (cloudMask > 0.546f) {
      PlanetRgb cloudColor = selectPlanetPalette4(
        1.0f - light,
        makePlanetRgb(225, 242, 255),
        makePlanetRgb(192, 227, 255),
        makePlanetRgb(94, 112, 165),
        makePlanetRgb(64, 73, 115)
      );
      float cloudAlpha = clampFloat((cloudMask - 0.546f) * 2.0f, 0.0f, 0.64f);
      base = mixPlanetRgb(base, cloudColor, cloudAlpha);
    }
    return planetShade(base, light);
  }

  if (planetPresetEquals("lava_world")) {
    float craterMask = samplePlanetCraterMask(longitude, latitude, flow, seed + 19U, 3.5f);
    float lavaMask = samplePlanetRiverMask(longitude, latitude, flow, seed + 29U, 10.0f);
    PlanetRgb base = selectPlanetPalette3(
      clampFloat(n1 * 0.72f + n2 * 0.28f + (1.0f - light) * 0.24f, 0.0f, 1.0f),
      makePlanetRgb(143, 77, 87),
      makePlanetRgb(82, 51, 63),
      makePlanetRgb(61, 41, 54)
    );
    if (craterMask > 0.4f) {
      PlanetRgb crater = mixPlanetRgb(makePlanetRgb(82, 51, 63), makePlanetRgb(61, 41, 54), clampFloat((craterMask - 0.4f) * 1.8f, 0.0f, 1.0f));
      base = mixPlanetRgb(base, crater, clampFloat((craterMask - 0.4f) * 1.5f, 0.0f, 0.68f));
    }
    if (lavaMask > 0.58f) {
      PlanetRgb lava = selectPlanetPalette3(
        clampFloat(lavaMask + light * 0.18f, 0.0f, 1.0f),
        makePlanetRgb(255, 137, 51),
        makePlanetRgb(230, 69, 57),
        makePlanetRgb(173, 47, 69)
      );
      base = mixPlanetRgb(base, lava, clampFloat((lavaMask - 0.58f) * 2.0f, 0.0f, 0.92f));
    }
    return planetShade(base, light);
  }

  return planetShade(makePlanetRgb(120, 150, 180), light);
}

void drawSpherePlanet() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  uint32_t seed = s_planetConfig.seed;
  float radius = planetRadius();
  const float lightX = -0.55f;
  const float lightY = -0.35f;
  const float lightZ = 0.77f;

  if (planetPresetEquals("gas_giant_2")) {
    drawRingHalf(radius * 1.7f, 0.14f, 3.2f, seed, false);
  }

  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float dx = ((float)x + 0.5f - 32.0f) / radius;
      float dy = ((float)y + 0.5f - 32.0f) / radius;
      float rr = dx * dx + dy * dy;
      if (rr > 1.0f) {
        continue;
      }
      float dz = sqrtf(1.0f - rr);
      float light = clampFloat(dx * lightX + dy * lightY + dz * lightZ, 0.0f, 1.0f);
      PlanetRgb color = sampleSphereColor(dx, dy, dz, light, seed);
      if (rr > 0.84f) {
        color = darkenPlanetRgb(color, (rr - 0.84f) * 1.8f);
      }
      drawPlanetPixel(display, x, y, color);
    }
  }

  if (planetPresetEquals("gas_giant_2")) {
    drawRingHalf(radius * 1.7f, 0.14f, 3.2f, seed, true);
  }
}

void drawBlackHolePlanet() {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  uint32_t seed = s_planetConfig.seed;
  float planeSize = resolvePlanetPreviewPlaneSize(3.0f);
  float wobble = sinf(resolvePlanetRotationTime() * 0.4f) * 0.01f;
  const PlanetRgb diskPalette[] = {
    makePlanetRgb(255, 255, 235),
    makePlanetRgb(255, 245, 64),
    makePlanetRgb(255, 184, 74),
    makePlanetRgb(237, 123, 57),
    makePlanetRgb(189, 64, 53)
  };

  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPixelToUv(x, y, planeSize, u, v)) {
        continue;
      }

      float rotatedU = 0.0f;
      float rotatedV = 0.0f;
      rotatePlanetUv(u, v, 0.766f, rotatedU, rotatedV);
      float preservedU = rotatedU;
      float preservedV = rotatedV;

      float compressedX = (rotatedU - 0.5f) * 1.3f + 0.5f;
      float compressedU = 0.0f;
      float compressedV = 0.0f;
      rotatePlanetUv(compressedX, rotatedV, wobble, compressedU, compressedV);

      float lightOriginX = 0.5f;
      float lightOriginY = 0.5f;
      float diskWidth = 0.065f;
      float distanceToCenter = distancePlanet(compressedU, compressedV, 0.5f, 0.5f);
      if (compressedV < 0.5f) {
        float lift = smoothstepFloat(distanceToCenter, 0.5f, 0.2f);
        compressedV += lift;
        diskWidth += smoothstepFloat(distanceToCenter, 0.5f, 0.3f);
        lightOriginY -= lift;
      } else if (compressedV > 0.53f) {
        float drop = smoothstepFloat(distanceToCenter, 0.4f, 0.17f);
        compressedV -= drop;
        diskWidth += smoothstepFloat(distanceToCenter, 0.5f, 0.2f);
        lightOriginY += smoothstepFloat(distanceToCenter, 0.5f, 0.2f);
      }

      float lightDistance = distancePlanet(
        preservedU,
        preservedV * 14.0f,
        lightOriginX,
        lightOriginY * 14.0f
      ) * 0.3f;
      float centerX = compressedU;
      float centerY = (compressedV - 0.5f) * 14.0f;
      float centerDistance = distancePlanet(centerX, centerY, 0.5f, 0.0f);
      float disk = smoothstepFloat(0.1f - diskWidth * 2.0f, 0.5f - diskWidth, centerDistance);
      disk *= smoothstepFloat(centerDistance - diskWidth, centerDistance, 0.4f);

      float materialU = 0.0f;
      float materialV = 0.0f;
      rotatePlanetUv(centerX, centerY + 0.5f, resolvePlanetRotationTime() * 3.0f, materialU, materialV);
      disk *= sqrtf(clampFloat(fbmPlanet(materialU * 6.598f, materialV * 6.598f, seed + 137U, 3), 0.0f, 1.0f));
      if (disk <= 0.15f) {
        continue;
      }

      int colorIndex = clampInt((int)floorf((disk + lightDistance) * 4.0f), 0, 4);
      drawPlanetPixel(display, x, y, diskPalette[colorIndex]);
    }
  }

  float corePlaneSize = resolvePlanetPreviewPlaneSize(1.0f);
  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPixelToUv(x, y, corePlaneSize, u, v)) {
        continue;
      }
      float dist = distancePlanet(u, v, 0.5f, 0.5f);
      if (dist > 0.247f) {
        continue;
      }
      PlanetRgb color = makePlanetRgb(39, 39, 54);
      if (dist > 0.219f) {
        color = makePlanetRgb(255, 255, 235);
      }
      if (dist > 0.233f) {
        color = makePlanetRgb(237, 123, 57);
      }
      drawPlanetPixel(display, x, y, color);
    }
  }
}

void drawPlanetScreensaver() {
  drawPlanetBackground();

  if (planetPresetEquals("black_hole")) {
    drawBlackHolePlanet();
    return;
  }
  if (planetPresetEquals("galaxy")) {
    drawGalaxyPlanet();
    return;
  }
  if (planetPresetEquals("star")) {
    drawStarPlanet();
    return;
  }
  if (planetPresetEquals("asteroid")) {
    drawAsteroidPlanet();
    return;
  }
  drawSpherePlanet();
}

void setActiveMode(BoardNativeMode mode) {
  s_mode = mode;
  s_active = true;
  s_lastTickAt = millis();
  s_lastRenderAt = 0;
  s_animTick = 0;
  if (mode == BOARD_NATIVE_PLANET) {
    s_planetPhase = 0.0f;
    s_planetDirty = true;
  }
}
}

namespace BoardNativeEffect {
void init() {
  s_mode = BOARD_NATIVE_NONE;
  s_active = false;
  s_lastTickAt = millis();
  s_animTick = 0;
  s_planetPhase = 0.0f;
  s_planetDirty = false;
  refreshPlanetColorVariant();
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

void applyPlanetScreensaverConfig(const PlanetScreensaverNativeConfig& config) {
  s_planetConfig = config;
  refreshPlanetColorVariant();
  setActiveMode(BOARD_NATIVE_PLANET);
}

void update() {
  if (!s_active) {
    return;
  }

  unsigned long now = millis();
  if (s_mode == BOARD_NATIVE_PLANET) {
    unsigned long tickMs = resolvePlanetTickMs();
    float phaseStep = resolvePlanetPhaseStep(s_planetConfig.speed);
    if (s_lastTickAt == 0) {
      s_lastTickAt = now;
    }
    while (now - s_lastTickAt >= tickMs) {
      s_lastTickAt += tickMs;
      s_animTick += 1;
      s_planetPhase += phaseStep * planetFlowSign();
      s_planetDirty = true;
    }
    return;
  }

  if (s_lastTickAt == 0 || now - s_lastTickAt >= 33) {
    s_lastTickAt = now;
    s_animTick += 1;
  }

  if (s_mode == BOARD_NATIVE_COUNTDOWN) {
    while (now - s_countdownLastSecondAt >= 1000) {
      s_countdownLastSecondAt += 1000;
      if (s_countdownRemainingSeconds > 0) {
        s_countdownRemainingSeconds -= 1;
      }
    }

    while (now - s_countdownLastPhysicsAt >= kCountdownPhysicsStepMs) {
      s_countdownLastPhysicsAt += kCountdownPhysicsStepMs;
      if (s_countdownRemainingSeconds > 0 && s_countdownReleasePerMs > 0.0f) {
        s_countdownReleaseAccumulator += s_countdownReleasePerMs * (float)kCountdownPhysicsStepMs;
      }
      countdownScheduleRelease();
      countdownSettleChamber(
        s_countdownTopDepth,
        s_countdownTopMask,
        kCountdownTopRegion
      );
      countdownSettleChamber(
        s_countdownBottomDepth,
        s_countdownBottomMask,
        kCountdownBottomRegion
      );
      countdownAdvanceParticles();
      countdownUpdateSplashes();
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
  if (s_mode == BOARD_NATIVE_PLANET) {
    if (!s_planetDirty) {
      return;
    }
    s_lastRenderAt = now;
  } else {
    if (s_lastRenderAt != 0 && now - s_lastRenderAt < 33) {
      return;
    }
    s_lastRenderAt = now;
  }

  uint16_t* frameBuffer = &DisplayManager::animationBuffer[0][0];
  if (!DisplayManager::beginRedirectedFrame(frameBuffer, 0)) {
    return;
  }

  bool rendered = false;
  if (s_mode == BOARD_NATIVE_TEXT_DISPLAY) {
    drawTextDisplay();
    rendered = true;
  } else if (s_mode == BOARD_NATIVE_WEATHER) {
    drawWeatherBoard();
    rendered = true;
  } else if (s_mode == BOARD_NATIVE_COUNTDOWN) {
    drawCountdownBoard();
    rendered = true;
  } else if (s_mode == BOARD_NATIVE_STOPWATCH) {
    drawStopwatchBoard();
    rendered = true;
  } else if (s_mode == BOARD_NATIVE_NOTIFICATION) {
    drawNotificationBoard();
    rendered = true;
  } else if (s_mode == BOARD_NATIVE_PLANET) {
    drawPlanetScreensaver();
    rendered = true;
  }

  if (!rendered) {
    DisplayManager::endRedirectedFrame(false);
    return;
  }

  DisplayManager::endRedirectedFrame(true);
  if (s_mode == BOARD_NATIVE_PLANET) {
    s_planetDirty = false;
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

const PlanetScreensaverNativeConfig& getPlanetScreensaverConfig() {
  return s_planetConfig;
}

int getCountdownRemainingSeconds() {
  return s_countdownRemainingSeconds;
}

unsigned long getStopwatchElapsedMs() {
  return s_stopwatchElapsedMs;
}
}
