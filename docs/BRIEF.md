# Prompt: Rebuild ChristmasReckoning.com as a Qt/QML WebAssembly app

You are rebuilding **www.ChristmasReckoning.com** (repo: `RobDeGeorge/ChristmasReckoning`, hosted on GitHub Pages with a `CNAME`). Today it is a buildless HTML/CSS/vanilla JS site. Replace it with a **Qt 6 / QML application compiled to WebAssembly**. The app has exactly two pages:

1. **Countdown**: a live countdown to Christmas.
2. **The Reckoning** (working name for the logistics page): the main feature. It is an interactive simulation and real-world analysis of the math and physics it would take for Santa to deliver presents to every household on Earth in one night.

The logistics page is the reason the site exists. The countdown page is the front door.

---

## 0. Before writing code

- Read the existing repo. `about.html` already contains "Santa's Christmas Eve Journey: The Facts" (sections: The Monumental Task, Speed Requirements, The Payload, Reindeer Power, Delivery Logistics, Environmental Challenges, Time Zone Mastery, Navigation, The Magic Equation). Use it as a starting outline, but recompute every number from first principles and sources. Don't copy its figures.
- `countdown.js` has the current countdown logic: local time zone, Christmas Day state, and rollover to next year. Port that behaviour faithfully. Its tests are in `tests/countdown.test.cjs`.
- Propose a short plan (architecture, file layout, data pipeline, milestones) and confirm it with me before building everything.
- The old site's other features (snow game, Santa letter composer, daily gifts and ornaments, blog) are **out of scope**. Leave them out, but list them in the plan so I can confirm they're being dropped.

## 1. Tech stack and hosting constraints (hard requirements)

