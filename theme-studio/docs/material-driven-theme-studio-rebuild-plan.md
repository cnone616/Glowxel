# Plan: 素材驱动的像素时钟主题工作室重建

**Generated**: 2026-04-20
**Estimated Complexity**: High

## Overview
当前 `theme-studio/` 的方向偏“逐帧像素编辑器”，这和目标用户真正想做的事情不一致。用户的核心工作流并不是手工逐帧画完一整套主题，而是：

1. 上传一些图片，把它们当成背景或移动对象素材
2. 给素材做分类和用途标记
3. 设定背景轮播、对象闪烁、对象沿固定路径移动
4. 配置时间、日期、星期、年份等展示位置、大小、字体
5. 配置分钟变化或整点时的特殊表现
6. 预览结果并导出可用主题包

因此本次不应继续把产品做成“专业像素逐帧软件”，而应重建为“素材驱动 + 规则驱动”的主题搭建器。

## Product Direction
### 目标定位
- 独立 Vue 项目，继续使用 `theme-studio/` 目录和现有工程基座
- 保留当前 `Vue 3 + Vite + Pinia + Vue Router + IndexedDB + 导出链` 基础设施
- 推倒当前以帧、层、像素工具为中心的主交互，改为以“素材、场景、规则、时间组件”为中心
- 最终导出仍然保持现有 `ThemePackage` 合同不变：`name / f / d[] / d[].t / d[].d / d[].c / d[].p`
- 首版只做本地闭环，不接服务端、不接设备、不接 `uniapp`

### V1 核心能力
- 本地上传图片素材，素材在上传时直接分类
- 背景素材支持：静态背景、轮播背景、闪烁背景
- 对象素材支持：静止、闪烁、沿路径移动、路径循环
- 时间组件支持：时间、日期、星期、年份的显隐、位置、字号档位、字体、颜色
- 事件规则支持：分钟切换时动作、整点时动作
- 一键预览、验包、导出 ZIP

### V1 非目标
- 不把逐帧绘制作为主工作流
- 不做自由图层系统
- 不做复杂选区、像素笔刷、补帧软件范式
- 不做自然语言 AI 解析器；描述式交互优先使用“规则句柄 / 规则模板 / 预设表达”实现
- 不改导出合同字段

## User Flow
### 主流程
1. 新建项目
2. 上传素材并分类
3. 搭建场景
4. 给对象设路径或闪烁规则
5. 配置时间显示样式
6. 配置分钟 / 整点触发
7. 预览一天中的几个关键时刻
8. 导出 `theme.json + cover.png + preview.gif + validation.json + zip`

### 关键交互原则
- 用户优先操作“素材卡片”和“场景对象”，而不是直接操作帧
- 用户看到的是“对象会怎么动”，不是“第几帧变成什么”
- 帧和像素细节只作为编译结果，不作为主工作台入口
- 所有规则都要能在界面上被一句话读懂
- 页面密度偏实用工具，不做虚胖工作台

## Information Architecture
### 页面结构
- `/` 项目库
- `/studio/:id` 场景工作室
- `/validate/:id` 验包
- `/publish/:id` 发布

### 场景工作室结构
采用“顶部步骤栏 + 左侧素材区 + 中央场景预览区 + 右侧规则区”的简洁工作台。

#### 顶部
- 项目名
- 当前步骤
- 保存
- 预览
- 验包
- 发布

#### 左侧：素材区
- 上传按钮
- 素材分类切换：背景 / 对象 / 装饰
- 素材缩略图列表
- 当前素材用途说明

#### 中央：场景区
- 64x64 主预览
- 路径叠加层
- 当前时间预览切换
- 播放 / 暂停
- 场景对象列表

#### 右侧：规则区
- 背景规则
- 对象规则
- 时间组件规则
- 事件规则
- 导出预算摘要

## Functional Model
### 1. 素材系统
上传时要求用户至少完成以下信息：
- 素材类型：背景 / 对象 / 装饰
- 素材名称
- 是否允许重复使用
- 初始缩放模式

