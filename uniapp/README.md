# uni-app 版本

专为 64x64 LED 矩阵板设计的像素艺术布局工具 - uni-app 多端版本

## 📱 支持平台

- 微信小程序
- H5 网页
- iOS App
- Android App

## ✨ 核心功能

### 画布编辑

- **多工具支持** - 绘画/擦除/移动工具
- **撤销重做** - 完整的历史记录管理
- **缩放平移** - 双指缩放和单指拖动
- **网格显示** - 可切换的像素网格
- **调色板** - 按字母分组的颜色选择器

### 图片导入

- **智能像素化** - 自动转换为像素艺术
- **颜色匹配** - 匹配 Artkal 拼豆颜色库
- **尺寸调整** - 支持自定义画布尺寸

### 看板模式

- **自动分割** - 52x52 板子自动分割大画布
- **进度追踪** - 每个看板独立进度显示
- **快速导航** - 字母索引快速跳转
- **批量导出** - 支持单个或全部看板导出

### 拼豆辅助

- **逐行模式** - 按行拼装，自动遮罩其他行
- **逐色模式** - 按颜色高亮，批量拼装同色豆子
- **进度管理** - 标记完成的行和颜色
- **行导航** - 快速跳转到任意行

### 导出功能

- **PNG 图纸** - 带坐标和色号标注
- **高清输出** - 可自定义像素大小
- **保存相册** - 直接保存到系统相册
- **跨平台** - H5/App/小程序全平台支持

### 主题系统

- **深色主题** - 护眼的深色界面
- **浅色主题** - 明亮的浅色界面
- **自动适配** - 所有组件自动适配主题
- **平滑切换** - 无闪烁的主题切换

### 颜色库

- **Artkal 221 色** - 完整的 Artkal 拼豆颜色库
- **按字母分组** - A-Z 字母索引
- **快速搜索** - 字母索引条快速定位
- **颜色预览** - 真实颜色显示

## 🚀 快速开始

### 开发环境

