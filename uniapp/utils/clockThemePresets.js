const CLOCK_THEME_PRESETS = [
  {
    id: "glowxel_minimal_digital",
    name: "Glowxel 极简数字",
    description: "保留大时间主体，弱化装饰信息，适合常亮桌面和低干扰展示。",
    accentColor: "#7bf2d0",
    styleTag: "极简数字",
    previewImage: "",
    requiresImage: false,
    config: {
      font: "lcd_6x8",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 18,
        color: "#7bf2d0",
        align: "center",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 36,
        color: "#d2fff3",
        align: "center",
      },
      week: {
        show: true,
        x: 32,
        y: 46,
        color: "#4cc7a5",
        align: "center",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "glowxel_frame_clock",
    name: "Glowxel 边框时钟",
    description: "用整屏像素边框包住时间区域，画面更完整，适合做常驻主题。",
    accentColor: "#ffcf5a",
    styleTag: "边框时钟",
    previewImage: "",
    requiresImage: false,
    config: {
      font: "classic_5x7",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 20,
        color: "#ffcf5a",
        align: "center",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 36,
        color: "#fff5cb",
        align: "center",
      },
      week: {
        show: true,
        x: 32,
        y: 46,
        color: "#ff9f43",
        align: "center",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "glowxel_centerpiece_badge",
    name: "Glowxel 中央主视觉",
    description: "中间保留一块主视觉徽章，时间收在角标位，适合主题化展示。",
    accentColor: "#ff7f96",
    styleTag: "主视觉角标",
    previewImage: "",
    requiresImage: false,
    config: {
      font: "rounded_4x6",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 58,
        y: 6,
        color: "#ffe0e7",
        align: "right",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 58,
        y: 16,
        color: "#ff9daf",
        align: "right",
      },
      week: {
        show: true,
        x: 58,
        y: 24,
        color: "#ffd15f",
        align: "right",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "glowxel_topbar_motion",
    name: "Glowxel 顶栏波形",
    description: "把时间压到顶部横栏，底部用波形和状态点补氛围，适合科技感主题。",
    accentColor: "#5ecbff",
    styleTag: "顶栏动感",
    previewImage: "",
    requiresImage: false,
    config: {
      font: "minimal_3x5",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 2,
        x: 4,
        y: 5,
        color: "#5ecbff",
        align: "left",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 4,
        y: 46,
        color: "#d8f5ff",
        align: "left",
      },
      week: {
        show: true,
        x: 60,
        y: 46,
        color: "#7ff0ff",
        align: "right",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "glowxel_festival_poster",
    name: "Glowxel 节日海报",
    description: "偏节庆海报式的整屏排版，强调氛围图形和暖色块组合。",
    accentColor: "#ff6c4d",
    styleTag: "节日海报",
    previewImage: "",
    requiresImage: false,
    config: {
      font: "retro_5x7",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 18,
        color: "#fff0d6",
        align: "center",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 36,
        color: "#ffd059",
        align: "center",
      },
      week: {
        show: true,
        x: 32,
        y: 46,
        color: "#ff9e63",
        align: "center",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "glowxel_pixel_arcade",
    name: "Glowxel 像素游戏",
    description: "借游戏 HUD 的信息排布做成整屏主题，适合偏活跃的像素展示。",
    accentColor: "#8dff6a",
    styleTag: "像素游戏",
    previewImage: "",
    requiresImage: false,
    config: {
      font: "seven_seg_5x7",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 18,
        color: "#f3fff0",
        align: "center",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 6,
        y: 40,
        color: "#8dff6a",
        align: "left",
      },
      week: {
        show: true,
        x: 58,
        y: 40,
        color: "#5ed2ff",
        align: "right",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "glowxel_fireworks_bloom",
    name: "Glowxel 烟花绽放",
    description: "引用已入库烟花素材做成节庆主题，适合做气氛更强的展示页面。",
    accentColor: "#ffb347",
    styleTag: "素材烟花",
    previewImage: "",
    requiresImage: false,
    config: {
      font: "retro_5x7",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 12,
        color: "#fff6d2",
        align: "center",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 49,
        color: "#ffcf6a",
        align: "center",
      },
      week: {
        show: true,
        x: 32,
        y: 57,
        color: "#ff8f66",
        align: "center",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "glowxel_fireworks_rise",
    name: "Glowxel 烟花升空",
    description: "引用已入库烟花升空素材，适合搭配夜空底色和角标式时间布局。",
    accentColor: "#7ec8ff",
    styleTag: "素材夜空",
    previewImage: "",
    requiresImage: false,
    config: {
      font: "rounded_4x6",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 58,
        y: 6,
        color: "#f2fbff",
        align: "right",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 58,
        y: 15,
        color: "#9cd7ff",
        align: "right",
      },
      week: {
        show: true,
        x: 58,
        y: 23,
        color: "#6bc0ff",
        align: "right",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "clockwise_mario",
    name: "马里奥时钟",
    description: "完整的马里奥像素场景，时间嵌在地面砖块和角色场景里。",
    accentColor: "#ff4e45",
    styleTag: "场景时钟",
    previewImage: "/static/clockwise-themes/cw-cf-0x01-thumb.jpg",
    requiresImage: false,
    config: {
      font: "classic_5x7",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 14,
        color: "#ff4e45",
        align: "center",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 28,
        color: "#ffd54a",
        align: "center",
      },
      week: {
        show: true,
        x: 32,
        y: 40,
        color: "#4fc3ff",
        align: "center",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "clockwise_words",
    name: "文字时间主题",
    description: "用整句英文表达当前时间，日期和状态信息放在底部信息带中。",
    accentColor: "#f3f3f3",
    styleTag: "文字时间",
    previewImage: "/static/clockwise-themes/cw-cf-0x02-thumb.jpg",
    requiresImage: false,
    config: {
      font: "minimal_3x5",
      showSeconds: false,
      hourFormat: 12,
      time: {
        show: true,
        fontSize: 2,
        x: 32,
        y: 10,
        color: "#f3f3f3",
        align: "center",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 28,
        color: "#c8c8c8",
        align: "center",
      },
      week: {
        show: true,
        x: 32,
        y: 37,
        color: "#8a8a8a",
        align: "center",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "clockwise_worldmap",
    name: "地图时钟",
    description: "以世界地图为主视觉，时间信息贴在地图层上，整体偏航图仪表盘风格。",
    accentColor: "#4fd2a3",
    styleTag: "地图时钟",
    previewImage: "/static/clockwise-themes/cw-cf-0x03-thumb.jpg",
    requiresImage: false,
    config: {
      font: "minimal_3x5",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 2,
        x: 60,
        y: 45,
        color: "#4fd2a3",
        align: "right",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 4,
        y: 6,
        color: "#dffcf2",
        align: "left",
      },
      week: {
        show: true,
        x: 4,
        y: 14,
        color: "#8fd8bf",
        align: "left",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "clockwise_castlevania",
    name: "针塔时钟主题",
    description: "钟塔场景配合中心表针，整体是暗夜哥特氛围。",
    accentColor: "#d94fff",
    styleTag: "钟塔指针",
    previewImage: "/static/clockwise-themes/cw-cf-0x04-thumb.jpg",
    requiresImage: false,
    config: {
      font: "hollow_5x7",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 13,
        color: "#d94fff",
        align: "center",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 32,
        y: 27,
        color: "#ff6b7c",
        align: "center",
      },
      week: {
        show: true,
        x: 32,
        y: 39,
        color: "#7a5cff",
        align: "center",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "clockwise_pacman",
    name: "吃豆人主题",
    description: "可动的迷宫场景，时间信息和吃豆人动效融合在同一个画面里。",
    accentColor: "#ffd400",
    styleTag: "街机迷宫",
    previewImage: "/static/clockwise-themes/cw-cf-0x05-thumb.jpg",
    requiresImage: false,
    config: {
      font: "seven_seg_5x7",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 58,
        y: 8,
        color: "#ffd400",
        align: "right",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 58,
        y: 22,
        color: "#ff6464",
        align: "right",
      },
      week: {
        show: true,
        x: 58,
        y: 34,
        color: "#64c8ff",
        align: "right",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
  {
    id: "clockwise_pokedex",
    name: "图鉴主题",
    description: "图鉴机身界面，包含精灵窗口、时间区、星期格和加载条。",
    accentColor: "#ff5c5c",
    styleTag: "图鉴面板",
    previewImage: "/static/clockwise-themes/cw-cf-0x06-thumb.jpg",
    requiresImage: false,
    config: {
      font: "lcd_6x8",
      showSeconds: false,
      hourFormat: 24,
      time: {
        show: true,
        fontSize: 1,
        x: 6,
        y: 8,
        color: "#ff5c5c",
        align: "left",
      },
      date: {
        show: true,
        fontSize: 1,
        x: 6,
        y: 23,
        color: "#ffffff",
        align: "left",
      },
      week: {
        show: true,
        x: 6,
        y: 34,
        color: "#9a9a9a",
        align: "left",
      },
      image: {
        show: false,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        data: null,
      },
    },
  },
];

function isSameSection(currentSection, presetSection) {
  const keys = Object.keys(presetSection);
  for (let i = 0; i < keys.length; i++) {
    const key = keys[i];
    if (currentSection[key] !== presetSection[key]) {
      return false;
    }
  }
  return true;
}

export function getClockThemePresets() {
  return CLOCK_THEME_PRESETS.filter((preset) => preset.id.startsWith("clockwise_"));
}

export function findClockThemePreset(themeId) {
  return CLOCK_THEME_PRESETS.find((preset) => preset.id === themeId) || null;
}

export function applyClockThemePreset(currentConfig, themeId) {
  const preset = findClockThemePreset(themeId);
  if (!preset) {
    return currentConfig;
  }

  return {
    ...currentConfig,
    font: preset.config.font,
    showSeconds: preset.config.showSeconds,
    hourFormat: preset.config.hourFormat,
    time: {
      ...currentConfig.time,
      ...preset.config.time,
    },
    date: {
      ...currentConfig.date,
      ...preset.config.date,
    },
    week: {
      ...currentConfig.week,
      ...preset.config.week,
    },
    image: {
      ...currentConfig.image,
      ...(preset.config.image || {}),
    },
  };
}

export function getMatchingClockThemeId(currentConfig) {
  for (let i = 0; i < CLOCK_THEME_PRESETS.length; i++) {
    const preset = CLOCK_THEME_PRESETS[i];
    if (currentConfig.font !== preset.config.font) {
      continue;
    }
    if (currentConfig.showSeconds !== preset.config.showSeconds) {
      continue;
    }
    if (currentConfig.hourFormat !== preset.config.hourFormat) {
      continue;
    }
    if (!isSameSection(currentConfig.time, preset.config.time)) {
      continue;
    }
    if (!isSameSection(currentConfig.date, preset.config.date)) {
      continue;
    }
    if (!isSameSection(currentConfig.week, preset.config.week)) {
      continue;
    }
    return preset.id;
  }

  return "";
}
