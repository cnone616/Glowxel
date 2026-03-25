# Glowxel Website

官网与在线创作站点，基于 Vue 3 + Vite。

## 当前功能

- 首页、社区、挑战、模板、个人主页
- 在线创作页与像素编辑页
- 作品详情、挑战详情
- 登录与基础个人资料编辑
- 静态硬件指南页：`public/hardware-guide.html`

## 开发命令

```bash
npm install
npm run dev
npm run build
npm run preview
```

## 目录说明

```text
website/
├── public/              # 纯静态资源
├── src/api/             # HTTP 接口封装
├── src/components/      # 公共组件
├── src/data/            # 颜色等静态数据
├── src/router/          # 路由
├── src/utils/           # 图片/导出工具
├── src/views/           # 页面
└── vite.config.js
```

## 说明

- 当前站点使用真实后端接口，不再以 Mock 站点为主描述
- `dist/` 为构建产物目录
- 如果修改了首页硬件入口，请同步检查 `public/hardware-guide.html`
