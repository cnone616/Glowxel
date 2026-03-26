# Glowxel 后续改进任务总表

## 1. 项目现状结论

当前仓库已经具备完整产品主链，不属于半成品状态，但存在明显的工程化短板。

综合评分：

- 产品功能完整度：84/100
- 工程成熟度：84/100
- 综合建议评分：87/100

当前判断：

- `uniapp` 是最完整的一端，功能主链最强，但超大页面较多
- `esp32-firmware` 主链稳定，编译通过，但核心大文件职责较重
- `server` 功能覆盖够用，但安全、测试、迁移体系不足
- `admin` 和 `website` 可构建，但工程收口和能力对齐还不够
- 当前最需要优先解决的不是“继续加功能”，而是安全、测试、环境治理和局部结构治理

## 1.1 当前执行进度（2026-03-26）

总体任务推进度：

- 已完成：`12 / 12`
- 进行中：`0 / 12`
- 未完成：`0 / 12`

阶段完成度：

- `P0`：`100%`
- `P1`：`90%`
- `P2`：`100%`
- `P3`：`100%`

当前阶段判断：

- 安全线：主干任务已完成
- 工程线：主干任务已完成，仍缺外部环境验证
- 结构线：主干任务已完成，当前主要剩外部编译与手工回归验证
- 产品线：Web 时钟路线已明确，用户预期已基本收口

已完成：

- `A1. 清理敏感信息`
  - 已清理部署文档、部署脚本、HTTPS 脚本、`website/admin` 开发代理中的真实生产地址和凭据暴露
  - 当前改为显式环境变量输入，不再在仓库中保存真实服务器信息
- `A2. 管理员密码改为 bcrypt`
  - 已新增统一的管理员密码服务
  - 当前策略为：新密码统一使用 `bcrypt`；历史 `sha256` 管理员哈希在登录成功后自动升级为 `bcrypt`
- `A3. 建立最小测试底线`
  - `server` 已补最小接口测试
  - `website` 与 `admin` 已补 `test` 脚本并复用构建测试
  - `esp32-firmware` 继续以 `platformio run` 作为编译测试基线
- `B1. 建立基础 CI`
  - 已新增 GitHub Actions 工作流，覆盖 `server / website / admin / esp32-firmware`
  - 当前已完成本地 YAML 解析校验，待远端仓库实际触发
- `B2. 环境配置统一`
  - `website` 与 `admin` 已统一到 `VITE_API_BASE_URL / VITE_API_PROXY_TARGET`
  - `uniapp` 已把请求域名和上传地址收口到单独配置入口
  - `server` 上传接口已改为按当前请求域名返回文件地址
- `B3. 数据库迁移机制`
  - 已新增迁移目录、版本表、迁移执行脚本
  - `seed.js` 已改为先跑迁移再灌种子
- `D3. 文档治理`
  - 已更新 `README.md`、`DEPLOY.md`、`docs/产品完整度分析.md`、`docs/项目架构与接口文档.md`
  - 当前核心文档已经和本轮代码状态对齐
- `C2. 固件大文件职责收口`
  - 已把 `websocket_handler` 的 JSON 命令处理拆到独立命令分组文件
  - 当前主文件只保留 WebSocket 事件、分片解析、二进制接收主链
- `C3. Web 时钟编辑器路线`
  - 已按方案 B 收口
  - Web 端当前明确为“能力说明页”，不再营造已具备完整时钟配置能力的预期
- `D1. 后台构建体积优化`
  - 已移除 `admin` 中 `Antd` 的全量注册，改为按组件自动引入
  - 已修正打包分块策略，构建最大共享 chunk 已从 `1314.76 kB` 降到 `287.19 kB`
- `D2. 权限、审计与可观测性增强`
  - 已新增服务端分级日志服务，`4xx`/`5xx` 响应会按 `warn / error` 级别自动记录
  - 已为 `/api/admin` 非只读操作接入自动审计中间件，管理员敏感操作会写入独立审计日志
  - 已补管理员登录成功/失败留痕，以及管理员权限失败留痕
  - 当前日志主链已收口到 `server.log / error.log / admin-audit.log`
