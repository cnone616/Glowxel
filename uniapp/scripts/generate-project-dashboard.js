#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const ROOT_DIR = process.cwd();
const OUTPUT_DIR = path.join(ROOT_DIR, "output");
const OUTPUT_FILE = path.join(OUTPUT_DIR, "project-audit-dashboard.html");

const SOURCE_EXTENSIONS = new Set([".vue", ".js", ".css", ".scss", ".json"]);
const IMAGE_EXTENSIONS = new Set([".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp"]);
const SKIP_SOURCE_DIRS = new Set(["node_modules", ".git", "dist", "unpackage", "output", "static", "docs"]);

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function readText(filePath) {
  return fs.readFileSync(filePath, "utf8");
}

function countLines(text) {
  if (text.length === 0) {
    return 0;
  }
  return text.split(/\r?\n/).length;
}

function formatNumber(value) {
  return new Intl.NumberFormat("zh-CN").format(value);
}

function formatBytes(bytes) {
  const units = ["B", "KB", "MB", "GB"];
  let current = bytes;
  let unitIndex = 0;
  while (current >= 1024 && unitIndex < units.length - 1) {
    current /= 1024;
    unitIndex += 1;
  }
  const digits = unitIndex === 0 ? 0 : 2;
  return `${current.toFixed(digits)} ${units[unitIndex]}`;
}

function formatPercent(value) {
  return `${(value * 100).toFixed(1)}%`;
}

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

function relativeToRoot(filePath) {
  return path.relative(ROOT_DIR, filePath).split(path.sep).join("/");
}

function listFiles(dirPath, options = {}) {
  const {
    includeExtensions,
    skipDirs,
    includeAllFiles,
  } = options;
  const results = [];

  function walk(currentDir) {
    const entries = fs.readdirSync(currentDir, { withFileTypes: true });
    entries.forEach((entry) => {
      const fullPath = path.join(currentDir, entry.name);
      if (entry.isDirectory()) {
        if (skipDirs && skipDirs.has(entry.name)) {
          return;
        }
        walk(fullPath);
        return;
      }

      if (includeAllFiles) {
        results.push(fullPath);
        return;
      }

      const extension = path.extname(entry.name).toLowerCase();
      if (includeExtensions && includeExtensions.has(extension)) {
        results.push(fullPath);
      }
    });
  }

  if (fs.existsSync(dirPath)) {
    walk(dirPath);
  }

  return results;
}

function parsePages() {
  const pagesJson = JSON.parse(readText(path.join(ROOT_DIR, "pages.json")));
  return pagesJson.pages.map((page) => page.path);
}

function matchBlocks(text, tagName) {
  const pattern = new RegExp(`<${tagName}[^>]*>[\\s\\S]*?<\\/${tagName}>`, "g");
  const matches = text.match(pattern);
  if (matches) {
    return matches;
  }
  return [];
}

function computePageMetrics(pagePaths) {
  return pagePaths
    .map((pagePath) => {
      const filePath = path.join(ROOT_DIR, `${pagePath}.vue`);
      if (!fs.existsSync(filePath)) {
        return null;
      }

      const source = readText(filePath);
      const templateLines = matchBlocks(source, "template")
        .map((block) => countLines(block))
        .reduce((sum, value) => sum + value, 0);
      const scriptLines = matchBlocks(source, "script")
        .map((block) => countLines(block))
        .reduce((sum, value) => sum + value, 0);
      const styleLines = matchBlocks(source, "style")
        .map((block) => countLines(block))
        .reduce((sum, value) => sum + value, 0);
      const totalLines = countLines(source);
      const bytes = fs.statSync(filePath).size;

      return {
        pagePath,
        filePath,
        totalLines,
        templateLines,
        scriptLines,
        styleLines,
        bytes,
        styleRatio: totalLines > 0 ? styleLines / totalLines : 0,
        scriptRatio: totalLines > 0 ? scriptLines / totalLines : 0,
      };
    })
    .filter((item) => item !== null);
}

function collectSourceMetrics() {
  const sourceFiles = listFiles(ROOT_DIR, {
    includeExtensions: SOURCE_EXTENSIONS,
    skipDirs: SKIP_SOURCE_DIRS,
  });

  const fileRows = sourceFiles.map((filePath) => {
    const text = readText(filePath);
    const stat = fs.statSync(filePath);
    return {
      path: relativeToRoot(filePath),
      extension: path.extname(filePath).toLowerCase(),
      lines: countLines(text),
      bytes: stat.size,
    };
  });

  const totalsByExtension = {};
  fileRows.forEach((row) => {
    if (!totalsByExtension[row.extension]) {
      totalsByExtension[row.extension] = {
        extension: row.extension,
        files: 0,
        lines: 0,
        bytes: 0,
      };
    }
    totalsByExtension[row.extension].files += 1;
    totalsByExtension[row.extension].lines += row.lines;
    totalsByExtension[row.extension].bytes += row.bytes;
  });

  const totals = Object.values(totalsByExtension).sort((left, right) => {
    return right.lines - left.lines;
  });

  return {
    files: fileRows,
    totals,
    totalLines: fileRows.reduce((sum, row) => sum + row.lines, 0),
    totalBytes: fileRows.reduce((sum, row) => sum + row.bytes, 0),
  };
}

