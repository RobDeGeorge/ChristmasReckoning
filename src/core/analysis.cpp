// SPDX-License-Identifier: GPL-3.0-or-later
#include "analysis.h"

#include "atmosphere.h"
#include "constants.h"
#include "format.h"
#include "physics.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cr {
namespace {

using namespace cr::units;
namespace C = cr::constants;
namespace F = cr::format;
namespace P = cr::physics;

constexpr double kPi = std::numbers::pi;
constexpr double kYear = 365.25 * 86400.0;
constexpr double kDegToRad = kPi / 180.0;

// Reference silhouettes (assumptions, stated in the consoles).
constexpr double kReindeerNoseRadiusM = 0.1;
constexpr double kReindeerFrontalAreaM2 = 0.6;
constexpr double kWalkingMet = 3.5;
constexpr double kAn225PayloadKg = 253'820.0; // heaviest airlifted payload (An-225, 1988)
constexpr double kMoonMassKg = 7.346e22;
constexpr double kSpyHomes = 91.8e6;
constexpr double kSpySpeedMps = 650 * 1609.344;
constexpr double kSpyGs = 17'500;
constexpr double kSpyVaporiseS = 4.26e-3;

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
    return 2 * C::earthMeanRadius.si() * std::asin(std::min(1.0, std::sqrt(a)));
}

std::string utcTime(double secondsFromChristmasUtc)
{
    // "24 Dec 08:00 UTC"
    const double day = std::floor(secondsFromChristmasUtc / 86400.0);
    const int dateOfMonth = 25 + int(day);
    std::string dayText = dateOfMonth >= 1 && dateOfMonth <= 31
        ? std::to_string(dateOfMonth) + " Dec"
        : (dateOfMonth > 31 ? std::to_string(dateOfMonth - 31) + " Jan" : "Dec");
    return dayText + " " + F::clock(secondsFromChristmasUtc - day * 86400.0) + " UTC";
}

Step makeStep(std::string id, std::string label, std::string tex, std::string plain,
              std::string substitution, std::string result, std::vector<std::string> sources,
              std::string validity = {})
{
    Step s;
    s.id = std::move(id);
    s.label = std::move(label);
    s.tex = std::move(tex);
    s.plain = std::move(plain);
    s.substitution = std::move(substitution);
    s.result = std::move(result);
    s.sources = std::move(sources);
    s.validity = std::move(validity);
    return s;
}

std::string gs(double accelerationMps2)
{
    return F::acceleration(accelerationMps2 / C::standardGravity.si());
}

// Share of the power that magic must supply, 1 − supplied/needed, written as 99.99…% (computed from
// the ratio so a 10⁻¹⁸ shortfall doesn't round to exactly 100%).
std::string magicShareText(double supplied, double needed)
{
    if (needed <= 0 || supplied >= needed)
        return "0%";
    const double ratio = supplied / needed;
    if (ratio > 1e-3)
        return F::percent(1 - ratio, 4);
    const int n = int(std::floor(-std::log10(ratio))); // 1 − 10⁻ⁿ has n nines
    std::string text = "99." + std::string(std::size_t(std::min(n - 2, 10)), '9');
    if (n - 2 > 10)
        text += "…";
    return text + "% (all but 10" + F::superscript(-n) + ")";
}

} // namespace

double christmasUtcEpochMs(int year)
{
    return double(daysFromCivil(year, 12, 25)) * 86'400'000.0;
}

