import { clamp, fillRect, normalizeSpeed } from "./common.js";

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state = (state * 1664525 + 1013904223) >>> 0;
    return state / 0x100000000;
  };
}

function hsvToRgb(hue) {
  let safeHue = hue % 360;
  if (safeHue < 0) {
    safeHue += 360;
  }
  const chroma = 0.9;
  const x = chroma * (1 - Math.abs(((safeHue / 60) % 2) - 1));

  if (safeHue < 60) {
    return { r: chroma * 255, g: x * 255, b: 0 };
  }
  if (safeHue < 120) {
    return { r: x * 255, g: chroma * 255, b: 0 };
  }
  if (safeHue < 180) {
    return { r: 0, g: chroma * 255, b: x * 255 };
  }
  if (safeHue < 240) {
    return { r: 0, g: x * 255, b: chroma * 255 };
  }
  if (safeHue < 300) {
    return { r: x * 255, g: 0, b: chroma * 255 };
  }
  return { r: chroma * 255, g: 0, b: x * 255 };
}

function resolveSnakeWidth(options) {
  if (!options || !Number.isFinite(Number(options.snakeWidth))) {
    return 2;
  }
  return clamp(Math.round(Number(options.snakeWidth)), 1, 4);
}

function resolveCellSize(sizeLevel) {
  return clamp(Math.round(sizeLevel), 1, 4);
}

function toKey(x, y) {
  return `${x},${y}`;
}

function clonePoint(point) {
  return { x: point.x, y: point.y };
}

function drawBoardFrame(map, originX, originY, boardWidth, boardHeight) {
  const left = Math.max(0, originX - 1);
  const top = Math.max(0, originY - 1);
  const right = Math.min(63, originX + boardWidth);
  const bottom = Math.min(63, originY + boardHeight);

  for (let x = left; x <= right; x += 1) {
    map.set(`${x},${top}`, "rgb(24, 28, 38)");
    map.set(`${x},${bottom}`, "rgb(24, 28, 38)");
  }
  for (let y = top; y <= bottom; y += 1) {
    map.set(`${left},${y}`, "rgb(24, 28, 38)");
    map.set(`${right},${y}`, "rgb(24, 28, 38)");
  }
}

function drawCell(map, x, y, originX, originY, cellSize, color) {
  fillRect(
    map,
    originX + x * cellSize,
    originY + y * cellSize,
    cellSize,
    cellSize,
    color.r,
    color.g,
    color.b,
  );
}

function getNextPosition(position, direction) {
  if (direction === "UP") {
    return { x: position.x, y: position.y - 1 };
  }
  if (direction === "DOWN") {
    return { x: position.x, y: position.y + 1 };
  }
  if (direction === "LEFT") {
    return { x: position.x - 1, y: position.y };
  }
  return { x: position.x + 1, y: position.y };
}

function isOppositeDirection(currentDirection, nextDirection) {
  return (
    (currentDirection === "UP" && nextDirection === "DOWN") ||
    (currentDirection === "DOWN" && nextDirection === "UP") ||
    (currentDirection === "LEFT" && nextDirection === "RIGHT") ||
    (currentDirection === "RIGHT" && nextDirection === "LEFT")
  );
}

function checkCollision(position, snake, gridWidth, gridHeight) {
  if (position.x < 0 || position.x >= gridWidth || position.y < 0 || position.y >= gridHeight) {
    return true;
  }
  for (let index = 0; index < snake.length; index += 1) {
    if (snake[index].x === position.x && snake[index].y === position.y) {
      return true;
    }
  }
  return false;
}

function buildDistanceMap(target, snake, gridWidth, gridHeight) {
  const map = Array.from({ length: gridWidth }, () => Array(gridHeight).fill(-1));
  const queue = [clonePoint(target)];
  map[target.x][target.y] = 0;
  let queueIndex = 0;

  while (queueIndex < queue.length) {
    const current = queue[queueIndex];
    queueIndex += 1;
    const directions = ["UP", "DOWN", "LEFT", "RIGHT"];
    for (let index = 0; index < directions.length; index += 1) {
      const next = getNextPosition(current, directions[index]);
      if (next.x < 0 || next.x >= gridWidth || next.y < 0 || next.y >= gridHeight) {
        continue;
      }
      if (map[next.x][next.y] !== -1) {
        continue;
      }

      let blocked = false;
      for (let segmentIndex = 0; segmentIndex < snake.length; segmentIndex += 1) {
        if (snake[segmentIndex].x === next.x && snake[segmentIndex].y === next.y) {
          blocked = true;
          break;
        }
      }
      if (blocked) {
        continue;
      }

      map[next.x][next.y] = map[current.x][current.y] + 1;
      queue.push(next);
    }
  }

  return map;
}

