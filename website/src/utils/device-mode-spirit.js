import { clamp, setPixel } from "@/utils/device-mode-core.js";
import {
  DEVICE_CLOCK_FONTS,
  drawClockTextToMap,
  getClockTextHeight,
  getClockTextWidth,
  getCurrentTimeText,
} from "@/utils/device-mode-clock.js";

export const EYES_CONFIG_STORAGE_KEY = "eyes_config";
export const EYES_EXPRESSION_STORAGE_KEY = "eyes_expression";
export const EYES_LOCAL_PREVIEW_STORAGE_KEY = "eyes_local_preview";

export const EXPRESSION_MODE_OPTIONS = Object.freeze([
  { value: "auto", label: "自动" },
  { value: "manual", label: "手动" },
]);

export const EXPRESSION_RHYTHM_OPTIONS = Object.freeze([
  { label: "慢速", value: "slow" },
  { label: "标准", value: "standard" },
  { label: "活泼", value: "lively" },
]);

export const EXPRESSION_OPTIONS = Object.freeze([
  { label: "正常", value: "Normal" },
  { label: "生气", value: "Angry" },
  { label: "偷笑", value: "Glee" },
  { label: "开心", value: "Happy" },
  { label: "爱心", value: "Heart" },
  { label: "难过", value: "Sad" },
  { label: "担忧", value: "Worried" },
  { label: "专注", value: "Focused" },
  { label: "不耐烦", value: "Annoyed" },
  { label: "惊讶", value: "Surprised" },
  { label: "怀疑", value: "Skeptic" },
  { label: "挫败", value: "Frustrated" },
  { label: "无语", value: "Unimpressed" },
  { label: "困倦", value: "Sleepy" },
  { label: "警觉", value: "Suspicious" },
  { label: "眯眼", value: "Squint" },
  { label: "愤怒", value: "Furious" },
  { label: "受惊", value: "Scared" },
  { label: "惊叹", value: "Awe" },
  { label: "兴奋", value: "Excited" },
  { label: "坚定", value: "Determined" },
  { label: "迷糊", value: "Confused" },
]);

export const EYES_TIME_FONT_OPTIONS = DEVICE_CLOCK_FONTS;

export const EYES_PRESET_COLORS = Object.freeze([
  { label: "蓝色", value: "#64c8ff" },
  { label: "青色", value: "#9bdcff" },
  { label: "绿色", value: "#78ffb6" },
  { label: "黄色", value: "#ffe08a" },
  { label: "粉色", value: "#ff8fc9" },
  { label: "白色", value: "#ffffff" },
]);

const HEART_EYE_COLOR = "#ff6fb3";

const EXPRESSION_STYLE_MAP = Object.freeze({
  Normal: { height: 10, slope: 0, mouth: "flat" },
  Angry: { height: 7, slope: 2, mouth: "flat" },
  Glee: { height: 4, slope: 0, mouth: "smile" },
  Happy: { height: 5, slope: 0, mouth: "smile" },
  Heart: { height: 10, slope: 0, mouth: "smile" },
  Sad: { height: 8, slope: -1, mouth: "frown" },
  Worried: { height: 9, slope: -1, mouth: "frown" },
  Focused: { height: 7, slope: 1, mouth: "flat" },
  Annoyed: { height: 6, slope: 0, mouth: "flat" },
  Surprised: { height: 14, slope: 0, mouth: "o" },
  Skeptic: { height: 8, slope: 1, mouth: "flat" },
  Frustrated: { height: 6, slope: 0, mouth: "frown" },
  Unimpressed: { height: 4, slope: 0, mouth: "none" },
  Sleepy: { height: 5, slope: -1, mouth: "flat" },
  Suspicious: { height: 8, slope: 0, mouth: "none" },
  Squint: { height: 4, slope: 0, mouth: "none" },
  Furious: { height: 9, slope: 2, mouth: "none" },
  Scared: { height: 13, slope: -1, mouth: "o" },
  Awe: { height: 12, slope: 0, mouth: "smile" },
  Excited: { height: 13, slope: 0, mouth: "o" },
  Determined: { height: 8, slope: 1, mouth: "flat" },
  Confused: { height: 9, slope: -1, mouth: "flat" },
});

