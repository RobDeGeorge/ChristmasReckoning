// SPDX-License-Identifier: GPL-3.0-or-later
// Boots the Qt for WebAssembly app.
//
// The .wasm is shipped precompressed (brotli and gzip) because static hosts like GitHub Pages don't
// reliably compress it. We stream the smallest variant this browser can decode through
// DecompressionStream, count the bytes for a real progress bar, compile while downloading, and hand
// the compiled module to Qt's stock qtloader.js via its documented `qt.module` option.
(function () {
  'use strict';

  const build = window.CR_BUILD;
  const root = document.documentElement;
  const loader = document.getElementById('loader');
  const bar = document.getElementById('loader-bar');
  const statusText = document.getElementById('loader-status');
  const retry = document.getElementById('loader-retry');

  function setStatus(text) {
    statusText.textContent = text;
  }

  function setProgress(fraction) {
    const clamped = Math.max(0.02, Math.min(1, fraction));
    bar.style.transform = 'scaleX(' + clamped.toFixed(4) + ')';
    loader.setAttribute('aria-valuenow', String(Math.round(clamped * 100)));
  }

  function showProblem(message, canRetry) {
    root.classList.add('no-app');
    setStatus(message);
    if (canRetry) {
      retry.hidden = false;
      retry.onclick = function () { location.reload(); };
    }
  }

  function supportsWebGL2() {
    try {
      return !!document.createElement('canvas').getContext('webgl2');
    } catch (error) {
      return false;
    }
  }

  function canDecompress(format) {
    if (typeof DecompressionStream !== 'function')
      return false;
    try {
      new DecompressionStream(format);
      return true;
    } catch (error) {
      return false;
    }
  }

  function isWasmMagic(bytes) {
    return bytes.length >= 4 && bytes[0] === 0x00 && bytes[1] === 0x61 && bytes[2] === 0x73 && bytes[3] === 0x6d;
  }

  async function compileFrom(response, candidate) {
    const reader = response.body.getReader();
    const first = await reader.read();
    const head = first.done ? new Uint8Array(0) : first.value;
    // If the server already decoded the payload for us, it starts with the wasm magic number.
    const format = isWasmMagic(head) ? null : candidate.format;
    const total = format ? candidate.bytes : build.wasmBytes;
    let loaded = 0;

    const counted = new ReadableStream({
      start(controller) {
        if (head.length) {
          loaded += head.length;
          setProgress(loaded / total);
          controller.enqueue(head);
        }
        if (first.done)
          controller.close();
      },
      async pull(controller) {
        const chunk = await reader.read();
        if (chunk.done) {
          controller.close();
          return;
        }
        loaded += chunk.value.length;
        setProgress(loaded / total);
        controller.enqueue(chunk.value);
      },
      cancel(reason) {
        return reader.cancel(reason);
      }
    });

    const body = format ? counted.pipeThrough(new DecompressionStream(format)) : counted;
    const wasm = new Response(body, { headers: { 'Content-Type': 'application/wasm' } });
    if (typeof WebAssembly.compileStreaming === 'function')
      return WebAssembly.compileStreaming(wasm);
    return WebAssembly.compile(await wasm.arrayBuffer());
  }

  async function fetchModule() {
    const candidates = [];
    if (build.wasmBr && canDecompress('brotli'))
      candidates.push({ url: build.wasmBr, format: 'brotli', bytes: build.wasmBrBytes });
    if (build.wasmGz && canDecompress('gzip'))
      candidates.push({ url: build.wasmGz, format: 'gzip', bytes: build.wasmGzBytes });
    candidates.push({ url: build.wasm, format: null, bytes: build.wasmBytes });

    let lastError = null;
    for (const candidate of candidates) {
      try {
        // Reuse the download index.html started while the scripts were still loading.
        const early = build.early && build.early.candidate.url === candidate.url ? build.early.response : null;
        build.early = null;
        const response = await (early || fetch(candidate.url, { credentials: 'same-origin' }));
        if (!response.ok || !response.body)
          throw new Error('HTTP ' + response.status + ' for ' + candidate.url);
        const module = await compileFrom(response, candidate);
        window.crLoadInfo = { url: candidate.url, format: candidate.format };
        return module;
      } catch (error) {
        console.warn('Could not load', candidate.url, error);
        lastError = error;
      }
    }
    throw lastError;
  }

  async function start() {
    if (typeof WebAssembly !== 'object' || !supportsWebGL2()) {
      showProblem('This browser can’t run the interactive site (it needs WebAssembly and WebGL 2). Here’s the summary instead.', false);
      return;
    }
    if (typeof window.qtLoad !== 'function' || typeof window[build.entry] !== 'function') {
      showProblem('Part of the app didn’t download. Check your connection and try again.', true);
      return;
    }

    window.crOnAppReady = function () {
      root.classList.add('app-ready');
      loader.setAttribute('aria-hidden', 'true');
    };
    if (window.crAppReady)
      window.crOnAppReady();

    const module = fetchModule();
    module.then(function () {
      setProgress(1);
      setStatus('Warming up the sleigh…');
    }, function () {});

    try {
      const instance = await window.qtLoad({
        qt: {
          module: module,
          entryFunction: window[build.entry],
          containerElements: [document.getElementById('screen')],
          onExit: function (exit) {
            if (exit.crashed || (exit.code !== undefined && exit.code !== 0)) {
              root.classList.remove('app-ready');
              showProblem('Mission Control hit turbulence and stopped. Reloading usually fixes it.', true);
              console.error('Application exit', exit);
            }
          }
        }
      });
      window.crInstance = instance;
      window.addEventListener('hashchange', function () {
        instance.crOnHashChange(location.hash);
      });
    } catch (error) {
      console.error(error);
      showProblem('Mission Control couldn’t start. Check your connection and try again.', true);
    }
  }

  start();
})();
