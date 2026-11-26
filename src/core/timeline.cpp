// SPDX-License-Identifier: GPL-3.0-or-later
#include "timeline.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cr {
namespace {

constexpr double kDeg = std::numbers::pi / 180.0;

struct LatLon
{
    double lat;
    double lon;
};

// Spherical linear interpolation along the great circle from a to b.
LatLon slerp(LatLon a, LatLon b, double t)
{
    const double la1 = a.lat * kDeg, lo1 = a.lon * kDeg, la2 = b.lat * kDeg, lo2 = b.lon * kDeg;
    const double x1 = std::cos(la1) * std::cos(lo1), y1 = std::cos(la1) * std::sin(lo1), z1 = std::sin(la1);
    const double x2 = std::cos(la2) * std::cos(lo2), y2 = std::cos(la2) * std::sin(lo2), z2 = std::sin(la2);
    const double dot = std::clamp(x1 * x2 + y1 * y2 + z1 * z2, -1.0, 1.0);
    const double omega = std::acos(dot);
    double x, y, z;
    if (omega < 1e-9) {
        x = x1 + (x2 - x1) * t;
        y = y1 + (y2 - y1) * t;
        z = z1 + (z2 - z1) * t;
    } else {
        const double s = std::sin(omega);
        const double wa = std::sin((1 - t) * omega) / s, wb = std::sin(t * omega) / s;
        x = wa * x1 + wb * x2;
        y = wa * y1 + wb * y2;
        z = wa * z1 + wb * z2;
    }
    return {std::atan2(z, std::hypot(x, y)) / kDeg, std::atan2(y, x) / kDeg};
}

LatLon position(const Checkpoint &cp)
{
    return {cp.latitudeDeg, cp.longitudeDeg};
}

LatLon zoneEnd(const WorldData &world, int zone)
{
    if (zone < 0)
        return {90, 0};
    return position(world.zones[std::size_t(zone)].checkpoints.back());
}

} // namespace

const char *phaseName(Phase phase)
{
    switch (phase) {
    case Phase::PreLaunch: return "prelaunch";
    case Phase::Transit: return "transit";
    case Phase::Delivering: return "delivering";
    case Phase::Holding: return "holding";
    case Phase::Returning: return "returning";
    case Phase::Landed: return "landed";
    }
    return "unknown";
}

SantaState stateAt(const WorldData &world, const FlightPlan &plan, double epochMs)
{
    SantaState state;
    const double t = (epochMs - plan.referenceEpochMs) / 1000.0;
    const auto &zones = plan.zones;
    const std::size_t n = std::min(zones.size(), world.zones.size());
    if (n == 0)
        return state;

    auto finishZonesBefore = [&](std::size_t k) {
        for (std::size_t j = 0; j < k; ++j) {
            state.householdsDelivered += zones[j].households;
            state.presentsDelivered += zones[j].children;
            state.distanceM += zones[j].pathM + zones[j].transitionM;
        }
    };

    if (t < zones.front().startS)
        return state;

    // Last zone whose work has started.
    std::size_t k = 0;
    while (k + 1 < n && zones[k + 1].startS <= t)
        ++k;
    const ZonePlan &z = zones[k];
    const Zone &wz = world.zones[k];
    state.zone = int(k);

    if (t >= z.endS) {
        finishZonesBefore(k + 1);
        state.frontierZone = int(k) + 1;
        const LatLon here = zoneEnd(world, int(k));
        if (k + 1 < n) {
            state.phase = Phase::Holding;
            state.latitudeDeg = here.lat;
            state.longitudeDeg = here.lon;
            return state;
        }
        // Heading home.
        const double homeTime = plan.landingS - plan.lastDeliveryS;
        const double u = homeTime > 0 ? std::clamp((t - z.endS) / homeTime, 0.0, 1.0) : 1.0;
        state.phase = u < 1 ? Phase::Returning : Phase::Landed;
        const LatLon p = slerp(here, {90, 0}, u);
        state.latitudeDeg = p.lat;
        state.longitudeDeg = p.lon;
        state.distanceM += plan.returnHomeM * u;
        return state;
    }

    finishZonesBefore(k);
    state.frontierZone = int(k);
    const double dt = t - z.startS;
    const LatLon from = zoneEnd(world, int(k) - 1);
    const LatLon first = position(wz.checkpoints.front());

    if (dt < z.transitionTimeS) {
        const double u = z.transitionTimeS > 0 ? dt / z.transitionTimeS : 1.0;
        const LatLon p = slerp(from, first, u);
        state.phase = Phase::Transit;
        state.latitudeDeg = p.lat;
        state.longitudeDeg = p.lon;
        state.distanceM += z.transitionM * u;
        return state;
    }

    // He flies at a steady speed inside a zone, so the share of the delivery time that has passed
    // is the share of the zone's route he has covered.
    state.phase = Phase::Delivering;
    state.distanceM += z.transitionM;
    const float f = float(z.deliveryTimeS > 0 ? std::clamp((dt - z.transitionTimeS) / z.deliveryTimeS, 0.0, 1.0) : 1.0);
    const auto &cps = wz.checkpoints;
    // First checkpoint whose cumulative share of the route reaches f.
    const auto it = std::lower_bound(cps.begin(), cps.end(), f,
                                     [](const Checkpoint &cp, float share) { return cp.pathFrac < share; });
    const std::size_t i = std::min<std::size_t>(std::size_t(it - cps.begin()), cps.size() - 1);
    const double w0 = i == 0 ? 0.0 : cps[i - 1].pathFrac;
    const double w1 = cps[i].pathFrac;
    const double u = w1 > w0 ? std::clamp((f - w0) / (w1 - w0), 0.0, 1.0) : 1.0;

    auto lerp = [u](double a, double b) { return a + (b - a) * u; };
    const Checkpoint *prev = i == 0 ? nullptr : &cps[i - 1];
    const Checkpoint &next = cps[i];
    const LatLon p = slerp(prev ? position(*prev) : first, position(next), u);

    state.latitudeDeg = p.lat;
    state.longitudeDeg = p.lon;
    state.householdsDelivered += z.households * lerp(prev ? prev->householdsFrac : 0.0, next.householdsFrac);
    state.presentsDelivered += z.children * lerp(prev ? prev->childrenFrac : 0.0, next.childrenFrac);
    state.distanceM += z.pathM * lerp(prev ? prev->pathFrac : 0.0, next.pathFrac);
    state.countryIndex = next.country;
    state.frontierRank = lerp(prev ? double(prev->rank) : 0.0, double(next.rank));
    return state;
}

} // namespace cr
