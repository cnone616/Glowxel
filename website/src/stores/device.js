import { defineStore } from "pinia";
import { DeviceWebSocket } from "@/utils/deviceWebSocket.js";

const DEVICE_SETTINGS_KEY = "device_ws_settings";
const VALID_BUSINESS_MODES = [
  "clock",
  "animation",
  "theme",
  "canvas",
  "gif_player",
  "led_matrix_showcase",
  "game_screensaver",
  "tetris",
  "tetris_clock",
  "planet_screensaver",
  "eyes",
];

function normalizeHost(host) {
  return String(host || "").trim();
}

function buildEndpoint(host, port, secure) {
  const proto = secure ? "wss" : "ws";
  return `${proto}://${host}:${port}/ws`;
}

function isCommandResponsePayload(data) {
  if (data == null || typeof data !== "object") {
    return false;
  }

  if (data.error || data.status === "error") {
    return true;
  }

  return (
    (data.status === "ok" || data.status === "success") &&
    typeof data.message === "string" &&
    data.message.length > 0
  );
}

export const useDeviceStore = defineStore("device", {
  state: () => ({
    socket: null,
    connected: false,
    connecting: false,
    host: "",
    deviceIp: "",
    port: 80,
    secure: false,
    mode: "",
    businessMode: "",
    effectMode: "",
    brightness: 0,
    width: 0,
    height: 0,
    lastMessage: "",
    error: "",
    deviceInfo: null,
  }),

  actions: {
    isDeviceStatusPayload(data) {
      if (data == null || typeof data !== "object") {
        return false;
      }

      if (data.status !== "ok") {
        return false;
      }

      if (typeof data.ip !== "string") {
        return false;
      }

      if (typeof data.width !== "number") {
        return false;
      }

      if (typeof data.height !== "number") {
        return false;
      }

      if (typeof data.brightness !== "number") {
        return false;
      }

      if (typeof data.mode !== "string") {
        return false;
      }

      if (typeof data.businessMode !== "string") {
        return false;
      }

      if (typeof data.effectMode !== "string") {
        return false;
      }

      return true;
    },
    applyDeviceStatus(status) {
      if (!this.isDeviceStatusPayload(status)) {
        return false;
      }

      this.deviceInfo = status;
      this.deviceIp = status.ip;
      this.width = status.width;
      this.height = status.height;
      this.brightness = status.brightness;
      this.mode = status.mode;
      this.effectMode = status.effectMode;
      if (VALID_BUSINESS_MODES.includes(status.businessMode)) {
        this.businessMode = status.businessMode;
      }
      return true;
    },
    init() {
      const raw = localStorage.getItem(DEVICE_SETTINGS_KEY);
      if (!raw) {
        return;
      }

      try {
        const parsed = JSON.parse(raw);
        if (typeof parsed.host === "string") {
          this.host = parsed.host;
          this.deviceIp = parsed.host;
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
      this.deviceIp = cleanHost;
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
          if (this.isDeviceStatusPayload(message)) {
            this.applyDeviceStatus(message);
            return;
          }
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
        try {
          await this.requestStatus();
        } catch (error) {
          if (!this.connected) {
            throw error;
          }
          this.error =
            error instanceof Error ? error.message : "设备状态读取失败";
        }
      } catch (error) {
        this.disconnect();
        throw error;
      } finally {
        this.connecting = false;
      }
    },

    async restoreConnection() {
      if (this.connected || this.connecting) {
        return true;
      }

      if (!(typeof this.host === "string" && this.host.length > 0)) {
        return false;
      }

      if (!(Number.isInteger(this.port) && this.port > 0 && this.port <= 65535)) {
        return false;
      }

      try {
        await this.connect({
          host: this.host,
          port: this.port,
          secure: this.secure,
        });
        try {
          await this.syncDeviceStatus();
        } catch (error) {
          return this.connected;
        }
        return true;
      } catch (error) {
        return false;
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
      this.businessMode = "";
      this.effectMode = "";
      this.brightness = 0;
      this.width = 0;
      this.height = 0;
      this.deviceInfo = null;
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
      const resp = await this.sendAndWait(
        { cmd: "status" },
        {
          matcher: (message) =>
            this.isDeviceStatusPayload(message) ||
            (message != null &&
              typeof message === "object" &&
              (message.error || message.status === "error")),
        },
      );
      if (this.isDeviceStatusPayload(resp)) {
        this.applyDeviceStatus(resp);
      }
      return resp;
    },

    async syncDeviceStatus() {
      return this.requestStatus();
    },

    async setMode(mode) {
      const resp = await this.sendAndWait(
        { cmd: "set_mode", mode },
        {
          matcher: isCommandResponsePayload,
        },
      );
      this.businessMode = mode;
      return resp;
    },

    async setBrightness(value) {
      const brightness = Number(value);
      if (!Number.isFinite(brightness)) {
        throw new Error("亮度值无效");
      }

      const boundedValue = Math.max(0, Math.min(178, Math.round(brightness)));
      return this.sendAndWait(
        { cmd: "brightness", value: boundedValue },
        {
          matcher: isCommandResponsePayload,
        },
      );
    },

    async clearScreen() {
      return this.sendAndWait(
        { cmd: "clear" },
        {
          matcher: isCommandResponsePayload,
        },
      );
    },

    async setClockConfig(clockMode, config) {
      return this.sendAndWait({
        cmd: "set_clock_config",
        clockMode,
        config,
      }, {
        matcher: isCommandResponsePayload,
      });
    },
  },
});
