// 用户数据Mock - 用户信息和社区用户
export const mockUsers = [
  {
    id: 1,
    name: "像素艺术家小明",
    avatar: "/static/images/avatar1.png",
    bio: "专注像素艺术创作5年，喜欢用像素表达生活中的美好",
    level: 15,
    followersCount: 1250,
    followingCount: 89,
    worksCount: 45,
    totalLikes: 3420,
    joinTime: "2023-03-15T10:00:00Z",
    badges: ["创作达人", "人气作者", "精品推荐"],
    isFollowing: false,
    status: "active"
  },
  {
    id: 2,
    name: "设计师小红",
    avatar: "/static/images/avatar2.png",
    bio: "UI设计师，业余时间创作像素艺术",
    level: 12,
    followersCount: 890,
    followingCount: 156,
    worksCount: 32,
    totalLikes: 2180,
    joinTime: "2023-06-20T14:30:00Z",
    badges: ["设计新星", "色彩大师"],
    isFollowing: true,
    status: "active"
  },
  {
    id: 3,
    name: "未来创作者",
    avatar: "/static/images/avatar3.png",
    bio: "赛博朋克风格像素艺术爱好者",
    level: 18,
    followersCount: 2340,
    followingCount: 67,
    worksCount: 78,
    totalLikes: 5670,
    joinTime: "2022-11-08T09:15:00Z",
    badges: ["科幻大师", "创作达人", "人气作者", "月度之星"],
    isFollowing: false,
    status: "active"
  },
  {
    id: 4,
    name: "萌系画师",
    avatar: "/static/images/avatar4.png",
    bio: "专门创作可爱风格的像素艺术",
    level: 10,
    followersCount: 567,
    followingCount: 234,
    worksCount: 28,
    totalLikes: 1890,
    joinTime: "2023-09-12T16:45:00Z",
    badges: ["萌系专家", "新人推荐"],
    isFollowing: true,
    status: "active"
  },
  {
    id: 5,
    name: "春日画者",
    avatar: "/static/images/avatar5.png",
    bio: "热爱自然，用像素记录四季之美",
    level: 14,
    followersCount: 1120,
    followingCount: 98,
    worksCount: 56,
    totalLikes: 2890,
    joinTime: "2023-01-25T11:30:00Z",
    badges: ["自然之友", "创作达人"],
    isFollowing: false,
    status: "active"
  },
  {
    id: 6,
    name: "美食艺术家",
    avatar: "/static/images/avatar6.png",
    bio: "用像素艺术展现美食的诱人魅力",
    level: 11,
    followersCount: 678,
    followingCount: 145,
    worksCount: 34,
    totalLikes: 2100,
    joinTime: "2023-07-30T13:20:00Z",
    badges: ["美食达人", "创意新星"],
    isFollowing: true,
    status: "active"
  }
]

// 当前用户信息
export const currentUser = {
  id: 999,
  name: "我的昵称",
  avatar: "/static/images/my_avatar.png",
  bio: "像素艺术爱好者，正在学习中...",
  level: 3,
  followersCount: 12,
  followingCount: 45,
  worksCount: 6,
  totalLikes: 89,
  joinTime: "2024-01-01T00:00:00Z",
  badges: ["新手上路"],
  status: "active",
  // 用户设置
  settings: {
    autoSync: true,
    notifications: true,
    privacy: "public", // public, friends, private
    language: "zh-CN"
  },
  // 统计数据
  stats: {
    totalProjects: 6,
    completedProjects: 2,
    totalWorkingHours: 24,
    favoriteColors: ["#4F7FFF", "#FF7A45", "#2ECC71"],
    mostUsedSize: "52x52"
  }
}

// 获取所有用户
export function getAllUsers() {
  return mockUsers
}

// 根据ID获取用户
export function getUserById(id) {
  if (id === 999) return currentUser
  return mockUsers.find(user => user.id === id)
}

// 获取热门用户（按粉丝数排序）
export function getPopularUsers(limit = 6) {
  return mockUsers
    .sort((a, b) => b.followersCount - a.followersCount)
    .slice(0, limit)
}

// 获取推荐用户
export function getRecommendedUsers(limit = 4) {
  return mockUsers
    .filter(user => !user.isFollowing)
    .sort((a, b) => b.level - a.level)
    .slice(0, limit)
}

// 搜索用户
export function searchUsers(keyword) {
  return mockUsers.filter(user => 
    user.name.includes(keyword) || 
    user.bio.includes(keyword)
  )
}

// 关注/取消关注用户
export function toggleFollowUser(userId) {
  const user = getUserById(userId)
  if (user && user.id !== 999) {
    user.isFollowing = !user.isFollowing
    if (user.isFollowing) {
      user.followersCount += 1
      currentUser.followingCount += 1
    } else {
      user.followersCount -= 1
      currentUser.followingCount -= 1
    }
    return { 
      success: true, 
      isFollowing: user.isFollowing,
      message: user.isFollowing ? "关注成功" : "取消关注成功"
    }
  }
  return { success: false, message: "操作失败" }
}

// 获取用户的关注列表
export function getUserFollowing(userId, page = 1, limit = 20) {
  // 模拟分页数据
  const startIndex = (page - 1) * limit
  const endIndex = startIndex + limit
  
  // 返回部分用户作为关注列表
  return mockUsers
    .filter(user => user.id !== userId)
    .slice(startIndex, endIndex)
}

// 获取用户的粉丝列表
export function getUserFollowers(userId, page = 1, limit = 20) {
  // 模拟分页数据
  const startIndex = (page - 1) * limit
  const endIndex = startIndex + limit
  
  // 返回部分用户作为粉丝列表
  return mockUsers
    .filter(user => user.id !== userId)
    .slice(startIndex, endIndex)
}