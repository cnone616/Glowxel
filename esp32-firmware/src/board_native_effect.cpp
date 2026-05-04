#include "board_native_effect.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "clock_font_renderer.h"
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
  "badge",
  6,
  100,
  200,
  255,
  10,
  18,
  36
};

PlanetScreensaverNativeConfig s_planetConfig = {
  "terran_wet",
  "medium",
  "right",
  3,
  20260415UL,
  20260415UL,
  32,
  32,
  CLOCK_FONT_CLASSIC_5X7,
  false,
  {
    true,
    1,
    32,
    5,
    255,
    255,
    255
  }
};

float s_planetPhase = 0.0f;
unsigned long s_planetPhaseBaseAt = 0;
bool s_planetDirty = false;
bool s_planetForceFullRefresh = false;

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

constexpr int kPlanetCanvasSize = 64;
constexpr int kPlanetBufferChannels = 3;
constexpr int kPlanetPreviewPixels = 100;
constexpr unsigned long kPlanetPreviewFrameCount = 48UL;
constexpr unsigned long kPlanetRenderTickMs = 16UL;
constexpr int kPlanetDirectColorCacheSize = 32;

float* s_planetPreviewBuffer = nullptr;

struct PlanetBufferColor {
  float r;
  float g;
  float b;
  float a;
};

struct PlanetColorVariant {
  float hueShift;
  float saturationScale;
  float valueScale;
  float tintMix;
  PlanetBufferColor tint;
};

PlanetColorVariant s_activePlanetColorVariant = {
  0.0f,
  1.0f,
  1.0f,
  0.0f,
  {0.0f, 0.0f, 0.0f, 1.0f}
};
bool s_planetColorVariantActive = false;
bool s_planetDirectColorVariantActive = false;
bool s_planetPreviewDirectActive = false;
bool s_planetDirectRenderActive = false;

constexpr bool kPlanetPerfTraceEnabled = false;
constexpr unsigned long kPlanetPerfLogIntervalMs = 1000UL;
constexpr int kPlanetDirectMaxOctaves = 3;
constexpr int kPlanetDirectMaxCloudNoiseSamples = 4;
constexpr int kPlanetDirectSphereRenderStep = 1;
constexpr int kPlanetDirectSpecialRenderStep = 1;
constexpr int kPlanetTerranWetLandTileWidth = 96;
constexpr int kPlanetTerranWetLandTileHeight = 48;
constexpr int kPlanetTerranWetCloudTileWidth = 64;
constexpr int kPlanetTerranWetCloudTileHeight = 64;
constexpr int kPlanetTerranWetCloudNoiseSamples = 5;
constexpr float kPlanetTerranWetCloudNoiseMax = (float)kPlanetTerranWetCloudNoiseSamples;
constexpr int kPlanetTerranWetRandCacheMax = 128;
constexpr int kPlanetGenericFbmTileWidth = 64;
constexpr int kPlanetGenericFbmTileHeight = 64;
constexpr int kPlanetGenericFbmTileSlotCount = 4;
constexpr int kPlanetUntiledFbmRectWidth = 96;
constexpr int kPlanetUntiledFbmRectHeight = 96;
constexpr int kPlanetUntiledFbmRectSlotCount = 1;
constexpr int kPlanetStarCellsTileWidth = 64;
constexpr int kPlanetStarCellsTileHeight = 64;
constexpr int kPlanetStarBlobTileWidth = 64;
constexpr int kPlanetStarBlobTileHeight = 64;
constexpr float kPlanetStarCellsTilePeriod = 1.0f;
constexpr float kPlanetStarBlobTilePeriod = 2.0f;

void releasePlanetRuntimeBuffers();
uint8_t quantizePlanetTileValue(float value, float maxValue);
float decodePlanetTileValue(uint8_t value, float maxValue);
float samplePlanetTileBilinear(
  const uint8_t* tile,
  int width,
  int height,
  float periodX,
  float periodY,
  float sampleX,
  float sampleY,
  float maxValue
);

struct PlanetPerfStats {
  unsigned long windowStartedAt;
  unsigned long updateCalls;
  unsigned long tickWakeups;
  unsigned long elapsedTicks;
  unsigned long droppedTicks;
  unsigned long renderCalls;
  unsigned long renderSkippedClean;
  unsigned long drawUsTotal;
  unsigned long drawUsMax;
  unsigned long presentUsTotal;
  unsigned long presentUsMax;
  unsigned long changedPixelsTotal;
  uint16_t changedPixelsMax;
  unsigned long renderGapTotalMs;
  unsigned long renderGapMaxMs;
  float progressDeltaTotal;
  float progressDeltaMax;
  bool hasLastProgress;
  float lastProgress;
  unsigned long lastRenderAtMs;
};

PlanetPerfStats s_planetPerfStats = {};

struct PlanetDirectColorCacheEntry {
  uint32_t redBits;
  uint32_t greenBits;
  uint32_t blueBits;
  uint16_t color565;
  bool valid;
};

PlanetDirectColorCacheEntry* s_planetDirectColorCache = nullptr;
bool s_planetDirectColorCacheDisabled = false;

struct PlanetTerranWetTileState {
  bool valid;
  uint32_t seed;
};

PlanetTerranWetTileState s_planetTerranWetLandTileState = {};
PlanetTerranWetTileState s_planetTerranWetCloudTileState = {};
PlanetTerranWetTileState s_planetTerranWetCloudNoiseTileState = {};
uint8_t* s_planetTerranWetLandTile = nullptr;
uint8_t* s_planetTerranWetCloudTile = nullptr;
uint8_t* s_planetTerranWetCloudNoiseTile = nullptr;
bool s_planetTerranWetTileCacheDisabled = false;

struct PlanetTerranWetStaticPixel {
  uint16_t offset;
  float pixelU;
  float pixelV;
  float sphereX;
  float sphereY;
  bool dith;
};

struct PlanetTerranWetStaticCache {
  bool valid;
  float sizeScale;
  int pixels;
  float centerX;
  float centerY;
  int activeCount;
  PlanetTerranWetStaticPixel* pixelsData;
};

struct PlanetTiledRandCache {
  bool valid;
  uint32_t seedBits;
  uint32_t sizeBits;
  uint32_t tileXBits;
  uint32_t tileYBits;
  int width;
  int height;
  float values[kPlanetTerranWetRandCacheMax];
};

PlanetTerranWetStaticCache s_planetTerranWetStaticCache = {};
PlanetTiledRandCache s_planetTerranWetLandRandCache = {};
PlanetTiledRandCache s_planetTerranWetCloudRandCache = {};
PlanetTiledRandCache s_planetDirectNoiseRandCache = {};

struct PlanetGenericFbmTileCache {
  bool valid;
  uint32_t seedBits;
  uint32_t sizeBits;
  uint32_t tileXBits;
  uint32_t tileYBits;
  int octaves;
  uint8_t* values;
  unsigned long useTick;
};

PlanetGenericFbmTileCache s_planetGenericFbmTileCaches[kPlanetGenericFbmTileSlotCount] = {};
bool s_planetGenericFbmTileCacheDisabled = false;
unsigned long s_planetGenericFbmTileUseTick = 0UL;
int s_planetGenericFbmTileBuildDepth = 0;

struct PlanetUntiledFbmRectCache {
  bool valid;
  uint32_t seedBits;
  uint32_t sizeBits;
  uint32_t minXBits;
  uint32_t maxXBits;
  uint32_t minYBits;
  uint32_t maxYBits;
  int octaves;
  uint8_t* values;
  unsigned long useTick;
};

PlanetUntiledFbmRectCache* s_planetUntiledFbmRectCaches = nullptr;
bool s_planetUntiledFbmRectCacheDisabled = false;
unsigned long s_planetUntiledFbmRectUseTick = 0UL;

struct PlanetStarCellsTileCache {
  bool valid;
  int numCells;
  int tiles;
  uint8_t* values;
};

PlanetStarCellsTileCache* s_planetStarCellsTileCaches = nullptr;
bool s_planetStarCellsTileCacheDisabled = false;

struct PlanetStarBlobTileCache {
  bool valid;
  uint32_t seedBits;
  uint32_t sizeBits;
  uint32_t circleAmountBits;
  uint32_t circleSizeBits;
  uint8_t* values;
};

PlanetStarBlobTileCache* s_planetStarBlobTileCache = nullptr;
bool s_planetStarBlobTileCacheDisabled = false;

struct PlanetRenderFrame {
  const char* presetId;
  uint32_t seed;
  uint32_t colorSeed;
  float centerX;
  float centerY;
  float progress;
  float motionFactor;
  float spinFactor;
  float spinAngle;
  float sizeScale;
  float shaderSeed;
  int pixels;
  bool dither;
};

struct PlanetUnderLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float timeSpeed;
  float ditherSize;
  float lightBorder1;
  float lightBorder2;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetLandMassLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float timeSpeed;
  float lightBorder1;
  float lightBorder2;
  float landCutoff;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetCloudLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float cloudCover;
  float timeSpeed;
  float stretch;
  float cloudCurve;
  float lightBorder1;
  float lightBorder2;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetIceLakeLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float timeSpeed;
  float lightBorder1;
  float lightBorder2;
  float lakeCutoff;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetCraterLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float timeSpeed;
  float lightBorder;
  float size;
  float offset;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetLavaRiverLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float timeSpeed;
  float lightBorder1;
  float lightBorder2;
  float riverCutoff;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetDryTerranLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float lightDistance1;
  float lightDistance2;
  float timeSpeed;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
  int colorCount;
};

struct PlanetLandRiverLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float timeSpeed;
  float ditherSize;
  float lightBorder1;
  float lightBorder2;
  float riverCutoff;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetDenseGasLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float timeSpeed;
  float bands;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
  const PlanetBufferColor* darkColors;
};

struct PlanetRingLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float timeSpeed;
  float ringWidth;
  float ringPerspective;
  float scaleRelToPlanet;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
  const PlanetBufferColor* darkColors;
};

struct PlanetAsteroidLayer {
  float planeScale;
  float pixelsScale;
  float lightOriginX;
  float lightOriginY;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetStarCoreLayer {
  float planeScale;
  float pixelsScale;
  float timeSpeed;
  int tiles;
  float rotationOffset;
  const PlanetBufferColor* colors;
  int colorCount;
};

struct PlanetStarBlobLayer {
  float planeScale;
  float pixelsScale;
  float timeSpeed;
  float size;
  float circleAmount;
  float circleSize;
  float rotationOffset;
  PlanetBufferColor color;
};

struct PlanetStarFlareLayer {
  float planeScale;
  float pixelsScale;
  float timeSpeed;
  float stormWidth;
  float stormDitherWidth;
  float circleAmount;
  float circleScale;
  float scale;
  float size;
  int octaves;
  float rotationOffset;
  const PlanetBufferColor* colors;
  int colorCount;
};

struct PlanetBlackHoleCoreLayer {
  float planeScale;
  float pixelsScale;
  float radius;
  float lightWidth;
  float rotationOffset;
  const PlanetBufferColor* colors;
};

struct PlanetBlackHoleDiskLayer {
  float planeScale;
  float pixelsScale;
  float timeSpeed;
  float diskWidth;
  float ringPerspective;
  float rotationOffset;
  float size;
  int octaves;
  const PlanetBufferColor* colors;
  int colorCount;
};

struct PlanetGalaxyLayer {
  float planeScale;
  float pixelsScale;
  float timeSpeed;
  float rotationOffset;
  float size;
  int octaves;
  float tilt;
  float layerCount;
  float layerHeight;
  float zoom;
  float swirl;
  int nColors;
  const PlanetBufferColor* colors;
  int colorCount;
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

PlanetRgb mixPlanetRgb(const PlanetRgb& left, const PlanetRgb& right, float amount);
void rgbToPlanetHsv(const PlanetRgb& color, float& hue, float& saturation, float& value);
PlanetRgb hsvToPlanetRgb(float hue, float saturation, float value);
bool shouldDrawPlanetDirectPixel(int x, int y, float alpha);
void spherifyPlanetUv(float x, float y, float& outX, float& outY);

PlanetRgb makePlanetRgbRaw(uint8_t r, uint8_t g, uint8_t b) {
  PlanetRgb color = { r, g, b };
  return color;
}

PlanetRgb applyPlanetDirectColorVariant(const PlanetRgb& color) {
  float hue = 0.0f;
  float saturation = 0.0f;
  float value = 0.0f;
  rgbToPlanetHsv(color, hue, saturation, value);
  PlanetRgb remapped = hsvToPlanetRgb(
    hue + s_planetHueShift,
    clampFloat(saturation * s_planetSaturationScale, 0.0f, 1.0f),
    clampFloat(value * s_planetTintBrightness, 0.0f, 1.0f)
  );
  return mixPlanetRgb(remapped, s_planetTintColor, s_planetTintMix);
}

PlanetRgb makePlanetRgb(uint8_t r, uint8_t g, uint8_t b) {
  return makePlanetRgbRaw(r, g, b);
}

PlanetRgb mixPlanetRgb(const PlanetRgb& left, const PlanetRgb& right, float amount) {
  float t = clampFloat(amount, 0.0f, 1.0f);
  return makePlanetRgbRaw(
    (uint8_t)roundf(mixFloat((float)left.r, (float)right.r, t)),
    (uint8_t)roundf(mixFloat((float)left.g, (float)right.g, t)),
    (uint8_t)roundf(mixFloat((float)left.b, (float)right.b, t))
  );
}

PlanetRgb scalePlanetRgb(const PlanetRgb& color, float factor) {
  float clamped = clampFloat(factor, 0.0f, 2.0f);
  return makePlanetRgbRaw(
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

  return makePlanetRgbRaw(
    (uint8_t)roundf(clampFloat((red + match) * 255.0f, 0.0f, 255.0f)),
    (uint8_t)roundf(clampFloat((green + match) * 255.0f, 0.0f, 255.0f)),
    (uint8_t)roundf(clampFloat((blue + match) * 255.0f, 0.0f, 255.0f))
  );
}

PlanetRgb brightenPlanetRgb(const PlanetRgb& color, float amount) {
  return mixPlanetRgb(color, makePlanetRgbRaw(255, 255, 255), clampFloat(amount, 0.0f, 1.0f));
}

PlanetRgb darkenPlanetRgb(const PlanetRgb& color, float amount) {
  return mixPlanetRgb(color, makePlanetRgbRaw(0, 0, 0), clampFloat(amount, 0.0f, 1.0f));
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

uint32_t planetFloatBits(float value) {
  uint32_t bits = 0;
  memcpy(&bits, &value, sizeof(bits));
  return bits;
}

void invalidatePlanetDirectCaches() {
  if (s_planetDirectColorCache == nullptr) {
    return;
  }
  for (int index = 0; index < kPlanetDirectColorCacheSize; index += 1) {
    s_planetDirectColorCache[index].valid = false;
  }
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
  invalidatePlanetDirectCaches();
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

float resolveDefaultPlanetSizeScale(const char* size) {
  if (strcmp(size, "small") == 0) {
    return 0.64f;
  }
  if (strcmp(size, "medium") == 0) {
    return 0.82f;
  }
  if (strcmp(size, "large") == 0) {
    return 1.0f;
  }
  return 1.0f;
}

float resolvePlanetSizeScaleFor(const char* preset, const char* size) {
  if (strcmp(preset, "gas_giant_2") == 0) {
    if (strcmp(size, "small") == 0) {
      return 1.15f;
    }
    if (strcmp(size, "medium") == 0) {
      return 1.40f;
    }
    if (strcmp(size, "large") == 0) {
      return 1.60f;
    }
  }
  if (strcmp(preset, "black_hole") == 0) {
    if (strcmp(size, "small") == 0) {
      return 1.00f;
    }
    if (strcmp(size, "medium") == 0) {
      return 1.25f;
    }
    if (strcmp(size, "large") == 0) {
      return 1.45f;
    }
  }
  if (strcmp(preset, "galaxy") == 0) {
    if (strcmp(size, "small") == 0) {
      return 1.00f;
    }
    if (strcmp(size, "medium") == 0) {
      return 1.25f;
    }
    if (strcmp(size, "large") == 0) {
      return 1.50f;
    }
  }
  if (strcmp(preset, "star") == 0) {
    if (strcmp(size, "small") == 0) {
      return 1.00f;
    }
    if (strcmp(size, "medium") == 0) {
      return 1.25f;
    }
    if (strcmp(size, "large") == 0) {
      return 1.50f;
    }
  }
  return resolveDefaultPlanetSizeScale(size);
}

float resolvePlanetSizeScale(const char* size) {
  return resolvePlanetSizeScaleFor(s_planetConfig.preset, size);
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
  int index = clampInt((int)speed, 1, 10) - 1;
  return kPlanetDelayBySpeed[index];
}

unsigned long resolvePlanetCycleDuration(uint8_t speed) {
  return resolvePlanetFrameDelay(speed) * (kPlanetPreviewFrameCount - 1UL);
}

unsigned long resolvePlanetTickMs() {
  return kPlanetRenderTickMs;
}

float resolvePlanetPhaseStep(uint8_t speed) {
  unsigned long cycleDuration = resolvePlanetCycleDuration(speed);
  if (cycleDuration == 0UL) {
    return 0.0f;
  }
  return (float)resolvePlanetTickMs() / (float)cycleDuration;
}

float resolvePlanetPlaybackPhase(unsigned long now) {
  unsigned long cycleDuration = resolvePlanetCycleDuration(s_planetConfig.speed);
  if (cycleDuration == 0UL || s_planetPhaseBaseAt == 0UL) {
    return wrapPlanetUnit(s_planetPhase);
  }

  float elapsedPhase = (float)(now - s_planetPhaseBaseAt) / (float)cycleDuration;
  return wrapPlanetUnit(s_planetPhase + elapsedPhase);
}

float resolvePlanetFlow() {
  return resolvePlanetPlaybackPhase(millis()) * 0.11f;
}

float resolvePlanetWrappedDelta(float previous, float current) {
  float delta = current - previous;
  if (delta < 0.0f) {
    delta += 1.0f;
  }
  return delta;
}

void resetPlanetPerfWindow(unsigned long now) {
  PlanetPerfStats next = {};
  next.windowStartedAt = now;
  next.hasLastProgress = s_planetPerfStats.hasLastProgress;
  next.lastProgress = s_planetPerfStats.lastProgress;
  next.lastRenderAtMs = s_planetPerfStats.lastRenderAtMs;
  s_planetPerfStats = next;
}

void ensurePlanetPerfWindowStarted(unsigned long now) {
  if (!kPlanetPerfTraceEnabled) {
    return;
  }
  if (s_planetPerfStats.windowStartedAt == 0UL) {
    resetPlanetPerfWindow(now);
  }
}

void logPlanetPerfIfNeeded(unsigned long now) {
  if (!kPlanetPerfTraceEnabled || s_mode != BOARD_NATIVE_PLANET) {
    return;
  }
  ensurePlanetPerfWindowStarted(now);
  unsigned long windowMs = now - s_planetPerfStats.windowStartedAt;
  if (windowMs < kPlanetPerfLogIntervalMs) {
    return;
  }

  float windowSeconds = (float)windowMs / 1000.0f;
  float renderCount = (float)s_planetPerfStats.renderCalls;
  float renderFps = windowSeconds > 0.0f ? renderCount / windowSeconds : 0.0f;
  float tickFps = windowSeconds > 0.0f ? (float)s_planetPerfStats.elapsedTicks / windowSeconds : 0.0f;
  float drawAvgMs = renderCount > 0.0f
    ? ((float)s_planetPerfStats.drawUsTotal / renderCount) / 1000.0f
    : 0.0f;
  float presentAvgMs = renderCount > 0.0f
    ? ((float)s_planetPerfStats.presentUsTotal / renderCount) / 1000.0f
    : 0.0f;
  float changedAvg = renderCount > 0.0f
    ? (float)s_planetPerfStats.changedPixelsTotal / renderCount
    : 0.0f;
  float gapAvgMs = renderCount > 1.0f
    ? (float)s_planetPerfStats.renderGapTotalMs / (renderCount - 1.0f)
    : 0.0f;
  float progressAvg = renderCount > 1.0f
    ? s_planetPerfStats.progressDeltaTotal / (renderCount - 1.0f)
    : 0.0f;

  Serial.printf(
    "[PLANET][PERF] preset=%s speed=%u progress=%.5f hash=%08lx updates=%lu wakes=%lu ticks=%lu drop=%lu renders=%lu skip=%lu tickFps=%.1f renderFps=%.1f drawAvg=%.2fms drawMax=%.2fms presentAvg=%.2fms presentMax=%.2fms changedAvg=%.1f changedMax=%u gapAvg=%.1fms gapMax=%lums progressAvg=%.5f progressMax=%.5f\n",
    s_planetConfig.preset,
    s_planetConfig.speed,
    s_planetPerfStats.lastProgress,
    (unsigned long)DisplayManager::getLastPresentFrameHash(),
    s_planetPerfStats.updateCalls,
    s_planetPerfStats.tickWakeups,
    s_planetPerfStats.elapsedTicks,
    s_planetPerfStats.droppedTicks,
    s_planetPerfStats.renderCalls,
    s_planetPerfStats.renderSkippedClean,
    tickFps,
    renderFps,
    drawAvgMs,
    (float)s_planetPerfStats.drawUsMax / 1000.0f,
    presentAvgMs,
    (float)s_planetPerfStats.presentUsMax / 1000.0f,
    changedAvg,
    s_planetPerfStats.changedPixelsMax,
    gapAvgMs,
    s_planetPerfStats.renderGapMaxMs,
    progressAvg,
    s_planetPerfStats.progressDeltaMax
  );

  resetPlanetPerfWindow(now);
}

void recordPlanetRenderPerf(
  unsigned long now,
  float progress,
  unsigned long drawUs,
  unsigned long presentUs,
  uint16_t changedPixels
) {
  if (!kPlanetPerfTraceEnabled) {
    return;
  }

  ensurePlanetPerfWindowStarted(now);
  s_planetPerfStats.renderCalls += 1UL;
  s_planetPerfStats.drawUsTotal += drawUs;
  if (drawUs > s_planetPerfStats.drawUsMax) {
    s_planetPerfStats.drawUsMax = drawUs;
  }
  s_planetPerfStats.presentUsTotal += presentUs;
  if (presentUs > s_planetPerfStats.presentUsMax) {
    s_planetPerfStats.presentUsMax = presentUs;
  }
  s_planetPerfStats.changedPixelsTotal += (unsigned long)changedPixels;
  if (changedPixels > s_planetPerfStats.changedPixelsMax) {
    s_planetPerfStats.changedPixelsMax = changedPixels;
  }

  if (s_planetPerfStats.lastRenderAtMs != 0UL) {
    unsigned long gapMs = now - s_planetPerfStats.lastRenderAtMs;
    s_planetPerfStats.renderGapTotalMs += gapMs;
    if (gapMs > s_planetPerfStats.renderGapMaxMs) {
      s_planetPerfStats.renderGapMaxMs = gapMs;
    }
  }

  if (s_planetPerfStats.hasLastProgress) {
    float progressDelta = resolvePlanetWrappedDelta(s_planetPerfStats.lastProgress, progress);
    s_planetPerfStats.progressDeltaTotal += progressDelta;
    if (progressDelta > s_planetPerfStats.progressDeltaMax) {
      s_planetPerfStats.progressDeltaMax = progressDelta;
    }
  } else {
    s_planetPerfStats.hasLastProgress = true;
  }

  s_planetPerfStats.lastProgress = progress;
  s_planetPerfStats.lastRenderAtMs = now;
}

void drawPlanetPixel(MatrixPanel_I2S_DMA* display, int x, int y, const PlanetRgb& color) {
  PlanetRgb output = color;
  if (s_planetDirectColorVariantActive) {
    output = applyPlanetDirectColorVariant(color);
  }
  display->drawPixel(x, y, planetColor565(output));
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
constexpr float kPlanetRingTimeBase = 314.15f;
constexpr float kPlanetDirectCircleCutoff = 0.49999f;
constexpr float kPlanetDirectCircleCutoffSquared =
  kPlanetDirectCircleCutoff * kPlanetDirectCircleCutoff;
constexpr float kPlanetHalfSquared = 0.25f;

float distancePlanetSquared(float x1, float y1, float x2, float y2) {
  float dx = x1 - x2;
  float dy = y1 - y2;
  return dx * dx + dy * dy;
}

float distancePlanet(float x1, float y1, float x2, float y2) {
  return sqrtf(distancePlanetSquared(x1, y1, x2, y2));
}

bool planetDirectInsideCircle(float x, float y) {
  return distancePlanetSquared(x, y, 0.5f, 0.5f) <= kPlanetDirectCircleCutoffSquared;
}

bool planetInsideUnitCircle(float x, float y) {
  return distancePlanetSquared(x, y, 0.5f, 0.5f) <= kPlanetHalfSquared;
}

void rotatePlanetUv(float x, float y, float angle, float& outX, float& outY) {
  if (angle == 0.0f) {
    outX = x;
    outY = y;
    return;
  }
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

float resolvePlanetRenderCenterX() {
  return (float)s_planetConfig.planetX;
}

float resolvePlanetRenderCenterY() {
  return (float)s_planetConfig.planetY;
}

bool mapPlanetPixelToUv(
  int x,
  int y,
  float planeSize,
  float centerX,
  float centerY,
  float& u,
  float& v
) {
  float inversePlaneSize = 1.0f / planeSize;
  u = ((float)x + 0.5f - centerX) * inversePlaneSize + 0.5f;
  v = ((float)y + 0.5f - centerY) * inversePlaneSize + 0.5f;
  return u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f;
}

bool mapPlanetPixelToUv(int x, int y, float planeSize, float& u, float& v) {
  return mapPlanetPixelToUv(
    x,
    y,
    planeSize,
    resolvePlanetRenderCenterX(),
    resolvePlanetRenderCenterY(),
    u,
    v
  );
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
  return seededPlanetRange(seed, "wet_terran_cloud_cover", 0.31f, 0.56f);
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
      PlanetRgb band = makePlanetRgb(255, 241, 188);
      float tone = clampFloat((noiseValue + dist * 0.6f) * 0.7f, 0.0f, 1.0f);
      if (tone > 0.66f) {
        band = frontHalf ? makePlanetRgb(238, 122, 52) : makePlanetRgb(164, 78, 40);
      } else if (tone > 0.33f) {
        band = frontHalf ? makePlanetRgb(255, 184, 74) : makePlanetRgb(196, 122, 58);
      } else if (!frontHalf) {
        band = makePlanetRgb(204, 168, 120);
      }
      drawPlanetPixel(display, x, y, band);
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
      if (!shouldDrawPlanetDirectPixel(x, y, alpha)) {
        continue;
      }
      PlanetRgb ring = frontHalf ? makePlanetRgb(246, 224, 162) : makePlanetRgb(188, 164, 118);
      if (bandNoise > 0.66f) {
        ring = frontHalf ? makePlanetRgb(164, 120, 82) : makePlanetRgb(112, 84, 60);
      } else if (bandNoise > 0.33f) {
        ring = frontHalf ? makePlanetRgb(210, 172, 118) : makePlanetRgb(148, 116, 84);
      }
      drawPlanetPixel(display, x, y, ring);
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
      PlanetRgb color = makePlanetRgb(118, 108, 102);
      if (surface > 0.68f && light > 0.42f) {
        color = makePlanetRgb(176, 150, 120);
      } else if (surface < 0.28f || light < 0.18f) {
        color = makePlanetRgb(76, 70, 68);
      }
      if (noisePlanet(rx * 8.0f + 30.0f, ry * 8.0f + 12.0f, seed + 211U) > 0.77f) {
        color = makePlanetRgb(76, 70, 68);
      }
      drawPlanetPixel(display, x, y, color);
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
          if (shouldDrawPlanetDirectPixel(x, y, blobAlpha)) {
            PlanetRgb blobColor = makePlanetRgb(255, 255, 228);
            drawPlanetPixel(display, x, y, blobColor);
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
      drawPlanetPixel(display, x, y, starPalette[colorIndex]);
    }
  }

  for (int y = 0; y < 64; y += 1) {
    for (int x = 0; x < 64; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPixelToUv(x, y, corePlaneSize, u, v)) {
        continue;
      }

      float dx = u - 0.5f;
      float dy = v - 0.5f;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist > 0.5f || dist <= 0.18f) {
        continue;
      }

      float angle = atan2f(dx, dy);
      float flareNoise = fbmPlanet(dist * 1.6f - flow * 0.05f + 43.0f, angle * 1.6f + 79.0f, seed + 331U, 4);
      if (flareNoise <= 0.66f) {
        continue;
      }

      PlanetRgb flareColor = flareNoise > 0.82f ? flarePalette[1] : flarePalette[0];
      drawPlanetPixel(display, x, y, flareColor);
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
      clampFloat(n2 * 0.75f, 0.0f, 1.0f),
      makePlanetRgb(79, 164, 184),
      makePlanetRgb(64, 73, 115),
      makePlanetRgb(40, 53, 64)
    );
    if (landMask < 0.43f && landMask > 0.37f) {
      ocean = makePlanetRgb(92, 192, 220);
    }
    PlanetRgb land = selectPlanetPalette4(
      clampFloat(n2 * 0.7f + n3 * 0.3f, 0.0f, 1.0f),
      makePlanetRgb(99, 171, 63),
      makePlanetRgb(59, 125, 79),
      makePlanetRgb(47, 87, 83),
      makePlanetRgb(40, 53, 64)
    );
    float cloudCover = getWetTerranCloudCover(seed);
    if (cloudMask > cloudCover) {
      return selectPlanetPalette4(
        clampFloat(cloudMask, 0.0f, 1.0f),
        makePlanetRgb(245, 255, 232),
        makePlanetRgb(223, 224, 232),
        makePlanetRgb(104, 111, 153),
        makePlanetRgb(64, 73, 115)
      );
    }
    if (landMask >= 0.41f && riverMask > 0.58f) {
      return riverMask > 0.79f
        ? makePlanetRgb(64, 73, 115)
        : makePlanetRgb(79, 164, 184);
    }
    return landMask >= 0.41f ? land : ocean;
  }

  if (planetPresetEquals("terran_dry")) {
    return selectPlanetPalette3(
      clampFloat(n1 + n2 * 0.25f, 0.0f, 1.0f),
      makePlanetRgb(228, 154, 72),
      makePlanetRgb(172, 96, 56),
      makePlanetRgb(112, 54, 44)
    );
  }

  if (planetPresetEquals("islands")) {
    float islandMask = clampFloat(n1 + n3 * 0.12f - n2 * 0.08f, 0.0f, 1.0f);
    float cloudMask = samplePlanetCloudMask(longitude, latitude, dx, flow * 1.8f, seed, 2.0f, 7.7f, 2);
    PlanetRgb ocean = selectPlanetPalette3(
      clampFloat(n2 * 0.85f, 0.0f, 1.0f),
      makePlanetRgb(146, 232, 192),
      makePlanetRgb(79, 164, 184),
      makePlanetRgb(44, 53, 77)
    );
    PlanetRgb land = selectPlanetPalette4(
      clampFloat(n2 * 0.65f + n3 * 0.35f, 0.0f, 1.0f),
      makePlanetRgb(200, 212, 93),
      makePlanetRgb(99, 171, 63),
      makePlanetRgb(47, 87, 83),
      makePlanetRgb(40, 53, 64)
    );
    float cloudCover = getIslandCloudCover(seed);
    if (cloudMask > cloudCover) {
      return selectPlanetPalette4(
        clampFloat(cloudMask, 0.0f, 1.0f),
        makePlanetRgb(223, 224, 232),
        makePlanetRgb(163, 167, 194),
        makePlanetRgb(104, 111, 153),
        makePlanetRgb(64, 73, 115)
      );
    }
    if (islandMask > 0.58f) {
      return land;
    }
    if (islandMask > 0.52f) {
      return makePlanetRgb(146, 232, 192);
    }
    return ocean;
  }

  if (planetPresetEquals("no_atmosphere")) {
    float craterMask = samplePlanetCraterMask(longitude, latitude, flow, seed, 5.0f);
    PlanetRgb base = selectPlanetPalette3(
      clampFloat(n1 * 0.78f + n2 * 0.22f, 0.0f, 1.0f),
      makePlanetRgb(163, 167, 194),
      makePlanetRgb(76, 104, 133),
      makePlanetRgb(58, 63, 94)
    );
    if (craterMask > 0.42f) {
      return craterMask > 0.66f
        ? makePlanetRgb(58, 63, 94)
        : makePlanetRgb(76, 104, 133);
    }
    return base;
  }

  if (planetPresetEquals("gas_giant_1")) {
    float darkMask = samplePlanetGasMask(longitude, latitude, flow * 1.45f, seed, 9.0f, 1.0f);
    float warmMask = samplePlanetGasMask(longitude, latitude, flow, seed + 17U, 9.0f, 1.0f);
    PlanetRgb base = selectPlanetPalette4(
      clampFloat(darkMask, 0.0f, 1.0f),
      makePlanetRgb(59, 32, 39),
      makePlanetRgb(59, 32, 39),
      makePlanetRgb(0, 0, 0),
      makePlanetRgb(33, 24, 27)
    );
    float cloudCover = getGasGiantCloudCover(seed);
    if (warmMask > cloudCover) {
      return selectPlanetPalette4(
        clampFloat(warmMask * 0.8f, 0.0f, 1.0f),
        makePlanetRgb(240, 181, 65),
        makePlanetRgb(207, 117, 43),
        makePlanetRgb(171, 81, 48),
        makePlanetRgb(125, 56, 51)
      );
    }
    return base;
  }

  if (planetPresetEquals("gas_giant_2")) {
    float bandMask = samplePlanetGasMask(longitude, latitude, flow * 0.55f, seed, 10.1f, 0.89f);
    PlanetRgb base = selectPlanetPalette3(
      clampFloat(bandMask, 0.0f, 1.0f),
      makePlanetRgb(238, 195, 154),
      makePlanetRgb(217, 160, 102),
      makePlanetRgb(143, 86, 59)
    );
    if (bandMask > 0.63f) {
      return selectPlanetPalette3(
        clampFloat((bandMask - 0.63f) * 2.7f, 0.0f, 1.0f),
        makePlanetRgb(102, 57, 49),
        makePlanetRgb(69, 40, 60),
        makePlanetRgb(34, 32, 52)
      );
    }
    return base;
  }

  if (planetPresetEquals("ice_world")) {
    float lakeMask = samplePlanetLakeMask(longitude, latitude, flow, seed, 10.0f);
    float cloudMask = samplePlanetCloudMask(longitude, latitude, dx, flow, seed, 2.5f, 4.0f, 4);
    PlanetRgb base = selectPlanetPalette3(
      clampFloat(n1 * 0.72f + n2 * 0.28f, 0.0f, 1.0f),
      makePlanetRgb(250, 255, 255),
      makePlanetRgb(199, 212, 225),
      makePlanetRgb(146, 143, 184)
    );
    if (lakeMask > 0.55f) {
      PlanetRgb lake = selectPlanetPalette3(
        clampFloat(lakeMask * 0.9f, 0.0f, 1.0f),
        makePlanetRgb(79, 164, 184),
        makePlanetRgb(76, 104, 133),
        makePlanetRgb(58, 63, 94)
      );
      if (cloudMask <= 0.546f) {
        return lake;
      }
    }
    if (cloudMask > 0.546f) {
      return selectPlanetPalette4(
        clampFloat(cloudMask, 0.0f, 1.0f),
        makePlanetRgb(225, 242, 255),
        makePlanetRgb(192, 227, 255),
        makePlanetRgb(94, 112, 165),
        makePlanetRgb(64, 73, 115)
      );
    }
    return base;
  }

  if (planetPresetEquals("lava_world")) {
    float craterMask = samplePlanetCraterMask(longitude, latitude, flow, seed + 19U, 3.5f);
    float lavaMask = samplePlanetRiverMask(longitude, latitude, flow, seed + 29U, 10.0f);
    PlanetRgb base = selectPlanetPalette3(
      clampFloat(n1 * 0.72f + n2 * 0.28f, 0.0f, 1.0f),
      makePlanetRgb(143, 77, 87),
      makePlanetRgb(82, 51, 63),
      makePlanetRgb(61, 41, 54)
    );
    if (lavaMask > 0.58f) {
      return selectPlanetPalette3(
        clampFloat(lavaMask, 0.0f, 1.0f),
        makePlanetRgb(255, 137, 51),
        makePlanetRgb(230, 69, 57),
        makePlanetRgb(173, 47, 69)
      );
    }
    if (craterMask > 0.4f) {
      return craterMask > 0.66f
        ? makePlanetRgb(61, 41, 54)
        : makePlanetRgb(82, 51, 63);
    }
    return base;
  }

  return makePlanetRgb(120, 150, 180);
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
      float diskAlpha = clampFloat((disk - 0.05f) / 0.25f, 0.0f, 1.0f);
      if (!shouldDrawPlanetDirectPixel(x, y, diskAlpha)) {
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

PlanetBufferColor makePlanetBufferColor(float r, float g, float b, float a = 1.0f) {
  PlanetBufferColor color = { r, g, b, a };
  return color;
}

PlanetBufferColor applyActivePlanetColorVariant(const PlanetBufferColor& color);
void* allocatePlanetBuffer(size_t size);
bool ensurePlanetPreviewBuffer();

float modPlanetFloat(float value, float divisor) {
  return value - divisor * floorf(value / divisor);
}

float planetStep(float edge, float value) {
  if (value < edge) {
    return 0.0f;
  }
  return 1.0f;
}

void* allocatePlanetBuffer(size_t size) {
  if (size == 0) {
    return nullptr;
  }

  if (psramFound()) {
    void* buffer = ps_malloc(size);
    if (buffer != nullptr) {
      return buffer;
    }
  }

  return malloc(size);
}

bool ensurePlanetPreviewBuffer() {
  if (s_planetPreviewBuffer != nullptr) {
    return true;
  }

  size_t bytes = (size_t)kPlanetCanvasSize *
                 (size_t)kPlanetCanvasSize *
                 (size_t)kPlanetBufferChannels *
                 sizeof(float);
  s_planetPreviewBuffer = static_cast<float*>(allocatePlanetBuffer(bytes));
  if (s_planetPreviewBuffer == nullptr) {
    Serial.printf("planet preview buffer alloc failed: %u bytes\n", (unsigned int)bytes);
    return false;
  }
  memset(s_planetPreviewBuffer, 0, bytes);
  return true;
}

void clearPlanetPreviewBuffer() {
  if (!ensurePlanetPreviewBuffer()) {
    return;
  }
  size_t bytes = (size_t)kPlanetCanvasSize *
                 (size_t)kPlanetCanvasSize *
                 (size_t)kPlanetBufferChannels *
                 sizeof(float);
  memset(s_planetPreviewBuffer, 0, bytes);
}

void storePlanetPreviewBuffer(uint16_t* targetBuffer) {
  if (targetBuffer == nullptr || !ensurePlanetPreviewBuffer()) {
    return;
  }

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float red = clampFloat(s_planetPreviewBuffer[offset], 0.0f, 1.0f);
      float green = clampFloat(s_planetPreviewBuffer[offset + 1], 0.0f, 1.0f);
      float blue = clampFloat(s_planetPreviewBuffer[offset + 2], 0.0f, 1.0f);
      targetBuffer[y * kPlanetCanvasSize + x] = MatrixPanel_I2S_DMA::color565(
        (uint8_t)roundf(red * 255.0f),
        (uint8_t)roundf(green * 255.0f),
        (uint8_t)roundf(blue * 255.0f)
      );
    }
  }
}

bool shouldDrawPlanetDirectPixel(int x, int y, float alpha) {
  if (alpha <= 0.0f) {
    return false;
  }
  if (alpha >= 1.0f) {
    return true;
  }

  static const uint8_t kPlanetDirectDither4x4[16] = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 5
  };
  int index = ((y & 3) << 2) | (x & 3);
  float threshold = ((float)kPlanetDirectDither4x4[index] + 0.5f) / 16.0f;
  return alpha >= threshold;
}

void drawPlanetPreviewDirectPixel(int offset, const PlanetBufferColor& color, float alpha) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  if (display == nullptr) {
    return;
  }

  int pixelIndex = offset / kPlanetBufferChannels;
  int x = pixelIndex % kPlanetCanvasSize;
  int y = pixelIndex / kPlanetCanvasSize;
  float cover = clampFloat(alpha, 0.0f, 1.0f);
  if (!shouldDrawPlanetDirectPixel(x, y, cover)) {
    return;
  }

  PlanetBufferColor mappedColor = applyActivePlanetColorVariant(color);
  display->drawPixel(
    x,
    y,
    MatrixPanel_I2S_DMA::color565(
      (uint8_t)roundf(clampFloat(mappedColor.r, 0.0f, 1.0f) * 255.0f),
      (uint8_t)roundf(clampFloat(mappedColor.g, 0.0f, 1.0f) * 255.0f),
      (uint8_t)roundf(clampFloat(mappedColor.b, 0.0f, 1.0f) * 255.0f)
    )
  );
}

void blendPlanetPreviewPixel(int offset, const PlanetBufferColor& color, float alpha) {
  if (alpha <= 0.0f) {
    return;
  }
  if (s_planetPreviewDirectActive) {
    drawPlanetPreviewDirectPixel(offset, color, alpha);
    return;
  }
  if (!ensurePlanetPreviewBuffer()) {
    return;
  }

  PlanetBufferColor mappedColor = applyActivePlanetColorVariant(color);
  float blendAlpha = clampFloat(alpha, 0.0f, 1.0f);
  float inverseAlpha = 1.0f - blendAlpha;
  float currentRed = s_planetPreviewBuffer[offset];
  float currentGreen = s_planetPreviewBuffer[offset + 1];
  float currentBlue = s_planetPreviewBuffer[offset + 2];
  s_planetPreviewBuffer[offset] =
    clampFloat(currentRed * inverseAlpha + mappedColor.r * blendAlpha, 0.0f, 1.0f);
  s_planetPreviewBuffer[offset + 1] =
    clampFloat(currentGreen * inverseAlpha + mappedColor.g * blendAlpha, 0.0f, 1.0f);
  s_planetPreviewBuffer[offset + 2] =
    clampFloat(currentBlue * inverseAlpha + mappedColor.b * blendAlpha, 0.0f, 1.0f);
}

void coverPlanetPreviewPixel(int offset, const PlanetBufferColor& color, float alpha) {
  if (alpha <= 0.0f) {
    return;
  }
  if (s_planetPreviewDirectActive) {
    drawPlanetPreviewDirectPixel(offset, color, alpha);
    return;
  }
  if (!ensurePlanetPreviewBuffer()) {
    return;
  }

  PlanetBufferColor mappedColor = applyActivePlanetColorVariant(color);
  float coverAlpha = clampFloat(alpha, 0.0f, 1.0f);
  float inverseAlpha = 1.0f - coverAlpha;
  float currentRed = s_planetPreviewBuffer[offset];
  float currentGreen = s_planetPreviewBuffer[offset + 1];
  float currentBlue = s_planetPreviewBuffer[offset + 2];
  s_planetPreviewBuffer[offset] =
    clampFloat(currentRed * inverseAlpha + mappedColor.r * coverAlpha, 0.0f, 1.0f);
  s_planetPreviewBuffer[offset + 1] =
    clampFloat(currentGreen * inverseAlpha + mappedColor.g * coverAlpha, 0.0f, 1.0f);
  s_planetPreviewBuffer[offset + 2] =
    clampFloat(currentBlue * inverseAlpha + mappedColor.b * coverAlpha, 0.0f, 1.0f);
}

void setPlanetPreviewPixelOpaque(int offset, const PlanetBufferColor& color) {
  if (s_planetPreviewBuffer == nullptr) {
    return;
  }
  s_planetPreviewBuffer[offset] = clampFloat(color.r, 0.0f, 1.0f);
  s_planetPreviewBuffer[offset + 1] = clampFloat(color.g, 0.0f, 1.0f);
  s_planetPreviewBuffer[offset + 2] = clampFloat(color.b, 0.0f, 1.0f);
}

void flushPlanetPreviewBuffer() {
  if (!ensurePlanetPreviewBuffer()) {
    return;
  }
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  storePlanetPreviewBuffer(&DisplayManager::animationBuffer[0][0]);
  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      uint16_t color = DisplayManager::animationBuffer[y][x];
      if (color == 0) {
        continue;
      }
      display->drawPixel(x, y, color);
    }
  }
}

