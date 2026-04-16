const WIDTH = 64;
const HEIGHT = 64;
const STACK_DEPTH = 6;
const SAND_FILL_RATIO = 0.84;
const TOP_POLYGON = [
  { x: 20, y: 8 },
  { x: 44, y: 8 },
  { x: 32, y: 28 },
];
const BOTTOM_POLYGON = [
  { x: 32, y: 31 },
  { x: 20, y: 47 },
  { x: 44, y: 47 },
];
const TOP_RANGE = { minX: 20, maxX: 44, minY: 8, maxY: 28 };
const BOTTOM_RANGE = { minX: 20, maxX: 44, minY: 31, maxY: 47 };
const NECK_X = 32;
const NECK_Y = 29;
const STREAM_ENTRY_Y = 30;
const MAX_STREAM_PARTICLES = 6;
const PHYSICS_STEPS_PER_FRAME = 2;
const PREVIEW_TARGET_INTERVAL_MS = 120;
const PREVIEW_MIN_FRAME_COUNT = 300;
const PREVIEW_MAX_FRAME_COUNT = 2400;
const FLOW_SETTLE_FRAMES = 8;

const COLORS = {
  outline: "#c8d6f6",
  sand: "#ffd15a",
  sandBright: "#fff0a6",
  text: "#ffe9ab",
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

function indexOf(x, y) {
  return y * WIDTH + x;
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

function createMask(cells) {
  const mask = new Uint8Array(WIDTH * HEIGHT);
  for (let index = 0; index < cells.length; index += 1) {
    const cell = cells[index];
    mask[indexOf(cell.x, cell.y)] = 1;
  }
  return mask;
}

function canReceive(mask, depthGrid, x, y) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return false;
  }
  const index = indexOf(x, y);
  if (mask[index] !== 1) {
    return false;
  }
  return depthGrid[index] < STACK_DEPTH;
}

function hasSand(depthGrid, x, y) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return false;
  }
  return depthGrid[indexOf(x, y)] > 0;
}

function moveDepth(depthGrid, fromX, fromY, toX, toY) {
  const fromIndex = indexOf(fromX, fromY);
  const toIndex = indexOf(toX, toY);
  depthGrid[fromIndex] -= 1;
  depthGrid[toIndex] += 1;
}

function settleOne(depthGrid, mask, random, x, y) {
  const currentIndex = indexOf(x, y);
  if (depthGrid[currentIndex] <= 0) {
    return;
  }

  const midX = x;
  const midY = y + 1;
  const leftX = x - 1;
  const leftY = y + 1;
  const rightX = x + 1;
  const rightY = y + 1;

  const midOpen = canReceive(mask, depthGrid, midX, midY);
  const leftOpen = canReceive(mask, depthGrid, leftX, leftY);
  const rightOpen = canReceive(mask, depthGrid, rightX, rightY);

  if (midOpen) {
    moveDepth(depthGrid, x, y, midX, midY);
    return;
  }
  if (leftOpen && !rightOpen) {
    moveDepth(depthGrid, x, y, leftX, leftY);
    return;
  }
  if (!leftOpen && rightOpen) {
    moveDepth(depthGrid, x, y, rightX, rightY);
    return;
  }
  if (leftOpen && rightOpen) {
    if (random() < 0.5) {
      moveDepth(depthGrid, x, y, leftX, leftY);
      return;
    }
    moveDepth(depthGrid, x, y, rightX, rightY);
  }
}

function settleChamber(depthGrid, mask, range, random) {
  for (let y = range.maxY - 1; y >= range.minY; y -= 1) {
    if (random() < 0.5) {
      for (let x = range.minX; x <= range.maxX; x += 1) {
        settleOne(depthGrid, mask, random, x, y);
      }
      continue;
    }
    for (let x = range.maxX; x >= range.minX; x -= 1) {
      settleOne(depthGrid, mask, random, x, y);
    }
  }
}

function findTopReleaseSource(topDepthGrid) {
  for (let y = TOP_RANGE.maxY; y >= TOP_RANGE.minY; y -= 1) {
    for (let offset = 0; offset <= TOP_RANGE.maxX - TOP_RANGE.minX; offset += 1) {
      const leftX = NECK_X - offset;
      if (leftX >= TOP_RANGE.minX && hasSand(topDepthGrid, leftX, y)) {
        return { x: leftX, y };
      }
      if (offset === 0) {
        continue;
      }
      const rightX = NECK_X + offset;
      if (rightX <= TOP_RANGE.maxX && hasSand(topDepthGrid, rightX, y)) {
        return { x: rightX, y };
      }
    }
  }
  return null;
}

