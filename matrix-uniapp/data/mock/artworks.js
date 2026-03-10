// 作品数据Mock - 社区作品展示
export const mockArtworks = [
  {
    id: 1,
    title: "梦幻独角兽",
    coverUrl: "/static/images/artwork1.png",
    previewUrl: "/static/images/artwork1_preview.png",
    author: {
      id: 1,
      name: "像素艺术家小明",
      avatar: "/static/images/avatar1.png",
      worksCount: 25,
      isFollowing: false
    },
    likes: 128,
    comments: 23,
    views: 456,
    collects: 67,
    tags: ["独角兽", "梦幻", "彩虹"],
    createdAt: "2024-01-15T10:30:00Z",
    description: "一只在彩虹中飞舞的梦幻独角兽",
    width: 52,
    height: 52,
    colorCount: 16,
    boardCount: 1,
    difficulty: "中等",
    status: "published"
  },
  {
    id: 2,
    title: "像素咖啡杯",
    coverUrl: "/static/images/artwork2.png",
    previewUrl: "/static/images/artwork2_preview.png",
    author: {
      id: 2,
      name: "设计师小红",
      avatar: "/static/images/avatar2.png",
      worksCount: 18,
      isFollowing: true
    },
    likes: 89,
    comments: 15,
    views: 234,
    collects: 34,
    tags: ["咖啡", "生活", "温馨"],
    createdAt: "2024-01-18T14:20:00Z",
    description: "温暖的咖啡时光像素艺术",
    width: 32,
    height: 32,
    colorCount: 8,
    boardCount: 1,
    difficulty: "简单",
    status: "published"
  },
  {
    id: 3,
    title: "赛博朋克城市",
    coverUrl: "/static/images/artwork3.png",
    previewUrl: "/static/images/artwork3_preview.png",
    author: {
      id: 3,
      name: "未来创作者",
      avatar: "/static/images/avatar3.png",
      worksCount: 42,
      isFollowing: false
    },
    likes: 256,
    comments: 45,
    views: 789,
    collects: 123,
    tags: ["赛博朋克", "城市", "科幻"],
    createdAt: "2024-01-20T09:15:00Z",
    description: "霓虹闪烁的未来城市景象",
    width: 64,
    height: 64,
    colorCount: 24,
    boardCount: 4,
    difficulty: "困难",
    status: "published"
  },
  {
    id: 4,
    title: "可爱小恐龙",
    coverUrl: "/static/images/artwork4.png",
    previewUrl: "/static/images/artwork4_preview.png",
    author: {
      id: 4,
      name: "萌系画师",
      avatar: "/static/images/avatar4.png",
      worksCount: 33,
      isFollowing: true
    },
    likes: 167,
    comments: 32,
    views: 345,
    collects: 78,
    tags: ["恐龙", "可爱", "卡通"],
    createdAt: "2024-01-22T16:45:00Z",
    description: "Q版小恐龙的像素艺术",
    width: 24,
    height: 24,
    colorCount: 6,
    boardCount: 1,
    difficulty: "简单",
    status: "published"
  },
  {
    id: 5,
    title: "樱花飞舞",
    coverUrl: "/static/images/artwork5.png",
    previewUrl: "/static/images/artwork5_preview.png",
    author: {
      id: 5,
      name: "春日画者",
      avatar: "/static/images/avatar5.png",
      worksCount: 29,
      isFollowing: false
    },
    likes: 203,
    comments: 28,
    views: 567,
    collects: 89,
    tags: ["樱花", "春天", "浪漫"],
    createdAt: "2024-01-25T11:30:00Z",
    description: "春日樱花飞舞的美丽瞬间",
    width: 52,
    height: 52,
    colorCount: 12,
    boardCount: 1,
    difficulty: "中等",
    status: "published"
  },
  {
    id: 6,
    title: "像素汉堡",
    coverUrl: "/static/images/artwork6.png",
    previewUrl: "/static/images/artwork6_preview.png",
    author: {
      id: 6,
      name: "美食艺术家",
      avatar: "/static/images/avatar6.png",
      worksCount: 15,
      isFollowing: false
    },
    likes: 94,
    comments: 18,
    views: 278,
    collects: 45,
    tags: ["汉堡", "美食", "诱人"],
    createdAt: "2024-01-28T13:20:00Z",
    description: "让人垂涎欲滴的像素汉堡",
    width: 32,
    height: 32,
    colorCount: 10,
    boardCount: 1,
    difficulty: "简单",
    status: "published"
  }
]

