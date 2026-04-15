const WIDTH = 64;
const HEIGHT = 64;

const TOP_CHAMBER = [
  { x: 22, y: 10 },
  { x: 42, y: 10 },
  { x: 32, y: 27 },
];

const BOTTOM_CHAMBER = [
  { x: 32, y: 30 },
  { x: 22, y: 46 },
  { x: 42, y: 46 },
];

const TOP_RANGE = { minX: 22, maxX: 42, minY: 10, maxY: 27 };
const BOTTOM_RANGE = { minX: 22, maxX: 42, minY: 30, maxY: 46 };
const NECK_X = 32;
const NECK_Y = 28;
const DROP_TARGET_Y = BOTTOM_RANGE.minY;

const PREVIEW_TARGET_INTERVAL_MS = 120;
const PREVIEW_MIN_FRAME_COUNT = 300;
const PREVIEW_MAX_FRAME_COUNT = 2400;
const FLOW_SETTLE_FRAMES = 12;
const PHYSICS_STEPS_PER_FRAME = 1;

const COLORS = {
  outline: "#c4d2f4",
  sand: "#ffcf4a",
  sandHighlight: "#fff1a6",
  sandShadow: "#f4a935",
  panelLine: "#203044",
  text: "#ffe7a4",
};

const FONT_3X5 = {
  "0": [0b111, 0b101, 0b101, 0b101, 0b111],
  "1": [0b010, 0b110, 0b010, 0b010, 0b111],
  "2": [0b111, 0b001, 0b111, 0b100, 0b111],
  "3": [0b111, 0b001, 0b111, 0b001, 0b111],
  "4": [0b101, 0b101, 0b111, 0b001, 0b001],
  "5": [0b111, 0b100, 0b111, 0b001, 0b111],
  "6": [0b111, 0b100, 0b111, 0b101, 0b111],
  "7": [0b111, 0b001, 0b001, 0b001, 0b001],
  "8": [0b111, 0b101, 0b111, 0b101, 0b111],
  "9": [0b111, 0b101, 0b111, 0b001, 0b111],
  ":": [0b000, 0b010, 0b000, 0b010, 0b000],
};

function clampInt(value, minValue, maxValue) {
  if (!Number.isFinite(value)) {
    return minValue;
  }
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

function setPixel(frameMap, x, y, color) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return;
  }
  frameMap.set(`${x},${y}`, color);
}

function drawLine(frameMap, x0, y0, x1, y1, color) {
  let startX = x0;
  let startY = y0;
  const dx = Math.abs(x1 - startX);
  const sx = startX < x1 ? 1 : -1;
  const dy = -Math.abs(y1 - startY);
  const sy = startY < y1 ? 1 : -1;
  let error = dx + dy;

  while (true) {
    setPixel(frameMap, startX, startY, color);
    if (startX === x1 && startY === y1) {
      break;
    }
    const twiceError = error * 2;
    if (twiceError >= dy) {
      error += dy;
      startX += sx;
    }
    if (twiceError <= dx) {
      error += dx;
      startY += sy;
    }
  }
}

function drawPolygon(frameMap, points, color) {
  for (let index = 0; index < points.length; index += 1) {
    const current = points[index];
    const next = points[(index + 1) % points.length];
    drawLine(frameMap, current.x, current.y, next.x, next.y, color);
  }
}

function pointInPolygon(x, y, points) {
  let inside = false;
  for (
    let currentIndex = 0, previousIndex = points.length - 1;
    currentIndex < points.length;
    previousIndex = currentIndex, currentIndex += 1
  ) {
    const current = points[currentIndex];
    const previous = points[previousIndex];
    const intersect =
      current.y > y !== previous.y > y &&
      x < ((previous.x - current.x) * (y - current.y)) / (previous.y - current.y) + current.x;
    if (intersect) {
      inside = !inside;
    }
  }
  return inside;
}

function collectCellsInPolygon(points, range) {
  const cells = [];
  for (let y = range.minY; y <= range.maxY; y += 1) {
    for (let x = range.minX; x <= range.maxX; x += 1) {
      if (!pointInPolygon(x + 0.5, y + 0.5, points)) {
        continue;
      }
      cells.push({ x, y });
    }
  }
  return cells;
}