1. 安装 [HBuilderX](https://www.dcloud.io/hbuilderx.html)
2. 导入项目到 HBuilderX
3. 安装依赖（如需要）
4. 运行到微信开发者工具/浏览器/模拟器

### 生产构建

- **微信小程序**: 发行 → 小程序-微信
- **H5**: 发行 → 网站-H5
- **App**: 发行 → 原生App-云打包

## 📦 项目结构

```
uniapp/
├── components/          # 组件
│   ├── CustomTabBar.vue # 自定义底部导航栏
│   ├── ColorPalette.vue # 颜色选择器
│   ├── PixelCanvas.vue  # 像素画布
│   ├── ProjectCard.vue  # 项目卡片
│   ├── ProjectThumbnail.vue # 项目缩略图
│   ├── BoardThumbnail.vue   # 看板缩略图
│   └── HelpModal.vue    # 帮助弹窗
├── pages/              # 页面
│   ├── library/        # 画布库 - 项目列表
│   ├── create/         # 新建画布 - 创建向导
│   ├── editor/         # 编辑器 - 像素绘制
│   ├── overview/       # 看板总览 - 分板管理
│   ├── assist/         # 拼豆辅助 - 拼装辅助
│   ├── control/        # 设备控制 - 蓝牙/WiFi
│   └── profile/        # 个人中心 - 设置和主题
├── store/              # Pinia 状态管理
│   ├── project.js      # 项目数据管理
│   ├── theme.js        # 主题管理
│   ├── device.js       # 设备连接管理
│   └── user.js         # 用户状态管理
├── utils/              # 工具函数
│   ├── imageImport.js  # 图片导入和像素化
│   ├── exportCanvas.js # 导出图纸功能
│   ├── palette.js      # 调色板工具
│   ├── storage.js      # 本地存储封装
│   ├── webSocket.js    # WebSocket 连接
│   └── uuid.js         # UUID 生成
├── composables/        # 组合式函数
│   └── useToast.js     # Toast 提示
├── data/               # 数据
│   └── artkal-colors-full.js # Artkal 221 色库
├── styles/             # 全局样式
│   ├── theme.css       # 主题 CSS 变量
│   └── themes/         # 深色/浅色主题
└── static/             # 静态资源
    ├── RenLight-logo.png # 应用Logo图标
```

## 🎨 设计规范

### 颜色系统

#### 深色主题

- 主强调色: `#00f3ff` (青色)
- 次强调色: `#00ff9d` (绿色)
- 背景色: `#0a0a0a` / `#111111` / `#1a1a1a`
- 文字色: `#ffffff` / `#a0a0a0` / `#666666`

#### 浅色主题

- 主强调色: `#0099cc` (蓝色)
- 次强调色: `#00aa88` (绿色)
- 背景色: `#fafafa` / `#f5f5f5` / `#eeeeee`
- 文字色: `#2c2c2c` / `#5a5a5a` / `#8a8a8a`

### 圆角规范

- 6rpx - 极小元素（勾选框）
- 12rpx - 小元素（按钮、输入框）
- 16rpx - 中等元素（卡片）
- 24rpx - 大元素（面板）
- 32rpx - 特大元素（弹窗）

### 阴影系统

- `shadow-sm` - 轻微阴影 (0 2rpx 8rpx)
- `shadow-md` - 中等阴影 (0 4rpx 16rpx)
- `shadow-lg` - 大阴影 (0 8rpx 32rpx)
- `shadow-glow` - 发光效果 (0 0 30rpx)

### 动画时长

- `transition-fast` - 0.15s (快速交互)
- `transition-base` - 0.2s (标准交互)
- `transition-slow` - 0.3s (慢速动画)

## 🔧 技术栈

- **框架**: uni-app 3.x
- **状态管理**: Pinia
- **样式**: CSS Variables + scoped CSS
- **图标**: 自定义 SVG 组件
- **存储**: uni.storage (本地存储)
- **Canvas**: uni-app Canvas 2D API
- **通信**: WebSocket (设备控制)

## 📝 开发说明

### 图标组件

所有图标组件统一支持 `color` prop：

```vue
<IconHome :size="48" color="#00f3ff" />
```

默认使用 `currentColor` 继承父元素颜色。

### 主题切换

通过 CSS Variables 和动态 class 实现：

```javascript
// store/theme.js
setTheme(themeId) {
  this.currentTheme = themeId
  this.applyTheme()
}

applyTheme() {
  const page = document.querySelector('page')
  if (this.isDarkMode) {
    page.classList.remove('light-theme')
  } else {
    page.classList.add('light-theme')
  }
}
```

### 底部导航栏

使用自定义 `CustomTabBar` 组件：

- 中间凸起的加号按钮
- 渐变背景和发光效果
- 图标和文字颜色自动适配主题

### Canvas 绘制

使用 uni-app Canvas 2D API：

```javascript
const canvas = res[0].node;
const ctx = canvas.getContext("2d");
canvas.width = width;
canvas.height = height;
ctx.fillStyle = color;
ctx.fillRect(x, y, w, h);
```

### 数据持久化

使用 Pinia + uni.storage 实现本地存储：

```javascript
// 保存项目
saveProjects() {
  uni.setStorageSync('projects', JSON.stringify(this.projects))
}

// 保存像素数据
saveProjectPixels(projectId, pixels) {
  const data = JSON.stringify(Array.from(pixels.entries()))
  uni.setStorageSync(`pixels-${projectId}`, data)
}
```

### 图片导入

支持多种方式导入图片：

1. 相册选择
2. 相机拍照
3. 文件选择（H5）

自动进行像素化和颜色匹配。

### 导出功能

跨平台导出方案：

- **H5**: 使用离屏 Canvas + Blob 下载
- **App/小程序**: 使用页面 Canvas + `canvasToTempFilePath`

## 🎯 版本历史

### v1.0.0 (2024-02-27)

**首次发布 - 完整功能实现**

#### 核心功能

- ✅ 7 个完整页面（画布库、新建、编辑器、看板总览、拼豆辅助、设备控制、设置）
- ✅ 33 个 SVG 图标组件，全部支持 color prop
- ✅ 完整的主题系统（深色/浅色）
- ✅ Artkal 221 色完整支持
- ✅ 图片导入和像素化
- ✅ 跨平台导出功能

#### 界面优化

- ✅ 统一的颜色系统（深色 #00f3ff，浅色 #0099cc）
- ✅ 所有图标和文字颜色保持一致
- ✅ 移除所有 emoji，使用图标组件
- ✅ 优化弹窗样式，更紧凑的布局
- ✅ 浅色主题下的颜色卡片使用白色背景

#### 性能优化

- ✅ 移除所有 console.log 打印
- ✅ 页面加载时立即应用主题，避免闪烁
- ✅ 优化 Canvas 绘制性能
- ✅ 延迟加载像素数据（10ms）

#### 交互优化

- ✅ 移除颜色选中时的缩放效果，避免遮挡
- ✅ 增加颜色列表底部间距
- ✅ 调色板从 6 列改为 5 列，避免超出容器
- ✅ 移除调色板选中颜色的勾选标记
- ✅ 编辑模式不自动选中颜色

#### Bug 修复

- ✅ 修复逐行模式高亮问题
- ✅ 修复重命名弹窗输入框样式
- ✅ 修复辅助拼豆页面初始化闪屏
- ✅ 修复导出功能在不同平台的兼容性
- ✅ 修复勾选框显示两个边框的问题
- ✅ 修复图标颜色不一致的问题

## 📄 许可

MIT License

## 🙏 致谢

- 图标基于 [lucide-vue-next](https://lucide.dev/)
- 颜色库来自 [Artkal](https://www.artkal.com/)
- 框架使用 [uni-app](https://uniapp.dcloud.io/)