export function createDefaultEyesConfig() {
  return {
    layout: {
      eyeY: 24,
      eyeSpacing: 14,
      eyeWidth: 16,
      eyeHeight: 10,
      timeX: 32,
      timeY: 5,
    },
    behavior: {
      autoSwitch: true,
      blinkIntervalMs: 3200,
      lookIntervalMs: 4200,
      idleMove: 2,
      sleepyAfterMs: 45000,
      expressionRhythm: "standard",
    },
    interaction: {
      lookHoldMs: 1200,
      moodHoldMs: 1800,
    },
    time: {
      show: true,
      showSeconds: false,
      font: "classic_5x7",
      fontSize: 1,
      align: "center",
    },
    style: {
      eyeColor: "#9bdcff",
      timeColor: "#64c8ff",
    },
  };
}

export function createDefaultLocalPreviewState() {
  return {
    mouthEnabled: true,
    mouthOffsetX: 0,
    mouthOffsetY: 0,
  };
}

export function normalizeEyesConfig(saved) {
  const next = createDefaultEyesConfig();
  if (!saved || typeof saved !== "object") {
    return next;
  }
  if (saved.layout && typeof saved.layout === "object") {
    Object.assign(next.layout, saved.layout);
  }
  if (saved.behavior && typeof saved.behavior === "object") {
    Object.assign(next.behavior, saved.behavior);
  }
  if (saved.interaction && typeof saved.interaction === "object") {
    Object.assign(next.interaction, saved.interaction);
  }
  if (saved.time && typeof saved.time === "object") {
    Object.assign(next.time, saved.time);
  }
  if (saved.style && typeof saved.style === "object") {
    Object.assign(next.style, saved.style);
  }
  return next;
}

export function normalizeLocalPreview(saved) {
  const next = createDefaultLocalPreviewState();
  if (!saved || typeof saved !== "object") {
    return next;
  }
  if (saved.mouthEnabled === true || saved.mouthEnabled === false) {
    next.mouthEnabled = saved.mouthEnabled;
  }
  if (Number.isFinite(Number(saved.mouthOffsetX))) {
    next.mouthOffsetX = clamp(Math.round(Number(saved.mouthOffsetX)), -3, 3);
  }
  if (Number.isFinite(Number(saved.mouthOffsetY))) {
    next.mouthOffsetY = clamp(Math.round(Number(saved.mouthOffsetY)), -3, 3);
  }
  return next;
}

function drawEye(map, centerX, centerY, config, style, color, blinkClosed, lookOffsetX = 0, lookOffsetY = 0) {
  const halfWidth = Math.floor(config.layout.eyeWidth / 2);
  const halfHeight = Math.max(1, Math.floor(style.height / 2));
  for (let x = -halfWidth; x <= halfWidth; x += 1) {
    const slopeOffset = Math.round((style.slope * x) / Math.max(1, halfWidth));
    const top = centerY - halfHeight + slopeOffset;
    const bottom = centerY + halfHeight + slopeOffset;
    if (blinkClosed) {
      setPixel(map, centerX + x, centerY + slopeOffset, color);
      continue;
    }
    for (let y = top; y <= bottom; y += 1) {
      setPixel(map, centerX + x, y, color);
    }
  }
  if (!blinkClosed && style.height > 5 && style.mouth !== "o") {
    setPixel(map, centerX + lookOffsetX, centerY + lookOffsetY, "#000000");
  }
}

function drawMouth(map, centerX, centerY, type, color) {
  if (type === "none") {
    return;
  }
  if (type === "o") {
    setPixel(map, centerX, centerY - 1, color);
    setPixel(map, centerX - 1, centerY, color);
    setPixel(map, centerX + 1, centerY, color);
    setPixel(map, centerX, centerY + 1, color);
    return;
  }
  if (type === "smile") {
    setPixel(map, centerX - 1, centerY, color);
    setPixel(map, centerX, centerY + 1, color);
    setPixel(map, centerX + 1, centerY, color);
    return;
  }
  if (type === "frown") {
    setPixel(map, centerX - 1, centerY, color);
    setPixel(map, centerX, centerY - 1, color);
    setPixel(map, centerX + 1, centerY, color);
    return;
  }
  setPixel(map, centerX - 1, centerY, color);
  setPixel(map, centerX, centerY, color);
  setPixel(map, centerX + 1, centerY, color);
}

