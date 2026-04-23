#!/usr/bin/env python3

import json
import re
import sys
from collections import Counter, deque
from datetime import datetime
from pathlib import Path

from PIL import Image


ROOT_DIR = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT_DIR / "material-library" / "nes-game-assets" / "adventure-island"
OUTPUT_DIR = ASSET_DIR / "processed"
SPECS_DIR = ROOT_DIR / "scripts" / "character-animation-specs"
CHARACTER_ANIMATIONS_FILENAME = "character-animations.json"

MASTER_HIGGINS_SOURCE = "character_01__master_higgins.png"
ENEMIES_SOURCE = "character_02__enemies_and_bosses.png"
ITEMS_SOURCE = "item_01__items_and_objects.png"
TINA_SOURCE = "character_06__tina.png"
BEACH_CHUNKS_SOURCE = "character_03__beach_and_mountains_chunks.png"
BEACH_METATILES_SOURCE = "character_04__beach_and_mountains_metatiles.png"
CAVE_CHUNKS_SOURCE = "character_05__cave_chunks.png"
FOREST_CHUNKS_SOURCE = "map_01__forest_and_boss_rooms_chunks.png"
TITLE_SCREEN_SOURCE = "title_screen.png"

MASTER_HIGGINS_BOXES = [
    {"id": "frame_01", "sourceBox": {"x": 1, "y": 1, "width": 16, "height": 32}},
    {"id": "frame_02", "sourceBox": {"x": 18, "y": 1, "width": 16, "height": 32}},
    {"id": "frame_03", "sourceBox": {"x": 37, "y": 1, "width": 16, "height": 32}},
    {"id": "frame_04", "sourceBox": {"x": 54, "y": 1, "width": 24, "height": 32}},
    {"id": "frame_05", "sourceBox": {"x": 79, "y": 1, "width": 16, "height": 32}},
    {"id": "frame_06", "sourceBox": {"x": 96, "y": 1, "width": 24, "height": 32}},
    {"id": "frame_07", "sourceBox": {"x": 123, "y": 1, "width": 24, "height": 32}},
    {"id": "frame_08", "sourceBox": {"x": 150, "y": 1, "width": 24, "height": 32}},
    {"id": "frame_09", "sourceBox": {"x": 177, "y": 9, "width": 32, "height": 24}},
    {"id": "frame_10", "sourceBox": {"x": 210, "y": 9, "width": 32, "height": 24}},
    {"id": "frame_11", "sourceBox": {"x": 1, "y": 34, "width": 24, "height": 32}},
    {"id": "frame_12", "sourceBox": {"x": 28, "y": 34, "width": 24, "height": 32}},
    {"id": "frame_13", "sourceBox": {"x": 53, "y": 34, "width": 24, "height": 32}},
    {"id": "frame_14", "sourceBox": {"x": 80, "y": 34, "width": 24, "height": 32}},
    {"id": "frame_15", "sourceBox": {"x": 105, "y": 34, "width": 24, "height": 32}},
    {"id": "frame_16", "sourceBox": {"x": 132, "y": 34, "width": 24, "height": 32}},
    {"id": "frame_17", "sourceBox": {"x": 159, "y": 34, "width": 32, "height": 32}},
    {"id": "frame_18", "sourceBox": {"x": 194, "y": 34, "width": 32, "height": 32}},
    {"id": "frame_19", "sourceBox": {"x": 229, "y": 34, "width": 16, "height": 32}},
    {"id": "frame_20", "sourceBox": {"x": 257, "y": 50, "width": 8, "height": 16}},
    {"id": "frame_21", "sourceBox": {"x": 248, "y": 58, "width": 8, "height": 8}},
    {"id": "frame_22", "sourceBox": {"x": 1, "y": 79, "width": 16, "height": 32}},
    {"id": "frame_23", "sourceBox": {"x": 18, "y": 79, "width": 16, "height": 32}},
    {"id": "frame_24", "sourceBox": {"x": 35, "y": 79, "width": 24, "height": 32}},
    {"id": "frame_25", "sourceBox": {"x": 62, "y": 95, "width": 8, "height": 16}},
]

PRIMARY_RUN_CYCLE = ["frame_11", "frame_12", "frame_13", "frame_14", "frame_15", "frame_16"]

BEACH_RECOMMENDED_GROUPS = {
    "clouds": ["tile_r6_c4", "tile_r6_c5", "tile_r7_c3", "tile_r7_c4", "tile_r7_c5"],
    "ground": [
        "tile_r4_c3",
        "tile_r4_c4",
        "tile_r4_c5",
        "tile_r4_c6",
        "tile_r5_c0",
        "tile_r5_c1",
        "tile_r6_c0",
        "tile_r6_c1",
        "tile_r6_c2",
    ],
    "props": [
        "tile_r0_c0",
        "tile_r0_c1",
        "tile_r0_c3",
        "tile_r0_c4",
        "tile_r0_c5",
        "tile_r0_c6",
        "tile_r1_c4",
        "tile_r1_c5",
        "tile_r1_c6",
        "tile_r2_c4",
        "tile_r2_c5",
        "tile_r2_c6",
        "tile_r3_c0",
        "tile_r3_c1",
        "tile_r4_c0",
        "tile_r4_c1",
    ],
    "water_columns": [
        "tile_r5_c6",
        "tile_r5_c7",
        "tile_r5_c8",
        "tile_r5_c9",
        "tile_r6_c6",
        "tile_r6_c7",
        "tile_r6_c8",
        "tile_r6_c9",
        "tile_r7_c6",
        "tile_r7_c7",
        "tile_r7_c8",
        "tile_r7_c9",
        "tile_r8_c6",
        "tile_r8_c7",
        "tile_r8_c8",
        "tile_r8_c9",
    ],
}

ITEMS_SECTION_SPLIT_Y = 114
ITEMS_MAX_EXPORT_BOTTOM = 197
TINA_MAX_EXPORT_TOP = 32
ENEMIES_MAX_EXPORT_BOTTOM = 181
CHUNK_WIDTH = 128
CHUNK_HEIGHT = 352

GENERIC_OPAQUE_KEYWORDS = (
    "title screen",
    "intro",
    "ending",
    "endings",
    "menu",
    "menus",
    "cutscene",
    "cutscenes",
    "background",
    "backgrounds",
    "map",
    "maps",
    "stage",
    "stages",
    "level",
    "levels",
    "world",
    "worlds",
    "room",
    "rooms",
    "area",
    "areas",
    "track",
    "board",
    "maze",
    "scene",
    "scenes",
    "terrain",
    "tile",
    "tiles",
    "tile set",
    "tileset",
    "metatile",
    "metatiles",
    "chunk",
    "chunks",
    "playfield",
    "playfields",
    "hud",
    "inventory",
)

CHARACTER_TITLE_EXCLUDE_KEYWORDS = (
    "menu",
    "menus",
    "hud",
    "title",
    "credits",
    "opening",
    "ending",
    "endings",
    "tile",
    "tiles",
    "terrain",
    "playfield",
    "playfields",
    "metatile",
    "metatiles",
    "chunk",
    "chunks",
    "screen tiles",
    "battle mode",
    "goal roulette",
    "fireworks",
    "particles",
    "effects",
    "unused content",
    "tv screen",
    "tunnel",
    "bridge",
    "downtown",
    "mall",
    "warehouse",
    "factory",
    "park",
    "village",
    "thru-way",
)

GENERIC_MULTI_ROLE_KEYWORDS = (
    "enemies",
    "bosses",
    "obstacles",
    "npcs",
    "players",
    "fighters",
    "villains",
    "general sprites",
    "miscellaneous",
)

ITEM_CHARACTER_INCLUDE_KEYWORDS = (
    "character",
    "characters",
    "enemy",
    "enemies",
    "boss",
    "bosses",
    "player",
    "players",
)

ACTION_OVERRIDE_RULES = (
    {
        "keywords": ("bill rizer", "lance bean"),
        "actions": ("idle", "run", "shoot", "jump", "hurt"),
    },
    {
        "keywords": ("steve & ben",),
        "actions": ("idle", "run", "attack", "jump", "hurt"),
    },
    {
        "keywords": ("mario & luigi - super mario bros. 3",),
        "actions": ("idle", "walk", "run", "jump", "attack", "hurt", "transform"),
    },
    {
        "keywords": ("mario & luigi - super mario bros.",),
        "actions": ("idle", "walk", "run", "jump", "attack", "hurt"),
    },
    {
        "keywords": ("leonardo", "raphael", "michelangelo", "donatello"),
        "actions": ("idle", "walk", "jump", "attack", "hurt"),
    },
)

ACTION_LABELS = {
    "idle": "Idle",
    "walk": "Walk",
    "run": "Run",
    "jump": "Jump",
    "attack": "Attack",
    "shoot": "Shoot",
    "hurt": "Hurt",
    "climb": "Climb",
    "swim": "Swim",
    "roll": "Roll",
    "transform": "Transform",
    "knockdown": "Knockdown",
}

BASE_ACTION_IDS = ("idle", "walk", "run", "climb", "swim", "roll")
ONE_SHOT_ACTION_IDS = (
    "jump",
    "attack",
    "shoot",
    "hurt",
    "knockdown",
    "transform",
)
PREFERRED_BASE_ACTION_IDS = ("idle", "walk", "run")
STATIC_ROLE_KEYWORDS = (
    "turret",
    "turrets",
    "cannon",
    "cannons",
    "wall",
    "gate",
    "core",
    "pod",
    "launcher",
    "barrier",
)

MAX_AUTO_FRAME_DIMENSION = 512
MAX_AUTO_FRAME_AREA = 100000


def rgba_to_rgb_list(pixel):
    return [pixel[0], pixel[1], pixel[2]]


def write_json(path: Path, payload) -> None:
    path.write_text(
        json.dumps(payload, ensure_ascii=False, separators=(",", ":")),
        encoding="utf-8",
    )


def collect_border_pixels(image: Image.Image):
    width, height = image.size
    pixels = []

    for x in range(width):
        pixels.append(image.getpixel((x, 0)))
        pixels.append(image.getpixel((x, height - 1)))

    for y in range(1, height - 1):
        pixels.append(image.getpixel((0, y)))
        pixels.append(image.getpixel((width - 1, y)))

    return pixels


def collect_corner_pixels(image: Image.Image):
    width, height = image.size
    return [
        image.getpixel((0, 0)),
        image.getpixel((width - 1, 0)),
        image.getpixel((0, height - 1)),
        image.getpixel((width - 1, height - 1)),
    ]


def build_sparse_pixels(image: Image.Image, transparent_colors=None):
    transparent_colors = set(transparent_colors or [])
    pixels = []

    for y in range(image.height):
        for x in range(image.width):
            pixel = image.getpixel((x, y))
            if pixel in transparent_colors:
                continue
            pixels.append([x, y, pixel[0], pixel[1], pixel[2]])

    return pixels


def trim_sparse_pixels(pixels):
    if not pixels:
        return {"x": 0, "y": 0, "width": 0, "height": 0}, []

    min_x = min(pixel[0] for pixel in pixels)
    max_x = max(pixel[0] for pixel in pixels)
    min_y = min(pixel[1] for pixel in pixels)
    max_y = max(pixel[1] for pixel in pixels)

    trimmed = []
    for x, y, r, g, b in pixels:
        trimmed.append([x - min_x, y - min_y, r, g, b])

    trim_box = {
        "x": min_x,
        "y": min_y,
        "width": max_x - min_x + 1,
        "height": max_y - min_y + 1,
    }
    return trim_box, trimmed


def crop_by_box(image: Image.Image, source_box):
    x = source_box["x"]
    y = source_box["y"]
    width = source_box["width"]
    height = source_box["height"]
    return image.crop((x, y, x + width, y + height))


def build_sheet_row_groups(entries, row_gap_threshold: int = 16):
    groups = {}
    current_row_index = 0
    current_row_anchor = None

    for entry in entries:
        source_box = entry["sourceBox"]
        y = source_box["y"]

        if current_row_anchor is None or y - current_row_anchor > row_gap_threshold:
            current_row_index += 1
            current_row_anchor = y

        group_key = f"sheet_row_{current_row_index:02d}"
        if group_key not in groups:
            groups[group_key] = []
        groups[group_key].append(entry["id"])

    return groups


