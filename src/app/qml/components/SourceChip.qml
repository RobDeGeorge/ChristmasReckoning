// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import ChristmasReckoning

// A citation: short title, opens the source in a new tab when it has a link.
AbstractButton {
    id: chip
    property var source   // {key, title, publisher, url, note}
    readonly property bool linked: !!source && source.url.length > 0

    visible: !!source
    hoverEnabled: true
    implicitHeight: 24
    implicitWidth: Math.min(label.implicitWidth + 18, 320)
    enabled: linked
    onClicked: Qt.openUrlExternally(source.url)
    ToolTip.visible: hovered && !!source
    ToolTip.text: source ? source.title + " — " + source.publisher + (source.note ? "\n" + source.note : "") : ""
    ToolTip.delay: 400
    Accessible.role: Accessible.Link
    Accessible.name: source ? qsTr("Source: %1").arg(source.title) : ""

    background: Rectangle {
        radius: 12
        color: chip.hovered ? Theme.panelHover : "transparent"
        border.color: chip.visualFocus ? Theme.text : Theme.lineStrong
    }
    contentItem: Label {
        id: label
        text: (chip.linked ? "↗ " : "") + (chip.source ? chip.source.title : "")
        color: Theme.telemetry
        font.pixelSize: 12
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        leftPadding: 9
        rightPadding: 9
    }
}
