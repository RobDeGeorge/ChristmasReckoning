// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import ChristmasReckoning

// Rounded button in three flavours: primary (gold), quiet (outlined) and selected (segmented).
Button {
    id: control
    property bool primary: false
    property bool selected: false
    property int padding2: 14

    implicitHeight: 34
    leftPadding: padding2
    rightPadding: padding2
    font.family: Theme.sansFamily
    font.pixelSize: Theme.fontSmall + 1
    font.weight: Font.Medium
    hoverEnabled: true
    Accessible.name: text

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.primary ? Theme.sky : (control.selected ? Theme.sky : Theme.text)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: height / 2
        color: control.primary ? (control.down ? Theme.gold : (control.hovered ? Theme.goldBright : Theme.gold))
             : control.selected ? Theme.telemetry
             : (control.down ? Theme.panelHover : (control.hovered ? Theme.panelRaised : "transparent"))
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.text
                    : (control.primary || control.selected ? "transparent" : Theme.lineStrong)
    }
}
