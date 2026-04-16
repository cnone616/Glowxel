import { drawClockTextToPixels } from "./clockCanvas.js";

const PANEL_SIZE = 64;
const PIECES = [
  [0x0f00, 0x2222, 0x00f0, 0x4444],
  [0x6600, 0x6600, 0x6600, 0x6600],
  [0x0e40, 0x4c40, 0x4e00, 0x4640],
  [0x06c0, 0x8c40, 0x6c00, 0x4620],
  [0x0c60, 0x4c80, 0xc600, 0x2640],
  [0x0e80, 0xc440, 0x2e00, 0x44c0],
  [0x0e20, 0x44c0, 0x8e00, 0xc880],
];

const COLORS = [
  { r: 0, g: 240, b: 240 },
  { r: 240, g: 240, b: 0 },
  { r: 160, g: 0, b: 240 },
  { r: 0, g: 240, b: 0 },
  { r: 240, g: 0, b: 0 },
  { r: 0, g: 0, b: 240 },
  { r: 240, g: 160, b: 0 },
];

function createRandom(seed) {
  let value = seed >>> 0;
  return function nextRandom() {
    value = (value * 1664525 + 1013904223) >>> 0;
    return value / 4294967296;
  };
}

function createBoard(rows, cols) {
  return Array.from({ length: rows }, () => Array(cols).fill(0));
}

function cloneBoard(board) {
  return board.map((row) => row.slice());
}

function resolveSpeedMode(speed) {
  if (speed === "slow" || speed === "normal" || speed === "fast") {
    return speed;
  }
  return "normal";
}

function resolveFrameDelay(speed) {
  const speedMode = resolveSpeedMode(speed);
  if (speedMode === "slow") {
    return 180;
  }
  if (speedMode === "fast") {
    return 75;
  }
  return 110;
}

function getPieceCells(type, rotation) {
  const mask = PIECES[type][rotation];
  const cells = [];
  for (let row = 0; row < 4; row += 1) {
    for (let col = 0; col < 4; col += 1) {
      if (mask & (0x8000 >> (row * 4 + col))) {
        cells.push({ x: col, y: row });
      }
    }
  }
  return cells;
}

function canMove(board, cols, rows, type, rotation, offsetX, offsetY) {
  const cells = getPieceCells(type, rotation);
  for (let index = 0; index < cells.length; index += 1) {
    const cell = cells[index];
    const nextX = offsetX + cell.x;
    const nextY = offsetY + cell.y;
    if (nextX < 0 || nextX >= cols || nextY >= rows) {
      return false;
    }
    if (nextY >= 0 && board[nextY][nextX] !== 0) {
      return false;
    }
  }
  return true;
}

function buildTemporaryBoard(board, cols, rows, type, rotation, offsetX, offsetY, colorIndex) {
  const snapshot = cloneBoard(board);
  const cells = getPieceCells(type, rotation);
  for (let index = 0; index < cells.length; index += 1) {
    const cell = cells[index];
    const nextX = offsetX + cell.x;
    const nextY = offsetY + cell.y;
    if (nextX < 0 || nextX >= cols || nextY < 0 || nextY >= rows) {
      return null;
    }
    snapshot[nextY][nextX] = colorIndex;
  }
  return snapshot;
}

