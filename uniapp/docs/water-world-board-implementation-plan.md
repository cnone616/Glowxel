# Plan: Water World Board Implementation

**Generated**: 2026-04-27
**Status**: Active

## Overview

`水世界` 当前正式产品只保留 `海面波浪 / 深海海流 / 海底焦散` 三条路线。
`uniapp` 页面、本地预览、正式发送和板载 preset 必须保持同一套路线集合，不允许再保留任何已下架路线的入口、分支或兼容路径。

## Current Route Set

| 路线 | `preset` | 当前状态 | 备注 |
| --- | --- | --- | --- |
| `海面波浪` | `surface` | 正式保留 | 继续作为海面主视觉路线 |
| `深海海流` | `current` | 正式保留 | 继续作为深海流动路线 |
| `海底焦散` | `caustics` | 正式保留 | 继续作为海底折射路线 |

## Contract Boundary

本轮继续复用现有 `ambient_effect` 发送链，不新增字段，不新增业务模式。
`水世界 / led_matrix_showcase` 的时间样式沿用既有 `ClockConfig` 负载字段，但必须作为该模式自己的独立配置保存与下发，不得复用 `animation` 或其他模式的时钟配置实例。

| field | source |
| --- | --- |
| `preset` | `水世界` 当前选中的正式路线 |
| `speed` | 该路线约定的默认速度值 |
| `intensity` | 该路线约定的默认强度值 |
| `loop` | 固定持续播放 |
| `config.font` | `水世界` 页字体选择 |
| `config.showSeconds` | `水世界` 页秒钟开关 |
| `config.hourFormat` | `水世界` 页小时制式 |
| `config.time.*` | `水世界` 页时间分组设置 |
| `config.date.*` | `水世界` 页独立时钟配置中的日期占位字段 |
| `config.week.*` | `水世界` 页独立时钟配置中的星期占位字段 |
| `config.image.*` | `水世界` 页独立时钟配置中的图片占位字段 |

## Non-Negotiable Rules

- `水世界` 正式页、预览页、发送判断和板载枚举必须同时只保留这三条路线。
- 不允许为 `水世界` 新开独立设备业务模式、兼容字段或第二套持久化结构。
- 若后续要恢复任何已下架路线，必须先重新确认，再补文档、页面、发送链和板载实现。
- 预览观感调整必须同步核对板载观感，不能形成“页面和设备两套状态”。

## Verification Checklist

- `water-world.vue` 里只显示 `海面波浪 / 深海海流 / 海底焦散`
- `waterWorldPreview.js` 只渲染这三条路线
- 固件 `AmbientEffectPreset`、preset 编解码、Web 页面列表只保留这三条路线
- 旧设备若仍存有已移除 preset，开机加载时必须迁回默认 preset
