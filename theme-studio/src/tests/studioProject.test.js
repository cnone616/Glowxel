import { describe, expect, it } from 'vitest'
import {
  buildThemePackageFromProject,
  buildTweenFramesFromTuples,
  createBlankStudioProject,
} from '../lib/studioProject'
import { FRAME_TYPE_DIFF, FRAME_TYPE_FULL } from '../utils/themeContract'

describe('buildThemePackageFromProject', () => {
  it('同一工程重复导出结果保持一致', () => {
    const project = createBlankStudioProject()
    project.timeline.frames = [
      { id: 'f1', delay: 100, type: FRAME_TYPE_FULL },
      { id: 'f2', delay: 120, type: FRAME_TYPE_DIFF },
    ]
    project.layers.background.frames = [
      [[1, 1, 255, 0, 0]],
      [],
    ]
    project.layers.motion.frames = [[], []]
    project.layers.accent.frames = [[], []]

    const first = buildThemePackageFromProject(project)
    const second = buildThemePackageFromProject(project)

    expect(first).toEqual(second)
    expect(first.d[1].p).toEqual([[1, 1, 0, 0, 0]])
  })

  it('根据前后帧生成过渡帧', () => {
    const tweenFrames = buildTweenFramesFromTuples(
      [[1, 1, 255, 0, 0]],
      [
        [1, 1, 0, 0, 0],
        [2, 2, 0, 255, 0],
      ],
      2,
    )

    expect(tweenFrames).toHaveLength(2)
    expect(tweenFrames[0]).toEqual([])
    expect(tweenFrames[1]).toEqual([[2, 2, 0, 255, 0]])
  })

  it('空白工程包含素材工作室所需的基础语义', () => {
    const project = createBlankStudioProject()

    expect(project.preview.currentDate).toBe('2026-01-01')
    expect(project.studio.materials).toEqual([])
    expect(project.studio.scene.background.assetIds).toEqual([])
    expect(project.studio.scene.objects).toEqual([])
    expect(project.studio.timeWidgets.year.visible).toBe(false)
    expect(project.studio.triggers.minute).toEqual([])
    expect(project.studio.triggers.hour).toEqual([])
  })
})
