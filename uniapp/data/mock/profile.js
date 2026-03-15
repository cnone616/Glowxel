// 个人资料相关Mock数据
import { currentUser } from './users.js'
import { getAllProjects } from './projects.js'

// 我的收藏数据
export const myFavorites = [
  {
    id: 1,
    type: 'artwork', // artwork, template, challenge
    title: '赛博朋克城市',
    author: '未来创作者',
    thumbnail: '/static/images/artwork1.png',
    likes: 234,
    createdAt: '2024-01-15T10:30:00Z',
    favoriteAt: '2024-01-16T14:20:00Z'
  },
  {
    id: 2,
    type: 'template',
    title: '可爱小猫模板',
    author: '萌系画师',
    thumbnail: '/static/images/template1.png',
    downloads: 156,
    createdAt: '2024-01-10T09:15:00Z',
    favoriteAt: '2024-01-12T16:45:00Z'
  },
  {
    id: 3,
    type: 'artwork',
    title: '春日樱花',
    author: '春日画者',
    thumbnail: '/static/images/artwork2.png',
    likes: 189,
    createdAt: '2024-01-08T11:20:00Z',
    favoriteAt: '2024-01-10T13:30:00Z'
  },
  {
    id: 4,
    type: 'artwork',
    title: '美味拉面',
    author: '美食艺术家',
    thumbnail: '/static/images/artwork3.png',
    likes: 167,
    createdAt: '2024-01-05T15:45:00Z',
    favoriteAt: '2024-01-08T10:15:00Z'
  },
  {
    id: 5,
    type: 'template',
    title: '像素小屋模板',
    author: '像素艺术家小明',
    thumbnail: '/static/images/template2.png',
    downloads: 89,
    createdAt: '2024-01-03T14:30:00Z',
    favoriteAt: '2024-01-05T09:20:00Z'
  }
]

// 成就系统数据
export const achievements = [
  {
    id: 1,
    name: '初出茅庐',
    description: '完成第一个作品',
    icon: 'star',
    category: 'creation',
    unlocked: true,
    unlockedAt: '2024-01-02T10:00:00Z',
    progress: 1,
    target: 1,
    rarity: 'common', // common, rare, epic, legendary
    points: 10
  },
  {
    id: 2,
    name: '创作达人',
    description: '完成10个作品',
    icon: 'award',
    category: 'creation',
    unlocked: false,
    progress: 6,
    target: 10,
    rarity: 'rare',
    points: 50
  },
  {
    id: 3,
    name: '人气新星',
    description: '获得100个点赞',
    icon: 'heart',
    category: 'social',
    unlocked: true,
    unlockedAt: '2024-01-10T15:30:00Z',
    progress: 89,
    target: 100,
    rarity: 'rare',
    points: 30
  },
  {
    id: 4,
    name: '收藏家',
    description: '收藏20个作品',
    icon: 'bookmark',
    category: 'collection',
    unlocked: false,
    progress: 5,
    target: 20,
    rarity: 'common',
    points: 20
  },
  {
    id: 5,
    name: '社交达人',
    description: '关注50个用户',
    icon: 'users',
    category: 'social',
    unlocked: false,
    progress: 45,
    target: 50,
    rarity: 'common',
    points: 25
  },
  {
    id: 6,
    name: '像素大师',
    description: '完成100个作品',
    icon: 'crown',
    category: 'creation',
    unlocked: false,
    progress: 6,
    target: 100,
    rarity: 'legendary',
    points: 200
  },
  {
    id: 7,
    name: '连续创作者',
    description: '连续7天创作',
    icon: 'calendar',
    category: 'habit',
    unlocked: true,
    unlockedAt: '2024-01-08T20:00:00Z',
    progress: 7,
    target: 7,
    rarity: 'rare',
    points: 40
  },
  {
    id: 8,
    name: '色彩专家',
    description: '使用超过50种颜色',
    icon: 'palette',
    category: 'skill',
    unlocked: false,
    progress: 32,
    target: 50,
    rarity: 'epic',
    points: 75
  }
]

// 个人统计数据
export const profileStats = {
  // 基础统计
  totalWorks: 6,
  totalLikes: 89,
  totalViews: 456,
  totalComments: 23,
  totalShares: 12,
  
  // 关注统计
  followers: 12,
  following: 45,
  
  // 收藏统计
  favorites: 5,
  collections: 2,
  
  // 成就统计
  totalAchievements: 8,
  unlockedAchievements: 3,
  achievementPoints: 80,
  
  // 创作统计
  creationDays: 15,
  averageWorkTime: 2.5, // 小时
  mostActiveHour: 20, // 20点最活跃
  favoriteSize: '52x52',
  
  // 社交统计
  profileViews: 234,
  newFollowersThisWeek: 3,
  likesThisWeek: 15,
  
  // 设备统计
  connectedDevices: 3,
  totalSyncTime: 45, // 分钟
  lastSyncTime: '2024-01-15T18:30:00Z'
}

