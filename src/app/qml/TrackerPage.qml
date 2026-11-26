// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import ChristmasReckoning
import "js/calendar.js" as Calendar

// The whole site: a globe with Santa's flight path, and one message.
//  - before Christmas Eve: a countdown to liftoff, a preview of the route, and a calendar reminder;
//  - on the night: Santa's real position, live;
//  - afterwards: a replay, until the countdown to next year takes over.
//
// Wide screens put the words on the left and the globe on the right; tall ones stack them.
Item {
    id: page

    readonly property bool wide: width >= 720 && width > height * 1.2
    readonly property bool small: wide ? height < 520 : height < 700

    // Clock ------------------------------------------------------------------------------------------
    CountdownClock {
        id: clock
        onChanged: {
            const lead = isChristmas ? qsTr("Merry Christmas!")
                       : days > 0 ? (days === 1 ? qsTr("1 day until Christmas") : qsTr("%1 days until Christmas").arg(days))
                       : qsTr("%1h %2m until Christmas").arg(hours).arg(minutes);
            Platform.setDocumentTitle(lead + " · " + qsTr("Christmas Reckoning"));
        }
    }
    readonly property real nowMs: clock.nowMs

    // Which flight: this year's until the end of Boxing Day (UTC), then next year's. The flight itself
    // runs from the morning of 24 Dec to the evening of 25 Dec, UTC.
    Binding { target: Mission; property: "year"; value: new Date(page.nowMs + 5 * 86400000).getUTCFullYear() }

    // Live from two hours before liftoff until an hour after landing.
    readonly property string stage: nowMs < Mission.launchMs - 2 * 3600000 ? "before"
                                  : nowMs <= Mission.landingMs + 3600000 ? "live" : "after"
    readonly property bool live: stage === "live"

    // Santa in real life (the globe may be showing a preview or a replay instead).
    readonly property var santaNow: { page.nowMs; return Mission.stateAt(Platform.nowMs()); }
    readonly property string phaseNow: santaNow.phase ?? "prelaunch"
    readonly property bool waiting: stage === "before" || (live && phaseNow === "prelaunch")
    readonly property bool home: stage === "after" || (live && phaseNow === "landed")
    readonly property bool flying: live && !waiting && !home

    readonly property var zone: Mission.zoneForOffset(Platform.utcOffsetMinutes(Mission.referenceMs))
    readonly property string zoneText: !zone.startMs || home ? ""
        : nowMs < zone.startMs ? qsTr("He reaches you around %1.")   // the date stays on one line
                                     .arg(Platform.localDateTime(zone.startMs).replace(/ /g, "\u00a0"))
        : nowMs <= zone.endMs ? qsTr("He's over your time zone right now!")
        : qsTr("He's already visited your time zone.")

    // What the globe shows ---------------------------------------------------------------------------
    // Live: the real moment. Otherwise the whole night on a loop (or a still, for reduced motion).
    property real loop: 0
    readonly property real loopSeconds: 45
    onStageChanged: {
        loop = 0;
        advance(0);
    }
    function advance(seconds) {
        if (page.live) {
            Mission.timeMs = Platform.nowMs();
            return;
        }
        if (Platform.reducedMotion) {
            Mission.timeMs = page.stage === "after" ? Mission.landingMs : Mission.launchMs;
            return;
        }
        page.loop += seconds / page.loopSeconds;
        if (page.loop > 1.1)   // rest at the North Pole for a moment, then go again
            page.loop = 0;
        Mission.timeMs = Mission.launchMs + Math.min(1, page.loop) * (Mission.landingMs - Mission.launchMs);
    }
    FrameAnimation {
        running: page.visible
        onTriggered: page.advance(frameTime)
    }

    // Keep Santa in view. A drag hands the globe to the visitor for a few seconds.
    property bool following: true
    Timer { id: refollow; interval: 6000; onTriggered: page.following = true }
    FrameAnimation {
        running: page.following && page.visible
        onTriggered: {
            const s = Mission.state;
            if (s.longitude === undefined)
                return;
            // The preview covers a day and a half in under a minute, so there the globe just turns
            // steadily with him instead of chasing every swerve.
            const k = Math.min(1, frameTime * (page.live ? 2.5 : 1.2));
            let dLon = s.longitude - globe.centerLonDeg;
            dLon = ((dLon + 540) % 360) - 180;
            globe.centerLonDeg += dLon * k;
            const lat = Math.max(-55, Math.min(55, s.latitude)) * (page.live ? 1 : 0.6);
            globe.centerLatDeg += (lat - globe.centerLatDeg) * k;
        }
    }

    // Lets the smoke tests check the tracker without reading pixels.
    function publish() {
        Platform.publishState("route", Platform.route);
        Platform.publishState("tracker", {
            live: page.live, mode: page.live ? "live" : (page.stage === "after" ? "replay" : "preview"),
            phase: Mission.state.phase ?? "", timeMs: Mission.timeMs });
    }
    onNowMsChanged: publish()
    Component.onCompleted: {
        advance(0);
        publish();
    }
    Connections {
        target: Platform
        function onRouteChanged() { page.publish(); }
    }

    function saveReminder() {
        const url = "https://www.christmasreckoning.com/#/live";
        const events = [{
            uid: "santa-launch-" + Mission.year + "@christmasreckoning.com",
            startMs: Mission.launchMs,
            endMs: Mission.launchMs + 30 * 60000,
            summary: qsTr("Santa takes off - watch him live"),
            description: qsTr("Track Santa live: %1").arg(url),
            url: url,
            alarmMinutes: 15
        }];
        if (zone.startMs) {
            events.push({
                uid: "santa-your-zone-" + Mission.year + "@christmasreckoning.com",
                startMs: zone.startMs,
                endMs: zone.endMs,
                summary: qsTr("Santa is in your time zone"),
                description: qsTr("Watch him live: %1").arg(url),
                url: url,
                alarmMinutes: 15
            });
        }
        Platform.downloadText("santa-tracker-" + Mission.year + ".ics", Calendar.ics(events), "text/calendar");
        saved.restart();
    }
    Timer { id: saved; interval: 4000 }

    // Layout -----------------------------------------------------------------------------------------
    readonly property real leftWidth: wide ? Math.max(360, Math.min(560, width * 0.42)) : width
    readonly property real textWidth: Math.min(leftWidth - 2 * Theme.s5, 520)
    readonly property real gap: small ? Theme.s4 : Theme.s6
    readonly property real footerHeight: 40

    readonly property real globeX: wide ? leftWidth + (width - leftWidth) / 2 - Theme.s3 : width / 2
    readonly property real globeTop: wide ? 0 : words.y + words.height
    readonly property real globeBottom: wide ? height : action.y
    readonly property real globeY: (globeTop + globeBottom) / 2
    readonly property real globeRadius: Math.max(80, wide ? Math.min((width - leftWidth) * 0.46, height * 0.43)
                                                          : Math.min(width * 0.46, (globeBottom - globeTop) / 2 - Theme.s4))

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.sky }
            GradientStop { position: 1.0; color: Theme.skyHigh }
        }
    }

    Globe {
        id: globe
        anchors.fill: parent
        fovDeg: 30
        gridAlpha: 0.06
        centerLatDeg: 20
        centerLonDeg: 170
        timeMs: Mission.timeMs
        showDeliveries: true
        frontierZone: Mission.frontierZone + 1
        frontierRank: Mission.frontierRank
        offsetX: page.globeX - width / 2
        offsetY: page.globeY - height / 2
        // The camera distance that draws the disc at the radius the layout wants.
        distance: 1 / Math.sin(Math.atan(page.globeRadius * Math.tan(fovDeg * Math.PI / 360) / (height / 2)))
        onUserInteracted: {
            page.following = false;
            refollow.restart();
        }
    }

    FlightOverlay {
        anchors.fill: parent
        globe: globe
        // Live, the bright tail is the last few minutes; in the sped-up loop, the same share of the screen.
        recentMs: page.live ? 12 * 60000 : (Mission.landingMs - Mission.launchMs) / 50
    }

    Tag {
        x: page.globeX - width / 2
        y: page.globeY + page.globeRadius - height - Theme.s4
        text: page.live ? qsTr("LIVE") : page.stage === "after" ? qsTr("REPLAY") : qsTr("PREVIEW")
        tint: page.live ? Theme.nogo : Theme.amber
        pulsing: page.live
    }

    // The words --------------------------------------------------------------------------------------
    Column {
        id: words
        width: page.textWidth
        x: (page.leftWidth - width) / 2
        y: page.wide ? (page.height - height - page.gap - action.height) / 2
                     : (page.small ? Theme.s5 : Theme.s7)
        spacing: page.small ? Theme.s2 : Theme.s3

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("CHRISTMAS RECKONING")
            color: Theme.gold
            font.pixelSize: 12
            font.letterSpacing: 3
            font.weight: Font.Medium
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: page.waiting ? qsTr("Santa takes off in")
                : page.home ? qsTr("Santa is home. Merry Christmas!")
                : page.phaseNow === "returning" ? qsTr("Santa is heading home")
                : qsTr("Santa is over %1").arg(page.santaNow.country || (page.santaNow.zoneCountries ?? [])[0] || qsTr("the sea"))
            color: Theme.text
            font.family: Theme.displayFamily
            font.pixelSize: page.small ? 26 : 34
            Accessible.role: Accessible.Heading
        }

        Row { // countdown to liftoff
            id: digits
            anchors.horizontalCenter: parent.horizontalCenter
            visible: page.waiting
            spacing: Math.min(Theme.s5, page.textWidth / 18)
            readonly property int seconds: Math.max(0, Math.ceil((Mission.launchMs - page.nowMs) / 1000))
            readonly property int size: Math.max(34, Math.min(page.small ? 44 : 64, page.textWidth / 7.2))
            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("%1 days, %2 hours, %3 minutes and %4 seconds until Santa takes off")
                .arg(Math.floor(seconds / 86400)).arg(Math.floor(seconds / 3600) % 24)
                .arg(Math.floor(seconds / 60) % 60).arg(seconds % 60)

            Repeater {
                model: [
                    { value: Math.floor(digits.seconds / 86400), label: qsTr("DAYS"), pad: false },
                    { value: Math.floor(digits.seconds / 3600) % 24, label: qsTr("HOURS"), pad: true },
                    { value: Math.floor(digits.seconds / 60) % 60, label: qsTr("MINUTES"), pad: true },
                    { value: digits.seconds % 60, label: qsTr("SECONDS"), pad: true }
                ]
                delegate: Column {
                    required property var modelData
                    required property int index
                    visible: index > 0 || modelData.value > 0
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.pad ? String(modelData.value).padStart(2, "0") : String(modelData.value)
                        color: Theme.text
                        font.family: Theme.monoFamily
                        font.pixelSize: digits.size
                        font.weight: Font.Bold
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.label
                        color: Theme.textFaint
                        font.pixelSize: 11
                        font.letterSpacing: 1.5
                    }
                }
            }
        }

        Column { // presents so far
            anchors.horizontalCenter: parent.horizontalCenter
            visible: page.flying
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: page.santaNow.presents ?? "0"
                color: Theme.goldBright
                font.family: Theme.monoFamily
                font.pixelSize: Math.max(26, Math.min(page.small ? 34 : 44, page.textWidth / 9))
                font.weight: Font.Bold
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("PRESENTS DELIVERED")
                color: Theme.textFaint
                font.pixelSize: 11
                font.letterSpacing: 1.5
            }
        }
    }

    // The nudge to come back -------------------------------------------------------------------------
    Column {
        id: action
        width: page.textWidth
        x: (page.leftWidth - width) / 2
        y: page.wide ? words.y + words.height + page.gap
                     : page.height - height - page.footerHeight - (page.small ? 0 : Theme.s3)
        spacing: page.small ? Theme.s2 : Theme.s3

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: !page.flying
            text: page.stage === "before" ? qsTr("Come back on Christmas Eve to watch him fly, live.")
                : page.waiting ? qsTr("Stay right here. Liftoff is live.")
                : qsTr("See you next Christmas Eve.")
            color: Theme.text
            font.pixelSize: page.small ? 15 : 17
            font.weight: Font.Medium
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: text.length > 0
            text: page.zoneText
            color: page.flying ? Theme.goldBright : Theme.textMuted
            font.pixelSize: page.flying ? (page.small ? 15 : 17) : 14
            font.weight: page.flying ? Font.Medium : Font.Normal
        }

        ActionButton {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: page.stage === "before"
            text: saved.running ? qsTr("Saved to your calendar") : qsTr("Remind me")
            Accessible.description: qsTr("Downloads a calendar reminder for Santa's liftoff and his visit to your time zone")
            onClicked: page.saveReminder()
        }
    }

    Row {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.s3
        x: page.wide ? page.width - width - Theme.s4 : (page.width - width) / 2
        spacing: Theme.s4
        Repeater {
            model: [
                { text: qsTr("Credits"), url: Platform.sourceUrl + "/blob/master/THIRD_PARTY_NOTICES.md" },
                { text: qsTr("Source"), url: Platform.sourceUrl }
            ]
            delegate: Text {
                required property var modelData
                text: modelData.text
                color: Theme.textFaint
                font.pixelSize: 12
                font.underline: linkHover.hovered
                Accessible.role: Accessible.Link
                Accessible.name: text
                Accessible.onPressAction: Qt.openUrlExternally(modelData.url)
                HoverHandler { id: linkHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { margin: 8; onTapped: Qt.openUrlExternally(modelData.url) }
            }
        }
    }
}
