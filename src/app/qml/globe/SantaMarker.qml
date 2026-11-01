// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import ChristmasReckoning

// Santa on the globe: a glowing marker with a heading chevron, a fading trail of where he's just
// been, and a callout label. Positions come from the simulation; projection from the Globe.
Item {
    id: overlay
    required property Item globe
    property var santa: Mission.state
    property real trailSpanMs: 12 * 60 * 1000
    property bool showLabel: true

    readonly property var here: globe.project(santa.latitude ?? 90, santa.longitude ?? 0)
    readonly property var trail: {
        const t = Mission.timeMs;
        const pts = Mission.trail(t, overlay.trailSpanMs, 28);
        return pts.map(p => globe.project(p.lat, p.lon));
    }
    // Screen-space heading from the last bit of the trail.
    readonly property real screenAngle: {
        const n = trail.length;
        if (n < 2) return 0;
        const a = trail[Math.max(0, n - 4)], b = trail[n - 1];
        return Math.atan2(b.y - a.y, b.x - a.x) * 180 / Math.PI;
    }
    readonly property bool active: (santa.phase ?? "prelaunch") !== "prelaunch" && (santa.phase ?? "") !== "landed"

    Shape {
        anchors.fill: parent
        visible: overlay.active
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeWidth: 3
            strokeColor: Qt.alpha(Theme.goldBright, 0.75)
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathPolyline {
                // Drop points on the far side of the globe and long jumps (map wrap-around).
                path: {
                    const out = [];
                    let prev = null;
                    for (const p of overlay.trail) {
                        if (!p.visible) { prev = null; continue; }
                        if (prev && Math.hypot(p.x - prev.x, p.y - prev.y) > overlay.width / 3) out.length = 0;
                        out.push(Qt.point(p.x, p.y));
                        prev = p;
                    }
                    return out;
                }
            }
        }
    }

    Item {
        id: marker
        visible: overlay.here.visible
        x: overlay.here.x
        y: overlay.here.y
        opacity: overlay.active ? 1 : 0.55

        Rectangle { // glow
            anchors.centerIn: parent
            width: 34; height: 34; radius: 17
            color: Qt.alpha(Theme.goldBright, 0.18)
            SequentialAnimation on scale {
                running: overlay.active && !Platform.reducedMotion
                loops: Animation.Infinite
                NumberAnimation { from: 0.8; to: 1.25; duration: 900; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1.25; to: 0.8; duration: 900; easing.type: Easing.InOutSine }
            }
        }
        Rectangle { // core
            anchors.centerIn: parent
            width: 12; height: 12; radius: 6
            color: Theme.goldBright
            border.color: Theme.text
            border.width: 2
        }
        Shape { // heading chevron
            visible: overlay.active
            width: 30; height: 30
            x: -15; y: -15
            rotation: overlay.screenAngle
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeWidth: 0
                strokeColor: "transparent"
                fillColor: Theme.red
                startX: 28; startY: 15
                PathLine { x: 19; y: 9 }
                PathLine { x: 21; y: 15 }
                PathLine { x: 19; y: 21 }
                PathLine { x: 28; y: 15 }
            }
        }

        Rectangle { // callout
            visible: overlay.showLabel
            x: 14
            y: -36
            width: label.implicitWidth + 14
            height: label.implicitHeight + 8
            radius: 6
            color: Theme.scrim
            border.color: Theme.lineStrong
            Label {
                id: label
                anchors.centerIn: parent
                text: qsTr("SANTA") + (overlay.active ? "  " + (overlay.santa.text?.speed ?? "") : "")
                color: Theme.goldBright
                font.family: Theme.monoFamily
                font.pixelSize: 11
                font.weight: Font.Bold
            }
        }
    }
}
