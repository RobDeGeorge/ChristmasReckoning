#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// End-to-end smoke test: loads the built site in headless Chromium, waits for the Qt app's first
// frame, checks routing (hash links, back/forward, deep links), checks the countdown against
// JavaScript's Date in several emulated time zones, and saves screenshots.
//
//   node tools/smoke/smoke.cjs --dist ~/build/christmasreckoning/dist [--out shots/] [--chrome path]
//   node tools/smoke/smoke.cjs --url https://www.christmasreckoning.com
//
// Locally, puppeteer-core lives outside the repo: NODE_PATH=~/.cache/christmasreckoning/node/node_modules
'use strict';

const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');
const puppeteer = require('puppeteer-core');

function parseArgs(argv) {
  const args = { out: path.resolve('smoke-output'), timeout: 120000, only: null };
  for (let i = 2; i < argv.length; i++) {
    const key = argv[i].replace(/^--/, '');
    const value = argv[i + 1];
    if (['dist', 'url', 'out', 'chrome', 'only'].includes(key)) { args[key] = value; i++; }
    else if (key === 'timeout') { args.timeout = Number(value); i++; }
    else if (key === 'gpu') args.gpu = true;
  }
  return args;
}

const MIME = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.css': 'text/css',
  '.wasm': 'application/wasm', '.json': 'application/json', '.svg': 'image/svg+xml',
  '.jpg': 'image/jpeg', '.png': 'image/png', '.br': 'application/octet-stream',
  '.gz': 'application/octet-stream', '.txt': 'text/plain'
};

function serve(root) {
  const server = http.createServer((req, res) => {
    const url = new URL(req.url, 'http://localhost');
    let file = path.join(root, decodeURIComponent(url.pathname));
    if (!file.startsWith(root)) { res.writeHead(403).end(); return; }
    if (fs.existsSync(file) && fs.statSync(file).isDirectory()) file = path.join(file, 'index.html');
    fs.readFile(file, (err, data) => {
      if (err) { res.writeHead(404).end('not found'); return; }
      res.writeHead(200, { 'Content-Type': MIME[path.extname(file)] || 'application/octet-stream',
                           'Cache-Control': 'no-store' });
      res.end(data);
    });
  });
  return new Promise(resolve => server.listen(0, '127.0.0.1', () => resolve(server)));
}

function findChrome(explicit) {
  const candidates = [explicit, process.env.CHROME_PATH, '/usr/bin/chromium', '/usr/bin/chromium-browser',
    '/usr/bin/google-chrome', '/usr/bin/google-chrome-stable'].filter(Boolean);
  const found = candidates.find(p => fs.existsSync(p));
  if (!found) throw new Error('No Chrome/Chromium found; pass --chrome');
  return found;
}

// The original site's algorithm, evaluated inside the page so it uses the page's time zone.
const EXPECTED_COUNTDOWN = `(() => {
  const now = new Date(window.crState.countdown.nowMs);
  const isChristmas = now.getMonth() === 11 && now.getDate() === 25;
  const year = now.getFullYear() + (now.getMonth() === 11 && now.getDate() > 25 ? 1 : 0);
  const remaining = isChristmas ? 0 : Math.max(0, new Date(year, 11, 25).getTime() - now.getTime());
  return { year, isChristmas, totalSeconds: Math.ceil(remaining / 1000) };
})()`;