def build_box_rows(boxes, row_gap_threshold: int = 16):
    rows = []
    current_row = []
    current_row_anchor = None

    for box in boxes:
        if current_row_anchor is None or box["y"] - current_row_anchor > row_gap_threshold:
            if current_row:
                rows.append(current_row)
            current_row = [box]
            current_row_anchor = box["y"]
            continue

        current_row.append(box)

    if current_row:
        rows.append(current_row)

    return rows


def infer_footer_start_y(boxes, image_width: int):
    rows = build_box_rows(boxes)
    if len(rows) < 2:
        return None

    footer_start_y = None

    for index in range(len(rows) - 1, 0, -1):
        row = rows[index]
        previous_row = rows[index - 1]
        row_min_y = min(box["y"] for box in row)
        previous_row_max_y = max(box["y"] + box["height"] - 1 for box in previous_row)
        gap_from_previous = row_min_y - previous_row_max_y - 1

        average_height = sum(box["height"] for box in row) / len(row)
        max_width = max(box["width"] for box in row)
        max_height = max(box["height"] for box in row)
        wide_banner = max_width >= image_width * 0.5 and max_height <= 32
        tiny_dense_row = len(row) >= 6 and average_height <= 16
        tiny_row = average_height <= 10

        if gap_from_previous >= 24 and (wide_banner or tiny_dense_row or tiny_row):
            footer_start_y = row_min_y
            continue

        if footer_start_y is not None:
            break

    return footer_start_y


def filter_footer_boxes(boxes, image_width: int):
    footer_start_y = infer_footer_start_y(boxes, image_width)
    if footer_start_y is None:
        return boxes, None

    filtered_boxes = [box for box in boxes if box["y"] < footer_start_y]
    return filtered_boxes, footer_start_y


def get_generic_export_mode(asset_meta):
    filename = asset_meta["filename"].lower()
    title = asset_meta["title"].lower()
    combined = f"{filename} {title}"

    if filename == TITLE_SCREEN_SOURCE or "title screen" in combined:
        return "opaque"
    if asset_meta["category"] == "map":
        return "opaque"
    if any(keyword in combined for keyword in GENERIC_OPAQUE_KEYWORDS):
        return "opaque"

    return "transparent"


def find_connected_component_boxes(image: Image.Image, min_pixels: int):
    width, height = image.size
    sheet_background = image.getpixel((0, 0))
    visited = [[False for _ in range(width)] for _ in range(height)]
    boxes = []

    for y in range(height):
        for x in range(width):
            if visited[y][x]:
                continue

            visited[y][x] = True
            if image.getpixel((x, y)) == sheet_background:
                continue

            queue = deque([(x, y)])
            min_x = x
            max_x = x
            min_y = y
            max_y = y
            pixel_count = 0

            while queue:
                current_x, current_y = queue.popleft()
                pixel_count += 1
                min_x = min(min_x, current_x)
                max_x = max(max_x, current_x)
                min_y = min(min_y, current_y)
                max_y = max(max_y, current_y)

                for next_x, next_y in (
                    (current_x + 1, current_y),
                    (current_x - 1, current_y),
                    (current_x, current_y + 1),
                    (current_x, current_y - 1),
                ):
                    if not (0 <= next_x < width and 0 <= next_y < height):
                        continue
                    if visited[next_y][next_x]:
                        continue

                    visited[next_y][next_x] = True
                    if image.getpixel((next_x, next_y)) != sheet_background:
                        queue.append((next_x, next_y))

            if pixel_count < min_pixels:
                continue

            boxes.append(
                {
                    "x": min_x,
                    "y": min_y,
                    "width": max_x - min_x + 1,
                    "height": max_y - min_y + 1,
                    "pixelCountOnSheet": pixel_count,
                }
            )

    boxes.sort(key=lambda box: (box["y"], box["x"]))
    return boxes


