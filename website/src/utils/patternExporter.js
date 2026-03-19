import { getColorByCode } from "@/data/artkal-colors.js";
import logoUrl from "@/assets/images/RenLight-logo.png";

/**
 * 导出图案为图纸
 * @param {Map} pixels - 像素数据
 * @param {number} width - 画布宽度
 * @param {number} height - 画布高度
 * @param {string} projectName - 项目名称
 * @param {Array} usedColors - 使用的颜色列表
 * @returns {Promise<string>} 图片 Data URL
 */
export async function exportPattern(
  pixels,
  width,
  height,
  projectName = "图案",
  usedColors = [],
) {
  const cellSize = 40; // 每个像素格子的大小
  const gridLineWidth = 1;
  const padding = 80;
  const headerHeight = 100;
  const footerHeight = 200;
  const statsWidth = 300;

  // 计算画布尺寸
  const gridWidth = width * cellSize + (width + 1) * gridLineWidth;
  const gridHeight = height * cellSize + (height + 1) * gridLineWidth;
  const canvasWidth = gridWidth + padding * 2 + statsWidth;
  const canvasHeight = gridHeight + padding * 2 + headerHeight + footerHeight;

  // 创建 Canvas
  const canvas = document.createElement("canvas");
  canvas.width = canvasWidth;
  canvas.height = canvasHeight;
  const ctx = canvas.getContext("2d");

  // 背景
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, canvasWidth, canvasHeight);

  // 绘制标题
  ctx.fillStyle = "#000000";
  ctx.font = "bold 32px Arial";
  ctx.textAlign = "center";
  ctx.fillText(projectName, canvasWidth / 2, 50);

  // 绘制尺寸信息
  ctx.font = "18px Arial";
  ctx.fillStyle = "#666666";
  ctx.fillText(`${width} × ${height} 像素`, canvasWidth / 2, 85);

  const gridStartX = padding;
  const gridStartY = headerHeight + padding;

  // 绘制网格背景
  ctx.fillStyle = "#f5f5f5";
  ctx.fillRect(gridStartX, gridStartY, gridWidth, gridHeight);

  // 绘制网格线
  ctx.strokeStyle = "#cccccc";
  ctx.lineWidth = gridLineWidth;

  // 垂直线
  for (let x = 0; x <= width; x++) {
    const px = gridStartX + x * (cellSize + gridLineWidth);
    ctx.beginPath();
    ctx.moveTo(px, gridStartY);
    ctx.lineTo(px, gridStartY + gridHeight);
    ctx.stroke();

    // 每5格加粗
    if (x % 5 === 0) {
      ctx.lineWidth = 2;
      ctx.strokeStyle = "#999999";
      ctx.beginPath();
      ctx.moveTo(px, gridStartY);
      ctx.lineTo(px, gridStartY + gridHeight);
      ctx.stroke();
      ctx.lineWidth = gridLineWidth;
      ctx.strokeStyle = "#cccccc";
    }
  }

  // 水平线
  for (let y = 0; y <= height; y++) {
    const py = gridStartY + y * (cellSize + gridLineWidth);
    ctx.beginPath();
    ctx.moveTo(gridStartX, py);
    ctx.lineTo(gridStartX + gridWidth, py);
    ctx.stroke();

    // 每5格加粗
    if (y % 5 === 0) {
      ctx.lineWidth = 2;
      ctx.strokeStyle = "#999999";
      ctx.beginPath();
      ctx.moveTo(gridStartX, py);
      ctx.lineTo(gridStartX + gridWidth, py);
      ctx.stroke();
      ctx.lineWidth = gridLineWidth;
      ctx.strokeStyle = "#cccccc";
    }
  }

  // 绘制坐标轴标签
  ctx.fillStyle = "#666666";
  ctx.font = "12px monospace";
  ctx.textAlign = "center";

  // X 轴坐标（每5格显示）
  for (let x = 0; x <= width; x += 5) {
    const px = gridStartX + x * (cellSize + gridLineWidth) + gridLineWidth / 2;
    ctx.fillText(x.toString(), px, gridStartY - 10);
    ctx.fillText(x.toString(), px, gridStartY + gridHeight + 20);
  }

  // Y 轴坐标（每5格显示）
  ctx.textAlign = "right";
  for (let y = 0; y <= height; y += 5) {
    const py =
      gridStartY + y * (cellSize + gridLineWidth) + gridLineWidth / 2 + 4;
    ctx.fillText(y.toString(), gridStartX - 10, py);
  }

  ctx.textAlign = "left";
  for (let y = 0; y <= height; y += 5) {
    const py =
      gridStartY + y * (cellSize + gridLineWidth) + gridLineWidth / 2 + 4;
    ctx.fillText(y.toString(), gridStartX + gridWidth + 10, py);
  }

  // 绘制像素
  pixels.forEach((colorCode, key) => {
    const [x, y] = key.split(",").map(Number);
    const color = getColorByCode(colorCode);

    if (color) {
      const px = gridStartX + x * (cellSize + gridLineWidth) + gridLineWidth;
      const py = gridStartY + y * (cellSize + gridLineWidth) + gridLineWidth;

      // 填充颜色
      ctx.fillStyle = color.hex;
      ctx.fillRect(px, py, cellSize, cellSize);

      // 绘制颜色代码
      ctx.fillStyle = getContrastColor(color.hex);
      ctx.font = "bold 11px monospace";
      ctx.textAlign = "center";
      ctx.fillText(color.code, px + cellSize / 2, py + cellSize / 2 + 4);
    }
  });

  // 绘制统计信息
  const statsX = gridStartX + gridWidth + 40;
  const statsY = gridStartY;

  ctx.fillStyle = "#000000";
  ctx.font = "bold 18px Arial";
  ctx.textAlign = "left";
  ctx.fillText("颜色统计", statsX, statsY);

  let currentY = statsY + 30;
  const colorBoxSize = 24;
  const lineHeight = 32;

  ctx.font = "14px monospace";

  usedColors.forEach((color, index) => {
    if (currentY > canvasHeight - footerHeight - 50) return; // 避免超出画布

    // 颜色方块
    ctx.fillStyle = color.hex;
    ctx.fillRect(
      statsX,
      currentY - colorBoxSize + 4,
      colorBoxSize,
      colorBoxSize,
    );
    ctx.strokeStyle = "#cccccc";
    ctx.lineWidth = 1;
    ctx.strokeRect(
      statsX,
      currentY - colorBoxSize + 4,
      colorBoxSize,
      colorBoxSize,
    );

    // 颜色代码和数量
    ctx.fillStyle = "#000000";
    ctx.fillText(
      `${color.code}: ${color.count}`,
      statsX + colorBoxSize + 10,
      currentY,
    );

    currentY += lineHeight;
  });

  // 绘制总计
  const totalPixels = Array.from(pixels.values()).length;
  currentY += 10;
  ctx.font = "bold 16px Arial";
  ctx.fillStyle = "#000000";
  ctx.fillText(`总计: ${totalPixels} 个`, statsX, currentY);

  // 绘制 Logo（底部）
  try {
    const logo = await loadImage(logoUrl);
    const logoHeight = 40;
    const logoWidth = logo.width * (logoHeight / logo.height);
    const logoX = canvasWidth - logoWidth - 40;
    const logoY = canvasHeight - 60;
    ctx.drawImage(logo, logoX, logoY, logoWidth, logoHeight);
  } catch (err) {
    console.warn("Logo 加载失败:", err);
  }

  // 绘制底部信息
  ctx.fillStyle = "#999999";
  ctx.font = "14px Arial";
  ctx.textAlign = "left";
  ctx.fillText("RenLight", 40, canvasHeight - 40);
  ctx.fillText(new Date().toLocaleDateString("zh-CN"), 40, canvasHeight - 20);

  return canvas.toDataURL("image/png");
}

/**
 * 下载图片
 * @param {string} dataUrl - 图片 Data URL
 * @param {string} filename - 文件名
 */
export function downloadImage(dataUrl, filename = "pattern.png") {
  const link = document.createElement("a");
  link.download = filename;
  link.href = dataUrl;
  link.click();
}

/**
 * 加载图片
 * @param {string} src - 图片路径
 * @returns {Promise<HTMLImageElement>}
 */
function loadImage(src) {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => resolve(img);
    img.onerror = reject;
    img.src = src;
  });
}

/**
 * 获取对比色（用于文字显示）
 * @param {string} hex - 十六进制颜色
 * @returns {string} 黑色或白色
 */
function getContrastColor(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  if (!result) return "#000000";

  const r = parseInt(result[1], 16);
  const g = parseInt(result[2], 16);
  const b = parseInt(result[3], 16);

  // 计算亮度
  const brightness = (r * 299 + g * 587 + b * 114) / 1000;

  return brightness > 128 ? "#000000" : "#ffffff";
}
