// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import ChristmasReckoning

// One delivery hop with the gentlest possible force profile: full acceleration for half the hop,
// full braking for the other half. Speed rises and falls in a triangle; acceleration is a square wave.
Item {
    id: chart
    property string hopText
    property string timeText
    property string peakText
    property string accelText

    implicitHeight: 190
    Accessible.role: Accessible.Chart
    Accessible.name: qsTr("One hop of %1 in %2: speed peaks at %3 mid-hop; acceleration ±%4.")
                     .arg(hopText).arg(timeText).arg(peakText).arg(accelText)

    readonly property real plotLeft: 70
    readonly property real w: width - plotLeft - 12
    readonly property real speedTop: 18
    readonly property real speedH: 70
    readonly property real accTop: speedTop + speedH + 26
    readonly property real accH: 56

    Label { x: 0; y: chart.speedTop; text: qsTr("speed"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
    Label { x: 0; y: chart.accTop; text: qsTr("acceleration"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath { // speed triangle
            strokeWidth: 2
            strokeColor: Theme.gold
            fillColor: Qt.alpha(Theme.gold, 0.15)
            startX: chart.plotLeft; startY: chart.speedTop + chart.speedH
            PathLine { x: chart.plotLeft + chart.w / 2; y: chart.speedTop }
            PathLine { x: chart.plotLeft + chart.w; y: chart.speedTop + chart.speedH }
        }
        ShapePath { // acceleration square wave
            strokeWidth: 2
            strokeColor: Theme.telemetry
            fillColor: "transparent"
            startX: chart.plotLeft; startY: chart.accTop + chart.accH / 2
            PathLine { x: chart.plotLeft; y: chart.accTop }
            PathLine { x: chart.plotLeft + chart.w / 2; y: chart.accTop }
            PathLine { x: chart.plotLeft + chart.w / 2; y: chart.accTop + chart.accH }
            PathLine { x: chart.plotLeft + chart.w; y: chart.accTop + chart.accH }
            PathLine { x: chart.plotLeft + chart.w; y: chart.accTop + chart.accH / 2 }
        }
        ShapePath { // zero line
            strokeWidth: 1
            strokeColor: Theme.line
            fillColor: "transparent"
            startX: chart.plotLeft; startY: chart.accTop + chart.accH / 2
            PathLine { x: chart.plotLeft + chart.w; y: chart.accTop + chart.accH / 2 }
        }
    }

    Label {
        x: chart.plotLeft + chart.w / 2 - width / 2
        y: chart.speedTop - 16
        text: qsTr("peak %1").arg(chart.peakText)
        color: Theme.goldBright
        font.family: Theme.monoFamily
        font.pixelSize: 11
    }
    Label {
        x: chart.plotLeft + 6
        y: chart.accTop + 4
        text: "+" + chart.accelText
        color: Theme.telemetry
        font.family: Theme.monoFamily
        font.pixelSize: 11
    }
    Label {
        x: chart.plotLeft + chart.w - width - 6
        y: chart.accTop + chart.accH - height - 4
        text: "−" + chart.accelText
        color: Theme.telemetry
        font.family: Theme.monoFamily
        font.pixelSize: 11
    }
    Label {
        x: chart.plotLeft
        y: chart.accTop + chart.accH + 6
        width: chart.w
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("one hop: %1 in %2").arg(chart.hopText).arg(chart.timeText)
        color: Theme.textFaint
        font.family: Theme.monoFamily
        font.pixelSize: 11
    }
}
