import { defineStore } from "pinia";
import { DeviceWebSocket } from "@/utils/deviceWebSocket.js";

const DEVICE_IP_KEY = "device_ip";
const DEVICE_SETTINGS_KEY = "device_ws_settings";
const DEVICE_MODE_KEY = "device_mode";
const DEVICE_LAST_BUSINESS_MODE_KEY = "device_last_business_mode";
const STATUS_SYNC_TIMEOUT_MS = 15000;
const CONNECT_STATUS_VERIFY_TIMEOUT_MS = 4000;
const INITIAL_CONNECT_MAX_ATTEMPTS = 2;
const INITIAL_CONNECT_WS_TIMEOUT_MS = 10000;
const INITIAL_CONNECT_RETRY_DELAY_MS = 1200;
const MAZE_MODE_CONFIG_KEY = "maze_mode_config";
const TETRIS_CONFIG_STORAGE_KEY = "tetris_config";
const TETRIS_CLOCK_STORAGE_KEY = "tetris_clock_config";
const SNAKE_MODE_CONFIG_KEY = "snake_mode_config";
const CLOCK_DEVICE_THEME_ID_KEY = "clock_device_theme_id";

const TETRIS_SPEED_OPTIONS = Object.freeze({
  slow: 300,
  normal: 150,
  fast: 80,
});

const DEFAULT_TETRIS_MODE_PAYLOAD = Object.freeze({
  clearMode: true,
  cellSize: 2,
  speed: 150,
  showClock: true,
  pieces: [0, 1, 2, 3, 4, 5, 6],
});

const DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD = Object.freeze({
  cellSize: 2,
  speed: 150,
  hourFormat: 24,
});

const DEFAULT_SNAKE_MODE_PAYLOAD = Object.freeze({
  speed: 6,
  snakeWidth: 2,
  snakeColor: "#56d678",
  foodColor: "#ffa854",
  font: "minimal_3x5",
  showSeconds: false,
  snakeSkin: "gradient",
});

const DEFAULT_MAZE_MODE_CONFIG = Object.freeze({
  speed: 3,
  mazeSizeMode: "wide",
  showClock: true,
  panelBgColor: "#05070f",
  borderColor: "#182c4c",
  timeColor: "#ffd400",
  dateColor: "#ff6464",
  generationPathColor: "#4f4f55",
  searchVisitedColor: "#ff4444",
  searchFrontierColor: "#70ff9c",
  solvedPathStartColor: "#1a60ff",
  solvedPathEndColor: "#42bcff",
});

const MAZE_COLOR_FIELDS = Object.freeze([
  "panelBgColor",
  "borderColor",
  "timeColor",
  "dateColor",
  "generationPathColor",
  "searchVisitedColor",
  "searchFrontierColor",
  "solvedPathStartColor",
  "solvedPathEndColor",
]);

const VALID_BUSINESS_MODES = Object.freeze([
  "clock",
  "animation",
  "theme",
  "canvas",
  "gif_player",
  "led_matrix_showcase",
  "maze",
  "snake",
  "tetris",
  "tetris_clock",
  "planet_screensaver",
  "eyes",
]);

function getLocalStorageRef() {
  if (typeof globalThis === "undefined") {
    return null;
  }
  if (!("localStorage" in globalThis)) {
    return null;
  }
  return globalThis.localStorage;
}

function readStorageText(key) {
  const storage = getLocalStorageRef();
  if (storage === null) {
    return null;
  }

  const value = storage.getItem(key);
  if (typeof value !== "string") {
    return null;
  }
  return value;
}

function writeStorageText(key, value) {
  const storage = getLocalStorageRef();
  if (storage === null) {
    return;
  }
  storage.setItem(key, value);
}

function readStorageJson(key) {
  const text = readStorageText(key);
  if (text === null) {
    return null;
  }

  try {
    return JSON.parse(text);
  } catch (error) {
    return null;
  }
}

function writeStorageJson(key, value) {
  writeStorageText(key, JSON.stringify(value));
}

