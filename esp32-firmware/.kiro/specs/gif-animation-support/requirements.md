# Requirements Document

## Introduction

为 LED 模拟器添加 GIF 动画支持功能，允许用户上传 GIF 动图作为像素画背景，并优化数据格式以适配 ESP32 的内存和处理能力限制。该功能将扩展现有的图片显示功能，支持动画帧切换和差分数据压缩。

## Glossary

- **LED_Simulator**: 64x64 像素矩阵 LED 显示模拟器系统
- **GIF_Parser**: 使用外部库（如 gifuct-js）解析 GIF 文件的组件
- **Frame_Buffer**: 存储单帧像素数据的缓冲区
- **Differential_Data**: 相邻帧之间变化像素的差分数据
- **ESP32_Optimizer**: 针对 ESP32 内存和处理能力优化数据格式的组件
- **Animation_Controller**: 控制动画播放、暂停、循环的组件
- **Pixel_Matrix**: 64x64 像素显示矩阵

## Requirements

### Requirement 1: GIF 文件上传和解析

**User Story:** 作为用户，我希望能够上传 GIF 动图文件，以便在 LED 矩阵上显示动画效果。

#### Acceptance Criteria

1. WHEN 用户点击图片上传区域，THE LED_Simulator SHALL 允许选择 GIF 格式文件
2. WHEN 用户选择有效的 GIF 文件，THE GIF_Parser SHALL 解析 GIF 文件并提取所有帧数据
3. WHEN 用户选择无效的 GIF 文件，THE LED_Simulator SHALL 显示错误提示信息
4. THE GIF_Parser SHALL 支持标准 GIF89a 和 GIF87a 格式
5. WHEN GIF 文件解析完成，THE LED_Simulator SHALL 在预览区域显示第一帧图像

### Requirement 2: 动画帧数据优化

**User Story:** 作为开发者，我希望动画数据能够针对 ESP32 进行优化，以便在有限的内存和处理能力下正常播放。

#### Acceptance Criteria

1. THE ESP32_Optimizer SHALL 将每帧图像缩放到 64x64 像素
2. WHEN 处理连续帧时，THE ESP32_Optimizer SHALL 计算帧间差分数据
3. THE ESP32_Optimizer SHALL 仅存储变化的像素位置和颜色值
4. WHEN 差分数据超过完整帧数据大小的 80%，THE ESP32_Optimizer SHALL 存储完整帧数据
5. THE ESP32_Optimizer SHALL 将颜色深度优化为 16 位 RGB565 格式
6. THE ESP32_Optimizer SHALL 限制总动画数据大小不超过 512KB

### Requirement 3: 动画播放控制

**User Story:** 作为用户，我希望能够控制 GIF 动画的播放状态，以便根据需要播放或暂停动画。

#### Acceptance Criteria

1. THE Animation_Controller SHALL 提供播放、暂停、停止控制按钮
2. WHEN 用户点击播放按钮，THE Animation_Controller SHALL 按照原始 GIF 的帧率播放动画
3. WHEN 用户点击暂停按钮，THE Animation_Controller SHALL 暂停在当前帧
4. WHEN 用户点击停止按钮，THE Animation_Controller SHALL 停止播放并返回第一帧
5. THE Animation_Controller SHALL 支持循环播放模式
6. WHEN 动画播放到最后一帧，THE Animation_Controller SHALL 自动循环到第一帧

### Requirement 4: 动画参数调整

**User Story:** 作为用户，我希望能够调整动画的显示参数，以便获得最佳的显示效果。

#### Acceptance Criteria

1. THE LED_Simulator SHALL 提供动画播放速度调节功能（0.5x 到 3x）
2. THE LED_Simulator SHALL 提供动画位置调整功能（X、Y 坐标）
3. THE LED_Simulator SHALL 提供动画尺寸调整功能（保持宽高比）
4. WHEN 用户调整播放速度，THE Animation_Controller SHALL 实时更新帧率
5. WHEN 用户调整位置或尺寸，THE LED_Simulator SHALL 实时更新显示效果
6. THE LED_Simulator SHALL 保存用户的动画参数设置

### Requirement 5: ESP32 数据格式输出

**User Story:** 作为开发者，我希望获得优化后的动画数据，以便在 ESP32 设备上播放动画。

#### Acceptance Criteria

1. THE LED_Simulator SHALL 生成 ESP32 兼容的动画数据格式
2. THE LED_Simulator SHALL 包含帧数、帧率、尺寸等元数据
3. THE LED_Simulator SHALL 提供差分数据和完整帧数据的混合格式
4. WHEN 用户点击复制数据按钮，THE LED_Simulator SHALL 将优化后的数据复制到剪贴板
5. THE LED_Simulator SHALL 提供数据大小和内存使用量的统计信息
6. THE LED_Simulator SHALL 支持导出为 C 数组格式，便于嵌入 ESP32 代码

### Requirement 6: 性能和内存管理

**User Story:** 作为系统，我需要高效管理内存和性能，以便在浏览器中流畅处理 GIF 动画。

#### Acceptance Criteria

1. THE LED_Simulator SHALL 限制同时加载的 GIF 文件数量为 1 个
2. WHEN 加载新的 GIF 文件，THE LED_Simulator SHALL 释放之前的动画数据
3. THE LED_Simulator SHALL 使用 Web Workers 进行 GIF 解析，避免阻塞主线程
4. THE LED_Simulator SHALL 实现帧数据的懒加载机制
5. WHEN 内存使用超过 100MB，THE LED_Simulator SHALL 显示内存警告
6. THE LED_Simulator SHALL 提供动画预览的低分辨率模式以节省内存

### Requirement 7: 用户界面集成

**User Story:** 作为用户，我希望 GIF 动画功能能够无缝集成到现有的图片标签页中，以便统一管理所有图像内容。

#### Acceptance Criteria

1. THE LED_Simulator SHALL 在图片标签页中添加 GIF 动画选项
2. THE LED_Simulator SHALL 提供静态图片和 GIF 动画的切换功能
3. WHEN 选择 GIF 模式，THE LED_Simulator SHALL 显示动画专用控制面板
4. WHEN 选择静态图片模式，THE LED_Simulator SHALL 隐藏动画控制面板
5. THE LED_Simulator SHALL 保持与现有图片功能的界面一致性
6. THE LED_Simulator SHALL 在上传区域显示支持的文件格式提示