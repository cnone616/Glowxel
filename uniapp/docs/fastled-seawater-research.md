# Water World Seawater Research

## Current Product Scope

`水世界` 当前正式产品只保留三条路线：

| 路线 | `preset` | 上游参考 | 当前定位 |
| --- | --- | --- | --- |
| `海面波浪` | `surface` | `Wave2d`, `Pacifica` | 海面主视觉 |
| `深海海流` | `current` | `NoisePlusPalette`, 海流噪声组织 | 深海流动氛围 |
| `海底焦散` | `caustics` | `water-caustics` | 海底折射亮纹 |

## What We Borrow

| 路线 | 核心机制 | 明确借用点 |
| --- | --- | --- |
| `海面波浪` | 二维波场 + 白浪层次 | 波峰传播、蓝绿海色、亮脊组织 |
| `深海海流` | 噪声流场 + 冷色深浅过渡 | 深海流动方向感、低频层次 |
| `海底焦散` | 焦散网纹 + 低频扫动 | 海底亮纹、折射感、深蓝底色 |

## What We Do Not Borrow

- 不新开 `water_world` 独立设备业务模式。
- 不新增正式字段、兼容字段或第二套持久化合同。
- 不把研究文档里的候选路线直接视为正式产品路线。

## Current Agreed Contract

| field | source |
| --- | --- |
| `preset` | `水世界` 当前选中的正式路线 |
| `speed` | 该路线约定的默认速度值 |
| `intensity` | 该路线约定的默认强度值 |
| `loop` | 固定持续播放 |

## Implementation Notes

- `uniapp` 本地预览与板载 preset 必须保持同一套三路线集合。
- `海面波浪 / 深海海流 / 海底焦散` 需要优先保持更深、更偏海水蓝的主色盘。
- `海面波浪 / 海底焦散` 需要优先压掉高频闪烁。
- 若未来要增加新路线，必须先重新确认产品范围，再更新本文件与正式合同。

## Reference Links

- [FastLED Pacifica](https://github.com/FastLED/FastLED/blob/master/examples/Pacifica/Pacifica.ino)
- [FastLED Wave2d](https://github.com/FastLED/FastLED/blob/master/examples/Wave2d/Wave2d.ino)
- [FastLED NoisePlusPalette](https://github.com/FastLED/FastLED/blob/master/examples/NoisePlusPalette/NoisePlusPalette.h)
