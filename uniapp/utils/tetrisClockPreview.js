const PANEL_SIZE = 64;

const PIECE_COLORS = [
  { r: 0, g: 240, b: 240 },
  { r: 240, g: 240, b: 0 },
  { r: 160, g: 0, b: 240 },
  { r: 0, g: 240, b: 0 },
  { r: 240, g: 0, b: 0 },
  { r: 0, g: 0, b: 240 },
  { r: 240, g: 160, b: 0 },
];

const PIECE_SHAPES = {
  0: [
    [0, 1],
    [1, 1],
    [2, 1],
    [3, 1],
  ],
  1: [
    [0, 0],
    [1, 0],
    [0, 1],
    [1, 1],
  ],
  2: [
    [1, 0],
    [0, 1],
    [1, 1],
    [2, 1],
  ],
  3: [
    [1, 0],
    [2, 0],
    [0, 1],
    [1, 1],
  ],
  4: [
    [0, 0],
    [1, 0],
    [1, 1],
    [2, 1],
  ],
  5: [
    [0, 0],
    [0, 1],
    [1, 1],
    [2, 1],
  ],
  6: [
    [2, 0],
    [0, 1],
    [1, 1],
    [2, 1],
  ],
};

const DIGIT_PATTERNS = [
  ["111", "101", "101", "101", "111"],
  ["010", "110", "010", "010", "111"],
  ["111", "001", "111", "100", "111"],
  ["111", "001", "011", "001", "111"],
  ["101", "101", "111", "001", "001"],
  ["111", "100", "111", "001", "111"],
  ["111", "100", "111", "101", "111"],
  ["111", "001", "010", "010", "010"],
  ["111", "101", "111", "101", "111"],
  ["111", "101", "111", "001", "111"],
];

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function normalizeShape(cells) {
  let minX = Infinity;
  let minY = Infinity;
  for (let index = 0; index < cells.length; index += 1) {
    minX = Math.min(minX, cells[index][0]);
    minY = Math.min(minY, cells[index][1]);
  }
  return cells
    .map(([x, y]) => [x - minX, y - minY])
    .sort((left, right) => left[1] - right[1] || left[0] - right[0]);
}

function rotateShape(cells) {
  const rotated = cells.map(([x, y]) => [y, -x]);
  return normalizeShape(rotated);
}

function getPieceRotations(pieceType) {
  const base = PIECE_SHAPES[pieceType];
  const rotations = [];
  let current = normalizeShape(base);

  for (let index = 0; index < 4; index += 1) {
    const key = JSON.stringify(current);
    if (!rotations.some((item) => JSON.stringify(item) === key)) {
      rotations.push(current);
    }
    current = rotateShape(current);
  }

  return rotations;
}

function createMatrix(rows, cols, fillValue) {
  const matrix = [];
  for (let row = 0; row < rows; row += 1) {
    matrix.push(new Array(cols).fill(fillValue));
  }
  return matrix;
}

function setPixel(pixels, x, y, color) {
  if (x < 0 || x >= PANEL_SIZE || y < 0 || y >= PANEL_SIZE) {
    return;
  }
  pixels.set(`${x},${y}`, color);
}

function fillRect(pixels, x, y, width, height, color) {
  for (let row = 0; row < height; row += 1) {
    for (let col = 0; col < width; col += 1) {
      setPixel(pixels, x + col, y + row, color);
    }
  }
}

function rgbToHex(color) {
  const toHex = (value) => clamp(Math.round(value), 0, 255).toString(16).padStart(2, "0");
  return `#${toHex(color.r)}${toHex(color.g)}${toHex(color.b)}`;
}

function brighten(color, amount) {
  return {
    r: clamp(color.r + amount, 0, 255),
    g: clamp(color.g + amount, 0, 255),
    b: clamp(color.b + amount, 0, 255),
  };
}

function darkenHalf(color) {
  return {
    r: Math.floor(color.r / 2),
    g: Math.floor(color.g / 2),
    b: Math.floor(color.b / 2),
  };
}

