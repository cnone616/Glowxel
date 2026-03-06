/**
 * WebSocket 通讯工具类
 * 用于与 ESP32 LED 矩阵板进行 WebSocket 通讯
 */

class WebSocket {
  constructor() {
    this.socket = null
    this.connected = false
    this.host = ''
    this.port = 80
    this.onMessageCallbacks = [] // 改为数组，支持多个监听器
    this.onConnectCallback = null
    this.onDisconnectCallback = null
    this.onErrorCallback = null
  }

  /**
   * 连接到 ESP32
   * @param {string} host - IP 地址
   * @param {number} port - 端口号，默认 80
   */
  connect(host, port = 80) {
    return new Promise((resolve, reject) => {
      // 如果已经有连接，先关闭
      if (this.socket) {
        console.log('关闭旧连接')
        this.socket.close({
          success: () => {
            console.log('旧连接已关闭')
          }
        })
        this.socket = null
        this.connected = false
      }
      
      this.host = host
      this.port = port

      // 使用 WebSocket 协议，连接到 /ws 路径
      const url = `ws://${host}/ws`
      console.log('准备连接 WebSocket:', url)

      // 创建 WebSocket 连接
      const socketTask = uni.connectSocket({
        url: url,
        success: () => {
          console.log('WebSocket 创建成功')
        },
        fail: (err) => {
          console.error('WebSocket 创建失败:', err)
          this.connected = false
          if (this.onErrorCallback) {
            this.onErrorCallback(err)
          }
          reject(err)
        }
      })

      this.socket = socketTask

      // 连接成功
      socketTask.onOpen(() => {
        console.log('WebSocket 连接成功')
        this.connected = true
        if (this.onConnectCallback) {
          this.onConnectCallback()
        }
        resolve()
      })

      // 接收消息
      socketTask.onMessage((res) => {
        console.log('收到消息:', res.data)
        try {
          const data = JSON.parse(res.data)
          // 调用所有注册的回调
          this.onMessageCallbacks.forEach(callback => {
            if (callback) callback(data)
          })
        } catch (e) {
          console.error('JSON 解析失败:', e)
        }
      })

      // 连接关闭
      socketTask.onClose(() => {
        console.log('WebSocket 连接已关闭')
        this.connected = false
        if (this.onDisconnectCallback) {
          this.onDisconnectCallback()
        }
      })

      // 连接错误
      socketTask.onError((err) => {
        console.error('WebSocket 错误:', err)
        this.connected = false
        if (this.onErrorCallback) {
          this.onErrorCallback(err)
        }
        reject(err)
      })
    })
  }

  /**
   * 发送命令
   * @param {object} data - 命令数据
   */
  send(data) {
    if (!this.connected || !this.socket) {
      console.error('未连接到设备')
      return Promise.reject(new Error('未连接到设备'))
    }

    return new Promise((resolve, reject) => {
      const message = JSON.stringify(data)
      
      this.socket.send({
        data: message,
        success: () => {
          resolve()
        },
        fail: (err) => {
          console.error('发送失败:', err)
          reject(err)
        }
      })
    })
  }

  /**
   * 断开连接
   */
  disconnect() {
    if (this.socket) {
      this.socket.close({
        success: () => {
          console.log('WebSocket 已关闭')
        }
      })
      this.socket = null
      this.connected = false
    }
  }

  /**
   * 添加消息回调
   */
  onMessage(callback) {
    if (!this.onMessageCallbacks.includes(callback)) {
      this.onMessageCallbacks.push(callback)
    }
  }
  
  /**
   * 移除消息回调
   */
  offMessage(callback) {
    const index = this.onMessageCallbacks.indexOf(callback)
    if (index > -1) {
      this.onMessageCallbacks.splice(index, 1)
    }
  }

  /**
   * 设置连接回调
   */
  onConnect(callback) {
    this.onConnectCallback = callback
  }

  /**
   * 设置断开回调
   */
  onDisconnect(callback) {
    this.onDisconnectCallback = callback
  }

  /**
   * 设置错误回调
   */
  onError(callback) {
    this.onErrorCallback = callback
  }

  /**
   * 发送 ping 命令
   */
  async ping() {
    return this.send({ cmd: 'ping' })
  }

