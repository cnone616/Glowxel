#!/usr/bin/env python3

import json
import re
import shutil
import time
import unicodedata
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple
from urllib.parse import urljoin, urlparse

import requests
from bs4 import BeautifulSoup


BASE_URL = "https://www.spriters-resource.com"
ROOT_DIR = Path(__file__).resolve().parents[1]
TITLE_SCREEN_DIR = ROOT_DIR / "material-library" / "nes-title-screens"
OUTPUT_DIR = ROOT_DIR / "material-library" / "nes-game-assets"
SUMMARY_PATH = OUTPUT_DIR / "index.json"

SESSION = requests.Session()
SESSION.trust_env = False
SESSION.headers.update(
    {
        "User-Agent": (
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
            "AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/124.0.0.0 Safari/537.36"
        )
    }
)

MAP_KEYWORDS = (
    "stage",
    "stages",
    "map",
    "maps",
    "level",
    "levels",
    "room",
    "rooms",
    "area",
    "areas",
    "background",
    "backgrounds",
    "world",
    "course",
    "board",
    "maze",
    "zone",
    "scene",
    "track",
    "floor",
    "tileset",
    "tile set",
)

ITEM_KEYWORDS = (
    "item",
    "items",
    "weapon",
    "weapons",
    "pickup",
    "pickups",
    "power-up",
    "powerups",
    "power ups",
    "powerups",
    "capsule",
    "capsules",
    "bonus",
    "objects",
    "object",
    "icons",
    "icon",
    "hud",
    "inventory",
    "equipment",
    "sub-weapon",
    "sub weapon",
    "subweapons",
    "sub weapons",
)

SKIP_KEYWORDS = (
    "title screen",
    "title screens",
    "logo",
    "logos",
    "font",
    "fonts",
    "text",
    "alphabet",
)


@dataclass(frozen=True)
class GameTarget:
    game_id: str
    label: str
    page_url: str


TARGET_GAMES = (
    GameTarget("contra", "魂斗罗 Contra", f"{BASE_URL}/nes/contra/"),
    GameTarget("super-c", "超级魂斗罗 Super C", f"{BASE_URL}/nes/superc/"),
    GameTarget("battle-city", "坦克大战 Battle City", f"{BASE_URL}/nes/batcity/"),
    GameTarget("super-mario-bros", "超级马里奥兄弟", f"{BASE_URL}/nes/supermariobros/"),
    GameTarget("super-mario-bros-3", "超级马里奥兄弟3", f"{BASE_URL}/nes/supermariobros3/"),
    GameTarget("adventure-island", "冒险岛", f"{BASE_URL}/nes/advisle/"),
    GameTarget("adventure-island-2", "冒险岛2", f"{BASE_URL}/nes/advisle2/"),
    GameTarget("mega-man-2", "洛克人2", f"{BASE_URL}/nes/mm2/"),
    GameTarget("ninja-gaiden", "忍者龙剑传", f"{BASE_URL}/nes/ninjagaiden/"),
    GameTarget("double-dragon", "双截龙", f"{BASE_URL}/nes/doubledragon/"),
    GameTarget("double-dragon-2", "双截龙2", f"{BASE_URL}/nes/doubledragoniitherevenge/"),
    GameTarget("tetris", "俄罗斯方块", f"{BASE_URL}/nes/tetris/"),
    GameTarget("bomberman", "炸弹人", f"{BASE_URL}/nes/bomberman/"),
    GameTarget("chip-n-dale", "松鼠大作战", f"{BASE_URL}/nes/chipndale/"),
    GameTarget("tmnt", "忍者神龟", f"{BASE_URL}/nes/teenagemutantninjaturtles/"),
    GameTarget("jackal", "赤色要塞 Jackal", f"{BASE_URL}/nes/jackaltopgunner/"),
    GameTarget("life-force", "沙罗曼蛇 Life Force", f"{BASE_URL}/nes/lifeforcesalamander/"),
    GameTarget("gradius", "宇宙巡航机 Gradius", f"{BASE_URL}/nes/gradius/"),
    GameTarget("river-city-ransom", "热血物语", f"{BASE_URL}/nes/rivercityransomstreetgangs/"),
    GameTarget("mighty-final-fight", "热血快打", f"{BASE_URL}/nes/mffight/"),
    GameTarget("circus-charlie", "马戏团", f"{BASE_URL}/nes/circuscharliejpn/"),
    GameTarget("excitebike", "越野机车", f"{BASE_URL}/nes/excitebike/"),
    GameTarget("balloon-fight", "气球大战", f"{BASE_URL}/nes/balloonfight/"),
    GameTarget("ice-climber", "攀冰 Ice Climber", f"{BASE_URL}/nes/iceclimbers/"),
    GameTarget("pac-man", "吃豆人 Pac-Man", f"{BASE_URL}/nes/pacman/"),
    GameTarget("galaga", "小蜜蜂 Galaga", f"{BASE_URL}/nes/galaga/"),
    GameTarget("donkey-kong", "大金刚 Donkey Kong", f"{BASE_URL}/nes/dk/"),
    GameTarget("road-fighter", "火箭车 Road Fighter", f"{BASE_URL}/nes/roadfighter/"),
    GameTarget("spartan-x", "成龙踢馆 Spartan X", f"{BASE_URL}/nes/kungfu/"),
    GameTarget("yie-ar-kung-fu", "功夫 Yie Ar Kung-Fu", f"{BASE_URL}/nes/yiearkungfu/"),
    GameTarget("rush-n-attack", "绿色兵团 Rush'n Attack", f"{BASE_URL}/nes/rushnattack/"),
    GameTarget("metroid", "银河战士 Metroid", f"{BASE_URL}/nes/metroid/"),
    GameTarget("castlevania", "恶魔城 Castlevania", f"{BASE_URL}/nes/cv/"),
    GameTarget("dr-mario", "马力欧医生 Dr. Mario", f"{BASE_URL}/nes/drmario/"),
)


