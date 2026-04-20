import { describe, expect, it } from 'vitest'
import { buildThemeExportBundle } from '../utils/exporters'

const textLayout = {
  hourFormat: 24,
  showSeconds: true,
  time: { visible: true, x: 8, y: 8, font: 2, color: { r: 255, g: 255, b: 255 } },
  date: { visible: true, x: 8, y: 20, font: 1, color: { r: 255, g: 191, b: 105 } },
  week: { visible: true, x: 8, y: 28, font: 1, color: { r: 102, g: 234, b: 214 } },
}

describe('buildThemeExportBundle', () => {
  it('生成 JSON、PNG、GIF 与校验报告', async () => {
    const bundle = await buildThemeExportBundle(
      {
        name: '导出测试',
        f: 2,
        d: [
          { t: 1, d: 120, c: 1, p: [[1, 1, 255, 0, 0]] },
          { t: 0, d: 120, c: 1, p: [[1, 1, 0, 0, 0]] },
        ],
      },
      textLayout,
    )

    expect(bundle.report.ok).toBe(true)
    expect(bundle.files.themeJsonBlob.type).toBe('application/json')
    expect(bundle.files.validationBlob.type).toBe('application/json')
    expect(bundle.files.coverPngBlob.type).toBe('image/png')
    expect(bundle.files.previewGifBlob.type).toBe('image/gif')
  })
})
