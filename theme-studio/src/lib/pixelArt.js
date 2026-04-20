import { pixelMapToTuples } from '../utils/frameRuntime'

function pixelKey(x, y) {
  return `${x}:${y}`
}

function cloneColor(color) {
  return [color[0], color[1], color[2]]
}

export function createPixelMap() {
  return new Map()
}

export function clonePixelMap(map) {
  const next = new Map()
  map.forEach((color, key) => {
    next.set(key, cloneColor(color))
  })
  return next
}

export function setPixel(map, x, y, color) {
  map.set(pixelKey(x, y), cloneColor(color))
}

export function removePixel(map, x, y) {
  map.delete(pixelKey(x, y))
}

export function drawHorizontal(map, x, y, length, color) {
  for (let i = 0; i < length; i += 1) {
    setPixel(map, x + i, y, color)
  }
}

export function drawVertical(map, x, y, length, color) {
  for (let i = 0; i < length; i += 1) {
    setPixel(map, x, y + i, color)
  }
}

export function drawRect(map, x, y, width, height, color) {
  for (let yy = 0; yy < height; yy += 1) {
    for (let xx = 0; xx < width; xx += 1) {
      setPixel(map, x + xx, y + yy, color)
    }
  }
}

export function drawFrame(map, x, y, width, height, color) {
  drawHorizontal(map, x, y, width, color)
  drawHorizontal(map, x, y + height - 1, width, color)
  drawVertical(map, x, y, height, color)
  drawVertical(map, x + width - 1, y, height, color)
}

export function drawPoints(map, points, color) {
  for (let i = 0; i < points.length; i += 1) {
    const point = points[i]
    setPixel(map, point[0], point[1], color)
  }
}

export function mapToTuples(map) {
  return pixelMapToTuples(map)
}

export function buildStaticFrames(frameCount, painter) {
  const map = createPixelMap()
  painter(map)
  const tuples = mapToTuples(map)
  return Array.from({ length: frameCount }, () => structuredClone(tuples))
}

export function buildAnimatedFrames(frameCount, painter) {
  const frames = []
  for (let i = 0; i < frameCount; i += 1) {
    const map = createPixelMap()
    painter(map, i, frameCount)
    frames.push(mapToTuples(map))
  }
  return frames
}
