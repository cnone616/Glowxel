/**
 * WebSocket 通讯工具类
 * 用于与 ESP32 LED 矩阵板进行 WebSocket 通讯
 */

function normalizeHexColor(value) {
  if (typeof value !== "string") {
    throw new Error("颜色值无效");
  }
  const body = value.trim().replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    throw new Error("颜色值无效");
  }
  return `#${body.toLowerCase()}`;
}

function hexToRgb(value) {
  const normalized = normalizeHexColor(value);
  return {
    r: parseInt(normalized.slice(1, 3), 16),
    g: parseInt(normalized.slice(3, 5), 16),
    b: parseInt(normalized.slice(5, 7), 16),
  };
}

function isErrorResponse(message) {
  return !!(message && (message.error || message.status === "error"));
}

function buildMessageMatcher(expectedMessages) {
  const targets = Array.isArray(expectedMessages)
    ? expectedMessages
    : [expectedMessages];

  return (message) => {
    if (!message || typeof message !== "object") {
      return false;
    }
    if (isErrorResponse(message)) {
      return true;
    }
    if (typeof message.message !== "string") {
      return false;
    }
    return targets.includes(message.message);
  };
}

function getSetModeSuccessMessage(mode) {
  if (mode === "clock") {
    return "switched to static clock mode";
  }
  if (mode === "canvas") {
    return "switched to canvas mode";
  }
  if (mode === "animation") {
    return "switched to animation mode";
  }
  if (mode === "gif_player") {
    return "switched to gif player mode";
  }
  if (mode === "theme") {
    return "switched to theme mode";
  }
  if (mode === "text_display") {
    return "switched to text display mode";
  }
  if (mode === "breath_effect") {
    return "switched to breath effect mode";
  }
  if (mode === "rhythm_effect") {
    return "switched to rhythm effect mode";
  }
  if (mode === "ambient_effect") {
    return "switched to ambient effect mode";
  }
  if (mode === "led_matrix_showcase") {
    return "switched to led matrix showcase mode";
  }
  if (mode === "transferring") {
    return "entered transferring mode";
  }
  if (mode === "tetris") {
    return "tetris started";
  }
  if (mode === "weather") {
    return "switched to weather mode";
  }
  if (mode === "countdown") {
    return "switched to countdown mode";
  }
  if (mode === "stopwatch") {
    return "switched to stopwatch mode";
  }
  if (mode === "notification") {
    return "switched to notification mode";
  }
  if (mode === "planet_screensaver") {
    return "switched to planet screensaver mode";
  }
  if (mode === "eyes") {
    return "switched to eyes mode";
  }
  return "";
}

class WebSocket {
  constructor() {
    this.socket = null;
    this.connected = false;
    this.host = "";
    this.port = 80;
    this.connectionState = "idle";

    this.onMessageCallbacks = [];
    this.onConnectCallback = null;
    this.onDisconnectCallback = null;
    this.onErrorCallback = null;

    this._reconnectAttempts = 0;
    this._reconnectTimer = null;
    this._manualDisconnect = false;
    this._maxReconnectAttempts = 10;
    this._heartbeatTimer = null;

    this._socketIdSeed = 0;
    this._currentSocketId = 0;
    this._connectPromise = null;
    this._connectKey = "";
    this._closeMetaBySocketId = new Map();
    this._messageWaiters = new Set();
    this._hasReceivedMessage = false;
  }

