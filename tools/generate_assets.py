from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "assets"
SIZE = 32


SKINS = {
    "default": {
        "ground": (69, 126, 78),
        "ground2": (78, 140, 87),
        "wall": (92, 95, 98),
        "wall2": (69, 72, 76),
        "obstacle": (104, 75, 55),
        "food": (222, 72, 64),
        "bonus": (246, 188, 75),
        "speed": (61, 181, 219),
        "slow": (151, 116, 215),
        "trap": (37, 39, 45),
        "shield": (87, 211, 145),
        "bow": (206, 154, 78),
        "spike": (232, 232, 232),
        "clock": (121, 120, 220),
        "player": (62, 214, 96),
        "player2": (45, 159, 82),
        "ai": (238, 82, 86),
        "ai2": (174, 48, 65),
    },
    "neon": {
        "ground": (30, 38, 63),
        "ground2": (38, 48, 80),
        "wall": (89, 96, 128),
        "wall2": (62, 67, 98),
        "obstacle": (96, 67, 121),
        "food": (255, 84, 132),
        "bonus": (255, 203, 89),
        "speed": (60, 226, 234),
        "slow": (180, 116, 255),
        "trap": (22, 24, 36),
        "shield": (80, 255, 175),
        "bow": (255, 190, 95),
        "spike": (235, 244, 255),
        "clock": (180, 116, 255),
        "player": (74, 255, 135),
        "player2": (32, 178, 110),
        "ai": (255, 95, 119),
        "ai2": (185, 45, 86),
    },
    "ice": {
        "ground": (139, 187, 203),
        "ground2": (157, 207, 220),
        "wall": (210, 226, 232),
        "wall2": (151, 178, 190),
        "obstacle": (101, 126, 138),
        "food": (230, 80, 91),
        "bonus": (255, 216, 101),
        "speed": (39, 161, 213),
        "slow": (111, 120, 218),
        "trap": (51, 70, 82),
        "shield": (81, 213, 195),
        "bow": (189, 124, 74),
        "spike": (239, 250, 255),
        "clock": (111, 120, 218),
        "player": (43, 188, 148),
        "player2": (32, 133, 119),
        "ai": (223, 79, 95),
        "ai2": (154, 52, 72),
    },
}


