import {
  drawClockTextToPixels,
  getClockFontOptions,
  getClockTextHeight,
  getClockTextWidth,
  getCurrentTimeText,
} from "../clockCanvas.js";
import { clamp, fillRect, normalizeSpeed, setPixel } from "./common.js";

const PANEL_SIZE = 64;
const CLOCK_TEXT_TOP = 2;
const CLOCK_PADDING_X = 1;
const CLOCK_PADDING_Y = 1;
const DEFAULT_FONT = "minimal_3x5";
const DEFAULT_SNAKE_SKIN = "gradient";
const CLOCK_FONT_IDS = getClockFontOptions().map((item) => item.id);
const DIRECTIONS = [
  { x: 0, y: -1 },
  { x: 1, y: 0 },
  { x: 0, y: 1 },
  { x: -1, y: 0 },
];

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state = (state * 1664525 + 1013904223) >>> 0;
    return state / 0x100000000;
  };
}

function positiveModulo(value, base) {
  if (base <= 0) {
    return 0;
  }
  const result = value % base;
  return result < 0 ? result + base : result;
}

function normalizeHexColor(value) {
  if (typeof value !== "string") {
    throw new Error("snake color value is invalid");
  }
  const body = value.trim().replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    throw new Error("snake color value is invalid");
  }
  return `#${body.toLowerCase()}`;
}

function hexToRgb(hex) {
  const normalized = normalizeHexColor(hex);
  return {
    r: parseInt(normalized.slice(1, 3), 16),
    g: parseInt(normalized.slice(3, 5), 16),
    b: parseInt(normalized.slice(5, 7), 16),
  };
}

function resolveSnakeWidth(options) {
  if (!options || !Number.isFinite(Number(options.snakeWidth))) {
    return 2;
  }
  return clamp(Math.round(Number(options.snakeWidth)), 2, 4);
}

function resolveSnakeColor(options) {
  return hexToRgb(options.snakeColor);
}

function resolveFoodColor(options) {
  return hexToRgb(options.foodColor);
}

function resolveShowSeconds(options) {
  return options && options.showSeconds === true;
}

function resolveFont(options) {
  if (!options || typeof options.font !== "string") {
    return DEFAULT_FONT;
  }
  return CLOCK_FONT_IDS.includes(options.font) ? options.font : DEFAULT_FONT;
}

function resolveSnakeSkin(options) {
  if (!options || typeof options.snakeSkin !== "string") {
    return DEFAULT_SNAKE_SKIN;
  }
  if (
    options.snakeSkin === "solid" ||
    options.snakeSkin === "gradient" ||
    options.snakeSkin === "spotted"
  ) {
    return options.snakeSkin;
  }
  return DEFAULT_SNAKE_SKIN;
}

function buildClockLayout(font, showSeconds) {
  const sampleText = showSeconds ? "88:88:88" : "88:88";
  const textWidth = getClockTextWidth(sampleText, font, 1);
  const textHeight = getClockTextHeight(font, 1);
  const textX = Math.floor((PANEL_SIZE - textWidth) / 2);
  const textY = CLOCK_TEXT_TOP;
  return {
    active: true,
    font,
    textX,
    textY,
    reservedLeft: clamp(textX - CLOCK_PADDING_X, 0, PANEL_SIZE - 1),
    reservedTop: clamp(textY - CLOCK_PADDING_Y, 0, PANEL_SIZE - 1),
    reservedRight: clamp(
      textX + textWidth - 1 + CLOCK_PADDING_X,
      0,
      PANEL_SIZE - 1,
    ),
    reservedBottom: clamp(
      textY + textHeight - 1 + CLOCK_PADDING_Y,
      0,
      PANEL_SIZE - 1,
    ),
  };
}