function collectDirectorySizes() {
  const directories = ["pages", "utils", "components", "styles", "store", "api", "static"];

  function sizeDir(dirPath) {
    const files = listFiles(path.join(ROOT_DIR, dirPath), {
      includeAllFiles: true,
    });
    const bytes = files.reduce((sum, filePath) => {
      return sum + fs.statSync(filePath).size;
    }, 0);
    return {
      name: dirPath,
      bytes,
      files: files.length,
    };
  }

  return directories
    .filter((dirPath) => fs.existsSync(path.join(ROOT_DIR, dirPath)))
    .map(sizeDir)
    .sort((left, right) => right.bytes - left.bytes);
}

function collectAssets() {
  const assetFiles = listFiles(path.join(ROOT_DIR, "static"), { includeAllFiles: true })
    .map((filePath) => {
      const stat = fs.statSync(filePath);
      return {
        path: relativeToRoot(filePath),
        extension: path.extname(filePath).toLowerCase(),
        bytes: stat.size,
      };
    });

  const byExtensionMap = {};
  assetFiles.forEach((asset) => {
    if (!byExtensionMap[asset.extension]) {
      byExtensionMap[asset.extension] = {
        extension: asset.extension,
        count: 0,
        bytes: 0,
      };
    }
    byExtensionMap[asset.extension].count += 1;
    byExtensionMap[asset.extension].bytes += asset.bytes;
  });

  const imageBytes = assetFiles
    .filter((asset) => IMAGE_EXTENSIONS.has(asset.extension))
    .reduce((sum, asset) => sum + asset.bytes, 0);

  const iconfontBytes = assetFiles
    .filter((asset) => asset.path.startsWith("static/iconfont/"))
    .reduce((sum, asset) => sum + asset.bytes, 0);

  const figmaAssetBytes = assetFiles
    .filter((asset) => asset.path.startsWith("static/figma-assets/"))
    .reduce((sum, asset) => sum + asset.bytes, 0);

  const themeThumbBytes = assetFiles
    .filter((asset) => asset.path.startsWith("static/clockwise-themes/"))
    .reduce((sum, asset) => sum + asset.bytes, 0);

  return {
    files: assetFiles.sort((left, right) => right.bytes - left.bytes),
    totalBytes: assetFiles.reduce((sum, asset) => sum + asset.bytes, 0),
    byExtension: Object.values(byExtensionMap).sort((left, right) => right.bytes - left.bytes),
    imageBytes,
    iconfontBytes,
    figmaAssetBytes,
    themeThumbBytes,
  };
}

function parseStyleAuditData() {
  const auditFile = path.join(ROOT_DIR, "utils", "styleAuditData.js");
  const source = readText(auditFile);

  function extractExport(exportName) {
    const marker = `export const ${exportName} = `;
    const start = source.indexOf(marker);
    if (start < 0) {
      throw new Error(`未找到 ${exportName}`);
    }
    const exportStart = start + marker.length;
    const rest = source.slice(exportStart);
    const nextExportIndex = rest.indexOf("\n\nexport const ");
    const rawValue = nextExportIndex >= 0 ? rest.slice(0, nextExportIndex) : rest;
    return JSON.parse(rawValue.trim().replace(/;$/, ""));
  }

  const meta = extractExport("STYLE_AUDIT_META");
  const pages = extractExport("STYLE_AUDIT_PAGES");
  const groups = extractExport("STYLE_AUDIT_GROUPS");

  return {
    meta,
    pages,
    groups,
  };
}

