# UniApp 客户端说明

这是仓库里的主客户端，承担创作、社区、设备控制和热点配网引导，也是当前最完整的设备编辑端。

## 主要能力

- 像素创作：新建、编辑、分板、辅助拼豆
- 社区链路：发布作品、详情、评论、点赞、收藏、关注
- 设备链路：WebSocket 控制设备、亮度、模式切换
- 设备编辑：时钟、主题、效果、功能扩展、通知提醒、宠物眼睛
- 配网链路：连接设备热点 `Glowxel-设备序列号`，打开 `192.168.4.1` 写入 WiFi 凭据

## 运行方式

默认运行方式：

1. 使用 HBuilderX 导入 `uniapp/`
2. 运行到微信开发者工具 / H5 / 模拟器

说明：

- 当前项目默认按 HBuilderX 工程维护与验证
- 终端里的 `npm run dev:h5`、`npm run build:h5` 只有在本机额外具备 `uni` 命令时才可用
- 若执行时报 `uni: command not found`，表示当前终端缺少对应 CLI 环境，不表示项目本身不可运行

如果本机已经单独安装可用的 `uni` 命令，也可以尝试：

```bash
npm install
npm run dev:h5
npm run dev:mp-weixin
```

## 关键页面

- `pages/create/create.vue`
- `pages/editor/editor.vue`
- `pages/overview/overview.vue`
- `pages/assist/assist.vue`
- `pages/control/control.vue`
- `pages/clock-editor/clock-editor.vue`
- `pages/feature-editor/feature-editor.vue`
- `pages/spirit-screen/spirit-screen.vue`
- `pages/canvas-editor/canvas-editor.vue`

## 关键模块

- `store/project.js`
- `store/device.js`
- `utils/webSocket.js`
- `utils/exportCanvas.js`

## 相关文档

1. [development-guardrails.md](/Users/aflylong/Desktop/project/Glowxel/uniapp/docs/development-guardrails.md)
2. [设备显示迁移参考.md](/Users/aflylong/Desktop/project/Glowxel/uniapp/docs/设备显示迁移参考.md)
3. [开发路线图.md](/Users/aflylong/Desktop/project/Glowxel/docs/开发路线图.md)
