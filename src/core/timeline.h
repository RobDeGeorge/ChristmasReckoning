// SPDX-License-Identifier: GPL-3.0-or-later
// Santa's state at any instant: a pure function of time, so the timeline can be scrubbed freely and
// every viewer sees the same thing at the same moment.
#pragma once

#include "analysis.h"
#include "worlddata.h"

namespace cr {

enum class Phase {
    PreLaunch,   // still at the North Pole
    Transit,     // flying between time zones (or out from the Pole)
    Delivering,
    Holding,     // ahead of schedule, waiting for the next zone's children to fall asleep
    Returning,   // flying home
    Landed
};

struct SantaState
{
    Phase phase = Phase::PreLaunch;
    int zone = -1;               // index into WorldData::zones, −1 before launch
    double latitudeDeg = 90;
    double longitudeDeg = 0;
    double headingDeg = 180;     // compass bearing of travel
    double speedMps = 0;
    double householdsDelivered = 0;
    double childrenDelivered = 0;
    double presentsDelivered = 0;
    double stopsMade = 0;
    double distanceM = 0;
    double massKg = 0;
    double dragEnergyJ = 0;
    double missionElapsedS = 0;  // since launch (negative before)
    double localSecondsOfDay = 0; // clock time where Santa is
    int countryIndex = -1;       // country under the sleigh, if known
    // Delivery frontier for the globe shader: cells of zones < frontierZone are done, and cells of
    // frontierZone with rank < frontierRank.
    int frontierZone = 0;
    double frontierRank = 0;
};

SantaState stateAt(const WorldData &world, const Results &results, double epochMs);

const char *phaseName(Phase phase);

} // namespace cr