bool planetPresetEqualsValue(const char* left, const char* right) {
  return strcmp(left, right) == 0;
}

float resolvePlanetPresetRelativeScaleFor(const char* presetId) {
  if (planetPresetEqualsValue(presetId, "gas_giant_2")) {
    return 3.0f;
  }
  if (planetPresetEqualsValue(presetId, "black_hole") ||
      planetPresetEqualsValue(presetId, "star")) {
    return 2.0f;
  }
  return 1.0f;
}

float resolvePlanetPlaneSizeFor(const PlanetRenderFrame& frame, float planeScale) {
  return kPlanetPreviewDiameter * frame.sizeScale * planeScale /
         resolvePlanetPresetRelativeScaleFor(frame.presetId);
}

bool mapPlanetPreviewPixelToUv(
  const PlanetRenderFrame& frame,
  float planeScale,
  int x,
  int y,
  float& u,
  float& v
) {
  return mapPlanetPixelToUv(
    x,
    y,
    resolvePlanetPlaneSizeFor(frame, planeScale),
    frame.centerX,
    frame.centerY,
    u,
    v
  );
}

float convertPlanetShaderSeed(uint32_t seed) {
  return (float)(seed % 1000UL) / 100.0f;
}

void rgbBufferToHsv(float red, float green, float blue, float& hue, float& saturation, float& value) {
  float maxValue = fmaxf(red, fmaxf(green, blue));
  float minValue = fminf(red, fminf(green, blue));
  float delta = maxValue - minValue;

  hue = 0.0f;
  if (delta > 0.0f) {
    if (maxValue == red) {
      hue = modPlanetFloat((green - blue) / delta, 6.0f) / 6.0f;
    } else if (maxValue == green) {
      hue = (((blue - red) / delta) + 2.0f) / 6.0f;
    } else {
      hue = (((red - green) / delta) + 4.0f) / 6.0f;
    }
  }

  saturation = maxValue <= 0.0f ? 0.0f : delta / maxValue;
  value = maxValue;
}

PlanetBufferColor hsvToBufferRgb(float hue, float saturation, float value) {
  float normalizedHue = modPlanetFloat(hue, 1.0f);
  float clampedSaturation = clampFloat(saturation, 0.0f, 1.0f);
  float clampedValue = clampFloat(value, 0.0f, 1.0f);
  float segment = normalizedHue * 6.0f;
  float chroma = clampedValue * clampedSaturation;
  float xValue = chroma * (1.0f - fabsf(modPlanetFloat(segment, 2.0f) - 1.0f));
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

  return makePlanetBufferColor(red + match, green + match, blue + match, 1.0f);
}

PlanetBufferColor applyActivePlanetColorVariant(const PlanetBufferColor& color) {
  if (!s_planetColorVariantActive) {
    return color;
  }

  float hue = 0.0f;
  float saturation = 0.0f;
  float value = 0.0f;
  rgbBufferToHsv(color.r, color.g, color.b, hue, saturation, value);
  PlanetBufferColor remapped = hsvToBufferRgb(
    hue + s_activePlanetColorVariant.hueShift,
    clampFloat(saturation * s_activePlanetColorVariant.saturationScale, 0.0f, 1.0f),
    clampFloat(value * s_activePlanetColorVariant.valueScale, 0.0f, 1.0f)
  );
  return makePlanetBufferColor(
    clampFloat(mixFloat(remapped.r, s_activePlanetColorVariant.tint.r, s_activePlanetColorVariant.tintMix), 0.0f, 1.0f),
    clampFloat(mixFloat(remapped.g, s_activePlanetColorVariant.tint.g, s_activePlanetColorVariant.tintMix), 0.0f, 1.0f),
    clampFloat(mixFloat(remapped.b, s_activePlanetColorVariant.tint.b, s_activePlanetColorVariant.tintMix), 0.0f, 1.0f),
    color.a
  );
}

PlanetColorVariant buildPlanetColorVariant(const PlanetRenderFrame& frame) {
  PlanetColorVariant variant;
  char key[48];

  snprintf(key, sizeof(key), "%s_color_variant_hue", frame.presetId);
  variant.hueShift = seededPlanetRange(frame.colorSeed, key, -0.32f, 0.32f);
  snprintf(key, sizeof(key), "%s_color_variant_sat", frame.presetId);
  variant.saturationScale = seededPlanetRange(frame.colorSeed, key, 0.68f, 1.42f);
  snprintf(key, sizeof(key), "%s_color_variant_val", frame.presetId);
  variant.valueScale = seededPlanetRange(frame.colorSeed, key, 0.78f, 1.26f);
  snprintf(key, sizeof(key), "%s_color_variant_mix", frame.presetId);
  variant.tintMix = seededPlanetRange(frame.colorSeed, key, 0.04f, 0.26f);
  snprintf(key, sizeof(key), "%s_color_variant_r", frame.presetId);
  float tintR = seededPlanetRange(frame.colorSeed, key, 0.2f, 1.0f);
  snprintf(key, sizeof(key), "%s_color_variant_g", frame.presetId);
  float tintG = seededPlanetRange(frame.colorSeed, key, 0.2f, 1.0f);
  snprintf(key, sizeof(key), "%s_color_variant_b", frame.presetId);
  float tintB = seededPlanetRange(frame.colorSeed, key, 0.2f, 1.0f);
  variant.tint = makePlanetBufferColor(tintR, tintG, tintB, 1.0f);
  return variant;
}

void applyPlanetColorVariant(const PlanetRenderFrame& frame) {
  if (!ensurePlanetPreviewBuffer()) {
    return;
  }
  PlanetColorVariant variant = buildPlanetColorVariant(frame);
  for (int offset = 0;
       offset < kPlanetCanvasSize * kPlanetCanvasSize * kPlanetBufferChannels;
       offset += kPlanetBufferChannels) {
    float red = s_planetPreviewBuffer[offset];
    float green = s_planetPreviewBuffer[offset + 1];
    float blue = s_planetPreviewBuffer[offset + 2];
    if (red <= 0.0f && green <= 0.0f && blue <= 0.0f) {
      continue;
    }
    float hue = 0.0f;
    float saturation = 0.0f;
    float value = 0.0f;
    rgbBufferToHsv(red, green, blue, hue, saturation, value);
    PlanetBufferColor remapped = hsvToBufferRgb(
      hue + variant.hueShift,
      clampFloat(saturation * variant.saturationScale, 0.0f, 1.0f),
      clampFloat(value * variant.valueScale, 0.0f, 1.0f)
    );
    s_planetPreviewBuffer[offset] =
      clampFloat(mixFloat(remapped.r, variant.tint.r, variant.tintMix), 0.0f, 1.0f);
    s_planetPreviewBuffer[offset + 1] =
      clampFloat(mixFloat(remapped.g, variant.tint.g, variant.tintMix), 0.0f, 1.0f);
    s_planetPreviewBuffer[offset + 2] =
      clampFloat(mixFloat(remapped.b, variant.tint.b, variant.tintMix), 0.0f, 1.0f);
  }
}

float quantizePlanetUv(float value, float pixels) {
  float safePixels = pixels <= 0.0f ? 1.0f : pixels;
  return floorf(value * safePixels) / safePixels;
}

float tiledPlanetRand(float seed, float size, float x, float y, float tileX = 1.0f, float tileY = 1.0f) {
  float rounded = fmaxf(1.0f, roundf(size));
  float wrappedX = modPlanetFloat(x, tileX * rounded);
  float wrappedY = modPlanetFloat(y, tileY * rounded);
  return fractFloat(sinf(wrappedX * 12.9898f + wrappedY * 78.233f) * 15.5453f * seed);
}

float untiledPlanetRand(float seed, float x, float y) {
  return fractFloat(sinf(x * 12.9898f + y * 78.233f) * 15.5453f * seed);
}

float noisePlanetPreviewTiled(float x, float y, float seed, float size, float tileX = 1.0f, float tileY = 1.0f) {
  float ix = floorf(x);
  float iy = floorf(y);
  float fx = fractFloat(x);
  float fy = fractFloat(y);

  float a = tiledPlanetRand(seed, size, ix, iy, tileX, tileY);
  float b = tiledPlanetRand(seed, size, ix + 1.0f, iy, tileX, tileY);
  float c = tiledPlanetRand(seed, size, ix, iy + 1.0f, tileX, tileY);
  float d = tiledPlanetRand(seed, size, ix + 1.0f, iy + 1.0f, tileX, tileY);

  float cubicX = fx * fx * (3.0f - 2.0f * fx);
  float cubicY = fy * fy * (3.0f - 2.0f * fy);

  return mixFloat(a, b, cubicX) +
         (c - a) * cubicY * (1.0f - cubicX) +
         (d - b) * cubicX * cubicY;
}

float noisePlanetPreviewUntiled(float x, float y, float seed) {
  float ix = floorf(x);
  float iy = floorf(y);
  float fx = fractFloat(x);
  float fy = fractFloat(y);

  float a = untiledPlanetRand(seed, ix, iy);
  float b = untiledPlanetRand(seed, ix + 1.0f, iy);
  float c = untiledPlanetRand(seed, ix, iy + 1.0f);
  float d = untiledPlanetRand(seed, ix + 1.0f, iy + 1.0f);

  float cubicX = fx * fx * (3.0f - 2.0f * fx);
  float cubicY = fy * fy * (3.0f - 2.0f * fy);

  return mixFloat(a, b, cubicX) +
         (c - a) * cubicY * (1.0f - cubicX) +
         (d - b) * cubicX * cubicY;
}

int resolvePlanetDirectOctaves(int octaves) {
  int normalized = octaves < 1 ? 1 : octaves;
  if (!s_planetDirectRenderActive) {
    return normalized;
  }
  return normalized > kPlanetDirectMaxOctaves ? kPlanetDirectMaxOctaves : normalized;
}

float fbmPlanetPreviewTiledRaw(
  float x,
  float y,
  float seed,
  float size,
  int octaves,
  float tileX = 1.0f,
  float tileY = 1.0f
) {
  int effectiveOctaves = resolvePlanetDirectOctaves(octaves);
  float value = 0.0f;
  float scale = 0.5f;
  float coordX = x;
  float coordY = y;
  for (int index = 0; index < effectiveOctaves; index += 1) {
    value += noisePlanetPreviewTiled(coordX, coordY, seed, size, tileX, tileY) * scale;
    coordX *= 2.0f;
    coordY *= 2.0f;
    scale *= 0.5f;
  }

  return value;
}

float samplePlanetGenericFbmTiled(
  float x,
  float y,
  float seed,
  float size,
  int octaves,
  float tileX,
  float tileY
);

float fbmPlanetPreviewTiled(
  float x,
  float y,
  float seed,
  float size,
  int octaves,
  float tileX = 1.0f,
  float tileY = 1.0f
) {
  if (!s_planetDirectRenderActive || s_planetGenericFbmTileBuildDepth > 0) {
    return fbmPlanetPreviewTiledRaw(x, y, seed, size, resolvePlanetDirectOctaves(octaves), tileX, tileY);
  }
  return samplePlanetGenericFbmTiled(x, y, seed, size, octaves, tileX, tileY);
}

float fbmPlanetPreviewUntiled(float x, float y, float seed, int octaves) {
  int effectiveOctaves = resolvePlanetDirectOctaves(octaves);
  float value = 0.0f;
  float scale = 0.5f;
  float coordX = x;
  float coordY = y;
  for (int index = 0; index < effectiveOctaves; index += 1) {
    value += noisePlanetPreviewUntiled(coordX, coordY, seed) * scale;
    coordX *= 2.0f;
    coordY *= 2.0f;
    scale *= 0.5f;
  }

  return value;
}

float circleNoisePlanetCloud(float x, float y, float seed, float size) {
  float uvY = floorf(y);
  float shiftedX = x + uvY * 0.31f;
  float fx = fractFloat(shiftedX);
  float fy = fractFloat(y);
  float h = tiledPlanetRand(seed, size, floorf(shiftedX), uvY);
  float dx = fx - 0.25f - h * 0.5f;
  float dy = fy - 0.25f - h * 0.5f;
  float magnitude = sqrtf(dx * dx + dy * dy);
  float radius = h * 0.25f;
  return smoothstepFloat(0.0f, radius, magnitude * 0.75f);
}

float circleNoisePlanetCrater(float x, float y, float seed, float size) {
  float uvY = floorf(y);
  float shiftedX = x + uvY * 0.31f;
  float fx = fractFloat(shiftedX);
  float fy = fractFloat(y);
  float h = tiledPlanetRand(seed, size, floorf(shiftedX), uvY);
  float dx = fx - 0.25f - h * 0.5f;
  float dy = fy - 0.25f - h * 0.5f;
  float magnitude = sqrtf(dx * dx + dy * dy);
  float radius = h * 0.25f;
  return smoothstepFloat(radius - 0.1f * radius, radius, magnitude);
}

void cellsPlanetHash2(float x, float y, float& outX, float& outY) {
  float r = 523.0f * sinf(x * 53.3158f + y * 43.6143f);
  outX = fractFloat(15.32354f * r);
  outY = fractFloat(17.25865f * r);
}

float cellsPlanetValue(float x, float y, float numCells, float tiles) {
  float scaledX = x * numCells;
  float scaledY = y * numCells;
  float minDistance = 1.0e10f;

  for (int xo = -1; xo <= 1; xo += 1) {
    for (int yo = -1; yo <= 1; yo += 1) {
      float tileX = floorf(scaledX) + (float)xo;
      float tileY = floorf(scaledY) + (float)yo;
      float hashX = 0.0f;
      float hashY = 0.0f;
      cellsPlanetHash2(
        modPlanetFloat(tileX, numCells / tiles),
        modPlanetFloat(tileY, numCells / tiles),
        hashX,
        hashY
      );
      float pointX = scaledX - tileX - hashX;
      float pointY = scaledY - tileY - hashY;
      float distanceValue = pointX * pointX + pointY * pointY;
      if (distanceValue < minDistance) {
        minDistance = distanceValue;
      }
    }
  }

  return sqrtf(minDistance);
}

float samplePlanetRectBilinear(
  const uint8_t* tile,
  int width,
  int height,
  float minX,
  float maxX,
  float minY,
  float maxY,
  float sampleX,
  float sampleY,
  float maxValue
) {
  if (tile == nullptr || width <= 0 || height <= 0) {
    return 0.0f;
  }
  if (maxX <= minX || maxY <= minY) {
    return decodePlanetTileValue(tile[0], maxValue);
  }

  float normalizedX = clampFloat((sampleX - minX) / (maxX - minX), 0.0f, 1.0f);
  float normalizedY = clampFloat((sampleY - minY) / (maxY - minY), 0.0f, 1.0f);
  float texX = normalizedX * (float)(width - 1);
  float texY = normalizedY * (float)(height - 1);
  int x0 = clampInt((int)floorf(texX), 0, width - 1);
  int y0 = clampInt((int)floorf(texY), 0, height - 1);
  int x1 = clampInt(x0 + 1, 0, width - 1);
  int y1 = clampInt(y0 + 1, 0, height - 1);
  float fx = texX - (float)x0;
  float fy = texY - (float)y0;

  float v00 = decodePlanetTileValue(tile[y0 * width + x0], maxValue);
  float v10 = decodePlanetTileValue(tile[y0 * width + x1], maxValue);
  float v01 = decodePlanetTileValue(tile[y1 * width + x0], maxValue);
  float v11 = decodePlanetTileValue(tile[y1 * width + x1], maxValue);
  float top = mixFloat(v00, v10, fx);
  float bottom = mixFloat(v01, v11, fx);
  return mixFloat(top, bottom, fy);
}

bool planetUntiledFbmRectMatches(
  const PlanetUntiledFbmRectCache& cache,
  float seed,
  float size,
  int octaves,
  float minX,
  float maxX,
  float minY,
  float maxY
) {
  return cache.valid &&
         cache.seedBits == planetFloatBits(seed) &&
         cache.sizeBits == planetFloatBits(size) &&
         cache.minXBits == planetFloatBits(minX) &&
         cache.maxXBits == planetFloatBits(maxX) &&
         cache.minYBits == planetFloatBits(minY) &&
         cache.maxYBits == planetFloatBits(maxY) &&
         cache.octaves == octaves;
}

bool ensurePlanetUntiledFbmRectCacheMetadata() {
  if (s_planetUntiledFbmRectCaches != nullptr) {
    return true;
  }

  s_planetUntiledFbmRectCaches = static_cast<PlanetUntiledFbmRectCache*>(
    allocatePlanetBuffer(sizeof(PlanetUntiledFbmRectCache) * kPlanetUntiledFbmRectSlotCount)
  );
  if (s_planetUntiledFbmRectCaches == nullptr) {
    s_planetUntiledFbmRectCacheDisabled = true;
    return false;
  }
  memset(
    s_planetUntiledFbmRectCaches,
    0,
    sizeof(PlanetUntiledFbmRectCache) * kPlanetUntiledFbmRectSlotCount
  );
  return true;
}

PlanetUntiledFbmRectCache* acquirePlanetUntiledFbmRectCache(
  float seed,
  float size,
  int octaves,
  float minX,
  float maxX,
  float minY,
  float maxY
) {
  if (s_planetUntiledFbmRectCacheDisabled) {
    return nullptr;
  }
  if (!ensurePlanetUntiledFbmRectCacheMetadata()) {
    return nullptr;
  }

  int effectiveOctaves = resolvePlanetDirectOctaves(octaves);
  s_planetUntiledFbmRectUseTick += 1UL;
  PlanetUntiledFbmRectCache* reuseSlot = nullptr;
  for (int index = 0; index < kPlanetUntiledFbmRectSlotCount; index += 1) {
    PlanetUntiledFbmRectCache& cache = s_planetUntiledFbmRectCaches[index];
    if (planetUntiledFbmRectMatches(cache, seed, size, effectiveOctaves, minX, maxX, minY, maxY)) {
      cache.useTick = s_planetUntiledFbmRectUseTick;
      return &cache;
    }
    if (reuseSlot == nullptr && !cache.valid) {
      reuseSlot = &cache;
    }
  }

  if (reuseSlot == nullptr) {
    reuseSlot = &s_planetUntiledFbmRectCaches[0];
    for (int index = 1; index < kPlanetUntiledFbmRectSlotCount; index += 1) {
      if (s_planetUntiledFbmRectCaches[index].useTick < reuseSlot->useTick) {
        reuseSlot = &s_planetUntiledFbmRectCaches[index];
      }
    }
  }

  if (reuseSlot->values == nullptr) {
    reuseSlot->values = static_cast<uint8_t*>(
      allocatePlanetBuffer((size_t)kPlanetUntiledFbmRectWidth * (size_t)kPlanetUntiledFbmRectHeight)
    );
    if (reuseSlot->values == nullptr) {
      s_planetUntiledFbmRectCacheDisabled = true;
      return nullptr;
    }
  }

  for (int y = 0; y < kPlanetUntiledFbmRectHeight; y += 1) {
    float sampleY = minY + (((float)y + 0.5f) / (float)kPlanetUntiledFbmRectHeight) * (maxY - minY);
    for (int x = 0; x < kPlanetUntiledFbmRectWidth; x += 1) {
      float sampleX = minX + (((float)x + 0.5f) / (float)kPlanetUntiledFbmRectWidth) * (maxX - minX);
      float value = fbmPlanetPreviewUntiled(sampleX, sampleY, seed, effectiveOctaves);
      reuseSlot->values[y * kPlanetUntiledFbmRectWidth + x] =
        quantizePlanetTileValue(value, 1.0f);
    }
  }

  reuseSlot->valid = true;
  reuseSlot->seedBits = planetFloatBits(seed);
  reuseSlot->sizeBits = planetFloatBits(size);
  reuseSlot->minXBits = planetFloatBits(minX);
  reuseSlot->maxXBits = planetFloatBits(maxX);
  reuseSlot->minYBits = planetFloatBits(minY);
  reuseSlot->maxYBits = planetFloatBits(maxY);
  reuseSlot->octaves = effectiveOctaves;
  reuseSlot->useTick = s_planetUntiledFbmRectUseTick;
  return reuseSlot;
}

float samplePlanetUntiledFbmRectCached(
  float x,
  float y,
  float seed,
  float size,
  int octaves,
  float minX,
  float maxX,
  float minY,
  float maxY
) {
  PlanetUntiledFbmRectCache* cache = acquirePlanetUntiledFbmRectCache(
    seed,
    size,
    octaves,
    minX,
    maxX,
    minY,
    maxY
  );
  if (cache == nullptr || cache->values == nullptr) {
    return fbmPlanetPreviewUntiled(x, y, seed, resolvePlanetDirectOctaves(octaves));
  }

  return samplePlanetRectBilinear(
    cache->values,
    kPlanetUntiledFbmRectWidth,
    kPlanetUntiledFbmRectHeight,
    minX,
    maxX,
    minY,
    maxY,
    x,
    y,
    1.0f
  );
}

bool ensurePlanetStarCellsCacheMetadata() {
  if (s_planetStarCellsTileCaches != nullptr) {
    return true;
  }

  s_planetStarCellsTileCaches = static_cast<PlanetStarCellsTileCache*>(
    allocatePlanetBuffer(sizeof(PlanetStarCellsTileCache) * 2U)
  );
  if (s_planetStarCellsTileCaches == nullptr) {
    s_planetStarCellsTileCacheDisabled = true;
    return false;
  }
  memset(s_planetStarCellsTileCaches, 0, sizeof(PlanetStarCellsTileCache) * 2U);
  return true;
}

bool ensurePlanetStarCellsTileStorage(PlanetStarCellsTileCache& cache) {
  if (cache.values != nullptr) {
    return true;
  }

  cache.values = static_cast<uint8_t*>(
    allocatePlanetBuffer((size_t)kPlanetStarCellsTileWidth * (size_t)kPlanetStarCellsTileHeight)
  );
  if (cache.values == nullptr) {
    s_planetStarCellsTileCacheDisabled = true;
    return false;
  }
  memset(cache.values, 0, (size_t)kPlanetStarCellsTileWidth * (size_t)kPlanetStarCellsTileHeight);
  return true;
}

bool preparePlanetStarCellsTile(PlanetStarCellsTileCache& cache, int numCells, int tiles) {
  if (s_planetStarCellsTileCacheDisabled) {
    return false;
  }
  if (!ensurePlanetStarCellsTileStorage(cache)) {
    return false;
  }
  if (cache.valid && cache.numCells == numCells && cache.tiles == tiles) {
    return true;
  }

  for (int y = 0; y < kPlanetStarCellsTileHeight; y += 1) {
    float sampleY = ((float)y + 0.5f) / (float)kPlanetStarCellsTileHeight * kPlanetStarCellsTilePeriod;
    for (int x = 0; x < kPlanetStarCellsTileWidth; x += 1) {
      float sampleX = ((float)x + 0.5f) / (float)kPlanetStarCellsTileWidth * kPlanetStarCellsTilePeriod;
      float value = cellsPlanetValue(sampleX, sampleY, (float)numCells, (float)tiles);
      cache.values[y * kPlanetStarCellsTileWidth + x] = quantizePlanetTileValue(value, 1.0f);
    }
  }

  cache.valid = true;
  cache.numCells = numCells;
  cache.tiles = tiles;
  return true;
}

float samplePlanetStarCellsCached(float x, float y, int numCells, int tiles) {
  if (s_planetStarCellsTileCacheDisabled || !ensurePlanetStarCellsCacheMetadata()) {
    return cellsPlanetValue(x, y, (float)numCells, (float)tiles);
  }
  int slotIndex = numCells <= 10 ? 0 : 1;
  PlanetStarCellsTileCache& cache = s_planetStarCellsTileCaches[slotIndex];
  if (!preparePlanetStarCellsTile(cache, numCells, tiles) || cache.values == nullptr) {
    return cellsPlanetValue(x, y, (float)numCells, (float)tiles);
  }

  return samplePlanetTileBilinear(
    cache.values,
    kPlanetStarCellsTileWidth,
    kPlanetStarCellsTileHeight,
    kPlanetStarCellsTilePeriod,
    kPlanetStarCellsTilePeriod,
    x,
    y,
    1.0f
  );
}

