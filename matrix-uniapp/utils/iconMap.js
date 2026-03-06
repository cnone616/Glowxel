// SVG图标名称到iconfont类名的映射
export const iconMap = {
  // 基础图标
  'Search': 'search',
  'Plus': 'add',
  'X': 'close',
  'Check': 'check-item',
  'ArrowLeft': 'arrow-left-bold',
  'ArrowRight': 'arrow-right-bold',
  'ArrowUp': 'arrow-up-bold',
  'ArrowDown': 'arrow-down-bold',
  
  // 功能图标
  'Upload': 'upload',
  'Download': 'download',
  'Edit': 'edit',
  'Trash': 'ashbin',
  'Save': 'save',
  'Grid': 'column-4',
  'Palette': 'picture',
  'List': 'menu',
  'Settings': 'setting',
  'Home': 'home',
  
  // 视图控制
  'Eye': 'browse',
  'EyeOff': 'eye-close',
  'Maximize': 'fullscreen-expand',
  'ZoomIn': 'zoom-in',
  'ZoomOut': 'zoom-out',
  
  // 工具图标
  'Pencil': 'edit',
  'Eraser': 'delete',
  'Move': 'move',
  'Hammer': 'work',
  'HelpCircle': 'help',
  'Lightbulb': 'prompt',
  
  // 连接图标
  'Wifi': 'link',
  'Bluetooth': 'link',
  'Vibrate': 'notification',
  'Database': 'file-common',
  
  // 其他
  'Undo': 'back',
  'Redo': 'forward'
}

// 获取iconfont类名
export function getIconClass(svgName) {
  return iconMap[svgName] || 'help'
}
