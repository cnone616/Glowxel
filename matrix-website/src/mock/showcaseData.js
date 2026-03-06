// Mock 作品展示数据
export const showcaseItems = [
  {
    id: 1,
    title: '像素爱心',
    author: '用户A',
    category: 'animation',
    imageUrl: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMTExIi8+PHRleHQgeD0iNTAlIiB5PSI1MCUiIGZvbnQtc2l6ZT0iMzAiIGZpbGw9IiNmZjY0NjQiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGRvbWluYW50LWJhc2VsaW5lPSJtaWRkbGUiPuKdpO+4jzwvdGV4dD48L3N2Zz4=',
    likes: 128,
    views: 456,
    createdAt: '2026-03-01'
  },
  {
    id: 2,
    title: '数字时钟',
    author: '用户B',
    category: 'clock',
    imageUrl: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMTExIi8+PHRleHQgeD0iNTAlIiB5PSI1MCUiIGZvbnQtc2l6ZT0iMTYiIGZpbGw9IiM2NGM4ZmYiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGRvbWluYW50LWJhc2VsaW5lPSJtaWRkbGUiPjEyOjM0PC90ZXh0Pjwvc3ZnPg==',
    likes: 95,
    views: 320,
    createdAt: '2026-03-02'
  },
  {
    id: 3,
    title: '彩虹渐变',
    author: '用户C',
    category: 'pattern',
    imageUrl: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PGRlZnM+PGxpbmVhckdyYWRpZW50IGlkPSJyYWluYm93IiB4MT0iMCUiIHkxPSIwJSIgeDI9IjEwMCUiIHkyPSIxMDAlIj48c3RvcCBvZmZzZXQ9IjAlIiBzdHlsZT0ic3RvcC1jb2xvcjojZmYwMDAwIi8+PHN0b3Agb2Zmc2V0PSIzMyUiIHN0eWxlPSJzdG9wLWNvbG9yOiMwMGZmMDAiLz48c3RvcCBvZmZzZXQ9IjY2JSIgc3R5bGU9InN0b3AtY29sb3I6IzAwMDBmZiIvPjxzdG9wIG9mZnNldD0iMTAwJSIgc3R5bGU9InN0b3AtY29sb3I6I2ZmMDBmZiIvPjwvbGluZWFyR3JhZGllbnQ+PC9kZWZzPjxyZWN0IHdpZHRoPSI2NCIgaGVpZ2h0PSI2NCIgZmlsbD0idXJsKCNyYWluYm93KSIvPjwvc3ZnPg==',
    likes: 203,
    views: 678,
    createdAt: '2026-02-28'
  },
  {
    id: 4,
    title: '游戏角色',
    author: '用户D',
    category: 'game',
    imageUrl: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMTExIi8+PHRleHQgeD0iNTAlIiB5PSI1MCUiIGZvbnQtc2l6ZT0iMzAiIGZpbGw9IiNmZmRjMDAiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGRvbWluYW50LWJhc2VsaW5lPSJtaWRkbGUiPvCfjoI8L3RleHQ+PC9zdmc+',
    likes: 167,
    views: 543,
    createdAt: '2026-03-03'
  },
  {
    id: 5,
    title: '星空效果',
    author: '用户E',
    category: 'animation',
    imageUrl: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMDAwMDIwIi8+PGNpcmNsZSBjeD0iMTAiIGN5PSIxMCIgcj0iMSIgZmlsbD0iI2ZmZiIvPjxjaXJjbGUgY3g9IjMwIiBjeT0iMjAiIHI9IjEiIGZpbGw9IiNmZmYiLz48Y2lyY2xlIGN4PSI1MCIgY3k9IjE1IiByPSIxIiBmaWxsPSIjZmZmIi8+PGNpcmNsZSBjeD0iMjAiIGN5PSI0MCIgcj0iMSIgZmlsbD0iI2ZmZiIvPjxjaXJjbGUgY3g9IjQ1IiBjeT0iNTAiIHI9IjEiIGZpbGw9IiNmZmYiLz48L3N2Zz4=',
    likes: 234,
    views: 789,
    createdAt: '2026-03-04'
  },
  {
    id: 6,
    title: '天气图标',
    author: '用户F',
    category: 'icon',
    imageUrl: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMTExIi8+PHRleHQgeD0iNTAlIiB5PSI1MCUiIGZvbnQtc2l6ZT0iMzAiIGZpbGw9IiNmZmRjMDAiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGRvbWluYW50LWJhc2VsaW5lPSJtaWRkbGUiPuKYgO+4jzwvdGV4dD48L3N2Zz4=',
    likes: 89,
    views: 234,
    createdAt: '2026-03-05'
  }
]

export const categories = [
  { id: 'all', name: '全部', count: showcaseItems.length },
  { id: 'clock', name: '时钟', count: showcaseItems.filter(i => i.category === 'clock').length },
  { id: 'animation', name: '动画', count: showcaseItems.filter(i => i.category === 'animation').length },
  { id: 'pattern', name: '图案', count: showcaseItems.filter(i => i.category === 'pattern').length },
  { id: 'game', name: '游戏', count: showcaseItems.filter(i => i.category === 'game').length },
  { id: 'icon', name: '图标', count: showcaseItems.filter(i => i.category === 'icon').length }
]