function drawTinyText(frameMap, text, x, y, color, size = 1) {
  let cursorX = x;
  const scale = Math.max(1, size);
  for (let index = 0; index < text.length; index += 1) {
    const glyph = FONT_3X5[text[index]];
    if (!glyph) {
      cursorX += 4 * scale;
      continue;
    }
    for (let row = 0; row < glyph.length; row += 1) {
      const bits = glyph[row];
      for (let col = 0; col < 3; col += 1) {
        if ((bits & (1 << (2 - col))) === 0) {
          continue;
        }
        for (let dx = 0; dx < scale; dx += 1) {
          for (let dy = 0; dy < scale; dy += 1) {
            setPixel(frameMap, cursorX + col * scale + dx, y + row * scale + dy, color);
          }
        }
      }
    }
    cursorX += 4 * scale;
  }
}

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return function random() {
    state = (state * 1664525 + 1013904223) >>> 0;
    return state / 4294967296;
  };
}

function keyOfCell(x, y) {
  return `${x},${y}`;
}

function parseCellKey(key) {
  const [x, y] = key.split(",").map(Number);
  return { x, y };
}

function hashConfig(config) {
  return (
    (config.hours * 73856093) ^
    (config.minutes * 19349663) ^
    (config.seconds * 83492791) ^
    (config.progress * 2654435761)
  ) >>> 0;
}

function padTime(value) {
  const normalized = Math.max(0, Math.floor(value));
  if (normalized < 10) {
    return `0${normalized}`;
  }
  return `${normalized}`;
}

function resolveTotalSeconds(config) {
  return config.hours * 3600 + config.minutes * 60 + config.seconds;
}

function buildMaskSet(cells) {
  const set = new Set();
  cells.forEach((cell) => set.add(keyOfCell(cell.x, cell.y)));
  return set;
}

function buildTopFilledSet(cells, count) {
  const sorted = cells
    .slice()
    .sort((a, b) => (a.y - b.y) || (Math.abs(a.x - NECK_X) - Math.abs(b.x - NECK_X)));
  const set = new Set();
  const limit = Math.min(count, sorted.length);
  for (let index = 0; index < limit; index += 1) {
    const cell = sorted[index];
    set.add(keyOfCell(cell.x, cell.y));
  }
  return set;
}

function buildBottomFilledSet(cells, count) {
  const sorted = cells
    .slice()
    .sort((a, b) => (b.y - a.y) || (Math.abs(a.x - NECK_X) - Math.abs(b.x - NECK_X)));
  const set = new Set();
  const limit = Math.min(count, sorted.length);
  for (let index = 0; index < limit; index += 1) {
    const cell = sorted[index];
    set.add(keyOfCell(cell.x, cell.y));
  }
  return set;
}

function isEmpty(maskSet, grainSet, x, y) {
  const key = keyOfCell(x, y);
  if (!maskSet.has(key)) {
    return false;
  }
  return !grainSet.has(key);
}

function moveGrain(grainSet, fromX, fromY, toX, toY) {
  const fromKey = keyOfCell(fromX, fromY);
  const toKey = keyOfCell(toX, toY);
  grainSet.delete(fromKey);
  grainSet.add(toKey);
}

// eHorglass SandMove downward semantics:
// MID -> LEFT -> RIGHT with random split when both diagonals are available.
function settleChamber(grainSet, maskSet, range, random) {
  for (let y = range.maxY - 1; y >= range.minY; y -= 1) {
    const leftFirst = random() < 0.5;
    if (leftFirst) {
      for (let x = range.minX; x <= range.maxX; x += 1) {
        settleOne(grainSet, maskSet, random, x, y);
      }
      continue;
    }
    for (let x = range.maxX; x >= range.minX; x -= 1) {
      settleOne(grainSet, maskSet, random, x, y);
    }
  }
}

