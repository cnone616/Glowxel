const fs = require("fs");
const path = require("path");

const LOG_DIR = path.join(__dirname, "../../logs");
const SERVER_LOG_FILE = path.join(LOG_DIR, "server.log");
const ERROR_LOG_FILE = path.join(LOG_DIR, "error.log");

function ensureLogDir() {
  if (!fs.existsSync(LOG_DIR)) {
    fs.mkdirSync(LOG_DIR, { recursive: true });
  }
}

function writeLogLine(filePath, entry) {
  ensureLogDir();
  fs.appendFileSync(filePath, `${JSON.stringify(entry)}\n`, "utf8");
}

function createLogEntry(level, event, payload) {
  return {
    timestamp: new Date().toISOString(),
    level,
    event,
    ...payload,
  };
}

function log(level, event, payload = {}) {
  const entry = createLogEntry(level, event, payload);
  writeLogLine(SERVER_LOG_FILE, entry);

  if (level === "error") {
    writeLogLine(ERROR_LOG_FILE, entry);
  }
}

function info(event, payload) {
  log("info", event, payload);
}

function warn(event, payload) {
  log("warn", event, payload);
}

function error(event, payload) {
  log("error", event, payload);
}

function getClientIp(req) {
  const forwardedFor = req.headers["x-forwarded-for"];
  if (typeof forwardedFor === "string" && forwardedFor.trim()) {
    return forwardedFor.split(",")[0].trim();
  }

  return req.ip || req.socket?.remoteAddress || "";
}

module.exports = {
  log,
  info,
  warn,
  error,
  getClientIp,
};