function isReservedCell(x, y, size, originX, originY, clockLayout) {
  if (!clockLayout || !clockLayout.active) {
    return false;
  }

  const cellLeft = originX + x * size;
  const cellTop = originY + y * size;
  const cellRight = cellLeft + size - 1;
  const cellBottom = cellTop + size - 1;

  return !(
    cellRight < clockLayout.reservedLeft ||
    cellLeft > clockLayout.reservedRight ||
    cellBottom < clockLayout.reservedTop ||
    cellTop > clockLayout.reservedBottom
  );
}

function countPlayableCells(gridWidth, gridHeight, size, originX, originY, clockLayout) {
  let count = 0;
  for (let y = 0; y < gridHeight; y += 1) {
    for (let x = 0; x < gridWidth; x += 1) {
      if (!isReservedCell(x, y, size, originX, originY, clockLayout)) {
        count += 1;
      }
    }
  }
  return count;
}

function isSnakeCellOccupied(snake, x, y, ignoreTailCount = 0) {
  const limit = snake.length - ignoreTailCount;
  for (let index = 0; index < limit; index += 1) {
    if (snake[index].x === x && snake[index].y === y) {
      return true;
    }
  }
  return false;
}

function placeSnakeFood(
  rand,
  snake,
  gridWidth,
  gridHeight,
  size,
  originX,
  originY,
  clockLayout,
) {
  const candidates = [];
  for (let y = 0; y < gridHeight; y += 1) {
    for (let x = 0; x < gridWidth; x += 1) {
      if (isReservedCell(x, y, size, originX, originY, clockLayout)) {
        continue;
      }
      if (!isSnakeCellOccupied(snake, x, y, 0)) {
        candidates.push({ x, y });
      }
    }
  }

  if (candidates.length === 0) {
    return null;
  }

  return candidates[Math.floor(rand() * candidates.length)];
}

function buildSnakeDistanceMap(
  food,
  snake,
  gridWidth,
  gridHeight,
  size,
  originX,
  originY,
  clockLayout,
) {
  if (!food) {
    return null;
  }

  const distance = Array.from({ length: gridHeight }, () =>
    Array(gridWidth).fill(-1),
  );
  const visited = Array.from({ length: gridHeight }, () =>
    Array(gridWidth).fill(false),
  );
  const queue = [{ x: food.x, y: food.y }];

  visited[food.y][food.x] = true;
  distance[food.y][food.x] = 0;

  for (let index = 0; index < queue.length; index += 1) {
    const current = queue[index];
    for (let dirIndex = 0; dirIndex < DIRECTIONS.length; dirIndex += 1) {
      const direction = DIRECTIONS[dirIndex];
      const nextX = current.x + direction.x;
      const nextY = current.y + direction.y;
      if (nextX < 0 || nextX >= gridWidth || nextY < 0 || nextY >= gridHeight) {
        continue;
      }
      if (isReservedCell(nextX, nextY, size, originX, originY, clockLayout)) {
        continue;
      }
      if (isSnakeCellOccupied(snake, nextX, nextY, 1)) {
        continue;
      }
      if (visited[nextY][nextX]) {
        continue;
      }

      visited[nextY][nextX] = true;
      distance[nextY][nextX] = distance[current.y][current.x] + 1;
      queue.push({ x: nextX, y: nextY });
    }
  }

  return distance;
}

function canSnakeMoveTo(
  snake,
  food,
  x,
  y,
  gridWidth,
  gridHeight,
  size,
  originX,
  originY,
  clockLayout,
) {
  if (x < 0 || x >= gridWidth || y < 0 || y >= gridHeight) {
    return false;
  }
  if (isReservedCell(x, y, size, originX, originY, clockLayout)) {
    return false;
  }
  const eating = !!food && food.x === x && food.y === y;
  return !isSnakeCellOccupied(snake, x, y, eating ? 0 : 1);
}

