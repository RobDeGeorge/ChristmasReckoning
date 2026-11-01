const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

process.env.TZ = "America/New_York";
const script = fs.readFileSync(path.join(__dirname, "..", "festive.js"), "utf8");
const key = "christmas-reckoning.magic.v1";

function browser(options = {}) {
  let now = new Date(options.now || "2026-10-11T12:00:00-04:00").getTime();
  let timerId = 0;
  const timers = new Map();
  const store = options.store || new Map();
  if (Object.hasOwn(options, "raw")) store.set(key, options.raw);
  class Element {
    constructor(tagName = "DIV") {
      this.tagName = tagName.toUpperCase();
      this.children = [];
      this.listeners = {};
      this.attributes = {};
      this.style = {};
      this.textContent = "";
      this.open = false;
      this.classes = new Set();
      this.classList = { toggle: (name, enabled) => enabled ? this.classes.add(name) : this.classes.delete(name) };
    }
    setAttribute(name, value) { this.attributes[name] = value; }
    addEventListener(name, callback) { (this.listeners[name] ||= []).push(callback); }
    dispatch(name, event = {}) { (this.listeners[name] || []).forEach((callback) => callback(event)); }
    appendChild(child) { this.children.push(child); }
    append(...children) { this.children.push(...children); }
    replaceChildren(...children) { this.children = children; }
    showModal() { this.open = true; }
  }
  const ids = ["gift-open", "gift-status", "gift-copy", "streak-count", "collection-count", "tree-ornaments", "collection-list", "collection-open", "collection-dialog", "gift-dialog", "gift-icon", "gift-name", "gift-description", "mission-title", "mission-description", "mission-complete", "mission-status", "toast"];
  const elements = Object.fromEntries(ids.map((id) => [id, new Element(id === "collection-list" ? "UL" : "DIV")]));
  const document = new Element();
  document.readyState = "complete";
  document.hidden = false;
  document.getElementById = (id) => elements[id] || null;
  document.createElement = (tag) => new Element(tag);
  const window = new Element();
  window.localStorage = {
    getItem: (name) => { if (options.blockStorage) throw new Error("Storage blocked"); return store.has(name) ? store.get(name) : null; },
    setItem: (name, value) => { if (options.blockStorage || options.failWrites) throw new Error("Storage write blocked"); store.set(name, value); }
  };
  window.setTimeout = (callback, delay) => { timers.set(++timerId, { callback, delay }); return timerId; };
  window.clearTimeout = (id) => timers.delete(id);
  window.setInterval = () => 1;
  window.dispatchEvent = (event) => window.dispatch(event.type, event);
  class FakeDate extends Date {
    constructor(...args) { super(...(args.length ? args : [now])); }
    static now() { return now; }
  }
  class CustomEvent {
    constructor(type, init) { this.type = type; this.detail = init.detail; }
  }
  vm.runInNewContext(script, { window, document, Date: FakeDate, CustomEvent });
  return {
    elements, window, document, store, timers,
    state: () => JSON.parse(JSON.stringify(window.ChristmasMagic.getState())),
    click: (id) => elements[id].dispatch("click"),
    advance: (value) => { now = new Date(value).getTime(); },
    refresh: () => window.ChristmasMagic.refresh()
  };
}

test("one gift per civil day persists across reloads and can be viewed again", () => {
  const page = browser();
  assert.equal(page.state().visit.streak, 1);
  page.click("gift-open");
  assert.equal(page.state().collection.length, 1);
  assert.equal(page.elements["gift-dialog"].open, true);
  assert.equal(page.elements["tree-ornaments"].children.length, 1);
  page.click("gift-open");
  assert.equal(page.state().collection.length, 1);
  const reloaded = browser({ store: page.store });
  assert.equal(reloaded.state().visit.streak, 1);
  assert.equal(reloaded.elements["gift-open"].textContent, "View today’s gift");
  reloaded.click("gift-open");
  assert.equal(reloaded.elements["gift-name"].textContent, "Golden star");
  assert.equal(reloaded.elements["collection-list"].children.length, 12);
});

test("the first twelve opening days give distinct ornaments, even with skipped days", () => {
  const page = browser({ now: "2026-10-01T12:00:00-04:00" });
  for (let index = 0; index < 12; index += 1) {
    page.advance(new Date(2026, 9, 1 + index * 2, 12));
    page.click("gift-open");
    assert.equal(page.state().collection.length, index + 1);
  }
  assert.equal(new Set(page.state().collection).size, 12);
  assert.equal(page.elements["collection-count"].textContent, "12 / 12");
  page.advance("2026-10-26T12:00:00-04:00");
  page.click("gift-open");
  assert.equal(page.state().collection.length, 12);
  assert.equal(page.elements["gift-name"].textContent, "A little Christmas joy");
  assert.match(page.elements["gift-description"].textContent, /^Today’s little delight:/);
});