function willCauseSelfTrap(direction, snake, gridWidth, gridHeight) {
  const next = getNextPosition(snake[0], direction);
  if (checkCollision(next, snake, gridWidth, gridHeight)) {
    return true;
  }

  let availableMoves = 0;
  const directions = ["UP", "DOWN", "LEFT", "RIGHT"];
  for (let index = 0; index < directions.length; index += 1) {
    const test = getNextPosition(next, directions[index]);
    if (!checkCollision(test, snake, gridWidth, gridHeight)) {
      availableMoves += 1;
    }
  }
  return availableMoves < 2;
}

function findPathDirection(currentDirection, snake, food, gridWidth, gridHeight) {
  const distanceMap = buildDistanceMap(food, snake, gridWidth, gridHeight);
  const directions = ["UP", "DOWN", "LEFT", "RIGHT"];
  let bestDirection = currentDirection;
  let bestDistance = Number.MAX_SAFE_INTEGER;

  for (let index = 0; index < directions.length; index += 1) {
    const direction = directions[index];
    if (isOppositeDirection(currentDirection, direction)) {
      continue;
    }
    const next = getNextPosition(snake[0], direction);
    if (checkCollision(next, snake, gridWidth, gridHeight)) {
      continue;
    }
    if (distanceMap[next.x][next.y] !== -1 && distanceMap[next.x][next.y] < bestDistance) {
      bestDistance = distanceMap[next.x][next.y];
      bestDirection = direction;
    }
  }

  if (!isOppositeDirection(currentDirection, bestDirection) &&
      !willCauseSelfTrap(bestDirection, snake, gridWidth, gridHeight)) {
    return bestDirection;
  }
  return currentDirection;
}

function generateFood(rand, snake, gridWidth, gridHeight) {
  const occupied = new Set();
  for (let index = 0; index < snake.length; index += 1) {
    occupied.add(toKey(snake[index].x, snake[index].y));
  }

  const candidates = [];
  for (let y = 0; y < gridHeight; y += 1) {
    for (let x = 0; x < gridWidth; x += 1) {
      if (!occupied.has(toKey(x, y))) {
        candidates.push({ x, y });
      }
    }
  }

  if (candidates.length === 0) {
    return null;
  }
  return candidates[Math.floor(rand() * candidates.length)];
}

function getSnakeColor(segmentIndex, frameCounter) {
  const hue = (segmentIndex * 30 + frameCounter * 0.8) % 360;
  const rgb = hsvToRgb(hue);
  return {
    r: Math.round(rgb.r),
    g: Math.round(rgb.g),
    b: Math.round(rgb.b),
  };
}

function getFoodColor(frameCounter) {
  const pulse = (Math.sin(frameCounter * 0.2) + 1) / 2;
  const intensity = Math.round(150 + pulse * 105);
  return {
    r: intensity,
    g: 50,
    b: 50,
  };
}