float computePlanetStarBlobPattern(
  const PlanetRenderFrame& frame,
  const PlanetStarBlobLayer& layer,
  float circleUvX,
  float circleUvY
) {
  float value = 0.0f;
  for (int index = 0; index < 15; index += 1) {
    float randomValue = tiledPlanetRand(frame.shaderSeed, layer.size, (float)index, 0.0f);
    float invert = 1.0f / layer.circleAmount;
    float adjustedX = circleUvX + randomValue;
    if (modPlanetFloat(circleUvY, invert * 2.0f) < invert) {
      adjustedX += invert * 0.5f;
    }
    float randCoordX = floorf(adjustedX * layer.circleAmount) / layer.circleAmount;
    float randCoordY = floorf(circleUvY * layer.circleAmount) / layer.circleAmount;
    float localX = modPlanetFloat(adjustedX, invert) * layer.circleAmount;
    float localY = modPlanetFloat(circleUvY, invert) * layer.circleAmount;
    float radius = tiledPlanetRand(frame.shaderSeed, layer.size, randCoordX, randCoordY);
    radius = clampFloat(radius, invert, 1.0f - invert);
    float circleDistance = distancePlanet(localX, localY, radius, radius);
    float circleAlpha = smoothstepFloat(
      circleDistance,
      circleDistance + 0.5f,
      invert * layer.circleSize *
        tiledPlanetRand(frame.shaderSeed, layer.size, randCoordX * 1.5f, randCoordY * 1.5f)
    );
    value += circleAlpha;
  }
  return value;
}

bool ensurePlanetStarBlobCacheMetadata() {
  if (s_planetStarBlobTileCache != nullptr) {
    return true;
  }

  s_planetStarBlobTileCache = static_cast<PlanetStarBlobTileCache*>(
    allocatePlanetBuffer(sizeof(PlanetStarBlobTileCache))
  );
  if (s_planetStarBlobTileCache == nullptr) {
    s_planetStarBlobTileCacheDisabled = true;
    return false;
  }
  memset(s_planetStarBlobTileCache, 0, sizeof(PlanetStarBlobTileCache));
  return true;
}

bool ensurePlanetStarBlobTileStorage() {
  if (!ensurePlanetStarBlobCacheMetadata()) {
    return false;
  }
  if (s_planetStarBlobTileCache->values != nullptr) {
    return true;
  }

  s_planetStarBlobTileCache->values = static_cast<uint8_t*>(
    allocatePlanetBuffer((size_t)kPlanetStarBlobTileWidth * (size_t)kPlanetStarBlobTileHeight)
  );
  if (s_planetStarBlobTileCache->values == nullptr) {
    s_planetStarBlobTileCacheDisabled = true;
    return false;
  }
  memset(
    s_planetStarBlobTileCache->values,
    0,
    (size_t)kPlanetStarBlobTileWidth * (size_t)kPlanetStarBlobTileHeight
  );
  return true;
}

bool preparePlanetStarBlobTile(const PlanetRenderFrame& frame, const PlanetStarBlobLayer& layer) {
  if (s_planetStarBlobTileCacheDisabled) {
    return false;
  }
  if (!ensurePlanetStarBlobTileStorage()) {
    return false;
  }

  uint32_t seedBits = planetFloatBits(frame.shaderSeed);
  uint32_t sizeBits = planetFloatBits(layer.size);
  uint32_t circleAmountBits = planetFloatBits(layer.circleAmount);
  uint32_t circleSizeBits = planetFloatBits(layer.circleSize);
  if (s_planetStarBlobTileCache->valid &&
      s_planetStarBlobTileCache->seedBits == seedBits &&
      s_planetStarBlobTileCache->sizeBits == sizeBits &&
      s_planetStarBlobTileCache->circleAmountBits == circleAmountBits &&
      s_planetStarBlobTileCache->circleSizeBits == circleSizeBits) {
    return true;
  }

  for (int y = 0; y < kPlanetStarBlobTileHeight; y += 1) {
    float circleUvY =
      ((float)y + 0.5f) / (float)kPlanetStarBlobTileHeight * kPlanetStarBlobTilePeriod;
    for (int x = 0; x < kPlanetStarBlobTileWidth; x += 1) {
      float circleUvX =
        ((float)x + 0.5f) / (float)kPlanetStarBlobTileWidth * kPlanetStarBlobTilePeriod;
      float value = computePlanetStarBlobPattern(frame, layer, circleUvX, circleUvY);
      s_planetStarBlobTileCache->values[y * kPlanetStarBlobTileWidth + x] =
        quantizePlanetTileValue(value, 15.0f);
    }
  }

  s_planetStarBlobTileCache->valid = true;
  s_planetStarBlobTileCache->seedBits = seedBits;
  s_planetStarBlobTileCache->sizeBits = sizeBits;
  s_planetStarBlobTileCache->circleAmountBits = circleAmountBits;
  s_planetStarBlobTileCache->circleSizeBits = circleSizeBits;
  return true;
}

bool samplePlanetStarBlobPatternCached(
  const PlanetRenderFrame& frame,
  const PlanetStarBlobLayer& layer,
  float circleUvX,
  float circleUvY,
  float& outValue
) {
  if (!preparePlanetStarBlobTile(frame, layer) || s_planetStarBlobTileCache == nullptr ||
      s_planetStarBlobTileCache->values == nullptr) {
    return false;
  }

  outValue = samplePlanetTileBilinear(
    s_planetStarBlobTileCache->values,
    kPlanetStarBlobTileWidth,
    kPlanetStarBlobTileHeight,
    kPlanetStarBlobTilePeriod,
    kPlanetStarBlobTilePeriod,
    circleUvX,
    circleUvY,
    15.0f
  );
  return true;
}

void spherifyPlanetUv(float x, float y, float& outX, float& outY) {
  float centeredX = x * 2.0f - 1.0f;
  float centeredY = y * 2.0f - 1.0f;
  float dotValue = centeredX * centeredX + centeredY * centeredY;
  float z = sqrtf(fmaxf(0.0f, 1.0f - dotValue));
  float divisor = z + 1.0f;
  outX = centeredX / divisor * 0.5f + 0.5f;
  outY = centeredY / divisor * 0.5f + 0.5f;
}

float loopPlanetTimeFromMultiplier(float progress, float size, float timeSpeed, float motionFactor) {
  return progress * motionFactor * fmaxf(1.0f, roundf(size)) * 2.0f / timeSpeed;
}

float loopPlanetTimeFromRotation(float progress, float timeSpeed, float motionFactor) {
  return progress * motionFactor * kPlanetTwoPi / timeSpeed;
}

void mapPlanetColorArray(
  const PlanetBufferColor* source,
  PlanetBufferColor* target,
  int count
) {
  for (int index = 0; index < count; index += 1) {
    target[index] = applyActivePlanetColorVariant(source[index]);
  }
}

int wrapPlanetRandCacheIndex(int value, int period) {
  int wrapped = value % period;
  if (wrapped < 0) {
    wrapped += period;
  }
  return wrapped;
}

bool buildPlanetTiledRandCache(
  PlanetTiledRandCache& cache,
  float seed,
  float size,
  float tileX,
  float tileY
) {
  uint32_t seedBits = planetFloatBits(seed);
  uint32_t sizeBits = planetFloatBits(size);
  uint32_t tileXBits = planetFloatBits(tileX);
  uint32_t tileYBits = planetFloatBits(tileY);
  int width = clampInt((int)roundf(fmaxf(1.0f, roundf(size)) * tileX), 1, kPlanetTerranWetRandCacheMax);
  int height = clampInt((int)roundf(fmaxf(1.0f, roundf(size)) * tileY), 1, kPlanetTerranWetRandCacheMax);
  if (width * height > kPlanetTerranWetRandCacheMax) {
    Serial.printf("terran wet rand cache overflow: %d x %d\n", width, height);
    cache.valid = false;
    return false;
  }

  if (cache.valid &&
      cache.seedBits == seedBits &&
      cache.sizeBits == sizeBits &&
      cache.tileXBits == tileXBits &&
      cache.tileYBits == tileYBits) {
    return true;
  }

  for (int y = 0; y < height; y += 1) {
    for (int x = 0; x < width; x += 1) {
      cache.values[y * width + x] = tiledPlanetRand(seed, size, (float)x, (float)y, tileX, tileY);
    }
  }

  cache.valid = true;
  cache.seedBits = seedBits;
  cache.sizeBits = sizeBits;
  cache.tileXBits = tileXBits;
  cache.tileYBits = tileYBits;
  cache.width = width;
  cache.height = height;
  return true;
}

float samplePlanetTiledRandCache(const PlanetTiledRandCache& cache, int x, int y) {
  int wrappedX = wrapPlanetRandCacheIndex(x, cache.width);
  int wrappedY = wrapPlanetRandCacheIndex(y, cache.height);
  return cache.values[wrappedY * cache.width + wrappedX];
}

float noisePlanetPreviewTiledCached(float x, float y, const PlanetTiledRandCache& cache) {
  float ixFloat = floorf(x);
  float iyFloat = floorf(y);
  int ix = (int)ixFloat;
  int iy = (int)iyFloat;
  float fx = fractFloat(x);
  float fy = fractFloat(y);

  float a = samplePlanetTiledRandCache(cache, ix, iy);
  float b = samplePlanetTiledRandCache(cache, ix + 1, iy);
  float c = samplePlanetTiledRandCache(cache, ix, iy + 1);
  float d = samplePlanetTiledRandCache(cache, ix + 1, iy + 1);

  float cubicX = fx * fx * (3.0f - 2.0f * fx);
  float cubicY = fy * fy * (3.0f - 2.0f * fy);

  return mixFloat(a, b, cubicX) +
         (c - a) * cubicY * (1.0f - cubicX) +
         (d - b) * cubicX * cubicY;
}

float fbmPlanetPreviewTiledCached(
  float x,
  float y,
  int octaves,
  const PlanetTiledRandCache& cache
) {
  float value = 0.0f;
  float scale = 0.5f;
  float coordX = x;
  float coordY = y;

  for (int index = 0; index < octaves; index += 1) {
    value += noisePlanetPreviewTiledCached(coordX, coordY, cache) * scale;
    coordX *= 2.0f;
    coordY *= 2.0f;
    scale *= 0.5f;
  }

  return value;
}

float circleNoisePlanetCloudCached(float x, float y, const PlanetTiledRandCache& cache) {
  float uvYFloat = floorf(y);
  int uvY = (int)uvYFloat;
  float shiftedX = x + uvYFloat * 0.31f;
  float fx = fractFloat(shiftedX);
  float fy = fractFloat(y);
  float h = samplePlanetTiledRandCache(cache, (int)floorf(shiftedX), uvY);
  float dx = fx - 0.25f - h * 0.5f;
  float dy = fy - 0.25f - h * 0.5f;
  float magnitude = sqrtf(dx * dx + dy * dy);
  float radius = h * 0.25f;
  return smoothstepFloat(0.0f, radius, magnitude * 0.75f);
}

float circleNoisePlanetCraterCached(float x, float y, const PlanetTiledRandCache& cache) {
  float uvYFloat = floorf(y);
  int uvY = (int)uvYFloat;
  float shiftedX = x + uvYFloat * 0.31f;
  float fx = fractFloat(shiftedX);
  float fy = fractFloat(y);
  float h = samplePlanetTiledRandCache(cache, (int)floorf(shiftedX), uvY);
  float dx = fx - 0.25f - h * 0.5f;
  float dy = fy - 0.25f - h * 0.5f;
  float magnitude = sqrtf(dx * dx + dy * dy);
  float radius = h * 0.25f;
  return smoothstepFloat(radius - 0.1f * radius, radius, magnitude);
}

bool ensurePlanetTerranWetStaticCache(const PlanetRenderFrame& frame) {
  if (s_planetTerranWetStaticCache.valid &&
      s_planetTerranWetStaticCache.sizeScale == frame.sizeScale &&
      s_planetTerranWetStaticCache.pixels == frame.pixels &&
      s_planetTerranWetStaticCache.centerX == frame.centerX &&
      s_planetTerranWetStaticCache.centerY == frame.centerY &&
      s_planetTerranWetStaticCache.pixelsData != nullptr) {
    return true;
  }

  float pixels = (float)frame.pixels;
  int activeCount = 0;
  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, 1.0f, x, y, u, v)) {
        continue;
      }
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      if (planetStep(distancePlanet(pixelU, pixelV, 0.5f, 0.5f), 0.49999f) == 0.0f) {
        continue;
      }
      activeCount += 1;
    }
  }

  size_t bytes = (size_t)activeCount * sizeof(PlanetTerranWetStaticPixel);
  PlanetTerranWetStaticPixel* pixelsData = nullptr;
  if (bytes > 0U) {
    pixelsData = static_cast<PlanetTerranWetStaticPixel*>(allocatePlanetBuffer(bytes));
    if (pixelsData == nullptr) {
      Serial.printf("terran wet static cache alloc failed: %u bytes\n", (unsigned int)bytes);
      return false;
    }
  }

  if (s_planetTerranWetStaticCache.pixelsData != nullptr) {
    free(s_planetTerranWetStaticCache.pixelsData);
  }

  int index = 0;
  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, 1.0f, x, y, u, v)) {
        continue;
      }
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      if (planetStep(distancePlanet(pixelU, pixelV, 0.5f, 0.5f), 0.49999f) == 0.0f) {
        continue;
      }

      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(pixelU, pixelV, sphereX, sphereY);

      PlanetTerranWetStaticPixel& pixel = pixelsData[index];
      pixel.offset = (uint16_t)((y * kPlanetCanvasSize + x) * kPlanetBufferChannels);
      pixel.pixelU = pixelU;
      pixel.pixelV = pixelV;
      pixel.sphereX = sphereX;
      pixel.sphereY = sphereY;
      pixel.dith = modPlanetFloat(pixelU + v, 2.0f / pixels) <= 1.0f / pixels;
      index += 1;
    }
  }

  s_planetTerranWetStaticCache.valid = true;
  s_planetTerranWetStaticCache.sizeScale = frame.sizeScale;
  s_planetTerranWetStaticCache.pixels = frame.pixels;
  s_planetTerranWetStaticCache.centerX = frame.centerX;
  s_planetTerranWetStaticCache.centerY = frame.centerY;
  s_planetTerranWetStaticCache.activeCount = activeCount;
  s_planetTerranWetStaticCache.pixelsData = pixelsData;
  return true;
}

float planetBackgroundStarExclusionRadius(const PlanetRenderFrame& frame) {
  if (planetPresetEqualsValue(frame.presetId, "galaxy")) {
    return 30.0f;
  }
  if (planetPresetEqualsValue(frame.presetId, "star")) {
    return 28.0f;
  }
  if (planetPresetEqualsValue(frame.presetId, "black_hole")) {
    return 29.0f;
  }

  float radius = kPlanetPreviewDiameter * frame.sizeScale * 0.5f + 5.0f;
  if (planetPresetEqualsValue(frame.presetId, "gas_giant_2")) {
    radius += 4.0f;
  }
  return clampFloat(radius, 22.0f, 30.0f);
}

void renderPlanetBackgroundStars(const PlanetRenderFrame& frame) {
  if (!ensurePlanetPreviewBuffer()) {
    return;
  }
  uint32_t state = frame.seed ^ hashPlanetString("planet_starfield_sparse");
  float exclusionRadius = planetBackgroundStarExclusionRadius(frame);
  int attempts = 0;
  int placed = 0;

  while (placed < 10 && attempts < 400) {
    attempts += 1;
    int x = (int)floorf(nextPlanetSeededRandom(state) * (float)kPlanetCanvasSize);
    int y = (int)floorf(nextPlanetSeededRandom(state) * (float)kPlanetCanvasSize);
    float dx = (float)x - frame.centerX;
    float dy = (float)y - frame.centerY;
    if (sqrtf(dx * dx + dy * dy) < exclusionRadius) {
      continue;
    }

    PlanetBufferColor starColor = nextPlanetSeededRandom(state) > 0.45f
      ? makePlanetBufferColor(1.0f, 1.0f, 1.0f, 1.0f)
      : makePlanetBufferColor(1.0f, 0.937255f, 0.619608f, 1.0f);
    float alpha = 0.44f + nextPlanetSeededRandom(state) * 0.24f;
    blendPlanetPreviewPixel((y * kPlanetCanvasSize + x) * kPlanetBufferChannels, starColor, alpha);
    placed += 1;
  }
}

PlanetRenderFrame buildPlanetRenderFrame(unsigned long now) {
  PlanetRenderFrame frame;
  frame.presetId = s_planetConfig.preset;
  frame.seed = s_planetConfig.seed;
  frame.colorSeed = s_planetConfig.colorSeed;
  frame.centerX = (float)s_planetConfig.planetX;
  frame.centerY = (float)s_planetConfig.planetY;
  frame.progress = resolvePlanetPlaybackPhase(now);
  frame.motionFactor = planetFlowSign();
  frame.spinFactor = planetFlowSign();
  frame.spinAngle = frame.progress * frame.spinFactor * kPlanetTwoPi;
  frame.sizeScale = resolvePlanetSizeScaleFor(s_planetConfig.preset, s_planetConfig.size);
  frame.shaderSeed = convertPlanetShaderSeed(s_planetConfig.seed);
  frame.pixels = kPlanetPreviewPixels;
  frame.dither = true;
  return frame;
}

void renderPlanetUnderBuffer(const PlanetRenderFrame& frame, const PlanetUnderLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      bool dith = modPlanetFloat(pixelU + v, 2.0f / pixels) <= 1.0f / pixels;
      float dLightBase = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float dCircle = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float alphaCircle = planetStep(dCircle, 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }

      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(pixelU, pixelV, sphereX, sphereY);
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(sphereX, sphereY, layer.rotationOffset, rotatedX, rotatedY);

      float dLight = dLightBase;
      dLight += fbmPlanetPreviewTiled(
        rotatedX * layer.size + time * layer.timeSpeed,
        rotatedY * layer.size,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      ) * 0.3f;

      float ditherBorder = 1.0f / pixels * layer.ditherSize;
      PlanetBufferColor color = layer.colors[0];
      if (dLight > layer.lightBorder1) {
        color = layer.colors[1];
        if (dLight < layer.lightBorder1 + ditherBorder &&
            (dith || !frame.dither)) {
          color = layer.colors[0];
        }
      }
      if (dLight > layer.lightBorder2) {
        color = layer.colors[2];
        if (dLight < layer.lightBorder2 + ditherBorder &&
            (dith || !frame.dither)) {
          color = layer.colors[1];
        }
      }

      blendPlanetPreviewPixel(offset, color, alphaCircle * color.a);
    }
  }
}

void renderLandMassBuffer(const PlanetRenderFrame& frame, const PlanetLandMassLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float dLightBase = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float dCircle = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float alphaCircle = planetStep(dCircle, 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }

      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);

      float baseX = sphereX * layer.size + time * layer.timeSpeed;
      float baseY = sphereY * layer.size;
      float fbm1 = fbmPlanetPreviewTiled(baseX, baseY, frame.shaderSeed, layer.size, layer.octaves, 2.0f, 1.0f);
      float fbm2Value = fbmPlanetPreviewTiled(
        baseX - layer.lightOriginX * fbm1,
        baseY - layer.lightOriginY * fbm1,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );
      float fbm3Value = fbmPlanetPreviewTiled(
        baseX - layer.lightOriginX * 1.5f * fbm1,
        baseY - layer.lightOriginY * 1.5f * fbm1,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );
      float fbm4Value = fbmPlanetPreviewTiled(
        baseX - layer.lightOriginX * 2.0f * fbm1,
        baseY - layer.lightOriginY * 2.0f * fbm1,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );

      if (dLightBase < layer.lightBorder1) {
        fbm4Value *= 0.9f;
      }
      if (dLightBase > layer.lightBorder1) {
        fbm2Value *= 1.05f;
        fbm3Value *= 1.05f;
        fbm4Value *= 1.05f;
      }
      if (dLightBase > layer.lightBorder2) {
        fbm2Value *= 1.3f;
        fbm3Value *= 1.4f;
        fbm4Value *= 1.8f;
      }

      float lightValue = powf(dLightBase, 2.0f) * 0.1f;
      PlanetBufferColor color = layer.colors[3];
      if (fbm4Value + lightValue < fbm1) {
        color = layer.colors[2];
      }
      if (fbm3Value + lightValue < fbm1) {
        color = layer.colors[1];
      }
      if (fbm2Value + lightValue < fbm1) {
        color = layer.colors[0];
      }

      float alpha = planetStep(layer.landCutoff, fbm1) * alphaCircle;
      coverPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderCloudsBuffer(const PlanetRenderFrame& frame, const PlanetCloudLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);
  float timeOffset = time * layer.timeSpeed;

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float dLight = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float dCenter = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float alphaCircle = planetStep(dCenter, 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }

      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);
      float sphereCloudY = sphereY + smoothstepFloat(0.0f, layer.cloudCurve, fabsf(sphereX - 0.4f));
      sphereCloudY *= layer.stretch;

      float cloudNoise = 0.0f;
      for (int index = 0; index < 9; index += 1) {
        cloudNoise += circleNoisePlanetCloud(
          sphereX * layer.size * 0.3f + (float)index + 11.0f + timeOffset,
          sphereCloudY * layer.size * 0.3f,
          frame.shaderSeed,
          layer.size
        );
      }

      float cloudValue = fbmPlanetPreviewTiled(
        sphereX * layer.size + cloudNoise + timeOffset,
        sphereCloudY * layer.size,
        frame.shaderSeed,
        layer.size,
        layer.octaves
      );

      PlanetBufferColor color = layer.colors[0];
      if (cloudValue < layer.cloudCover + 0.03f) {
        color = layer.colors[1];
      }
      if (dLight + cloudValue * 0.2f > layer.lightBorder1) {
        color = layer.colors[2];
      }
      if (dLight + cloudValue * 0.2f > layer.lightBorder2) {
        color = layer.colors[3];
      }

      float alpha = planetStep(layer.cloudCover, cloudValue) * alphaCircle * planetStep(dCenter, 0.5f);
      coverPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderGasCloudsBuffer(const PlanetRenderFrame& frame, const PlanetCloudLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);
  float timeOffset = time * layer.timeSpeed;

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float dLight = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float dCircle = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float alphaCircle = planetStep(dCircle, 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }

      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);
      float sphereCloudY = sphereY + smoothstepFloat(0.0f, layer.cloudCurve, fabsf(sphereX - 0.4f));
      sphereCloudY *= layer.stretch;

      float cloudNoise = 0.0f;
      for (int index = 0; index < 9; index += 1) {
        cloudNoise += circleNoisePlanetCloud(
          sphereX * layer.size * 0.3f + (float)index + 11.0f + timeOffset,
          sphereCloudY * layer.size * 0.3f,
          frame.shaderSeed,
          layer.size
        );
      }

      float cloudValue = fbmPlanetPreviewTiled(
        sphereX * layer.size + cloudNoise + timeOffset,
        sphereCloudY * layer.size,
        frame.shaderSeed,
        layer.size,
        layer.octaves
      );

      PlanetBufferColor color = layer.colors[0];
      if (cloudValue < layer.cloudCover + 0.03f) {
        color = layer.colors[1];
      }
      if (dLight + cloudValue * 0.2f > layer.lightBorder1) {
        color = layer.colors[2];
      }
      if (dLight + cloudValue * 0.2f > layer.lightBorder2) {
        color = layer.colors[3];
      }

      float alpha = planetStep(layer.cloudCover, cloudValue) * alphaCircle;
      coverPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderIceLakesBuffer(const PlanetRenderFrame& frame, const PlanetIceLakeLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float dLight = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float dCircle = distancePlanet(rotatedX, rotatedY, 0.5f, 0.5f);
      if (dCircle > 0.5f) {
        continue;
      }

      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);
      float lake = fbmPlanetPreviewTiled(
        sphereX * layer.size + time * layer.timeSpeed,
        sphereY * layer.size,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );

      float adjustedLight = powf(dLight, 2.0f) * 0.4f;
      adjustedLight -= adjustedLight * lake;

      PlanetBufferColor color = layer.colors[0];
      if (adjustedLight > layer.lightBorder1) {
        color = layer.colors[1];
      }
      if (adjustedLight > layer.lightBorder2) {
        color = layer.colors[2];
      }

      float alpha = planetStep(layer.lakeCutoff, lake);
      coverPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderNoAtmosphereBaseBuffer(const PlanetRenderFrame& frame, const PlanetUnderLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float dCircle = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float dLightBase = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float alphaCircle = planetStep(dCircle, 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }

      bool dith = modPlanetFloat(pixelU + v, 2.0f / pixels) <= 1.0f / pixels;
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float fbm1 = fbmPlanetPreviewTiled(rotatedX, rotatedY, frame.shaderSeed, layer.size, layer.octaves);
      float dLight = dLightBase;
      dLight += fbmPlanetPreviewTiled(
        rotatedX * layer.size + fbm1 + time * layer.timeSpeed,
        rotatedY * layer.size,
        frame.shaderSeed,
        layer.size,
        layer.octaves
      ) * 0.3f;

      float ditherBorder = 1.0f / pixels * layer.ditherSize;
      PlanetBufferColor color = layer.colors[0];
      if (dLight > layer.lightBorder1) {
        color = layer.colors[1];
        if (dLight < layer.lightBorder1 + ditherBorder &&
            (dith || !frame.dither)) {
          color = layer.colors[0];
        }
      }
      if (dLight > layer.lightBorder2) {
        color = layer.colors[2];
        if (dLight < layer.lightBorder2 + ditherBorder &&
            (dith || !frame.dither)) {
          color = layer.colors[1];
        }
      }

      blendPlanetPreviewPixel(offset, color, alphaCircle * color.a);
    }
  }
}

void renderCratersBuffer(const PlanetRenderFrame& frame, const PlanetCraterLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);
  float timeOffset = time * layer.timeSpeed;

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float dCircle = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float dLight = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float alpha = planetStep(dCircle, 0.49999f);
      if (alpha == 0.0f) {
        continue;
      }

      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);

      float craterOne = 1.0f;
      float craterTwo = 1.0f;
      for (int index = 0; index < 2; index += 1) {
        craterOne *= circleNoisePlanetCrater(
          sphereX * layer.size + (float)index + 11.0f + timeOffset,
          sphereY * layer.size,
          frame.shaderSeed,
          layer.size
        );
        craterTwo *= circleNoisePlanetCrater(
          (sphereX + (layer.lightOriginX - 0.5f) * layer.offset) * layer.size + (float)index + 11.0f + timeOffset,
          (sphereY + (layer.lightOriginY - 0.5f) * layer.offset) * layer.size,
          frame.shaderSeed,
          layer.size
        );
      }
      craterOne = 1.0f - craterOne;
      craterTwo = 1.0f - craterTwo;

      alpha *= planetStep(0.5f, craterOne);
      if (alpha == 0.0f) {
        continue;
      }

      PlanetBufferColor color = layer.colors[0];
      if (craterTwo < craterOne - (0.5f - dLight) * 2.0f) {
        color = layer.colors[1];
      }
      if (dLight > layer.lightBorder) {
        color = layer.colors[1];
      }

      coverPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderLavaRiversBuffer(const PlanetRenderFrame& frame, const PlanetLavaRiverLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float dLight = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float dCircle = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float alpha = planetStep(dCircle, 0.49999f);
      if (alpha == 0.0f) {
        continue;
      }

      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);

      float fbm1 = fbmPlanetPreviewTiled(
        sphereX * layer.size + time * layer.timeSpeed,
        sphereY * layer.size,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );
      float river = planetStep(
        layer.riverCutoff,
        fbmPlanetPreviewTiled(
          sphereX + fbm1 * 2.5f,
          sphereY + fbm1 * 2.5f,
          frame.shaderSeed,
          layer.size,
          layer.octaves,
          2.0f,
          1.0f
        )
      );

      PlanetBufferColor color = layer.colors[0];
      if (dLight > layer.lightBorder1) {
        color = layer.colors[1];
      }
      if (dLight > layer.lightBorder2) {
        color = layer.colors[2];
      }

      alpha *= planetStep(layer.riverCutoff, river);
      coverPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderDryTerranBuffer(const PlanetRenderFrame& frame, const PlanetDryTerranLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      bool dith = modPlanetFloat(pixelU + v, 2.0f / pixels) <= 1.0f / pixels;
      float dCircle = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float alphaCircle = planetStep(dCircle, 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }

      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(pixelU, pixelV, sphereX, sphereY);
      float dLightBase = smoothstepFloat(
        -0.3f,
        1.2f,
        distancePlanet(sphereX, sphereY, layer.lightOriginX, layer.lightOriginY)
      );
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(sphereX, sphereY, layer.rotationOffset, rotatedX, rotatedY);
      float noiseValue = fbmPlanetPreviewTiled(
        rotatedX * layer.size + time * layer.timeSpeed,
        rotatedY * layer.size,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );

      float dLight = dLightBase;
      if (dLight < layer.lightDistance1) {
        dLight *= 0.9f;
      }
      if (dLight < layer.lightDistance2) {
        dLight *= 0.9f;
      }

      float colorValue = dLight * powf(noiseValue, 0.8f) * 3.5f;
      if (dith || !frame.dither) {
        colorValue += 0.02f;
        colorValue *= 1.05f;
      }

      float posterize = floorf(colorValue * 4.0f) / 4.0f;
      if (posterize > 1.0f) {
        posterize = 1.0f;
      }
      int colorIndex = clampInt(
        (int)roundf(posterize * (float)(layer.colorCount - 1)),
        0,
        layer.colorCount - 1
      );
      PlanetBufferColor color = layer.colors[colorIndex];
      blendPlanetPreviewPixel(offset, color, alphaCircle * color.a);
    }
  }
}

void renderLandRiversBuffer(const PlanetRenderFrame& frame, const PlanetLandRiverLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      bool dith = modPlanetFloat(pixelU + v, 2.0f / pixels) <= 1.0f / pixels;
      float alphaCircle = planetStep(distancePlanet(pixelU, pixelV, 0.5f, 0.5f), 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }

      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(pixelU, pixelV, sphereX, sphereY);
      float dLight = distancePlanet(sphereX, sphereY, layer.lightOriginX, layer.lightOriginY);
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(sphereX, sphereY, layer.rotationOffset, rotatedX, rotatedY);
      float baseX = rotatedX * layer.size + time * layer.timeSpeed;
      float baseY = rotatedY * layer.size;
      float fbm1 = fbmPlanetPreviewTiled(baseX, baseY, frame.shaderSeed, layer.size, layer.octaves, 2.0f, 1.0f);
      float fbm2Value = fbmPlanetPreviewTiled(
        baseX - layer.lightOriginX * fbm1,
        baseY - layer.lightOriginY * fbm1,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );
      float fbm3Value = fbmPlanetPreviewTiled(
        baseX - layer.lightOriginX * 1.5f * fbm1,
        baseY - layer.lightOriginY * 1.5f * fbm1,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );
      float fbm4Value = fbmPlanetPreviewTiled(
        baseX - layer.lightOriginX * 2.0f * fbm1,
        baseY - layer.lightOriginY * 2.0f * fbm1,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );
      float riverValue = planetStep(
        layer.riverCutoff,
        fbmPlanetPreviewTiled(
          baseX + fbm1 * 6.0f,
          baseY + fbm1 * 6.0f,
          frame.shaderSeed,
          layer.size,
          layer.octaves,
          2.0f,
          1.0f
        )
      );

      float ditherBorder = 1.0f / pixels * layer.ditherSize;
      if (dLight < layer.lightBorder1) {
        fbm4Value *= 0.9f;
      }
      if (dLight > layer.lightBorder1) {
        fbm2Value *= 1.05f;
        fbm3Value *= 1.05f;
        fbm4Value *= 1.05f;
      }
      if (dLight > layer.lightBorder2) {
        fbm2Value *= 1.3f;
        fbm3Value *= 1.4f;
        fbm4Value *= 1.8f;
        if (dLight < layer.lightBorder2 + ditherBorder && (dith || !frame.dither)) {
          fbm4Value *= 0.5f;
        }
      }

      float lightValue = powf(dLight, 2.0f) * 0.4f;
      PlanetBufferColor color = layer.colors[3];
      if (fbm4Value + lightValue < fbm1 * 1.5f) {
        color = layer.colors[2];
      }
      if (fbm3Value + lightValue < fbm1) {
        color = layer.colors[1];
      }
      if (fbm2Value + lightValue < fbm1) {
        color = layer.colors[0];
      }
      if (riverValue < fbm1 * 0.5f) {
        color = layer.colors[5];
        if (fbm4Value + lightValue < fbm1 * 1.5f) {
          color = layer.colors[4];
        }
      }

      blendPlanetPreviewPixel(offset, color, alphaCircle * color.a);
    }
  }
}

void renderDenseGasBuffer(const PlanetRenderFrame& frame, const PlanetDenseGasLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);
  float timeOffset = time * layer.timeSpeed;

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float lightDistance = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      bool dith = modPlanetFloat(pixelU + v, 2.0f / pixels) <= 1.0f / pixels;
      float alphaCircle = planetStep(distancePlanet(pixelU, pixelV, 0.5f, 0.5f), 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }

      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);
      float band = fbmPlanetPreviewTiled(0.0f, sphereY * layer.size * layer.bands, frame.shaderSeed, layer.size, layer.octaves, 2.0f, 1.0f);

      float turbulence = 0.0f;
      for (int index = 0; index < 10; index += 1) {
        turbulence += circleNoisePlanetCloud(
          sphereX * layer.size * 0.3f + (float)index + 11.0f + timeOffset,
          sphereY * layer.size * 0.3f,
          frame.shaderSeed,
          layer.size
        );
      }

      float fbm1 = fbmPlanetPreviewTiled(sphereX * layer.size, sphereY * layer.size, frame.shaderSeed, layer.size, layer.octaves, 2.0f, 1.0f);
      float fbm2Value = fbmPlanetPreviewTiled(
        sphereX * layer.size + fbm1 - timeOffset + turbulence,
        sphereY * layer.size * 2.0f,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );
      fbm2Value *= powf(band, 2.0f) * 7.0f;
      float lightValue = fbm2Value + lightDistance * 1.8f;
      fbm2Value += powf(lightDistance, 1.0f) - 0.3f;
      fbm2Value = smoothstepFloat(-0.2f, 4.0f - fbm2Value, lightValue);

      if (dith && frame.dither) {
        fbm2Value *= 1.1f;
      }

      float posterized = floorf(fbm2Value * 4.0f) / 2.0f;
      PlanetBufferColor color;
      if (fbm2Value < 0.625f) {
        int index = clampInt((int)roundf(posterized * 2.0f), 0, 2);
        color = layer.colors[index];
      } else {
        int index = clampInt((int)roundf((posterized - 1.0f) * 2.0f), 0, 2);
        color = layer.darkColors[index];
      }

      blendPlanetPreviewPixel(offset, color, alphaCircle * color.a);
    }
  }
}

