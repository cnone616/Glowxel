# UniApp 客户端说明

这是仓库里的主客户端，承担创作、社区、设备控制和 BLE 配网。

## 主要能力

- 像素创作：新建、编辑、分板、辅助拼豆
- 社区链路：发布作品、详情、评论、点赞、收藏、关注
- 挑战链路：挑战列表、详情、参与、投稿
- 设备链路：WebSocket 控制设备、亮度、模式切换
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

- `pages/create/create.vue`：创建项目
- `pages/editor/editor.vue`：像素编辑器
- `pages/overview/overview.vue`：分板总览
- `pages/assist/assist.vue`：辅助拼豆
- `pages/control/control.vue`：设备控制
- `pages/clock-editor/clock-editor.vue`：时钟编辑器
- `pages/effect-editor/effect-editor.vue`：文本展示 / 呼吸灯 / 律动编辑
- `pages/canvas-editor/canvas-editor.vue`：设备画板模式编辑
- `pages/ble-config/ble-config.vue`：BLE 配网
- `pages/publish-project/publish-project.vue`：发布作品

## 关键模块

- `store/project.js`：本地项目与作品衍生数据
- `store/user.js`：用户信息、登录与社交关系
- `store/device.js`：设备连接状态
- `utils/webSocket.js`：设备通信封装
- `utils/gifParser.js`：GIF 解析与 ESP32 传输数据生成
- `utils/exportCanvas.js`：导出图纸

## 备注

- 当前仓库保留 `utils/cloud.js`，但主链以当前 API/本地数据流为主
- 设备控制链路与固件当前契约已对齐：
  - 模式：`clock / animation / canvas / transferring / text_display / breath_effect / rhythm_effect`
  - 效果命令：`set_breath_effect / set_rhythm_effect`
  - 画板命令：`save_canvas / load_canvas / clear_canvas`
- 旧的重复页面副本已清理，当前仅保留一个 `clock-editor.vue`