  normalizeHostInput(host) {
    if (typeof host !== "string") {
      return "";
    }

    let normalized = host.trim();
    normalized = normalized.replace(/^wss?:\/\//i, "");
    normalized = normalized.replace(/^https?:\/\//i, "");
    normalized = normalized.replace(/\/+$/, "");

    const slashIndex = normalized.indexOf("/");
    if (slashIndex >= 0) {
      normalized = normalized.slice(0, slashIndex);
    }

    return normalized;
  }

  _clearReconnectTimer() {
    if (this._reconnectTimer) {
      clearTimeout(this._reconnectTimer);
      this._reconnectTimer = null;
    }
  }

  _emitError(err) {
    if (this.onErrorCallback) {
      this.onErrorCallback(err);
    }
  }

  _registerCloseMeta(socketId, meta) {
    this._closeMetaBySocketId.set(socketId, meta);
  }

  _consumeCloseMeta(socketId) {
    const meta = this._closeMetaBySocketId.get(socketId) || null;
    if (meta) {
      this._closeMetaBySocketId.delete(socketId);
    }
    return meta;
  }

  _closeSocketTask(socketTask, socketId, meta) {
    if (!socketTask) {
      return;
    }

    this._registerCloseMeta(socketId, meta);
    socketTask.close({
      success: () => {
        if (meta && meta.logClose) {
          console.log(meta.logClose);
        }
      },
    });
  }

  _setDisconnectedState(nextState = "idle") {
    this.connected = false;
    this.connectionState = nextState;
    this._stopHeartbeat();
    this._hasReceivedMessage = false;
  }

  _addMessageWaiter(waiter) {
    this._messageWaiters.add(waiter);
  }

  _removeMessageWaiter(waiter) {
    this._messageWaiters.delete(waiter);
  }

  _rejectMessageWaitersForSocket(socketId, err) {
    Array.from(this._messageWaiters).forEach((waiter) => {
      if (!waiter || waiter.socketId !== socketId) {
        return;
      }
      this._removeMessageWaiter(waiter);
      waiter.reject(err);
    });
  }

  /**
   * 连接到 ESP32
   * @param {string} host - IP 地址
   * @param {number} port - 端口号，默认 80
   * @param {object} options - 内部连接选项
   */
  connect(host, port = 80, options = {}) {
    const normalizedHost = this.normalizeHostInput(host);
    if (!normalizedHost) {
      const err = new Error("设备 IP 地址无效");
      this._emitError(err);
      return Promise.reject(err);
    }

    const connectKey = `${normalizedHost}:${port}`;
    const isReconnect = options.isReconnect === true;

    if (
      this.connected &&
      this.connectionState === "open" &&
      this.host === normalizedHost &&
      this.port === port
    ) {
      return Promise.resolve();
    }

    if (
      this.connectionState === "connecting" &&
      this._connectPromise &&
      this._connectKey === connectKey
    ) {
      return this._connectPromise;
    }

    this._clearReconnectTimer();
    this._manualDisconnect = false;

    const previousSocket = this.socket;
    const previousSocketId = this._currentSocketId;
    if (previousSocket) {
      console.log("关闭旧连接");
      this._closeSocketTask(previousSocket, previousSocketId, {
        suppressReconnect: true,
        logClose: "旧连接已关闭",
      });
    }

    this.host = normalizedHost;
    this.port = port;
    this._connectKey = connectKey;
    this._setDisconnectedState("connecting");

    const socketId = ++this._socketIdSeed;
    this._currentSocketId = socketId;

    this._connectPromise = new Promise((resolve, reject) => {
      let settled = false;
      let connectTimeoutTimer = null;
      let opened = false;

      const finishResolve = () => {
        if (settled) {
          return;
        }
        settled = true;
        if (connectTimeoutTimer) {
          clearTimeout(connectTimeoutTimer);
          connectTimeoutTimer = null;
        }
        resolve();
      };

      const finishReject = (err) => {
        if (settled) {
          return;
        }
        settled = true;
        if (connectTimeoutTimer) {
          clearTimeout(connectTimeoutTimer);
          connectTimeoutTimer = null;
        }
        reject(err);
      };

      const url = `ws://${normalizedHost}/ws`;
      console.log("准备连接 WebSocket:", url);

      const socketTask = uni.connectSocket({
        url,
        success: () => {
          console.log("WebSocket 创建成功");
        },
        fail: (err) => {
          if (socketId !== this._currentSocketId) {
            return;
          }
          console.error("WebSocket 创建失败:", err);
          this._setDisconnectedState("idle");
          this._emitError(err);
          if (isReconnect && !this._manualDisconnect) {
            this._scheduleReconnect();
          }
          finishReject(err);
        },
      });

      this.socket = socketTask;

      connectTimeoutTimer = setTimeout(() => {
        if (socketId !== this._currentSocketId) {
          return;
        }
        const err = new Error("WebSocket 连接超时");
        console.error(err.message);
        this._setDisconnectedState("idle");
        this._emitError(err);
        this._closeSocketTask(socketTask, socketId, {
          suppressReconnect: true,
        });
        if (isReconnect && !this._manualDisconnect) {
          this._scheduleReconnect();
        }
        finishReject(err);
      }, 12000);

      socketTask.onOpen(() => {
        if (socketId !== this._currentSocketId) {
          return;
        }
        console.log("WebSocket 连接成功");
        opened = true;
        this.connected = true;
        this.connectionState = "open";
        this._reconnectAttempts = 0;
        if (this.onConnectCallback) {
          this.onConnectCallback();
        }
        finishResolve();
      });

      socketTask.onMessage((res) => {
        if (socketId !== this._currentSocketId) {
          return;
        }
        console.log("收到消息:", res.data);
        if (!this._hasReceivedMessage) {
          this._hasReceivedMessage = true;
          this._startHeartbeat();
        }
        try {
          const data = JSON.parse(res.data);
          this.onMessageCallbacks.forEach((callback) => {
            if (callback) {
              callback(data);
            }
          });
        } catch (err) {
          console.error("JSON 解析失败:", err);
        }
      });

      socketTask.onClose((event) => {
        const closeMeta = this._consumeCloseMeta(socketId);
        if (socketId !== this._currentSocketId) {
          return;
        }

        const closeCode =
          event && typeof event.code !== "undefined" ? event.code : "unknown";
        const closeReason =
          event && typeof event.reason === "string" ? event.reason : "";
        console.log(
          `WebSocket 连接已关闭 code=${closeCode}${closeReason ? ` reason=${closeReason}` : ""}`,
        );
        if (this.socket === socketTask) {
          this.socket = null;
        }
        this._rejectMessageWaitersForSocket(
          socketId,
          new Error("WebSocket 连接已关闭"),
        );
        this._setDisconnectedState("idle");

        if (this.onDisconnectCallback) {
          this.onDisconnectCallback();
        }

        const shouldReconnect =
          !this._manualDisconnect &&
          this.host &&
          (!closeMeta || closeMeta.suppressReconnect !== true) &&
          (opened || isReconnect);

        if (shouldReconnect) {
          this._scheduleReconnect();
        }

        if (!settled) {
          finishReject(new Error("WebSocket 连接已关闭"));
        }
      });

      socketTask.onError((err) => {
        if (socketId !== this._currentSocketId) {
          return;
        }
        console.error("WebSocket 错误:", err);
        this._emitError(err);
      });
    });

    return this._connectPromise.finally(() => {
      if (this._connectPromise && this._connectKey === connectKey) {
        this._connectPromise = null;
      }
    });
  }

  /**
   * 发送命令
   * @param {object} data - 命令数据
   */
  send(data) {
    if (!this.connected || !this.socket) {
      const err = new Error("未连接到设备");
      console.error(err.message);
      return Promise.reject(err);
    }

    return new Promise((resolve, reject) => {
      this.socket.send({
        data: JSON.stringify(data),
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
    this._clearReconnectTimer();
    this._reconnectAttempts = 0;
    this._stopHeartbeat();

    if (!this.socket) {
      this._setDisconnectedState("idle");
      return;
    }

    const socketTask = this.socket;
    const socketId = this._currentSocketId;
    this._setDisconnectedState("closing");
    this._closeSocketTask(socketTask, socketId, {
      suppressReconnect: true,
      logClose: "WebSocket 已关闭",
    });
  }

  _startHeartbeat() {
    this._stopHeartbeat();
    this._heartbeatTimer = setInterval(() => {
      if (!this.connected || !this.socket) {
        return;
      }
      this.send({ cmd: "ping" }).catch(() => {});
    }, 5000);
  }

  _stopHeartbeat() {
    if (this._heartbeatTimer) {
      clearInterval(this._heartbeatTimer);
      this._heartbeatTimer = null;
    }
  }

  pauseHeartbeat() {
    this._stopHeartbeat();
  }

  resumeHeartbeat() {
    if (this.connected && this.connectionState === "open") {
      this._startHeartbeat();
    }
  }

  /**
   * 指数退避重连（意外断线时自动调用）
   */
  _scheduleReconnect() {
    if (!this.host) {
      return;
    }
    if (this._manualDisconnect) {
      return;
    }
    if (this.connected || this.connectionState === "connecting") {
      return;
    }
    if (this._reconnectTimer) {
      return;
    }
    if (this._reconnectAttempts >= this._maxReconnectAttempts) {
      console.log(`已达最大重连次数 ${this._maxReconnectAttempts}，停止重连`);
      return;
    }

    const delay = Math.min(1000 * Math.pow(2, this._reconnectAttempts), 30000);
    this._reconnectAttempts += 1;
    this.connectionState = "reconnect_wait";
    console.log(`将在 ${delay}ms 后第 ${this._reconnectAttempts} 次重连...`);

    this._reconnectTimer = setTimeout(() => {
      this._reconnectTimer = null;
      if (this._manualDisconnect) {
        return;
      }
      if (this.connected || this.connectionState === "connecting") {
        return;
      }
      console.log("正在重连...");
      this.connect(this.host, this.port, { isReconnect: true }).catch((err) => {
        console.error("重连失败:", err);
      });
    }, delay);
  }

  onMessage(callback) {
    if (!this.onMessageCallbacks.includes(callback)) {
      this.onMessageCallbacks.push(callback);
    }
  }

  offMessage(callback) {
    const index = this.onMessageCallbacks.indexOf(callback);
    if (index >= 0) {
      this.onMessageCallbacks.splice(index, 1);
    }
  }

  onConnect(callback) {
    this.onConnectCallback = callback;
  }

  onDisconnect(callback) {
    this.onDisconnectCallback = callback;
  }

  onError(callback) {
    this.onErrorCallback = callback;
  }

  async ping() {
    return this.send({ cmd: "ping" });
  }

  waitForMessage(matcher, timeout = 3000) {
    if (!this.connected || !this.socket) {
      return Promise.reject(new Error("未连接到设备"));
    }

    return new Promise((resolve, reject) => {
      let done = false;
      let timer = null;

      const finishResolve = (message) => {
        if (done) {
          return;
        }
        done = true;
        if (timer) {
          clearTimeout(timer);
          timer = null;
        }
        this.offMessage(handler);
        this._removeMessageWaiter(waiter);
        resolve(message);
      };

      const finishReject = (err) => {
        if (done) {
          return;
        }
        done = true;
        if (timer) {
          clearTimeout(timer);
          timer = null;
        }
        this.offMessage(handler);
        this._removeMessageWaiter(waiter);
        reject(err);
      };

      const handler = (message) => {
        if (!matcher(message)) {
          return;
        }
        if (message && (message.error || message.status === "error")) {
          finishReject(
            new Error(message.error || message.message || "设备返回错误"),
          );
          return;
        }
        finishResolve(message);
      };

      this.onMessage(handler);
      const waiter = {
        socketId: this._currentSocketId,
        reject: finishReject,
      };
      this._addMessageWaiter(waiter);

      timer = setTimeout(() => {
        finishReject(new Error("等待回复超时"));
      }, timeout);
    });
  }

  sendAndWait(data, timeout = 3000, matcher = null) {
    const finalMatcher =
      typeof matcher === "function"
        ? matcher
        : (message) => {
            if (!message || typeof message !== "object") {
              return false;
            }
            return (
              message.status === "ok" ||
              message.status === "success" ||
              message.status === "error"
            );
          };

    return new Promise((resolve, reject) => {
      this.waitForMessage(finalMatcher, timeout)
        .then(resolve)
        .catch(reject);

      this.send(data).catch(reject);
    });
  }

  async getStatus() {
    return this.sendAndWait(
      { cmd: "status" },
      3000,
      (response) => {
        if (!response || typeof response !== "object") {
          return false;
        }
        if (response.error || response.status === "error") {
          return true;
        }
        return (
          response.status === "ok" &&
          typeof response.ip === "string" &&
          typeof response.width === "number" &&
          typeof response.height === "number" &&
          typeof response.brightness === "number" &&
          typeof response.mode === "string"
        );
      },
    );
  }

  async getInfo() {
    return this.sendAndWait(
      { cmd: "get_info" },
      3000,
      (response) => {
        if (!response || typeof response !== "object") {
          return false;
        }
        if (isErrorResponse(response)) {
          return true;
        }
        return (
          response.status === "ok" &&
          typeof response.firmware_version === "string" &&
          typeof response.ip === "string"
        );
      },
    );
  }

  waitForCommand(data, expectedMessages, timeout = 5000) {
    return this.sendAndWait(
      data,
      timeout,
      buildMessageMatcher(expectedMessages),
    );
  }

  async setMode(mode) {
    const expectedMessage = getSetModeSuccessMessage(mode);
    if (!expectedMessage) {
      throw new Error(`未支持的模式切换确认：${mode}`);
    }
    return this.waitForCommand({ cmd: "set_mode", mode }, expectedMessage, 5000);
  }

  async setClockConfig(config) {
    return this.send({ cmd: "set_clock_config", config });
  }

  async setThemeConfig(themeId) {
    return this.waitForCommand(
      { cmd: "set_theme_config", themeId },
      "theme config updated",
      5000,
    );
  }

  async setEyesConfig(config) {
    return this.waitForCommand(
      { cmd: "set_eyes_config", config },
      "eyes config updated",
      5000,
    );
  }

  async setAmbientEffect(config) {
    if (config.preset === "rain_scene") {
      return this.waitForCommand(
        {
          cmd: "set_ambient_effect",
          preset: config.preset,
          speed: config.speed,
          density: config.density,
          color: hexToRgb(config.color),
          loop: config.loop,
        },
        "ambient effect applied",
        5000,
      );
    }

    return this.waitForCommand(
      {
        cmd: "set_ambient_effect",
        preset: config.preset,
        speed: config.speed,
        intensity: config.intensity,
        loop: config.loop,
      },
      "ambient effect applied",
      5000,
    );
  }

  async setCountdownBoard(config) {
    return this.waitForCommand(
      {
        cmd: "set_countdown_board",
        hours: config.hours,
        minutes: config.minutes,
        seconds: config.seconds,
        progress: config.progress,
      },
      "countdown board applied",
      5000,
    );
  }

  async setPlanetScreensaver(config) {
    return this.waitForCommand(
      {
        cmd: "set_planet_screensaver",
        preset: config.preset,
        size: config.size,
        direction: config.direction,
        speed: config.speed,
        seed: config.seed,
        colorSeed: config.colorSeed,
      },
      "planet screensaver applied",
      5000,
    );
  }

  async setGifAnimation(animationData) {
    return this.waitForCommand(
      {
        cmd: "set_gif_animation",
        animationData,
      },
      animationData == null ? "animation cleared" : "compact animation loaded",
      10000,
    );
  }

  async eyesInteract(action) {
    return this.send({ cmd: "eyes_interact", action });
  }

  async clear() {
    return this.send({ cmd: "clear" });
  }

  async saveCanvas() {
    return this.waitForCommand({ cmd: "save_canvas" }, "canvas saved", 5000);
  }

  async clearCanvas() {
    return this.waitForCommand({ cmd: "clear_canvas" }, "canvas cleared", 5000);
  }

  async setBrightness(value) {
    return this.send({ cmd: "brightness", value });
  }

  async showText(text, x = 0, y = 0) {
    return this.send({ cmd: "text", text, x, y });
  }

  async setPixel(x, y, r, g, b) {
    return this.send({ cmd: "pixel", x, y, r, g, b });
  }

  async showImage(pixels, width, height) {
    await this.send({ cmd: "clear" });

    const sparseData = [];
    let idx = 0;

    for (let y = 0; y < height; y += 1) {
      for (let x = 0; x < width; x += 1) {
        const r = pixels[idx++];
        const g = pixels[idx++];
        const b = pixels[idx++];
        sparseData.push(x, y, r, g, b);
      }
    }

    if (sparseData.length === 0) {
      return;
    }

    const batchSize = 64;
    const pixelsPerBatch = batchSize * 5;

    console.log(
      `发送完整画布数据: ${width}x${height}, 总像素: ${sparseData.length / 5}`,
    );

    for (let i = 0; i < sparseData.length; i += pixelsPerBatch) {
      const batch = sparseData.slice(i, Math.min(i + pixelsPerBatch, sparseData.length));
      const buffer = new Uint8Array(batch);

      await new Promise((resolve, reject) => {
        this.socket.send({
          data: buffer.buffer,
          success: resolve,
          fail: reject,
        });
      });

      await new Promise((resolve) => setTimeout(resolve, 200));
    }
  }

  async showSparseImage(sparsePixels, width = 64, height = 64) {
    if (sparsePixels.length === 0) {
      return;
    }

    const offsetX = Math.floor((64 - width) / 2);
    const offsetY = Math.floor((64 - height) / 2);

    await this.send({ cmd: "clear" });

    const offsetData = [];
    for (let i = 0; i < sparsePixels.length; i += 5) {
      offsetData.push(
        sparsePixels[i] + offsetX,
        sparsePixels[i + 1] + offsetY,
        sparsePixels[i + 2],
        sparsePixels[i + 3],
        sparsePixels[i + 4],
      );
    }

    const batchSize = 64;
    const pixelsPerBatch = batchSize * 5;

    console.log(`发送稀疏像素数据: ${offsetData.length / 5} 个像素`);

    for (let i = 0; i < offsetData.length; i += pixelsPerBatch) {
      const batch = offsetData.slice(i, Math.min(i + pixelsPerBatch, offsetData.length));
      const buffer = new Uint8Array(batch);

      await new Promise((resolve, reject) => {
        this.socket.send({
          data: buffer.buffer,
          success: resolve,
          fail: reject,
        });
      });

      await new Promise((resolve) => setTimeout(resolve, 200));
    }
  }

  async sendPartialUpdate(pixelData, width = 64, height = 64) {
    if (pixelData.length === 0) {
      return;
    }

    const offsetX = Math.floor((64 - width) / 2);
    const offsetY = Math.floor((64 - height) / 2);

    const offsetData = [];
    for (let i = 0; i < pixelData.length; i += 5) {
      offsetData.push(
        pixelData[i] + offsetX,
        pixelData[i + 1] + offsetY,
        pixelData[i + 2],
        pixelData[i + 3],
        pixelData[i + 4],
      );
    }

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
