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

double bearing(LatLon a, LatLon b)
{
    const double la1 = a.lat * kDeg, la2 = b.lat * kDeg, dlo = (b.lon - a.lon) * kDeg;
    const double y = std::sin(dlo) * std::cos(la2);
    const double x = std::cos(la1) * std::sin(la2) - std::sin(la1) * std::cos(la2) * std::cos(dlo);
    if (std::abs(x) < 1e-12 && std::abs(y) < 1e-12)
        return 180;
    return std::fmod(std::atan2(y, x) / kDeg + 360.0, 360.0);
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

SantaState stateAt(const WorldData &world, const Results &results, double epochMs)
{
    SantaState state;
    const double t = (epochMs - results.referenceEpochMs) / 1000.0;
    const auto &zones = results.zones;
    const std::size_t n = std::min(zones.size(), world.zones.size());
    state.massKg = results.launchMassKg;
    state.missionElapsedS = t - results.launchS;
    if (n == 0)
        return state;

    const double giftKg = results.presents > 0 ? results.payloadKg / results.presents : 0;
    auto finishZonesBefore = [&](std::size_t k) {
        for (std::size_t j = 0; j < k; ++j) {
            state.householdsDelivered += zones[j].households;
            state.childrenDelivered += zones[j].children;
            state.presentsDelivered += zones[j].presents;
            state.stopsMade += zones[j].stops;
            state.distanceM += zones[j].pathM + zones[j].transitionM;
            state.dragEnergyJ += zones[j].dragEnergyJ;
        }
    };
    auto finalise = [&](int offsetMinutes) {
        state.massKg = results.launchMassKg - state.presentsDelivered * giftKg;
        double local = std::fmod(t + offsetMinutes * 60.0, 86400.0);
        state.localSecondsOfDay = local < 0 ? local + 86400.0 : local;
    };

    if (t < zones.front().startS) {
        state.phase = Phase::PreLaunch;
        state.frontierZone = 0;
        finalise(world.zones.front().offsetMinutes);
        return state;
    }

    // Last zone whose work has started.
    std::size_t k = 0;
    while (k + 1 < n && zones[k + 1].startS <= t)
        ++k;
    const ZoneResult &z = zones[k];
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
            state.headingDeg = bearing(here, zoneEnd(world, int(k + 1)));
            finalise(wz.offsetMinutes);
            return state;
        }
        // Heading home.
        const double homeTime = results.landingS - results.lastDeliveryS;
        const double u = homeTime > 0 ? std::clamp((t - z.endS) / homeTime, 0.0, 1.0) : 1.0;
        state.phase = u < 1 ? Phase::Returning : Phase::Landed;
        const LatLon p = slerp(here, {90, 0}, u);
        state.latitudeDeg = p.lat;
        state.longitudeDeg = p.lon;
        state.headingDeg = bearing(here, {90, 0});
        state.speedMps = u < 1 ? z.speedMps : 0;
        state.distanceM += results.returnHomeM * u;
        finalise(wz.offsetMinutes);
        return state;
    }

    finishZonesBefore(k);
    state.frontierZone = int(k);
    state.speedMps = z.speedMps;
    const double dt = t - z.startS;
    const LatLon from = zoneEnd(world, int(k) - 1);
    const LatLon first = position(wz.checkpoints.front());

    if (dt < z.transitionTimeS) {
        const double u = z.transitionTimeS > 0 ? dt / z.transitionTimeS : 1.0;
        const LatLon p = slerp(from, first, u);
        state.phase = Phase::Transit;
        state.latitudeDeg = p.lat;
        state.longitudeDeg = p.lon;
        state.headingDeg = bearing(from, first);
        state.distanceM += z.transitionM * u;
        finalise(wz.offsetMinutes);
        return state;
    }

    state.phase = Phase::Delivering;
    state.distanceM += z.transitionM;
    const double f = z.deliveryTimeS > 0 ? std::clamp((dt - z.transitionTimeS) / z.deliveryTimeS, 0.0, 1.0) : 1.0;
    const auto &work = z.checkpointWork;
    const auto &cps = wz.checkpoints;
    // First checkpoint whose cumulative work reaches f.
    const auto it = std::lower_bound(work.begin(), work.end(), float(f));
    const std::size_t i = std::min<std::size_t>(std::size_t(it - work.begin()), cps.size() - 1);
    const double w0 = i == 0 ? 0.0 : work[i - 1];
    const double w1 = work[i];
    const double u = w1 > w0 ? std::clamp((f - w0) / (w1 - w0), 0.0, 1.0) : 1.0;

    auto lerp = [u](double a, double b) { return a + (b - a) * u; };
    const Checkpoint *prev = i == 0 ? nullptr : &cps[i - 1];
    const Checkpoint &next = cps[i];
    const double hh = lerp(prev ? prev->householdsFrac : 0.0, next.householdsFrac);
    const double ch = lerp(prev ? prev->childrenFrac : 0.0, next.childrenFrac);
    const double path = lerp(prev ? prev->pathFrac : 0.0, next.pathFrac);
    const LatLon a = prev ? position(*prev) : first;
    const LatLon b = position(next);
    const LatLon p = slerp(a, b, u);

    state.latitudeDeg = p.lat;
    state.longitudeDeg = p.lon;
    state.headingDeg = bearing(a, b);
    state.householdsDelivered += z.households * hh;
    state.childrenDelivered += z.children * ch;
    state.presentsDelivered += z.presents * ch;
    state.stopsMade += z.stops * hh;
    state.distanceM += z.pathM * path;
    state.dragEnergyJ += z.dragEnergyJ * path;
    state.countryIndex = next.country;
    state.frontierRank = lerp(prev ? double(prev->rank) : 0.0, double(next.rank));
    finalise(wz.offsetMinutes);
    return state;
}

} // namespace cr
