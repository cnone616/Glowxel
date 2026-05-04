const COMMAND_TIMEOUT_MS = 15000;
const COMMAND_FINAL_TIMEOUT_MS = 45000;
const CONNECT_TIMEOUT_MS = 30000;
const HTTP_RUNTIME_PRECHECK_TIMEOUT_MS = 4000;
const AUTO_RECONNECT_DELAY_MS = 1500;
const AUTO_RECONNECT_MAX_ATTEMPTS = 3;
const ACCEPTED_MESSAGE = "accepted";
const TRANSACTION_ACCEPTED_STATUS = "accepted";
const TRANSACTION_FINAL_OK_STATUS = "final_ok";
const TRANSACTION_FINAL_ERROR_STATUS = "final_error";
const TRANSACTION_ACCEPTED_TIMEOUT_MS = 8000;
const TRANSACTION_FINAL_TIMEOUT_MS = 90000;
const STATIC_PIXEL_BINARY_CHUNK_SIZE = 320;
const STATIC_PIXEL_BINARY_CHUNK_DELAY_MS = 200;
const COMPACT_ANIMATION_BINARY_CHUNK_SIZE = 1000;
const COMPACT_ANIMATION_BINARY_CHUNK_DELAY_MS = 80;

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

function normalizeRgbColor(value) {
  if (value == null || typeof value !== "object") {
    throw new Error("颜色值无效");
  }

  if (!Number.isInteger(value.r) || value.r < 0 || value.r > 255) {
    throw new Error("颜色值无效");
  }
  if (!Number.isInteger(value.g) || value.g < 0 || value.g > 255) {
    throw new Error("颜色值无效");
  }
  if (!Number.isInteger(value.b) || value.b < 0 || value.b > 255) {
    throw new Error("颜色值无效");
  }

  return {
    r: value.r,
    g: value.g,
    b: value.b,
  };
}

function isErrorResponse(message) {
  if (!message || typeof message !== "object") {
    return false;
  }
  if (typeof message.error === "string" && message.error.length > 0) {
    return true;
  }
  return message.status === "error";
}

function isRuntimeStatusPayload(message) {
  if (!message || typeof message !== "object") {
    return false;
  }

  return (
    message.status === "ok" &&
    typeof message.ip === "string" &&
    typeof message.width === "number" &&
    typeof message.height === "number" &&
    typeof message.brightness === "number" &&
    typeof message.mode === "string" &&
    typeof message.businessMode === "string" &&
    typeof message.effectMode === "string"
  );
}

function isAcceptedResponse(message) {
  if (!message || typeof message !== "object") {
    return false;
  }

  return message.status === "ok" && message.message === ACCEPTED_MESSAGE;
}

function isTransactionAcceptedResponse(message) {
  if (!message || typeof message !== "object") {
    return false;
  }

  return message.status === TRANSACTION_ACCEPTED_STATUS;
}

function isTransactionFinalErrorResponse(message) {
  if (!message || typeof message !== "object") {
    return false;
  }

  return message.status === TRANSACTION_FINAL_ERROR_STATUS;
}

function isTransactionFinalResponse(message) {
  if (!message || typeof message !== "object") {
    return false;
  }

  return (
    message.status === TRANSACTION_FINAL_OK_STATUS ||
    message.status === TRANSACTION_FINAL_ERROR_STATUS
  );
}

function buildTransactionError(message, fallbackReason) {
  const error = new Error(fallbackReason);

  if (message && typeof message === "object") {
    if (typeof message.reason === "string" && message.reason.length > 0) {
      error.message = message.reason;
    }
    error.transactionResponse = message;
    if (isTransactionFinalResponse(message)) {
      error.transactionFinalReceived = true;
    }
  }

  return error;
}

function normalizeBinaryPayload(data) {
  if (data instanceof ArrayBuffer) {
    return data;
  }

  if (ArrayBuffer.isView(data)) {
    return data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
  }

  throw new Error("二进制数据无效");
}

function buildPixelBinaryBufferFromObjects(pixels) {
  if (!Array.isArray(pixels)) {
    throw new Error("像素数据格式无效");
  }

  const buffer = new Uint8Array(pixels.length * 5);

  for (let index = 0; index < pixels.length; index += 1) {
    const pixel = pixels[index];
    if (pixel == null || typeof pixel !== "object") {
      throw new Error(`第 ${index + 1} 个像素格式无效`);
    }

    const values = [pixel.x, pixel.y, pixel.r, pixel.g, pixel.b];
    for (let channelIndex = 0; channelIndex < values.length; channelIndex += 1) {
      const value = Number(values[channelIndex]);
      if (!Number.isInteger(value) || value < 0 || value > 255) {
        throw new Error(`第 ${index + 1} 个像素数据无效`);
      }
      buffer[index * 5 + channelIndex] = value;
    }
  }

  return buffer.buffer;
}

