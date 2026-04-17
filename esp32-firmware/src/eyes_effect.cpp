#include "eyes_effect.h"
#include "display_manager.h"
#include "clock_font_renderer.h"
#include <math.h>
#include <string.h>
#include <time.h>

namespace {

enum EyeExpressionId : uint8_t {
  EYE_EXPRESSION_NORMAL = 0,
  EYE_EXPRESSION_ANGRY,
  EYE_EXPRESSION_GLEE,
  EYE_EXPRESSION_HAPPY,
  EYE_EXPRESSION_SAD,
  EYE_EXPRESSION_WORRIED,
  EYE_EXPRESSION_FOCUSED,
  EYE_EXPRESSION_ANNOYED,
  EYE_EXPRESSION_SURPRISED,
  EYE_EXPRESSION_SKEPTIC,
  EYE_EXPRESSION_FRUSTRATED,
  EYE_EXPRESSION_UNIMPRESSED,
  EYE_EXPRESSION_SLEEPY,
  EYE_EXPRESSION_SUSPICIOUS,
  EYE_EXPRESSION_SQUINT,
  EYE_EXPRESSION_FURIOUS,
  EYE_EXPRESSION_SCARED,
  EYE_EXPRESSION_AWE,
  EYE_EXPRESSION_EXCITED,
  EYE_EXPRESSION_DETERMINED,
  EYE_EXPRESSION_CONFUSED,
  EYE_EXPRESSION_COUNT
};

enum TimeOfDay : uint8_t {
  TIME_DEEP_NIGHT = 0,
  TIME_EARLY_MORNING,
  TIME_MORNING,
  TIME_NOON,
  TIME_AFTERNOON,
  TIME_EVENING,
  TIME_NIGHT
};

struct EyeShapePreset {
  float offsetX;
  float offsetY;
  float height;
  float width;
  float slopeTop;
  float slopeBottom;
  float radiusTop;
  float radiusBottom;
};

struct EyeRenderTransform {
  float moveX;
  float moveY;
  float scaleX;
  float scaleY;
};

struct EyeShapeDelta {
  float offsetX;
  float offsetY;
  float height;
  float width;
  float slopeTop;
  float slopeBottom;
  float radiusTop;
  float radiusBottom;
};

struct EyeVariationLayerConfig {
  EyeShapeDelta delta;
  unsigned long periodMs;
  float phaseOffset;
  bool bidirectional;
};

struct RasterEyeShape {
  float centerX;
  float centerY;
  float offsetX;
  float offsetY;
  float width;
  float height;
  float slopeTop;
  float slopeBottom;
  float radiusTop;
  float radiusBottom;
  float deltaTop;
  float deltaBottom;
  float TLcX;
  float TLcY;
  float TRcX;
  float TRcY;
  float BLcX;
  float BLcY;
  float BRcX;
  float BRcY;
  float minCX;
  float maxCX;
  float minCY;
  float maxCY;
  float minX;
  float maxX;
  float minY;
  float maxY;
};

struct EyesRuntimeState {
  float lookX;
  float lookY;
  float targetLookX;
  float targetLookY;
  float leftLookOffsetX;
  float leftLookOffsetY;
  float rightLookOffsetX;
  float rightLookOffsetY;
  float targetLeftLookOffsetX;
  float targetLeftLookOffsetY;
  float targetRightLookOffsetX;
  float targetRightLookOffsetY;
  unsigned long lastBlinkAt;
  unsigned long nextBlinkAfterMs;
  unsigned long blinkStartAt;
  bool blinkActive;
  bool transitionActive;
  bool transitionApplied;
  uint16_t leftBlinkDelayMs;
  uint16_t rightBlinkDelayMs;
  float leftBlinkScale;
  float rightBlinkScale;
  unsigned long lastLookAt;
  unsigned long nextLookAfterMs;
  unsigned long lastExpressionAt;
  unsigned long actionExpireAt;
  unsigned long lastInteractionAt;
  EyeExpressionId expression;
  EyeExpressionId pendingExpression;
  EyeExpressionId history[5];
  uint8_t historyCount;
};

struct WeightedExpression {
  EyeExpressionId expression;
  float weight;
};

struct PetPose {
  int headShiftX;
  int headShiftY;
  int bodyShiftY;
  int handPressY;
  int earShakeX;
  int tailSwingX;
};

static const EyeShapePreset kEyePresets[EYE_EXPRESSION_COUNT] = {
  {0.0f, 0.0f, 20.0f, 20.0f, 0.0f, 0.0f, 4.0f, 4.0f},   // Normal
  {-2.0f, 0.0f, 10.0f, 20.0f, 0.3f, 0.0f, 1.0f, 6.0f},  // Angry
  {0.0f, 0.0f, 4.0f, 20.0f, 0.0f, 0.0f, 4.0f, 0.0f},    // Glee
  {0.0f, 0.0f, 5.0f, 20.0f, 0.0f, 0.0f, 5.0f, 0.0f},    // Happy
  {0.0f, 0.0f, 8.0f, 20.0f, -0.5f, 0.0f, 1.0f, 5.0f},   // Sad
  {0.0f, 0.0f, 13.0f, 20.0f, -0.1f, 0.0f, 3.0f, 5.0f},  // Worried
  {0.0f, 0.0f, 7.0f, 20.0f, 0.2f, 0.0f, 2.0f, 1.0f},    // Focused
  {0.0f, 0.0f, 6.0f, 20.0f, 0.0f, 0.0f, 0.0f, 5.0f},    // Annoyed
  {-1.0f, 0.0f, 23.0f, 23.0f, 0.0f, 0.0f, 8.0f, 8.0f},  // Surprised
  {0.0f, -3.0f, 13.0f, 20.0f, 0.3f, 0.0f, 1.0f, 5.0f},  // Skeptic
  {2.0f, -3.0f, 6.0f, 20.0f, 0.0f, 0.0f, 0.0f, 5.0f},   // Frustrated
  {2.0f, 0.0f, 6.0f, 20.0f, 0.0f, 0.0f, 1.0f, 5.0f},    // Unimpressed
  {0.0f, -1.0f, 7.0f, 20.0f, -0.5f, 0.0f, 2.0f, 1.5f},  // Sleepy
  {0.0f, 0.0f, 11.0f, 20.0f, 0.0f, 0.0f, 4.0f, 2.0f},   // Suspicious
  {3.0f, 0.0f, 10.0f, 10.0f, 0.0f, 0.0f, 3.0f, 3.0f},   // Squint
  {-1.0f, 0.0f, 15.0f, 20.0f, 0.4f, 0.0f, 1.0f, 4.0f},  // Furious
  {-2.0f, 0.0f, 20.0f, 20.0f, -0.1f, 0.0f, 6.0f, 4.0f}, // Scared
  {1.0f, 0.0f, 18.0f, 23.0f, -0.1f, 0.1f, 6.0f, 6.0f},  // Awe
  {0.0f, -2.0f, 24.0f, 22.0f, 0.05f, -0.05f, 7.0f, 7.0f}, // Excited
  {0.0f, 0.0f, 15.0f, 20.0f, 0.3f, 0.0f, 2.0f, 2.0f},   // Determined
  {0.0f, 0.0f, 18.0f, 20.0f, -0.25f, 0.15f, 5.0f, 5.0f} // Confused
};
static const float kReferenceEyeSize = 20.0f;
static const unsigned long kBlinkDurationMs = 150;
static const unsigned long kEyesFrameIntervalMs = 42;
static const unsigned long kPetBreathingPeriodMs = 2200;
static const unsigned long kPetEarShakeWindowMs = 120;
static const unsigned long kPetTailSwingPeriodMs = 1400;

static const int kPetHeadMinX = 12;
static const int kPetHeadMaxX = 51;
static const int kPetHeadMinY = 14;
static const int kPetHeadMaxY = 43;
static const int kPetBodyMinX = 18;
static const int kPetBodyMaxX = 45;
static const int kPetBodyMinY = 34;
static const int kPetBodyMaxY = 55;
static const int kPetLeftHandMinX = 8;
static const int kPetLeftHandMaxX = 23;
static const int kPetLeftHandMinY = 42;
static const int kPetLeftHandMaxY = 58;
static const int kPetRightHandMinX = 40;
static const int kPetRightHandMaxX = 55;
static const int kPetRightHandMinY = 42;
static const int kPetRightHandMaxY = 58;
static const int kPetLeftEarAnchorX = 18;
static const int kPetLeftEarAnchorY = 12;
static const int kPetRightEarAnchorX = 45;
static const int kPetRightEarAnchorY = 12;
static const int kPetTailAnchorX = 51;
static const int kPetTailAnchorY = 46;

static const uint8_t kPetColorBaseR = 255;
static const uint8_t kPetColorBaseG = 215;
static const uint8_t kPetColorBaseB = 119;
static const uint8_t kPetColorShadowR = 241;
static const uint8_t kPetColorShadowG = 178;
static const uint8_t kPetColorShadowB = 92;
static const uint8_t kPetColorPawR = 255;
static const uint8_t kPetColorPawG = 236;
static const uint8_t kPetColorPawB = 207;
static const uint8_t kPetColorAccentR = 242;
static const uint8_t kPetColorAccentG = 151;
static const uint8_t kPetColorAccentB = 74;
static const uint8_t kPetColorTailR = 225;
static const uint8_t kPetColorTailG = 127;
static const uint8_t kPetColorTailB = 69;
static const uint8_t kPetColorOutlineR = 95;
static const uint8_t kPetColorOutlineG = 58;
static const uint8_t kPetColorOutlineB = 27;

EyesRuntimeState s_state = {};
unsigned long s_lastEyesRenderAt = 0;

static uint8_t clampByte(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 255) {
    return 255;
  }
  return (uint8_t)value;
}

static int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

static float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

static float randomFloat(float minValue, float maxValue) {
  long raw = random(0, 10001);
  return minValue + (maxValue - minValue) * ((float)raw / 10000.0f);
}

static unsigned long jitteredInterval(unsigned long baseMs, int minPercent, int maxPercent) {
  long percent = random(minPercent, maxPercent + 1);
  return (unsigned long)((float)baseMs * ((float)percent / 100.0f));
}

