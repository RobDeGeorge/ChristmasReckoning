// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window
import ChristmasReckoning

ApplicationWindow {
    id: window

    width: 1280
    height: 800
    visible: true
    visibility: Platform.web ? Window.FullScreen : Window.AutomaticVisibility
    title: qsTr("Christmas Reckoning")
    color: Theme.sky
    font.family: Theme.sansFamily
    font.pixelSize: Theme.fontBody
    palette.toolTipBase: Theme.panelRaised
    palette.toolTipText: Theme.text

    // The simulation always describes the Christmas the countdown is counting towards.
    CountdownClock {
        id: appClock
        onChanged: {
            const page = Platform.page === "mission-control" ? qsTr("North Pole Mission Control")
                                                              : qsTr("Christmas Reckoning");
            const lead = isChristmas ? qsTr("Merry Christmas!")
                       : days > 0 ? (days === 1 ? qsTr("1 day until Christmas") : qsTr("%1 days until Christmas").arg(days))
                       : qsTr("%1h %2m until Christmas").arg(hours).arg(minutes);
            Platform.setDocumentTitle(lead + " · " + page);
        }
    }
    Binding { target: Mission; property: "year"; value: appClock.year }

    Component {
        id: countdownPage
        CountdownPage {
            onOpenMissionControl: Platform.navigate("mission-control")
        }
    }

    Component {
        id: missionControlPage
        MissionControlPage {
            onBack: Platform.navigate("")
        }
    }

    function componentFor(page) {
        return page === "mission-control" ? missionControlPage : countdownPage;
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: window.componentFor(Platform.page)
        // Deliberately not a binding: it records which page is on screen.
        property string shownPage
        Component.onCompleted: shownPage = Platform.page

        replaceEnter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Platform.reducedMotion ? 0 : 280; easing.type: Easing.OutCubic }
        }
        replaceExit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Platform.reducedMotion ? 0 : 200; easing.type: Easing.InCubic }
        }
    }

    Connections {
        target: Platform
        function onRouteChanged() {
            if (stack.shownPage !== Platform.page) {
                stack.shownPage = Platform.page;
                stack.replace(null, window.componentFor(Platform.page));
            }
            Platform.publishState("route", Platform.route);
        }
    }

    property bool readyAnnounced: false
    onFrameSwapped: {
        if (readyAnnounced)
            return;
        readyAnnounced = true;
        Platform.publishState("route", Platform.route);
        Platform.notifyReady();
    }
}
