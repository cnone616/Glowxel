const PIECE_COLORS = [
  "#00f0f0",
  "#f0f000",
  "#a000f0",
  "#00f000",
  "#f00000",
  "#0000f0",
  "#f0a000",
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

function clampUnit(value) {
  if (value < 0) {
    return 0;
  }
  if (value > 1) {
    return 1;
  }
  return value;
}

function parseHexColor(hex) {
  return {
    r: parseInt(hex.slice(1, 3), 16),
    g: parseInt(hex.slice(3, 5), 16),
    b: parseInt(hex.slice(5, 7), 16),
  };
}

function toHexColor({ r, g, b }) {
  const toHex = (value) => {
    const safe = Math.max(0, Math.min(255, Math.round(value)));
    return safe.toString(16).padStart(2, "0");
  };
  return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
}

function mixHexColor(base, target, ratio) {
  const safeRatio = clampUnit(ratio);
  const left = parseHexColor(base);
  const right = parseHexColor(target);
  return toHexColor({
    r: left.r * (1 - safeRatio) + right.r * safeRatio,
    g: left.g * (1 - safeRatio) + right.g * safeRatio,
    b: left.b * (1 - safeRatio) + right.b * safeRatio,
  });
}

function scaleHexColor(base, scale) {
  const safeScale = Math.max(0, scale);
  const rgb = parseHexColor(base);
  return toHexColor({
    r: rgb.r * safeScale,
    g: rgb.g * safeScale,
    b: rgb.b * safeScale,
  });
}

function setPixel(pixels, x, y, color) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
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

function normalizeShape(cells) {
  let minX = Infinity;
  let minY = Infinity;
  for (let i = 0; i < cells.length; i += 1) {
    minX = Math.min(minX, cells[i][0]);
    minY = Math.min(minY, cells[i][1]);
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

function easeOutCubic(value) {
  const safe = Math.max(0, Math.min(1, value));
  const inverse = 1 - safe;
  return 1 - inverse * inverse * inverse;
}

function chooseColors(selectedPieces) {
  if (!Array.isArray(selectedPieces) || selectedPieces.length === 0) {
    return PIECE_COLORS;
  }
  return selectedPieces
    .filter((index) => Number.isInteger(index) && index >= 0 && index < PIECE_COLORS.length)
    .map((index) => PIECE_COLORS[index]);
}

function choosePieceTypes(selectedPieces) {
  if (!Array.isArray(selectedPieces) || selectedPieces.length === 0) {
    return [0, 1, 2, 3, 4, 5, 6];
  }
  const filtered = selectedPieces.filter(
    (index) => Number.isInteger(index) && index >= 0 && index < 7,
  );
  return filtered.length > 0 ? filtered : [0, 1, 2, 3, 4, 5, 6];
}

function drawGridBackground(pixels, cellSize) {
  fillRect(pixels, 0, 0, 64, 64, "#060910");
  for (let pos = 0; pos < 64; pos += cellSize) {
    for (let cursor = 0; cursor < 64; cursor += 1) {
      setPixel(pixels, pos, cursor, "#0d1520");
      setPixel(pixels, cursor, pos, "#0d1520");
    }
  }
  fillRect(pixels, 0, 0, 64, 1, "#182433");
  fillRect(pixels, 0, 63, 64, 1, "#182433");
  fillRect(pixels, 0, 0, 1, 64, "#182433");
  fillRect(pixels, 63, 0, 1, 64, "#182433");
}

function buildTimeCells(showClock, cellSize) {
  const cols = Math.floor(64 / cellSize);
  const rows = Math.floor(64 / cellSize);
  const placements = [];

  if (!showClock) {
    return placements;
  }

  const now = new Date();
  const digits = `${String(now.getHours()).padStart(2, "0")}${String(
    now.getMinutes(),
  ).padStart(2, "0")}`;

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
    const value = Number(digits[digitIndex]);
    const pattern = DIGIT_PATTERNS[value];
    for (let row = 0; row < pattern.length; row += 1) {
      for (let col = 0; col < pattern[row].length; col += 1) {
        if (pattern[row][col] !== "1") {
          continue;
        }
        for (let sy = 0; sy < scale; sy += 1) {
          for (let sx = 0; sx < scale; sx += 1) {
            placements.push({
              x: cursorX + col * scale + sx,
              y: startY + row * scale + sy,
            });
          }
        }
      }
    }
    cursorX += 3 * scale + scale;
    if (digitIndex === 1) {
      for (let sy = 0; sy < scale; sy += 1) {
        for (let sx = 0; sx < scale; sx += 1) {
          placements.push({ x: cursorX + sx, y: startY + scale + sy });
          placements.push({ x: cursorX + sx, y: startY + scale * 3 + sy });
        }
      }
      cursorX += scale + scale;
    }
  }

  return placements;
}

function cellsToSet(cells) {
  const result = new Set();
  for (let index = 0; index < cells.length; index += 1) {
    result.add(`${cells[index].x},${cells[index].y}`);
  }
  return result;
}

function buildClockPlacements(targetCells, allowedPieces) {
  const remaining = cellsToSet(targetCells);
  const placements = [];
  let pieceCursor = 0;

  function getPriorityTypes() {
    const offset = pieceCursor % allowedPieces.length;
    return allowedPieces
      .slice(offset)
      .concat(allowedPieces.slice(0, offset));
  }

  function findPlacement(anchor) {
    const priorityTypes = getPriorityTypes();
    let best = null;

    for (let typeIndex = 0; typeIndex < priorityTypes.length; typeIndex += 1) {
      const pieceType = priorityTypes[typeIndex];
      const rotations = getPieceRotations(pieceType);

      for (let rotationIndex = 0; rotationIndex < rotations.length; rotationIndex += 1) {
        const rotation = rotations[rotationIndex];

        for (let anchorIndex = 0; anchorIndex < rotation.length; anchorIndex += 1) {
          const [offsetX, offsetY] = rotation[anchorIndex];
          const originX = anchor.x - offsetX;
          const originY = anchor.y - offsetY;
          const pieceCells = [];
          let valid = true;
          let neighborScore = 0;

          for (let blockIndex = 0; blockIndex < rotation.length; blockIndex += 1) {
            const cellX = originX + rotation[blockIndex][0];
            const cellY = originY + rotation[blockIndex][1];
            const key = `${cellX},${cellY}`;

            if (!remaining.has(key)) {
              valid = false;
              break;
            }

            pieceCells.push({ x: cellX, y: cellY });
          }

          if (!valid) {
            continue;
          }

          for (let blockIndex = 0; blockIndex < pieceCells.length; blockIndex += 1) {
            const cell = pieceCells[blockIndex];
            const neighbors = [
              `${cell.x - 1},${cell.y}`,
              `${cell.x + 1},${cell.y}`,
              `${cell.x},${cell.y - 1}`,
              `${cell.x},${cell.y + 1}`,
            ];
            for (let neighborIndex = 0; neighborIndex < neighbors.length; neighborIndex += 1) {
              if (remaining.has(neighbors[neighborIndex])) {
                neighborScore += 1;
              }
            }
          }

          const score =
            pieceCells.length * 100 +
            neighborScore -
            Math.abs(originX - anchor.x) -
            Math.abs(originY - anchor.y);

          if (!best || score > best.score) {
            best = {
              pieceType,
              cells: pieceCells,
              score,
            };
          }
        }
      }
    }

    return best;
  }

  while (remaining.size > 0) {
    const nextKey = Array.from(remaining)
      .map((item) => {
        const [x, y] = item.split(",").map(Number);
        return { key: item, x, y };
      })
      .sort((left, right) => right.y - left.y || left.x - right.x)[0];

    const placement = findPlacement(nextKey);
    if (placement) {
      placements.push(placement);
      for (let index = 0; index < placement.cells.length; index += 1) {
        remaining.delete(`${placement.cells[index].x},${placement.cells[index].y}`);
      }
    } else {
      placements.push({
        pieceType: allowedPieces[pieceCursor % allowedPieces.length],
        cells: [{ x: nextKey.x, y: nextKey.y }],
      });
      remaining.delete(nextKey.key);
    }
    pieceCursor += 1;
  }

  return placements.sort((left, right) => {
    const leftY = Math.max(...left.cells.map((cell) => cell.y));
    const rightY = Math.max(...right.cells.map((cell) => cell.y));
    const leftX = Math.min(...left.cells.map((cell) => cell.x));
    const rightX = Math.min(...right.cells.map((cell) => cell.x));
    return rightY - leftY || leftX - rightX;
  });
}

function canPlacePiece(board, cols, rows, cells, originX, originY) {
  for (let index = 0; index < cells.length; index += 1) {
    const x = originX + cells[index][0];
    const y = originY + cells[index][1];
    if (x < 0 || x >= cols || y >= rows) {
      return false;
    }
    if (y >= 0 && board.has(`${x},${y}`)) {
      return false;
    }
  }
  return true;
}

function buildScreenSaverPlacements(cellSize, allowedPieces) {
  const cols = Math.floor(64 / cellSize);
  const rows = Math.floor(64 / cellSize);
  const board = new Set();
  const placements = [];
  const pieceCount = Math.max(8, Math.min(14, Math.floor((cols * 0.72) / 2)));

  for (let index = 0; index < pieceCount; index += 1) {
    const pieceType = allowedPieces[index % allowedPieces.length];
    const rotations = getPieceRotations(pieceType);
    const rotation = rotations[(index * 3 + pieceType) % rotations.length];
    const width = Math.max(...rotation.map((cell) => cell[0])) + 1;
    const maxX = Math.max(0, cols - width);
    const originX = (index * 5 + pieceType * 3) % (maxX + 1);
    let originY = -4;

    while (canPlacePiece(board, cols, rows, rotation, originX, originY + 1)) {
      originY += 1;
    }

    const finalCells = rotation
      .map(([x, y]) => ({ x: originX + x, y: originY + y }))
      .filter((cell) => cell.y >= 0 && cell.y < rows);

    for (let cellIndex = 0; cellIndex < finalCells.length; cellIndex += 1) {
      board.add(`${finalCells[cellIndex].x},${finalCells[cellIndex].y}`);
    }

    placements.push({
      pieceType,
      cells: finalCells,
    });
  }

  return placements.sort((left, right) => {
    const leftY = Math.max(...left.cells.map((cell) => cell.y));
    const rightY = Math.max(...right.cells.map((cell) => cell.y));
    return leftY - rightY;
  });
}

function buildAnimatedPlacements(config) {
  const safeConfig = config || {};
  const cellSize = Math.max(1, Math.min(3, Number(safeConfig.cellSize) || 2));
  const allowedPieces = choosePieceTypes(safeConfig.pieces);
  const showClock = safeConfig.showClock !== false;
  const targetCells = buildTimeCells(showClock, cellSize);
  const placements = showClock
    ? buildClockPlacements(targetCells, allowedPieces)
    : buildScreenSaverPlacements(cellSize, allowedPieces);

  return placements.map((placement, index) => {
    const minX = Math.min(...placement.cells.map((cell) => cell.x));
    const minY = Math.min(...placement.cells.map((cell) => cell.y));
    const maxX = Math.max(...placement.cells.map((cell) => cell.x));
    const width = maxX - minX + 1;
    const cols = Math.floor(64 / cellSize);
    const shift = index % 2 === 0 ? -(4 + (index % 3)) : 4 + (index % 3);
    let spawnX = minX + shift;
    if (spawnX < 0) {
      spawnX = 0;
    }
    if (spawnX > cols - width) {
      spawnX = cols - width;
    }
    return {
      ...placement,
      spawnX,
      spawnY: -6 - (index % 4) * 3,
      minX,
      minY,
    };
  });
}

function drawPieceCell(pixels, cellX, cellY, cellSize, color, landed) {
  const px = cellX * cellSize;
  const py = cellY * cellSize;
  fillRect(pixels, px, py, cellSize, cellSize, color);

  if (cellSize >= 2) {
    const highlight = mixHexColor(color, "#ffffff", 0.32);
    const shadow = scaleHexColor(color, 0.42);
    fillRect(pixels, px, py, cellSize, 1, highlight);
    fillRect(pixels, px, py, 1, cellSize, highlight);
    if (landed) {
      fillRect(pixels, px, py + cellSize - 1, cellSize, 1, shadow);
      fillRect(pixels, px + cellSize - 1, py, 1, cellSize, shadow);
    }
  }
}

function drawGhostCell(pixels, cellX, cellY, cellSize) {
  const px = cellX * cellSize;
  const py = cellY * cellSize;
  fillRect(pixels, px, py, cellSize, cellSize, "#102038");
  if (cellSize >= 2) {
    fillRect(pixels, px, py, cellSize, 1, "#214166");
    fillRect(pixels, px, py, 1, cellSize, "#214166");
  }
}

export function buildTetrisPreviewFrames(config) {
  const safeConfig = config || {};
  const cellSize = Math.max(1, Math.min(3, Number(safeConfig.cellSize) || 2));
  const colors = chooseColors(safeConfig.pieces);
  const showClock = safeConfig.showClock !== false;
  const targetCells = buildTimeCells(showClock, cellSize);
  const animatedPlacements = buildAnimatedPlacements(safeConfig);
  const frameCount = 36;
  const frames = [];
  const speedLabel =
    typeof safeConfig.speed === "string" ? safeConfig.speed : "normal";
  const speedFactor = speedLabel === "slow" ? 0.82 : speedLabel === "fast" ? 1.3 : 1;

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const pixels = new Map();
    drawGridBackground(pixels, cellSize);

    if (showClock) {
      for (let index = 0; index < targetCells.length; index += 1) {
        const cell = targetCells[index];
        drawGhostCell(pixels, cell.x, cell.y, cellSize);
      }
    }

    for (let placementIndex = 0; placementIndex < animatedPlacements.length; placementIndex += 1) {
      const placement = animatedPlacements[placementIndex];
      let color = colors[placement.pieceType % colors.length];
      const startFrame = placementIndex * 0.72;
      const duration = 5.8 / speedFactor;
      const progress = (frameIndex - startFrame) / duration;

      if (progress <= 0) {
        continue;
      }

      const eased = easeOutCubic(Math.min(1, progress));
      const currentOffsetX = Math.round(
        placement.spawnX + (placement.minX - placement.spawnX) * eased,
      );
      const currentOffsetY = Math.round(
        placement.spawnY + (placement.minY - placement.spawnY) * eased,
      );
      const landed = progress >= 1;
      const holdPulseFrame = Math.max(0, frameIndex - (frameCount - 8));
      const holdPulse = landed && holdPulseFrame > 0;
      if (holdPulse) {
        const pulse = 0.18 + 0.18 * Math.sin((holdPulseFrame / 8) * Math.PI * 2);
        color = mixHexColor(color, "#dbeafe", pulse);
      }

      for (let cellIndex = 0; cellIndex < placement.cells.length; cellIndex += 1) {
        const cell = placement.cells[cellIndex];
        const localX = cell.x - placement.minX;
        const localY = cell.y - placement.minY;
        const drawX = currentOffsetX + localX;
        const drawY = currentOffsetY + localY;
        drawPieceCell(pixels, drawX, drawY, cellSize, color, landed);

        if (holdPulse && cellSize >= 2) {
          const baseX = drawX * cellSize;
          const baseY = drawY * cellSize;
          setPixel(pixels, baseX - 1, baseY, "#dbeafe");
          setPixel(pixels, baseX, baseY - 1, "#dbeafe");
          setPixel(pixels, baseX + cellSize, baseY + cellSize - 1, "#93c5fd");
          setPixel(pixels, baseX + cellSize - 1, baseY + cellSize, "#93c5fd");
        }
      }
    }

    frames.push(pixels);
  }

  return frames;
}