function settleOne(grainSet, maskSet, random, x, y) {
  const key = keyOfCell(x, y);
  if (!grainSet.has(key)) {
    return;
  }
  const midX = x;
  const midY = y + 1;
  const leftX = x - 1;
  const leftY = y + 1;
  const rightX = x + 1;
  const rightY = y + 1;

  const midEmpty = isEmpty(maskSet, grainSet, midX, midY);
  const leftEmpty = isEmpty(maskSet, grainSet, leftX, leftY);
  const rightEmpty = isEmpty(maskSet, grainSet, rightX, rightY);

  if (midEmpty) {
    moveGrain(grainSet, x, y, midX, midY);
    return;
  }
  if (leftEmpty && !rightEmpty) {
    moveGrain(grainSet, x, y, leftX, leftY);
    return;
  }
  if (!leftEmpty && rightEmpty) {
    moveGrain(grainSet, x, y, rightX, rightY);
    return;
  }
  if (leftEmpty && rightEmpty) {
    if (random() < 0.5) {
      moveGrain(grainSet, x, y, leftX, leftY);
      return;
    }
    moveGrain(grainSet, x, y, rightX, rightY);
  }
}

function pickTopLeakCell(topSet) {
  for (let y = TOP_RANGE.maxY; y >= TOP_RANGE.minY; y -= 1) {
    for (let offset = 0; offset <= TOP_RANGE.maxX - TOP_RANGE.minX; offset += 1) {
      const leftX = NECK_X - offset;
      if (leftX >= TOP_RANGE.minX) {
        const leftKey = keyOfCell(leftX, y);
        if (topSet.has(leftKey)) {
          return { x: leftX, y };
        }
      }
      if (offset === 0) {
        continue;
      }
      const rightX = NECK_X + offset;
      if (rightX <= TOP_RANGE.maxX) {
        const rightKey = keyOfCell(rightX, y);
        if (topSet.has(rightKey)) {
          return { x: rightX, y };
        }
      }
    }
  }
  return null;
}

function findBottomLandingCell(bottomSet, bottomMask, originX, originY, random) {
  let x = originX;
  let y = originY;

  for (let step = 0; step < 36; step += 1) {
    const downEmpty = isEmpty(bottomMask, bottomSet, x, y + 1);
    const leftEmpty = isEmpty(bottomMask, bottomSet, x - 1, y + 1);
    const rightEmpty = isEmpty(bottomMask, bottomSet, x + 1, y + 1);

    if (downEmpty) {
      y += 1;
      continue;
    }
    if (leftEmpty && !rightEmpty) {
      x -= 1;
      y += 1;
      continue;
    }
    if (!leftEmpty && rightEmpty) {
      x += 1;
      y += 1;
      continue;
    }
    if (leftEmpty && rightEmpty) {
      if (random() < 0.5) {
        x -= 1;
      } else {
        x += 1;
      }
      y += 1;
      continue;
    }
    break;
  }

  const targetKey = keyOfCell(x, y);
  if (bottomMask.has(targetKey) && !bottomSet.has(targetKey)) {
    return { x, y };
  }

  for (let scanY = BOTTOM_RANGE.minY; scanY <= BOTTOM_RANGE.maxY; scanY += 1) {
    for (let offset = 0; offset <= BOTTOM_RANGE.maxX - BOTTOM_RANGE.minX; offset += 1) {
      const leftX = NECK_X - offset;
      if (leftX >= BOTTOM_RANGE.minX) {
        const leftKey = keyOfCell(leftX, scanY);
        if (bottomMask.has(leftKey) && !bottomSet.has(leftKey)) {
          return { x: leftX, y: scanY };
        }
      }
      if (offset === 0) {
        continue;
      }
      const rightX = NECK_X + offset;
      if (rightX <= BOTTOM_RANGE.maxX) {
        const rightKey = keyOfCell(rightX, scanY);
        if (bottomMask.has(rightKey) && !bottomSet.has(rightKey)) {
          return { x: rightX, y: scanY };
        }
      }
    }
  }

  return null;
}

function drawHourglassOutline(frameMap) {
  drawPolygon(frameMap, TOP_CHAMBER, COLORS.outline);
  drawPolygon(frameMap, BOTTOM_CHAMBER, COLORS.outline);
  drawLine(frameMap, 22, 29, 30, 29, COLORS.outline);
  drawLine(frameMap, 34, 29, 42, 29, COLORS.outline);
  drawLine(frameMap, 32, 28, 32, 30, COLORS.outline);
}