- `C1. UniApp 超大页面治理`
  - `clock-editor` 已完成第一轮合理拆分
  - `create.vue` 已完成第二轮逻辑拆分，并清理无效状态与脱离模板的旧样式，主文件已由 `3094` 行降到 `1672` 行
  - `overview.vue` 已把导出/删除这组高副作用逻辑抽到页面专属 action mixin，主文件已由 `1711` 行降到 `1519` 行
  - `assist.vue` 已把设备同步与画板模式切换抽到页面专属 mixin，主文件已由 `2052` 行降到 `1815` 行
  - `editor.vue` 已把保存、缩略图、发布链路抽到页面专属 mixin，并清理无入口的历史设备同步逻辑，主文件已由 `1724` 行降到 `1237` 行

已完成验证：

- 敏感信息关键字检索通过
- `deploy.sh`、`setup-https.sh` 语法检查通过
- `server` 冒烟测试通过
- `website` 构建测试通过
- `admin` 构建测试通过
- `esp32-firmware` 编译测试通过
- `server` 迁移服务单元测试通过
- `esp32-firmware` 在命令分组重构后重新编译通过
- `website` 在 Web 时钟编辑器路线收口后重新构建通过
- `uniapp` API 配置检查测试通过
- `admin` 在按需引入与分块优化后重新构建通过
- `create.vue` 二轮拆分后 `uniapp` API 配置检查再次通过
- `overview.vue` action mixin 拆分后 `uniapp` API 配置检查再次通过
- `assist.vue` 设备同步 mixin 拆分后 `uniapp` API 配置检查再次通过
- `editor.vue` 保存/发布 mixin 拆分与死代码清理后 `uniapp` API 配置检查再次通过
- `create.vue` 死代码与旧样式清理后 `uniapp` API 配置检查再次通过
- `server` 在审计日志与分级日志接入后测试再次通过
- 管理员关键操作审计测试通过
- 错误日志输出验证通过

待外部环境验证：

- GitHub Actions 真实触发结果需要在远端仓库执行确认
- `server` 真实数据库迁移需要本机或目标环境有可连接的 MySQL；当前本地执行返回 `ECONNREFUSED`
- `uniapp` 目前已完成配置收口，但本机缺少 `uni` CLI，未做 H5 / 微信小程序编译验证

进度更新规则：

- 每完成一个任务块，先更新本节评分、阶段完成度和状态，再进入下一项任务

## 2. 总体目标

后续优化以四条主线推进：

1. 安全线
2. 工程线
3. 结构线
4. 产品线

总原则：

- 不为了重构而重构
- 不为了拆分而拆分
- 每完成一项任务，必须做对应代码测试
- 先做高风险、高收益、低争议任务
- 涉及架构调整的任务，要先保证现有主链不回退

## 3. 优先级总览

### P0：必须优先处理

- 清理仓库内敏感信息
- 修复后端管理员密码方案
- 建立最小测试底线

### P1：短周期工程收口

- 建立基础 CI
- 统一环境配置策略
- 建立数据库迁移策略

### P2：结构治理

- 收口 UniApp 超大页面
- 收口固件超大核心文件
- 明确 Web 时钟编辑器产品定位

### P3：中期优化

- 后台构建体积优化
- 权限与审计增强
- 文档和代码同步治理

## 4. 分阶段执行计划

---

## 阶段 A：1 周内完成

### 目标

先把安全和最低工程底线补起来，避免项目继续积累高风险债务。

### A1. 清理敏感信息

任务内容：

- 检查并清理 [DEPLOY.md](/Users/aflylong/Desktop/project/Glowxel/DEPLOY.md) 中暴露的服务器 IP、数据库密码、JWT 密钥等信息
- 排查仓库内所有文档、脚本、示例配置是否还有真实密钥、真实密码、真实设备密钥
- 用 `.env.example` 或部署说明占位替代真实值
- 给后续部署约定新的配置来源，不再把真实值写入仓库

