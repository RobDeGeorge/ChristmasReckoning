// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import ChristmasReckoning

// Slide-in panel for a console's write-up, the scenario, or the sources. Full screen on phones.
Drawer {
    id: drawer
    property string mode: "console"   // "console" | "scenario" | "sources"
    property var station: null

    edge: Qt.RightEdge
    width: parent ? (parent.width < 760 ? parent.width : Math.min(680, parent.width * 0.56)) : 600
    height: parent ? parent.height : 600
    modal: true
    dim: true
    interactive: opened
    Overlay.modal: Rectangle { color: Qt.alpha(Theme.sky, 0.6) }

    background: Rectangle {
        color: Theme.panel
        Rectangle { width: 1; height: parent.height; color: Theme.lineStrong }
    }

    function showConsole(c) {
        station = c;
        mode = "console";
        flick.contentY = 0;
        open();
    }
    function showScenario() {
        mode = "scenario";
        flick.contentY = 0;
        open();
    }
    function showSources() {
        mode = "sources";
        flick.contentY = 0;
        open();
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: loader.height + 2 * Theme.s5
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Loader {
            id: loader
            x: Theme.s5
            y: Theme.s5
            width: flick.width - 2 * Theme.s5
            sourceComponent: drawer.mode === "scenario" ? scenarioComponent
                           : drawer.mode === "sources" ? sourcesComponent : consoleComponent
        }
    }

    Component {
        id: consoleComponent
        ConsoleDetail {
            station: drawer.station ?? ({ id: "", callsign: "", title: "", question: "", go: false, status: "",
                                           readouts: [], steps: [], comparisons: [], params: [], caveat: "",
                                           bars: [], barTitle: "", barUnit: "" })
            onCloseRequested: drawer.close()
        }
    }
    Component {
        id: scenarioComponent
        ScenarioDetail { onCloseRequested: drawer.close() }
    }
    Component {
        id: sourcesComponent
        SourcesDetail { onCloseRequested: drawer.close() }
    }
}