void renderRingBuffer(const PlanetRenderFrame& frame, const PlanetRingLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromRotation(frame.progress, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float lightDistance = distancePlanet(pixelU, pixelV, layer.lightOriginX, layer.lightOriginY);
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float centerX = rotatedX;
      float centerY = rotatedY - 0.5f;
      centerY *= layer.ringPerspective;
      float centerDistance = distancePlanet(centerX, centerY, 0.5f, 0.0f);

      float ringValue = smoothstepFloat(0.5f - layer.ringWidth * 2.0f, 0.5f - layer.ringWidth, centerDistance);
      ringValue *= smoothstepFloat(centerDistance - layer.ringWidth, centerDistance, 0.4f);

      if (rotatedY < 0.5f) {
        ringValue *= planetStep(
          1.0f / layer.scaleRelToPlanet,
          distancePlanet(rotatedX, rotatedY, 0.5f, 0.5f)
        );
      }

      float materialU = 0.0f;
      float materialV = 0.0f;
      rotatePlanetUv(centerX, centerY + 0.5f, time * layer.timeSpeed, materialU, materialV);
      ringValue *= fbmPlanetPreviewTiled(materialU * layer.size, materialV * layer.size, frame.shaderSeed, layer.size, layer.octaves, 2.0f, 1.0f);

      float posterized = fminf(2.0f, floorf((ringValue + powf(lightDistance, 2.0f) * 2.0f) * 4.0f) / 4.0f);
      PlanetBufferColor color;
      if (posterized <= 1.0f) {
        int index = clampInt((int)roundf(posterized * 2.0f), 0, 2);
        color = layer.colors[index];
      } else {
        int index = clampInt((int)roundf((posterized - 1.0f) * 2.0f), 0, 2);
        color = layer.darkColors[index];
      }

      float alpha = planetStep(0.28f, ringValue);
      coverPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderAsteroidBuffer(const PlanetRenderFrame& frame, const PlanetAsteroidLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float rotation = frame.spinAngle + layer.rotationOffset;

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      bool dith = modPlanetFloat(pixelU + v, 2.0f / pixels) <= 1.0f / pixels;
      float distanceFromCenter = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, rotation, rotatedX, rotatedY);
      float rotatedLightX = 0.0f;
      float rotatedLightY = 0.0f;
      rotatePlanetUv(layer.lightOriginX, layer.lightOriginY, rotation, rotatedLightX, rotatedLightY);

      float n = fbmPlanetPreviewUntiled(rotatedX * layer.size, rotatedY * layer.size, frame.shaderSeed, layer.octaves);
      float n2 = fbmPlanetPreviewUntiled(
        rotatedX * layer.size + (rotatedLightX - 0.5f) * 0.5f,
        rotatedY * layer.size + (rotatedLightY - 0.5f) * 0.5f,
        frame.shaderSeed,
        layer.octaves
      );
      float nStep = planetStep(0.2f, n - distanceFromCenter);
      if (nStep == 0.0f) {
        continue;
      }
      float n2Step = planetStep(0.2f, n2 - distanceFromCenter);
      float noiseRelation = n2Step + n2 - (nStep + n);

      float craterOne = 1.0f;
      float craterTwo = 1.0f;
      for (int index = 0; index < 2; index += 1) {
        craterOne *= circleNoisePlanetCrater(
          rotatedX * layer.size + (float)index + 11.0f,
          rotatedY * layer.size,
          frame.shaderSeed,
          layer.size
        );
        craterTwo *= circleNoisePlanetCrater(
          (rotatedX + (layer.lightOriginX - 0.5f) * 0.03f) * layer.size + (float)index + 11.0f,
          (rotatedY + (layer.lightOriginY - 0.5f) * 0.03f) * layer.size,
          frame.shaderSeed,
          layer.size
        );
      }
      craterOne = 1.0f - craterOne;
      craterTwo = 1.0f - craterTwo;

      PlanetBufferColor color = layer.colors[1];
      if (noiseRelation < -0.06f || (noiseRelation < -0.04f && (dith || !frame.dither))) {
        color = layer.colors[0];
      }
      if (noiseRelation > 0.05f || (noiseRelation > 0.03f && (dith || !frame.dither))) {
        color = layer.colors[2];
      }
      if (craterOne > 0.4f) {
        color = layer.colors[1];
      }
      if (craterTwo < craterOne) {
        color = layer.colors[2];
      }

      blendPlanetPreviewPixel(offset, color, nStep * color.a);
    }
  }
}

void renderStarCoreBuffer(const PlanetRenderFrame& frame, const PlanetStarCoreLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = frame.progress * frame.motionFactor / layer.timeSpeed;

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float alphaCircle = planetStep(distancePlanet(pixelU, pixelV, 0.5f, 0.5f), 0.49999f);
      if (alphaCircle == 0.0f) {
        continue;
      }
      bool dith = modPlanetFloat(u + pixelV, 2.0f / pixels) <= 1.0f / pixels;

      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float sphereX = 0.0f;
      float sphereY = 0.0f;
      spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);

      float cellValue = cellsPlanetValue(sphereX - time * layer.timeSpeed * 2.0f, sphereY, 10.0f, (float)layer.tiles);
      cellValue *= cellsPlanetValue(sphereX - time * layer.timeSpeed, sphereY, 20.0f, (float)layer.tiles);
      cellValue *= 2.0f;
      cellValue = clampFloat(cellValue, 0.0f, 1.0f);
      if (dith || !frame.dither) {
        cellValue *= 1.3f;
      }

      float interpolate = floorf(cellValue * (float)(layer.colorCount - 1)) / (float)(layer.colorCount - 1);
      int colorIndex = clampInt(
        (int)roundf(interpolate * (float)(layer.colorCount - 1)),
        0,
        layer.colorCount - 1
      );
      PlanetBufferColor color = layer.colors[colorIndex];
      blendPlanetPreviewPixel(offset, color, alphaCircle * color.a);
    }
  }
}

void renderStarBlobsBuffer(const PlanetRenderFrame& frame, const PlanetStarBlobLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float angle = atan2f(rotatedX - 0.5f, rotatedY - 0.5f);
      float distanceFromCenter = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);

      float value = 0.0f;
      for (int index = 0; index < 15; index += 1) {
        float randomValue = tiledPlanetRand(frame.shaderSeed, layer.size, (float)index, 0.0f);
        float circleUvX = distanceFromCenter * layer.size - time * layer.timeSpeed -
                          0.1f / fmaxf(distanceFromCenter, 0.0001f) + randomValue;
        float circleUvY = angle * layer.size;

        float invert = 1.0f / layer.circleAmount;
        float adjustedX = circleUvX;
        if (modPlanetFloat(circleUvY, invert * 2.0f) < invert) {
          adjustedX += invert * 0.5f;
        }
        float randCoordX = floorf(adjustedX * layer.circleAmount) / layer.circleAmount;
        float randCoordY = floorf(circleUvY * layer.circleAmount) / layer.circleAmount;
        float localX = modPlanetFloat(adjustedX, invert) * layer.circleAmount;
        float localY = modPlanetFloat(circleUvY, invert) * layer.circleAmount;
        float radius = tiledPlanetRand(frame.shaderSeed, layer.size, randCoordX, randCoordY);
        radius = clampFloat(radius, invert, 1.0f - invert);
        float circleDistance = distancePlanet(localX, localY, radius, radius);
        float circleAlpha = smoothstepFloat(
          circleDistance,
          circleDistance + 0.5f,
          invert * layer.circleSize *
            tiledPlanetRand(frame.shaderSeed, layer.size, randCoordX * 1.5f, randCoordY * 1.5f)
        );
        value += circleAlpha;
      }

      value *= 0.37f - distanceFromCenter;
      value = planetStep(0.07f, value - distanceFromCenter);
      blendPlanetPreviewPixel(offset, layer.color, value * layer.color.a);
    }
  }
}

void renderStarFlaresBuffer(const PlanetRenderFrame& frame, const PlanetStarFlareLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromMultiplier(frame.progress, layer.size, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      bool dith = modPlanetFloat(u + pixelV, 2.0f / pixels) <= 1.0f / pixels;
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float angle = atan2f(rotatedX - 0.5f, rotatedY - 0.5f) * 0.4f;
      float distanceFromCenter = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      float circleUvX = distanceFromCenter;
      float circleUvY = angle;

      float n = fbmPlanetPreviewTiled(
        circleUvX * layer.size - time * layer.timeSpeed,
        circleUvY * layer.size,
        frame.shaderSeed,
        layer.size,
        layer.octaves
      );

      float invert = 1.0f / layer.circleAmount;
      float adjustedX = circleUvX * layer.scale - time * layer.timeSpeed + n;
      float adjustedY = circleUvY * layer.scale;
      if (modPlanetFloat(adjustedY, invert * 2.0f) < invert) {
        adjustedX += invert * 0.5f;
      }
      float randCoordX = floorf(adjustedX * layer.circleAmount) / layer.circleAmount;
      float randCoordY = floorf(adjustedY * layer.circleAmount) / layer.circleAmount;
      float localX = modPlanetFloat(adjustedX, invert) * layer.circleAmount;
      float localY = modPlanetFloat(adjustedY, invert) * layer.circleAmount;
      float radius = tiledPlanetRand(frame.shaderSeed, layer.size, randCoordX, randCoordY);
      radius = clampFloat(radius, invert, 1.0f - invert);
      float circleDistance = distancePlanet(localX, localY, radius, radius);
      float nc = smoothstepFloat(
        circleDistance,
        circleDistance + 0.5f,
        invert * layer.circleScale *
          tiledPlanetRand(frame.shaderSeed, layer.size, randCoordX * 1.5f, randCoordY * 1.5f)
      );
      nc *= 1.5f;
      float n2 = fbmPlanetPreviewTiled(
        circleUvX * layer.size - time + 100.0f,
        circleUvY * layer.size + 100.0f,
        frame.shaderSeed,
        layer.size,
        layer.octaves
      );
      nc -= n2 * 0.1f;

      float alpha = 0.0f;
      if (1.0f - distanceFromCenter > nc) {
        if (nc > layer.stormWidth - layer.stormDitherWidth + distanceFromCenter &&
            (dith || !frame.dither)) {
          alpha = 1.0f;
        } else if (nc > layer.stormWidth + distanceFromCenter) {
          alpha = 1.0f;
        }
      }

      int colorIndex = clampInt((int)floorf(n2 + nc), 0, layer.colorCount - 1);
      PlanetBufferColor color = layer.colors[colorIndex];
      alpha *= planetStep(n2 * 0.25f, distanceFromCenter);
      blendPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderBlackHoleCoreBuffer(const PlanetRenderFrame& frame, const PlanetBlackHoleCoreLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      float distanceFromCenter = distancePlanet(pixelU, pixelV, 0.5f, 0.5f);
      PlanetBufferColor color = layer.colors[0];
      if (distanceFromCenter > layer.radius - layer.lightWidth) {
        color = layer.colors[1];
      }
      if (distanceFromCenter > layer.radius - layer.lightWidth * 0.5f) {
        color = layer.colors[2];
      }
      float alpha = planetStep(distanceFromCenter, layer.radius);
      blendPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderBlackHoleDiskBuffer(const PlanetRenderFrame& frame, const PlanetBlackHoleDiskLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float wobbleTime = loopPlanetTimeFromRotation(frame.progress, layer.timeSpeed, frame.motionFactor);
  float flowTime = loopPlanetTimeFromRotation(frame.progress, layer.timeSpeed, frame.spinFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float pixelU = quantizePlanetUv(u, pixels);
      float pixelV = quantizePlanetUv(v, pixels);
      bool dith = modPlanetFloat(u + pixelV, 2.0f / pixels) <= 1.0f / pixels;
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(pixelU, pixelV, layer.rotationOffset, rotatedX, rotatedY);
      float preservedX = rotatedX;
      float preservedY = rotatedY;

      float compressedX = (rotatedX - 0.5f) * 1.3f + 0.5f;
      float compressedU = 0.0f;
      float compressedV = 0.0f;
      rotatePlanetUv(
        compressedX,
        rotatedY,
        sinf(wobbleTime * layer.timeSpeed * 2.0f) * 0.01f,
        compressedU,
        compressedV
      );

      float lightOriginX = 0.5f;
      float lightOriginY = 0.5f;
      float diskWidth = layer.diskWidth;
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
        preservedX,
        preservedY * layer.ringPerspective,
        lightOriginX,
        lightOriginY * layer.ringPerspective
      ) * 0.3f;

      float centerX = compressedU;
      float centerY = (compressedV - 0.5f) * layer.ringPerspective;
      float centerDistance = distancePlanet(centerX, centerY, 0.5f, 0.0f);
      float disk = smoothstepFloat(0.1f - diskWidth * 2.0f, 0.5f - diskWidth, centerDistance);
      disk *= smoothstepFloat(centerDistance - diskWidth, centerDistance, 0.4f);

      float materialU = 0.0f;
      float materialV = 0.0f;
      rotatePlanetUv(centerX, centerY + 0.5f, flowTime * layer.timeSpeed * 3.0f, materialU, materialV);
      disk *= sqrtf(clampFloat(
        fbmPlanetPreviewTiled(materialU * layer.size, materialV * layer.size, frame.shaderSeed, layer.size, layer.octaves, 2.0f, 1.0f),
        0.0f,
        1.0f
      ));

      if (dith || !frame.dither) {
        disk *= 1.2f;
      }

      int posterized = clampInt(
        (int)floorf((disk + lightDistance) * (float)(layer.colorCount - 1)),
        0,
        layer.colorCount - 1
      );
      PlanetBufferColor color = layer.colors[posterized];
      float alpha = planetStep(0.15f, disk);
      blendPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderGalaxyBuffer(const PlanetRenderFrame& frame, const PlanetGalaxyLayer& layer) {
  float pixels = (float)frame.pixels * layer.pixelsScale;
  float time = loopPlanetTimeFromRotation(frame.progress, layer.timeSpeed, frame.motionFactor);

  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      float u = 0.0f;
      float v = 0.0f;
      if (!mapPlanetPreviewPixelToUv(frame, layer.planeScale, x, y, u, v)) {
        continue;
      }

      int offset = (y * kPlanetCanvasSize + x) * kPlanetBufferChannels;
      float quantizedU = quantizePlanetUv(u, pixels);
      float quantizedV = quantizePlanetUv(v, pixels);
      bool dith = modPlanetFloat(quantizedU + v, 2.0f / pixels) <= 1.0f / pixels;

      quantizedU = quantizedU * layer.zoom - (layer.zoom - 1.0f) / 2.0f;
      quantizedV = quantizedV * layer.zoom - (layer.zoom - 1.0f) / 2.0f;
      float rotatedX = 0.0f;
      float rotatedY = 0.0f;
      rotatePlanetUv(quantizedU, quantizedV, layer.rotationOffset, rotatedX, rotatedY);
      float preservedX = rotatedX;
      float preservedY = rotatedY;

      float layerU = rotatedX;
      float layerV = rotatedY * layer.tilt - (layer.tilt - 1.0f) / 2.0f;
      float distanceOne = distancePlanet(layerU, layerV, 0.5f, 0.5f);
      float swirlOne = layer.swirl * powf(distanceOne, 0.4f);
      float rotatedLayerU = 0.0f;
      float rotatedLayerV = 0.0f;
      rotatePlanetUv(layerU, layerV, swirlOne + time * layer.timeSpeed, rotatedLayerU, rotatedLayerV);
      float f1 = fbmPlanetPreviewUntiled(rotatedLayerU * layer.size, rotatedLayerV * layer.size, frame.shaderSeed, layer.octaves);
      f1 = floorf(f1 * layer.layerCount) / layer.layerCount;

      float galaxyU = preservedX;
      float galaxyV = preservedY * layer.tilt - (layer.tilt - 1.0f) / 2.0f - f1 * layer.layerHeight;
      float distanceTwo = distancePlanet(galaxyU, galaxyV, 0.5f, 0.5f);
      float swirlTwo = layer.swirl * powf(distanceTwo, 0.4f);
      float rotatedGalaxyU = 0.0f;
      float rotatedGalaxyV = 0.0f;
      rotatePlanetUv(galaxyU, galaxyV, swirlTwo + time * layer.timeSpeed, rotatedGalaxyU, rotatedGalaxyV);
      float f2 = fbmPlanetPreviewUntiled(
        rotatedGalaxyU * layer.size + f1 * 10.0f,
        rotatedGalaxyV * layer.size + f1 * 10.0f,
        frame.shaderSeed,
        layer.octaves
      );

      float alpha = planetStep(f2 + distanceTwo, 0.7f);
      if (alpha == 0.0f) {
        continue;
      }

      f2 *= 2.3f;
      if (frame.dither && dith) {
        f2 *= 0.94f;
      }

      int colorIndex = clampInt((int)floorf(f2 * (float)layer.nColors), 0, layer.colorCount - 1);
      PlanetBufferColor color = layer.colors[colorIndex];
      blendPlanetPreviewPixel(offset, color, alpha * color.a);
    }
  }
}

void renderTerranWetPreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor landColors[] = {
    makePlanetBufferColor(0.388235f, 0.670588f, 0.247059f),
    makePlanetBufferColor(0.231373f, 0.490196f, 0.309804f),
    makePlanetBufferColor(0.184314f, 0.341176f, 0.32549f),
    makePlanetBufferColor(0.156863f, 0.207843f, 0.25098f),
    makePlanetBufferColor(0.309804f, 0.643137f, 0.721569f),
    makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
  };
  PlanetLandRiverLayer landLayer = {
    1.0f, 1.0f, 0.39f, 0.39f, 0.1f, 3.951f, 0.287f, 0.476f, 0.368f, 4.6f, 6, 0.0f, landColors
  };
  renderLandRiversBuffer(frame, landLayer);

  const PlanetBufferColor cloudColors[] = {
    makePlanetBufferColor(0.960784f, 1.0f, 0.909804f),
    makePlanetBufferColor(0.87451f, 0.878431f, 0.909804f),
    makePlanetBufferColor(0.407843f, 0.435294f, 0.6f),
    makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
  };
  PlanetCloudLayer cloudLayer = {
    1.0f,
    1.0f,
    0.39f,
    0.39f,
    getWetTerranCloudCover(frame.seed),
    0.1f,
    2.0f,
    1.3f,
    0.52f,
    0.62f,
    7.315f,
    1,
    0.0f,
    cloudColors
  };
  renderCloudsBuffer(frame, cloudLayer);
}

void renderTerranWetPreviewOptimized(const PlanetRenderFrame& frame) {
  const PlanetBufferColor landColors[] = {
    makePlanetBufferColor(0.388235f, 0.670588f, 0.247059f),
    makePlanetBufferColor(0.231373f, 0.490196f, 0.309804f),
    makePlanetBufferColor(0.184314f, 0.341176f, 0.32549f),
    makePlanetBufferColor(0.156863f, 0.207843f, 0.25098f),
    makePlanetBufferColor(0.309804f, 0.643137f, 0.721569f),
    makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
  };
  const PlanetBufferColor cloudColors[] = {
    makePlanetBufferColor(0.960784f, 1.0f, 0.909804f),
    makePlanetBufferColor(0.87451f, 0.878431f, 0.909804f),
    makePlanetBufferColor(0.407843f, 0.435294f, 0.6f),
    makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
  };
  PlanetLandRiverLayer landLayer = {
    1.0f, 1.0f, 0.39f, 0.39f, 0.1f, 3.951f, 0.287f, 0.476f, 0.368f, 4.6f, 6, 0.0f, landColors
  };
  PlanetCloudLayer cloudLayer = {
    1.0f,
    1.0f,
    0.39f,
    0.39f,
    getWetTerranCloudCover(frame.seed),
    0.1f,
    2.0f,
    1.3f,
    0.52f,
    0.62f,
    7.315f,
    1,
    0.0f,
    cloudColors
  };

  if (!ensurePlanetPreviewBuffer() ||
      !ensurePlanetTerranWetStaticCache(frame) ||
      !buildPlanetTiledRandCache(s_planetTerranWetLandRandCache, frame.shaderSeed, landLayer.size, 2.0f, 1.0f) ||
      !buildPlanetTiledRandCache(s_planetTerranWetCloudRandCache, frame.shaderSeed, cloudLayer.size, 1.0f, 1.0f)) {
    renderTerranWetPreview(frame);
    return;
  }

  PlanetBufferColor mappedLandColors[6];
  PlanetBufferColor mappedCloudColors[4];
  mapPlanetColorArray(landColors, mappedLandColors, 6);
  mapPlanetColorArray(cloudColors, mappedCloudColors, 4);

  float landTime = loopPlanetTimeFromMultiplier(frame.progress, landLayer.size, landLayer.timeSpeed, frame.motionFactor);
  float landTimeOffset = landTime * landLayer.timeSpeed;
  float cloudTime = loopPlanetTimeFromMultiplier(frame.progress, cloudLayer.size, cloudLayer.timeSpeed, frame.motionFactor);
  float cloudTimeOffset = cloudTime * cloudLayer.timeSpeed;

  for (int index = 0; index < s_planetTerranWetStaticCache.activeCount; index += 1) {
    const PlanetTerranWetStaticPixel& pixel = s_planetTerranWetStaticCache.pixelsData[index];
    float dLight = distancePlanet(pixel.sphereX, pixel.sphereY, landLayer.lightOriginX, landLayer.lightOriginY);
    float baseX = pixel.sphereX * landLayer.size + landTimeOffset;
    float baseY = pixel.sphereY * landLayer.size;
    float fbm1 = fbmPlanetPreviewTiledCached(baseX, baseY, landLayer.octaves, s_planetTerranWetLandRandCache);
    float fbm2Value = fbmPlanetPreviewTiledCached(
      baseX - landLayer.lightOriginX * fbm1,
      baseY - landLayer.lightOriginY * fbm1,
      landLayer.octaves,
      s_planetTerranWetLandRandCache
    );
    float fbm3Value = fbmPlanetPreviewTiledCached(
      baseX - landLayer.lightOriginX * 1.5f * fbm1,
      baseY - landLayer.lightOriginY * 1.5f * fbm1,
      landLayer.octaves,
      s_planetTerranWetLandRandCache
    );
    float fbm4Value = fbmPlanetPreviewTiledCached(
      baseX - landLayer.lightOriginX * 2.0f * fbm1,
      baseY - landLayer.lightOriginY * 2.0f * fbm1,
      landLayer.octaves,
      s_planetTerranWetLandRandCache
    );
    float riverValue = planetStep(
      landLayer.riverCutoff,
      fbmPlanetPreviewTiledCached(
        baseX + fbm1 * 6.0f,
        baseY + fbm1 * 6.0f,
        landLayer.octaves,
        s_planetTerranWetLandRandCache
      )
    );

    float ditherBorder = 1.0f / (float)frame.pixels * landLayer.ditherSize;
    if (dLight < landLayer.lightBorder1) {
      fbm4Value *= 0.9f;
    }
    if (dLight > landLayer.lightBorder1) {
      fbm2Value *= 1.05f;
      fbm3Value *= 1.05f;
      fbm4Value *= 1.05f;
    }
    if (dLight > landLayer.lightBorder2) {
      fbm2Value *= 1.3f;
      fbm3Value *= 1.4f;
      fbm4Value *= 1.8f;
      if (dLight < landLayer.lightBorder2 + ditherBorder &&
          (pixel.dith || !frame.dither)) {
        fbm4Value *= 0.5f;
      }
    }

    float lightValue = dLight * dLight * 0.4f;
    PlanetBufferColor color = mappedLandColors[3];
    if (fbm4Value + lightValue < fbm1 * 1.5f) {
      color = mappedLandColors[2];
    }
    if (fbm3Value + lightValue < fbm1) {
      color = mappedLandColors[1];
    }
    if (fbm2Value + lightValue < fbm1) {
      color = mappedLandColors[0];
    }
    if (riverValue < fbm1 * 0.5f) {
      color = mappedLandColors[5];
      if (fbm4Value + lightValue < fbm1 * 1.5f) {
        color = mappedLandColors[4];
      }
    }

    setPlanetPreviewPixelOpaque(pixel.offset, color);
  }

  for (int index = 0; index < s_planetTerranWetStaticCache.activeCount; index += 1) {
    const PlanetTerranWetStaticPixel& pixel = s_planetTerranWetStaticCache.pixelsData[index];
    float dLight = distancePlanet(pixel.pixelU, pixel.pixelV, cloudLayer.lightOriginX, cloudLayer.lightOriginY);
    float sphereCloudY = pixel.sphereY + smoothstepFloat(0.0f, cloudLayer.cloudCurve, fabsf(pixel.sphereX - 0.4f));
    sphereCloudY *= cloudLayer.stretch;

    float scaledSphereX = pixel.sphereX * cloudLayer.size;
    float scaledSphereCloudY = sphereCloudY * cloudLayer.size;
    float cloudNoise = 0.0f;
    for (int noiseIndex = 0; noiseIndex < 9; noiseIndex += 1) {
      cloudNoise += circleNoisePlanetCloudCached(
        scaledSphereX * 0.3f + (float)noiseIndex + 11.0f + cloudTimeOffset,
        scaledSphereCloudY * 0.3f,
        s_planetTerranWetCloudRandCache
      );
    }

    float cloudValue = fbmPlanetPreviewTiledCached(
      scaledSphereX + cloudNoise + cloudTimeOffset,
      scaledSphereCloudY,
      cloudLayer.octaves,
      s_planetTerranWetCloudRandCache
    );
    if (planetStep(cloudLayer.cloudCover, cloudValue) == 0.0f) {
      continue;
    }

    PlanetBufferColor color = mappedCloudColors[0];
    if (cloudValue < cloudLayer.cloudCover + 0.03f) {
      color = mappedCloudColors[1];
    }
    if (dLight + cloudValue * 0.2f > cloudLayer.lightBorder1) {
      color = mappedCloudColors[2];
    }
    if (dLight + cloudValue * 0.2f > cloudLayer.lightBorder2) {
      color = mappedCloudColors[3];
    }

    setPlanetPreviewPixelOpaque(pixel.offset, color);
  }
}

void renderTerranDryPreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor colors[] = {
    makePlanetBufferColor(1.0f, 0.537255f, 0.2f),
    makePlanetBufferColor(0.901961f, 0.270588f, 0.223529f),
    makePlanetBufferColor(0.678431f, 0.184314f, 0.270588f),
    makePlanetBufferColor(0.321569f, 0.2f, 0.247059f),
    makePlanetBufferColor(0.239216f, 0.160784f, 0.211765f)
  };
  PlanetDryTerranLayer layer = {
    1.0f, 1.0f, 0.4f, 0.3f, 0.362f, 0.525f, 0.1f, 8.0f, 3, 0.0f, colors, 5
  };
  renderDryTerranBuffer(frame, layer);
}

void renderIslandsPreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor underColors[] = {
    makePlanetBufferColor(0.572549f, 0.909804f, 0.752941f),
    makePlanetBufferColor(0.309804f, 0.643137f, 0.721569f),
    makePlanetBufferColor(0.172549f, 0.207843f, 0.301961f)
  };
  PlanetUnderLayer underLayer = {
    1.0f, 1.0f, 0.39f, 0.39f, 0.1f, 2.0f, 0.4f, 0.6f, 5.228f, 3, 0.0f, underColors
  };
  renderPlanetUnderBuffer(frame, underLayer);

  const PlanetBufferColor landColors[] = {
    makePlanetBufferColor(0.784314f, 0.831373f, 0.364706f),
    makePlanetBufferColor(0.388235f, 0.670588f, 0.247059f),
    makePlanetBufferColor(0.184314f, 0.341176f, 0.32549f),
    makePlanetBufferColor(0.156863f, 0.207843f, 0.25098f)
  };
  PlanetLandMassLayer landLayer = {
    1.0f, 1.0f, 0.39f, 0.39f, 0.2f, 0.32f, 0.534f, 0.633f, 4.292f, 6, 0.0f, landColors
  };
  renderLandMassBuffer(frame, landLayer);

  const PlanetBufferColor cloudColors[] = {
    makePlanetBufferColor(0.87451f, 0.878431f, 0.909804f),
    makePlanetBufferColor(0.639216f, 0.654902f, 0.760784f),
    makePlanetBufferColor(0.407843f, 0.435294f, 0.6f),
    makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
  };
  PlanetCloudLayer cloudLayer = {
    1.0f,
    1.0f,
    0.39f,
    0.39f,
    getIslandCloudCover(frame.seed),
    0.47f,
    2.0f,
    1.3f,
    0.52f,
    0.62f,
    7.745f,
    2,
    0.0f,
    cloudColors
  };
  renderCloudsBuffer(frame, cloudLayer);
}

void renderNoAtmospherePreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor baseColors[] = {
    makePlanetBufferColor(0.639216f, 0.654902f, 0.760784f),
    makePlanetBufferColor(0.298039f, 0.407843f, 0.521569f),
    makePlanetBufferColor(0.227451f, 0.247059f, 0.368627f)
  };
  PlanetUnderLayer baseLayer = {
    1.0f, 1.0f, 0.25f, 0.25f, 0.4f, 2.0f, 0.615f, 0.729f, 8.0f, 4, 0.0f, baseColors
  };
  renderNoAtmosphereBaseBuffer(frame, baseLayer);

  const PlanetBufferColor craterColors[] = {
    makePlanetBufferColor(0.298039f, 0.407843f, 0.521569f),
    makePlanetBufferColor(0.227451f, 0.247059f, 0.368627f)
  };
  PlanetCraterLayer craterLayer = {
    1.0f, 1.0f, 0.25f, 0.25f, 0.001f, 0.465f, 5.0f, 0.03f, 0, 0.0f, craterColors
  };
  renderCratersBuffer(frame, craterLayer);
}

void renderGasGiantOnePreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor darkColors[] = {
    makePlanetBufferColor(0.231373f, 0.12549f, 0.152941f),
    makePlanetBufferColor(0.231373f, 0.12549f, 0.152941f),
    makePlanetBufferColor(0.0f, 0.0f, 0.0f),
    makePlanetBufferColor(0.129412f, 0.0941176f, 0.105882f)
  };
  PlanetCloudLayer darkLayer = {
    1.0f, 1.0f, 0.25f, 0.25f, 0.0f, 0.7f, 1.0f, 1.3f, 0.692f, 0.666f, 9.0f, 5, 0.0f, darkColors
  };
  renderGasCloudsBuffer(frame, darkLayer);

  const PlanetBufferColor warmColors[] = {
    makePlanetBufferColor(0.941176f, 0.709804f, 0.254902f),
    makePlanetBufferColor(0.811765f, 0.458824f, 0.168627f),
    makePlanetBufferColor(0.670588f, 0.317647f, 0.188235f),
    makePlanetBufferColor(0.490196f, 0.219608f, 0.2f)
  };
  PlanetCloudLayer warmLayer = {
    1.0f,
    1.0f,
    0.25f,
    0.25f,
    getGasGiantCloudCover(frame.seed),
    0.47f,
    1.0f,
    1.3f,
    0.439f,
    0.746f,
    9.0f,
    5,
    0.0f,
    warmColors
  };
  renderGasCloudsBuffer(frame, warmLayer);
}

void renderGasGiantTwoPreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor colors[] = {
    makePlanetBufferColor(0.933333f, 0.764706f, 0.603922f),
    makePlanetBufferColor(0.85098f, 0.627451f, 0.4f),
    makePlanetBufferColor(0.560784f, 0.337255f, 0.231373f)
  };
  const PlanetBufferColor darkColors[] = {
    makePlanetBufferColor(0.4f, 0.223529f, 0.192157f),
    makePlanetBufferColor(0.270588f, 0.156863f, 0.235294f),
    makePlanetBufferColor(0.133333f, 0.12549f, 0.203922f)
  };
  PlanetDenseGasLayer gasLayer = {
    1.0f, 1.0f, -0.1f, 0.3f, 0.05f, 0.892f, 10.107f, 3, 0.0f, colors, darkColors
  };
  renderDenseGasBuffer(frame, gasLayer);

  PlanetRingLayer ringLayer = {
    3.0f, 3.0f, -0.1f, 0.3f, 0.2f, 0.127f, 6.0f, 6.0f, 15.0f, 4, 0.7f, colors, darkColors
  };
  renderRingBuffer(frame, ringLayer);
}

void renderIceWorldPreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor underColors[] = {
    makePlanetBufferColor(0.980392f, 1.0f, 1.0f),
    makePlanetBufferColor(0.780392f, 0.831373f, 0.882353f),
    makePlanetBufferColor(0.572549f, 0.560784f, 0.721569f)
  };
  PlanetUnderLayer underLayer = {
    1.0f, 1.0f, 0.3f, 0.3f, 0.25f, 2.0f, 0.48f, 0.632f, 8.0f, 2, 0.0f, underColors
  };
  renderPlanetUnderBuffer(frame, underLayer);

  const PlanetBufferColor lakeColors[] = {
    makePlanetBufferColor(0.309804f, 0.643137f, 0.721569f),
    makePlanetBufferColor(0.298039f, 0.407843f, 0.521569f),
    makePlanetBufferColor(0.227451f, 0.247059f, 0.368627f)
  };
  PlanetIceLakeLayer lakeLayer = {
    1.0f, 1.0f, 0.3f, 0.3f, 0.2f, 0.024f, 0.047f, 0.55f, 10.0f, 3, 0.0f, lakeColors
  };
  renderIceLakesBuffer(frame, lakeLayer);

  const PlanetBufferColor cloudColors[] = {
    makePlanetBufferColor(0.882353f, 0.94902f, 1.0f),
    makePlanetBufferColor(0.752941f, 0.890196f, 1.0f),
    makePlanetBufferColor(0.368627f, 0.439216f, 0.647059f),
    makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
  };
  PlanetCloudLayer cloudLayer = {
    1.0f, 1.0f, 0.3f, 0.3f, 0.546f, 0.1f, 2.5f, 1.3f, 0.566f, 0.781f, 4.0f, 4, 0.0f, cloudColors
  };
  renderCloudsBuffer(frame, cloudLayer);
}

void renderLavaWorldPreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor baseColors[] = {
    makePlanetBufferColor(0.560784f, 0.301961f, 0.341176f),
    makePlanetBufferColor(0.321569f, 0.2f, 0.247059f),
    makePlanetBufferColor(0.239216f, 0.160784f, 0.211765f)
  };
  PlanetUnderLayer baseLayer = {
    1.0f, 1.0f, 0.3f, 0.3f, 0.2f, 2.0f, 0.4f, 0.6f, 10.0f, 3, 0.0f, baseColors
  };
  renderNoAtmosphereBaseBuffer(frame, baseLayer);

  const PlanetBufferColor craterColors[] = {
    makePlanetBufferColor(0.321569f, 0.2f, 0.247059f),
    makePlanetBufferColor(0.239216f, 0.160784f, 0.211765f)
  };
  PlanetCraterLayer craterLayer = {
    1.0f, 1.0f, 0.3f, 0.3f, 0.2f, 0.4f, 3.5f, 0.03f, 0, 0.0f, craterColors
  };
  renderCratersBuffer(frame, craterLayer);

  const PlanetBufferColor lavaColors[] = {
    makePlanetBufferColor(1.0f, 0.537255f, 0.2f),
    makePlanetBufferColor(0.901961f, 0.270588f, 0.223529f),
    makePlanetBufferColor(0.678431f, 0.184314f, 0.270588f)
  };
  PlanetLavaRiverLayer lavaLayer = {
    1.0f, 1.0f, 0.3f, 0.3f, 0.2f, 0.019f, 0.036f, 0.579f, 10.0f, 4, 0.0f, lavaColors
  };
  renderLavaRiversBuffer(frame, lavaLayer);
}

void renderAsteroidPreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor colors[] = {
    makePlanetBufferColor(0.639216f, 0.654902f, 0.760784f),
    makePlanetBufferColor(0.298039f, 0.407843f, 0.521569f),
    makePlanetBufferColor(0.227451f, 0.247059f, 0.368627f)
  };
  PlanetAsteroidLayer layer = {
    1.0f, 1.0f, 0.0f, 0.0f, 5.294f, 2, 0.0f, colors
  };
  renderAsteroidBuffer(frame, layer);
}

void renderBlackHolePreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor coreColors[] = {
    makePlanetBufferColor(0.152941f, 0.152941f, 0.211765f),
    makePlanetBufferColor(1.0f, 1.0f, 0.921569f),
    makePlanetBufferColor(0.929412f, 0.482353f, 0.223529f)
  };
  PlanetBlackHoleCoreLayer coreLayer = {
    1.0f, 1.0f, 0.247f, 0.028f, 0.0f, coreColors
  };
  renderBlackHoleCoreBuffer(frame, coreLayer);

  const PlanetBufferColor diskColors[] = {
    makePlanetBufferColor(1.0f, 1.0f, 0.921569f),
    makePlanetBufferColor(1.0f, 0.960784f, 0.25098f),
    makePlanetBufferColor(1.0f, 0.721569f, 0.290196f),
    makePlanetBufferColor(0.929412f, 0.482353f, 0.223529f),
    makePlanetBufferColor(0.741176f, 0.25098f, 0.207843f)
  };
  PlanetBlackHoleDiskLayer diskLayer = {
    3.0f, 3.0f, 0.2f, 0.065f, 14.0f, 0.766f, 6.598f, 3, diskColors, 5
  };
  renderBlackHoleDiskBuffer(frame, diskLayer);
}

void renderGalaxyPreview(const PlanetRenderFrame& frame) {
  const PlanetBufferColor colors[] = {
    makePlanetBufferColor(1.0f, 1.0f, 0.921569f),
    makePlanetBufferColor(1.0f, 0.913725f, 0.552941f),
    makePlanetBufferColor(0.709804f, 0.878431f, 0.4f),
    makePlanetBufferColor(0.396078f, 0.647059f, 0.4f),
    makePlanetBufferColor(0.223529f, 0.364706f, 0.392157f),
    makePlanetBufferColor(0.196078f, 0.223529f, 0.301961f),
    makePlanetBufferColor(0.196078f, 0.160784f, 0.278431f)
  };
  PlanetGalaxyLayer layer = {
    1.0f, 1.0f, 1.0f, 0.674f, 7.0f, 1, 3.0f, 4.0f, 0.4f, 1.375f, -9.0f, 6, colors, 7
  };
  renderGalaxyBuffer(frame, layer);
}

void renderStarPreview(const PlanetRenderFrame& frame) {
  bool evenSeed = (frame.colorSeed % 2U) == 0U;
  PlanetBufferColor starColors[4];
  PlanetBufferColor flareColors[2];
  if (evenSeed) {
    starColors[0] = makePlanetBufferColor(0.960784f, 1.0f, 0.909804f);
    starColors[1] = makePlanetBufferColor(1.0f, 0.847059f, 0.196078f);
    starColors[2] = makePlanetBufferColor(1.0f, 0.509804f, 0.231373f);
    starColors[3] = makePlanetBufferColor(0.486275f, 0.0980392f, 0.101961f);
    flareColors[0] = makePlanetBufferColor(1.0f, 0.847059f, 0.196078f);
    flareColors[1] = makePlanetBufferColor(0.960784f, 1.0f, 0.909804f);
  } else {
    starColors[0] = makePlanetBufferColor(0.960784f, 1.0f, 0.909804f);
    starColors[1] = makePlanetBufferColor(0.466667f, 0.839216f, 0.756863f);
    starColors[2] = makePlanetBufferColor(0.109804f, 0.572549f, 0.654902f);
    starColors[3] = makePlanetBufferColor(0.0117647f, 0.243137f, 0.368627f);
    flareColors[0] = makePlanetBufferColor(0.466667f, 0.839216f, 0.756863f);
    flareColors[1] = makePlanetBufferColor(0.960784f, 1.0f, 0.909804f);
  }

  PlanetStarBlobLayer blobLayer = {
    2.0f, 2.0f, 0.05f, 4.93f, 2.0f, 1.0f, 0.0f, makePlanetBufferColor(1.0f, 1.0f, 0.894118f)
  };
  renderStarBlobsBuffer(frame, blobLayer);

  PlanetStarCoreLayer coreLayer = {
    1.0f, 1.0f, 0.05f, 1, 0.0f, starColors, 4
  };
  renderStarCoreBuffer(frame, coreLayer);

  PlanetStarFlareLayer flareLayer = {
    2.0f, 2.0f, 0.05f, 0.3f, 0.0f, 2.0f, 1.0f, 1.0f, 1.6f, 4, 0.0f, flareColors, 2
  };
  renderStarFlaresBuffer(frame, flareLayer);
}

void renderPlanetPreviewToBuffer(const PlanetRenderFrame& frame) {
  if (planetPresetEqualsValue(frame.presetId, "terran_wet")) {
    renderTerranWetPreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "terran_dry")) {
    renderTerranDryPreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "islands")) {
    renderIslandsPreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "no_atmosphere")) {
    renderNoAtmospherePreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "gas_giant_1")) {
    renderGasGiantOnePreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "gas_giant_2")) {
    renderGasGiantTwoPreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "ice_world")) {
    renderIceWorldPreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "lava_world")) {
    renderLavaWorldPreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "asteroid")) {
    renderAsteroidPreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "black_hole")) {
    renderBlackHolePreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "galaxy")) {
    renderGalaxyPreview(frame);
    return;
  }
  if (planetPresetEqualsValue(frame.presetId, "star")) {
    renderStarPreview(frame);
    return;
  }

  renderTerranWetPreview(frame);
}

void renderPlanetPreviewFrame() {
  PlanetRenderFrame frame = buildPlanetRenderFrame(millis());
  clearPlanetPreviewBuffer();
  s_activePlanetColorVariant = buildPlanetColorVariant(frame);
  s_planetColorVariantActive = true;
  renderPlanetPreviewToBuffer(frame);
  s_planetColorVariantActive = false;
  renderPlanetBackgroundStars(frame);
  flushPlanetPreviewBuffer();
}

void renderPlanetPreviewToAnimationBuffer(const PlanetRenderFrame& frame) {
  clearPlanetPreviewBuffer();
  s_activePlanetColorVariant = buildPlanetColorVariant(frame);
  s_planetColorVariantActive = true;
  if (planetPresetEqualsValue(frame.presetId, "terran_wet")) {
    renderTerranWetPreviewOptimized(frame);
  } else {
    renderPlanetPreviewToBuffer(frame);
  }
  renderPlanetBackgroundStars(frame);
  s_planetColorVariantActive = false;
  storePlanetPreviewBuffer(&DisplayManager::animationBuffer[0][0]);
}

struct PlanetDirectLayerSample {
  float u;
  float v;
  float pixelU;
  float pixelV;
  float pixels;
};

struct PlanetDirectLayerSampleCache {
  bool valid;
  bool hit;
  uint32_t presetHash;
  float progress;
  float sizeScale;
  float centerX;
  float centerY;
  int pixels;
  float planeScale;
  float pixelsScale;
  int x;
  int y;
  PlanetDirectLayerSample sample;
};

struct PlanetDirectSphereGeometry {
  PlanetDirectLayerSample sample;
  bool ditherPixelU;
  bool ditherPixelV;
  float sphereX;
  float sphereY;
};

struct PlanetDirectSphereGeometryCache {
  bool valid;
  bool hit;
  uint32_t presetHash;
  float progress;
  float sizeScale;
  float centerX;
  float centerY;
  int pixels;
  float planeScale;
  float pixelsScale;
  int x;
  int y;
  PlanetDirectSphereGeometry geometry;
};

struct PlanetDirectLayerSampleGridCell {
  bool hit;
  PlanetDirectLayerSample sample;
};

struct PlanetDirectLayerSampleGridCache {
  bool valid;
  uint32_t presetHash;
  float progress;
  float sizeScale;
  float centerX;
  float centerY;
  int pixels;
  float planeScale;
  float pixelsScale;
  PlanetDirectLayerSampleGridCell* cells;
};

struct PlanetDirectSphereGeometryGridCell {
  bool hit;
  PlanetDirectSphereGeometry geometry;
};

struct PlanetDirectSphereGeometryGridCache {
  bool valid;
  uint32_t presetHash;
  float progress;
  float sizeScale;
  float centerX;
  float centerY;
  int pixels;
  float planeScale;
  float pixelsScale;
  PlanetDirectSphereGeometryGridCell* cells;
};

PlanetDirectLayerSampleCache* s_planetDirectLayerSampleCache = nullptr;
PlanetDirectSphereGeometryCache* s_planetDirectSphereGeometryCache = nullptr;
PlanetDirectLayerSampleGridCache* s_planetDirectLayerSampleGridCache = nullptr;
PlanetDirectSphereGeometryGridCache* s_planetDirectSphereGeometryGridCache = nullptr;
bool s_planetDirectGridCacheDisabled = false;

bool planetDirectDitherPixelU(const PlanetDirectLayerSample& sample);
bool planetDirectDitherPixelV(const PlanetDirectLayerSample& sample);

bool planetDirectCacheMatches(
  const PlanetRenderFrame& frame,
  float planeScale,
  float pixelsScale,
  int x,
  int y,
  uint32_t presetHash,
  uint32_t cachedPresetHash,
  float cachedProgress,
  float cachedSizeScale,
  float cachedCenterX,
  float cachedCenterY,
  int cachedPixels,
  float cachedPlaneScale,
  float cachedPixelsScale,
  int cachedX,
  int cachedY
) {
  return cachedPresetHash == presetHash &&
         cachedProgress == frame.progress &&
         cachedSizeScale == frame.sizeScale &&
         cachedCenterX == frame.centerX &&
         cachedCenterY == frame.centerY &&
         cachedPixels == frame.pixels &&
         cachedPlaneScale == planeScale &&
         cachedPixelsScale == pixelsScale &&
         cachedX == x &&
         cachedY == y;
}

int planetDirectGridCellIndex(int x, int y) {
  return y * kPlanetCanvasSize + x;
}

bool ensurePlanetDirectColorCacheStorage() {
  if (s_planetDirectColorCacheDisabled) {
    return false;
  }
  if (s_planetDirectColorCache != nullptr) {
    return true;
  }

  s_planetDirectColorCache = static_cast<PlanetDirectColorCacheEntry*>(
    allocatePlanetBuffer(sizeof(PlanetDirectColorCacheEntry) * (size_t)kPlanetDirectColorCacheSize)
  );
  if (s_planetDirectColorCache == nullptr) {
    s_planetDirectColorCacheDisabled = true;
    return false;
  }
  memset(s_planetDirectColorCache, 0, sizeof(PlanetDirectColorCacheEntry) * (size_t)kPlanetDirectColorCacheSize);
  return true;
}

bool ensurePlanetDirectLayerSampleCacheStorage() {
  if (s_planetDirectLayerSampleCache != nullptr) {
    return true;
  }

  s_planetDirectLayerSampleCache = static_cast<PlanetDirectLayerSampleCache*>(
    allocatePlanetBuffer(sizeof(PlanetDirectLayerSampleCache))
  );
  if (s_planetDirectLayerSampleCache == nullptr) {
    return false;
  }
  memset(s_planetDirectLayerSampleCache, 0, sizeof(PlanetDirectLayerSampleCache));
  return true;
}

bool ensurePlanetDirectSphereGeometryCacheStorage() {
  if (s_planetDirectSphereGeometryCache != nullptr) {
    return true;
  }

  s_planetDirectSphereGeometryCache = static_cast<PlanetDirectSphereGeometryCache*>(
    allocatePlanetBuffer(sizeof(PlanetDirectSphereGeometryCache))
  );
  if (s_planetDirectSphereGeometryCache == nullptr) {
    return false;
  }
  memset(s_planetDirectSphereGeometryCache, 0, sizeof(PlanetDirectSphereGeometryCache));
  return true;
}

bool ensurePlanetDirectLayerSampleGridCacheStorage() {
  if (s_planetDirectGridCacheDisabled) {
    return false;
  }
  if (s_planetDirectLayerSampleGridCache != nullptr) {
    return true;
  }

  s_planetDirectLayerSampleGridCache = static_cast<PlanetDirectLayerSampleGridCache*>(
    allocatePlanetBuffer(sizeof(PlanetDirectLayerSampleGridCache))
  );
  if (s_planetDirectLayerSampleGridCache == nullptr) {
    s_planetDirectGridCacheDisabled = true;
    return false;
  }
  memset(s_planetDirectLayerSampleGridCache, 0, sizeof(PlanetDirectLayerSampleGridCache));
  return true;
}

bool ensurePlanetDirectSphereGeometryGridCacheStorage() {
  if (s_planetDirectGridCacheDisabled) {
    return false;
  }
  if (s_planetDirectSphereGeometryGridCache != nullptr) {
    return true;
  }

  s_planetDirectSphereGeometryGridCache = static_cast<PlanetDirectSphereGeometryGridCache*>(
    allocatePlanetBuffer(sizeof(PlanetDirectSphereGeometryGridCache))
  );
  if (s_planetDirectSphereGeometryGridCache == nullptr) {
    s_planetDirectGridCacheDisabled = true;
    return false;
  }
  memset(s_planetDirectSphereGeometryGridCache, 0, sizeof(PlanetDirectSphereGeometryGridCache));
  return true;
}

bool ensurePlanetDirectLayerSampleGridStorage() {
  if (!ensurePlanetDirectLayerSampleGridCacheStorage() || s_planetDirectGridCacheDisabled) {
    return false;
  }
  if (s_planetDirectLayerSampleGridCache->cells != nullptr) {
    return true;
  }

  s_planetDirectLayerSampleGridCache->cells = static_cast<PlanetDirectLayerSampleGridCell*>(
    allocatePlanetBuffer(sizeof(PlanetDirectLayerSampleGridCell) * (size_t)kPlanetCanvasSize * (size_t)kPlanetCanvasSize)
  );
  if (s_planetDirectLayerSampleGridCache->cells == nullptr) {
    s_planetDirectGridCacheDisabled = true;
    return false;
  }
  return true;
}

bool ensurePlanetDirectSphereGeometryGridStorage() {
  if (!ensurePlanetDirectSphereGeometryGridCacheStorage() || s_planetDirectGridCacheDisabled) {
    return false;
  }
  if (s_planetDirectSphereGeometryGridCache->cells != nullptr) {
    return true;
  }

  s_planetDirectSphereGeometryGridCache->cells = static_cast<PlanetDirectSphereGeometryGridCell*>(
    allocatePlanetBuffer(
      sizeof(PlanetDirectSphereGeometryGridCell) * (size_t)kPlanetCanvasSize * (size_t)kPlanetCanvasSize
    )
  );
  if (s_planetDirectSphereGeometryGridCache->cells == nullptr) {
    s_planetDirectGridCacheDisabled = true;
    return false;
  }
  return true;
}

void buildPlanetDirectLayerSampleGrid(
  const PlanetRenderFrame& frame,
  uint32_t presetHash,
  float planeScale,
  float pixelsScale
) {
  PlanetDirectLayerSampleGridCache& cache = *s_planetDirectLayerSampleGridCache;
  float pixels = (float)frame.pixels * pixelsScale;
  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      PlanetDirectLayerSampleGridCell& cell = cache.cells[planetDirectGridCellIndex(x, y)];
      if (!mapPlanetPreviewPixelToUv(frame, planeScale, x, y, cell.sample.u, cell.sample.v)) {
        cell.hit = false;
        continue;
      }

      cell.hit = true;
      cell.sample.pixels = pixels;
      cell.sample.pixelU = quantizePlanetUv(cell.sample.u, pixels);
      cell.sample.pixelV = quantizePlanetUv(cell.sample.v, pixels);
    }
  }

  cache.valid = true;
  cache.presetHash = presetHash;
  cache.progress = frame.progress;
  cache.sizeScale = frame.sizeScale;
  cache.centerX = frame.centerX;
  cache.centerY = frame.centerY;
  cache.pixels = frame.pixels;
  cache.planeScale = planeScale;
  cache.pixelsScale = pixelsScale;
}

void buildPlanetDirectSphereGeometryGrid(
  const PlanetRenderFrame& frame,
  uint32_t presetHash,
  float planeScale,
  float pixelsScale
) {
  PlanetDirectSphereGeometryGridCache& cache = *s_planetDirectSphereGeometryGridCache;
  float pixels = (float)frame.pixels * pixelsScale;
  for (int y = 0; y < kPlanetCanvasSize; y += 1) {
    for (int x = 0; x < kPlanetCanvasSize; x += 1) {
      PlanetDirectSphereGeometryGridCell& cell = cache.cells[planetDirectGridCellIndex(x, y)];
      PlanetDirectLayerSample& sample = cell.geometry.sample;
      if (!mapPlanetPreviewPixelToUv(frame, planeScale, x, y, sample.u, sample.v)) {
        cell.hit = false;
        continue;
      }

      sample.pixels = pixels;
      sample.pixelU = quantizePlanetUv(sample.u, pixels);
      sample.pixelV = quantizePlanetUv(sample.v, pixels);
      if (!planetDirectInsideCircle(sample.pixelU, sample.pixelV)) {
        cell.hit = false;
        continue;
      }

      cell.hit = true;
      cell.geometry.ditherPixelU = planetDirectDitherPixelU(sample);
      cell.geometry.ditherPixelV = planetDirectDitherPixelV(sample);
      spherifyPlanetUv(sample.pixelU, sample.pixelV, cell.geometry.sphereX, cell.geometry.sphereY);
    }
  }

  cache.valid = true;
  cache.presetHash = presetHash;
  cache.progress = frame.progress;
  cache.sizeScale = frame.sizeScale;
  cache.centerX = frame.centerX;
  cache.centerY = frame.centerY;
  cache.pixels = frame.pixels;
  cache.planeScale = planeScale;
  cache.pixelsScale = pixelsScale;
}

bool preparePlanetDirectLayerSampleFromGrid(
  const PlanetRenderFrame& frame,
  float planeScale,
  float pixelsScale,
  int x,
  int y,
  PlanetDirectLayerSample& sample
) {
  if (!ensurePlanetDirectLayerSampleGridStorage()) {
    return false;
  }

  uint32_t presetHash = hashPlanetString(frame.presetId);
  PlanetDirectLayerSampleGridCache& cache = *s_planetDirectLayerSampleGridCache;
  if (!cache.valid ||
      !planetDirectCacheMatches(
        frame,
        planeScale,
        pixelsScale,
        x,
        y,
        presetHash,
        cache.presetHash,
        cache.progress,
        cache.sizeScale,
        cache.centerX,
        cache.centerY,
        cache.pixels,
        cache.planeScale,
        cache.pixelsScale,
        x,
        y
      )) {
    buildPlanetDirectLayerSampleGrid(frame, presetHash, planeScale, pixelsScale);
  }

  const PlanetDirectLayerSampleGridCell& cell =
    cache.cells[planetDirectGridCellIndex(x, y)];
  if (!cell.hit) {
    return false;
  }

  sample = cell.sample;
  return true;
}

bool preparePlanetDirectSphereGeometryFromGrid(
  const PlanetRenderFrame& frame,
  float planeScale,
  float pixelsScale,
  int x,
  int y,
  PlanetDirectSphereGeometry& geometry
) {
  if (!ensurePlanetDirectSphereGeometryGridStorage()) {
    return false;
  }

  uint32_t presetHash = hashPlanetString(frame.presetId);
  PlanetDirectSphereGeometryGridCache& cache = *s_planetDirectSphereGeometryGridCache;
  if (!cache.valid ||
      !planetDirectCacheMatches(
        frame,
        planeScale,
        pixelsScale,
        x,
        y,
        presetHash,
        cache.presetHash,
        cache.progress,
        cache.sizeScale,
        cache.centerX,
        cache.centerY,
        cache.pixels,
        cache.planeScale,
        cache.pixelsScale,
        x,
        y
      )) {
    buildPlanetDirectSphereGeometryGrid(frame, presetHash, planeScale, pixelsScale);
  }

  const PlanetDirectSphereGeometryGridCell& cell =
    cache.cells[planetDirectGridCellIndex(x, y)];
  if (!cell.hit) {
    return false;
  }

  geometry = cell.geometry;
  return true;
}

bool preparePlanetDirectLayerSample(
  const PlanetRenderFrame& frame,
  float planeScale,
  float pixelsScale,
  int x,
  int y,
  PlanetDirectLayerSample& sample
) {
  if (preparePlanetDirectLayerSampleFromGrid(frame, planeScale, pixelsScale, x, y, sample)) {
    return true;
  }
  if (!ensurePlanetDirectLayerSampleCacheStorage()) {
    if (!mapPlanetPreviewPixelToUv(frame, planeScale, x, y, sample.u, sample.v)) {
      return false;
    }
    sample.pixels = (float)frame.pixels * pixelsScale;
    sample.pixelU = quantizePlanetUv(sample.u, sample.pixels);
    sample.pixelV = quantizePlanetUv(sample.v, sample.pixels);
    return true;
  }

  PlanetDirectLayerSampleCache& cache = *s_planetDirectLayerSampleCache;
  uint32_t presetHash = hashPlanetString(frame.presetId);
  if (cache.valid &&
      planetDirectCacheMatches(
        frame,
        planeScale,
        pixelsScale,
        x,
        y,
        presetHash,
        cache.presetHash,
        cache.progress,
        cache.sizeScale,
        cache.centerX,
        cache.centerY,
        cache.pixels,
        cache.planeScale,
        cache.pixelsScale,
        cache.x,
        cache.y
      )) {
    if (!cache.hit) {
      return false;
    }
    sample = cache.sample;
    return true;
  }

  if (!mapPlanetPreviewPixelToUv(frame, planeScale, x, y, sample.u, sample.v)) {
    cache.valid = true;
    cache.hit = false;
    cache.presetHash = presetHash;
    cache.progress = frame.progress;
    cache.sizeScale = frame.sizeScale;
    cache.centerX = frame.centerX;
    cache.centerY = frame.centerY;
    cache.pixels = frame.pixels;
    cache.planeScale = planeScale;
    cache.pixelsScale = pixelsScale;
    cache.x = x;
    cache.y = y;
    return false;
  }

  sample.pixels = (float)frame.pixels * pixelsScale;
  sample.pixelU = quantizePlanetUv(sample.u, sample.pixels);
  sample.pixelV = quantizePlanetUv(sample.v, sample.pixels);
  cache.valid = true;
  cache.hit = true;
  cache.presetHash = presetHash;
  cache.progress = frame.progress;
  cache.sizeScale = frame.sizeScale;
  cache.centerX = frame.centerX;
  cache.centerY = frame.centerY;
  cache.pixels = frame.pixels;
  cache.planeScale = planeScale;
  cache.pixelsScale = pixelsScale;
  cache.x = x;
  cache.y = y;
  cache.sample = sample;
  return true;
}

bool preparePlanetDirectSphereGeometry(
  const PlanetRenderFrame& frame,
  float planeScale,
  float pixelsScale,
  int x,
  int y,
  PlanetDirectSphereGeometry& geometry
) {
  if (preparePlanetDirectSphereGeometryFromGrid(frame, planeScale, pixelsScale, x, y, geometry)) {
    return true;
  }
  if (!ensurePlanetDirectSphereGeometryCacheStorage()) {
    PlanetDirectLayerSample sample;
    if (!preparePlanetDirectLayerSample(frame, planeScale, pixelsScale, x, y, sample) ||
        !planetDirectInsideCircle(sample.pixelU, sample.pixelV)) {
      return false;
    }
    geometry.sample = sample;
    geometry.ditherPixelU = planetDirectDitherPixelU(sample);
    geometry.ditherPixelV = planetDirectDitherPixelV(sample);
    spherifyPlanetUv(sample.pixelU, sample.pixelV, geometry.sphereX, geometry.sphereY);
    return true;
  }

  PlanetDirectSphereGeometryCache& cache = *s_planetDirectSphereGeometryCache;
  uint32_t presetHash = hashPlanetString(frame.presetId);
  if (cache.valid &&
      planetDirectCacheMatches(
        frame,
        planeScale,
        pixelsScale,
        x,
        y,
        presetHash,
        cache.presetHash,
        cache.progress,
        cache.sizeScale,
        cache.centerX,
        cache.centerY,
        cache.pixels,
        cache.planeScale,
        cache.pixelsScale,
        cache.x,
        cache.y
      )) {
    if (!cache.hit) {
      return false;
    }
    geometry = cache.geometry;
    return true;
  }

  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, planeScale, pixelsScale, x, y, sample) ||
      !planetDirectInsideCircle(sample.pixelU, sample.pixelV)) {
    cache.valid = true;
    cache.hit = false;
    cache.presetHash = presetHash;
    cache.progress = frame.progress;
    cache.sizeScale = frame.sizeScale;
    cache.centerX = frame.centerX;
    cache.centerY = frame.centerY;
    cache.pixels = frame.pixels;
    cache.planeScale = planeScale;
    cache.pixelsScale = pixelsScale;
    cache.x = x;
    cache.y = y;
    return false;
  }

  geometry.sample = sample;
  geometry.ditherPixelU = planetDirectDitherPixelU(sample);
  geometry.ditherPixelV = planetDirectDitherPixelV(sample);
  spherifyPlanetUv(sample.pixelU, sample.pixelV, geometry.sphereX, geometry.sphereY);

  cache.valid = true;
  cache.hit = true;
  cache.presetHash = presetHash;
  cache.progress = frame.progress;
  cache.sizeScale = frame.sizeScale;
  cache.centerX = frame.centerX;
  cache.centerY = frame.centerY;
  cache.pixels = frame.pixels;
  cache.planeScale = planeScale;
  cache.pixelsScale = pixelsScale;
  cache.x = x;
  cache.y = y;
  cache.geometry = geometry;
  return true;
}

bool planetDirectAlphaHit(const PlanetRenderFrame& frame, int x, int y, float alpha) {
  float clampedAlpha = clampFloat(alpha, 0.0f, 1.0f);
  if (clampedAlpha <= 0.0f) {
    return false;
  }
  if (!frame.dither || clampedAlpha >= 1.0f) {
    return true;
  }
  return shouldDrawPlanetDirectPixel(x, y, clampedAlpha);
}

bool planetDirectDitherPixelU(const PlanetDirectLayerSample& sample) {
  return modPlanetFloat(sample.pixelU + sample.v, 2.0f / sample.pixels) <=
         1.0f / sample.pixels;
}

bool planetDirectDitherPixelV(const PlanetDirectLayerSample& sample) {
  return modPlanetFloat(sample.u + sample.pixelV, 2.0f / sample.pixels) <=
         1.0f / sample.pixels;
}

float squarePlanetValue(float value) {
  return value * value;
}

void resolvePlanetDirectRotatedPixelUv(
  const PlanetDirectLayerSample& sample,
  float rotationOffset,
  float& rotatedX,
  float& rotatedY
) {
  if (rotationOffset == 0.0f) {
    rotatedX = sample.pixelU;
    rotatedY = sample.pixelV;
    return;
  }
  rotatePlanetUv(sample.pixelU, sample.pixelV, rotationOffset, rotatedX, rotatedY);
}

void resolvePlanetDirectSphereUv(
  const PlanetDirectSphereGeometry& geometry,
  float rotationOffset,
  bool rotateBeforeSpherify,
  float& outX,
  float& outY
) {
  if (rotationOffset == 0.0f) {
    outX = geometry.sphereX;
    outY = geometry.sphereY;
    return;
  }

  if (rotateBeforeSpherify) {
    float rotatedX = 0.0f;
    float rotatedY = 0.0f;
    rotatePlanetUv(geometry.sample.pixelU, geometry.sample.pixelV, rotationOffset, rotatedX, rotatedY);
    spherifyPlanetUv(rotatedX, rotatedY, outX, outY);
    return;
  }

  rotatePlanetUv(geometry.sphereX, geometry.sphereY, rotationOffset, outX, outY);
}

float accumulatePlanetDirectCloudNoise(
  float scaledX,
  float scaledY,
  float timeOffset,
  float shaderSeed,
  float size,
  int count
) {
  int effectiveCount = count < 1 ? 1 : count;
  if (s_planetDirectRenderActive && effectiveCount > kPlanetDirectMaxCloudNoiseSamples) {
    effectiveCount = kPlanetDirectMaxCloudNoiseSamples;
  }
  float noise = 0.0f;
  float noiseX = scaledX * 0.3f + 11.0f + timeOffset;
  float noiseY = scaledY * 0.3f;
  bool cachedNoiseReady =
    buildPlanetTiledRandCache(s_planetDirectNoiseRandCache, shaderSeed, size, 1.0f, 1.0f);
  for (int index = 0; index < effectiveCount; index += 1) {
    if (cachedNoiseReady) {
      noise += circleNoisePlanetCloudCached(
        noiseX + (float)index,
        noiseY,
        s_planetDirectNoiseRandCache
      );
    } else {
      noise += circleNoisePlanetCloud(
        noiseX + (float)index,
        noiseY,
        shaderSeed,
        size
      );
    }
  }
  return noise;
}

float resolvePlanetTiledNoisePeriod(float size, float tileScale) {
  return fmaxf(1.0f, roundf(size)) * tileScale;
}

uint8_t quantizePlanetTileValue(float value, float maxValue) {
  if (maxValue <= 0.0f) {
    return 0;
  }
  float normalized = clampFloat(value / maxValue, 0.0f, 1.0f);
  return (uint8_t)roundf(normalized * 255.0f);
}

float decodePlanetTileValue(uint8_t value, float maxValue) {
  return ((float)value / 255.0f) * maxValue;
}

float samplePlanetTileBilinear(
  const uint8_t* tile,
  int width,
  int height,
  float periodX,
  float periodY,
  float sampleX,
  float sampleY,
  float maxValue
) {
  float wrappedX = modPlanetFloat(sampleX, periodX);
  float wrappedY = modPlanetFloat(sampleY, periodY);
  float texX = wrappedX / periodX * (float)width;
  float texY = wrappedY / periodY * (float)height;
  int x0 = (int)floorf(texX);
  int y0 = (int)floorf(texY);
  float fx = texX - (float)x0;
  float fy = texY - (float)y0;

  if (x0 >= width) {
    x0 = 0;
  }
  if (y0 >= height) {
    y0 = 0;
  }

  int x1 = x0 + 1;
  int y1 = y0 + 1;
  if (x1 >= width) {
    x1 = 0;
  }
  if (y1 >= height) {
    y1 = 0;
  }

  float v00 = decodePlanetTileValue(tile[y0 * width + x0], maxValue);
  float v10 = decodePlanetTileValue(tile[y0 * width + x1], maxValue);
  float v01 = decodePlanetTileValue(tile[y1 * width + x0], maxValue);
  float v11 = decodePlanetTileValue(tile[y1 * width + x1], maxValue);
  float top = mixFloat(v00, v10, fx);
  float bottom = mixFloat(v01, v11, fx);
  return mixFloat(top, bottom, fy);
}

bool planetGenericFbmTileMatches(
  const PlanetGenericFbmTileCache& cache,
  float seed,
  float size,
  int octaves,
  float tileX,
  float tileY
) {
  return cache.valid &&
         cache.seedBits == planetFloatBits(seed) &&
         cache.sizeBits == planetFloatBits(size) &&
         cache.tileXBits == planetFloatBits(tileX) &&
         cache.tileYBits == planetFloatBits(tileY) &&
         cache.octaves == octaves;
}

