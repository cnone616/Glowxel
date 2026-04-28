import { ARTKAL_COLORS_FULL } from "../../../data/artkal-colors-full.js";
import {
  createDefaultMazeModeConfig,
  readSavedMazeModeConfig,
} from "../../../utils/mazeModeConfig.js";

const TETRIS_MODE_PAYLOAD = Object.freeze({
  clearMode: true,
  cellSize: 2,
  speed: 150,
  showClock: true,
  pieces: [0, 1, 2, 3, 4, 5, 6],
});

const TETRIS_CONFIG_STORAGE_KEY = "tetris_config";
const TETRIS_CLOCK_STORAGE_KEY = "tetris_clock_config";
const TETRIS_CLOCK_SPEED_OPTIONS = Object.freeze({
  slow: 300,
  normal: 150,
  fast: 80,
});
const DEFAULT_TETRIS_MODE_PAYLOAD = Object.freeze({
  clearMode: TETRIS_MODE_PAYLOAD.clearMode,
  cellSize: TETRIS_MODE_PAYLOAD.cellSize,
  speed: TETRIS_MODE_PAYLOAD.speed,
  showClock: TETRIS_MODE_PAYLOAD.showClock,
  pieces: TETRIS_MODE_PAYLOAD.pieces,
});
const DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD = Object.freeze({
  cellSize: 2,
  speed: 150,
  hourFormat: 24,
});
const SNAKE_MODE_CONFIG_KEY = "snake_mode_config";
const DEFAULT_SNAKE_MODE_PAYLOAD = Object.freeze({
  speed: 6,
  snakeWidth: 2,
  snakeColor: "#56d678",
  foodColor: "#ffa854",
  font: "minimal_3x5",
  showSeconds: false,
  snakeSkin: "gradient",
});

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
  if (
    saved.speed === "slow" ||
    saved.speed === "normal" ||
    saved.speed === "fast"
  ) {
    normalized.speed = TETRIS_CLOCK_SPEED_OPTIONS[saved.speed];
  }
  if (typeof saved.showClock === "boolean") {
    normalized.showClock = saved.showClock;
  }
  if (Array.isArray(saved.pieces) && saved.pieces.length > 0) {
    const pieces = saved.pieces.filter(
      (piece) => Number.isInteger(piece) && piece >= 0 && piece <= 6,
    );
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
    normalized.speed = TETRIS_CLOCK_SPEED_OPTIONS[saved.speed];
  }
  if (saved.hourFormat === 12 || saved.hourFormat === 24) {
    normalized.hourFormat = saved.hourFormat;
  }

  return normalized;
}

function getStoredTetrisClockModePayload() {
  return normalizeStoredTetrisClockModePayload(
    uni.getStorageSync(TETRIS_CLOCK_STORAGE_KEY),
  );
}

function getStoredTetrisModePayload() {
  return normalizeStoredTetrisModePayload(
    uni.getStorageSync(TETRIS_CONFIG_STORAGE_KEY),
  );
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
  return normalizeStoredSnakeModePayload(
    uni.getStorageSync(SNAKE_MODE_CONFIG_KEY),
  );
}

function hexToRgb(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result
    ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16),
      }
    : { r: 0, g: 0, b: 0 };
}

