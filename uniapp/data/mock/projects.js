// 项目数据Mock - 像素艺术项目
export const mockProjects = [
  {
    id: 1,
    name: "彩虹像素画",
    icon: "picture",
    progress: 85,
    createTime: "2024-01-15",
    updateTime: "2024-01-20",
    thumbnail: "/static/images/project1.png",
    size: "52x52",
    colorCount: 12,
    status: "in_progress", // in_progress, completed, draft
    description: "一幅充满活力的彩虹主题像素艺术作品"
  },
  {
    id: 2,
    name: "像素小猫",
    icon: "picture",
    progress: 100,
    createTime: "2024-01-10",
    updateTime: "2024-01-18",
    thumbnail: "/static/images/project2.png",
    size: "32x32",
    colorCount: 8,
    status: "completed",
    description: "可爱的像素风格小猫咪"
  },
  {
    id: 3,
    name: "星空夜景",
    icon: "picture",
    progress: 45,
    createTime: "2024-01-22",
    updateTime: "2024-01-25",
    thumbnail: "/static/images/project3.png",
    size: "64x64",
    colorCount: 15,
    status: "in_progress",
    description: "浪漫的星空主题像素艺术"
  },
  {
    id: 4,
    name: "像素花园",
    icon: "picture",
    progress: 20,
    createTime: "2024-01-28",
    updateTime: "2024-01-30",
    thumbnail: "/static/images/project4.png",
    size: "52x52",
    colorCount: 18,
    status: "draft",
    description: "春天花园的像素艺术表现"
  },
  {
    id: 5,
    name: "复古游戏角色",
    icon: "picture",
    progress: 75,
    createTime: "2024-02-01",
    updateTime: "2024-02-05",
    thumbnail: "/static/images/project5.png",
    size: "16x16",
    colorCount: 6,
    status: "in_progress",
    description: "经典8位游戏风格的角色设计"
  },
  {
    id: 6,
    name: "现代建筑",
    icon: "picture",
    progress: 60,
    createTime: "2024-02-08",
    updateTime: "2024-02-12",
    thumbnail: "/static/images/project6.png",
    size: "52x52",
    colorCount: 10,
    status: "in_progress",
    description: "现代城市建筑的像素化表现"
  }
]

// 获取所有项目
export function getAllProjects() {
  return mockProjects
}

// 根据ID获取项目
export function getProjectById(id) {
  return mockProjects.find(project => project.id === id)
}

// 根据状态筛选项目
export function getProjectsByStatus(status) {
  return mockProjects.filter(project => project.status === status)
}

// 获取最近项目
export function getRecentProjects(limit = 3) {
  return mockProjects
    .sort((a, b) => new Date(b.updateTime) - new Date(a.updateTime))
    .slice(0, limit)
}