Results analyse(const WorldData &world, const Scenario &s)
{
    Results r;
    r.referenceEpochMs = christmasUtcEpochMs(s.year);

    const double e = s[Param::Eligibility] / 100.0;
    const double perStop = s[Param::HouseholdsPerStop];
    const double eta = s[Param::RouteQuality];
    const double tau = s[Param::StopTimeS];
    const double window = s.windowLengthS();
    const double startRel = s.windowStartRelativeS();
    const double giftKg = s[Param::GiftMassKg];
    const double presentsPerChild = s[Param::PresentsPerChild];
    const double beta = C::bhhBeta;
    const double g0 = C::standardGravity.si();
    const double altitude = s[Param::AltitudeM];
    const double radius = C::earthMeanRadius.si() + altitude;
    const auto air = atmosphere::standardAtmosphere(Length(altitude));

    r.windowS = window;
    r.baseMassKg = s[Param::SleighMassKg] + s[Param::SantaMassKg]
        + s[Param::ReindeerCount] * s[Param::ReindeerMassKg];
    r.airDensity = air.density.si();
    r.airTemperatureK = air.temperature.si();
    r.speedOfSoundMps = air.speedOfSound.si();

    // --- Per-zone jobs --------------------------------------------------------------------------
    const std::size_t zoneCount = world.zones.size();
    std::vector<ZoneJob> flightJobs(zoneCount), unitStopJobs(zoneCount), stopGoJobs(zoneCount);
    r.zones.resize(zoneCount);
    double minOffset = 1e9, maxOffset = -1e9;
    for (std::size_t k = 0; k < zoneCount; ++k) {
        const Zone &zone = world.zones[k];
        ZoneResult &z = r.zones[k];
        z.index = int(k);
        z.label = zone.label().toStdString();
        z.offsetMinutes = zone.offsetMinutes;
        z.households = e * zone.households;
        z.children = e * zone.children;
        z.presents = z.children * presentsPerChild;
        z.stops = z.households / perStop;
        z.pathM = eta * beta * std::sqrt(e / perStop) * zone.pathSumM + zone.gapM;
        z.transitionM = zone.transitionM;
        z.windowOpenS = startRel - zone.offsetMinutes * 60.0;
        z.windowCloseS = z.windowOpenS + window;
        for (std::uint16_t c : zone.topCountries) {
            if (c < world.countries.size() && z.countries.size() < 4)
                z.countries.push_back(world.countries[c].name.toStdString());
        }

        flightJobs[k] = {z.windowOpenS, z.windowCloseS, z.pathM + z.transitionM, tau * z.stops};
        unitStopJobs[k] = {z.windowOpenS, z.windowCloseS, 0.0, z.stops};
        // Stop-and-go: per-zone time 2√β·Q'/√a with Q' = Q·(e/k)^¾, so "rate" is √a.
        stopGoJobs[k] = {z.windowOpenS, z.windowCloseS,
                         2 * std::sqrt(beta) * zone.stopGoSum * std::pow(e / perStop, 0.75), tau * z.stops};

        r.children += z.children;
        r.households += z.households;
        r.presents += z.presents;
        r.stops += z.stops;
        r.pathM += z.pathM - zone.gapM;
        r.gapM += zone.gapM;
        r.transitionM += zone.transitionM;
        minOffset = std::min(minOffset, double(zone.offsetMinutes));
        maxOffset = std::max(maxOffset, double(zone.offsetMinutes));
    }
    if (zoneCount == 0)
        return r;

    r.nightSpanS = window + (maxOffset - minOffset) * 60.0;
    r.maxStopTimeS = maximumFixedScale(unitStopJobs);
    r.feasible = tau <= r.maxStopTimeS;

    // If the requested stop time can't fit at any speed, report that and run the physics on the
    // fly-by schedule (τ = 0) so every other console still has numbers to show.
    std::vector<ZoneJob> jobs = flightJobs;
    if (!r.feasible) {
        double needed = 0;
        for (const ZoneResult &z : r.zones)
            needed += tau * z.stops;
        r.feasibilityNote = "Stopping " + F::duration(tau) + " at each of " + F::count(r.stops)
            + " stops takes " + F::duration(needed) + " even at infinite speed. The longest stop that fits is "
            + F::duration(r.maxStopTimeS) + ". Physics below assumes presents are dropped in flight.";
        for (ZoneJob &job : jobs)
            job.fixedS = 0;
    }
    r.schedule = solveSchedule(jobs);
    const ScheduleResult &sched = r.schedule;
    r.peakSpeedMps = sched.peakRate;
    r.bindingFirst = sched.binding.first;
    r.bindingLast = sched.binding.last;

    double bindingHouseholds = 0;
    for (int k = r.bindingFirst; k >= 0 && k <= r.bindingLast; ++k)
        bindingHouseholds += r.zones[std::size_t(k)].households;
    r.bindingHouseholdShare = r.households > 0 ? bindingHouseholds / r.households : 0;
    if (r.bindingFirst >= 0)
        r.bindingSpanS = r.zones[std::size_t(r.bindingLast)].windowCloseS - r.zones[std::size_t(r.bindingFirst)].windowOpenS;

    // --- Sack, mass and drag per zone -----------------------------------------------------------
    r.payloadKg = r.presents * giftKg;
    r.launchMassKg = r.baseMassKg + r.payloadKg;
    auto sackArea = [&](double payloadKg) {
        const double volume = payloadKg / C::presentChargeableDensity.si();
        const double diameter = std::cbrt(6 * volume / kPi);
        return kPi * diameter * diameter / 4;
    };
    r.sackDiameterM = std::cbrt(6 * (r.payloadKg / C::presentChargeableDensity.si()) / kPi);
    r.frontalAreaM2 = sackArea(r.payloadKg);

    const double cd = s[Param::DragCoefficient];
    double presentsLeft = r.presents;
    double lastLat = 90, lastLon = 0;
    for (std::size_t k = 0; k < zoneCount; ++k) {
        ZoneResult &z = r.zones[k];
        const Zone &zone = world.zones[k];
        const double v = sched.rates[k];
        z.speedMps = v;
        z.startS = sched.startS[k];
        z.endS = sched.endS[k];
        z.transitionTimeS = v > 0 ? z.transitionM / v : 0;
        z.deliveryTimeS = std::max(0.0, z.endS - z.startS - z.transitionTimeS);
        z.massStartKg = r.baseMassKg + presentsLeft * giftKg;
        const double area = sackArea(presentsLeft * giftKg);
        z.dragEnergyJ = 0.5 * air.density.si() * cd * area * v * v * (z.pathM + z.transitionM);
        r.dragWorkJ += z.dragEnergyJ;
        presentsLeft -= z.presents;

        // Cumulative share of delivery time at each checkpoint (distance at speed v, plus stops).
        const double fixed = r.feasible ? tau * z.stops : 0.0;
        const double flying = v > 0 ? z.pathM / v : 0.0;
        const double total = flying + fixed;
        z.checkpointWork.resize(zone.checkpoints.size());
        for (std::size_t i = 0; i < zone.checkpoints.size(); ++i) {
            const Checkpoint &cp = zone.checkpoints[i];
            const double w = total > 0 ? (flying * cp.pathFrac + fixed * cp.householdsFrac) / total : 1.0;
            z.checkpointWork[i] = float(std::clamp(w, 0.0, 1.0));
        }
        if (!zone.checkpoints.empty()) {
            lastLat = zone.checkpoints.back().latitudeDeg;
            lastLon = zone.checkpoints.back().longitudeDeg;
        }
        r.busyS += z.endS - z.startS;
    }
    r.launchS = r.zones.front().startS;
    r.lastDeliveryS = r.zones.back().endS;
    r.returnHomeM = greatCircleM(lastLat, lastLon, 90, 0);
    const double homeSpeed = std::max(r.zones.back().speedMps, 1.0);
    r.landingS = r.lastDeliveryS + r.returnHomeM / homeSpeed;
    r.distanceM = r.pathM + r.gapM + r.transitionM + r.returnHomeM;
    r.averageSpeedMps = r.busyS > 0 ? (r.distanceM - r.returnHomeM) / r.busyS : 0;
    r.timePerStopS = r.stops > 0 ? r.busyS / r.stops : 0;
    r.averageHopM = r.stops > 0 ? r.pathM / r.stops : 0;

    // --- Headline physics at the bottleneck speed --------------------------------------------------
    const double v = r.peakSpeedMps;
    const ZoneResult &bottleneck = r.zones[std::size_t(std::max(0, r.bindingFirst))];
    const double massAtBottleneck = bottleneck.massStartKg;
    const double areaAtBottleneck = sackArea(massAtBottleneck - r.baseMassKg);
    const Speed speed(v);
    r.mach = P::machNumber(speed, air.speedOfSound);
    r.dragForceN = P::dragForce(air.density, speed, cd, Area(areaAtBottleneck)).si();
    r.dragPowerW = r.dragForceN * v;
    r.kineticEnergyJ = P::kineticEnergy(Mass(massAtBottleneck), speed).si();
    r.curvatureGs = P::curvatureDownforce(speed, Length(radius), C::standardGravity).si() / g0;

    const CriticalRun stopGoRun = [&] {
        CriticalRun run;
        std::vector<ZoneJob> sg = stopGoJobs;
        if (!r.feasible)
            for (ZoneJob &job : sg)
                job.fixedS = 0;
        minimumPeakRate(sg, &run);
        return run;
    }();
    r.stopGoAccelMps2 = stopGoRun.rate * stopGoRun.rate;

    r.reindeerPowerW = P::allometricPower(Mass(s[Param::ReindeerMassKg]), C::horseMass, C::horseSustainedPower,
                                          C::metabolicPowerExponent).si();
    r.reindeerNeeded = r.dragPowerW / r.reindeerPowerW;
    const double available = s[Param::ReindeerCount] * r.reindeerPowerW;
    r.magicShare = r.dragPowerW > 0 ? std::max(0.0, 1 - available / r.dragPowerW) : 0;

    double lag = 0;
    for (const ZoneResult &z : r.zones)
        lag += P::lorentzGammaMinusOne(Speed(z.speedMps), C::speedOfLight) * (z.endS - z.startS);
    r.clockLagS = lag;

    // --- Consoles ---------------------------------------------------------------------------------
    const std::string bindingText = r.bindingFirst >= 0
        ? r.zones[std::size_t(r.bindingFirst)].label + " → " + r.zones[std::size_t(r.bindingLast)].label
        : std::string("—");
    const double orbit = P::circularOrbitSpeed(C::earthGM, Length(radius)).si();
    const double escape = P::escapeSpeed(C::earthGM, Length(radius)).si();
    const double timeAtFiveG = [&] {
        double total = 0;
        for (const ZoneJob &job : stopGoJobs)
            total += job.work / std::sqrt(5 * g0) + (r.feasible ? job.fixedS : 0);
        return total;
    }();
    const double hopTime = r.stopGoAccelMps2 > 0 ? 2 * std::sqrt(r.averageHopM / r.stopGoAccelMps2) : 0;
    const double hopPeak = hopTime > 0 ? 2 * r.averageHopM / hopTime : 0;
    const double ambientP = air.pressure.si();

    // MANIFEST ------------------------------------------------------------------------------------
    {
        Console c;
        c.id = "manifest";
        c.callsign = "MANIFEST";
        c.title = "Payload manifest";
        c.question = "Who is on the list?";
        c.go = true;
        c.status = "GO — the list is long, but counting is the easy part.";
        c.readouts = {{"Children under 18", F::count(r.children)},
                      {"Households to visit", F::count(r.households)},
                      {"Presents", F::count(r.presents)},
                      {"Stops", F::count(r.stops)}};
        c.steps.push_back(makeStep(
            "children", "Children", R"(C = e \sum_{c} P_{c,\,0\text{–}17})",
            "C = e × Σ (children aged 0–17 in each country)",
            "= " + F::percent(e) + " × " + F::count(world.totalChildren()),
            F::count(r.children), {"wpp2024"}));
        c.steps.push_back(makeStep(
            "households", "Households with children",
            R"(H = e \sum_{c} \frac{P_c}{\bar{s}_c}\,\varphi_{c,18})",
            "H = e × Σ (population ÷ average household size × share of households with a child)",
            "= " + F::percent(e) + " × " + F::count(world.totalHouseholds()),
            F::count(r.households), {"un-households", "wpp2024"}));
        c.steps.push_back(makeStep(
            "kappa", "Children per household with children", R"(\kappa = C / H)", "κ = C ÷ H",
            "= " + F::count(r.children) + " ÷ " + F::count(r.households),
            F::number(r.households > 0 ? r.children / r.households : 0, 2), {}));
        c.steps.push_back(makeStep(
            "presents", "Presents", R"(N_{\text{gifts}} = C \times g)", "presents = C × presents per child",
            "= " + F::count(r.children) + " × " + F::number(presentsPerChild, 2), F::count(r.presents), {}));
        c.comparisons.push_back("Spy magazine's famous 1990 estimate counted " + F::count(kSpyHomes)
                                + " homes. The real list is " + F::times(r.households / kSpyHomes) + " longer.");
        c.comparisons.push_back("That is one child for every " + F::number(world.totalPopulation() / std::max(1.0, r.children), 2)
                                + " people on Earth.");
        c.params = {"eligibility", "presentsPerChild"};
        c.caveat = "Assumes every child lives in a household (not a boarding school, hospital or camp) and that "
                   "each country's household make-up matches its latest census or survey.";
        r.consoles.push_back(std::move(c));
    }

    // LAUNCH WINDOWS -----------------------------------------------------------------------------
    {
        Console c;
        c.id = "windows";
        c.callsign = "LAUNCH";
        c.title = "Launch windows";
        c.question = "When is Santa allowed to deliver?";
        c.go = r.feasible;
        c.status = r.feasible ? "GO — " + F::duration(r.nightSpanS) + " of darkness, if he never stops."
                              : "NO-GO — " + r.feasibilityNote;
        c.readouts = {{"Delivery window", F::clock(s[Param::WindowStartHour] * 3600) + "–"
                                              + F::clock((s[Param::WindowStartHour] + s[Param::WindowLengthHours]) * 3600)},
                      {"Whole night", F::duration(r.nightSpanS)},
                      {"Launch", utcTime(r.launchS)},
                      {"Bottleneck", bindingText}};
        c.steps.push_back(makeStep(
            "span", "Length of the night", R"(T = W + (o_{\max} - o_{\min}))",
            "T = window + (largest UTC offset − smallest)",
            "= " + F::duration(window) + " + (" + F::number(maxOffset / 60, 3) + " h − (" + F::number(minOffset / 60, 3) + " h))",
            F::duration(r.nightSpanS), {"tzdata", "aasm-sleep"}));
        c.steps.push_back(makeStep(
            "bottleneck", "The tightest run of time zones",
            R"(\max_{i \le j} \frac{L_{ij}}{W + o_i - o_j - \tau N_{ij}})",
            "worst over every run of zones i…j of distance ÷ (time available − stop time)",
            bindingText + ": " + F::percent(r.bindingHouseholdShare) + " of households must be done in "
                + F::duration(r.bindingSpanS),
            "binding", {"horn-1974"}));
        c.steps.push_back(makeStep(
            "taumax", "Longest possible stop at each house",
            R"(\tau_{\max} = \min_{i \le j} \frac{W + o_i - o_j}{N_{ij}})",
            "τ_max = smallest (time available ÷ stops) over every run of zones",
            "with infinitely fast flight", F::duration(r.maxStopTimeS), {"horn-1974"}));
        c.comparisons.push_back("Spy assumed 31 hours as one shared pool of time. In reality "
                                + F::percent(r.bindingHouseholdShare) + " of the work is squeezed into "
                                + F::duration(r.bindingSpanS) + ".");
        c.comparisons.push_back("Santa can't ride midnight around the world: in the busiest zones he is still "
                                "delivering hours after local midnight.");
        c.params = {"windowStartHour", "windowLengthHours", "stopTimeS"};
        c.caveat = "Time zones are as observed on Christmas Eve " + std::to_string(world.dataYear)
            + " (New Zealand is on daylight time, UTC+13). Officially all of China uses Beijing time, but "
              "the time zone database puts Xinjiang on UTC+6 as many residents do. Many families open "
              "presents on Christmas Eve, and Orthodox Christmas is on 7 January.";
        r.consoles.push_back(std::move(c));
    }

    // FIDO -----------------------------------------------------------------------------------------
    {
        Console c;
        c.id = "fido";
        c.callsign = "FIDO";
        c.title = "Flight dynamics";
        c.question = "How far, and how fast?";
        c.go = false;
        c.status = "NO-GO — " + F::times(v / escape) + " Earth's escape velocity.";
        c.readouts = {{"Route", F::length(r.distanceM)},
                      {"Peak speed", F::speed(v)},
                      {"Average speed", F::speed(r.averageSpeedMps)},
                      {"Mach", F::number(r.mach, 2)}};
        c.steps.push_back(makeStep(
            "route", "Shortest tour through every household",
            R"(L = \eta\,\beta \sum_i \sqrt{N_i A_i} + G)",
            "L = η × β × Σ √(households × area) over 1 km cells + gaps between clusters",
            "= " + F::number(eta, 3) + " × " + F::number(beta, 4) + " × " + F::length(std::sqrt(e / perStop) * world.totalPathSumM())
                + " + " + F::length(r.gapM),
            F::length(r.pathM + r.gapM), {"bhh-constant", "ghsl", "spacefilling"}));
        c.steps.push_back(makeStep(
            "distance", "Everything flown", R"(D = L + \text{zone hops} + \text{trip home})",
            "D = tour + hops between time zones + trip home",
            "= " + F::length(r.pathM + r.gapM) + " + " + F::length(r.transitionM) + " + " + F::length(r.returnHomeM),
            F::length(r.distanceM), {}));
        c.steps.push_back(makeStep(
            "peak", "Lowest possible top speed", R"(v_{\min} = \frac{L_{ij}}{W + o_i - o_j})",
            "v_min = distance of the bottleneck run ÷ the time it has",
            bindingText + " in " + F::duration(r.bindingSpanS), F::speed(v), {"horn-1974", "yds-1995"}));
        c.steps.push_back(makeStep(
            "orbit", "Circular orbit speed at this height", R"(v_{\text{orb}} = \sqrt{GM/(R+h)})",
            "v_orb = √(GM ÷ (R + h))",
            "= √(" + F::scientific(C::earthGM.si(), 4) + " m³/s² ÷ " + F::length(radius, 5) + ")",
            F::speed(orbit, 3), {"iers", "iugg-radius"}));
        c.steps.push_back(makeStep(
            "downforce", "Push needed to keep hugging the ground", R"(a_{\downarrow} = \frac{v^2}{R+h} - g)",
            "a = v² ÷ (R + h) − g",
            "= (" + F::speed(v) + ")² ÷ " + F::length(radius, 5) + " − " + F::number(g0, 6) + " m/s²",
            F::acceleration(r.curvatureGs), {"cgpm-g0"},
            v > orbit ? "Above orbital speed, gravity can't hold the sleigh down: the reindeer must pull "
                        "towards the ground just to stay low."
                      : ""));
        c.comparisons = {
            F::times(v / C::rifleBulletSpeed.si()) + " a rifle bullet",
            F::times(v / C::issOrbitalSpeed.si()) + " the International Space Station",
            F::times(v / C::parkerSolarProbeSpeed.si()) + " Parker Solar Probe, the fastest object ever built",
            F::times(v / C::milkyWayEscapeSpeed.si()) + " the speed needed to escape the Milky Way",
            "Laps the Earth in " + F::duration(2 * kPi * radius / v)};
        c.params = {"routeQuality", "householdsPerStop", "eligibility", "windowLengthHours", "altitudeM"};
        c.caveat = "The tour estimate assumes households are scattered at random within each 1 km cell. Real "
                   "streets make tours a little shorter; apartment blocks make them much shorter (try "
                   "households per stop).";
        r.consoles.push_back(std::move(c));
    }

    // RETRO ----------------------------------------------------------------------------------------
    {
        Console c;
        c.id = "retro";
        c.callsign = "RETRO";
        c.title = "Stop-and-go";
        c.question = "What if he actually stops at every house?";
        c.go = false;
        c.status = "NO-GO — every stop needs " + gs(r.stopGoAccelMps2) + " of braking.";
        c.readouts = {{"Time per stop", F::duration(r.timePerStopS)},
                      {"Average hop", F::length(r.averageHopM)},
                      {"Braking", gs(r.stopGoAccelMps2)},
                      {"Night needed at 5 g", F::duration(timeAtFiveG)}};
        c.steps.push_back(makeStep(
            "perstop", "Time per stop", R"(\Delta t = T_{\text{busy}} / N)", "Δt = time spent working ÷ stops",
            "= " + F::duration(r.busyS) + " ÷ " + F::count(r.stops), F::duration(r.timePerStopS), {}));
        c.steps.push_back(makeStep(
            "hop", "Average hop between stops", R"(\bar{\ell} = L / N)", "ℓ = tour length ÷ stops",
            "= " + F::length(r.pathM) + " ÷ " + F::count(r.stops), F::length(r.averageHopM), {"bhh-constant"}));
        c.steps.push_back(makeStep(
            "stopgo", "Acceleration to stop at every house",
            R"(a_{\min} = \max_{i \le j}\Big[\frac{2\sqrt{\beta}\,Q_{ij}}{W + o_i - o_j}\Big]^2,\ Q = \sum N^{3/4}A^{1/4})",
            "a = [2√β × Σ N^¾ A^¼ ÷ time available]², worst run of zones",
            "accelerate for half of every hop, brake for the other half", gs(r.stopGoAccelMps2),
            {"bhh-constant", "horn-1974"}));
        c.steps.push_back(makeStep(
            "hoppeak", "Top speed within an average hop", R"(v_{\text{hop}} = 2\bar{\ell}/t_{\text{hop}})",
            "v = 2 × hop ÷ hop time",
            "= 2 × " + F::length(r.averageHopM) + " ÷ " + F::duration(hopTime), F::speed(hopPeak), {}));
        c.steps.push_back(makeStep(
            "stopping", "Braking distance from cruise at 5 g", R"(s = v^2 / 2a)", "s = v² ÷ (2 × 5 g)",
            "= (" + F::speed(v) + ")² ÷ (2 × 5 × " + F::number(g0, 6) + " m/s²)",
            F::length(P::stoppingDistance(speed, 5.0 * C::standardGravity).si()), {"nasa-g-tolerance"}));
        c.steps.push_back(makeStep(
            "fiveg", "How long the night must last at a survivable 5 g",
            R"(T = \sum_k 2\sqrt{\beta}\,Q_k / \sqrt{5g})", "T = Σ 2√β Q ÷ √(5 g)",
            "every hop at 5 g, ignoring time zones", F::duration(timeAtFiveG), {"nasa-g-tolerance"}));
        c.comparisons = {"Peak braking is " + F::times(r.stopGoAccelMps2 / g0 / C::humanBriefGTolerance)
                             + " the most a strapped-in human survives, even for a split second.",
                         "Spy's version never stopped at all; it only computed straight-line flight."};
        c.params = {"householdsPerStop", "stopTimeS", "windowLengthHours"};
        c.caveat = "Assumes straight hops with constant acceleration then constant braking, the profile with "
                   "the lowest peak force.";
        r.consoles.push_back(std::move(c));
    }

    // LOADMASTER -------------------------------------------------------------------------------------
    {
        Console c;
        c.id = "loadmaster";
        c.callsign = "LOADMASTER";
        c.title = "Cargo";
        c.question = "How heavy is the sleigh?";
        c.go = r.payloadKg <= kAn225PayloadKg;
        c.status = (c.go ? "GO — " : "NO-GO — ") + F::times(r.payloadKg / kAn225PayloadKg)
            + " the heaviest load ever airlifted.";
        c.readouts = {{"Presents", F::mass(r.payloadKg)},
                      {"Launch mass", F::mass(r.launchMassKg)},
                      {"Sack diameter", F::length(r.sackDiameterM)},
                      {"Frontal area", F::number(r.frontalAreaM2, 2) + " m²"}};
        c.steps.push_back(makeStep(
            "payload", "Mass of the presents", R"(M_p = N_{\text{gifts}} \times m)", "M = presents × mass each",
            "= " + F::count(r.presents) + " × " + F::mass(giftKg), F::mass(r.payloadKg), {}));
        c.steps.push_back(makeStep(
            "sack", "Size of the sack (as a ball)", R"(d = \sqrt[3]{6 M_p / (\pi \rho_p)})",
            "d = ∛(6 × mass ÷ (π × parcel density))",
            "ρ = " + F::number(C::presentChargeableDensity.si(), 3) + " kg/m³", F::length(r.sackDiameterM),
            {"iata-volumetric"}));
        c.steps.push_back(makeStep(
            "launchmass", "Launch mass", R"(M_0 = M_{\text{sleigh}} + M_{\text{Santa}} + n\,m_r + M_p)",
            "sleigh + Santa + reindeer + presents",
            "= " + F::mass(s[Param::SleighMassKg]) + " + " + F::mass(s[Param::SantaMassKg]) + " + "
                + F::number(s[Param::ReindeerCount], 2) + " × " + F::mass(s[Param::ReindeerMassKg]) + " + "
                + F::mass(r.payloadKg),
            F::mass(r.launchMassKg), {"reindeer-mass"}));
        c.comparisons = {"The sack alone is " + F::length(r.sackDiameterM) + " across — the Burj Khalifa is 828 m tall.",
                         "The sleigh gets lighter all night: by the last time zone it carries "
                             + F::percent(r.zones.back().presents / std::max(1.0, r.presents)) + " of its load."};
        c.params = {"giftMassKg", "presentsPerChild", "eligibility", "sleighMassKg"};
        c.caveat = "Parcel density uses the air-cargo volumetric rule (6,000 cm³ per kg). Packed tighter, the "
                   "sack shrinks but drag barely changes.";
        r.consoles.push_back(std::move(c));
    }

    // PROP -------------------------------------------------------------------------------------------
    {
        Console c;
        c.id = "prop";
        c.callsign = "PROP";
        c.title = "Propulsion";
        c.question = "Can nine reindeer pull it?";
        c.go = r.reindeerNeeded <= s[Param::ReindeerCount];
        c.status = std::string(c.go ? "GO" : "NO-GO") + " — it takes " + F::count(r.reindeerNeeded)
            + " reindeer, not " + F::number(s[Param::ReindeerCount], 2) + ".";
        c.readouts = {{"Thrust needed", F::number(r.dragForceN, 2) + " N"},
                      {"Power needed", F::power(r.dragPowerW)},
                      {"One reindeer", F::power(r.reindeerPowerW)},
                      {"Reindeer needed", F::count(r.reindeerNeeded)}};
        c.steps.push_back(makeStep(
            "reindeerpower", "Sustained power of one reindeer (estimate)",
            R"(p_r = p_{\text{horse}} \left(\frac{m_r}{m_{\text{horse}}}\right)^{0.87})",
            "p = horse power × (reindeer mass ÷ horse mass)^0.87",
            "= " + F::number(C::horseSustainedPower.si(), 4) + " W × (" + F::mass(s[Param::ReindeerMassKg]) + " ÷ "
                + F::mass(C::horseMass.si()) + ")^0.87",
            F::power(r.reindeerPowerW), {"horsepower", "weibel-2005"},
            "An allometric estimate: no direct measurement of reindeer draught power was found."));
        c.steps.push_back(makeStep(
            "drag", "Air resistance at the bottleneck", R"(F_D = \tfrac12 \rho v^2 C_D A)",
            "F = ½ × air density × v² × C_D × frontal area",
            "= ½ × " + F::number(r.airDensity, 3) + " kg/m³ × (" + F::speed(v) + ")² × " + F::number(cd, 2) + " × "
                + F::number(areaAtBottleneck, 2) + " m²",
            F::number(r.dragForceN, 2) + " N", {"hoerner", "ussa1976"}));
        c.steps.push_back(makeStep(
            "power", "Power to keep going", R"(P = F_D\, v)", "P = drag × speed",
            "= " + F::number(r.dragForceN, 2) + " N × " + F::speed(v), F::power(r.dragPowerW), {}));
        c.steps.push_back(makeStep(
            "herd", "Reindeer needed", R"(n = P / p_r)", "n = power needed ÷ power per reindeer",
            "= " + F::power(r.dragPowerW) + " ÷ " + F::power(r.reindeerPowerW), F::count(r.reindeerNeeded), {}));
        c.comparisons = {"That herd would weigh " + F::mass(r.reindeerNeeded * s[Param::ReindeerMassKg]) + " — "
                             + F::times(r.reindeerNeeded * s[Param::ReindeerMassKg] / kMoonMassKg) + " the Moon.",
                         "Spy sized the team by pulling force (300 lb each). What limits a fast sleigh is power, P = F·v.",
                         "The drag power is " + F::times(r.dragPowerW / C::solarLuminosity.si()) + " the Sun's entire output."};
        c.params = {"reindeerCount", "reindeerMassKg", "dragCoefficient", "altitudeM", "giftMassKg"};
        c.caveat = "Steady flight only. Accelerating out of each stop (see RETRO) needs vastly more.";
        r.consoles.push_back(std::move(c));
    }

    // THERMAL ----------------------------------------------------------------------------------------
    {
        const double t0Ideal = P::stagnationTemperatureIdeal(air.temperature, speed, C::airSpecificHeatCp).si();
        const double t0Plasma = P::stagnationTemperaturePlasmaBound(
            speed, C::fullyIonisedAirParticleMassAmu * C::atomicMassUnit, C::boltzmann).si();
        const double qSg = P::suttonGravesHeatFlux(air.density, Length(kReindeerNoseRadiusM), speed, C::suttonGravesK).si();
        const double lambda = s[Param::HeatAbsorbedFraction];
        const double qCeiling = lambda * P::kineticEnergyFlux(air.density, speed).si();
        const double vaporise = P::vaporisationTime(Mass(s[Param::ReindeerMassKg]), C::vaporisationEnergy, lambda,
                                                    Area(kReindeerFrontalAreaM2), air.density, speed).si();
        Console c;
        c.id = "thermal";
        c.callsign = "THERMAL";
        c.title = "Heating";
        c.question = "Does the lead reindeer survive?";
        c.go = false;
        c.status = "NO-GO — Rudolph vaporises in " + F::duration(vaporise) + ".";
        c.readouts = {{"Mach", F::number(r.mach, 2)},
                      {"Stagnation temperature", "up to " + F::temperature(t0Plasma)},
                      {"Heat flux on the nose", F::number(qCeiling, 2) + " W/m²"},
                      {"Rudolph lasts", F::duration(vaporise)}};
        c.steps.push_back(makeStep(
            "mach", "Mach number", R"(M = v / a)", "Mach = speed ÷ speed of sound",
            "= " + F::speed(v) + " ÷ " + F::speed(r.speedOfSoundMps, 3), F::number(r.mach, 2), {"ussa1976"}));
        c.steps.push_back(makeStep(
            "t0ideal", "Stagnation temperature (ideal gas)", R"(T_0 = T + \frac{v^2}{2 c_p})",
            "T₀ = T + v² ÷ (2 c_p)",
            "= " + F::temperature(r.airTemperatureK, 3) + " + (" + F::speed(v) + ")² ÷ (2 × "
                + F::number(C::airSpecificHeatCp.si(), 4) + " J/kg·K)",
            F::temperature(t0Ideal), {"ussa1976", "nasa-real-gas"},
            t0Ideal > 800 ? "Only valid below ~800 K. Real air dissociates and ionises, so this is far too high." : ""));
        c.steps.push_back(makeStep(
            "t0plasma", "Upper bound once the air is a plasma", R"(T_0 \lesssim \frac{\bar{m} v^2}{5 k_B})",
            "T₀ ≲ m v² ÷ (5 k_B), for fully ionised air (m ≈ 1.76 u per particle)",
            "= 1.76 u × (" + F::speed(v) + ")² ÷ (5 × " + F::scientific(C::boltzmann.si(), 4) + " J/K)",
            F::temperature(t0Plasma), {"si", "codata"},
            "Ignores the energy spent ionising the air and radiated away as light, so the true value is lower."));
        c.steps.push_back(makeStep(
            "sutton", "Heating at the nose (re-entry formula)", R"(q = k\sqrt{\rho / r_n}\; v^3)",
            "q = 1.74×10⁻⁴ × √(ρ ÷ nose radius) × v³",
            "rₙ = " + F::length(kReindeerNoseRadiusM) + ", v = " + F::speed(v), F::number(qSg, 2) + " W/m²",
            {"sutton-graves"},
            v > C::suttonGravesMaxValidSpeed ? "Fitted for ~2–15 km/s; here it is used far outside that range." : ""));
        c.steps.push_back(makeStep(
            "ceiling", "Physical ceiling on heating", R"(q \le \Lambda\,\tfrac12 \rho v^3)",
            "q ≤ Λ × ½ ρ v³ (the share Λ of the oncoming air's energy that reaches the body)",
            "Λ = " + F::number(lambda, 2), F::number(qCeiling, 2) + " W/m²", {"meteor-ablation"}));
        c.steps.push_back(makeStep(
            "vaporise", "Time to vaporise the lead reindeer",
            R"(t = \frac{m\,Q}{\Lambda\,\tfrac12\rho v^3\, S})", "t = mass × 2.6 MJ/kg ÷ (heating × frontal area)",
            "m = " + F::mass(s[Param::ReindeerMassKg]) + ", S = " + F::number(kReindeerFrontalAreaM2, 2) + " m²",
            F::duration(vaporise), {"water-hvap", "meteor-ablation"}));
        c.comparisons = {"Spy said 4.26 thousandths of a second. That is " + F::times(kSpyVaporiseS / vaporise)
                             + " too slow.",
                         "Each nitrogen molecule hits the sleigh with "
                             + F::number(0.5 * 28.0134 * C::atomicMassUnit.si() * v * v / 1.602176634e-19 / 1000, 2)
                             + " keV — more than a medical X-ray photon."};
        c.params = {"heatAbsorbedFraction", "altitudeM", "reindeerMassKg"};
        c.caveat = "Nose radius 10 cm and a 0.6 m² head-on silhouette are assumptions. At these speeds a "
                   "shock-heated plasma forms ahead of the sleigh; the numbers are bounds, not predictions.";
        r.consoles.push_back(std::move(c));
    }

    // RANGE SAFETY ------------------------------------------------------------------------------------
    {
        const double tntPerMetre = r.dragForceN / C::tonOfTnt.si();
        const double blast = P::cylindricalBlastRadius(Force(r.dragForceN), Pressure(ambientP)).si();
        const double stopGoEnergy = 0.5 * r.stopGoAccelMps2 * (r.baseMassKg + r.payloadKg / 2) * r.pathM;
        Console c;
        c.id = "range";
        c.callsign = "RANGE SAFETY";
        c.title = "Energy and blast";
        c.question = "What happens to the neighbourhood?";
        c.go = false;
        c.status = "NO-GO — every metre of flight releases " + F::number(tntPerMetre, 2) + " t of TNT.";
        c.readouts = {{"Kinetic energy", F::energy(r.kineticEnergyJ)},
                      {"In TNT", F::number(r.kineticEnergyJ / C::tonOfTnt.si() / 1e6, 2) + " Mt"},
                      {"Blast radius", F::length(blast)},
                      {"Drag work, whole night", F::energy(r.dragWorkJ)}};
        c.steps.push_back(makeStep(
            "ke", "Kinetic energy at the bottleneck", R"(E_k = \tfrac12 M v^2)", "E = ½ × mass × v²",
            "= ½ × " + F::mass(massAtBottleneck) + " × (" + F::speed(v) + ")²", F::energy(r.kineticEnergyJ), {}));
        c.steps.push_back(makeStep(
            "worldenergy", "Compared with humanity's energy use", R"(E_k / E_{\text{world}})",
            "E ÷ world primary energy per year",
            "= " + F::energy(r.kineticEnergyJ) + " ÷ " + F::energy(C::worldPrimaryEnergyPerYear.si()),
            F::number(r.kineticEnergyJ / C::worldPrimaryEnergyPerYear.si(), 2) + " years of it", {"ei-review-2025"}));
        c.steps.push_back(makeStep(
            "permetre", "Energy dumped into the air per metre", R"(E_\ell = F_D)",
            "energy per metre = drag force", "1 t TNT = 4.184 GJ",
            F::number(tntPerMetre, 2) + " t TNT per metre", {"tnt-convention"}));
        c.steps.push_back(makeStep(
            "blast", "Blast radius of that line of energy", R"(R_0 = \sqrt{E_\ell / p_0})",
            "R = √(energy per metre ÷ air pressure)",
            "= √(" + F::number(r.dragForceN, 2) + " J/m ÷ " + F::number(ambientP, 3) + " Pa)", F::length(blast),
            {"glasstone"}));
        c.steps.push_back(makeStep(
            "dragwork", "Energy lost to drag all night", R"(W = \sum_k \tfrac12 \rho C_D A_k v_k^2 L_k)",
            "Σ drag × distance, zone by zone", "sack shrinking as presents are delivered",
            F::energy(r.dragWorkJ), {"yds-1995"}));
        c.steps.push_back(makeStep(
            "stopgoenergy", "Energy to stop and start at every house", R"(E = \tfrac12\, a\, \bar{M} L)",
            "½ × braking × average mass × tour length", "no energy recovered when braking",
            F::energy(stopGoEnergy), {}));
        c.comparisons = {"The whole night's drag is " + F::duration(r.dragWorkJ / C::solarLuminosity.si())
                             + " of the Sun's total output.",
                         "Stopping at every house would take " + F::duration(stopGoEnergy / C::solarLuminosity.si())
                             + " of sunshine."};
        c.params = {"altitudeM", "dragCoefficient", "giftMassKg"};
        c.caveat = "Treats the shock as a cylindrical blast. At these energies much of it would leave as light "
                   "and X-rays, so the blast estimate is rough.";
        r.consoles.push_back(std::move(c));
    }

    // SURGEON ------------------------------------------------------------------------------------------
    {
        const double cookies = r.stops * s[Param::CookiesPerStop];
        const double cookieKg = cookies * s[Param::CookieMassG] / 1000;
        const double kcal = cookieKg * 10 * C::cookieEnergyKcalPer100g;
        const double burnPerMinute = P::kcalPerMinute(kWalkingMet, Mass(s[Param::SantaMassKg]));
        const double burnTime = kcal / burnPerMinute * 60;
        const double milkM3 = r.stops * s[Param::MilkPerStopMl] * 1e-6;
        Console c;
        c.id = "surgeon";
        c.callsign = "SURGEON";
        c.title = "Flight surgeon";
        c.question = "How is Santa feeling?";
        c.go = false;
        c.status = "NO-GO — " + F::times(r.curvatureGs / C::humanSustainedGTolerance)
            + " the g-force a person can sustain, just to follow the ground.";
        c.readouts = {{"Cornering load", F::acceleration(r.curvatureGs)},
                      {"Cookies", F::count(cookies)},
                      {"Calories", F::count(kcal) + " kcal"},
                      {"Milk", F::number(milkM3 / C::olympicPoolVolume.si(), 2) + " Olympic pools"}};
        c.steps.push_back(makeStep(
            "gload", "g-force from following the curved Earth", R"(n = \frac{v^2}{g(R+h)} - 1)",
            "n = v² ÷ (g × (R + h)) − 1",
            "= (" + F::speed(v) + ")² ÷ (" + F::number(g0, 6) + " m/s² × " + F::length(radius, 5)
                + ") − 1, felt as weight pulling Santa up out of his seat",
            F::acceleration(r.curvatureGs), {"nasa-g-tolerance"}));
        c.steps.push_back(makeStep(
            "kcal", "Cookie energy", R"(E = N \times m \times 4.44\ \text{kcal/g})", "cookies × mass × 4.44 kcal/g",
            "= " + F::count(cookies) + " × " + F::number(s[Param::CookieMassG], 2) + " g × 4.44 kcal/g",
            F::count(kcal) + " kcal", {"usda-cookie"}));
        c.steps.push_back(makeStep(
            "eatrate", "Eating rate", R"(\dot m = M_{\text{cookies}} / T)", "cookie mass ÷ time",
            "= " + F::mass(cookieKg) + " ÷ " + F::duration(r.busyS), F::number(cookieKg / std::max(1.0, r.busyS), 2) + " kg/s",
            {}));
        c.steps.push_back(makeStep(
            "burn", "Walking it off", R"(t = \frac{E}{\text{MET} \times 3.5 \times m / 200})",
            "kcal ÷ (3.5 MET × 3.5 × mass ÷ 200 per minute)",
            "= " + F::count(kcal) + " kcal ÷ " + F::number(burnPerMinute, 2) + " kcal/min", F::duration(burnTime),
            {"compendium-met"}));
        c.steps.push_back(makeStep(
            "milk", "Milk", R"(V = N \times v_{\text{glass}})", "stops × one glass",
            "= " + F::count(r.stops) + " × " + F::number(s[Param::MilkPerStopMl], 3) + " mL",
            F::number(milkM3 / 1000, 2) + " million litres", {"fina-pool"}));
        c.comparisons = {"Spy's figure was 17,500 g. Following the ground at the bottleneck speed takes "
                             + F::acceleration(r.curvatureGs) + ".",
                         "Burning off the cookies by walking would take longer than recorded history."};
        if (burnTime < 5000 * kYear)
            c.comparisons.back() = "Burning off the cookies by walking would take " + F::duration(burnTime) + ".";
        c.params = {"cookiesPerStop", "cookieMassG", "milkPerStopMl", "santaMassKg"};
        c.caveat = "Human tolerance: about 5 g for minutes, tens of g for a fraction of a second when strapped in.";
        r.consoles.push_back(std::move(c));
    }

    // NAV -----------------------------------------------------------------------------------------------
    {
        const double coriolisGs = 2 * C::earthRotationRate.si() * v / g0;
        const double midnightSpeed = C::earthRotationRate.si() * C::earthMeanRadius.si();
        Console c;
        c.id = "nav";
        c.callsign = "NAV";
        c.title = "Navigation";
        c.question = "Can he keep up with midnight?";
        c.go = true;
        c.status = "GO — at this speed being in the right time zone is the easy part.";
        c.readouts = {{"Coriolis push", F::acceleration(coriolisGs)},
                      {"Midnight moves at", F::speed(midnightSpeed)},
                      {"Santa vs midnight", F::times(v / midnightSpeed)},
                      {"Time zones", std::to_string(world.zones.size())}};
        c.steps.push_back(makeStep(
            "coriolis", "Coriolis acceleration (worst case)", R"(a_{\text{Cor}} \le 2\Omega v)",
            "a ≤ 2 × Earth's rotation rate × v",
            "= 2 × " + F::scientific(C::earthRotationRate.si(), 4) + " rad/s × " + F::speed(v), F::acceleration(coriolisGs),
            {"iers"}));
        c.steps.push_back(makeStep(
            "midnight", "How fast midnight sweeps the equator", R"(v_{\text{midnight}} = \Omega R)",
            "speed = rotation rate × Earth's radius",
            "= " + F::scientific(C::earthRotationRate.si(), 4) + " rad/s × " + F::length(C::earthMeanRadius.si(), 5),
            F::speed(midnightSpeed), {"iers", "iugg-radius"}));
        c.steps.push_back(makeStep(
            "lap", "Time to lap the planet", R"(t = 2\pi (R+h) / v)", "t = circumference ÷ v",
            "= " + F::length(2 * kPi * radius, 4) + " ÷ " + F::speed(v), F::duration(2 * kPi * radius / v), {}));
        c.comparisons = {"Midnight crawls; Santa zig-zags. Almost all of his flying is back and forth inside "
                         "one time zone, not chasing the night westward.",
                         "Flying west also makes him slightly heavier (the Eötvös effect), but next to "
                             + F::acceleration(r.curvatureGs) + " nobody will notice."};
        c.params = {"windowLengthHours"};
        c.caveat = "Near the solstice it is still light at 22:00 at high southern latitudes, so 'night' there "
                   "means bedtime rather than darkness.";
        r.consoles.push_back(std::move(c));
    }

    // TIMING ----------------------------------------------------------------------------------------------
    {
        const double gammaMinusOne = P::lorentzGammaMinusOne(speed, C::speedOfLight);
        const double gravitational = g0 * altitude / std::pow(C::speedOfLight.si(), 2) * r.busyS;
        Console c;
        c.id = "timing";
        c.callsign = "TIMING";
        c.title = "Relativity";
        c.question = "Does relativity buy Santa time?";
        c.go = true;
        c.status = "GO — but it doesn't help: Santa's clock falls " + F::duration(r.clockLagS) + " behind.";
        c.readouts = {{"Fraction of light speed", F::percent(v / C::speedOfLight.si())},
                      {"γ − 1", F::scientific(gammaMinusOne)},
                      {"Santa's clock lag", F::duration(r.clockLagS)},
                      {"Route at light speed", F::duration(r.distanceM / C::speedOfLight.si())}};
        c.steps.push_back(makeStep(
            "beta", "Speed as a fraction of light", R"(\beta = v / c)", "β = v ÷ c",
            "= " + F::speed(v) + " ÷ " + F::grouped(C::speedOfLight.si()) + " m/s", F::scientific(v / C::speedOfLight.si()),
            {"si"}));
        c.steps.push_back(makeStep(
            "gamma", "Time dilation factor", R"(\gamma - 1 = \frac{1}{\sqrt{1-\beta^2}} - 1 \approx \tfrac12\beta^2)",
            "γ − 1 ≈ ½ β²", "", F::scientific(gammaMinusOne), {"si"}));
        c.steps.push_back(makeStep(
            "lag", "How far Santa's watch falls behind", R"(\Delta\tau = \sum_k (\gamma_k - 1)\, t_k)",
            "Σ (γ − 1) × time at each speed", "moving clocks run slow, so Santa gets less time, not more",
            F::duration(r.clockLagS), {"si"}));
        c.steps.push_back(makeStep(
            "gravity", "…partly paid back by flying higher", R"(\Delta\tau_g = \frac{g h}{c^2}\, t)",
            "g × h ÷ c² × time", "clocks run faster higher up", F::duration(gravitational), {"si"}));
        c.steps.push_back(makeStep(
            "lightspeed", "The whole route at the speed of light", R"(t = D / c)", "t = distance ÷ c",
            "= " + F::length(r.distanceM) + " ÷ c", F::duration(r.distanceM / C::speedOfLight.si()), {"si"}));
        c.comparisons = {"Even at light speed the route takes only " + F::duration(r.distanceM / C::speedOfLight.si())
                         + ", so the speed of light isn't what stops him. Everything else is."};
        c.params = {"altitudeM"};
        c.caveat = "Special-relativistic dilation from Santa's ground speed; Earth's own motion is common to "
                   "everyone and cancels.";
        r.consoles.push_back(std::move(c));
    }

    // DEBRIEF (Spy, 1990) -----------------------------------------------------------------------------
    {
        Console c;
        c.id = "debrief";
        c.callsign = "DEBRIEF";
        c.title = "The 1990 analysis, revisited";
        c.question = "Where did the famous version go wrong?";
        c.go = true;
        c.status = "GO — its conclusion survives; most of its numbers don't.";
        c.readouts = {{"Homes: 1990 vs now", F::count(kSpyHomes) + " → " + F::count(r.households)},
                      {"Speed", F::speed(kSpySpeedMps) + " → " + F::speed(v)},
                      {"g-force", F::acceleration(kSpyGs) + " → " + F::acceleration(r.curvatureGs)},
                      {"Rudolph lasts", F::duration(kSpyVaporiseS) + " → see THERMAL"}};
        c.comparisons = {
            "Averaging before a non-linear step: spacing homes evenly over all land overstates the route about "
            "three-fold. We sum √(N·A) cell by cell on a 1 km grid.",
            "Arithmetic: 15% of 2 billion is 300 million, not 378 million; and 378 million ÷ 3.5 is 108 million, "
            "not 91.8 million. 3.5 children per household matches no census.",
            "One pool of time: 31 hours shared evenly ignores that " + F::percent(r.bindingHouseholdShare)
                + " of households fall in " + F::duration(r.bindingSpanS) + ", and leaves out stop time entirely.",
            "Load instead of power: reindeer numbers depend on power (force × speed), not on how much each can pull.",
            "Formulas outside their range: an ideal-gas temperature for what is really a plasma, and false "
            "precision such as '17,500.06 g'. We show validity warnings and two significant figures."};
        c.params = {};
        c.caveat = "Spy magazine, January 1990, widely reprinted as 'Is there a Santa Claus?'.";
        r.consoles.push_back(std::move(c));
    }

    // FLIGHT (verdict) ----------------------------------------------------------------------------------
    {
        Console c;
        c.id = "flight";
        c.callsign = "FLIGHT";
        c.title = "Flight director";
        c.question = "GO / NO-GO for Christmas Eve?";
        int nogo = 0;
        for (const Console &other : r.consoles)
            nogo += other.go ? 0 : 1;
        c.go = false;
        c.status = "NO-GO on physics (" + std::to_string(nogo) + " consoles). GO on Christmas magic.";
        c.readouts = {{"Consoles NO-GO", std::to_string(nogo) + " of " + std::to_string(r.consoles.size())},
                      {"Power from reindeer", F::power(s[Param::ReindeerCount] * r.reindeerPowerW)},
                      {"Power needed", F::power(r.dragPowerW)},
                      {"Supplied by magic", magicShareText(s[Param::ReindeerCount] * r.reindeerPowerW, r.dragPowerW)}};
        for (const Console &other : r.consoles)
            c.comparisons.push_back(other.callsign + ": " + other.status);
        c.steps.push_back(makeStep(
            "magic", "Share of the power that must come from magic",
            R"(\mu = 1 - \frac{n\,p_r}{P})", "μ = 1 − (reindeer × power each) ÷ power needed",
            "= 1 − " + F::power(s[Param::ReindeerCount] * r.reindeerPowerW) + " ÷ " + F::power(r.dragPowerW),
            magicShareText(s[Param::ReindeerCount] * r.reindeerPowerW, r.dragPowerW), {}));
        c.params = {};
        c.caveat = "Every console runs on the same scenario. Change any slider and the whole room updates.";
        r.consoles.push_back(std::move(c));
    }

    return r;
}

} // namespace cr
