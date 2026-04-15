import { clamp, fillRect, normalizeSpeed, setPixel } from "./common.js";

function createSeededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state = (state * 1664525 + 1013904223) >>> 0;
    return state / 0x100000000;
  };
}

function resolveBarWidth(options) {
  const parsed = Number(options && options.sortBarWidth);
  if (!Number.isFinite(parsed)) {
    return 2;
  }
  return clamp(Math.round(parsed), 1, 4);
}

function resolveAlgorithm(options) {
  const value = options && options.sortAlgorithm;
  if (
    value === "bubble" ||
    value === "selection" ||
    value === "insertion" ||
    value === "binary_insertion"
  ) {
    return value;
  }
  return "bubble";
}

function shuffleValues(count, rand) {
  const values = [];
  for (let index = 1; index <= count; index += 1) {
    values.push(index);
  }
  for (let index = values.length - 1; index > 0; index -= 1) {
    const swapIndex = Math.floor(rand() * (index + 1));
    const temp = values[index];
    values[index] = values[swapIndex];
    values[swapIndex] = temp;
  }
  return values;
}

function estimateOperationCount(kind, count) {
  if (kind === "selection") {
    return Math.floor((count * (count - 1)) / 2) + count;
  }
  if (kind === "insertion" || kind === "binary_insertion") {
    return Math.floor((count * count) / 4) + count * 3;
  }
  return Math.floor((count * (count - 1)) / 2) + count * 2;
}

function mixColor(a, b, ratio) {
  return {
    r: Math.round(a.r + (b.r - a.r) * ratio),
    g: Math.round(a.g + (b.g - a.g) * ratio),
    b: Math.round(a.b + (b.b - a.b) * ratio),
  };
}

function drawSortingFrame(maps, values, config) {
  const map = new Map();
  const activeSet = new Set(config.activeIndices);
  const sortedSet = new Set(config.sortedIndices);
  const barCount = values.length;
  const usableHeight = 56;
  const boardHeight = 58;
  const boardWidth = barCount * config.barWidth;
  const offsetX = Math.floor((64 - boardWidth) / 2);
  const top = 3;
  const bottom = top + boardHeight - 1;
  const left = Math.max(0, offsetX - 1);
  const right = Math.min(63, offsetX + boardWidth);

  for (let x = left; x <= right; x += 1) {
    setPixel(map, x, top - 1, 22, 28, 38);
    setPixel(map, x, bottom, 22, 28, 38);
  }
  for (let y = top - 1; y <= bottom; y += 1) {
    setPixel(map, left, y, 22, 28, 38);
    setPixel(map, right, y, 22, 28, 38);
  }

  for (let index = 0; index < values.length; index += 1) {
    const ratio = (values[index] - 1) / Math.max(1, barCount - 1);
    const barHeight = Math.max(1, Math.round(((values[index]) / barCount) * usableHeight));
    let color = mixColor(
      { r: 82, g: 156, b: 255 },
      { r: 104, g: 248, b: 198 },
      ratio,
    );

    if (sortedSet.has(index)) {
      color = { r: 118, g: 236, b: 142 };
    }
    if (activeSet.has(index)) {
      color = { r: 255, g: 188, b: 82 };
    }

    const x = offsetX + index * config.barWidth;
    const y = bottom - barHeight;
    fillRect(map, x, y, config.barWidth, barHeight, color.r, color.g, color.b);
  }

  maps.push(map);
}

function buildEmitter(maps, values, config) {
  let opCount = 0;
  return {
    maybeFrame(activeIndices, sortedIndices, force) {
      opCount += 1;
      if (!force && opCount % config.sampleEvery !== 0) {
        return;
      }
      drawSortingFrame(maps, values, {
        barWidth: config.barWidth,
        activeIndices,
        sortedIndices,
      });
    },
  };
}

function buildBubbleSortMaps(values, config, maps) {
  const emitter = buildEmitter(maps, values, config);
  const sorted = new Set();
  for (let end = values.length - 1; end > 0; end -= 1) {
    let swapped = false;
    for (let index = 0; index < end; index += 1) {
      emitter.maybeFrame([index, index + 1], Array.from(sorted), false);
      if (values[index] > values[index + 1]) {
        const temp = values[index];
        values[index] = values[index + 1];
        values[index + 1] = temp;
        swapped = true;
        emitter.maybeFrame([index, index + 1], Array.from(sorted), true);
      }
    }
    sorted.add(end);
    emitter.maybeFrame([], Array.from(sorted), true);
    if (!swapped) {
      break;
    }
  }
  sorted.add(0);
  drawSortingFrame(maps, values, {
    barWidth: config.barWidth,
    activeIndices: [],
    sortedIndices: Array.from(sorted),
  });
}