static bool parseHexColor(const char* hex, uint8_t& r, uint8_t& g, uint8_t& b) {
  if (hex == nullptr) {
    return false;
  }

  if (strlen(hex) != 7 || hex[0] != '#') {
    return false;
  }

  auto hexDigit = [](char ch) -> int {
    if (ch >= '0' && ch <= '9') {
      return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
      return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
      return ch - 'A' + 10;
    }
    return -1;
  };

  int digits[6];
  for (int i = 0; i < 6; i++) {
    digits[i] = hexDigit(hex[i + 1]);
    if (digits[i] < 0) {
      return false;
    }
  }

  r = (uint8_t)(digits[0] * 16 + digits[1]);
  g = (uint8_t)(digits[2] * 16 + digits[3]);
  b = (uint8_t)(digits[4] * 16 + digits[5]);
  return true;
}

static float normalizedLoopPhase(unsigned long now, unsigned long periodMs, float phaseOffset) {
  if (periodMs == 0) {
    return 0.0f;
  }

  float phase = ((float)(now % periodMs) / (float)periodMs) + phaseOffset;
  phase = phase - floorf(phase);
  return phase;
}

static float trianglePulse(unsigned long now, unsigned long periodMs, float phaseOffset) {
  float phase = normalizedLoopPhase(now, periodMs, phaseOffset);
  if (phase < 0.5f) {
    return phase * 2.0f;
  }
  return (1.0f - phase) * 2.0f;
}

static float signedTrianglePulse(unsigned long now, unsigned long periodMs, float phaseOffset) {
  return trianglePulse(now, periodMs, phaseOffset) * 2.0f - 1.0f;
}

static void applyShapeDelta(EyeShapePreset& preset, const EyeShapeDelta& delta, float factor) {
  preset.offsetX += delta.offsetX * factor;
  preset.offsetY += delta.offsetY * factor;
  preset.height += delta.height * factor;
  preset.width += delta.width * factor;
  preset.slopeTop += delta.slopeTop * factor;
  preset.slopeBottom += delta.slopeBottom * factor;
  preset.radiusTop += delta.radiusTop * factor;
  preset.radiusBottom += delta.radiusBottom * factor;
}

static EyeShapeDelta makeShapeDelta(
  float offsetX,
  float offsetY,
  float height,
  float width,
  float slopeTop,
  float slopeBottom,
  float radiusTop,
  float radiusBottom
) {
  EyeShapeDelta delta;
  delta.offsetX = offsetX;
  delta.offsetY = offsetY;
  delta.height = height;
  delta.width = width;
  delta.slopeTop = slopeTop;
  delta.slopeBottom = slopeBottom;
  delta.radiusTop = radiusTop;
  delta.radiusBottom = radiusBottom;
  return delta;
}

static EyeVariationLayerConfig makeVariationLayer(
  const EyeShapeDelta& delta,
  unsigned long periodMs,
  float phaseOffset,
  bool bidirectional
) {
  EyeVariationLayerConfig layer;
  layer.delta = delta;
  layer.periodMs = periodMs;
  layer.phaseOffset = phaseOffset;
  layer.bidirectional = bidirectional;
  return layer;
}