V1 素材导入建议范围：
- 支持 `PNG / JPG / WebP`
- 对象素材优先支持带透明背景的图片
- GIF、视频、精灵表不纳入 V1 主范围

导入后系统要完成：
- 自动缩略图生成
- 自动适配到 64x64 工作画布
- 对象素材可调基准点
- 背景素材可选择铺满、居中、裁切

### 2. 场景系统
场景由三类内容组成：
- 背景
- 动态对象
- 时间组件

V1 限制：
- 同时活跃的对象数量需要有明确上限
- 每个对象只允许一种主运动方式
- 一个项目可包含多个场景状态，但首版建议先限制为一个主场景 + 若干触发变体

### 3. 规则系统
V1 只做四类可读规则：
- 背景静止
- 背景轮播
- 对象闪烁
- 对象沿路径移动

规则表达方式不做自由脚本，而是做以下组合：
- 行为类型选择器
- 少量参数输入
- 规则说明句自动生成

示例：
- 背景每 5 秒轮播一次
- 小车沿路径匀速循环
- 星星每 1 秒闪烁一次
- 整点时播放一次烟花对象动画

### 4. 路径系统
路径编辑要从“帧编辑”改成“路线编辑”。

V1 路径能力：
- 点击主预览创建路径点
- 拖动路径点修改线路
- 支持闭环和非闭环
- 支持匀速、往返、单次三种模式
- 支持起始延时与总时长
- 预览中可显示路径和对象当前位置

V1 不做：
- 贝塞尔曲线
- 复杂缓动曲线编辑器
- 多路径混合

### 5. 时间组件系统
时间组件不再作为普通图层，而作为单独配置模块。

V1 支持：
- 时间
- 日期
- 星期
- 年份

每个组件支持：
- 显示 / 隐藏
- 位置
- 大小档位
- 字体
- 颜色
- 12/24 小时制
- 秒显示开关

注意：
- 这里仅作为本地工程语义与预览能力
- 若未来需要接设备时钟配置，必须先单独输出“字段 -> 来源”映射表，再编码接入

### 6. 事件系统
事件是让主题在特定时机发生变化，而不是让用户手画额外逐帧。

V1 事件范围：
- 分钟切换时
- 整点时

V1 事件动作范围：
- 触发一次闪烁
- 切换一次背景素材
- 播放一次对象路径动画
- 暂时显示一个装饰素材

首版限制：
- 每类时机先只允许一个主事件
- 不做复杂条件组合

## Technical Approach
### 保留的现有基础
- `theme-studio/src/router/`
- `theme-studio/src/stores/`
- `theme-studio/src/lib/persistence.js`
- `theme-studio/src/lib/exportPipeline.js`
- `theme-studio/src/utils/themeContract.js`
- `theme-studio/src/utils/exporters.js`
- `theme-studio/src/utils/frameRuntime.js`

### 需要重建的核心模块
- `theme-studio/src/views/EditorView.vue`
  - 改名并重做为 `StudioView.vue` 更合理
- `theme-studio/src/components/editor/`
  - 现有帧工具组件大部分不再作为主入口
- 新增素材、场景、规则、路径相关模块

### 建议新增目录
- `theme-studio/src/components/studio/asset/`
- `theme-studio/src/components/studio/scene/`
- `theme-studio/src/components/studio/rules/`
- `theme-studio/src/components/studio/time/`
- `theme-studio/src/lib/studio-compiler/`
- `theme-studio/src/lib/materials/`
- `theme-studio/src/lib/pathing/`
- `theme-studio/src/lib/triggers/`
- `theme-studio/src/tests/compiler/`

### 编译思路
编辑器内部不再直接编辑帧，而是维护：
- 素材清单
- 场景对象清单
- 路径定义
- 背景规则
- 时间组件规则
- 事件规则

