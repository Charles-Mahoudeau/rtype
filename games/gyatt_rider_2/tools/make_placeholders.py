#!/usr/bin/env python3
"""Draws the placeholder assets of GyattRider 2: flat shapes of one or two colors, to replace with real art.

Run it from anywhere: python3 games/gyatt_rider_2/tools/make_placeholders.py
It overwrites the files it makes in assets/. A replacement keeps the same file name and the same size in tiles.
The roads are vertical: everything faces up, the way the riders go.
"""

import json
import math
from pathlib import Path

from PIL import Image, ImageDraw

TILE = 64  # pixels per tile
ROAD_TILES = 3  # a road is three tiles wide: a vehicle (2) drives in its middle
ROAD_COUNT = 8
SIDEBAR_TILES = 1  # the strip on the left, under the timer and the gauge
VIEW_WIDTH = (SIDEBAR_TILES + ROAD_TILES * ROAD_COUNT) * TILE  # 25 tiles: 1600
VIEW_HEIGHT = 900  # 16:9 with that width
ROAD_SCROLL = 600  # pixels per second the roads scroll down: Game.roadSpeed
BEAM_SQUARE = 52  # pixels: one square of a shot's beam, Game.beamSquare

ASSETS = Path(__file__).resolve().parent.parent / "assets"

ASPHALT = (43, 43, 46, 255)
EDGE_LINE = (232, 182, 40, 255)
LANE_DASH = (235, 235, 235, 255)

# Name, size in tiles (width, height), body color: the obstacles, front up.
VEHICLES = [
    ("car", (2, 3), (214, 48, 49, 255)),
    ("electric_car", (2, 2), (0, 184, 148, 255)),
    ("van", (2, 4), (223, 230, 233, 255)),
    ("truck", (2, 5), (64, 64, 190, 255)),
    ("bus", (2, 6), (253, 171, 61, 255)),
    ("limousine", (2, 7), (108, 52, 131, 255)),
]
CYCLIST = ("cyclist", (1, 2), (250, 250, 250, 255))
# The players' colors, in slot order: blue, red, green, purple.
RIDERS = [(110, 193, 255, 255), (255, 154, 154, 255), (124, 252, 124, 255), (215, 155, 255, 255)]
WINDSHIELD = (40, 52, 64, 255)


def darker(color, factor=0.55):
    return tuple(int(channel * factor) for channel in color[:3]) + (255,)


def save(image, relative):
    path = ASSETS / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)
    print(f"wrote {path.relative_to(ASSETS.parent.parent)}")


def body(size_tiles, color, windshield=True):
    """A rounded box with a darker border and, near the top, a windshield showing which way it faces."""
    width, height = size_tiles[0] * TILE, size_tiles[1] * TILE
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    margin = 4
    draw.rounded_rectangle((margin, margin, width - margin - 1, height - margin - 1), radius=0, fill=color,
                           outline=darker(color), width=4)
    if windshield:
        top = margin + 14
        draw.rounded_rectangle((margin + 12, top, width - margin - 13, top + min(36, height // 5)), radius=0,
                               fill=WINDSHIELD)
    return image


def roads():
    """The eight roads side by side, one road length high, tiling vertically: yellow edges, a dashed middle line."""
    road_width = ROAD_TILES * TILE
    height = 2 * TILE
    image = Image.new("RGBA", (road_width * ROAD_COUNT, height), ASPHALT)
    draw = ImageDraw.Draw(image)
    for road in range(ROAD_COUNT):
        left = road * road_width
        draw.rectangle((left + 4, 0, left + 7, height - 1), fill=EDGE_LINE)
        draw.rectangle((left + road_width - 8, 0, left + road_width - 5, height - 1), fill=EDGE_LINE)
        middle = left + road_width // 2
        for top in range(0, height, TILE):
            draw.rectangle((middle - 2, top + TILE // 4, middle + 1, top + 3 * TILE // 4 - 1), fill=LANE_DASH)
    return image


def roads_map(image_size):
    """A Tiled map of the view with one image layer: the roads, repeated down and scrolling on their own."""
    return {
        "type": "map",
        "version": "1.10",
        "tiledversion": "1.11.2",
        "orientation": "orthogonal",
        "renderorder": "right-down",
        "infinite": False,
        "width": VIEW_WIDTH // TILE,
        "height": math.ceil(VIEW_HEIGHT / TILE),
        "tilewidth": TILE,
        "tileheight": TILE,
        "nextlayerid": 2,
        "nextobjectid": 1,
        # Where the camera stands when it sees the layers at their place: the middle of the view.
        "parallaxoriginx": VIEW_WIDTH // 2,
        "parallaxoriginy": VIEW_HEIGHT // 2,
        "tilesets": [],
        "layers": [
            {
                "id": 1,
                "name": "roads",
                "type": "imagelayer",
                "image": "../roads.png",
                "imagewidth": image_size[0],
                "imageheight": image_size[1],
                "offsetx": SIDEBAR_TILES * TILE,
                "offsety": 0,
                "parallaxx": 0,
                "parallaxy": 0,
                "repeatx": False,
                "repeaty": True,
                "opacity": 1,
                "visible": True,
                "x": 0,
                "y": 0,
                "properties": [{"name": "scrolly", "type": "float", "value": ROAD_SCROLL}],
            }
        ],
    }


def particle_square():
    """The particles' plain white square, tinted by each effect."""
    return Image.new("RGBA", (8, 8), (255, 255, 255, 255))


def beam_square():
    """One square of a shot's beam, which the client repeats from the shooter to what it hits."""
    return Image.new("RGBA", (BEAM_SQUARE, BEAM_SQUARE), (255, 255, 255, 255))


def main():
    road_image = roads()
    save(road_image, "roads.png")
    path = ASSETS / "levels" / "roads.tmj"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(roads_map(road_image.size), indent=2) + "\n")
    print(f"wrote {path.relative_to(ASSETS.parent.parent)}")
    for name, size, color in VEHICLES:
        save(body(size, color), f"vehicles/{name}.png")
    name, size, color = CYCLIST
    save(body(size, color), f"cyclists/{name}.png")
    for slot, color in enumerate(RIDERS):
        save(body((1, 2), color), f"riders/rider_{slot}.png")
    save(beam_square(), "beam.png")
    save(particle_square(), "particle.png")


if __name__ == "__main__":
    main()