export default {
  methods: {
    getBusinessModeForCanvasRestore() {
      return this.deviceStore.lastBusinessMode;
    },

    toggleDeviceSync() {
      if (this.deviceConnected) {
        this.deviceStore.disconnect();
        this.deviceCanvasSynced = false;
        this.toast.showInfo("设备已断开");
      } else {
        this.showConnectModal = true;
      }
    },

    async ensureCanvasModeAndSync() {
      const previousMode = this.deviceStore.deviceMode;
      try {
        await this.deviceStore.ensureCanvasMode();
        await this.syncToDevice();
        this.deviceCanvasSynced = true;
      } catch (err) {
        this.deviceCanvasSynced = false;
        await this.deviceStore.rollbackBusinessMode(previousMode, {
          expectedMode: "canvas",
        });
        console.error("切换模式或同步失败:", err);
        this.toast.showError("同步失败");
      }
    },

    async syncConnectedCanvas() {
      if (!this.deviceConnected) {
        this.showConnectModal = true;
        return;
      }

      await this.ensureCanvasModeAndSync();
    },

    async handleConnectConfirm(ip) {
      try {
        const result = await this.deviceStore.connect(ip);
        if (result.success) {
          this.deviceCanvasSynced = false;
          this.$refs.connectModal.onSuccess();
          this.toast.showSuccess("设备已连接");
        } else {
          this.$refs.connectModal.onError("连接失败，请检查 IP 地址");
        }
      } catch (err) {
        console.error("连接失败:", err);
        this.$refs.connectModal.onError("连接失败，请检查 IP 地址");
      }
    },

    handleConnectCancel() {
      // 取消连接
    },

    async restoreModeAfterCanvas() {
      if (this._modeRestoredOnExit) {
        return;
      }
      this._modeRestoredOnExit = true;

      if (!this.deviceCanvasSynced) {
        return;
      }

      const restoreMode = this.getBusinessModeForCanvasRestore();

      if (!this.deviceConnected) {
        return;
      }

      try {
        const ws = this.deviceStore.getWebSocket();
        if (!ws) {
          return;
        }

        if (restoreMode === "tetris") {
          const tetrisPayload = getStoredTetrisModePayload();
          await ws.startTetris({
            clearMode: tetrisPayload.clearMode,
            cellSize: tetrisPayload.cellSize,
            speed: tetrisPayload.speed,
            showClock: tetrisPayload.showClock,
            pieces: tetrisPayload.pieces.slice(),
          });
          return;
        }

        if (restoreMode === "tetris_clock") {
          const tetrisClockPayload = getStoredTetrisClockModePayload();
          await ws.startTetrisClock({
            cellSize: tetrisClockPayload.cellSize,
            speed: tetrisClockPayload.speed,
            hourFormat: tetrisClockPayload.hourFormat,
          });
          return;
        }

        if (restoreMode === "maze") {
          const savedMazeConfig = readSavedMazeModeConfig();
          let mazePayload = createDefaultMazeModeConfig();
          if (savedMazeConfig) {
            mazePayload = savedMazeConfig;
          }
          await ws.startMaze(mazePayload);
          return;
        }

        if (restoreMode === "snake") {
          const snakePayload = getStoredSnakeModePayload();
          await ws.startSnake({
            speed: snakePayload.speed,
            snakeWidth: snakePayload.snakeWidth,
            snakeColor: snakePayload.snakeColor,
            foodColor: snakePayload.foodColor,
            font: snakePayload.font,
            showSeconds: snakePayload.showSeconds,
            snakeSkin: snakePayload.snakeSkin,
          });
          return;
        }

        if (restoreMode === "eyes") {
          await ws.runModeTransaction({
            mode: restoreMode,
            params: {},
          });
          return;
        }

        await ws.runModeTransaction({
          mode: restoreMode,
          params: {},
        });
      } catch (err) {
        console.error("退出拼豆恢复模式失败:", err);
        this.toast.showError("恢复上次模式失败");
      }
    },

    async syncToDevice() {
      if (!this.deviceConnected) {
        console.log("设备未连接，跳过同步");
        return;
      }

      if (this.localPixels.size === 0) {
        console.log("没有像素数据，跳过同步");
        return;
      }

      try {
        console.log("开始同步画布到设备，模式:", this.assistMode);
        const sparsePixels = [];

        this.localPixels.forEach((hexColor, key) => {
          const [x, y] = key.split(",").map(Number);
          const colorObj = ARTKAL_COLORS_FULL.find(
            (color) => color.hex.toLowerCase() === hexColor.toLowerCase(),
          );

          if (!colorObj) {
            return;
          }

          const rgb = hexToRgb(colorObj.hex);
          let r = rgb.r;
          let g = rgb.g;
          let b = rgb.b;

          if (this.assistMode === "row" && y !== this.currentRow) {
            r = Math.floor(r * 0.2);
            g = Math.floor(g * 0.2);
            b = Math.floor(b * 0.2);
          } else if (
            this.assistMode === "color" &&
            this.highlightColor &&
            hexColor.toLowerCase() !== this.highlightColor.toLowerCase()
          ) {
            r = Math.floor(r * 0.2);
            g = Math.floor(g * 0.2);
            b = Math.floor(b * 0.2);
          }

          sparsePixels.push(x, y, r, g, b);
        });

        console.log(
          `准备发送 ${sparsePixels.length / 5} 个有效像素（稀疏格式）`,
        );

        await this.deviceStore.sendSparseImage(sparsePixels);
        console.log("画布同步成功");
        this.deviceCanvasSynced = true;
      } catch (err) {
        console.error("同步到设备失败:", err);
        this.deviceCanvasSynced = false;
        this.toast.showError("同步失败");
      }
    },

    async updateRowBrightness() {
      if (!this.deviceConnected) {
        console.log("设备未连接，跳过行亮度更新");
        return;
      }

      if (this.assistMode !== "row") {
        console.log("不在逐行模式，跳过行亮度更新");
        return;
      }

      if (!this.deviceCanvasSynced) {
        this.toast.showInfo("请先同步到设备");
        return;
      }

      try {
        const ws = this.deviceStore.getWebSocket();
        if (!ws) {
          console.error("WebSocket 未初始化");
          return;
        }

        console.log("发送高亮行命令:", this.currentRow);
        await ws.highlightRow(this.currentRow);

        this.lastSyncedRow = this.currentRow;
      } catch (err) {
        console.error("更新行亮度失败:", err);
      }
    },

    async updateHighlightBrightness(previousColor, newColor) {
      if (!this.deviceConnected) {
        console.log("设备未连接，跳过高亮更新");
        return;
      }

      if (this.assistMode !== "color") {
        console.log("不在颜色模式，跳过高亮更新");
        return;
      }

      if (!this.deviceCanvasSynced) {
        this.toast.showInfo("请先同步到设备");
        return;
      }

      try {
        const ws = this.deviceStore.getWebSocket();
        if (!ws) {
          console.error("WebSocket 未初始化");
          return;
        }

        if (newColor) {
          const colorObj = ARTKAL_COLORS_FULL.find(
            (color) => color.hex.toLowerCase() === newColor.toLowerCase(),
          );

          if (colorObj) {
            const rgb = hexToRgb(colorObj.hex);
            console.log("发送高亮颜色命令:", colorObj.code, rgb);
            await ws.highlightColor(rgb);
          }
        } else {
          console.log("发送取消高亮命令");
          await ws.highlightColor(null);
        }
      } catch (err) {
        console.error("更新高亮亮度失败:", err);
      }
    },
  },
};
