#!/usr/bin/env python3

import json
import re
import tempfile
from pathlib import Path

import requests
from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By


ROOT_DIR = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT_DIR / "static" / "figma-assets"
ICON_DIR = OUTPUT_DIR / "icons-svg"
MODE_DIR = OUTPUT_DIR / "mode-badges"
ACHIEVEMENT_DIR = OUTPUT_DIR / "achievement-badges"
MANIFEST_PATH = OUTPUT_DIR / "manifest.json"
TEMP_DIR = Path(tempfile.gettempdir()) / "glowxel-figma-asset-build"

LUCIDE_BASE_URL = "https://unpkg.com/lucide-static/icons/{slug}.svg"
CHROME_BINARY = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"


BASIC_ICONS = [
    {"file": "icon-back.svg", "slug": "arrow-left", "label": "返回", "target": "direction-left"},
    {"file": "icon-close.svg", "slug": "x", "label": "关闭", "target": "close"},
    {"file": "icon-search.svg", "slug": "search", "label": "搜索", "target": "search"},
    {"file": "icon-settings.svg", "slug": "settings", "label": "设置", "target": "setting"},
    {"file": "icon-save.svg", "slug": "save", "label": "保存", "target": "save"},
    {"file": "icon-trash.svg", "slug": "trash-2", "label": "删除", "target": "delete"},
    {"file": "icon-edit.svg", "slug": "square-pen", "label": "编辑", "target": "edit"},
    {"file": "icon-refresh.svg", "slug": "refresh-cw", "label": "刷新", "target": "refresh"},
    {"file": "icon-image.svg", "slug": "image", "label": "图片", "target": "picture"},
    {"file": "icon-upload.svg", "slug": "upload", "label": "上传", "target": "upload"},
    {"file": "icon-download.svg", "slug": "download", "label": "下载", "target": "download"},
    {"file": "icon-link.svg", "slug": "link", "label": "链接", "target": "link"},
    {"file": "icon-play.svg", "slug": "play", "label": "播放", "target": "play"},
    {"file": "icon-message.svg", "slug": "message-square", "label": "消息", "target": "comment"},
    {"file": "icon-task.svg", "slug": "list-todo", "label": "任务", "target": "task"},
    {"file": "icon-calendar.svg", "slug": "calendar", "label": "日历", "target": "calendar"},
    {"file": "icon-success.svg", "slug": "check-circle", "label": "成功", "target": "success"},
    {"file": "icon-warning.svg", "slug": "alert-circle", "label": "警告", "target": "warning"},
    {"file": "icon-info.svg", "slug": "info", "label": "信息", "target": "info"},
    {"file": "icon-heart.svg", "slug": "heart", "label": "喜欢", "target": "heart"},
    {"file": "icon-star.svg", "slug": "star", "label": "收藏", "target": "star"},
    {"file": "icon-users.svg", "slug": "users", "label": "用户", "target": "users"},
    {"file": "icon-award.svg", "slug": "award", "label": "奖励", "target": "award"},
    {"file": "icon-bell.svg", "slug": "bell", "label": "通知", "target": "bell"},
    {"file": "icon-smartphone.svg", "slug": "smartphone", "label": "手机", "target": "mobile-phone"},
    {"file": "icon-scan.svg", "slug": "scan", "label": "扫描", "target": "scanning"},
    {"file": "icon-palette.svg", "slug": "palette", "label": "调色板", "target": "palette"},
    {"file": "icon-grid.svg", "slug": "grid-3x3", "label": "网格", "target": "grid"},
    {"file": "icon-clock.svg", "slug": "clock", "label": "时间", "target": "time"},
    {"file": "icon-trophy.svg", "slug": "trophy", "label": "奖杯", "target": "trophy"},
    {"file": "icon-crown.svg", "slug": "crown", "label": "皇冠", "target": "crown"},
    {"file": "icon-zap.svg", "slug": "zap", "label": "闪电", "target": "prompt"},
]


