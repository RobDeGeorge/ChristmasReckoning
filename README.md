# Christmas Reckoning

A cozy Christmas countdown at www.ChristmasReckoning.com. Buildless HTML, CSS, and vanilla JavaScript, ready for static hosting.

## Preview locally

From this directory, run:

```sh
python3 -m http.server 8000
```

Open http://localhost:8000. There is no dependency installation or build step.

## The experience

- A live countdown to December 25 in the visitor’s local time zone, a Christmas Day celebration, and automatic next-year rollover.
- An illustrated snowy village with Evergreen, Moonlight, and Fireside colors, optional synthesized chimes, snowfall, fullscreen, and sharing.
- One daily gift, twelve collectible tree ornaments, visit streaks, and 24 rotating festive kindness prompts.
- A 30-second snowflake game with pointer, touch, and keyboard controls and a saved personal best.
- A Santa keepsake composer with live preview, browser-local draft saving, printable stationery, and a text download.
- The existing North Pole blog and Santa’s journey charts.

Daily progress, scene preferences, game records, and letter drafts use browser localStorage. These remain on the same browser, do not sync across devices, and reset when site data is cleared. When storage is blocked, features continue for the session. Letters are not transmitted to a server. Sound starts only on request, and background animations respect reduced-motion preferences and pause when hidden.

Google Fonts is the only external asset service; system font fallbacks keep the site usable if it is unavailable. Illustrations, snow, audio, and game assets are local.

## Check the behavior

Node.js 18 or later can run the dependency-free tests:

```sh
node --test tests/*.test.cjs
```

The suite covers countdown boundaries, local time and daylight saving, midnight gift and mission resets, duplicate gifts, streaks, corrupted storage, and persistence. Use a modern browser to check layouts, Web Audio, fullscreen, sharing, printing, and the snowflake game.

## Source guide

- `index.html`, `christmas.css`, `christmas.js`: homepage, scene controls, dialogs, snowfall, chimes, and sharing.
- `countdown.js`: countdown calculation and accessible display.
- `festive.js`: daily gifts, collection, missions, and streaks.
- `snow-game.js`, `snow-game.css`: snowflake game.
- `writeToSanta.html`, `santa.js`, `santa.css`: keepsake composer.
- `blog.html`, `about.html`, `legacy.css`: existing North Pole articles and charts in the shared visual style.
- `assets/`: local SVG illustrations and favicon.

Static hosting can serve the repository root directly. Keep the existing domain/hosting configuration when publishing.
