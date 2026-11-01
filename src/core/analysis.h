// SPDX-License-Identifier: GPL-3.0-or-later
// Turns the world data and the user's scenario into everything Mission Control shows: the schedule,
// the headline numbers, and the console write-ups (each step: formula, substituted values, result,
// validity warning and sources).
#pragma once

#include "scenario.h"
#include "schedule.h"
#include "worlddata.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cr {

struct Step
{
    std::string id;
    std::string label;        // what this line computes
    std::string tex;          // symbolic formula (rendered to SVG at build time)
    std::string plain;        // the same formula in plain Unicode (fallback and accessibility)
    std::string substitution; // the formula with today's numbers plugged in
    std::string result;       // "1,500 km/s"
    std::string validity;     // non-empty when the formula is being used outside its range
    std::vector<std::string> sources;
};

struct Readout
{
    std::string label;
    std::string value;
};

struct Console
{
    std::string id;        // url-safe, e.g. "fido"
    std::string callsign;  // "FIDO"
    std::string title;     // "Flight Dynamics"
    std::string question;  // the question this console answers
    bool go = false;
    std::string status;    // one-line GO / NO-GO reason
    std::vector<Readout> readouts;
    std::vector<Step> steps;
    std::vector<std::string> comparisons;
    std::vector<std::string> params;  // scenario parameter keys that matter here
    std::string caveat;
};

struct ZoneResult
{
    int index = 0;
    std::string label;
    int offsetMinutes = 0;
    double households = 0;      // visited
    double children = 0;
    double presents = 0;
    double stops = 0;
    double pathM = 0;           // tour inside the zone (incl. gaps)
    double transitionM = 0;
    double windowOpenS = 0;     // seconds relative to the reference epoch (25 Dec 00:00 UTC)
    double windowCloseS = 0;
    double startS = 0;
    double endS = 0;
    double transitionTimeS = 0;
    double deliveryTimeS = 0;
    double speedMps = 0;
    double massStartKg = 0;
    double dragEnergyJ = 0;
    std::vector<std::string> countries; // biggest first
    std::vector<float> checkpointWork;  // cumulative share of delivery time at each checkpoint
};

struct Results
{
    double referenceEpochMs = 0; // 25 December 00:00 UTC of the scenario year
    bool feasible = false;       // with the requested stop time
    std::string feasibilityNote;

    // What gets delivered
    double children = 0;
    double households = 0;
    double presents = 0;
    double stops = 0;

    // Route
    double pathM = 0;        // Σ BHH tours
    double gapM = 0;
    double transitionM = 0;  // zone-to-zone hops, including leaving the North Pole
    double returnHomeM = 0;
    double distanceM = 0;    // everything flown
    double averageHopM = 0;

    // Time
    double windowS = 0;
    double nightSpanS = 0;   // first window opening → last window closing
    double launchS = 0;      // relative to the reference epoch
    double lastDeliveryS = 0;
    double landingS = 0;
    double busyS = 0;        // time actually moving or delivering
    double timePerStopS = 0;
    double maxStopTimeS = 0; // τ_max: largest stop time any speed could accommodate

    // Speed
    double peakSpeedMps = 0;     // minimum possible peak (Horn bottleneck)
    double averageSpeedMps = 0;
    int bindingFirst = -1;
    int bindingLast = -1;
    double bindingSpanS = 0;
    double bindingHouseholdShare = 0;

    // Mass
    double baseMassKg = 0;
    double payloadKg = 0;
    double launchMassKg = 0;
    double sackDiameterM = 0;
    double frontalAreaM2 = 0;

    // Physics at the bottleneck
    double airDensity = 0;
    double airTemperatureK = 0;
    double speedOfSoundMps = 0;
    double mach = 0;
    double dragForceN = 0;
    double dragPowerW = 0;
    double kineticEnergyJ = 0;
    double dragWorkJ = 0;
    double curvatureGs = 0;
    double stopGoAccelMps2 = 0;
    double reindeerPowerW = 0;   // sustained, each
    double reindeerNeeded = 0;
    double magicShare = 0;       // share of the power that has to come from somewhere else
    double clockLagS = 0;

    std::vector<ZoneResult> zones;
    std::vector<Console> consoles;
    ScheduleResult schedule;
};

// Milliseconds since the Unix epoch of 25 December 00:00 UTC of `year`.
double christmasUtcEpochMs(int year);

Results analyse(const WorldData &world, const Scenario &scenario);

} // namespace cr
