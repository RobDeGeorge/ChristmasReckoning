(function () {
  "use strict";

  const STORAGE_KEY = "christmas-reckoning.magic.v1";
  const DAY = 86400000;
  const ornaments = [
    { id: "golden-star", icon: "⭐", name: "Golden star", description: "A little light for the top of your world. Here’s to the good things still on their way.", x: 50, y: 22 },
    { id: "candy-cane", icon: "🍬", name: "Sugarplum sweet", description: "A sweet reminder: small joys count. Make a little room for one today.", x: 40, y: 36 },
    { id: "winter-mitten", icon: "🧤", name: "Cozy mittens", description: "For warm hands and warmer hearts. Your tree just got a little cozier.", x: 60, y: 36 },
    { id: "jingle-bell", icon: "🔔", name: "Jingle bell", description: "A tiny bell with a big job: making ordinary moments feel like a celebration.", x: 31, y: 50 },
    { id: "snowflake", icon: "❄️", name: "First snowflake", description: "A quiet little wonder, saved just for you. Take a breath and watch the world sparkle.", x: 50, y: 51 },
    { id: "red-bow", icon: "🎀", name: "Velvet bow", description: "No perfect wrapping required. The best things you give are your time and attention.", x: 69, y: 50 },
    { id: "gingerbread", icon: "🍪", name: "Cookie ornament", description: "Fresh from the imaginary oven. May your day have a soft center and a little extra sweetness.", x: 24, y: 64 },
    { id: "winter-candle", icon: "🕯️", name: "Winter candle", description: "A soft glow for slow evenings. You’re allowed to pause and enjoy the waiting.", x: 41, y: 65 },
    { id: "snowman", icon: "☃️", name: "Snow-day friend", description: "A cheerful little companion who thinks today is an excellent day to play.", x: 59, y: 65 },
    { id: "wrapped-gift", icon: "🎁", name: "Pocket present", description: "Some of the best presents fit in a sentence: ‘I thought of you today.’", x: 76, y: 64 },
    { id: "christmas-sock", icon: "🧦", name: "Christmas stocking", description: "A stocking full of possibility. Leave a little space for something unexpected.", x: 34, y: 79 },
    { id: "little-reindeer", icon: "🦌", name: "Starlight reindeer", description: "Your final little friend has arrived. Twelve tiny joys, one tree that’s completely yours.", x: 66, y: 79 }
  ];
  const missions = [
    ["Send a little warmth", "Text someone one specific thing you appreciate about them."],
    ["Make your cozy cup", "Make a favorite warm drink. Enjoy the first few sips without a screen."],
    ["Find your Christmas song", "Play the song that makes it feel like Christmas. Singing along is encouraged."],
    ["Share a good memory", "Tell someone about a holiday moment that still makes you smile."],
    ["A tiny act of kindness", "Do one small helpful thing for someone, just because you can."],
    ["Take the scenic route", "Step outside for a few minutes and notice something beautiful nearby."],
    ["Make a little magic", "Add a little festive touch to your space. Even a paper star counts."],
    ["Give someone your time", "Spend ten unhurried minutes with someone you care about."],
    ["Write a tiny thank-you", "Leave a kind note for someone who makes your days a little easier."],
    ["Make room for wonder", "Look at the sky for a moment. Find a cloud, a star, or a new shade of blue."],
    ["Start a wish list of joy", "Write down three little things you’d love to do this season."],
    ["Bring back a favorite", "Revisit a festive book, movie scene, recipe, or tradition you love."],
    ["Share something sweet", "Share a treat, a recipe, or a favorite silly joke with someone."],
    ["A moment just for you", "Put on something comfortable and take five minutes to simply rest."],
    ["Pass along some cheer", "Send a friend a song, photo, or memory you think they’ll love."],
    ["Make a paper snowflake", "Fold a scrap of paper and create a tiny winter decoration."],
    ["Notice the good", "Write down one ordinary thing that made today a little better."],
    ["Plan a cozy evening", "Choose one simple thing to look forward to tonight."],
    ["A little less hurry", "Take one everyday task slowly. Let yourself enjoy the small details."],
    ["Make someone smile", "Offer a sincere compliment or send a cheerful hello."],
    ["Remember your people", "Think of someone you haven’t caught up with lately. Send a friendly check-in."],
    ["Turn up the twinkle", "Enjoy some fairy lights, a candle’s glow, or the evening sky."],
    ["Give a favorite a new home", "Set aside something useful you no longer need to share or donate."],
    ["Keep a little tradition", "Do one small thing that feels like the holidays to you."]
  ];
  const dailyDelights = [
    "Today’s little delight: imagine your favorite Christmas morning. Keep the best detail with you.",
    "Today’s little delight: put on one song that makes your shoulders relax.",
    "Today’s little delight: you don’t have to make the season perfect to make it lovely.",
    "Today’s little delight: look for the smallest beautiful thing in the room.",
    "Today’s little delight: think of someone who makes you laugh. Send them a little hello.",
    "Today’s little delight: give yourself permission to enjoy the anticipation.",
    "Today’s little delight: a warm drink tastes even better when you slow down.",
    "Today’s little delight: your favorite holiday tradition can be a very small one.",
    "Today’s little delight: revisit one happy memory, just for a moment.",
    "Today’s little delight: there’s still room in the day for something cozy.",
    "Today’s little delight: leave a little kindness wherever today takes you.",
    "Today’s little delight: you bring something to the season that nobody else can."
  ];
  const ornamentIds = new Set(ornaments.map(function (ornament) { return ornament.id; }));
  let state = emptyState();
  let memoryAuthoritative = false;
  let midnightTimer;
  let initialized = false;

  function emptyState() {
    return { version: 1, collection: [], gift: null, visit: { day: null, streak: 0 }, mission: { day: null, completed: false } };
  }

  function object(value) {
    return value !== null && typeof value === "object" && !Array.isArray(value);
  }

  function dayKey(date) {
    return String(date.getFullYear()).padStart(4, "0") + "-" + String(date.getMonth() + 1).padStart(2, "0") + "-" + String(date.getDate()).padStart(2, "0");
  }

  // Compare civil dates, not elapsed hours: a local day may be 23 or 25 hours.
  function dayNumber(key) {
    if (typeof key !== "string" || !/^[1-9]\d{3}-\d{2}-\d{2}$/.test(key)) return null;
    const parts = key.split("-").map(Number);
    const date = new Date(Date.UTC(parts[0], parts[1] - 1, parts[2]));
    if (date.getUTCFullYear() !== parts[0] || date.getUTCMonth() !== parts[1] - 1 || date.getUTCDate() !== parts[2]) return null;
    return date.getTime() / DAY;
  }

  function validDay(value) { return dayNumber(value) !== null; }

  function normalize(value) {
    const clean = emptyState();
    if (!object(value)) return clean;
    if (Array.isArray(value.collection)) {
      clean.collection = Array.from(new Set(value.collection.filter(function (id) { return typeof id === "string" && ornamentIds.has(id); })));
    }
    if (object(value.gift) && validDay(value.gift.day) && ornamentIds.has(value.gift.id)) {
      clean.gift = { day: value.gift.day, id: value.gift.id, isNew: value.gift.isNew === true };
      if (!clean.collection.includes(value.gift.id)) clean.collection.push(value.gift.id);
    }
    if (object(value.visit) && validDay(value.visit.day) && Number.isSafeInteger(value.visit.streak) && value.visit.streak > 0) {
      clean.visit = { day: value.visit.day, streak: Math.min(value.visit.streak, 100000) };
    }
    if (object(value.mission) && validDay(value.mission.day)) {
      clean.mission = { day: value.mission.day, completed: value.mission.completed === true };
    }
    return clean;
  }

  function readState() {
    try {
      const raw = window.localStorage.getItem(STORAGE_KEY);
      return raw === null ? null : normalize(JSON.parse(raw));
    } catch (_) {
      // Private browsing and blocked storage still get a working session.
      return null;
    }
  }

  function saveState() {
    try {
      window.localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
      memoryAuthoritative = false;
    } catch (_) {
      // Readable storage may still contain an older record after a failed write.
      memoryAuthoritative = true;
    }
  }

  function enterDay(today) {
    let changed = false;
    if (state.visit.day !== today) {
      const yesterday = dayNumber(state.visit.day);
      state.visit = { day: today, streak: yesterday !== null && dayNumber(today) - yesterday === 1 ? Math.min(state.visit.streak + 1, 100000) : 1 };
      changed = true;
    }
    if (state.mission.day !== today) {
      state.mission = { day: today, completed: false };
      changed = true;
    }
    if (changed) saveState();
  }

  function byId(id) { return document.getElementById(id); }
  function setText(id, text) { const element = byId(id); if (element) element.textContent = text; }

  function toast(message) {
    if (typeof window.ChristmasToast === "function") window.ChristmasToast(message);
    else setText("toast", message);
  }

  function celebrate(source) {
    window.dispatchEvent(new CustomEvent("christmas:celebrate", { detail: { source: source } }));
  }

  function showDialog(id) {
    const dialog = byId(id);
    if (!dialog || dialog.open) return;
    if (typeof dialog.showModal === "function") dialog.showModal();
    else { dialog.hidden = false; dialog.setAttribute("open", ""); }
  }

  function renderCollection() {
    const tree = byId("tree-ornaments");
    const list = byId("collection-list");
    if (tree) tree.replaceChildren();
    if (list) { list.replaceChildren(); list.setAttribute("role", "list"); }
    ornaments.forEach(function (ornament) {
      const collected = state.collection.includes(ornament.id);
      if (collected && tree) {
        const decoration = document.createElement("span");
        decoration.className = "tree-ornament";
        decoration.textContent = ornament.icon;
        decoration.style.left = ornament.x + "%";
        decoration.style.top = ornament.y + "%";
        decoration.setAttribute("role", "img");
        decoration.setAttribute("aria-label", ornament.name);
        decoration.title = ornament.name;
        tree.appendChild(decoration);
      }
      if (list) {
        const item = document.createElement(list.tagName === "UL" || list.tagName === "OL" ? "li" : "div");
        item.className = "collection-item" + (collected ? " is-collected" : " is-locked");
        item.setAttribute("role", "listitem");
        const icon = document.createElement("span");
        icon.className = "collection-icon";
        icon.textContent = collected ? ornament.icon : "✧";
        icon.setAttribute("aria-hidden", "true");
        const name = document.createElement("span");
        name.textContent = collected ? ornament.name : "A little mystery";
        const status = document.createElement("small");
        status.textContent = collected ? "Collected" : "Waiting to be unwrapped";
        item.append(icon, name, status);
        list.appendChild(item);
      }
    });
    setText("collection-count", state.collection.length + " / " + ornaments.length);
  }

  function render(today) {
    const opened = state.gift !== null && state.gift.day === today;
    const complete = state.collection.length === ornaments.length;
    setText("streak-count", String(state.visit.streak));
    setText("gift-status", opened ? "Today’s gift, unwrapped" : complete ? "A little joy, every day" : "A surprise is waiting");
    setText("gift-open", opened ? "View today’s gift" : complete ? "Open today’s little joy" : "Unwrap today’s gift");
    setText("gift-copy", opened ? "A little joy, saved to your tree. There’s another tomorrow." : complete ? "Your collection is complete. There’s still a fresh little moment to enjoy each day." : "A tiny surprise for your tree. Yours to keep, one day at a time.");
    const mission = missions[((dayNumber(today) % missions.length) + missions.length) % missions.length];
    setText("mission-title", mission[0]);
    setText("mission-description", mission[1]);
    setText("mission-status", state.mission.completed ? "A little kindness makes the season brighter." : "One small thing. A little more Christmas.");
    const button = byId("mission-complete");
    if (button) {
      button.textContent = state.mission.completed ? "Completed · undo" : "I did it";
      button.setAttribute("aria-pressed", String(state.mission.completed));
      button.classList.toggle("is-complete", state.mission.completed);
    }
    renderCollection();
  }

  function refresh(readLatest) {
    if (readLatest !== false && !memoryAuthoritative) {
      const latest = readState();
      if (latest) state = latest;
    }
    const today = dayKey(new Date());
    enterDay(today);
    render(today);
    return today;
  }

  function revealGift() {
    const today = refresh();
    if (state.gift === null || state.gift.day !== today) {
      const next = ornaments.find(function (ornament) { return !state.collection.includes(ornament.id); });
      const gift = next || ornaments[dayNumber(today) % ornaments.length];
      if (next) state.collection.push(gift.id);
      state.gift = { day: today, id: gift.id, isNew: Boolean(next) };
      saveState();
      render(today);
      celebrate("gift");
      toast(next ? gift.name + " added to your tree!" : "A little Christmas joy, just for you.");
    }
    const ornament = ornaments.find(function (item) { return item.id === state.gift.id; });
    setText("gift-icon", ornament.icon);
    setText("gift-name", state.gift.isNew ? ornament.name : "A little Christmas joy");
    setText("gift-description", state.gift.isNew ? ornament.description : dailyDelights[dayNumber(today) % dailyDelights.length]);
    showDialog("gift-dialog");
  }

  function toggleMission() {
    const today = refresh();
    state.mission.completed = !state.mission.completed;
    saveState();
    render(today);
    if (state.mission.completed) {
      celebrate("mission");
      toast("That’s a little more joy in the world. Nicely done.");
    } else toast("Today’s little mission is ready when you are.");
  }

  function scheduleMidnight() {
    window.clearTimeout(midnightTimer);
    const now = new Date();
    const nextDay = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1);
    midnightTimer = window.setTimeout(function () { refresh(); scheduleMidnight(); }, Math.max(100, nextDay.getTime() - now.getTime() + 50));
  }

  function initialize() {
    if (initialized) return;
    initialized = true;
    state = readState() || emptyState();
    refresh(false);
    const giftButton = byId("gift-open");
    const missionButton = byId("mission-complete");
    const collectionButton = byId("collection-open");
    if (giftButton) giftButton.addEventListener("click", revealGift);
    if (missionButton) missionButton.addEventListener("click", toggleMission);
    if (collectionButton) collectionButton.addEventListener("click", function () { refresh(); showDialog("collection-dialog"); });
    scheduleMidnight();
    window.setInterval(function () {
      if (!document.hidden && dayKey(new Date()) !== state.visit.day) { refresh(); scheduleMidnight(); }
    }, 60000);
    document.addEventListener("visibilitychange", function () {
      if (!document.hidden) { refresh(); scheduleMidnight(); }
    });
    window.addEventListener("focus", function () { refresh(); scheduleMidnight(); });
    window.addEventListener("storage", function (event) {
      if (event.key !== STORAGE_KEY && event.key !== null) return;
      if (memoryAuthoritative) return;
      state = readState() || emptyState();
      refresh(false);
    });
  }

  // Read-only snapshots help integrations inspect progress without bypassing the daily rules.
  window.ChristmasMagic = Object.freeze({
    refresh: function () { return refresh(); },
    getState: function () { return JSON.parse(JSON.stringify(state)); }
  });
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", initialize, { once: true });
  else initialize();
})();
