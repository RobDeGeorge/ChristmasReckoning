#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Typesets every console formula (TeX from `cr-report --formulas`) into SVG with MathJax, adjusted for
// Qt's SVG renderer: fixed pixel sizes, explicit colour, no external font cache.
//
//   cr-report --formulas > formulas.json
//   NODE_PATH=~/.cache/christmasreckoning/node/node_modules node tools/formulas/render.cjs formulas.json assets/formulas
'use strict';

const fs = require('node:fs');
const path = require('node:path');
const { mathjax } = require('mathjax-full/js/mathjax.js');
const { TeX } = require('mathjax-full/js/input/tex.js');
const { SVG } = require('mathjax-full/js/output/svg.js');
const { liteAdaptor } = require('mathjax-full/js/adaptors/liteAdaptor.js');
const { RegisterHTMLHandler } = require('mathjax-full/js/handlers/html.js');
const { AllPackages } = require('mathjax-full/js/input/tex/AllPackages.js');

const COLOUR = '#f4f0e4'; // Theme.text
const EX_PX = 8.6;        // pixels per ex at the size the consoles show formulas

function main() {
  const [input, outDir] = process.argv.slice(2);
  if (!input || !outDir) {
    console.error('usage: render.cjs formulas.json output-dir');
    process.exit(2);
  }
  const formulas = JSON.parse(fs.readFileSync(input, 'utf8'));
  fs.mkdirSync(outDir, { recursive: true });
  for (const old of fs.readdirSync(outDir))
    if (old.endsWith('.svg')) fs.unlinkSync(path.join(outDir, old));

  const adaptor = liteAdaptor();
  RegisterHTMLHandler(adaptor);
  const doc = mathjax.document('', {
    InputJax: new TeX({ packages: AllPackages }),
    OutputJax: new SVG({ fontCache: 'none' })
  });

  let count = 0;
  for (const [id, tex] of Object.entries(formulas)) {
    const node = doc.convert(tex, { display: true, em: 16, ex: 8, containerWidth: 1200 });
    let svg = adaptor.innerHTML(node);
    svg = svg
      .replace(/width="([\d.]+)ex"/, (_, w) => `width="${(parseFloat(w) * EX_PX).toFixed(1)}"`)
      .replace(/height="([\d.]+)ex"/, (_, h) => `height="${(parseFloat(h) * EX_PX).toFixed(1)}"`)
      .replace(/ style="[^"]*"/g, '')
      .replace(/currentColor/g, COLOUR)
      .replace(/ data-mml-node="[^"]*"/g, '')
      .replace(/ data-c="[^"]*"/g, '')
      .replace(/ role="img"/g, '')
      .replace(/ focusable="false"/g, '')
      // MathJax emits empty paths for spaces; Qt's SVG parser warns about them.
      .replace(/<path d=""[^>]*\/>/g, '')
      .replace(/<path d=""[^>]*><\/path>/g, '');
    if (!/^<svg/.test(svg))
      throw new Error(`${id}: MathJax did not return an SVG`);
    if (/merror|data-mjx-error/.test(svg))
      throw new Error(`${id}: TeX error in ${tex}`);
    fs.writeFileSync(path.join(outDir, `${id}.svg`),
                     '<?xml version="1.0" encoding="UTF-8"?>\n' + svg + '\n');
    count++;
  }
  console.log(`wrote ${count} formulas to ${outDir}`);
}

main();