export function createDefaultCountdownBoardConfig() {
  return {
    hours: 0,
    minutes: 10,
    seconds: 0,
    progress: 100,
  };
}

export function normalizeCountdownBoardConfig(savedConfig) {
  const base = createDefaultCountdownBoardConfig();
  if (!savedConfig || typeof savedConfig !== "object") {
    return base;
  }

  return {
    hours: clampInt(Math.round(Number(savedConfig.hours)), 0, 99),
    minutes: clampInt(Math.round(Number(savedConfig.minutes)), 0, 59),
    seconds: clampInt(Math.round(Number(savedConfig.seconds)), 0, 59),
    progress: clampInt(Math.round(Number(savedConfig.progress)), 0, 100),
  };
}

export function resolveCountdownBoardRemainingSeconds(config) {
  const normalized = normalizeCountdownBoardConfig(config);
  const totalSeconds = resolveTotalSeconds(normalized);
  if (totalSeconds <= 0) {
    return 0;
  }
  return clampInt(Math.round((totalSeconds * normalized.progress) / 100), 0, totalSeconds);
}

export function formatCountdownBoardText(config) {
  const normalized = normalizeCountdownBoardConfig(config);
  const remainingSeconds = resolveCountdownBoardRemainingSeconds(normalized);
  const hours = Math.floor(remainingSeconds / 3600);
  const minutes = Math.floor((remainingSeconds % 3600) / 60);
  const seconds = remainingSeconds % 60;

  if (normalized.hours > 0) {
    return {
      text: `${padTime(hours)}:${padTime(minutes)}:${padTime(seconds)}`,
      size: 1,
    };
  }
  return {
    text: `${padTime(minutes)}:${padTime(seconds)}`,
    size: 1,
  };
}

function drawTimeText(frameMap, config) {
  const textConfig = formatCountdownBoardText(config);
  const textWidth = textConfig.text.length * 4 * textConfig.size - textConfig.size;
  const textX = Math.floor((WIDTH - textWidth) / 2);
  const textY = HEIGHT - 8;
  drawTinyText(frameMap, textConfig.text, textX + 1, textY + 1, COLORS.panelLine, textConfig.size);
  drawTinyText(frameMap, textConfig.text, textX, textY, COLORS.text, textConfig.size);
}

function isSurface(set, x, y) {
  return !set.has(keyOfCell(x, y - 1));
}

function drawGrainSet(frameMap, set, frameIndex, mainColor, highlightColor, shadowColor) {
  set.forEach((key) => {
    const cell = parseCellKey(key);
    let color = mainColor;
    if (isSurface(set, cell.x, cell.y)) {
      color = highlightColor;
    } else if ((cell.x + cell.y + frameIndex) % 6 === 0) {
      color = shadowColor;
    }
    setPixel(frameMap, cell.x, cell.y, color);
  });
}

function drawLeakParticle(frameMap, leakParticle) {
  if (!leakParticle) {
    return;
  }
  setPixel(frameMap, leakParticle.x, leakParticle.y, COLORS.sand);
}

function createSandState(config, frameCount) {
  const totalSeconds = resolveTotalSeconds(config);
  const remainingSeconds = resolveCountdownBoardRemainingSeconds(config);
  let ratio = 0;
  if (totalSeconds > 0) {
    ratio = remainingSeconds / totalSeconds;
  }
  ratio = Math.max(0, Math.min(1, ratio));

  const topCells = collectCellsInPolygon(TOP_CHAMBER, TOP_RANGE);
  const bottomCells = collectCellsInPolygon(BOTTOM_CHAMBER, BOTTOM_RANGE);
  const topMask = buildMaskSet(topCells);
  const bottomMask = buildMaskSet(bottomCells);
  const random = createSeededRandom(hashConfig(config));
  const totalCapacity = topCells.length + bottomCells.length;
  const grainTotal = clampInt(Math.round(totalCapacity * 0.58), 96, totalCapacity);
  const topCount = clampInt(Math.round(grainTotal * ratio), 0, grainTotal);
  const bottomCount = grainTotal - topCount;
  const drainFrames = Math.max(1, frameCount - FLOW_SETTLE_FRAMES);

  return {
    random,
    topMask,
    bottomMask,
    topSet: buildTopFilledSet(topCells, topCount),
    bottomSet: buildBottomFilledSet(bottomCells, bottomCount),
    hasFlow: totalSeconds > 0 && ratio > 0,
    drainFrames,
    topInitialCount: topCount,
    releasedCount: 0,
    landedCount: 0,
    pendingRelease: 0,
    leakParticle: null,
  };
}