PlanetGenericFbmTileCache* acquirePlanetGenericFbmTileCache(
  float seed,
  float size,
  int octaves,
  float tileX,
  float tileY
) {
  if (s_planetGenericFbmTileCacheDisabled) {
    return nullptr;
  }

  s_planetGenericFbmTileUseTick += 1UL;
  PlanetGenericFbmTileCache* reuseSlot = nullptr;
  for (int index = 0; index < kPlanetGenericFbmTileSlotCount; index += 1) {
    PlanetGenericFbmTileCache& cache = s_planetGenericFbmTileCaches[index];
    if (planetGenericFbmTileMatches(cache, seed, size, octaves, tileX, tileY)) {
      cache.useTick = s_planetGenericFbmTileUseTick;
      return &cache;
    }
    if (reuseSlot == nullptr && !cache.valid) {
      reuseSlot = &cache;
    }
  }

  if (reuseSlot == nullptr) {
    reuseSlot = &s_planetGenericFbmTileCaches[0];
    for (int index = 1; index < kPlanetGenericFbmTileSlotCount; index += 1) {
      if (s_planetGenericFbmTileCaches[index].useTick < reuseSlot->useTick) {
        reuseSlot = &s_planetGenericFbmTileCaches[index];
      }
    }
  }

  if (reuseSlot->values == nullptr) {
    reuseSlot->values = static_cast<uint8_t*>(
      allocatePlanetBuffer((size_t)kPlanetGenericFbmTileWidth * (size_t)kPlanetGenericFbmTileHeight)
    );
    if (reuseSlot->values == nullptr) {
      s_planetGenericFbmTileCacheDisabled = true;
      return nullptr;
    }
  }

  float periodX = resolvePlanetTiledNoisePeriod(size, tileX);
  float periodY = resolvePlanetTiledNoisePeriod(size, tileY);
  s_planetGenericFbmTileBuildDepth += 1;
  for (int y = 0; y < kPlanetGenericFbmTileHeight; y += 1) {
    float sampleY = ((float)y + 0.5f) / (float)kPlanetGenericFbmTileHeight * periodY;
    for (int x = 0; x < kPlanetGenericFbmTileWidth; x += 1) {
      float sampleX = ((float)x + 0.5f) / (float)kPlanetGenericFbmTileWidth * periodX;
      float value = fbmPlanetPreviewTiledRaw(sampleX, sampleY, seed, size, octaves, tileX, tileY);
      reuseSlot->values[y * kPlanetGenericFbmTileWidth + x] =
        quantizePlanetTileValue(value, 1.0f);
    }
  }
  s_planetGenericFbmTileBuildDepth -= 1;

  reuseSlot->valid = true;
  reuseSlot->seedBits = planetFloatBits(seed);
  reuseSlot->sizeBits = planetFloatBits(size);
  reuseSlot->tileXBits = planetFloatBits(tileX);
  reuseSlot->tileYBits = planetFloatBits(tileY);
  reuseSlot->octaves = octaves;
  reuseSlot->useTick = s_planetGenericFbmTileUseTick;
  return reuseSlot;
}

float samplePlanetGenericFbmTiled(
  float x,
  float y,
  float seed,
  float size,
  int octaves,
  float tileX,
  float tileY
) {
  int effectiveOctaves = resolvePlanetDirectOctaves(octaves);
  PlanetGenericFbmTileCache* cache = acquirePlanetGenericFbmTileCache(
    seed,
    size,
    effectiveOctaves,
    tileX,
    tileY
  );
  if (cache == nullptr || cache->values == nullptr) {
    return fbmPlanetPreviewTiledRaw(x, y, seed, size, effectiveOctaves, tileX, tileY);
  }

  float periodX = resolvePlanetTiledNoisePeriod(size, tileX);
  float periodY = resolvePlanetTiledNoisePeriod(size, tileY);
  return samplePlanetTileBilinear(
    cache->values,
    kPlanetGenericFbmTileWidth,
    kPlanetGenericFbmTileHeight,
    periodX,
    periodY,
    x,
    y,
    1.0f
  );
}

bool ensureTerranWetTileStorage(
  uint8_t*& tile,
  size_t bytes,
  const char* label
) {
  if (tile != nullptr) {
    return true;
  }

  tile = static_cast<uint8_t*>(allocatePlanetBuffer(bytes));
  if (tile == nullptr) {
    Serial.printf("terran wet tile alloc failed: %s %u bytes\n", label, (unsigned int)bytes);
    return false;
  }
  memset(tile, 0, bytes);
  return true;
}

void buildTerranWetLandTile(
  const PlanetRenderFrame& frame,
  const PlanetLandRiverLayer& layer
) {
  bool directRenderWasActive = s_planetDirectRenderActive;
  s_planetDirectRenderActive = false;
  float periodX = resolvePlanetTiledNoisePeriod(layer.size, 2.0f);
  float periodY = resolvePlanetTiledNoisePeriod(layer.size, 1.0f);

  for (int y = 0; y < kPlanetTerranWetLandTileHeight; y += 1) {
    float sampleY = ((float)y + 0.5f) / (float)kPlanetTerranWetLandTileHeight * periodY;
    for (int x = 0; x < kPlanetTerranWetLandTileWidth; x += 1) {
      float sampleX = ((float)x + 0.5f) / (float)kPlanetTerranWetLandTileWidth * periodX;
      float value = fbmPlanetPreviewTiled(
        sampleX,
        sampleY,
        frame.shaderSeed,
        layer.size,
        layer.octaves,
        2.0f,
        1.0f
      );
      s_planetTerranWetLandTile[y * kPlanetTerranWetLandTileWidth + x] =
        quantizePlanetTileValue(value, 1.0f);
    }
  }

  s_planetDirectRenderActive = directRenderWasActive;
  s_planetTerranWetLandTileState.valid = true;
  s_planetTerranWetLandTileState.seed = frame.seed;
}

void buildTerranWetCloudTile(
  const PlanetRenderFrame& frame,
  const PlanetCloudLayer& layer
) {
  bool directRenderWasActive = s_planetDirectRenderActive;
  s_planetDirectRenderActive = false;
  float period = resolvePlanetTiledNoisePeriod(layer.size, 1.0f);

  for (int y = 0; y < kPlanetTerranWetCloudTileHeight; y += 1) {
    float sampleY = ((float)y + 0.5f) / (float)kPlanetTerranWetCloudTileHeight * period;
    for (int x = 0; x < kPlanetTerranWetCloudTileWidth; x += 1) {
      float sampleX = ((float)x + 0.5f) / (float)kPlanetTerranWetCloudTileWidth * period;
      float value = fbmPlanetPreviewTiled(
        sampleX,
        sampleY,
        frame.shaderSeed,
        layer.size,
        layer.octaves
      );
      s_planetTerranWetCloudTile[y * kPlanetTerranWetCloudTileWidth + x] =
        quantizePlanetTileValue(value, 1.0f);
    }
  }

  s_planetDirectRenderActive = directRenderWasActive;
  s_planetTerranWetCloudTileState.valid = true;
  s_planetTerranWetCloudTileState.seed = frame.seed;
}

void buildTerranWetCloudNoiseTile(
  const PlanetRenderFrame& frame,
  const PlanetCloudLayer& layer
) {
  float period = resolvePlanetTiledNoisePeriod(layer.size, 1.0f);

  for (int y = 0; y < kPlanetTerranWetCloudTileHeight; y += 1) {
    float sampleY = ((float)y + 0.5f) / (float)kPlanetTerranWetCloudTileHeight * period;
    for (int x = 0; x < kPlanetTerranWetCloudTileWidth; x += 1) {
      float sampleX = ((float)x + 0.5f) / (float)kPlanetTerranWetCloudTileWidth * period;
      float noise = 0.0f;
      for (int index = 0; index < kPlanetTerranWetCloudNoiseSamples; index += 1) {
        noise += circleNoisePlanetCloud(
          sampleX + (float)index + 11.0f,
          sampleY,
          frame.shaderSeed,
          layer.size
        );
      }
      s_planetTerranWetCloudNoiseTile[y * kPlanetTerranWetCloudTileWidth + x] =
        quantizePlanetTileValue(noise, kPlanetTerranWetCloudNoiseMax);
    }
  }

  s_planetTerranWetCloudNoiseTileState.valid = true;
  s_planetTerranWetCloudNoiseTileState.seed = frame.seed;
}

bool prepareTerranWetTiles(
  const PlanetRenderFrame& frame,
  const PlanetLandRiverLayer& landLayer,
  const PlanetCloudLayer& cloudLayer
) {
  if (s_planetTerranWetTileCacheDisabled) {
    return false;
  }

  if (!ensureTerranWetTileStorage(
        s_planetTerranWetLandTile,
        (size_t)kPlanetTerranWetLandTileWidth * (size_t)kPlanetTerranWetLandTileHeight,
        "land"
      )) {
    s_planetTerranWetTileCacheDisabled = true;
    return false;
  }
  if (!ensureTerranWetTileStorage(
        s_planetTerranWetCloudTile,
        (size_t)kPlanetTerranWetCloudTileWidth * (size_t)kPlanetTerranWetCloudTileHeight,
        "cloud"
      )) {
    s_planetTerranWetTileCacheDisabled = true;
    return false;
  }
  if (!ensureTerranWetTileStorage(
        s_planetTerranWetCloudNoiseTile,
        (size_t)kPlanetTerranWetCloudTileWidth * (size_t)kPlanetTerranWetCloudTileHeight,
        "cloud_noise"
      )) {
    s_planetTerranWetTileCacheDisabled = true;
    return false;
  }

  if (!s_planetTerranWetLandTileState.valid || s_planetTerranWetLandTileState.seed != frame.seed) {
    buildTerranWetLandTile(frame, landLayer);
  }
  if (!s_planetTerranWetCloudTileState.valid || s_planetTerranWetCloudTileState.seed != frame.seed) {
    buildTerranWetCloudTile(frame, cloudLayer);
  }
  if (!s_planetTerranWetCloudNoiseTileState.valid ||
      s_planetTerranWetCloudNoiseTileState.seed != frame.seed) {
    buildTerranWetCloudNoiseTile(frame, cloudLayer);
  }
  return true;
}

float loopPlanetDirectTimeFromMultiplier(
  const PlanetRenderFrame& frame,
  float size,
  float timeSpeed
) {
  return loopPlanetTimeFromMultiplier(frame.progress, size, timeSpeed, frame.motionFactor);
}

float loopPlanetDirectTimeFromRotation(
  const PlanetRenderFrame& frame,
  float timeSpeed,
  float factor
) {
  return loopPlanetTimeFromRotation(frame.progress, timeSpeed, factor);
}

float loopPlanetDirectTimeFromRing(
  const PlanetRenderFrame& frame,
  float timeSpeed,
  float factor,
  float scale = 0.5f
) {
  return frame.progress * factor * kPlanetRingTimeBase * timeSpeed * scale;
}

float loopPlanetDirectTimeFromGalaxy(
  const PlanetRenderFrame& frame,
  float timeSpeed
) {
  return frame.progress * frame.motionFactor * kPlanetTwoPi * timeSpeed;
}

float loopPlanetDirectTimeFromProgress(
  const PlanetRenderFrame& frame,
  float timeSpeed
) {
  return frame.progress * frame.motionFactor / timeSpeed;
}

void resolvePlanetDirectBounds(
  const PlanetRenderFrame& frame,
  float planeScale,
  int& minX,
  int& maxXExclusive,
  int& minY,
  int& maxYExclusive
) {
  float planeSize = resolvePlanetPlaneSizeFor(frame, planeScale);
  float halfPlane = planeSize * 0.5f;
  minX = clampInt((int)floorf(frame.centerX - halfPlane - 0.5f), 0, kPlanetCanvasSize - 1);
  minY = clampInt((int)floorf(frame.centerY - halfPlane - 0.5f), 0, kPlanetCanvasSize - 1);
  maxXExclusive = clampInt((int)ceilf(frame.centerX + halfPlane + 0.5f), 1, kPlanetCanvasSize);
  maxYExclusive = clampInt((int)ceilf(frame.centerY + halfPlane + 0.5f), 1, kPlanetCanvasSize);
}

PlanetRgb planetBufferColorToRgb(const PlanetBufferColor& color) {
  return makePlanetRgbRaw(
    (uint8_t)roundf(clampFloat(color.r, 0.0f, 1.0f) * 255.0f),
    (uint8_t)roundf(clampFloat(color.g, 0.0f, 1.0f) * 255.0f),
    (uint8_t)roundf(clampFloat(color.b, 0.0f, 1.0f) * 255.0f)
  );
}

uint16_t resolvePlanetDirectColor565(const PlanetBufferColor& color) {
  uint32_t redBits = planetFloatBits(color.r);
  uint32_t greenBits = planetFloatBits(color.g);
  uint32_t blueBits = planetFloatBits(color.b);
  PlanetRgb resolved = planetBufferColorToRgb(color);
  if (s_planetDirectColorVariantActive) {
    resolved = applyPlanetDirectColorVariant(resolved);
  }
  if (!ensurePlanetDirectColorCacheStorage()) {
    return planetColor565(resolved);
  }
  uint32_t hash = redBits;
  hash ^= greenBits * 33UL;
  hash ^= blueBits * 97UL;
  int slot = (int)(hash % (uint32_t)kPlanetDirectColorCacheSize);
  PlanetDirectColorCacheEntry& entry = s_planetDirectColorCache[slot];
  if (entry.valid &&
      entry.redBits == redBits &&
      entry.greenBits == greenBits &&
      entry.blueBits == blueBits) {
    return entry.color565;
  }

  entry.redBits = redBits;
  entry.greenBits = greenBits;
  entry.blueBits = blueBits;
  entry.color565 = planetColor565(resolved);
  entry.valid = true;
  return entry.color565;
}

void drawPlanetDirectBufferColor(MatrixPanel_I2S_DMA* display, int x, int y, const PlanetBufferColor& color) {
  display->drawPixel(x, y, resolvePlanetDirectColor565(color));
}

void drawPlanetDirectBlockColor(
  MatrixPanel_I2S_DMA* display,
  int x,
  int y,
  int step,
  const PlanetBufferColor& color
) {
  uint16_t color565 = resolvePlanetDirectColor565(color);
  int maxXExclusive = x + step;
  int maxYExclusive = y + step;
  if (maxXExclusive > kPlanetCanvasSize) {
    maxXExclusive = kPlanetCanvasSize;
  }
  if (maxYExclusive > kPlanetCanvasSize) {
    maxYExclusive = kPlanetCanvasSize;
  }
  for (int py = y; py < maxYExclusive; py += 1) {
    for (int px = x; px < maxXExclusive; px += 1) {
      display->drawPixel(px, py, color565);
    }
  }
}

int resolvePlanetDirectRenderStep(const PlanetRenderFrame& frame) {
  if (planetPresetEqualsValue(frame.presetId, "black_hole") ||
      planetPresetEqualsValue(frame.presetId, "galaxy") ||
      planetPresetEqualsValue(frame.presetId, "star")) {
    return kPlanetDirectSpecialRenderStep;
  }
  return kPlanetDirectSphereRenderStep;
}

void resolvePlanetDirectSamplePoint(
  int x,
  int y,
  int sampleStep,
  int maxXExclusive,
  int maxYExclusive,
  int& sampleX,
  int& sampleY
) {
  if (sampleStep <= 1) {
    sampleX = x;
    sampleY = y;
    return;
  }

  sampleX = x + sampleStep / 2;
  sampleY = y + sampleStep / 2;
  if (sampleX >= maxXExclusive) {
    sampleX = maxXExclusive - 1;
  }
  if (sampleY >= maxYExclusive) {
    sampleY = maxYExclusive - 1;
  }
}

bool samplePlanetCloudDirect(
  const PlanetRenderFrame& frame,
  const PlanetCloudLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
);

bool samplePlanetCloudDirectWithNoiseSamples(
  const PlanetRenderFrame& frame,
  const PlanetCloudLayer& layer,
  int x,
  int y,
  int noiseSamples,
  PlanetBufferColor& outColor
);

bool samplePlanetLandRiversDirect(
  const PlanetRenderFrame& frame,
  const PlanetLandRiverLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
);

bool sampleTerranWetDirectCached(
  const PlanetRenderFrame& frame,
  const PlanetLandRiverLayer& landLayer,
  const PlanetCloudLayer& cloudLayer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, cloudLayer.planeScale, cloudLayer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  float sphereX = 0.0f;
  float sphereY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, cloudLayer.rotationOffset, true, sphereX, sphereY);

  float cloudDistance = distancePlanet(sample.pixelU, sample.pixelV, cloudLayer.lightOriginX, cloudLayer.lightOriginY);
  float sphereCloudY = sphereY + smoothstepFloat(0.0f, cloudLayer.cloudCurve, fabsf(sphereX - 0.4f));
  sphereCloudY *= cloudLayer.stretch;

  float cloudTimeOffset = loopPlanetDirectTimeFromMultiplier(frame, cloudLayer.size, cloudLayer.timeSpeed) *
                          cloudLayer.timeSpeed;
  float scaledSphereX = sphereX * cloudLayer.size;
  float scaledSphereCloudY = sphereCloudY * cloudLayer.size;
  float cloudPeriod = resolvePlanetTiledNoisePeriod(cloudLayer.size, 1.0f);
  float cloudNoise = samplePlanetTileBilinear(
    s_planetTerranWetCloudNoiseTile,
    kPlanetTerranWetCloudTileWidth,
    kPlanetTerranWetCloudTileHeight,
    cloudPeriod,
    cloudPeriod,
    scaledSphereX * 0.3f + cloudTimeOffset,
    scaledSphereCloudY * 0.3f,
    kPlanetTerranWetCloudNoiseMax
  );
  float cloudValue = samplePlanetTileBilinear(
    s_planetTerranWetCloudTile,
    kPlanetTerranWetCloudTileWidth,
    kPlanetTerranWetCloudTileHeight,
    cloudPeriod,
    cloudPeriod,
    scaledSphereX + cloudNoise + cloudTimeOffset,
    scaledSphereCloudY,
    1.0f
  );

  PlanetBufferColor cloudColor = cloudLayer.colors[0];
  if (cloudValue < cloudLayer.cloudCover + 0.03f) {
    cloudColor = cloudLayer.colors[1];
  }
  if (cloudDistance + cloudValue * 0.2f > cloudLayer.lightBorder1) {
    cloudColor = cloudLayer.colors[2];
  }
  if (cloudDistance + cloudValue * 0.2f > cloudLayer.lightBorder2) {
    cloudColor = cloudLayer.colors[3];
  }

  float cloudAlpha = planetStep(cloudLayer.cloudCover, cloudValue);
  if (planetDirectAlphaHit(frame, x, y, cloudAlpha * cloudColor.a)) {
    outColor = cloudColor;
    return true;
  }

  bool dith = geometry.ditherPixelU;
  float landDistance = distancePlanet(sphereX, sphereY, landLayer.lightOriginX, landLayer.lightOriginY);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, landLayer.rotationOffset, false, rotatedX, rotatedY);
  float landTimeOffset = loopPlanetDirectTimeFromMultiplier(frame, landLayer.size, landLayer.timeSpeed) *
                         landLayer.timeSpeed;
  float baseX = rotatedX * landLayer.size + landTimeOffset;
  float baseY = rotatedY * landLayer.size;
  float landPeriodX = resolvePlanetTiledNoisePeriod(landLayer.size, 2.0f);
  float landPeriodY = resolvePlanetTiledNoisePeriod(landLayer.size, 1.0f);
  float fbm1 = samplePlanetTileBilinear(
    s_planetTerranWetLandTile,
    kPlanetTerranWetLandTileWidth,
    kPlanetTerranWetLandTileHeight,
    landPeriodX,
    landPeriodY,
    baseX,
    baseY,
    1.0f
  );
  float fbm2Value = samplePlanetTileBilinear(
    s_planetTerranWetLandTile,
    kPlanetTerranWetLandTileWidth,
    kPlanetTerranWetLandTileHeight,
    landPeriodX,
    landPeriodY,
    baseX - landLayer.lightOriginX * fbm1,
    baseY - landLayer.lightOriginY * fbm1,
    1.0f
  );
  float fbm3Value = samplePlanetTileBilinear(
    s_planetTerranWetLandTile,
    kPlanetTerranWetLandTileWidth,
    kPlanetTerranWetLandTileHeight,
    landPeriodX,
    landPeriodY,
    baseX - landLayer.lightOriginX * 1.5f * fbm1,
    baseY - landLayer.lightOriginY * 1.5f * fbm1,
    1.0f
  );
  float fbm4Value = samplePlanetTileBilinear(
    s_planetTerranWetLandTile,
    kPlanetTerranWetLandTileWidth,
    kPlanetTerranWetLandTileHeight,
    landPeriodX,
    landPeriodY,
    baseX - landLayer.lightOriginX * 2.0f * fbm1,
    baseY - landLayer.lightOriginY * 2.0f * fbm1,
    1.0f
  );
  float riverNoise = samplePlanetTileBilinear(
    s_planetTerranWetLandTile,
    kPlanetTerranWetLandTileWidth,
    kPlanetTerranWetLandTileHeight,
    landPeriodX,
    landPeriodY,
    baseX + fbm1 * 6.0f,
    baseY + fbm1 * 6.0f,
    1.0f
  );
  float riverValue = planetStep(landLayer.riverCutoff, riverNoise);

  float ditherBorder = 1.0f / sample.pixels * landLayer.ditherSize;
  if (landDistance < landLayer.lightBorder1) {
    fbm4Value *= 0.9f;
  }
  if (landDistance > landLayer.lightBorder1) {
    fbm2Value *= 1.05f;
    fbm3Value *= 1.05f;
    fbm4Value *= 1.05f;
  }
  if (landDistance > landLayer.lightBorder2) {
    fbm2Value *= 1.3f;
    fbm3Value *= 1.4f;
    fbm4Value *= 1.8f;
    if (landDistance < landLayer.lightBorder2 + ditherBorder && (dith || !frame.dither)) {
      fbm4Value *= 0.5f;
    }
  }

  float lightValue = squarePlanetValue(landDistance) * 0.4f;
  PlanetBufferColor landColor = landLayer.colors[3];
  if (fbm4Value + lightValue < fbm1 * 1.5f) {
    landColor = landLayer.colors[2];
  }
  if (fbm3Value + lightValue < fbm1) {
    landColor = landLayer.colors[1];
  }
  if (fbm2Value + lightValue < fbm1) {
    landColor = landLayer.colors[0];
  }
  if (riverValue < fbm1 * 0.5f) {
    landColor = landLayer.colors[5];
    if (fbm4Value + lightValue < fbm1 * 1.5f) {
      landColor = landLayer.colors[4];
    }
  }

  outColor = landColor;
  return true;
}

bool sampleTerranWetDirect(
  const PlanetRenderFrame& frame,
  const PlanetLandRiverLayer& landLayer,
  const PlanetCloudLayer& cloudLayer,
  bool cachedTilesReady,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  if (cachedTilesReady) {
    return sampleTerranWetDirectCached(frame, landLayer, cloudLayer, x, y, outColor);
  }
  return samplePlanetCloudDirectWithNoiseSamples(
           frame,
           cloudLayer,
           x,
           y,
           kPlanetTerranWetCloudNoiseSamples,
           outColor
         ) ||
         samplePlanetLandRiversDirect(frame, landLayer, x, y, outColor);
}

bool samplePlanetUnderDirect(
  const PlanetRenderFrame& frame,
  const PlanetUnderLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  bool dith = geometry.ditherPixelU;
  float dLightBase = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, false, rotatedX, rotatedY);
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float dLight = dLightBase;
  dLight += samplePlanetGenericFbmTiled(
    rotatedX * layer.size + timeOffset,
    rotatedY * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  ) * 0.3f;

  float ditherBorder = 1.0f / sample.pixels * layer.ditherSize;
  PlanetBufferColor color = layer.colors[0];
  if (dLight > layer.lightBorder1) {
    color = layer.colors[1];
    if (dLight < layer.lightBorder1 + ditherBorder && (dith || !frame.dither)) {
      color = layer.colors[0];
    }
  }
  if (dLight > layer.lightBorder2) {
    color = layer.colors[2];
    if (dLight < layer.lightBorder2 + ditherBorder && (dith || !frame.dither)) {
      color = layer.colors[1];
    }
  }

  outColor = color;
  return true;
}

bool samplePlanetLandMassDirect(
  const PlanetRenderFrame& frame,
  const PlanetLandMassLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  float dLightBase = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float sphereX = 0.0f;
  float sphereY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, true, sphereX, sphereY);
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float baseX = sphereX * layer.size + timeOffset;
  float baseY = sphereY * layer.size;
  float fbm1 = samplePlanetGenericFbmTiled(
    baseX,
    baseY,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float fbm2Value = samplePlanetGenericFbmTiled(
    baseX - layer.lightOriginX * fbm1,
    baseY - layer.lightOriginY * fbm1,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float fbm3Value = samplePlanetGenericFbmTiled(
    baseX - layer.lightOriginX * 1.5f * fbm1,
    baseY - layer.lightOriginY * 1.5f * fbm1,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float fbm4Value = samplePlanetGenericFbmTiled(
    baseX - layer.lightOriginX * 2.0f * fbm1,
    baseY - layer.lightOriginY * 2.0f * fbm1,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );

  if (dLightBase < layer.lightBorder1) {
    fbm4Value *= 0.9f;
  }
  if (dLightBase > layer.lightBorder1) {
    fbm2Value *= 1.05f;
    fbm3Value *= 1.05f;
    fbm4Value *= 1.05f;
  }
  if (dLightBase > layer.lightBorder2) {
    fbm2Value *= 1.3f;
    fbm3Value *= 1.4f;
    fbm4Value *= 1.8f;
  }

  float lightValue = squarePlanetValue(dLightBase) * 0.1f;
  PlanetBufferColor color = layer.colors[3];
  if (fbm4Value + lightValue < fbm1) {
    color = layer.colors[2];
  }
  if (fbm3Value + lightValue < fbm1) {
    color = layer.colors[1];
  }
  if (fbm2Value + lightValue < fbm1) {
    color = layer.colors[0];
  }

  float alpha = planetStep(layer.landCutoff, fbm1);
  if (!planetDirectAlphaHit(frame, x, y, alpha * color.a)) {
    return false;
  }

  outColor = color;
  return true;
}

bool samplePlanetCloudDirect(
  const PlanetRenderFrame& frame,
  const PlanetCloudLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  return samplePlanetCloudDirectWithNoiseSamples(frame, layer, x, y, 9, outColor);
}

bool samplePlanetCloudDirectWithNoiseSamples(
  const PlanetRenderFrame& frame,
  const PlanetCloudLayer& layer,
  int x,
  int y,
  int noiseSamples,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  float dLight = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float sphereX = 0.0f;
  float sphereY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, true, sphereX, sphereY);
  float sphereCloudY = sphereY + smoothstepFloat(0.0f, layer.cloudCurve, fabsf(sphereX - 0.4f));
  sphereCloudY *= layer.stretch;

  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float scaledSphereX = sphereX * layer.size;
  float scaledSphereCloudY = sphereCloudY * layer.size;
  float cloudNoise = accumulatePlanetDirectCloudNoise(
    scaledSphereX,
    scaledSphereCloudY,
    timeOffset,
    frame.shaderSeed,
    layer.size,
    noiseSamples
  );

  float cloudValue = samplePlanetGenericFbmTiled(
    scaledSphereX + cloudNoise + timeOffset,
    scaledSphereCloudY,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    1.0f,
    1.0f
  );

  PlanetBufferColor color = layer.colors[0];
  if (cloudValue < layer.cloudCover + 0.03f) {
    color = layer.colors[1];
  }
  if (dLight + cloudValue * 0.2f > layer.lightBorder1) {
    color = layer.colors[2];
  }
  if (dLight + cloudValue * 0.2f > layer.lightBorder2) {
    color = layer.colors[3];
  }

  float alpha = planetStep(layer.cloudCover, cloudValue);
  if (!planetDirectAlphaHit(frame, x, y, alpha * color.a)) {
    return false;
  }

  outColor = color;
  return true;
}

bool samplePlanetGasCloudDirect(
  const PlanetRenderFrame& frame,
  const PlanetCloudLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  float dLight = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float sphereX = 0.0f;
  float sphereY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, true, sphereX, sphereY);
  float sphereCloudY = sphereY + smoothstepFloat(0.0f, layer.cloudCurve, fabsf(sphereX - 0.4f));
  sphereCloudY *= layer.stretch;

  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float scaledSphereX = sphereX * layer.size;
  float scaledSphereCloudY = sphereCloudY * layer.size;
  float cloudNoise = accumulatePlanetDirectCloudNoise(
    scaledSphereX,
    scaledSphereCloudY,
    timeOffset,
    frame.shaderSeed,
    layer.size,
    9
  );

  float cloudValue = samplePlanetGenericFbmTiled(
    scaledSphereX + cloudNoise + timeOffset,
    scaledSphereCloudY,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    1.0f,
    1.0f
  );

  PlanetBufferColor color = layer.colors[0];
  if (cloudValue < layer.cloudCover + 0.03f) {
    color = layer.colors[1];
  }
  if (dLight + cloudValue * 0.2f > layer.lightBorder1) {
    color = layer.colors[2];
  }
  if (dLight + cloudValue * 0.2f > layer.lightBorder2) {
    color = layer.colors[3];
  }

  float alpha = planetStep(layer.cloudCover, cloudValue);
  if (!planetDirectAlphaHit(frame, x, y, alpha * color.a)) {
    return false;
  }

  outColor = color;
  return true;
}

bool samplePlanetIceLakeDirect(
  const PlanetRenderFrame& frame,
  const PlanetIceLakeLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  float dLight = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  resolvePlanetDirectRotatedPixelUv(sample, layer.rotationOffset, rotatedX, rotatedY);
  if (!planetInsideUnitCircle(rotatedX, rotatedY)) {
    return false;
  }

  float sphereX = 0.0f;
  float sphereY = 0.0f;
  spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float lake = samplePlanetGenericFbmTiled(
    sphereX * layer.size + timeOffset,
    sphereY * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );

  float adjustedLight = squarePlanetValue(dLight) * 0.4f;
  adjustedLight -= adjustedLight * lake;

  PlanetBufferColor color = layer.colors[0];
  if (adjustedLight > layer.lightBorder1) {
    color = layer.colors[1];
  }
  if (adjustedLight > layer.lightBorder2) {
    color = layer.colors[2];
  }

  float alpha = planetStep(layer.lakeCutoff, lake);
  if (!planetDirectAlphaHit(frame, x, y, alpha * color.a)) {
    return false;
  }

  outColor = color;
  return true;
}

bool samplePlanetNoAtmosphereBaseDirect(
  const PlanetRenderFrame& frame,
  const PlanetUnderLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  if (!planetDirectInsideCircle(sample.pixelU, sample.pixelV)) {
    return false;
  }

  float dLightBase = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  bool dith = planetDirectDitherPixelU(sample);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  resolvePlanetDirectRotatedPixelUv(sample, layer.rotationOffset, rotatedX, rotatedY);
  float fbm1 = samplePlanetGenericFbmTiled(
    rotatedX,
    rotatedY,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    1.0f,
    1.0f
  );
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float dLight = dLightBase;
  dLight += samplePlanetGenericFbmTiled(
    rotatedX * layer.size + fbm1 + timeOffset,
    rotatedY * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    1.0f,
    1.0f
  ) * 0.3f;

  float ditherBorder = 1.0f / sample.pixels * layer.ditherSize;
  PlanetBufferColor color = layer.colors[0];
  if (dLight > layer.lightBorder1) {
    color = layer.colors[1];
    if (dLight < layer.lightBorder1 + ditherBorder && (dith || !frame.dither)) {
      color = layer.colors[0];
    }
  }
  if (dLight > layer.lightBorder2) {
    color = layer.colors[2];
    if (dLight < layer.lightBorder2 + ditherBorder && (dith || !frame.dither)) {
      color = layer.colors[1];
    }
  }

  outColor = color;
  return true;
}

bool samplePlanetCraterDirect(
  const PlanetRenderFrame& frame,
  const PlanetCraterLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  float dLight = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float sphereX = 0.0f;
  float sphereY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, true, sphereX, sphereY);
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  bool cachedNoiseReady =
    buildPlanetTiledRandCache(s_planetDirectNoiseRandCache, frame.shaderSeed, layer.size, 1.0f, 1.0f);

  float craterOne = 1.0f;
  float craterTwo = 1.0f;
  for (int index = 0; index < 2; index += 1) {
    float craterX = sphereX * layer.size + (float)index + 11.0f + timeOffset;
    float craterY = sphereY * layer.size;
    float shiftedCraterX =
      (sphereX + (layer.lightOriginX - 0.5f) * layer.offset) * layer.size +
      (float)index +
      11.0f +
      timeOffset;
    float shiftedCraterY =
      (sphereY + (layer.lightOriginY - 0.5f) * layer.offset) * layer.size;
    if (cachedNoiseReady) {
      craterOne *= circleNoisePlanetCraterCached(
        craterX,
        craterY,
        s_planetDirectNoiseRandCache
      );
      craterTwo *= circleNoisePlanetCraterCached(
        shiftedCraterX,
        shiftedCraterY,
        s_planetDirectNoiseRandCache
      );
    } else {
      craterOne *= circleNoisePlanetCrater(
        craterX,
        craterY,
        frame.shaderSeed,
        layer.size
      );
      craterTwo *= circleNoisePlanetCrater(
        shiftedCraterX,
        shiftedCraterY,
        frame.shaderSeed,
        layer.size
      );
    }
  }
  craterOne = 1.0f - craterOne;
  craterTwo = 1.0f - craterTwo;
  if (planetStep(0.5f, craterOne) == 0.0f) {
    return false;
  }

  PlanetBufferColor color = layer.colors[0];
  if (craterTwo < craterOne - (0.5f - dLight) * 2.0f) {
    color = layer.colors[1];
  }
  if (dLight > layer.lightBorder) {
    color = layer.colors[1];
  }

  outColor = color;
  return true;
}

bool samplePlanetLavaRiverDirect(
  const PlanetRenderFrame& frame,
  const PlanetLavaRiverLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  float dLight = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float sphereX = 0.0f;
  float sphereY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, true, sphereX, sphereY);
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float fbm1 = samplePlanetGenericFbmTiled(
    sphereX * layer.size + timeOffset,
    sphereY * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float river = planetStep(
    layer.riverCutoff,
    samplePlanetGenericFbmTiled(
      sphereX + fbm1 * 2.5f,
      sphereY + fbm1 * 2.5f,
      frame.shaderSeed,
      layer.size,
      layer.octaves,
      2.0f,
      1.0f
    )
  );
  if (river == 0.0f) {
    return false;
  }

  PlanetBufferColor color = layer.colors[0];
  if (dLight > layer.lightBorder1) {
    color = layer.colors[1];
  }
  if (dLight > layer.lightBorder2) {
    color = layer.colors[2];
  }

  outColor = color;
  return true;
}

