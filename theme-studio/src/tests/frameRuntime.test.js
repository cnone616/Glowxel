import { describe, expect, it } from 'vitest'
import { diffTuplesFromMaps } from '../utils/frameRuntime'

describe('diffTuplesFromMaps', () => {
  it('像素删除会被编码成黑色差分点', () => {
    const previousMap = new Map([['4:5', [255, 0, 0]]])
    const currentMap = new Map()
    const diff = diffTuplesFromMaps(previousMap, currentMap)

    expect(diff).toEqual([[4, 5, 0, 0, 0]])
  })
})