function pickSnakeDirection(
  snake,
  food,
  gridWidth,
  gridHeight,
  size,
  originX,
  originY,
  clockLayout,
) {
  const distanceMap = buildSnakeDistanceMap(
    food,
    snake,
    gridWidth,
    gridHeight,
    size,
    originX,
    originY,
    clockLayout,
  );
  let bestDirection = -1;
  let bestDistance = Number.MAX_SAFE_INTEGER;

  for (let dirIndex = 0; dirIndex < DIRECTIONS.length; dirIndex += 1) {
    const direction = DIRECTIONS[dirIndex];
    const nextX = snake[0].x + direction.x;
    const nextY = snake[0].y + direction.y;
    if (
      !canSnakeMoveTo(
        snake,
        food,
        nextX,
        nextY,
        gridWidth,
        gridHeight,
        size,
        originX,
        originY,
        clockLayout,
      )
    ) {
      continue;
    }

    const nextDistance =
      distanceMap &&
      nextY >= 0 &&
      nextY < gridHeight &&
      nextX >= 0 &&
      nextX < gridWidth
        ? distanceMap[nextY][nextX]
        : -1;

    if (nextDistance >= 0 && nextDistance < bestDistance) {
      bestDistance = nextDistance;
      bestDirection = dirIndex;
    }
  }

  if (bestDirection >= 0) {
    return bestDirection;
  }

  for (let dirIndex = 0; dirIndex < DIRECTIONS.length; dirIndex += 1) {
    const direction = DIRECTIONS[dirIndex];
    const nextX = snake[0].x + direction.x;
    const nextY = snake[0].y + direction.y;
    if (
      canSnakeMoveTo(
        snake,
        food,
        nextX,
        nextY,
        gridWidth,
        gridHeight,
        size,
        originX,
        originY,
        clockLayout,
      )
    ) {
      return dirIndex;
    }
  }

  return -1;
}

function drawGlowPixel(map, x, y, r, g, b) {
  setPixel(map, x, y, r, g, b);
}

function drawSnakeCellBorder(map, x, y, size, color, emphasize) {
  if (size < 2) {
    return;
  }

  const light = {
    r: clamp(Math.round(color.r + (emphasize ? 56 : 28)), 0, 255),
    g: clamp(Math.round(color.g + (emphasize ? 56 : 28)), 0, 255),
    b: clamp(Math.round(color.b + (emphasize ? 56 : 28)), 0, 255),
  };
  const dark = {
    r: clamp(Math.round(color.r - (emphasize ? 34 : 18)), 0, 255),
    g: clamp(Math.round(color.g - (emphasize ? 34 : 18)), 0, 255),
    b: clamp(Math.round(color.b - (emphasize ? 34 : 18)), 0, 255),
  };

  for (let offset = 0; offset < size; offset += 1) {
    drawGlowPixel(map, x + offset, y, light.r, light.g, light.b);
    drawGlowPixel(map, x, y + offset, light.r, light.g, light.b);
    drawGlowPixel(map, x + offset, y + size - 1, dark.r, dark.g, dark.b);
    drawGlowPixel(map, x + size - 1, y + offset, dark.r, dark.g, dark.b);
  }
}

function drawSnakeCellSpots(map, x, y, size, color, patternSeed) {
  if (size < 2) {
    return;
  }

  const spot = {
    r: clamp(Math.round(color.r - 46), 0, 255),
    g: clamp(Math.round(color.g - 46), 0, 255),
    b: clamp(Math.round(color.b - 46), 0, 255),
  };
  const spotCount = size >= 4 ? 3 : 2;

  for (let index = 0; index < spotCount; index += 1) {
    const seed = patternSeed + index * 19;
    const localX = positiveModulo(seed * 3 + 1, size);
    const localY = positiveModulo(seed * 5 + 2, size);
    drawGlowPixel(map, x + localX, y + localY, spot.r, spot.g, spot.b);
    if (size >= 4 && index === 0) {
      const mirrorX = positiveModulo(localX + 1, size);
      drawGlowPixel(map, x + mirrorX, y + localY, spot.r, spot.g, spot.b);
    }
  }
}