bool samplePlanetDryTerranDirect(
  const PlanetRenderFrame& frame,
  const PlanetDryTerranLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  bool dith = geometry.ditherPixelU;
  float sphereX = geometry.sphereX;
  float sphereY = geometry.sphereY;
  float dLightBase = smoothstepFloat(
    -0.3f,
    1.2f,
    distancePlanet(sphereX, sphereY, layer.lightOriginX, layer.lightOriginY)
  );
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, false, rotatedX, rotatedY);
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float noiseValue = samplePlanetGenericFbmTiled(
    rotatedX * layer.size + timeOffset,
    rotatedY * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );

  float dLight = dLightBase;
  if (dLight < layer.lightDistance1) {
    dLight *= 0.9f;
  }
  if (dLight < layer.lightDistance2) {
    dLight *= 0.9f;
  }

  float colorValue = dLight * powf(noiseValue, 0.8f) * 3.5f;
  if (dith || !frame.dither) {
    colorValue += 0.02f;
    colorValue *= 1.05f;
  }

  float posterize = floorf(colorValue * 4.0f) / 4.0f;
  if (posterize > 1.0f) {
    posterize = 1.0f;
  }
  int colorIndex = clampInt(
    (int)roundf(posterize * (float)(layer.colorCount - 1)),
    0,
    layer.colorCount - 1
  );
  outColor = layer.colors[colorIndex];
  return true;
}

bool samplePlanetLandRiversDirect(
  const PlanetRenderFrame& frame,
  const PlanetLandRiverLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  bool dith = geometry.ditherPixelU;
  float sphereX = geometry.sphereX;
  float sphereY = geometry.sphereY;
  float dLight = distancePlanet(sphereX, sphereY, layer.lightOriginX, layer.lightOriginY);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, false, rotatedX, rotatedY);
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float baseX = rotatedX * layer.size + timeOffset;
  float baseY = rotatedY * layer.size;
  float fbm1 = samplePlanetGenericFbmTiled(
    baseX,
    baseY,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float fbm2Value = samplePlanetGenericFbmTiled(
    baseX - layer.lightOriginX * fbm1,
    baseY - layer.lightOriginY * fbm1,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float fbm3Value = samplePlanetGenericFbmTiled(
    baseX - layer.lightOriginX * 1.5f * fbm1,
    baseY - layer.lightOriginY * 1.5f * fbm1,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float fbm4Value = samplePlanetGenericFbmTiled(
    baseX - layer.lightOriginX * 2.0f * fbm1,
    baseY - layer.lightOriginY * 2.0f * fbm1,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float riverValue = planetStep(
    layer.riverCutoff,
    samplePlanetGenericFbmTiled(
      baseX + fbm1 * 6.0f,
      baseY + fbm1 * 6.0f,
      frame.shaderSeed,
      layer.size,
      layer.octaves,
      2.0f,
      1.0f
    )
  );

  float ditherBorder = 1.0f / sample.pixels * layer.ditherSize;
  if (dLight < layer.lightBorder1) {
    fbm4Value *= 0.9f;
  }
  if (dLight > layer.lightBorder1) {
    fbm2Value *= 1.05f;
    fbm3Value *= 1.05f;
    fbm4Value *= 1.05f;
  }
  if (dLight > layer.lightBorder2) {
    fbm2Value *= 1.3f;
    fbm3Value *= 1.4f;
    fbm4Value *= 1.8f;
    if (dLight < layer.lightBorder2 + ditherBorder && (dith || !frame.dither)) {
      fbm4Value *= 0.5f;
    }
  }

  float lightValue = squarePlanetValue(dLight) * 0.4f;
  PlanetBufferColor color = layer.colors[3];
  if (fbm4Value + lightValue < fbm1 * 1.5f) {
    color = layer.colors[2];
  }
  if (fbm3Value + lightValue < fbm1) {
    color = layer.colors[1];
  }
  if (fbm2Value + lightValue < fbm1) {
    color = layer.colors[0];
  }
  if (riverValue < fbm1 * 0.5f) {
    color = layer.colors[5];
    if (fbm4Value + lightValue < fbm1 * 1.5f) {
      color = layer.colors[4];
    }
  }

  outColor = color;
  return true;
}

bool samplePlanetDenseGasDirect(
  const PlanetRenderFrame& frame,
  const PlanetDenseGasLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectSphereGeometry geometry;
  if (!preparePlanetDirectSphereGeometry(frame, layer.planeScale, layer.pixelsScale, x, y, geometry)) {
    return false;
  }

  const PlanetDirectLayerSample& sample = geometry.sample;
  float lightDistance = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float sphereX = 0.0f;
  float sphereY = 0.0f;
  bool dith = geometry.ditherPixelU;
  resolvePlanetDirectSphereUv(geometry, layer.rotationOffset, true, sphereX, sphereY);
  float timeOffset = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed) *
                     layer.timeSpeed;
  float scaledSphereX = sphereX * layer.size;
  float scaledSphereY = sphereY * layer.size;
  float band = samplePlanetGenericFbmTiled(
    0.0f,
    scaledSphereY * layer.bands,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );

  float turbulence = accumulatePlanetDirectCloudNoise(
    scaledSphereX,
    scaledSphereY,
    timeOffset,
    frame.shaderSeed,
    layer.size,
    10
  );

  float fbm1 = samplePlanetGenericFbmTiled(
    scaledSphereX,
    scaledSphereY,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  float fbm2Value = samplePlanetGenericFbmTiled(
    scaledSphereX + fbm1 - timeOffset + turbulence,
    scaledSphereY * 2.0f,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );
  fbm2Value *= squarePlanetValue(band) * 7.0f;
  float lightValue = fbm2Value + lightDistance * 1.8f;
  fbm2Value += lightDistance - 0.3f;
  fbm2Value = smoothstepFloat(-0.2f, 4.0f - fbm2Value, lightValue);
  if (dith && frame.dither) {
    fbm2Value *= 1.1f;
  }

  float posterized = floorf(fbm2Value * 4.0f) / 2.0f;
  if (fbm2Value < 0.625f) {
    int index = clampInt((int)roundf(posterized * 2.0f), 0, 2);
    outColor = layer.colors[index];
    return true;
  }

  int index = clampInt((int)roundf((posterized - 1.0f) * 2.0f), 0, 2);
  outColor = layer.darkColors[index];
  return true;
}

bool samplePlanetRingDirect(
  const PlanetRenderFrame& frame,
  const PlanetRingLayer& layer,
  int x,
  int y,
  bool frontHalf,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  float lightDistance = distancePlanet(sample.pixelU, sample.pixelV, layer.lightOriginX, layer.lightOriginY);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  resolvePlanetDirectRotatedPixelUv(sample, layer.rotationOffset, rotatedX, rotatedY);
  bool isFront = rotatedY >= 0.5f;
  if (isFront != frontHalf) {
    return false;
  }

  float centerX = rotatedX;
  float centerY = rotatedY - 0.5f;
  centerY *= layer.ringPerspective;
  float centerDistance = distancePlanet(centerX, centerY, 0.5f, 0.0f);
  float ringValue = smoothstepFloat(0.5f - layer.ringWidth * 2.0f, 0.5f - layer.ringWidth, centerDistance);
  ringValue *= smoothstepFloat(centerDistance - layer.ringWidth, centerDistance, 0.4f);

  if (!frontHalf) {
    ringValue *= planetStep(
      1.0f / layer.scaleRelToPlanet,
      distancePlanet(rotatedX, rotatedY, 0.5f, 0.5f)
    );
  }

  float time = loopPlanetDirectTimeFromGalaxy(frame, layer.timeSpeed);
  float materialU = 0.0f;
  float materialV = 0.0f;
  rotatePlanetUv(centerX, centerY + 0.5f, time * layer.timeSpeed, materialU, materialV);
  ringValue *= samplePlanetGenericFbmTiled(
    materialU * layer.size,
    materialV * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    2.0f,
    1.0f
  );

  float posterized = fminf(
    2.0f,
    floorf((ringValue + squarePlanetValue(lightDistance) * 2.0f) * 4.0f) / 4.0f
  );
  PlanetBufferColor color;
  if (posterized <= 1.0f) {
    int index = clampInt((int)roundf(posterized * 2.0f), 0, 2);
    color = layer.colors[index];
  } else {
    int index = clampInt((int)roundf((posterized - 1.0f) * 2.0f), 0, 2);
    color = layer.darkColors[index];
  }

  float alpha = planetStep(0.28f, ringValue);
  if (!planetDirectAlphaHit(frame, x, y, alpha * color.a)) {
    return false;
  }

  outColor = color;
  return true;
}

bool samplePlanetAsteroidDirect(
  const PlanetRenderFrame& frame,
  const PlanetAsteroidLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  bool dith = planetDirectDitherPixelU(sample);
  float distanceFromCenter = distancePlanet(sample.pixelU, sample.pixelV, 0.5f, 0.5f);
  float rotation = frame.spinAngle + layer.rotationOffset;
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  rotatePlanetUv(sample.pixelU, sample.pixelV, rotation, rotatedX, rotatedY);
  float rotatedLightX = 0.0f;
  float rotatedLightY = 0.0f;
  rotatePlanetUv(layer.lightOriginX, layer.lightOriginY, rotation, rotatedLightX, rotatedLightY);

  bool cachedNoiseReady =
    buildPlanetTiledRandCache(s_planetDirectNoiseRandCache, frame.shaderSeed, layer.size, 1.0f, 1.0f);
  float untiledMin = -0.75f * layer.size;
  float untiledMax = 1.4f * layer.size;
  float n = samplePlanetUntiledFbmRectCached(
    rotatedX * layer.size,
    rotatedY * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    untiledMin,
    untiledMax,
    untiledMin,
    untiledMax
  );
  float n2 = samplePlanetUntiledFbmRectCached(
    rotatedX * layer.size + (rotatedLightX - 0.5f) * 0.5f,
    rotatedY * layer.size + (rotatedLightY - 0.5f) * 0.5f,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    untiledMin,
    untiledMax,
    untiledMin,
    untiledMax
  );
  float nStep = planetStep(0.2f, n - distanceFromCenter);
  if (nStep == 0.0f) {
    return false;
  }
  float n2Step = planetStep(0.2f, n2 - distanceFromCenter);
  float noiseRelation = n2Step + n2 - (nStep + n);

  float craterOne = 1.0f;
  float craterTwo = 1.0f;
  for (int index = 0; index < 2; index += 1) {
    float craterX = rotatedX * layer.size + (float)index + 11.0f;
    float craterY = rotatedY * layer.size;
    float shiftedCraterX =
      (rotatedX + (layer.lightOriginX - 0.5f) * 0.03f) * layer.size +
      (float)index +
      11.0f;
    float shiftedCraterY =
      (rotatedY + (layer.lightOriginY - 0.5f) * 0.03f) * layer.size;
    if (cachedNoiseReady) {
      craterOne *= circleNoisePlanetCraterCached(
        craterX,
        craterY,
        s_planetDirectNoiseRandCache
      );
      craterTwo *= circleNoisePlanetCraterCached(
        shiftedCraterX,
        shiftedCraterY,
        s_planetDirectNoiseRandCache
      );
    } else {
      craterOne *= circleNoisePlanetCrater(
        craterX,
        craterY,
        frame.shaderSeed,
        layer.size
      );
      craterTwo *= circleNoisePlanetCrater(
        shiftedCraterX,
        shiftedCraterY,
        frame.shaderSeed,
        layer.size
      );
    }
  }
  craterOne = 1.0f - craterOne;
  craterTwo = 1.0f - craterTwo;

  PlanetBufferColor color = layer.colors[1];
  if (noiseRelation < -0.06f || (noiseRelation < -0.04f && (dith || !frame.dither))) {
    color = layer.colors[0];
  }
  if (noiseRelation > 0.05f || (noiseRelation > 0.03f && (dith || !frame.dither))) {
    color = layer.colors[2];
  }
  if (craterOne > 0.4f) {
    color = layer.colors[1];
  }
  if (craterTwo < craterOne) {
    color = layer.colors[2];
  }

  outColor = color;
  return true;
}

bool samplePlanetStarCoreDirect(
  const PlanetRenderFrame& frame,
  const PlanetStarCoreLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  float alphaCircle = planetStep(distancePlanet(sample.pixelU, sample.pixelV, 0.5f, 0.5f), 0.49999f);
  if (alphaCircle == 0.0f) {
    return false;
  }

  bool dith = planetDirectDitherPixelV(sample);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  rotatePlanetUv(sample.pixelU, sample.pixelV, layer.rotationOffset, rotatedX, rotatedY);
  float sphereX = 0.0f;
  float sphereY = 0.0f;
  spherifyPlanetUv(rotatedX, rotatedY, sphereX, sphereY);
  float time = loopPlanetDirectTimeFromProgress(frame, layer.timeSpeed);
  float cellValue = samplePlanetStarCellsCached(
    sphereX - time * layer.timeSpeed * 2.0f,
    sphereY,
    10,
    layer.tiles
  );
  cellValue *= samplePlanetStarCellsCached(
    sphereX - time * layer.timeSpeed,
    sphereY,
    20,
    layer.tiles
  );
  cellValue *= 2.0f;
  cellValue = clampFloat(cellValue, 0.0f, 1.0f);
  if (dith || !frame.dither) {
    cellValue *= 1.3f;
  }

  float interpolate = floorf(cellValue * (float)(layer.colorCount - 1)) / (float)(layer.colorCount - 1);
  int colorIndex = clampInt(
    (int)roundf(interpolate * (float)(layer.colorCount - 1)),
    0,
    layer.colorCount - 1
  );
  outColor = layer.colors[colorIndex];
  return true;
}

bool samplePlanetStarBlobDirect(
  const PlanetRenderFrame& frame,
  const PlanetStarBlobLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  rotatePlanetUv(sample.pixelU, sample.pixelV, layer.rotationOffset, rotatedX, rotatedY);
  float angle = atan2f(rotatedX - 0.5f, rotatedY - 0.5f);
  float distanceFromCenter = distancePlanet(sample.pixelU, sample.pixelV, 0.5f, 0.5f);
  float time = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed);
  float circleUvX = distanceFromCenter * layer.size - time * layer.timeSpeed -
                    0.1f / fmaxf(distanceFromCenter, 0.0001f);
  float circleUvY = angle * layer.size;
  float value = 0.0f;
  if (!samplePlanetStarBlobPatternCached(frame, layer, circleUvX, circleUvY, value)) {
    value = computePlanetStarBlobPattern(frame, layer, circleUvX, circleUvY);
  }

  value *= 0.37f - distanceFromCenter;
  float alpha = planetStep(0.07f, value - distanceFromCenter);
  if (!planetDirectAlphaHit(frame, x, y, alpha * layer.color.a)) {
    return false;
  }

  outColor = layer.color;
  return true;
}

bool samplePlanetStarFlareDirect(
  const PlanetRenderFrame& frame,
  const PlanetStarFlareLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  bool dith = planetDirectDitherPixelV(sample);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  rotatePlanetUv(sample.pixelU, sample.pixelV, layer.rotationOffset, rotatedX, rotatedY);
  float angle = atan2f(rotatedX - 0.5f, rotatedY - 0.5f) * 0.4f;
  float distanceFromCenter = distancePlanet(sample.pixelU, sample.pixelV, 0.5f, 0.5f);
  float circleUvX = distanceFromCenter;
  float circleUvY = angle;
  float time = loopPlanetDirectTimeFromMultiplier(frame, layer.size, layer.timeSpeed);

  float n = samplePlanetGenericFbmTiled(
    circleUvX * layer.size - time * layer.timeSpeed,
    circleUvY * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    1.0f,
    1.0f
  );
  float invert = 1.0f / layer.circleAmount;
  float adjustedX = circleUvX * layer.scale - time * layer.timeSpeed + n;
  float adjustedY = circleUvY * layer.scale;
  if (modPlanetFloat(adjustedY, invert * 2.0f) < invert) {
    adjustedX += invert * 0.5f;
  }
  float randCoordX = floorf(adjustedX * layer.circleAmount) / layer.circleAmount;
  float randCoordY = floorf(adjustedY * layer.circleAmount) / layer.circleAmount;
  float localX = modPlanetFloat(adjustedX, invert) * layer.circleAmount;
  float localY = modPlanetFloat(adjustedY, invert) * layer.circleAmount;
  float radius = tiledPlanetRand(frame.shaderSeed, layer.size, randCoordX, randCoordY);
  radius = clampFloat(radius, invert, 1.0f - invert);
  float circleDistance = distancePlanet(localX, localY, radius, radius);
  float nc = smoothstepFloat(
    circleDistance,
    circleDistance + 0.5f,
    invert * layer.circleScale *
      tiledPlanetRand(frame.shaderSeed, layer.size, randCoordX * 1.5f, randCoordY * 1.5f)
  );
  nc *= 1.5f;
  float n2 = samplePlanetGenericFbmTiled(
    circleUvX * layer.size - time + 100.0f,
    circleUvY * layer.size + 100.0f,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    1.0f,
    1.0f
  );
  nc -= n2 * 0.1f;

  float alpha = 0.0f;
  if (1.0f - distanceFromCenter > nc) {
    if (nc > layer.stormWidth - layer.stormDitherWidth + distanceFromCenter &&
        (dith || !frame.dither)) {
      alpha = 1.0f;
    } else if (nc > layer.stormWidth + distanceFromCenter) {
      alpha = 1.0f;
    }
  }

  if (alpha == 0.0f) {
    return false;
  }

  int colorIndex = clampInt((int)floorf(n2 + nc), 0, layer.colorCount - 1);
  PlanetBufferColor color = layer.colors[colorIndex];
  alpha *= planetStep(n2 * 0.25f, distanceFromCenter);
  if (!planetDirectAlphaHit(frame, x, y, alpha * color.a)) {
    return false;
  }

  outColor = color;
  return true;
}

bool samplePlanetBlackHoleCoreDirect(
  const PlanetRenderFrame& frame,
  const PlanetBlackHoleCoreLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  float distanceFromCenter = distancePlanet(sample.pixelU, sample.pixelV, 0.5f, 0.5f);
  PlanetBufferColor color = layer.colors[0];
  if (distanceFromCenter > layer.radius - layer.lightWidth) {
    color = layer.colors[1];
  }
  if (distanceFromCenter > layer.radius - layer.lightWidth * 0.5f) {
    color = layer.colors[2];
  }

  float alpha = planetStep(distanceFromCenter, layer.radius);
  if (!planetDirectAlphaHit(frame, x, y, alpha * color.a)) {
    return false;
  }

  outColor = color;
  return true;
}

bool samplePlanetBlackHoleDiskDirect(
  const PlanetRenderFrame& frame,
  const PlanetBlackHoleDiskLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  bool dith = planetDirectDitherPixelV(sample);
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  rotatePlanetUv(sample.pixelU, sample.pixelV, layer.rotationOffset, rotatedX, rotatedY);
  float preservedX = rotatedX;
  float preservedY = rotatedY;

  float wobbleTime = loopPlanetDirectTimeFromRing(frame, layer.timeSpeed, frame.motionFactor);
  float flowTime = loopPlanetDirectTimeFromRing(frame, layer.timeSpeed, frame.spinFactor);
  float compressedX = (rotatedX - 0.5f) * 1.3f + 0.5f;
  float compressedU = 0.0f;
  float compressedV = 0.0f;
  rotatePlanetUv(
    compressedX,
    rotatedY,
    sinf(wobbleTime * layer.timeSpeed * 2.0f) * 0.01f,
    compressedU,
    compressedV
  );

  float lightOriginX = 0.5f;
  float lightOriginY = 0.5f;
  float diskWidth = layer.diskWidth;
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
    preservedX,
    preservedY * layer.ringPerspective,
    lightOriginX,
    lightOriginY * layer.ringPerspective
  ) * 0.3f;
  float centerX = compressedU;
  float centerY = (compressedV - 0.5f) * layer.ringPerspective;
  float centerDistance = distancePlanet(centerX, centerY, 0.5f, 0.0f);
  float disk = smoothstepFloat(0.1f - diskWidth * 2.0f, 0.5f - diskWidth, centerDistance);
  disk *= smoothstepFloat(centerDistance - diskWidth, centerDistance, 0.4f);

  float materialU = 0.0f;
  float materialV = 0.0f;
  rotatePlanetUv(centerX, centerY + 0.5f, flowTime * layer.timeSpeed * 3.0f, materialU, materialV);
  disk *= sqrtf(clampFloat(
    samplePlanetGenericFbmTiled(
      materialU * layer.size,
      materialV * layer.size,
      frame.shaderSeed,
      layer.size,
      layer.octaves,
      2.0f,
      1.0f
    ),
    0.0f,
    1.0f
  ));

  if (dith || !frame.dither) {
    disk *= 1.2f;
  }

  int posterized = clampInt(
    (int)floorf((disk + lightDistance) * (float)(layer.colorCount - 1)),
    0,
    layer.colorCount - 1
  );
  PlanetBufferColor color = layer.colors[posterized];
  float alpha = planetStep(0.15f, disk);
  if (!planetDirectAlphaHit(frame, x, y, alpha * color.a)) {
    return false;
  }

  outColor = color;
  return true;
}

bool samplePlanetGalaxyDirect(
  const PlanetRenderFrame& frame,
  const PlanetGalaxyLayer& layer,
  int x,
  int y,
  PlanetBufferColor& outColor
) {
  PlanetDirectLayerSample sample;
  if (!preparePlanetDirectLayerSample(frame, layer.planeScale, layer.pixelsScale, x, y, sample)) {
    return false;
  }

  bool dith = planetDirectDitherPixelU(sample);
  float quantizedU = sample.pixelU * layer.zoom - (layer.zoom - 1.0f) / 2.0f;
  float quantizedV = sample.pixelV * layer.zoom - (layer.zoom - 1.0f) / 2.0f;
  float rotatedX = 0.0f;
  float rotatedY = 0.0f;
  rotatePlanetUv(quantizedU, quantizedV, layer.rotationOffset, rotatedX, rotatedY);
  float preservedX = rotatedX;
  float preservedY = rotatedY;

  float time = loopPlanetDirectTimeFromRotation(frame, layer.timeSpeed, frame.motionFactor);
  float layerU = rotatedX;
  float layerV = rotatedY * layer.tilt - (layer.tilt - 1.0f) / 2.0f;
  float distanceOne = distancePlanet(layerU, layerV, 0.5f, 0.5f);
  float swirlOne = layer.swirl * powf(distanceOne, 0.4f);
  float rotatedLayerU = 0.0f;
  float rotatedLayerV = 0.0f;
  rotatePlanetUv(layerU, layerV, swirlOne + time * layer.timeSpeed, rotatedLayerU, rotatedLayerV);
  float untiledMin = -1.75f * layer.size;
  float untiledMax = 3.8f * layer.size;
  float f1 = samplePlanetUntiledFbmRectCached(
    rotatedLayerU * layer.size,
    rotatedLayerV * layer.size,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    untiledMin,
    untiledMax,
    untiledMin,
    untiledMax
  );
  f1 = floorf(f1 * layer.layerCount) / layer.layerCount;

  float galaxyU = preservedX;
  float galaxyV = preservedY * layer.tilt - (layer.tilt - 1.0f) / 2.0f - f1 * layer.layerHeight;
  float distanceTwo = distancePlanet(galaxyU, galaxyV, 0.5f, 0.5f);
  float swirlTwo = layer.swirl * powf(distanceTwo, 0.4f);
  float rotatedGalaxyU = 0.0f;
  float rotatedGalaxyV = 0.0f;
  rotatePlanetUv(galaxyU, galaxyV, swirlTwo + time * layer.timeSpeed, rotatedGalaxyU, rotatedGalaxyV);
  float f2 = samplePlanetUntiledFbmRectCached(
    rotatedGalaxyU * layer.size + f1 * 10.0f,
    rotatedGalaxyV * layer.size + f1 * 10.0f,
    frame.shaderSeed,
    layer.size,
    layer.octaves,
    untiledMin,
    untiledMax,
    untiledMin,
    untiledMax
  );

  float alpha = planetStep(f2 + distanceTwo, 0.7f);
  if (!planetDirectAlphaHit(frame, x, y, alpha)) {
    return false;
  }

  f2 *= 2.3f;
  if (frame.dither && dith) {
    f2 *= 0.94f;
  }

  int colorIndex = clampInt((int)floorf(f2 * (float)layer.nColors), 0, layer.colorCount - 1);
  outColor = layer.colors[colorIndex];
  return true;
}

void drawPlanetBackgroundFastPreview(const PlanetRenderFrame& frame) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;

  uint32_t state = frame.seed ^ hashPlanetString("planet_starfield_sparse");
  float exclusionRadius = planetBackgroundStarExclusionRadius(frame);
  int attempts = 0;
  int placed = 0;
  while (placed < 10 && attempts < 400) {
    attempts += 1;
    int x = (int)floorf(nextPlanetSeededRandom(state) * (float)kPlanetCanvasSize);
    int y = (int)floorf(nextPlanetSeededRandom(state) * (float)kPlanetCanvasSize);
    float dx = (float)x - frame.centerX;
    float dy = (float)y - frame.centerY;
    if (sqrtf(dx * dx + dy * dy) < exclusionRadius) {
      continue;
    }

    PlanetRgb starColor = nextPlanetSeededRandom(state) > 0.45f
      ? makePlanetRgb(255, 255, 255)
      : makePlanetRgb(255, 239, 158);
    float brightness = 0.44f + nextPlanetSeededRandom(state) * 0.24f;
    display->drawPixel(x, y, planetColor565(scalePlanetRgb(starColor, brightness)));
    placed += 1;
  }
}