function collectStyleMetrics(registeredPages) {
  const styleAudit = parseStyleAuditData();
  const actualEntryCount = styleAudit.groups.reduce((sum, group) => sum + group.total, 0);
  const actualGroupCount = styleAudit.groups.length;
  const actualPageCount = styleAudit.pages.length;
  const registeredSet = new Set(registeredPages);
  const auditPageSet = new Set(styleAudit.pages.map((page) => page.path));

  const missingPages = registeredPages.filter((pagePath) => !auditPageSet.has(pagePath));
  const extraPages = styleAudit.pages
    .map((page) => page.path)
    .filter((pagePath) => !registeredSet.has(pagePath));

  const roleTotalsMap = {};
  styleAudit.groups.forEach((group) => {
    if (!roleTotalsMap[group.role]) {
      roleTotalsMap[group.role] = {
        role: group.role,
        groups: 0,
        entries: 0,
        pages: 0,
      };
    }
    roleTotalsMap[group.role].groups += 1;
    roleTotalsMap[group.role].entries += group.total;
    roleTotalsMap[group.role].pages += group.pageCount;
  });

  return {
    auditMeta: styleAudit.meta,
    actualPageCount,
    actualEntryCount,
    actualGroupCount,
    pageCoverage: registeredPages.length > 0 ? actualPageCount / registeredPages.length : 0,
    pagesWithMostStyles: styleAudit.pages
      .slice()
      .sort((left, right) => right.total - left.total)
      .slice(0, 8),
    sharedGroups: styleAudit.groups
      .slice()
      .sort((left, right) => {
        if (right.pageCount !== left.pageCount) {
          return right.pageCount - left.pageCount;
        }
        return right.total - left.total;
      })
      .slice(0, 8)
      .map((group) => ({
        key: group.key,
        role: group.role,
        pageCount: group.pageCount,
        total: group.total,
      })),
    roleTotals: Object.values(roleTotalsMap).sort((left, right) => right.entries - left.entries),
    missingPages,
    extraPages,
    metaMismatch: {
      pageCount: styleAudit.meta.pageCount !== actualPageCount,
      entryCount: styleAudit.meta.entryCount !== actualEntryCount,
      groupCount: styleAudit.meta.groupCount !== actualGroupCount,
    },
  };
}

function collectPackageMetrics() {
  const packageJson = JSON.parse(readText(path.join(ROOT_DIR, "package.json")));
  const packageLock = JSON.parse(readText(path.join(ROOT_DIR, "package-lock.json")));
  const packageLockText = readText(path.join(ROOT_DIR, "package-lock.json"));
  const packageEntries = packageLock.packages ? Object.keys(packageLock.packages).length : 0;
  const dependencyCount = packageJson.dependencies ? Object.keys(packageJson.dependencies).length : 0;
  const devDependencyCount = packageJson.devDependencies ? Object.keys(packageJson.devDependencies).length : 0;

  return {
    dependencyCount,
    devDependencyCount,
    packageEntries,
    lockfileBytes: Buffer.byteLength(packageLockText, "utf8"),
  };
}

function deriveRecommendations(snapshot) {
  const recommendations = [];
  const heaviestPage = snapshot.pages.topByTotal[0];
  const heaviestScriptPage = snapshot.pages.topByScript[0];
  const styleCoverageIssue = snapshot.style.missingPages.length > 0 || snapshot.style.extraPages.length > 0;

  if (heaviestPage) {
    recommendations.push({
      level: "高优先级",
      title: "拆分超大页面",
      detail: `${heaviestPage.pagePath} 已达 ${formatNumber(heaviestPage.totalLines)} 行，其中脚本 ${formatNumber(heaviestPage.scriptLines)} 行，建议按预览、面板、发送链路拆层。`,
    });
  }

  if (styleCoverageIssue) {
    recommendations.push({
      level: "高优先级",
      title: "刷新样式审计快照",
      detail: `styleAuditData 当前只覆盖 ${snapshot.style.actualPageCount}/${snapshot.routes.registeredPages} 个注册页，并且仍保留了已不存在的 ${snapshot.style.extraPages.length} 个旧页面。`,
    });
  }

  if (snapshot.assets.iconfontBytes + snapshot.assets.figmaAssetBytes >= 800 * 1024) {
    recommendations.push({
      level: "中优先级",
      title: "优先瘦身静态资源",
      detail: `iconfont 与 figma 资产合计 ${formatBytes(snapshot.assets.iconfontBytes + snapshot.assets.figmaAssetBytes)}，已经接近静态资源主体，适合先清理不用的字体与审计素材。`,
    });
  }

  if (heaviestScriptPage && heaviestScriptPage.scriptRatio >= 0.5) {
    recommendations.push({
      level: "中优先级",
      title: "收口脚本集中页",
      detail: `${heaviestScriptPage.pagePath} 的脚本占比达到 ${formatPercent(heaviestScriptPage.scriptRatio)}，状态管理和协议逻辑可以继续外提。`,
    });
  }

  return recommendations.slice(0, 4);
}

