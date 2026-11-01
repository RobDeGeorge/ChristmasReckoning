// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// Live telemetry for the simulated moment.
Rectangle {
    id: panel
    property var santa: Mission.state
    property var summary: Mission.summary
    property bool compact: false

    color: Theme.panel
    border.color: Theme.line
    radius: Theme.radius
    implicitHeight: layout.implicitHeight + 2 * Theme.s4

    readonly property var phaseColors: ({
        prelaunch: Theme.telemetry, transit: Theme.amber, delivering: Theme.go,
        holding: Theme.amber, returning: Theme.telemetry, landed: Theme.go })

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.s4
        spacing: Theme.s3

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s2
            Rectangle {
                implicitWidth: phase.implicitWidth + 16
                implicitHeight: 24
                radius: 12
                color: Qt.alpha(panel.phaseColors[panel.santa.phase ?? "prelaunch"] ?? Theme.telemetry, 0.16)
                Label {
                    id: phase
                    anchors.centerIn: parent
                    text: (panel.santa.phaseLabel ?? "").toUpperCase()
                    color: panel.phaseColors[panel.santa.phase ?? "prelaunch"] ?? Theme.telemetry
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    font.letterSpacing: 1
                }
            }
            Label {
                Layout.fillWidth: true
                text: Fmt.met(panel.santa.elapsedS ?? 0)
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 18
                font.weight: Font.Bold
                horizontalAlignment: Text.AlignRight
                Accessible.name: qsTr("Mission elapsed time %1").arg(text)
            }
        }

        Label {
            Layout.fillWidth: true
            text: {
                const s = panel.santa;
                if (!s.phase || s.phase === "prelaunch")
                    return qsTr("On the pad at the North Pole. Launch %1.").arg(Fmt.utcDayClock(Mission.launchMs));
                if (s.phase === "landed")
                    return qsTr("Back at the North Pole. Every present delivered.");
                if (s.phase === "returning")
                    return qsTr("Heading home to the North Pole.");
                const where = s.country && s.country.length ? s.country : (s.zoneCountries ?? []).slice(0, 2).join(", ");
                if (s.phase === "holding")
                    return qsTr("Holding over %1, waiting for %2 to fall asleep.").arg(where).arg(Mission.zones[s.zone + 1]?.label ?? "");
                return qsTr("%1 over %2 · local time %3").arg(s.zoneLabel ?? "").arg(where).arg(s.text?.localTime ?? "");
            }
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall + 1
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            text: Fmt.utcDayClock(Mission.timeMs)
            color: Theme.textFaint
            font.family: Theme.monoFamily
            font.pixelSize: 12
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.line }

        GridLayout {
            Layout.fillWidth: true
            columns: panel.compact ? 2 : 2
            columnSpacing: Theme.s4
            rowSpacing: Theme.s3

            Readout {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                label: qsTr("Households visited")
                value: panel.santa.text?.households ?? "0"
                valueColor: Theme.goldBright
                valueSize: panel.compact ? 20 : 24
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Presents")
                value: panel.santa.text?.presents ?? "0"
                valueSize: 15
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Of")
                value: panel.summary.text?.presents ?? ""
                valueColor: Theme.textMuted
                valueSize: 15
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Speed")
                value: panel.santa.text?.speed ?? "0"
                valueColor: Theme.telemetry
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Mach · light")
                value: (panel.santa.text?.mach ?? "0") + " · " + (panel.santa.text?.lightFraction ?? "0%")
                valueSize: 15
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Load on Santa")
                value: panel.santa.text?.gload ?? "0 g"
                valueColor: (panel.santa.speedMps ?? 0) > 7900 ? Theme.nogo : Theme.text
                valueSize: 15
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Time per house")
                value: panel.santa.text?.timePerStop ?? ""
                valueSize: 15
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Distance")
                value: panel.santa.text?.distance ?? "0 m"
                valueSize: 15
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Sleigh mass")
                value: panel.santa.text?.mass ?? ""
                valueSize: 15
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Cookies eaten")
                value: panel.santa.text?.cookies ?? "0"
                valueSize: 15
            }
            Readout {
                Layout.fillWidth: true
                label: qsTr("Drag energy")
                value: panel.santa.text?.energy ?? "0 J"
                valueSize: 15
            }
        }
    }
}