// 评论数据Mock
export const mockComments = [
  {
    id: 1,
    artworkId: 1,
    author: {
      id: 7,
      name: "评论者A",
      avatar: "/static/images/avatar7.png"
    },
    content: "太棒了！独角兽的颜色搭配很棒",
    likes: 12,
    isLiked: false,
    createdAt: "2024-01-16T10:30:00Z",
    replies: []
  },
  {
    id: 2,
    artworkId: 1,
    author: {
      id: 8,
      name: "评论者B",
      avatar: "/static/images/avatar8.png"
    },
    content: "请问用了多少种颜色？",
    likes: 5,
    isLiked: true,
    createdAt: "2024-01-16T14:20:00Z",
    replies: [
      {
        id: 21,
        author: {
          id: 1,
          name: "像素艺术家小明",
          avatar: "/static/images/avatar1.png"
        },
        content: "一共用了16种颜色",
        createdAt: "2024-01-16T15:00:00Z"
      }
    ]
  }
]

// 获取所有作品
export function getAllArtworks() {
  return mockArtworks
}

// 根据ID获取作品
export function getArtworkById(id) {
  const artwork = mockArtworks.find(artwork => artwork.id == id)
  if (!artwork) return null
  
  // 返回完整的作品信息
  return {
    ...artwork,
    // 兼容旧的image字段
    image: artwork.coverUrl
  }
}

// 获取热门作品（按点赞数排序）
export function getPopularArtworks(limit = 6) {
  return mockArtworks
    .sort((a, b) => b.likes - a.likes)
    .slice(0, limit)
    .map(artwork => ({
      ...artwork,
      image: artwork.coverUrl // 兼容性
    }))
}

// 获取最新作品
export function getLatestArtworks(limit = 6) {
  return mockArtworks
    .sort((a, b) => new Date(b.createdAt) - new Date(a.createdAt))
    .slice(0, limit)
    .map(artwork => ({
      ...artwork,
      image: artwork.coverUrl // 兼容性
    }))
}

// 根据标签搜索作品
export function searchArtworksByTag(tag) {
  return mockArtworks.filter(artwork => 
    artwork.tags.some(t => t.includes(tag))
  ).map(artwork => ({
    ...artwork,
    image: artwork.coverUrl // 兼容性
  }))
}

// 获取相关推荐作品
export function getRelatedArtworks(artworkId, limit = 4) {
  const artwork = getArtworkById(artworkId)
  if (!artwork) return []
  
  // 基于标签推荐相关作品
  const related = mockArtworks
    .filter(item => item.id != artworkId)
    .filter(item => {
      // 检查是否有共同标签
      return item.tags.some(tag => artwork.tags.includes(tag))
    })
    .sort((a, b) => {
      // 按共同标签数量排序
      const aCommonTags = a.tags.filter(tag => artwork.tags.includes(tag)).length
      const bCommonTags = b.tags.filter(tag => artwork.tags.includes(tag)).length
      return bCommonTags - aCommonTags
    })
    .slice(0, limit)
  
  // 如果相关作品不够，用热门作品补充
  if (related.length < limit) {
    const popular = getPopularArtworks(limit)
      .filter(item => item.id != artworkId)
      .filter(item => !related.find(r => r.id === item.id))
      .slice(0, limit - related.length)
    
    related.push(...popular)
  }
  
  return related.map(artwork => ({
    ...artwork,
    image: artwork.coverUrl // 兼容性
  }))
}

// 获取作品评论
export function getCommentsByArtworkId(artworkId) {
  return mockComments.filter(comment => comment.artworkId == artworkId)
}

// 添加评论
export function addComment(data) {
  const newComment = {
    id: mockComments.length + 1,
    artworkId: data.artworkId,
    author: {
      id: 999, // 当前用户ID
      name: "当前用户",
      avatar: "/static/images/current_user_avatar.png"
    },
    content: data.content,
    likes: 0,
    isLiked: false,
    createdAt: new Date().toISOString(),
    replies: []
  }
  
  mockComments.unshift(newComment)
  return newComment
}

// 点赞评论
export function likeComment(commentId) {
  const comment = mockComments.find(c => c.id == commentId)
  if (comment) {
    comment.isLiked = !comment.isLiked
    comment.likes += comment.isLiked ? 1 : -1
    comment.likes = Math.max(0, comment.likes)
  }
  return comment
}

// 删除评论
export function deleteComment(commentId) {
  const index = mockComments.findIndex(c => c.id == commentId)
  if (index > -1) {
    mockComments.splice(index, 1)
    return true
  }
  return false
}

// 根据用户ID获取作品
export function getArtworksByUserId(userId, page = 1, limit = 20) {
  // 模拟分页
  const startIndex = (page - 1) * limit
  const endIndex = startIndex + limit
  
  // 筛选该用户的作品
  const userArtworks = mockArtworks.filter(artwork => artwork.author.id == userId)
  
  return userArtworks
    .slice(startIndex, endIndex)
    .map(artwork => ({
      ...artwork,
      image: artwork.coverUrl // 兼容性
    }))
}