function scheduleLeak(state, frameIndex) {
  if (!state.hasFlow) {
    return;
  }
  if (frameIndex >= state.drainFrames) {
    return;
  }
  const expectedReleased = Math.floor(((frameIndex + 1) * state.topInitialCount) / state.drainFrames);
  if (expectedReleased <= state.releasedCount) {
    return;
  }
  state.pendingRelease += expectedReleased - state.releasedCount;
}

function spawnLeak(state) {
  if (!state.hasFlow) {
    return;
  }
  if (state.pendingRelease <= 0) {
    return;
  }
  if (state.leakParticle) {
    return;
  }
  const leakCell = pickTopLeakCell(state.topSet);
  if (!leakCell) {
    return;
  }
  state.topSet.delete(keyOfCell(leakCell.x, leakCell.y));
  state.leakParticle = { x: NECK_X, y: NECK_Y };
  state.pendingRelease -= 1;
  state.releasedCount += 1;
}

function advanceLeak(state) {
  if (!state.leakParticle) {
    return;
  }

  state.leakParticle.y += 1;
  if (state.leakParticle.y < DROP_TARGET_Y) {
    return;
  }

  const landing = findBottomLandingCell(
    state.bottomSet,
    state.bottomMask,
    state.leakParticle.x,
    DROP_TARGET_Y,
    state.random,
  );
  if (landing) {
    state.bottomSet.add(keyOfCell(landing.x, landing.y));
    state.landedCount += 1;
  }
  state.leakParticle = null;
}

function runPhysicsStep(state) {
  settleChamber(state.topSet, state.topMask, TOP_RANGE, state.random);
  settleChamber(state.bottomSet, state.bottomMask, BOTTOM_RANGE, state.random);
}

function resolvePreviewFrameCount(totalSeconds) {
  if (totalSeconds <= 0) {
    return PREVIEW_MIN_FRAME_COUNT;
  }
  const estimated = Math.round((totalSeconds * 1000) / PREVIEW_TARGET_INTERVAL_MS);
  return clampInt(estimated, PREVIEW_MIN_FRAME_COUNT, PREVIEW_MAX_FRAME_COUNT);
}

export function resolveCountdownBoardPreviewFrameInterval(config) {
  const normalized = normalizeCountdownBoardConfig(config);
  const totalSeconds = resolveTotalSeconds(normalized);
  const frameCount = resolvePreviewFrameCount(totalSeconds);
  if (totalSeconds <= 0) {
    return 120;
  }
  return Math.max(1, Math.round((totalSeconds * 1000) / frameCount));
}

export function buildCountdownBoardPreviewFrames(config) {
  const normalized = normalizeCountdownBoardConfig(config);
  const totalSeconds = resolveTotalSeconds(normalized);
  const frameCount = resolvePreviewFrameCount(totalSeconds);
  const frames = [];
  const state = createSandState(normalized, frameCount);

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    for (let step = 0; step < PHYSICS_STEPS_PER_FRAME; step += 1) {
      runPhysicsStep(state);
    }
    scheduleLeak(state, frameIndex);
    spawnLeak(state);
    advanceLeak(state);

    const frameMap = new Map();
    drawGrainSet(
      frameMap,
      state.topSet,
      frameIndex,
      COLORS.sand,
      COLORS.sandHighlight,
      COLORS.sandShadow,
    );
    drawGrainSet(
      frameMap,
      state.bottomSet,
      frameIndex,
      COLORS.sandShadow,
      COLORS.sandHighlight,
      COLORS.sand,
    );
    drawLeakParticle(frameMap, state.leakParticle);
    drawHourglassOutline(frameMap);
    drawTimeText(frameMap, normalized);
    frames.push(frameMap);
  }

  return frames;
}
