# AI Agents 工作流程与分组

本项目包含多个专业的AI Agent，每个Agent负责特定的开发环节。所有Agent按功能分组，使用不同颜色标识。

## 🔄 标准工作流程

### 1. 项目初始化阶段
```
@steering-architect 分析现有代码库并创建项目指导文件
↓ 生成 .ai-rules/product.md, tech.md, structure.md
```

### 2. 编码规范生成阶段
```
@code-standards-architect 分析现有代码库的技术栈并生成编码规范规则文件
↓ 识别技术栈（语言、框架、构建工具）
↓ 自动创建 .cursor/rules/ 文件夹（如果不存在）
↓ 生成对应的编码规范文件（git.mdc, language-rules.mdc, framework-rules.mdc 等）
```
**说明**：这一步应在项目初始化后进行，为后续开发提供统一的编码规范。生成的规则文件会根据实际技术栈自动匹配。

### 3. 设计系统建立阶段
```
@ui-design-architect 分析现有代码库的UI设计实现并创建设计系统文档
↓ 从代码中提取设计规范（颜色、字体、间距、组件样式等）
↓ 生成 .ai-rules/ui-design.md（通用设计系统文档）
```
**说明**：这一步应在项目初始化后或重构时进行，为后续开发提供统一的设计规范参考。

### 4. 功能规划阶段
```
@strategic-planner 规划新功能
↓ 生成 specs/feature-name/requirements.md, design.md, tasks.md
↓ 询问验证和提交策略（每个任务验证/提交 或 最后统一验证/提交）
↓ 根据用户选择在 tasks.md 中添加验证和提交任务
↓ 参考 .ai-rules/ui-design.md 保持设计一致性
```

### 5. 功能实现阶段
```
@task-executor 执行 specs/feature-name/tasks.md 中的任务
↓ 逐个完成任务，一次只执行一个任务
↓ 如有设计参考，自动使用 @ui-fidelity-implementer 工作流程
↓ 执行验证任务（Test: 子任务）进行编译/测试验证
↓ 执行提交任务（Commit: 子任务）进行代码提交
↓ 参考 .ai-rules/ui-design.md 保持设计一致性
```

### 6. 代码审查阶段
```
@code-reviewer 审查功能代码
↓ 审查代码质量、安全性、可维护性
↓ 检查是否符合 .ai-rules/ui-design.md 设计规范
↓ 发现问题，提出改进建议（按优先级组织）
```

### 7. 迭代优化阶段（循环步骤 4-6）
```
@strategic-planner 规划下一个功能
@task-executor 执行任务（自动参考设计系统文档和编码规范）
@code-reviewer 审查代码（检查是否符合编码规范）
```

---

## ⚠️ 使用注意事项

1. **遵循Agent职责边界**：每个Agent只负责自己的专业领域，不要越界
2. **按流程顺序使用**：遵循标准工作流程，确保项目有序推进
3. **及时代码审查**：每次代码修改后都应使用 `@code-reviewer` 进行审查
4. **文档优先**：架构类和文档类Agent应优先使用，确保项目规范
5. **协作配合**：多个Agent可以配合使用，形成完整的开发闭环
6. **规则文件自动生效**：`.cursor/rules/` 中的规则文件会自动加载，无需手动引用（仅限 Cursor IDE）
7. **Agent需要显式调用**：`.cursor/agents/` 中的Agent需要通过 `@agent-name` 显式调用

---

## 📋 Agent 功能分组

### 🏗️ 架构/规划类 (Architecture & Planning)

**颜色标识：红色/蓝色系**

#### @steering-architect 🔴 (red)
- **职责**：项目分析师和文档架构师
- **功能**：分析现有代码库并创建项目核心指导文件
- **产出**：`.ai-rules/product.md`, `.ai-rules/tech.md`, `.ai-rules/structure.md`
- **使用场景**：项目初始化、架构分析、创建项目规范、分析技术栈

