// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// North Pole Mission Control.
//
// Two layouts, chosen by the window's shape:
//  - side by side (laptops, desktops, landscape tablets): globe and timeline on the left, a
//    scrolling column of telemetry and consoles on the right; everything fits on one screen.
//  - stacked (phones, portrait tablets): the globe on top in its own touch area (so rotating it never
//    fights page scrolling), with telemetry, timeline and consoles scrolling underneath.
Item {
    id: page
    signal back()

    readonly property bool sideBySide: width >= 900 && width > height * 1.15
    readonly property bool phone: width < 600

    // Simulation clock --------------------------------------------------------------------------------
    property bool followSanta: true
    focus: true

    QtObject {
        id: simClock
        property bool playing: false
        property real speed: 1
        property bool live: false
        readonly property real startMs: Mission.launchMs - 30 * 60000
        readonly property real endMs: Mission.landingMs + 30 * 60000
        readonly property real nightSpeed: Math.max(1, (endMs - startMs) / 60000)
    }

    function startPreview() {
        const now = Platform.nowMs();
        if (now >= simClock.startMs && now <= simClock.endMs) {
            simClock.live = true;
            simClock.speed = 1;
            Mission.timeMs = now;
            simClock.playing = true;
        } else {
            simClock.live = false;
            Mission.timeMs = Mission.launchMs;
            simClock.speed = simClock.nightSpeed;
            simClock.playing = !Platform.reducedMotion;
        }
    }

    FrameAnimation {
        running: simClock.playing && page.visible
        onTriggered: {
            if (simClock.live) {
                Mission.timeMs = Platform.nowMs();
                return;
            }
            let t = Mission.timeMs + frameTime * 1000 * simClock.speed;
            if (t >= simClock.endMs) {
                t = simClock.endMs;
                simClock.playing = false;
            }
            Mission.timeMs = t;
        }
    }

    // Keep the camera on Santa unless the visitor takes over.
    FrameAnimation {
        running: page.followSanta && page.visible
        onTriggered: {
            const s = Mission.state;
            if (!s.phase || s.phase === "prelaunch" || s.phase === "landed")
                return;
            const k = Math.min(1, frameTime * 2.5);
            let dLon = s.longitude - globe.centerLonDeg;
            dLon = ((dLon + 540) % 360) - 180;
            globe.centerLonDeg += dLon * k;
            if (!globe.mapMode)
                globe.centerLatDeg += (Math.max(-60, Math.min(60, s.latitude)) - globe.centerLatDeg) * k;
        }
    }

    Component.onCompleted: {
        startPreview();
        openFromRoute();
    }

    function openFromRoute() {
        const id = Platform.subRoute;
        if (!id.length) {
            if (panel.opened && panel.mode === "console")
                panel.close();
            return;
        }
        if (id === "scenario") { panel.showScenario(); return; }
        if (id === "sources") { panel.showSources(); return; }
        const i = Mission.consoleIndex(id);
        if (i >= 0)
            panel.showConsole(Mission.consoles[i]);
    }
    Connections {
        target: Platform
        function onRouteChanged() { page.openFromRoute(); }
    }
    Connections {
        target: Mission
        // Keep an open console's numbers live as the scenario changes.
        function onResultsChanged() {
            if (panel.mode === "console" && panel.station) {
                const i = Mission.consoleIndex(panel.station.id);
                if (i >= 0) panel.station = Mission.consoles[i];
            }
        }
    }

    function openConsole(c) { Platform.navigate("mission-control/" + c.id); }

    Rectangle { anchors.fill: parent; color: Theme.sky }

    // Header -----------------------------------------------------------------------------------------
    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: page.phone ? 52 : 60
        color: Theme.panel
        z: 2
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.s3
            anchors.rightMargin: Theme.s3
            spacing: Theme.s2

            PillButton {
                text: page.phone ? "←" : qsTr("← Countdown")
                Accessible.name: qsTr("Back to the countdown")
                onClicked: page.back()
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Label {
                    Layout.fillWidth: true
                    text: page.phone ? qsTr("MISSION CONTROL") : qsTr("NORTH POLE MISSION CONTROL")
                    color: Theme.text
                    font.family: Theme.monoFamily
                    font.pixelSize: page.phone ? 14 : 17
                    font.weight: Font.Bold
                    font.letterSpacing: 2
                    elide: Text.ElideRight
                    Accessible.role: Accessible.Heading
                }
                Label {
                    Layout.fillWidth: true
                    visible: !page.phone
                    text: qsTr("Christmas Eve %1 · every child under 18 · the real numbers").arg(Mission.year)
                    color: Theme.textFaint
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }
            PillButton {
                text: page.phone ? "⚙" : qsTr("Scenario")
                Accessible.name: qsTr("Scenario settings")
                onClicked: Platform.navigate("mission-control/scenario")
            }
            PillButton {
                visible: !page.phone
                text: qsTr("Sources")
                onClicked: Platform.navigate("mission-control/sources")
            }
        }
    }

    // Globe area -------------------------------------------------------------------------------------
    Item {
        id: globeArea
        anchors.top: header.bottom
        anchors.left: parent.left
        width: page.sideBySide ? Math.round(parent.width * 0.6) : parent.width
        height: page.sideBySide ? parent.height - header.height - timelineHolder.height
                                : Math.round((parent.height - header.height) * (page.phone ? 0.46 : 0.5))
        clip: true

        Globe {
            id: globe
            anchors.fill: parent
            distance: page.phone ? 2.7 : 2.9
            fovDeg: 34
            timeMs: Mission.timeMs
            showDeliveries: true
            frontierZone: Mission.frontierZone + 1
            frontierRank: Mission.frontierRank
            gridAlpha: 0.12
            onUserInteracted: page.followSanta = false
        }

        SantaMarker {
            anchors.fill: parent
            globe: globe
            showLabel: !page.phone || globe.width > 360
        }

        Column {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.s3
            spacing: Theme.s2
            PillButton {
                text: globe.mapMode ? qsTr("Globe") : qsTr("Ground track")
                Accessible.name: globe.mapMode ? qsTr("Show the globe") : qsTr("Show the ground-track map")
                onClicked: globe.mapMode = !globe.mapMode
            }
            PillButton {
                text: page.followSanta ? qsTr("Following") : qsTr("Follow Santa")
                selected: page.followSanta
                onClicked: page.followSanta = !page.followSanta
            }
        }

        Rectangle { // legend
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: Theme.s3
            visible: globeArea.height > 260
            width: legend.implicitWidth + 16
            height: legend.implicitHeight + 10
            radius: 6
            color: Theme.scrim
            Row {
                id: legend
                anchors.centerIn: parent
                spacing: 12
                Repeater {
                    model: [
                        { color: Theme.gold, text: qsTr("delivered") },
                        { color: Theme.red, text: qsTr("delivering now") },
                        { color: Theme.telemetry, text: qsTr("still waiting") }
                    ]
                    delegate: Row {
                        required property var modelData
                        spacing: 5
                        Rectangle { width: 9; height: 9; radius: 2; color: modelData.color; anchors.verticalCenter: parent.verticalCenter }
                        Label { text: modelData.text; color: Theme.textMuted; font.pixelSize: 11 }
                    }
                }
            }
        }
    }

    // Timeline under the globe when side by side (the stacked layout has its own copy in the column).
    Item {
        id: timelineHolder
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: globeArea.width
        height: page.sideBySide ? sideTimeline.implicitHeight + 2 * Theme.s3 : 0
        visible: page.sideBySide
        TimelineBar {
            id: sideTimeline
            x: Theme.s3
            y: Theme.s3
            width: parent.width - 2 * Theme.s3
            clock: simClock
        }
    }

    // Scrolling column: telemetry and the consoles ----------------------------------------------------
    Flickable {
        id: scroller
        anchors.top: page.sideBySide ? header.bottom : globeArea.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: page.sideBySide ? parent.width - globeArea.width : parent.width
        contentWidth: width
        contentHeight: stackedColumn.implicitHeight + 2 * Theme.s3
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: stackedColumn
            x: Theme.s3
            y: Theme.s3
            width: scroller.width - 2 * Theme.s3
            spacing: Theme.s3

            TimelineBar {
                Layout.fillWidth: true
                visible: !page.sideBySide
                clock: simClock
            }

            Rectangle { // feasibility banner
                Layout.fillWidth: true
                visible: !(Mission.summary.feasible ?? true)
                implicitHeight: banner.implicitHeight + 2 * Theme.s3
                radius: Theme.radius
                color: Qt.alpha(Theme.nogo, 0.12)
                border.color: Qt.alpha(Theme.nogo, 0.5)
                Label {
                    id: banner
                    anchors.fill: parent
                    anchors.margins: Theme.s3
                    text: Mission.summary.feasibilityNote ?? ""
                    color: Theme.text
                    wrapMode: Text.WordWrap
                }
            }

            Telemetry {
                Layout.fillWidth: true
                compact: page.phone
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s2
                Label {
                    Layout.fillWidth: true
                    text: qsTr("FLIGHT CONTROLLERS")
                    color: Theme.textFaint
                    font.pixelSize: 11
                    font.letterSpacing: 1.5
                    font.weight: Font.Medium
                }
                Label {
                    text: qsTr("tap a console for the full working")
                    color: Theme.textFaint
                    font.pixelSize: 11
                    visible: !page.phone
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: page.phone ? 2 : (stackedColumn.width > 760 ? 3 : 2)
                columnSpacing: Theme.s3
                rowSpacing: Theme.s3
                Repeater {
                    model: Mission.consoles
                    delegate: ConsoleTile {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        station: modelData
                        implicitHeight: page.phone ? 120 : 132
                        onClicked: page.openConsole(modelData)
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s3
                text: qsTr("Population and household data: UN WPP 2024, UN Household Size & Composition 2026, GHSL 2025 · Time zones: tzdata, timezone-boundary-builder (ODbL) · Imagery: NASA. All numbers are computed live in your browser.")
                color: Theme.textFaint
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            PillButton {
                visible: page.phone
                text: qsTr("Sources & licences")
                onClicked: Platform.navigate("mission-control/sources")
            }
        }
    }

    SidePanel {
        id: panel
        parent: page
        onClosed: {
            if (Platform.subRoute.length)
                Platform.navigate("mission-control");
        }
    }

    Keys.onSpacePressed: simClock.playing = !simClock.playing
}