function drawSnakeSegment(map, x, y, size, color, emphasize, drawSpots, patternSeed) {
  fillRect(map, x, y, size, size, color.r, color.g, color.b);

  if (size === 1) {
    const halo = emphasize
      ? {
          r: clamp(Math.round(color.r / 2 + 48), 0, 255),
          g: clamp(Math.round(color.g / 2 + 48), 0, 255),
          b: clamp(Math.round(color.b / 2 + 48), 0, 255),
        }
      : {
          r: clamp(Math.round(color.r / 3), 0, 255),
          g: clamp(Math.round(color.g / 3), 0, 255),
          b: clamp(Math.round(color.b / 3), 0, 255),
        };

    const orthogonalOffsets = [
      { x: -1, y: 0 },
      { x: 1, y: 0 },
      { x: 0, y: -1 },
      { x: 0, y: 1 },
    ];

    for (let index = 0; index < orthogonalOffsets.length; index += 1) {
      const offset = orthogonalOffsets[index];
      drawGlowPixel(map, x + offset.x, y + offset.y, halo.r, halo.g, halo.b);
    }

    if (!emphasize) {
      return;
    }

    const corner = {
      r: clamp(Math.round(halo.r / 2), 0, 255),
      g: clamp(Math.round(halo.g / 2), 0, 255),
      b: clamp(Math.round(halo.b / 2), 0, 255),
    };
    const diagonalOffsets = [
      { x: -1, y: -1 },
      { x: 1, y: -1 },
      { x: -1, y: 1 },
      { x: 1, y: 1 },
    ];

    for (let index = 0; index < diagonalOffsets.length; index += 1) {
      const offset = diagonalOffsets[index];
      drawGlowPixel(map, x + offset.x, y + offset.y, corner.r, corner.g, corner.b);
    }
    return;
  }

  drawSnakeCellBorder(map, x, y, size, color, emphasize);
  if (drawSpots) {
    drawSnakeCellSpots(map, x, y, size, color, patternSeed);
  }
}

function getSnakeSegmentColor(segmentIndex, snakeColor, snakeLength, snakeSkin, dead, won) {
  if (dead) {
    return { r: 255, g: 80, b: 80 };
  }
  if (won) {
    return { r: 120, g: 210, b: 255 };
  }

  const color = {
    r: snakeColor.r,
    g: snakeColor.g,
    b: snakeColor.b,
  };

  if (snakeSkin === "gradient" && snakeLength > 1) {
    const gradientSpan = Math.max(1, snakeLength - 1);
    const factorPercent = 120 - Math.floor((segmentIndex * 55) / gradientSpan);
    color.r = clamp(Math.round((color.r * factorPercent) / 100), 0, 255);
    color.g = clamp(Math.round((color.g * factorPercent) / 100), 0, 255);
    color.b = clamp(Math.round((color.b * factorPercent) / 100), 0, 255);
  }

  if (segmentIndex === 0) {
    color.r = clamp(Math.round(color.r + (255 - color.r) * 0.35), 0, 255);
    color.g = clamp(Math.round(color.g + (255 - color.g) * 0.35), 0, 255);
    color.b = clamp(Math.round(color.b + (255 - color.b) * 0.35), 0, 255);
  }

  return color;
}

function drawClock(map, clockLayout, font, showSeconds) {
  const timeText = getCurrentTimeText(showSeconds, 24);
  drawClockTextToPixels(
    timeText,
    clockLayout.textX,
    clockLayout.textY,
    "#ffffff",
    map,
    font,
    1,
    "left",
  );
}

