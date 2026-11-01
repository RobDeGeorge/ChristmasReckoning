// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import ChristmasReckoning

// What the visitor is watching: LIVE (real time), PREVIEW (before the night) or REPLAY (after it).
AbstractButton {
    id: badge
    property string mode: "preview"   // "live" | "preview" | "replay"
    readonly property color tint: mode === "live" ? Theme.nogo : (mode === "replay" ? Theme.telemetry : Theme.amber)

    implicitWidth: row.implicitWidth + 18
    implicitHeight: 26
    hoverEnabled: enabled
    Accessible.role: Accessible.Button
    Accessible.name: mode === "live" ? qsTr("Live: Santa's real position now")
                   : mode === "replay" ? qsTr("Replay of the night") : qsTr("Preview of Christmas Eve")

    background: Rectangle {
        radius: height / 2
        color: Qt.alpha(badge.tint, badge.hovered ? 0.24 : 0.14)
        border.color: Qt.alpha(badge.tint, 0.6)
        border.width: badge.visualFocus ? 2 : 1
    }
    contentItem: Item {
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 6
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 8; height: 8; radius: 4
                color: badge.tint
                SequentialAnimation on opacity {
                    running: badge.mode === "live" && !Platform.reducedMotion
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.25; duration: 650 }
                    NumberAnimation { to: 1; duration: 650 }
                }
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: badge.mode === "live" ? qsTr("LIVE") : badge.mode === "replay" ? qsTr("REPLAY") : qsTr("PREVIEW")
                color: badge.tint
                font.family: Theme.monoFamily
                font.pixelSize: 12
                font.weight: Font.Bold
                font.letterSpacing: 1.5
            }
        }
    }
}
