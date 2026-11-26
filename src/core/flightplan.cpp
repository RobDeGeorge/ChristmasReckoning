// SPDX-License-Identifier: GPL-3.0-or-later
#include "flightplan.h"

#include "schedule.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace cr {
namespace {

constexpr double kDegToRad = std::numbers::pi / 180.0;
constexpr double kEarthRadiusM = 6'371'008.8;
// Children are asleep from 22:00 on Christmas Eve to 06:00 on Christmas morning, local time.
constexpr double kWindowOpensS = -2 * 3600.0; // relative to local midnight
constexpr double kWindowLengthS = 8 * 3600.0;
// Beardwood–Halton–Hammersley constant: the shortest tour through N random points in area A is
// about β·√(N·A). The world data carries Σ√(N·A) for each zone.
constexpr double kBhhBeta = 0.7124;

std::int64_t daysFromCivil(int y, unsigned m, unsigned d)
{
    // Howard Hinnant's algorithm (proleptic Gregorian calendar).
    y -= m <= 2;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = unsigned(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + std::int64_t(doe) - 719468;
}

double greatCircleM(double lat1, double lon1, double lat2, double lon2)
{
    const double p1 = lat1 * kDegToRad, p2 = lat2 * kDegToRad;
    const double dp = p2 - p1, dl = (lon2 - lon1) * kDegToRad;
    const double a = std::sin(dp / 2) * std::sin(dp / 2)
        + std::cos(p1) * std::cos(p2) * std::sin(dl / 2) * std::sin(dl / 2);
    return 2 * kEarthRadiusM * std::asin(std::min(1.0, std::sqrt(a)));
}

} // namespace

double christmasUtcEpochMs(int year)
{
    return double(daysFromCivil(year, 12, 25)) * 86'400'000.0;
}

FlightPlan planFlight(const WorldData &world, int year)
{
    FlightPlan plan;
    plan.referenceEpochMs = christmasUtcEpochMs(year);

    const std::size_t zoneCount = world.zones.size();
    if (zoneCount == 0)
        return plan;

    std::vector<ZoneJob> jobs(zoneCount);
    plan.zones.resize(zoneCount);
    for (std::size_t k = 0; k < zoneCount; ++k) {
        const Zone &zone = world.zones[k];
        ZonePlan &z = plan.zones[k];
        z.label = zone.label().toStdString();
        z.offsetMinutes = zone.offsetMinutes;
        z.households = zone.households;
        z.children = zone.children;
        z.pathM = kBhhBeta * zone.pathSumM + zone.gapM;
        z.transitionM = zone.transitionM;
        z.windowOpenS = kWindowOpensS - zone.offsetMinutes * 60.0;
        z.windowCloseS = z.windowOpenS + kWindowLengthS;
        for (std::uint16_t c : zone.topCountries) {
            if (c < world.countries.size() && z.countries.size() < 4)
                z.countries.push_back(world.countries[c].name.toStdString());
        }
        jobs[k] = {z.windowOpenS, z.windowCloseS, z.pathM + z.transitionM, 0.0};

        plan.households += z.households;
        plan.children += z.children;
        plan.distanceM += z.pathM + z.transitionM;
    }

    const ScheduleResult schedule = solveSchedule(jobs);
    double lastLat = 90, lastLon = 0;
    for (std::size_t k = 0; k < zoneCount; ++k) {
        ZonePlan &z = plan.zones[k];
        z.speedMps = schedule.rates[k];
        z.startS = schedule.startS[k];
        z.endS = schedule.endS[k];
        z.transitionTimeS = z.speedMps > 0 ? z.transitionM / z.speedMps : 0;
        z.deliveryTimeS = std::max(0.0, z.endS - z.startS - z.transitionTimeS);
        if (!world.zones[k].checkpoints.empty()) {
            lastLat = world.zones[k].checkpoints.back().latitudeDeg;
            lastLon = world.zones[k].checkpoints.back().longitudeDeg;
        }
    }
    plan.launchS = plan.zones.front().startS;
    plan.lastDeliveryS = plan.zones.back().endS;
    plan.returnHomeM = greatCircleM(lastLat, lastLon, 90, 0);
    plan.landingS = plan.lastDeliveryS + plan.returnHomeM / std::max(plan.zones.back().speedMps, 1.0);
    plan.distanceM += plan.returnHomeM;
    return plan;
}

} // namespace cr