function isPersistedBusinessMode(mode) {
  return typeof mode === "string" && VALID_BUSINESS_MODES.includes(mode);
}

function normalizeHexColorText(value) {
  if (typeof value !== "string") {
    return null;
  }

  const body = value.trim().replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    return null;
  }

  return `#${body.toLowerCase()}`;
}

function createDefaultMazeModeConfig() {
  return {
    speed: DEFAULT_MAZE_MODE_CONFIG.speed,
    mazeSizeMode: DEFAULT_MAZE_MODE_CONFIG.mazeSizeMode,
    showClock: DEFAULT_MAZE_MODE_CONFIG.showClock,
    panelBgColor: DEFAULT_MAZE_MODE_CONFIG.panelBgColor,
    borderColor: DEFAULT_MAZE_MODE_CONFIG.borderColor,
    timeColor: DEFAULT_MAZE_MODE_CONFIG.timeColor,
    dateColor: DEFAULT_MAZE_MODE_CONFIG.dateColor,
    generationPathColor: DEFAULT_MAZE_MODE_CONFIG.generationPathColor,
    searchVisitedColor: DEFAULT_MAZE_MODE_CONFIG.searchVisitedColor,
    searchFrontierColor: DEFAULT_MAZE_MODE_CONFIG.searchFrontierColor,
    solvedPathStartColor: DEFAULT_MAZE_MODE_CONFIG.solvedPathStartColor,
    solvedPathEndColor: DEFAULT_MAZE_MODE_CONFIG.solvedPathEndColor,
  };
}

function createMazeModeConfig(source) {
  if (!source || typeof source !== "object") {
    return null;
  }

  const speed = Number(source.speed);
  if (!Number.isInteger(speed) || speed < 1 || speed > 10) {
    return null;
  }
  if (source.mazeSizeMode !== "wide" && source.mazeSizeMode !== "dense") {
    return null;
  }
  if (source.showClock !== true && source.showClock !== false) {
    return null;
  }

  const normalized = {
    speed,
    mazeSizeMode: source.mazeSizeMode,
    showClock: source.showClock,
  };

  for (let index = 0; index < MAZE_COLOR_FIELDS.length; index += 1) {
    const field = MAZE_COLOR_FIELDS[index];
    const value = normalizeHexColorText(source[field]);
    if (value === null) {
      return null;
    }
    normalized[field] = value;
  }

  return normalized;
}

function readSavedMazeModeConfig() {
  return createMazeModeConfig(readStorageJson(MAZE_MODE_CONFIG_KEY));
}

function normalizeStoredTetrisModePayload(saved) {
  const normalized = {
    clearMode: DEFAULT_TETRIS_MODE_PAYLOAD.clearMode,
    cellSize: DEFAULT_TETRIS_MODE_PAYLOAD.cellSize,
    speed: DEFAULT_TETRIS_MODE_PAYLOAD.speed,
    showClock: DEFAULT_TETRIS_MODE_PAYLOAD.showClock,
    pieces: DEFAULT_TETRIS_MODE_PAYLOAD.pieces.slice(),
  };

  if (!saved || typeof saved !== "object") {
    return normalized;
  }

  if (typeof saved.clearMode === "boolean") {
    normalized.clearMode = saved.clearMode;
  }
  if (saved.cellSize === 1 || saved.cellSize === 2 || saved.cellSize === 3) {
    normalized.cellSize = saved.cellSize;
  }
  if (saved.speed === "slow" || saved.speed === "normal" || saved.speed === "fast") {
    normalized.speed = TETRIS_SPEED_OPTIONS[saved.speed];
  }
  if (typeof saved.showClock === "boolean") {
    normalized.showClock = saved.showClock;
  }
  if (Array.isArray(saved.pieces) && saved.pieces.length > 0) {
    const pieces = saved.pieces.filter((piece) => {
      return Number.isInteger(piece) && piece >= 0 && piece <= 6;
    });
    if (pieces.length > 0) {
      normalized.pieces = pieces;
    }
  }

  return normalized;
}

