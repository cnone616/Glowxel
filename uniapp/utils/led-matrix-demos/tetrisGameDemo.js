import { setPixel } from "./common.js";

const CLOCK_GLYPHS_3X5 = {
  "0": [0b111, 0b101, 0b101, 0b101, 0b111],
  "1": [0b010, 0b110, 0b010, 0b010, 0b111],
  "2": [0b111, 0b001, 0b111, 0b100, 0b111],
  "3": [0b111, 0b001, 0b111, 0b001, 0b111],
  "4": [0b101, 0b101, 0b111, 0b001, 0b001],
  "5": [0b111, 0b100, 0b111, 0b001, 0b111],
  "6": [0b111, 0b100, 0b111, 0b101, 0b111],
  "7": [0b111, 0b001, 0b010, 0b010, 0b010],
  "8": [0b111, 0b101, 0b111, 0b101, 0b111],
  "9": [0b111, 0b101, 0b111, 0b001, 0b111],
  ":": [0b000, 0b010, 0b000, 0b010, 0b000],
};

const BEST_PARAMS = [
  -0.212454,
  0.440242,
  -0.370061,
  -0.0793521,
  0.000290688,
  0.397267,
  -0.824796,
  0.532017,
  -0.283169,
  0.228108,
  0.657395,
  -0.166412,
  -0.85031,
  -0.231146,
  -0.81578,
  -0.367808,
  -0.287443,
  0.623546,
  -0.760371,
  18777,
];

const POSSIBLE_MOVES = ["llll", "lll", "ll", "l", "rrrrr", "rrrr", "rrr", "rr", "r", ""];
const SHAPE_ROTATIONS = [1, 2, 2, 2, 4, 4, 4];
const SHAPE_LIST = [
  ["        oo  oo  ", "        oo  oo  ", "        oo  oo  ", "        oo  oo  "],
  ["    oooo        ", " o   o   o   o  ", "    oooo        ", " o   o   o   o  "],
  ["    oo   oo     ", "  o  oo  o      ", "    oo   oo     ", "  o  oo  o      "],
  ["      oo oo     ", " o   oo   o     ", "      oo oo     ", " o   oo   o     "],
  ["     o   o   oo ", "    ooo o       ", "     oo   o   o ", "      o ooo     "],
  ["    o   ooo     ", "     oo  o   o  ", "    ooo   o     ", "      o   o  oo "],
  ["    ooo  o      ", " o  oo   o      ", " o  ooo         ", " o   oo  o      "],
];

const PIECE_COLORS = [
  { r: 255, g: 255, b: 0 },
  { r: 255, g: 165, b: 0 },
  { r: 148, g: 0, b: 211 },
  { r: 255, g: 0, b: 0 },
  { r: 0, g: 255, b: 0 },
  { r: 255, g: 192, b: 203 },
  { r: 0, g: 0, b: 139 },
  { r: 255, g: 255, b: 255 },
];

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state = (state * 1664525 + 1013904223) >>> 0;
    return state / 0x100000000;
  };
}

