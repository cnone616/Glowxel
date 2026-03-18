/**
 * WebSocket 通讯工具类
 * 用于与 ESP32 LED 矩阵板进行 WebSocket 通讯
 */

class WebSocket {
  constructor() {
    this.socket = null;
    this.connected = false;
    this.host = "";
    this.port = 80;
    this.onMessageCallbacks = []; // 改为数组，支持多个监听器
    this.onConnectCallback = null;
    this.onDisconnectCallback = null;
    this.onErrorCallback = null;
    // 指数退避重连
    this._reconnectAttempts = 0;
    this._reconnectTimer = null;
    this._manualDisconnect = false;
    this._maxReconnectAttempts = 10;
    this._heartbeatTimer = null;
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
        console.log("关闭旧连接");
        this.socket.close({
          success: () => {
            console.log("旧连接已关闭");
          },
        });
        this.socket = null;
        this.connected = false;
      }

      this.host = host;
      this.port = port;

      // 使用 WebSocket 协议，连接到 /ws 路径
      const url = `ws://${host}/ws`;
      console.log("准备连接 WebSocket:", url);

      // 创建 WebSocket 连接
      const socketTask = uni.connectSocket({
        url: url,
        success: () => {
          console.log("WebSocket 创建成功");
        },
        fail: (err) => {
          console.error("WebSocket 创建失败:", err);
          this.connected = false;
          if (this.onErrorCallback) {
            this.onErrorCallback(err);
          }
          reject(err);
        },
      });

      this.socket = socketTask;

      // 连接成功
      socketTask.onOpen(() => {
        console.log("WebSocket 连接成功");
        this.connected = true;
        this._reconnectAttempts = 0;
        this._manualDisconnect = false;
        this._startHeartbeat();
        if (this.onConnectCallback) {
          this.onConnectCallback();
        }
        resolve();
      });

      // 接收消息
      socketTask.onMessage((res) => {
        console.log("收到消息:", res.data);
        try {
          const data = JSON.parse(res.data);
          // 调用所有注册的回调
          this.onMessageCallbacks.forEach((callback) => {
            if (callback) callback(data);
          });
        } catch (e) {
          console.error("JSON 解析失败:", e);
        }
      });

      // 连接关闭
      socketTask.onClose(() => {
        console.log("WebSocket 连接已关闭");
        this.connected = false;
        this._stopHeartbeat();
        if (this.onDisconnectCallback) {
          this.onDisconnectCallback();
        }
        // 意外断线时自动重连（指数退避）
        if (!this._manualDisconnect && this.host) {
          this._scheduleReconnect();
        }
      });

