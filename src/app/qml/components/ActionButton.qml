// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import ChristmasReckoning

// The one prominent button on the page.
Rectangle {
    id: control
    property alias text: label.text
    signal clicked()

    implicitWidth: label.implicitWidth + 56
    implicitHeight: 48
    radius: height / 2
    color: tap.pressed ? Theme.gold : (hover.hovered ? Theme.goldBright : Theme.gold)
    border.width: activeFocus ? 2 : 0
    border.color: Theme.text
    activeFocusOnTab: true

    Accessible.role: Accessible.Button
    Accessible.name: text
    Accessible.onPressAction: clicked()
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
    Keys.onSpacePressed: clicked()

    Text {
        id: label
        anchors.centerIn: parent
        color: Theme.sky
        font.pixelSize: 16
        font.weight: Font.Bold
    }
    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { id: tap; onTapped: control.clicked() }
}