#### @strategic-planner 🔵 (blue)
- **职责**：专家级软件架构师和协作规划师
- **功能**：功能需求分析、技术设计和任务规划
- **产出**：`specs/feature-name/requirements.md`, `design.md`, `tasks.md`
- **使用场景**：制定新功能规划、需求分析、技术设计、创建开发任务
- **约束**：绝对不编写代码，只做规划设计
- **工作流程**：三阶段交互式规划（需求定义 → 技术设计 → 任务生成）
- **关键功能**：
  - 在任务生成阶段询问验证和提交策略（每个任务验证/提交 或 最后统一验证/提交）
  - 根据用户选择在 tasks.md 中添加验证和提交任务，或添加建议提醒

---

### ⚙️ 执行类 (Execution)

**颜色标识：绿色/青色系**

#### @task-executor 🟢 (green)
- **职责**：AI软件工程师，专注于执行单个具体任务
- **功能**：严格按照任务清单逐项实现，具有外科手术般的精确度
- **使用场景**：执行具体编码任务、实现特定功能、修复bug、运行测试
- **工作模式**：一次只执行一个任务，完成后等待确认或自动推进
- **关键功能**：
  - 识别并执行 tasks.md 中的验证任务（Test: 子任务）
  - 识别并执行 tasks.md 中的提交任务（Commit: 子任务）
  - 如果任务包含设计参考，自动使用 @ui-fidelity-implementer 工作流程
  - 遵循项目规范（.ai-rules/ 文档）保持代码一致性

#### @code-reviewer 🔷 (teal)
- **职责**：专业代码审查专家
- **功能**：主动审查代码质量、安全性和可维护性
- **审查内容**：代码简洁性、命名清晰度、重复代码、错误处理、安全漏洞、性能优化
- **使用场景**：编写或修改代码后必须立即使用
- **反馈格式**：按优先级组织（严重问题/警告问题/建议改进）

---

### 🎨 设计类 (Design)

**颜色标识：紫色系**

#### @ui-design-architect 🟣 (purple)
- **职责**：UI设计规范架构师
- **功能**：分析现有代码库的UI设计实现并创建专业UI设计系统文档
- **产出**：`.ai-rules/ui-design.md`（通用设计系统文档）
- **工作方式**：从代码库中**逆向提取**设计规范 → 生成通用设计系统文档
- **使用场景**：
  - 项目初始化时，需要建立统一的设计系统文档
  - 项目重构时，需要梳理现有设计规范
  - 团队需要统一设计标准文档时
- **特点**：❌ 不写代码，只生成文档
- **规范内容**：色彩系统、字体系统、间距系统、组件规范、布局系统、动画交互、可访问性

#### @ui-fidelity-implementer 🟣 (violet)
- **职责**：UI保真度实现专家
- **功能**：分析用户提供的设计参考（网站截图、设计图、URL等）并生成高度保真的代码实现
- **产出**：代码实现（高度匹配设计参考）
- **工作方式**：从设计参考中**正向提取**设计规范 → 生成保真代码
- **使用场景**：
  - 用户提供设计参考（截图/设计图/URL/Figma链接）并要求实现类似设计
  - `@task-executor` 执行任务时，任务描述包含"参考设计图/设计参考/设计稿"或提供了设计材料
  - 需要参考某个网站或设计图的样式实现具体功能
- **特点**：✅ 直接生成代码，专注于保真度
- **核心能力**：
  - 设计图分析（Figma、Sketch、Adobe XD、截图、网站URL）
  - 像素级精确实现（提取确切颜色、字体、间距值）
  - 视觉规范提取（颜色、字体、间距、组件样式、布局结构）
  - 保真度验证（确保生成代码与设计参考高度一致）
- **关键原则**：保真度第一，精确度优先。代码必须与设计参考高度一致。

**两个Agent的区别**：
- **@ui-design-architect**：从代码→文档（逆向工程，建立通用设计系统）
- **@ui-fidelity-implementer**：从设计图→代码（正向实现，生成保真代码）
- **协作关系**：`@task-executor` 在执行包含设计参考的UI任务时，会自动使用 `@ui-fidelity-implementer` 的工作流程

---

### 📐 技术规范类 (Technical Standards)

**颜色标识：青色/橙色/石板灰色系**

