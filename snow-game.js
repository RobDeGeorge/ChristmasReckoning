(() => {
  'use strict';

  function initFlurry() {
    const dialog = document.getElementById('game-dialog');
    const mount = document.getElementById('game-mount');
    const openButton = document.getElementById('game-open');
    if (!dialog || !mount || !openButton || mount.dataset.flurryMounted) return;
    mount.dataset.flurryMounted = 'true';

    const ROUND_MS = 30000;
    const GOLD_AFTER_MS = 300;
    const FLAKE_MS = 1800;
    const BEST_KEY = 'christmas-reckoning.flurry-best.v1';
    let best = 0;
    try {
      const saved = Number(localStorage.getItem(BEST_KEY));
      if (Number.isInteger(saved) && saved >= 0 && saved <= 1000) best = saved;
    } catch { /* The game also works when browser storage is unavailable. */ }

    mount.innerHTML = `
      <div class="flurry-game">
        <p class="flurry-description">A tiny snow day, just for you. Catch the snowflakes when their rings turn gold.</p>
        <div class="flurry-stats" aria-label="Round statistics">
          <div><span class="flurry-stat-label">Caught</span><span class="flurry-stat-number" data-flurry-score>0</span></div>
          <div><span class="flurry-stat-label">Seconds left</span><span class="flurry-stat-number" data-flurry-time role="timer" aria-live="off">30</span></div>
          <div><span class="flurry-stat-label">Your best</span><span class="flurry-stat-number" data-flurry-best>${best}</span></div>
        </div>
        <div class="flurry-timeline" aria-hidden="true"><span data-flurry-progress></span></div>
        <div class="flurry-field" data-flurry-field data-state="idle" tabindex="0" role="group" aria-label="Snowfield. Press Space when the snowflake ring turns gold.">
          <span class="flurry-scenery flurry-scenery-one" aria-hidden="true">✦</span>
          <span class="flurry-scenery flurry-scenery-two" aria-hidden="true">✧</span>
          <span class="flurry-scenery flurry-scenery-three" aria-hidden="true">✦</span>
          <span class="flurry-hill flurry-hill-back" aria-hidden="true"></span>
          <span class="flurry-hill flurry-hill-front" aria-hidden="true"></span>
          <button class="flurry-snowflake" type="button" data-flurry-target aria-label="Snowflake. Wait for the gold ring." hidden><span aria-hidden="true">❄︎</span></button>
          <div class="flurry-overlay" data-flurry-overlay>
            <span class="flurry-intro-icon" aria-hidden="true">❄︎</span>
            <h3 class="flurry-result-title" data-flurry-title>Make a little flurry.</h3>
            <p class="flurry-result-copy" data-flurry-copy>30 seconds. A handful of snowflakes. All the festive feels.</p>
            <button class="flurry-start" type="button" data-flurry-start>Let it snow <span aria-hidden="true">↗</span></button>
          </div>
          <span class="flurry-hint" data-flurry-hint hidden aria-hidden="true">Wait for gold…</span>
        </div>
        <p class="flurry-instructions"><span aria-hidden="true">✧</span> Tap a gold snowflake, or focus the snowfield and press <kbd>Space</kbd>. One press per catch.</p>
        <p class="flurry-status" data-flurry-status role="status" aria-live="polite" aria-atomic="true">Ready whenever you are.</p>
      </div>`;

    const get = (name) => mount.querySelector(`[data-flurry-${name}]`);
    const field = get('field');
    const target = get('target');
    const overlay = get('overlay');
    const startButton = get('start');
    const title = get('title');
    const copy = get('copy');
    const scoreText = get('score');
    const timeText = get('time');
    const bestText = get('best');
    const progress = get('progress');
    const hint = get('hint');
    const status = get('status');
    let playing = false;
    let score = 0;
    let streak = 0;
    let startTime = 0;
    let flakeTime = 0;
    let ready = false;
    let frame = null;
    let shownSecond = 30;
    let announcedTen = false;
    let previousPosition = { x: -100, y: -100 };

    function spawnFlake(now) {
      flakeTime = now;
      ready = false;
      target.classList.remove('flurry-ready');
      target.setAttribute('aria-label', 'Snowflake. Wait for the gold ring.');
      hint.textContent = 'Wait for gold…';
      const bounds = field.getBoundingClientRect();
      const inset = 48;
      let x = bounds.width / 2;
      let y = bounds.height / 2;
      for (let attempt = 0; attempt < 8; attempt += 1) {
        x = inset + Math.random() * Math.max(0, bounds.width - inset * 2);
        y = inset + Math.random() * Math.max(0, bounds.height - inset * 2 - 28);
        if (Math.hypot(x - previousPosition.x, y - previousPosition.y) > 90) break;
      }
      previousPosition = { x, y };
      target.style.left = `${x}px`;
      target.style.top = `${y}px`;
      target.hidden = false;
    }

    function finish(reason = 'complete') {
      if (!playing) return;
      playing = false;
      cancelAnimationFrame(frame);
      frame = null;
      target.hidden = true;
      hint.hidden = true;
      field.dataset.state = 'done';
      overlay.hidden = false;
      const newBest = score > best;
      if (newBest) {
        best = score;
        bestText.textContent = String(best);
        try { localStorage.setItem(BEST_KEY, String(best)); } catch { /* Optional persistence. */ }
      }
      title.textContent = newBest ? 'A new snowy best!' : score > 0 ? 'A pocketful of winter.' : 'There’s always another snow day.';
      const caught = `${score} snowflake${score === 1 ? '' : 's'} caught.`;
      copy.textContent = reason === 'hidden'
        ? `${caught} Your round ended when you left the tab. Ready for a fresh flurry?`
        : reason === 'closed'
          ? `${caught} Your round ended when you closed the snowfield.`
          : `${caught} ${newBest ? 'Your own little winter record.' : 'Take a breath. Enjoy the glow.'}`;
      startButton.innerHTML = 'Another snow day <span aria-hidden="true">↗</span>';
      status.textContent = `${caught}${newBest ? ' A new personal best!' : ''} Round finished.`;
      if (reason === 'complete') {
        timeText.textContent = '0';
        progress.style.transform = 'scaleX(0)';
        document.dispatchEvent(new CustomEvent('christmas:flurry-complete', { detail: { score, best } }));
      }
      if (dialog.open && reason !== 'hidden') startButton.focus({ preventScroll: true });
    }

    function tick(now) {
      if (!playing) return;
      if (!dialog.open) return finish('closed');
      if (document.hidden) return finish('hidden');
      const remaining = Math.max(0, ROUND_MS - (now - startTime));
      if (remaining === 0) return finish();
      const second = Math.ceil(remaining / 1000);
      if (second !== shownSecond) {
        shownSecond = second;
        timeText.textContent = String(second);
      }
      progress.style.transform = `scaleX(${remaining / ROUND_MS})`;
      if (remaining <= 10000 && !announcedTen) {
        announcedTen = true;
        status.textContent = `10 seconds left. ${score} snowflakes caught.`;
      }
      if (now - flakeTime >= FLAKE_MS) {
        streak = 0;
        spawnFlake(now);
      } else if (!ready && now - flakeTime >= GOLD_AFTER_MS) {
        ready = true;
        target.classList.add('flurry-ready');
        target.setAttribute('aria-label', 'Gold snowflake. Catch it now!');
        hint.textContent = 'Gold! Catch it ✦';
      }
      frame = requestAnimationFrame(tick);
    }

    function catchFlake() {
      if (!playing) return;
      const now = performance.now();
      if (now - startTime >= ROUND_MS) return finish();
      // Check elapsed time here too, so clicks cannot catch a stale frame.
      if (now - flakeTime >= FLAKE_MS) {
        streak = 0;
        spawnFlake(now);
        return;
      }
      if (now - flakeTime < GOLD_AFTER_MS) {
        hint.textContent = 'Almost… wait for gold!';
        status.textContent = 'Almost. Wait for the snowflake ring to turn gold.';
        return;
      }
      score += 1;
      streak += 1;
      scoreText.textContent = String(score);
      status.textContent = streak >= 3 ? `${score} caught. ${streak} in a row!` : `${score} snowflake${score === 1 ? '' : 's'} caught.`;
      spawnFlake(now);
    }

    startButton.addEventListener('click', () => {
      if (playing || !dialog.open || document.hidden) return;
      playing = true;
      score = 0;
      streak = 0;
      shownSecond = 30;
      announcedTen = false;
      scoreText.textContent = '0';
      timeText.textContent = '30';
      progress.style.transform = 'scaleX(1)';
      overlay.hidden = true;
      hint.hidden = false;
      field.dataset.state = 'playing';
      status.textContent = 'Snow day started. Catch each snowflake when its ring turns gold.';
      startTime = performance.now();
      spawnFlake(startTime);
      field.focus({ preventScroll: true });
      frame = requestAnimationFrame(tick);
    });

    target.addEventListener('click', catchFlake);
    field.addEventListener('keydown', (event) => {
      // Native button keyboard behavior is preserved for the target and Start.
      if (event.target !== field || event.code !== 'Space' || !playing) return;
      event.preventDefault();
      if (!event.repeat) catchFlake();
    });
    openButton.addEventListener('click', () => {
      if (!dialog.open) dialog.showModal();
      startButton.focus({ preventScroll: true });
    });
    dialog.addEventListener('close', () => finish('closed'));
    document.addEventListener('visibilitychange', () => {
      if (document.hidden) finish('hidden');
    });
    window.addEventListener('pagehide', () => finish('hidden'));
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initFlurry, { once: true });
  } else {
    initFlurry();
  }
})();
