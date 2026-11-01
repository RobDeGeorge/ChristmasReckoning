#!/usr/bin/env python3
"""Fetch the site's OFL fonts and produce small static, subset TTFs for the app.

Families (all SIL Open Font License 1.1, from github.com/google/fonts):
  DM Sans          - interface text (brand font of the original site)
  Playfair Display - display headings (brand font of the original site)
  JetBrains Mono   - telemetry readouts and formula substitutions (has Greek and math symbols)

Variable fonts are instanced to the static weights we use, because Qt's font matching is most
predictable with static faces, then subset to Latin, Greek and the symbols the physics pages need.
"""
from __future__ import annotations

import argparse
import pathlib
import shutil
import sys
import urllib.parse
import urllib.request

from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

RAW = "https://github.com/google/fonts/raw/main/ofl/"
CACHE = pathlib.Path.home() / ".cache" / "christmasreckoning" / "raw" / "fonts"
REPO = pathlib.Path(__file__).resolve().parents[2]

# (source file in google/fonts, output stem, axis locations)
FACES = [
    ("dmsans/DMSans[opsz,wght].ttf", "DMSans-Regular", {"wght": 400, "opsz": 14}),
    ("dmsans/DMSans[opsz,wght].ttf", "DMSans-Medium", {"wght": 500, "opsz": 14}),
    ("dmsans/DMSans[opsz,wght].ttf", "DMSans-Bold", {"wght": 700, "opsz": 14}),
    ("playfairdisplay/PlayfairDisplay[wght].ttf", "PlayfairDisplay-SemiBold", {"wght": 600}),
    ("playfairdisplay/PlayfairDisplay-Italic[wght].ttf", "PlayfairDisplay-SemiBoldItalic", {"wght": 600}),
    ("jetbrainsmono/JetBrainsMono[wght].ttf", "JetBrainsMono-Regular", {"wght": 400}),
    ("jetbrainsmono/JetBrainsMono[wght].ttf", "JetBrainsMono-Bold", {"wght": 700}),
]
LICENSES = ["dmsans/OFL.txt", "playfairdisplay/OFL.txt", "jetbrainsmono/OFL.txt"]

# Basic Latin + Latin-1 + Latin Extended-A, Greek, general punctuation, super/subscripts,
# currency, letterlike symbols, arrows, math operators, misc technical, box drawing, geometric shapes.
UNICODES = (
    "U+0020-007E,U+00A0-017F,U+0370-03FF,U+2000-206F,U+2070-209F,U+20A0-20CF,"
    "U+2100-214F,U+2190-21FF,U+2200-22FF,U+2300-23FF,U+2500-257F,U+25A0-25FF,U+2600-26FF,"
    "U+2713,U+2715,U+2744,U+2746,U+FFFD"
)


def fetch(rel: str) -> pathlib.Path:
    dest = CACHE / rel
    if dest.exists() and dest.stat().st_size > 0:
        return dest
    dest.parent.mkdir(parents=True, exist_ok=True)
    url = RAW + urllib.parse.quote(rel)
    print(f"downloading {url}", file=sys.stderr)
    with urllib.request.urlopen(url, timeout=120) as response, open(dest, "wb") as out:
        shutil.copyfileobj(response, out)
    return dest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=pathlib.Path, default=REPO / "assets" / "fonts")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    options = subset.Options()
    options.layout_features = ["kern", "liga", "calt", "tnum", "lnum", "sups", "subs", "frac", "case"]
    options.name_IDs = ["*"]
    options.notdef_outline = True
    options.hinting = False
    options.desubroutinize = True
    unicodes = subset.parse_unicodes(UNICODES)

    for source, stem, location in FACES:
        font = TTFont(fetch(source))
        if "fvar" in font:
            axes = {axis.axisTag for axis in font["fvar"].axes}
            font = instancer.instantiateVariableFont(
                font, {tag: value for tag, value in location.items() if tag in axes},
                updateFontNames=True)
        subsetter = subset.Subsetter(options)
        subsetter.populate(unicodes=unicodes)
        subsetter.subset(font)
        path = args.out / f"{stem}.ttf"
        font.save(path)
        print(f"{path.relative_to(REPO)}: {path.stat().st_size / 1024:.0f} KiB", file=sys.stderr)

    licenses = args.out / "LICENSES"
    licenses.mkdir(exist_ok=True)
    for rel in LICENSES:
        family = rel.split("/")[0]
        shutil.copyfile(fetch(rel), licenses / f"{family}-OFL.txt")
    return 0


if __name__ == "__main__":
    sys.exit(main())
