// SPDX-License-Identifier: GPL-3.0-or-later
// Santa's timetable for the night: which time zone he works when, and how far he flies in each.
//
// He visits every household with children, one time zone after another, while the children there
// are asleep (22:00 to 06:00 local time). The scheduler picks the gentlest speeds that still get
// every zone finished inside its window.
#pragma once

#include "worlddata.h"

#include <string>
#include <vector>

namespace cr {

struct ZonePlan
{
    std::string label;
    int offsetMinutes = 0;
    double households = 0;
    double children = 0;        // one present each
    double pathM = 0;           // route inside the zone
    double transitionM = 0;     // hop in from the previous zone (or from the North Pole)
    double windowOpenS = 0;     // seconds relative to the reference epoch (25 Dec 00:00 UTC)
    double windowCloseS = 0;
    double startS = 0;
    double endS = 0;
    double transitionTimeS = 0;
    double deliveryTimeS = 0;
    double speedMps = 0;
    std::vector<std::string> countries; // biggest first
};

struct FlightPlan
{
    double referenceEpochMs = 0; // 25 December 00:00 UTC of the flight's year
    double households = 0;
    double children = 0;
    double distanceM = 0;        // everything flown, including the trip home
    double returnHomeM = 0;
    double launchS = 0;          // relative to the reference epoch
    double lastDeliveryS = 0;
    double landingS = 0;
    std::vector<ZonePlan> zones; // in flying order, matching WorldData::zones
};

// Milliseconds since the Unix epoch of 25 December 00:00 UTC of `year`.
double christmasUtcEpochMs(int year);

FlightPlan planFlight(const WorldData &world, int year);

} // namespace cr