async function main() {
  const args = parseArgs(process.argv);
  fs.mkdirSync(args.out, { recursive: true });
  let server = null;
  let base = args.url;
  if (!base) {
    if (!args.dist) throw new Error('pass --dist or --url');
    server = await serve(path.resolve(args.dist));
    base = `http://127.0.0.1:${server.address().port}`;
  }
  base = base.replace(/\/$/, '');

  const browser = await puppeteer.launch({
    executablePath: findChrome(args.chrome),
    headless: true,
    args: args.gpu ? ['--ignore-gpu-blocklist', '--enable-gpu']
                   : ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'],
    defaultViewport: { width: 1440, height: 900, deviceScaleFactor: 1 }
  });

  const results = [];
  const failures = [];
  function check(name, ok, detail) {
    results.push({ name, ok, detail });
    if (!ok) failures.push(`${name}: ${detail}`);
    console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}${detail ? '  — ' + detail : ''}`);
  }

  async function openPage(url, { timezone, viewport } = {}) {
    const page = await browser.newPage();
    const errors = [];
    page.on('pageerror', e => errors.push(String(e)));
    page.on('console', m => {
      if (m.type() === 'error') errors.push(m.text());
      if (process.env.SMOKE_VERBOSE) console.log('[console]', m.type(), m.text());
    });
    if (timezone) await page.emulateTimezone(timezone);
    if (viewport) await page.setViewport(viewport);
    const started = Date.now();
    await page.goto(url, { waitUntil: 'domcontentloaded' });
    await page.waitForFunction('window.crAppReady === true', { timeout: args.timeout, polling: 100 });
    const readyMs = Date.now() - started;
    return { page, errors, readyMs };
  }

  const sleep = ms => new Promise(r => setTimeout(r, ms));

  try {
    // 1. Countdown page loads and renders.
    {
      const { page, errors, readyMs } = await openPage(`${base}/`);
      check('app reaches first frame', true, `${readyMs} ms`);
      const loadInfo = await page.evaluate(() => window.crLoadInfo);
      check('wasm streamed precompressed', !!loadInfo && !!loadInfo.format, JSON.stringify(loadInfo));
      const transfer = await page.evaluate(() => performance.getEntriesByType('resource')
        .reduce((sum, e) => sum + (e.transferSize || e.encodedBodySize || 0), 0));
      check('total transfer measured', transfer > 0, `${(transfer / 1048576).toFixed(2)} MiB`);
      await sleep(1500);
      await page.screenshot({ path: path.join(args.out, 'countdown.png') });
      const route = await page.evaluate(() => window.crState && window.crState.route);
      check('initial route is countdown', route === '', JSON.stringify(route));

      // 2. Hash navigation into Mission Control, then browser back.
      await page.evaluate(() => { location.hash = '#/mission-control'; });
      await page.waitForFunction(() => window.crState.route === 'mission-control', { timeout: 15000 });
      check('hash link opens Mission Control', true);
      await sleep(4000); // let the Earth textures arrive and fade in
      await page.screenshot({ path: path.join(args.out, 'mission-control.png') });
      await page.goBack();
      await page.waitForFunction(() => window.crState.route === '', { timeout: 15000 });
      check('back button returns to countdown', true);
      check('no console errors (countdown session)', errors.length === 0, errors.join(' | '));
      await page.close();
    }

    // 3. Deep link straight into Mission Control.
    {
      const { page, errors } = await openPage(`${base}/#/mission-control`);
      const route = await page.evaluate(() => window.crState.route);
      check('deep link to #/mission-control', route === 'mission-control', JSON.stringify(route));
      check('no console errors (deep link)', errors.length === 0, errors.join(' | '));
      await page.close();
    }

    // 4. Countdown matches JavaScript Date across time zones, including Christmas Day itself.
    const cases = [
      ['America/New_York', '2026-10-31T04:00:00Z'],
      ['Pacific/Auckland', '2026-12-24T11:00:00Z'],
      ['America/Phoenix', '2026-12-25T06:59:59Z'],
      ['Asia/Kolkata', '2026-12-26T00:00:00Z']
    ];
    for (const [timezone, now] of cases) {
      const { page } = await openPage(`${base}/?now=${encodeURIComponent(now)}`, { timezone });
      const actual = await page.evaluate(() => window.crState.countdown);
      const expected = await page.evaluate(EXPECTED_COUNTDOWN);
      const ok = actual.year === expected.year && actual.isChristmas === expected.isChristmas
        && Math.abs(actual.totalSeconds - expected.totalSeconds) <= 1;
      check(`countdown in ${timezone} @ ${now}`, ok,
            `app ${JSON.stringify({ y: actual.year, c: actual.isChristmas, s: actual.totalSeconds })} vs js ${JSON.stringify({ y: expected.year, c: expected.isChristmas, s: expected.totalSeconds })}`);
      await page.close();
    }

    // 5. The live tracker: preview before the night, live (and actually lifting off) on Christmas
    //    Eve, replay after landing. ?now= starts the page's clock at that instant and lets it run.
    {
      const tracker = async (page) => page.evaluate(() => window.crState && window.crState.tracker);
      const waitTracker = (page, predicate, timeout) =>
        page.waitForFunction(`window.crState && window.crState.tracker && (${predicate})(window.crState.tracker)`,
                             { timeout: timeout || 20000, polling: 250 });

      let { page } = await openPage(`${base}/?now=2026-12-10T12:00:00Z#/live`, { timezone: 'Europe/London' });
      await waitTracker(page, 't => true');
      let t = await tracker(page);
      check('#/live before Christmas Eve is a preview', t.mode === 'preview' && !t.live, JSON.stringify(t));
      check('#/live opens Mission Control', await page.evaluate(() => window.crState.route) === 'mission-control/live');
      await page.close();

      ({ page } = await openPage(`${base}/?now=2026-12-24T07:59:52Z#/live`, { timezone: 'America/New_York' }));
      await waitTracker(page, 't => t.live');
      t = await tracker(page);
      check('live on the pad before launch', t.live && t.phase === 'prelaunch', JSON.stringify(t));
      await sleep(1500);
      await page.screenshot({ path: path.join(args.out, 'live-prelaunch.png') });
      await waitTracker(page, 't => t.phase !== "prelaunch"', 30000);
      t = await tracker(page);
      check('Santa lifts off live at launch time', t.live && t.phase !== 'prelaunch', JSON.stringify(t));
      await page.close();

      ({ page } = await openPage(`${base}/?now=2026-12-24T20:00:00Z#/mission-control`, { timezone: 'America/New_York' }));
      await waitTracker(page, 't => t.live');
      t = await tracker(page);
      check('live during the night', t.live && t.phase === 'delivering', JSON.stringify(t));
      await sleep(2000);
      await page.screenshot({ path: path.join(args.out, 'live-night.png') });
      await page.close();

      ({ page } = await openPage(`${base}/?now=2026-12-25T21:00:00Z#/mission-control`, { timezone: 'Europe/London' }));
      await waitTracker(page, 't => true');
      t = await tracker(page);
      check('after landing it is a replay', t.mode === 'replay' && !t.live, JSON.stringify(t));
      await page.close();
    }

    // 6. Phone-sized viewport renders.
    {
      const { page } = await openPage(`${base}/`, { viewport: { width: 390, height: 844, deviceScaleFactor: 2, isMobile: true, hasTouch: true } });
      await sleep(1500);
      await page.screenshot({ path: path.join(args.out, 'countdown-phone.png') });
      check('phone viewport renders', true);
      await page.close();
    }
  } catch (error) {
    check('smoke run', false, error.stack || String(error));
  } finally {
    await browser.close();
    if (server) server.close();
  }

  fs.writeFileSync(path.join(args.out, 'results.json'), JSON.stringify(results, null, 2));
  console.log(failures.length ? `\n${failures.length} check(s) failed` : '\nall checks passed');
  process.exit(failures.length ? 1 : 0);
}

main().catch(error => { console.error(error); process.exit(1); });