function drawGridBackground(pixels, cellSize) {
  fillRect(pixels, 0, 0, PANEL_SIZE, PANEL_SIZE, "#000000");
  for (let pos = 0; pos < PANEL_SIZE; pos += cellSize) {
    for (let cursor = 0; cursor < PANEL_SIZE; cursor += 1) {
      setPixel(pixels, pos, cursor, "#0d1520");
      setPixel(pixels, cursor, pos, "#0d1520");
    }
  }
}

function drawGhostCell(pixels, cellX, cellY, cellSize) {
  const px = cellX * cellSize;
  const py = cellY * cellSize;
  for (let dy = 0; dy < cellSize; dy += 1) {
    for (let dx = 0; dx < cellSize; dx += 1) {
      let color = { r: 16, g: 32, b: 56 };
      if (cellSize >= 2 && (dy === 0 || dx === 0)) {
        color = { r: 28, g: 58, b: 92 };
      }
      setPixel(pixels, px + dx, py + dy, rgbToHex(color));
    }
  }
}

function drawPieceCell(pixels, cellX, cellY, cellSize, pieceType, pulseBright) {
  const rgb = PIECE_COLORS[pieceType];
  const baseBoost = pulseBright ? 20 : 0;
  const highlightBoost = pulseBright ? 60 : 40;
  const baseColor = brighten(rgb, baseBoost);
  const highlightColor = brighten(rgb, highlightBoost);
  const shadowColor = darkenHalf(rgb);
  const px = cellX * cellSize;
  const py = cellY * cellSize;

  for (let dy = 0; dy < cellSize; dy += 1) {
    for (let dx = 0; dx < cellSize; dx += 1) {
      let color = baseColor;
      if (cellSize >= 2) {
        if (dy === 0 || dx === 0) {
          color = highlightColor;
        } else if (dy === cellSize - 1 || dx === cellSize - 1) {
          color = shadowColor;
        }
      }
      setPixel(pixels, px + dx, py + dy, rgbToHex(color));
    }
  }
}

function getDropSpeed(config) {
  if (config.speed === "slow") {
    return 300;
  }
  if (config.speed === "fast") {
    return 80;
  }
  return 150;
}

function getClockSnapshot() {
  const now = new Date();
  return {
    hours: now.getHours(),
    minutes: now.getMinutes(),
  };
}

function buildTargetMap(cols, rows, showClock, clock) {
  const targetMap = createMatrix(rows, cols, false);
  if (!showClock) {
    return targetMap;
  }

  const digits = `${String(clock.hours).padStart(2, "0")}${String(clock.minutes).padStart(2, "0")}`;
  const glyphUnits = 17;
  let scale = 1;
  for (let tryScale = 1; tryScale <= 6; tryScale += 1) {
    if (glyphUnits * tryScale <= cols - 2 && 5 * tryScale <= rows - 2) {
      scale = tryScale;
    }
  }

  const totalWidth = glyphUnits * scale;
  const totalHeight = 5 * scale;
  const startX = Math.floor((cols - totalWidth) / 2);
  const startY = Math.floor((rows - totalHeight) / 2);
  let cursorX = startX;

  for (let digitIndex = 0; digitIndex < 4; digitIndex += 1) {
    const digit = Number(digits[digitIndex]);
    const pattern = DIGIT_PATTERNS[digit];
    for (let row = 0; row < 5; row += 1) {
      for (let col = 0; col < 3; col += 1) {
        if (pattern[row][col] !== "1") {
          continue;
        }
        for (let sy = 0; sy < scale; sy += 1) {
          for (let sx = 0; sx < scale; sx += 1) {
            const x = cursorX + col * scale + sx;
            const y = startY + row * scale + sy;
            if (x >= 0 && x < cols && y >= 0 && y < rows) {
              targetMap[y][x] = true;
            }
          }
        }
      }
    }

    cursorX += 3 * scale + scale;
    if (digitIndex === 1) {
      for (let sy = 0; sy < scale; sy += 1) {
        for (let sx = 0; sx < scale; sx += 1) {
          const topY = startY + scale + sy;
          const bottomY = startY + scale * 3 + sy;
          const x = cursorX + sx;
          if (x >= 0 && x < cols) {
            if (topY >= 0 && topY < rows) {
              targetMap[topY][x] = true;
            }
            if (bottomY >= 0 && bottomY < rows) {
              targetMap[bottomY][x] = true;
            }
          }
        }
      }
      cursorX += scale + scale;
    }
  }

  return targetMap;
}

