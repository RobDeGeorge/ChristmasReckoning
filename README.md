# Christmas Reckoning

**www.ChristmasReckoning.com** is a Santa tracker. It shows the flight path Santa will take around
the world, counts down to liftoff, and on Christmas Eve follows him live.

It is a [Qt 6](https://www.qt.io) / QML application compiled to WebAssembly and hosted on GitHub Pages.

## What's on the site

One screen:

- **Before Christmas Eve:** a countdown to liftoff, a preview of the route on a globe, when Santa
  reaches your time zone, and a "Remind me" button that saves a calendar reminder.
- **On the night** (from two hours before liftoff): Santa's real position, live, with the path he
  has flown and the path still to come. `christmasreckoning.com/#/live` is the link to share.
- **Afterwards:** a replay, until the countdown to next year takes over.

### How the flight is worked out

Santa visits every child under 18, between 22:00 and 06:00 local time, one time zone after another.
Households come from UN population and household data placed on the ~1 km GHSL population grid;
the scheduler in `src/core` turns the time zones' delivery windows into a timetable, and the route
follows the populated places inside each zone. The position is a pure function of the clock, so
everyone watching sees the same sleigh at the same moment.

## Repository layout

```
src/core/      scheduler, timeline, physics, world-data reader (C++20 + QtCore, unit-tested)
src/app/       Qt Quick app: the tracker page, globe shader, platform bridge
tests/         Qt Test suites (countdown, sun position, physics, formatting, scheduler, analysis, real data)
data/          world.bin + delivery.png (generated, committed) and world-report.json
tools/data/    dataset fetching (pinned SHA-256) and the world builder
tools/fonts/   subset OFL fonts     tools/web/  site packager
tools/smoke/   headless-Chromium smoke test and screenshot gallery
web/           HTML shell, streaming loader, styles, NASA textures
docs/BRIEF.md  the original brief
```

## Building

Requirements: Qt 6.12.0 for desktop (`gcc_64`) and for WebAssembly (`wasm_singlethread`, with
`qtshadertools`); Emscripten **5.0.5** (the version Qt 6.12 is built with); CMake ≥ 3.24 and Ninja.
Build trees live outside the repository, in `~/build/christmasreckoning`.

```sh
pip install aqtinstall
aqt install-qt linux desktop 6.12.0 linux_gcc_64 -m qtshadertools -O ~/Qt
aqt install-qt all_os wasm 6.12.0 wasm_singlethread -m qtshadertools -O ~/Qt
# Emscripten: https://emscripten.org/docs/getting_started/downloads.html → emsdk install/activate 5.0.5

tools/dev.sh native     # desktop build + tests
tools/dev.sh test       # run the unit tests
tools/dev.sh wasm       # WebAssembly build
tools/dev.sh package    # assemble the site in ~/build/christmasreckoning/dist
tools/dev.sh serve      # http://localhost:8000
tools/dev.sh smoke      # headless browser checks (needs puppeteer-core; see tools/package.json)
```

Preview any moment: add `?now=2026-12-24T20:00:00Z` to the URL (`CR_NOW=…` for the desktop build).

### Regenerating the data

```sh
python3 tools/data/fetch.py          # downloads ~600 MB into ~/.cache/christmasreckoning/raw
python3 tools/data/build_world.py    # ~2 minutes; writes data/world.bin, delivery.png, world-report.json
```

Dependencies: `numpy rasterio pyshp openpyxl scipy pillow`. Every input is pinned by SHA-256 in
`tools/data/sources.json`.

## Deployment

Every push runs `.github/workflows/site.yml`:
1. Native unit tests.
2. The WebAssembly build.
3. A headless Chrome smoke test (the tracker before, during and after the flight; the countdown
   across time zones).
4. On `master`, a deploy to GitHub Pages.

The `.wasm` ships precompressed and with content-hashed names, so a browser can never mix an old
script with a new binary.

## Licence

The code is licensed under **GPL-3.0-or-later** (`LICENSE`). Fonts are SIL OFL. The data comes from
UN WPP and the UN household database (CC BY 3.0 IGO), JRC GHSL (CC BY 4.0), Natural Earth (public
domain), and timezone-boundary-builder (ODbL; the derived zone tables are shared under ODbL). NASA
imagery is public domain. Full credits are in `THIRD_PARTY_NOTICES.md`.
