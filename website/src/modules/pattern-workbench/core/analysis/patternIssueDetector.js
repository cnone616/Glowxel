import { createPatternIssue } from "../model/patternDocument.js";

function hasSameColorNeighbor(document, x, y, color) {
  let sameCount = 0;
  for (let offsetY = -1; offsetY <= 1; offsetY++) {
    for (let offsetX = -1; offsetX <= 1; offsetX++) {
      if (offsetX === 0 && offsetY === 0) {
        continue;
      }
      const neighbor = document.pixels.get(`${x + offsetX},${y + offsetY}`) || "";
      if (neighbor === color) {
        sameCount += 1;
      }
    }
  }
  return sameCount;
}

export function detectPatternIssues(document, diffResult) {
  if (!document) {
    return [];
  }

  const issues = [];

  document.pixels.forEach((color, key) => {
    const [x, y] = key.split(",").map(Number);
    const neighborCount = hasSameColorNeighbor(document, x, y, color);
    if (neighborCount <= 1) {
      issues.push(
        createPatternIssue({
          id: `isolated-${x}-${y}`,
          type: "孤立点",
          title: `(${x}, ${y}) 存在单点杂色`,
          meta: "建议合并到周围主色",
          position: { x, y },
          severity: "medium",
        }),
      );
    }
  });

  if (diffResult) {
    diffResult.missing.slice(0, 12).forEach((item) => {
      issues.push(
        createPatternIssue({
          id: `missing-${item.x}-${item.y}`,
          type: "缺失点",
          title: `(${item.x}, ${item.y}) 缺少参考像素`,
          meta: "建议补齐到参考结果",
          position: { x: item.x, y: item.y },
          severity: "high",
        }),
      );
    });

    diffResult.extra.slice(0, 12).forEach((item) => {
      issues.push(
        createPatternIssue({
          id: `extra-${item.x}-${item.y}`,
          type: "多余点",
          title: `(${item.x}, ${item.y}) 多出当前像素`,
          meta: "建议检查是否需要删除",
          position: { x: item.x, y: item.y },
          severity: "medium",
        }),
      );
    });

    diffResult.changed.slice(0, 12).forEach((item) => {
      issues.push(
        createPatternIssue({
          id: `changed-${item.x}-${item.y}`,
          type: "颜色偏差",
          title: `(${item.x}, ${item.y}) 与参考颜色不一致`,
          meta: `当前 ${item.current}，参考 ${item.reference}`,
          position: { x: item.x, y: item.y },
          severity: "medium",
        }),
      );
    });
  }

  return issues;
}