function normalizeStoredTetrisClockModePayload(saved) {
  const normalized = {
    cellSize: DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD.cellSize,
    speed: DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD.speed,
    hourFormat: DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD.hourFormat,
  };

  if (!saved || typeof saved !== "object") {
    return normalized;
  }

  if (saved.speed === "slow" || saved.speed === "normal" || saved.speed === "fast") {
    normalized.speed = TETRIS_SPEED_OPTIONS[saved.speed];
  }
  if (saved.hourFormat === 12 || saved.hourFormat === 24) {
    normalized.hourFormat = saved.hourFormat;
  }

  return normalized;
}

function getStoredTetrisModePayload() {
  return normalizeStoredTetrisModePayload(readStorageJson(TETRIS_CONFIG_STORAGE_KEY));
}

function getStoredTetrisClockModePayload() {
  return normalizeStoredTetrisClockModePayload(readStorageJson(TETRIS_CLOCK_STORAGE_KEY));
}

function normalizeStoredSnakeModePayload(saved) {
  const normalized = {
    speed: DEFAULT_SNAKE_MODE_PAYLOAD.speed,
    snakeWidth: DEFAULT_SNAKE_MODE_PAYLOAD.snakeWidth,
    snakeColor: DEFAULT_SNAKE_MODE_PAYLOAD.snakeColor,
    foodColor: DEFAULT_SNAKE_MODE_PAYLOAD.foodColor,
    font: DEFAULT_SNAKE_MODE_PAYLOAD.font,
    showSeconds: DEFAULT_SNAKE_MODE_PAYLOAD.showSeconds,
    snakeSkin: DEFAULT_SNAKE_MODE_PAYLOAD.snakeSkin,
  };

  if (!saved || typeof saved !== "object") {
    return normalized;
  }

  if (Number.isFinite(Number(saved.speed))) {
    normalized.speed = Number(saved.speed);
  }
  if (Number.isFinite(Number(saved.snakeWidth))) {
    normalized.snakeWidth = Number(saved.snakeWidth);
  }
  if (typeof saved.snakeColor === "string") {
    normalized.snakeColor = saved.snakeColor;
  }
  if (typeof saved.foodColor === "string") {
    normalized.foodColor = saved.foodColor;
  }
  if (typeof saved.font === "string") {
    normalized.font = saved.font;
  }
  if (saved.showSeconds === true || saved.showSeconds === false) {
    normalized.showSeconds = saved.showSeconds;
  }
  if (
    saved.snakeSkin === "solid" ||
    saved.snakeSkin === "gradient" ||
    saved.snakeSkin === "spotted"
  ) {
    normalized.snakeSkin = saved.snakeSkin;
  }

  return normalized;
}

function getStoredSnakeModePayload() {
  return normalizeStoredSnakeModePayload(readStorageJson(SNAKE_MODE_CONFIG_KEY));
}

function delay(ms) {
  return new Promise((resolve) => {
    globalThis.setTimeout(resolve, ms);
  });
}

function toError(error, fallbackMessage) {
  if (error instanceof Error) {
    return error;
  }

  if (error && typeof error.message === "string" && error.message.length > 0) {
    return new Error(error.message);
  }

  return new Error(fallbackMessage);
}

