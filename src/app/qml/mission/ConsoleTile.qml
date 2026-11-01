// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning

// One flight-controller station in the room: callsign, status lamp, question and headline number.
AbstractButton {
    id: tile
    required property var station
    hoverEnabled: true
    implicitHeight: 132
    padding: Theme.s3
    leftPadding: Theme.s4
    Accessible.role: Accessible.Button
    Accessible.name: station.callsign + ", " + station.title + ". " + station.status

    background: Rectangle {
        radius: Theme.radius
        color: tile.down ? Theme.panelHover : (tile.hovered ? Theme.panelRaised : Theme.panel)
        border.width: tile.visualFocus ? 2 : 1
        border.color: tile.visualFocus ? Theme.text : Qt.alpha(Theme.statusColor(tile.station.go), 0.45)
        Rectangle { // status stripe
            width: 3
            height: parent.height - 2 * parent.radius
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.statusColor(tile.station.go)
            radius: 1
        }
    }

    contentItem: ColumnLayout {
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: tile.station.callsign
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 14
                font.weight: Font.Bold
                font.letterSpacing: 1
                elide: Text.ElideRight
            }
            StatusLight { go: tile.station.go; compact: true }
        }
        Label {
            Layout.fillWidth: true
            text: tile.station.question
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            elide: Text.ElideRight
        }
        Item { Layout.fillHeight: true }
        Label {
            Layout.fillWidth: true
            text: tile.station.readouts.length ? tile.station.readouts[0].label.toUpperCase() : ""
            color: Theme.textFaint
            font.pixelSize: 10
            font.letterSpacing: 1
            elide: Text.ElideRight
        }
        Label {
            Layout.fillWidth: true
            text: tile.station.readouts.length ? tile.station.readouts[0].value : ""
            color: tile.station.go ? Theme.text : Theme.goldBright
            font.family: Theme.monoFamily
            font.pixelSize: 18
            font.weight: Font.Bold
            elide: Text.ElideRight
        }
    }
}