导出时由“场景编译器”统一生成帧序列，再映射为当前 `ThemePackage`。

这意味着：
- 用户操作的是高层语义
- 导出阶段再生成 `f / d / p`
- 现有严格校验合同仍可完全保留

## Sprint 1: 产品重定位与工程骨架重建
**Goal**: 把工程从“逐帧编辑器”重构成“素材驱动主题搭建器”的骨架。
**Demo/Validation**:
- 能创建项目并进入新的 `Studio` 页面
- 页面结构已切换为素材区 / 场景区 / 规则区
- 旧逐帧工具不再是主入口

### Task 1.1: 重新定义编辑器入口
- **Location**: `theme-studio/src/router/index.js`, `theme-studio/src/App.vue`
- **Description**: 将编辑器路由和导航语义改为“工作室 / 场景搭建器”而不是帧编辑器。
- **Dependencies**: None
- **Acceptance Criteria**:
  - 页面命名、入口文案、导航文案全部指向素材驱动工作流
  - 用户从项目库进入的是新的工作室页面
- **Validation**:
  - 本地运行后能通过 UI 进入新页面

### Task 1.2: 重写工作台布局
- **Location**: `theme-studio/src/views/EditorView.vue` 或新的 `theme-studio/src/views/StudioView.vue`
- **Description**: 用步骤化工具布局替代当前帧工具布局。
- **Dependencies**: Task 1.1
- **Acceptance Criteria**:
  - 左侧素材、中央场景、右侧规则布局成立
  - 没有以笔刷、橡皮、逐帧为主的主视觉区域
- **Validation**:
  - 页面可运行并适配桌面 / 平板 / 手机宽度

### Task 1.3: 定义新的本地工程语义
- **Location**: `theme-studio/src/lib/`, `theme-studio/src/stores/`
- **Description**: 重新定义本地工程结构，围绕素材、场景、规则、事件组织，而不是围绕图层和帧。
- **Dependencies**: Task 1.1
- **Acceptance Criteria**:
  - 本地持久化结构能表达素材、路径、时间规则、事件规则
  - 不影响最终导出合同
- **Validation**:
  - 新建项目后刷新页面仍能恢复工程内容

## Sprint 2: 素材上传与分类系统
**Goal**: 让用户真的可以上传图片并把它们当作主题素材使用。
**Demo/Validation**:
- 上传图片后生成缩略图
- 素材可被标记为背景或对象
- 素材能出现在场景中

### Task 2.1: 实现本地图片导入
- **Location**: `theme-studio/src/components/studio/asset/`, `theme-studio/src/lib/materials/`
- **Description**: 支持从本地导入图片，生成预览图和像素化中间结果。
- **Dependencies**: Sprint 1
- **Acceptance Criteria**:
  - 支持 `PNG / JPG / WebP`
  - 导入失败时有明确错误提示
  - 能生成素材缩略图
- **Validation**:
  - 上传不同尺寸的图片后都能进入素材列表

### Task 2.2: 实现素材分类和用途标记
- **Location**: `theme-studio/src/components/studio/asset/`, `theme-studio/src/stores/`
- **Description**: 上传时和上传后都可以设置素材用途。
- **Dependencies**: Task 2.1
- **Acceptance Criteria**:
  - 素材至少可标记为背景 / 对象 / 装饰
  - 分类后素材区能按类型筛选
- **Validation**:
  - 三类素材在界面中可独立查看

### Task 2.3: 实现素材适配策略
- **Location**: `theme-studio/src/lib/materials/`
- **Description**: 为背景和对象实现不同的适配规则。
- **Dependencies**: Task 2.1
- **Acceptance Criteria**:
  - 背景支持铺满、居中、裁切
  - 对象支持基准点和缩放档位
- **Validation**:
  - 同一素材切换适配模式时预览会变化

## Sprint 3: 场景搭建与路径运动
**Goal**: 从“画帧”改为“摆素材 + 设规则”。
**Demo/Validation**:
- 背景能轮播
- 一个对象能沿路径移动
- 一个对象能闪烁
- 中央预览能连续播放结果

