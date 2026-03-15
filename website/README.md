# MATRIX 官网

LED 矩阵创作平台 - 在线设计、实时预览、一键导出

## 功能特性

- 时钟设计器 - 自定义时间、日期、星期的显示样式
- 图案编辑器 - 64x64 像素画布创作，支持画笔、橡皮擦、填充工具
- 作品展示 - 浏览社区创作，获取灵感
- 响应式设计 - 完美支持移动端和桌面端
- 配置导出 - 一键导出 JSON 配置文件

## 技术栈

- Vue 3 - 渐进式 JavaScript 框架
- Vite - 下一代前端构建工具
- Vue Router - 官方路由管理器
- Pinia - 状态管理
- Canvas API - 图形渲染

## 开发

```bash
# 安装依赖
npm install

# 启动开发服务器
npm run dev

# 构建生产版本
npm run build

# 预览生产构建
npm run preview
```

## 项目结构

```
website/
├── public/              # 静态资源
├── src/
│   ├── api/            # API 接口
│   ├── assets/         # 资源文件
│   ├── components/     # 公共组件
│   ├── mock/           # Mock 数据
│   ├── router/         # 路由配置
│   ├── utils/          # 工具函数
│   ├── views/          # 页面组件
│   ├── App.vue         # 根组件
│   └── main.js         # 入口文件
├── index.html
├── package.json
└── vite.config.js
```

## Mock 数据

当前使用 Mock 数据进行前端开发，所有 API 调用都返回模拟数据。

- `src/mock/showcaseData.js` - 作品展示数据
- `src/mock/clockTemplates.js` - 时钟模板数据

## 核心功能

### 时钟设计器
- 基于 esp32-firmware 的 led-simulator
- 实时 Canvas 预览
- 支持时间、日期、星期自定义
- 9 种预设颜色
- 配置保存和导出

### 图案编辑器
- 64x64 像素点阵编辑
- 画笔、橡皮擦、填充工具
- 可调节画笔大小（1-4像素）
- 图片导入和应用
- 快速操作：填充全部、反转颜色、镜像
- 像素数据导出

## 待办事项

- [ ] 添加用户系统
- [ ] 实现作品上传功能
- [ ] 集成真实后端 API
- [ ] 添加更多时钟模板
- [ ] 优化移动端体验
- [ ] 支持二维码分享配置
- [ ] 添加撤销/重做功能

## License

MIT