export function buildSnakeDemoMaps(speed, intensity, options) {
  const safeSpeed = normalizeSpeed(speed);
  const snakeWidth = resolveSnakeWidth(options);
  const snakeColor = resolveSnakeColor(options);
  const foodColor = resolveFoodColor(options);
  const showSeconds = resolveShowSeconds(options);
  const font = resolveFont(options);
  const snakeSkin = resolveSnakeSkin(options);
  const gridWidth = Math.floor(PANEL_SIZE / snakeWidth);
  const gridHeight = Math.floor(PANEL_SIZE / snakeWidth);
  const originX = Math.floor((PANEL_SIZE - gridWidth * snakeWidth) / 2);
  const originY = Math.floor((PANEL_SIZE - gridHeight * snakeWidth) / 2);
  const clockLayout = buildClockLayout(font, showSeconds);
  const playableCellCount = countPlayableCells(
    gridWidth,
    gridHeight,
    snakeWidth,
    originX,
    originY,
    clockLayout,
  );
  const rand = createSeededRandom(20260423 + safeSpeed * 17 + snakeWidth * 31);
  const maps = [];
  const maxFrames = 900;
  const updateEvery = Math.max(
    2,
    Math.round((0.2 + Math.max(0, 10 - safeSpeed) * 0.05) * 30),
  );

  let snake = [];
  let food = null;
  let direction = 1;
  let dead = false;
  let won = false;
  let holdTicks = 0;
  let frameCounter = 0;

  function initSnake() {
    const startX = Math.floor(gridWidth / 2);
    const startY = Math.floor(gridHeight / 2);
    snake = [
      { x: startX, y: startY },
      { x: startX - 1, y: startY },
      { x: startX - 2, y: startY },
      { x: startX - 3, y: startY },
    ];
    food = placeSnakeFood(
      rand,
      snake,
      gridWidth,
      gridHeight,
      snakeWidth,
      originX,
      originY,
      clockLayout,
    );
    direction = 1;
    dead = false;
    won = false;
    holdTicks = 0;
  }

  function updateSnake() {
    if (won || dead) {
      holdTicks += 1;
      if (holdTicks >= 20) {
        initSnake();
      }
      return;
    }

    const nextDirection = pickSnakeDirection(
      snake,
      food,
      gridWidth,
      gridHeight,
      snakeWidth,
      originX,
      originY,
      clockLayout,
    );
    if (nextDirection < 0) {
      dead = true;
      holdTicks = 0;
      return;
    }

    direction = nextDirection;
    const nextHead = {
      x: snake[0].x + DIRECTIONS[direction].x,
      y: snake[0].y + DIRECTIONS[direction].y,
    };
    const eating = !!food && nextHead.x === food.x && nextHead.y === food.y;

    snake.unshift(nextHead);
    if (eating) {
      if (snake.length >= playableCellCount) {
        won = true;
        holdTicks = 0;
      } else {
        food = placeSnakeFood(
          rand,
          snake,
          gridWidth,
          gridHeight,
          snakeWidth,
          originX,
          originY,
          clockLayout,
        );
      }
      return;
    }

    snake.pop();
  }

  initSnake();

  while (maps.length < maxFrames) {
    const map = new Map();
    frameCounter += 1;

    if (frameCounter > 1 && frameCounter % updateEvery === 0) {
      updateSnake();
    }

    const drawSpots = snakeSkin === "spotted" && !dead && !won;
    for (let index = 0; index < snake.length; index += 1) {
      drawSnakeSegment(
        map,
        originX + snake[index].x * snakeWidth,
        originY + snake[index].y * snakeWidth,
        snakeWidth,
        getSnakeSegmentColor(
          index,
          snakeColor,
          snake.length,
          snakeSkin,
          dead,
          won,
        ),
        index === 0,
        drawSpots,
        snake[index].x * 31 + snake[index].y * 17 + index * 13,
      );
    }

    if (!won && food) {
      drawSnakeSegment(
        map,
        originX + food.x * snakeWidth,
        originY + food.y * snakeWidth,
        snakeWidth,
        foodColor,
        true,
        false,
        0,
      );
    }

    drawClock(map, clockLayout, font, showSeconds);
    maps.push(map);
  }

  return maps;
}
