// Mock数据管理器 - 统一管理所有Mock数据
import * as Projects from './projects.js'
import * as Artworks from './artworks.js'
import * as Templates from './templates.js'
import * as Devices from './devices.js'
import * as Users from './users.js'
import * as Challenges from './challenges.js'

// 导出所有Mock数据模块
export {
  Projects,
  Artworks,
  Templates,
  Devices,
  Users,
  Challenges
}

// 统一的Mock API接口
export const MockAPI = {
  // 项目相关
  projects: {
    getAll: Projects.getAllProjects,
    getById: Projects.getProjectById,
    getByStatus: Projects.getProjectsByStatus,
    getRecent: Projects.getRecentProjects
  },
  
  // 作品相关
  artworks: {
    getAll: Artworks.getAllArtworks,
    getById: Artworks.getArtworkById,
    getByUserId: Artworks.getArtworksByUserId,
    getPopular: Artworks.getPopularArtworks,
    getLatest: Artworks.getLatestArtworks,
    getRelated: Artworks.getRelatedArtworks,
    searchByTag: Artworks.searchArtworksByTag
  },
  
  // 评论相关
  comments: {
    getByArtworkId: Artworks.getCommentsByArtworkId,
    add: Artworks.addComment,
    like: Artworks.likeComment,
    unlike: Artworks.likeComment, // 同一个函数，切换状态
    delete: Artworks.deleteComment
  },
  
  // 点赞相关
  likes: {
    isLiked: (artworkId) => Promise.resolve(Math.random() > 0.5), // 随机返回是否已点赞
    like: (artworkId) => Promise.resolve({ success: true }),
    unlike: (artworkId) => Promise.resolve({ success: true })
  },
  
  // 收藏相关
  collections: {
    isCollected: (artworkId) => Promise.resolve(Math.random() > 0.7), // 随机返回是否已收藏
    collect: (artworkId) => Promise.resolve({ success: true }),
    uncollect: (artworkId) => Promise.resolve({ success: true })
  },
  
  // 关注相关
  follows: {
    follow: (userId) => Promise.resolve({ success: true }),
    unfollow: (userId) => Promise.resolve({ success: true })
  },
  
  // 模板相关
  templates: {
    getAll: Templates.getAllTemplates,
    getByCategory: Templates.getTemplatesByCategory,
    getPopular: Templates.getPopularTemplates,
    getByDifficulty: Templates.getTemplatesByDifficulty,
    search: Templates.searchTemplates,
    getCategories: () => Templates.templateCategories
  },
  
  // 设备相关
  devices: {
    getAll: Devices.getAllDevices,
    getById: Devices.getDeviceById,
    getByStatus: Devices.getDevicesByStatus,
    getConnected: Devices.getConnectedDevices,
    getLowBattery: Devices.getLowBatteryDevices,
    getByConnectionType: Devices.getDevicesByConnectionType,
    connect: Devices.connectDevice,
    disconnect: Devices.disconnectDevice,
    getStats: () => Devices.deviceStats
  },
  
  // 用户相关
  users: {
    getAll: Users.getAllUsers,
    getById: Users.getUserById,
    getPopular: Users.getPopularUsers,
    getRecommended: Users.getRecommendedUsers,
    getFollowing: Users.getUserFollowing,
    getFollowers: Users.getUserFollowers,
    search: Users.searchUsers,
    toggleFollow: Users.toggleFollowUser,
    getCurrent: () => Users.currentUser
  },
  
  // 挑战相关
  challenges: {
    getAll: Challenges.getAllChallenges,
    getById: Challenges.getChallengeById,
    getByStatus: Challenges.getChallengesByStatus,
    getActive: Challenges.getActiveChallenges,
    getUpcoming: Challenges.getUpcomingChallenges,
    getPopular: Challenges.getPopularChallenges,
    join: Challenges.joinChallenge,
    submit: Challenges.submitToChallenge,
    getStats: () => Challenges.challengeStats
  }
}

// 模拟异步API调用
export function createMockAPI(data, delay = 300) {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve({
        success: true,
        data: data,
        message: "请求成功"
      })
    }, delay)
  })
}

// 模拟API错误
export function createMockError(message = "请求失败", delay = 300) {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve({
        success: false,
        data: null,
        message: message
      })
    }, delay)
  })
}

// 通用Mock API包装器
export function wrapMockAPI(fn, errorRate = 0) {
  return async (...args) => {
    // 模拟网络延迟
    await new Promise(resolve => setTimeout(resolve, Math.random() * 500 + 200))
    
    // 模拟错误率
    if (Math.random() < errorRate) {
      return createMockError("网络请求失败")
    }
    
    try {
      const result = fn(...args)
      return createMockAPI(result)
    } catch (error) {
      return createMockError(error.message)
    }
  }
}

// 导出包装后的API
export const AsyncMockAPI = {
  projects: {
    getAll: wrapMockAPI(Projects.getAllProjects),
    getById: wrapMockAPI(Projects.getProjectById),
    getByStatus: wrapMockAPI(Projects.getProjectsByStatus),
    getRecent: wrapMockAPI(Projects.getRecentProjects)
  },
  
  artworks: {
    getAll: wrapMockAPI(Artworks.getAllArtworks),
    getById: wrapMockAPI(Artworks.getArtworkById),
    getPopular: wrapMockAPI(Artworks.getPopularArtworks),
    getLatest: wrapMockAPI(Artworks.getLatestArtworks),
    searchByTag: wrapMockAPI(Artworks.searchArtworksByTag)
  },
  
  templates: {
    getAll: wrapMockAPI(Templates.getAllTemplates),
    getByCategory: wrapMockAPI(Templates.getTemplatesByCategory),
    getPopular: wrapMockAPI(Templates.getPopularTemplates)
  },
  
  devices: {
    getAll: wrapMockAPI(Devices.getAllDevices),
    getConnected: wrapMockAPI(Devices.getConnectedDevices),
    connect: wrapMockAPI(Devices.connectDevice),
    disconnect: wrapMockAPI(Devices.disconnectDevice)
  },
  
  users: {
    getPopular: wrapMockAPI(Users.getPopularUsers),
    getRecommended: wrapMockAPI(Users.getRecommendedUsers),
    toggleFollow: wrapMockAPI(Users.toggleFollowUser)
  },
  
  challenges: {
    getActive: wrapMockAPI(Challenges.getActiveChallenges),
    getPopular: wrapMockAPI(Challenges.getPopularChallenges),
    join: wrapMockAPI(Challenges.joinChallenge)
  }
}