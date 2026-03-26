const fs = require("fs");
const path = require("path");

const LOG_DIR = path.join(__dirname, "../../logs");
const AUDIT_LOG_FILE = path.join(LOG_DIR, "admin-audit.log");
const REDACTED_FIELDS = new Set([
  "password",
  "admin_password",
  "token",
  "authorization",
]);

function ensureLogDir() {
  if (!fs.existsSync(LOG_DIR)) {
    fs.mkdirSync(LOG_DIR, { recursive: true });
  }
}

function sanitizeValue(key, value) {
  if (REDACTED_FIELDS.has(key)) {
    return "[REDACTED]";
  }

  if (Array.isArray(value)) {
    return value.map((item) => sanitizeBody(item));
  }

  if (value && typeof value === "object") {
    return sanitizeBody(value);
  }

  return value;
}

function sanitizeBody(body) {
  if (!body || typeof body !== "object" || Array.isArray(body)) {
    return body || null;
  }

  const sanitized = {};
  for (const [key, value] of Object.entries(body)) {
    sanitized[key] = sanitizeValue(key, value);
  }
  return sanitized;
}

function buildAuditEntry(payload) {
  return {
    timestamp: new Date().toISOString(),
    ...payload,
  };
}

function writeAdminAuditLog(payload) {
  ensureLogDir();
  const entry = buildAuditEntry(payload);
  fs.appendFileSync(AUDIT_LOG_FILE, `${JSON.stringify(entry)}\n`, "utf8");
}

module.exports = {
  sanitizeBody,
  writeAdminAuditLog,
};
