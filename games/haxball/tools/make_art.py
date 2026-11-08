#!/usr/bin/env python3
"""Draws the art of Haxball that is geometry: the pitch, the players' discs, the ball, the kick ring and the particles.

Run it from anywhere (needs Pillow): `python3 games/haxball/tools/make_art.py`. It overwrites the files in
assets/sprites/ and assets/particles/. The sizes repeat scripts/shared/rules.luau: change both together.
"""

import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ASSETS = Path(__file__).resolve().parent.parent / "assets"
SCALE = 4  # drawn bigger, then shrunk: smooth edges

VIEW = (1280, 720)
CENTER = (640, 396)
PITCH = (1100, 520)
GOAL_HALF = 85
GOAL_DEPTH = 50
POST_RADIUS = 8
PLAYER_RADIUS = 20
BALL_RADIUS = 12

GRASS = [(96, 140, 74), (88, 131, 67)]
OUTSIDE = (76, 112, 60)
LINE = (232, 240, 226)
NET = (215, 222, 210)


def canvas(size, color=(0, 0, 0, 0)):
    return Image.new("RGBA", (size[0] * SCALE, size[1] * SCALE), color)


def shrink(image, size):
    return image.resize(size, Image.LANCZOS)


def s(value):
    return value * SCALE


def pitch():
    left, right = CENTER[0] - PITCH[0] / 2, CENTER[0] + PITCH[0] / 2
    top, bottom = CENTER[1] - PITCH[1] / 2, CENTER[1] + PITCH[1] / 2
    image = canvas(VIEW, OUTSIDE + (255,))
    draw = ImageDraw.Draw(image)

    # The stripes of the grass, over the whole view, darker outside the lines.
    stripe = PITCH[0] / 10
    x = left - stripe * math.ceil(left / stripe)
    index = 0
    while x < VIEW[0]:
        draw.rectangle([s(x), 0, s(x + stripe), s(VIEW[1])], fill=GRASS[index % 2] + (255,))
        x += stripe
        index += 1
    shade = Image.new("RGBA", image.size, (0, 0, 0, 0))
    ImageDraw.Draw(shade).rectangle([0, 0, image.size[0], image.size[1]], fill=(20, 40, 10, 70))
    ImageDraw.Draw(shade).rectangle([s(left), s(top), s(right), s(bottom)], fill=(0, 0, 0, 0))
    image = Image.alpha_composite(image, shade)
    draw = ImageDraw.Draw(image)

    width = s(3)
    # The nets, behind the goal lines.
    for line_x, outward in ((left, -1), (right, 1)):
        back = line_x + outward * GOAL_DEPTH
        x0, x1 = sorted((line_x, back))
        draw.rectangle([s(x0), s(CENTER[1] - GOAL_HALF), s(x1), s(CENTER[1] + GOAL_HALF)], fill=(60, 80, 52, 255))
        step = 10
        gx = x0
        while gx <= x1:
            draw.line([s(gx), s(CENTER[1] - GOAL_HALF), s(gx), s(CENTER[1] + GOAL_HALF)], fill=NET + (120,), width=s(1))
            gx += step
        gy = CENTER[1] - GOAL_HALF
        while gy <= CENTER[1] + GOAL_HALF:
            draw.line([s(x0), s(gy), s(x1), s(gy)], fill=NET + (120,), width=s(1))
            gy += step
        draw.line(
            [s(line_x), s(CENTER[1] - GOAL_HALF), s(back), s(CENTER[1] - GOAL_HALF), s(back), s(CENTER[1] + GOAL_HALF),
             s(line_x), s(CENTER[1] + GOAL_HALF)],
            fill=NET + (255,), width=width, joint="curve")

    # The lines: the touchlines, the halfway line, the center circle and spot, the goal areas.
    draw.rectangle([s(left), s(top), s(right), s(bottom)], outline=LINE + (255,), width=width)
    draw.line([s(CENTER[0]), s(top), s(CENTER[0]), s(bottom)], fill=LINE + (255,), width=width)
    radius = 80
    draw.ellipse([s(CENTER[0] - radius), s(CENTER[1] - radius), s(CENTER[0] + radius), s(CENTER[1] + radius)],
                 outline=LINE + (255,), width=width)
    draw.ellipse([s(CENTER[0] - 5), s(CENTER[1] - 5), s(CENTER[0] + 5), s(CENTER[1] + 5)], fill=LINE + (255,))
    area = (110, GOAL_HALF + 70)
    for line_x, inward in ((left, 1), (right, -1)):
        x0, x1 = sorted((line_x, line_x + inward * area[0]))
        draw.rectangle([s(x0), s(CENTER[1] - area[1]), s(x1), s(CENTER[1] + area[1])], outline=LINE + (255,), width=width)
        arc = 60
        cx = line_x + inward * (area[0] - arc / 2)  # the arc meets the area's edge at 60 degrees
        start, end = (-60, 60) if inward == 1 else (120, 240)
        draw.arc([s(cx - arc), s(CENTER[1] - arc), s(cx + arc), s(CENTER[1] + arc)], start, end, fill=LINE + (255,),
                 width=width)

    # The posts.
    for line_x in (left, right):
        for post_y in (CENTER[1] - GOAL_HALF, CENTER[1] + GOAL_HALF):
            r = POST_RADIUS
            draw.ellipse([s(line_x - r), s(post_y - r), s(line_x + r), s(post_y + r)], fill=(250, 250, 250, 255),
                         outline=(40, 40, 40, 255), width=s(2))

    shrink(image, VIEW).convert("RGB").save(ASSETS / "sprites" / "pitch.png")


