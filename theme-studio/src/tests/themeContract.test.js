import { describe, expect, it } from 'vitest'
import { validateThemePackageStrict } from '../utils/themeContract'

describe('validateThemePackageStrict', () => {
  it('在字段缺失或出现未约定字段时阻断', () => {
    const report = validateThemePackageStrict({
      name: '坏主题',
      f: 1,
      d: [
        {
          t: 1,
          d: 120,
          c: 1,
          p: [[0, 0, 255, 0, 0]],
          extra: true,
        },
      ],
      extraTop: 'x',
    })

    expect(report.ok).toBe(false)
    expect(report.errors.some((error) => error.code === 'UNAGREED_FIELD')).toBe(true)
  })

  it('首帧为差分帧时直接报错', () => {
    const report = validateThemePackageStrict({
      name: '差分首帧',
      f: 1,
      d: [
        {
          t: 0,
          d: 120,
          c: 1,
          p: [[0, 0, 255, 0, 0]],
        },
      ],
    })

    expect(report.ok).toBe(false)
    expect(report.errors.some((error) => error.code === 'FIRST_FRAME_DIFF_FORBIDDEN')).toBe(true)
  })
})
