// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// Tells the visitor what they're watching (live / preview / replay), when to come back for the real
// thing, and when Santa reaches their own time zone.
Rectangle {
    id: banner
    required property string mode       // "live" | "preview" | "replay"
    required property bool liveWindow   // is the real flight (or its run-up) happening now?
    signal goLive()

    readonly property var zone: Mission.zoneForOffset(Platform.utcOffsetMinutes(Mission.referenceMs))
    readonly property real simNow: Mission.timeMs
    readonly property string zoneStatus: !zone.startMs ? ""
                                        : simNow < zone.startMs ? "upcoming"
                                        : simNow <= zone.endMs ? "now" : "done"
    readonly property color tint: mode === "live" ? Theme.nogo : (mode === "replay" ? Theme.telemetry : Theme.amber)

    color: Theme.panel
    border.color: Qt.alpha(tint, 0.45)
    radius: Theme.radius
    implicitHeight: layout.implicitHeight + 2 * Theme.s4

    ColumnLayout {
        id: layout
        x: Theme.s4
        y: Theme.s4
        width: banner.width - 2 * Theme.s4
        spacing: Theme.s2

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s3
            LiveBadge {
                Layout.alignment: Qt.AlignTop
                mode: banner.mode
                enabled: banner.liveWindow && banner.mode !== "live"
                onClicked: banner.goLive()
            }
            Label {
                Layout.fillWidth: true
                text: banner.mode === "live" ? qsTr("Santa's real position, right now.")
                    : banner.mode === "replay" ? qsTr("Santa has landed. This is a replay of the night.")
                    : banner.liveWindow ? qsTr("Santa is flying for real right now.")
                    : qsTr("A preview of Christmas Eve %1.").arg(Mission.year)
                color: Theme.text
                font.pixelSize: Theme.fontLead
                font.weight: Font.Bold
                wrapMode: Text.WordWrap
            }
        }

        Label {
            Layout.fillWidth: true
            text: banner.mode === "live"
                  ? qsTr("Everyone watching sees the same sleigh at the same moment. Leave it open: it updates live until he's home.")
                  : banner.mode === "replay"
                  ? qsTr("He landed back at the North Pole at %1 your time. Live tracking returns next Christmas Eve.")
                        .arg(Platform.localDateTime(Mission.landingMs))
                  : banner.liveWindow
                  ? qsTr("You're scrubbing through the night. Jump back to live any time.")
                  : qsTr("The real flight launches at %1 your time. Come back then to watch him live at christmasreckoning.com/#/live")
                        .arg(Platform.localDateTime(Mission.launchMs))
            color: Theme.textMuted
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.s2
            visible: calendar.visible || watchLive.visible
            CalendarButton {
                id: calendar
                visible: banner.mode === "preview" && !banner.liveWindow
            }
            PillButton {
                id: watchLive
                visible: banner.liveWindow && banner.mode !== "live"
                primary: true
                text: qsTr("● Watch live")
                onClicked: banner.goLive()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
            implicitHeight: 1
            color: Theme.line
            visible: banner.zoneStatus.length > 0
        }

        Label {
            visible: banner.zoneStatus.length > 0
            text: qsTr("YOUR TIME ZONE · %1").arg(banner.zone.label ?? "")
            color: Theme.textFaint
            font.pixelSize: 11
            font.letterSpacing: 1.2
            font.weight: Font.Medium
        }
        Label {
            Layout.fillWidth: true
            visible: banner.zoneStatus.length > 0
            text: banner.zoneStatus === "now"
                  ? qsTr("Santa is delivering in your time zone right now!")
                  : banner.zoneStatus === "done"
                  ? qsTr("Santa has visited your time zone (%1 – %2 your time).")
                        .arg(Platform.localTime(banner.zone.startMs)).arg(Platform.localTime(banner.zone.endMs))
                  : qsTr("Santa arrives %1, at about %2 your time, and is done by %3.")
                        .arg(Fmt.relative(banner.zone.startMs - banner.simNow))
                        .arg(Platform.localTime(banner.zone.startMs)).arg(Platform.localTime(banner.zone.endMs))
            color: banner.zoneStatus === "now" ? Theme.go : Theme.goldBright
            font.pixelSize: Theme.fontBody
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: banner.zoneStatus.length > 0 && (banner.zone.countries ?? []).length > 0
            text: qsTr("Shared with %1.").arg((banner.zone.countries ?? []).slice(0, 3).join(", "))
            color: Theme.textFaint
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