function buildInitialDepthGrid(cells, count, preferNeck) {
  const depthGrid = new Uint8Array(WIDTH * HEIGHT);
  const sorted = cells.slice().sort((a, b) => {
    const yDelta = b.y - a.y;
    if (yDelta !== 0) {
      return yDelta;
    }
    if (preferNeck) {
      return Math.abs(a.x - NECK_X) - Math.abs(b.x - NECK_X);
    }
    return a.x - b.x;
  });

  let remaining = count;
  for (let index = 0; index < sorted.length && remaining > 0; index += 1) {
    const cell = sorted[index];
    const fill = Math.min(STACK_DEPTH, remaining);
    depthGrid[indexOf(cell.x, cell.y)] = fill;
    remaining -= fill;
  }
  return depthGrid;
}

function countParticles(depthGrid, cells) {
  let total = 0;
  for (let index = 0; index < cells.length; index += 1) {
    const cell = cells[index];
    total += depthGrid[indexOf(cell.x, cell.y)];
  }
  return total;
}

function buildOccupiedParticleSet(particles) {
  const occupied = new Set();
  for (let index = 0; index < particles.length; index += 1) {
    const particle = particles[index];
    occupied.add(`${particle.x},${particle.y}`);
  }
  return occupied;
}

function canMoveParticleTo(state, occupied, x, y) {
  if (occupied.has(`${x},${y}`)) {
    return false;
  }
  if (y < BOTTOM_RANGE.minY) {
    return x >= NECK_X - 1 && x <= NECK_X + 1 && y <= BOTTOM_RANGE.minY;
  }
  return canReceive(state.bottomMask, state.bottomDepthGrid, x, y);
}

function chooseParticleTarget(state, occupied, particle) {
  const targets = [
    { x: particle.x, y: particle.y + 1 },
    { x: particle.x - 1, y: particle.y + 1 },
    { x: particle.x + 1, y: particle.y + 1 },
  ];

  const downOpen = canMoveParticleTo(state, occupied, targets[0].x, targets[0].y);
  const leftOpen = canMoveParticleTo(state, occupied, targets[1].x, targets[1].y);
  const rightOpen = canMoveParticleTo(state, occupied, targets[2].x, targets[2].y);

  if (downOpen) {
    return targets[0];
  }
  if (leftOpen && !rightOpen) {
    return targets[1];
  }
  if (!leftOpen && rightOpen) {
    return targets[2];
  }
  if (leftOpen && rightOpen) {
    return state.random() < 0.5 ? targets[1] : targets[2];
  }
  return null;
}

function pushSplash(state, x, y) {
  const left = x - 1;
  const right = x + 1;
  if (left >= BOTTOM_RANGE.minX) {
    state.splashes.push({ x: left, y, ttl: 2 });
  }
  if (right <= BOTTOM_RANGE.maxX) {
    state.splashes.push({ x: right, y, ttl: 2 });
  }
}

function advanceParticles(state) {
  if (state.particles.length === 0) {
    return;
  }

  const occupied = buildOccupiedParticleSet(state.particles);
  const nextParticles = [];
  const sorted = state.particles.slice().sort((a, b) => b.y - a.y || a.x - b.x);

  for (let index = 0; index < sorted.length; index += 1) {
    const particle = sorted[index];
    occupied.delete(`${particle.x},${particle.y}`);

    const target = chooseParticleTarget(state, occupied, particle);
    if (target) {
      particle.x = target.x;
      particle.y = target.y;
      occupied.add(`${particle.x},${particle.y}`);
      nextParticles.push(particle);
      continue;
    }

    if (
      particle.y >= BOTTOM_RANGE.minY &&
      particle.y <= BOTTOM_RANGE.maxY &&
      canReceive(state.bottomMask, state.bottomDepthGrid, particle.x, particle.y)
    ) {
      state.bottomDepthGrid[indexOf(particle.x, particle.y)] += 1;
      pushSplash(state, particle.x, particle.y);
      continue;
    }

    occupied.add(`${particle.x},${particle.y}`);
    nextParticles.push(particle);
  }

  state.particles = nextParticles.slice(0, MAX_STREAM_PARTICLES);
}

function updateSplashes(state) {
  const next = [];
  for (let index = 0; index < state.splashes.length; index += 1) {
    const splash = state.splashes[index];
    const ttl = splash.ttl - 1;
    if (ttl <= 0) {
      continue;
    }
    next.push({ x: splash.x, y: splash.y, ttl });
  }
  state.splashes = next;
}

function scheduleRelease(state, frameIndex) {
  if (!state.hasFlow) {
    return;
  }
  if (frameIndex >= state.drainFrames) {
    return;
  }
  state.releaseAccumulator += state.releaseRatePerFrame;
  while (
    state.releaseAccumulator >= 1 &&
    state.particles.length < MAX_STREAM_PARTICLES
  ) {
    const source = findTopReleaseSource(state.topDepthGrid);
    if (!source) {
      state.releaseAccumulator = 0;
      break;
    }
    state.topDepthGrid[indexOf(source.x, source.y)] -= 1;
    state.particles.push({ x: NECK_X, y: NECK_Y });
    state.releaseAccumulator -= 1;
  }
}

