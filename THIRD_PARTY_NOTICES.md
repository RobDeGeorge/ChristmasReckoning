# Third-party notices

Christmas Reckoning is free software under the GNU General Public License v3.0 or later (see `LICENSE`).
It is built with, and ships, the following third-party software, data and assets.

## Software

- **Qt 6.12** (Qt Base, Qt Declarative, Qt Quick Controls, Qt SVG, Qt Network, Qt Shader Tools),
  © The Qt Company Ltd. and other contributors. Used under the GNU LGPL v3 (`qtloader.js`: GNU GPL v3).
  The WebAssembly build links Qt statically. The complete source of this application is in this
  repository, and Qt's source is available from <https://download.qt.io/official_releases/qt/6.12/>,
  so you can rebuild the site against a modified Qt.
- **Emscripten** runtime support code, © Emscripten authors, MIT / University of Illinois NCSA licence.

## Fonts (SIL Open Font License 1.1; licence texts in `assets/fonts/LICENSES/`)

- **DM Sans**, © The DM Sans Project Authors.
- **Playfair Display**, © The Playfair Display Project Authors.
- **JetBrains Mono**, © The JetBrains Mono Project Authors.

The bundled files are subset, static instances made by `tools/fonts/build_fonts.py`.

## Data

`data/world.bin`, `data/delivery.png` and `data/world-report.json` are derived by
`tools/data/build_world.py` from:

- **GHS-POP R2023A** (epoch 2025, 30″), European Commission, Joint Research Centre.
  Schiavina M., Freire S., Carioli A., MacManus K. (2023), doi:10.2905/2FF68A52-5B5B-4A22-8F40-C41DA8332CFE.
  **CC BY 4.0.**
- **World Population Prospects 2024**, United Nations, DESA, Population Division. **CC BY 3.0 IGO.**
- **Database on Household Size and Composition 2026**, United Nations, DESA, Population Division
  (UN DESA/POP/2026/DC/NO.15). **CC BY 3.0 IGO.**
- **Natural Earth** 1:10m Admin 0 countries. **Public domain.**
- **timezone-boundary-builder** release 2026f, © Evan Siroky and contributors, containing information
  from OpenStreetMap. Licensed under the **Open Database License (ODbL) 1.0**.
  The time zone tables derived from it (the zone assignments inside `data/world.bin` and the zone
  layer of `data/delivery.png`) are a derivative database and are made available under the
  **ODbL 1.0**: <https://opendatacommons.org/licenses/odbl/1-0/>. © OpenStreetMap contributors.
- **IANA Time Zone Database** (tzdata 2026e). Public domain.

## Imagery

- **Blue Marble: Next Generation**, December (`web/textures/earth-day.jpg`), NASA Earth Observatory.
- **Black Marble 2016** (`web/textures/earth-night.jpg`), NASA Earth Observatory.

NASA imagery is not protected by copyright in the United States. See
<https://www.nasa.gov/nasa-brand-center/images-and-media/>.

Every constant and reference figure used in the calculations is cited in the app's
"Sources & assumptions" panel (`src/core/sources.cpp`).
