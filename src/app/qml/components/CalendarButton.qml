// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// Downloads a calendar file with two reminders: Santa's launch, and his visit to your time zone.
PillButton {
    id: button
    text: qsTr("Add to calendar")
    Accessible.name: qsTr("Add Santa's launch and his visit to your time zone to your calendar")

    onClicked: {
        const url = "https://www.christmasreckoning.com/#/live";
        const zone = Mission.zoneForOffset(Platform.utcOffsetMinutes(Mission.referenceMs));
        const events = [{
            uid: "santa-launch-" + Mission.year + "@christmasreckoning.com",
            startMs: Mission.launchMs,
            endMs: Mission.launchMs + 30 * 60000,
            summary: qsTr("Santa launches - watch him live"),
            description: qsTr("Track Santa live at North Pole Mission Control: %1").arg(url),
            url: url,
            alarmMinutes: 15
        }];
        if (zone.startMs) {
            events.push({
                uid: "santa-your-zone-" + Mission.year + "@christmasreckoning.com",
                startMs: zone.startMs,
                endMs: zone.endMs,
                summary: qsTr("Santa is over your time zone (%1)").arg(zone.label),
                description: qsTr("Santa reaches your time zone now. Watch live: %1").arg(url),
                url: url,
                alarmMinutes: 15
            });
        }
        Platform.downloadText("santa-tracker-" + Mission.year + ".ics", Fmt.calendar(events), "text/calendar");
    }
}
