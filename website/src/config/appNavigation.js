const PRIMARY_DEVICE_NAVIGATION = [
  { key: "device_control", label: "设备控制", to: "/" },
  { key: "clock", label: "静态时钟", to: "/clock" },
  { key: "animation_clock", label: "动态时钟", to: "/animation-clock" },
  { key: "theme_clock", label: "主题模式", to: "/theme-clock" },
  { key: "device_params", label: "设备参数", to: "/device-params" },
  { key: "ble", label: "热点配网", to: "/ble-config" },
];

const DEVICE_CONTROL_ROUTE_PREFIXES = [
  "/device-control",
  "/spirit-screen",
  "/canvas-editor",
  "/gif-player",
  "/led-matrix",
  "/maze-mode",
  "/snake-mode",
  "/planet-screensaver",
  "/tetris-settings",
  "/tetris-clock-settings",
  "/water-world",
];

export const publicNavigation = PRIMARY_DEVICE_NAVIGATION;

export const appNavigation = PRIMARY_DEVICE_NAVIGATION;

export const devicePageGroups = [
  {
    key: "system",
    title: "设备基础",
    meta: "连接 / 配网 / 参数",
    entries: [
      {
        key: "device_control",
        title: "设备控制",
        desc: "连接设备、切换业务模式、查看当前运行态回显。",
        to: "/device-control",
        icon: "wifi",
        iconShellClass: "device-home__tool-icon--paper",
      },
      {
        key: "device_params",
        title: "设备参数",
        desc: "亮度、旋转、色彩顺序、NTP 与驱动参数统一在这里保存。",
        to: "/device-params",
        icon: "params",
        iconShellClass: "device-home__tool-icon--yellow",
      },
      {
        key: "ble_config",
        title: "热点配网",
        desc: "按 192.168.4.1 配网页流程接入 2.4GHz WiFi。",
        to: "/ble-config",
        icon: "phone",
        iconShellClass: "device-home__tool-icon--green",
      },
    ],
  },
  {
    key: "clock_family",
    title: "时钟与主题",
    meta: "预览 / 发送 / 缓存恢复",
    entries: [
      {
        key: "clock",
        title: "静态时钟",
        desc: "编辑时间、日期、星期、图片与静态像素预览。",
        to: "/clock",
        icon: "clock",
        iconShellClass: "device-home__tool-icon--cyan",
      },
      {
        key: "animation_clock",
        title: "动态时钟",
        desc: "上传 GIF、预览动画背景，并发送时钟叠加配置。",
        to: "/animation-clock",
        icon: "animation",
        iconShellClass: "device-home__tool-icon--teal",
      },
      {
        key: "theme_clock",
        title: "主题模式",
        desc: "切主题、看大预览，按设备主题合同下发。",
        to: "/theme-clock",
        icon: "theme",
        iconShellClass: "device-home__tool-icon--purple",
      },
    ],
  },
  {
    key: "scene_family",
    title: "创作与场景",
    meta: "桌面宠物 / 画板 / GIF / 场景",
    entries: [
      {
        key: "spirit_screen",
        title: "桌面宠物",
        desc: "表情、眼球、眨眼与时钟叠加保持同页编辑。",
        to: "/spirit-screen",
        icon: "eyes",
        iconShellClass: "device-home__tool-icon--pink",
      },
      {
        key: "canvas_editor",
        title: "画板模式",
        desc: "在 64×64 画布上编辑像素并按稀疏像素链发送。",
        to: "/canvas-editor",
        icon: "canvas",
        iconShellClass: "device-home__tool-icon--orange",
      },
      {
        key: "gif_player",
        title: "GIF 播放器",
        desc: "选择离线素材或 GIF，预览播放后发送动画底座。",
        to: "/gif-player",
        icon: "gif",
        iconShellClass: "device-home__tool-icon--indigo",
      },
      {
        key: "led_matrix",
        title: "矩阵流光",
        desc: "切换本地流光场景并按 ambient effect 合同发送。",
        to: "/led-matrix",
        icon: "matrix",
        iconShellClass: "device-home__tool-icon--azure",
      },
      {
        key: "water_world",
        title: "水世界",
        desc: "独立水世界时钟配置、预览动画和主题切换。",
        to: "/water-world",
        icon: "water",
        iconShellClass: "device-home__tool-icon--blue",
      },
    ],
  },
  {
    key: "game_family",
    title: "游戏与屏保",
    meta: "迷宫 / 贪吃蛇 / 方块 / 星球",
    entries: [
      {
        key: "maze_mode",
        title: "迷宫漫游",
        desc: "调整迷宫尺寸、速度和颜色面板，再发送到设备。",
        to: "/maze-mode",
        icon: "maze",
        iconShellClass: "device-home__tool-icon--orange",
      },
      {
        key: "snake_mode",
        title: "贪吃蛇",
        desc: "编辑蛇宽、颜色、字体和时间叠加参数。",
        to: "/snake-mode",
        icon: "snake",
        iconShellClass: "device-home__tool-icon--green",
      },
      {
        key: "tetris_settings",
        title: "俄罗斯方块屏保",
        desc: "按屏保页独立调 cellSize、speed、showClock 与块型。",
        to: "/tetris-settings",
        icon: "tetris",
        iconShellClass: "device-home__tool-icon--indigo",
      },
      {
        key: "tetris_clock_settings",
        title: "俄罗斯方块时钟",
        desc: "独立配置方块时钟预览、速度和小时制。",
        to: "/tetris-clock-settings",
        icon: "tetris-clock",
        iconShellClass: "device-home__tool-icon--gold",
      },
      {
        key: "planet_screensaver",
        title: "星球屏保",
        desc: "编辑星球预设、大小、方向、速度与时间层参数。",
        to: "/planet-screensaver",
        icon: "planet",
        iconShellClass: "device-home__tool-icon--slate",
      },
    ],
  },
];

export const devicePreviewRoutes = devicePageGroups.flatMap((group) => {
  return group.entries.map((entry) => {
    return {
      title: entry.title,
      to: entry.to,
      description: entry.desc,
    };
  });
});

export function resolveAppNavKey(path) {
  if (path === "/") {
    return "device_control";
  }

  for (let index = 0; index < DEVICE_CONTROL_ROUTE_PREFIXES.length; index += 1) {
    if (path.startsWith(DEVICE_CONTROL_ROUTE_PREFIXES[index])) {
      return "device_control";
    }
  }

  if (path.startsWith("/clock")) {
    return "clock";
  }

  if (path.startsWith("/animation-clock")) {
    return "animation_clock";
  }

  if (path.startsWith("/theme-clock")) {
    return "theme_clock";
  }

  if (path.startsWith("/device-params")) {
    return "device_params";
  }

  if (path.startsWith("/ble-config")) {
    return "ble";
  }

  return "";
}