### Task 3.1: 搭建场景对象系统
- **Location**: `theme-studio/src/components/studio/scene/`, `theme-studio/src/stores/`
- **Description**: 支持把素材放进场景，并管理对象列表。
- **Dependencies**: Sprint 2
- **Acceptance Criteria**:
  - 背景与对象都能进入当前场景
  - 对象支持显示顺序和显隐切换
- **Validation**:
  - 多个对象可同时出现在场景预览中

### Task 3.2: 实现背景规则
- **Location**: `theme-studio/src/components/studio/rules/`, `theme-studio/src/lib/studio-compiler/`
- **Description**: 支持静态背景、背景轮播、背景闪烁。
- **Dependencies**: Task 3.1
- **Acceptance Criteria**:
  - 用户能选择背景行为并设置基本参数
  - 预览中可看到轮播或闪烁效果
- **Validation**:
  - 两张背景图可按设定节奏切换

### Task 3.3: 实现对象闪烁规则
- **Location**: `theme-studio/src/components/studio/rules/`, `theme-studio/src/lib/studio-compiler/`
- **Description**: 支持对象按照频率闪烁或间歇显示。
- **Dependencies**: Task 3.1
- **Acceptance Criteria**:
  - 用户能设置显示频率和持续时间
  - 规则句能清楚显示当前行为
- **Validation**:
  - 对象在预览中按节奏亮灭

### Task 3.4: 实现路径编辑与路径运动规则
- **Location**: `theme-studio/src/components/studio/scene/`, `theme-studio/src/lib/pathing/`, `theme-studio/src/lib/studio-compiler/`
- **Description**: 允许用户在场景上画出路径，并让对象按路径移动。
- **Dependencies**: Task 3.1
- **Acceptance Criteria**:
  - 可以添加、拖动、删除路径点
  - 支持循环、往返、单次三种模式
  - 支持总时长和起始延时
- **Validation**:
  - 对象能沿指定路线完成运动预览

## Sprint 4: 时间组件与事件触发
**Goal**: 让主题真正具备时钟产品语义，而不是只有动态背景。
**Demo/Validation**:
- 时间、日期、星期、年份都能配置
- 分钟切换或整点时能触发一次变化

### Task 4.1: 时间组件面板重建
- **Location**: `theme-studio/src/components/studio/time/`, `theme-studio/src/stores/`
- **Description**: 把时间相关内容从普通图层中独立出来。
- **Dependencies**: Sprint 1
- **Acceptance Criteria**:
  - 时间 / 日期 / 星期 / 年份都可配置显隐、位置、大小、字体、颜色
  - 能切换 12/24 小时制与秒显示
- **Validation**:
  - 不同时间预设下展示结果正确变化

### Task 4.2: 预览时刻系统增强
- **Location**: `theme-studio/src/components/studio/time/`, `theme-studio/src/views/StudioView.vue`
- **Description**: 增加关键时刻预览，不只是一组普通时间。
- **Dependencies**: Task 4.1
- **Acceptance Criteria**:
  - 支持普通时刻预览
  - 支持分钟切换前后、整点前后预览
- **Validation**:
  - 用户能快速看到触发前后状态差异

### Task 4.3: 分钟 / 整点触发规则
- **Location**: `theme-studio/src/components/studio/rules/`, `theme-studio/src/lib/triggers/`, `theme-studio/src/lib/studio-compiler/`
- **Description**: 为分钟变化和整点建立事件触发器。
- **Dependencies**: Task 4.1, Task 3.2, Task 3.3, Task 3.4
- **Acceptance Criteria**:
  - 可为分钟切换配置一个动作
  - 可为整点配置一个动作
  - 动作范围限定在背景切换 / 闪烁 / 路径播放 / 装饰显示
- **Validation**:
  - 在关键时刻预览中能看到触发效果