def fetch_soup(url: str) -> BeautifulSoup:
    response = SESSION.get(url, timeout=30)
    response.raise_for_status()
    return BeautifulSoup(response.text, "html.parser")


def normalize_text(text: str) -> str:
    text = unicodedata.normalize("NFKD", text).encode("ascii", "ignore").decode("ascii")
    text = text.lower()
    text = text.replace("&", " and ")
    text = text.replace("'", "")
    text = re.sub(r"[^a-z0-9]+", " ", text)
    return " ".join(text.split())


def slugify(text: str) -> str:
    text = unicodedata.normalize("NFKD", text).encode("ascii", "ignore").decode("ascii")
    text = text.lower()
    text = text.replace("&", " and ")
    text = text.replace("'", "")
    text = re.sub(r"[^a-z0-9]+", "_", text)
    text = text.strip("_")
    return text or "asset"


def collect_asset_links(game_url: str) -> List[Tuple[str, str]]:
    soup = fetch_soup(game_url)
    parsed = urlparse(game_url)
    game_prefix = parsed.path.rstrip("/")
    seen = set()
    assets: List[Tuple[str, str]] = []

    for anchor in soup.find_all("a", href=True):
        href = anchor["href"]
        if not href.startswith(f"{game_prefix}/asset/"):
            continue
        if not re.search(r"/asset/\d+/?$", href):
            continue

        title = " ".join(anchor.get_text(" ", strip=True).split())
        if not title:
            continue

        absolute = urljoin(BASE_URL, href)
        if absolute in seen:
            continue
        seen.add(absolute)
        assets.append((title, absolute))

    return assets


def categorize_asset(title: str) -> Optional[str]:
    normalized = normalize_text(title)
    if any(keyword in normalized for keyword in SKIP_KEYWORDS):
        return None
    if any(keyword in normalized for keyword in MAP_KEYWORDS):
        return "map"
    if any(keyword in normalized for keyword in ITEM_KEYWORDS):
        return "item"
    return "character"


def extract_download_url(asset_page_url: str) -> str:
    soup = fetch_soup(asset_page_url)
    for anchor in soup.find_all("a", href=True):
        href = anchor["href"]
        text = " ".join(anchor.get_text(" ", strip=True).split()).lower()
        if text == "download" or "/media/assets/" in href:
            return urljoin(BASE_URL, href)

    for image in soup.find_all("img", src=True):
        src = image["src"]
        if "/media/assets/" in src:
            return urljoin(BASE_URL, src)

    raise RuntimeError(f"未找到下载地址: {asset_page_url}")


def download_binary(url: str, destination: Path) -> None:
    response = SESSION.get(url, timeout=60)
    response.raise_for_status()
    destination.write_bytes(response.content)


def extension_from_url(url: str) -> str:
    suffix = Path(urlparse(url).path).suffix.lower()
    return suffix if suffix else ".png"


def ensure_clean_dir(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    summary = {
        "generatedAt": time.strftime("%Y-%m-%d %H:%M:%S"),
        "source": BASE_URL,
        "games": [],
    }

    for target in TARGET_GAMES:
        game_url = target.page_url
        assets = collect_asset_links(game_url)
        game_dir = OUTPUT_DIR / target.game_id
        ensure_clean_dir(game_dir)

        title_screen_src = TITLE_SCREEN_DIR / f"{target.game_id}.png"
        if title_screen_src.exists():
            shutil.copy2(title_screen_src, game_dir / "title_screen.png")

        category_counters = {"character": 0, "map": 0, "item": 0}
        saved_assets = []

        for title, asset_page_url in assets:
            category = categorize_asset(title)
            if category is None:
                continue

            download_url = extract_download_url(asset_page_url)
            category_counters[category] += 1
            ext = extension_from_url(download_url)
            filename = f"{category}_{category_counters[category]:02d}__{slugify(title)}{ext}"
            download_binary(download_url, game_dir / filename)
            saved_assets.append(
                {
                    "title": title,
                    "category": category,
                    "assetPage": asset_page_url,
                    "downloadUrl": download_url,
                    "filename": filename,
                }
            )
            time.sleep(0.1)

        manifest = {
            "gameId": target.game_id,
            "label": target.label,
            "gamePage": game_url,
            "counts": category_counters,
            "assets": saved_assets,
        }
        (game_dir / "sources.json").write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2),
            encoding="utf-8",
        )
        summary["games"].append(manifest)

    SUMMARY_PATH.write_text(
        json.dumps(summary, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
