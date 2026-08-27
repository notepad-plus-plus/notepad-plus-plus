#!/usr/bin/env python3
"""Extract PNG app icons from a Windows .ico for the freedesktop hicolor theme.

Usage: make-icons.py <input.ico> <hicolor-root>
Writes <hicolor-root>/<WxH>/apps/notepad-plus-plus.png for each native size,
plus an upscaled 256x256 for high-DPI menus.
"""
import sys
from pathlib import Path

from PIL import Image

APP = "notepad-plus-plus"


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: make-icons.py <input.ico> <hicolor-root>", file=sys.stderr)
        return 2
    ico_path, hicolor = Path(sys.argv[1]), Path(sys.argv[2])

    im = Image.open(ico_path)
    # Pillow exposes every embedded frame via the .ico helper.
    try:
        sizes = sorted(im.ico.sizes())  # type: ignore[attr-defined]
    except Exception:
        sizes = [im.size]

    written = []
    largest = None
    for w, h in sizes:
        try:
            frame = im.ico.getimage((w, h))  # type: ignore[attr-defined]
        except Exception:
            im.size = (w, h)
            frame = im.copy()
        frame = frame.convert("RGBA")
        out = hicolor / f"{w}x{h}" / "apps" / f"{APP}.png"
        out.parent.mkdir(parents=True, exist_ok=True)
        frame.save(out, "PNG")
        written.append(str(out))
        if largest is None or w * h > largest[0]:
            largest = (w * h, frame)

    # High-DPI upscale so app launchers have a crisp-enough large icon.
    if largest is not None:
        big = largest[1].resize((256, 256), Image.LANCZOS)
        out = hicolor / "256x256" / "apps" / f"{APP}.png"
        out.parent.mkdir(parents=True, exist_ok=True)
        big.save(out, "PNG")
        written.append(str(out))

    for w in written:
        print("icon:", w)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
