import { normalizeIntensity, normalizeSpeed } from "../led-matrix-demos/common.js";
import { resolveDemoById } from "./demoCatalog.js";

function normalizeDensity(value) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) {
    return 72;
  }
  return Math.max(10, Math.min(100, Math.round(parsed)));
}

function normalizeHexColor(value) {
  if (typeof value !== "string") {
    throw new Error("颜色值无效");
  }
  const body = value.trim().replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    throw new Error("颜色值无效");
  }
  return `#${body.toLowerCase()}`;
}

function hexToRgb(value) {
  const normalized = normalizeHexColor(value);
  return {
    r: parseInt(normalized.slice(1, 3), 16),
    g: parseInt(normalized.slice(3, 5), 16),
    b: parseInt(normalized.slice(5, 7), 16),
  };
}

function buildLedMatrixSendPlan(options) {
  const safeOptions = options || {};
  const demo = resolveDemoById(safeOptions.demoId);
  if (!demo) {
    throw new Error("未找到对应演示");
  }

  const speed = normalizeSpeed(safeOptions.speed);
  const intensity = normalizeIntensity(safeOptions.intensity);
  const density = normalizeDensity(safeOptions.density);

  if (demo.type === "ambient") {
    if (demo.id === "rain") {
      return {
        type: "command",
        deviceMode: "led_matrix_showcase",
        command: {
          cmd: "set_ambient_effect",
          preset: demo.ambientPreset,
          speed,
          density,
          color: hexToRgb(safeOptions.color),
          loop: true,
        },
      };
    }

    return {
      type: "command",
      deviceMode: "led_matrix_showcase",
      command: {
        cmd: "set_ambient_effect",
        preset: demo.ambientPreset,
        speed,
        intensity,
        loop: true,
      },
    };
  }

  throw new Error(`未支持的板载命令场景：${demo.id}`);
}

export { buildLedMatrixSendPlan };
