# UniApp 客户端说明

这是仓库里的主客户端，承担创作、社区、设备控制和 BLE 配网，也是当前最完整的设备编辑端。

## 主要能力

- 像素创作：新建、编辑、分板、辅助拼豆
- 社区链路：发布作品、详情、评论、点赞、收藏、关注
- 设备链路：WebSocket 控制设备、亮度、模式切换
- 设备编辑：时钟、主题、效果、功能扩展、通知提醒、宠物眼睛
- 配网链路：蓝牙扫描 `RenLight-Setup`，写入 WiFi 凭据

## 运行方式

推荐方式：

1. 使用 HBuilderX 导入 `uniapp/`
2. 运行到微信开发者工具 / H5 / 模拟器

如果本机有 `uni` 命令，也可以使用：

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
- `pages/effect-editor/effect-editor.vue`
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
