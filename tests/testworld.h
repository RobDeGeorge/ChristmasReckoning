// SPDX-License-Identifier: GPL-3.0-or-later
// A small synthetic world for tests: three time zones with hand-picked totals.
#pragma once

#include "worlddata.h"

#include <cmath>

namespace crtest {

inline cr::Zone makeZone(int offsetMinutes, double households, double pathSumM, double lat, double lon,
                         int checkpoints = 4)
{
    cr::Zone z;
    z.offsetMinutes = offsetMinutes;
    z.households = households;
    z.children = households * 2.2;
    z.population = households * 8;
    z.pathSumM = pathSumM;
    z.stopGoSum = std::pow(households, 0.75) * 1000;
    z.gapM = 1000;
    z.transitionM = 2.0e6;
    z.cellCount = std::uint32_t(checkpoints * 10);
    for (int i = 1; i <= checkpoints; ++i) {
        const float f = float(i) / float(checkpoints);
        cr::Checkpoint cp;
        cp.childrenFrac = f;
        cp.householdsFrac = f;
        cp.pathFrac = f;
        cp.stopGoFrac = f;
        cp.latitudeDeg = float(lat + i);
        cp.longitudeDeg = float(lon - i);
        cp.rank = std::uint32_t(i * 10);
        cp.country = 0;
        z.checkpoints.push_back(cp);
    }
    z.topCountries = {0};
    return z;
}

inline cr::WorldData makeWorld()
{
    cr::WorldData w;
    w.dataYear = 2026;
    w.tzVersion = QStringLiteral("test");
    w.countries.push_back({QStringLiteral("TST"), QStringLiteral("Testland"), 8e9, 2.4e9, 1.1e9, 4, 0.5, 2020, false});
    // East to west: UTC+10, UTC+0, UTC−5. The middle zone is the big one.
    w.zones.push_back(makeZone(600, 1.0e8, 1.0e10, -30, 150));
    w.zones.push_back(makeZone(0, 8.0e8, 6.0e10, 45, 5));
    w.zones.push_back(makeZone(-300, 2.0e8, 3.0e10, 35, -80));
    w.resolution.push_back({QStringLiteral("30″"), 30, 1.0e11});
    w.landAreaM2 = 1.49e14;
    return w;
}

} // namespace crtest