export function createSpiritPreviewRuntime(expression) {
  return {
    expression,
    blinkUntil: 0,
    lookX: 0,
    lookY: 0,
    nextExpressionAt: Date.now() + 6000,
  };
}

export function triggerSpiritPreviewAction(runtime, action) {
  const now = Date.now();
  if (action === "blink") {
    runtime.blinkUntil = now + 180;
    return;
  }
  if (action === "look_left") {
    runtime.lookX = -1;
    runtime.lookY = 0;
    return;
  }
  if (action === "look_center") {
    runtime.lookX = 0;
    runtime.lookY = 0;
    return;
  }
  if (action === "look_right") {
    runtime.lookX = 1;
    runtime.lookY = 0;
  }
}

export function stepSpiritPreview(runtime, eyesConfig, selectedExpression) {
  const now = Date.now();
  if (eyesConfig.behavior.autoSwitch && now >= runtime.nextExpressionAt) {
    const index = EXPRESSION_OPTIONS.findIndex((item) => item.value === runtime.expression);
    const nextIndex = index >= 0 ? (index + 1) % EXPRESSION_OPTIONS.length : 0;
    runtime.expression = EXPRESSION_OPTIONS[nextIndex].value;
    runtime.nextExpressionAt =
      now +
      (eyesConfig.behavior.expressionRhythm === "lively"
        ? 4200
        : eyesConfig.behavior.expressionRhythm === "slow"
          ? 9000
          : 6000);
  }
  if (!eyesConfig.behavior.autoSwitch) {
    runtime.expression = selectedExpression;
  }
}

export function getSpiritTimePreviewText(eyesConfig) {
  return eyesConfig.time.showSeconds ? "12:34:56" : "12:34";
}

export function getSpiritTimeLayoutBounds(eyesConfig) {
  const textWidth = Math.max(
    0,
    Math.round(getClockTextWidth(getSpiritTimePreviewText(eyesConfig), eyesConfig.time.font, eyesConfig.time.fontSize)),
  );
  const textHeight = Math.max(0, Math.round(getClockTextHeight(eyesConfig.time.font, eyesConfig.time.fontSize)));
  const maxStartX = Math.max(0, 64 - textWidth);
  const maxTimeY = Math.max(0, 64 - textHeight);

  if (eyesConfig.time.align === "center") {
    const anchorOffset = Math.floor(textWidth / 2);
    return {
      minAnchorX: anchorOffset,
      maxAnchorX: maxStartX + anchorOffset,
      minTimeY: 0,
      maxTimeY,
      minStartX: 0,
      maxStartX,
    };
  }

  if (eyesConfig.time.align === "right") {
    return {
      minAnchorX: textWidth,
      maxAnchorX: maxStartX + textWidth,
      minTimeY: 0,
      maxTimeY,
      minStartX: 0,
      maxStartX,
    };
  }

  return {
    minAnchorX: 0,
    maxAnchorX: maxStartX,
    minTimeY: 0,
    maxTimeY,
    minStartX: 0,
    maxStartX,
  };
}

export function resolveSpiritTimeStartX(eyesConfig) {
  const bounds = getSpiritTimeLayoutBounds(eyesConfig);
  const textWidth = getClockTextWidth(getSpiritTimePreviewText(eyesConfig), eyesConfig.time.font, eyesConfig.time.fontSize);
  const anchorX = Number(eyesConfig.layout.timeX);

  if (eyesConfig.time.align === "center") {
    return clamp(anchorX - Math.floor(textWidth / 2), bounds.minStartX, bounds.maxStartX);
  }

  if (eyesConfig.time.align === "right") {
    return clamp(anchorX - textWidth, bounds.minStartX, bounds.maxStartX);
  }

  return clamp(anchorX, bounds.minStartX, bounds.maxStartX);
}

