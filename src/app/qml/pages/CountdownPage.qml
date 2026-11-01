// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// The front door: a countdown to Christmas in the visitor's own time zone, Santa's launch status,
// and the way into Mission Control. Sized from phone portrait up to large desktops.
Item {
    id: page
    signal openMissionControl()

    readonly property bool phone: width < 600
    readonly property bool shortScreen: height < 700

    CountdownClock { id: clock }

    // Santa right now (launch countdown, live progress, landed).
    readonly property var santa: { clock.nowMs; return Mission.stateAt(clock.nowMs); }
    readonly property string santaPhase: santa.phase ?? "prelaunch"
    readonly property bool inFlight: santaPhase !== "prelaunch" && santaPhase !== "landed"
    readonly property bool landedToday: santaPhase === "landed" && clock.isChristmas

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.sky }
            GradientStop { position: 0.7; color: "#081a17" }
            GradientStop { position: 1.0; color: Theme.skyHigh }
        }
    }

    // The Earth rising from the bottom of the screen, turning slowly (live progress on Christmas Eve).
    Globe {
        id: teaser
        anchors.fill: parent
        interactive: false
        gridAlpha: 0.05
        fovDeg: 30
        timeMs: clock.nowMs
        showDeliveries: page.inFlight || page.landedToday
        frontierZone: (page.santa.frontierZone ?? 0) + 1
        frontierRank: page.santa.frontierRank ?? 0
        centerLatDeg: 32
        // Disc radius we want on screen, and the camera distance that produces it.
        readonly property real wantRadius: page.phone ? width * 0.95 : Math.max(width * 0.42, height * 0.62)
        distance: {
            const t = Math.tan(fovDeg * Math.PI / 360);
            const alpha = Math.atan(wantRadius * t / (height / 2));
            return Math.max(1.05, 1 / Math.sin(alpha));
        }
        offsetY: height * (page.phone ? 0.24 : 0.2) + discRadius
        opacity: 0.92
        NumberAnimation on centerLonDeg {
            running: !Platform.reducedMotion && !page.inFlight
            from: 60; to: -300; duration: 240000; loops: Animation.Infinite
        }
        Binding on centerLonDeg {
            when: page.inFlight
            value: page.santa.longitude ?? 0
        }
    }

    SnowField {
        anchors.fill: parent
        density: page.phone ? 0.45 : 0.55
    }

    Rectangle { // keep text readable over the globe
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.alpha(Theme.sky, 0.55) }
            GradientStop { position: 0.55; color: Qt.alpha(Theme.sky, 0.25) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    ColumnLayout {
        id: content
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: page.shortScreen ? 28 : Math.max(40, parent.height * 0.09)
        width: Math.min(parent.width - 2 * Theme.s4, 1000)
        spacing: page.phone ? Theme.s3 : Theme.s4

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: clock.isChristmas ? qsTr("THE MOST WONDERFUL DAY OF THE YEAR") : qsTr("CHRISTMAS RECKONING")
            color: Theme.gold
            font.pixelSize: page.phone ? 12 : 14
            font.letterSpacing: 3
            font.weight: Font.Medium
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: clock.isChristmas ? qsTr("Merry Christmas!") : qsTr("The most wonderful wait of the year")
            color: Theme.text
            font.family: Theme.displayFamily
            font.pixelSize: Math.max(28, Math.min(52, page.width / 15))
            Accessible.role: Accessible.Heading
        }

        RowLayout {
            id: digits
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: page.phone ? Theme.s2 : Theme.s4
            spacing: Math.min(Theme.s6, page.width / 28)
            Accessible.role: Accessible.StaticText
            Accessible.name: clock.accessibleText
            visible: !clock.isChristmas

            Repeater {
                model: [
                    { value: clock.days, label: qsTr("days"), pad: false },
                    { value: clock.hours, label: qsTr("hours"), pad: true },
                    { value: clock.minutes, label: qsTr("minutes"), pad: true },
                    { value: clock.seconds, label: qsTr("seconds"), pad: true }
                ]
                delegate: ColumnLayout {
                    required property var modelData
                    spacing: 0
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.pad ? String(modelData.value).padStart(2, "0") : String(modelData.value)
                        color: Theme.text
                        font.family: Theme.monoFamily
                        font.pixelSize: Math.max(40, Math.min(96, page.width / 8.2))
                        font.weight: Font.Bold
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.label.toUpperCase()
                        color: Theme.textMuted
                        font.pixelSize: page.phone ? 11 : 13
                        font.letterSpacing: 2
                    }
                }
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: clock.isChristmas ? qsTr("The day is here. Make it a magical one.")
                                    : qsTr("Until December 25, %1 · your local time").arg(clock.year)
            color: Theme.textMuted
            font.pixelSize: page.phone ? 14 : 16
        }

        // Santa's status --------------------------------------------------------------------------
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: page.phone ? Theme.s2 : Theme.s3
            Layout.preferredWidth: Math.min(content.width, status.implicitWidth + 2 * Theme.s4)
            implicitHeight: status.implicitHeight + 2 * Theme.s3
            radius: Theme.radius
            color: Qt.alpha(Theme.panel, 0.82)
            border.color: page.inFlight ? Qt.alpha(Theme.go, 0.6) : Theme.line

            ColumnLayout {
                id: status
                anchors.centerIn: parent
                width: Math.min(implicitWidth, content.width - 2 * Theme.s4)
                spacing: 2
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: page.inFlight ? qsTr("● SANTA IS IN FLIGHT")
                          : page.landedToday ? qsTr("SANTA HAS LANDED")
                          : qsTr("SANTA'S LAUNCH WINDOW OPENS IN")
                    color: page.inFlight ? Theme.go : Theme.gold
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.letterSpacing: 1.5
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: content.width - 2 * Theme.s4
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: page.inFlight
                          ? qsTr("%1 presents delivered · over %2").arg(page.santa.text?.presents ?? "0")
                                .arg(page.santa.country || (page.santa.zoneLabel ?? ""))
                          : page.landedToday ? qsTr("Every present delivered. Merry Christmas!")
                          : "T−" + Fmt.countdown(Mission.launchMs - clock.nowMs)
                    color: Theme.text
                    font.family: Theme.monoFamily
                    font.pixelSize: page.inFlight || page.landedToday ? 15 : (page.phone ? 22 : 26)
                    font.weight: Font.Bold
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: content.width - 2 * Theme.s4
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    visible: !page.inFlight && !page.landedToday
                    text: qsTr("22:00 on Christmas Eve in Kiribati, the first place to reach it (%1)")
                          .arg(Fmt.utcDayClock(Mission.launchMs))
                    color: Theme.textFaint
                    font.pixelSize: 12
                }
            }
        }

        PillButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: page.phone ? Theme.s2 : Theme.s4
            primary: true
            implicitHeight: 48
            padding2: 26
            font.pixelSize: 16
            text: page.inFlight ? qsTr("Watch live in Mission Control  →") : qsTr("Enter North Pole Mission Control  →")
            onClicked: page.openMissionControl()
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.maximumWidth: 560
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: !page.shortScreen || !page.phone
            text: qsTr("What would it really take to visit %1 children in one night? Real population data, real physics, every number shown.")
                  .arg(Mission.summary.text?.children ?? "2.4 billion")
            color: Theme.textMuted
            font.pixelSize: page.phone ? 13 : 15
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.s3
        spacing: Theme.s4
        Label {
            text: qsTr("Sources")
            color: Theme.textMuted
            font.pixelSize: 12
            font.underline: sourcesArea.containsMouse
            MouseArea {
                id: sourcesArea
                anchors.fill: parent
                anchors.margins: -6
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Platform.navigate("mission-control/sources")
            }
            Accessible.role: Accessible.Link
            Accessible.name: qsTr("Sources and licences")
        }
        Label {
            text: qsTr("Source code (GPL-3.0)")
            color: Theme.textMuted
            font.pixelSize: 12
            font.underline: codeArea.containsMouse
            MouseArea {
                id: codeArea
                anchors.fill: parent
                anchors.margins: -6
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally(Platform.sourceUrl)
            }
            Accessible.role: Accessible.Link
            Accessible.name: qsTr("Source code on GitHub")
        }
    }
}
