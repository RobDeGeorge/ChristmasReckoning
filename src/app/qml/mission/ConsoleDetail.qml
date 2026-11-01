// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// The full write-up of one console: question, verdict, readouts, every step of the calculation
// (formula, today's numbers, result, validity warning, sources), a chart, sliders and caveats.
ColumnLayout {
    id: detail
    required property var station
    property var sourceMap: {
        const m = {};
        for (const s of Mission.sources) m[s.key] = s;
        return m;
    }
    property var paramMap: {
        const m = {};
        for (const p of Mission.parameters) m[p.key] = p;
        return m;
    }
    signal closeRequested()

    spacing: Theme.s4

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.s3
        Label {
            text: detail.station.callsign
            color: Theme.text
            font.family: Theme.monoFamily
            font.pixelSize: 20
            font.weight: Font.Bold
            font.letterSpacing: 1.5
        }
        StatusLight { go: detail.station.go }
        Item { Layout.fillWidth: true }
        PillButton {
            text: qsTr("Close")
            onClicked: detail.closeRequested()
        }
    }

    Label {
        Layout.fillWidth: true
        text: detail.station.title + " — " + detail.station.question
        color: Theme.text
        font.family: Theme.displayFamily
        font.pixelSize: 26
        wrapMode: Text.WordWrap
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: verdict.implicitHeight + 2 * Theme.s3
        radius: 8
        color: Qt.alpha(Theme.statusColor(detail.station.go), 0.10)
        border.color: Qt.alpha(Theme.statusColor(detail.station.go), 0.4)
        Label {
            id: verdict
            anchors.fill: parent
            anchors.margins: Theme.s3
            text: detail.station.status
            color: Theme.text
            font.pixelSize: Theme.fontLead
            wrapMode: Text.WordWrap
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: Theme.s4
        rowSpacing: Theme.s3
        Repeater {
            model: detail.station.readouts
            delegate: Readout {
                required property var modelData
                Layout.fillWidth: true
                label: modelData.label
                value: modelData.value
                valueColor: Theme.goldBright
            }
        }
    }

    // Console-specific charts --------------------------------------------------------------------
    ZoneGantt {
        visible: detail.station.id === "windows"
        Layout.fillWidth: true
    }
    Label {
        visible: detail.station.id === "windows"
        Layout.fillWidth: true
        text: qsTr("Faint bars: when each time zone's children are asleep. Solid bars: when Santa is there. Red: the bottleneck run of time zones that sets his minimum speed.")
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }
    LineChart {
        visible: detail.station.id === "loadmaster"
        Layout.fillWidth: true
        Layout.preferredHeight: 200
        title: qsTr("Sleigh mass through the night")
        points: Mission.massSeries.map(p => ({ x: p.t, y: p.massKg / 1000 }))
        xFormat: v => Fmt.utcClock(v) + " UTC"
        yFormat: v => Fmt.num(v) + " t"
        markerX: Mission.timeMs
    }
    LineChart {
        visible: detail.station.id === "fido"
        Layout.fillWidth: true
        Layout.preferredHeight: 200
        title: qsTr("Route length vs. population grid size")
        xLog: true
        yLog: true
        showPoints: true
        fill: false
        points: Mission.resolution.filter(r => r.arcSeconds > 0).map(r => ({ x: r.arcSeconds / 3600, y: r.tourM / 1000 }))
        xFormat: v => v >= 0.999 ? Fmt.num(v) + "°" : (v >= 1 / 60 ? Fmt.num(v * 60) + "′" : Fmt.num(v * 3600) + "″")
        yFormat: v => Fmt.num(v) + " km"
    }
    Label {
        visible: detail.station.id === "fido"
        Layout.fillWidth: true
        text: {
            const r = Mission.resolution;
            if (!r.length) return "";
            const fine = r[0], uniform = r[r.length - 1];
            return qsTr("Spreading households evenly over all land would give %1 km; working cell by cell on the ~1 km grid gives %2 km. Real towns cluster, and clustered houses are quicker to tour.")
                .arg(Fmt.num(uniform.tourM / 1000)).arg(Fmt.num(fine.tourM / 1000));
        }
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }
    HopProfile {
        visible: detail.station.id === "retro"
        Layout.fillWidth: true
        hopText: Mission.summary.text?.hop ?? ""
        timeText: Mission.summary.text?.hopTime ?? ""
        peakText: Mission.summary.text?.hopPeak ?? ""
        accelText: Mission.summary.text?.stopGoG ?? ""
    }
    LogBarChart {
        visible: detail.station.bars.length > 0
        Layout.fillWidth: true
        title: detail.station.barTitle
        unit: detail.station.barUnit
        bars: detail.station.bars
    }

    // The calculation, step by step ----------------------------------------------------------------
    Label {
        visible: detail.station.steps.length > 0
        text: qsTr("THE CALCULATION")
        color: Theme.textFaint
        font.pixelSize: 11
        font.letterSpacing: 1.5
        font.weight: Font.Medium
    }
    Repeater {
        model: detail.station.steps
        delegate: Rectangle {
            id: stepCard
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: stepLayout.implicitHeight + 2 * Theme.s3
            radius: 8
            color: Theme.panelRaised
            border.color: Theme.line

            ColumnLayout {
                id: stepLayout
                anchors.fill: parent
                anchors.margins: Theme.s3
                spacing: Theme.s2

                Label {
                    Layout.fillWidth: true
                    text: stepCard.modelData.label
                    color: Theme.text
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.Bold
                    wrapMode: Text.WordWrap
                }
                Formula {
                    Layout.fillWidth: true
                    formulaId: detail.station.id + "-" + stepCard.modelData.id
                    plain: stepCard.modelData.plain
                }
                Label {
                    Layout.fillWidth: true
                    visible: stepCard.modelData.substitution.length > 0
                    text: stepCard.modelData.substitution
                    color: Theme.textMuted
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    text: "= " + stepCard.modelData.result
                    color: Theme.goldBright
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontLead
                    font.weight: Font.Bold
                    wrapMode: Text.WordWrap
                    visible: stepCard.modelData.result !== "binding"
                }
                Rectangle {
                    Layout.fillWidth: true
                    visible: stepCard.modelData.validity.length > 0
                    implicitHeight: validity.implicitHeight + 2 * Theme.s2
                    radius: 6
                    color: Qt.alpha(Theme.amber, 0.12)
                    border.color: Qt.alpha(Theme.amber, 0.5)
                    Label {
                        id: validity
                        anchors.fill: parent
                        anchors.margins: Theme.s2
                        text: "⚠ " + stepCard.modelData.validity
                        color: Theme.amber
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
                Flow {
                    Layout.fillWidth: true
                    visible: stepCard.modelData.sources.length > 0
                    spacing: 6
                    Repeater {
                        model: stepCard.modelData.sources
                        delegate: SourceChip {
                            required property string modelData
                            source: detail.sourceMap[modelData]
                        }
                    }
                }
            }
        }
    }

    // How absurd is this? --------------------------------------------------------------------------
    Label {
        visible: detail.station.comparisons.length > 0
        text: detail.station.id === "flight" ? qsTr("THE POLL") : qsTr("FOR SCALE")
        color: Theme.textFaint
        font.pixelSize: 11
        font.letterSpacing: 1.5
        font.weight: Font.Medium
    }
    Repeater {
        model: detail.station.comparisons
        delegate: Label {
            required property string modelData
            Layout.fillWidth: true
            text: "•  " + modelData
            color: Theme.text
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }

    // Try it ----------------------------------------------------------------------------------------
    Label {
        visible: detail.station.params.length > 0
        text: qsTr("TRY IT")
        color: Theme.textFaint
        font.pixelSize: 11
        font.letterSpacing: 1.5
        font.weight: Font.Medium
    }
    Repeater {
        model: detail.station.params
        delegate: ParamSlider {
            required property string modelData
            Layout.fillWidth: true
            param: detail.paramMap[modelData]
        }
    }

    Label {
        Layout.fillWidth: true
        visible: detail.station.caveat.length > 0
        text: qsTr("Caveat: ") + detail.station.caveat
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        font.italic: true
        wrapMode: Text.WordWrap
    }
}