function scoreBoard(board, cols, rows, type, rotation, offsetX, landingY, colorIndex) {
  const placedBoard = buildTemporaryBoard(
    board,
    cols,
    rows,
    type,
    rotation,
    offsetX,
    landingY,
    colorIndex,
  );
  if (placedBoard === null) {
    return -99999;
  }

  let score = landingY * 20;

  for (let row = 0; row < rows; row += 1) {
    let filled = 0;
    let full = true;
    for (let col = 0; col < cols; col += 1) {
      if (placedBoard[row][col] !== 0) {
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

  for (let col = 0; col < cols; col += 1) {
    let blocked = false;
    for (let row = 0; row < rows; row += 1) {
      if (placedBoard[row][col] !== 0) {
        blocked = true;
      } else if (blocked) {
        score -= 500;
      }
    }
  }

  const cells = getPieceCells(type, rotation);
  for (let index = 0; index < cells.length; index += 1) {
    const cell = cells[index];
    const nextX = offsetX + cell.x;
    const nextY = landingY + cell.y;
    if (nextY + 1 >= rows) {
      score += 10;
    } else if (
      nextY + 1 >= 0 &&
      placedBoard[nextY + 1][nextX] !== 0 &&
      placedBoard[nextY + 1][nextX] !== colorIndex
    ) {
      score += 10;
    }

    if (
      nextX - 1 < 0 ||
      (placedBoard[nextY][nextX - 1] !== 0 &&
        placedBoard[nextY][nextX - 1] !== colorIndex)
    ) {
      score += 5;
    }

    if (
      nextX + 1 >= cols ||
      (placedBoard[nextY][nextX + 1] !== 0 &&
        placedBoard[nextY][nextX + 1] !== colorIndex)
    ) {
      score += 5;
    }
  }

  const heights = [];
  for (let col = 0; col < cols; col += 1) {
    let height = rows;
    for (let row = 0; row < rows; row += 1) {
      if (placedBoard[row][col] !== 0) {
        height = row;
        break;
      }
    }
    heights.push(height);
  }

  for (let col = 1; col < cols; col += 1) {
    score -= Math.abs(heights[col] - heights[col - 1]) * 30;
  }

  return score;
}

function resolvePieceChoices(pieces) {
  if (!Array.isArray(pieces)) {
    return [0, 1, 2, 3, 4, 5, 6];
  }
  const normalized = pieces
    .map((item) => Number(item))
    .filter((item) => Number.isInteger(item) && item >= 0 && item < 7);
  if (normalized.length === 0) {
    return [0, 1, 2, 3, 4, 5, 6];
  }
  return Array.from(new Set(normalized)).sort((left, right) => left - right);
}

function pickPiece(state) {
  const choiceIndex = Math.floor(state.random() * state.pieces.length);
  return state.pieces[choiceIndex];
}

function resolveSpawnX(state, type) {
  const preferredSpawnX = Math.max(
    0,
    Math.min(state.cols - 1, Math.floor(state.cols / 2) - 2),
  );

  if (canMove(state.board, state.cols, state.rows, type, 0, preferredSpawnX, 0)) {
    return preferredSpawnX;
  }

  for (let delta = 1; delta <= state.cols; delta += 1) {
    const leftX = preferredSpawnX - delta;
    if (
      leftX >= 0 &&
      canMove(state.board, state.cols, state.rows, type, 0, leftX, 0)
    ) {
      return leftX;
    }

    const rightX = preferredSpawnX + delta;
    if (
      rightX < state.cols &&
      canMove(state.board, state.cols, state.rows, type, 0, rightX, 0)
    ) {
      return rightX;
    }
  }

  return null;
}

function spawnPiece(state) {
  const type = pickPiece(state);
  const colorIndex = type + 1;
  let bestRotation = 0;
  let bestX = 0;
  let bestScore = -99999;

  for (let rotation = 0; rotation < 4; rotation += 1) {
    for (let offsetX = -2; offsetX < state.cols; offsetX += 1) {
      let offsetY = -2;
      if (!canMove(state.board, state.cols, state.rows, type, rotation, offsetX, offsetY)) {
        continue;
      }

      while (
        canMove(state.board, state.cols, state.rows, type, rotation, offsetX, offsetY + 1)
      ) {
        offsetY += 1;
      }

      const cells = getPieceCells(type, rotation);
      let valid = true;
      for (let index = 0; index < cells.length; index += 1) {
        if (offsetY + cells[index].y < 0) {
          valid = false;
          break;
        }
      }
      if (!valid) {
        continue;
      }

      const score = scoreBoard(
        state.board,
        state.cols,
        state.rows,
        type,
        rotation,
        offsetX,
        offsetY,
        colorIndex,
      );
      if (score > bestScore) {
        bestScore = score;
        bestRotation = rotation;
        bestX = offsetX;
      }
    }
  }

  const spawnX = resolveSpawnX(state, type);
  if (spawnX === null) {
    state.board = createBoard(state.rows, state.cols);
    return spawnPiece(state);
  }

  return {
    type,
    rotation: 0,
    x: spawnX,
    y: 0,
    colorIndex,
    targetRotation: bestRotation,
    targetX: bestX,
  };
}

function lockPiece(state, piece) {
  const cells = getPieceCells(piece.type, piece.rotation);
  for (let index = 0; index < cells.length; index += 1) {
    const cell = cells[index];
    const nextX = piece.x + cell.x;
    const nextY = piece.y + cell.y;
    if (nextX >= 0 && nextX < state.cols && nextY >= 0 && nextY < state.rows) {
      state.board[nextY][nextX] = piece.colorIndex;
    }
  }
}

function clearLines(state) {
  for (let row = state.rows - 1; row >= 0; row -= 1) {
    let full = true;
    for (let col = 0; col < state.cols; col += 1) {
      if (state.board[row][col] === 0) {
        full = false;
        break;
      }
    }

    if (!full) {
      continue;
    }

    for (let shiftRow = row; shiftRow > 0; shiftRow -= 1) {
      state.board[shiftRow] = state.board[shiftRow - 1].slice();
    }
    state.board[0] = Array(state.cols).fill(0);
    row += 1;
  }
}

function topReached(state) {
  for (let col = 0; col < state.cols; col += 1) {
    if (state.board[0][col] !== 0 || state.board[1][col] !== 0) {
      return true;
    }
  }
  return false;
}

function drawCell(map, cellX, cellY, cellSize, color) {
  const pixelX = cellX * cellSize;
  const pixelY = cellY * cellSize;
  for (let offsetY = 0; offsetY < cellSize; offsetY += 1) {
    for (let offsetX = 0; offsetX < cellSize; offsetX += 1) {
      const drawX = pixelX + offsetX;
      const drawY = pixelY + offsetY;
      if (drawX >= 0 && drawX < PANEL_SIZE && drawY >= 0 && drawY < PANEL_SIZE) {
        map.set(`${drawX},${drawY}`, `rgb(${color.r}, ${color.g}, ${color.b})`);
      }
    }
  }
}

function drawBoard(map, board, cols, rows, cellSize) {
  for (let row = 0; row < rows; row += 1) {
    for (let col = 0; col < cols; col += 1) {
      const colorIndex = board[row][col];
      if (colorIndex <= 0) {
        continue;
      }
      drawCell(map, col, row, cellSize, COLORS[colorIndex - 1]);
    }
  }
}

function drawActivePiece(map, piece, cols, rows, cellSize) {
  const cells = getPieceCells(piece.type, piece.rotation);
  for (let index = 0; index < cells.length; index += 1) {
    const cell = cells[index];
    const nextX = piece.x + cell.x;
    const nextY = piece.y + cell.y;
    if (nextX >= 0 && nextX < cols && nextY >= 0 && nextY < rows) {
      drawCell(map, nextX, nextY, cellSize, COLORS[piece.type]);
    }
  }
}

function buildClockText() {
  const now = new Date();
  const hour = `${now.getHours()}`.padStart(2, "0");
  const minute = `${now.getMinutes()}`.padStart(2, "0");
  return `${hour}:${minute}`;
}

function drawClockOverlay(map) {
  drawClockTextToPixels(
    buildClockText(),
    32,
    2,
    "#ffffff",
    map,
    "classic_5x7",
    1,
    "center",
  );
}

function settlePiece(state, piece) {
  lockPiece(state, piece);
  if (state.clearMode) {
    clearLines(state);
  }
  if (topReached(state)) {
    state.board = createBoard(state.rows, state.cols);
    return true;
  }
  return false;
}

function renderFrame(state, piece, showClock) {
  const map = new Map();
  drawBoard(map, state.board, state.cols, state.rows, state.cellSize);
  drawActivePiece(map, piece, state.cols, state.rows, state.cellSize);
  if (showClock) {
    drawClockOverlay(map);
  }
  return map;
}

function normalizeConfig(config) {
  const source = config && typeof config === "object" ? config : {};
  const cellSize = [1, 2, 3].includes(source.cellSize) ? source.cellSize : 2;
  return {
    clearMode: source.clearMode !== false,
    cellSize,
    speed: resolveSpeedMode(source.speed),
    showClock: source.showClock !== false,
    pieces: resolvePieceChoices(source.pieces),
  };
}

function createTetrisScreensaverPreviewState(config) {
  const safeConfig = normalizeConfig(config);
  const state = {
    clearMode: safeConfig.clearMode,
    cellSize: safeConfig.cellSize,
    cols: Math.floor(PANEL_SIZE / safeConfig.cellSize),
    rows: Math.floor(PANEL_SIZE / safeConfig.cellSize),
    board: createBoard(
      Math.floor(PANEL_SIZE / safeConfig.cellSize),
      Math.floor(PANEL_SIZE / safeConfig.cellSize),
    ),
    pieces: safeConfig.pieces,
    random: createRandom(
      safeConfig.cellSize * 17 +
        safeConfig.pieces.length * 29 +
        (safeConfig.clearMode ? 1 : 0) * 43 +
        (safeConfig.showClock ? 1 : 0) * 59,
    ),
    showClock: safeConfig.showClock,
    frameDelay: resolveFrameDelay(safeConfig.speed),
    currentPiece: null,
  };
  state.currentPiece = spawnPiece(state);
  return state;
}

function renderTetrisScreensaverPreviewState(state) {
  if (!state || !state.currentPiece) {
    return new Map();
  }
  return renderFrame(state, state.currentPiece, state.showClock);
}

function stepTetrisScreensaverPreviewState(state) {
  if (!state || !state.currentPiece) {
    return state;
  }

  if (state.currentPiece.rotation !== state.currentPiece.targetRotation) {
    const nextRotation = (state.currentPiece.rotation + 1) % 4;
    if (
      canMove(
        state.board,
        state.cols,
        state.rows,
        state.currentPiece.type,
        nextRotation,
        state.currentPiece.x,
        state.currentPiece.y,
      )
    ) {
      state.currentPiece.rotation = nextRotation;
      return state;
    }
    state.currentPiece.targetRotation = state.currentPiece.rotation;
  }

  if (
    state.currentPiece.x < state.currentPiece.targetX &&
    canMove(
      state.board,
      state.cols,
      state.rows,
      state.currentPiece.type,
      state.currentPiece.rotation,
      state.currentPiece.x + 1,
      state.currentPiece.y,
    )
  ) {
    state.currentPiece.x += 1;
    return state;
  }

  if (
    state.currentPiece.x > state.currentPiece.targetX &&
    canMove(
      state.board,
      state.cols,
      state.rows,
      state.currentPiece.type,
      state.currentPiece.rotation,
      state.currentPiece.x - 1,
      state.currentPiece.y,
    )
  ) {
    state.currentPiece.x -= 1;
    return state;
  }

  if (
    canMove(
      state.board,
      state.cols,
      state.rows,
      state.currentPiece.type,
      state.currentPiece.rotation,
      state.currentPiece.x,
      state.currentPiece.y + 1,
    )
  ) {
    state.currentPiece.y += 1;
    return state;
  }

  const didResetBoard = settlePiece(state, state.currentPiece);
  if (didResetBoard) {
    state.board = createBoard(state.rows, state.cols);
  }
  state.currentPiece = spawnPiece(state);
  return state;
}

function buildTetrisScreensaverPreviewSequence(config) {
  const state = createTetrisScreensaverPreviewState(config);
  const maps = [];
  const delays = [];
  for (let frameIndex = 0; frameIndex < 32; frameIndex += 1) {
    maps.push(renderTetrisScreensaverPreviewState(state));
    delays.push(state.frameDelay);
    stepTetrisScreensaverPreviewState(state);
  }

  return {
    maps,
    delays,
  };
}

export {
  buildTetrisScreensaverPreviewSequence,
  createTetrisScreensaverPreviewState,
  renderTetrisScreensaverPreviewState,
  stepTetrisScreensaverPreviewState,
};
