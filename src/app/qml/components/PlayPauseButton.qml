// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import ChristmasReckoning

// Round play/pause button with drawn icons (no reliance on symbol fonts).
AbstractButton {
    id: control
    property bool playing: false

    implicitWidth: 40
    implicitHeight: 34
    hoverEnabled: true
    Accessible.role: Accessible.Button
    Accessible.name: playing ? qsTr("Pause") : qsTr("Play")

    background: Rectangle {
        radius: height / 2
        color: control.down ? Theme.goldBright : Theme.gold
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.text
    }

    contentItem: Item {
        Row { // pause
            visible: control.playing
            anchors.centerIn: parent
            spacing: 4
            Rectangle { width: 4; height: 14; radius: 1; color: Theme.sky }
            Rectangle { width: 4; height: 14; radius: 1; color: Theme.sky }
        }
        Shape { // play
            visible: !control.playing
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: 2
            width: 13
            height: 14
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeWidth: 0
                strokeColor: "transparent"
                fillColor: Theme.sky
                startX: 0; startY: 0
                PathLine { x: 13; y: 7 }
                PathLine { x: 0; y: 14 }
                PathLine { x: 0; y: 0 }
            }
        }
    }
}