MODE_BADGES = [
    {"key": "eyes", "name": "桌面宠物", "slug": "paw-print", "start": "#EC4899", "end": "#F472B6"},
    {"key": "clock", "name": "静态时钟", "slug": "watch", "start": "#00D9D9", "end": "#1AE4E4"},
    {"key": "animation", "name": "动态时钟", "slug": "clock", "start": "#06B6D4", "end": "#22D3EE"},
    {"key": "theme", "name": "主题模式", "slug": "palette", "start": "#8B5CF6", "end": "#9B6FF7"},
    {"key": "canvas", "name": "画板模式", "slug": "canvas", "start": "#4F7FFF", "end": "#6B8FFF"},
    {"key": "tetris", "name": "方块屏保", "slug": "square", "start": "#7C3AED", "end": "#8B5CF6"},
    {"key": "text-display", "name": "文本展示", "slug": "type", "start": "#10B981", "end": "#20C997"},
    {"key": "ambient-light", "name": "环绕灯", "slug": "circle-dot", "start": "#F59E0B", "end": "#FBBF24"},
    {"key": "rhythm", "name": "律动", "slug": "zap", "start": "#FF7A45", "end": "#FF8B5C"},
    {"key": "countdown", "name": "倒计时", "slug": "timer", "start": "#EF4444", "end": "#F87171"},
    {"key": "stopwatch", "name": "秒表", "slug": "timer", "start": "#14B8A6", "end": "#2DD4BF"},
    {"key": "notification", "name": "通知提醒", "slug": "bell", "start": "#3B82F6", "end": "#60A5FA"},
]


ACHIEVEMENT_BADGES = [
    {"key": "first-create-common", "name": "首次创作", "slug": "sparkles", "start": "#9CA3AF", "end": "#6B7280"},
    {"key": "streak-7-rare", "name": "连续创作7天", "slug": "target", "start": "#60A5FA", "end": "#3B82F6"},
    {"key": "community-epic", "name": "社区达人", "slug": "users", "start": "#A78BFA", "end": "#8B5CF6"},
    {"key": "pixel-master-legendary", "name": "像素大师", "slug": "crown", "start": "#FBBF24", "end": "#F59E0B"},
]


PROJECT_ACHIEVEMENTS = [
    {"key": "project-first-work-common", "name": "初出茅庐", "slug": "image", "start": "#9CA3AF", "end": "#6B7280"},
    {"key": "project-work-master-common", "name": "作品达人", "slug": "images", "start": "#9CA3AF", "end": "#6B7280"},
    {"key": "project-pixel-master-rare", "name": "像素大师", "slug": "crown", "start": "#60A5FA", "end": "#3B82F6"},
    {"key": "project-legendary-creator-legendary", "name": "传奇创作者", "slug": "trophy", "start": "#FBBF24", "end": "#F59E0B"},
    {"key": "project-social-starter-common", "name": "社交新手", "slug": "heart", "start": "#9CA3AF", "end": "#6B7280"},
    {"key": "project-popular-author-rare", "name": "人气作者", "slug": "users", "start": "#60A5FA", "end": "#3B82F6"},
    {"key": "project-like-harvester-epic", "name": "点赞收割机", "slug": "thumbs-up", "start": "#A78BFA", "end": "#8B5CF6"},
    {"key": "project-color-artist-common", "name": "色彩搭配师", "slug": "palette", "start": "#9CA3AF", "end": "#6B7280"},
    {"key": "project-craftsman-rare", "name": "精密工匠", "slug": "target", "start": "#60A5FA", "end": "#3B82F6"},
    {"key": "project-night-owl-epic", "name": "夜猫子", "slug": "moon", "start": "#A78BFA", "end": "#8B5CF6"},
    {"key": "project-perfectionist-legendary", "name": "完美主义者", "slug": "calendar", "start": "#FBBF24", "end": "#F59E0B"},
]


RARITY_BADGES = [
    {"key": "common", "name": "普通", "slug": "trophy", "start": "#9CA3AF", "end": "#6B7280"},
    {"key": "rare", "name": "稀有", "slug": "trophy", "start": "#60A5FA", "end": "#3B82F6"},
    {"key": "epic", "name": "史诗", "slug": "trophy", "start": "#A78BFA", "end": "#8B5CF6"},
    {"key": "legendary", "name": "传说", "slug": "trophy", "start": "#FBBF24", "end": "#F59E0B"},
]