function collectSnapshot() {
  const registeredPages = parsePages();
  const pageMetrics = computePageMetrics(registeredPages);
  const sourceMetrics = collectSourceMetrics();
  const directorySizes = collectDirectorySizes();
  const assetMetrics = collectAssets();
  const styleMetrics = collectStyleMetrics(registeredPages);
  const packageMetrics = collectPackageMetrics();

  const componentCount = listFiles(path.join(ROOT_DIR, "components"), {
    includeExtensions: new Set([".vue"]),
  }).length;
  const utilityCount = listFiles(path.join(ROOT_DIR, "utils"), {
    includeExtensions: new Set([".js"]),
  }).length;

  const topByTotal = pageMetrics.slice().sort((left, right) => right.totalLines - left.totalLines).slice(0, 8);
  const topByStyle = pageMetrics.slice().sort((left, right) => right.styleLines - left.styleLines).slice(0, 8);
  const topByScript = pageMetrics.slice().sort((left, right) => right.scriptLines - left.scriptLines).slice(0, 8);
  const topSourceFiles = sourceMetrics.files.slice().sort((left, right) => right.lines - left.lines).slice(0, 10);
  const heavyRuntimeFiles = sourceMetrics.files
    .filter((row) => row.bytes >= 30 * 1024)
    .filter((row) => row.extension === ".vue" || row.extension === ".js")
    .sort((left, right) => right.bytes - left.bytes)
    .slice(0, 10);

  const snapshot = {
    generatedAt: new Date().toLocaleString("zh-CN", {
      hour12: false,
      timeZone: "Asia/Shanghai",
    }),
    rootDir: ROOT_DIR,
    routes: {
      registeredPages: registeredPages.length,
      componentCount,
      utilityCount,
      pageFiles: pageMetrics.length,
    },
    source: sourceMetrics,
    pages: {
      topByTotal,
      topByStyle,
      topByScript,
    },
    directories: directorySizes,
    assets: assetMetrics,
    style: styleMetrics,
    packages: packageMetrics,
    heavyRuntimeFiles,
    topSourceFiles,
  };

  snapshot.recommendations = deriveRecommendations(snapshot);
  return snapshot;
}

function renderMetricCard(card) {
  return `
    <section class="metric-card tone-${escapeHtml(card.tone)}">
      <div class="metric-label">${escapeHtml(card.label)}</div>
      <div class="metric-value">${escapeHtml(card.value)}</div>
      <div class="metric-copy">${escapeHtml(card.copy)}</div>
    </section>
  `;
}

function renderBarList(rows, valueKey, formatter, maxValue, labelBuilder) {
  return rows.map((row) => {
    const value = row[valueKey];
    const ratio = maxValue > 0 ? Math.max(value / maxValue, 0.04) : 0.04;
    return `
      <div class="bar-row">
        <div class="bar-copy">
          <div class="bar-label">${labelBuilder(row)}</div>
          <div class="bar-value">${formatter(value)}</div>
        </div>
        <div class="bar-track">
          <div class="bar-fill" style="width:${(ratio * 100).toFixed(1)}%"></div>
        </div>
      </div>
    `;
  }).join("");
}

function renderTable(headers, rows) {
  const headerHtml = headers.map((header) => `<th>${escapeHtml(header)}</th>`).join("");
  const rowHtml = rows.map((row) => {
    const columns = row.map((column) => `<td>${column}</td>`).join("");
    return `<tr>${columns}</tr>`;
  }).join("");
  return `
    <div class="table-shell">
      <table>
        <thead><tr>${headerHtml}</tr></thead>
        <tbody>${rowHtml}</tbody>
      </table>
    </div>
  `;
}

function renderTagList(items, tone) {
  if (items.length === 0) {
    return `<div class="empty-note">当前没有条目</div>`;
  }
  return `
    <div class="tag-list">
      ${items.map((item) => `<span class="tag tone-${escapeHtml(tone)}">${escapeHtml(item)}</span>`).join("")}
    </div>
  `;
}