function drawHourglassOutline(frameMap) {
  drawPolygon(frameMap, TOP_POLYGON, COLORS.outline);
  drawPolygon(frameMap, BOTTOM_POLYGON, COLORS.outline);
  drawLine(frameMap, 31, 28, 31, 30, COLORS.outline);
  drawLine(frameMap, 33, 28, 33, 30, COLORS.outline);
}

function isSurface(depthGrid, mask, x, y) {
  if (!hasSand(depthGrid, x, y)) {
    return false;
  }
  if (y - 1 < 0) {
    return true;
  }
  const aboveIndex = indexOf(x, y - 1);
  if (mask[aboveIndex] !== 1) {
    return true;
  }
  return depthGrid[aboveIndex] === 0;
}

function drawDepthGrid(frameMap, depthGrid, mask, cells, frameIndex) {
  for (let index = 0; index < cells.length; index += 1) {
    const cell = cells[index];
    if (!hasSand(depthGrid, cell.x, cell.y)) {
      continue;
    }
    const highlight =
      isSurface(depthGrid, mask, cell.x, cell.y) ||
      ((cell.x * 11 + cell.y * 7 + frameIndex) % 17 === 0);
    setPixel(frameMap, cell.x, cell.y, highlight ? COLORS.sandBright : COLORS.sand);
  }
}

function drawParticles(frameMap, state) {
  for (let index = 0; index < state.particles.length; index += 1) {
    const particle = state.particles[index];
    setPixel(frameMap, particle.x, particle.y, index % 2 === 0 ? COLORS.sandBright : COLORS.sand);
  }
  for (let index = 0; index < state.splashes.length; index += 1) {
    const splash = state.splashes[index];
    setPixel(frameMap, splash.x, splash.y, COLORS.sandBright);
  }
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
  drawTinyText(frameMap, textConfig.text, textX, 55, COLORS.text, textConfig.size);
}

function resolvePreviewFrameCount(totalSeconds) {
  if (totalSeconds <= 0) {
    return PREVIEW_MIN_FRAME_COUNT;
  }
  const estimated = Math.round((totalSeconds * 1000) / PREVIEW_TARGET_INTERVAL_MS);
  return clampInt(estimated, PREVIEW_MIN_FRAME_COUNT, PREVIEW_MAX_FRAME_COUNT);
}

function createSandState(config, frameCount) {
  const totalSeconds = resolveTotalSeconds(config);
  const remainingSeconds = resolveCountdownBoardRemainingSeconds(config);
  const ratio = totalSeconds > 0 ? Math.max(0, Math.min(1, remainingSeconds / totalSeconds)) : 0;
  const topCells = collectCellsInPolygon(TOP_POLYGON, TOP_RANGE);
  const bottomCells = collectCellsInPolygon(BOTTOM_POLYGON, BOTTOM_RANGE);
  const totalCapacity = (topCells.length + bottomCells.length) * STACK_DEPTH;
  const grainTotal = clampInt(
    Math.round(totalCapacity * SAND_FILL_RATIO),
    Math.min(totalCapacity, 180),
    totalCapacity,
  );
  const topCount = clampInt(Math.round(grainTotal * ratio), 0, grainTotal);
  const bottomCount = grainTotal - topCount;
  const drainFrames = Math.max(1, frameCount - FLOW_SETTLE_FRAMES);

  return {
    random: createSeededRandom(hashConfig(config)),
    topCells,
    bottomCells,
    topMask: createMask(topCells),
    bottomMask: createMask(bottomCells),
    topDepthGrid: buildInitialDepthGrid(topCells, topCount, true),
    bottomDepthGrid: buildInitialDepthGrid(bottomCells, bottomCount, true),
    particles: [],
    splashes: [],
    hasFlow: totalSeconds > 0 && remainingSeconds > 0,
    drainFrames,
    releaseAccumulator: 0,
    releaseRatePerFrame: topCount / drainFrames,
  };
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
    scheduleRelease(state, frameIndex);
    for (let step = 0; step < PHYSICS_STEPS_PER_FRAME; step += 1) {
      settleChamber(state.topDepthGrid, state.topMask, TOP_RANGE, state.random);
      settleChamber(state.bottomDepthGrid, state.bottomMask, BOTTOM_RANGE, state.random);
      advanceParticles(state);
    }
    updateSplashes(state);

    const frameMap = new Map();
    drawDepthGrid(frameMap, state.topDepthGrid, state.topMask, state.topCells, frameIndex);
    drawDepthGrid(frameMap, state.bottomDepthGrid, state.bottomMask, state.bottomCells, frameIndex);
    drawParticles(frameMap, state);
    drawHourglassOutline(frameMap);
    drawTimeText(frameMap, normalized);
    frames.push(frameMap);
  }

  return frames;
}
