// SPDX-License-Identifier: GPL-3.0-or-later
// Small display helpers for QML (the physics numbers themselves are formatted in C++).
.pragma library

const MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
const SUPERSCRIPTS = "⁰¹²³⁴⁵⁶⁷⁸⁹";

function pad2(n) {
    n = Math.floor(Math.abs(n));
    return (n < 10 ? "0" : "") + n;
}

function superscript(n) {
    const s = String(Math.abs(n)).split("").map(c => SUPERSCRIPTS[+c]).join("");
    return (n < 0 ? "⁻" : "") + s;
}

// 2 significant figures by default; ×10ⁿ outside 0.001…1,000,000.
function num(v, sig) {
    sig = sig || 2;
    if (!isFinite(v))
        return isNaN(v) ? "—" : (v > 0 ? "∞" : "−∞");
    if (v === 0)
        return "0";
    const a = Math.abs(v);
    if (a >= 1e6 || a < 1e-3) {
        let e = Math.floor(Math.log10(a));
        let m = v / Math.pow(10, e);
        let text = m.toFixed(sig - 1);
        if (Math.abs(parseFloat(text)) >= 10) {
            e += 1;
            text = (v / Math.pow(10, e)).toFixed(sig - 1);
        }
        return text.replace("-", "−") + "×10" + superscript(e);
    }
    const magnitude = Math.floor(Math.log10(a));
    const decimals = Math.max(0, sig - 1 - magnitude);
    const rounded = Number(v.toPrecision(sig));
    return rounded.toLocaleString("en-US", { minimumFractionDigits: decimals, maximumFractionDigits: decimals })
        .replace("-", "−");
}

function grouped(v) {
    return Math.round(v).toLocaleString("en-US");
}

function duration(s) {
    const a = Math.abs(s);
    if (s === 0) return "0 s";
    if (a < 1e-6) return num(s * 1e9) + " ns";
    if (a < 1e-3) return num(s * 1e6) + " µs";
    if (a < 1) return num(s * 1e3) + " ms";
    if (a < 120) return num(s, 2) + " s";
    if (a < 7200) return num(s / 60) + " min";
    if (a < 172800) return num(s / 3600) + " h";
    return num(s / 86400) + " days";
}

function utcClock(ms) {
    const d = new Date(ms);
    return pad2(d.getUTCHours()) + ":" + pad2(d.getUTCMinutes());
}

function utcDayClock(ms) {
    const d = new Date(ms);
    return d.getUTCDate() + " " + MONTHS[d.getUTCMonth()] + " " + utcClock(ms) + " UTC";
}

function clockOfDay(seconds) {
    let s = ((seconds % 86400) + 86400) % 86400;
    return pad2(s / 3600) + ":" + pad2((s % 3600) / 60);
}

// Mission elapsed time: T+05:12:33 / T−02:00:00
function met(seconds) {
    const sign = seconds < 0 ? "T−" : "T+";
    let s = Math.floor(Math.abs(seconds));
    const h = Math.floor(s / 3600);
    s -= h * 3600;
    return sign + pad2(h) + ":" + pad2(s / 60) + ":" + pad2(s % 60);
}

// Countdown to an instant: "3d 04:12:33" (or "04:12:33" inside a day)
function countdown(ms) {
    let s = Math.max(0, Math.ceil(ms / 1000));
    const d = Math.floor(s / 86400);
    s -= d * 86400;
    const hms = pad2(s / 3600) + ":" + pad2((s % 3600) / 60) + ":" + pad2(s % 60);
    return d > 0 ? d + "d " + hms : hms;
}

// Display a scenario parameter value with its unit.
function param(value, unit) {
    switch (unit) {
    case "%": return num(value, 3) + "%";
    case "clock": return clockOfDay(value * 3600);
    case "h": return num(value, 3) + " h";
    case "s": return value === 0 ? "0 (drop from the sky)" : duration(value);
    case "m": return value >= 10000 ? num(value / 1000, 3) + " km" : num(value, 3) + " m";
    case "kg": return num(value, 3) + " kg";
    case "g": return num(value, 3) + " g";
    case "mL": return num(value, 3) + " mL";
    case "×": return num(value, 3) + "×";
    case "": return num(value, 3);
    default: return num(value, 3) + " " + unit;
    }
}

function hoursLength(value) {
    return num(value, 3) + " h";
}

// --- Calendar (.ics) ---------------------------------------------------------------------------
function icsDate(ms) {
    const d = new Date(ms);
    return d.getUTCFullYear() + pad2(d.getUTCMonth() + 1) + pad2(d.getUTCDate()) + "T"
        + pad2(d.getUTCHours()) + pad2(d.getUTCMinutes()) + pad2(d.getUTCSeconds()) + "Z";
}

function icsEscape(text) {
    return String(text).replace(/\\/g, "\\\\").replace(/;/g, "\;").replace(/,/g, "\\,").replace(/\n/g, "\\n");
}

// RFC 5545 lines are folded at 75 octets; continuation lines start with a space.
function icsFold(line) {
    const out = [];
    let rest = line;
    while (rest.length > 72) {
        out.push(rest.slice(0, 72));
        rest = " " + rest.slice(72);
    }
    out.push(rest);
    return out.join("\r\n");
}

// events: [{uid, startMs, endMs, summary, description, url, alarmMinutes}]
function calendar(events) {
    const lines = ["BEGIN:VCALENDAR", "VERSION:2.0", "PRODID:-//Christmas Reckoning//Santa Tracker//EN",
                   "CALSCALE:GREGORIAN", "METHOD:PUBLISH"];
    const stamp = icsDate(Date.now());
    for (const e of events) {
        lines.push("BEGIN:VEVENT", "UID:" + e.uid, "DTSTAMP:" + stamp, "DTSTART:" + icsDate(e.startMs),
                   "DTEND:" + icsDate(e.endMs), "SUMMARY:" + icsEscape(e.summary),
                   "DESCRIPTION:" + icsEscape(e.description), "URL:" + e.url);
        if (e.alarmMinutes)
            lines.push("BEGIN:VALARM", "ACTION:DISPLAY", "DESCRIPTION:" + icsEscape(e.summary),
                       "TRIGGER:-PT" + e.alarmMinutes + "M", "END:VALARM");
        lines.push("END:VEVENT");
    }
    lines.push("END:VCALENDAR");
    return lines.map(icsFold).join("\r\n") + "\r\n";
}

// "in 3 h 12 min" / "in 45 min" / "in 2 days"
function relative(ms) {
    const m = Math.round(ms / 60000);
    if (m < 1) return qsTrNoop("any moment");
    if (m < 60) return "in " + m + " min";
    const h = Math.floor(m / 60);
    if (h < 48) return "in " + h + " h " + pad2(m % 60) + " min";
    return "in " + Math.round(h / 24) + " days";
}

function qsTrNoop(s) { return s; }