function choosePieceTypes(selectedPieces) {
  if (!Array.isArray(selectedPieces) || selectedPieces.length === 0) {
    return [0, 1, 2, 3, 4, 5, 6];
  }
  const result = selectedPieces.filter(
    (index) => Number.isInteger(index) && index >= 0 && index < 7,
  );
  if (result.length === 0) {
    return [0, 1, 2, 3, 4, 5, 6];
  }
  return result;
}

function buildSequenceSeed(state) {
  const minuteValue = state.clock.hours * 60 + state.clock.minutes;
  let seed = minuteValue < 0 ? 0 : minuteValue;
  seed = seed * 131 + state.cellSize * 17;
  seed = seed * 131 + state.dropSpeed;
  seed = seed * 131 + (state.showClock ? 1 : 0);
  seed = seed * 131 + (state.clearMode ? 1 : 0);
  seed = seed * 131 + state.piecesMask;
  return seed;
}

function computeSpawnDrift(state, pieceType) {
  const seed = buildSequenceSeed(state);
  const mixed = seed + state.spawnCounter * 29 + pieceType * 41;
  return 4 + (mixed % 5);
}

function getRotationCells(pieceType, rotation) {
  return getPieceRotations(pieceType)[rotation];
}

function canMove(state, pieceType, rotation, x, y) {
  const cells = getRotationCells(pieceType, rotation);
  for (let index = 0; index < cells.length; index += 1) {
    const nx = x + cells[index][0];
    const ny = y + cells[index][1];
    if (nx < 0 || nx >= state.cols || ny >= state.rows) {
      return false;
    }
    if (ny >= 0 && state.board[ny][nx] !== 0) {
      return false;
    }
  }
  return true;
}

function pieceMinX(pieceType, rotation) {
  const cells = getRotationCells(pieceType, rotation);
  let minX = 4;
  for (let index = 0; index < cells.length; index += 1) {
    minX = Math.min(minX, cells[index][0]);
  }
  return minX;
}

function pieceMaxX(pieceType, rotation) {
  const cells = getRotationCells(pieceType, rotation);
  let maxX = 0;
  for (let index = 0; index < cells.length; index += 1) {
    maxX = Math.max(maxX, cells[index][0]);
  }
  return maxX;
}

function computeSpawnX(state, pieceType, rotation, finalX) {
  const minOffset = pieceMinX(pieceType, rotation);
  const maxOffset = pieceMaxX(pieceType, rotation);
  const minSpawnX = -minOffset;
  const maxSpawnX = state.cols - 1 - maxOffset;
  const drift = computeSpawnDrift(state, pieceType);
  let startX = finalX + (state.spawnBiasLeft ? -drift : drift);
  state.spawnBiasLeft = !state.spawnBiasLeft;
  if (startX < minSpawnX) {
    startX = minSpawnX;
  }
  if (startX > maxSpawnX) {
    startX = maxSpawnX;
  }
  return startX;
}