function buildPixelBinaryBufferFromPackedPixels(pixelData, width = 64, height = 64) {
  if (!Array.isArray(pixelData) || pixelData.length % 5 !== 0) {
    throw new Error("像素数据格式无效");
  }

  const offsetX = Math.floor((64 - width) / 2);
  const offsetY = Math.floor((64 - height) / 2);
  const buffer = new Uint8Array(pixelData.length);

  for (let index = 0; index < pixelData.length; index += 5) {
    const x = Number(pixelData[index]);
    const y = Number(pixelData[index + 1]);
    const r = Number(pixelData[index + 2]);
    const g = Number(pixelData[index + 3]);
    const b = Number(pixelData[index + 4]);

    if (!Number.isInteger(x) || !Number.isInteger(y)) {
      throw new Error("像素数据格式无效");
    }
    if (!Number.isInteger(r) || !Number.isInteger(g) || !Number.isInteger(b)) {
      throw new Error("像素数据格式无效");
    }

    buffer[index] = x + offsetX;
    buffer[index + 1] = y + offsetY;
    buffer[index + 2] = r;
    buffer[index + 3] = g;
    buffer[index + 4] = b;
  }

  return buffer.buffer;
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

function normalizeCompactAnimationPixelBytes(pixels, totalPixels, frameIndex) {
  if (pixels instanceof Uint8Array) {
    if (pixels.length !== totalPixels * 5) {
      throw new Error(`第 ${frameIndex + 1} 帧像素长度不匹配`);
    }
    return pixels;
  }

  if (ArrayBuffer.isView(pixels)) {
    const bytes = new Uint8Array(
      pixels.buffer,
      pixels.byteOffset,
      pixels.byteLength,
    );
    if (bytes.length !== totalPixels * 5) {
      throw new Error(`第 ${frameIndex + 1} 帧像素长度不匹配`);
    }
    return bytes;
  }

  if (!Array.isArray(pixels) || pixels.length !== totalPixels) {
    throw new Error(`第 ${frameIndex + 1} 帧像素数量不匹配`);
  }

  const bytes = new Uint8Array(totalPixels * 5);
  for (let pixelIndex = 0; pixelIndex < totalPixels; pixelIndex += 1) {
    const pixel = pixels[pixelIndex];
    if (!Array.isArray(pixel) || pixel.length < 5) {
      throw new Error(`第 ${frameIndex + 1} 帧第 ${pixelIndex + 1} 个像素格式错误`);
    }

    const offset = pixelIndex * 5;
    for (let channelIndex = 0; channelIndex < 5; channelIndex += 1) {
      const value = Number(pixel[channelIndex]);
      if (!Number.isInteger(value) || value < 0 || value > 255) {
        throw new Error(`第 ${frameIndex + 1} 帧第 ${pixelIndex + 1} 个像素数据无效`);
      }
      bytes[offset + channelIndex] = value;
    }
  }

  return bytes;
}

function buildCompactAnimationBinaryBuffer(animationData) {
  if (!Array.isArray(animationData) || animationData.length === 0) {
    throw new Error("动画帧不能为空");
  }

  const normalizedFrames = animationData.map((frame, frameIndex) => {
    if (!Array.isArray(frame) || frame.length < 4) {
      throw new Error(`第 ${frameIndex + 1} 帧数据格式错误`);
    }

    const type = Number(frame[0]);
    const delay = Number(frame[1]);
    const totalPixels = Number(frame[2]);
    const pixels = frame[3];

    if (!Number.isInteger(type) || (type !== 0 && type !== 1)) {
      throw new Error(`第 ${frameIndex + 1} 帧类型无效`);
    }
    if (!Number.isFinite(delay) || delay < 0 || delay > 65535) {
      throw new Error(`第 ${frameIndex + 1} 帧延迟无效`);
    }
    if (!Number.isInteger(totalPixels) || totalPixels < 0 || totalPixels > 65535) {
      throw new Error(`第 ${frameIndex + 1} 帧像素数量无效`);
    }

    return {
      type,
      delay: Math.round(delay),
      totalPixels,
      pixels: normalizeCompactAnimationPixelBytes(pixels, totalPixels, frameIndex),
    };
  });

  let totalBytes = 2;
  normalizedFrames.forEach((frame) => {
    totalBytes += 5 + frame.totalPixels * 5;
  });

  const buffer = new ArrayBuffer(totalBytes);
  const bytes = new Uint8Array(buffer);
  const view = new DataView(buffer);
  let offset = 0;
  view.setUint16(offset, normalizedFrames.length, true);
  offset += 2;

  normalizedFrames.forEach((frame) => {
    bytes[offset] = frame.type;
    offset += 1;
    view.setUint16(offset, frame.delay, true);
    offset += 2;
    view.setUint16(offset, frame.totalPixels, true);
    offset += 2;
    bytes.set(frame.pixels, offset);
    offset += frame.pixels.length;
  });

  return buffer;
}

function isCompactAnimationBinaryPayload(mode, byteLength) {
  if (mode === "gif_player") {
    return true;
  }
  if (mode !== "animation") {
    return false;
  }
  return byteLength > 0 && byteLength % 5 === 2;
}

function resolveTransactionBinaryChunkProfile(mode, byteLength) {
  if (isCompactAnimationBinaryPayload(mode, byteLength)) {
    return {
      chunkSize: COMPACT_ANIMATION_BINARY_CHUNK_SIZE,
      delayMs: COMPACT_ANIMATION_BINARY_CHUNK_DELAY_MS,
    };
  }

  return {
    chunkSize: STATIC_PIXEL_BINARY_CHUNK_SIZE,
    delayMs: STATIC_PIXEL_BINARY_CHUNK_DELAY_MS,
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
  if (mode === "transferring") {
    return "entered transferring mode";
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
  if (mode === "tetris") {
    return "tetris started";
  }
  if (mode === "tetris_clock") {
    return "tetris clock started";
  }
  if (mode === "maze") {
    return "maze mode started";
  }
  if (mode === "snake") {
    return "snake mode started";
  }
  if (mode === "planet_screensaver") {
    return "switched to planet screensaver mode";
  }
  if (mode === "eyes") {
    return "switched to eyes mode";
  }
  return "";
}

function buildSocketError(event, fallbackMessage) {
  if (event instanceof Error) {
    return event;
  }

  if (event && typeof event.message === "string" && event.message.length > 0) {
    return new Error(event.message);
  }

  if (event && typeof event.reason === "string" && event.reason.length > 0) {
    return new Error(event.reason);
  }

  return new Error(fallbackMessage);
}

function buildMessageError(message) {
  let errorText = "设备返回错误";
  if (message && typeof message === "object") {
    if (typeof message.error === "string" && message.error.length > 0) {
      errorText = message.error;
    } else if (typeof message.message === "string" && message.message.length > 0) {
      errorText = message.message;
    }
  }

  let details = "";
  if (message && typeof message === "object") {
    if (typeof message.details === "string" && message.details.length > 0) {
      details = message.details;
    }
  }

  if (details.length > 0) {
    return new Error(`${errorText} - ${details}`);
  }

  return new Error(errorText);
}

function buildEndpoint(host, port, secure) {
  const protocol = secure === true ? "wss" : "ws";
  return `${protocol}://${host}:${port}/ws`;
}

function buildStatusUrl(host, port, secure) {
  const protocol = secure === true ? "https" : "http";
  return `${protocol}://${host}:${port}/status?ts=${Date.now()}`;
}

function buildRuntimeHttpUrl(host, port, secure, path) {
  const protocol = secure === true ? "https" : "http";
  return `${protocol}://${host}:${port}${path}`;
}

export class DeviceWebSocket {
  constructor() {
    this.socket = null;
    this.connected = false;
    this.host = "";
    this.port = 80;
    this.secure = false;
    this.connectionState = "idle";

    this.onMessageCallbacks = [];
    this.onConnectCallback = null;
    this.onDisconnectCallback = null;
    this.onErrorCallback = null;

    this._socketIdSeed = 0;
    this._currentSocketId = 0;
    this._connectPromise = null;
    this._connectKey = "";
    this._closeMetaBySocketId = new Map();
    this._messageWaiters = new Set();
    this._jsonCommandQueue = Promise.resolve();
    this._transactionIdSeed = 0;
    this._reconnectTimer = null;
    this._reconnectAttemptCount = 0;
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

  _emitError(err) {
    if (typeof this.onErrorCallback === "function") {
      this.onErrorCallback(err);
    }
  }

  async probeRuntimeStatus(
    host,
    port = 80,
    timeout = HTTP_RUNTIME_PRECHECK_TIMEOUT_MS,
    options = {},
  ) {
    const normalizedHost = this.normalizeHostInput(host);
    if (normalizedHost.length === 0) {
      throw new Error("设备 IP 地址无效");
    }

    const secure = options && options.secure === true;
    const controller = new AbortController();
    const timeoutId = globalThis.setTimeout(() => {
      controller.abort();
    }, timeout);

    try {
      const response = await fetch(buildStatusUrl(normalizedHost, port, secure), {
        method: "GET",
        signal: controller.signal,
      });
      if (!response.ok) {
        throw new Error(`设备运行态 HTTP 不可用（${response.status}）`);
      }

      const text = await response.text();
      let data = null;
      try {
        data = JSON.parse(text);
      } catch (error) {
        throw new Error("设备运行态状态响应不是有效 JSON");
      }

      if (!isRuntimeStatusPayload(data)) {
        throw new Error("设备运行态状态响应无效");
      }

      return data;
    } catch (error) {
      if (error && error.name === "AbortError") {
        throw new Error("设备运行态 HTTP 预检超时");
      }
      throw error;
    } finally {
      globalThis.clearTimeout(timeoutId);
    }
  }

  async requestRuntimeJson(path, init = {}) {
    const normalizedHost = this.normalizeHostInput(this.host);
    if (normalizedHost.length === 0) {
      throw new Error("设备 IP 地址无效");
    }

    const response = await fetch(
      buildRuntimeHttpUrl(normalizedHost, this.port, this.secure, path),
      init,
    );
    const text = await response.text();

    let data = null;
    try {
      data = JSON.parse(text);
    } catch (error) {
      throw new Error("设备返回的不是有效 JSON");
    }

    if (!response.ok) {
      if (data && typeof data.error === "string" && data.error.length > 0) {
        throw new Error(data.error);
      }
      throw new Error("设备请求失败");
    }

    if (data && typeof data.error === "string" && data.error.length > 0) {
      throw new Error(data.error);
    }

    return data;
  }

  _clearReconnectTimer() {
    if (this._reconnectTimer !== null) {
      globalThis.clearTimeout(this._reconnectTimer);
      this._reconnectTimer = null;
    }
  }

  _scheduleReconnect(reason = null) {
    if (this.host.length === 0) {
      return;
    }
    if (!Number.isInteger(this.port) || this.port <= 0) {
      return;
    }
    if (this.connected) {
      return;
    }
    if (this.connectionState === "connecting") {
      return;
    }
    if (this._connectPromise !== null) {
      return;
    }
    if (this._reconnectAttemptCount >= AUTO_RECONNECT_MAX_ATTEMPTS) {
      return;
    }

    this._clearReconnectTimer();
    this._reconnectAttemptCount += 1;
    this._reconnectTimer = globalThis.setTimeout(() => {
      this._reconnectTimer = null;
      if (this.host.length === 0) {
        return;
      }
      if (!Number.isInteger(this.port) || this.port <= 0) {
        return;
      }
      if (this.connected || this.connectionState === "connecting" || this._connectPromise !== null) {
        return;
      }

      this.connect(this.host, this.port, {
        autoReconnect: true,
        secure: this.secure,
      }).catch(() => {
        if (reason) {
          return;
        }
      });
    }, AUTO_RECONNECT_DELAY_MS);
  }

  _registerCloseMeta(socketId, meta) {
    this._closeMetaBySocketId.set(socketId, meta);
  }

  _consumeCloseMeta(socketId) {
    if (!this._closeMetaBySocketId.has(socketId)) {
      return null;
    }

    const meta = this._closeMetaBySocketId.get(socketId);
    this._closeMetaBySocketId.delete(socketId);
    return meta;
  }

  _closeSocketTask(socketTask, socketId, meta) {
    if (!socketTask) {
      return;
    }

    this._registerCloseMeta(socketId, meta);
    socketTask.close();
  }

  _setDisconnectedState(nextState = "idle") {
    this.connected = false;
    this.connectionState = nextState;
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

  _enqueueJsonCommand(task) {
    const queuedTask = this._jsonCommandQueue
      .catch(() => {})
      .then(() => {
        if (!this.connected || this.socket === null) {
          throw new Error("未连接到设备");
        }
        return task();
      });

    this._jsonCommandQueue = queuedTask.catch(() => {});
    return queuedTask;
  }

  _createTransactionId() {
    this._transactionIdSeed += 1;
    return `tx-${Date.now()}-${this._transactionIdSeed}`;
  }

  _createMessageWaiter(matcher, timeout) {
    if (!this.connected || this.socket === null) {
      return {
        promise: Promise.reject(new Error("未连接到设备")),
        reject: () => {},
      };
    }

    let finishReject = null;
    const promise = new Promise((resolve, reject) => {
      let done = false;
      let timer = null;

      const finishResolve = (message) => {
        if (done) {
          return;
        }
        done = true;
        if (timer !== null) {
          globalThis.clearTimeout(timer);
          timer = null;
        }
        this.offMessage(handler);
        this._removeMessageWaiter(waiter);
        resolve(message);
      };

      finishReject = (err) => {
        if (done) {
          return;
        }
        done = true;
        if (timer !== null) {
          globalThis.clearTimeout(timer);
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
          finishReject(buildMessageError(message));
          return;
        }
        finishResolve(message);
      };

      this.onMessage(handler);
      const waiter = {
        socketId: this._currentSocketId,
        reject: (err) => {
          finishReject(err);
        },
      };
      this._addMessageWaiter(waiter);

      timer = globalThis.setTimeout(() => {
        finishReject(new Error("等待回复超时"));
      }, timeout);
    });

    return {
      promise,
      reject: (err) => {
        if (typeof finishReject === "function") {
          finishReject(err);
        }
      },
    };
  }

  _createTransactionWaiter(txId, matcher, timeout) {
    return this._createMessageWaiter(
      (message) => {
        if (!message || typeof message !== "object") {
          return false;
        }
        return message.txId === txId && matcher(message);
      },
      timeout,
    );
  }

  async _sendBinaryChunks(mode, payload) {
    const bytes = new Uint8Array(payload);
    const profile = resolveTransactionBinaryChunkProfile(mode, bytes.byteLength);

    for (let offset = 0; offset < bytes.byteLength; offset += profile.chunkSize) {
      const end = Math.min(offset + profile.chunkSize, bytes.byteLength);
      await this.sendBinary(bytes.slice(offset, end));
      if (end < bytes.byteLength) {
        await new Promise((resolve) => {
          globalThis.setTimeout(resolve, profile.delayMs);
        });
      }
    }
  }

  async _sendTransactionAbort(txId) {
    if (!this.connected || this.socket === null) {
      return;
    }

    try {
      await this.send({
        cmd: "tx_abort",
        txId,
      });
    } catch (error) {
      console.warn("事务中止指令发送失败:", error);
    }
  }

  async runModeTransaction(options = {}) {
    if (!options || typeof options !== "object") {
      throw new Error("事务参数无效");
    }

    const { mode } = options;
    if (typeof mode !== "string" || mode.length === 0) {
      throw new Error("事务模式无效");
    }

    let params = {};
    if (Object.prototype.hasOwnProperty.call(options, "params")) {
      if (!options.params || typeof options.params !== "object" || Array.isArray(options.params)) {
        throw new Error("事务参数对象无效");
      }
      params = options.params;
    }

    let binaryPayload = null;
    if (typeof options.binary !== "undefined" && options.binary !== null) {
      binaryPayload = normalizeBinaryPayload(options.binary);
    }

    const txId = this._createTransactionId();
    const acceptedTimeout =
      Number.isInteger(options.acceptedTimeout) && options.acceptedTimeout > 0
        ? options.acceptedTimeout
        : TRANSACTION_ACCEPTED_TIMEOUT_MS;
    const finalTimeout =
      Number.isInteger(options.finalTimeout) && options.finalTimeout > 0
        ? options.finalTimeout
        : TRANSACTION_FINAL_TIMEOUT_MS;

    return this._enqueueJsonCommand(async () => {
      let accepted = false;
      const acceptedWaiter = this._createTransactionWaiter(
        txId,
        (message) => {
          return isTransactionAcceptedResponse(message) || isTransactionFinalErrorResponse(message);
        },
        acceptedTimeout,
      );
      const finalWaiter = this._createTransactionWaiter(
        txId,
        (message) => {
          return isTransactionFinalResponse(message);
        },
        finalTimeout,
      );

      try {
        await this.send({
          cmd: "tx_begin",
          txId,
          mode,
          params,
          hasBinary: binaryPayload !== null,
          binarySize: binaryPayload === null ? 0 : binaryPayload.byteLength,
        });

        const acceptedResponse = await acceptedWaiter.promise;
        if (!isTransactionAcceptedResponse(acceptedResponse)) {
          throw buildTransactionError(acceptedResponse, "事务未被设备接受");
        }

        accepted = true;
        if (binaryPayload !== null) {
          await this._sendBinaryChunks(mode, binaryPayload);
        }

        await this.send({
          cmd: "tx_commit",
          txId,
        });

        const finalResponse = await finalWaiter.promise;
        if (!finalResponse || typeof finalResponse !== "object") {
          throw new Error("设备事务响应无效");
        }
        if (finalResponse.status === TRANSACTION_FINAL_ERROR_STATUS) {
          throw buildTransactionError(finalResponse, "设备事务执行失败");
        }

        return finalResponse;
      } catch (error) {
        acceptedWaiter.reject(error);
        finalWaiter.reject(error);
        if (accepted && !error.transactionFinalReceived) {
          await this._sendTransactionAbort(txId);
        }
        throw error;
      }
    });
  }

  connect(host, port = 80, options = {}) {
    if (typeof globalThis.WebSocket !== "function") {
      const err = new Error("当前环境不支持 WebSocket");
      this._emitError(err);
      return Promise.reject(err);
    }

    this._clearReconnectTimer();
    if (options.autoReconnect !== true) {
      this._reconnectAttemptCount = 0;
    }

    const connectTimeoutMs =
      Number.isInteger(options.connectTimeoutMs) && options.connectTimeoutMs > 0
        ? options.connectTimeoutMs
        : CONNECT_TIMEOUT_MS;
    const secure = options.secure === true;
    const normalizedHost = this.normalizeHostInput(host);

    if (normalizedHost.length === 0) {
      const err = new Error("设备 IP 地址无效");
      this._emitError(err);
      return Promise.reject(err);
    }

    const connectKey = `${secure ? "1" : "0"}:${normalizedHost}:${port}`;

    if (
      this.connected &&
      this.connectionState === "open" &&
      this.host === normalizedHost &&
      this.port === port &&
      this.secure === secure
    ) {
      return Promise.resolve();
    }

    if (
      this.connectionState === "connecting" &&
      this._connectPromise !== null &&
      this._connectKey === connectKey
    ) {
      return this._connectPromise;
    }

    const previousSocket = this.socket;
    const previousSocketId = this._currentSocketId;
    if (previousSocket) {
      this._closeSocketTask(previousSocket, previousSocketId, {
        suppressReconnect: true,
      });
    }

    this.host = normalizedHost;
    this.port = port;
    this.secure = secure;
    this._connectKey = connectKey;
    this._setDisconnectedState("connecting");

    const socketId = ++this._socketIdSeed;
    this._currentSocketId = socketId;

    this._connectPromise = new Promise((resolve, reject) => {
      let settled = false;
      let connectTimeoutTimer = null;
      let closingAfterConnectTimeout = false;
      let hasOpened = false;

      const finishResolve = () => {
        if (settled) {
          return;
        }
        settled = true;
        if (connectTimeoutTimer !== null) {
          globalThis.clearTimeout(connectTimeoutTimer);
          connectTimeoutTimer = null;
        }
        resolve();
      };

      const finishReject = (err) => {
        if (settled) {
          return;
        }
        settled = true;
        if (connectTimeoutTimer !== null) {
          globalThis.clearTimeout(connectTimeoutTimer);
          connectTimeoutTimer = null;
        }
        reject(err);
      };

      const socketTask = new globalThis.WebSocket(
        buildEndpoint(normalizedHost, port, secure),
      );
      socketTask.binaryType = "arraybuffer";
      this.socket = socketTask;

      connectTimeoutTimer = globalThis.setTimeout(() => {
        if (socketId !== this._currentSocketId) {
          return;
        }
        const err = new Error("WebSocket 连接超时");
        this._setDisconnectedState("idle");
        this._emitError(err);
        closingAfterConnectTimeout = true;
        finishReject(err);
        this._closeSocketTask(socketTask, socketId, {
          timeout: true,
        });
      }, connectTimeoutMs);

      socketTask.onopen = () => {
        if (socketId !== this._currentSocketId) {
          return;
        }
        hasOpened = true;
        this.connected = true;
        this.connectionState = "open";
        this._reconnectAttemptCount = 0;
        if (typeof this.onConnectCallback === "function") {
          this.onConnectCallback();
        }
        finishResolve();
      };

      socketTask.onmessage = (event) => {
        if (socketId !== this._currentSocketId) {
          return;
        }

        let data = event.data;
        if (typeof data === "string") {
          try {
            data = JSON.parse(data);
          } catch (error) {
            data = event.data;
          }
        }

        const callbacks = this.onMessageCallbacks.slice();
        callbacks.forEach((callback) => {
          if (typeof callback === "function") {
            callback(data);
          }
        });
      };

      socketTask.onclose = (event) => {
        const closeMeta = this._consumeCloseMeta(socketId);
        if (socketId !== this._currentSocketId) {
          return;
        }

        if (this.socket === socketTask) {
          this.socket = null;
        }
        this._rejectMessageWaitersForSocket(socketId, new Error("WebSocket 连接已关闭"));
        this._setDisconnectedState("idle");

        if (typeof this.onDisconnectCallback === "function") {
          this.onDisconnectCallback();
        }

        if (!settled) {
          finishReject(
            new Error(
              closingAfterConnectTimeout ? "WebSocket 连接超时" : "WebSocket 连接已关闭",
            ),
          );
          return;
        }

        if (hasOpened && (!closeMeta || closeMeta.suppressReconnect !== true)) {
          const closeCode =
            event && typeof event.code === "number" ? event.code : 1000;
          const closeReason =
            event && typeof event.reason === "string" ? event.reason : "";
          this._scheduleReconnect({
            stage: "connected",
            code: closeCode,
            reason: closeReason,
          });
        }
      };

      socketTask.onerror = (event) => {
        if (socketId !== this._currentSocketId) {
          return;
        }
        if (closingAfterConnectTimeout) {
          return;
        }

        const socketError = buildSocketError(event, "WebSocket 错误");
        this._rejectMessageWaitersForSocket(socketId, socketError);
        this._emitError(socketError);
        finishReject(socketError);
      };
    });

    return this._connectPromise.finally(() => {
      if (this._connectPromise !== null && this._connectKey === connectKey) {
        this._connectPromise = null;
      }
    });
  }

  send(data) {
    if (!this.connected || this.socket === null || this.socket.readyState !== globalThis.WebSocket.OPEN) {
      return Promise.reject(new Error("未连接到设备"));
    }

    return new Promise((resolve, reject) => {
      try {
        this.socket.send(JSON.stringify(data));
        resolve();
      } catch (error) {
        reject(error);
      }
    });
  }

  sendBinary(data) {
    if (!this.connected || this.socket === null || this.socket.readyState !== globalThis.WebSocket.OPEN) {
      return Promise.reject(new Error("未连接到设备"));
    }

    let payload = null;
    if (data instanceof ArrayBuffer) {
      payload = data;
    } else if (ArrayBuffer.isView(data)) {
      payload = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
    }

    if (payload === null) {
      return Promise.reject(new Error("二进制数据无效"));
    }

    return new Promise((resolve, reject) => {
      try {
        this.socket.send(payload);
        resolve();
      } catch (error) {
        reject(error);
      }
    });
  }

  disconnect() {
    this._clearReconnectTimer();
    if (this.socket === null) {
      this._setDisconnectedState("idle");
      return;
    }

    const socketTask = this.socket;
    const socketId = this._currentSocketId;
    this._setDisconnectedState("closing");
    this._closeSocketTask(socketTask, socketId, {
      suppressReconnect: true,
    });
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

  waitForMessage(matcher, timeout = COMMAND_TIMEOUT_MS) {
    return this._createMessageWaiter(matcher, timeout).promise;
  }

  sendAndWait(data, timeout = COMMAND_TIMEOUT_MS, matcher = null, options = {}) {
    const requiresAccepted = options.requiresAccepted === true;
    const waitForFinal = options.waitForFinal !== false;
    const acceptedTimeout =
      Number.isInteger(options.acceptedTimeout) && options.acceptedTimeout > 0
        ? options.acceptedTimeout
        : timeout;
    const finalTimeout =
      Number.isInteger(options.finalTimeout) && options.finalTimeout > 0
        ? options.finalTimeout
        : timeout;
    const baseFinalMatcher =
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
    const finalMatcher = (message) => {
      if (requiresAccepted && isAcceptedResponse(message)) {
        return false;
      }
      return baseFinalMatcher(message);
    };

    return this._enqueueJsonCommand(async () => {
      const acceptedWaiter = requiresAccepted
        ? this._createMessageWaiter(
            (message) => {
              return isAcceptedResponse(message) || isErrorResponse(message);
            },
            acceptedTimeout,
          )
        : null;
      const finalWaiter = waitForFinal
        ? this._createMessageWaiter(finalMatcher, finalTimeout)
        : null;

      try {
        await this.send(data);
        if (acceptedWaiter !== null) {
          await acceptedWaiter.promise;
        }
        if (finalWaiter !== null) {
          return await finalWaiter.promise;
        }
        return {
          status: "ok",
          message: ACCEPTED_MESSAGE,
        };
      } catch (error) {
        if (acceptedWaiter !== null) {
          acceptedWaiter.reject(error);
        }
        if (finalWaiter !== null) {
          finalWaiter.reject(error);
        }
        throw error;
      }
    });
  }

  async getStatus(timeout = COMMAND_TIMEOUT_MS) {
    return this.sendAndWait(
      { cmd: "status" },
      timeout,
      (response) => {
        if (!response || typeof response !== "object") {
          return false;
        }
        if (response.error || response.status === "error") {
          return true;
        }
        return isRuntimeStatusPayload(response);
      },
    );
  }

  async getInfo() {
    return this.sendAndWait(
      { cmd: "get_info" },
      8000,
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

  waitForCommand(data, expectedMessages, timeout = COMMAND_TIMEOUT_MS) {
    return this.sendAndWait(data, timeout, buildMessageMatcher(expectedMessages));
  }

  waitForAcceptedCommand(data, timeout = COMMAND_TIMEOUT_MS) {
    return this.sendAndWait(data, timeout, null, {
      requiresAccepted: true,
      waitForFinal: false,
    });
  }

  waitForCommandWithAccepted(
    data,
    expectedMessages,
    timeout = COMMAND_TIMEOUT_MS,
    finalTimeout = COMMAND_FINAL_TIMEOUT_MS,
  ) {
    return this.sendAndWait(data, timeout, buildMessageMatcher(expectedMessages), {
      requiresAccepted: true,
      waitForFinal: true,
      finalTimeout,
    });
  }

  async setMode(mode, options = {}) {
    const expectedMessage = getSetModeSuccessMessage(mode);
    if (expectedMessage.length === 0) {
      throw new Error(`未支持的模式切换确认：${mode}`);
    }

    if (options.waitForFinal === false) {
      return this.waitForAcceptedCommand({ cmd: "set_mode", mode }, 8000);
    }

    return this.waitForCommandWithAccepted(
      { cmd: "set_mode", mode },
      expectedMessage,
      8000,
      COMMAND_FINAL_TIMEOUT_MS,
    );
  }

  async setClockConfig(clockMode, config) {
    return this.waitForCommand(
      { cmd: "set_clock_config", clockMode, config },
      "clock config updated",
      8000,
    );
  }

  async applyClockMode(mode, config, binary = null, options = {}) {
    if (mode !== "clock" && mode !== "animation") {
      throw new Error(`未支持的时钟事务模式：${mode}`);
    }

    return this.runModeTransaction({
      mode,
      params: { config },
      binary,
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async applyThemeMode(config, themeId, options = {}) {
    return this.runModeTransaction({
      mode: "theme",
      params: {
        config,
        themeId,
      },
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async ensureCanvasMode(options = {}) {
    return this.runModeTransaction({
      mode: "canvas",
      params: {},
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async setThemeConfig(themeId, options = {}) {
    return this.runModeTransaction({
      mode: "theme",
      params: { themeId },
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async setEyesConfig(config) {
    return this.runModeTransaction({
      mode: "eyes",
      params: { config },
    });
  }

  async setAmbientEffect(config, options = {}) {
    const isWaterWorldPreset =
      config.preset === "surface" ||
      config.preset === "current" ||
      config.preset === "caustics";

    let params = null;
    if (config.preset === "rain_scene") {
      params = {
        preset: config.preset,
        speed: config.speed,
        density: config.density,
        color: hexToRgb(config.color),
        loop: config.loop,
      };
    } else if (isWaterWorldPreset) {
      params = {
        preset: config.preset,
        speed: config.speed,
        loop: config.loop,
      };
    } else {
      params = {
        preset: config.preset,
        speed: config.speed,
        intensity: config.intensity,
        loop: config.loop,
      };
    }

    if (options.clockConfig) {
      params.config = options.clockConfig;
    }

    return this.runModeTransaction({
      mode: "led_matrix_showcase",
      params,
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async startMaze(config, options = {}) {
    return this.runModeTransaction({
      mode: "maze",
      params: {
        speed: config.speed,
        mazeSizeMode: config.mazeSizeMode,
        showClock: config.showClock,
        panelBgColor: normalizeHexColor(config.panelBgColor),
        borderColor: normalizeHexColor(config.borderColor),
        timeColor: normalizeHexColor(config.timeColor),
        dateColor: normalizeHexColor(config.dateColor),
        generationPathColor: normalizeHexColor(config.generationPathColor),
        searchVisitedColor: normalizeHexColor(config.searchVisitedColor),
        searchFrontierColor: normalizeHexColor(config.searchFrontierColor),
        solvedPathStartColor: normalizeHexColor(config.solvedPathStartColor),
        solvedPathEndColor: normalizeHexColor(config.solvedPathEndColor),
      },
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async startSnake(config, options = {}) {
    return this.runModeTransaction({
      mode: "snake",
      params: {
        speed: config.speed,
        snakeWidth: config.snakeWidth,
        snakeColor: hexToRgb(config.snakeColor),
        foodColor: hexToRgb(config.foodColor),
        font: config.font,
        showSeconds: config.showSeconds,
        snakeSkin: config.snakeSkin,
      },
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async startTetris(config, options = {}) {
    const params = {
      clearMode: config.clearMode,
      cellSize: config.cellSize,
      speed: config.speed,
      showClock: config.showClock,
      pieces: config.pieces,
    };

    if (
      Object.prototype.hasOwnProperty.call(config, "config") &&
      config.config &&
      typeof config.config === "object" &&
      !Array.isArray(config.config)
    ) {
      params.config = config.config;
    }

    return this.runModeTransaction({
      mode: "tetris",
      params,
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async startTetrisClock(config, options = {}) {
    return this.runModeTransaction({
      mode: "tetris_clock",
      params: {
        cellSize: 2,
        speed: config.speed,
        hourFormat: config.hourFormat,
      },
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async setPlanetScreensaver(config, options = {}) {
    return this.runModeTransaction({
      mode: "planet_screensaver",
      params: {
        preset: config.preset,
        size: config.size,
        direction: config.direction,
        speed: config.speed,
        seed: config.seed,
        colorSeed: config.colorSeed,
        planetX: config.planetX,
        planetY: config.planetY,
        font: config.font,
        showSeconds: config.showSeconds,
        time: config.time,
      },
      acceptedTimeout: options.acceptedTimeout,
      finalTimeout: options.finalTimeout,
    });
  }

  async setGifAnimation(animationData) {
    if (animationData == null) {
      return this.waitForCommand(
        {
          cmd: "set_gif_animation",
          animationData,
        },
        "animation cleared",
        10000,
      );
    }

    const binaryPayload = buildCompactAnimationBinaryBuffer(animationData);
    return this.runModeTransaction({
      mode: "gif_player",
      params: {},
      binary: binaryPayload,
      finalTimeout: TRANSACTION_FINAL_TIMEOUT_MS,
    });
  }

  async eyesInteract(action) {
    return this.waitForCommandWithAccepted(
      { cmd: "eyes_interact", action },
      "eyes action applied",
      8000,
      COMMAND_FINAL_TIMEOUT_MS,
    );
  }

  async startLoading() {
    return this.waitForCommand({ cmd: "start_loading" }, "loading started", 8000);
  }

  async stopLoading() {
    return this.waitForCommand({ cmd: "stop_loading" }, "loading stopped", 8000);
  }

  async highlightRow(row) {
    const expectedMessage = row >= 0 ? "row highlighted" : "highlight cleared";
    return this.waitForCommandWithAccepted(
      { cmd: "highlight_row", row },
      expectedMessage,
      8000,
      8000,
    );
  }

  async highlightColor(color) {
    if (color === null) {
      return this.waitForCommandWithAccepted(
        { cmd: "highlight_color", color: null },
        "highlight cleared",
        8000,
        8000,
      );
    }

    return this.waitForCommandWithAccepted(
      {
        cmd: "highlight_color",
        color: normalizeRgbColor(color),
      },
      "color highlighted",
      8000,
      8000,
    );
  }

  async otaCheck() {
    return this.sendAndWait(
      { cmd: "ota_check" },
      8000,
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
          typeof response.has_update === "boolean"
        );
      },
    );
  }

  async otaUpdate() {
    return this.waitForCommand({ cmd: "ota_update" }, "starting update", 8000);
  }

  async clear() {
    return this.send({ cmd: "clear" });
  }

  async setBrightness(value) {
    return this.send({ cmd: "brightness", value });
  }

  async getDeviceParams() {
    return this.requestRuntimeJson("/get", { method: "GET" });
  }

  async setDeviceParam(key, value) {
    const body = new URLSearchParams();
    body.set(key, String(value));
    return this.requestRuntimeJson("/set", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8",
      },
      body: body.toString(),
    });
  }

  async clearWifiConfig() {
    return this.requestRuntimeJson("/clear-wifi", { method: "GET" });
  }

  async showImage(pixels, width, height) {
    const sparseData = [];
    let idx = 0;

    for (let y = 0; y < height; y += 1) {
      for (let x = 0; x < width; x += 1) {
        const r = pixels[idx];
        idx += 1;
        const g = pixels[idx];
        idx += 1;
        const b = pixels[idx];
        idx += 1;
        sparseData.push(x, y, r, g, b);
      }
    }

    return this.runModeTransaction({
      mode: "canvas",
      params: {},
      binary:
        sparseData.length > 0
          ? buildPixelBinaryBufferFromPackedPixels(sparseData, width, height)
          : new ArrayBuffer(0),
      finalTimeout: TRANSACTION_FINAL_TIMEOUT_MS,
    });
  }

  async showSparseImage(sparsePixels, width = 64, height = 64) {
    return this.runModeTransaction({
      mode: "canvas",
      params: {},
      binary:
        sparsePixels.length > 0
          ? buildPixelBinaryBufferFromPackedPixels(sparsePixels, width, height)
          : new ArrayBuffer(0),
      finalTimeout: TRANSACTION_FINAL_TIMEOUT_MS,
    });
  }

  buildPixelBinaryFromObjects(pixels) {
    return buildPixelBinaryBufferFromObjects(pixels);
  }

  buildPixelBinaryFromPackedPixels(pixelData, width = 64, height = 64) {
    return buildPixelBinaryBufferFromPackedPixels(pixelData, width, height);
  }

  buildCompactAnimationBinaryBuffer(animationData) {
    return buildCompactAnimationBinaryBuffer(animationData);
  }
}

export default DeviceWebSocket;
