(function () {
  'use strict';
  const $ = (id) => document.getElementById(id);
  const hero = $('countdown');
  if (!hero) return;
  const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
  const preferenceKey = 'christmas-reckoning.preferences.v1';
  const scenes = ['evergreen', 'moonlight', 'fireside'];
  let preferences = { snow: true, scene: 'evergreen' };
  try {
    const saved = JSON.parse(localStorage.getItem(preferenceKey));
    if (saved && typeof saved === 'object') {
      if (typeof saved.snow === 'boolean') preferences.snow = saved.snow;
      if (scenes.includes(saved.scene)) preferences.scene = saved.scene;
    }
  } catch (_) { /* The scene remains usable without browser storage. */ }
  function savePreferences() {
    try { localStorage.setItem(preferenceKey, JSON.stringify(preferences)); } catch (_) { /* Session settings still work. */ }
  }
  let toastTimer;
  window.ChristmasToast = function (message) {
    const toast = $('toast');
    clearTimeout(toastTimer);
    toast.textContent = message;
    toast.classList.add('visible');
    toastTimer = setTimeout(() => toast.classList.remove('visible'), 4000);
  };

  document.querySelectorAll('[data-close-dialog]').forEach((button) => {
    button.addEventListener('click', () => button.closest('dialog').close());
  });
  document.querySelectorAll('dialog').forEach((dialog) => {
    dialog.addEventListener('click', (event) => {
      if (event.target !== dialog) return;
      const bounds = dialog.getBoundingClientRect();
      if (event.clientX < bounds.left || event.clientX > bounds.right || event.clientY < bounds.top || event.clientY > bounds.bottom) dialog.close();
    });
    dialog.addEventListener('close', () => {
      if (!document.querySelector('dialog[open]')) document.documentElement.style.overflow = '';
    });
    new MutationObserver(() => {
      if (dialog.open) document.documentElement.style.overflow = 'hidden';
    }).observe(dialog, { attributes: true, attributeFilter: ['open'] });
  });
  $('how-open').addEventListener('click', () => $('how-dialog').showModal());

  function shareMessage() {
    const state = window.ChristmasCountdown?.getState();
    const lead = state?.isChristmas ? 'Merry Christmas! A little magic, just for you.' : `${state ? Math.ceil(state.remainingMilliseconds / 86400000) : 'A few'} sleeps until Christmas. Let’s make the waiting wonderful!`;
    const local = location.protocol === 'file:' || ['localhost', '127.0.0.1', '[::1]'].includes(location.hostname);
    const url = local ? 'https://www.ChristmasReckoning.com/' : new URL('./', location.href).href;
    return `${lead}\n\nUnwrap a daily surprise at Christmas Reckoning.\n${url}`;
  }
  $('share-open').addEventListener('click', () => {
    $('share-message').value = shareMessage();
    $('share-status').textContent = 'Share the countdown with your favorite Christmas person.';
    $('share-dialog').showModal();
  });
  $('share-copy').addEventListener('click', async () => {
    try {
      if (!navigator.clipboard?.writeText) throw new Error('Clipboard unavailable');
      await navigator.clipboard.writeText($('share-message').value);
      $('share-status').textContent = 'Copied! Go make someone’s day a little merrier.';
    } catch (_) {
      $('share-message').focus();
      $('share-message').select();
      $('share-status').textContent = 'Select and copy this message using your device’s copy command.';
    }
  });

  function renderScene() {
    hero.dataset.scene = preferences.scene;
    const name = preferences.scene[0].toUpperCase() + preferences.scene.slice(1);
    $('scene-label').textContent = name;
    $('scene-toggle').setAttribute('aria-label', `Change village colors, current scene ${name}`);
  }
  $('scene-toggle').addEventListener('click', () => {
    preferences.scene = scenes[(scenes.indexOf(preferences.scene) + 1) % scenes.length];
    renderScene();
    savePreferences();
  });
  renderScene();

  $('fullscreen-toggle').addEventListener('click', async () => {
    try {
      if (document.fullscreenElement) await document.exitFullscreen();
      else if (hero.requestFullscreen) await hero.requestFullscreen();
      else window.ChristmasToast('Fullscreen isn’t available in this browser. The village is still yours to enjoy.');
    } catch (_) { window.ChristmasToast('Your browser couldn’t enter fullscreen this time.'); }
  });
  document.addEventListener('fullscreenchange', () => {
    $('fullscreen-toggle').setAttribute('aria-label', document.fullscreenElement ? 'Exit fullscreen countdown' : 'Enter fullscreen countdown');
  });

  // A light snowfall, drawn only while its scene is visible, at no more than 30 fps.
  const canvas = $('snow-canvas');
  const context = canvas.getContext('2d');
  let width = 0;
  let height = 0;
  let flakes = [];
  let snowFrame = 0;
  let lastSnowTime = 0;
  let heroVisible = true;
  function makeFlake(randomY) {
    return { x: Math.random() * width, y: randomY ? Math.random() * height : -8, radius: Math.random() * 1.55 + 0.6, speed: Math.random() * 17 + 10, drift: Math.random() * 8 - 4, opacity: Math.random() * 0.4 + 0.15 };
  }
  function resizeSnow() {
    width = hero.clientWidth;
    height = hero.clientHeight;
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    canvas.width = width * ratio;
    canvas.height = height * ratio;
    context?.setTransform(ratio, 0, 0, ratio, 0, 0);
    flakes = Array.from({ length: width < 600 ? 34 : 70 }, () => makeFlake(true));
  }
  function snowfall(time) {
    snowFrame = requestAnimationFrame(snowfall);
    if (time - lastSnowTime < 1000 / 30) return;
    const elapsed = lastSnowTime ? Math.min((time - lastSnowTime) / 1000, 0.1) : 0.033;
    lastSnowTime = time;
    context.clearRect(0, 0, width, height);
    flakes.forEach((flake, index) => {
      flake.y += flake.speed * elapsed;
      flake.x += (flake.drift + Math.sin(time / 3500 + index) * 3) * elapsed;
      if (flake.y > height + 6) { flakes[index] = makeFlake(false); return; }
      if (flake.x > width + 6) flake.x = -5;
      if (flake.x < -6) flake.x = width + 5;
      context.beginPath();
      context.arc(flake.x, flake.y, flake.radius, 0, Math.PI * 2);
      context.fillStyle = `rgba(250,249,230,${flake.opacity})`;
      context.fill();
    });
  }
  function updateSnow() {
    const enabled = preferences.snow && !reducedMotion.matches;
    $('snow-toggle').setAttribute('aria-pressed', String(enabled));
    $('snow-toggle').querySelector('span').textContent = reducedMotion.matches ? 'Motion off' : enabled ? 'Snow on' : 'Snow off';
    $('snow-toggle').title = reducedMotion.matches ? 'Snow is paused to respect your reduced-motion setting' : 'Toggle snowfall';
    canvas.hidden = !enabled;
    cancelAnimationFrame(snowFrame);
    snowFrame = 0;
    lastSnowTime = 0;
    if (context && enabled && heroVisible && !document.hidden) snowFrame = requestAnimationFrame(snowfall);
  }
  $('snow-toggle').addEventListener('click', () => {
    if (reducedMotion.matches) {
      window.ChristmasToast('Snow is paused because your device prefers reduced motion.');
      return;
    }
    preferences.snow = !preferences.snow;
    savePreferences();
    updateSnow();
  });
  resizeSnow();
  new ResizeObserver(resizeSnow).observe(hero);
  new IntersectionObserver(([entry]) => { heroVisible = entry.isIntersecting; updateSnow(); }, { threshold: 0 }).observe(hero);
  reducedMotion.addEventListener('change', updateSnow);
  updateSnow();

  // Soft original bell tones, synthesized locally. Audio never starts on its own.
  let audioContext = null;
  let soundEnabled = false;
  let chimeTimer = null;
  let noteIndex = 0;
  const chimeNotes = [523.25, 659.25, 783.99, 987.77, 783.99, 659.25, 587.33, 783.99, 880, 659.25, 523.25, 587.33];
  function chime() {
    if (!audioContext || !soundEnabled || document.hidden) return;
    const now = audioContext.currentTime;
    const frequency = chimeNotes[noteIndex++ % chimeNotes.length];
    [1, 2, 3].forEach((harmonic) => {
      const oscillator = audioContext.createOscillator();
      const gain = audioContext.createGain();
      oscillator.type = 'sine';
      oscillator.frequency.value = frequency * harmonic;
      gain.gain.setValueAtTime(0, now);
      gain.gain.linearRampToValueAtTime(0.055 / (harmonic * harmonic), now + 0.025);
      gain.gain.exponentialRampToValueAtTime(0.0001, now + 3.6);
      oscillator.connect(gain);
      gain.connect(audioContext.destination);
      oscillator.start(now);
      oscillator.stop(now + 3.7);
      oscillator.onended = () => { oscillator.disconnect(); gain.disconnect(); };
    });
  }
  function soundLabel() {
    $('sound-toggle').setAttribute('aria-pressed', String(soundEnabled));
    $('sound-toggle').querySelector('span').textContent = soundEnabled ? 'Sound on' : 'Sound off';
  }
  function startChimes() {
    clearInterval(chimeTimer);
    chime();
    chimeTimer = setInterval(chime, 2500);
  }
  $('sound-toggle').addEventListener('click', async () => {
    try {
      const Audio = window.AudioContext || window.webkitAudioContext;
      if (!Audio) throw new Error('Web Audio unavailable');
      if (!audioContext) audioContext = new Audio();
      soundEnabled = !soundEnabled;
      if (soundEnabled) {
        await audioContext.resume();
        if (soundEnabled) startChimes();
      } else {
        clearInterval(chimeTimer);
        await audioContext.suspend();
      }
      soundLabel();
    } catch (_) {
      soundEnabled = false;
      clearInterval(chimeTimer);
      soundLabel();
      window.ChristmasToast('Cozy sounds aren’t available in this browser.');
    }
  });
  document.addEventListener('visibilitychange', () => {
    updateSnow();
    if (!audioContext || !soundEnabled) return;
    if (document.hidden) {
      clearInterval(chimeTimer);
      audioContext.suspend().catch(() => {});
    } else {
      audioContext.resume().then(() => { if (soundEnabled) startChimes(); }).catch(() => {
        soundEnabled = false;
        soundLabel();
      });
    }
  });
  window.addEventListener('pagehide', () => {
    cancelAnimationFrame(snowFrame);
    clearInterval(chimeTimer);
    if (audioContext) audioContext.suspend().catch(() => {});
  });
  window.addEventListener('pageshow', (event) => {
    if (!event.persisted) return;
    updateSnow();
    if (audioContext && soundEnabled) audioContext.resume().then(startChimes).catch(() => {});
  });

  let confettiTimer;
  window.addEventListener('christmas:celebrate', () => {
    if (reducedMotion.matches) return;
    const container = $('confetti');
    container.replaceChildren();
    clearTimeout(confettiTimer);
    const colors = ['#bf5a46', '#dbb36b', '#86a17b', '#efe2b3'];
    for (let i = 0; i < 32; i++) {
      const piece = document.createElement('span');
      piece.className = 'confetti-piece';
      piece.style.background = colors[i % colors.length];
      piece.style.setProperty('--x', `${(Math.random() - 0.5) * Math.min(innerWidth, 750)}px`);
      piece.style.setProperty('--y', `${Math.random() * 480 - 270}px`);
      piece.style.setProperty('--rotation', `${Math.random() * 650 - 325}deg`);
      container.append(piece);
    }
    confettiTimer = setTimeout(() => container.replaceChildren(), 1600);
  });
  const navLinks = document.querySelectorAll('.main-nav a');
  const sectionObserver = new IntersectionObserver((entries) => {
    for (const entry of entries) {
      if (!entry.isIntersecting) continue;
      navLinks.forEach((link) => link.classList.toggle('active', link.hash === '#' + entry.target.id));
    }
  }, { rootMargin: '-10% 0px -60% 0px' });
  ['countdown', 'daily-magic', 'play'].forEach((id) => sectionObserver.observe($(id)));
})();
