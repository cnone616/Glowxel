// 设备数据Mock - Glowxel 像素设备
export const mockDevices = [
  {
    id: 1,
    name: "Glowxel-01",
    type: "52x52",
    icon: "electronics",
    status: "connected", // connected, disconnected, connecting
    battery: 85,
    brightness: 80,
    lastConnected: "2024-02-15 14:30:25",
    macAddress: "AA:BB:CC:DD:EE:01",
    firmwareVersion: "v2.1.3",
    connectionType: "bluetooth", // bluetooth, wifi
    signalStrength: 4, // 1-5
    temperature: 28,
    workingHours: 156
  },
  {
    id: 2,
    name: "便携LED屏",
    type: "32x32",
    icon: "electronics",
    status: "disconnected",
    battery: 45,
    brightness: 60,
    lastConnected: "2024-02-14 09:15:42",
    macAddress: "AA:BB:CC:DD:EE:02",
    firmwareVersion: "v2.0.8",
    connectionType: "wifi",
    signalStrength: 3,
    temperature: 25,
    workingHours: 89
  },
  {
    id: 3,
    name: "大屏LED显示器",
    type: "64x64",
    icon: "electronics",
    status: "connected",
    battery: 92,
    brightness: 100,
    lastConnected: "2024-02-15 16:45:12",
    macAddress: "AA:BB:CC:DD:EE:03",
    firmwareVersion: "v2.2.1",
    connectionType: "bluetooth",
    signalStrength: 5,
    temperature: 32,
    workingHours: 234
  },
  {
    id: 4,
    name: "迷你像素屏",
    type: "16x16",
    icon: "electronics",
    status: "connecting",
    battery: 68,
    brightness: 75,
    lastConnected: "2024-02-15 12:20:18",
    macAddress: "AA:BB:CC:DD:EE:04",
    firmwareVersion: "v1.9.5",
    connectionType: "bluetooth",
    signalStrength: 2,
    temperature: 24,
    workingHours: 67
  },
  {
    id: 5,
    name: "专业LED面板",
    type: "52x52",
    icon: "electronics",
    status: "disconnected",
    battery: 15,
    brightness: 50,
    lastConnected: "2024-02-13 18:30:45",
    macAddress: "AA:BB:CC:DD:EE:05",
    firmwareVersion: "v2.1.0",
    connectionType: "wifi",
    signalStrength: 1,
    temperature: 22,
    workingHours: 445
  }
]

// 设备状态统计
export const deviceStats = {
  total: mockDevices.length,
  connected: mockDevices.filter(d => d.status === "connected").length,
  disconnected: mockDevices.filter(d => d.status === "disconnected").length,
  connecting: mockDevices.filter(d => d.status === "connecting").length,
  lowBattery: mockDevices.filter(d => d.battery < 20).length
}

// 获取所有设备
export function getAllDevices() {
  return mockDevices
}

// 根据状态获取设备
export function getDevicesByStatus(status) {
  return mockDevices.filter(device => device.status === status)
}

// 获取已连接设备
export function getConnectedDevices() {
  return mockDevices.filter(device => device.status === "connected")
}

// 获取低电量设备
export function getLowBatteryDevices() {
  return mockDevices.filter(device => device.battery < 20)
}

// 根据连接类型获取设备
export function getDevicesByConnectionType(type) {
  return mockDevices.filter(device => device.connectionType === type)
}

// 根据ID获取设备
export function getDeviceById(id) {
  return mockDevices.find(device => device.id === id)
}

// 模拟连接设备
export function connectDevice(deviceId) {
  const device = getDeviceById(deviceId)
  if (device) {
    device.status = "connecting"
    // 模拟连接过程
    setTimeout(() => {
      device.status = "connected"
      device.lastConnected = new Date().toLocaleString()
    }, 2000)
    return { success: true, message: "正在连接设备..." }
  }
  return { success: false, message: "设备不存在" }
}

// 模拟断开设备
export function disconnectDevice(deviceId) {
  const device = getDeviceById(deviceId)
  if (device) {
    device.status = "disconnected"
    return { success: true, message: "设备已断开连接" }
  }
  return { success: false, message: "设备不存在" }
}