test("midnight resets the mission and gift while retaining the collected tree", () => {
  const page = browser({ now: "2026-10-11T23:59:59-04:00" });
  page.click("gift-open");
  page.click("mission-complete");
  assert.equal(page.state().mission.completed, true);
  const scheduled = [...page.timers.values()][0];
  assert.equal(scheduled.delay, 1050);
  page.advance("2026-10-12T00:00:00.050-04:00");
  scheduled.callback();
  assert.equal(page.state().visit.streak, 2);
  assert.equal(page.state().mission.completed, false);
  assert.equal(page.elements["gift-open"].textContent, "Unwrap today’s gift");
  assert.equal(page.elements["tree-ornaments"].children.length, 1);
  page.click("gift-open");
  assert.equal(page.state().collection.length, 2);
});

test("streaks use calendar days across both daylight-saving transitions", () => {
  const spring = browser({ now: "2026-03-08T00:01:00-05:00" });
  spring.advance("2026-03-09T00:01:00-04:00");
  spring.refresh();
  assert.equal(spring.state().visit.streak, 2);
  const fall = browser({ now: "2026-11-01T00:01:00-04:00" });
  fall.advance("2026-11-02T00:01:00-05:00");
  fall.refresh();
  assert.equal(fall.state().visit.streak, 2);
  fall.advance("2026-11-04T12:00:00-05:00");
  fall.refresh();
  assert.equal(fall.state().visit.streak, 1);
});

test("malformed JSON and unexpected valid JSON shapes recover without crashing", () => {
  const samples = ["{broken", "null", "true", "42", "[]", '"text"', JSON.stringify({ collection: [null, {}, "golden-star", "golden-star", "unknown"], gift: { day: "2026-02-31", id: "golden-star" }, visit: { day: "2026-10-11", streak: "99" }, mission: { day: "2026-10-11", completed: "true" } })];
  for (const raw of samples) {
    const page = browser({ raw });
    assert.equal(page.state().visit.streak, 1);
    assert.equal(page.state().mission.completed, false);
    assert.ok(page.state().collection.length <= 1);
    page.click("gift-open");
    assert.ok(page.state().collection.length >= 1);
  }
});

test("progress continues in memory when browser storage is blocked", () => {
  const page = browser({ blockStorage: true });
  page.click("gift-open");
  page.click("mission-complete");
  page.refresh();
  assert.equal(page.state().collection.length, 1);
  assert.equal(page.state().mission.completed, true);
  page.click("gift-open");
  assert.equal(page.state().collection.length, 1);
});

test("mission completion is reversible and persisted for the day", () => {
  const page = browser();
  page.click("mission-complete");
  assert.equal(page.elements["mission-complete"].attributes["aria-pressed"], "true");
  const reloaded = browser({ store: page.store });
  assert.equal(reloaded.state().mission.completed, true);
  reloaded.click("mission-complete");
  assert.equal(reloaded.state().mission.completed, false);
  assert.equal(reloaded.elements["mission-complete"].attributes["aria-pressed"], "false");
});

test("failed writes keep session progress authoritative over readable stale storage", () => {
  const options = {};
  const page = browser(options);
  const persistedBeforeFailure = page.store.get(key);
  options.failWrites = true;
  page.click("gift-open");
  page.click("mission-complete");
  page.refresh();
  assert.equal(page.store.get(key), persistedBeforeFailure);
  assert.equal(page.state().collection.length, 1);
  assert.equal(page.state().mission.completed, true);
  page.window.dispatch("storage", { key });
  page.window.dispatch("storage", { key: null });
  assert.equal(page.state().collection.length, 1);
  assert.equal(page.state().mission.completed, true);
  page.click("gift-open");
  assert.equal(page.state().collection.length, 1);

  // A later successful save persists the whole session and resumes cross-tab reads.
  options.failWrites = false;
  page.click("mission-complete");
  assert.equal(JSON.parse(page.store.get(key)).collection.length, 1);
  const otherTab = browser({ store: page.store });
  otherTab.click("mission-complete");
  page.window.dispatch("storage", { key });
  assert.equal(page.state().mission.completed, true);
});

test("another tab reads the latest daily gift before opening and syncs storage events", () => {
  const store = new Map();
  const first = browser({ store });
  const second = browser({ store });
  first.click("gift-open");
  second.click("gift-open");
  assert.deepEqual(second.state().collection, first.state().collection);
  first.click("mission-complete");
  second.window.dispatch("storage", { key });
  assert.equal(second.state().mission.completed, true);
});

test("the collection opens as a dialog and exposes a detached state snapshot", () => {
  const page = browser();
  page.click("collection-open");
  assert.equal(page.elements["collection-dialog"].open, true);
  const snapshot = page.window.ChristmasMagic.getState();
  snapshot.collection.push("golden-star");
  assert.equal(page.state().collection.length, 0);
});
