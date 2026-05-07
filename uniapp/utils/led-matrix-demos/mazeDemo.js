import {
  drawClockTextToPixels,
  drawTinyTextToPixels,
  getClockTextHeight,
  getClockTextWidth,
  getCurrentDateText,
  getCurrentTimeText,
  getTinyTextWidth,
} from "../clockCanvas.js";
import { fillRect, normalizeSpeed } from "./common.js";
import { createMazeModeConfig } from "../mazeModeConfig.js";

const DISPLAY_SIZE = 64;
const MAZE_INFO_PANEL_METRICS = {
  panelPaddingX: 3,
  panelPaddingTop: 2,
  panelPaddingBottom: 2,
  sectionGapY: 1,
  reservedMarginX: 0,
  reservedMarginY: 0,
};

const MAZE_LAYOUTS = {
  wide: {
    borderSize: 1,
    wallSize: 1,
    cellSize: 2,
    cellCount: 21,
  },
  dense: {
    borderSize: 0,
    wallSize: 1,
    cellSize: 1,
    cellCount: 32,
  },
};

const MAZE_RUNTIME_COLORS = {
  wall: { r: 12, g: 12, b: 14 },
  generationCursor: { r: 76, g: 220, b: 72 },
  solveCursor: { r: 255, g: 196, b: 92 },
  revealCursor: { r: 255, g: 244, b: 172 },
  start: { r: 40, g: 255, b: 120 },
  end: { r: 90, g: 188, b: 255 },
};

const DIRS = [
  { dx: 0, dy: -1, key: "top", opposite: "bottom" },
  { dx: 1, dy: 0, key: "right", opposite: "left" },
  { dx: 0, dy: 1, key: "bottom", opposite: "top" },
  { dx: -1, dy: 0, key: "left", opposite: "right" },
];

function resolveMazeLayout(options) {
  if (options && options.mazeSizeMode === "dense") {
    return {
      ...MAZE_LAYOUTS.dense,
      stride: MAZE_LAYOUTS.dense.cellSize + MAZE_LAYOUTS.dense.wallSize,
    };
  }
  return {
    ...MAZE_LAYOUTS.wide,
    stride: MAZE_LAYOUTS.wide.cellSize + MAZE_LAYOUTS.wide.wallSize,
  };
}

function rectanglesIntersect(left, right) {
  return (
    left.x < right.x + right.width &&
    left.x + left.width > right.x &&
    left.y < right.y + right.height &&
    left.y + left.height > right.y
  );
}

function drawRectOutline(map, x, y, width, height, r, g, b) {
  fillRect(map, x, y, width, 1, r, g, b);
  fillRect(map, x, y + height - 1, width, 1, r, g, b);
  fillRect(map, x, y, 1, height, r, g, b);
  fillRect(map, x + width - 1, y, 1, height, r, g, b);
}

function normalizeHexColorText(value) {
  if (typeof value !== "string") {
    return null;
  }
  const body = value.trim().replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    return null;
  }
  return `#${body.toLowerCase()}`;
}

function hexToRgb(hex) {
  const normalized = normalizeHexColorText(hex);
  if (normalized === null) {
    throw new Error("迷宫颜色配置无效");
  }
  return {
    r: parseInt(normalized.slice(1, 3), 16),
    g: parseInt(normalized.slice(3, 5), 16),
    b: parseInt(normalized.slice(5, 7), 16),
  };
}

function interpolateColor(startColor, endColor, ratio) {
  return {
    r: Math.round(startColor.r + (endColor.r - startColor.r) * ratio),
    g: Math.round(startColor.g + (endColor.g - startColor.g) * ratio),
    b: Math.round(startColor.b + (endColor.b - startColor.b) * ratio),
  };
}