function buildClockPlacement(state, pieceType) {
  let bestRot = 0;
  let bestX = 0;
  let bestScore = -999999;

  for (let rotation = 0; rotation < 4; rotation += 1) {
    for (let x = -2; x < state.cols; x += 1) {
      let y = -2;
      if (!canMove(state, pieceType, rotation, x, y)) {
        continue;
      }
      while (canMove(state, pieceType, rotation, x, y + 1)) {
        y += 1;
      }

      const cells = getRotationCells(pieceType, rotation);
      let score = 0;
      let valid = true;

      for (let index = 0; index < cells.length; index += 1) {
        const nx = x + cells[index][0];
        const ny = y + cells[index][1];
        if (nx < 0 || nx >= state.cols || ny < 0 || ny >= state.rows) {
          valid = false;
          break;
        }
        if (!state.targetMap[ny][nx]) {
          valid = false;
          break;
        }
        score += 2000;
        if (
          ny + 1 >= state.rows ||
          state.board[ny + 1][nx] !== 0 ||
          !state.targetMap[ny + 1][nx]
        ) {
          score += 40;
        }
        if (nx > 0 && state.board[ny][nx - 1] !== 0) {
          score += 18;
        }
        if (nx + 1 < state.cols && state.board[ny][nx + 1] !== 0) {
          score += 18;
        }
      }
      if (!valid) {
        continue;
      }

      for (let index = 0; index < cells.length; index += 1) {
        const nx = x + cells[index][0];
        const ny = y + cells[index][1];
        state.board[ny][nx] = pieceType + 1;
      }

      let holes = 0;
      for (let col = 0; col < state.cols; col += 1) {
        let seenTargetBlock = false;
        for (let row = 0; row < state.rows; row += 1) {
          if (state.targetMap[row][col] && state.board[row][col] !== 0) {
            seenTargetBlock = true;
          } else if (seenTargetBlock && state.targetMap[row][col] && state.board[row][col] === 0) {
            holes += 1;
          }
        }
      }
      score -= holes * 180;

      for (let row = 0; row < state.rows; row += 1) {
        let filledInTarget = 0;
        let targetCount = 0;
        for (let col = 0; col < state.cols; col += 1) {
          if (state.targetMap[row][col]) {
            targetCount += 1;
            if (state.board[row][col] !== 0) {
              filledInTarget += 1;
            }
          }
        }
        score += filledInTarget * 8;
        if (targetCount > 0 && filledInTarget === targetCount) {
          score += 120;
        }
      }

      for (let index = 0; index < cells.length; index += 1) {
        const nx = x + cells[index][0];
        const ny = y + cells[index][1];
        state.board[ny][nx] = 0;
      }

      score += y * 4;
      if (score > bestScore) {
        bestScore = score;
        bestRot = rotation;
        bestX = x;
      }
    }
  }

  if (bestScore <= -999999) {
    return null;
  }

  return { rotation: bestRot, x: bestX };
}