function readConnectionSettings() {
  const saved = readStorageJson(DEVICE_SETTINGS_KEY);
  if (!saved || typeof saved !== "object") {
    return null;
  }

  const normalized = {
    host: "",
    port: 80,
    secure: false,
  };

  if (typeof saved.host === "string" && saved.host.length > 0) {
    normalized.host = saved.host;
  }
  if (Number.isInteger(saved.port) && saved.port > 0 && saved.port <= 65535) {
    normalized.port = saved.port;
  }
  if (saved.secure === true || saved.secure === false) {
    normalized.secure = saved.secure;
  }

  return normalized;
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
    deviceMode: "clock",
    lastBusinessMode: "clock",
    brightness: 0,
    width: 0,
    height: 0,
    lastMessage: "",
    error: null,
    deviceInfo: null,
    statusSyncInFlight: false,
  }),

  getters: {
    isConnected: (state) => state.connected,
    getDeviceIp: (state) => state.deviceIp,
    getDeviceInfo: (state) => state.deviceInfo,
  },

  actions: {
    isDeviceStatusPayload(data) {
      if (!data || typeof data !== "object") {
        return false;
      }

      return (
        data.status === "ok" &&
        typeof data.ip === "string" &&
        typeof data.width === "number" &&
        typeof data.height === "number" &&
        typeof data.brightness === "number" &&
        typeof data.mode === "string" &&
        typeof data.businessMode === "string" &&
        typeof data.effectMode === "string"
      );
    },

    resolveDeviceModeFromStatus(data) {
      if (!data || typeof data !== "object") {
        return null;
      }

      if (
        typeof data.businessMode === "string" &&
        VALID_BUSINESS_MODES.includes(data.businessMode)
      ) {
        return data.businessMode;
      }

      return null;
    },

    applyResolvedBusinessMode(mode) {
      if (!isPersistedBusinessMode(mode)) {
        return false;
      }

      this.deviceMode = mode;
      this.businessMode = mode;
      writeStorageText(DEVICE_MODE_KEY, mode);

      if (mode !== "canvas") {
        this.lastBusinessMode = mode;
        writeStorageText(DEVICE_LAST_BUSINESS_MODE_KEY, mode);
      }

      return true;
    },

    applyDeviceStatus(status) {
      if (!this.isDeviceStatusPayload(status)) {
        return false;
      }

      this.deviceInfo = status;
      this.deviceIp = status.ip;
      this.host = status.ip;
      this.width = status.width;
      this.height = status.height;
      this.brightness = status.brightness;
      this.mode = status.mode;
      this.businessMode = status.businessMode;
      this.effectMode = status.effectMode;

      const resolvedMode = this.resolveDeviceModeFromStatus(status);
      if (resolvedMode !== null) {
        this.applyResolvedBusinessMode(resolvedMode);
      }

      return true;
    },

    init() {
      if (this.socket === null) {
        this.socket = new DeviceWebSocket();
        this.setupCallbacks();
      }

      const savedConnection = readConnectionSettings();
      if (savedConnection !== null) {
        if (savedConnection.host.length > 0) {
          this.host = savedConnection.host;
          this.deviceIp = savedConnection.host;
        }
        this.port = savedConnection.port;
        this.secure = savedConnection.secure;
      }

      const savedIp = readStorageText(DEVICE_IP_KEY);
      if (savedIp !== null && savedIp.length > 0) {
        this.deviceIp = savedIp;
        this.host = savedIp;
      }

      const savedMode = readStorageText(DEVICE_MODE_KEY);
      if (isPersistedBusinessMode(savedMode)) {
        this.deviceMode = savedMode;
        this.businessMode = savedMode;
      }

      const savedBusinessMode = readStorageText(DEVICE_LAST_BUSINESS_MODE_KEY);
      if (isPersistedBusinessMode(savedBusinessMode)) {
        this.lastBusinessMode = savedBusinessMode;
      } else if (isPersistedBusinessMode(savedMode) && savedMode !== "canvas") {
        this.lastBusinessMode = savedMode;
      }
    },

    setupCallbacks() {
      this.socket.onConnect(() => {
        this.connected = true;
        this.connecting = false;
        this.error = null;
      });

      this.socket.onDisconnect(() => {
        this.connected = false;
        this.connecting = false;
        this.deviceInfo = null;
        this.statusSyncInFlight = false;
        this.mode = "";
        this.businessMode = "";
        this.effectMode = "";
        this.width = 0;
        this.height = 0;
        this.brightness = 0;
      });

      this.socket.onError((error) => {
        this.error = toError(error, "设备连接异常");
      });

      this.socket.onMessage((data) => {
        if (typeof data === "string") {
          this.lastMessage = data;
          return;
        }
        if (!data || typeof data !== "object") {
          return;
        }

        if (data.status === "connected") {
          if (typeof data.mode === "string") {
            this.mode = data.mode;
          }
          if (typeof data.businessMode === "string") {
            this.businessMode = data.businessMode;
            const resolvedMode = this.resolveDeviceModeFromStatus(data);
            if (resolvedMode !== null) {
              this.applyResolvedBusinessMode(resolvedMode);
            }
          }
          if (typeof data.effectMode === "string") {
            this.effectMode = data.effectMode;
          }
          return;
        }

        if (
          (data.status === "final_ok" || data.status === "final_error") &&
          typeof data.activeMode === "string"
        ) {
          this.applyResolvedBusinessMode(data.activeMode);
        }

        if (typeof data.message === "string") {
          this.lastMessage = data.message;
        }
        if (typeof data.error === "string") {
          this.error = new Error(data.error);
        }

        if (this.isDeviceStatusPayload(data)) {
          this.applyDeviceStatus(data);
        }
      });
    },

    async syncDeviceStatus(options = {}) {
      if (this.socket === null || !this.connected || this.statusSyncInFlight) {
        return null;
      }

      const timeout =
        Number.isInteger(options.timeout) && options.timeout > 0
          ? options.timeout
          : STATUS_SYNC_TIMEOUT_MS;

      this.statusSyncInFlight = true;
      try {
        const status = await this.socket.getStatus(timeout);
        if (!this.connected) {
          return null;
        }
        if (!this.isDeviceStatusPayload(status)) {
          return null;
        }

        this.applyDeviceStatus(status);
        return status;
      } catch (error) {
        this.error = toError(error, "同步设备状态失败");
        throw this.error;
      } finally {
        this.statusSyncInFlight = false;
      }
    },

    async rollbackBusinessMode(previousMode, options = {}) {
      if (this.socket === null || !this.connected || !isPersistedBusinessMode(previousMode)) {
        return false;
      }

      if (
        typeof options.expectedMode === "string" &&
        options.expectedMode === previousMode
      ) {
        return false;
      }

      if (this.deviceMode === previousMode) {
        return false;
      }

      try {
        if (
          previousMode === "clock" ||
          previousMode === "canvas" ||
          previousMode === "animation" ||
          previousMode === "gif_player" ||
          previousMode === "theme" ||
          previousMode === "led_matrix_showcase" ||
          previousMode === "planet_screensaver" ||
          previousMode === "eyes"
        ) {
          await this.socket.runModeTransaction({
            mode: previousMode,
            params: {},
          });
        } else if (previousMode === "maze") {
          const savedMazeConfig = readSavedMazeModeConfig();
          const mazePayload =
            savedMazeConfig === null ? createDefaultMazeModeConfig() : savedMazeConfig;
          await this.socket.startMaze(mazePayload);
        } else if (previousMode === "snake") {
          await this.socket.startSnake(getStoredSnakeModePayload());
        } else if (previousMode === "tetris") {
          await this.socket.startTetris(getStoredTetrisModePayload());
        } else if (previousMode === "tetris_clock") {
          await this.socket.startTetrisClock(getStoredTetrisClockModePayload());
        } else {
          return false;
        }
        return true;
      } catch (error) {
        this.error = toError(error, "恢复上一次模式失败");
        return false;
      }
    },

    async connect(connection) {
      if (this.socket === null) {
        this.init();
      }

      if (!connection || typeof connection !== "object") {
        throw new Error("连接参数无效");
      }

      const cleanHost = this.socket.normalizeHostInput(connection.host);
      if (cleanHost.length === 0) {
        throw new Error("请输入设备 IP 或域名");
      }

      const parsedPort = Number(connection.port);
      if (!Number.isInteger(parsedPort) || parsedPort <= 0 || parsedPort > 65535) {
        throw new Error("端口范围应为 1-65535");
      }

      const secure = connection.secure === true;

      if (
        this.connected &&
        this.host === cleanHost &&
        this.port === parsedPort &&
        this.secure === secure
      ) {
        const status = await this.syncDeviceStatus({
          timeout: CONNECT_STATUS_VERIFY_TIMEOUT_MS,
        });
        return { success: true, status };
      }

      if (this.connected) {
        this.disconnect();
      }

      this.connecting = true;
      this.error = null;
      this.lastMessage = "";
      this.host = cleanHost;
      this.deviceIp = cleanHost;
      this.port = parsedPort;
      this.secure = secure;
      writeStorageText(DEVICE_IP_KEY, cleanHost);
      writeStorageJson(DEVICE_SETTINGS_KEY, {
        host: cleanHost,
        port: parsedPort,
        secure,
      });

      let lastError = null;
      try {
        for (let attempt = 1; attempt <= INITIAL_CONNECT_MAX_ATTEMPTS; attempt += 1) {
          try {
            await this.socket.connect(cleanHost, parsedPort, {
              connectTimeoutMs: INITIAL_CONNECT_WS_TIMEOUT_MS,
              secure,
            });

            const status = await this.syncDeviceStatus({
              timeout: CONNECT_STATUS_VERIFY_TIMEOUT_MS,
            });

            if (status === null) {
              throw new Error("WebSocket 已连接，但设备状态同步失败");
            }

            return { success: true, status };
          } catch (error) {
            lastError = toError(error, "设备连接失败");
            this.socket.disconnect();
            this.connected = false;
            if (attempt < INITIAL_CONNECT_MAX_ATTEMPTS) {
              await delay(INITIAL_CONNECT_RETRY_DELAY_MS);
            }
          }
        }

        throw lastError === null ? new Error("设备连接失败") : lastError;
      } finally {
        this.connecting = false;
      }
    },

    async restoreConnection() {
      if (this.connected || this.connecting) {
        return true;
      }

      if (typeof this.host !== "string" || this.host.length === 0) {
        return false;
      }
      if (!Number.isInteger(this.port) || this.port <= 0 || this.port > 65535) {
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
      if (this.socket !== null) {
        this.socket.disconnect();
      }
      this.connected = false;
      this.connecting = false;
      this.mode = "";
      this.businessMode = "";
      this.effectMode = "";
      this.brightness = 0;
      this.width = 0;
      this.height = 0;
      this.deviceInfo = null;
      this.statusSyncInFlight = false;
    },

    async send(payload) {
      if (this.socket === null) {
        throw new Error("设备未连接");
      }
      return this.socket.send(payload);
    },

    async sendBinary(payload) {
      if (this.socket === null) {
        throw new Error("设备未连接");
      }
      return this.socket.sendBinary(payload);
    },

    async sendAndWait(payload, timeout = 15000, matcher = null, options = {}) {
      if (this.socket === null) {
        throw new Error("设备未连接");
      }
      return this.socket.sendAndWait(payload, timeout, matcher, options);
    },

    async waitForMessage(predicate, options = {}) {
      if (this.socket === null) {
        throw new Error("设备未连接");
      }

      const timeout =
        Number.isInteger(options.timeout) && options.timeout > 0
          ? options.timeout
          : 15000;
      return this.socket.waitForMessage(predicate, timeout);
    },

    async requestStatus() {
      if (this.socket === null) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.getStatus();
      if (this.isDeviceStatusPayload(response)) {
        this.applyDeviceStatus(response);
      }
      return response;
    },

    async getInfo() {
      if (this.socket === null) {
        throw new Error("设备未连接");
      }
      return this.socket.getInfo();
    },

    async runModeTransaction(options = {}) {
      if (this.socket === null) {
        throw new Error("设备未连接");
      }
      return this.socket.runModeTransaction(options);
    },

    async setMode(mode, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.setMode(mode, options);
      if (isPersistedBusinessMode(mode)) {
        this.applyResolvedBusinessMode(mode);
      }
      return response;
    },

    async setClockConfig(clockMode, config) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }
      return this.socket.setClockConfig(clockMode, config);
    },

    async applyClockMode(mode, config, binary = null, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.applyClockMode(mode, config, binary, options);
      if (isPersistedBusinessMode(mode)) {
        this.applyResolvedBusinessMode(mode);
      }
      return response;
    },

    async applyThemeMode(config, themeId, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.applyThemeMode(config, themeId, options);
      writeStorageText(CLOCK_DEVICE_THEME_ID_KEY, themeId);
      this.applyResolvedBusinessMode("theme");
      return response;
    },

    async ensureCanvasMode(options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.ensureCanvasMode(options);
      this.applyResolvedBusinessMode("canvas");
      return response;
    },

    async setThemeConfig(themeId, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.setThemeConfig(themeId, options);
      writeStorageText(CLOCK_DEVICE_THEME_ID_KEY, themeId);
      this.applyResolvedBusinessMode("theme");
      return response;
    },

    async setEyesConfig(config) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.setEyesConfig(config);
      this.applyResolvedBusinessMode("eyes");
      return response;
    },

    async setAmbientEffect(config, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.setAmbientEffect(config, options);
      this.applyResolvedBusinessMode("led_matrix_showcase");
      return response;
    },

    async startMaze(config, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.startMaze(config, options);
      this.applyResolvedBusinessMode("maze");
      return response;
    },

    async startSnake(config, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.startSnake(config, options);
      this.applyResolvedBusinessMode("snake");
      return response;
    },

    async startTetris(config, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.startTetris(config, options);
      this.applyResolvedBusinessMode("tetris");
      return response;
    },

    async startTetrisClock(config, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.startTetrisClock(config, options);
      this.applyResolvedBusinessMode("tetris_clock");
      return response;
    },

    async setPlanetScreensaver(config, options = {}) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.setPlanetScreensaver(config, options);
      this.applyResolvedBusinessMode("planet_screensaver");
      return response;
    },

    async setGifAnimation(animationData) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.setGifAnimation(animationData);
      if (animationData !== null) {
        this.applyResolvedBusinessMode("gif_player");
      }
      return response;
    },

    async showImage(pixels, width, height) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.showImage(pixels, width, height);
      this.applyResolvedBusinessMode("canvas");
      return response;
    },

    async sendImage(pixels, width, height) {
      return this.showImage(pixels, width, height);
    },

    async showSparseImage(sparsePixels, width, height) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const response = await this.socket.showSparseImage(sparsePixels, width, height);
      this.applyResolvedBusinessMode("canvas");
      return response;
    },

    async sendSparseImage(sparsePixels, width, height) {
      return this.showSparseImage(sparsePixels, width, height);
    },

    async setBrightness(value) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      const brightness = Number(value);
      if (!Number.isFinite(brightness)) {
        throw new Error("亮度值无效");
      }

      const boundedValue = Math.max(0, Math.min(178, Math.round(brightness)));
      await this.socket.setBrightness(boundedValue);
      this.brightness = boundedValue;
      return { success: true, value: boundedValue };
    },

    async getDeviceParams() {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }
      return this.socket.getDeviceParams();
    },

    async setDeviceParam(key, value) {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }
      return this.socket.setDeviceParam(key, value);
    },

    async clearWifiConfig() {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }
      return this.socket.clearWifiConfig();
    },

    async clear() {
      if (this.socket === null || !this.connected) {
        throw new Error("设备未连接");
      }

      await this.socket.clear();
      return { success: true };
    },

    buildPixelBinaryFromObjects(pixels) {
      if (this.socket === null) {
        this.init();
      }
      return this.socket.buildPixelBinaryFromObjects(pixels);
    },

    buildCompactAnimationBinaryBuffer(animationData) {
      if (this.socket === null) {
        this.init();
      }
      return this.socket.buildCompactAnimationBinaryBuffer(animationData);
    },

    async clearScreen() {
      return this.clear();
    },

    async probeRuntimeStatus(options = {}) {
      if (this.socket === null) {
        this.init();
      }

      const host =
        typeof options.host === "string" && options.host.length > 0
          ? options.host
          : this.host;
      const port =
        Number.isInteger(options.port) && options.port > 0 ? options.port : this.port;
      const timeout =
        Number.isInteger(options.timeout) && options.timeout > 0
          ? options.timeout
          : 4000;
      const secure = options.secure === true ? true : this.secure;

      return this.socket.probeRuntimeStatus(host, port, timeout, { secure });
    },

    getWebSocket() {
      if (this.socket === null) {
        this.init();
      }
      return this.socket;
    },
  },
});