#### @code-standards-architect 🔘 (slate)
- **职责**：编码规范架构师
- **功能**：分析现有代码库的技术栈并自动生成对应的编码规范规则文件
- **产出**：`.cursor/rules/git.mdc`, `{language}-rules.mdc`, `{framework}-rules.mdc`, `declarative-ui-rules.mdc` 等
- **工作方式**：识别技术栈（语言、框架、构建工具）→ 自动创建 `.cursor/rules/` 文件夹 → 生成对应的编码规范文件
- **使用场景**：
  - 新项目初始化时自动生成编码规范
  - 跨项目使用 `agents` 文件夹时，为新项目生成 `rules` 文件夹
  - 需要创建或更新编码规范规则文件时
- **技术栈识别**：
  - 语言：TypeScript、JavaScript、Python、Java、Go、Rust、ArkTS 等
  - 框架：Vue3、React、Angular、Django、Spring Boot 等
  - UI框架：Vue、React、ArkUI 等声明式UI
  - 构建工具：Vite、Webpack、HarmonyOS Build 等
- **规范生成**：
  - 必须生成：`git.mdc`（Git提交规范）
  - 根据语言生成：`typescript-rules.mdc`, `python-rules.mdc` 等
  - 根据框架生成：`vue3-typescript-rules.mdc`, `react-typescript-rules.mdc` 等
  - 如果检测到声明式UI：生成对应的UI开发规范
- **项目特定规则**：
  - 根据项目构建方式生成：`auto-build.mdc`（自动构建规则）
  - 生成：`command-execution.mdc`（命令执行标准）

#### @api-architect 🔵 (cyan)
- **职责**：API设计规范架构师
- **功能**：分析现有代码库的API接口设计并创建专业API设计规范文档
- **产出**：`.ai-rules/api-design.md`
- **使用场景**：API设计规范、接口标准、RESTful设计、API文档
- **规范内容**：RESTful设计、请求响应格式、认证授权、错误处理、API版本ing、速率限制

#### @security-architect 🟠 (orange)
- **职责**：安全规范架构师
- **功能**：分析现有代码库的安全实现并创建专业安全规范文档
- **产出**：`.ai-rules/security-design.md`
- **使用场景**：安全规范、安全标准、漏洞防护、安全最佳实践
- **规范内容**：认证授权、数据保护、输入验证、安全头部、漏洞管理、合规要求、事件响应

---

### 📝 文档类 (Documentation)

**颜色标识：靛蓝色系**

#### @prd-writer 🔵 (indigo)
- **职责**：专业的产品需求文档(PRD)生成专家和产品经理助手
- **功能**：生成PRD文档、产品需求文档、产品规格书、功能需求分析
- **使用场景**：PRD文档生成、产品规划、需求整合、用户故事编写
- **文档内容**：产品概述、功能需求、非功能需求、技术方案、用户体验设计、实施计划

---

### 📊 数据分析类 (Data Analysis)

**颜色标识：翠绿色系**

#### @data-scientist 🟢 (emerald)
- **职责**：数据分析和数据科学专家
- **功能**：处理SQL查询、BigQuery操作和数据洞察分析
- **使用场景**：数据分析、数据库查询、数据挖掘、统计分析、数据可视化、数据驱动决策
- **专业技能**：SQL优化、数据建模、统计分析、商业智能

---

### 🔧 调试类 (Debugging)

**颜色标识：黄色系**

#### @debugger 🟡 (yellow)
- **职责**：错误调试和问题排查专家
- **功能**：处理程序错误、测试失败和异常行为
- **使用场景**：技术问题、代码报错、功能异常、问题排查
- **专业技能**：根因分析、错误定位、Bug修复、系统诊断
- **调试流程**：捕获错误信息 → 确定重现步骤 → 定位故障位置 → 实施最小化修复 → 验证解决方案

---

## 📋 特殊场景使用

### 设计系统开发（建立通用设计规范）
```
@ui-design-architect 分析现有代码库的UI组件并创建设计系统文档
↓ 从代码中逆向提取设计规范
↓ 生成 .ai-rules/ui-design.md（通用设计系统文档）
```
**使用时机**：项目初始化、重构、需要建立统一设计标准文档时