function resolveMazeRuntimeConfig(speed, options) {
  if (!options || typeof options !== "object") {
    return null;
  }
  return createMazeModeConfig({
    speed,
    mazeSizeMode: options.mazeSizeMode,
    showClock: options.showClock,
    panelBgColor: options.panelBgColor,
    borderColor: options.borderColor,
    timeColor: options.timeColor,
    dateColor: options.dateColor,
    generationPathColor: options.generationPathColor,
    searchVisitedColor: options.searchVisitedColor,
    searchFrontierColor: options.searchFrontierColor,
    solvedPathStartColor: options.solvedPathStartColor,
    solvedPathEndColor: options.solvedPathEndColor,
  });
}

function buildMazeInfoPanel(showClock) {
  if (!showClock) {
    return null;
  }

  const timeText = getCurrentTimeText(false, 24);
  const dateText = getCurrentDateText();
  const timeScale = 2;
  const dateScale = 1;
  const timeWidth = getClockTextWidth(timeText, "minimal_3x5", timeScale);
  const timeHeight = getClockTextHeight("minimal_3x5", timeScale);
  const dateWidth = getTinyTextWidth(dateText, dateScale);
  const dateHeight = getClockTextHeight("minimal_3x5", dateScale);
  const contentWidth = Math.max(timeWidth, dateWidth);
  const panelWidth = contentWidth + MAZE_INFO_PANEL_METRICS.panelPaddingX * 2;
  const panelHeight =
    MAZE_INFO_PANEL_METRICS.panelPaddingTop +
    timeHeight +
    MAZE_INFO_PANEL_METRICS.sectionGapY +
    dateHeight +
    MAZE_INFO_PANEL_METRICS.panelPaddingBottom;
  const panelX = Math.floor((DISPLAY_SIZE - panelWidth) / 2);
  const panelY = Math.floor((DISPLAY_SIZE - panelHeight) / 2);
  const reservedX = Math.max(0, panelX - MAZE_INFO_PANEL_METRICS.reservedMarginX);
  const reservedY = Math.max(0, panelY - MAZE_INFO_PANEL_METRICS.reservedMarginY);
  const reservedRect = {
    x: reservedX,
    y: reservedY,
    width: Math.min(
      DISPLAY_SIZE - reservedX,
      panelWidth + MAZE_INFO_PANEL_METRICS.reservedMarginX * 2,
    ),
    height: Math.min(
      DISPLAY_SIZE - reservedY,
      panelHeight + MAZE_INFO_PANEL_METRICS.reservedMarginY * 2,
    ),
  };

  return {
    panelX,
    panelY,
    panelWidth,
    panelHeight,
    reservedRect,
    timeText,
    dateText,
    timeScale,
    dateScale,
    timeBaselineY: panelY + MAZE_INFO_PANEL_METRICS.panelPaddingTop,
    dateBaselineY:
      panelY +
      MAZE_INFO_PANEL_METRICS.panelPaddingTop +
      timeHeight +
      MAZE_INFO_PANEL_METRICS.sectionGapY,
  };
}

function drawMazeInfoPanel(map, panel, colors) {
  if (!panel) {
    return;
  }

  const panelBgColor = hexToRgb(colors.panelBgColor);
  const borderColor = hexToRgb(colors.borderColor);
  fillRect(
    map,
    panel.panelX,
    panel.panelY,
    panel.panelWidth,
    panel.panelHeight,
    panelBgColor.r,
    panelBgColor.g,
    panelBgColor.b,
  );
  drawRectOutline(
    map,
    panel.panelX,
    panel.panelY,
    panel.panelWidth,
    panel.panelHeight,
    borderColor.r,
    borderColor.g,
    borderColor.b,
  );
  drawClockTextToPixels(
    panel.timeText,
    Math.floor(DISPLAY_SIZE / 2),
    panel.timeBaselineY,
    colors.timeColor,
    map,
    "minimal_3x5",
    panel.timeScale,
    "center",
  );
  drawTinyTextToPixels(
    panel.dateText,
    Math.floor(DISPLAY_SIZE / 2),
    panel.dateBaselineY,
    colors.dateColor,
    map,
    panel.dateScale,
    "center",
  );
}

