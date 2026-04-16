from math import ceil
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFont, ImageOps


ROOT = Path("/Users/aflylong/Desktop/project/Glowxel/uniapp")
SOURCE_DIR = ROOT / "output" / "github-previews"
OUTPUT_DIR = ROOT / "output" / "github-finalists"


def ensure_output_dir():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def load_image(name: str) -> Image.Image:
    return Image.open(SOURCE_DIR / name).convert("RGBA")


def crop_box(image: Image.Image, box) -> Image.Image:
    return image.crop(box)


def resize_to_board(image: Image.Image) -> Image.Image:
    return ImageOps.fit(image, (64, 64), method=Image.Resampling.BOX, centering=(0.5, 0.5))


def enhance_image(image: Image.Image, contrast=1.0, color=1.0, brightness=1.0, sharpness=1.0) -> Image.Image:
    result = image.convert("RGBA")
    if contrast != 1.0:
        result = ImageEnhance.Contrast(result).enhance(contrast)
    if color != 1.0:
        result = ImageEnhance.Color(result).enhance(color)
    if brightness != 1.0:
        result = ImageEnhance.Brightness(result).enhance(brightness)
    if sharpness != 1.0:
        result = ImageEnhance.Sharpness(result).enhance(sharpness)
    return result


def quantize_adaptive(image: Image.Image, color_count: int) -> Image.Image:
    return image.convert("RGB").quantize(colors=color_count, method=Image.Quantize.MEDIANCUT).convert("RGBA")


def add_city_neon(image: Image.Image) -> Image.Image:
    result = image.copy()
    pixels = result.load()
    neon_points = [
        (29, 6, (255, 102, 186, 255)),
        (30, 6, (255, 102, 186, 255)),
        (31, 6, (93, 235, 255, 255)),
        (32, 6, (93, 235, 255, 255)),
        (29, 7, (255, 190, 98, 255)),
        (30, 7, (255, 190, 98, 255)),
        (31, 7, (255, 102, 186, 255)),
        (32, 7, (255, 102, 186, 255)),
        (18, 36, (120, 82, 255, 255)),
        (18, 37, (120, 82, 255, 255)),
        (19, 36, (93, 235, 255, 255)),
        (19, 37, (93, 235, 255, 255)),
    ]
    for x, y, rgba in neon_points:
        pixels[x, y] = rgba
    return result


def add_city_window_sparks(image: Image.Image) -> Image.Image:
    result = image.copy()
    pixels = result.load()
    spark_points = [
        (7, 44, (108, 225, 255, 255)),
        (8, 44, (108, 225, 255, 255)),
        (25, 22, (255, 122, 198, 255)),
        (26, 22, (255, 122, 198, 255)),
        (41, 12, (255, 198, 86, 255)),
        (42, 12, (255, 198, 86, 255)),
        (43, 12, (255, 198, 86, 255)),
        (53, 28, (110, 255, 216, 255)),
    ]
    for x, y, rgba in spark_points:
        pixels[x, y] = rgba
    return result


def add_sunset_glow(image: Image.Image) -> Image.Image:
    result = image.copy()
    pixels = result.load()
    glow_points = [
        (28, 31, (255, 212, 84, 255)),
        (29, 31, (255, 212, 84, 255)),
        (30, 31, (255, 212, 84, 255)),
        (31, 31, (255, 192, 72, 255)),
        (32, 31, (255, 192, 72, 255)),
        (27, 32, (255, 182, 58, 255)),
        (28, 32, (255, 182, 58, 255)),
        (29, 32, (255, 182, 58, 255)),
        (30, 32, (255, 182, 58, 255)),
        (31, 32, (255, 182, 58, 255)),
    ]
    for x, y, rgba in glow_points:
        pixels[x, y] = rgba
    return result


