// 模板数据Mock - 预设模板
export const mockTemplates = [
  {
    id: 1,
    name: "经典马里奥",
    image: "/static/images/template1.png",
    category: "游戏角色",
    size: "16x16",
    colorCount: 4,
    difficulty: "简单",
    usageCount: 1250,
    description: "经典的马里奥像素形象",
    tags: ["游戏", "经典", "简单"]
  },
  {
    id: 2,
    name: "可爱皮卡丘",
    image: "/static/images/template2.png",
    category: "动漫角色",
    size: "24x24",
    colorCount: 6,
    difficulty: "简单",
    usageCount: 980,
    description: "人气动漫角色皮卡丘",
    tags: ["动漫", "可爱", "宠物小精灵"]
  },
  {
    id: 3,
    name: "像素爱心",
    image: "/static/images/template3.png",
    category: "图案符号",
    size: "12x12",
    colorCount: 2,
    difficulty: "简单",
    usageCount: 2100,
    description: "简单的爱心图案",
    tags: ["爱心", "简单", "符号"]
  },
  {
    id: 4,
    name: "彩虹独角兽",
    image: "/static/images/template4.png",
    category: "奇幻生物",
    size: "32x32",
    colorCount: 12,
    difficulty: "中等",
    usageCount: 756,
    description: "梦幻的彩虹独角兽",
    tags: ["独角兽", "彩虹", "梦幻"]
  },
  {
    id: 5,
    name: "像素花朵",
    image: "/static/images/template5.png",
    category: "自然植物",
    size: "20x20",
    colorCount: 8,
    difficulty: "简单",
    usageCount: 1340,
    description: "美丽的像素花朵",
    tags: ["花朵", "自然", "美丽"]
  },
  {
    id: 6,
    name: "太空飞船",
    image: "/static/images/template6.png",
    category: "科幻载具",
    size: "28x28",
    colorCount: 10,
    difficulty: "中等",
    usageCount: 623,
    description: "未来科幻风格飞船",
    tags: ["太空", "科幻", "飞船"]
  },
  {
    id: 7,
    name: "像素汉堡",
    image: "/static/images/template7.png",
    category: "美食",
    size: "24x24",
    colorCount: 8,
    difficulty: "简单",
    usageCount: 890,
    description: "诱人的像素汉堡",
    tags: ["汉堡", "美食", "可爱"]
  },
  {
    id: 8,
    name: "城堡建筑",
    image: "/static/images/template8.png",
    category: "建筑",
    size: "52x52",
    colorCount: 15,
    difficulty: "困难",
    usageCount: 445,
    description: "中世纪风格城堡",
    tags: ["城堡", "建筑", "中世纪"]
  }
]

// 模板分类
export const templateCategories = [
  { id: "all", name: "全部", count: mockTemplates.length },
  { id: "game", name: "游戏角色", count: mockTemplates.filter(t => t.category === "游戏角色").length },
  { id: "anime", name: "动漫角色", count: mockTemplates.filter(t => t.category === "动漫角色").length },
  { id: "symbol", name: "图案符号", count: mockTemplates.filter(t => t.category === "图案符号").length },
  { id: "fantasy", name: "奇幻生物", count: mockTemplates.filter(t => t.category === "奇幻生物").length },
  { id: "nature", name: "自然植物", count: mockTemplates.filter(t => t.category === "自然植物").length },
  { id: "scifi", name: "科幻载具", count: mockTemplates.filter(t => t.category === "科幻载具").length },
  { id: "food", name: "美食", count: mockTemplates.filter(t => t.category === "美食").length },
  { id: "building", name: "建筑", count: mockTemplates.filter(t => t.category === "建筑").length }
]

// 获取所有模板
export function getAllTemplates() {
  return mockTemplates
}

// 根据分类获取模板
export function getTemplatesByCategory(category) {
  if (category === "all") return mockTemplates
  return mockTemplates.filter(template => template.category === category)
}

// 获取热门模板（按使用次数排序）
export function getPopularTemplates(limit = 6) {
  return mockTemplates
    .sort((a, b) => b.usageCount - a.usageCount)
    .slice(0, limit)
}

// 根据难度获取模板
export function getTemplatesByDifficulty(difficulty) {
  return mockTemplates.filter(template => template.difficulty === difficulty)
}

// 搜索模板
export function searchTemplates(keyword) {
  return mockTemplates.filter(template => 
    template.name.includes(keyword) || 
    template.tags.some(tag => tag.includes(keyword))
  )
}