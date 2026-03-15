// Mock 时钟模板数据
export const clockTemplates = [
  {
    id: 1,
    name: '经典蓝',
    thumbnail: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMTExIi8+PHRleHQgeD0iMzIiIHk9IjI4IiBmb250LXNpemU9IjE0IiBmaWxsPSIjNjRjOGZmIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIj4xMjozNDwvdGV4dD48dGV4dCB4PSIzMiIgeT0iNDIiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzc4Nzg3OCIgdGV4dC1hbmNob3I9Im1pZGRsZSI+MDMtMDY8L3RleHQ+PC9zdmc+',
    config: {
      time: {
        fontSize: 1,
        x: 17,
        y: 18,
        r: 100, g: 200, b: 255
      },
      date: {
        show: true,
        fontSize: 1,
        x: 3,
        y: 30,
        r: 120, g: 120, b: 120
      },
      week: {
        show: true,
        x: 23,
        y: 44,
        r: 100, g: 100, b: 100
      }
    }
  },
  {
    id: 2,
    name: '霓虹紫',
    thumbnail: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMTExIi8+PHRleHQgeD0iMzIiIHk9IjMyIiBmb250LXNpemU9IjE4IiBmaWxsPSIjYzg2NGZmIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIj4xMjozNDwvdGV4dD48L3N2Zz4=',
    config: {
      time: {
        fontSize: 2,
        x: 8,
        y: 22,
        r: 200, g: 100, b: 255
      },
      date: {
        show: false,
        fontSize: 1,
        x: 0,
        y: 0,
        r: 0, g: 0, b: 0
      },
      week: {
        show: false,
        x: 0,
        y: 0,
        r: 0, g: 0, b: 0
      }
    }
  },
  {
    id: 3,
    name: '暖阳橙',
    thumbnail: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMTExIi8+PHRleHQgeD0iMzIiIHk9IjI4IiBmb250LXNpemU9IjE0IiBmaWxsPSIjZmZhNTAwIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIj4xMjozNDwvdGV4dD48dGV4dCB4PSIzMiIgeT0iNDIiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iI2ZmZGMwMCIgdGV4dC1hbmNob3I9Im1pZGRsZSI+MDMtMDY8L3RleHQ+PC9zdmc+',
    config: {
      time: {
        fontSize: 1,
        x: 17,
        y: 18,
        r: 255, g: 165, b: 0
      },
      date: {
        show: true,
        fontSize: 1,
        x: 3,
        y: 30,
        r: 255, g: 220, b: 0
      },
      week: {
        show: true,
        x: 23,
        y: 44,
        r: 255, g: 165, b: 0
      }
    }
  },
  {
    id: 4,
    name: '清新绿',
    thumbnail: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHJlY3Qgd2lkdGg9IjY0IiBoZWlnaHQ9IjY0IiBmaWxsPSIjMTExIi8+PHRleHQgeD0iMzIiIHk9IjMyIiBmb250LXNpemU9IjE2IiBmaWxsPSIjMDBmZjlkIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIj4xMjozNDwvdGV4dD48L3N2Zz4=',
    config: {
      time: {
        fontSize: 2,
        x: 8,
        y: 22,
        r: 0, g: 255, b: 157
      },
      date: {
        show: false,
        fontSize: 1,
        x: 0,
        y: 0,
        r: 0, g: 0, b: 0
      },
      week: {
        show: false,
        x: 0,
        y: 0,
        r: 0, g: 0, b: 0
      }
    }
  }
]

export const presetColors = [
  { name: '青色', hex: '#64c8ff', rgb: { r: 100, g: 200, b: 255 } },
  { name: '绿色', hex: '#00ff9d', rgb: { r: 0, g: 255, b: 157 } },
  { name: '黄色', hex: '#ffdc00', rgb: { r: 255, g: 220, b: 0 } },
  { name: '橙色', hex: '#ffa500', rgb: { r: 255, g: 165, b: 0 } },
  { name: '红色', hex: '#ff6464', rgb: { r: 255, g: 100, b: 100 } },
  { name: '紫色', hex: '#c864ff', rgb: { r: 200, g: 100, b: 255 } },
  { name: '白色', hex: '#ffffff', rgb: { r: 255, g: 255, b: 255 } },
  { name: '灰色', hex: '#787878', rgb: { r: 120, g: 120, b: 120 } },
  { name: '深灰', hex: '#646464', rgb: { r: 100, g: 100, b: 100 } }
]
