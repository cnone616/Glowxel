import { FRAME_TYPE_FULL, sortPixelTuples } from './themeContract'

export function pixelKey(x, y) {
  return `${x}:${y}`
}

export function clonePixelTuple(tuple) {
  return [tuple[0], tuple[1], tuple[2], tuple[3], tuple[4]]
}

function isBlack(r, g, b) {
  return r === 0 && g === 0 && b === 0
}

function parsePixelKey(key) {
  const split = key.split(':')
  return {
    x: Number(split[0]),
    y: Number(split[1])
  }
}

export function tuplesToPixelMap(pixelTuples) {
  const map = new Map()

  for (let i = 0; i < pixelTuples.length; i += 1) {
    const tuple = pixelTuples[i]
    const key = pixelKey(tuple[0], tuple[1])
    if (isBlack(tuple[2], tuple[3], tuple[4])) {
      map.delete(key)
    } else {
      map.set(key, [tuple[2], tuple[3], tuple[4]])
    }
  }

  return map
}

export function pixelMapToTuples(map) {
  const tuples = []
  map.forEach((color, key) => {
    const parsed = parsePixelKey(key)
    tuples.push([parsed.x, parsed.y, color[0], color[1], color[2]])
  })

  sortPixelTuples(tuples)
  return tuples
}

export function clonePixelMap(map) {
  const nextMap = new Map()
  map.forEach((color, key) => {
    nextMap.set(key, [color[0], color[1], color[2]])
  })
  return nextMap
}

export function resolveFrameMap(themePackage, frameIndex) {
  const resolved = new Map()

  for (let i = 0; i <= frameIndex; i += 1) {
    const frame = themePackage.d[i]

    if (frame.t === FRAME_TYPE_FULL) {
      resolved.clear()
    }

    for (let j = 0; j < frame.p.length; j += 1) {
      const tuple = frame.p[j]
      const key = pixelKey(tuple[0], tuple[1])
      if (isBlack(tuple[2], tuple[3], tuple[4])) {
        resolved.delete(key)
      } else {
        resolved.set(key, [tuple[2], tuple[3], tuple[4]])
      }
    }
  }

  return resolved
}

export function resolveAllFrameMaps(themePackage) {
  const maps = []
  let workingMap = new Map()

  for (let i = 0; i < themePackage.d.length; i += 1) {
    const frame = themePackage.d[i]

    if (frame.t === FRAME_TYPE_FULL) {
      workingMap = new Map()
    }

    for (let j = 0; j < frame.p.length; j += 1) {
      const tuple = frame.p[j]
      const key = pixelKey(tuple[0], tuple[1])
      if (isBlack(tuple[2], tuple[3], tuple[4])) {
        workingMap.delete(key)
      } else {
        workingMap.set(key, [tuple[2], tuple[3], tuple[4]])
      }
    }

    maps.push(clonePixelMap(workingMap))
  }

  return maps
}

export function findPixelIndex(pixelTuples, x, y) {
  for (let i = 0; i < pixelTuples.length; i += 1) {
    if (pixelTuples[i][0] === x && pixelTuples[i][1] === y) {
      return i
    }
  }

  return -1
}

export function upsertPixel(pixelTuples, x, y, r, g, b) {
  const index = findPixelIndex(pixelTuples, x, y)
  if (index === -1) {
    pixelTuples.push([x, y, r, g, b])
  } else {
    pixelTuples[index] = [x, y, r, g, b]
  }
  sortPixelTuples(pixelTuples)
}

export function removePixel(pixelTuples, x, y) {
  const index = findPixelIndex(pixelTuples, x, y)
  if (index !== -1) {
    pixelTuples.splice(index, 1)
  }
}

export function diffTuplesFromMaps(previousMap, currentMap) {
  const allKeys = new Set()

  previousMap.forEach((_, key) => {
    allKeys.add(key)
  })

  currentMap.forEach((_, key) => {
    allKeys.add(key)
  })

  const tuples = []

  allKeys.forEach((key) => {
    const parsed = parsePixelKey(key)

    let prevColor = [0, 0, 0]
    if (previousMap.has(key)) {
      const value = previousMap.get(key)
      prevColor = [value[0], value[1], value[2]]
    }

    let nextColor = [0, 0, 0]
    if (currentMap.has(key)) {
      const value = currentMap.get(key)
      nextColor = [value[0], value[1], value[2]]
    }

    const changed = prevColor[0] !== nextColor[0] || prevColor[1] !== nextColor[1] || prevColor[2] !== nextColor[2]

    if (changed) {
      tuples.push([parsed.x, parsed.y, nextColor[0], nextColor[1], nextColor[2]])
    }
  })

  sortPixelTuples(tuples)
  return tuples
}
