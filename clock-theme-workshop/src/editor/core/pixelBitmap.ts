import type { Point, Rect } from '../types'

const MATRIX_SIZE = 64

type PixelColor = [number, number, number, number]

function clampChannel(value: number): number {
  return Math.max(0, Math.min(255, Math.round(value)))
}

function isInside(x: number, y: number): boolean {
  return x >= 0 && y >= 0 && x < MATRIX_SIZE && y < MATRIX_SIZE
}

function getPixelOffset(x: number, y: number): number {
  return (y * MATRIX_SIZE + x) * 4
}

export function cloneBitmap(bitmap: number[]): number[] {
  return bitmap.slice()
}

export function hexToRgba(color: string): PixelColor {
  const normalized = color.startsWith('#') ? color.slice(1) : color
  if (normalized.length !== 6) {
    return [255, 255, 255, 255]
  }

  return [
    Number.parseInt(normalized.slice(0, 2), 16),
    Number.parseInt(normalized.slice(2, 4), 16),
    Number.parseInt(normalized.slice(4, 6), 16),
    255,
  ]
}

export function rgbaToHex(color: PixelColor): string {
  return `#${color
    .slice(0, 3)
    .map((channel) => clampChannel(channel).toString(16).padStart(2, '0'))
    .join('')}`
}

export function readPixel(bitmap: number[], x: number, y: number): PixelColor {
  if (!isInside(x, y)) {
    return [0, 0, 0, 0]
  }

  const offset = getPixelOffset(x, y)
  return [
    bitmap[offset] ?? 0,
    bitmap[offset + 1] ?? 0,
    bitmap[offset + 2] ?? 0,
    bitmap[offset + 3] ?? 0,
  ]
}

export function writePixel(bitmap: number[], x: number, y: number, color: PixelColor): void {
  if (!isInside(x, y)) {
    return
  }

  const offset = getPixelOffset(x, y)
  bitmap[offset] = clampChannel(color[0])
  bitmap[offset + 1] = clampChannel(color[1])
  bitmap[offset + 2] = clampChannel(color[2])
  bitmap[offset + 3] = clampChannel(color[3])
}

export function erasePixel(bitmap: number[], x: number, y: number): void {
  writePixel(bitmap, x, y, [0, 0, 0, 0])
}

export function paintPoints(bitmap: number[], points: Point[], color: string): void {
  const rgba = hexToRgba(color)
  for (const point of points) {
    writePixel(bitmap, point.x, point.y, rgba)
  }
}

export function erasePoints(bitmap: number[], points: Point[]): void {
  for (const point of points) {
    erasePixel(bitmap, point.x, point.y)
  }
}

export function buildLinePoints(start: Point, end: Point): Point[] {
  const points: Point[] = []
  const deltaX = Math.abs(end.x - start.x)
  const deltaY = Math.abs(end.y - start.y)
  const stepX = start.x < end.x ? 1 : -1
  const stepY = start.y < end.y ? 1 : -1
  let currentX = start.x
  let currentY = start.y
  let error = deltaX - deltaY

  while (true) {
    points.push({ x: currentX, y: currentY })
    if (currentX === end.x && currentY === end.y) {
      break
    }
    const doubledError = error * 2
    if (doubledError > -deltaY) {
      error -= deltaY
      currentX += stepX
    }
    if (doubledError < deltaX) {
      error += deltaX
      currentY += stepY
    }
  }

  return points
}

export function buildRectOutlinePoints(start: Point, end: Point): Point[] {
  const minX = Math.min(start.x, end.x)
  const maxX = Math.max(start.x, end.x)
  const minY = Math.min(start.y, end.y)
  const maxY = Math.max(start.y, end.y)
  const points: Point[] = []

  for (let x = minX; x <= maxX; x += 1) {
    points.push({ x, y: minY })
    points.push({ x, y: maxY })
  }

  for (let y = minY + 1; y < maxY; y += 1) {
    points.push({ x: minX, y })
    points.push({ x: maxX, y })
  }

  return points
}

function colorsEqual(first: PixelColor, second: PixelColor): boolean {
  return (
    first[0] === second[0] &&
    first[1] === second[1] &&
    first[2] === second[2] &&
    first[3] === second[3]
  )
}

export function floodFill(bitmap: number[], point: Point, color: string): void {
  if (!isInside(point.x, point.y)) {
    return
  }

  const fillColor = hexToRgba(color)
  const targetColor = readPixel(bitmap, point.x, point.y)
  if (colorsEqual(fillColor, targetColor)) {
    return
  }

  const queue: Point[] = [point]
  const visited = new Set<string>()

  while (queue.length > 0) {
    const current = queue.pop()
    if (!current) {
      continue
    }
    if (!isInside(current.x, current.y)) {
      continue
    }

    const key = `${current.x}:${current.y}`
    if (visited.has(key)) {
      continue
    }
    visited.add(key)

    if (!colorsEqual(readPixel(bitmap, current.x, current.y), targetColor)) {
      continue
    }

    writePixel(bitmap, current.x, current.y, fillColor)
    queue.push({ x: current.x + 1, y: current.y })
    queue.push({ x: current.x - 1, y: current.y })
    queue.push({ x: current.x, y: current.y + 1 })
    queue.push({ x: current.x, y: current.y - 1 })
  }
}

export function moveSelection(bitmap: number[], selection: Rect, delta: Point): number[] {
  const nextBitmap = cloneBitmap(bitmap)
  const selectedPixels: Array<{ x: number; y: number; color: PixelColor }> = []

  for (let y = selection.y; y < selection.y + selection.height; y += 1) {
    for (let x = selection.x; x < selection.x + selection.width; x += 1) {
      if (!isInside(x, y)) {
        continue
      }
      selectedPixels.push({
        x,
        y,
        color: readPixel(nextBitmap, x, y),
      })
      erasePixel(nextBitmap, x, y)
    }
  }

  for (const pixel of selectedPixels) {
    writePixel(nextBitmap, pixel.x + delta.x, pixel.y + delta.y, pixel.color)
  }

  return nextBitmap
}

export function flipBitmap(bitmap: number[], axis: 'horizontal' | 'vertical'): number[] {
  const nextBitmap = cloneBitmap(bitmap)

  for (let y = 0; y < MATRIX_SIZE; y += 1) {
    for (let x = 0; x < MATRIX_SIZE; x += 1) {
      const sourceX = axis === 'horizontal' ? MATRIX_SIZE - 1 - x : x
      const sourceY = axis === 'vertical' ? MATRIX_SIZE - 1 - y : y
      writePixel(nextBitmap, x, y, readPixel(bitmap, sourceX, sourceY))
    }
  }

  return nextBitmap
}