function buildSelectionSortMaps(values, config, maps) {
  const emitter = buildEmitter(maps, values, config);
  const sorted = new Set();
  for (let start = 0; start < values.length - 1; start += 1) {
    let minIndex = start;
    for (let index = start + 1; index < values.length; index += 1) {
      emitter.maybeFrame([minIndex, index], Array.from(sorted), false);
      if (values[index] < values[minIndex]) {
        minIndex = index;
        emitter.maybeFrame([start, minIndex], Array.from(sorted), true);
      }
    }
    if (minIndex !== start) {
      const temp = values[start];
      values[start] = values[minIndex];
      values[minIndex] = temp;
      emitter.maybeFrame([start, minIndex], Array.from(sorted), true);
    }
    sorted.add(start);
    emitter.maybeFrame([], Array.from(sorted), true);
  }
  sorted.add(values.length - 1);
  drawSortingFrame(maps, values, {
    barWidth: config.barWidth,
    activeIndices: [],
    sortedIndices: Array.from(sorted),
  });
}

function buildInsertionSortMaps(values, config, maps) {
  const emitter = buildEmitter(maps, values, config);
  const sorted = new Set([0]);
  for (let index = 1; index < values.length; index += 1) {
    const current = values[index];
    let insertIndex = index;
    emitter.maybeFrame([index], Array.from(sorted), true);
    while (insertIndex > 0 && values[insertIndex - 1] > current) {
      emitter.maybeFrame([insertIndex - 1, insertIndex], Array.from(sorted), false);
      values[insertIndex] = values[insertIndex - 1];
      insertIndex -= 1;
      emitter.maybeFrame([insertIndex, insertIndex + 1], Array.from(sorted), true);
    }
    values[insertIndex] = current;
    for (let sortedIndex = 0; sortedIndex <= index; sortedIndex += 1) {
      sorted.add(sortedIndex);
    }
    emitter.maybeFrame([insertIndex], Array.from(sorted), true);
  }
  drawSortingFrame(maps, values, {
    barWidth: config.barWidth,
    activeIndices: [],
    sortedIndices: Array.from(sorted),
  });
}

function buildBinaryInsertionSortMaps(values, config, maps) {
  const emitter = buildEmitter(maps, values, config);
  const sorted = new Set([0]);
  for (let index = 1; index < values.length; index += 1) {
    const current = values[index];
    let left = 0;
    let right = index;

    while (left < right) {
      const middle = Math.floor((left + right) / 2);
      emitter.maybeFrame([middle, index], Array.from(sorted), true);
      if (values[middle] <= current) {
        left = middle + 1;
      } else {
        right = middle;
      }
    }

    for (let move = index; move > left; move -= 1) {
      values[move] = values[move - 1];
      emitter.maybeFrame([move - 1, move], Array.from(sorted), move === index);
    }
    values[left] = current;

    for (let sortedIndex = 0; sortedIndex <= index; sortedIndex += 1) {
      sorted.add(sortedIndex);
    }
    emitter.maybeFrame([left], Array.from(sorted), true);
  }
  drawSortingFrame(maps, values, {
    barWidth: config.barWidth,
    activeIndices: [],
    sortedIndices: Array.from(sorted),
  });
}

export function buildSortingVisualizerDemoMaps(speed, intensity, options) {
  const safeSpeed = normalizeSpeed(speed);
  const barWidth = resolveBarWidth(options);
  const algorithm = resolveAlgorithm(options);
  const barCount = Math.max(8, Math.floor(64 / barWidth));
  const rand = createSeededRandom(20260420 + barWidth * 31 + safeSpeed * 19);
  const values = shuffleValues(barCount, rand);
  const maps = [];
  const estimate = estimateOperationCount(algorithm, barCount);
  const targetFrames = 120 + safeSpeed * 10;
  const sampleEvery = Math.max(1, Math.ceil(estimate / targetFrames));
  const config = {
    barWidth,
    sampleEvery,
  };

  drawSortingFrame(maps, values, {
    barWidth,
    activeIndices: [],
    sortedIndices: [],
  });

  if (algorithm === "selection") {
    buildSelectionSortMaps(values, config, maps);
  } else if (algorithm === "insertion") {
    buildInsertionSortMaps(values, config, maps);
  } else if (algorithm === "binary_insertion") {
    buildBinaryInsertionSortMaps(values, config, maps);
  } else {
    buildBubbleSortMaps(values, config, maps);
  }

  const holdFrames = 10 + Math.floor((10 - safeSpeed) / 2);
  const lastFrame = maps[maps.length - 1];
  for (let index = 0; index < holdFrames; index += 1) {
    maps.push(new Map(lastFrame));
  }

  return maps;
}
