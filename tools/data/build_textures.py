#!/usr/bin/env python3
"""Fetch NASA Earth imagery and produce the globe textures shipped with the site.

Outputs (equirectangular, left edge = 180°W, top edge = 90°N):
  web/textures/earth-day.jpg    NASA Blue Marble Next Generation, December 2004 (with snow cover)
  web/textures/earth-night.jpg  NASA Black Marble 2016 (city lights), greyscale

Both source images are NASA imagery and are not copyrighted (NASA media usage guidelines).
Raw downloads are cached outside the repository in ~/.cache/christmasreckoning/raw.
"""
from __future__ import annotations

import argparse
import hashlib
import pathlib
import shutil
import sys
import urllib.request

from PIL import Image

Image.MAX_IMAGE_PIXELS = None  # The Black Marble source is ~91 megapixels.

SOURCES = {
    "day": {
        "url": "https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73909/"
               "world.topo.bathy.200412.3x5400x2700.jpg",
        "file": "world.topo.bathy.200412.3x5400x2700.jpg",
    },
    "night": {
        "url": "https://eoimages.gsfc.nasa.gov/images/imagerecords/144000/144898/"
               "BlackMarble_2016_3km.jpg",
        "file": "BlackMarble_2016_3km.jpg",
    },
}

CACHE = pathlib.Path.home() / ".cache" / "christmasreckoning" / "raw"
REPO = pathlib.Path(__file__).resolve().parents[2]


def fetch(url: str, dest: pathlib.Path) -> pathlib.Path:
    if dest.exists() and dest.stat().st_size > 0:
        return dest
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(dest.suffix + ".part")
    print(f"downloading {url}", file=sys.stderr)
    with urllib.request.urlopen(url, timeout=120) as response, open(tmp, "wb") as out:
        shutil.copyfileobj(response, out)
    tmp.rename(dest)
    return dest


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--width", type=int, default=4096, help="output width in pixels (height = width/2)")
    parser.add_argument("--out", type=pathlib.Path, default=REPO / "web" / "textures")
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    size = (args.width, args.width // 2)

    day_src = fetch(SOURCES["day"]["url"], CACHE / SOURCES["day"]["file"])
    night_src = fetch(SOURCES["night"]["url"], CACHE / SOURCES["night"]["file"])
    print(f"day   sha256 {sha256(day_src)}", file=sys.stderr)
    print(f"night sha256 {sha256(night_src)}", file=sys.stderr)

    day = Image.open(day_src).convert("RGB").resize(size, Image.Resampling.LANCZOS)
    day.save(args.out / "earth-day.jpg", quality=86, optimize=True, progressive=True, subsampling="4:2:0")

    night = Image.open(night_src).convert("L").resize(size, Image.Resampling.LANCZOS)
    night.save(args.out / "earth-night.jpg", quality=88, optimize=True, progressive=True)

    for name in ("earth-day.jpg", "earth-night.jpg"):
        path = args.out / name
        print(f"{path.relative_to(REPO)}: {path.stat().st_size / 1024:.0f} KiB", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
