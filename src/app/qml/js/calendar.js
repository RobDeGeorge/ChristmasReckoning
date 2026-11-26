// SPDX-License-Identifier: GPL-3.0-or-later
// Builds the calendar file (.ics) behind the "Remind me" button.
.pragma library

function pad2(n) {
    n = Math.floor(Math.abs(n));
    return (n < 10 ? "0" : "") + n;
}

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
function ics(events) {
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