def ensure_dirs():
    for path in [OUTPUT_DIR, ICON_DIR, MODE_DIR, ACHIEVEMENT_DIR, TEMP_DIR]:
        path.mkdir(parents=True, exist_ok=True)


def fetch_lucide_svg(slug: str) -> str:
    response = requests.get(LUCIDE_BASE_URL.format(slug=slug), timeout=20)
    response.raise_for_status()
    return response.text


def clean_svg(svg_text: str) -> str:
    text = re.sub(r"<!--.*?-->", "", svg_text, flags=re.S).strip()
    return text


def svg_inner(svg_text: str) -> str:
    text = clean_svg(svg_text)
    match = re.search(r"<svg[^>]*>(.*)</svg>", text, flags=re.S)
    if not match:
        raise ValueError("无法解析 SVG")
    return match.group(1).strip()


def write_text(path: Path, content: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def make_icon_svg(slug: str) -> str:
    raw = clean_svg(fetch_lucide_svg(slug))
    raw = re.sub(r'width="[^"]*"', 'width="24"', raw)
    raw = re.sub(r'height="[^"]*"', 'height="24"', raw)
    raw = re.sub(r'stroke-width="[^"]*"', 'stroke-width="2"', raw)
    return raw


def make_canvas_icon_group() -> str:
    cells = []
    start = 74
    size = 20
    gap = 10
    for row in range(3):
        for col in range(3):
            x = start + col * (size + gap)
            y = start + row * (size + gap)
            cells.append(
                f'<rect x="{x}" y="{y}" width="{size}" height="{size}" rx="6" fill="rgba(255,255,255,0.72)" />'
            )
    return "".join(cells)


def make_badge_svg(slug: str, start: str, end: str, size: int = 240) -> str:
    icon_size = 84
    icon_x = (size - icon_size) / 2
    icon_y = (size - icon_size) / 2
    if slug == "canvas":
        icon_group = make_canvas_icon_group()
    else:
        inner = svg_inner(fetch_lucide_svg(slug))
        scale = icon_size / 24
        icon_group = (
            f'<g transform="translate({icon_x:.2f} {icon_y:.2f}) scale({scale:.4f})" '
            f'fill="none" stroke="#FFFFFF" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">'
            f"{inner}</g>"
        )

    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="0 0 {size} {size}" fill="none">
  <defs>
    <linearGradient id="badgeGradient" x1="32" y1="32" x2="{size - 32}" y2="{size - 32}" gradientUnits="userSpaceOnUse">
      <stop stop-color="{start}" />
      <stop offset="1" stop-color="{end}" />
    </linearGradient>
    <filter id="badgeShadow" x="0" y="0" width="{size}" height="{size}" filterUnits="userSpaceOnUse">
      <feDropShadow dx="0" dy="16" stdDeviation="16" flood-color="rgba(15,23,42,0.16)" />
    </filter>
  </defs>
  <rect x="36" y="36" width="{size - 72}" height="{size - 72}" rx="44" fill="url(#badgeGradient)" filter="url(#badgeShadow)" />
  {icon_group}
</svg>"""


def make_achievement_badge_svg(slug: str, start: str, end: str, size: int = 192) -> str:
    icon_size = 76
    icon_x = (size - icon_size) / 2
    icon_y = (size - icon_size) / 2
    inner = svg_inner(fetch_lucide_svg(slug))
    scale = icon_size / 24
    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="0 0 {size} {size}" fill="none">
  <defs>
    <linearGradient id="achievementGradient" x1="24" y1="24" x2="{size - 24}" y2="{size - 24}" gradientUnits="userSpaceOnUse">
      <stop stop-color="{start}" />
      <stop offset="1" stop-color="{end}" />
    </linearGradient>
    <filter id="achievementShadow" x="0" y="0" width="{size}" height="{size}" filterUnits="userSpaceOnUse">
      <feDropShadow dx="0" dy="14" stdDeviation="14" flood-color="rgba(15,23,42,0.18)" />
    </filter>
  </defs>
  <rect x="16" y="16" width="{size - 32}" height="{size - 32}" rx="34" fill="url(#achievementGradient)" filter="url(#achievementShadow)" />
  <rect x="16" y="16" width="{size - 32}" height="{size - 32}" rx="34" fill="white" fill-opacity="0.08" />
  <g transform="translate({icon_x:.2f} {icon_y:.2f}) scale({scale:.4f})" fill="none" stroke="#FFFFFF" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    {inner}
  </g>
</svg>"""


def build_driver():
    options = Options()
    options.add_argument("--headless=new")
    options.add_argument("--window-size=1200,1200")
    options.binary_location = CHROME_BINARY
    driver = webdriver.Chrome(options=options)
    driver.execute_cdp_cmd(
        "Emulation.setDefaultBackgroundColorOverride",
        {"color": {"r": 0, "g": 0, "b": 0, "a": 0}},
    )
    return driver


def render_png(driver, svg_text: str, output_path: Path, width: int, height: int):
    html = f"""<!doctype html>
<html>
  <body style="margin:0;background:transparent;">
    <div id="capture" style="width:{width}px;height:{height}px;background:transparent;">
      {svg_text}
    </div>
  </body>
</html>"""
    temp_html = TEMP_DIR / f"{output_path.stem}.html"
    temp_html.write_text(html, encoding="utf-8")
    driver.get(temp_html.as_uri())
    element = driver.find_element(By.ID, "capture")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    element.screenshot(str(output_path))


def generate_basic_icons(manifest: list):
    for icon in BASIC_ICONS:
        svg_content = make_icon_svg(icon["slug"])
        path = ICON_DIR / icon["file"]
        write_text(path, svg_content)
        manifest.append(
            {
                "type": "basic-icon",
                "file": str(path.relative_to(ROOT_DIR)),
                "label": icon["label"],
                "lucide_slug": icon["slug"],
                "suggested_target": icon["target"],
                "format": "svg",
                "size": "24x24",
            }
        )


def generate_mode_badges(driver, manifest: list):
    for badge in MODE_BADGES:
        svg_text = make_badge_svg(badge["slug"], badge["start"], badge["end"], 240)
        svg_path = MODE_DIR / f"badge-{badge['key']}@2x.svg"
        png_path = MODE_DIR / f"badge-{badge['key']}@2x.png"
        write_text(svg_path, svg_text)
        render_png(driver, svg_text, png_path, 240, 240)
        manifest.append(
            {
                "type": "mode-badge",
                "file": str(png_path.relative_to(ROOT_DIR)),
                "source_svg": str(svg_path.relative_to(ROOT_DIR)),
                "name": badge["name"],
                "key": badge["key"],
                "format": "png",
                "size": "120x120@2x",
            }
        )


def generate_achievement_badges(driver, manifest: list):
    for badge in ACHIEVEMENT_BADGES + PROJECT_ACHIEVEMENTS + RARITY_BADGES:
        svg_text = make_achievement_badge_svg(badge["slug"], badge["start"], badge["end"], 192)
        svg_path = ACHIEVEMENT_DIR / f"achievement-{badge['key']}@2x.svg"
        png_path = ACHIEVEMENT_DIR / f"achievement-{badge['key']}@2x.png"
        write_text(svg_path, svg_text)
        render_png(driver, svg_text, png_path, 192, 192)
        manifest.append(
            {
                "type": "achievement-badge",
                "file": str(png_path.relative_to(ROOT_DIR)),
                "source_svg": str(svg_path.relative_to(ROOT_DIR)),
                "name": badge["name"],
                "key": badge["key"],
                "format": "png",
                "size": "96x96@2x",
            }
        )


def main():
    ensure_dirs()
    manifest = []
    generate_basic_icons(manifest)
    driver = build_driver()
    try:
        generate_mode_badges(driver, manifest)
        generate_achievement_badges(driver, manifest)
    finally:
        driver.quit()

    MANIFEST_PATH.write_text(
        json.dumps(
            {
                "generator": "uniapp/scripts/generate_figma_assets.py",
                "source": "Implement Now (2)",
                "assets": manifest,
            },
            ensure_ascii=False,
            indent=2,
        ),
        encoding="utf-8",
    )
    print(f"Generated {len(manifest)} assets into {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