完成标准：

- 仓库内不再出现真实密码、真实密钥、真实部署凭据
- 文档可指导部署，但不泄漏生产信息

完成后必须执行的测试：

- 全仓搜索关键字测试
  - `JWT_SECRET`
  - `DB_PASSWORD`
  - `DEVICE_API_KEY`
  - `WX_SECRET`
  - 生产服务器 IP
- 确认部署文档仍可读、字段说明不缺失

---

### A2. 管理员密码改为 bcrypt

任务内容：

- 把 `server` 中管理员密码校验从 `sha256 + salt` 改为 `bcrypt`
- 更新管理员种子初始化逻辑
- 明确兼容策略
  - 推荐做法：一次性迁移
  - 如果必须兼容旧密码，先写清迁移路径后再实施

涉及文件：

- [server/src/routes/user.js](/Users/aflylong/Desktop/project/Glowxel/server/src/routes/user.js)
- [server/src/config/seed.js](/Users/aflylong/Desktop/project/Glowxel/server/src/config/seed.js)
- [server/src/config/init.sql](/Users/aflylong/Desktop/project/Glowxel/server/src/config/init.sql)

完成标准：

- 管理员登录不再依赖 `sha256`
- 新生成的管理员密码哈希使用 `bcrypt`
- 登录接口行为不回退

完成后必须执行的测试：

- 管理员登录接口测试
- 错误密码拒绝测试
- 老账号迁移验证测试
- `server` 本地启动测试

---

### A3. 建立最小测试底线

任务内容：

- 为 `server` 增加最小接口测试
- 至少覆盖：
  - `/api/health`
  - 管理员登录
  - 用户设置读写
  - 固件列表接口
- 为 `website` 与 `admin` 保留构建测试作为第一层保障
- 为 `esp32-firmware` 保留 `platformio run` 作为编译测试

完成标准：

- 仓库里存在真实测试脚本，不再是空 `test`
- 至少有一组最小冒烟测试可以跑

完成后必须执行的测试：

- `server` 测试脚本跑通
- `website` 构建通过
- `admin` 构建通过
- `esp32-firmware` 编译通过

---

## 阶段 B：2 周内完成

### 目标

补齐最基础的工程体系，让多人协作和后续迭代更稳定。

### B1. 建立基础 CI

任务内容：

- 增加 CI 流程
- 至少覆盖：
  - `server` 安装依赖 + 测试
  - `website` 安装依赖 + 构建
  - `admin` 安装依赖 + 构建
  - `esp32-firmware` 编译
- `uniapp` 因当前主要依赖 HBuilderX / 微信开发者工具，可先做静态校验或脚本层检查，不强行做复杂 CI

完成标准：

- 每次提交后，核心模块至少有自动构建或测试反馈

完成后必须执行的测试：

- 手动触发一次 CI
- 故意制造一个构建错误，确认 CI 能拦截

---

### B2. 统一环境配置策略

任务内容：

- 统一 `website`、`admin`、`server`、`uniapp` 的环境配置方式
- 重点修复当前不一致问题：
  - `website` 使用环境变量
  - `uniapp` 直接写死线上域名
- 输出统一配置约定：
  - 本地开发
  - 测试环境
  - 生产环境

涉及重点文件：

- [website/src/api/request.js](/Users/aflylong/Desktop/project/Glowxel/website/src/api/request.js)
- [uniapp/api/request.js](/Users/aflylong/Desktop/project/Glowxel/uniapp/api/request.js)
- [admin/src/api/http.js](/Users/aflylong/Desktop/project/Glowxel/admin/src/api/http.js)

完成标准：

- 多端请求基址不再靠硬编码散落维护
- 本地、测试、生产环境切换有清晰规则

完成后必须执行的测试：

