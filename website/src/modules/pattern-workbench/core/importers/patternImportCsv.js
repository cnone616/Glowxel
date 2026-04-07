import { ARTKAL_COLORS_FULL, getColorByCode } from "@/data/artkal-colors.js";
import { createPatternDocument } from "../model/patternDocument.js";

function normalizeCsvCell(value) {
  const cell = String(value || "").trim();
  if (!cell || cell === "TRANSPARENT" || cell === "EMPTY") {
    return "";
  }

  const colorByCode = getColorByCode(cell.toUpperCase());
  if (colorByCode) {
    return colorByCode.code;
  }

  const normalizedHex = cell.startsWith("#") ? cell.toUpperCase() : `#${cell.toUpperCase()}`;
  const colorByHex = ARTKAL_COLORS_FULL.find((item) => item.hex.toUpperCase() === normalizedHex);
  if (colorByHex) {
    return colorByHex.code;
  }

  throw new Error(`无法识别的颜色值：${cell}`);
}

export function importPatternFromCsv(text, options = {}) {
  const lines = String(text || "")
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter(Boolean);

  if (!lines.length) {
    throw new Error("CSV 内容为空");
  }

  const rows = lines.map((line) => line.split(","));
  const width = rows[0].length;
  const height = rows.length;

  if (!width) {
    throw new Error("CSV 列数无效");
  }

  const pixels = new Map();

  rows.forEach((row, y) => {
    if (row.length !== width) {
      throw new Error(`第 ${y + 1} 行列数不一致`);
    }

    row.forEach((raw, x) => {
      const code = normalizeCsvCell(raw);
      if (code) {
        pixels.set(`${x},${y}`, code);
      }
    });
  });

  return createPatternDocument({
    id: options.id || `csv-${Date.now()}`,
    name: options.name || "导入图纸",
    width,
    height,
    pixels,
    sourceType: "csv",
  });
}
