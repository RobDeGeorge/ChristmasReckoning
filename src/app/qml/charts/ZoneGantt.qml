// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// Every time zone's delivery window (faint) and when Santa actually works there (solid), across the
// night in UTC. The bottleneck run of zones is highlighted.
Item {
    id: chart
    property var zones: Mission.zones
    property real nowMs: Mission.timeMs
    property bool compact: false

    readonly property real t0: zones.length ? zones[0].windowOpenMs : 0
    readonly property real t1: zones.length ? zones[zones.length - 1].windowCloseMs : 1
    readonly property int labelWidth: compact ? 0 : 76
    readonly property real rowHeight: compact ? Math.max(2, height / Math.max(1, zones.length)) : 13
    readonly property real plotWidth: width - labelWidth

    function xOf(ms) { return labelWidth + (ms - t0) / Math.max(1, t1 - t0) * plotWidth }

    implicitHeight: compact ? 40 : zones.length * rowHeight + 28
    Accessible.role: Accessible.Chart
    Accessible.name: qsTr("Delivery windows and Santa's schedule for %1 time zones").arg(zones.length)

    // Hour gridlines every 3 h.
    Repeater {
        model: chart.compact ? 0 : Math.floor((chart.t1 - chart.t0) / 10800000) + 1
        delegate: Rectangle {
            required property int index
            readonly property real ms: Math.ceil(chart.t0 / 10800000) * 10800000 + index * 10800000
            visible: ms <= chart.t1
            x: chart.xOf(ms)
            y: 0
            width: 1
            height: chart.zones.length * chart.rowHeight
            color: Theme.line
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.bottom
                anchors.topMargin: 4
                text: Fmt.utcClock(parent.ms)
                color: Theme.textFaint
                font.family: Theme.monoFamily
                font.pixelSize: 10
            }
        }
    }

    Repeater {
        model: chart.zones
        delegate: Item {
            required property var modelData
            required property int index
            y: index * chart.rowHeight
            width: chart.width
            height: chart.rowHeight

            Label {
                visible: !chart.compact
                width: chart.labelWidth - 6
                anchors.verticalCenter: parent.verticalCenter
                text: modelData.label.replace("UTC", "")
                color: modelData.binding ? Theme.nogo : Theme.textFaint
                font.family: Theme.monoFamily
                font.pixelSize: 9
                horizontalAlignment: Text.AlignRight
            }
            Rectangle { // window
                x: chart.xOf(modelData.windowOpenMs)
                width: chart.xOf(modelData.windowCloseMs) - x
                anchors.verticalCenter: parent.verticalCenter
                height: Math.max(1, chart.rowHeight - 4)
                radius: 2
                color: Qt.alpha(Theme.telemetry, 0.12)
            }
            Rectangle { // actual work
                x: chart.xOf(modelData.startMs)
                width: Math.max(1.5, chart.xOf(modelData.endMs) - x)
                anchors.verticalCenter: parent.verticalCenter
                height: Math.max(1, chart.rowHeight - 4)
                radius: 2
                color: modelData.binding ? Theme.nogo : Theme.gold
                opacity: 0.35 + 0.65 * Math.min(1, Math.sqrt(modelData.households / 2e8))
            }
        }
    }

    Rectangle { // now
        visible: chart.nowMs >= chart.t0 && chart.nowMs <= chart.t1
        x: chart.xOf(chart.nowMs)
        width: 2
        height: chart.zones.length * chart.rowHeight
        color: Theme.text
    }
}
