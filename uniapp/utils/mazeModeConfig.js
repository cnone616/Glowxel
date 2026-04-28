const MAZE_MODE_CONFIG_KEY = "maze_mode_config";

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

  if (
    source.mazeSizeMode !== "wide" &&
    source.mazeSizeMode !== "dense"
  ) {
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

function cloneMazeModeConfig(config) {
  return {
    speed: config.speed,
    mazeSizeMode: config.mazeSizeMode,
    showClock: config.showClock,
    panelBgColor: config.panelBgColor,
    borderColor: config.borderColor,
    timeColor: config.timeColor,
    dateColor: config.dateColor,
    generationPathColor: config.generationPathColor,
    searchVisitedColor: config.searchVisitedColor,
    searchFrontierColor: config.searchFrontierColor,
    solvedPathStartColor: config.solvedPathStartColor,
    solvedPathEndColor: config.solvedPathEndColor,
  };
}

function readSavedMazeModeConfig() {
  return createMazeModeConfig(uni.getStorageSync(MAZE_MODE_CONFIG_KEY));
}

function writeSavedMazeModeConfig(config) {
  const normalized = createMazeModeConfig(config);
  if (!normalized) {
    throw new Error("迷宫颜色配置无效");
  }
  uni.setStorageSync(MAZE_MODE_CONFIG_KEY, normalized);
  return cloneMazeModeConfig(normalized);
}

export {
  MAZE_COLOR_FIELDS,
  MAZE_MODE_CONFIG_KEY,
  cloneMazeModeConfig,
  createDefaultMazeModeConfig,
  createMazeModeConfig,
  normalizeHexColorText,
  readSavedMazeModeConfig,
  writeSavedMazeModeConfig,
};