function buildFreeFallPlacement(state, pieceType) {
  let bestRot = 0;
  let bestX = 0;
  let bestScore = -99999;

  for (let rotation = 0; rotation < 4; rotation += 1) {
    for (let x = -2; x < state.cols; x += 1) {
      let y = -2;
      if (!canMove(state, pieceType, rotation, x, y)) {
        continue;
      }
      while (canMove(state, pieceType, rotation, x, y + 1)) {
        y += 1;
      }

      const cells = getRotationCells(pieceType, rotation);
      let valid = true;
      for (let index = 0; index < cells.length; index += 1) {
        const ny = y + cells[index][1];
        if (ny < 0) {
          valid = false;
          break;
        }
      }
      if (!valid) {
        continue;
      }

      for (let index = 0; index < cells.length; index += 1) {
        const nx = x + cells[index][0];
        const ny = y + cells[index][1];
        if (ny >= 0 && ny < state.rows && nx >= 0 && nx < state.cols) {
          state.board[ny][nx] = pieceType + 1;
        }
      }

      let score = 0;
      score += y * 20;

      for (let row = 0; row < state.rows; row += 1) {
        let full = true;
        let filled = 0;
        for (let col = 0; col < state.cols; col += 1) {
          if (state.board[row][col] !== 0) {
            filled += 1;
          } else {
            full = false;
          }
        }
        if (full) {
          score += 5000;
        }
        score += filled * 2;
      }

      for (let col = 0; col < state.cols; col += 1) {
        let blocked = false;
        for (let row = 0; row < state.rows; row += 1) {
          if (state.board[row][col] !== 0) {
            blocked = true;
          } else if (blocked) {
            score -= 500;
          }
        }
      }

      for (let index = 0; index < cells.length; index += 1) {
        const nx = x + cells[index][0];
        const ny = y + cells[index][1];
        if (ny + 1 >= state.rows) {
          score += 10;
        } else if (
          ny + 1 >= 0 &&
          state.board[ny + 1][nx] !== 0 &&
          state.board[ny + 1][nx] !== pieceType + 1
        ) {
          score += 10;
        }
        if (nx - 1 < 0 || state.board[ny][nx - 1] !== 0) {
          score += 5;
        }
        if (nx + 1 >= state.cols || state.board[ny][nx + 1] !== 0) {
          score += 5;
        }
      }

      const maxH = new Array(state.cols).fill(state.rows);
      for (let col = 0; col < state.cols; col += 1) {
        for (let row = 0; row < state.rows; row += 1) {
          if (state.board[row][col] !== 0) {
            maxH[col] = row;
            break;
          }
        }
      }
      for (let col = 1; col < state.cols; col += 1) {
        score -= Math.abs(maxH[col] - maxH[col - 1]) * 30;
      }

      for (let index = 0; index < cells.length; index += 1) {
        const nx = x + cells[index][0];
        const ny = y + cells[index][1];
        if (ny >= 0 && ny < state.rows && nx >= 0 && nx < state.cols) {
          state.board[ny][nx] = 0;
        }
      }

      if (score > bestScore) {
        bestScore = score;
        bestRot = rotation;
        bestX = x;
      }
    }
  }

  return { rotation: bestRot, x: bestX };
}

function lockPiece(state) {
  const cells = getRotationCells(state.curType, state.curRot);
  for (let index = 0; index < cells.length; index += 1) {
    const nx = state.curX + cells[index][0];
    const ny = state.curY + cells[index][1];
    if (ny >= 0 && ny < state.rows && nx >= 0 && nx < state.cols) {
      state.board[ny][nx] = state.curType + 1;
    }
  }
}

function clearLines(state) {
  for (let y = state.rows - 1; y >= 0; y -= 1) {
    let full = true;
    for (let x = 0; x < state.cols; x += 1) {
      if (state.board[y][x] === 0) {
        full = false;
        break;
      }
    }
    if (!full) {
      continue;
    }
    for (let yy = y; yy > 0; yy -= 1) {
      for (let col = 0; col < state.cols; col += 1) {
        state.board[yy][col] = state.board[yy - 1][col];
      }
    }
    for (let col = 0; col < state.cols; col += 1) {
      state.board[0][col] = 0;
    }
    y += 1;
  }
}

function spawnPiece(state) {
  const enabled = choosePieceTypes(state.selectedPieces);
  const seed = buildSequenceSeed(state);
  const pieceIndex =
    (seed + state.spawnCounter * 73 + state.spawnCounter * state.spawnCounter * 11) %
    enabled.length;
  state.curType = enabled[pieceIndex];

  if (state.showClock) {
    const placement = buildClockPlacement(state, state.curType);
    if (placement) {
      state.holdClockFrame = false;
      state.curRot = placement.rotation;
      state.targetX = placement.x;
      state.curX = computeSpawnX(state, state.curType, state.curRot, state.targetX);
      if (!canMove(state, state.curType, state.curRot, state.curX, -2)) {
        state.curX = state.targetX;
      }
      state.curY = -2;
      state.spawnCounter += 1;
      return;
    }
    state.holdClockFrame = true;
    state.curRot = 0;
    state.curX = 0;
    state.curY = -100;
    return;
  }

  const placement = buildFreeFallPlacement(state, state.curType);
  state.curRot = placement.rotation;
  state.targetX = placement.x;
  state.curX = computeSpawnX(state, state.curType, state.curRot, state.targetX);
  if (!canMove(state, state.curType, state.curRot, state.curX, -2)) {
    state.curX = state.targetX;
  }
  state.curY = -2;
  state.spawnCounter += 1;
}