def tile_base(colors):
    image = Image.new("RGB", (SIZE, SIZE), colors["ground"])
    draw = ImageDraw.Draw(image)
    for y in range(0, SIZE, 8):
        for x in range(0, SIZE, 8):
            if (x // 8 + y // 8) % 2 == 0:
                draw.rectangle([x, y, x + 7, y + 7], fill=colors["ground2"])
    return image


def save_tile(folder, name, image):
    out = ASSET_DIR / folder / name
    out.parent.mkdir(parents=True, exist_ok=True)
    image.save(out)
    if out.suffix.lower() == ".png":
        image.save(out.with_suffix(".bmp"))


def draw_wall(colors):
    image = Image.new("RGB", (SIZE, SIZE), colors["wall"])
    draw = ImageDraw.Draw(image)
    for y in range(0, SIZE, 8):
        offset = 0 if y % 16 == 0 else 8
        for x in range(-offset, SIZE, 16):
            draw.rectangle([x, y, x + 15, y + 7], outline=colors["wall2"], fill=colors["wall"])
    return image


def draw_obstacle(colors):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    draw.polygon([(7, 25), (12, 9), (20, 6), (27, 23), (19, 28)], fill=colors["obstacle"])
    draw.line([(12, 10), (16, 18), (22, 14)], fill=(70, 50, 40), width=2)
    return image


def draw_food(colors, key):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    color = colors[key]
    if key == "bonus":
        points = [(16, 5), (19, 13), (28, 13), (21, 18), (24, 27),
                  (16, 22), (8, 27), (11, 18), (4, 13), (13, 13)]
        draw.polygon(points, fill=color)
    elif key == "speed":
        draw.polygon([(18, 3), (7, 18), (15, 18), (12, 29), (25, 13), (17, 13)], fill=color)
    elif key == "slow":
        draw.ellipse([7, 7, 25, 25], fill=color)
        draw.arc([10, 10, 22, 22], 20, 300, fill=(240, 240, 255), width=3)
    else:
        draw.ellipse([8, 9, 24, 26], fill=color)
        draw.rectangle([15, 5, 17, 10], fill=(92, 62, 36))
        draw.ellipse([18, 5, 24, 11], fill=(94, 175, 78))
    return image


def draw_trap(colors):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    draw.rectangle([5, 20, 27, 27], fill=colors["trap"])
    for x in range(7, 26, 6):
        draw.polygon([(x, 20), (x + 3, 8), (x + 6, 20)], fill=(232, 232, 232))
    return image


def draw_shield(colors):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    draw.polygon([(16, 5), (25, 9), (23, 22), (16, 28), (9, 22), (7, 9)], fill=colors["shield"])
    draw.line([(16, 8), (16, 25)], fill=(230, 255, 240), width=2)
    return image


def draw_battle_shield(colors):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    draw.polygon([(16, 4), (26, 8), (25, 20), (16, 29), (7, 20), (6, 8)], fill=colors["shield"])
    draw.polygon([(16, 8), (22, 11), (21, 19), (16, 25), (11, 19), (10, 11)], fill=(35, 58, 70))
    draw.line([(16, 8), (16, 25)], fill=(230, 255, 240), width=2)
    return image


def draw_bow(colors):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    draw.arc([4, 4, 28, 28], 70, 290, fill=colors["bow"], width=4)
    draw.line([(17, 6), (17, 26)], fill=(245, 235, 210), width=1)
    draw.line([(7, 16), (25, 16)], fill=(245, 245, 245), width=2)
    draw.polygon([(25, 16), (20, 12), (21, 16), (20, 20)], fill=(235, 235, 235))
    return image


def draw_spike(colors):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    draw.rectangle([5, 22, 27, 27], fill=colors["trap"])
    for x in range(6, 25, 7):
        draw.polygon([(x, 22), (x + 4, 6), (x + 8, 22)], fill=colors["spike"])
        draw.line([(x + 4, 7), (x + 4, 22)], fill=(150, 160, 170), width=1)
    return image


def draw_clock(colors):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    draw.ellipse([6, 6, 26, 26], fill=colors["clock"], outline=(235, 240, 255), width=2)
    draw.line([(16, 16), (16, 9)], fill=(245, 245, 255), width=2)
    draw.line([(16, 16), (21, 19)], fill=(245, 245, 255), width=2)
    draw.rectangle([12, 3, 20, 6], fill=(235, 240, 255))
    return image


def draw_snake(colors, primary_key, secondary_key, head):
    image = tile_base(colors)
    draw = ImageDraw.Draw(image)
    primary = colors[primary_key]
    secondary = colors[secondary_key]
    if head:
        draw.rounded_rectangle([5, 5, 27, 27], radius=8, fill=primary, outline=secondary, width=3)
        draw.ellipse([10, 11, 14, 15], fill=(20, 25, 25))
        draw.ellipse([22, 11, 26, 15], fill=(20, 25, 25))
        draw.rectangle([15, 20, 21, 22], fill=secondary)
    else:
        draw.rounded_rectangle([4, 7, 28, 25], radius=7, fill=primary, outline=secondary, width=3)
        draw.line([(8, 16), (24, 16)], fill=(230, 255, 230), width=2)
    return image


def generate_skin(folder, colors):
    save_tile(folder, "ground.png", tile_base(colors))
    save_tile(folder, "wall.png", draw_wall(colors))
    save_tile(folder, "obstacle.png", draw_obstacle(colors))
    save_tile(folder, "food.png", draw_food(colors, "food"))
    save_tile(folder, "food_bonus.png", draw_food(colors, "bonus"))
    save_tile(folder, "food_speed.png", draw_food(colors, "speed"))
    save_tile(folder, "food_slow.png", draw_food(colors, "slow"))
    save_tile(folder, "trap.png", draw_trap(colors))
    save_tile(folder, "shield.png", draw_shield(colors))
    save_tile(folder, "bow.png", draw_bow(colors))
    save_tile(folder, "battle_shield.png", draw_battle_shield(colors))
    save_tile(folder, "spike.png", draw_spike(colors))
    save_tile(folder, "clock.png", draw_clock(colors))
    save_tile(folder, "player_head.png", draw_snake(colors, "player", "player2", True))
    save_tile(folder, "player_body.png", draw_snake(colors, "player", "player2", False))
    save_tile(folder, "ai_head.png", draw_snake(colors, "ai", "ai2", True))
    save_tile(folder, "ai_body.png", draw_snake(colors, "ai", "ai2", False))


def main():
    for folder, colors in SKINS.items():
        generate_skin(folder, colors)
    print(f"Generated assets in {ASSET_DIR}")


if __name__ == "__main__":
    main()