### 设计参考保真实现（参考具体设计图实现）
```
@ui-fidelity-implementer 分析用户提供的设计参考并生成保真代码
↓ 分析设计图（截图/URL/Figma等）
↓ 提取设计规范（颜色、字体、间距等）
↓ 生成匹配代码（像素级精确实现）
↓ 验证保真度
```
**使用时机**：
- 用户直接提供设计参考并要求实现
- `@task-executor` 执行包含设计参考的UI任务时自动使用
- 任务描述中包含设计参考关键词（"reference design", "design file", "screenshot", "Figma link"等）或提供了设计材料时

### 编码规范生成（自动生成规则文件）
```
@code-standards-architect 分析代码库技术栈并生成编码规范
↓ 识别语言、框架、构建工具
↓ 创建 .cursor/rules/ 文件夹
↓ 生成 git.mdc（必须）
↓ 生成对应技术栈的编码规范文件（vue3-typescript-rules.mdc, react-typescript-rules.mdc 等）
↓ 根据项目构建方式生成 auto-build.mdc 和 command-execution.mdc
```
**使用时机**：新项目初始化、跨项目使用 agents 时、需要更新编码规范时

### API设计开发
```
@api-architect 分析API接口并创建API设计规范
↓ 生成 .ai-rules/api-design.md
```

### 安全规范制定
```
@security-architect 分析安全实现并创建安全规范
↓ 生成 .ai-rules/security-design.md
```

### 产品文档编写
```
@prd-writer 生成产品需求文档
↓ 生成完整的PRD文档
```

### 数据分析任务
```
@data-scientist 执行数据分析
↓ 编写SQL查询、分析结果、提供数据洞察
```

### 问题排查与调试
```
@debugger 排查技术问题
↓ 定位错误、提供修复方案
```

---

## 🎨 颜色分组说明

### 架构/规划类 (Architecture & Planning)

- **🔴 `steering-architect` (red)**：项目核心指导
  - 分析现有代码库并创建项目指导文件
  - 生成 `.ai-rules/product.md`, `tech.md`, `structure.md`

- **🔵 `strategic-planner` (blue)**：功能规划
  - 规划新功能、需求分析、技术设计
  - 生成 `specs/feature-name/requirements.md`, `design.md`, `tasks.md`

### 执行类 (Execution)

- **🟢 `task-executor` (green)**：代码实现
  - 执行具体编码任务、实现特定功能、修复bug
  - 严格按照任务清单逐项实现

- **🔷 `code-reviewer` (teal)**：代码审查
  - 审查代码质量、安全性、可维护性
  - 检查是否符合编码规范和设计规范

### 设计类 (Design)

- **🟣 `ui-design-architect` (purple)**：UI设计规范
  - 分析现有代码库的UI设计实现
  - 生成通用设计系统文档 `.ai-rules/ui-design.md`

- **🟣 `ui-fidelity-implementer` (violet)**：UI保真度实现
  - 分析设计参考（截图、设计图、URL等）
  - 生成像素级精确的代码实现

### 技术规范类 (Technical Standards)

- **🔷 `api-architect` (cyan)**：API设计规范
  - 分析现有代码库的API设计
  - 生成API设计规范文档 `.ai-rules/api-design.md`

- **🟠 `security-architect` (orange)**：安全规范
  - 分析现有代码库的安全实现
  - 生成安全规范文档 `.ai-rules/security.md`

- **🔘 `code-standards-architect` (slate)**：编码规范生成
  - 分析现有代码库的技术栈（语言、框架、构建工具）
  - 自动生成对应的编码规范规则文件 `.cursor/rules/{language}-rules.mdc`, `{framework}-rules.mdc` 等

### 文档类 (Documentation)

- **🔵 `prd-writer` (indigo)**：产品文档
  - 生成产品需求文档、用户手册等
  - 整理和优化项目文档

### 数据分析类 (Data Science)

- **🟢 `data-scientist` (emerald)**：数据科学
  - 数据分析和处理
  - 机器学习模型训练和优化

### 调试类 (Debugging)

- **🟡 `debugger` (yellow)**：问题排查
  - 定位技术问题、提供修复方案
  - 性能优化和错误诊断
