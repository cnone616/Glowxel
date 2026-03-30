const DEFAULT_IGNORE_MESSAGES = new Set([
  "pong",
  "loading displayed",
  "loading started",
  "loading stopped",
  "pixels_received",
]);

export class DeviceWebSocket {
  constructor(endpoint) {
    this.endpoint = endpoint;
    this.socket = null;
    this.connected = false;
    this._messageHandlers = new Set();
    this._openHandlers = new Set();
    this._closeHandlers = new Set();
    this._errorHandlers = new Set();
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

  connect({ timeoutMs = 8000 } = {}) {
    if (this.connected && this.socket && this.socket.readyState === WebSocket.OPEN) {
      return Promise.resolve();
    }

    if (this.socket) {
      this.disconnect();
    }

    return new Promise((resolve, reject) => {
      const ws = new WebSocket(this.endpoint);
      let timer = null;

      const cleanup = () => {
        if (timer) {
          window.clearTimeout(timer);
          timer = null;
        }
      };

      ws.onopen = () => {
        cleanup();
        this.socket = ws;
        this.connected = true;
        this._openHandlers.forEach((handler) => handler());
        resolve();
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
        this.socket = null;
        this._closeHandlers.forEach((handler) => handler());
      };

      ws.onerror = (event) => {
        this._errorHandlers.forEach((handler) => handler(event));
      };

      timer = window.setTimeout(() => {
        try {
          ws.close();
        } catch (error) {
          // ignore
        }
        reject(new Error("连接设备超时"));
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
      const off = this.onMessage((message) => {
        if (settled) {
          return;
        }
        if (!predicate(message)) {
          return;
        }
        settled = true;
        off();
        if (timer !== null) {
          window.clearTimeout(timer);
        }
        resolve(message);
      });

      const timer = window.setTimeout(() => {
        if (settled) {
          return;
        }
        settled = true;
        off();
        reject(new Error("等待设备响应超时"));
      }, timeoutMs);
    });
  }

  sendAndWait(payload, options = {}) {
    const timeoutMs = options.timeoutMs || 10000;
    const ignoreMessages = new Set(options.ignoreMessages || DEFAULT_IGNORE_MESSAGES);

    return new Promise((resolve, reject) => {
      let timer = null;
      let settled = false;

      const off = this.onMessage((message) => {
        if (settled) {
          return;
        }

        if (typeof message === "object" && message) {
          const text = typeof message.message === "string" ? message.message : "";
          if (ignoreMessages.has(text)) {
            return;
          }
          const hasError = message.error || message.status === "error";
          if (hasError) {
            settled = true;
            off();
            if (timer) {
              window.clearTimeout(timer);
            }
            const errorText =
              typeof message.error === "string" && message.error
                ? message.error
                : typeof message.message === "string" && message.message
                  ? message.message
                  : "设备返回错误";
            reject(new Error(`${errorText}${message.details ? ` - ${message.details}` : ""}`));
            return;
          }
        }

        settled = true;
        off();
        if (timer) {
          window.clearTimeout(timer);
        }
        resolve(message);
      });

      timer = window.setTimeout(() => {
        if (settled) {
          return;
        }
        settled = true;
        off();
        reject(new Error("等待设备响应超时"));
      }, timeoutMs);

      this.send(payload).catch((error) => {
        if (settled) {
          return;
        }
        settled = true;
        off();
        if (timer) {
          window.clearTimeout(timer);
        }
        reject(error);
      });
    });
  }
}