export function buildSnakeDemoMaps(speed, intensity, options) {
  const safeSpeed = normalizeSpeed(speed);
  const snakeWidth = resolveSnakeWidth(options);
  const cellSize = resolveCellSize(snakeWidth);
  const gridWidth = Math.max(8, Math.floor(64 / cellSize));
  const gridHeight = Math.max(8, Math.floor(64 / cellSize));
  const boardWidth = gridWidth * cellSize;
  const boardHeight = gridHeight * cellSize;
  const originX = Math.floor((64 - boardWidth) / 2);
  const originY = Math.floor((64 - boardHeight) / 2);
  const rand = createSeededRandom(20260423 + safeSpeed * 17 + snakeWidth * 31);
  const maps = [];
  const maxFrames = 900;
  const updateEvery = Math.max(2, Math.round((0.2 + Math.max(0, 10 - safeSpeed) * 0.05) * 30));

  let snake = [];
  let food = null;
  let currentDirection = "RIGHT";
  let nextDirection = "RIGHT";
  let gameOver = false;
  let gameWon = false;
  let score = 0;
  let level = 1;
  let gameSpeed = 0.2;
  let deathAnimationFrame = 0;
  let winAnimationFrame = 0;
  let frameCounter = 0;

  function initializeGame() {
    snake = [];
    gameOver = false;
    gameWon = false;
    score = 0;
    level = 1;
    currentDirection = "RIGHT";
    nextDirection = "RIGHT";
    deathAnimationFrame = 0;
    winAnimationFrame = 0;
    frameCounter = 0;

    const startX = Math.floor(gridWidth / 2);
    const startY = Math.floor(gridHeight / 2);
    snake.push({ x: startX, y: startY });
    snake.push({ x: startX - 1, y: startY });
    snake.push({ x: startX - 2, y: startY });
    food = generateFood(rand, snake, gridWidth, gridHeight);
    gameSpeed = 0.2;
  }

  function calculateLevel() {
    level = Math.floor(score / 5) + 1;
    gameSpeed = Math.min(0.2 + (level - 1) * 0.05, 0.8);
  }

  function moveSnake() {
    const head = snake[0];
    const nextHead = getNextPosition(head, currentDirection);
    const bodyWithoutTail = snake.slice(0, snake.length - 1);

    if (checkCollision(nextHead, bodyWithoutTail, gridWidth, gridHeight)) {
      gameOver = true;
      deathAnimationFrame = 0;
      return;
    }

    snake.unshift(nextHead);
    if (food && nextHead.x === food.x && nextHead.y === food.y) {
      score += 1;
      food = generateFood(rand, snake, gridWidth, gridHeight);
      calculateLevel();
    } else {
      snake.pop();
    }

    const reasonableMaxSize = Math.min(gridWidth * gridHeight - 1, Math.floor(gridWidth * gridHeight * 0.75));
    if (snake.length >= reasonableMaxSize) {
      gameWon = true;
      winAnimationFrame = 0;
    }
  }

  initializeGame();

  while (maps.length < maxFrames) {
    const map = new Map();
    drawBoardFrame(map, originX, originY, boardWidth, boardHeight);

    frameCounter += 1;

    if (gameOver) {
      deathAnimationFrame += 1;
      const flashIntensity = deathAnimationFrame % 10 < 5 ? 255 : 100;
      for (let y = 0; y < gridHeight; y += 1) {
        for (let x = 0; x < gridWidth; x += 1) {
          drawCell(map, x, y, originX, originY, cellSize, { r: flashIntensity, g: 0, b: 0 });
        }
      }
      for (let index = 0; index < snake.length; index += 1) {
        drawCell(map, snake[index].x, snake[index].y, originX, originY, cellSize, { r: 255, g: 255, b: 255 });
      }
      if (deathAnimationFrame >= 60) {
        initializeGame();
      }
      maps.push(map);
      continue;
    }

    if (gameWon) {
      winAnimationFrame += 1;
      const hueOffset = winAnimationFrame * 0.1;
      for (let y = 0; y < gridHeight; y += 1) {
        for (let x = 0; x < gridWidth; x += 1) {
          const rgb = hsvToRgb((hueOffset * 180 + (x + y) * 20) % 360);
          drawCell(map, x, y, originX, originY, cellSize, {
            r: Math.round(rgb.r * 0.8),
            g: Math.round(rgb.g * 0.8),
            b: Math.round(rgb.b * 0.8),
          });
        }
      }
      if (winAnimationFrame > 120) {
        initializeGame();
      }
      maps.push(map);
      continue;
    }

    if (frameCounter % Math.max(1, Math.round(updateEvery * gameSpeed)) === 0) {
      if (food) {
        nextDirection = findPathDirection(currentDirection, snake, food, gridWidth, gridHeight);
      }
      currentDirection = nextDirection;
      moveSnake();
    }

    for (let index = 0; index < snake.length; index += 1) {
      const color = getSnakeColor(index, frameCounter);
      drawCell(map, snake[index].x, snake[index].y, originX, originY, cellSize, color);
    }
    if (food) {
      drawCell(map, food.x, food.y, originX, originY, cellSize, getFoodColor(frameCounter));
    }
    maps.push(map);
  }

  return maps;
}