def add_stars(image: Image.Image) -> Image.Image:
    result = image.copy()
    pixels = result.load()
    star_points = [
        (6, 8),
        (10, 10),
        (16, 6),
        (45, 7),
        (54, 11),
        (58, 14),
    ]
    for x, y in star_points:
        pixels[x, y] = (215, 245, 255, 255)
    return result


def create_preview_card(image: Image.Image) -> Image.Image:
    return image.resize((384, 384), Image.Resampling.NEAREST)


def save_variant(stem: str, image: Image.Image):
    board_path = OUTPUT_DIR / f"{stem}.png"
    preview_path = OUTPUT_DIR / f"{stem}_preview.png"
    image.save(board_path)
    create_preview_card(image).save(preview_path)
    return board_path, preview_path


def build_variants():
    sunset_source = load_image("aseprite_landscape_full.png")
    scifi_source = load_image("aseprite_scifi_full.png")
    city_evening_source = load_image("cityscape_evening_raw2.png")
    city_night_source = load_image("cityscape_night_raw2.png")

    variants = []

    sunset_a = resize_to_board(crop_box(sunset_source, (20, 0, 180, 160)))
    sunset_a = enhance_image(sunset_a, contrast=1.18, color=1.08, brightness=1.03, sharpness=1.15)
    sunset_a = quantize_adaptive(sunset_a, 12)
    variants.append(("sunset_ridge_a", "Sunset Ridge A", "warm horizon", sunset_a))

    sunset_b = resize_to_board(crop_box(sunset_source, (12, 0, 172, 160)))
    sunset_b = enhance_image(sunset_b, contrast=1.28, color=1.18, brightness=0.98, sharpness=1.2)
    sunset_b = quantize_adaptive(sunset_b, 10)
    variants.append(("sunset_ridge_b", "Sunset Ridge B", "deeper silhouette", sunset_b))

    sunset_c = resize_to_board(crop_box(sunset_source, (30, 6, 170, 146)))
    sunset_c = enhance_image(sunset_c, contrast=1.34, color=1.16, brightness=1.04, sharpness=1.18)
    sunset_c = quantize_adaptive(sunset_c, 9)
    sunset_c = add_sunset_glow(sunset_c)
    variants.append(("sunset_ridge_c", "Sunset Ridge C", "tighter sun core", sunset_c))

    sunset_d = resize_to_board(crop_box(sunset_source, (0, 0, 200, 150)))
    sunset_d = enhance_image(sunset_d, contrast=1.14, color=1.24, brightness=1.02, sharpness=1.1)
    sunset_d = quantize_adaptive(sunset_d, 11)
    variants.append(("sunset_ridge_d", "Sunset Ridge D", "wider crimson sky", sunset_d))

    scifi_a = resize_to_board(crop_box(scifi_source, (10, 36, 170, 196)))
    scifi_a = enhance_image(scifi_a, contrast=1.2, color=1.15, brightness=1.02, sharpness=1.1)
    scifi_a = quantize_adaptive(scifi_a, 12)
    variants.append(("scifi_lake_a", "SciFi Lake A", "planet focus", scifi_a))

    scifi_b = resize_to_board(crop_box(scifi_source, (0, 24, 160, 184)))
    scifi_b = enhance_image(scifi_b, contrast=1.15, color=1.2, brightness=1.0, sharpness=1.12)
    scifi_b = quantize_adaptive(scifi_b, 14)
    scifi_b = add_stars(scifi_b)
    variants.append(("scifi_lake_b", "SciFi Lake B", "moon + planet", scifi_b))

    scifi_c = resize_to_board(crop_box(scifi_source, (22, 48, 172, 198)))
    scifi_c = enhance_image(scifi_c, contrast=1.24, color=1.22, brightness=1.02, sharpness=1.15)
    scifi_c = quantize_adaptive(scifi_c, 12)
    variants.append(("scifi_lake_c", "SciFi Lake C", "closer reflection", scifi_c))

    scifi_d = resize_to_board(crop_box(scifi_source, (0, 0, 180, 160)))
    scifi_d = enhance_image(scifi_d, contrast=1.08, color=1.26, brightness=0.98, sharpness=1.08)
    scifi_d = quantize_adaptive(scifi_d, 13)
    scifi_d = add_stars(scifi_d)
    variants.append(("scifi_lake_d", "SciFi Lake D", "more sky space", scifi_d))

    city_a = resize_to_board(city_evening_source)
    city_a = enhance_image(city_a, contrast=1.2, color=1.05, brightness=1.0, sharpness=1.08)
    city_a = quantize_adaptive(city_a, 10)
    variants.append(("city_evening_a", "City Evening A", "clean skyline", city_a))

    city_b = resize_to_board(city_evening_source)
    city_b = enhance_image(city_b, contrast=1.32, color=1.18, brightness=1.0, sharpness=1.12)
    city_b = quantize_adaptive(city_b, 12)
    city_b = add_city_neon(city_b)
    variants.append(("city_evening_b", "City Evening B", "neon accent", city_b))

    city_c = resize_to_board(city_night_source)
    city_c = enhance_image(city_c, contrast=1.3, color=1.08, brightness=1.04, sharpness=1.14)
    city_c = quantize_adaptive(city_c, 10)
    variants.append(("city_night_c", "City Night C", "dense night blocks", city_c))

    city_d = resize_to_board(city_night_source)
    city_d = enhance_image(city_d, contrast=1.38, color=1.18, brightness=1.06, sharpness=1.18)
    city_d = quantize_adaptive(city_d, 12)
    city_d = add_city_window_sparks(add_city_neon(city_d))
    variants.append(("city_night_d", "City Night D", "spark windows", city_d))

    city_e = resize_to_board(crop_box(city_evening_source, (14, 0, 242, 256)))
    city_e = enhance_image(city_e, contrast=1.22, color=1.14, brightness=1.02, sharpness=1.1)
    city_e = quantize_adaptive(city_e, 11)
    variants.append(("city_evening_e", "City Evening E", "tighter tower crop", city_e))

    city_f = resize_to_board(crop_box(city_evening_source, (0, 10, 256, 238)))
    city_f = enhance_image(city_f, contrast=1.3, color=1.2, brightness=1.0, sharpness=1.15)
    city_f = quantize_adaptive(city_f, 12)
    city_f = add_city_window_sparks(city_f)
    variants.append(("city_evening_f", "City Evening F", "extra window lights", city_f))

    return variants


