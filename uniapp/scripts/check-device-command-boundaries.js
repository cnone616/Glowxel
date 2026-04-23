const fs = require("fs");
const path = require("path");

const projectRoot = path.resolve(__dirname, "..");
const targets = [
  path.join(projectRoot, "pages"),
];

const scanExtensions = new Set([".vue", ".js"]);
const bannedDirectCommands = new Set([
  "set_mode",
  "set_clock_config",
  "set_theme_config",
  "set_eyes_config",
  "eyes_interact",
  "set_game_screensaver",
  "set_ambient_effect",
  "set_planet_screensaver",
  "set_gif_animation",
  "set_countdown_board",
  "set_notification_board",
  "ota_check",
  "ota_update",
  "highlight_row",
  "highlight_color",
  "start_loading",
  "stop_loading",
]);
const bannedStatusFields = [
  "effectMode",
  "effectPreset",
  "game",
  "animationFrames",
  "preset",
  "seed",
  "hours",
  "minutes",
  "seconds",
  "progress",
];

function walk(dirPath, fileList) {
  const entries = fs.readdirSync(dirPath, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dirPath, entry.name);
    if (entry.isDirectory()) {
      walk(fullPath, fileList);
      continue;
    }
    if (scanExtensions.has(path.extname(entry.name))) {
      fileList.push(fullPath);
    }
  }
}

function getLineNumber(source, index) {
  return source.slice(0, index).split("\n").length;
}

function reportIssue(issues, filePath, line, message) {
  const relativePath = path.relative(projectRoot, filePath);
  issues.push(`${relativePath}:${line} ${message}`);
}

function scanFile(filePath, issues) {
  const source = fs.readFileSync(filePath, "utf8");
  const sendRegex = /ws\.send\s*\(\s*\{\s*cmd:\s*["']([^"']+)["']/g;
  let match = sendRegex.exec(source);
  while (match) {
    const command = match[1];
    if (bannedDirectCommands.has(command)) {
      reportIssue(
        issues,
        filePath,
        getLineNumber(source, match.index),
        `页面直发命令未收口到 webSocket helper: ${command}`,
      );
    }
    match = sendRegex.exec(source);
  }

  const storageRegex = /setStorageSync\(\s*["']device_(mode|last_business_mode)["']/g;
  match = storageRegex.exec(source);
  while (match) {
    reportIssue(
      issues,
      filePath,
      getLineNumber(source, match.index),
      "页面直接写 device_mode/device_last_business_mode",
    );
    match = storageRegex.exec(source);
  }

  for (const field of bannedStatusFields) {
    const fieldRegex = new RegExp(`status\\.${field}\\b`, "g");
    match = fieldRegex.exec(source);
    while (match) {
      reportIssue(
        issues,
        filePath,
        getLineNumber(source, match.index),
        `页面仍使用私有状态字段作为成功前置: status.${field}`,
      );
      match = fieldRegex.exec(source);
    }
  }
}

function main() {
  const files = [];
  for (const target of targets) {
    walk(target, files);
  }

  const issues = [];
  for (const filePath of files) {
    scanFile(filePath, issues);
  }

  if (issues.length > 0) {
    console.error("发现未收口的通讯/状态边界问题：");
    for (const issue of issues) {
      console.error(`- ${issue}`);
    }
    process.exit(1);
  }

  console.log("通讯与状态边界检查通过");
}

main();
