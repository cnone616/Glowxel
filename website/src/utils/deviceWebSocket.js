const DEFAULT_IGNORE_MESSAGES = new Set([
  "pong",
  "loading displayed",
  "loading started",
  "loading stopped",
  "pixels_received",
]);

function buildSocketError(event, fallbackMessage) {
  if (event instanceof Error) {
    return event;
  }

  if (event && typeof event.message === "string" && event.message.length > 0) {
    return new Error(event.message);
  }

  return new Error(fallbackMessage);
}

function buildMessageError(message) {
  const errorText =
    typeof message?.error === "string" && message.error
      ? message.error
      : typeof message?.message === "string" && message.message
        ? message.message
        : "设备返回错误";

  return new Error(
    `${errorText}${message && message.details ? ` - ${message.details}` : ""}`,
  );
}

function isDefaultResponseMessage(message, ignoreMessages) {
  if (typeof message === "string") {
    return message.length > 0 && !ignoreMessages.has(message);
  }

  if (!message || typeof message !== "object") {
    return false;
  }

  const text = typeof message.message === "string" ? message.message : "";
  if (ignoreMessages.has(text)) {
    return false;
  }

  if (message.error || message.status === "error") {
    return true;
  }

  return (
    (message.status === "ok" || message.status === "success") &&
    text.length > 0
  );
}

export class DeviceWebSocket {
  constructor(endpoint) {
    this.endpoint = endpoint;
    this.socket = null;
    this.connected = false;
    this._messageHandlers = new Set();
    this._openHandlers = new Set();
    this._closeHandlers = new Set();
    this._errorHandlers = new Set();
    this._commandQueue = Promise.resolve();
  }

  onMessage(handler) {
    this._messageHandlers.add(handler);
    return () => this._messageHandlers.delete(handler);
  }

  onOpen(handler) {
    this._openHandlers.add(handler);
    return () => this._openHandlers.delete(handler);
  }

  onClose(handler) {
    this._closeHandlers.add(handler);
    return () => this._closeHandlers.delete(handler);
  }

  onError(handler) {
    this._errorHandlers.add(handler);
    return () => this._errorHandlers.delete(handler);
  }

  _enqueueCommand(task) {
    const queuedTask = this._commandQueue
      .catch(() => {})
      .then(() => {
        if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
          throw new Error("设备未连接");
        }
        return task();
      });

    this._commandQueue = queuedTask.catch(() => {});
    return queuedTask;
  }

  connect({ timeoutMs = 12000 } = {}) {
    if (this.connected && this.socket && this.socket.readyState === WebSocket.OPEN) {
      return Promise.resolve();
    }

    if (this.socket) {
      this.disconnect();
    }

    return new Promise((resolve, reject) => {
      const ws = new WebSocket(this.endpoint);
      this.socket = ws;
      let timer = null;
      let settled = false;

      const cleanup = () => {
        if (timer) {
          window.clearTimeout(timer);
          timer = null;
        }
      };

      const finishResolve = () => {
        if (settled) {
          return;
        }
        settled = true;
        cleanup();
        resolve();
      };

      const finishReject = (error) => {
        if (settled) {
          return;
        }
        settled = true;
        cleanup();
        reject(error);
      };

      ws.onopen = () => {
        this.connected = true;
        this._openHandlers.forEach((handler) => handler());
        finishResolve();
      };

      ws.onmessage = (event) => {
        let data = event.data;
        if (typeof event.data === "string") {
          try {
            data = JSON.parse(event.data);
          } catch (error) {
            data = event.data;
          }
        }
        this._messageHandlers.forEach((handler) => handler(data));
      };

      ws.onclose = () => {
        cleanup();
        this.connected = false;
        if (this.socket === ws) {
          this.socket = null;
        }
        this._closeHandlers.forEach((handler) => handler());
        finishReject(new Error("WebSocket 连接已关闭"));
      };

      ws.onerror = (event) => {
        this._errorHandlers.forEach((handler) => handler(event));
        finishReject(buildSocketError(event, "连接设备失败"));
      };

      timer = window.setTimeout(() => {
        try {
          ws.close();
        } catch (error) {
          // ignore
        }
        finishReject(new Error("连接设备超时"));
      }, timeoutMs);
    });
  }

  disconnect() {
    if (!this.socket) {
      this.connected = false;
      return;
    }
    try {
      this.socket.close();
    } catch (error) {
      // ignore
    }
    this.socket = null;
    this.connected = false;
  }

  send(payload) {
    if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
      return Promise.reject(new Error("设备未连接"));
    }

    return new Promise((resolve, reject) => {
      try {
        const data = typeof payload === "string" ? payload : JSON.stringify(payload);
        this.socket.send(data);
        resolve();
      } catch (error) {
        reject(error);
      }
    });
  }

  sendBinary(payload) {
    if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
      return Promise.reject(new Error("设备未连接"));
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

  waitForMessage(predicate, options = {}) {
    const timeoutMs = options.timeoutMs ?? 10000;
    return new Promise((resolve, reject) => {
      let settled = false;

      const finishResolve = (message) => {
        if (settled) {
          return;
        }
        settled = true;
        offMessage();
        offClose();
        offError();
        window.clearTimeout(timer);
        resolve(message);
      };

      const finishReject = (error) => {
        if (settled) {
          return;
        }
        settled = true;
        offMessage();
        offClose();
        offError();
        window.clearTimeout(timer);
        reject(error);
      };

      const offMessage = this.onMessage((message) => {
        if (!predicate(message)) {
          return;
        }
        finishResolve(message);
      });
      const offClose = this.onClose(() => {
        finishReject(new Error("WebSocket 连接已关闭"));
      });
      const offError = this.onError((event) => {
        finishReject(buildSocketError(event, "WebSocket 连接异常"));
      });

      const timer = window.setTimeout(() => {
        finishReject(new Error("等待设备响应超时"));
      }, timeoutMs);
    });
  }

  sendAndWait(payload, options = {}) {
    const timeoutMs = options.timeoutMs || 10000;
    const ignoreMessages = new Set(options.ignoreMessages || DEFAULT_IGNORE_MESSAGES);
    const matcher =
      typeof options.matcher === "function"
        ? options.matcher
        : (message) => isDefaultResponseMessage(message, ignoreMessages);

    return this._enqueueCommand(() => new Promise((resolve, reject) => {
      let timer = null;
      let settled = false;

      const finishResolve = (message) => {
        if (settled) {
          return;
        }
        settled = true;
        offMessage();
        offClose();
        offError();
        if (timer) {
          window.clearTimeout(timer);
        }
        resolve(message);
      };

      const finishReject = (error) => {
        if (settled) {
          return;
        }
        settled = true;
        offMessage();
        offClose();
        offError();
        if (timer) {
          window.clearTimeout(timer);
        }
        reject(error);
      };

      const offMessage = this.onMessage((message) => {
        if (!matcher(message)) {
          return;
        }

        if (typeof message === "object" && message && (message.error || message.status === "error")) {
          finishReject(buildMessageError(message));
          return;
        }

        finishResolve(message);
      });
      const offClose = this.onClose(() => {
        finishReject(new Error("WebSocket 连接已关闭"));
      });
      const offError = this.onError((event) => {
        finishReject(buildSocketError(event, "WebSocket 连接异常"));
      });

      timer = window.setTimeout(() => {
        finishReject(new Error("等待设备响应超时"));
      }, timeoutMs);

      this.send(payload).catch((error) => {
        finishReject(error);
      });
    }));
  }
}