def build_sheet(variants):
    cols = 4
    rows = ceil(len(variants) / cols)
    card_w = 420
    card_h = 470
    pad = 16
    sheet = Image.new("RGBA", (pad + cols * (card_w + pad), pad + rows * (card_h + pad)), (15, 18, 28, 255))
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()

    for idx, (_, title, subtitle, board) in enumerate(variants):
        col = idx % cols
        row = idx // cols
        x = pad + col * (card_w + pad)
        y = pad + row * (card_h + pad)
        draw.rounded_rectangle(
            (x, y, x + card_w, y + card_h),
            radius=18,
            fill=(28, 32, 48, 255),
            outline=(87, 94, 125, 255),
            width=2,
        )
        draw.text((x + 16, y + 16), title, fill=(244, 247, 255, 255), font=font)
        draw.text((x + 16, y + 34), subtitle, fill=(157, 172, 206, 255), font=font)
        sheet.alpha_composite(create_preview_card(board), (x + 18, y + 60))
        draw.text((x + 18, y + 436), "64x64 finalist", fill=(168, 180, 210, 255), font=font)

    sheet.save(OUTPUT_DIR / "github_finalists_sheet.png")


def main():
    ensure_output_dir()
    variants = build_variants()
    for stem, _, _, board in variants:
        save_variant(stem, board)
    build_sheet(variants)


if __name__ == "__main__":
    main()
