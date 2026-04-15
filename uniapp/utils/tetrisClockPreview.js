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

const SEGMENT_MAP = [
  ["a", "b", "c", "d", "e", "f"],
  ["b", "c"],
  ["a", "b", "g", "e", "d"],
  ["a", "b", "g", "c", "d"],
  ["f", "g", "b", "c"],
  ["a", "f", "g", "c", "d"],
  ["a", "f", "g", "e", "c", "d"],
  ["a", "b", "c"],
  ["a", "b", "c", "d", "e", "f", "g"],
  ["a", "b", "c", "d", "f", "g"],
];

const ORIGINAL_TETRIS_COLORS = {
  myRED: { r: 255, g: 0, b: 0 },
  myGREEN: { r: 0, g: 255, b: 0 },
  myBLUE: { r: 48, g: 73, b: 255 },
  myWHITE: { r: 255, g: 255, b: 255 },
  myYELLOW: { r: 255, g: 255, b: 0 },
  myCYAN: { r: 0, g: 255, b: 255 },
  myMAGENTA: { r: 255, g: 0, b: 255 },
  myORANGE: { r: 255, g: 96, b: 0 },
};

const ORIGINAL_TETRIS_COLOR_SEQUENCE = [
  "myRED",
  "myGREEN",
  "myBLUE",
  "myWHITE",
  "myYELLOW",
  "myCYAN",
  "myMAGENTA",
  "myORANGE",
];

const ORIGINAL_DIGIT_X_SHIFTS = [1, 8, 18, 25];
const ORIGINAL_PREVIEW_SCALE = 2;
const ORIGINAL_PREVIEW_OFFSET_X = 0;
const ORIGINAL_PREVIEW_OFFSET_Y = 12;
const ORIGINAL_FALL_START_Y = -5;
function buildOriginalColonFalls(digits) {
  const topDigit = Number(digits[1]);
  const bottomDigit = Number(digits[2]);
  const topColor =
    ORIGINAL_TETRIS_COLOR_SEQUENCE[
      (topDigit + 2) % ORIGINAL_TETRIS_COLOR_SEQUENCE.length
    ];
  const bottomColor =
    ORIGINAL_TETRIS_COLOR_SEQUENCE[
      (bottomDigit + 5) % ORIGINAL_TETRIS_COLOR_SEQUENCE.length
    ];

  return [
    { block: 0, color: bottomColor, x: 15, y: 12, rot: 0 },
    { block: 0, color: topColor, x: 15, y: 8, rot: 0 },
  ];
}

