import { describe, expect, it } from 'vitest'
import { buildStudioSummary, estimateStudioCompilePlan } from '../lib/studio-compiler'
import { buildCanvasTextLayout, createBlankStudioProject } from '../lib/studioProject'

describe('studio compiler helpers', () => {
  it('根据工作室规则给出编译计划和摘要', () => {
    const project = createBlankStudioProject()
    project.studio.materials = [
      { id: 'bg-1', category: 'background' },
      { id: 'bg-2', category: 'background' },
    ]
    project.studio.scene.background.assetIds = ['bg-1', 'bg-2']
    project.studio.scene.background.behavior = 'rotate'
    project.studio.scene.background.intervalMs = 1200
    project.studio.scene.objects = [
      {
        id: 'object-1',
        assetId: 'bg-1',
        visible: true,
        x: 10,
        y: 10,
        size: 12,
        motionType: 'path',
        pathMode: 'loop',
        durationMs: 2400,
        delayMs: 0,
        blinkIntervalMs: 1000,
        pathPoints: [{ x: 10, y: 10 }, { x: 30, y: 30 }],
      },
    ]
    project.studio.triggers.minute = [{ id: 'minute-1' }]
    project.studio.triggers.hour = [{ id: 'hour-1' }]

    const plan = estimateStudioCompilePlan(project)
    const summary = buildStudioSummary(project)

    expect(plan.loopDurationMs).toBeGreaterThan(0)
    expect(plan.estimatedFrameCount).toBeGreaterThan(0)
    expect(summary.materialCount).toBe(2)
    expect(summary.objectCount).toBe(1)
    expect(summary.minuteTriggerCount).toBe(1)
    expect(summary.hourTriggerCount).toBe(1)
  })

  it('验包预览读取工作室时间组件而不是旧 clock 图层位置', () => {
    const project = createBlankStudioProject()
    project.studio.timeWidgets.time.x = 3
    project.studio.timeWidgets.time.y = 4
    project.studio.timeWidgets.year.visible = true
    project.studio.timeWidgets.year.x = 9
    project.studio.timeWidgets.year.y = 42

    const layout = buildCanvasTextLayout(project)

    expect(layout.time.x).toBe(3)
    expect(layout.time.y).toBe(4)
    expect(layout.year.visible).toBe(true)
    expect(layout.year.x).toBe(9)
  })
})