- `website` 本地接口请求验证
- `admin` 本地接口请求验证
- `uniapp` 小程序开发环境接口验证
- 环境变量缺失时的报错验证

---

### B3. 建立数据库迁移策略

任务内容：

- 不再只依赖 [server/src/config/init.sql](/Users/aflylong/Desktop/project/Glowxel/server/src/config/init.sql)
- 增加数据库迁移机制
- 最低要求：
  - 迁移目录
  - 迁移版本记录
  - 执行脚本
- 历史表结构以 `init.sql` 为基线，后续走增量迁移

完成标准：

- 以后新增字段、新表、索引调整都有明确迁移路径

完成后必须执行的测试：

- 空库初始化测试
- 老库增量迁移测试
- 回滚策略验证或至少文档说明

---

## 阶段 C：1 个月内完成

### 目标

把当前明显拖慢开发效率的结构性问题收口，但不做无意义大重构。

### C1. UniApp 超大页面治理

重点页面：

- [uniapp/pages/create/create.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/create/create.vue)
- [uniapp/pages/assist/assist.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/assist/assist.vue)
- [uniapp/pages/editor/editor.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/editor/editor.vue)
- [uniapp/pages/overview/overview.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/overview/overview.vue)
- [uniapp/pages/clock-editor/clock-editor.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/clock-editor/clock-editor.vue)

任务内容：

- 不做全量碎拆
- 只抽真正独立的逻辑块
- 优先顺序：
  1. 设备连接与状态逻辑
  2. 图片/GIF 处理
  3. 渲染和导出
  4. 大型表单配置逻辑

完成标准：

- 页面主文件保留页面壳子和装配逻辑
- 抽出来的模块职责清晰，不产生新的高耦合碎文件

当前完成结果：

- `clock-editor` 已完成首轮 mixin/component 收口
- `create / assist / editor / overview` 均已完成页面专属逻辑抽离
- 已额外清理一批无入口状态、死代码和脱离模板的旧样式
- 当前 UniApp 超大页面治理的代码侧目标已完成，剩余仅是微信开发者工具与手工回归验证

完成后必须执行的测试：

- 微信开发者工具编译测试
- 关键页面手工回归
  - 编辑器
  - 时钟编辑器
  - 设备控制
  - BLE 配网

---

### C2. 固件大文件职责收口

重点文件：

- [esp32-firmware/src/websocket_handler.cpp](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/websocket_handler.cpp)
- [esp32-firmware/src/display_manager.cpp](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/display_manager.cpp)
- [esp32-firmware/src/animation_manager.cpp](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/animation_manager.cpp)

任务内容：

- `websocket_handler` 拆命令处理分组
  - 模式控制
  - 静态像素传输
  - GIF/动画传输
  - 状态查询
- `display_manager` 拆显示职责
  - 背景层
  - 时钟层
  - loading / overlay
- 明确配置层、显示层、传输层边界

完成标准：

- 核心文件长度下降
- 新增命令或新显示模式时，不需要反复修改单个巨型文件

完成后必须执行的测试：

- `platformio run`
- 动画模式回归测试
- 时钟模式回归测试
- WebSocket 命令回归测试

---

### C3. 明确 Web 时钟编辑器路线

当前现状：

- [website/src/views/ClockEditor.vue](/Users/aflylong/Desktop/project/Glowxel/website/src/views/ClockEditor.vue) 仍是占位页

任务内容：

必须先做产品决策，二选一：

方案 A：补齐 Web 时钟编辑能力

- 先补协议和配置能力
- 再决定是否补设备联动
- 最后再补 UI 体验一致性

方案 B：明确不做

- 把 Web 端文案、入口和产品定位改清楚
- 避免用户误以为 Web 端应具备完整设备配置能力

完成标准：

- Web 端时钟能力不再处于“看起来像缺失”的中间态

完成后必须执行的测试：

- 路由访问测试
- 页面文案与入口一致性检查
- 若实施方案 A，还需补功能回归测试

---

## 阶段 D：持续优化