def disc(radius, fill, outline, thickness):
    """A disc with a dark outline and a soft shadow, `radius` + 4 pixels around its center."""
    size = (radius + 4) * 2
    image = canvas((size, size))
    shadow = canvas((size, size))
    c = size / 2
    ImageDraw.Draw(shadow).ellipse([s(c - radius + 1), s(c - radius + 3), s(c + radius + 1), s(c + radius + 3)],
                                   fill=(0, 0, 0, 90))
    shadow = shadow.filter(ImageFilter.GaussianBlur(s(2)))
    image = Image.alpha_composite(image, shadow)
    draw = ImageDraw.Draw(image)
    draw.ellipse([s(c - radius), s(c - radius), s(c + radius), s(c + radius)], fill=fill + (255,),
                 outline=outline + (255,), width=s(thickness))
    return image, draw, c


def player(name, color):
    image, draw, c = disc(PLAYER_RADIUS, color, (20, 20, 20), 2)
    # A light along the top: the disc looks round.
    size = image.size[0] // SCALE
    highlight = canvas((size, size))
    ImageDraw.Draw(highlight).ellipse([s(c - 12), s(c - 16), s(c + 8), s(c - 4)], fill=(255, 255, 255, 50))
    highlight = highlight.filter(ImageFilter.GaussianBlur(s(2)))
    image = Image.alpha_composite(image, highlight)
    shrink(image, (size, size)).save(ASSETS / "sprites" / f"{name}.png")


def ball():
    image, draw, c = disc(BALL_RADIUS, (250, 250, 250), (20, 20, 20), 2)
    # The pentagons of a football: one in the middle, five around.
    def pentagon(cx, cy, r, turn):
        points = [(s(cx + r * math.cos(turn + i * 2 * math.pi / 5)), s(cy + r * math.sin(turn + i * 2 * math.pi / 5)))
                  for i in range(5)]
        draw.polygon(points, fill=(30, 30, 30, 255))
    pentagon(c, c, 3.6, -math.pi / 2)
    for i in range(5):
        angle = -math.pi / 2 + i * 2 * math.pi / 5 + math.pi / 5
        pentagon(c + 8.5 * math.cos(angle), c + 8.5 * math.sin(angle), 2.6, angle)
    # The outline again, over the pentagons that reach it.
    draw.ellipse([s(c - BALL_RADIUS), s(c - BALL_RADIUS), s(c + BALL_RADIUS), s(c + BALL_RADIUS)],
                 outline=(20, 20, 20, 255), width=s(2))
    size = image.size[0] // SCALE
    shrink(image, (size, size)).save(ASSETS / "sprites" / "ball.png")


def ring():
    """The white ring around a player holding kick, and the marker over the local player."""
    radius = PLAYER_RADIUS
    size = (radius + 4) * 2
    image = canvas((size, size))
    c = size / 2
    ImageDraw.Draw(image).ellipse([s(c - radius), s(c - radius), s(c + radius), s(c + radius)],
                                  outline=(255, 255, 255, 255), width=s(3))
    shrink(image, (size, size)).save(ASSETS / "sprites" / "kick_ring.png")

    image = canvas((16, 12))
    ImageDraw.Draw(image).polygon([(s(1), s(1)), (s(15), s(1)), (s(8), s(11))], fill=(255, 255, 255, 255),
                                  outline=(20, 20, 20, 255))
    shrink(image, (16, 12)).save(ASSETS / "sprites" / "marker.png")