function cellIndex(layout, x, y) {
  return y * layout.cellCount + x;
}

function inBounds(layout, x, y) {
  return x >= 0 && y >= 0 && x < layout.cellCount && y < layout.cellCount;
}

function createMazeCells(layout) {
  return new Array(layout.cellCount * layout.cellCount).fill(null).map(() => ({
    top: true,
    right: true,
    bottom: true,
    left: true,
  }));
}

function getCellOrigin(layout, x, y) {
  return {
    x: layout.borderSize + x * layout.stride,
    y: layout.borderSize + y * layout.stride,
  };
}

function getCellReservedRect(layout, x, y) {
  const origin = getCellOrigin(layout, x, y);
  return {
    x: origin.x - layout.wallSize,
    y: origin.y - layout.wallSize,
    width: layout.cellSize + layout.wallSize * 2,
    height: layout.cellSize + layout.wallSize * 2,
  };
}

function buildReservedMask(layout, panel) {
  const mask = new Array(layout.cellCount * layout.cellCount).fill(false);
  if (!panel) {
    return mask;
  }

  for (let y = 0; y < layout.cellCount; y += 1) {
    for (let x = 0; x < layout.cellCount; x += 1) {
      mask[cellIndex(layout, x, y)] = rectanglesIntersect(
        getCellReservedRect(layout, x, y),
        panel.reservedRect,
      );
    }
  }

  return mask;
}

function findAvailableAnchor(layout, reservedMask, preferEnd = false) {
  const yStart = preferEnd ? layout.cellCount - 1 : 0;
  const yEnd = preferEnd ? -1 : layout.cellCount;
  const yStep = preferEnd ? -1 : 1;
  const xStart = preferEnd ? layout.cellCount - 1 : 0;
  const xEnd = preferEnd ? -1 : layout.cellCount;
  const xStep = preferEnd ? -1 : 1;

  for (let y = yStart; y !== yEnd; y += yStep) {
    for (let x = xStart; x !== xEnd; x += xStep) {
      if (!reservedMask[cellIndex(layout, x, y)]) {
        return { x, y };
      }
    }
  }

  return null;
}

function drawCell(layout, map, x, y, r, g, b) {
  const origin = getCellOrigin(layout, x, y);
  fillRect(map, origin.x, origin.y, layout.cellSize, layout.cellSize, r, g, b);
}

function drawPassage(layout, map, x, y, direction, r, g, b) {
  const origin = getCellOrigin(layout, x, y);
  if (direction === "right") {
    fillRect(
      map,
      origin.x + layout.cellSize,
      origin.y,
      layout.wallSize,
      layout.cellSize,
      r,
      g,
      b,
    );
    return;
  }
  if (direction === "bottom") {
    fillRect(
      map,
      origin.x,
      origin.y + layout.cellSize,
      layout.cellSize,
      layout.wallSize,
      r,
      g,
      b,
    );
  }
}

function drawConnection(layout, map, fromNode, toNode, r, g, b) {
  if (!fromNode || !toNode) {
    return;
  }
  drawCell(layout, map, fromNode.x, fromNode.y, r, g, b);
  drawCell(layout, map, toNode.x, toNode.y, r, g, b);

  if (toNode.x === fromNode.x + 1 && toNode.y === fromNode.y) {
    drawPassage(layout, map, fromNode.x, fromNode.y, "right", r, g, b);
    return;
  }
  if (toNode.x === fromNode.x - 1 && toNode.y === fromNode.y) {
    drawPassage(layout, map, toNode.x, toNode.y, "right", r, g, b);
    return;
  }
  if (toNode.y === fromNode.y + 1 && toNode.x === fromNode.x) {
    drawPassage(layout, map, fromNode.x, fromNode.y, "bottom", r, g, b);
    return;
  }
  if (toNode.y === fromNode.y - 1 && toNode.x === fromNode.x) {
    drawPassage(layout, map, toNode.x, toNode.y, "bottom", r, g, b);
  }
}