  /**
   * 获取设备状态
   */
  async getStatus() {
    return this.send({ cmd: 'status' })
  }

  /**
   * 设置设备模式
   * @param {string} mode - 模式：'clock' 或 'canvas'
   */
  async setMode(mode) {
    return this.send({ cmd: 'set_mode', mode })
  }

  /**
   * 设置闹钟配置
   * @param {object} config - 闹钟配置对象
   */
  async setClockConfig(config) {
    return this.send({ cmd: 'set_clock_config', config })
  }

  /**
   * 清空屏幕
   */
  async clear() {
    return this.send({ cmd: 'clear' })
  }

  /**
   * 设置亮度
   * @param {number} value - 亮度值 0-255
   */
  async setBrightness(value) {
    return this.send({ cmd: 'brightness', value })
  }

  /**
   * 显示文字
   * @param {string} text - 文字内容
   * @param {number} x - X 坐标
   * @param {number} y - Y 坐标
   */
  async showText(text, x = 0, y = 0) {
    return this.send({ cmd: 'text', text, x, y })
  }

  /**
   * 设置单个像素
   * @param {number} x - X 坐标
   * @param {number} y - Y 坐标
   * @param {number} r - 红色 0-255
   * @param {number} g - 绿色 0-255
   * @param {number} b - 蓝色 0-255
   */
  async setPixel(x, y, r, g, b) {
    return this.send({ cmd: 'pixel', x, y, r, g, b })
  }

  /**
   * 显示图片（二进制传输，最高效）
   * @param {Array} pixels - 像素数组 [r,g,b,r,g,b,...]
   * @param {number} width - 宽度
   * @param {number} height - 高度
   */
  async showImage(pixels, width, height) {
    // 计算居中偏移（板子是64x64，数据是52x52）
    const offsetX = Math.floor((64 - width) / 2)
    const offsetY = Math.floor((64 - height) / 2)
    
    // 先清屏
    await this.send({ cmd: 'clear' })
    
    // 统计非空像素
    let nonZeroCount = 0
    const sparseData = []
    let idx = 0
    
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const r = pixels[idx++]
        const g = pixels[idx++]
        const b = pixels[idx++]
        
        if (r > 0 || g > 0 || b > 0) {
          sparseData.push(x + offsetX, y + offsetY, r, g, b)
          nonZeroCount++
        }
      }
    }
    
    if (nonZeroCount === 0) return
    
    // 使用二进制传输：每个像素5字节 (x, y, r, g, b)
    // 分批发送，每批64个像素
    const batchSize = 64
    const pixelsPerBatch = batchSize * 5
    
    for (let i = 0; i < sparseData.length; i += pixelsPerBatch) {
      const batch = sparseData.slice(i, Math.min(i + pixelsPerBatch, sparseData.length))
      const buffer = new Uint8Array(batch)
      
      await new Promise((resolve, reject) => {
        this.socket.send({
          data: buffer.buffer,
          success: resolve,
          fail: reject
        })
      })
      
      // 每批之间等待200ms
      await new Promise(resolve => setTimeout(resolve, 200))
    }
  }

  /**
   * 发送部分更新（不清屏，直接更新像素）
   * @param {Array} pixelData - 像素数据 [x, y, r, g, b, x, y, r, g, b, ...]
   */
  async sendPartialUpdate(pixelData) {
    if (pixelData.length === 0) return
    
    // 计算居中偏移
    const offsetX = Math.floor((64 - 52) / 2)
    const offsetY = Math.floor((64 - 52) / 2)
    
    // 添加偏移
    const offsetData = []
    for (let i = 0; i < pixelData.length; i += 5) {
      offsetData.push(
        pixelData[i] + offsetX,     // x
        pixelData[i + 1] + offsetY, // y
        pixelData[i + 2],           // r
        pixelData[i + 3],           // g
        pixelData[i + 4]            // b
      )
    }
    
    // 二进制传输
    const buffer = new Uint8Array(offsetData)
    
    await new Promise((resolve, reject) => {
      this.socket.send({
        data: buffer.buffer,
        success: resolve,
        fail: reject
      })
    })
  }
}

export default WebSocket
