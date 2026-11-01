#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Screenshot gallery of the built site at phone and laptop sizes, in several states, for visual
// review.  node tools/smoke/screens.cjs --dist ~/build/christmasreckoning/dist --out shots/ [--only name]
'use strict';

const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');
const puppeteer = require('puppeteer-core');

const MIME = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.css': 'text/css',
  '.wasm': 'application/wasm', '.json': 'application/json', '.svg': 'image/svg+xml',
  '.jpg': 'image/jpeg', '.png': 'image/png', '.br': 'application/octet-stream', '.gz': 'application/octet-stream'
};

const VIEWPORTS = {
  phone: { width: 390, height: 844, deviceScaleFactor: 2, isMobile: true, hasTouch: true },
  laptop: { width: 1366, height: 768, deviceScaleFactor: 1 },
  desktop: { width: 1680, height: 1000, deviceScaleFactor: 1 },
  smallLaptop: { width: 1280, height: 720, deviceScaleFactor: 1 },
  phoneLandscape: { width: 844, height: 390, deviceScaleFactor: 2, isMobile: true, hasTouch: true },
  og: { width: 1200, height: 630, deviceScaleFactor: 1 }
};

// name, viewport, hash/query, wait ms, optional action
const SHOTS = [
  ['countdown-laptop', 'laptop', '/', 1500],
  ['countdown-phone', 'phone', '/', 1500],
  ['countdown-eve-laptop', 'laptop', '/?now=2026-12-24T20:00:00Z', 2000],
  ['countdown-eve-phone', 'phone', '/?now=2026-12-24T20:00:00Z', 2000],
  ['mission-laptop', 'laptop', '/?now=2026-12-24T20:00:00Z#/mission-control', 4000],
  ['mission-phone', 'phone', '/?now=2026-12-24T20:00:00Z#/mission-control', 4000],
  ['mission-desktop', 'desktop', '/?now=2026-12-24T20:00:00Z#/mission-control', 4000],
  ['console-fido-laptop', 'laptop', '/?now=2026-12-24T20:00:00Z#/mission-control/fido', 3000],
  ['console-fido-phone', 'phone', '/?now=2026-12-24T20:00:00Z#/mission-control/fido', 3000],
  ['console-windows-laptop', 'laptop', '/?now=2026-12-24T20:00:00Z#/mission-control/windows', 3000],
  ['scenario-phone', 'phone', '/#/mission-control/scenario', 3000],
  ['mission-preview-laptop', 'laptop', '/#/mission-control', 9000],
  ['countdown-smalllaptop', 'smallLaptop', '/', 1500],
  ['mission-smalllaptop', 'smallLaptop', '/?now=2026-12-24T20:00:00Z#/mission-control', 4000],
  ['mission-phone-landscape', 'phoneLandscape', '/?now=2026-12-24T20:00:00Z#/mission-control', 4000],
  ['og', 'og', '/?now=2026-12-24T21:30:00Z#/mission-control', 5000]
];

function serve(root) {
  const server = http.createServer((req, res) => {
    const url = new URL(req.url, 'http://localhost');
    let file = path.join(root, decodeURIComponent(url.pathname));
    if (fs.existsSync(file) && fs.statSync(file).isDirectory()) file = path.join(file, 'index.html');
    fs.readFile(file, (err, data) => {
      if (err) { res.writeHead(404).end(); return; }
      res.writeHead(200, { 'Content-Type': MIME[path.extname(file)] || 'application/octet-stream' });
      res.end(data);
    });
  });
  return new Promise(resolve => server.listen(0, '127.0.0.1', () => resolve(server)));
}

async function main() {
  const args = {};
  for (let i = 2; i < process.argv.length; i += 2) args[process.argv[i].replace(/^--/, '')] = process.argv[i + 1];
  const out = path.resolve(args.out || 'shots');
  fs.mkdirSync(out, { recursive: true });
  const server = await serve(path.resolve(args.dist));
  const base = `http://127.0.0.1:${server.address().port}`;
  const browser = await puppeteer.launch({
    executablePath: args.chrome || process.env.CHROME_PATH || '/usr/bin/chromium',
    headless: true,
    args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist']
  });
  const problems = [];
  for (const [name, viewport, url, wait] of SHOTS) {
    if (args.only && !name.includes(args.only)) continue;
    const page = await browser.newPage();
    await page.setViewport(VIEWPORTS[viewport]);
    await page.emulateTimezone('Europe/London');
    page.on('console', m => { if (m.type() === 'error' || m.type() === 'warn') problems.push(`${name}: ${m.text()}`); });
    page.on('pageerror', e => problems.push(`${name}: ${e}`));
    await page.goto(base + url, { waitUntil: 'domcontentloaded' });
    await page.waitForFunction('window.crAppReady === true', { timeout: 120000 });
    await new Promise(r => setTimeout(r, wait));
    await page.screenshot({ path: path.join(out, `${name}.png`) });
    console.log('shot', name);
    await page.close();
  }
  await browser.close();
  server.close();
  if (problems.length) console.log('console problems:\n  ' + [...new Set(problems)].join('\n  '));
}

main().catch(e => { console.error(e); process.exit(1); });
