// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import ChristmasReckoning

// GO / NO-GO indicator lamp with its text.
Rectangle {
    id: light
    property bool go: false
    property bool compact: false

    implicitWidth: row.implicitWidth + (compact ? 12 : 16)
    implicitHeight: compact ? 22 : 26
    radius: height / 2
    color: Qt.alpha(Theme.statusColor(go), 0.14)
    border.color: Qt.alpha(Theme.statusColor(go), 0.55)
    Accessible.role: Accessible.StaticText
    Accessible.name: go ? qsTr("GO") : qsTr("NO-GO")

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 8
            height: 8
            radius: 4
            color: Theme.statusColor(light.go)
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: light.go ? qsTr("GO") : qsTr("NO-GO")
            color: Theme.statusColor(light.go)
            font.family: Theme.monoFamily
            font.pixelSize: light.compact ? 11 : 12
            font.weight: Font.Bold
            font.letterSpacing: 1
        }
    }
}