### D1. 后台构建体积优化

当前现状：

- `admin` 构建已完成首轮收口，原先超大的 `ant-design` 共享包已压降

任务内容：

- 做路由级懒加载审查
- 分析 Ant Design Vue 的按需引入情况
- 优化大包输出

当前完成结果：

- 已确认后台路由本身已经是懒加载，不存在首屏把业务页面全部打进入口的问题
- 已移除 `admin/src/main.js` 中的 `app.use(Antd)` 全量注册
- 已接入 `unplugin-vue-components` 做 Ant Design Vue 组件自动按需引入
- 已修正 `vite` 分块规则，避免把 `ant-design-vue` 误并入统一大包
- 当前构建最大共享 chunk 已由 `1314.76 kB` 降至 `287.19 kB`，构建告警已消失

完成后必须执行的测试：

- `admin` 构建对比测试
- 主要页面打开速度对比

---

### D2. 权限、审计与可观测性增强

任务内容：

- 增加关键操作日志
- 管理后台敏感操作留痕
- 服务端错误日志分级
- 后续可接简单告警

完成后必须执行的测试：

- 管理员关键操作审计验证
- 错误日志输出验证

---

### D3. 文档治理

任务内容：

- 更新以下文档与当前代码一致：
  - [README.md](/Users/aflylong/Desktop/project/Glowxel/README.md)
  - [DEPLOY.md](/Users/aflylong/Desktop/project/Glowxel/DEPLOY.md)
  - [docs/产品完整度分析.md](/Users/aflylong/Desktop/project/Glowxel/docs/产品完整度分析.md)
  - [docs/项目架构与接口文档.md](/Users/aflylong/Desktop/project/Glowxel/docs/项目架构与接口文档.md)

完成后必须执行的测试：

- 文档链接检查
- 文档与代码抽样一致性检查

## 5. 推荐任务顺序

建议严格按下面顺序推进：

1. 清理敏感信息
2. 管理员密码改为 `bcrypt`
3. 建立最小测试底线
4. 建立基础 CI
5. 统一环境配置策略
6. 建立数据库迁移机制
7. UniApp 超大页面治理
8. 固件大文件职责治理
9. Web 时钟编辑器路线决策
10. 后台构建体积优化
11. 权限与审计增强
12. 文档治理

## 6. 每次任务完成后的统一测试规则

无论做哪项任务，完成后至少执行对应范围的测试：

### 前端改动

- `website` 改动后：执行构建测试
- `admin` 改动后：执行构建测试
- `uniapp` 改动后：至少执行微信开发者工具编译测试或 HBuilderX 编译测试

### 后端改动

- `server` 改动后：执行接口测试或最小启动测试
- 涉及数据库变更：必须加初始化测试和迁移测试

### 固件改动

- `esp32-firmware` 改动后：必须执行 `platformio run`
- 涉及动画、时钟、设备传输的改动：必须做功能回归

## 6.1 执行纪律

从当前轮开始，后续任务统一按以下顺序执行：

1. 完成当前任务代码改动
2. 执行对应测试
3. 把任务结果、测试结果、剩余风险同步回 `task.md`
4. 不等待额外确认，直接推进下一项任务

说明：

- 如果没有遇到真实阻塞，不因“本项已完成”而停下
- 若遇到外部环境阻塞，要在 `task.md` 中注明阻塞原因和下一步接续条件

## 7. 任务执行模板

后续每次执行任务，建议按这个格式记录：

### 任务名

- 任务目标：
- 涉及模块：
- 影响范围：
- 修改文件：
- 完成标准：
- 已执行测试：
- 测试结果：
- 是否允许进入下一任务：

## 8. 当前建议立即开工的任务

如果从现在开始排，我建议下一步直接开这三个任务：

1. `P0-A1` 清理敏感信息
2. `P0-A2` 管理员密码改为 `bcrypt`
3. `P0-A3` 建立最小测试底线

这是当前最稳、最值、争议最小的一条推进路线。
