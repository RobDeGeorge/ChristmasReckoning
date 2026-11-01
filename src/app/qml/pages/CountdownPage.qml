// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning

Item {
    id: page
    signal openMissionControl()

    CountdownClock { id: clock }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.sky }
            GradientStop { position: 1.0; color: Theme.skyHigh }
        }
    }

    Globe {
        id: teaser
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -height * 0.42
        width: Math.min(parent.width * 1.1, 1100)
        height: width
        distance: 2.4
        fovDeg: 40
        centerLatDeg: 35
        interactive: false
        gridAlpha: 0.06
        opacity: 0.9
        NumberAnimation on centerLonDeg {
            running: !Platform.reducedMotion
            from: 60; to: -300; duration: 240000; loops: Animation.Infinite
        }
    }

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Math.max(48, parent.height * 0.12)
        spacing: Theme.s4
        width: Math.min(parent.width - 2 * Theme.s5, 820)

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: clock.isChristmas ? qsTr("THE MOST WONDERFUL DAY OF THE YEAR") : qsTr("CHRISTMAS RECKONING")
            color: Theme.gold
            font.pixelSize: 14
            font.letterSpacing: 3
            font.weight: Font.Medium
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: clock.isChristmas ? qsTr("Merry Christmas!") : qsTr("The most wonderful wait of the year")
            color: Theme.text
            font.family: Theme.displayFamily
            font.pixelSize: Math.min(56, page.width / 12)
        }

        RowLayout {
            id: digits
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.s4
            spacing: Math.min(Theme.s6, page.width / 30)
            Accessible.role: Accessible.StaticText
            Accessible.name: clock.accessibleText

            Repeater {
                model: [
                    { value: clock.days, label: qsTr("days"), pad: false },
                    { value: clock.hours, label: qsTr("hours"), pad: true },
                    { value: clock.minutes, label: qsTr("minutes"), pad: true },
                    { value: clock.seconds, label: qsTr("seconds"), pad: true }
                ]
                delegate: ColumnLayout {
                    required property var modelData
                    spacing: 2
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.pad ? String(modelData.value).padStart(2, "0") : String(modelData.value)
                        color: Theme.text
                        font.family: Theme.monoFamily
                        font.pixelSize: Math.min(88, page.width / 8.5)
                        font.weight: Font.Bold
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.label.toUpperCase()
                        color: Theme.textMuted
                        font.pixelSize: 13
                        font.letterSpacing: 2
                    }
                }
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: clock.isChristmas ? qsTr("The day is here. Make it a magical one.")
                                    : qsTr("Until December 25, %1 · your local time").arg(clock.year)
            color: Theme.textMuted
            font.pixelSize: 16
        }

        Button {
            id: enter
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.s5
            text: qsTr("Enter North Pole Mission Control  →")
            font.pixelSize: 16
            font.weight: Font.Medium
            onClicked: page.openMissionControl()
            Accessible.name: qsTr("Enter North Pole Mission Control")
            contentItem: Label {
                text: enter.text
                font: enter.font
                color: Theme.sky
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                implicitHeight: 48
                implicitWidth: 300
                radius: 24
                color: enter.down ? Theme.gold : (enter.hovered || enter.visualFocus ? Theme.goldBright : Theme.gold)
                border.color: enter.visualFocus ? Theme.text : "transparent"
                border.width: 2
            }
        }
    }
}