void drawSpherePlanetFastPreview(const PlanetRenderFrame& frame) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  int sampleStep = resolvePlanetDirectRenderStep(frame);

  if (planetPresetEqualsValue(frame.presetId, "terran_wet")) {
    const PlanetBufferColor landColors[] = {
      makePlanetBufferColor(0.388235f, 0.670588f, 0.247059f),
      makePlanetBufferColor(0.231373f, 0.490196f, 0.309804f),
      makePlanetBufferColor(0.184314f, 0.341176f, 0.32549f),
      makePlanetBufferColor(0.156863f, 0.207843f, 0.25098f),
      makePlanetBufferColor(0.309804f, 0.643137f, 0.721569f),
      makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
    };
    const PlanetBufferColor cloudColors[] = {
      makePlanetBufferColor(0.960784f, 1.0f, 0.909804f),
      makePlanetBufferColor(0.87451f, 0.878431f, 0.909804f),
      makePlanetBufferColor(0.407843f, 0.435294f, 0.6f),
      makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
    };
    PlanetLandRiverLayer landLayer = {
      1.0f, 1.0f, 0.39f, 0.39f, 0.1f, 3.951f, 0.287f, 0.476f, 0.368f, 4.6f, 6, 0.0f, landColors
    };
    PlanetCloudLayer cloudLayer = {
      1.0f,
      1.0f,
      0.39f,
      0.39f,
      getWetTerranCloudCover(frame.seed),
      0.1f,
      2.0f,
      1.3f,
      0.52f,
      0.62f,
      7.315f,
      1,
      0.0f,
      cloudColors
    };
    int minX = 0;
    int maxXExclusive = kPlanetCanvasSize;
    int minY = 0;
    int maxYExclusive = kPlanetCanvasSize;
    resolvePlanetDirectBounds(frame, cloudLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);
    bool terranWetTilesReady = prepareTerranWetTiles(frame, landLayer, cloudLayer);

    for (int y = minY; y < maxYExclusive; y += 1) {
      for (int x = minX; x < maxXExclusive; x += 1) {
        PlanetBufferColor color;
        if (sampleTerranWetDirect(frame, landLayer, cloudLayer, terranWetTilesReady, x, y, color)) {
          drawPlanetDirectBufferColor(display, x, y, color);
        }
      }
    }
    return;
  }

  if (planetPresetEqualsValue(frame.presetId, "terran_dry")) {
    const PlanetBufferColor colors[] = {
      makePlanetBufferColor(1.0f, 0.537255f, 0.2f),
      makePlanetBufferColor(0.901961f, 0.270588f, 0.223529f),
      makePlanetBufferColor(0.678431f, 0.184314f, 0.270588f),
      makePlanetBufferColor(0.321569f, 0.2f, 0.247059f),
      makePlanetBufferColor(0.239216f, 0.160784f, 0.211765f)
    };
    PlanetDryTerranLayer layer = {
      1.0f, 1.0f, 0.4f, 0.3f, 0.362f, 0.525f, 0.1f, 8.0f, 3, 0.0f, colors, 5
    };
    int minX = 0;
    int maxXExclusive = kPlanetCanvasSize;
    int minY = 0;
    int maxYExclusive = kPlanetCanvasSize;
    resolvePlanetDirectBounds(frame, layer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

    for (int y = minY; y < maxYExclusive; y += sampleStep) {
      for (int x = minX; x < maxXExclusive; x += sampleStep) {
        int sampleX = x;
        int sampleY = y;
        resolvePlanetDirectSamplePoint(
          x,
          y,
          sampleStep,
          maxXExclusive,
          maxYExclusive,
          sampleX,
          sampleY
        );
        PlanetBufferColor color;
        if (samplePlanetDryTerranDirect(frame, layer, sampleX, sampleY, color)) {
          drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
        }
      }
    }
    return;
  }

  if (planetPresetEqualsValue(frame.presetId, "islands")) {
    const PlanetBufferColor underColors[] = {
      makePlanetBufferColor(0.572549f, 0.909804f, 0.752941f),
      makePlanetBufferColor(0.309804f, 0.643137f, 0.721569f),
      makePlanetBufferColor(0.172549f, 0.207843f, 0.301961f)
    };
    const PlanetBufferColor landColors[] = {
      makePlanetBufferColor(0.784314f, 0.831373f, 0.364706f),
      makePlanetBufferColor(0.388235f, 0.670588f, 0.247059f),
      makePlanetBufferColor(0.184314f, 0.341176f, 0.32549f),
      makePlanetBufferColor(0.156863f, 0.207843f, 0.25098f)
    };
    const PlanetBufferColor cloudColors[] = {
      makePlanetBufferColor(0.87451f, 0.878431f, 0.909804f),
      makePlanetBufferColor(0.639216f, 0.654902f, 0.760784f),
      makePlanetBufferColor(0.407843f, 0.435294f, 0.6f),
      makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
    };
    PlanetUnderLayer underLayer = {
      1.0f, 1.0f, 0.39f, 0.39f, 0.1f, 2.0f, 0.4f, 0.6f, 5.228f, 3, 0.0f, underColors
    };
    PlanetLandMassLayer landLayer = {
      1.0f, 1.0f, 0.39f, 0.39f, 0.2f, 0.32f, 0.534f, 0.633f, 4.292f, 6, 0.0f, landColors
    };
    PlanetCloudLayer cloudLayer = {
      1.0f,
      1.0f,
      0.39f,
      0.39f,
      getIslandCloudCover(frame.seed),
      0.47f,
      2.0f,
      1.3f,
      0.52f,
      0.62f,
      7.745f,
      2,
      0.0f,
      cloudColors
    };
    int minX = 0;
    int maxXExclusive = kPlanetCanvasSize;
    int minY = 0;
    int maxYExclusive = kPlanetCanvasSize;
    resolvePlanetDirectBounds(frame, cloudLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

    for (int y = minY; y < maxYExclusive; y += sampleStep) {
      for (int x = minX; x < maxXExclusive; x += sampleStep) {
        int sampleX = x;
        int sampleY = y;
        resolvePlanetDirectSamplePoint(
          x,
          y,
          sampleStep,
          maxXExclusive,
          maxYExclusive,
          sampleX,
          sampleY
        );
        PlanetBufferColor color;
        if (samplePlanetCloudDirect(frame, cloudLayer, sampleX, sampleY, color) ||
            samplePlanetLandMassDirect(frame, landLayer, sampleX, sampleY, color) ||
            samplePlanetUnderDirect(frame, underLayer, sampleX, sampleY, color)) {
          drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
        }
      }
    }
    return;
  }

  if (planetPresetEqualsValue(frame.presetId, "no_atmosphere")) {
    const PlanetBufferColor baseColors[] = {
      makePlanetBufferColor(0.639216f, 0.654902f, 0.760784f),
      makePlanetBufferColor(0.298039f, 0.407843f, 0.521569f),
      makePlanetBufferColor(0.227451f, 0.247059f, 0.368627f)
    };
    const PlanetBufferColor craterColors[] = {
      makePlanetBufferColor(0.298039f, 0.407843f, 0.521569f),
      makePlanetBufferColor(0.227451f, 0.247059f, 0.368627f)
    };
    PlanetUnderLayer baseLayer = {
      1.0f, 1.0f, 0.25f, 0.25f, 0.4f, 2.0f, 0.615f, 0.729f, 8.0f, 4, 0.0f, baseColors
    };
    PlanetCraterLayer craterLayer = {
      1.0f, 1.0f, 0.25f, 0.25f, 0.001f, 0.465f, 5.0f, 0.03f, 0, 0.0f, craterColors
    };
    int minX = 0;
    int maxXExclusive = kPlanetCanvasSize;
    int minY = 0;
    int maxYExclusive = kPlanetCanvasSize;
    resolvePlanetDirectBounds(frame, baseLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

    for (int y = minY; y < maxYExclusive; y += sampleStep) {
      for (int x = minX; x < maxXExclusive; x += sampleStep) {
        int sampleX = x;
        int sampleY = y;
        resolvePlanetDirectSamplePoint(
          x,
          y,
          sampleStep,
          maxXExclusive,
          maxYExclusive,
          sampleX,
          sampleY
        );
        PlanetBufferColor color;
        if (samplePlanetCraterDirect(frame, craterLayer, sampleX, sampleY, color) ||
            samplePlanetNoAtmosphereBaseDirect(frame, baseLayer, sampleX, sampleY, color)) {
          drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
        }
      }
    }
    return;
  }

  if (planetPresetEqualsValue(frame.presetId, "gas_giant_1")) {
    const PlanetBufferColor darkColors[] = {
      makePlanetBufferColor(0.231373f, 0.12549f, 0.152941f),
      makePlanetBufferColor(0.231373f, 0.12549f, 0.152941f),
      makePlanetBufferColor(0.0f, 0.0f, 0.0f),
      makePlanetBufferColor(0.129412f, 0.0941176f, 0.105882f)
    };
    const PlanetBufferColor warmColors[] = {
      makePlanetBufferColor(0.941176f, 0.709804f, 0.254902f),
      makePlanetBufferColor(0.811765f, 0.458824f, 0.168627f),
      makePlanetBufferColor(0.670588f, 0.317647f, 0.188235f),
      makePlanetBufferColor(0.490196f, 0.219608f, 0.2f)
    };
    PlanetCloudLayer darkLayer = {
      1.0f, 1.0f, 0.25f, 0.25f, 0.0f, 0.7f, 1.0f, 1.3f, 0.692f, 0.666f, 9.0f, 5, 0.0f, darkColors
    };
    PlanetCloudLayer warmLayer = {
      1.0f,
      1.0f,
      0.25f,
      0.25f,
      getGasGiantCloudCover(frame.seed),
      0.47f,
      1.0f,
      1.3f,
      0.439f,
      0.746f,
      9.0f,
      5,
      0.0f,
      warmColors
    };
    int minX = 0;
    int maxXExclusive = kPlanetCanvasSize;
    int minY = 0;
    int maxYExclusive = kPlanetCanvasSize;
    resolvePlanetDirectBounds(frame, warmLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

    for (int y = minY; y < maxYExclusive; y += sampleStep) {
      for (int x = minX; x < maxXExclusive; x += sampleStep) {
        int sampleX = x;
        int sampleY = y;
        resolvePlanetDirectSamplePoint(
          x,
          y,
          sampleStep,
          maxXExclusive,
          maxYExclusive,
          sampleX,
          sampleY
        );
        PlanetBufferColor color;
        if (samplePlanetGasCloudDirect(frame, warmLayer, sampleX, sampleY, color) ||
            samplePlanetGasCloudDirect(frame, darkLayer, sampleX, sampleY, color)) {
          drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
        }
      }
    }
    return;
  }

  if (planetPresetEqualsValue(frame.presetId, "gas_giant_2")) {
    const PlanetBufferColor colors[] = {
      makePlanetBufferColor(0.933333f, 0.764706f, 0.603922f),
      makePlanetBufferColor(0.85098f, 0.627451f, 0.4f),
      makePlanetBufferColor(0.560784f, 0.337255f, 0.231373f)
    };
    const PlanetBufferColor darkColors[] = {
      makePlanetBufferColor(0.4f, 0.223529f, 0.192157f),
      makePlanetBufferColor(0.270588f, 0.156863f, 0.235294f),
      makePlanetBufferColor(0.133333f, 0.12549f, 0.203922f)
    };
    PlanetDenseGasLayer gasLayer = {
      1.0f, 1.0f, -0.1f, 0.3f, 0.05f, 0.892f, 10.107f, 3, 0.0f, colors, darkColors
    };
    PlanetRingLayer ringLayer = {
      3.0f, 3.0f, -0.1f, 0.3f, 0.2f, 0.127f, 6.0f, 6.0f, 15.0f, 4, 0.7f, colors, darkColors
    };
    int minX = 0;
    int maxXExclusive = kPlanetCanvasSize;
    int minY = 0;
    int maxYExclusive = kPlanetCanvasSize;
    resolvePlanetDirectBounds(frame, ringLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

    for (int y = minY; y < maxYExclusive; y += sampleStep) {
      for (int x = minX; x < maxXExclusive; x += sampleStep) {
        int sampleX = x;
        int sampleY = y;
        resolvePlanetDirectSamplePoint(
          x,
          y,
          sampleStep,
          maxXExclusive,
          maxYExclusive,
          sampleX,
          sampleY
        );
        PlanetBufferColor color;
        if (samplePlanetRingDirect(frame, ringLayer, sampleX, sampleY, true, color) ||
            samplePlanetDenseGasDirect(frame, gasLayer, sampleX, sampleY, color) ||
            samplePlanetRingDirect(frame, ringLayer, sampleX, sampleY, false, color)) {
          drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
        }
      }
    }
    return;
  }

  if (planetPresetEqualsValue(frame.presetId, "ice_world")) {
    const PlanetBufferColor underColors[] = {
      makePlanetBufferColor(0.980392f, 1.0f, 1.0f),
      makePlanetBufferColor(0.780392f, 0.831373f, 0.882353f),
      makePlanetBufferColor(0.572549f, 0.560784f, 0.721569f)
    };
    const PlanetBufferColor lakeColors[] = {
      makePlanetBufferColor(0.309804f, 0.643137f, 0.721569f),
      makePlanetBufferColor(0.298039f, 0.407843f, 0.521569f),
      makePlanetBufferColor(0.227451f, 0.247059f, 0.368627f)
    };
    const PlanetBufferColor cloudColors[] = {
      makePlanetBufferColor(0.882353f, 0.94902f, 1.0f),
      makePlanetBufferColor(0.752941f, 0.890196f, 1.0f),
      makePlanetBufferColor(0.368627f, 0.439216f, 0.647059f),
      makePlanetBufferColor(0.25098f, 0.286275f, 0.45098f)
    };
    PlanetUnderLayer underLayer = {
      1.0f, 1.0f, 0.3f, 0.3f, 0.25f, 2.0f, 0.48f, 0.632f, 8.0f, 2, 0.0f, underColors
    };
    PlanetIceLakeLayer lakeLayer = {
      1.0f, 1.0f, 0.3f, 0.3f, 0.2f, 0.024f, 0.047f, 0.55f, 10.0f, 3, 0.0f, lakeColors
    };
    PlanetCloudLayer cloudLayer = {
      1.0f, 1.0f, 0.3f, 0.3f, 0.546f, 0.1f, 2.5f, 1.3f, 0.566f, 0.781f, 4.0f, 4, 0.0f, cloudColors
    };
    int minX = 0;
    int maxXExclusive = kPlanetCanvasSize;
    int minY = 0;
    int maxYExclusive = kPlanetCanvasSize;
    resolvePlanetDirectBounds(frame, cloudLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

    for (int y = minY; y < maxYExclusive; y += sampleStep) {
      for (int x = minX; x < maxXExclusive; x += sampleStep) {
        int sampleX = x;
        int sampleY = y;
        resolvePlanetDirectSamplePoint(
          x,
          y,
          sampleStep,
          maxXExclusive,
          maxYExclusive,
          sampleX,
          sampleY
        );
        PlanetBufferColor color;
        if (samplePlanetCloudDirect(frame, cloudLayer, sampleX, sampleY, color) ||
            samplePlanetIceLakeDirect(frame, lakeLayer, sampleX, sampleY, color) ||
            samplePlanetUnderDirect(frame, underLayer, sampleX, sampleY, color)) {
          drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
        }
      }
    }
    return;
  }

  if (planetPresetEqualsValue(frame.presetId, "lava_world")) {
    const PlanetBufferColor baseColors[] = {
      makePlanetBufferColor(0.560784f, 0.301961f, 0.341176f),
      makePlanetBufferColor(0.321569f, 0.2f, 0.247059f),
      makePlanetBufferColor(0.239216f, 0.160784f, 0.211765f)
    };
    const PlanetBufferColor craterColors[] = {
      makePlanetBufferColor(0.321569f, 0.2f, 0.247059f),
      makePlanetBufferColor(0.239216f, 0.160784f, 0.211765f)
    };
    const PlanetBufferColor lavaColors[] = {
      makePlanetBufferColor(1.0f, 0.537255f, 0.2f),
      makePlanetBufferColor(0.901961f, 0.270588f, 0.223529f),
      makePlanetBufferColor(0.678431f, 0.184314f, 0.270588f)
    };
    PlanetUnderLayer baseLayer = {
      1.0f, 1.0f, 0.3f, 0.3f, 0.2f, 2.0f, 0.4f, 0.6f, 10.0f, 3, 0.0f, baseColors
    };
    PlanetCraterLayer craterLayer = {
      1.0f, 1.0f, 0.3f, 0.3f, 0.2f, 0.4f, 3.5f, 0.03f, 0, 0.0f, craterColors
    };
    PlanetLavaRiverLayer lavaLayer = {
      1.0f, 1.0f, 0.3f, 0.3f, 0.2f, 0.019f, 0.036f, 0.579f, 10.0f, 4, 0.0f, lavaColors
    };
    int minX = 0;
    int maxXExclusive = kPlanetCanvasSize;
    int minY = 0;
    int maxYExclusive = kPlanetCanvasSize;
    resolvePlanetDirectBounds(frame, lavaLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

    for (int y = minY; y < maxYExclusive; y += sampleStep) {
      for (int x = minX; x < maxXExclusive; x += sampleStep) {
        int sampleX = x;
        int sampleY = y;
        resolvePlanetDirectSamplePoint(
          x,
          y,
          sampleStep,
          maxXExclusive,
          maxYExclusive,
          sampleX,
          sampleY
        );
        PlanetBufferColor color;
        if (samplePlanetLavaRiverDirect(frame, lavaLayer, sampleX, sampleY, color) ||
            samplePlanetCraterDirect(frame, craterLayer, sampleX, sampleY, color) ||
            samplePlanetNoAtmosphereBaseDirect(frame, baseLayer, sampleX, sampleY, color)) {
          drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
        }
      }
    }
  }
}

void drawAsteroidPlanetFastPreview(const PlanetRenderFrame& frame) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  int sampleStep = resolvePlanetDirectRenderStep(frame);
  const PlanetBufferColor colors[] = {
    makePlanetBufferColor(0.639216f, 0.654902f, 0.760784f),
    makePlanetBufferColor(0.298039f, 0.407843f, 0.521569f),
    makePlanetBufferColor(0.227451f, 0.247059f, 0.368627f)
  };
  PlanetAsteroidLayer layer = {
    1.0f, 1.0f, 0.0f, 0.0f, 5.294f, 2, 0.0f, colors
  };
  int minX = 0;
  int maxXExclusive = kPlanetCanvasSize;
  int minY = 0;
  int maxYExclusive = kPlanetCanvasSize;
  resolvePlanetDirectBounds(frame, layer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

  for (int y = minY; y < maxYExclusive; y += sampleStep) {
    for (int x = minX; x < maxXExclusive; x += sampleStep) {
      int sampleX = x;
      int sampleY = y;
      resolvePlanetDirectSamplePoint(
        x,
        y,
        sampleStep,
        maxXExclusive,
        maxYExclusive,
        sampleX,
        sampleY
      );
      PlanetBufferColor color;
      if (samplePlanetAsteroidDirect(frame, layer, sampleX, sampleY, color)) {
        drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
      }
    }
  }
}

void drawBlackHolePlanetFastPreview(const PlanetRenderFrame& frame) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  int sampleStep = resolvePlanetDirectRenderStep(frame);
  const PlanetBufferColor coreColors[] = {
    makePlanetBufferColor(0.152941f, 0.152941f, 0.211765f),
    makePlanetBufferColor(1.0f, 1.0f, 0.921569f),
    makePlanetBufferColor(0.929412f, 0.482353f, 0.223529f)
  };
  const PlanetBufferColor diskColors[] = {
    makePlanetBufferColor(1.0f, 1.0f, 0.921569f),
    makePlanetBufferColor(1.0f, 0.960784f, 0.25098f),
    makePlanetBufferColor(1.0f, 0.721569f, 0.290196f),
    makePlanetBufferColor(0.929412f, 0.482353f, 0.223529f),
    makePlanetBufferColor(0.741176f, 0.25098f, 0.207843f)
  };
  PlanetBlackHoleCoreLayer coreLayer = {
    1.0f, 1.0f, 0.247f, 0.028f, 0.0f, coreColors
  };
  PlanetBlackHoleDiskLayer diskLayer = {
    3.0f, 3.0f, 0.2f, 0.065f, 14.0f, 0.766f, 6.598f, 3, diskColors, 5
  };
  int minX = 0;
  int maxXExclusive = kPlanetCanvasSize;
  int minY = 0;
  int maxYExclusive = kPlanetCanvasSize;
  resolvePlanetDirectBounds(frame, diskLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

  for (int y = minY; y < maxYExclusive; y += sampleStep) {
    for (int x = minX; x < maxXExclusive; x += sampleStep) {
      int sampleX = x;
      int sampleY = y;
      resolvePlanetDirectSamplePoint(
        x,
        y,
        sampleStep,
        maxXExclusive,
        maxYExclusive,
        sampleX,
        sampleY
      );
      PlanetBufferColor color;
      if (samplePlanetBlackHoleDiskDirect(frame, diskLayer, sampleX, sampleY, color) ||
          samplePlanetBlackHoleCoreDirect(frame, coreLayer, sampleX, sampleY, color)) {
        drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
      }
    }
  }
}

void drawGalaxyPlanetFastPreview(const PlanetRenderFrame& frame) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  int sampleStep = resolvePlanetDirectRenderStep(frame);
  const PlanetBufferColor colors[] = {
    makePlanetBufferColor(1.0f, 1.0f, 0.921569f),
    makePlanetBufferColor(1.0f, 0.913725f, 0.552941f),
    makePlanetBufferColor(0.709804f, 0.878431f, 0.4f),
    makePlanetBufferColor(0.396078f, 0.647059f, 0.4f),
    makePlanetBufferColor(0.223529f, 0.364706f, 0.392157f),
    makePlanetBufferColor(0.196078f, 0.223529f, 0.301961f),
    makePlanetBufferColor(0.196078f, 0.160784f, 0.278431f)
  };
  PlanetGalaxyLayer layer = {
    1.0f, 1.0f, 1.0f, 0.674f, 7.0f, 1, 3.0f, 4.0f, 0.4f, 1.375f, -9.0f, 6, colors, 7
  };
  int minX = 0;
  int maxXExclusive = kPlanetCanvasSize;
  int minY = 0;
  int maxYExclusive = kPlanetCanvasSize;
  resolvePlanetDirectBounds(frame, layer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

  for (int y = minY; y < maxYExclusive; y += sampleStep) {
    for (int x = minX; x < maxXExclusive; x += sampleStep) {
      int sampleX = x;
      int sampleY = y;
      resolvePlanetDirectSamplePoint(
        x,
        y,
        sampleStep,
        maxXExclusive,
        maxYExclusive,
        sampleX,
        sampleY
      );
      PlanetBufferColor color;
      if (samplePlanetGalaxyDirect(frame, layer, sampleX, sampleY, color)) {
        drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
      }
    }
  }
}

void drawStarPlanetFastPreview(const PlanetRenderFrame& frame) {
  MatrixPanel_I2S_DMA* display = DisplayManager::dma_display;
  int sampleStep = resolvePlanetDirectRenderStep(frame);
  PlanetBufferColor starColors[4];
  PlanetBufferColor flareColors[2];
  bool evenSeed = (frame.colorSeed % 2U) == 0U;
  if (evenSeed) {
    starColors[0] = makePlanetBufferColor(0.960784f, 1.0f, 0.909804f);
    starColors[1] = makePlanetBufferColor(1.0f, 0.847059f, 0.196078f);
    starColors[2] = makePlanetBufferColor(1.0f, 0.509804f, 0.231373f);
    starColors[3] = makePlanetBufferColor(0.486275f, 0.0980392f, 0.101961f);
    flareColors[0] = makePlanetBufferColor(1.0f, 0.847059f, 0.196078f);
    flareColors[1] = makePlanetBufferColor(0.960784f, 1.0f, 0.909804f);
  } else {
    starColors[0] = makePlanetBufferColor(0.960784f, 1.0f, 0.909804f);
    starColors[1] = makePlanetBufferColor(0.466667f, 0.839216f, 0.756863f);
    starColors[2] = makePlanetBufferColor(0.109804f, 0.572549f, 0.654902f);
    starColors[3] = makePlanetBufferColor(0.0117647f, 0.243137f, 0.368627f);
    flareColors[0] = makePlanetBufferColor(0.466667f, 0.839216f, 0.756863f);
    flareColors[1] = makePlanetBufferColor(0.960784f, 1.0f, 0.909804f);
  }

  PlanetStarBlobLayer blobLayer = {
    2.0f, 2.0f, 0.05f, 4.93f, 2.0f, 1.0f, 0.0f, makePlanetBufferColor(1.0f, 1.0f, 0.894118f)
  };
  PlanetStarCoreLayer coreLayer = {
    1.0f, 1.0f, 0.05f, 1, 0.0f, starColors, 4
  };
  PlanetStarFlareLayer flareLayer = {
    2.0f, 2.0f, 0.05f, 0.3f, 0.0f, 2.0f, 1.0f, 1.0f, 1.6f, 4, 0.0f, flareColors, 2
  };
  int minX = 0;
  int maxXExclusive = kPlanetCanvasSize;
  int minY = 0;
  int maxYExclusive = kPlanetCanvasSize;
  resolvePlanetDirectBounds(frame, flareLayer.planeScale, minX, maxXExclusive, minY, maxYExclusive);

  for (int y = minY; y < maxYExclusive; y += sampleStep) {
    for (int x = minX; x < maxXExclusive; x += sampleStep) {
      int sampleX = x;
      int sampleY = y;
      resolvePlanetDirectSamplePoint(
        x,
        y,
        sampleStep,
        maxXExclusive,
        maxYExclusive,
        sampleX,
        sampleY
      );
      PlanetBufferColor color;
      if (samplePlanetStarFlareDirect(frame, flareLayer, sampleX, sampleY, color) ||
          samplePlanetStarCoreDirect(frame, coreLayer, sampleX, sampleY, color) ||
          samplePlanetStarBlobDirect(frame, blobLayer, sampleX, sampleY, color)) {
        drawPlanetDirectBlockColor(display, x, y, sampleStep, color);
      }
    }
  }
}

// NOTE: 当前回到板端单帧直绘链作为活跃分析基线。
// 这版的目标是保留性能日志与连续相位，回到“会动但卡”的可分析状态。
float drawPlanetScreensaver() {
  PlanetRenderFrame frame = buildPlanetRenderFrame(millis());
  drawPlanetBackgroundFastPreview(frame);
  s_planetDirectColorVariantActive = true;
  s_planetDirectRenderActive = true;

  if (planetPresetEqualsValue(frame.presetId, "black_hole")) {
    drawBlackHolePlanetFastPreview(frame);
  } else if (planetPresetEqualsValue(frame.presetId, "galaxy")) {
    drawGalaxyPlanetFastPreview(frame);
  } else if (planetPresetEqualsValue(frame.presetId, "star")) {
    drawStarPlanetFastPreview(frame);
  } else if (planetPresetEqualsValue(frame.presetId, "asteroid")) {
    drawAsteroidPlanetFastPreview(frame);
  } else {
    drawSpherePlanetFastPreview(frame);
  }

  s_planetDirectRenderActive = false;
  s_planetDirectColorVariantActive = false;

  if (s_planetConfig.time.show) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
      char buffer[16];
      if (s_planetConfig.showSeconds) {
        snprintf(
          buffer,
          sizeof(buffer),
          "%02d:%02d:%02d",
          timeinfo.tm_hour,
          timeinfo.tm_min,
          timeinfo.tm_sec
        );
      } else {
        snprintf(
          buffer,
          sizeof(buffer),
          "%02d:%02d",
          timeinfo.tm_hour,
          timeinfo.tm_min
        );
      }

      uint8_t fontId = s_planetConfig.font;
      if (fontId > CLOCK_FONT_PIXEL_RAIL_5X8) {
        fontId = CLOCK_FONT_CLASSIC_5X7;
      }

      int fontSize = (int)s_planetConfig.time.fontSize;
      if (fontSize < 1) {
        fontSize = 1;
      }
      if (fontSize > 3) {
        fontSize = 3;
      }

      int width = getClockTextWidth(buffer, fontId, fontSize);
      int height = getClockTextHeight(fontId, fontSize);
      int maxX = DisplayManager::PANEL_RES_X - width;
      int maxY = DisplayManager::PANEL_RES_Y - height;
      if (maxX < 0) {
        maxX = 0;
      }
      if (maxY < 0) {
        maxY = 0;
      }

      int x = (int)s_planetConfig.time.x;
      int y = (int)s_planetConfig.time.y;
      if (x < 0) {
        x = 0;
      }
      if (x > maxX) {
        x = maxX;
      }
      if (y < 0) {
        y = 0;
      }
      if (y > maxY) {
        y = maxY;
      }

      drawClockText(
        DisplayManager::dma_display,
        buffer,
        x,
        y,
        DisplayManager::dma_display->color565(
          s_planetConfig.time.r,
          s_planetConfig.time.g,
          s_planetConfig.time.b
        ),
        fontId,
        fontSize
      );
    }
  }
  return frame.progress;
}

void setActiveMode(BoardNativeMode mode) {
  if (s_mode == BOARD_NATIVE_PLANET && mode != BOARD_NATIVE_PLANET) {
    releasePlanetRuntimeBuffers();
  }
  s_mode = mode;
  s_active = true;
  s_lastTickAt = millis();
  s_lastRenderAt = 0;
  s_animTick = 0;
  resetPlanetPerfWindow(s_lastTickAt);
  if (mode == BOARD_NATIVE_PLANET) {
    s_planetPhase = 0.0f;
    s_planetPhaseBaseAt = s_lastTickAt;
    s_planetDirty = true;
    s_planetForceFullRefresh = true;
    s_planetTerranWetTileCacheDisabled = false;
  }
}

bool planetScreensaverConfigsEqual(
  const PlanetScreensaverNativeConfig& left,
  const PlanetScreensaverNativeConfig& right
) {
  return strcmp(left.preset, right.preset) == 0 &&
         strcmp(left.size, right.size) == 0 &&
         strcmp(left.direction, right.direction) == 0 &&
         left.speed == right.speed &&
         left.seed == right.seed &&
         left.colorSeed == right.colorSeed &&
         left.planetX == right.planetX &&
         left.planetY == right.planetY &&
         left.font == right.font &&
         left.showSeconds == right.showSeconds &&
         left.time.show == right.time.show &&
         left.time.fontSize == right.time.fontSize &&
         left.time.x == right.time.x &&
         left.time.y == right.time.y &&
         left.time.r == right.time.r &&
         left.time.g == right.time.g &&
         left.time.b == right.time.b;
}

void releasePlanetRuntimeBuffers() {
  if (s_planetPreviewBuffer != nullptr) {
    free(s_planetPreviewBuffer);
    s_planetPreviewBuffer = nullptr;
  }

  if (s_planetTerranWetStaticCache.pixelsData != nullptr) {
    free(s_planetTerranWetStaticCache.pixelsData);
    s_planetTerranWetStaticCache.pixelsData = nullptr;
  }
  s_planetTerranWetStaticCache.valid = false;
  s_planetTerranWetStaticCache.sizeScale = 0.0f;
  s_planetTerranWetStaticCache.pixels = 0;
  s_planetTerranWetStaticCache.activeCount = 0;

  if (s_planetTerranWetLandTile != nullptr) {
    free(s_planetTerranWetLandTile);
    s_planetTerranWetLandTile = nullptr;
  }
  if (s_planetTerranWetCloudTile != nullptr) {
    free(s_planetTerranWetCloudTile);
    s_planetTerranWetCloudTile = nullptr;
  }
  if (s_planetTerranWetCloudNoiseTile != nullptr) {
    free(s_planetTerranWetCloudNoiseTile);
    s_planetTerranWetCloudNoiseTile = nullptr;
  }

  for (int index = 0; index < kPlanetGenericFbmTileSlotCount; index += 1) {
    if (s_planetGenericFbmTileCaches[index].values != nullptr) {
      free(s_planetGenericFbmTileCaches[index].values);
      s_planetGenericFbmTileCaches[index].values = nullptr;
    }
    s_planetGenericFbmTileCaches[index].valid = false;
    s_planetGenericFbmTileCaches[index].useTick = 0UL;
  }
  s_planetGenericFbmTileCacheDisabled = false;
  s_planetGenericFbmTileUseTick = 0UL;
  s_planetGenericFbmTileBuildDepth = 0;

  if (s_planetUntiledFbmRectCaches != nullptr) {
    for (int index = 0; index < kPlanetUntiledFbmRectSlotCount; index += 1) {
      if (s_planetUntiledFbmRectCaches[index].values != nullptr) {
        free(s_planetUntiledFbmRectCaches[index].values);
        s_planetUntiledFbmRectCaches[index].values = nullptr;
      }
      s_planetUntiledFbmRectCaches[index].valid = false;
      s_planetUntiledFbmRectCaches[index].useTick = 0UL;
    }
    free(s_planetUntiledFbmRectCaches);
    s_planetUntiledFbmRectCaches = nullptr;
  }
  s_planetUntiledFbmRectCacheDisabled = false;
  s_planetUntiledFbmRectUseTick = 0UL;

  if (s_planetStarCellsTileCaches != nullptr) {
    for (int index = 0; index < 2; index += 1) {
      if (s_planetStarCellsTileCaches[index].values != nullptr) {
        free(s_planetStarCellsTileCaches[index].values);
        s_planetStarCellsTileCaches[index].values = nullptr;
      }
      s_planetStarCellsTileCaches[index].valid = false;
      s_planetStarCellsTileCaches[index].numCells = 0;
      s_planetStarCellsTileCaches[index].tiles = 0;
    }
    free(s_planetStarCellsTileCaches);
    s_planetStarCellsTileCaches = nullptr;
  }
  s_planetStarCellsTileCacheDisabled = false;

  if (s_planetStarBlobTileCache != nullptr) {
    if (s_planetStarBlobTileCache->values != nullptr) {
      free(s_planetStarBlobTileCache->values);
      s_planetStarBlobTileCache->values = nullptr;
    }
    s_planetStarBlobTileCache->valid = false;
    s_planetStarBlobTileCache->seedBits = 0U;
    s_planetStarBlobTileCache->sizeBits = 0U;
    s_planetStarBlobTileCache->circleAmountBits = 0U;
    s_planetStarBlobTileCache->circleSizeBits = 0U;
    free(s_planetStarBlobTileCache);
    s_planetStarBlobTileCache = nullptr;
  }
  s_planetStarBlobTileCacheDisabled = false;

  if (s_planetDirectColorCache != nullptr) {
    free(s_planetDirectColorCache);
    s_planetDirectColorCache = nullptr;
  }
  s_planetDirectColorCacheDisabled = false;

  if (s_planetDirectLayerSampleGridCache != nullptr) {
    if (s_planetDirectLayerSampleGridCache->cells != nullptr) {
      free(s_planetDirectLayerSampleGridCache->cells);
      s_planetDirectLayerSampleGridCache->cells = nullptr;
    }
    s_planetDirectLayerSampleGridCache->valid = false;
    s_planetDirectLayerSampleGridCache->presetHash = 0U;
    free(s_planetDirectLayerSampleGridCache);
    s_planetDirectLayerSampleGridCache = nullptr;
  }

  if (s_planetDirectSphereGeometryGridCache != nullptr) {
    if (s_planetDirectSphereGeometryGridCache->cells != nullptr) {
      free(s_planetDirectSphereGeometryGridCache->cells);
      s_planetDirectSphereGeometryGridCache->cells = nullptr;
    }
    s_planetDirectSphereGeometryGridCache->valid = false;
    s_planetDirectSphereGeometryGridCache->presetHash = 0U;
    free(s_planetDirectSphereGeometryGridCache);
    s_planetDirectSphereGeometryGridCache = nullptr;
  }
  s_planetDirectGridCacheDisabled = false;

  if (s_planetDirectLayerSampleCache != nullptr) {
    s_planetDirectLayerSampleCache->valid = false;
    s_planetDirectLayerSampleCache->hit = false;
    s_planetDirectLayerSampleCache->presetHash = 0U;
    free(s_planetDirectLayerSampleCache);
    s_planetDirectLayerSampleCache = nullptr;
  }
  if (s_planetDirectSphereGeometryCache != nullptr) {
    s_planetDirectSphereGeometryCache->valid = false;
    s_planetDirectSphereGeometryCache->hit = false;
    s_planetDirectSphereGeometryCache->presetHash = 0U;
    free(s_planetDirectSphereGeometryCache);
    s_planetDirectSphereGeometryCache = nullptr;
  }
  invalidatePlanetDirectCaches();

  s_planetTerranWetLandTileState.valid = false;
  s_planetTerranWetLandTileState.seed = 0U;
  s_planetTerranWetCloudTileState.valid = false;
  s_planetTerranWetCloudTileState.seed = 0U;
  s_planetTerranWetCloudNoiseTileState.valid = false;
  s_planetTerranWetCloudNoiseTileState.seed = 0U;
  s_planetTerranWetLandRandCache.valid = false;
  s_planetTerranWetCloudRandCache.valid = false;
  s_planetDirectNoiseRandCache.valid = false;
  s_planetTerranWetTileCacheDisabled = false;
}
}

namespace BoardNativeEffect {
void init() {
  s_mode = BOARD_NATIVE_NONE;
  s_active = false;
  s_lastTickAt = millis();
  s_animTick = 0;
  s_planetPhase = 0.0f;
  s_planetPhaseBaseAt = 0;
  s_planetDirty = false;
  s_planetForceFullRefresh = false;
  s_planetTerranWetTileCacheDisabled = false;
  releasePlanetRuntimeBuffers();
  resetPlanetPerfWindow(s_lastTickAt);
  refreshPlanetColorVariant();
}

void deactivate() {
  if (s_mode == BOARD_NATIVE_PLANET) {
    releasePlanetRuntimeBuffers();
  }
  s_active = false;
  s_mode = BOARD_NATIVE_NONE;
  resetPlanetPerfWindow(millis());
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

void setPlanetScreensaverConfig(const PlanetScreensaverNativeConfig& config) {
  bool runtimeConfigChanged = !planetScreensaverConfigsEqual(s_planetConfig, config);
  unsigned long now = millis();
  if (s_active && s_mode == BOARD_NATIVE_PLANET) {
    s_planetPhase = resolvePlanetPlaybackPhase(now);
    s_planetPhaseBaseAt = now;
    s_planetDirty = true;
    if (runtimeConfigChanged) {
      s_planetForceFullRefresh = true;
    }
  }
  if (runtimeConfigChanged) {
    releasePlanetRuntimeBuffers();
  }
  s_planetConfig = config;
  refreshPlanetColorVariant();
}

void applyPlanetScreensaverConfig(const PlanetScreensaverNativeConfig& config) {
  setPlanetScreensaverConfig(config);
  setActiveMode(BOARD_NATIVE_PLANET);
}

void update() {
  if (!s_active) {
    return;
  }

  unsigned long now = millis();
  if (s_mode == BOARD_NATIVE_PLANET) {
    ensurePlanetPerfWindowStarted(now);
    s_planetPerfStats.updateCalls += 1UL;
    unsigned long tickMs = resolvePlanetTickMs();
    if (s_lastTickAt == 0) {
      s_lastTickAt = now;
    }
    if (now - s_lastTickAt >= tickMs) {
      unsigned long ticksElapsed = (now - s_lastTickAt) / tickMs;
      if (ticksElapsed > 0UL) {
        unsigned long appliedTicks = 1UL;
        unsigned long advancedAt = s_lastTickAt + tickMs;
        s_planetPerfStats.tickWakeups += 1UL;
        s_planetPerfStats.elapsedTicks += appliedTicks;
        if (ticksElapsed > 1UL) {
          s_planetPerfStats.droppedTicks += (ticksElapsed - 1UL);
        }
        s_planetPhase = resolvePlanetPlaybackPhase(advancedAt);
        s_planetPhaseBaseAt = advancedAt;
        s_lastTickAt = advancedAt;
        s_animTick += appliedTicks;
        s_planetDirty = true;
      }
    }
    logPlanetPerfIfNeeded(now);
    return;
  }

  if (s_lastTickAt == 0 || now - s_lastTickAt >= 33) {
    s_lastTickAt = now;
    s_animTick += 1;
  }
}

void render() {
  if (!s_active || DisplayManager::dma_display == nullptr) {
    return;
  }

  unsigned long now = millis();
  if (s_mode == BOARD_NATIVE_PLANET) {
    ensurePlanetPerfWindowStarted(now);
    if (!s_planetDirty) {
      s_planetPerfStats.renderSkippedClean += 1UL;
      logPlanetPerfIfNeeded(now);
      return;
    }
    if (s_planetForceFullRefresh) {
      DisplayManager::dma_display->clearScreen();
      DisplayManager::liveFrameValid = false;
      DisplayManager::animationBufferValid = false;
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
  float renderedPlanetProgress = 0.0f;
  unsigned long drawStartedUs = 0UL;
  unsigned long drawEndedUs = 0UL;
  if (s_mode == BOARD_NATIVE_TEXT_DISPLAY) {
    drawTextDisplay();
    rendered = true;
  } else if (s_mode == BOARD_NATIVE_PLANET) {
    drawStartedUs = micros();
    renderedPlanetProgress = drawPlanetScreensaver();
    drawEndedUs = micros();
    rendered = true;
  }

  if (!rendered) {
    DisplayManager::endRedirectedFrame(false);
    return;
  }

  DisplayManager::endRedirectedFrame(true);
  if (s_mode == BOARD_NATIVE_PLANET) {
    unsigned long drawUs = drawEndedUs >= drawStartedUs ? (drawEndedUs - drawStartedUs) : 0UL;
    unsigned long presentUs = DisplayManager::getLastPresentDurationUs();
    uint16_t changedPixels = DisplayManager::getLastPresentChangedPixels();
    recordPlanetRenderPerf(now, renderedPlanetProgress, drawUs, presentUs, changedPixels);
    s_planetDirty = false;
    s_planetForceFullRefresh = false;
    logPlanetPerfIfNeeded(now);
  }
}

const TextDisplayNativeConfig& getTextDisplayConfig() {
  return s_textConfig;
}

const PlanetScreensaverNativeConfig& getPlanetScreensaverConfig() {
  return s_planetConfig;
}
}
