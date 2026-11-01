// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning

// A labelled telemetry number.
ColumnLayout {
    id: readout
    property string label
    property string value
    property color valueColor: Theme.text
    property int valueSize: 20

    spacing: 2
    Accessible.role: Accessible.StaticText
    Accessible.name: label + ": " + value

    Label {
        Layout.fillWidth: true
        text: readout.label.toUpperCase()
        color: Theme.textFaint
        font.pixelSize: 11
        font.letterSpacing: 1.2
        font.weight: Font.Medium
        elide: Text.ElideRight
    }
    Label {
        Layout.fillWidth: true
        text: readout.value
        color: readout.valueColor
        font.family: Theme.monoFamily
        font.pixelSize: readout.valueSize
        font.weight: Font.Bold
        elide: Text.ElideRight
    }
}
