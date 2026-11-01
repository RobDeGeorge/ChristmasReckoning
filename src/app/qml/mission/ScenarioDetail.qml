// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning

// Every assumption in one place. The whole room recalculates when any of them changes.
ColumnLayout {
    id: scenario
    signal closeRequested()
    spacing: Theme.s4

    readonly property var groups: [
        { title: qsTr("Who and what"), keys: ["eligibility", "presentsPerChild", "giftMassKg"] },
        { title: qsTr("When"), keys: ["windowStartHour", "windowLengthHours", "stopTimeS"] },
        { title: qsTr("Route"), keys: ["routeQuality", "householdsPerStop", "altitudeM"] },
        { title: qsTr("Sleigh and team"), keys: ["sleighMassKg", "santaMassKg", "reindeerCount", "reindeerMassKg",
                                                  "dragCoefficient", "heatAbsorbedFraction"] },
        { title: qsTr("Snacks"), keys: ["cookiesPerStop", "cookieMassG", "milkPerStopMl"] }
    ]
    readonly property var paramMap: {
        const m = {};
        for (const p of Mission.parameters) m[p.key] = p;
        return m;
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("SCENARIO")
            color: Theme.text
            font.family: Theme.monoFamily
            font.pixelSize: 20
            font.weight: Font.Bold
            font.letterSpacing: 1.5
        }
        PillButton { text: qsTr("Close"); onClicked: scenario.closeRequested() }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The defaults are as kind to Santa as reality allows: a perfect route, no time spent at each house, rooftop altitude. Change anything and every console recalculates.")
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
        wrapMode: Text.WordWrap
    }

    Rectangle {
        Layout.fillWidth: true
        visible: !(Mission.summary.feasible ?? true)
        implicitHeight: note.implicitHeight + 2 * Theme.s3
        radius: 8
        color: Qt.alpha(Theme.nogo, 0.12)
        border.color: Qt.alpha(Theme.nogo, 0.5)
        Label {
            id: note
            anchors.fill: parent
            anchors.margins: Theme.s3
            text: Mission.summary.feasibilityNote ?? ""
            color: Theme.text
            wrapMode: Text.WordWrap
        }
    }

    RowLayout {
        spacing: Theme.s2
        PillButton {
            text: qsTr("Reset everything")
            enabled: !Mission.scenarioIsDefault
            onClicked: Mission.resetParameters()
        }
        PillButton {
            text: qsTr("Download results (JSON)")
            onClicked: Platform.downloadText("christmas-reckoning-scenario.json", Mission.exportJson(), "application/json")
        }
    }

    Repeater {
        model: scenario.groups
        delegate: ColumnLayout {
            required property var modelData
            Layout.fillWidth: true
            spacing: Theme.s3
            Label {
                text: modelData.title.toUpperCase()
                color: Theme.gold
                font.pixelSize: 11
                font.letterSpacing: 1.5
                font.weight: Font.Bold
                Layout.topMargin: Theme.s2
            }
            Repeater {
                model: modelData.keys
                delegate: ParamSlider {
                    required property string modelData
                    Layout.fillWidth: true
                    param: scenario.paramMap[modelData]
                }
            }
        }
    }
}
