// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning

// Every dataset, constant and yardstick behind the numbers, plus licences and the source code.
ColumnLayout {
    id: sourcesView
    signal closeRequested()
    spacing: Theme.s3

    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("SOURCES & ASSUMPTIONS")
            color: Theme.text
            font.family: Theme.monoFamily
            font.pixelSize: 20
            font.weight: Font.Bold
            font.letterSpacing: 1.5
            wrapMode: Text.WordWrap
        }
        PillButton { text: qsTr("Close"); onClicked: sourcesView.closeRequested() }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Population: %1 time zones and %2 countries, for Christmas Eve %3 (%4). Every formula, input and result is shown in the consoles; nothing is hidden or hand-tuned.")
            .arg(Mission.dataInfo.zones ?? 0).arg(Mission.dataInfo.countries ?? 0)
            .arg(Mission.dataInfo.dataYear ?? "").arg(Mission.dataInfo.tzVersion ?? "")
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
        wrapMode: Text.WordWrap
    }

    RowLayout {
        spacing: Theme.s2
        PillButton {
            text: qsTr("Source code (GPL-3.0)")
            onClicked: Qt.openUrlExternally(Platform.sourceUrl)
        }
        PillButton {
            text: qsTr("Licences & credits")
            onClicked: Qt.openUrlExternally(Platform.sourceUrl + "/blob/master/THIRD_PARTY_NOTICES.md")
        }
    }

    Repeater {
        model: Mission.sources
        delegate: Rectangle {
            id: card
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: col.implicitHeight + 2 * Theme.s3
            radius: 8
            color: Theme.panelRaised
            border.color: Theme.line
            ColumnLayout {
                id: col
                anchors.fill: parent
                anchors.margins: Theme.s3
                spacing: 3
                Label {
                    Layout.fillWidth: true
                    text: card.modelData.title
                    color: Theme.text
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.Bold
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    text: card.modelData.publisher
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    text: card.modelData.note
                    color: Theme.textFaint
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
                SourceChip {
                    visible: card.modelData.url.length > 0
                    source: ({ key: card.modelData.key, title: card.modelData.url, publisher: "",
                               url: card.modelData.url, note: "" })
                }
            }
        }
    }
}