## Sprint 5: 场景编译器、验包与发布收口
**Goal**: 把高层语义稳定编译为当前导出合同，并形成完整本地闭环。
**Demo/Validation**:
- 上传素材到导出 ZIP 全链路可跑通
- 导出结果通过严格合同校验
- 项目刷新后内容仍完整恢复

### Task 5.1: 实现场景到帧的编译器
- **Location**: `theme-studio/src/lib/studio-compiler/`
- **Description**: 将背景规则、对象规则、时间组件和事件规则编译成帧序列。
- **Dependencies**: Sprint 3, Sprint 4
- **Acceptance Criteria**:
  - 编译输出可稳定映射到现有 `ThemePackage`
  - 同一输入多次导出结果一致
- **Validation**:
  - 编译结果通过 `themeContract` 严格校验

### Task 5.2: 重做验包页
- **Location**: `theme-studio/src/views/ValidateView.vue`
- **Description**: 让验包页更贴近“这个主题能不能用”，而不是只展示低层技术字段。
- **Dependencies**: Task 5.1
- **Acceptance Criteria**:
  - 验包页包含素材摘要、规则摘要、预算摘要、合同摘要
  - 能指出超预算对象、过长路径、过多素材等问题
- **Validation**:
  - 构造异常工程时能得到可读错误信息

### Task 5.3: 重做发布页与导出链路
- **Location**: `theme-studio/src/views/PublishView.vue`, `theme-studio/src/lib/exportPipeline.js`
- **Description**: 延续本地发布，但发布对象变成“主题作品”而不是“帧项目”。
- **Dependencies**: Task 5.1
- **Acceptance Criteria**:
  - ZIP 仍只包含既定四类文件
  - 发布记录可以回看封面和简介
- **Validation**:
  - 本地导出 ZIP 可解压并包含完整产物

## Testing Strategy
### 单元测试
- 素材导入后像素化结果稳定
- 路径采样结果正确
- 背景轮播和闪烁编译正确
- 分钟 / 整点触发编译正确
- 导出 `ThemePackage` 符合严格合同

### 集成测试
- 从上传图片到导出 ZIP 的闭环流程可跑通
- 刷新页面后项目内容完整恢复
- 删除素材、替换素材、修改规则后预览立即更新

### 构建验证
- `npm run build`
- `npm run test:unit`

## Potential Risks & Gotchas
- **风险 1：图片导入后像素化效果不稳定**
  - 需要尽早确定缩放、裁切、透明处理策略
- **风险 2：路径规则过多会重新滑向复杂软件**
  - V1 必须把行为类型限制在少数几个模板内
- **风险 3：分钟 / 整点触发容易变成复杂状态机**
  - V1 只允许单事件入口，不做条件组合
- **风险 4：用户想用“描述一句话”控制行为，但自由文本难以稳定解析**
  - V1 用规则模板和自动生成句子代替 AI 文本理解
- **风险 5：现有导出链是帧合同，而新编辑器是场景合同**
  - 必须优先实现并测试编译器，不能等到最后再补
- **风险 6：如果继续沿用旧 `EditorView` 代码修修补补，产品会持续混乱**
  - 建议直接新建 `StudioView` 和新组件目录，不要在旧逐帧工作台上继续迭代

## Recommended Default Decisions
- V1 图片导入只支持 `PNG / JPG / WebP`
- V1 不支持 GIF、视频、精灵表
- V1 路径只支持点线式路径，不支持曲线
- V1 行为只支持静止、轮播、闪烁、路径移动
- V1 整点 / 分钟事件只支持一条主动作
- V1 保留导出合同不变，不新增字段

## Rollback Plan
- 在重建过程中保留当前 `theme-studio/` 的导出与校验底层能力
- 新工作室优先以新增视图和新增模块落地，避免先破坏导出链
- 若新工作室在 Sprint 2 以前效果不达标，可回退到当前稳定工程基座，仅保留路由和存储层
