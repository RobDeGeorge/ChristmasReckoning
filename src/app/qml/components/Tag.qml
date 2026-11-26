// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import ChristmasReckoning

// A small pill that says what you're looking at: LIVE, PREVIEW or REPLAY.
Rectangle {
    id: tag
    property alias text: label.text
    property color tint: Theme.amber
    property bool pulsing: false

    implicitWidth: row.implicitWidth + 20
    implicitHeight: 26
    radius: height / 2
    color: Qt.alpha(tint, 0.16)
    border.color: Qt.alpha(tint, 0.6)
    Accessible.role: Accessible.StaticText
    Accessible.name: text

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 8; height: 8; radius: 4
            color: tag.tint
            visible: tag.pulsing
            SequentialAnimation on opacity {
                running: tag.pulsing && !Platform.reducedMotion
                loops: Animation.Infinite
                NumberAnimation { to: 0.25; duration: 650 }
                NumberAnimation { to: 1; duration: 650 }
            }
        }
        Text {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: tag.tint
            font.family: Theme.monoFamily
            font.pixelSize: 12
            font.weight: Font.Bold
            font.letterSpacing: 1.5
        }
    }
}