      // 连接错误
      socketTask.onError((err) => {
        console.error("WebSocket 错误:", err);
        this.connected = false;
        if (this.onErrorCallback) {
          this.onErrorCallback(err);
        }
        reject(err);
      });
    });
  }

  /**
   * 发送命令
   * @param {object} data - 命令数据
   */
  send(data) {
    if (!this.connected || !this.socket) {
      console.error("未连接到设备");
      return Promise.reject(new Error("未连接到设备"));
    }

    return new Promise((resolve, reject) => {
      const message = JSON.stringify(data);

      this.socket.send({
        data: message,
        success: () => {
          resolve();
        },
        fail: (err) => {
          console.error("发送失败:", err);
          reject(err);
        },
      });
    });
  }

  /**
   * 断开连接
   */
  disconnect() {
    this._manualDisconnect = true;
    this._stopHeartbeat();
    if (this._reconnectTimer) {
      clearTimeout(this._reconnectTimer);
      this._reconnectTimer = null;
    }
    this._reconnectAttempts = 0;
    if (this.socket) {
      this.socket.close({
        success: () => {
          console.log("WebSocket 已关闭");
        },
      });
      this.socket = null;
      this.connected = false;
    }
  }

  _startHeartbeat() {
    this._stopHeartbeat();
    this._heartbeatTimer = setInterval(() => {
      if (this.connected) {
        this.send({ cmd: "ping" }).catch(() => {});
      }
    }, 5000);
  }

  _stopHeartbeat() {
    if (this._heartbeatTimer) {
      clearInterval(this._heartbeatTimer);
      this._heartbeatTimer = null;
    }
  }

  /**
   * 指数退避重连（意外断线时自动调用）
   */
  _scheduleReconnect() {
    if (this._reconnectAttempts >= this._maxReconnectAttempts) {
      console.log(`已达最大重连次数 ${this._maxReconnectAttempts}，停止重连`);
      return;
    }
    const delay = Math.min(1000 * Math.pow(2, this._reconnectAttempts), 30000);
    this._reconnectAttempts++;
    console.log(`将在 ${delay}ms 后第 ${this._reconnectAttempts} 次重连...`);
    this._reconnectTimer = setTimeout(() => {
      if (this._manualDisconnect) return;
      console.log("正在重连...");
      this.connect(this.host, this.port).catch((err) => {
        console.error("重连失败:", err);
      });
    }, delay);
  }

  /**
   * 添加消息回调
   */
  onMessage(callback) {
    if (!this.onMessageCallbacks.includes(callback)) {
      this.onMessageCallbacks.push(callback);
    }
  }

  /**
   * 移除消息回调
   */
  offMessage(callback) {
    const index = this.onMessageCallbacks.indexOf(callback);
    if (index > -1) {
      this.onMessageCallbacks.splice(index, 1);
    }
  }

  /**
   * 设置连接回调
   */
  onConnect(callback) {
    this.onConnectCallback = callback;
  }

  /**
   * 设置断开回调
   */
  onDisconnect(callback) {
    this.onDisconnectCallback = callback;
  }

  /**
   * 设置错误回调
   */
  onError(callback) {
    this.onErrorCallback = callback;
  }

  /**
   * 发送 ping 命令
   */
  async ping() {
    return this.send({ cmd: "ping" });
  }

  /**
   * 发送命令并等待设备回复
   * @param {object} data - 命令数据
   * @param {number} timeout - 超时毫秒数
   * @returns {Promise<object>} 设备回复的 JSON 数据
   */
  sendAndWait(data, timeout = 3000) {
    if (!this.connected || !this.socket) {
      return Promise.reject(new Error("未连接到设备"));
    }

    return new Promise((resolve, reject) => {
      let timer = null;
      const handler = (response) => {
        clearTimeout(timer);
        this.offMessage(handler);
        resolve(response);
      };
      this.onMessage(handler);

      timer = setTimeout(() => {
        this.offMessage(handler);
        reject(new Error("等待回复超时"));
      }, timeout);

      const message = JSON.stringify(data);
      this.socket.send({
        data: message,
        fail: (err) => {
          clearTimeout(timer);
          this.offMessage(handler);
          reject(err);
        },
      });
    });
  }

  /**
   * 获取设备状态
   */
  async getStatus() {
    return this.sendAndWait({ cmd: "status" });
  }

  /**
   * 设置设备模式
   * @param {string} mode - 模式：'clock' 或 'canvas'
   */
  async setMode(mode) {
    return this.send({ cmd: "set_mode", mode });
  }

  /**
   * 设置闹钟配置
   * @param {object} config - 闹钟配置对象
   */
  async setClockConfig(config) {
    return this.send({ cmd: "set_clock_config", config });
  }

  /**
   * 清空屏幕
   */
  async clear() {
    return this.send({ cmd: "clear" });
  }

  /**
   * 设置亮度
   * @param {number} value - 亮度值 0-255
   */
  async setBrightness(value) {
    return this.send({ cmd: "brightness", value });
  }

  /**
   * 显示文字
   * @param {string} text - 文字内容
   * @param {number} x - X 坐标
   * @param {number} y - Y 坐标
   */
  async showText(text, x = 0, y = 0) {
    return this.send({ cmd: "text", text, x, y });
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
    return this.send({ cmd: "pixel", x, y, r, g, b });
  }

  /**
   * 显示图片（二进制传输，最高效）
   * @param {Array} pixels - 像素数组 [r,g,b,r,g,b,...]
   * @param {number} width - 宽度
   * @param {number} height - 高度
   */
  async showImage(pixels, width, height) {
    // 先清屏
    await this.send({ cmd: "clear" });

    // 收集像素数据，坐标直接使用前端传入的位置
    const sparseData = [];
    let idx = 0;

    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const r = pixels[idx++];
        const g = pixels[idx++];
        const b = pixels[idx++];

        sparseData.push(x, y, r, g, b);
      }
    }

    if (sparseData.length === 0) return;

    // 使用二进制传输：每个像素5字节 (x, y, r, g, b)
    // 分批发送，每批64个像素
    const batchSize = 64;
    const pixelsPerBatch = batchSize * 5;

    console.log(
      `发送完整画布数据: ${width}x${height}, 总像素: ${sparseData.length / 5}`,
    );

    for (let i = 0; i < sparseData.length; i += pixelsPerBatch) {
      const batch = sparseData.slice(
        i,
        Math.min(i + pixelsPerBatch, sparseData.length),
      );
      const buffer = new Uint8Array(batch);

      await new Promise((resolve, reject) => {
        this.socket.send({
          data: buffer.buffer,
          success: resolve,
          fail: reject,
        });
      });

      // 每批之间等待200ms
      await new Promise((resolve) => setTimeout(resolve, 200));
    }
  }

  /**
   * 发送稀疏像素数据（只发送有颜色的像素，包括黑色）
   * @param {Array} sparsePixels - 稀疏像素数据 [x, y, r, g, b, x, y, r, g, b, ...]
   * @param {number} width - 画布宽度（用于计算居中偏移）
   * @param {number} height - 画布高度（用于计算居中偏移）
   */
  async showSparseImage(sparsePixels, width = 64, height = 64) {
    if (sparsePixels.length === 0) return;

    // 根据实际画布尺寸动态计算居中偏移（板子是64x64）
    const offsetX = Math.floor((64 - width) / 2);
    const offsetY = Math.floor((64 - height) / 2);

    // 先清屏
    await this.send({ cmd: "clear" });

    // 添加偏移
    const offsetData = [];
    for (let i = 0; i < sparsePixels.length; i += 5) {
      offsetData.push(
        sparsePixels[i] + offsetX, // x
        sparsePixels[i + 1] + offsetY, // y
        sparsePixels[i + 2], // r
        sparsePixels[i + 3], // g
        sparsePixels[i + 4], // b
      );
    }

    // 使用二进制传输：每个像素5字节 (x, y, r, g, b)
    // 分批发送，每批64个像素
    const batchSize = 64;
    const pixelsPerBatch = batchSize * 5;

    console.log(`发送稀疏像素数据: ${offsetData.length / 5} 个像素`);

    for (let i = 0; i < offsetData.length; i += pixelsPerBatch) {
      const batch = offsetData.slice(
        i,
        Math.min(i + pixelsPerBatch, offsetData.length),
      );
      const buffer = new Uint8Array(batch);

      await new Promise((resolve, reject) => {
        this.socket.send({
          data: buffer.buffer,
          success: resolve,
          fail: reject,
        });
      });

      // 每批之间等待200ms
      await new Promise((resolve) => setTimeout(resolve, 200));
    }
  }

  /**
   * 发送部分更新（不清屏，直接更新像素）
   * @param {Array} pixelData - 像素数据 [x, y, r, g, b, x, y, r, g, b, ...]
   * @param {number} width - 画布宽度（用于计算居中偏移）
   * @param {number} height - 画布高度（用于计算居中偏移）
   */
  async sendPartialUpdate(pixelData, width = 64, height = 64) {
    if (pixelData.length === 0) return;

    // 根据实际画布尺寸动态计算居中偏移（板子是64x64）
    const offsetX = Math.floor((64 - width) / 2);
    const offsetY = Math.floor((64 - height) / 2);

    // 添加偏移
    const offsetData = [];
    for (let i = 0; i < pixelData.length; i += 5) {
      offsetData.push(
        pixelData[i] + offsetX, // x
        pixelData[i + 1] + offsetY, // y
        pixelData[i + 2], // r
        pixelData[i + 3], // g
        pixelData[i + 4], // b
      );
    }

    // 二进制传输
    const buffer = new Uint8Array(offsetData);

    await new Promise((resolve, reject) => {
      this.socket.send({
        data: buffer.buffer,
        success: resolve,
        fail: reject,
      });
    });
  }
}

export default WebSocket;
