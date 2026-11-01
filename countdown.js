(function (root) {
  'use strict';

  /** Count real elapsed seconds to Christmas midnight in the visitor's timezone. */
  function getChristmasCountdown(now = new Date()) {
    if (!(now instanceof Date) || !Number.isFinite(now.getTime())) {
      throw new TypeError('The countdown needs a valid Date.');
    }

    const isChristmas = now.getMonth() === 11 && now.getDate() === 25;
    const year = now.getFullYear() + (now.getMonth() === 11 && now.getDate() > 25 ? 1 : 0);
    const targetDate = new Date(year, 11, 25);
    const remainingMilliseconds = isChristmas ? 0 : Math.max(0, targetDate.getTime() - now.getTime());
    // Round up so the countdown doesn't announce Christmas before midnight.
    const totalSeconds = Math.ceil(remainingMilliseconds / 1000);

    return {
      year,
      targetDate,
      isChristmas,
      remainingMilliseconds,
      totalSeconds,
      days: Math.floor(totalSeconds / 86400),
      hours: Math.floor(totalSeconds / 3600) % 24,
      minutes: Math.floor(totalSeconds / 60) % 60,
      seconds: totalSeconds % 60
    };
  }

  if (typeof module !== 'undefined' && module.exports) {
    module.exports = { getChristmasCountdown };
  }

  if (!root || !root.document) return;
  root.ChristmasCountdown = { getState: getChristmasCountdown };

  function startCountdown() {
    const document = root.document;
    const elements = {};
    [
      'days', 'hours', 'minutes', 'seconds', 'countdown-date', 'countdown-note',
      'christmas-year', 'countdown-accessible', 'hero-eyebrow', 'hero-title'
    ].forEach(function (id) {
      elements[id] = document.getElementById(id);
    });

    const originalHeadline = elements['hero-title'] ? elements['hero-title'].innerHTML : '';
    const originalEyebrow = elements['hero-eyebrow'] ? elements['hero-eyebrow'].textContent : '';
    let wasChristmas = false;
    let timer;

    function setText(id, value) {
      const element = elements[id];
      const text = String(value);
      if (element && element.textContent !== text) element.textContent = text;
    }

    function render() {
      const state = getChristmasCountdown();
      setText('days', state.days);
      ['hours', 'minutes', 'seconds'].forEach(function (unit) {
        setText(unit, String(state[unit]).padStart(2, '0'));
      });
      setText('countdown-date', 'December 25, ' + state.year);
      setText('christmas-year', state.year);
      setText('countdown-note', state.isChristmas
        ? 'The day is here. Make it a magical one.'
        : 'Until December 25, ' + state.year + ' · your local time');
      setText('countdown-accessible', state.isChristmas
        ? 'It is Christmas Day. Merry Christmas!'
        : state.days + ' days, ' + state.hours + ' hours, ' + state.minutes + ' minutes, and ' +
          state.seconds + ' seconds until Christmas ' + state.year + '.');

      if (state.isChristmas) {
        setText('hero-eyebrow', 'THE MOST WONDERFUL DAY OF THE YEAR');
        setText('hero-title', 'Merry Christmas!');
      } else if (wasChristmas) {
        setText('hero-eyebrow', originalEyebrow);
        if (elements['hero-title']) elements['hero-title'].innerHTML = originalHeadline;
      }
      wasChristmas = state.isChristmas;
      document.documentElement.classList.toggle('is-christmas', state.isChristmas);

      const title = state.isChristmas ? 'Merry Christmas! · Christmas Reckoning'
        : (state.days ? state.days + (state.days === 1 ? ' day' : ' days')
          : state.hours + 'h ' + state.minutes + 'm') + ' until Christmas · Christmas Reckoning';
      if (document.title !== title) document.title = title;
    }

    function tick() {
      root.clearTimeout(timer);
      render();
      // Align with the next second and recalculate from the clock on every tick.
      if (!document.hidden) timer = root.setTimeout(tick, 1000 - Date.now() % 1000 + 10);
    }

    document.addEventListener('visibilitychange', tick);
    root.addEventListener('pageshow', tick);
    tick();
  }

  if (root.document.readyState === 'loading') {
    root.document.addEventListener('DOMContentLoaded', startCountdown, { once: true });
  } else {
    startCountdown();
  }
})(typeof window === 'undefined' ? null : window);
