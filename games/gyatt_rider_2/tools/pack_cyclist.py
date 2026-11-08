#!/usr/bin/env python3
"""Packs the cyclist's frames (assets/cyclists/cyclist1.png, 2, 3) side by side into assets/cyclists/cyclist_sheet.png,
the image of the cyclist's atlas (scripts/client/assets.luau). Run it after changing a frame:

    python3 games/gyatt_rider_2/tools/pack_cyclist.py
"""

from pathlib import Path

from PIL import Image

CYCLISTS = Path(__file__).resolve().parent.parent / "assets" / "cyclists"
FRAMES = ["cyclist1.png", "cyclist2.png", "cyclist3.png"]


def main():
    frames = [Image.open(CYCLISTS / name).convert("RGBA") for name in FRAMES]
    width, height = frames[0].size
    for name, frame in zip(FRAMES, frames):
        if frame.size != (width, height):
            raise SystemExit(f"{name} is {frame.size[0]}x{frame.size[1]}, the frames must all be {width}x{height}")
    sheet = Image.new("RGBA", (width * len(frames), height), (0, 0, 0, 0))
    for index, frame in enumerate(frames):
        sheet.paste(frame, (index * width, 0))
    sheet.save(CYCLISTS / "cyclist_sheet.png")
    print(f"wrote cyclists/cyclist_sheet.png: {len(frames)} frames of {width}x{height}")


if __name__ == "__main__":
    main()