function pickRandom(items) {
  return items[Math.floor(Math.random() * items.length)];
}

function resolvePreviewSpeedValue(values, speed) {
  const index = Math.max(0, Math.min(values.length - 1, speed - 1));
  return values[index];
}

function resolveMazePreviewPacing(layout, speed) {
  const effectiveSpeed = Math.min(speed, 6);
  if (layout.cellCount >= 32) {
    return {
      generationStepsPerFrame: resolvePreviewSpeedValue([2, 2, 2, 2, 3, 3], effectiveSpeed),
      solveStepsPerFrame: resolvePreviewSpeedValue([1, 1, 1, 2, 2, 2], effectiveSpeed),
      revealStepsPerFrame: resolvePreviewSpeedValue([1, 1, 1, 1, 1, 2], effectiveSpeed),
      travelPixelsPerFrame: resolvePreviewSpeedValue([1, 1, 1, 1, 2, 2], effectiveSpeed),
    };
  }

  return {
    generationStepsPerFrame: resolvePreviewSpeedValue([1, 1, 1, 2, 2, 2], effectiveSpeed),
    solveStepsPerFrame: resolvePreviewSpeedValue([1, 1, 1, 1, 2, 2], effectiveSpeed),
    revealStepsPerFrame: resolvePreviewSpeedValue([1, 1, 1, 1, 1, 1], effectiveSpeed),
    travelPixelsPerFrame: resolvePreviewSpeedValue([1, 1, 1, 1, 1, 2], effectiveSpeed),
  };
}

function reconstructPath(layout, parentX, parentY, endX, endY) {
  const path = [];
  let currentX = endX;
  let currentY = endY;

  while (currentX !== -1 && currentY !== -1) {
    path.push({ x: currentX, y: currentY });
    const index = cellIndex(layout, currentX, currentY);
    const nextX = parentX[index];
    const nextY = parentY[index];
    currentX = nextX;
    currentY = nextY;
  }

  path.reverse();
  return path;
}

function buildPixelRoute(layout, path) {
  const route = [];
  if (!Array.isArray(path) || path.length === 0) {
    return route;
  }

  for (let index = 0; index < path.length; index += 1) {
    const node = path[index];
    const origin = getCellOrigin(layout, node.x, node.y);
    if (route.length === 0) {
      route.push({ x: origin.x, y: origin.y });
    } else {
      let current = route[route.length - 1];
      while (current.x !== origin.x || current.y !== origin.y) {
        const stepX = origin.x === current.x ? 0 : origin.x > current.x ? 1 : -1;
        const stepY = origin.y === current.y ? 0 : origin.y > current.y ? 1 : -1;
        current = {
          x: current.x + stepX,
          y: current.y + stepY,
        };
        route.push(current);
      }
    }
  }

  return route;
}

function drawWalker(layout, map, point, r, g, b) {
  if (!point) {
    return;
  }
  fillRect(
    map,
    point.x,
    point.y,
    layout.cellSize,
    layout.cellSize,
    r,
    g,
    b,
  );
}

function drawVisitedMaze(layout, map, cells, visited, showFullMaze, floorR, floorG, floorB) {
  for (let y = 0; y < layout.cellCount; y += 1) {
    for (let x = 0; x < layout.cellCount; x += 1) {
      const index = cellIndex(layout, x, y);
      if (showFullMaze || visited[index]) {
        drawCell(layout, map, x, y, floorR, floorG, floorB);
      }

      if (x < layout.cellCount - 1 && !cells[index].right) {
        const nextIndex = cellIndex(layout, x + 1, y);
        if (showFullMaze || (visited[index] && visited[nextIndex])) {
          drawPassage(layout, map, x, y, "right", floorR, floorG, floorB);
        }
      }

      if (y < layout.cellCount - 1 && !cells[index].bottom) {
        const nextIndex = cellIndex(layout, x, y + 1);
        if (showFullMaze || (visited[index] && visited[nextIndex])) {
          drawPassage(layout, map, x, y, "bottom", floorR, floorG, floorB);
        }
      }
    }
  }
}