- **Qt 6.8 LTS or newer**, Qt Quick / QML, C++ for the simulation core. Use **Qt Quick 3D** for the globe if it runs acceptably on WebGL2. Otherwise fall back to a custom shader-based globe in Qt Quick, and explain the trade-off.
- **Build for single-threaded WebAssembly.** GitHub Pages can't set the COOP/COEP headers that multithreaded Qt for WebAssembly needs. If you think threads are essential, say so and propose a workaround (for example `coi-serviceworker`) with its caveats, but don't depend on it by default.
- Pin the **Emscripten version to exactly the one the chosen Qt version requires**, and document it.
- Build in **GitHub Actions** (`aqtinstall` for Qt, `emsdk` for Emscripten) and deploy the static output (`.html`, `.js`, `.wasm`, `qtloader.js`, assets, `CNAME`) to GitHub Pages. The site must keep working at `www.ChristmasReckoning.com`.
- **Load-time budget:** Qt wasm binaries are large. Use only the Qt modules you need, build with `-Os` or `-Oz`, compress where Pages serves it, and keep the data files small. Report the final `.wasm` size and the total transfer size.
- **Custom HTML shell:** design a branded loading screen with real download progress (not Qt's default), a `<noscript>`/no-WebGL fallback, plus `<title>`, meta description and Open Graph tags. A canvas app is invisible to search engines, so the shell should also contain a short static HTML summary of the key numbers.
- **Routing:** two pages in a `StackView` (or similar), with URL hash deep links (`#/` and `#/reckoning`) synced both ways through Emscripten JS interop, so links and browser back/forward work.
- **Responsive:** desktop and mobile, portrait and landscape, touch and mouse. Target 60 fps on a mid-range laptop and stay usable on a recent phone. Add a reduced-motion mode.
- **Licensing:** Qt for WebAssembly links statically. Check that Qt LGPL/GPL obligations are compatible with this repo's `license.txt` and the public source, and state the conclusion.

## 2. Page 1: Countdown

- A large live countdown (days, hours, minutes, seconds) to **midnight starting December 25 in the visitor's local time zone**. Handle the Christmas Day state and the automatic rollover to next year, the way `countdown.js` does today.
- A secondary readout: **"Santa launches in …"**. Santa's run starts when Dec 25 midnight arrives on the first inhabited time zone, UTC+14 (Kiribati). Show which time zone he's working through right now, and during Christmas Eve, live "presents delivered so far" from the simulation model.
- A festive but restrained visual style: night sky, snowfall that respects reduced motion, and a small rotating globe teaser that invites people into The Reckoning.
- A clear call to action leading to the logistics page.

## 3. Page 2: The Reckoning (the main feature)

The goal is a **real-world engineering analysis presented as an interactive simulation**. The tone is "what if we took this completely seriously?". It should be rigorous, funny where the numbers are absurd, and never hand-wavy. **Every number on the page must be derived on screen**: formula, inputs with units, result, and source for each input.

### 3a. The simulation (hero section)

- **A 3D Earth** with a day/night **terminator** computed for real on Dec 24–25 UTC, a city-lights texture on the night side, and Earth rotating in correct real time relative to the sun.
- **Santa's sleigh and route:** Santa moves westward, chasing local midnight across time zones from UTC+14 to UTC−12. Draw his trail, and render households as a heatmap that "lights up" as he delivers to them.
- **A timeline scrubber** across the whole delivery window, with play/pause and speed multipliers (1×, 60×, 3600×, or "the whole night in 60 seconds"). It must be deterministic, so the same time always gives the same state.
- **A live HUD:** UTC and local time at Santa's position, households visited, presents delivered, distance flown, current speed (km/s, Mach, % of c), current g-load, sleigh mass, cookies eaten and calories, energy used, and how much time per house is left.
- **Camera modes:** free orbit, follow Santa, and a top-down map view.

### 3b. Route model (be explicit and honest about approximations)

- Build a **household-density grid** from a real gridded population dataset (for example GPWv4, WorldPop or Kontur; pick one, cite it, and record the licence). Preprocess it offline into a compact binary or JSON grid (around 0.25°–1°), using a documented script checked into the repo (`tools/`).
- Turn population into **eligible households** with adjustable parameters: the share of the population who are children, children per household, and an "eligibility" slider. The famous analyses only count households that celebrate Christmas. Default to everyone and let the user narrow it.
- **Route ordering:** sweep cells in order of local midnight, which roughly follows longitude and time zone boundaries. Estimate the path length *inside* each cell with the **Beardwood–Halton–Hammersley result for the travelling salesman problem**, L ≈ 0.7124·√(N·A), and add the hops between cells. Show this formula, and explain why exact TSP is impossible at this scale.
- **Delivery window:** derive it from the time zone spread plus how long children are asleep. Make it a parameter (for example, deliver between 22:00 and 06:00 local time), and show how it changes everything.

### 3c. The analysis sections (each one interactive, with live formulas)

Each section has adjustable inputs (sliders), live-updating equations, a short plain-English explanation, a "how absurd is this?" comparison, and citations. Compute everything in the C++ core and expose it to QML through properties. No magic numbers in QML.

1. **The task:** world population, children, households, and presents. Show the dataset's numbers and the assumptions.
2. **Time budget:** the delivery window in hours, and time per household (microseconds).
3. **Distance:** total route length using the TSP model above, and its sensitivity to household clustering.
4. **Speed:** average required speed. Compare it to the speed of sound, a bullet, the ISS, **orbital velocity (7.9 km/s) and escape velocity (11.2 km/s)**. Above escape velocity, staying near the ground takes *downward* force: compute it.
5. **Stop-and-go kinematics:** deceleration and acceleration at every house. Compute the g-forces (and how fatal they are for Santa and the reindeer), the stopping distance needed at a survivable g, and why stopping at every house blows the time budget.
6. **Payload:** present mass (adjustable, for example 0.5–2 kg), total sleigh mass, and how it falls as presents are delivered. Plot mass against time, and compare it to the largest cargo ships and aircraft.
7. **Propulsion (reindeer):** the force needed to get that mass to that speed, F = ma, and the power required. Compare it to what a real reindeer can pull, and work out how many reindeer it would take.
8. **Aerodynamics:** drag F = ½ρv²C_dA at altitude (adjustable). Show the Mach number, **stagnation temperature**, and the heating power on the lead reindeer. Note where ideal-gas formulas stop being valid (real-gas and plasma effects at hypersonic speeds). Compare to re-entering spacecraft and the classic "reindeer vaporize in thousandths of a second" claim, and either check that claim or correct it.
9. **Energy:** kinetic energy at cruise speed, total energy for the night, the energy burned by the stop-and-go cycles, comparisons in TNT equivalent and in global annual energy use, and the sonic boom energy delivered to each neighbourhood.
10. **Relativity (for fun, done properly):** the Lorentz factor at the required speed, how much Santa's clock drifts over the night, and the speed he would need for time dilation to *actually* help. Spoiler: no.
11. **Cookies and milk:** calories eaten at one cookie per house, and how long a person would need to burn them off.
12. **Navigation and the Earth:** the Coriolis effect at that speed, great-circle routes against the time zone sweep, and keeping up with the terminator.
13. **The verdict / "The Magic Equation":** a summary dashboard of the most impossible numbers, framed with the joke that the shortfall has to be made up by Christmas magic. Quantify that shortfall in physical units.

### 3d. Charts and visuals

- Interactive charts in QML (Qt Graphs if it's available for wasm in the chosen Qt version, otherwise custom Canvas or Shapes): sleigh mass over time, speed and deliveries per time zone, energy breakdown, and the g-load profile of a single delivery.
- Use log scales where values span many orders of magnitude, and **label the axes with units**.
- Render formulas with a proper math renderer (KaTeX-generated SVGs at build time, or a clean QML formula component). No raw LaTeX on screen.

## 4. Correctness and quality

- **Unit-safe physics core:** use a small typed units approach or rigorous naming (`_m`, `_s`, `_kg`), keep all physical constants in one header, and cite sources (CODATA, ISA atmosphere tables, and so on).
- **Unit tests** (Qt Test or Catch2) for the physics core: known-answer tests for each formula, a check that the time window and countdown are right across time zones (port the cases from `tests/countdown.test.cjs`), and determinism tests for the simulation. Run them in CI with a native (non-wasm) build.
- A **"Sources & assumptions"** panel listing every dataset and constant, with links, plus a downloadable JSON of the current parameters and results.
- **Accessibility:** set Accessible names and roles on QML controls, make it fully keyboard-operable, and keep text contrast at WCAG AA. Mention any Qt-for-wasm accessibility limits you hit.

## 5. Deliverables

- New source layout (`src/`, `qml/`, `data/`, `tools/`, `tests/`, `web/` for the HTML shell), `CMakeLists.txt`, and the CI workflow.
- An updated `README.md` covering the native build, the wasm build, the exact Qt and Emscripten versions, the data preprocessing steps, and how deployment works.
- Remove the old site files only after the new build deploys successfully, and keep `CNAME`.
- At the end, a short report: `.wasm` and total download size, measured load time, fps on desktop and mobile, the known limitations, and any number in the analysis you aren't confident about.

## 6. Naming

"The Reckoning" is the working title for the logistics page. It ties into the ChristmasReckoning brand. Also suggest three to five alternatives for the page name and its tagline (for example "Sleigh Math", "The Flight Plan", or "One Night, Two Billion Stops").