function buildHtml(snapshot) {
  const overviewCards = [
    {
      label: "源码总行数",
      value: formatNumber(snapshot.source.totalLines),
      copy: `已排除 output / node_modules / dist，当前源码体量 ${formatBytes(snapshot.source.totalBytes)}`,
      tone: "yellow",
    },
    {
      label: "页面与组件",
      value: `${formatNumber(snapshot.routes.registeredPages)} / ${formatNumber(snapshot.routes.componentCount)}`,
      copy: `注册页 ${formatNumber(snapshot.routes.registeredPages)} 个，组件 ${formatNumber(snapshot.routes.componentCount)} 个，工具脚本 ${formatNumber(snapshot.routes.utilityCount)} 个`,
      tone: "blue",
    },
    {
      label: "样式审计覆盖",
      value: `${formatNumber(snapshot.style.actualPageCount)} / ${formatNumber(snapshot.routes.registeredPages)}`,
      copy: `当前样式快照覆盖率 ${formatPercent(snapshot.style.pageCoverage)}，类名分组 ${formatNumber(snapshot.style.actualGroupCount)} 组`,
      tone: "green",
    },
    {
      label: "静态资源体积",
      value: formatBytes(snapshot.assets.totalBytes),
      copy: `图片 ${formatBytes(snapshot.assets.imageBytes)}，iconfont ${formatBytes(snapshot.assets.iconfontBytes)}`,
      tone: "coral",
    },
  ];

  const extensionBars = renderBarList(
    snapshot.source.totals,
    "lines",
    formatNumber,
    snapshot.source.totals[0] ? snapshot.source.totals[0].lines : 0,
    (row) => `${escapeHtml(row.extension)} · ${escapeHtml(String(row.files))} 个文件`
  );

  const directoryBars = renderBarList(
    snapshot.directories,
    "bytes",
    formatBytes,
    snapshot.directories[0] ? snapshot.directories[0].bytes : 0,
    (row) => `${escapeHtml(row.name)} · ${escapeHtml(String(row.files))} 个文件`
  );

  const roleBars = renderBarList(
    snapshot.style.roleTotals,
    "entries",
    formatNumber,
    snapshot.style.roleTotals[0] ? snapshot.style.roleTotals[0].entries : 0,
    (row) => `${escapeHtml(row.role)} · ${escapeHtml(String(row.groups))} 组`
  );

  const assetBars = renderBarList(
    snapshot.assets.byExtension.slice(0, 8),
    "bytes",
    formatBytes,
    snapshot.assets.byExtension[0] ? snapshot.assets.byExtension[0].bytes : 0,
    (row) => `${escapeHtml(row.extension)} · ${escapeHtml(String(row.count))} 个`
  );

  const totalPageTable = renderTable(
    ["页面", "总行数", "模板", "脚本", "样式"],
    snapshot.pages.topByTotal.map((row) => ([
      `<code>${escapeHtml(row.pagePath)}</code>`,
      formatNumber(row.totalLines),
      formatNumber(row.templateLines),
      formatNumber(row.scriptLines),
      formatNumber(row.styleLines),
    ]))
  );

  const stylePageTable = renderTable(
    ["页面", "样式行数", "样式占比", "总行数"],
    snapshot.pages.topByStyle.map((row) => ([
      `<code>${escapeHtml(row.pagePath)}</code>`,
      formatNumber(row.styleLines),
      formatPercent(row.styleRatio),
      formatNumber(row.totalLines),
    ]))
  );

  const sourceFileTable = renderTable(
    ["文件", "类型", "行数", "体积"],
    snapshot.topSourceFiles.map((row) => ([
      `<code>${escapeHtml(row.path)}</code>`,
      escapeHtml(row.extension),
      formatNumber(row.lines),
      formatBytes(row.bytes),
    ]))
  );

  const sharedGroupTable = renderTable(
    ["类名分组", "角色", "覆盖页数", "条目数"],
    snapshot.style.sharedGroups.map((group) => ([
      `<code>.${escapeHtml(group.key)}</code>`,
      escapeHtml(group.role),
      formatNumber(group.pageCount),
      formatNumber(group.total),
    ]))
  );

  const heavyAssetTable = renderTable(
    ["资源", "类型", "体积"],
    snapshot.assets.files.slice(0, 10).map((row) => ([
      `<code>${escapeHtml(row.path)}</code>`,
      escapeHtml(row.extension),
      formatBytes(row.bytes),
    ]))
  );

  const heavyRuntimeTable = renderTable(
    ["源码热点", "类型", "体积"],
    snapshot.heavyRuntimeFiles.map((row) => ([
      `<code>${escapeHtml(row.path)}</code>`,
      escapeHtml(row.extension),
      formatBytes(row.bytes),
    ]))
  );

  const recommendationCards = snapshot.recommendations.map((item) => {
    return `
      <article class="recommend-card">
        <div class="recommend-level">${escapeHtml(item.level)}</div>
        <h3>${escapeHtml(item.title)}</h3>
        <p>${escapeHtml(item.detail)}</p>
      </article>
    `;
  }).join("");

  const metaWarnings = [];
  if (snapshot.style.metaMismatch.pageCount || snapshot.style.metaMismatch.entryCount || snapshot.style.metaMismatch.groupCount) {
    metaWarnings.push("styleAuditData.js 的 meta 统计与实际数组内容不完全一致，说明快照存在陈旧信号。");
  }
  if (snapshot.style.missingPages.length > 0) {
    metaWarnings.push(`样式审计缺少 ${snapshot.style.missingPages.length} 个注册页。`);
  }
  if (snapshot.style.extraPages.length > 0) {
    metaWarnings.push(`样式审计里还有 ${snapshot.style.extraPages.length} 个已不在 pages.json 的旧页面。`);
  }

  return `<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>光格像素工坊工程体检看板</title>
  <style>
    :root {
      --paper: #fffdf7;
      --surface: #ffffff;
      --ink: #111111;
      --muted: #5b5b5b;
      --yellow: #ffd23f;
      --blue: #74b9ff;
      --green: #88d498;
      --coral: #ff8a5b;
      --line: #111111;
      --shadow: 6px 6px 0 #111111;
      --soft-shadow: 4px 4px 0 #111111;
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      background: linear-gradient(180deg, #fff9df 0%, #fffdf7 28%, #ffffff 100%);
      color: var(--ink);
      font-family: "Avenir Next", "PingFang SC", "Noto Sans SC", sans-serif;
    }

    code, pre, .mono {
      font-family: "SFMono-Regular", "JetBrains Mono", "Menlo", monospace;
    }

    .page {
      max-width: 1480px;
      margin: 0 auto;
      padding: 32px 24px 56px;
    }

    .hero {
      background: var(--surface);
      border: 3px solid var(--line);
      box-shadow: var(--shadow);
      padding: 28px;
      display: grid;
      gap: 18px;
    }

    .hero-topline {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
    }

    .hero-kicker {
      display: inline-flex;
      align-items: center;
      min-height: 36px;
      padding: 0 12px;
      border: 2px solid var(--line);
      background: var(--yellow);
      font-size: 13px;
      font-weight: 800;
      letter-spacing: 0.04em;
    }

    .hero-meta {
      color: var(--muted);
      font-size: 14px;
    }

    h1 {
      margin: 0;
      font-size: clamp(36px, 5vw, 62px);
      line-height: 0.96;
      letter-spacing: -0.04em;
    }

    .hero-copy {
      max-width: 920px;
      color: var(--muted);
      font-size: 17px;
      line-height: 1.7;
    }

    .callout {
      border: 2px solid var(--line);
      background: #fff4c4;
      padding: 14px 16px;
      font-size: 14px;
      line-height: 1.65;
    }

    .section {
      margin-top: 28px;
      display: grid;
      gap: 16px;
    }

    .section-head {
      display: flex;
      flex-wrap: wrap;
      align-items: flex-end;
      justify-content: space-between;
      gap: 12px;
    }

    .section-title {
      margin: 0;
      font-size: 24px;
      font-weight: 900;
      letter-spacing: -0.03em;
    }

    .section-copy {
      color: var(--muted);
      font-size: 14px;
    }

    .metric-grid,
    .two-col,
    .three-col {
      display: grid;
      gap: 16px;
    }

    .metric-grid {
      grid-template-columns: repeat(4, minmax(0, 1fr));
    }

    .two-col {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .three-col {
      grid-template-columns: repeat(3, minmax(0, 1fr));
    }

    .metric-card,
    .panel,
    .recommend-card {
      background: var(--surface);
      border: 3px solid var(--line);
      box-shadow: var(--soft-shadow);
    }

    .metric-card {
      padding: 18px;
      display: grid;
      gap: 8px;
      min-height: 150px;
    }

    .metric-card.tone-yellow { background: #fff7cf; }
    .metric-card.tone-blue { background: #eef6ff; }
    .metric-card.tone-green { background: #eefbf0; }
    .metric-card.tone-coral { background: #fff1ea; }
    .tag.tone-yellow { background: #fff4c4; }
    .tag.tone-blue { background: #eef6ff; }
    .tag.tone-coral { background: #fff1ea; }

    .metric-label {
      color: var(--muted);
      font-size: 14px;
      font-weight: 700;
    }

    .metric-value {
      font-size: 38px;
      font-weight: 900;
      letter-spacing: -0.04em;
      line-height: 1;
    }

    .metric-copy {
      color: var(--muted);
      font-size: 14px;
      line-height: 1.6;
    }

    .panel {
      padding: 20px;
      display: grid;
      gap: 16px;
    }

    .panel-title {
      margin: 0;
      font-size: 18px;
      font-weight: 900;
    }

    .panel-copy {
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
    }

    .bar-row {
      display: grid;
      gap: 8px;
    }

    .bar-copy {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      font-size: 14px;
    }

    .bar-label {
      color: var(--ink);
      font-weight: 700;
    }

    .bar-value {
      color: var(--muted);
      font-weight: 800;
    }

    .bar-track {
      height: 14px;
      border: 2px solid var(--line);
      background: #ffffff;
      overflow: hidden;
    }

    .bar-fill {
      height: 100%;
      background: linear-gradient(90deg, var(--yellow) 0%, var(--blue) 100%);
    }

    .table-shell {
      overflow-x: auto;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      min-width: 720px;
      background: #ffffff;
    }

    th,
    td {
      border: 2px solid var(--line);
      padding: 10px 12px;
      text-align: left;
      vertical-align: top;
      font-size: 13px;
      line-height: 1.5;
    }

    th {
      background: #fff4c4;
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.05em;
    }

    .tag-list {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }

    .tag {
      display: inline-flex;
      align-items: center;
      min-height: 36px;
      padding: 0 12px;
      border: 2px solid var(--line);
      font-size: 13px;
      font-weight: 800;
      box-shadow: 3px 3px 0 #111111;
    }

    .empty-note {
      color: var(--muted);
      font-size: 14px;
    }

    .recommend-card {
      padding: 18px;
      display: grid;
      gap: 10px;
    }

    .recommend-level {
      display: inline-flex;
      align-items: center;
      width: fit-content;
      min-height: 32px;
      padding: 0 10px;
      background: #eef6ff;
      border: 2px solid var(--line);
      font-size: 12px;
      font-weight: 900;
      letter-spacing: 0.06em;
    }

    .recommend-card h3 {
      margin: 0;
      font-size: 18px;
      font-weight: 900;
    }

    .recommend-card p {
      margin: 0;
      color: var(--muted);
      font-size: 14px;
      line-height: 1.7;
    }

    .footer-note {
      margin-top: 28px;
      padding: 18px 20px;
      border: 3px solid var(--line);
      background: #ffffff;
      box-shadow: var(--soft-shadow);
      color: var(--muted);
      font-size: 14px;
      line-height: 1.8;
    }

    @media (max-width: 1100px) {
      .metric-grid,
      .three-col {
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }
    }

    @media (max-width: 820px) {
      .page {
        padding: 20px 14px 40px;
      }

      .metric-grid,
      .two-col,
      .three-col {
        grid-template-columns: minmax(0, 1fr);
      }

      .metric-value {
        font-size: 32px;
      }

      h1 {
        font-size: 40px;
      }
    }
  </style>
</head>
<body>
  <main class="page">
    <section class="hero">
      <div class="hero-topline">
        <div class="hero-kicker">光格像素工坊工程体检看板</div>
        <div class="hero-meta">生成时间 ${escapeHtml(snapshot.generatedAt)} · 根目录 <span class="mono">${escapeHtml(snapshot.rootDir)}</span></div>
      </div>
      <h1>把代码量、样式热区、资源体积和内存风险点一次看清。</h1>
      <div class="hero-copy">
        这份页面只面向当前仓库本地排查，统计范围主要覆盖 <code>pages</code>、<code>components</code>、<code>utils</code>、<code>styles</code>、<code>static</code> 等目录。
        “内存”部分是基于源码体积和静态资源的工程估算，不是真机运行时 profiler，但足够拿来判断哪里最该先动刀。
      </div>
      <div class="callout">
        当前工程共注册页面 <strong>${formatNumber(snapshot.routes.registeredPages)}</strong> 个，样式审计实际只覆盖 <strong>${formatNumber(snapshot.style.actualPageCount)}</strong> 个，
        覆盖率 <strong>${formatPercent(snapshot.style.pageCoverage)}</strong>。这已经能直接反映看板和样式台账之间的同步程度。
      </div>
    </section>

    <section class="section">
      <div class="metric-grid">
        ${overviewCards.map(renderMetricCard).join("")}
      </div>
    </section>

    <section class="section">
      <div class="section-head">
        <h2 class="section-title">代码结构</h2>
        <div class="section-copy">先看体量，再看集中度</div>
      </div>
      <div class="two-col">
        <section class="panel">
          <h3 class="panel-title">按文件类型拆分</h3>
          <div class="panel-copy"><code>.vue</code> 依旧是主要体量来源，<code>.js</code> 紧随其后。</div>
          ${extensionBars}
        </section>
        <section class="panel">
          <h3 class="panel-title">目录体积分布</h3>
          <div class="panel-copy"><code>static</code>、<code>pages</code>、<code>utils</code> 是目前最重的三块。</div>
          ${directoryBars}
        </section>
      </div>
      <div class="two-col">
        <section class="panel">
          <h3 class="panel-title">页面总量 Top 8</h3>
          ${totalPageTable}
        </section>
        <section class="panel">
          <h3 class="panel-title">源码大文件 Top 10</h3>
          ${sourceFileTable}
        </section>
      </div>
    </section>

    <section class="section">
      <div class="section-head">
        <h2 class="section-title">样式体检</h2>
        <div class="section-copy">看 style hot spot，也看审计快照有没有过期</div>
      </div>
      <div class="three-col">
        <section class="panel">
          <h3 class="panel-title">类名角色分布</h3>
          <div class="panel-copy">按条目数统计，<code>text</code>、<code>other</code>、<code>button</code> 占比最高。</div>
          ${roleBars}
        </section>
        <section class="panel">
          <h3 class="panel-title">缺失页面</h3>
          <div class="panel-copy">这些页面还没被当前样式审计快照纳入。</div>
          ${renderTagList(snapshot.style.missingPages, "yellow")}
        </section>
        <section class="panel">
          <h3 class="panel-title">陈旧页面</h3>
          <div class="panel-copy">这些路径仍留在快照里，但当前路由表已经没有。</div>
          ${renderTagList(snapshot.style.extraPages, "coral")}
        </section>
      </div>
      <div class="two-col">
        <section class="panel">
          <h3 class="panel-title">样式行数 Top 8</h3>
          ${stylePageTable}
        </section>
        <section class="panel">
          <h3 class="panel-title">共享度最高的类名分组</h3>
          ${sharedGroupTable}
        </section>
      </div>
    </section>

    <section class="section">
      <div class="section-head">
        <h2 class="section-title">体积与内存风险</h2>
        <div class="section-copy">这是工程侧静态估算，不是运行时堆内存抓取</div>
      </div>
      <div class="three-col">
        <section class="panel">
          <h3 class="panel-title">静态资源类型分布</h3>
          <div class="panel-copy">图片资源仍是主体，iconfont 本身也不轻。</div>
          ${assetBars}
        </section>
        <section class="panel">
          <h3 class="panel-title">资源热点摘要</h3>
          <div class="metric-copy">图片总量 ${formatBytes(snapshot.assets.imageBytes)}</div>
          <div class="metric-copy">iconfont 目录 ${formatBytes(snapshot.assets.iconfontBytes)}</div>
          <div class="metric-copy">figma 资产 ${formatBytes(snapshot.assets.figmaAssetBytes)}</div>
          <div class="metric-copy">clock theme 缩略图 ${formatBytes(snapshot.assets.themeThumbBytes)}</div>
          <div class="metric-copy">package-lock 体积 ${formatBytes(snapshot.packages.lockfileBytes)}</div>
          <div class="metric-copy">lock package 条目 ${formatNumber(snapshot.packages.packageEntries)}</div>
        </section>
        <section class="panel">
          <h3 class="panel-title">脚本/页面热点</h3>
          <div class="metric-copy">最大页面 <code>${escapeHtml(snapshot.pages.topByTotal[0].pagePath)}</code></div>
          <div class="metric-copy">总行数 ${formatNumber(snapshot.pages.topByTotal[0].totalLines)}</div>
          <div class="metric-copy">最大脚本占比 <code>${escapeHtml(snapshot.pages.topByScript[0].pagePath)}</code></div>
          <div class="metric-copy">脚本占比 ${formatPercent(snapshot.pages.topByScript[0].scriptRatio)}</div>
          <div class="metric-copy">30 KB 以上源码热点 ${formatNumber(snapshot.heavyRuntimeFiles.length)} 个</div>
        </section>
      </div>
      <div class="two-col">
        <section class="panel">
          <h3 class="panel-title">最重静态资源 Top 10</h3>
          ${heavyAssetTable}
        </section>
        <section class="panel">
          <h3 class="panel-title">大体积源码热点</h3>
          ${heavyRuntimeTable}
        </section>
      </div>
    </section>

    <section class="section">
      <div class="section-head">
        <h2 class="section-title">下一步建议</h2>
        <div class="section-copy">不是泛泛而谈，直接指向当前仓库最重的点</div>
      </div>
      <div class="two-col">
        ${recommendationCards}
      </div>
    </section>

    <div class="footer-note">
      <strong>额外观察：</strong>
      ${metaWarnings.length > 0 ? escapeHtml(metaWarnings.join(" ")) : "当前样式快照与路由表没有发现额外同步告警。"}
      如果后面你想继续刷新这份页面，直接重新运行 <code>npm run audit:dashboard</code> 就会覆盖生成最新 HTML。
    </div>
  </main>
</body>
</html>`;
}

function main() {
  ensureDir(OUTPUT_DIR);
  const snapshot = collectSnapshot();
  const html = buildHtml(snapshot);
  fs.writeFileSync(OUTPUT_FILE, html, "utf8");

  console.log(`已生成: ${OUTPUT_FILE}`);
  console.log(`源码总行数: ${formatNumber(snapshot.source.totalLines)}`);
  console.log(`注册页面: ${formatNumber(snapshot.routes.registeredPages)}`);
  console.log(`样式审计覆盖: ${formatNumber(snapshot.style.actualPageCount)}/${formatNumber(snapshot.routes.registeredPages)}`);
  console.log(`静态资源体积: ${formatBytes(snapshot.assets.totalBytes)}`);
}

main();