// 获取我的作品列表
export function getMyWorks(page = 1, limit = 10) {
  const allProjects = getAllProjects()
  const startIndex = (page - 1) * limit
  const endIndex = startIndex + limit
  
  return {
    data: allProjects.slice(startIndex, endIndex),
    total: allProjects.length,
    page,
    limit,
    hasMore: endIndex < allProjects.length
  }
}

// 获取我的收藏列表
export function getMyFavorites(type = 'all', page = 1, limit = 10) {
  let filteredFavorites = myFavorites
  
  if (type !== 'all') {
    filteredFavorites = myFavorites.filter(item => item.type === type)
  }
  
  const startIndex = (page - 1) * limit
  const endIndex = startIndex + limit
  
  return {
    data: filteredFavorites.slice(startIndex, endIndex),
    total: filteredFavorites.length,
    page,
    limit,
    hasMore: endIndex < filteredFavorites.length
  }
}

// 获取成就列表
export function getAchievements(category = 'all') {
  let filteredAchievements = achievements
  
  if (category !== 'all') {
    filteredAchievements = achievements.filter(achievement => achievement.category === category)
  }
  
  return filteredAchievements.sort((a, b) => {
    // 已解锁的排在前面，然后按进度排序
    if (a.unlocked && !b.unlocked) return -1
    if (!a.unlocked && b.unlocked) return 1
    if (a.unlocked && b.unlocked) return new Date(b.unlockedAt) - new Date(a.unlockedAt)
    return (b.progress / b.target) - (a.progress / a.target)
  })
}

// 获取成就统计
export function getAchievementStats() {
  const total = achievements.length
  const unlocked = achievements.filter(a => a.unlocked).length
  const totalPoints = achievements.reduce((sum, a) => sum + (a.unlocked ? a.points : 0), 0)
  const maxPoints = achievements.reduce((sum, a) => sum + a.points, 0)
  
  return {
    total,
    unlocked,
    progress: Math.round((unlocked / total) * 100),
    totalPoints,
    maxPoints,
    pointsProgress: Math.round((totalPoints / maxPoints) * 100)
  }
}

// 获取个人统计数据
export function getProfileStats() {
  return profileStats
}

// 添加收藏
export function addToFavorites(item) {
  const newFavorite = {
    id: myFavorites.length + 1,
    ...item,
    favoriteAt: new Date().toISOString()
  }
  
  myFavorites.unshift(newFavorite)
  profileStats.favorites += 1
  
  return {
    success: true,
    message: '收藏成功',
    data: newFavorite
  }
}

// 移除收藏
export function removeFromFavorites(id) {
  const index = myFavorites.findIndex(item => item.id === id)
  
  if (index !== -1) {
    myFavorites.splice(index, 1)
    profileStats.favorites -= 1
    
    return {
      success: true,
      message: '取消收藏成功'
    }
  }
  
  return {
    success: false,
    message: '收藏不存在'
  }
}

// 检查是否已收藏
export function isFavorited(itemId, itemType) {
  return myFavorites.some(item => 
    item.id === itemId && item.type === itemType
  )
}

// 解锁成就
export function unlockAchievement(achievementId) {
  const achievement = achievements.find(a => a.id === achievementId)
  
  if (achievement && !achievement.unlocked) {
    achievement.unlocked = true
    achievement.unlockedAt = new Date().toISOString()
    achievement.progress = achievement.target
    
    profileStats.unlockedAchievements += 1
    profileStats.achievementPoints += achievement.points
    
    return {
      success: true,
      message: `恭喜解锁成就：${achievement.name}`,
      data: achievement
    }
  }
  
  return {
    success: false,
    message: '成就已解锁或不存在'
  }
}

// 更新成就进度
export function updateAchievementProgress(achievementId, progress) {
  const achievement = achievements.find(a => a.id === achievementId)
  
  if (achievement && !achievement.unlocked) {
    achievement.progress = Math.min(progress, achievement.target)
    
    // 检查是否达到解锁条件
    if (achievement.progress >= achievement.target) {
      return unlockAchievement(achievementId)
    }
    
    return {
      success: true,
      message: '进度已更新',
      data: achievement
    }
  }
  
  return {
    success: false,
    message: '成就不存在或已解锁'
  }
}

// 获取当前用户信息
export function getCurrentUser() {
  return {
    ...currentUser,
    stats: profileStats
  }
}

// 更新用户信息
export function updateUserProfile(updates) {
  Object.assign(currentUser, updates)
  
  return {
    success: true,
    message: '资料更新成功',
    data: currentUser
  }
}