// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Window
import ChristmasReckoning

Window {
    id: window

    width: 1280
    height: 800
    visible: true
    visibility: Platform.web ? Window.FullScreen : Window.AutomaticVisibility
    title: qsTr("Christmas Reckoning")
    color: Theme.sky

    TrackerPage { anchors.fill: parent }

    property bool readyAnnounced: false
    onFrameSwapped: {
        if (readyAnnounced)
            return;
        readyAnnounced = true;
        Platform.notifyReady();
    }
}
