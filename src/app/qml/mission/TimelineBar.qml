// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// Play/pause, playback speed, live mode and a scrubber over the whole night, with Santa's
// zone-by-zone schedule drawn underneath. Controls wrap onto two lines on narrow screens.
Rectangle {
    id: bar
    required property QtObject clock   // the page's SimClock: playing, speed, live, startMs, endMs, nightSpeed
    readonly property bool liveAvailable: Platform.nowMs() >= clock.startMs && Platform.nowMs() <= clock.endMs
    property bool narrow: width < 640

    color: Theme.panel
    border.color: Theme.line
    radius: Theme.radius
    implicitHeight: layout.implicitHeight + 2 * Theme.s3

    function setSpeed(s) {
        clock.live = false;
        clock.speed = s;
        clock.playing = true;
    }

    ColumnLayout {
        id: layout
        x: Theme.s3
        y: Theme.s3
        width: bar.width - 2 * Theme.s3
        spacing: Theme.s2

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s2

            PlayPauseButton {
                Layout.alignment: Qt.AlignTop
                playing: bar.clock.playing
                onClicked: {
                    if (!bar.clock.playing && Mission.timeMs >= bar.clock.endMs - 1000)
                        Mission.timeMs = bar.clock.startMs;
                    bar.clock.live = false;
                    bar.clock.playing = !bar.clock.playing;
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: Theme.s2
                PillButton { text: "1×"; selected: !bar.clock.live && bar.clock.speed === 1; onClicked: bar.setSpeed(1) }
                PillButton { text: "60×"; selected: !bar.clock.live && bar.clock.speed === 60; onClicked: bar.setSpeed(60) }
                PillButton { text: "3600×"; selected: !bar.clock.live && bar.clock.speed === 3600; onClicked: bar.setSpeed(3600) }
                PillButton {
                    text: bar.narrow ? qsTr("Night in 60 s") : qsTr("Whole night in 60 s")
                    selected: !bar.clock.live && Math.abs(bar.clock.speed - bar.clock.nightSpeed) < 1
                    onClicked: bar.setSpeed(bar.clock.nightSpeed)
                    Accessible.name: qsTr("Play the whole night in 60 seconds")
                }
                PillButton {
                    visible: bar.liveAvailable
                    text: qsTr("● Live")
                    selected: bar.clock.live
                    onClicked: { bar.clock.live = true; bar.clock.playing = true; bar.clock.speed = 1; }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: Fmt.utcDayClock(Mission.timeMs)
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 12
            }
            Item { Layout.fillWidth: true }
            Label {
                text: Fmt.met((Mission.timeMs - Mission.launchMs) / 1000)
                color: Theme.textMuted
                font.family: Theme.monoFamily
                font.pixelSize: 12
            }
        }

        Slider {
            id: scrub
            Layout.fillWidth: true
            from: bar.clock.startMs
            to: bar.clock.endMs
            value: Mission.timeMs
            live: true
            stepSize: 60000
            Accessible.name: qsTr("Simulated time")
            Accessible.description: Fmt.utcDayClock(Mission.timeMs)
            onMoved: {
                bar.clock.live = false;
                Mission.timeMs = value;
            }
            onPressedChanged: if (pressed) bar.clock.playing = false
            background: Item {
                x: scrub.leftPadding
                y: scrub.topPadding + scrub.availableHeight / 2 - height / 2
                width: scrub.availableWidth
                height: 6
                Rectangle {
                    anchors.fill: parent
                    radius: 3
                    color: Theme.line
                }
                Rectangle {
                    width: scrub.visualPosition * parent.width
                    height: parent.height
                    radius: 3
                    color: Theme.gold
                }
            }
            handle: Rectangle {
                x: scrub.leftPadding + scrub.visualPosition * (scrub.availableWidth - width)
                y: scrub.topPadding + scrub.availableHeight / 2 - height / 2
                width: 20; height: 20; radius: 10
                color: Theme.goldBright
                border.color: scrub.visualFocus ? Theme.text : Theme.sky
                border.width: 2
            }
        }

        // Santa's schedule as one strip: where in the night each time zone's work happens.
        Item {
            Layout.fillWidth: true
            Layout.leftMargin: scrub.leftPadding + 10
            Layout.rightMargin: scrub.rightPadding + 10
            implicitHeight: 12
            Accessible.role: Accessible.Chart
            Accessible.name: qsTr("Santa's schedule; red marks the bottleneck time zones")
            Repeater {
                model: Mission.zones
                delegate: Rectangle {
                    required property var modelData
                    readonly property real span: bar.clock.endMs - bar.clock.startMs
                    x: (modelData.startMs - bar.clock.startMs) / span * parent.width
                    width: Math.max(1.5, (modelData.endMs - modelData.startMs) / span * parent.width)
                    height: parent.height
                    radius: 2
                    color: modelData.binding ? Theme.nogo : Theme.gold
                    opacity: 0.3 + 0.7 * Math.min(1, Math.sqrt(modelData.households / 2e8))
                }
            }
        }
    }
}
