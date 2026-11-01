# Christmas Reckoning

**www.ChristmasReckoning.com** is a Christmas countdown, plus **North Pole Mission Control**: a live
simulation of what it would really take for Santa to visit every child on Earth in one night,
using real population data and real physics. Every number shows its formula, its inputs and its
sources.

It is a [Qt 6](https://www.qt.io) / QML application compiled to WebAssembly and hosted on GitHub Pages.

## What's on the site

- **Countdown:** a live countdown to Christmas in your own time zone. It shows the countdown to
  Santa's launch window, his live progress during Christmas Eve, and his landing.
- **Mission Control:**
  - A ray-traced globe and a ground-track map, with the real day/night line for Christmas Eve.
  - Santa's position and trail, and delivered regions lighting up as he goes.
  - Telemetry and a timeline you can scrub, play at 1×/60×/3600×, run as "the whole night in
    60 s", or follow live.
- **13 flight-controller consoles,** each with a GO/NO-GO verdict: MANIFEST, LAUNCH, FIDO, RETRO,
  LOADMASTER, PROP, THERMAL, RANGE SAFETY, SURGEON, NAV, TIMING, DEBRIEF and FLIGHT. Every
  calculation is shown step by step:
  - the typeset formula;
  - today's numbers plugged in;
  - the result;
  - a warning when a formula is being pushed outside its range of validity;
  - links to its sources.
- **Scenario panel:** 18 assumptions you can change (who gets presents, the delivery window,
  stop time, route quality, altitude, sleigh, reindeer, cookies…). The whole room recalculates
  instantly.

### The model, briefly

| Piece | Approach |
|---|---|
| Households | Children aged 0–17 for each country (UN WPP 2024) ÷ children per household with children (UN Household Size & Composition 2026), placed on the ~1 km GHSL 2025 population grid |
| Route | Beardwood–Halton–Hammersley tour estimate `β·Σ√(N·A)`, summed cell by cell at 30″ so clustering counts, plus minimum-spanning-tree hops between disconnected clusters |
| Schedule | Time zones (37, with real Christmas Eve offsets) are jobs with delivery windows. Horn's condition gives the exact minimum peak speed and the bottleneck run of zones; Yao–Demers–Shenker gives energy-optimal speeds per zone |
| Physics | US Standard Atmosphere 1976, drag, Mach, stagnation temperature (ideal gas and plasma bound), Sutton–Graves heating, blast radius, orbital/escape speed, curvature downforce, Lorentz factor, allometric reindeer power… with compile-time dimensional analysis |

With the defaults (every child under 18, 22:00–06:00 local):
- 2.4 billion children in 1.1 billion households;
- a 66 million km route;
- 82% of households fall in a 15-hour bottleneck from UTC+8 to UTC+1;
- a minimum top speed of 940 km/s;
- 110 µs per household.

## Repository layout

```
src/core/      physics, scheduler, analysis, timeline, world-data reader (C++20 + QtCore, unit-tested)
src/app/       Qt Quick app: QML pages and components, globe/snow shaders, platform bridge
src/tools/     cr-report (build-time numbers for the HTML fallback and formula rendering)
tests/         Qt Test suites (countdown, sun position, physics, formatting, scheduler, analysis, real data)
data/          world.bin + delivery.png (generated, committed) and world-report.json
tools/data/    dataset fetching (pinned SHA-256) and the world builder
tools/fonts/   subset OFL fonts     tools/formulas/  MathJax → SVG     tools/web/  site packager
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

### Regenerating the formulas

```sh
cr-report --formulas > formulas.json
node tools/formulas/render.cjs formulas.json assets/formulas
```

CI fails if the committed SVGs don't match the code.

## Deployment

Every push runs `.github/workflows/site.yml`:
1. Native unit tests.
2. A formula check.
3. The WebAssembly build.
4. A headless Chrome smoke test (routing, deep links, the countdown across time zones).
5. On `master`, a deploy to GitHub Pages.

The `.wasm` ships precompressed and with content-hashed names, so a browser can never mix an old
script with a new binary.

## Licence

The code is licensed under **GPL-3.0-or-later** (`LICENSE`). Fonts are SIL OFL. The data comes from
UN WPP and the UN household database (CC BY 3.0 IGO), JRC GHSL (CC BY 4.0), Natural Earth (public
domain), and timezone-boundary-builder (ODbL; the derived zone tables are shared under ODbL). NASA
imagery is public domain. Full credits are in `THIRD_PARTY_NOTICES.md`.