function stepState(state) {
  if (state.showClock && state.holdClockFrame) {
    return;
  }

  if (state.curX !== state.targetX) {
    const step = state.curX < state.targetX ? 1 : -1;
    if (canMove(state, state.curType, state.curRot, state.curX + step, state.curY)) {
      state.curX += step;
    } else {
      state.targetX = state.curX;
    }
  }

  if (canMove(state, state.curType, state.curRot, state.curX, state.curY + 1)) {
    state.curY += 1;
    return;
  }

  lockPiece(state);

  if (state.clearMode && !state.showClock) {
    clearLines(state);
  }

  let topFull = false;
  for (let x = 0; x < state.cols; x += 1) {
    if (state.board[0][x] !== 0 || state.board[1][x] !== 0) {
      topFull = true;
      break;
    }
  }
  if (topFull) {
    state.board = createMatrix(state.rows, state.cols, 0);
  }

  spawnPiece(state);
}

function buildFrameMap(state, pulseBright) {
  const pixels = new Map();
  drawGridBackground(pixels, state.cellSize);

  for (let y = 0; y < state.rows; y += 1) {
    for (let x = 0; x < state.cols; x += 1) {
      let cell = state.board[y][x];
      if (state.curY > -100) {
        const currentCells = getRotationCells(state.curType, state.curRot);
        for (let index = 0; index < currentCells.length; index += 1) {
          const px = state.curX + currentCells[index][0];
          const py = state.curY + currentCells[index][1];
          if (px === x && py === y) {
            cell = state.curType + 1;
            break;
          }
        }
      }

      if (cell === 0 && state.showClock && state.targetMap[y][x]) {
        drawGhostCell(pixels, x, y, state.cellSize);
        continue;
      }

      if (cell > 0) {
        drawPieceCell(pixels, x, y, state.cellSize, cell - 1, pulseBright);
      }
    }
  }

  return pixels;
}

function createSimulationState(config) {
  const cellSize = clamp(Number(config.cellSize) || 2, 1, 3);
  const cols = Math.floor(PANEL_SIZE / cellSize);
  const rows = Math.floor(PANEL_SIZE / cellSize);
  const clock = getClockSnapshot();
  const pieces = choosePieceTypes(config.pieces);
  let piecesMask = 0;
  for (let index = 0; index < pieces.length; index += 1) {
    piecesMask |= 1 << pieces[index];
  }

  const state = {
    cellSize,
    cols,
    rows,
    board: createMatrix(rows, cols, 0),
    targetMap: buildTargetMap(cols, rows, config.showClock !== false, clock),
    selectedPieces: pieces,
    piecesMask,
    dropSpeed: getDropSpeed(config),
    showClock: config.showClock !== false,
    clearMode: config.clearMode !== false,
    clock,
    spawnCounter: 0,
    spawnBiasLeft: true,
    holdClockFrame: false,
    curType: 0,
    curRot: 0,
    curX: 0,
    curY: -2,
    targetX: 0,
  };

  spawnPiece(state);
  return state;
}

export function buildTetrisPreviewFrames(config) {
  const state = createSimulationState(config || {});
  const frames = [];
  const frameMs = state.dropSpeed;
  let holdElapsed = 0;
  let holdPulseBright = false;
  const activeFrameLimit = state.showClock ? 64 : 40;

  frames.push(buildFrameMap(state, false));

  for (let index = 0; index < activeFrameLimit; index += 1) {
    stepState(state);
    frames.push(buildFrameMap(state, false));
    if (state.showClock && state.holdClockFrame) {
      break;
    }
  }

  if (state.showClock && state.holdClockFrame) {
    for (let index = 0; index < 10; index += 1) {
      holdElapsed += frameMs;
      if (holdElapsed >= 220) {
        holdElapsed = 0;
        holdPulseBright = !holdPulseBright;
      }
      frames.push(buildFrameMap(state, holdPulseBright));
    }
  }

  return frames;
}
