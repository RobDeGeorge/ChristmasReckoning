// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// Horizontal bars on a logarithmic scale: Santa's numbers against familiar yardsticks.
Item {
    id: chart
    property var bars: []        // [{label, value, text, highlight}]
    property string title
    property string unit

    readonly property int rowHeight: 26
    readonly property int labelWidth: Math.min(200, width * 0.38)
    readonly property real lo: {
        let m = Infinity;
        for (const b of bars) if (b.value > 0) m = Math.min(m, b.value);
        return isFinite(m) ? Math.floor(Math.log10(m)) : 0;
    }
    readonly property real hi: {
        let m = 0;
        for (const b of bars) m = Math.max(m, b.value);
        return m > 0 ? Math.ceil(Math.log10(m)) : 1;
    }
    readonly property real span: Math.max(1, hi - lo)
    readonly property real trackWidth: Math.max(10, width - labelWidth - 8)

    implicitHeight: header.implicitHeight + bars.length * rowHeight + 24
    Accessible.role: Accessible.Chart
    Accessible.name: title + ", logarithmic scale: " + bars.map(b => b.label + " " + b.text).join("; ")

    Label {
        id: header
        text: chart.title + qsTr(" · logarithmic scale (each gridline is 10× the last)")
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        width: parent.width
        elide: Text.ElideRight
    }

    Item {
        id: plot
        anchors.top: header.bottom
        anchors.topMargin: 8
        width: parent.width
        height: chart.bars.length * chart.rowHeight

        // Decade gridlines (thinned out when there are many decades).
        Repeater {
            model: chart.span + 1
            delegate: Rectangle {
                required property int index
                readonly property int every: Math.ceil(chart.span / 8)
                visible: index % every === 0
                x: chart.labelWidth + 8 + index / chart.span * chart.trackWidth
                width: 1
                height: plot.height
                color: Theme.line
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.bottom
                    anchors.topMargin: 3
                    text: "10" + Fmt.superscript(chart.lo + index)
                    color: Theme.textFaint
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                }
            }
        }

        Repeater {
            model: chart.bars
            delegate: Item {
                required property var modelData
                required property int index
                y: index * chart.rowHeight
                width: plot.width
                height: chart.rowHeight

                Label {
                    width: chart.labelWidth
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label
                    color: modelData.highlight ? Theme.goldBright : Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    font.weight: modelData.highlight ? Font.Bold : Font.Normal
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                }
                Rectangle {
                    id: bar
                    x: chart.labelWidth + 8
                    anchors.verticalCenter: parent.verticalCenter
                    height: chart.rowHeight - 10
                    radius: 3
                    width: Math.max(2, (Math.log10(Math.max(modelData.value, Math.pow(10, chart.lo))) - chart.lo)
                                       / chart.span * chart.trackWidth)
                    color: modelData.highlight ? Theme.gold : Qt.alpha(Theme.telemetry, 0.45)
                }
                Label {
                    readonly property bool inside: bar.x + bar.width + 6 + implicitWidth > plot.width
                    anchors.verticalCenter: parent.verticalCenter
                    x: inside ? bar.x + bar.width - implicitWidth - 6 : bar.x + bar.width + 6
                    text: modelData.text
                    color: inside && modelData.highlight ? Theme.sky : (modelData.highlight ? Theme.goldBright : Theme.text)
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                    font.weight: inside && modelData.highlight ? Font.Bold : Font.Normal
                }
            }
        }
    }
}
