export const HOME_SECTIONS = [
  {
    title: "推荐灵感",
    description: "今日适合开工的像素主题和氛围方向。",
    entries: [
      { title: "霓光海岸", meta: "12 个场景", tone: "mint" },
      { title: "低饱和宇宙", meta: "8 个模板", tone: "blue" },
      { title: "清晨备忘", meta: "5 个看板", tone: "amber" },
    ],
  },
  {
    title: "最近动态",
    description: "社区和设备线最近最值得继续推进的内容。",
    entries: [
      { title: "屏保系列重构", meta: "已同步骨架", tone: "blue" },
      { title: "社区筛选收口", meta: "待迁移", tone: "mint" },
    ],
  },
];

export const COMMUNITY_WORKS = [
  { id: "cw-01", title: "冰川电台", author: "Aurora", stats: "368 喜欢" },
  { id: "cw-02", title: "月面车站", author: "Mori", stats: "241 喜欢" },
  { id: "cw-03", title: "薄荷记事簿", author: "Cloudy", stats: "125 喜欢" },
  { id: "cw-04", title: "深海信号", author: "Kite", stats: "92 喜欢" },
];

export const CREATOR_LIST = [
  { id: "cu-01", name: "Aurora", tag: "动效创作者", stats: "42 作品" },
  { id: "cu-02", name: "Mori", tag: "像素场景", stats: "28 作品" },
  { id: "cu-03", name: "Cloudy", tag: "看板设计", stats: "15 作品" },
];

export const PROJECT_ENTRIES = [
  { id: "pw-01", title: "像素宇宙待发布", description: "64x64 场景集", status: "待整理" },
  { id: "pw-02", title: "设备欢迎页", description: "首屏开机动画", status: "草稿" },
  { id: "pw-03", title: "海报实验稿", description: "颜色校对中", status: "进行中" },
];

export const BOARD_ENTRIES = [
  { id: "A1", title: "A1 板", description: "封面区域", progress: "12 / 16" },
  { id: "A2", title: "A2 板", description: "标题区域", progress: "10 / 16" },
  { id: "B1", title: "B1 板", description: "动效区域", progress: "8 / 16" },
  { id: "B2", title: "B2 板", description: "收尾区域", progress: "16 / 16" },
];

export const DEVICE_MODES = [
  { key: "clock", title: "时钟模式", description: "静态 / 动态 / 主题模式", tone: "mint" },
  { key: "scene", title: "像素场景集", description: "氛围屏保与图案组合", tone: "blue" },
  { key: "gif", title: "GIF 播放器", description: "动画素材下发与回放", tone: "amber" },
];

export const PROFILE_ENTRIES = [
  { key: "works", title: "我的作品", description: "查看作品与草稿", tone: "blue" },
  { key: "favorites", title: "我的收藏", description: "集中管理喜欢的内容", tone: "mint" },
  { key: "settings", title: "设置与同步", description: "账号、同步与主题", tone: "amber" },
];