def build_transparent_component(image: Image.Image, source_box, component_id):
    crop = crop_by_box(image, source_box)
    sheet_background = image.getpixel((0, 0))
    border_counter = Counter(collect_border_pixels(crop))
    inferred_background = border_counter.most_common(1)[0][0]
    transparent_colors = [inferred_background]

    if inferred_background != sheet_background:
        transparent_colors.append(sheet_background)

    sparse_pixels = build_sparse_pixels(crop, transparent_colors=transparent_colors)
    trim_box, trimmed_pixels = trim_sparse_pixels(sparse_pixels)
    trimmed_width = trim_box["width"]
    trimmed_height = trim_box["height"]

    return {
        "id": component_id,
        "sourceBox": source_box,
        "backgroundMode": "transparent",
        "transparentColors": [rgba_to_rgb_list(color) for color in transparent_colors],
        "trimBox": trim_box,
        "size": {"width": trimmed_width, "height": trimmed_height},
        "origin": {"x": trimmed_width // 2, "y": max(trimmed_height - 1, 0)},
        "pixelCount": len(trimmed_pixels),
        "pixels": trimmed_pixels,
    }


def build_opaque_component(image: Image.Image, source_box, component_id, extra=None):
    crop = crop_by_box(image, source_box)
    pixels = build_sparse_pixels(crop)
    unique_colors = sorted({tuple(pixel[2:5]) for pixel in pixels})
    payload = {
        "id": component_id,
        "sourceBox": source_box,
        "backgroundMode": "opaque",
        "size": {"width": source_box["width"], "height": source_box["height"]},
        "origin": {"x": 0, "y": 0},
        "pixelCount": len(pixels),
        "palette": [list(color) for color in unique_colors],
        "pixels": pixels,
    }

    if extra:
        payload.update(extra)

    return payload


def build_master_higgins_asset():
    image = Image.open(ASSET_DIR / MASTER_HIGGINS_SOURCE).convert("RGBA")
    frames = []

    for frame in MASTER_HIGGINS_BOXES:
        frames.append(build_transparent_component(image, frame["sourceBox"], frame["id"]))

    return {
        "gameId": "adventure-island",
        "assetId": "master_higgins",
        "label": "Master Higgins",
        "source": MASTER_HIGGINS_SOURCE,
        "pixelFormat": ["x", "y", "r", "g", "b"],
        "frameOrder": [frame["id"] for frame in frames],
        "recommendedAnimations": {
            "primary_run_cycle": PRIMARY_RUN_CYCLE,
            "sheet_row_01": ["frame_01", "frame_02", "frame_03", "frame_04", "frame_05", "frame_06", "frame_07", "frame_08"],
            "sheet_row_02": ["frame_11", "frame_12", "frame_13", "frame_14", "frame_15", "frame_16", "frame_17", "frame_18"],
            "sheet_row_03": ["frame_22", "frame_23", "frame_24"],
        },
        "notes": [
            "第二排连续 6 帧已单独归为 primary_run_cycle，优先用于横向跑动。",
            "主角帧会去掉每个切片自己的边框底色，以及整张图的深蓝外底色。",
            "frame_20 和 frame_21 是单独的小物件切片，保留用于后续扩展。",
        ],
        "frames": frames,
    }


def build_tina_asset():
    image = Image.open(ASSET_DIR / TINA_SOURCE).convert("RGBA")
    component_boxes = find_connected_component_boxes(image, min_pixels=20)
    exported_boxes = [box for box in component_boxes if box["y"] <= TINA_MAX_EXPORT_TOP]
    frames = []

    for index, box in enumerate(exported_boxes, start=1):
        frames.append(
            build_transparent_component(
                image,
                {"x": box["x"], "y": box["y"], "width": box["width"], "height": box["height"]},
                f"frame_{index:02d}",
            )
        )

    return {
        "gameId": "adventure-island",
        "assetId": "tina",
        "label": "Tina",
        "source": TINA_SOURCE,
        "pixelFormat": ["x", "y", "r", "g", "b"],
        "frameOrder": [frame["id"] for frame in frames],
        "recommendedAnimations": {"sheet_order": [frame["id"] for frame in frames]},
        "notes": [
            "只导出上方实际角色帧，底部版权说明文字不参与像素导出。",
            "Tina 帧同样做了切片底色和整张图深蓝外底色抠除。",
        ],
        "frames": frames,
    }


def build_enemies_and_bosses_asset():
    image = Image.open(ASSET_DIR / ENEMIES_SOURCE).convert("RGBA")
    component_boxes = find_connected_component_boxes(image, min_pixels=20)
    exported_boxes = [
        box for box in component_boxes if box["y"] + box["height"] - 1 <= ENEMIES_MAX_EXPORT_BOTTOM
    ]
    components = []

    for index, box in enumerate(exported_boxes, start=1):
        component_id = f"component_{index:02d}"
        components.append(
            build_transparent_component(
                image,
                {"x": box["x"], "y": box["y"], "width": box["width"], "height": box["height"]},
                component_id,
            )
        )

    return {
        "gameId": "adventure-island",
        "assetId": "enemies_and_bosses",
        "label": "Enemies and Bosses",
        "source": ENEMIES_SOURCE,
        "pixelFormat": ["x", "y", "r", "g", "b"],
        "componentOrder": [component["id"] for component in components],
        "recommendedGroups": build_sheet_row_groups(components),
        "notes": [
            "按素材表自然连通区域拆分导出，保留敌人与 Boss 的独立切片。",
            "每个组件都会去掉切片底色和整张图深蓝外底色，适合直接叠到主题场景里。",
            "底部版权说明与文字图例区域已整体排除，不会混进可用组件。",
        ],
        "components": components,
    }


def build_items_asset():
    image = Image.open(ASSET_DIR / ITEMS_SOURCE).convert("RGBA")
    component_boxes = find_connected_component_boxes(image, min_pixels=20)
    exported_boxes = [
        box for box in component_boxes if box["y"] + box["height"] - 1 <= ITEMS_MAX_EXPORT_BOTTOM
    ]
    components = []
    upper_section_ids = []
    lower_pattern_ids = []

    for index, box in enumerate(exported_boxes, start=1):
        component_id = f"component_{index:02d}"
        source_box = {"x": box["x"], "y": box["y"], "width": box["width"], "height": box["height"]}

        if box["y"] < ITEMS_SECTION_SPLIT_Y:
            components.append(build_transparent_component(image, source_box, component_id))
            upper_section_ids.append(component_id)
        else:
            components.append(build_opaque_component(image, source_box, component_id))
            lower_pattern_ids.append(component_id)

    return {
        "gameId": "adventure-island",
        "assetId": "items_and_objects",
        "label": "Items and Objects",
        "source": ITEMS_SOURCE,
        "pixelFormat": ["x", "y", "r", "g", "b"],
        "componentOrder": [component["id"] for component in components],
        "recommendedGroups": {
            "upper_section": upper_section_ids,
            "lower_pattern_rows": lower_pattern_ids,
        },
        "notes": [
            "上半区组件按透明素材导出，适合直接当道具/对象精灵使用。",
            "下半区连续花纹条按不透明素材导出，适合直接做边框、地面或 UI 条带。",
            "最底部版权说明文字整块已排除，不会混进组件数据。",
        ],
        "components": components,
    }


def build_beach_metatiles_asset():
    image = Image.open(ASSET_DIR / BEACH_METATILES_SOURCE).convert("RGBA")
    tiles = []
    tile_size = 32
    gap = 1
    start_offset = 1

    for row in range(9):
        for col in range(10):
            x = start_offset + col * (tile_size + gap)
            y = start_offset + row * (tile_size + gap)
            crop = image.crop((x, y, x + tile_size, y + tile_size))
            colors = set(crop.getdata())

            if len(colors) == 1 and next(iter(colors)) == image.getpixel((0, 0)):
                continue

            tile = build_opaque_component(
                image,
                {"x": x, "y": y, "width": tile_size, "height": tile_size},
                f"tile_r{row}_c{col}",
                extra={"grid": {"row": row, "col": col}},
            )
            tiles.append(tile)

    return {
        "gameId": "adventure-island",
        "assetId": "beach_mountains_metatiles",
        "label": "Beach and Mountains Metatiles",
        "source": BEACH_METATILES_SOURCE,
        "pixelFormat": ["x", "y", "r", "g", "b"],
        "grid": {"rows": 9, "cols": 10, "tileWidth": 32, "tileHeight": 32, "gap": 1, "startOffset": 1},
        "recommendedGroups": BEACH_RECOMMENDED_GROUPS,
        "notes": [
            "这批背景块保留 32x32 原始满块像素，不做透明抠除。",
            "云层、地面、建筑和水柱已在 recommendedGroups 里单独分组。",
            "如果后续要做更细粒度的背景拼装，再从这些 tile 继续二次组合即可。",
        ],
        "tiles": tiles,
    }


def build_chunk_asset(source_name: str, asset_id: str, label: str):
    image = Image.open(ASSET_DIR / source_name).convert("RGBA")
    component_boxes = find_connected_component_boxes(image, min_pixels=20)
    exported_boxes = [
        box for box in component_boxes if box["width"] == CHUNK_WIDTH and box["height"] == CHUNK_HEIGHT
    ]
    chunks = []
    current_row_index = 0
    current_row_anchor = None
    row_groups = {}

    for index, box in enumerate(exported_boxes, start=1):
        chunk_id = f"chunk_{index:02d}"
        if current_row_anchor is None or box["y"] - current_row_anchor > 16:
            current_row_index += 1
            current_row_anchor = box["y"]
        layout_row = current_row_index
        group_key = f"sheet_row_{layout_row:02d}"
        if group_key not in row_groups:
            row_groups[group_key] = []
        row_groups[group_key].append(chunk_id)

        chunks.append(
            build_opaque_component(
                image,
                {"x": box["x"], "y": box["y"], "width": box["width"], "height": box["height"]},
                chunk_id,
                extra={"layoutRow": layout_row},
            )
        )

    return {
        "gameId": "adventure-island",
        "assetId": asset_id,
        "label": label,
        "source": source_name,
        "pixelFormat": ["x", "y", "r", "g", "b"],
        "chunkSize": {"width": CHUNK_WIDTH, "height": CHUNK_HEIGHT},
        "chunkOrder": [chunk["id"] for chunk in chunks],
        "recommendedGroups": row_groups,
        "notes": [
            "只导出整块 128x352 场景 chunk，底部图例、调色板和说明文字不参与像素导出。",
            "chunk 保留原始不透明像素，适合直接做整屏/整段场景替换或滚动参考。",
            "recommendedGroups 会按原图实际排数自动分组，不再假定素材表只有两排。",
        ],
        "chunks": chunks,
    }


def build_title_screen_asset():
    image = Image.open(ASSET_DIR / TITLE_SCREEN_SOURCE).convert("RGBA")
    source_box = {"x": 0, "y": 0, "width": image.width, "height": image.height}
    component = build_opaque_component(image, source_box, "component_01")

    return {
        "gameId": "adventure-island",
        "assetId": "title_screen",
        "label": "Title Screen",
        "source": TITLE_SCREEN_SOURCE,
        "pixelFormat": ["x", "y", "r", "g", "b"],
        "componentOrder": [component["id"]],
        "recommendedGroups": {"full_screen": [component["id"]]},
        "notes": [
            "整张标题图按满屏不透明像素导出，仅作为素材参考，不放进默认主题推荐组合。",
        ],
        "components": [component],
    }


def build_manifest():
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    return {
        "gameId": "adventure-island",
        "label": "冒险岛",
        "generatedAt": timestamp,
        "rawSources": {
            "player": MASTER_HIGGINS_SOURCE,
            "enemies": ENEMIES_SOURCE,
            "npc": TINA_SOURCE,
            "items": ITEMS_SOURCE,
            "beachMetatiles": BEACH_METATILES_SOURCE,
            "beachSceneReference": BEACH_CHUNKS_SOURCE,
            "caveSceneReference": CAVE_CHUNKS_SOURCE,
            "alternateBiome": FOREST_CHUNKS_SOURCE,
            "titleScreen": TITLE_SCREEN_SOURCE,
        },
        "outputs": [
            {
                "id": "master_higgins",
                "path": "processed/master_higgins.json",
                "description": "主角动作像素数据，已做透明抠除。",
            },
            {
                "id": "enemies_and_bosses",
                "path": "processed/enemies_and_bosses.json",
                "description": "敌人与 Boss 的散片像素数据，已做透明抠除。",
            },
            {
                "id": "tina",
                "path": "processed/tina.json",
                "description": "Tina 角色帧像素数据，已做透明抠除。",
            },
            {
                "id": "items_and_objects",
                "path": "processed/items_and_objects.json",
                "description": "道具/物件像素数据，上半区透明导出，下半区花纹条不透明导出。",
            },
            {
                "id": "beach_mountains_metatiles",
                "path": "processed/beach_mountains_metatiles.json",
                "description": "海滩/山地 32x32 场景块像素数据，保留原始满块像素。",
            },
            {
                "id": "beach_mountains_chunks",
                "path": "processed/beach_mountains_chunks.json",
                "description": "海滩/山地整块场景 chunk 像素数据。",
            },
            {
                "id": "cave_chunks",
                "path": "processed/cave_chunks.json",
                "description": "洞穴整块场景 chunk 像素数据。",
            },
            {
                "id": "forest_and_boss_rooms_chunks",
                "path": "processed/forest_and_boss_rooms_chunks.json",
                "description": "森林/Boss 房整块场景 chunk 像素数据。",
            },
            {
                "id": "title_screen",
                "path": "processed/title_screen.json",
                "description": "标题封面整屏像素数据，仅作参考展示。",
            },
        ],
        "recommendedBuild": {
            "playerAnimation": {"assetId": "master_higgins", "group": "primary_run_cycle"},
            "backgroundTiles": {
                "assetId": "beach_mountains_metatiles",
                "groups": ["ground", "clouds", "props", "water_columns"],
            },
            "backgroundChunks": [
                {"assetId": "beach_mountains_chunks", "group": "sheet_row_01"},
                {"assetId": "cave_chunks", "group": "sheet_row_01"},
                {"assetId": "forest_and_boss_rooms_chunks", "group": "sheet_row_01"},
            ],
            "items": {"assetId": "items_and_objects", "group": "upper_section"},
            "npc": {"assetId": "tina", "group": "sheet_order"},
        },
        "nextSuggestedRawFiles": [],
    }


def build_generic_asset(game_id: str, game_label: str, game_dir: Path, asset_meta):
    image_path = game_dir / asset_meta["filename"]
    image = Image.open(image_path).convert("RGBA")
    asset_id = image_path.stem
    export_mode = get_generic_export_mode(asset_meta)

    if asset_meta["filename"] == TITLE_SCREEN_SOURCE:
        source_box = {"x": 0, "y": 0, "width": image.width, "height": image.height}
        component = build_opaque_component(image, source_box, "component_01")
        return {
            "gameId": game_id,
            "assetId": asset_id,
            "label": asset_meta["title"],
            "source": asset_meta["filename"],
            "category": asset_meta["category"],
            "pixelFormat": ["x", "y", "r", "g", "b"],
            "componentOrder": [component["id"]],
            "recommendedGroups": {"full_screen": [component["id"]]},
            "notes": [
                f"{game_label} 标题图按整屏不透明像素导出，保留原始布局。",
            ],
            "components": [component],
        }

    component_boxes = find_connected_component_boxes(image, min_pixels=20)
    export_boxes, footer_start_y = filter_footer_boxes(component_boxes, image.width)
    if not export_boxes:
        raise RuntimeError(f"{game_id}/{asset_meta['filename']} 未检测到可导出的组件")

    components = []
    builder = build_opaque_component if export_mode == "opaque" else build_transparent_component

    for index, box in enumerate(export_boxes, start=1):
        component_id = f"component_{index:02d}"
        source_box = {"x": box["x"], "y": box["y"], "width": box["width"], "height": box["height"]}
        components.append(builder(image, source_box, component_id))

    notes = []
    if export_mode == "opaque":
        notes.append("按自然连通区域拆分导出，保留组件完整不透明像素。")
    else:
        notes.append("按自然连通区域拆分导出，并对组件边框底色/整图外底色做透明抠除。")
    if footer_start_y is not None:
        notes.append(f"检测到底部说明区，自 y={footer_start_y} 起的组件已排除。")

    return {
        "gameId": game_id,
        "assetId": asset_id,
        "label": asset_meta["title"],
        "source": asset_meta["filename"],
        "category": asset_meta["category"],
        "pixelFormat": ["x", "y", "r", "g", "b"],
        "componentOrder": [component["id"] for component in components],
        "recommendedGroups": build_sheet_row_groups(components),
        "notes": notes,
        "components": components,
    }


def load_game_assets(game_dir: Path):
    sources_path = game_dir / "sources.json"
    data = json.loads(sources_path.read_text(encoding="utf-8"))
    assets = [
        {
            "title": item["title"],
            "category": item["category"],
            "filename": item["filename"],
        }
        for item in data.get("assets", [])
    ]

    title_screen_path = game_dir / TITLE_SCREEN_SOURCE
    if title_screen_path.exists():
        assets.append(
            {
                "title": "Title Screen",
                "category": "map",
                "filename": TITLE_SCREEN_SOURCE,
            }
        )

    return data, assets


def build_generic_manifest(game_data, processed_assets):
    raw_sources = {}
    outputs = []

    for asset in processed_assets:
        raw_sources[asset["assetId"]] = asset["source"]
        if asset["source"] == TITLE_SCREEN_SOURCE:
            description = "标题封面整屏像素数据，保留原始布局。"
        elif asset["components"][0]["backgroundMode"] == "opaque":
            description = f"{asset['label']} 像素数据，保留不透明像素。"
        else:
            description = f"{asset['label']} 像素数据，已做透明抠除。"
        outputs.append(
            {
                "id": asset["assetId"],
                "path": f"processed/{asset['assetId']}.json",
                "description": description,
            }
        )

    return {
        "gameId": game_data["gameId"],
        "label": game_data["label"],
        "generatedAt": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "rawSources": raw_sources,
        "outputs": outputs,
        "nextSuggestedRawFiles": [],
    }


def read_required_string(payload, key_path):
    value = payload
    for key in key_path:
        if not isinstance(value, dict) or key not in value:
            raise RuntimeError(f"缺少字段: {'.'.join(key_path)}")
        value = value[key]

    if not isinstance(value, str) or not value.strip():
        raise RuntimeError(f"字段必须是非空字符串: {'.'.join(key_path)}")
    return value


def read_required_bool(payload, key_path):
    value = payload
    for key in key_path:
        if not isinstance(value, dict) or key not in value:
            raise RuntimeError(f"缺少字段: {'.'.join(key_path)}")
        value = value[key]

    if not isinstance(value, bool):
        raise RuntimeError(f"字段必须是布尔值: {'.'.join(key_path)}")
    return value


def normalize_integer_dict(payload, key_path, keys):
    value = payload
    for key in key_path:
        if not isinstance(value, dict) or key not in value:
            raise RuntimeError(f"缺少字段: {'.'.join(key_path)}")
        value = value[key]

    if not isinstance(value, dict):
        raise RuntimeError(f"字段必须是对象: {'.'.join(key_path)}")

    normalized = {}
    for item_key in keys:
        item_value = value.get(item_key)
        if not isinstance(item_value, int):
            raise RuntimeError(f"字段必须是整数: {'.'.join(key_path + [item_key])}")
        normalized[item_key] = item_value
    return normalized


def normalize_source_box(payload, key_path):
    source_box = normalize_integer_dict(payload, key_path, ["x", "y", "width", "height"])
    if source_box["width"] <= 0 or source_box["height"] <= 0:
        raise RuntimeError(f"sourceBox 尺寸必须大于 0: {'.'.join(key_path)}")
    return source_box


def normalize_origin(payload, key_path):
    return normalize_integer_dict(payload, key_path, ["x", "y"])


def ensure_box_within_image(image: Image.Image, source_box, error_prefix: str):
    if source_box["x"] < 0 or source_box["y"] < 0:
        raise RuntimeError(f"{error_prefix} sourceBox 不能为负数")

    if source_box["x"] + source_box["width"] > image.width:
        raise RuntimeError(f"{error_prefix} sourceBox 超出原图宽度")

    if source_box["y"] + source_box["height"] > image.height:
        raise RuntimeError(f"{error_prefix} sourceBox 超出原图高度")


def slugify(value: str) -> str:
    normalized = re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-")
    if normalized:
        return normalized
    return "role"


def validate_unique_id(id_set, candidate_id: str, error_prefix: str):
    if candidate_id in id_set:
        raise RuntimeError(f"{error_prefix} 存在重复 id: {candidate_id}")
    id_set.add(candidate_id)


def make_unique_id(existing_ids, base_id: str) -> str:
    if base_id not in existing_ids:
        return base_id

    suffix = 2
    while True:
        candidate_id = f"{base_id}-{suffix:02d}"
        if candidate_id not in existing_ids:
            return candidate_id
        suffix += 1


def coerce_color_tuple(color_value):
    if isinstance(color_value, (list, tuple)):
        if len(color_value) == 4:
            return tuple(color_value)
        if len(color_value) == 3:
            return (color_value[0], color_value[1], color_value[2], 255)
    raise RuntimeError("颜色值必须是长度为 3 或 4 的数组")


def color_distance(color_a, color_b):
    a = coerce_color_tuple(color_a)
    b = coerce_color_tuple(color_b)
    return (
        (a[0] - b[0]) ** 2
        + (a[1] - b[1]) ** 2
        + (a[2] - b[2]) ** 2
    ) ** 0.5


def is_similar_matte_color(color, matte_palette, threshold: int = 90) -> bool:
    if not matte_palette:
        return False

    candidate = coerce_color_tuple(color)
    for matte_color in matte_palette:
        if color_distance(candidate, matte_color) <= threshold:
            return True
    return False


def box_intersects(box_a, box_b, pad_x: int = 0, pad_y: int = 0) -> bool:
    return not (
        box_a["x"] + box_a["width"] + pad_x <= box_b["x"] - pad_x
        or box_b["x"] + box_b["width"] + pad_x <= box_a["x"] - pad_x
        or box_a["y"] + box_a["height"] + pad_y <= box_b["y"] - pad_y
        or box_b["y"] + box_b["height"] + pad_y <= box_a["y"] - pad_y
    )


def box_contains(outer_box, inner_box, pad_x: int = 0, pad_y: int = 0) -> bool:
    return (
        outer_box["x"] - pad_x <= inner_box["x"]
        and outer_box["y"] - pad_y <= inner_box["y"]
        and outer_box["x"] + outer_box["width"] - 1 + pad_x
        >= inner_box["x"] + inner_box["width"] - 1
        and outer_box["y"] + outer_box["height"] - 1 + pad_y
        >= inner_box["y"] + inner_box["height"] - 1
    )


def box_area(source_box) -> int:
    return source_box["width"] * source_box["height"]


def box_intersection_area(box_a, box_b) -> int:
    left = max(box_a["x"], box_b["x"])
    top = max(box_a["y"], box_b["y"])
    right = min(box_a["x"] + box_a["width"] - 1, box_b["x"] + box_b["width"] - 1)
    bottom = min(box_a["y"] + box_a["height"] - 1, box_b["y"] + box_b["height"] - 1)
    if left > right or top > bottom:
        return 0
    return (right - left + 1) * (bottom - top + 1)


def union_box(boxes):
    min_x = min(box["x"] for box in boxes)
    min_y = min(box["y"] for box in boxes)
    max_x = max(box["x"] + box["width"] - 1 for box in boxes)
    max_y = max(box["y"] + box["height"] - 1 for box in boxes)
    return {
        "x": min_x,
        "y": min_y,
        "width": max_x - min_x + 1,
        "height": max_y - min_y + 1,
    }


def build_sparse_pixel_components(sparse_pixels):
    if not sparse_pixels:
        return []

    pixel_map = {(x, y): [x, y, red, green, blue] for x, y, red, green, blue in sparse_pixels}
    visited = set()
    components = []

    for coordinate, pixel in pixel_map.items():
        if coordinate in visited:
            continue

        queue = deque([coordinate])
        visited.add(coordinate)
        component_pixels = []

        while queue:
            current_x, current_y = queue.popleft()
            current_pixel = pixel_map[(current_x, current_y)]
            component_pixels.append(current_pixel)

            for next_x, next_y in (
                (current_x + 1, current_y),
                (current_x - 1, current_y),
                (current_x, current_y + 1),
                (current_x, current_y - 1),
                (current_x + 1, current_y + 1),
                (current_x + 1, current_y - 1),
                (current_x - 1, current_y + 1),
                (current_x - 1, current_y - 1),
            ):
                next_coordinate = (next_x, next_y)
                if next_coordinate not in pixel_map or next_coordinate in visited:
                    continue
                visited.add(next_coordinate)
                queue.append(next_coordinate)

        components.append(
            {
                "pixels": component_pixels,
                "bounds": union_box(
                    [{"x": item[0], "y": item[1], "width": 1, "height": 1} for item in component_pixels]
                ),
            }
        )

    components.sort(key=lambda item: len(item["pixels"]), reverse=True)
    return components


def point_box_distance(x: int, y: int, box) -> int:
    min_x = box["x"]
    max_x = box["x"] + box["width"] - 1
    min_y = box["y"]
    max_y = box["y"] + box["height"] - 1

    if x < min_x:
        dx = min_x - x
    elif x > max_x:
        dx = x - max_x
    else:
        dx = 0

    if y < min_y:
        dy = min_y - y
    elif y > max_y:
        dy = y - max_y
    else:
        dy = 0

    return dx + dy


def cluster_sparse_pixel_components(components):
    if len(components) <= 1:
        return components

    widths = sorted(component["bounds"]["width"] for component in components)
    heights = sorted(component["bounds"]["height"] for component in components)
    median_width = widths[len(widths) // 2]
    median_height = heights[len(heights) // 2]
    pad_x = max(3, min(median_width // 2, 10))
    pad_y = max(4, min(median_height // 2 + 2, 14))
    parents = list(range(len(components)))

    def find(index):
        while parents[index] != index:
            parents[index] = parents[parents[index]]
            index = parents[index]
        return index

    def union(left, right):
        left_root = find(left)
        right_root = find(right)
        if left_root != right_root:
            parents[right_root] = left_root

    for left_index in range(len(components)):
        for right_index in range(left_index + 1, len(components)):
            if box_intersects(
                components[left_index]["bounds"],
                components[right_index]["bounds"],
                pad_x=pad_x,
                pad_y=pad_y,
            ):
                union(left_index, right_index)

    clusters = {}
    for index, component in enumerate(components):
        root = find(index)
        if root not in clusters:
            clusters[root] = []
        clusters[root].append(component)

    merged_clusters = []
    for members in clusters.values():
        cluster_pixels = []
        cluster_boxes = []
        for member in members:
            cluster_pixels.extend(member["pixels"])
            cluster_boxes.append(member["bounds"])
        merged_clusters.append({"pixels": cluster_pixels, "bounds": union_box(cluster_boxes)})

    merged_clusters.sort(key=lambda item: len(item["pixels"]), reverse=True)
    return merged_clusters


def prune_detached_sparse_pixels(sparse_pixels, origin):
    components = build_sparse_pixel_components(sparse_pixels)
    if len(components) <= 1:
        return sparse_pixels

    clusters = cluster_sparse_pixel_components(components)
    total_pixels = len(sparse_pixels)
    best_cluster = None
    best_score = None

    for cluster in clusters:
        bounds = cluster["bounds"]
        pixel_count = len(cluster["pixels"])
        distance_to_origin = point_box_distance(origin["x"], origin["y"], bounds)
        bottom_y = bounds["y"] + bounds["height"] - 1
        baseline_gap = abs(origin["y"] - bottom_y)
        center_x = bounds["x"] + bounds["width"] // 2
        center_gap = abs(origin["x"] - center_x)
        score = pixel_count * 1000 - distance_to_origin * 120 - baseline_gap * 30 - center_gap * 8
        if distance_to_origin == 0:
            score += 2000
        if best_score is None or score > best_score:
            best_cluster = cluster
            best_score = score

    if best_cluster is None:
        return sparse_pixels

    kept_pixels = len(best_cluster["pixels"])
    removed_pixels = total_pixels - kept_pixels
    if removed_pixels <= 0:
        return sparse_pixels
    if kept_pixels * 100 < total_pixels * 45:
        return sparse_pixels
    if removed_pixels < 12 and len(clusters) <= 2:
        return sparse_pixels

    return sorted(best_cluster["pixels"], key=lambda item: (item[1], item[0]))


def should_skip_auto_frame_box(source_box) -> bool:
    if source_box["width"] > MAX_AUTO_FRAME_DIMENSION:
        return True
    if source_box["height"] > MAX_AUTO_FRAME_DIMENSION:
        return True
    if source_box["width"] * source_box["height"] > MAX_AUTO_FRAME_AREA:
        return True
    return False


def expand_box_within_image(source_box, image: Image.Image, padding: int):
    x = max(source_box["x"] - padding, 0)
    y = max(source_box["y"] - padding, 0)
    max_x = min(source_box["x"] + source_box["width"] - 1 + padding, image.width - 1)
    max_y = min(source_box["y"] + source_box["height"] - 1 + padding, image.height - 1)
    return {
        "x": x,
        "y": y,
        "width": max_x - x + 1,
        "height": max_y - y + 1,
    }


def should_include_character_asset(asset_meta) -> bool:
    category = asset_meta["category"]
    lowered_title = asset_meta["title"].lower()
    if category == "item":
        if not any(keyword in lowered_title for keyword in ITEM_CHARACTER_INCLUDE_KEYWORDS):
            return False
    elif category != "character":
        return False

    for keyword in CHARACTER_TITLE_EXCLUDE_KEYWORDS:
        if keyword in lowered_title:
            return False
    return True


def match_action_override_rule(asset_title: str, role_label: str = ""):
    combined_text = f"{asset_title.lower()} {role_label.lower()}".strip()
    for rule in ACTION_OVERRIDE_RULES:
        if all(keyword in combined_text for keyword in rule["keywords"]):
            return list(rule["actions"])
    return None


def iter_processed_transparent_entries(processed_asset):
    for field_name in ("components", "frames"):
        entries = processed_asset.get(field_name)
        if not isinstance(entries, list):
            continue
        for entry in entries:
            if entry.get("backgroundMode") != "transparent":
                continue
            transparent_colors = entry.get("transparentColors")
            source_box = entry.get("sourceBox")
            if not isinstance(transparent_colors, list) or not transparent_colors:
                continue
            if not isinstance(source_box, dict):
                continue
            yield {
                "sourceBox": source_box,
                "transparentColors": transparent_colors,
            }


def build_stage1_matte_hint_index(processed_assets):
    index = {}
    for processed_asset in processed_assets or []:
        source_asset = processed_asset.get("source")
        if not isinstance(source_asset, str) or not source_asset:
            continue
        entries = list(iter_processed_transparent_entries(processed_asset))
        if not entries:
            continue
        if source_asset not in index:
            index[source_asset] = []
        index[source_asset].extend(entries)
    return index


def build_matte_hint_colors(matte_hint_index, source_asset: str, source_box):
    candidates = matte_hint_index.get(source_asset)
    if not candidates:
        return None

    exact_matches = []
    contained_matches = []
    intersecting_matches = []
    for candidate in candidates:
        candidate_box = candidate["sourceBox"]
        if candidate_box == source_box:
            exact_matches.append(candidate)
            continue
        if box_contains(source_box, candidate_box, pad_x=1, pad_y=1):
            contained_matches.append(candidate)
            continue
        if not box_intersects(source_box, candidate_box, pad_x=1, pad_y=1):
            continue

        overlap_area = box_intersection_area(source_box, candidate_box)
        if overlap_area * 100 < box_area(candidate_box) * 72:
            continue
        intersecting_matches.append(candidate)

    resolved_matches = exact_matches or contained_matches or intersecting_matches
    if not resolved_matches:
        return None

    resolved_colors = []
    seen_colors = set()
    for candidate in resolved_matches:
        for color in candidate["transparentColors"]:
            color_tuple = coerce_color_tuple(color)
            if color_tuple in seen_colors:
                continue
            seen_colors.add(color_tuple)
            resolved_colors.append(color_tuple)
    return resolved_colors or None


def extract_named_title_parts(title: str):
    normalized = title.replace(" and ", " & ")
    if " & " not in normalized:
        return []
    parts = [part.strip() for part in normalized.split(" & ")]
    filtered = [part for part in parts if part]
    return filtered


def build_border_coordinates(min_x: int, min_y: int, max_x: int, max_y: int, band: int):
    coordinates = []
    for offset in range(band):
        start_x = min_x + offset
        start_y = min_y + offset
        end_x = max_x - offset
        end_y = max_y - offset
        if end_x < start_x or end_y < start_y:
            continue

        for x in range(start_x, end_x + 1):
            coordinates.append((x, start_y))
            if end_y != start_y:
                coordinates.append((x, end_y))

        for y in range(start_y + 1, end_y):
            coordinates.append((start_x, y))
            if end_x != start_x:
                coordinates.append((end_x, y))

    return coordinates


def find_remaining_bounds(removed_mask):
    min_x = None
    min_y = None
    max_x = None
    max_y = None

    for y in range(len(removed_mask)):
        for x in range(len(removed_mask[y])):
            if removed_mask[y][x]:
                continue
            if min_x is None:
                min_x = x
                min_y = y
                max_x = x
                max_y = y
                continue
            min_x = min(min_x, x)
            min_y = min(min_y, y)
            max_x = max(max_x, x)
            max_y = max(max_y, y)

    if min_x is None:
        return None
    return (min_x, min_y, max_x, max_y)


def collect_border_color_counter(image: Image.Image, removed_mask, band: int):
    bounds = find_remaining_bounds(removed_mask)
    if bounds is None:
        return Counter()

    counter = Counter()
    for x, y in build_border_coordinates(bounds[0], bounds[1], bounds[2], bounds[3], band):
        if removed_mask[y][x]:
            continue
        counter[image.getpixel((x, y))] += 1
    return counter


def remove_border_connected_pixels(image: Image.Image, removed_mask, candidate_colors) -> int:
    bounds = find_remaining_bounds(removed_mask)
    if bounds is None:
        return 0

    queue = deque()
    removed_count = 0
    for x, y in build_border_coordinates(bounds[0], bounds[1], bounds[2], bounds[3], 1):
        if removed_mask[y][x]:
            continue
        pixel = image.getpixel((x, y))
        if pixel not in candidate_colors:
            continue
        removed_mask[y][x] = True
        queue.append((x, y))
        removed_count += 1

    while queue:
        current_x, current_y = queue.popleft()
        for next_x, next_y in (
            (current_x + 1, current_y),
            (current_x - 1, current_y),
            (current_x, current_y + 1),
            (current_x, current_y - 1),
            (current_x + 1, current_y + 1),
            (current_x + 1, current_y - 1),
            (current_x - 1, current_y + 1),
            (current_x - 1, current_y - 1),
        ):
            if not (0 <= next_x < image.width and 0 <= next_y < image.height):
                continue
            if removed_mask[next_y][next_x]:
                continue
            if image.getpixel((next_x, next_y)) not in candidate_colors:
                continue
            removed_mask[next_y][next_x] = True
            queue.append((next_x, next_y))
            removed_count += 1

    return removed_count


def build_character_sparse_pixels(crop: Image.Image, matte_hint_colors=None, allow_hint_expansion: bool = True):
    removed_mask = [[False for _ in range(crop.width)] for _ in range(crop.height)]
    matte_candidates = set()
    if matte_hint_colors is not None:
        for color in matte_hint_colors:
            matte_candidates.add(coerce_color_tuple(color))

    for y in range(crop.height):
        for x in range(crop.width):
            if crop.getpixel((x, y))[3] == 0:
                removed_mask[y][x] = True

    for _ in range(6):
        border_counter = collect_border_color_counter(crop, removed_mask, 1)
        band_counter = collect_border_color_counter(crop, removed_mask, 2)
        total_border_pixels = sum(border_counter.values())
        threshold = max(min(crop.width, crop.height) // 3, 3)
        hint_threshold = max(2, threshold // 3)
        dynamic_candidates = set()

        for color in matte_candidates:
            if border_counter[color] > 0:
                dynamic_candidates.add(color)

        if matte_candidates and allow_hint_expansion:
            for color, count in border_counter.most_common(8):
                if color == (0, 0, 0, 255) and color not in matte_candidates:
                    continue
                if count < max(2, threshold // 2):
                    continue
                if not is_similar_matte_color(color, matte_candidates):
                    continue
                dynamic_candidates.add(color)

            for color, count in band_counter.most_common(8):
                if color == (0, 0, 0, 255) and color not in matte_candidates:
                    continue
                if count < hint_threshold:
                    continue
                if not is_similar_matte_color(color, matte_candidates):
                    continue
                dynamic_candidates.add(color)

        elif not matte_candidates:
            dominant_items = border_counter.most_common(2)
            if dominant_items:
                dominant_color, dominant_count = dominant_items[0]
                if dominant_count >= threshold and dominant_count * 100 >= max(total_border_pixels * 30, 1):
                    if dominant_color != (0, 0, 0, 255) or not matte_candidates:
                        dynamic_candidates.add(dominant_color)

                    if len(dominant_items) > 1:
                        secondary_color, secondary_count = dominant_items[1]
                        if secondary_count >= threshold and secondary_count * 100 >= dominant_count * 85:
                            if secondary_color != (0, 0, 0, 255) or secondary_color in matte_candidates:
                                dynamic_candidates.add(secondary_color)

            if not dynamic_candidates:
                for color, count in band_counter.most_common(2):
                    if count >= threshold:
                        dynamic_candidates.add(color)

        if not dynamic_candidates:
            break

        removed_count = remove_border_connected_pixels(crop, removed_mask, dynamic_candidates)
        matte_candidates.update(dynamic_candidates)
        if removed_count == 0:
            break

    sparse_pixels = []
    for y in range(crop.height):
        for x in range(crop.width):
            if removed_mask[y][x]:
                continue
            pixel = crop.getpixel((x, y))
            if pixel[3] == 0:
                continue
            sparse_pixels.append([x, y, pixel[0], pixel[1], pixel[2]])
    return sparse_pixels


def build_character_animation_frame(
    image: Image.Image,
    source_asset: str,
    frame_id: str,
    source_box,
    origin,
    matte_hint_colors=None,
    strict_origin: bool = True,
):
    ensure_box_within_image(image, source_box, f"{source_asset}/{frame_id}")

    if not (0 <= origin["x"] < source_box["width"] and 0 <= origin["y"] < source_box["height"]):
        raise RuntimeError(f"{source_asset}/{frame_id} origin 必须落在 sourceBox 范围内")

    crop = crop_by_box(image, source_box)
    sparse_pixels = build_character_sparse_pixels(
        crop,
        matte_hint_colors=matte_hint_colors,
        allow_hint_expansion=True,
    )
    sparse_pixels = prune_detached_sparse_pixels(sparse_pixels, origin)
    trim_box, trimmed_pixels = trim_sparse_pixels(sparse_pixels)

    if (not trimmed_pixels) and matte_hint_colors:
        sparse_pixels = build_character_sparse_pixels(
            crop,
            matte_hint_colors=matte_hint_colors,
            allow_hint_expansion=False,
        )
        sparse_pixels = prune_detached_sparse_pixels(sparse_pixels, origin)
        trim_box, trimmed_pixels = trim_sparse_pixels(sparse_pixels)

    if not trimmed_pixels:
        raise RuntimeError(f"{source_asset}/{frame_id} 裁剪后没有有效像素")

    if matte_hint_colors:
        tightened_crop = crop.crop(
            (
                trim_box["x"],
                trim_box["y"],
                trim_box["x"] + trim_box["width"],
                trim_box["y"] + trim_box["height"],
            )
        )
        tightened_sparse_pixels = build_character_sparse_pixels(
            tightened_crop,
            matte_hint_colors=matte_hint_colors,
            allow_hint_expansion=True,
        )
        tightened_origin = {
            "x": min(max(origin["x"] - trim_box["x"], 0), tightened_crop.width - 1),
            "y": min(max(origin["y"] - trim_box["y"], 0), tightened_crop.height - 1),
        }
        tightened_sparse_pixels = prune_detached_sparse_pixels(tightened_sparse_pixels, tightened_origin)
        tightened_trim_box, tightened_trimmed_pixels = trim_sparse_pixels(tightened_sparse_pixels)
        if tightened_trimmed_pixels:
            combined_trim_box = {
                "x": trim_box["x"] + tightened_trim_box["x"],
                "y": trim_box["y"] + tightened_trim_box["y"],
                "width": tightened_trim_box["width"],
                "height": tightened_trim_box["height"],
            }
            combined_area = combined_trim_box["width"] * combined_trim_box["height"]
            current_area = trim_box["width"] * trim_box["height"]
            if combined_area < current_area or len(tightened_trimmed_pixels) < len(trimmed_pixels):
                trim_box = combined_trim_box
                trimmed_pixels = tightened_trimmed_pixels

    translated_origin = {
        "x": origin["x"] - trim_box["x"],
        "y": origin["y"] - trim_box["y"],
    }

    translated_origin["x"] = min(max(translated_origin["x"], 0), trim_box["width"] - 1)
    translated_origin["y"] = min(max(translated_origin["y"], 0), trim_box["height"] - 1)

    return {
        "sourceAsset": source_asset,
        "sourceBox": source_box,
        "trimBox": trim_box,
        "size": {"width": trim_box["width"], "height": trim_box["height"]},
        "origin": translated_origin,
        "pixelCount": len(trimmed_pixels),
        "pixels": trimmed_pixels,
    }


def segment_occupancy_ranges(occupancy, max_gap: int, min_length: int):
    ranges = []
    start = None
    gap_count = 0

    for index, value in enumerate(occupancy):
        if value > 0:
            if start is None:
                start = index
            gap_count = 0
            continue

        if start is None:
            continue

        gap_count += 1
        if gap_count <= max_gap:
            continue

        end = index - gap_count
        if end - start + 1 >= min_length:
            ranges.append((start, end))
        start = None
        gap_count = 0

    if start is not None:
        end = len(occupancy) - 1 - gap_count
        if end - start + 1 >= min_length:
            ranges.append((start, end))

    return ranges


def build_frame_boxes_from_component_boxes(image: Image.Image, component_boxes):
    filtered_boxes = []
    for box in component_boxes:
        if box["pixelCountOnSheet"] < 4:
            continue
        if box["width"] * box["height"] < 12:
            continue
        filtered_boxes.append(box)

    if not filtered_boxes:
        return []

    row_occupancy = [0 for _ in range(image.height)]
    for box in filtered_boxes:
        for y in range(box["y"], box["y"] + box["height"]):
            row_occupancy[y] += 1

    row_ranges = segment_occupancy_ranges(row_occupancy, max_gap=5, min_length=2)
    frame_boxes = []

    for row_start, row_end in row_ranges:
        row_boxes = []
        for box in filtered_boxes:
            box_top = box["y"]
            box_bottom = box["y"] + box["height"] - 1
            if box_bottom < row_start or box_top > row_end:
                continue
            row_boxes.append(box)

        if not row_boxes:
            continue

        column_occupancy = [0 for _ in range(image.width)]
        for box in row_boxes:
            for x in range(box["x"], box["x"] + box["width"]):
                column_occupancy[x] += 1

        column_ranges = segment_occupancy_ranges(column_occupancy, max_gap=5, min_length=2)
        for column_start, column_end in column_ranges:
            candidate_boxes = []
            for box in row_boxes:
                box_left = box["x"]
                box_right = box["x"] + box["width"] - 1
                if box_right < column_start or box_left > column_end:
                    continue
                candidate_boxes.append(box)

            if not candidate_boxes:
                continue

            source_box = union_box(candidate_boxes)
            source_box = expand_box_within_image(source_box, image, padding=1)
            frame_boxes.append(
                {
                    "sourceBox": source_box,
                    "componentBoxes": candidate_boxes,
                    "pixelCountOnSheet": sum(box["pixelCountOnSheet"] for box in candidate_boxes),
                }
            )

    if len(frame_boxes) == 1 and len(filtered_boxes) > 12:
        widths = sorted(box["width"] for box in filtered_boxes)
        heights = sorted(box["height"] for box in filtered_boxes)
        pad_x = max(min(widths[len(widths) // 2] // 4, 4), 1)
        pad_y = max(min(heights[len(heights) // 2] // 4, 4), 1)
        parents = list(range(len(filtered_boxes)))

        def find(index):
            while parents[index] != index:
                parents[index] = parents[parents[index]]
                index = parents[index]
            return index

        def union(left, right):
            left_root = find(left)
            right_root = find(right)
            if left_root != right_root:
                parents[right_root] = left_root

        for left_index in range(len(filtered_boxes)):
            for right_index in range(left_index + 1, len(filtered_boxes)):
                if box_intersects(filtered_boxes[left_index], filtered_boxes[right_index], pad_x=pad_x, pad_y=pad_y):
                    union(left_index, right_index)

        clustered_boxes = {}
        for index, box in enumerate(filtered_boxes):
            root = find(index)
            if root not in clustered_boxes:
                clustered_boxes[root] = []
            clustered_boxes[root].append(box)

        frame_boxes = []
        for boxes in clustered_boxes.values():
            source_box = expand_box_within_image(union_box(boxes), image, padding=1)
            frame_boxes.append(
                {
                    "sourceBox": source_box,
                    "componentBoxes": boxes,
                    "pixelCountOnSheet": sum(box["pixelCountOnSheet"] for box in boxes),
                }
            )

    image_area = image.width * image.height
    filtered_frame_boxes = []
    for frame_box in frame_boxes:
        source_box = frame_box["sourceBox"]
        source_area = source_box["width"] * source_box["height"]
        if len(frame_boxes) > 1 and source_area * 100 > image_area * 70:
            continue
        filtered_frame_boxes.append(frame_box)

    if filtered_frame_boxes:
        frame_boxes = filtered_frame_boxes

    frame_boxes.sort(key=lambda item: (item["sourceBox"]["y"], item["sourceBox"]["x"]))
    return frame_boxes


def cluster_frame_boxes(frame_boxes):
    if not frame_boxes:
        return []

    widths = sorted(box["sourceBox"]["width"] for box in frame_boxes)
    heights = sorted(box["sourceBox"]["height"] for box in frame_boxes)
    median_width = widths[len(widths) // 2]
    median_height = heights[len(heights) // 2]
    pad_x = max(median_width // 2, 10)
    pad_y = max(median_height // 2, 12)

    parents = list(range(len(frame_boxes)))

    def find(index):
        while parents[index] != index:
            parents[index] = parents[parents[index]]
            index = parents[index]
        return index

    def union(left, right):
        left_root = find(left)
        right_root = find(right)
        if left_root != right_root:
            parents[right_root] = left_root

    for left_index in range(len(frame_boxes)):
        for right_index in range(left_index + 1, len(frame_boxes)):
            if box_intersects(
                frame_boxes[left_index]["sourceBox"],
                frame_boxes[right_index]["sourceBox"],
                pad_x=pad_x,
                pad_y=pad_y,
            ):
                union(left_index, right_index)

    groups = {}
    for index, frame_box in enumerate(frame_boxes):
        root = find(index)
        if root not in groups:
            groups[root] = []
        groups[root].append(frame_box)

    sections = []
    for boxes in groups.values():
        boxes.sort(key=lambda item: (item["sourceBox"]["y"], item["sourceBox"]["x"]))
        sections.append(boxes)

    sections.sort(key=lambda boxes: (boxes[0]["sourceBox"]["y"], boxes[0]["sourceBox"]["x"]))
    return sections


def build_rows_from_frame_boxes(frame_boxes):
    if not frame_boxes:
        return []

    sorted_boxes = sorted(frame_boxes, key=lambda item: (item["sourceBox"]["y"], item["sourceBox"]["x"]))
    heights = sorted(box["sourceBox"]["height"] for box in sorted_boxes)
    median_height = heights[len(heights) // 2]
    row_gap_threshold = max(min(median_height // 2, 12), 4)
    rows = []
    current_row = []
    current_anchor = None

    for frame_box in sorted_boxes:
        box_top = frame_box["sourceBox"]["y"]
        if current_anchor is None or box_top - current_anchor <= row_gap_threshold:
            current_row.append(frame_box)
            if current_anchor is None:
                current_anchor = box_top
            continue

        current_row.sort(key=lambda item: item["sourceBox"]["x"])
        rows.append(current_row)
        current_row = [frame_box]
        current_anchor = box_top

    if current_row:
        current_row.sort(key=lambda item: item["sourceBox"]["x"])
        rows.append(current_row)
    return rows


def split_row_into_segments(row_boxes):
    if not row_boxes:
        return []

    widths = sorted(box["sourceBox"]["width"] for box in row_boxes)
    median_width = widths[len(widths) // 2]
    gap_threshold = max(min(median_width, 18), 8)
    segments = []
    current_segment = [row_boxes[0]]

    for box in row_boxes[1:]:
        previous_box = current_segment[-1]["sourceBox"]
        current_box = box["sourceBox"]
        gap = current_box["x"] - (previous_box["x"] + previous_box["width"])
        if gap <= gap_threshold:
            current_segment.append(box)
            continue
        segments.append(current_segment)
        current_segment = [box]

    if current_segment:
        segments.append(current_segment)
    return segments


def should_split_asset_into_sections(title: str, section_count: int) -> bool:
    if section_count <= 1:
        return False

    lowered_title = title.lower()
    for keyword in GENERIC_MULTI_ROLE_KEYWORDS:
        if keyword in lowered_title:
            return True
    if len(extract_named_title_parts(title)) == section_count:
        return True
    return False


def build_role_groups_for_asset(asset_title: str, frame_boxes):
    rows = build_rows_from_frame_boxes(frame_boxes)
    if not rows:
        return []

    named_parts = extract_named_title_parts(asset_title)
    if not should_split_asset_into_sections(asset_title, len(rows)) and (
        len(frame_boxes) <= 24 or not named_parts
    ):
        return [{"label": asset_title, "rows": rows}]

    role_groups = []

    for row_index, row_boxes in enumerate(rows):
        for segment_boxes in split_row_into_segments(row_boxes):
            segment_bounds = union_box([box["sourceBox"] for box in segment_boxes])
            segment_center = segment_bounds["x"] + segment_bounds["width"] / 2
            matched_index = None
            matched_score = None

            for group_index, role_group in enumerate(role_groups):
                row_gap = row_index - role_group["lastRowIndex"]
                if row_gap < 0 or row_gap > 2:
                    continue

                group_bounds = role_group["bounds"]
                group_center = group_bounds["x"] + group_bounds["width"] / 2
                overlap_left = max(group_bounds["x"], segment_bounds["x"])
                overlap_right = min(
                    group_bounds["x"] + group_bounds["width"] - 1,
                    segment_bounds["x"] + segment_bounds["width"] - 1,
                )
                overlap_width = overlap_right - overlap_left + 1
                center_distance = abs(segment_center - group_center)
                if overlap_width < -12 and center_distance > max(group_bounds["width"], segment_bounds["width"]) + 28:
                    continue

                score = center_distance + row_gap * 18
                if matched_score is None or score < matched_score:
                    matched_index = group_index
                    matched_score = score

            if matched_index is None:
                role_groups.append(
                    {
                        "rows": [segment_boxes],
                        "bounds": segment_bounds,
                        "lastRowIndex": row_index,
                    }
                )
                continue

            role_group = role_groups[matched_index]
            if role_group["lastRowIndex"] == row_index:
                role_group["rows"][-1].extend(segment_boxes)
                role_group["rows"][-1].sort(key=lambda item: item["sourceBox"]["x"])
            else:
                role_group["rows"].append(segment_boxes)
                role_group["lastRowIndex"] = row_index
            role_group["bounds"] = union_box([role_group["bounds"], segment_bounds])

    role_groups.sort(key=lambda item: (item["bounds"]["y"], item["bounds"]["x"]))
    normalized_role_groups = []
    for index, role_group in enumerate(role_groups, start=1):
        if len(named_parts) == len(role_groups):
            label = named_parts[index - 1]
        elif len(role_groups) == 1:
            label = asset_title
        else:
            label = f"{asset_title} {index:02d}"
        normalized_role_groups.append({"label": label, "rows": role_group["rows"]})

    return normalized_role_groups


def infer_auto_action_ids(rows, asset_title: str, role_label: str = ""):
    lowered_title = asset_title.lower()
    lowered_role = role_label.lower()
    combined_text = f"{lowered_title} {lowered_role}".strip()
    uses_projectiles = any(keyword in combined_text for keyword in ("gun", "laser", "blaster", "cannon", "turret"))
    is_static_role = any(keyword in combined_text for keyword in STATIC_ROLE_KEYWORDS)
    action_override = match_action_override_rule(asset_title, role_label)
    action_ids = []
    has_idle = False
    has_walk = False
    has_jump = False
    has_attack = False
    special_index = 1

    for row_index, row_boxes in enumerate(rows):
        frame_count = len(row_boxes)
        if action_override and row_index < len(action_override):
            action_id = action_override[row_index]
            action_ids.append(action_id)
            has_idle = has_idle or action_id == "idle"
            has_walk = has_walk or action_id in ("walk", "run")
            has_jump = has_jump or action_id == "jump"
            has_attack = has_attack or action_id in ("attack", "shoot")
            continue

        if is_static_role:
            if row_index == 0:
                action_ids.append("idle")
                has_idle = True
                continue
            if not has_attack:
                action_ids.append("shoot" if uses_projectiles else "attack")
                has_attack = True
                continue
            if not any(item == "hurt" for item in action_ids):
                action_ids.append("hurt")
                continue
            if not any(item == "open" for item in action_ids):
                action_ids.append("open")
                continue
            if not any(item == "close" for item in action_ids):
                action_ids.append("close")
                continue
            action_ids.append(f"special-{special_index:02d}")
            special_index += 1
            continue

        if row_index == 0:
            if frame_count >= 3:
                action_ids.append("walk")
                has_walk = True
            else:
                action_ids.append("idle")
                has_idle = True
            continue

        if not has_idle and frame_count == 1:
            action_ids.append("idle")
            has_idle = True
            continue

        if not has_walk and frame_count >= 2:
            action_ids.append("walk")
            has_walk = True
            continue

        if not has_jump:
            action_ids.append("jump")
            has_jump = True
            continue

        if not has_attack:
            if uses_projectiles:
                action_ids.append("shoot")
            else:
                action_ids.append("attack")
            has_attack = True
            continue

        if not any(item == "hurt" for item in action_ids):
            action_ids.append("hurt")
            continue

        action_ids.append(f"special-{special_index:02d}")
        special_index += 1

    if not any(action_id in BASE_ACTION_IDS for action_id in action_ids):
        action_ids[0] = "idle"

    return action_ids


def build_action_label(action_id: str) -> str:
    if action_id in ACTION_LABELS:
        return ACTION_LABELS[action_id]
    return action_id.replace("-", " ").title()


def infer_action_duration_ms(action_id: str) -> int:
    if action_id in ("walk", "run", "roll", "climb", "swim"):
        return 110
    if action_id == "idle":
        return 180
    return 140


def build_character_animations_from_spec(game_dir: Path, matte_hint_index=None):
    spec_path = SPECS_DIR / f"{game_dir.name}.json"
    if not spec_path.exists():
        return {
            "gameId": game_dir.name,
            "label": "",
            "roles": [],
            "frames": {},
            "coveredSourceAssets": set(),
        }

    spec = json.loads(spec_path.read_text(encoding="utf-8"))
    game_id = read_required_string(spec, ["gameId"])
    label = read_required_string(spec, ["label"])

    if game_id != game_dir.name:
        raise RuntimeError(f"{spec_path.name} 的 gameId 与目录名不一致: {game_id} != {game_dir.name}")

    roles_spec = spec.get("roles")
    if not isinstance(roles_spec, list) or not roles_spec:
        raise RuntimeError(f"{spec_path.name} 必须包含非空 roles 数组")

    image_cache = {}
    frame_specs = {}
    covered_source_assets = set()
    covered_role_ids_by_source_asset = {}
    frames = {}
    role_ids = set()
    roles = []

    for role_index, role_spec in enumerate(roles_spec):
        role_id = read_required_string(role_spec, ["id"])
        role_label = read_required_string(role_spec, ["label"])
        validate_unique_id(role_ids, role_id, spec_path.name)

        forms_spec = role_spec.get("forms")
        if not isinstance(forms_spec, list) or not forms_spec:
            raise RuntimeError(f"{spec_path.name} role={role_id} 必须包含非空 forms 数组")

        form_ids = set()
        forms = []
        for form_spec in forms_spec:
            form_id = read_required_string(form_spec, ["id"])
            form_label = read_required_string(form_spec, ["label"])
            validate_unique_id(form_ids, form_id, f"{spec_path.name} role={role_id}")

            actions_spec = form_spec.get("actions")
            if not isinstance(actions_spec, list) or not actions_spec:
                raise RuntimeError(f"{spec_path.name} role={role_id} form={form_id} 必须包含非空 actions 数组")

            action_ids = set()
            actions = []
            for action_spec in actions_spec:
                action_id = read_required_string(action_spec, ["id"])
                action_label = read_required_string(action_spec, ["label"])
                action_loop = read_required_bool(action_spec, ["loop"])
                validate_unique_id(action_ids, action_id, f"{spec_path.name} role={role_id} form={form_id}")

                frames_spec = action_spec.get("frames")
                if not isinstance(frames_spec, list) or not frames_spec:
                    raise RuntimeError(f"{spec_path.name} role={role_id} form={form_id} action={action_id} 至少需要 1 帧")

                action_frames = []
                for frame_index, frame_spec in enumerate(frames_spec):
                    frame_id = read_required_string(frame_spec, ["frameId"])
                    source_asset = read_required_string(frame_spec, ["sourceAsset"])
                    source_box = normalize_source_box(frame_spec, ["sourceBox"])
                    origin = normalize_origin(frame_spec, ["origin"])
                    duration_ms = frame_spec.get("durationMs")

                    if not isinstance(duration_ms, int) or duration_ms <= 0:
                        raise RuntimeError(
                            f"{spec_path.name} role={role_id} form={form_id} action={action_id} "
                            f"frame={frame_index} 的 durationMs 必须是正整数"
                        )

                    image_path = game_dir / source_asset
                    if not image_path.exists():
                        raise RuntimeError(f"{spec_path.name} 引用的 sourceAsset 不存在: {source_asset}")

                    covered_source_assets.add(source_asset)
                    if source_asset not in covered_role_ids_by_source_asset:
                        covered_role_ids_by_source_asset[source_asset] = set()
                    covered_role_ids_by_source_asset[source_asset].add(role_id)
                    if source_asset not in image_cache:
                        image_cache[source_asset] = Image.open(image_path).convert("RGBA")

                    raw_frame_signature = {
                        "sourceAsset": source_asset,
                        "sourceBox": source_box,
                        "origin": origin,
                    }

                    if frame_id in frame_specs:
                        if frame_specs[frame_id] != raw_frame_signature:
                            raise RuntimeError(f"{spec_path.name} 中 frameId 重复且定义不一致: {frame_id}")
                    else:
                        frame_specs[frame_id] = raw_frame_signature
                        frames[frame_id] = build_character_animation_frame(
                            image_cache[source_asset],
                            source_asset,
                            frame_id,
                            source_box,
                            origin,
                            matte_hint_colors=build_matte_hint_colors(
                                matte_hint_index or {},
                                source_asset,
                                source_box,
                            ),
                        )

                    action_frames.append({"ref": frame_id, "durationMs": duration_ms})

                actions.append({"id": action_id, "label": action_label, "loop": action_loop, "frames": action_frames})

            forms.append({"id": form_id, "label": form_label, "actions": actions})

        roles.append({"id": role_id, "label": role_label, "forms": forms})

    return {
        "gameId": game_id,
        "label": label,
        "roles": roles,
        "frames": frames,
        "coveredSourceAssets": covered_source_assets,
        "coveredRoleIdsBySourceAsset": covered_role_ids_by_source_asset,
    }


def build_auto_roles_for_asset(
    game_dir: Path,
    asset_meta,
    existing_role_ids,
    existing_frame_ids,
    matte_hint_index=None,
    reserved_role_ids=None,
):
    image_path = game_dir / asset_meta["filename"]
    image = Image.open(image_path).convert("RGBA")
    component_boxes = find_connected_component_boxes(image, min_pixels=4)
    export_boxes, _ = filter_footer_boxes(component_boxes, image.width)
    lowered_title = asset_meta["title"].lower()
    if len(export_boxes) > 48 or any(keyword in lowered_title for keyword in GENERIC_MULTI_ROLE_KEYWORDS):
        frame_boxes = []
        for box in export_boxes:
            frame_boxes.append(
                {
                    "sourceBox": expand_box_within_image(
                        {"x": box["x"], "y": box["y"], "width": box["width"], "height": box["height"]},
                        image,
                        padding=1,
                    ),
                    "componentBoxes": [box],
                    "pixelCountOnSheet": box["pixelCountOnSheet"],
                }
            )
    else:
        frame_boxes = build_frame_boxes_from_component_boxes(image, export_boxes)
    role_groups = build_role_groups_for_asset(asset_meta["title"], frame_boxes)

    auto_roles = []
    auto_frames = {}

    for role_group in role_groups:
        rows = role_group["rows"]
        if not rows:
            continue

        base_role_id = slugify(role_group["label"])
        if reserved_role_ids and base_role_id in reserved_role_ids:
            continue

        role_id = make_unique_id(existing_role_ids, base_role_id)
        existing_role_ids.add(role_id)

        form_id = "default"
        actions = []
        action_ids = infer_auto_action_ids(rows, asset_meta["title"], role_group["label"])
        for action_index, row_boxes in enumerate(rows):
            action_id = action_ids[action_index]
            action_frames = []
            for frame_index, row_box in enumerate(row_boxes, start=1):
                frame_id = make_unique_id(existing_frame_ids, f"{role_id}-{form_id}-{action_id}-{frame_index:02d}")
                source_box = row_box["sourceBox"]
                if should_skip_auto_frame_box(source_box):
                    print(
                        "警告: 自动跳过超大帧",
                        asset_meta["filename"],
                        frame_id,
                        source_box,
                    )
                    continue
                component_union = union_box(row_box["componentBoxes"])
                origin = {
                    "x": component_union["x"] + component_union["width"] // 2 - source_box["x"],
                    "y": component_union["y"] + component_union["height"] - 1 - source_box["y"],
                }
                try:
                    auto_frames[frame_id] = build_character_animation_frame(
                        image,
                        asset_meta["filename"],
                        frame_id,
                        source_box,
                        origin,
                        matte_hint_colors=build_matte_hint_colors(
                            matte_hint_index or {},
                            asset_meta["filename"],
                            source_box,
                        ),
                        strict_origin=False,
                    )
                except RuntimeError as error:
                    print(f"警告: 自动跳过无效帧 {asset_meta['filename']} {frame_id}: {error}")
                    continue

                existing_frame_ids.add(frame_id)
                action_frames.append({"ref": frame_id, "durationMs": infer_action_duration_ms(action_id)})

            if not action_frames:
                continue

            actions.append(
                {
                    "id": action_id,
                    "label": build_action_label(action_id),
                    "loop": action_id in BASE_ACTION_IDS,
                    "frames": action_frames,
                }
            )

        if not actions:
            continue

        auto_roles.append(
            {
                "id": role_id,
                "label": role_group["label"],
                "forms": [{"id": form_id, "label": "Default", "actions": actions}],
            }
        )

    return auto_roles, auto_frames


def build_target_space_pixel_map(frame):
    pixel_map = {}
    for x, y, red, green, blue in frame["pixels"]:
        pixel_map[(x, y)] = (red, green, blue)
    return pixel_map


def build_aligned_base_pixel_map(base_frame, target_frame):
    aligned_map = {}
    for x, y, red, green, blue in base_frame["pixels"]:
        world_x = x - base_frame["origin"]["x"]
        world_y = y - base_frame["origin"]["y"]
        target_x = world_x + target_frame["origin"]["x"]
        target_y = world_y + target_frame["origin"]["y"]
        if not (0 <= target_x < target_frame["size"]["width"]):
            continue
        if not (0 <= target_y < target_frame["size"]["height"]):
            continue
        aligned_map[(target_x, target_y)] = (red, green, blue)
    return aligned_map


def build_patch_frame(base_ref: str, base_frame, target_frame):
    target_map = build_target_space_pixel_map(target_frame)
    base_map = build_aligned_base_pixel_map(base_frame, target_frame)
    coordinates = set(target_map.keys())
    coordinates.update(base_map.keys())

    diff_coordinates = []
    for coordinate in coordinates:
        if target_map.get(coordinate) != base_map.get(coordinate):
            diff_coordinates.append(coordinate)

    if not diff_coordinates:
        patch_box = {"x": 0, "y": 0, "width": 0, "height": 0}
        patch_pixels = []
    else:
        min_x = min(coordinate[0] for coordinate in diff_coordinates)
        max_x = max(coordinate[0] for coordinate in diff_coordinates)
        min_y = min(coordinate[1] for coordinate in diff_coordinates)
        max_y = max(coordinate[1] for coordinate in diff_coordinates)
        patch_box = {
            "x": min_x,
            "y": min_y,
            "width": max_x - min_x + 1,
            "height": max_y - min_y + 1,
        }
        patch_pixels = []
        for (x, y), color in target_map.items():
            if not (min_x <= x <= max_x and min_y <= y <= max_y):
                continue
            patch_pixels.append([x - min_x, y - min_y, color[0], color[1], color[2]])

    return {
        "sourceAsset": target_frame["sourceAsset"],
        "sourceBox": target_frame["sourceBox"],
        "trimBox": target_frame["trimBox"],
        "size": target_frame["size"],
        "origin": target_frame["origin"],
        "pixelCount": target_frame["pixelCount"],
        "baseRef": base_ref,
        "patchBox": patch_box,
        "patchPixels": patch_pixels,
    }


def resolve_frame_pixels(frame_id: str, frames, resolved_cache=None, pending_ids=None):
    if resolved_cache is None:
        resolved_cache = {}
    if pending_ids is None:
        pending_ids = set()

    if frame_id in resolved_cache:
        return resolved_cache[frame_id]
    if frame_id in pending_ids:
        raise RuntimeError(f"发现 baseRef 循环引用: {frame_id}")

    frame = frames[frame_id]
    if "pixels" in frame:
        resolved_pixels = list(frame["pixels"])
        resolved_cache[frame_id] = resolved_pixels
        return resolved_pixels

    pending_ids.add(frame_id)
    base_pixels = resolve_frame_pixels(frame["baseRef"], frames, resolved_cache=resolved_cache, pending_ids=pending_ids)
    base_frame = dict(frames[frame["baseRef"]])
    base_frame["pixels"] = base_pixels
    aligned_base_map = build_aligned_base_pixel_map(base_frame, frame)
    patch_box = frame["patchBox"]
    pixel_map = {}

    for (x, y), color in aligned_base_map.items():
        inside_patch = (
            patch_box["x"] <= x < patch_box["x"] + patch_box["width"]
            and patch_box["y"] <= y < patch_box["y"] + patch_box["height"]
        )
        if inside_patch:
            continue
        pixel_map[(x, y)] = [x, y, color[0], color[1], color[2]]

    for x, y, red, green, blue in frame["patchPixels"]:
        target_x = patch_box["x"] + x
        target_y = patch_box["y"] + y
        pixel_map[(target_x, target_y)] = [target_x, target_y, red, green, blue]

    resolved_pixels = sorted(pixel_map.values(), key=lambda item: (item[1], item[0]))
    pending_ids.remove(frame_id)
    resolved_cache[frame_id] = resolved_pixels
    return resolved_pixels


def should_use_patch_frame(target_frame, patch_frame) -> bool:
    if patch_frame["patchBox"]["width"] == 0 or patch_frame["patchBox"]["height"] == 0:
        return True

    full_pixels = target_frame["pixelCount"]
    patch_pixels = len(patch_frame["patchPixels"])
    if patch_pixels >= full_pixels:
        return False

    full_area = target_frame["size"]["width"] * target_frame["size"]["height"]
    patch_area = patch_frame["patchBox"]["width"] * patch_frame["patchBox"]["height"]
    if patch_area >= full_area:
        return False

    if patch_pixels * 100 > full_pixels * 72:
        return False
    if patch_area * 100 > full_area * 82:
        return False
    return True


def encode_diff_frames(roles, frames):
    for role in roles:
        for form in role["forms"]:
            ordered_refs = []
            action_ref_map = {}
            for action in form["actions"]:
                for frame_ref in action["frames"]:
                    if frame_ref["ref"] in action_ref_map:
                        continue
                    ordered_refs.append(frame_ref["ref"])
                    action_ref_map[frame_ref["ref"]] = action["id"]

            base_candidates = []
            for frame_id in ordered_refs:
                action_id = action_ref_map[frame_id]
                if action_id in PREFERRED_BASE_ACTION_IDS:
                    base_candidates.append(frame_id)

            full_frame_ids = []
            for frame_id in ordered_refs:
                action_id = action_ref_map[frame_id]
                current_frame = frames[frame_id]

                if action_id in PREFERRED_BASE_ACTION_IDS:
                    full_frame_ids.append(frame_id)
                    continue

                candidate_full_ids = list(full_frame_ids)
                for base_candidate in base_candidates:
                    if base_candidate not in candidate_full_ids:
                        candidate_full_ids.append(base_candidate)

                best_patch = None
                best_base_ref = None
                best_patch_score = None
                for base_ref in candidate_full_ids:
                    if base_ref == frame_id:
                        continue
                    base_frame = frames[base_ref]
                    if "pixels" not in base_frame:
                        continue
                    patch_frame = build_patch_frame(base_ref, base_frame, current_frame)
                    patch_score = len(patch_frame["patchPixels"])
                    if best_patch_score is None or patch_score < best_patch_score:
                        best_patch = patch_frame
                        best_base_ref = base_ref
                        best_patch_score = patch_score

                if best_patch is not None and best_base_ref is not None and should_use_patch_frame(current_frame, best_patch):
                    frames[frame_id] = best_patch
                    continue

                full_frame_ids.append(frame_id)


def validate_character_animations_payload(character_animations):
    frames = character_animations["frames"]
    roles = character_animations["roles"]
    resolved_pixel_cache = {}

    role_ids = set()
    for role in roles:
        validate_unique_id(role_ids, role["id"], "characterAnimations")
        form_ids = set()
        for form in role["forms"]:
            validate_unique_id(form_ids, form["id"], f"role={role['id']}")
            action_ids = set()
            for action in form["actions"]:
                validate_unique_id(action_ids, action["id"], f"role={role['id']} form={form['id']}")
                if not action["frames"]:
                    raise RuntimeError(f"role={role['id']} form={form['id']} action={action['id']} 至少需要 1 帧")
                for frame_ref in action["frames"]:
                    if frame_ref["ref"] not in frames:
                        raise RuntimeError(f"动作引用了不存在的 frameId: {frame_ref['ref']}")

    for frame_id, frame in frames.items():
        size = frame["size"]
        origin = frame["origin"]
        if frame["pixelCount"] <= 0:
            raise RuntimeError(f"{frame_id} pixelCount 必须大于 0")
        if not (0 <= origin["x"] < size["width"]):
            raise RuntimeError(f"{frame_id} origin.x 超出范围")
        if not (0 <= origin["y"] < size["height"]):
            raise RuntimeError(f"{frame_id} origin.y 超出范围")
        if "baseRef" in frame:
            if frame["baseRef"] not in frames:
                raise RuntimeError(f"{frame_id} baseRef 不存在: {frame['baseRef']}")
            if "patchBox" not in frame or "patchPixels" not in frame:
                raise RuntimeError(f"{frame_id} 缺少差异帧字段")
        elif "pixels" not in frame:
            raise RuntimeError(f"{frame_id} 缺少 pixels")

        resolved_pixels = resolve_frame_pixels(frame_id, frames, resolved_cache=resolved_pixel_cache)
        if len(resolved_pixels) != frame["pixelCount"]:
            raise RuntimeError(f"{frame_id} 差异帧展开后的像素数量与 pixelCount 不一致")
        for x, y, *_ in resolved_pixels:
            if not (0 <= x < size["width"]):
                raise RuntimeError(f"{frame_id} 展开后像素 x 超出范围: {x}")
            if not (0 <= y < size["height"]):
                raise RuntimeError(f"{frame_id} 展开后像素 y 超出范围: {y}")

    visiting = set()
    visited = set()

    def walk(frame_id):
        if frame_id in visited:
            return
        if frame_id in visiting:
            raise RuntimeError(f"发现 baseRef 循环引用: {frame_id}")
        visiting.add(frame_id)
        frame = frames[frame_id]
        if "baseRef" in frame:
            walk(frame["baseRef"])
        visiting.remove(frame_id)
        visited.add(frame_id)

    for frame_id in frames:
        walk(frame_id)


def build_character_animations(game_dir: Path, processed_assets=None):
    game_data, assets = load_game_assets(game_dir)
    matte_hint_index = build_stage1_matte_hint_index(processed_assets or [])
    spec_bundle = build_character_animations_from_spec(game_dir, matte_hint_index=matte_hint_index)
    roles = list(spec_bundle["roles"])
    frames = dict(spec_bundle["frames"])
    covered_source_assets = set(spec_bundle["coveredSourceAssets"])
    covered_role_ids_by_source_asset = {
        key: set(value) for key, value in spec_bundle.get("coveredRoleIdsBySourceAsset", {}).items()
    }
    role_ids = {role["id"] for role in roles}
    frame_ids = set(frames.keys())

    for asset_meta in assets:
        if not should_include_character_asset(asset_meta):
            continue

        auto_roles, auto_frames = build_auto_roles_for_asset(
            game_dir,
            asset_meta,
            role_ids,
            frame_ids,
            matte_hint_index=matte_hint_index,
            reserved_role_ids=covered_role_ids_by_source_asset.get(asset_meta["filename"]),
        )
        roles.extend(auto_roles)
        frames.update(auto_frames)

    if not roles:
        return None

    encode_diff_frames(roles, frames)
    character_animations = {
        "gameId": game_data["gameId"],
        "label": game_data["label"],
        "roles": roles,
        "frames": frames,
    }
    validate_character_animations_payload(character_animations)
    return character_animations


def attach_character_animations_to_manifest(game_dir: Path, manifest, processed_assets=None):
    character_animations = build_character_animations(game_dir, processed_assets=processed_assets)
    if character_animations is None:
        manifest.pop("characterAnimationsPath", None)
        return None

    manifest["characterAnimationsPath"] = f"processed/{CHARACTER_ANIMATIONS_FILENAME}"
    return character_animations


def write_game_outputs(game_dir: Path, processed_assets, manifest, character_animations=None):
    output_dir = game_dir / "processed"
    output_dir.mkdir(parents=True, exist_ok=True)

    for asset in processed_assets:
        write_json(output_dir / f"{asset['assetId']}.json", asset)
    if character_animations is not None:
        write_json(output_dir / CHARACTER_ANIMATIONS_FILENAME, character_animations)
    write_json(output_dir / "manifest.json", manifest)


def process_adventure_island():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    master_higgins = build_master_higgins_asset()
    enemies_and_bosses = build_enemies_and_bosses_asset()
    tina = build_tina_asset()
    items = build_items_asset()
    beach_metatiles = build_beach_metatiles_asset()
    beach_chunks = build_chunk_asset(
        BEACH_CHUNKS_SOURCE,
        "beach_mountains_chunks",
        "Beach and Mountains Chunks",
    )
    cave_chunks = build_chunk_asset(
        CAVE_CHUNKS_SOURCE,
        "cave_chunks",
        "Cave Chunks",
    )
    forest_chunks = build_chunk_asset(
        FOREST_CHUNKS_SOURCE,
        "forest_and_boss_rooms_chunks",
        "Forest and Boss Rooms Chunks",
    )
    title_screen = build_title_screen_asset()
    processed_assets = [
        master_higgins,
        enemies_and_bosses,
        tina,
        items,
    ]
    manifest = build_manifest()
    character_animations = attach_character_animations_to_manifest(
        ASSET_DIR,
        manifest,
        processed_assets=processed_assets,
    )

    write_json(OUTPUT_DIR / "master_higgins.json", master_higgins)
    write_json(OUTPUT_DIR / "enemies_and_bosses.json", enemies_and_bosses)
    write_json(OUTPUT_DIR / "tina.json", tina)
    write_json(OUTPUT_DIR / "items_and_objects.json", items)
    write_json(OUTPUT_DIR / "beach_mountains_metatiles.json", beach_metatiles)
    write_json(OUTPUT_DIR / "beach_mountains_chunks.json", beach_chunks)
    write_json(OUTPUT_DIR / "cave_chunks.json", cave_chunks)
    write_json(OUTPUT_DIR / "forest_and_boss_rooms_chunks.json", forest_chunks)
    write_json(OUTPUT_DIR / "title_screen.json", title_screen)
    if character_animations is not None:
        write_json(OUTPUT_DIR / CHARACTER_ANIMATIONS_FILENAME, character_animations)
    write_json(OUTPUT_DIR / "manifest.json", manifest)

    print(f"已生成: {OUTPUT_DIR / 'master_higgins.json'}")
    print(f"已生成: {OUTPUT_DIR / 'enemies_and_bosses.json'}")
    print(f"已生成: {OUTPUT_DIR / 'tina.json'}")
    print(f"已生成: {OUTPUT_DIR / 'items_and_objects.json'}")
    print(f"已生成: {OUTPUT_DIR / 'beach_mountains_metatiles.json'}")
    print(f"已生成: {OUTPUT_DIR / 'beach_mountains_chunks.json'}")
    print(f"已生成: {OUTPUT_DIR / 'cave_chunks.json'}")
    print(f"已生成: {OUTPUT_DIR / 'forest_and_boss_rooms_chunks.json'}")
    print(f"已生成: {OUTPUT_DIR / 'title_screen.json'}")
    if character_animations is not None:
        print(f"已生成: {OUTPUT_DIR / CHARACTER_ANIMATIONS_FILENAME}")
    print(f"已生成: {OUTPUT_DIR / 'manifest.json'}")


def process_generic_game(game_dir: Path):
    game_data, assets = load_game_assets(game_dir)
    processed_assets = []

    for asset_meta in assets:
        processed_assets.append(
            build_generic_asset(game_data["gameId"], game_data["label"], game_dir, asset_meta)
        )

    manifest = build_generic_manifest(game_data, processed_assets)
    character_animations = attach_character_animations_to_manifest(
        game_dir,
        manifest,
        processed_assets=processed_assets,
    )
    write_game_outputs(game_dir, processed_assets, manifest, character_animations=character_animations)
    print(f"已生成: {game_dir / 'processed' / 'manifest.json'}")
    if character_animations is not None:
        print(f"已生成: {game_dir / 'processed' / CHARACTER_ANIMATIONS_FILENAME}")
    print(f"已处理游戏: {game_data['label']} ({len(processed_assets)} 份素材)")


def iter_target_game_dirs(target_game_ids):
    root = ROOT_DIR / "material-library" / "nes-game-assets"
    game_dirs = sorted([path for path in root.iterdir() if path.is_dir()])
    if not target_game_ids:
        return game_dirs

    target_set = set(target_game_ids)
    filtered_dirs = [game_dir for game_dir in game_dirs if game_dir.name in target_set]
    missing = sorted(target_set - {game_dir.name for game_dir in filtered_dirs})
    if missing:
        raise RuntimeError(f"未找到游戏目录: {', '.join(missing)}")
    return filtered_dirs


def main():
    target_game_ids = sys.argv[1:]

    for game_dir in iter_target_game_dirs(target_game_ids):
        if game_dir.name == "adventure-island":
            process_adventure_island()
            continue
        process_generic_game(game_dir)


if __name__ == "__main__":
    main()