const ORIGINAL_DIGIT_FALLS = {
  0: [
    { block: 2, color: "myCYAN", x: 4, y: 16, rot: 0 },
    { block: 4, color: "myORANGE", x: 2, y: 16, rot: 1 },
    { block: 3, color: "myYELLOW", x: 0, y: 16, rot: 1 },
    { block: 6, color: "myMAGENTA", x: 1, y: 16, rot: 1 },
    { block: 5, color: "myGREEN", x: 4, y: 14, rot: 0 },
    { block: 6, color: "myMAGENTA", x: 0, y: 13, rot: 3 },
    { block: 5, color: "myGREEN", x: 4, y: 12, rot: 0 },
    { block: 5, color: "myGREEN", x: 0, y: 11, rot: 0 },
    { block: 6, color: "myMAGENTA", x: 4, y: 10, rot: 1 },
    { block: 6, color: "myMAGENTA", x: 0, y: 9, rot: 1 },
    { block: 5, color: "myGREEN", x: 1, y: 8, rot: 1 },
    { block: 2, color: "myCYAN", x: 3, y: 8, rot: 3 },
  ],
  1: [
    { block: 2, color: "myCYAN", x: 4, y: 16, rot: 0 },
    { block: 3, color: "myYELLOW", x: 4, y: 15, rot: 1 },
    { block: 3, color: "myYELLOW", x: 5, y: 13, rot: 3 },
    { block: 2, color: "myCYAN", x: 4, y: 11, rot: 2 },
    { block: 0, color: "myRED", x: 4, y: 8, rot: 0 },
  ],
  2: [
    { block: 0, color: "myRED", x: 4, y: 16, rot: 0 },
    { block: 3, color: "myYELLOW", x: 0, y: 16, rot: 1 },
    { block: 1, color: "myBLUE", x: 1, y: 16, rot: 3 },
    { block: 1, color: "myBLUE", x: 1, y: 15, rot: 0 },
    { block: 3, color: "myYELLOW", x: 1, y: 12, rot: 2 },
    { block: 1, color: "myBLUE", x: 0, y: 12, rot: 1 },
    { block: 2, color: "myCYAN", x: 3, y: 12, rot: 3 },
    { block: 0, color: "myRED", x: 4, y: 10, rot: 0 },
    { block: 3, color: "myYELLOW", x: 1, y: 8, rot: 0 },
    { block: 2, color: "myCYAN", x: 3, y: 8, rot: 3 },
    { block: 1, color: "myBLUE", x: 0, y: 8, rot: 1 },
  ],
  3: [
    { block: 1, color: "myBLUE", x: 3, y: 16, rot: 3 },
    { block: 2, color: "myCYAN", x: 0, y: 16, rot: 1 },
    { block: 3, color: "myYELLOW", x: 1, y: 15, rot: 2 },
    { block: 0, color: "myRED", x: 4, y: 14, rot: 0 },
    { block: 3, color: "myYELLOW", x: 1, y: 12, rot: 2 },
    { block: 1, color: "myBLUE", x: 0, y: 12, rot: 1 },
    { block: 3, color: "myYELLOW", x: 5, y: 12, rot: 3 },
    { block: 2, color: "myCYAN", x: 3, y: 11, rot: 0 },
    { block: 3, color: "myYELLOW", x: 1, y: 8, rot: 0 },
    { block: 1, color: "myBLUE", x: 0, y: 8, rot: 1 },
    { block: 2, color: "myCYAN", x: 3, y: 8, rot: 3 },
  ],
  4: [
    { block: 0, color: "myRED", x: 4, y: 16, rot: 0 },
    { block: 0, color: "myRED", x: 4, y: 14, rot: 0 },
    { block: 3, color: "myYELLOW", x: 1, y: 12, rot: 0 },
    { block: 1, color: "myBLUE", x: 0, y: 12, rot: 1 },
    { block: 2, color: "myCYAN", x: 0, y: 10, rot: 0 },
    { block: 2, color: "myCYAN", x: 3, y: 12, rot: 3 },
    { block: 3, color: "myYELLOW", x: 4, y: 10, rot: 3 },
    { block: 2, color: "myCYAN", x: 0, y: 9, rot: 2 },
    { block: 3, color: "myYELLOW", x: 5, y: 10, rot: 1 },
  ],
  5: [
    { block: 0, color: "myRED", x: 0, y: 16, rot: 0 },
    { block: 2, color: "myCYAN", x: 2, y: 16, rot: 1 },
    { block: 2, color: "myCYAN", x: 3, y: 15, rot: 0 },
    { block: 3, color: "myYELLOW", x: 5, y: 16, rot: 1 },
    { block: 3, color: "myYELLOW", x: 1, y: 12, rot: 0 },
    { block: 1, color: "myBLUE", x: 0, y: 12, rot: 1 },
    { block: 2, color: "myCYAN", x: 3, y: 12, rot: 3 },
    { block: 0, color: "myRED", x: 0, y: 10, rot: 0 },
    { block: 3, color: "myYELLOW", x: 1, y: 8, rot: 2 },
    { block: 1, color: "myBLUE", x: 0, y: 8, rot: 1 },
    { block: 2, color: "myCYAN", x: 3, y: 8, rot: 3 },
  ],
  6: [
    { block: 2, color: "myCYAN", x: 0, y: 16, rot: 1 },
    { block: 5, color: "myGREEN", x: 2, y: 16, rot: 1 },
    { block: 6, color: "myMAGENTA", x: 0, y: 15, rot: 3 },
    { block: 6, color: "myMAGENTA", x: 4, y: 16, rot: 3 },
    { block: 5, color: "myGREEN", x: 4, y: 14, rot: 0 },
    { block: 3, color: "myYELLOW", x: 1, y: 12, rot: 2 },
    { block: 2, color: "myCYAN", x: 0, y: 13, rot: 2 },
    { block: 3, color: "myYELLOW", x: 2, y: 11, rot: 0 },
    { block: 0, color: "myRED", x: 0, y: 10, rot: 0 },
    { block: 3, color: "myYELLOW", x: 1, y: 8, rot: 0 },
    { block: 1, color: "myBLUE", x: 0, y: 8, rot: 1 },
    { block: 2, color: "myCYAN", x: 3, y: 8, rot: 3 },
  ],
  7: [
    { block: 0, color: "myRED", x: 4, y: 16, rot: 0 },
    { block: 1, color: "myBLUE", x: 4, y: 14, rot: 0 },
    { block: 3, color: "myYELLOW", x: 5, y: 13, rot: 1 },
    { block: 2, color: "myCYAN", x: 4, y: 11, rot: 2 },
    { block: 3, color: "myYELLOW", x: 1, y: 8, rot: 2 },
    { block: 2, color: "myCYAN", x: 3, y: 8, rot: 3 },
    { block: 1, color: "myBLUE", x: 0, y: 8, rot: 1 },
  ],
  8: [
    { block: 3, color: "myYELLOW", x: 1, y: 16, rot: 0 },
    { block: 6, color: "myMAGENTA", x: 0, y: 16, rot: 1 },
    { block: 3, color: "myYELLOW", x: 5, y: 16, rot: 1 },
    { block: 1, color: "myBLUE", x: 2, y: 15, rot: 3 },
    { block: 4, color: "myORANGE", x: 0, y: 14, rot: 0 },
    { block: 1, color: "myBLUE", x: 1, y: 12, rot: 3 },
    { block: 6, color: "myMAGENTA", x: 4, y: 13, rot: 1 },
    { block: 2, color: "myCYAN", x: 0, y: 11, rot: 1 },
    { block: 4, color: "myORANGE", x: 0, y: 10, rot: 0 },
    { block: 4, color: "myORANGE", x: 4, y: 11, rot: 0 },
    { block: 5, color: "myGREEN", x: 0, y: 8, rot: 1 },
    { block: 5, color: "myGREEN", x: 2, y: 8, rot: 1 },
    { block: 1, color: "myBLUE", x: 4, y: 9, rot: 2 },
  ],
  9: [
    { block: 0, color: "myRED", x: 0, y: 16, rot: 0 },
    { block: 3, color: "myYELLOW", x: 2, y: 16, rot: 0 },
    { block: 1, color: "myBLUE", x: 2, y: 15, rot: 3 },
    { block: 1, color: "myBLUE", x: 4, y: 15, rot: 2 },
    { block: 3, color: "myYELLOW", x: 1, y: 12, rot: 2 },
    { block: 3, color: "myYELLOW", x: 5, y: 12, rot: 3 },
    { block: 5, color: "myGREEN", x: 0, y: 12, rot: 0 },
    { block: 1, color: "myBLUE", x: 2, y: 11, rot: 3 },
    { block: 5, color: "myGREEN", x: 4, y: 9, rot: 0 },
    { block: 6, color: "myMAGENTA", x: 0, y: 10, rot: 1 },
    { block: 5, color: "myGREEN", x: 0, y: 8, rot: 1 },
    { block: 6, color: "myMAGENTA", x: 2, y: 8, rot: 2 },
  ],
};

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
    rotations.push(current);
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

