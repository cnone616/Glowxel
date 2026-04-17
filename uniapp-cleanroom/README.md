# Glowxel uniapp Cleanroom

这个目录是 Glowxel `uniapp` 的并行重构工程，目标是：

- 用全新的自定义导航和自定义 `tabBar` 替换旧工程的原生 `tabBar` 依赖
- 清除旧工程的全局样式污染，不再使用 `!important` 和标签级强压
- 用统一组件收口顶部栏、搜索栏、返回按钮、工具条、loading、toast 和弹层
- 先完成 5 个一级页和高频二级页的骨架，再逐步迁移旧业务功能

当前阶段为首批骨架版，重点验证：

- 自定义 `tabBar`
- 一级/二级标题栏
- 固定搜索工具条
- 特殊操作按钮语义
- 自定义 loading / toast / dialog 宿主
