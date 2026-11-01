// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning

Item {
    id: page
    signal back()

    Rectangle {
        anchors.fill: parent
        color: Theme.sky
    }

    Globe {
        id: globe
        anchors.fill: parent
        anchors.topMargin: header.height
    }

    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 64
        color: Theme.panel
        border.color: Theme.line

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.s4
            anchors.rightMargin: Theme.s4
            spacing: Theme.s4

            Button {
                text: qsTr("← Countdown")
                onClicked: page.back()
                flat: true
                palette.buttonText: Theme.textMuted
            }

            Label {
                text: qsTr("NORTH POLE MISSION CONTROL")
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 18
                font.weight: Font.Bold
                font.letterSpacing: 2
                Layout.fillWidth: true
            }

            Button {
                text: globe.mapMode ? qsTr("Globe view") : qsTr("Ground track")
                onClicked: globe.mapMode = !globe.mapMode
            }
        }
    }
}