function rgbToText(color) {
  return `rgb(${clamp(Math.round(color.r), 0, 255)}, ${clamp(
    Math.round(color.g),
    0,
    255,
  )}, ${clamp(Math.round(color.b), 0, 255)})`;
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
  fillRect(pixels, 0, 0, PANEL_SIZE, PANEL_SIZE, "rgb(17, 24, 38)");
  for (let pos = 0; pos < PANEL_SIZE; pos += cellSize) {
    for (let cursor = 0; cursor < PANEL_SIZE; cursor += 1) {
      setPixel(pixels, pos, cursor, "rgb(36, 50, 72)");
      setPixel(pixels, cursor, pos, "rgb(36, 50, 72)");
    }
  }
}

function drawGhostCell(pixels, cellX, cellY, cellSize, pulseBright) {
  const px = cellX * cellSize;
  const py = cellY * cellSize;
  const baseColor = pulseBright
    ? { r: 126, g: 190, b: 255 }
    : { r: 98, g: 156, b: 228 };
  const highlightColor = pulseBright
    ? { r: 208, g: 236, b: 255 }
    : { r: 170, g: 214, b: 255 };
  const shadowColor = pulseBright
    ? { r: 54, g: 88, b: 138 }
    : { r: 42, g: 70, b: 114 };
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
      setPixel(pixels, px + dx, py + dy, rgbToText(color));
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
      setPixel(pixels, px + dx, py + dy, rgbToText(color));
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

function selectClockGlyphMetrics(cols, rows) {
  if (cols >= 48 && rows >= 16) {
    return { thickness: 4, horizontal: 4, vertical: 4 };
  }
  if (cols >= 30 && rows >= 8) {
    return { thickness: 2, horizontal: 2, vertical: 2 };
  }
  return { thickness: 1, horizontal: 1, vertical: 1 };
}

function paintTargetRect(targetMap, startX, startY, width, height, cols, rows) {
  for (let row = 0; row < height; row += 1) {
    for (let col = 0; col < width; col += 1) {
      const x = startX + col;
      const y = startY + row;
      if (x >= 0 && x < cols && y >= 0 && y < rows) {
        targetMap[y][x] = true;
      }
    }
  }
}

function paintDigitSegments(targetMap, digit, originX, originY, metrics, cols, rows) {
  const { thickness, horizontal, vertical } = metrics;
  const digitWidth = horizontal + thickness * 2;
  const segmentRects = {
    a: { x: originX + thickness, y: originY, width: horizontal, height: thickness },
    b: {
      x: originX + digitWidth - thickness,
      y: originY + thickness,
      width: thickness,
      height: vertical,
    },
    c: {
      x: originX + digitWidth - thickness,
      y: originY + vertical + thickness * 2,
      width: thickness,
      height: vertical,
    },
    d: {
      x: originX + thickness,
      y: originY + vertical * 2 + thickness * 2,
      width: horizontal,
      height: thickness,
    },
    e: {
      x: originX,
      y: originY + vertical + thickness * 2,
      width: thickness,
      height: vertical,
    },
    f: {
      x: originX,
      y: originY + thickness,
      width: thickness,
      height: vertical,
    },
    g: {
      x: originX + thickness,
      y: originY + vertical + thickness,
      width: horizontal,
      height: thickness,
    },
  };

  const segments = SEGMENT_MAP[digit];
  for (let index = 0; index < segments.length; index += 1) {
    const rect = segmentRects[segments[index]];
    paintTargetRect(targetMap, rect.x, rect.y, rect.width, rect.height, cols, rows);
  }
}

function buildTargetMap(cols, rows, showClock, clock) {
  const targetMap = createMatrix(rows, cols, false);
  if (!showClock) {
    return targetMap;
  }

  const metrics = selectClockGlyphMetrics(cols, rows);
  const { thickness, horizontal, vertical } = metrics;
  const digitWidth = horizontal + thickness * 2;
  const digitHeight = vertical * 2 + thickness * 3;
  const colonWidth = thickness;
  const gap = thickness;
  const digits = `${String(clock.hours).padStart(2, "0")}${String(clock.minutes).padStart(2, "0")}`;
  const totalWidth = digitWidth * 4 + gap * 3 + colonWidth + gap * 2;
  const totalHeight = digitHeight;
  const startX = Math.floor((cols - totalWidth) / 2);
  const startY = Math.max(0, rows - totalHeight);
  let cursorX = startX;

  for (let digitIndex = 0; digitIndex < 4; digitIndex += 1) {
    const digit = Number(digits[digitIndex]);
    paintDigitSegments(targetMap, digit, cursorX, startY, metrics, cols, rows);
    cursorX += digitWidth + gap;
    if (digitIndex === 1) {
      const topDotY = startY + vertical;
      const bottomDotY = startY + vertical + thickness * 2;
      paintTargetRect(targetMap, cursorX, topDotY, colonWidth, thickness, cols, rows);
      paintTargetRect(targetMap, cursorX, bottomDotY, colonWidth, thickness, cols, rows);
      cursorX += colonWidth + gap * 2;
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
        score += ny * 24;
        if (
          ny + 1 >= state.rows ||
          state.board[ny + 1][nx] !== 0 ||
          !state.targetMap[ny + 1][nx]
        ) {
          score += 120;
        } else {
          score -= 140;
        }
        if (nx > 0 && state.board[ny][nx - 1] !== 0) {
          score += 26;
        }
        if (nx + 1 < state.cols && state.board[ny][nx + 1] !== 0) {
          score += 26;
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

      score += y * 24;
      score -= x;
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

  if (state.showClock) {
    for (let offset = 0; offset < enabled.length; offset += 1) {
      const candidateType = enabled[(pieceIndex + offset) % enabled.length];
      const placement = buildClockPlacement(state, candidateType);
      if (!placement) {
        continue;
      }

      state.curType = candidateType;
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

  state.curType = enabled[pieceIndex];
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
        drawGhostCell(pixels, x, y, state.cellSize, pulseBright);
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

function getOriginalClockDigits() {
  const clock = getClockSnapshot();
  return `${String(clock.hours).padStart(2, "0")}${String(clock.minutes).padStart(2, "0")}`;
}

function createOriginalDigitStates(digits) {
  const states = [];
  for (let index = 0; index < 4; index += 1) {
    states.push({
      digit: Number(digits[index]),
      blockIndex: 0,
      fallIndex: 0,
      xShift: ORIGINAL_DIGIT_X_SHIFTS[index],
    });
  }
  return states;
}

function createOriginalColonStates(colonFalls) {
  const states = [];
  for (let index = 0; index < colonFalls.length; index += 1) {
    states.push({
      blockIndex: 0,
      fallIndex: 0,
      started: index === 0,
    });
  }
  return states;
}

function getOriginalShapeCells(blockType, rotation) {
  if (blockType === 0) {
    return [[0, 0], [1, 0], [0, -1], [1, -1]];
  }
  if (blockType === 1) {
    if (rotation === 0) return [[0, 0], [1, 0], [0, -1], [0, -2]];
    if (rotation === 1) return [[0, 0], [0, -1], [1, -1], [2, -1]];
    if (rotation === 2) return [[1, 0], [1, -1], [1, -2], [0, -2]];
    return [[0, 0], [1, 0], [2, 0], [2, -1]];
  }
  if (blockType === 2) {
    if (rotation === 0) return [[0, 0], [1, 0], [1, -1], [1, -2]];
    if (rotation === 1) return [[0, 0], [1, 0], [2, 0], [0, -1]];
    if (rotation === 2) return [[0, 0], [0, -1], [0, -2], [1, -2]];
    return [[0, -1], [1, -1], [2, -1], [2, 0]];
  }
  if (blockType === 3) {
    if (rotation === 1 || rotation === 3) {
      return [[0, 0], [0, -1], [0, -2], [0, -3]];
    }
    return [[0, 0], [1, 0], [2, 0], [3, 0]];
  }
  if (blockType === 4) {
    if (rotation === 1 || rotation === 3) {
      return [[0, 0], [1, 0], [1, -1], [2, -1]];
    }
    return [[1, 0], [0, -1], [1, -1], [0, -2]];
  }
  if (blockType === 5) {
    if (rotation === 1 || rotation === 3) {
      return [[1, 0], [2, 0], [0, -1], [1, -1]];
    }
    return [[0, 0], [0, -1], [1, -1], [1, -2]];
  }
  if (rotation === 0) return [[0, 0], [1, 0], [2, 0], [1, -1]];
  if (rotation === 1) return [[0, 0], [0, -1], [0, -2], [1, -1]];
  if (rotation === 2) return [[1, 0], [0, -1], [1, -1], [2, -1]];
  return [[1, 0], [0, -1], [1, -1], [1, -2]];
}

function resolveOriginalFallingRotation(targetRotation, fallIndex, stopY) {
  const travelDistance = Math.max(1, stopY - ORIGINAL_FALL_START_Y);
  let rotations = targetRotation;
  if (rotations === 1) {
    if (fallIndex < Math.floor(travelDistance / 2)) {
      rotations = 0;
    }
  } else if (rotations === 2) {
    if (fallIndex < Math.floor(travelDistance / 3)) {
      rotations = 0;
    } else if (fallIndex < Math.floor((travelDistance / 3) * 2)) {
      rotations = 1;
    }
  } else if (rotations === 3) {
    if (fallIndex < Math.floor(travelDistance / 4)) {
      rotations = 0;
    } else if (fallIndex < Math.floor((travelDistance / 4) * 2)) {
      rotations = 1;
    } else if (fallIndex < Math.floor((travelDistance / 4) * 3)) {
      rotations = 2;
    }
  }
  return rotations;
}

function drawOriginalBlockShape(pixels, block, colorName, xPos, yPos, rotation, scale, offsetX, offsetY) {
  const color = ORIGINAL_TETRIS_COLORS[colorName] || ORIGINAL_TETRIS_COLORS.myWHITE;
  const fillColor = rgbToText(color);
  const cells = getOriginalShapeCells(block, rotation);
  for (let index = 0; index < cells.length; index += 1) {
    const px = offsetX + (xPos + cells[index][0]) * scale;
    const py = offsetY + (yPos + cells[index][1]) * scale;
    fillRect(pixels, px, py, scale, scale, fillColor);
  }
}

function getOriginalCurrentFallY(fallIndex) {
  return ORIGINAL_FALL_START_Y + fallIndex - 1;
}

function buildOriginalTetrisFrame(states, colonFalls, colonStates) {
  const pixels = new Map();
  const scale = ORIGINAL_PREVIEW_SCALE;
  const offsetX = ORIGINAL_PREVIEW_OFFSET_X;
  const offsetY = ORIGINAL_PREVIEW_OFFSET_Y;

  for (let stateIndex = 0; stateIndex < states.length; stateIndex += 1) {
    const state = states[stateIndex];
    const instructions = ORIGINAL_DIGIT_FALLS[state.digit] || [];
    for (let index = 0; index < state.blockIndex; index += 1) {
      const item = instructions[index];
      drawOriginalBlockShape(
        pixels,
        item.block,
        item.color,
        item.x + state.xShift,
        item.y - 1,
        item.rot,
        scale,
        offsetX,
        offsetY,
      );
    }

    if (state.blockIndex < instructions.length) {
      const current = instructions[state.blockIndex];
      const rotation = resolveOriginalFallingRotation(
        current.rot,
        state.fallIndex,
        current.y,
      );
      drawOriginalBlockShape(
        pixels,
        current.block,
        current.color,
        current.x + state.xShift,
        getOriginalCurrentFallY(state.fallIndex),
        rotation,
        scale,
        offsetX,
        offsetY,
      );
    }
  }

  for (let stateIndex = 0; stateIndex < colonStates.length; stateIndex += 1) {
    const state = colonStates[stateIndex];
    const item = colonFalls[stateIndex];
    if (!state.started) {
      continue;
    }
    if (state.blockIndex > 0) {
      drawOriginalBlockShape(
        pixels,
        item.block,
        item.color,
        item.x,
        item.y - 1,
        item.rot,
        scale,
        offsetX,
        offsetY,
      );
      continue;
    }
    drawOriginalBlockShape(
      pixels,
      item.block,
      item.color,
      item.x,
      getOriginalCurrentFallY(state.fallIndex),
      item.rot,
      scale,
      offsetX,
      offsetY,
    );
  }

  return pixels;
}

function stepOriginalDigitStates(states) {
  for (let index = 0; index < states.length; index += 1) {
    const state = states[index];
    const instructions = ORIGINAL_DIGIT_FALLS[state.digit] || [];
    if (state.blockIndex >= instructions.length) {
      continue;
    }
    const current = instructions[state.blockIndex];
    state.fallIndex += 1;
    const travelDistance = current.y - ORIGINAL_FALL_START_Y;
    if (state.fallIndex > travelDistance) {
      state.fallIndex = 0;
      state.blockIndex += 1;
    }
  }
  for (let index = 0; index < states.length; index += 1) {
    const state = states[index];
    const instructions = ORIGINAL_DIGIT_FALLS[state.digit] || [];
    if (state.blockIndex < instructions.length) {
      return false;
    }
  }
  return true;
}

function stepOriginalColonStates(colonFalls, colonStates) {
  for (let index = 0; index < colonStates.length; index += 1) {
    const state = colonStates[index];
    const item = colonFalls[index];
    if (!state.started) {
      continue;
    }
    if (state.blockIndex > 0) {
      continue;
    }
    state.fallIndex += 1;
    const travelDistance = item.y - ORIGINAL_FALL_START_Y;
    if (state.fallIndex > travelDistance) {
      state.fallIndex = 0;
      state.blockIndex = 1;
      if (index + 1 < colonStates.length) {
        colonStates[index + 1].started = true;
      }
    }
    return false;
  }

  return true;
}

function buildOriginalTetrisPreviewFrames(config) {
  const digits = getOriginalClockDigits();
  const states = createOriginalDigitStates(digits);
  const colonFalls = buildOriginalColonFalls(digits);
  const colonStates = createOriginalColonStates(colonFalls);
  const frames = [];
  const speed = getDropSpeed(config);
  const holdFrames = speed <= 90 ? 40 : speed <= 170 ? 56 : 68;

  frames.push(buildOriginalTetrisFrame(states, colonFalls, colonStates));

  for (let frameIndex = 0; frameIndex < 320; frameIndex += 1) {
    const digitsDone = stepOriginalDigitStates(states);
    const colonDone = stepOriginalColonStates(colonFalls, colonStates);
    frames.push(buildOriginalTetrisFrame(states, colonFalls, colonStates));
    if (digitsDone && colonDone) {
      break;
    }
  }

  for (let holdIndex = 0; holdIndex < holdFrames; holdIndex += 1) {
    frames.push(buildOriginalTetrisFrame(states, colonFalls, colonStates));
  }

  return frames;
}

export function buildTetrisPreviewFrames(config) {
  const safeConfig =
    config && typeof config === "object" ? config : {};
  return buildOriginalTetrisPreviewFrames(safeConfig);
}
