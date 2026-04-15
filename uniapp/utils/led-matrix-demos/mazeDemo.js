import { fillRect, normalizeSpeed } from "./common.js";

const DISPLAY_SIZE = 64;

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
  const layout = resolveMazeLayout(options);
  const isPreviewMode = !!(options && options.previewMode);
  const maps = [];
  const phaseTags = [];
  const safeSpeed = normalizeSpeed(speed);
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

  const generationStack = [{ x: 0, y: 0 }];
  const openSet = [];
  const closedSet = [];
  let path = [];
  let travelerRoute = [];
  const previewPacing = resolveMazePreviewPacing(layout, safeSpeed);

  visited[cellIndex(layout, 0, 0)] = true;

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

  let generationHead = { x: 0, y: 0 };
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
      result.push({ x: nextX, y: nextY });
    }
    return result;
  }

  function solveStep() {
    if (openSet.length === 0 && path.length === 0) {
      openSet.push({ x: 0, y: 0 });
      openMask[cellIndex(layout, 0, 0)] = true;
      const startIndex = cellIndex(layout, 0, 0);
      nodeG[startIndex] = 0;
      nodeH[startIndex] = heuristic(0, 0, layout.cellCount - 1, layout.cellCount - 1);
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

    if (current.x === layout.cellCount - 1 && current.y === layout.cellCount - 1) {
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
        nodeH[nextIndex] = heuristic(next.x, next.y, layout.cellCount - 1, layout.cellCount - 1);
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
    const wallShade = 12;
    const floorShade = 60 + Math.round(26 * safeIntensity);
    const floorBlue = floorShade + 6;
    const showFullMaze = phase !== "generation" && phase !== "generation_hold";

    fillRect(map, 0, 0, DISPLAY_SIZE, DISPLAY_SIZE, wallShade, wallShade, wallShade + 2);
    drawVisitedMaze(
      layout,
      map,
      cells,
      visited,
      showFullMaze,
      floorShade,
      floorShade,
      floorBlue,
    );

    if (phase === "generation" || phase === "generation_hold") {
      drawCell(layout, map, generationHead.x, generationHead.y, 40, 255, 120);
    }

    if (phase === "solve" || phase === "solve_hold") {
      for (let index = 0; index < closedSet.length; index += 1) {
        const node = closedSet[index];
        const colorR = 255;
        const colorG = 68;
        const colorB = 68;
        drawCell(layout, map, node.x, node.y, colorR, colorG, colorB);
        const parentIndex = cellIndex(layout, node.x, node.y);
        if (parentX[parentIndex] !== -1 && parentY[parentIndex] !== -1) {
          drawConnection(
            layout,
            map,
            { x: parentX[parentIndex], y: parentY[parentIndex] },
            node,
            colorR,
            colorG,
            colorB,
          );
        }
      }
      for (let index = 0; index < openSet.length; index += 1) {
        const node = openSet[index];
        drawCell(layout, map, node.x, node.y, 112, 255, 156);
      }
    }

    if (path.length > 0 && phase !== "solve" && phase !== "solve_hold") {
      const visibleCount = phase === "reveal" ? Math.max(1, revealCount) : path.length;
      for (let index = 0; index < visibleCount; index += 1) {
        const node = path[index];
        const ratio = index / Math.max(1, path.length - 1);
        const blueR = Math.round(26 + ratio * 40);
        const blueG = Math.round(96 + ratio * 92);
        const blueB = 255;
        drawCell(layout, map, node.x, node.y, blueR, blueG, blueB);
        if (index > 0) {
          drawConnection(
            layout,
            map,
            path[index - 1],
            node,
            blueR,
            blueG,
            blueB,
          );
        }
      }
    }

    if (phase === "travel" || phase === "done_hold") {
      drawWalker(
        layout,
        map,
        travelerRoute[Math.max(0, travelerIndex)],
        255,
        244,
        172,
      );
    } else if (phase === "reveal" && revealCount > 0) {
      const currentNode = path[Math.max(0, revealCount - 1)];
      if (currentNode) {
        drawCell(layout, map, currentNode.x, currentNode.y, 255, 244, 172);
      }
    }

    drawCell(layout, map, 0, 0, 40, 255, 120);
    drawCell(
      layout,
      map,
      layout.cellCount - 1,
      layout.cellCount - 1,
      90,
      188,
      255,
    );
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
    ...(options || {}),
    previewMode: true,
  });
}
