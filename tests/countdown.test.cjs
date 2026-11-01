const test = require('node:test');
const assert = require('node:assert/strict');
const { execFileSync } = require('node:child_process');
const { getChristmasCountdown } = require('../countdown.js');

test('Christmas Eve counts down to midnight of the current year', () => {
  const state = getChristmasCountdown(new Date(2026, 11, 24, 0, 0, 0));
  assert.equal(state.year, 2026);
  assert.equal(state.days, 1);
  assert.equal(state.hours, 0);
  assert.equal(state.totalSeconds, 86400);
  assert.equal(state.isChristmas, false);
});

test('a fraction of a second before Christmas still has one second remaining', () => {
  const state = getChristmasCountdown(new Date(2026, 11, 24, 23, 59, 59, 999));
  assert.equal(state.totalSeconds, 1);
  assert.equal(state.seconds, 1);
  assert.equal(state.isChristmas, false);
});

test('the entire local Christmas Day is a zero countdown celebration', () => {
  for (const hour of [0, 12, 23]) {
    const state = getChristmasCountdown(new Date(2026, 11, 25, hour, 59, 59, 999));
    assert.equal(state.isChristmas, true);
    assert.equal(state.year, 2026);
    assert.equal(state.totalSeconds, 0);
    assert.deepEqual([state.days, state.hours, state.minutes, state.seconds], [0, 0, 0, 0]);
  }
});

test('December 26 rolls forward to the next Christmas', () => {
  const state = getChristmasCountdown(new Date(2026, 11, 26));
  assert.equal(state.year, 2027);
  assert.equal(state.targetDate.getFullYear(), 2027);
  assert.equal(state.isChristmas, false);
  assert.equal(state.days, 364);
});

test('leap years include February 29 when it is ahead', () => {
  assert.equal(getChristmasCountdown(new Date(2023, 11, 26)).days, 365);
  assert.equal(getChristmasCountdown(new Date(2024, 11, 26)).days, 364);
});

test('invalid dates are rejected instead of rendering NaN', () => {
  assert.throws(() => getChristmasCountdown(new Date('invalid')), TypeError);
});

function inTimezone(timezone, assertions) {
  const modulePath = JSON.stringify(require.resolve('../countdown.js'));
  execFileSync(process.execPath, ['-e', `
    const assert = require('node:assert/strict');
    const { getChristmasCountdown } = require(${modulePath});
    ${assertions}
  `], { env: { ...process.env, TZ: timezone } });
}

test('uses the local Christmas date even when UTC is on a different day', () => {
  inTimezone('America/Phoenix', `
    const eve = getChristmasCountdown(new Date('2026-12-25T06:59:59Z'));
    assert.equal(eve.isChristmas, false);
    assert.equal(eve.totalSeconds, 1);
    const christmas = getChristmasCountdown(new Date('2026-12-26T06:59:59Z'));
    assert.equal(christmas.isChristmas, true);
    const next = getChristmasCountdown(new Date('2026-12-26T07:00:00Z'));
    assert.equal(next.year, 2027);
  `);
  inTimezone('Pacific/Auckland', `
    const state = getChristmasCountdown(new Date('2026-12-24T11:00:00Z'));
    assert.equal(state.isChristmas, true);
  `);
});

test('accounts for the actual extra elapsed hour when daylight saving time ends', () => {
  inTimezone('America/New_York', `
    const state = getChristmasCountdown(new Date(2026, 9, 31));
    assert.equal(state.days, 55);
    assert.equal(state.hours, 1);
    assert.equal(state.totalSeconds, 55 * 86400 + 3600);
  `);
  inTimezone('America/Phoenix', `
    const state = getChristmasCountdown(new Date(2026, 9, 31));
    assert.equal(state.days, 55);
    assert.equal(state.hours, 0);
  `);
});