static void configureVariationLayers(
  EyeExpressionId expression,
  bool isLeftEye,
  EyeVariationLayerConfig& layerA,
  EyeVariationLayerConfig& layerB
) {
  layerA = makeVariationLayer(makeShapeDelta(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 0, 0.0f, false);
  layerB = makeVariationLayer(makeShapeDelta(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 0, 0.0f, false);

  if (expression == EYE_EXPRESSION_NORMAL) {
    layerA = makeVariationLayer(
      makeShapeDelta(0.0f, 0.0f, isLeftEye ? 2.4f : 1.6f, isLeftEye ? 0.9f : 1.3f, 0.0f, 0.0f, 0.0f, 0.0f),
      isLeftEye ? 1800UL : 2200UL,
      isLeftEye ? 0.10f : 0.62f,
      true
    );
    layerB = makeVariationLayer(
      makeShapeDelta(0.0f, isLeftEye ? -0.6f : 0.4f, 0.8f, 0.0f, isLeftEye ? -0.03f : 0.03f, 0.0f, 0.0f, 0.0f),
      isLeftEye ? 3100UL : 2700UL,
      isLeftEye ? 0.37f : 0.14f,
      false
    );
    return;
  }

  if (expression == EYE_EXPRESSION_WORRIED) {
    layerA = makeVariationLayer(
      makeShapeDelta(0.0f, isLeftEye ? -0.8f : 0.6f, isLeftEye ? 2.6f : 1.5f, 0.0f, isLeftEye ? -0.08f : 0.04f, 0.0f, 0.0f, 0.5f),
      isLeftEye ? 1450UL : 1800UL,
      isLeftEye ? 0.15f : 0.56f,
      false
    );
    return;
  }

  if (expression == EYE_EXPRESSION_SKEPTIC) {
    layerA = makeVariationLayer(
      makeShapeDelta(0.0f, isLeftEye ? -0.7f : 0.2f, isLeftEye ? 2.0f : 0.9f, 0.0f, isLeftEye ? 0.08f : -0.02f, 0.0f, 0.0f, 0.0f),
      isLeftEye ? 1600UL : 2100UL,
      isLeftEye ? 0.25f : 0.64f,
      false
    );
    layerB = makeVariationLayer(
      makeShapeDelta(isLeftEye ? 0.2f : -0.2f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f),
      isLeftEye ? 2900UL : 2500UL,
      isLeftEye ? 0.48f : 0.08f,
      true
    );
    return;
  }

  if (expression == EYE_EXPRESSION_SLEEPY) {
    layerA = makeVariationLayer(
      makeShapeDelta(0.0f, isLeftEye ? -0.2f : 0.3f, isLeftEye ? 1.1f : 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
      isLeftEye ? 2200UL : 2600UL,
      isLeftEye ? 0.12f : 0.58f,
      false
    );
    return;
  }

  if (expression == EYE_EXPRESSION_SQUINT) {
    layerA = makeVariationLayer(
      makeShapeDelta(isLeftEye ? 1.2f : -0.4f, isLeftEye ? 0.8f : 0.2f, isLeftEye ? 2.4f : 0.9f, isLeftEye ? 1.6f : 0.4f, 0.0f, 0.0f, 0.0f, 0.0f),
      isLeftEye ? 1700UL : 2300UL,
      isLeftEye ? 0.05f : 0.66f,
      false
    );
    return;
  }

  if (expression == EYE_EXPRESSION_UNIMPRESSED) {
    layerA = makeVariationLayer(
      makeShapeDelta(0.0f, isLeftEye ? -0.5f : 0.2f, isLeftEye ? 1.6f : 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f),
      isLeftEye ? 1550UL : 2050UL,
      isLeftEye ? 0.31f : 0.73f,
      false
    );
    return;
  }

  if (expression == EYE_EXPRESSION_CONFUSED) {
    layerA = makeVariationLayer(
      makeShapeDelta(0.0f, isLeftEye ? -0.7f : 0.3f, isLeftEye ? 1.8f : 0.8f, 0.4f, isLeftEye ? -0.05f : 0.03f, isLeftEye ? 0.03f : -0.02f, 0.2f, 0.2f),
      isLeftEye ? 1900UL : 2350UL,
      isLeftEye ? 0.18f : 0.61f,
      false
    );
    layerB = makeVariationLayer(
      makeShapeDelta(isLeftEye ? 0.4f : -0.3f, 0.0f, 0.0f, isLeftEye ? 0.7f : 0.4f, 0.0f, 0.0f, 0.0f, 0.0f),
      isLeftEye ? 3300UL : 2800UL,
      isLeftEye ? 0.49f : 0.12f,
      true
    );
    return;
  }

  if (expression == EYE_EXPRESSION_GLEE || expression == EYE_EXPRESSION_HAPPY) {
    layerA = makeVariationLayer(
      makeShapeDelta(0.0f, isLeftEye ? 0.7f : 0.4f, isLeftEye ? 0.7f : 0.5f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f),
      isLeftEye ? 1250UL : 1650UL,
      isLeftEye ? 0.21f : 0.53f,
      false
    );
    return;
  }
}

static EyeShapePreset applyVariationLayers(
  EyeExpressionId expression,
  bool isLeftEye,
  unsigned long now,
  EyeShapePreset preset
) {
  EyeVariationLayerConfig layerA;
  EyeVariationLayerConfig layerB;
  configureVariationLayers(expression, isLeftEye, layerA, layerB);

  if (layerA.periodMs > 0) {
    float factorA = layerA.bidirectional
      ? signedTrianglePulse(now, layerA.periodMs, layerA.phaseOffset)
      : trianglePulse(now, layerA.periodMs, layerA.phaseOffset);
    applyShapeDelta(preset, layerA.delta, factorA);
  }

  if (layerB.periodMs > 0) {
    float factorB = layerB.bidirectional
      ? signedTrianglePulse(now, layerB.periodMs, layerB.phaseOffset)
      : trianglePulse(now, layerB.periodMs, layerB.phaseOffset);
    applyShapeDelta(preset, layerB.delta, factorB);
  }

  preset.height = clampFloat(preset.height, 1.0f, 32.0f);
  preset.width = clampFloat(preset.width, 2.0f, 32.0f);
  preset.radiusTop = clampFloat(preset.radiusTop, 0.0f, 16.0f);
  preset.radiusBottom = clampFloat(preset.radiusBottom, 0.0f, 16.0f);
  return preset;
}

static EyeShapePreset presetForEye(EyeExpressionId expression, bool isLeftEye) {
  EyeShapePreset preset = kEyePresets[(int)expression];

  if (!isLeftEye) {
    return preset;
  }

  switch (expression) {
    case EYE_EXPRESSION_WORRIED:
      preset.offsetY -= 1.5f;
      preset.height += 3.0f;
      preset.slopeTop -= 0.08f;
      break;
    case EYE_EXPRESSION_ANNOYED:
      preset.offsetY -= 2.0f;
      preset.height += 4.0f;
      preset.radiusBottom += 1.5f;
      break;
    case EYE_EXPRESSION_SKEPTIC:
      preset.offsetY -= 1.5f;
      preset.height += 2.5f;
      preset.slopeTop += 0.05f;
      break;
    case EYE_EXPRESSION_UNIMPRESSED:
      preset.offsetY -= 2.0f;
      preset.height += 5.5f;
      preset.radiusBottom += 1.5f;
      break;
    case EYE_EXPRESSION_SLEEPY:
      preset.height -= 1.5f;
      preset.offsetY -= 0.4f;
      break;
    case EYE_EXPRESSION_SUSPICIOUS:
      preset.offsetY -= 1.8f;
      preset.height += 2.0f;
      preset.slopeTop += 0.12f;
      break;
    case EYE_EXPRESSION_SQUINT:
      preset.offsetX = 1.2f;
      preset.offsetY = -0.5f;
      preset.width += 5.0f;
      preset.height += 6.0f;
      break;
    case EYE_EXPRESSION_CONFUSED:
      preset.offsetY -= 1.4f;
      preset.height += 2.8f;
      preset.slopeTop -= 0.08f;
      preset.slopeBottom += 0.04f;
      break;
    default:
      break;
  }

  return preset;
}

static EyeRenderTransform buildLookTransform(const EyesConfig& config, bool isLeftEye) {
  float lookX = s_state.lookX + (isLeftEye ? s_state.leftLookOffsetX : s_state.rightLookOffsetX);
  float lookY = s_state.lookY + (isLeftEye ? s_state.leftLookOffsetY : s_state.rightLookOffsetY);
  float normalizedX = clampFloat(lookX, -1.0f, 1.0f);
  float normalizedY = clampFloat(lookY, -1.0f, 1.0f);

  EyeRenderTransform transform;
  float moveDirection = isLeftEye ? -1.0f : 1.0f;
  transform.moveX = (float)config.behavior.idleMove * 0.85f * normalizedX * moveDirection;
  transform.moveY = (float)config.behavior.idleMove * 0.65f * normalizedY;
  transform.scaleX = 1.0f;

  float lookSquash = 1.0f - fabsf(normalizedY) * 0.18f;
  float sideBias = isLeftEye ? (1.0f + normalizedX * 0.16f) : (1.0f - normalizedX * 0.16f);
  transform.scaleY = clampFloat(lookSquash * sideBias, 0.72f, 1.22f);
  return transform;
}

static TimeOfDay getCurrentTimeOfDay() {
  struct tm timeinfo;
  int hour = 10;
  if (getLocalTime(&timeinfo)) {
    hour = timeinfo.tm_hour;
  }

  if (hour >= 23 || hour < 6) return TIME_DEEP_NIGHT;
  if (hour < 9) return TIME_EARLY_MORNING;
  if (hour < 12) return TIME_MORNING;
  if (hour < 14) return TIME_NOON;
  if (hour < 18) return TIME_AFTERNOON;
  if (hour < 21) return TIME_EVENING;
  return TIME_NIGHT;
}

static bool parseExpressionAction(const char* action, EyeExpressionId& expression) {
  if (strncmp(action, "set_expression:", 15) != 0) {
    return false;
  }

  const char* name = action + 15;
  if (strcmp(name, "Normal") == 0) {
    expression = EYE_EXPRESSION_NORMAL;
    return true;
  }
  if (strcmp(name, "Angry") == 0) {
    expression = EYE_EXPRESSION_ANGRY;
    return true;
  }
  if (strcmp(name, "Glee") == 0) {
    expression = EYE_EXPRESSION_GLEE;
    return true;
  }
  if (strcmp(name, "Happy") == 0) {
    expression = EYE_EXPRESSION_HAPPY;
    return true;
  }
  if (strcmp(name, "Sad") == 0) {
    expression = EYE_EXPRESSION_SAD;
    return true;
  }
  if (strcmp(name, "Worried") == 0) {
    expression = EYE_EXPRESSION_WORRIED;
    return true;
  }
  if (strcmp(name, "Focused") == 0) {
    expression = EYE_EXPRESSION_FOCUSED;
    return true;
  }
  if (strcmp(name, "Annoyed") == 0) {
    expression = EYE_EXPRESSION_ANNOYED;
    return true;
  }
  if (strcmp(name, "Surprised") == 0) {
    expression = EYE_EXPRESSION_SURPRISED;
    return true;
  }
  if (strcmp(name, "Skeptic") == 0) {
    expression = EYE_EXPRESSION_SKEPTIC;
    return true;
  }
  if (strcmp(name, "Frustrated") == 0) {
    expression = EYE_EXPRESSION_FRUSTRATED;
    return true;
  }
  if (strcmp(name, "Unimpressed") == 0) {
    expression = EYE_EXPRESSION_UNIMPRESSED;
    return true;
  }
  if (strcmp(name, "Sleepy") == 0) {
    expression = EYE_EXPRESSION_SLEEPY;
    return true;
  }
  if (strcmp(name, "Suspicious") == 0) {
    expression = EYE_EXPRESSION_SUSPICIOUS;
    return true;
  }
  if (strcmp(name, "Squint") == 0) {
    expression = EYE_EXPRESSION_SQUINT;
    return true;
  }
  if (strcmp(name, "Furious") == 0) {
    expression = EYE_EXPRESSION_FURIOUS;
    return true;
  }
  if (strcmp(name, "Scared") == 0) {
    expression = EYE_EXPRESSION_SCARED;
    return true;
  }
  if (strcmp(name, "Awe") == 0) {
    expression = EYE_EXPRESSION_AWE;
    return true;
  }
  if (strcmp(name, "Excited") == 0) {
    expression = EYE_EXPRESSION_EXCITED;
    return true;
  }
  if (strcmp(name, "Determined") == 0) {
    expression = EYE_EXPRESSION_DETERMINED;
    return true;
  }
  if (strcmp(name, "Confused") == 0) {
    expression = EYE_EXPRESSION_CONFUSED;
    return true;
  }

  return false;
}

static unsigned long minExpressionInterval(TimeOfDay timeOfDay) {
  switch (timeOfDay) {
    case TIME_DEEP_NIGHT:
      return 5800;
    case TIME_EARLY_MORNING:
      return 4200;
    case TIME_NOON:
      return 3600;
    case TIME_EVENING:
      return 3200;
    case TIME_NIGHT:
      return 4000;
    case TIME_MORNING:
    case TIME_AFTERNOON:
    default:
      return 2800;
  }
}

static void fillTimeWeights(TimeOfDay timeOfDay, float weights[EYE_EXPRESSION_COUNT]) {
  for (int i = 0; i < EYE_EXPRESSION_COUNT; i++) {
    weights[i] = 0.45f;
  }

  weights[EYE_EXPRESSION_SLEEPY] = 0.18f;
  weights[EYE_EXPRESSION_SCARED] = 0.20f;
  weights[EYE_EXPRESSION_FURIOUS] = 0.20f;
  weights[EYE_EXPRESSION_ANGRY] = 0.24f;

  switch (timeOfDay) {
    case TIME_DEEP_NIGHT:
      weights[EYE_EXPRESSION_SLEEPY] += 18.0f;
      weights[EYE_EXPRESSION_SQUINT] += 11.0f;
      weights[EYE_EXPRESSION_NORMAL] += 8.0f;
      weights[EYE_EXPRESSION_WORRIED] += 7.0f;
      weights[EYE_EXPRESSION_SAD] += 6.0f;
      weights[EYE_EXPRESSION_UNIMPRESSED] += 4.0f;
      break;
    case TIME_EARLY_MORNING:
      weights[EYE_EXPRESSION_SLEEPY] += 12.0f;
      weights[EYE_EXPRESSION_NORMAL] += 8.0f;
      weights[EYE_EXPRESSION_FOCUSED] += 5.0f;
      weights[EYE_EXPRESSION_WORRIED] += 5.0f;
      weights[EYE_EXPRESSION_SQUINT] += 5.0f;
      weights[EYE_EXPRESSION_UNIMPRESSED] += 4.0f;
      weights[EYE_EXPRESSION_HAPPY] += 3.0f;
      weights[EYE_EXPRESSION_DETERMINED] += 2.0f;
      break;
    case TIME_MORNING:
      weights[EYE_EXPRESSION_NORMAL] += 7.0f;
      weights[EYE_EXPRESSION_FOCUSED] += 9.0f;
      weights[EYE_EXPRESSION_HAPPY] += 8.0f;
      weights[EYE_EXPRESSION_DETERMINED] += 8.0f;
      weights[EYE_EXPRESSION_EXCITED] += 5.0f;
      weights[EYE_EXPRESSION_GLEE] += 4.0f;
      weights[EYE_EXPRESSION_SURPRISED] += 3.0f;
      weights[EYE_EXPRESSION_AWE] += 2.0f;
      weights[EYE_EXPRESSION_SUSPICIOUS] += 2.0f;
      break;
    case TIME_NOON:
      weights[EYE_EXPRESSION_FOCUSED] += 7.0f;
      weights[EYE_EXPRESSION_NORMAL] += 6.0f;
      weights[EYE_EXPRESSION_HAPPY] += 5.0f;
      weights[EYE_EXPRESSION_DETERMINED] += 4.0f;
      weights[EYE_EXPRESSION_SLEEPY] += 4.0f;
      weights[EYE_EXPRESSION_UNIMPRESSED] += 4.0f;
      weights[EYE_EXPRESSION_SQUINT] += 4.0f;
      weights[EYE_EXPRESSION_FRUSTRATED] += 3.0f;
      weights[EYE_EXPRESSION_ANNOYED] += 2.0f;
      break;
    case TIME_AFTERNOON:
      weights[EYE_EXPRESSION_FOCUSED] += 8.0f;
      weights[EYE_EXPRESSION_DETERMINED] += 7.0f;
      weights[EYE_EXPRESSION_HAPPY] += 6.0f;
      weights[EYE_EXPRESSION_NORMAL] += 5.0f;
      weights[EYE_EXPRESSION_EXCITED] += 5.0f;
      weights[EYE_EXPRESSION_SKEPTIC] += 4.0f;
      weights[EYE_EXPRESSION_SUSPICIOUS] += 4.0f;
      weights[EYE_EXPRESSION_SURPRISED] += 3.0f;
      weights[EYE_EXPRESSION_ANNOYED] += 3.0f;
      weights[EYE_EXPRESSION_GLEE] += 3.0f;
      break;
    case TIME_EVENING:
      weights[EYE_EXPRESSION_HAPPY] += 8.0f;
      weights[EYE_EXPRESSION_GLEE] += 7.0f;
      weights[EYE_EXPRESSION_EXCITED] += 7.0f;
      weights[EYE_EXPRESSION_SURPRISED] += 5.0f;
      weights[EYE_EXPRESSION_AWE] += 5.0f;
      weights[EYE_EXPRESSION_NORMAL] += 4.0f;
      weights[EYE_EXPRESSION_DETERMINED] += 3.0f;
      weights[EYE_EXPRESSION_SUSPICIOUS] += 3.0f;
      weights[EYE_EXPRESSION_FOCUSED] += 2.0f;
      break;
    case TIME_NIGHT:
      weights[EYE_EXPRESSION_SLEEPY] += 8.0f;
      weights[EYE_EXPRESSION_NORMAL] += 6.0f;
      weights[EYE_EXPRESSION_SQUINT] += 5.0f;
      weights[EYE_EXPRESSION_UNIMPRESSED] += 5.0f;
      weights[EYE_EXPRESSION_WORRIED] += 4.0f;
      weights[EYE_EXPRESSION_SAD] += 4.0f;
      weights[EYE_EXPRESSION_HAPPY] += 2.0f;
      weights[EYE_EXPRESSION_AWE] += 2.0f;
      break;
  }
}

static void applyTransitionWeights(EyeExpressionId currentExpression, float weights[EYE_EXPRESSION_COUNT]) {
  const WeightedExpression* transitions = nullptr;
  int transitionCount = 0;

  static const WeightedExpression kNormal[] = {
    {EYE_EXPRESSION_NORMAL, 20.0f},
    {EYE_EXPRESSION_HAPPY, 20.0f},
    {EYE_EXPRESSION_FOCUSED, 20.0f},
    {EYE_EXPRESSION_SURPRISED, 15.0f},
    {EYE_EXPRESSION_SKEPTIC, 15.0f},
    {EYE_EXPRESSION_DETERMINED, 10.0f}
  };
  static const WeightedExpression kHappy[] = {
    {EYE_EXPRESSION_HAPPY, 20.0f},
    {EYE_EXPRESSION_GLEE, 25.0f},
    {EYE_EXPRESSION_EXCITED, 20.0f},
    {EYE_EXPRESSION_NORMAL, 15.0f},
    {EYE_EXPRESSION_SURPRISED, 10.0f}
  };
  static const WeightedExpression kAngry[] = {
    {EYE_EXPRESSION_ANGRY, 20.0f},
    {EYE_EXPRESSION_FURIOUS, 25.0f},
    {EYE_EXPRESSION_ANNOYED, 20.0f},
    {EYE_EXPRESSION_FRUSTRATED, 20.0f},
    {EYE_EXPRESSION_NORMAL, 10.0f}
  };
  static const WeightedExpression kSleepy[] = {
    {EYE_EXPRESSION_SLEEPY, 30.0f},
    {EYE_EXPRESSION_SQUINT, 25.0f},
    {EYE_EXPRESSION_UNIMPRESSED, 20.0f},
    {EYE_EXPRESSION_WORRIED, 15.0f},
    {EYE_EXPRESSION_NORMAL, 10.0f}
  };
  static const WeightedExpression kFocused[] = {
    {EYE_EXPRESSION_FOCUSED, 25.0f},
    {EYE_EXPRESSION_DETERMINED, 25.0f},
    {EYE_EXPRESSION_NORMAL, 20.0f},
    {EYE_EXPRESSION_SKEPTIC, 15.0f},
    {EYE_EXPRESSION_ANNOYED, 10.0f}
  };
  static const WeightedExpression kSad[] = {
    {EYE_EXPRESSION_SAD, 25.0f},
    {EYE_EXPRESSION_WORRIED, 25.0f},
    {EYE_EXPRESSION_FRUSTRATED, 20.0f},
    {EYE_EXPRESSION_NORMAL, 15.0f},
    {EYE_EXPRESSION_UNIMPRESSED, 10.0f}
  };
  static const WeightedExpression kExcited[] = {
    {EYE_EXPRESSION_EXCITED, 20.0f},
    {EYE_EXPRESSION_HAPPY, 25.0f},
    {EYE_EXPRESSION_SURPRISED, 20.0f},
    {EYE_EXPRESSION_GLEE, 20.0f},
    {EYE_EXPRESSION_NORMAL, 10.0f}
  };
  static const WeightedExpression kSurprised[] = {
    {EYE_EXPRESSION_SURPRISED, 15.0f},
    {EYE_EXPRESSION_AWE, 25.0f},
    {EYE_EXPRESSION_SCARED, 20.0f},
    {EYE_EXPRESSION_EXCITED, 15.0f},
    {EYE_EXPRESSION_NORMAL, 15.0f},
    {EYE_EXPRESSION_HAPPY, 10.0f}
  };
  switch (currentExpression) {
    case EYE_EXPRESSION_NORMAL:
      transitions = kNormal;
      transitionCount = sizeof(kNormal) / sizeof(kNormal[0]);
      break;
    case EYE_EXPRESSION_HAPPY:
      transitions = kHappy;
      transitionCount = sizeof(kHappy) / sizeof(kHappy[0]);
      break;
    case EYE_EXPRESSION_ANGRY:
      transitions = kAngry;
      transitionCount = sizeof(kAngry) / sizeof(kAngry[0]);
      break;
    case EYE_EXPRESSION_SLEEPY:
      transitions = kSleepy;
      transitionCount = sizeof(kSleepy) / sizeof(kSleepy[0]);
      break;
    case EYE_EXPRESSION_FOCUSED:
      transitions = kFocused;
      transitionCount = sizeof(kFocused) / sizeof(kFocused[0]);
      break;
    case EYE_EXPRESSION_SAD:
      transitions = kSad;
      transitionCount = sizeof(kSad) / sizeof(kSad[0]);
      break;
    case EYE_EXPRESSION_EXCITED:
      transitions = kExcited;
      transitionCount = sizeof(kExcited) / sizeof(kExcited[0]);
      break;
    case EYE_EXPRESSION_SURPRISED:
      transitions = kSurprised;
      transitionCount = sizeof(kSurprised) / sizeof(kSurprised[0]);
      break;
    default:
      break;
  }

  for (int i = 0; i < transitionCount; i++) {
    int index = (int)transitions[i].expression;
    float timeWeight = weights[index];
    weights[index] = timeWeight * 0.6f + transitions[i].weight * 0.4f;
  }
}

static void pushHistory(EyeExpressionId expression) {
  if (s_state.historyCount < 5) {
    s_state.history[s_state.historyCount] = expression;
    s_state.historyCount++;
    return;
  }

  for (int i = 0; i < 4; i++) {
    s_state.history[i] = s_state.history[i + 1];
  }
  s_state.history[4] = expression;
}

static EyeExpressionId weightedRandomChoice(const float weights[EYE_EXPRESSION_COUNT], EyeExpressionId fallbackExpression) {
  float totalWeight = 0.0f;
  for (int i = 0; i < EYE_EXPRESSION_COUNT; i++) {
    totalWeight += weights[i];
  }

  if (totalWeight <= 0.0f) {
    return fallbackExpression;
  }

  long roll = random(0, 10000);
  float target = ((float)roll / 10000.0f) * totalWeight;

  for (int i = 0; i < EYE_EXPRESSION_COUNT; i++) {
    target -= weights[i];
    if (target <= 0.0f) {
      return (EyeExpressionId)i;
    }
  }

  return fallbackExpression;
}

static EyeExpressionId chooseNextExpression() {
  TimeOfDay timeOfDay = getCurrentTimeOfDay();
  float weights[EYE_EXPRESSION_COUNT];
  fillTimeWeights(timeOfDay, weights);
  applyTransitionWeights(s_state.expression, weights);

  weights[(int)s_state.expression] *= 0.12f;

  int recentStart = s_state.historyCount > 4 ? (s_state.historyCount - 4) : 0;
  for (int i = recentStart; i < s_state.historyCount; i++) {
    float penalty = 0.05f + (float)(i - recentStart) * 0.03f;
    weights[(int)s_state.history[i]] *= penalty;
  }

  return weightedRandomChoice(weights, s_state.expression);
}

static void scheduleNextBlink(const EyesConfig& config) {
  s_state.nextBlinkAfterMs = jitteredInterval(config.behavior.blinkIntervalMs, 72, 138);
}

static void scheduleNextLook(const EyesConfig& config) {
  s_state.nextLookAfterMs = jitteredInterval(config.behavior.lookIntervalMs, 45, 100);
}

static void startBlink(unsigned long now) {
  s_state.blinkActive = true;
  s_state.blinkStartAt = now;
  s_state.leftBlinkDelayMs = 0;
  s_state.rightBlinkDelayMs = 0;
  s_state.leftBlinkScale = 1.0f;
  s_state.rightBlinkScale = 1.0f;

  long skewRoll = random(0, 100);
  if (skewRoll < 42) {
    int skew = random(-18, 19);
    if (skew > 0) {
      s_state.rightBlinkDelayMs = (uint16_t)skew;
    } else if (skew < 0) {
      s_state.leftBlinkDelayMs = (uint16_t)(-skew);
    }
  }

  long scaleRoll = random(0, 100);
  if (scaleRoll < 35) {
    float baseScale = randomFloat(0.93f, 1.0f);
    s_state.leftBlinkScale = baseScale;
    s_state.rightBlinkScale = baseScale;

    if (random(0, 100) < 30) {
      float driftScale = randomFloat(0.96f, 1.0f);
      if (random(0, 100) < 50) {
        s_state.leftBlinkScale *= driftScale;
      } else {
        s_state.rightBlinkScale *= driftScale;
      }
    }
  }
}

static void startExpressionTransition(EyeExpressionId nextExpression, unsigned long now) {
  s_state.pendingExpression = nextExpression;
  s_state.transitionActive = true;
  s_state.transitionApplied = false;
  startBlink(now);
}

static void resetState() {
  unsigned long now = millis();
  s_lastEyesRenderAt = 0;
  s_state.lookX = 0.0f;
  s_state.lookY = 0.0f;
  s_state.targetLookX = 0.0f;
  s_state.targetLookY = 0.0f;
  s_state.leftLookOffsetX = 0.0f;
  s_state.leftLookOffsetY = 0.0f;
  s_state.rightLookOffsetX = 0.0f;
  s_state.rightLookOffsetY = 0.0f;
  s_state.targetLeftLookOffsetX = 0.0f;
  s_state.targetLeftLookOffsetY = 0.0f;
  s_state.targetRightLookOffsetX = 0.0f;
  s_state.targetRightLookOffsetY = 0.0f;
  s_state.lastBlinkAt = now;
  s_state.nextBlinkAfterMs = 0;
  s_state.blinkStartAt = 0;
  s_state.blinkActive = false;
  s_state.transitionActive = false;
  s_state.transitionApplied = false;
  s_state.leftBlinkDelayMs = 0;
  s_state.rightBlinkDelayMs = 0;
  s_state.leftBlinkScale = 1.0f;
  s_state.rightBlinkScale = 1.0f;
  s_state.lastLookAt = now;
  s_state.nextLookAfterMs = 0;
  s_state.lastExpressionAt = now;
  s_state.actionExpireAt = 0;
  s_state.lastInteractionAt = now;
  s_state.expression = EYE_EXPRESSION_NORMAL;
  s_state.pendingExpression = EYE_EXPRESSION_NORMAL;
  s_state.historyCount = 0;
  pushHistory(EYE_EXPRESSION_NORMAL);
}

static bool pointInRectangle(float px, float py, float x0, float y0, float x1, float y1) {
  float left = fminf(x0, x1);
  float right = fmaxf(x0, x1);
  float top = fminf(y0, y1);
  float bottom = fmaxf(y0, y1);
  return px >= left && px <= right && py >= top && py <= bottom;
}

static float triangleSign(float px, float py, float ax, float ay, float bx, float by) {
  return (px - bx) * (ay - by) - (ax - bx) * (py - by);
}

static bool pointInTriangle(float px, float py, float ax, float ay, float bx, float by, float cx, float cy) {
  bool b1 = triangleSign(px, py, ax, ay, bx, by) < 0.0f;
  bool b2 = triangleSign(px, py, bx, by, cx, cy) < 0.0f;
  bool b3 = triangleSign(px, py, cx, cy, ax, ay) < 0.0f;
  return (b1 == b2) && (b2 == b3);
}

static bool pointInQuarterEllipse(
  float px,
  float py,
  float cx,
  float cy,
  float rx,
  float ry,
  bool leftSide,
  bool topSide
) {
  if (rx <= 0.0f || ry <= 0.0f) {
    return false;
  }

  if (leftSide && px > cx) {
    return false;
  }
  if (!leftSide && px < cx) {
    return false;
  }
  if (topSide && py > cy) {
    return false;
  }
  if (!topSide && py < cy) {
    return false;
  }

  float dx = (px - cx) / rx;
  float dy = (py - cy) / ry;
  return dx * dx + dy * dy <= 1.0f;
}

static int roundToInt(float value) {
  return (int)lroundf(value);
}

static void drawFilledRect(
  MatrixPanel_I2S_DMA* display,
  int x0,
  int y0,
  int x1,
  int y1,
  uint16_t color
) {
  int startX = clampInt(x0 < x1 ? x0 : x1, 0, DisplayManager::PANEL_RES_X - 1);
  int endX = clampInt(x0 > x1 ? x0 : x1, 0, DisplayManager::PANEL_RES_X - 1);
  int startY = clampInt(y0 < y1 ? y0 : y1, 0, DisplayManager::PANEL_RES_Y - 1);
  int endY = clampInt(y0 > y1 ? y0 : y1, 0, DisplayManager::PANEL_RES_Y - 1);
  for (int y = startY; y <= endY; y++) {
    for (int x = startX; x <= endX; x++) {
      display->drawPixel(x, y, color);
    }
  }
}

static void drawFilledCircle(
  MatrixPanel_I2S_DMA* display,
  float centerX,
  float centerY,
  float radius,
  uint16_t color
) {
  if (radius <= 0.0f) {
    return;
  }
  int startX = clampInt((int)floorf(centerX - radius), 0, DisplayManager::PANEL_RES_X - 1);
  int endX = clampInt((int)ceilf(centerX + radius), 0, DisplayManager::PANEL_RES_X - 1);
  int startY = clampInt((int)floorf(centerY - radius), 0, DisplayManager::PANEL_RES_Y - 1);
  int endY = clampInt((int)ceilf(centerY + radius), 0, DisplayManager::PANEL_RES_Y - 1);
  float radiusSq = radius * radius;
  for (int y = startY; y <= endY; y++) {
    for (int x = startX; x <= endX; x++) {
      float dx = ((float)x + 0.5f) - centerX;
      float dy = ((float)y + 0.5f) - centerY;
      if (dx * dx + dy * dy <= radiusSq) {
        display->drawPixel(x, y, color);
      }
    }
  }
}

static void drawFilledEllipse(
  MatrixPanel_I2S_DMA* display,
  float centerX,
  float centerY,
  float radiusX,
  float radiusY,
  uint16_t color
) {
  if (radiusX <= 0.0f || radiusY <= 0.0f) {
    return;
  }
  int startX = clampInt((int)floorf(centerX - radiusX), 0, DisplayManager::PANEL_RES_X - 1);
  int endX = clampInt((int)ceilf(centerX + radiusX), 0, DisplayManager::PANEL_RES_X - 1);
  int startY = clampInt((int)floorf(centerY - radiusY), 0, DisplayManager::PANEL_RES_Y - 1);
  int endY = clampInt((int)ceilf(centerY + radiusY), 0, DisplayManager::PANEL_RES_Y - 1);
  for (int y = startY; y <= endY; y++) {
    for (int x = startX; x <= endX; x++) {
      float nx = (((float)x + 0.5f) - centerX) / radiusX;
      float ny = (((float)y + 0.5f) - centerY) / radiusY;
      if (nx * nx + ny * ny <= 1.0f) {
        display->drawPixel(x, y, color);
      }
    }
  }
}

static void drawEllipseOutline(
  MatrixPanel_I2S_DMA* display,
  float centerX,
  float centerY,
  float radiusX,
  float radiusY,
  float thickness,
  uint16_t color
) {
  if (radiusX <= 0.0f || radiusY <= 0.0f) {
    return;
  }
  float innerRadiusX = radiusX - thickness;
  float innerRadiusY = radiusY - thickness;
  if (innerRadiusX < 0.1f) innerRadiusX = 0.1f;
  if (innerRadiusY < 0.1f) innerRadiusY = 0.1f;

  int startX = clampInt((int)floorf(centerX - radiusX), 0, DisplayManager::PANEL_RES_X - 1);
  int endX = clampInt((int)ceilf(centerX + radiusX), 0, DisplayManager::PANEL_RES_X - 1);
  int startY = clampInt((int)floorf(centerY - radiusY), 0, DisplayManager::PANEL_RES_Y - 1);
  int endY = clampInt((int)ceilf(centerY + radiusY), 0, DisplayManager::PANEL_RES_Y - 1);
  for (int y = startY; y <= endY; y++) {
    for (int x = startX; x <= endX; x++) {
      float ox = (((float)x + 0.5f) - centerX) / radiusX;
      float oy = (((float)y + 0.5f) - centerY) / radiusY;
      if (ox * ox + oy * oy > 1.0f) {
        continue;
      }
      float ix = (((float)x + 0.5f) - centerX) / innerRadiusX;
      float iy = (((float)y + 0.5f) - centerY) / innerRadiusY;
      if (ix * ix + iy * iy > 1.0f) {
        display->drawPixel(x, y, color);
      }
    }
  }
}

static void drawFilledTriangle(
  MatrixPanel_I2S_DMA* display,
  float ax,
  float ay,
  float bx,
  float by,
  float cx,
  float cy,
  uint16_t color
) {
  int startX = clampInt((int)floorf(fminf(ax, fminf(bx, cx))), 0, DisplayManager::PANEL_RES_X - 1);
  int endX = clampInt((int)ceilf(fmaxf(ax, fmaxf(bx, cx))), 0, DisplayManager::PANEL_RES_X - 1);
  int startY = clampInt((int)floorf(fminf(ay, fminf(by, cy))), 0, DisplayManager::PANEL_RES_Y - 1);
  int endY = clampInt((int)ceilf(fmaxf(ay, fmaxf(by, cy))), 0, DisplayManager::PANEL_RES_Y - 1);
  for (int y = startY; y <= endY; y++) {
    for (int x = startX; x <= endX; x++) {
      if (pointInTriangle((float)x + 0.5f, (float)y + 0.5f, ax, ay, bx, by, cx, cy)) {
        display->drawPixel(x, y, color);
      }
    }
  }
}

static void drawFilledRoundedRect(
  MatrixPanel_I2S_DMA* display,
  int x0,
  int y0,
  int x1,
  int y1,
  int radius,
  uint16_t color
) {
  int left = x0 < x1 ? x0 : x1;
  int right = x0 > x1 ? x0 : x1;
  int top = y0 < y1 ? y0 : y1;
  int bottom = y0 > y1 ? y0 : y1;
  int halfWidth = (right - left + 1) / 2;
  int halfHeight = (bottom - top + 1) / 2;
  int maxCorner = halfWidth < halfHeight ? halfWidth : halfHeight;
  int corner = clampInt(radius, 0, maxCorner);

  drawFilledRect(display, left + corner, top, right - corner, bottom, color);
  drawFilledRect(display, left, top + corner, right, bottom - corner, color);
  drawFilledCircle(display, (float)(left + corner), (float)(top + corner), (float)corner, color);
  drawFilledCircle(display, (float)(right - corner), (float)(top + corner), (float)corner, color);
  drawFilledCircle(display, (float)(left + corner), (float)(bottom - corner), (float)corner, color);
  drawFilledCircle(display, (float)(right - corner), (float)(bottom - corner), (float)corner, color);
}

static PetPose buildPetPose(unsigned long now) {
  PetPose pose;
  float breathing = signedTrianglePulse(now, kPetBreathingPeriodMs, 0.0f);
  int breathingOffset = roundToInt(breathing * 1.0f);
  int lookOffsetX = roundToInt(clampFloat(s_state.lookX, -1.0f, 1.0f));
  int lookOffsetY = roundToInt(clampFloat(s_state.lookY, -1.0f, 1.0f));
  pose.headShiftX = lookOffsetX;
  pose.headShiftY = breathingOffset + lookOffsetY;
  pose.bodyShiftY = breathingOffset;
  pose.handPressY = s_state.blinkActive ? 1 : 0;
  pose.earShakeX = 0;
  if (s_state.blinkActive) {
    unsigned long elapsed = now - s_state.blinkStartAt;
    unsigned long phase = elapsed % kPetEarShakeWindowMs;
    pose.earShakeX = phase < (kPetEarShakeWindowMs / 2) ? 1 : -1;
  }
  float tailSwing = signedTrianglePulse(now, kPetTailSwingPeriodMs, 0.25f);
  pose.tailSwingX = roundToInt(tailSwing * 2.0f);
  return pose;
}

static void drawPetBodyLayer(
  MatrixPanel_I2S_DMA* display,
  const PetPose& pose,
  uint16_t baseColor,
  uint16_t shadowColor,
  uint16_t outlineColor
) {
  int bodyTop = kPetBodyMinY + pose.bodyShiftY;
  int bodyBottom = kPetBodyMaxY + pose.bodyShiftY;
  drawFilledRoundedRect(
    display,
    kPetBodyMinX,
    bodyTop,
    kPetBodyMaxX,
    bodyBottom,
    4,
    outlineColor
  );
  drawFilledRoundedRect(
    display,
    kPetBodyMinX + 1,
    bodyTop + 1,
    kPetBodyMaxX - 1,
    bodyBottom - 1,
    3,
    baseColor
  );
  drawFilledRoundedRect(
    display,
    kPetBodyMinX + 1,
    bodyTop + 10,
    kPetBodyMaxX - 1,
    bodyBottom - 1,
    3,
    shadowColor
  );

  int headLeft = kPetHeadMinX + pose.headShiftX;
  int headRight = kPetHeadMaxX + pose.headShiftX;
  int headTop = kPetHeadMinY + pose.headShiftY;
  int headBottom = kPetHeadMaxY + pose.headShiftY;
  float centerX = ((float)headLeft + (float)headRight) * 0.5f;
  float centerY = ((float)headTop + (float)headBottom) * 0.5f;
  float radiusX = ((float)(headRight - headLeft + 1)) * 0.5f;
  float radiusY = ((float)(headBottom - headTop + 1)) * 0.5f;
  drawFilledEllipse(display, centerX, centerY, radiusX, radiusY, baseColor);
  drawFilledEllipse(
    display,
    centerX,
    centerY + 5.0f,
    radiusX > 1.0f ? radiusX - 1.0f : radiusX,
    radiusY > 6.0f ? radiusY - 6.0f : 1.0f,
    shadowColor
  );
  drawEllipseOutline(display, centerX, centerY, radiusX, radiusY, 1.2f, outlineColor);
}

static void drawPetHandsLayer(
  MatrixPanel_I2S_DMA* display,
  const PetPose& pose,
  uint16_t pawColor,
  uint16_t shadowColor,
  uint16_t outlineColor
) {
  int leftTop = kPetLeftHandMinY + pose.bodyShiftY + pose.handPressY;
  int leftBottom = kPetLeftHandMaxY + pose.bodyShiftY + pose.handPressY;
  int rightTop = kPetRightHandMinY + pose.bodyShiftY + pose.handPressY;
  int rightBottom = kPetRightHandMaxY + pose.bodyShiftY + pose.handPressY;

  drawFilledRoundedRect(display, kPetLeftHandMinX, leftTop, kPetLeftHandMaxX, leftBottom, 4, outlineColor);
  drawFilledRoundedRect(display, kPetLeftHandMinX + 1, leftTop + 1, kPetLeftHandMaxX - 1, leftBottom - 1, 3, pawColor);
  drawFilledRoundedRect(display, kPetLeftHandMinX + 1, leftTop + 7, kPetLeftHandMaxX - 1, leftBottom - 1, 3, shadowColor);

  drawFilledRoundedRect(display, kPetRightHandMinX, rightTop, kPetRightHandMaxX, rightBottom, 4, outlineColor);
  drawFilledRoundedRect(display, kPetRightHandMinX + 1, rightTop + 1, kPetRightHandMaxX - 1, rightBottom - 1, 3, pawColor);
  drawFilledRoundedRect(display, kPetRightHandMinX + 1, rightTop + 7, kPetRightHandMaxX - 1, rightBottom - 1, 3, shadowColor);
}

static void drawPetEarsTailLayer(
  MatrixPanel_I2S_DMA* display,
  const PetPose& pose,
  uint16_t accentColor,
  uint16_t tailColor,
  uint16_t shadowColor,
  uint16_t outlineColor
) {
  int leftAnchorX = kPetLeftEarAnchorX + pose.headShiftX;
  int leftAnchorY = kPetLeftEarAnchorY + pose.headShiftY;
  int rightAnchorX = kPetRightEarAnchorX + pose.headShiftX;
  int rightAnchorY = kPetRightEarAnchorY + pose.headShiftY;

  drawFilledTriangle(
    display,
    (float)(leftAnchorX - 5 + pose.earShakeX),
    (float)(leftAnchorY + 9),
    (float)leftAnchorX,
    (float)(leftAnchorY - 3),
    (float)(leftAnchorX + 5 + pose.earShakeX),
    (float)(leftAnchorY + 9),
    outlineColor
  );
  drawFilledTriangle(
    display,
    (float)(leftAnchorX - 4 + pose.earShakeX),
    (float)(leftAnchorY + 8),
    (float)leftAnchorX,
    (float)(leftAnchorY - 2),
    (float)(leftAnchorX + 4 + pose.earShakeX),
    (float)(leftAnchorY + 8),
    accentColor
  );
  drawFilledTriangle(
    display,
    (float)(rightAnchorX - 5 - pose.earShakeX),
    (float)(rightAnchorY + 9),
    (float)rightAnchorX,
    (float)(rightAnchorY - 3),
    (float)(rightAnchorX + 5 - pose.earShakeX),
    (float)(rightAnchorY + 9),
    outlineColor
  );
  drawFilledTriangle(
    display,
    (float)(rightAnchorX - 4 - pose.earShakeX),
    (float)(rightAnchorY + 8),
    (float)rightAnchorX,
    (float)(rightAnchorY - 2),
    (float)(rightAnchorX + 4 - pose.earShakeX),
    (float)(rightAnchorY + 8),
    accentColor
  );

  int tailBaseX = kPetTailAnchorX + pose.tailSwingX;
  int tailBaseY = kPetTailAnchorY + pose.bodyShiftY;
  for (int i = 0; i <= 5; i++) {
    float centerX = (float)(tailBaseX + i);
    float centerY = (float)(tailBaseY - (i / 2));
    drawFilledCircle(display, centerX, centerY, 2.0f, outlineColor);
    drawFilledCircle(display, centerX, centerY, 1.0f, tailColor);
  }
  drawFilledCircle(display, (float)(tailBaseX + 6), (float)(tailBaseY - 3), 1.5f, accentColor);
  display->drawPixel(
    clampInt(kPetTailAnchorX - 1, 0, DisplayManager::PANEL_RES_X - 1),
    clampInt(tailBaseY + 1, 0, DisplayManager::PANEL_RES_Y - 1),
    shadowColor
  );
}

static void drawPetBody(MatrixPanel_I2S_DMA* display, unsigned long now) {
  PetPose pose = buildPetPose(now);
  uint16_t baseColor = display->color565(kPetColorBaseR, kPetColorBaseG, kPetColorBaseB);
  uint16_t shadowColor = display->color565(kPetColorShadowR, kPetColorShadowG, kPetColorShadowB);
  uint16_t pawColor = display->color565(kPetColorPawR, kPetColorPawG, kPetColorPawB);
  uint16_t accentColor = display->color565(kPetColorAccentR, kPetColorAccentG, kPetColorAccentB);
  uint16_t tailColor = display->color565(kPetColorTailR, kPetColorTailG, kPetColorTailB);
  uint16_t outlineColor = display->color565(kPetColorOutlineR, kPetColorOutlineG, kPetColorOutlineB);

  drawPetBodyLayer(display, pose, baseColor, shadowColor, outlineColor);
  drawPetHandsLayer(display, pose, pawColor, shadowColor, outlineColor);
  drawPetEarsTailLayer(display, pose, accentColor, tailColor, shadowColor, outlineColor);
}

static RasterEyeShape buildRasterEyeShape(
  float centerX,
  float centerY,
  const EyeShapePreset& preset,
  const EyeRenderTransform& renderTransform,
  const EyesConfig& config,
  float blinkAmount
) {
  float scaleX = (float)config.layout.eyeWidth / kReferenceEyeSize;
  float scaleY = (float)config.layout.eyeHeight / kReferenceEyeSize;
  float radiusScale = scaleX < scaleY ? scaleX : scaleY;

  RasterEyeShape shape;
  shape.centerX = centerX;
  shape.centerY = centerY;
  shape.offsetX = (preset.offsetX + renderTransform.moveX) * scaleX;
  shape.offsetY = (preset.offsetY - renderTransform.moveY) * scaleY;
  shape.width = preset.width * scaleX * renderTransform.scaleX;
  shape.height = preset.height * scaleY * renderTransform.scaleY;
  shape.slopeTop = preset.slopeTop;
  shape.slopeBottom = preset.slopeBottom;
  shape.radiusTop = preset.radiusTop * radiusScale;
  shape.radiusBottom = preset.radiusBottom * radiusScale;

  float blinkScale = 1.0f - blinkAmount;
  shape.height = clampFloat(shape.height * blinkScale, 1.0f, 64.0f);

  shape.deltaTop = (shape.height * shape.slopeTop) * 0.5f;
  shape.deltaBottom = (shape.height * shape.slopeBottom) * 0.5f;

  float totalHeight = shape.height + shape.deltaTop - shape.deltaBottom;
  if (totalHeight <= 1.0f) {
    shape.radiusTop = 0.0f;
    shape.radiusBottom = 0.0f;
  } else if (shape.radiusBottom > 0.0f &&
             shape.radiusTop > 0.0f &&
             totalHeight - 1.0f < shape.radiusBottom + shape.radiusTop) {
    float scaleDown = (totalHeight - 1.0f) / (shape.radiusBottom + shape.radiusTop);
    shape.radiusTop *= scaleDown;
    shape.radiusBottom *= scaleDown;
  }

  shape.TLcY = centerY + shape.offsetY - shape.height * 0.5f + shape.radiusTop - shape.deltaTop;
  shape.TLcX = centerX + shape.offsetX - shape.width * 0.5f + shape.radiusTop;
  shape.TRcY = centerY + shape.offsetY - shape.height * 0.5f + shape.radiusTop + shape.deltaTop;
  shape.TRcX = centerX + shape.offsetX + shape.width * 0.5f - shape.radiusTop;
  shape.BLcY = centerY + shape.offsetY + shape.height * 0.5f - shape.radiusBottom - shape.deltaBottom;
  shape.BLcX = centerX + shape.offsetX - shape.width * 0.5f + shape.radiusBottom;
  shape.BRcY = centerY + shape.offsetY + shape.height * 0.5f - shape.radiusBottom + shape.deltaBottom;
  shape.BRcX = centerX + shape.offsetX + shape.width * 0.5f - shape.radiusBottom;

  shape.minCX = fminf(shape.TLcX, shape.BLcX);
  shape.maxCX = fmaxf(shape.TRcX, shape.BRcX);
  shape.minCY = fminf(shape.TLcY, shape.TRcY);
  shape.maxCY = fmaxf(shape.BLcY, shape.BRcY);

  shape.minX = centerX + shape.offsetX - shape.width * 0.5f;
  shape.maxX = centerX + shape.offsetX + shape.width * 0.5f;
  shape.minY = fminf(shape.TLcY - shape.radiusTop, shape.TRcY - shape.radiusTop);
  shape.maxY = fmaxf(shape.BLcY + shape.radiusBottom, shape.BRcY + shape.radiusBottom);

  return shape;
}

static bool containsShapePoint(const RasterEyeShape& shape, float sampleX, float sampleY) {
  if (pointInRectangle(sampleX, sampleY, shape.minCX, shape.minCY, shape.maxCX, shape.maxCY)) {
    return true;
  }

  if (pointInRectangle(sampleX, sampleY, shape.TRcX, shape.TRcY, shape.BRcX + shape.radiusBottom, shape.BRcY)) {
    return true;
  }

  if (pointInRectangle(sampleX, sampleY, shape.TLcX - shape.radiusTop, shape.TLcY, shape.BLcX, shape.BLcY)) {
    return true;
  }

  if (pointInRectangle(sampleX, sampleY, shape.TLcX, shape.TLcY - shape.radiusTop, shape.TRcX, shape.TRcY)) {
    return true;
  }

  if (pointInRectangle(sampleX, sampleY, shape.BLcX, shape.BLcY, shape.BRcX, shape.BRcY + shape.radiusBottom)) {
    return true;
  }

  if (shape.slopeTop > 0.0f &&
      pointInTriangle(
        sampleX,
        sampleY,
        shape.TLcX,
        shape.TLcY - shape.radiusTop,
        shape.TRcX,
        shape.TRcY - shape.radiusTop,
        shape.TRcX,
        shape.TLcY - shape.radiusTop
      )) {
    return true;
  }

  if (shape.slopeTop < 0.0f &&
      pointInTriangle(
        sampleX,
        sampleY,
        shape.TRcX,
        shape.TRcY - shape.radiusTop,
        shape.TLcX,
        shape.TLcY - shape.radiusTop,
        shape.TLcX,
        shape.TRcY - shape.radiusTop
      )) {
    return true;
  }

  if (shape.slopeBottom > 0.0f &&
      pointInTriangle(
        sampleX,
        sampleY,
        shape.BRcX + shape.radiusBottom,
        shape.BRcY + shape.radiusBottom,
        shape.BLcX - shape.radiusBottom,
        shape.BLcY + shape.radiusBottom,
        shape.BLcX - shape.radiusBottom,
        shape.BRcY + shape.radiusBottom
      )) {
    return true;
  }

  if (shape.slopeBottom < 0.0f &&
      pointInTriangle(
        sampleX,
        sampleY,
        shape.BLcX - shape.radiusBottom,
        shape.BLcY + shape.radiusBottom,
        shape.BRcX + shape.radiusBottom,
        shape.BRcY + shape.radiusBottom,
        shape.BRcX + shape.radiusBottom,
        shape.BLcY + shape.radiusBottom
      )) {
    return true;
  }

  if (shape.radiusTop > 0.0f &&
      pointInQuarterEllipse(sampleX, sampleY, shape.TLcX, shape.TLcY, shape.radiusTop, shape.radiusTop, true, true)) {
    return true;
  }

  if (shape.radiusTop > 0.0f &&
      pointInQuarterEllipse(sampleX, sampleY, shape.TRcX, shape.TRcY, shape.radiusTop, shape.radiusTop, false, true)) {
    return true;
  }

  if (shape.radiusBottom > 0.0f &&
      pointInQuarterEllipse(
        sampleX,
        sampleY,
        shape.BLcX,
        shape.BLcY,
        shape.radiusBottom,
        shape.radiusBottom,
        true,
        false
      )) {
    return true;
  }

  if (shape.radiusBottom > 0.0f &&
      pointInQuarterEllipse(
        sampleX,
        sampleY,
        shape.BRcX,
        shape.BRcY,
        shape.radiusBottom,
        shape.radiusBottom,
        false,
        false
      )) {
    return true;
  }

  return false;
}

static void drawEye(
  MatrixPanel_I2S_DMA* display,
  float centerX,
  float centerY,
  const EyesConfig& config,
  EyeExpressionId expression,
  const EyeShapePreset& preset,
  const EyeRenderTransform& renderTransform,
  bool mirror,
  float blinkAmount,
  uint8_t eyeR,
  uint8_t eyeG,
  uint8_t eyeB
) {
  (void)expression;
  RasterEyeShape shape = buildRasterEyeShape(centerX, centerY, preset, renderTransform, config, blinkAmount);
  float physicalMinX = mirror ? (centerX * 2.0f - shape.maxX) : shape.minX;
  float physicalMaxX = mirror ? (centerX * 2.0f - shape.minX) : shape.maxX;
  int startX = clampInt((int)floorf(physicalMinX) - 1, 0, DisplayManager::PANEL_RES_X - 1);
  int endX = clampInt((int)ceilf(physicalMaxX) + 1, 0, DisplayManager::PANEL_RES_X - 1);
  int startY = clampInt((int)floorf(shape.minY) - 1, 0, DisplayManager::PANEL_RES_Y - 1);
  int endY = clampInt((int)ceilf(shape.maxY) + 1, 0, DisplayManager::PANEL_RES_Y - 1);

  for (int y = startY; y <= endY; y++) {
    for (int x = startX; x <= endX; x++) {
      float sampleX = (float)x + 0.5f;
      float sampleY = (float)y + 0.5f;
      if (mirror) {
        sampleX = centerX * 2.0f - sampleX;
      }

      if (containsShapePoint(shape, sampleX, sampleY)) {
        display->drawPixel(x, y, display->color565(eyeR, eyeG, eyeB));
      }
    }
  }
}

static float blinkAmountForEye(bool isLeftEye) {
  if (!s_state.blinkActive) {
    return 0.0f;
  }

  unsigned long now = millis();
  uint16_t delay = isLeftEye ? s_state.leftBlinkDelayMs : s_state.rightBlinkDelayMs;
  float scale = isLeftEye ? s_state.leftBlinkScale : s_state.rightBlinkScale;

  if (now < s_state.blinkStartAt + delay) {
    return 0.0f;
  }

  float phase = (float)(now - s_state.blinkStartAt - delay) / (float)kBlinkDurationMs;
  if (phase < 0.5f) {
    return clampFloat(phase * 2.0f * scale, 0.0f, 1.0f);
  }
  if (phase < 1.0f) {
    return clampFloat((1.0f - (phase - 0.5f) * 2.0f) * scale, 0.0f, 1.0f);
  }
  return 0.0f;
}

static void updateBlink(const EyesConfig& config, unsigned long now) {
  if (s_state.blinkActive) {
    float phase = (float)(now - s_state.blinkStartAt) / (float)kBlinkDurationMs;

    if (s_state.transitionActive && !s_state.transitionApplied && phase >= 0.5f) {
      s_state.expression = s_state.pendingExpression;
      s_state.transitionApplied = true;
      s_state.lastExpressionAt = now;
      pushHistory(s_state.expression);
    }

    unsigned long totalBlinkMs = kBlinkDurationMs + (s_state.leftBlinkDelayMs > s_state.rightBlinkDelayMs
      ? s_state.leftBlinkDelayMs
      : s_state.rightBlinkDelayMs);
    if (now - s_state.blinkStartAt >= totalBlinkMs) {
      s_state.blinkActive = false;
      s_state.lastBlinkAt = now;
      scheduleNextBlink(config);
      s_state.transitionActive = false;
      s_state.transitionApplied = false;
    }
    return;
  }

  if (now - s_state.lastBlinkAt >= s_state.nextBlinkAfterMs) {
    startBlink(now);
  }
}

static void drawTimeOverlay(MatrixPanel_I2S_DMA* display, const EyesConfig& config) {
  if (!config.time.show) {
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  char buffer[16];
  if (config.time.showSeconds) {
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    snprintf(buffer, sizeof(buffer), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  if (!parseHexColor(config.style.timeColor, r, g, b)) {
    return;
  }

  uint8_t fontId = config.time.font;
  if (fontId > CLOCK_FONT_RETRO_5X7) {
    fontId = CLOCK_FONT_MINIMAL_3X5;
  }

  int fontSize = config.time.fontSize;
  if (fontSize < 1) {
    fontSize = 1;
  }
  if (fontSize > 3) {
    fontSize = 3;
  }

  uint16_t color = display->color565(r, g, b);
  int width = getClockTextWidth(buffer, fontId, fontSize);
  int height = getClockTextHeight(fontId, fontSize);
  int maxX = DisplayManager::PANEL_RES_X - width;
  if (maxX < 0) {
    maxX = 0;
  }
  int maxY = DisplayManager::PANEL_RES_Y - height;
  if (maxY < 0) {
    maxY = 0;
  }
  int x = config.layout.timeX;
  if (x < 0) {
    x = 0;
  }
  if (x > maxX) {
    x = maxX;
  }
  int y = config.layout.timeY;
  if (y < 0) {
    y = 0;
  }
  if (y > maxY) {
    y = maxY;
  }
  drawClockText(
    display,
    buffer,
    x,
    y,
    color,
    fontId,
    fontSize
  );
}

static void chooseIdleLook() {
  float amplitudeScale = 1.0f;
  if (s_state.expression == EYE_EXPRESSION_SLEEPY || s_state.expression == EYE_EXPRESSION_SQUINT) {
    amplitudeScale = 0.72f;
  } else if (s_state.expression == EYE_EXPRESSION_EXCITED || s_state.expression == EYE_EXPRESSION_SURPRISED) {
    amplitudeScale = 1.08f;
  }

  long roll = random(0, 100);
  float baseX = 0.0f;
  float baseY = 0.0f;
  if (roll < 10) {
    baseX = randomFloat(-0.18f, 0.18f);
    baseY = randomFloat(-0.10f, 0.10f);
  } else if (roll < 42) {
    float side = random(0, 100) < 50 ? -1.0f : 1.0f;
    baseX = side * randomFloat(0.42f, 0.80f);
    baseY = randomFloat(-0.18f, 0.20f);
  } else if (roll < 66) {
    baseX = randomFloat(-0.36f, 0.36f);
    baseY = randomFloat(-0.62f, -0.22f);
  } else if (roll < 78) {
    baseX = randomFloat(-0.28f, 0.28f);
    baseY = randomFloat(0.12f, 0.34f);
  } else if (roll < 92) {
    float side = random(0, 100) < 50 ? -1.0f : 1.0f;
    baseX = side * randomFloat(0.32f, 0.64f);
    baseY = random(0, 100) < 50 ? randomFloat(-0.42f, -0.14f) : randomFloat(0.08f, 0.24f);
  } else {
    float side = random(0, 100) < 50 ? -1.0f : 1.0f;
    baseX = side * randomFloat(0.18f, 0.30f);
    baseY = randomFloat(-0.08f, 0.08f);
  }

  s_state.targetLookX = clampFloat(baseX * amplitudeScale, -0.90f, 0.90f);
  s_state.targetLookY = clampFloat(baseY * amplitudeScale, -0.82f, 0.60f);

  s_state.targetLeftLookOffsetX = 0.0f;
  s_state.targetLeftLookOffsetY = 0.0f;
  s_state.targetRightLookOffsetX = 0.0f;
  s_state.targetRightLookOffsetY = 0.0f;

  int asymmetryChance = 30;
  if (s_state.expression == EYE_EXPRESSION_SKEPTIC ||
      s_state.expression == EYE_EXPRESSION_WORRIED) {
    asymmetryChance = 42;
  }

  if (random(0, 100) >= asymmetryChance) {
    return;
  }

  float microX = randomFloat(-0.10f, 0.10f);
  float microY = randomFloat(-0.05f, 0.05f);
  float lagFactor = randomFloat(0.28f, 0.58f);
  bool leftLead = random(0, 100) < 50;

  if (leftLead) {
    s_state.targetLeftLookOffsetX = microX;
    s_state.targetLeftLookOffsetY = microY;
    s_state.targetRightLookOffsetX = microX * lagFactor;
    s_state.targetRightLookOffsetY = microY * lagFactor;
  } else {
    s_state.targetLeftLookOffsetX = microX * lagFactor;
    s_state.targetLeftLookOffsetY = microY * lagFactor;
    s_state.targetRightLookOffsetX = microX;
    s_state.targetRightLookOffsetY = microY;
  }

  if (random(0, 100) < 24) {
    float tiltY = randomFloat(-0.06f, 0.06f);
    if (leftLead) {
      s_state.targetLeftLookOffsetY += tiltY;
      s_state.targetRightLookOffsetY -= tiltY * 0.35f;
    } else {
      s_state.targetLeftLookOffsetY -= tiltY * 0.35f;
      s_state.targetRightLookOffsetY += tiltY;
    }
  }
}

static void updateRuntime(const EyesConfig& config) {
  unsigned long now = millis();
  if (s_state.nextBlinkAfterMs == 0) {
    scheduleNextBlink(config);
  }
  if (s_state.nextLookAfterMs == 0) {
    scheduleNextLook(config);
  }

  updateBlink(config, now);

  if (s_state.actionExpireAt > 0 && now >= s_state.actionExpireAt) {
    s_state.actionExpireAt = 0;
    s_state.targetLookX = 0.0f;
    s_state.targetLookY = 0.0f;
    s_state.targetLeftLookOffsetX = 0.0f;
    s_state.targetLeftLookOffsetY = 0.0f;
    s_state.targetRightLookOffsetX = 0.0f;
    s_state.targetRightLookOffsetY = 0.0f;
  }

  if (config.behavior.autoSwitch && s_state.actionExpireAt == 0 && !s_state.transitionActive) {
    if (now - s_state.lastInteractionAt >= config.behavior.sleepyAfterMs) {
      if (s_state.expression != EYE_EXPRESSION_SLEEPY) {
        startExpressionTransition(EYE_EXPRESSION_SLEEPY, now);
      }
    } else if (now - s_state.lastExpressionAt >= minExpressionInterval(getCurrentTimeOfDay())) {
      EyeExpressionId nextExpression = chooseNextExpression();
      if (nextExpression != s_state.expression) {
        startExpressionTransition(nextExpression, now);
      } else {
        s_state.lastExpressionAt = now;
      }
    }
  }

  if (now - s_state.lastLookAt >= s_state.nextLookAfterMs) {
    s_state.lastLookAt = now;
    if (s_state.actionExpireAt == 0) {
      chooseIdleLook();
      scheduleNextLook(config);
    }
  }

  float lookEase = s_state.actionExpireAt > 0 ? 0.24f : 0.14f;
  s_state.lookX += (s_state.targetLookX - s_state.lookX) * lookEase;
  s_state.lookY += (s_state.targetLookY - s_state.lookY) * lookEase;
  s_state.leftLookOffsetX += (s_state.targetLeftLookOffsetX - s_state.leftLookOffsetX) * 0.18f;
  s_state.leftLookOffsetY += (s_state.targetLeftLookOffsetY - s_state.leftLookOffsetY) * 0.18f;
  s_state.rightLookOffsetX += (s_state.targetRightLookOffsetX - s_state.rightLookOffsetX) * 0.18f;
  s_state.rightLookOffsetY += (s_state.targetRightLookOffsetY - s_state.rightLookOffsetY) * 0.18f;
}

}  // namespace

namespace EyesEffect {

void init() {
  resetState();
}

void applyConfig(const EyesConfig& config) {
  (void)config;
  resetState();
}

bool triggerAction(const char* action) {
  if (action == nullptr) {
    return false;
  }

  unsigned long now = millis();
  s_state.lastInteractionAt = now;

  if (strcmp(action, "blink") == 0) {
    startBlink(now);
    return true;
  }

  if (strcmp(action, "look_left") == 0) {
    s_state.targetLookX = -1.0f;
    s_state.targetLookY = 0.0f;
    s_state.targetLeftLookOffsetX = 0.0f;
    s_state.targetLeftLookOffsetY = 0.0f;
    s_state.targetRightLookOffsetX = 0.0f;
    s_state.targetRightLookOffsetY = 0.0f;
    s_state.lastLookAt = now;
    scheduleNextLook(ConfigManager::eyesConfig);
    s_state.actionExpireAt = now + ConfigManager::eyesConfig.interaction.lookHoldMs;
    return true;
  }

  if (strcmp(action, "look_right") == 0) {
    s_state.targetLookX = 1.0f;
    s_state.targetLookY = 0.0f;
    s_state.targetLeftLookOffsetX = 0.0f;
    s_state.targetLeftLookOffsetY = 0.0f;
    s_state.targetRightLookOffsetX = 0.0f;
    s_state.targetRightLookOffsetY = 0.0f;
    s_state.lastLookAt = now;
    scheduleNextLook(ConfigManager::eyesConfig);
    s_state.actionExpireAt = now + ConfigManager::eyesConfig.interaction.lookHoldMs;
    return true;
  }

  if (strcmp(action, "look_center") == 0) {
    s_state.targetLookX = 0.0f;
    s_state.targetLookY = 0.0f;
    s_state.targetLeftLookOffsetX = 0.0f;
    s_state.targetLeftLookOffsetY = 0.0f;
    s_state.targetRightLookOffsetX = 0.0f;
    s_state.targetRightLookOffsetY = 0.0f;
    s_state.lastLookAt = now;
    scheduleNextLook(ConfigManager::eyesConfig);
    s_state.actionExpireAt = now + ConfigManager::eyesConfig.interaction.lookHoldMs;
    return true;
  }

  if (strcmp(action, "happy") == 0) {
    startExpressionTransition(EYE_EXPRESSION_HAPPY, now);
    s_state.actionExpireAt = now + ConfigManager::eyesConfig.interaction.moodHoldMs;
    return true;
  }

  if (strcmp(action, "sleepy") == 0) {
    startExpressionTransition(EYE_EXPRESSION_SLEEPY, now);
    s_state.actionExpireAt = now + ConfigManager::eyesConfig.interaction.moodHoldMs;
    return true;
  }

  EyeExpressionId selectedExpression = EYE_EXPRESSION_NORMAL;
  if (parseExpressionAction(action, selectedExpression)) {
    startExpressionTransition(selectedExpression, now);
    s_state.actionExpireAt = now + ConfigManager::eyesConfig.interaction.moodHoldMs;
    return true;
  }

  return false;
}

void render() {
  unsigned long now = millis();
  if (s_lastEyesRenderAt > 0 &&
      now - s_lastEyesRenderAt < kEyesFrameIntervalMs) {
    return;
  }
  s_lastEyesRenderAt = now;

  const EyesConfig& config = ConfigManager::eyesConfig;
  updateRuntime(config);

  uint8_t eyeR = 0;
  uint8_t eyeG = 0;
  uint8_t eyeB = 0;
  if (!parseHexColor(config.style.eyeColor, eyeR, eyeG, eyeB)) {
    return;
  }
  MatrixPanel_I2S_DMA* offscreen = DisplayManager::beginOffscreenFrame(
    &DisplayManager::animationBuffer[0][0],
    0
  );
  if (offscreen == nullptr) {
    return;
  }

  drawPetBody(offscreen, now);

  float totalWidth = (float)config.layout.eyeWidth * 2.0f + (float)config.layout.eyeSpacing;
  float leftCenterX = ((float)DisplayManager::PANEL_RES_X - totalWidth) * 0.5f +
                      (float)config.layout.eyeWidth * 0.5f;
  float rightCenterX = leftCenterX + (float)config.layout.eyeWidth + (float)config.layout.eyeSpacing;
  float centerY = (float)config.layout.eyeY;
  float leftBlinkAmount = blinkAmountForEye(true);
  float rightBlinkAmount = blinkAmountForEye(false);
  EyeShapePreset leftPreset = applyVariationLayers(
    s_state.expression,
    true,
    now,
    presetForEye(s_state.expression, true)
  );
  EyeShapePreset rightPreset = applyVariationLayers(
    s_state.expression,
    false,
    now,
    presetForEye(s_state.expression, false)
  );
  EyeRenderTransform leftTransform = buildLookTransform(config, true);
  EyeRenderTransform rightTransform = buildLookTransform(config, false);

  drawEye(
    offscreen,
    leftCenterX,
    centerY,
    config,
    s_state.expression,
    leftPreset,
    leftTransform,
    true,
    leftBlinkAmount,
    eyeR,
    eyeG,
    eyeB
  );
  drawEye(
    offscreen,
    rightCenterX,
    centerY,
    config,
    s_state.expression,
    rightPreset,
    rightTransform,
    false,
    rightBlinkAmount,
    eyeR,
    eyeG,
    eyeB
  );

  drawTimeOverlay(offscreen, config);
  DisplayManager::presentOffscreenFrame(&DisplayManager::animationBuffer[0][0]);
}

}  // namespace EyesEffect