export function normalizeSpiritTimeLayout(eyesConfig) {
  eyesConfig.time.fontSize = clamp(Number(eyesConfig.time.fontSize), 1, 3);
  if (
    eyesConfig.time.align !== "left" &&
    eyesConfig.time.align !== "center" &&
    eyesConfig.time.align !== "right"
  ) {
    eyesConfig.time.align = "center";
  }
  const bounds = getSpiritTimeLayoutBounds(eyesConfig);
  eyesConfig.layout.timeX = clamp(Number(eyesConfig.layout.timeX), bounds.minAnchorX, bounds.maxAnchorX);
  eyesConfig.layout.timeY = clamp(Number(eyesConfig.layout.timeY), bounds.minTimeY, bounds.maxTimeY);
  return eyesConfig;
}

export function buildEyesConfigPayload(eyesConfig) {
  const bounds = getSpiritTimeLayoutBounds(eyesConfig);
  return {
    layout: {
      eyeY: eyesConfig.layout.eyeY,
      eyeSpacing: eyesConfig.layout.eyeSpacing,
      eyeWidth: eyesConfig.layout.eyeWidth,
      eyeHeight: eyesConfig.layout.eyeHeight,
      timeX: resolveSpiritTimeStartX(eyesConfig),
      timeY: clamp(Number(eyesConfig.layout.timeY), bounds.minTimeY, bounds.maxTimeY),
    },
    behavior: {
      autoSwitch: eyesConfig.behavior.autoSwitch,
      blinkIntervalMs: eyesConfig.behavior.blinkIntervalMs,
      lookIntervalMs: eyesConfig.behavior.lookIntervalMs,
      idleMove: eyesConfig.behavior.idleMove,
      sleepyAfterMs: eyesConfig.behavior.sleepyAfterMs,
      expressionRhythm: eyesConfig.behavior.expressionRhythm,
    },
    interaction: {
      lookHoldMs: eyesConfig.interaction.lookHoldMs,
      moodHoldMs: eyesConfig.interaction.moodHoldMs,
    },
    time: {
      show: eyesConfig.time.show,
      showSeconds: eyesConfig.time.showSeconds,
      font: eyesConfig.time.font,
      fontSize: eyesConfig.time.fontSize,
    },
    style: {
      eyeColor: eyesConfig.style.eyeColor,
      timeColor: eyesConfig.style.timeColor,
    },
  };
}

export function buildSpiritPreviewPixels(eyesConfig, selectedExpression, localPreview, runtime) {
  const map = new Map();
  const expression = eyesConfig.behavior.autoSwitch ? runtime.expression : selectedExpression;
  const style = EXPRESSION_STYLE_MAP[expression];
  if (style === undefined) {
    throw new Error("未识别的桌面宠物表情");
  }
  const blinkClosed = Date.now() <= runtime.blinkUntil;
  const eyeColor = expression === "Heart" ? HEART_EYE_COLOR : eyesConfig.style.eyeColor;
  const leftCenterX = 32 - Math.floor(eyesConfig.layout.eyeSpacing / 2);
  const rightCenterX = 32 + Math.floor(eyesConfig.layout.eyeSpacing / 2);

  drawEye(map, leftCenterX, eyesConfig.layout.eyeY, eyesConfig, style, eyeColor, blinkClosed, runtime.lookX, runtime.lookY);
  drawEye(map, rightCenterX, eyesConfig.layout.eyeY, eyesConfig, style, eyeColor, blinkClosed, runtime.lookX, runtime.lookY);

  if (localPreview.mouthEnabled) {
    drawMouth(
      map,
      32 + localPreview.mouthOffsetX,
      eyesConfig.layout.eyeY + 16 + localPreview.mouthOffsetY,
      style.mouth,
      eyeColor,
    );
  }

  if (eyesConfig.time.show) {
    drawClockTextToMap({
      map,
      text: getCurrentTimeText(eyesConfig.time.showSeconds, 24),
      x: eyesConfig.layout.timeX,
      y: eyesConfig.layout.timeY,
      color: eyesConfig.style.timeColor,
      fontId: eyesConfig.time.font,
      fontSize: eyesConfig.time.fontSize,
      align: eyesConfig.time.align,
    });
  }

  return map;
}
