// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes
import ChristmasReckoning

// Santa's flight path drawn over the globe: where he has been (gold, brightest just behind him),
// where he is going (faint), and Santa himself. Positions come from the simulation, projection from
// the Globe.
Item {
    id: overlay
    required property Item globe
    // The moment being shown; Santa sits where the two parts of the route meet.
    property real timeMs: Mission.timeMs
    property var santa: Mission.state

    readonly property bool flying: (santa.phase ?? "prelaunch") !== "prelaunch" && santa.phase !== "landed"

    // How much of the path behind Santa is drawn bright.
    property real recentMs: 12 * 60000

    // The night's route as unit vectors, computed once per flight. Long legs are bent along the
    // great circle so they hug the globe instead of cutting through it.
    readonly property var route: {
        Mission.launchMs; Mission.landingMs;
        const out = [];
        let prev = null;
        for (const sample of Mission.flightPath(360)) {
            const p = overlay.unit(sample.lat, sample.lon, sample.t);
            if (prev) {
                const angle = Math.acos(Math.max(-1, Math.min(1, prev.x * p.x + prev.y * p.y + prev.z * p.z)));
                const steps = Math.min(12, Math.ceil(angle / 0.1));
                for (let i = 1; i < steps; ++i) {
                    const u = i / steps;
                    const a = Math.sin((1 - u) * angle) / Math.sin(angle), b = Math.sin(u * angle) / Math.sin(angle);
                    out.push({ x: a * prev.x + b * p.x, y: a * prev.y + b * p.y, z: a * prev.z + b * p.z,
                               t: prev.t + (p.t - prev.t) * u });
                }
            }
            out.push(p);
            prev = p;
        }
        return out;
    }

    function unit(latDeg, lonDeg, t) {
        const lat = latDeg * Math.PI / 180, lon = lonDeg * Math.PI / 180;
        return { x: Math.cos(lat) * Math.cos(lon), y: Math.cos(lat) * Math.sin(lon), z: Math.sin(lat), t: t };
    }

    // The route in screen space, in three parts, each a list of polylines (a line breaks wherever
    // the route passes behind the globe). Mirrors Globe.project(), with the camera maths hoisted out.
    readonly property var lines: {
        const g = overlay.globe;
        const cLat = g.centerLatDeg * Math.PI / 180, cLon = g.centerLonDeg * Math.PI / 180;
        const fx = Math.cos(cLat) * Math.cos(cLon), fy = Math.cos(cLat) * Math.sin(cLon), fz = Math.sin(cLat);
        const ex = -Math.sin(cLon), ey = Math.cos(cLon);
        const nx = -Math.sin(cLat) * Math.cos(cLon), ny = -Math.sin(cLat) * Math.sin(cLon), nz = Math.cos(cLat);
        const k = g.height / 2 / Math.tan(g.fovDeg * Math.PI / 360);
        const ox = g.width / 2 + g.offsetX, oy = g.height / 2 + g.offsetY;
        const horizon = 1 / g.distance, d = g.distance;

        const now = overlay.timeMs;
        const parts = [{ until: now - overlay.recentMs, runs: [] }, { until: now, runs: [] }, { until: Infinity, runs: [] }];
        let part = 0, run = null, last = null;
        const add = p => {
            const qz = p.x * fx + p.y * fy + p.z * fz;
            if (qz <= horizon) { run = null; last = null; return; }
            const s = k / (d - qz);
            last = Qt.point(ox + (p.x * ex + p.y * ey) * s, oy - (p.x * nx + p.y * ny + p.z * nz) * s);
            if (!run) { run = []; parts[part].runs.push(run); }
            run.push(last);
        };
        const here = overlay.unit(overlay.santa.latitude ?? 90, overlay.santa.longitude ?? 0, now);
        for (const p of overlay.route) {
            while (p.t > parts[part].until) {
                if (part === 1)
                    add(here);
                // Start the next part where this one ends, so the line stays joined up.
                ++part;
                run = last ? [last] : null;
                if (run) parts[part].runs.push(run);
            }
            add(p);
        }
        const drawable = runs => runs.filter(r => r.length > 1);
        return { flown: drawable(parts[0].runs), recent: drawable(parts[1].runs), ahead: drawable(parts[2].runs) };
    }

    readonly property var here: globe.project(santa.latitude ?? 90, santa.longitude ?? 0)

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeWidth: 1
            strokeColor: Qt.alpha(Theme.text, 0.22)
            fillColor: "transparent"
            joinStyle: ShapePath.RoundJoin
            PathMultiline { paths: overlay.lines.ahead }
        }
        ShapePath {
            strokeWidth: 1.5
            strokeColor: Qt.alpha(Theme.gold, 0.6)
            fillColor: "transparent"
            joinStyle: ShapePath.RoundJoin
            PathMultiline { paths: overlay.lines.flown }
        }
        ShapePath {
            strokeWidth: 3
            strokeColor: Theme.goldBright
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathMultiline { paths: overlay.lines.recent }
        }
    }

    Item {
        visible: overlay.here.visible
        x: overlay.here.x
        y: overlay.here.y

        Rectangle { // glow
            anchors.centerIn: parent
            width: 38; height: 38; radius: 19
            color: Qt.alpha(Theme.red, 0.3)
            SequentialAnimation on scale {
                running: overlay.flying && !Platform.reducedMotion
                loops: Animation.Infinite
                NumberAnimation { from: 0.7; to: 1.25; duration: 900; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1.25; to: 0.7; duration: 900; easing.type: Easing.InOutSine }
            }
        }
        Rectangle { // Santa
            anchors.centerIn: parent
            width: 14; height: 14; radius: 7
            color: Theme.red
            border.color: Theme.text
            border.width: 2
        }
    }
}