def particles():
    folder = ASSETS / "particles"
    # A soft round dot: sparks, trails, dust.
    size = 32
    image = canvas((size, size))
    ImageDraw.Draw(image).ellipse([s(8), s(8), s(24), s(24)], fill=(255, 255, 255, 255))
    image = image.filter(ImageFilter.GaussianBlur(s(4)))
    shrink(image, (size, size)).save(folder / "dot.png")
    # A puff of smoke: several blurred blobs.
    size = 64
    image = canvas((size, size))
    draw = ImageDraw.Draw(image)
    rng = random.Random(7)
    for _ in range(7):
        x, y, r = rng.uniform(22, 42), rng.uniform(22, 42), rng.uniform(9, 15)
        draw.ellipse([s(x - r), s(y - r), s(x + r), s(y + r)], fill=(255, 255, 255, 110))
    image = image.filter(ImageFilter.GaussianBlur(s(4)))
    shrink(image, (size, size)).save(folder / "smoke.png")
    # A piece of confetti, a ring and a star.
    image = canvas((12, 6))
    ImageDraw.Draw(image).rectangle([0, 0, s(12), s(6)], fill=(255, 255, 255, 255))
    shrink(image, (12, 6)).save(folder / "confetti.png")
    size = 64
    image = canvas((size, size))
    ImageDraw.Draw(image).ellipse([s(4), s(4), s(60), s(60)], outline=(255, 255, 255, 255), width=s(4))
    image = image.filter(ImageFilter.GaussianBlur(s(1)))
    shrink(image, (size, size)).save(folder / "ring.png")
    size = 32
    image = canvas((size, size))
    points = []
    for i in range(10):
        r = 15 if i % 2 == 0 else 6
        angle = -math.pi / 2 + i * math.pi / 5
        points.append((s(16 + r * math.cos(angle)), s(16 + r * math.sin(angle))))
    ImageDraw.Draw(image).polygon(points, fill=(255, 255, 255, 255))
    shrink(image, (size, size)).save(folder / "star.png")


def powerups():
    """The four power-ups in one row of 32x32 icons: speed, big, no inertia, strong kick (the order of rules.luau)."""
    size = 32
    colors = [(255, 200, 40), (70, 200, 110), (60, 200, 230), (255, 120, 40)]
    image = canvas((size * 4, size))
    draw = ImageDraw.Draw(image)
    white = (255, 255, 255, 255)
    for i, color in enumerate(colors):
        x0 = i * size
        c = x0 + size / 2
        draw.ellipse([s(x0 + 1), s(1), s(x0 + size - 1), s(size - 1)], fill=color + (255,), outline=white, width=s(2))
        if i == 0:  # two chevrons: speed
            for shift in (-4, 3):
                draw.line([(s(c + shift - 3), s(9)), (s(c + shift + 3), s(16)), (s(c + shift - 3), s(23))], fill=white,
                          width=s(3), joint="curve")
        elif i == 1:  # a small disc and a big ring: size
            draw.ellipse([s(c - 3), s(16 - 3), s(c + 3), s(16 + 3)], fill=white)
            draw.ellipse([s(c - 9), s(16 - 9), s(c + 9), s(16 + 9)], outline=white, width=s(2))
        elif i == 2:  # a stop bar and a slide mark: no inertia
            draw.rectangle([s(c + 3), s(8), s(c + 6), s(24)], fill=white)
            draw.line([(s(c - 9), s(16)), (s(c + 1), s(16))], fill=white, width=s(3))
        else:  # a star burst: strong kick
            points = []
            for k in range(16):
                r = 10 if k % 2 == 0 else 4
                angle = -math.pi / 2 + k * math.pi / 8
                points.append((s(c + r * math.cos(angle)), s(16 + r * math.sin(angle))))
            draw.polygon(points, fill=white)
    shrink(image, (size * 4, size)).save(ASSETS / "sprites" / "powerups.png")


def main():
    (ASSETS / "sprites").mkdir(parents=True, exist_ok=True)
    (ASSETS / "particles").mkdir(parents=True, exist_ok=True)
    pitch()
    player("player_red", (229, 72, 72))
    player("player_blue", (70, 128, 240))
    ball()
    ring()
    powerups()
    particles()


if __name__ == "__main__":
    main()
