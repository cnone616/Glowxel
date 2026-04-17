import { defineStore } from "pinia";
import { DeviceWebSocket } from "@/utils/deviceWebSocket.js";

const DEVICE_SETTINGS_KEY = "device_ws_settings";

function normalizeHost(host) {
  return String(host || "").trim();
}

function buildEndpoint(host, port, secure) {
  const proto = secure ? "wss" : "ws";
  return `${proto}://${host}:${port}/ws`;
}

export const useDeviceStore = defineStore("device", {
  state: () => ({
    socket: null,
    connected: false,
    connecting: false,
    host: "",
    port: 80,
    secure: false,
    mode: "",
    lastMessage: "",
    error: "",
  }),

  actions: {
    init() {
      const raw = localStorage.getItem(DEVICE_SETTINGS_KEY);
      if (!raw) {
        return;
      }

      try {
        const parsed = JSON.parse(raw);
        if (typeof parsed.host === "string") {
          this.host = parsed.host;
        }
        if (typeof parsed.port === "number") {
          this.port = parsed.port;
        }
        if (typeof parsed.secure === "boolean") {
          this.secure = parsed.secure;
        }
      } catch (error) {
        // ignore malformed storage
      }
    },

    saveSettings() {
      localStorage.setItem(
        DEVICE_SETTINGS_KEY,
        JSON.stringify({
          host: this.host,
          port: this.port,
          secure: this.secure,
        }),
      );
    },

    async connect({ host, port, secure }) {
      const cleanHost = normalizeHost(host);
      if (!cleanHost) {
        throw new Error("请输入设备 IP 或域名");
      }

      const parsedPort = Number(port);
      if (!Number.isInteger(parsedPort) || parsedPort <= 0 || parsedPort > 65535) {
        throw new Error("端口范围应为 1-65535");
      }

      this.disconnect();
      this.connecting = true;
      this.error = "";
      this.lastMessage = "";
      this.host = cleanHost;
      this.port = parsedPort;
      this.secure = !!secure;
      this.saveSettings();

      const endpoint = buildEndpoint(this.host, this.port, this.secure);
      const socket = new DeviceWebSocket(endpoint);

      socket.onOpen(() => {
        this.connected = true;
        this.connecting = false;
      });

      socket.onClose(() => {
        this.connected = false;
        this.connecting = false;
      });

      socket.onError(() => {
        this.error = "连接异常，请检查设备地址或网络";
      });

      socket.onMessage((message) => {
        if (typeof message === "object" && message) {
          if (typeof message.message === "string") {
            this.lastMessage = message.message;
          }
          if (typeof message.mode === "string") {
            this.mode = message.mode;
          }
          if (typeof message.error === "string") {
            this.error = message.error;
            return;
          }
          if (message.status === "error" && typeof message.message === "string") {
            this.error = message.message;
          }
        } else if (typeof message === "string") {
          this.lastMessage = message;
        }
      });

      this.socket = socket;
      try {
        await socket.connect();
        await this.requestStatus();
      } catch (error) {
        this.disconnect();
        throw error;
      } finally {
        this.connecting = false;
      }
    },

    disconnect() {
      if (this.socket) {
        this.socket.disconnect();
      }
      this.socket = null;
      this.connected = false;
      this.connecting = false;
      this.mode = "";
    },

    async send(payload) {
      if (!this.socket) {
        throw new Error("设备未连接");
      }
      await this.socket.send(payload);
    },

    async sendBinary(payload) {
      if (!this.socket) {
        throw new Error("设备未连接");
      }
      await this.socket.sendBinary(payload);
    },

    async sendAndWait(payload, options = {}) {
      if (!this.socket) {
        throw new Error("设备未连接");
      }
      return this.socket.sendAndWait(payload, options);
    },

    async waitForMessage(predicate, options = {}) {
      if (!this.socket) {
        throw new Error("设备未连接");
      }
      return this.socket.waitForMessage(predicate, options);
    },

    async requestStatus() {
      const resp = await this.sendAndWait({ cmd: "status" });
      if (resp && typeof resp === "object" && typeof resp.mode === "string") {
        this.mode = resp.mode;
      }
      return resp;
    },

    async setMode(mode) {
      const resp = await this.sendAndWait({ cmd: "set_mode", mode });
      this.mode = mode;
      return resp;
    },

    async setBrightness(value) {
      const brightness = Number(value);
      if (!Number.isFinite(brightness)) {
        throw new Error("亮度值无效");
      }

      const boundedValue = Math.max(0, Math.min(178, Math.round(brightness)));
      return this.sendAndWait({ cmd: "brightness", value: boundedValue });
    },

    async clearScreen() {
      return this.sendAndWait({ cmd: "clear" });
    },

    async setClockConfig(clockMode, config) {
      return this.sendAndWait({
        cmd: "set_clock_config",
        clockMode,
        config,
      });
    },
  },
});