function clampInt(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function drawFilledRect(map, x, y, width, height, color) {
  for (let row = 0; row < height; row += 1) {
    for (let col = 0; col < width; col += 1) {
      setPixel(map, x + col, y + row, color.r, color.g, color.b);
    }
  }
}

function drawClockText(map, text, x, y, scale, color) {
  let cursorX = x;
  for (let index = 0; index < text.length; index += 1) {
    const glyph = CLOCK_GLYPHS_3X5[text[index]];
    if (!glyph) {
      cursorX += scale * 4;
      continue;
    }
    for (let row = 0; row < glyph.length; row += 1) {
      for (let col = 0; col < 3; col += 1) {
        const mask = 1 << (2 - col);
        if ((glyph[row] & mask) === 0) {
          continue;
        }
        drawFilledRect(
          map,
          cursorX + col * scale,
          y + row * scale,
          scale,
          scale,
          color,
        );
      }
    }
    cursorX += scale * 4;
  }
}

function cloneMatrix(matrix) {
  return matrix.map((row) => row.slice());
}

function createPiece(random) {
  const pieceBag = [0, 1, 2, 3, 4, 5, 6];

  function removeElement(list, value) {
    const next = list.slice();
    const index = next.indexOf(value);
    if (index >= 0) {
      const temp = next[index];
      next[index] = next[next.length - 1];
      next[next.length - 1] = temp;
      next.pop();
    }
    return next;
  }

  const first = pieceBag[Math.floor(random() * pieceBag.length)];
  let nextBag = removeElement(pieceBag, first);
  const second = nextBag[Math.floor(random() * nextBag.length)];
  nextBag = removeElement(nextBag, second);

  return {
    pieceBag: nextBag,
    n: first,
    m: 0,
    next: second,
    shape: SHAPE_LIST[first][0],
    position: [0, 3],
    fixed: false,
  };
}

function pieceNewShape(piece) {
  piece.n = piece.next;
  piece.m = 0;
  piece.shape = SHAPE_LIST[piece.n][piece.m];
  piece.position = [0, 3];
  piece.fixed = false;
}

function pieceNewNext(piece, random) {
  let bag = piece.pieceBag.slice();
  if (bag.length === 0) {
    bag = [0, 1, 2, 3, 4, 5, 6];
  }
  const value = bag[Math.floor(random() * bag.length)];
  const index = bag.indexOf(value);
  bag[index] = bag[bag.length - 1];
  bag.pop();
  piece.next = value;
  piece.pieceBag = bag;
}

function createGrid(random) {
  const matrix = [];
  for (let index = 0; index < 24; index += 1) {
    matrix.push([0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
  }
  return {
    piece: createPiece(random),
    gameOver: false,
    clearedLines: 0,
    matrix,
    score: 0,
    isAnimating: false,
    animatingLines: [],
    animationStep: 0,
  };
}

function inRange(value, min, max) {
  return min <= value && value < max;
}

function contains(value, list) {
  return list.includes(value);
}

function rotatePiece(grid) {
  let possible = true;
  const nextRotation = (grid.piece.m + 1) % 4;
  for (let index = 1; index <= 16 && possible; index += 1) {
    const row = Math.floor(index / 4) + 1;
    const col = index % 4;
    const a = grid.piece.position[0] + row;
    const b = grid.piece.position[1] + col;
    if (
      SHAPE_LIST[grid.piece.n][nextRotation][index] === "o" &&
      (a > 23 || b < 0 || b > 9 || inRange(grid.matrix[a][b], 2, 9))
    ) {
      possible = false;
    }
  }
  if (!possible) {
    return;
  }
  grid.piece.m = nextRotation;
  grid.piece.shape = SHAPE_LIST[grid.piece.n][grid.piece.m];
}

function movePiece(grid, direction, borderIndex) {
  let limited = false;
  for (let index = 1; index <= 16 && !limited; index += 1) {
    const row = Math.floor(index / 4);
    const col = index % 4;
    const a = grid.piece.position[0] + row;
    const b = grid.piece.position[1] + col;
    if (grid.piece.shape[index] !== "o") {
      continue;
    }
    if (b === borderIndex) {
      limited = true;
      break;
    }
    if (0 <= b + direction && b + direction <= 9 && 0 <= a && a <= 23) {
      if (
        (a + 1 <= 23 && inRange(grid.matrix[a + 1][b + direction], 2, 9)) ||
        inRange(grid.matrix[a][b + direction], 2, 9)
      ) {
        limited = true;
      }
    }
  }
  if (!limited) {
    grid.piece.position[1] += direction;
  }
}

function fixPiece(grid) {
  let a = 0;
  for (let index = 0; index < 16; index += 1) {
    a = grid.piece.position[0] + Math.floor(index / 4);
    const b = grid.piece.position[1] + (index % 4);
    if (grid.piece.shape[index] !== "o") {
      continue;
    }
    if (a < 4) {
      grid.gameOver = true;
    } else if (a < 24) {
      grid.matrix[a][b] = grid.piece.n + 2;
    }
  }
  grid.piece.fixed = true;
  if (a < 4) {
    grid.gameOver = true;
  }
}

function updateGrid(grid) {
  let fixed = false;
  for (let row = 0; row < 24; row += 1) {
    for (let col = 0; col < 10; col += 1) {
      if (grid.matrix[row][col] === 1) {
        grid.matrix[row][col] = 0;
      }
    }
  }

  for (let index = 1; index <= 16 && !fixed; index += 1) {
    const a = grid.piece.position[0] + Math.floor(index / 4);
    const b = grid.piece.position[1] + (index % 4);
    if (
      grid.piece.shape[index] === "o" &&
      0 <= a &&
      a < 24 &&
      0 <= b &&
      b < 10
    ) {
      if (a === 23 || inRange(grid.matrix[a + 1][b], 2, 9)) {
        fixed = true;
        fixPiece(grid);
      } else {
        grid.matrix[a][b] = 1;
      }
    }
  }
}

function gravity(grid, value) {
  if (value === 1) {
    grid.piece.position[0] += 1;
    return;
  }
  if (value === 2) {
    gravity(grid, 1);
    updateGrid(grid);
    gravity(grid, 1);
    return;
  }
  while (!grid.piece.fixed) {
    gravity(grid, 1);
    updateGrid(grid);
  }
}

function clearLine(grid) {
  grid.animatingLines = [];
  for (let row = 4; row < 24; row += 1) {
    if (!contains(0, grid.matrix[row]) && !contains(1, grid.matrix[row])) {
      grid.animatingLines.push(row);
    }
  }
  if (grid.animatingLines.length > 0) {
    grid.isAnimating = true;
    grid.animationStep = 0;
  }
}

function updateAnimation(grid) {
  if (!grid.isAnimating) {
    return;
  }
  grid.animationStep += 1;
  if (grid.animationStep >= 6) {
    grid.isAnimating = false;
    grid.animationStep = 0;
    const lineCount = grid.animatingLines.length;
    for (let lineIndex = 0; lineIndex < grid.animatingLines.length; lineIndex += 1) {
      const row = grid.animatingLines[lineIndex];
      for (let offset = 1; offset <= row; offset += 1) {
        grid.matrix[row - offset + 1] = grid.matrix[row - offset].slice();
      }
      grid.matrix[0] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
      grid.piece.position[0] += 1;
      grid.clearedLines += 1;
    }
    if (lineCount === 1) {
      grid.score += 1;
    } else if (lineCount === 2) {
      grid.score += 3;
    } else if (lineCount === 3) {
      grid.score += 5;
    } else if (lineCount === 4) {
      grid.score += 8;
    }
    return;
  }
  for (let index = 0; index < grid.animatingLines.length; index += 1) {
    const row = grid.animatingLines[index];
    for (let col = 0; col < 10; col += 1) {
      grid.matrix[row][col] = grid.animationStep % 2 === 0 ? 0 : 9;
    }
  }
}

function loadBestParams() {
  const layer1 = [];
  const biases1 = [];
  let offset = 0;
  for (let row = 0; row < 3; row += 1) {
    layer1.push(BEST_PARAMS.slice(offset, offset + 4));
    offset += 4;
    biases1.push(BEST_PARAMS[offset]);
    offset += 1;
  }
  const layer2 = BEST_PARAMS.slice(offset, offset + 3);
  offset += 3;
  return {
    layer1,
    biases1,
    layer2,
    bias2: BEST_PARAMS[offset],
  };
}

function relu(value) {
  return value > 0 ? value : 0;
}

function forwardNetwork(network, inputs) {
  let output = 0;
  for (let row = 0; row < 3; row += 1) {
    let node = 0;
    for (let col = 0; col < 4; col += 1) {
      node += inputs[col] * network.layer1[row][col];
    }
    output += relu(node + network.biases1[row]) * network.layer2[row];
  }
  return output + network.bias2;
}

function getColumnHeights(matrix) {
  const heights = [];
  for (let col = 0; col < 10; col += 1) {
    let height = 20;
    for (let row = 4; row < 24; row += 1) {
      if (matrix[row][col] !== 0 && matrix[row][col] !== 1) {
        break;
      }
      height -= 1;
    }
    heights.push(height);
  }
  return heights;
}

function getCompletedLines(matrix) {
  let count = 0;
  for (let row = 0; row < 24; row += 1) {
    if (!matrix[row].includes(0)) {
      count += 1;
    }
  }
  return count;
}

function getAggregateHeight(matrix) {
  return getColumnHeights(matrix).reduce((sum, value) => sum + value, 0);
}

function getBumpiness(matrix) {
  const heights = getColumnHeights(matrix);
  let bumpiness = 0;
  for (let index = 0; index < heights.length - 1; index += 1) {
    bumpiness += Math.abs(heights[index] - heights[index + 1]);
  }
  return bumpiness;
}

function getHoles(matrix) {
  let holes = 0;
  for (let row = 23; row > 3; row -= 1) {
    const empty = matrix[row].every((value) => value === 0);
    if (empty) {
      return holes;
    }
    for (let col = 9; col >= 0; col -= 1) {
      if (matrix[row][col] !== 0) {
        continue;
      }
      let covered = false;
      for (let scan = row - 1; scan > 3; scan -= 1) {
        if (matrix[scan][col] !== 0) {
          covered = true;
          break;
        }
      }
      if (covered) {
        holes += 1;
      }
    }
  }
  return holes;
}

function cloneGrid(grid) {
  return {
    piece: {
      pieceBag: grid.piece.pieceBag.slice(),
      n: grid.piece.n,
      m: grid.piece.m,
      next: grid.piece.next,
      shape: grid.piece.shape,
      position: grid.piece.position.slice(),
      fixed: grid.piece.fixed,
    },
    gameOver: grid.gameOver,
    clearedLines: grid.clearedLines,
    matrix: cloneMatrix(grid.matrix),
    score: grid.score,
    isAnimating: grid.isAnimating,
    animatingLines: grid.animatingLines.slice(),
    animationStep: grid.animationStep,
  };
}

function getBestMove(grid, network) {
  let bestMove = "";
  let bestRotation = "0";
  let maxScore = -(10 ** 5);
  const rotationCount = SHAPE_ROTATIONS[grid.piece.n];
  const startingGrid = cloneGrid(grid);

  for (let rotation = 0; rotation < rotationCount; rotation += 1) {
    for (let moveIndex = 0; moveIndex < POSSIBLE_MOVES.length; moveIndex += 1) {
      let workingGrid = cloneGrid(startingGrid);
      for (let index = 0; index < rotation; index += 1) {
        rotatePiece(workingGrid);
      }

      const move = POSSIBLE_MOVES[moveIndex];
      if (move[0] === "l") {
        for (let index = 0; index < move.length; index += 1) {
          movePiece(workingGrid, -1, 0);
        }
      } else if (move[0] === "r") {
        for (let index = 0; index < move.length; index += 1) {
          movePiece(workingGrid, 1, 9);
        }
      }

      gravity(workingGrid, 3);
      const heuristics = [
        getAggregateHeight(workingGrid.matrix),
        getCompletedLines(workingGrid.matrix),
        getHoles(workingGrid.matrix),
        getBumpiness(workingGrid.matrix),
      ];
      const score = forwardNetwork(network, heuristics);

      pieceNewShape(workingGrid.piece);
      const nextRotationCount = SHAPE_ROTATIONS[workingGrid.piece.n];
      const nextStartGrid = cloneGrid(workingGrid);

      for (let nextRotation = 0; nextRotation < nextRotationCount; nextRotation += 1) {
        for (let nextMoveIndex = 0; nextMoveIndex < POSSIBLE_MOVES.length; nextMoveIndex += 1) {
          workingGrid = cloneGrid(nextStartGrid);
          for (let index = 0; index < nextRotation; index += 1) {
            rotatePiece(workingGrid);
          }
          const nextMove = POSSIBLE_MOVES[nextMoveIndex];
          if (nextMove[0] === "l") {
            for (let index = 0; index < nextMove.length; index += 1) {
              movePiece(workingGrid, -1, 0);
            }
          } else if (nextMove[0] === "r") {
            for (let index = 0; index < nextMove.length; index += 1) {
              movePiece(workingGrid, 1, 9);
            }
          }

          const nextHeuristics = [
            getAggregateHeight(workingGrid.matrix),
            getCompletedLines(workingGrid.matrix),
            getHoles(workingGrid.matrix),
            getBumpiness(workingGrid.matrix),
          ];
          const score2 = score + forwardNetwork(network, nextHeuristics);
          if (score2 >= maxScore) {
            maxScore = score2;
            bestMove = move;
            bestRotation = String(rotation);
          }
        }
      }
    }
  }

  return bestMove + bestRotation;
}

function createState(speed) {
  const random = createSeededRandom(20260413 + speed * 97);
  const grid = createGrid(random);
  const network = loadBestParams();
  const bestMove = getBestMove(grid, network);
  return {
    speed,
    grid,
    network,
    random,
    bestMove: bestMove.slice(0, -1),
    bestRotation: Number(bestMove.slice(-1) || 0),
    gameOver: false,
    fixed: false,
    rotated: false,
    fallCounter: 0,
    fallInterval: Math.max(1, 8 - Math.floor((speed - 1) / 2)),
  };
}

function resetState(state) {
  const next = createState(state.speed);
  state.speed = next.speed;
  state.grid = next.grid;
  state.network = next.network;
  state.random = next.random;
  state.bestMove = next.bestMove;
  state.bestRotation = next.bestRotation;
  state.gameOver = false;
  state.fixed = false;
  state.rotated = false;
  state.fallCounter = 0;
}

function stepState(state) {
  if (state.gameOver) {
    resetState(state);
  }

  if (!state.rotated) {
    for (let index = 0; index < state.bestRotation; index += 1) {
      rotatePiece(state.grid);
    }
    state.rotated = true;
  }

  if (state.bestMove.length > 0 && !state.grid.isAnimating) {
    if (state.bestMove[0] === "r") {
      movePiece(state.grid, 1, 9);
    } else {
      movePiece(state.grid, -1, 0);
    }
    state.bestMove = state.bestMove.slice(0, -1);
  }

  if (state.fixed) {
    const nextMove = getBestMove(state.grid, state.network);
    state.bestMove = nextMove.slice(0, -1);
    state.bestRotation = Number(nextMove.slice(-1) || 0);
    state.rotated = false;
    state.fixed = false;
  }

  state.fallCounter += 1;
  if (state.fallCounter >= state.fallInterval && !state.grid.isAnimating) {
    gravity(state.grid, 1);
    state.fallCounter = 0;
  }

  if (state.grid.isAnimating) {
    updateAnimation(state.grid);
  } else {
    clearLine(state.grid);
    updateGrid(state.grid);
  }

  if (state.grid.gameOver) {
    state.gameOver = true;
    return;
  }

  if (state.grid.piece.fixed) {
    pieceNewShape(state.grid.piece);
    pieceNewNext(state.grid.piece, state.random);
    state.fixed = true;
  }
}

function drawFrame(map, state) {
  const blockSize = state.blockSize;
  const boardWidth = 10 * blockSize;
  const boardHeight = 20 * blockSize;
  const offsetX = Math.floor((64 - boardWidth) / 2);
  const offsetY = Math.floor((64 - boardHeight) / 2);

  for (let x = offsetX - 1; x <= offsetX + boardWidth; x += 1) {
    setPixel(map, x, offsetY - 1, 50, 50, 50);
    setPixel(map, x, offsetY + boardHeight, 50, 50, 50);
  }
  for (let y = offsetY - 1; y <= offsetY + boardHeight; y += 1) {
    setPixel(map, offsetX - 1, y, 50, 50, 50);
    setPixel(map, offsetX + boardWidth, y, 50, 50, 50);
  }

  for (let row = 4; row < 24; row += 1) {
    for (let col = 0; col < 10; col += 1) {
      const value = state.grid.matrix[row][col];
      if (value !== 1 && value <= 1) {
        continue;
      }
      const color =
        value === 1
          ? PIECE_COLORS[state.grid.piece.n]
          : PIECE_COLORS[Math.max(0, Math.min(PIECE_COLORS.length - 1, value - 2))];
      for (let px = 0; px < blockSize; px += 1) {
        for (let py = 0; py < blockSize; py += 1) {
          setPixel(
            map,
            offsetX + col * blockSize + px,
            offsetY + (row - 4) * blockSize + py,
            color.r,
            color.g,
            color.b,
          );
        }
      }
    }
  }

  if (state.showClock) {
    const now = new Date();
    const timeText = `${String(now.getHours()).padStart(2, "0")}:${String(
      now.getMinutes(),
    ).padStart(2, "0")}`;
    const textScale = blockSize >= 3 ? 2 : 1;
    const textWidth = timeText.length * 4 * textScale - textScale;
    const textHeight = 5 * textScale;
    const panelWidth = textWidth + 6;
    const panelHeight = textHeight + 4;
    const panelX = Math.floor((64 - panelWidth) / 2);
    const panelY = Math.floor((64 - panelHeight) / 2);
    drawFilledRect(map, panelX, panelY, panelWidth, panelHeight, {
      r: 8,
      g: 14,
      b: 24,
    });
    drawClockText(map, timeText, panelX + 3, panelY + 2, textScale, {
      r: 114,
      g: 198,
      b: 255,
    });
  }
}

function buildTetrisGameDemoMaps(speed, intensity, options) {
  const safeSpeed = Math.max(1, Math.min(10, Math.round(Number(speed) || 6)));
  const safeOptions = options || {};
  const state = createState(safeSpeed);
  state.blockSize = clampInt(Number(safeOptions.cellSize) || 2, 1, 3);
  state.showClock = safeOptions.showClock !== false;
  const maps = [];
  const frameCount = 140 + safeSpeed * 8;

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const map = new Map();
    drawFrame(map, state);
    maps.push(map);
    stepState(state);
  }

  return maps;
}

export { buildTetrisGameDemoMaps };