function buildMazeSequence(speed, intensity, options) {
  const mazeConfig = resolveMazeRuntimeConfig(speed, options);
  if (!mazeConfig) {
    return {
      maps: [],
      phaseTags: [],
    };
  }
  const layout = resolveMazeLayout(mazeConfig);
  const isPreviewMode = options.previewMode === true;
  const generationPathColor = hexToRgb(mazeConfig.generationPathColor);
  const searchVisitedColor = hexToRgb(mazeConfig.searchVisitedColor);
  const searchFrontierColor = hexToRgb(mazeConfig.searchFrontierColor);
  const solvedPathStartColor = hexToRgb(mazeConfig.solvedPathStartColor);
  const solvedPathEndColor = hexToRgb(mazeConfig.solvedPathEndColor);
  const mazeInfoPanel = buildMazeInfoPanel(mazeConfig.showClock);
  const reservedMask = buildReservedMask(layout, mazeInfoPanel);
  const startNode = findAvailableAnchor(layout, reservedMask, false);
  const endNode = findAvailableAnchor(layout, reservedMask, true);
  if (!startNode || !endNode) {
    return {
      maps: [],
      phaseTags: [],
    };
  }
  const maps = [];
  const phaseTags = [];
  const safeSpeed = normalizeSpeed(mazeConfig.speed);
  const safeIntensity = 0.72;

  const cells = createMazeCells(layout);
  const visited = new Array(layout.cellCount * layout.cellCount).fill(false);
  const nodeG = new Array(layout.cellCount * layout.cellCount).fill(Infinity);
  const nodeH = new Array(layout.cellCount * layout.cellCount).fill(0);
  const nodeF = new Array(layout.cellCount * layout.cellCount).fill(Infinity);
  const parentX = new Array(layout.cellCount * layout.cellCount).fill(-1);
  const parentY = new Array(layout.cellCount * layout.cellCount).fill(-1);
  const openMask = new Array(layout.cellCount * layout.cellCount).fill(false);
  const closedMask = new Array(layout.cellCount * layout.cellCount).fill(false);

  const generationStack = [{ x: startNode.x, y: startNode.y }];
  const openSet = [];
  const closedSet = [];
  let path = [];
  let travelerRoute = [];
  const previewPacing = resolveMazePreviewPacing(layout, safeSpeed);

  visited[cellIndex(layout, startNode.x, startNode.y)] = true;

  const generationStepsPerFrame = isPreviewMode
    ? previewPacing.generationStepsPerFrame
    : layout.cellCount >= 32
      ? 1
      : 1;
  const solveStepsPerFrame = isPreviewMode
    ? previewPacing.solveStepsPerFrame
    : layout.cellCount >= 32
      ? 1
      : 1;
  const revealStepsPerFrame = isPreviewMode
    ? previewPacing.revealStepsPerFrame
    : 1;
  const travelPixelsPerFrame = isPreviewMode
    ? previewPacing.travelPixelsPerFrame
    : 1;
  const animationBaseDelay = Math.max(38, Math.min(120, 120 - safeSpeed * 8));
  const generationHoldFrames = isPreviewMode ? 2 : Math.max(1, Math.round(1000 / animationBaseDelay));
  const solveHoldFrames = isPreviewMode ? 2 : Math.max(1, Math.round(1000 / animationBaseDelay));
  const revealHoldFrames = isPreviewMode ? 2 : Math.max(1, Math.round(1000 / animationBaseDelay));
  const doneHoldFrames = isPreviewMode ? 24 : Math.max(1, Math.round(3000 / animationBaseDelay));
  const maxFrames = isPreviewMode
    ? layout.cellCount <= 21
      ? 2200
      : 3400
    : layout.cellCount <= 21
      ? 3600
      : 5200;

  let generationHead = { x: startNode.x, y: startNode.y };
  let phase = "generation";
  let generationHold = 0;
  let solveHold = 0;
  let revealCount = 0;
  let revealHold = 0;
  let travelerIndex = 0;
  let doneHold = 0;

  function heuristic(x1, y1, x2, y2) {
    return Math.abs(x2 - x1) + Math.abs(y2 - y1);
  }

  function carvePassage(fromX, fromY, dir) {
    const nextX = fromX + dir.dx;
    const nextY = fromY + dir.dy;
    const fromCell = cells[cellIndex(layout, fromX, fromY)];
    const nextCell = cells[cellIndex(layout, nextX, nextY)];
    fromCell[dir.key] = false;
    nextCell[dir.opposite] = false;
  }

  function generationStep() {
    if (generationStack.length === 0) {
      return true;
    }

    const current = generationStack[generationStack.length - 1];
    generationHead = current;

    const available = [];
    for (let index = 0; index < DIRS.length; index += 1) {
      const dir = DIRS[index];
      const nextX = current.x + dir.dx;
      const nextY = current.y + dir.dy;
      if (!inBounds(layout, nextX, nextY)) {
        continue;
      }
      if (reservedMask[cellIndex(layout, nextX, nextY)]) {
        continue;
      }
      if (visited[cellIndex(layout, nextX, nextY)]) {
        continue;
      }
      available.push(dir);
    }

    if (available.length === 0) {
      generationStack.pop();
      if (generationStack.length > 0) {
        generationHead = generationStack[generationStack.length - 1];
        return false;
      }
      return true;
    }

    const chosen = pickRandom(available);
    const nextX = current.x + chosen.dx;
    const nextY = current.y + chosen.dy;
    carvePassage(current.x, current.y, chosen);
    visited[cellIndex(layout, nextX, nextY)] = true;
    generationHead = { x: nextX, y: nextY };
    generationStack.push(generationHead);
    return false;
  }

  function getAccessibleNeighbors(x, y) {
    const result = [];
    const cell = cells[cellIndex(layout, x, y)];
    for (let index = 0; index < DIRS.length; index += 1) {
      const dir = DIRS[index];
      if (cell[dir.key]) {
        continue;
      }
      const nextX = x + dir.dx;
      const nextY = y + dir.dy;
      if (!inBounds(layout, nextX, nextY)) {
        continue;
      }
      if (reservedMask[cellIndex(layout, nextX, nextY)]) {
        continue;
      }
      result.push({ x: nextX, y: nextY });
    }
    return result;
  }

  function solveStep() {
    if (openSet.length === 0 && path.length === 0) {
      openSet.push({ x: startNode.x, y: startNode.y });
      openMask[cellIndex(layout, startNode.x, startNode.y)] = true;
      const startIndex = cellIndex(layout, startNode.x, startNode.y);
      nodeG[startIndex] = 0;
      nodeH[startIndex] = heuristic(startNode.x, startNode.y, endNode.x, endNode.y);
      nodeF[startIndex] = nodeH[startIndex];
      return false;
    }

    if (openSet.length === 0) {
      return false;
    }

    let bestPos = 0;
    let bestCost = Infinity;
    for (let index = 0; index < openSet.length; index += 1) {
      const node = openSet[index];
      const cost = nodeF[cellIndex(layout, node.x, node.y)];
      if (cost < bestCost) {
        bestCost = cost;
        bestPos = index;
      }
    }

    const current = openSet.splice(bestPos, 1)[0];
    openMask[cellIndex(layout, current.x, current.y)] = false;
    closedSet.push(current);
    closedMask[cellIndex(layout, current.x, current.y)] = true;

    if (current.x === endNode.x && current.y === endNode.y) {
      path = reconstructPath(
        layout,
        parentX,
        parentY,
        current.x,
        current.y,
      );
      travelerRoute = buildPixelRoute(layout, path);
      return true;
    }

    const neighbors = getAccessibleNeighbors(current.x, current.y);
    for (let index = 0; index < neighbors.length; index += 1) {
      const next = neighbors[index];
      const nextIndex = cellIndex(layout, next.x, next.y);
      if (closedMask[nextIndex]) {
        continue;
      }
      const tentativeG = nodeG[cellIndex(layout, current.x, current.y)] + 1;
      if (tentativeG < nodeG[nextIndex]) {
        parentX[nextIndex] = current.x;
        parentY[nextIndex] = current.y;
        nodeG[nextIndex] = tentativeG;
        nodeH[nextIndex] = heuristic(next.x, next.y, endNode.x, endNode.y);
        nodeF[nextIndex] = nodeG[nextIndex] + nodeH[nextIndex];
        if (!openMask[nextIndex]) {
          openSet.push(next);
          openMask[nextIndex] = true;
        }
      }
    }

    return false;
  }

  while (maps.length < maxFrames) {
    if (phase === "generation") {
      let completed = false;
      for (let step = 0; step < generationStepsPerFrame; step += 1) {
        completed = generationStep();
        if (completed) {
          break;
        }
      }
      if (completed) {
        phase = "generation_hold";
      }
    } else if (phase === "generation_hold") {
      generationHold += 1;
      if (generationHold >= generationHoldFrames) {
        phase = "solve";
      }
    } else if (phase === "solve") {
      let found = false;
      for (let step = 0; step < solveStepsPerFrame; step += 1) {
        found = solveStep();
        if (found) {
          break;
        }
      }
      if (found) {
        phase = "solve_hold";
      }
    } else if (phase === "solve_hold") {
      solveHold += 1;
      if (solveHold >= solveHoldFrames) {
        phase = "reveal";
      }
    } else if (phase === "reveal") {
      revealCount = Math.min(path.length, revealCount + revealStepsPerFrame);
      if (revealCount >= path.length) {
        phase = "reveal_hold";
      }
    } else if (phase === "reveal_hold") {
      revealHold += 1;
      if (revealHold >= revealHoldFrames) {
        phase = "travel";
      }
    } else if (phase === "travel") {
      travelerIndex = Math.min(
        travelerRoute.length - 1,
        travelerIndex + travelPixelsPerFrame,
      );
      if (travelerRoute.length > 0 && travelerIndex >= travelerRoute.length - 1) {
        phase = "done_hold";
      }
    } else if (phase === "done_hold") {
      doneHold += 1;
      if (doneHold >= doneHoldFrames) {
        break;
      }
    }

    const map = new Map();
    const showFullMaze = phase !== "generation" && phase !== "generation_hold";

    fillRect(
      map,
      0,
      0,
      DISPLAY_SIZE,
      DISPLAY_SIZE,
      MAZE_RUNTIME_COLORS.wall.r,
      MAZE_RUNTIME_COLORS.wall.g,
      MAZE_RUNTIME_COLORS.wall.b,
    );
    drawVisitedMaze(
      layout,
      map,
      cells,
      visited,
      showFullMaze,
      generationPathColor.r,
      generationPathColor.g,
      generationPathColor.b,
    );

    if (phase === "generation" || phase === "generation_hold") {
      drawCell(
        layout,
        map,
        generationHead.x,
        generationHead.y,
        MAZE_RUNTIME_COLORS.generationCursor.r,
        MAZE_RUNTIME_COLORS.generationCursor.g,
        MAZE_RUNTIME_COLORS.generationCursor.b,
      );
    }

    if (phase === "solve" || phase === "solve_hold") {
      for (let index = 0; index < closedSet.length; index += 1) {
        const node = closedSet[index];
        drawCell(
          layout,
          map,
          node.x,
          node.y,
          searchVisitedColor.r,
          searchVisitedColor.g,
          searchVisitedColor.b,
        );
        const parentIndex = cellIndex(layout, node.x, node.y);
        if (parentX[parentIndex] !== -1 && parentY[parentIndex] !== -1) {
          drawConnection(
            layout,
            map,
            { x: parentX[parentIndex], y: parentY[parentIndex] },
            node,
            searchVisitedColor.r,
            searchVisitedColor.g,
            searchVisitedColor.b,
          );
        }
      }
      for (let index = 0; index < openSet.length; index += 1) {
        const node = openSet[index];
        drawCell(
          layout,
          map,
          node.x,
          node.y,
          searchFrontierColor.r,
          searchFrontierColor.g,
          searchFrontierColor.b,
        );
      }

      if (closedSet.length > 0) {
        const solveCursor = closedSet[closedSet.length - 1];
        drawCell(
          layout,
          map,
          solveCursor.x,
          solveCursor.y,
          MAZE_RUNTIME_COLORS.solveCursor.r,
          MAZE_RUNTIME_COLORS.solveCursor.g,
          MAZE_RUNTIME_COLORS.solveCursor.b,
        );
      }
    }

    if (path.length > 0 && phase !== "solve" && phase !== "solve_hold") {
      const visibleCount = phase === "reveal" ? Math.max(1, revealCount) : path.length;
      for (let index = 0; index < visibleCount; index += 1) {
        const node = path[index];
        const ratio = index / Math.max(1, path.length - 1);
        const pathColor = interpolateColor(
          solvedPathStartColor,
          solvedPathEndColor,
          ratio,
        );
        drawCell(
          layout,
          map,
          node.x,
          node.y,
          pathColor.r,
          pathColor.g,
          pathColor.b,
        );
        if (index > 0) {
          drawConnection(
            layout,
            map,
            path[index - 1],
            node,
            pathColor.r,
            pathColor.g,
            pathColor.b,
          );
        }
      }
    }

    if (phase === "travel" || phase === "done_hold") {
      drawWalker(
        layout,
        map,
        travelerRoute[Math.max(0, travelerIndex)],
        MAZE_RUNTIME_COLORS.revealCursor.r,
        MAZE_RUNTIME_COLORS.revealCursor.g,
        MAZE_RUNTIME_COLORS.revealCursor.b,
      );
    } else if (phase === "reveal" && revealCount > 0) {
      const currentNode = path[Math.max(0, revealCount - 1)];
      if (currentNode) {
        drawCell(
          layout,
          map,
          currentNode.x,
          currentNode.y,
          MAZE_RUNTIME_COLORS.revealCursor.r,
          MAZE_RUNTIME_COLORS.revealCursor.g,
          MAZE_RUNTIME_COLORS.revealCursor.b,
        );
      }
    }

    drawCell(
      layout,
      map,
      startNode.x,
      startNode.y,
      MAZE_RUNTIME_COLORS.start.r,
      MAZE_RUNTIME_COLORS.start.g,
      MAZE_RUNTIME_COLORS.start.b,
    );
    drawCell(
      layout,
      map,
      endNode.x,
      endNode.y,
      MAZE_RUNTIME_COLORS.end.r,
      MAZE_RUNTIME_COLORS.end.g,
      MAZE_RUNTIME_COLORS.end.b,
    );
    drawMazeInfoPanel(map, mazeInfoPanel, mazeConfig);
    maps.push(map);
    phaseTags.push(phase);
  }

  return {
    maps,
    phaseTags,
  };
}

export function buildMazeDemoMaps(speed, intensity, options) {
  return buildMazeSequence(speed, intensity, options).maps;
}

export function buildMazePreviewSequence(speed, intensity, options) {
  return buildMazeSequence(speed, intensity, {
    ...options,
    previewMode: true,
  });
}
