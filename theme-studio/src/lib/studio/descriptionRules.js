function extractDurationMs(text, currentValue) {
  const matched = text.match(/(\d+)\s*(毫秒|ms|秒|s)/i)
  if (!matched) {
    return currentValue
  }

  const rawValue = Number(matched[1])
  if (matched[2] === '秒' || matched[2].toLowerCase() === 's') {
    return rawValue * 1000
  }
  return rawValue
}

export function parseBackgroundDescription(text, currentRule) {
  const patch = {
    description: text,
  }

  if (text.includes('轮播')) {
    patch.behavior = 'rotate'
  }

  if (text.includes('闪烁')) {
    patch.behavior = 'blink'
  }

  if (text.includes('静止') || text.includes('固定')) {
    patch.behavior = 'static'
  }

  patch.intervalMs = extractDurationMs(text, currentRule.intervalMs)
  return patch
}

export function parseObjectDescription(text, currentObject) {
  const patch = {
    pathDescription: text,
  }

  if (text.includes('闪烁')) {
    patch.motionType = 'blink'
    patch.blinkIntervalMs = extractDurationMs(text, currentObject.blinkIntervalMs)
  }

  if (text.includes('路径') || text.includes('走线') || text.includes('线路')) {
    patch.motionType = 'path'
  }

  if (text.includes('静止') || text.includes('固定')) {
    patch.motionType = 'static'
  }

  if (text.includes('往返')) {
    patch.pathMode = 'pingpong'
  }

  if (text.includes('单次') || text.includes('一次')) {
    patch.pathMode = 'once'
  }

  if (text.includes('循环')) {
    patch.pathMode = 'loop'
  }

  patch.durationMs = extractDurationMs(text, currentObject.durationMs)
  return patch
}
