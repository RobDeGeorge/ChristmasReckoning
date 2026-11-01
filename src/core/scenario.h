// SPDX-License-Identifier: GPL-3.0-or-later
// Every assumption the user can change, with its default, range and source. Defaults are generous to
// Santa wherever a choice exists (perfect routing, no stop time, rooftop altitude).
#pragma once

#include <array>
#include <string_view>

namespace cr {

enum class Param {
    Eligibility,        // share of households with children that Santa visits
    PresentsPerChild,
    GiftMassKg,
    WindowStartHour,    // local time when Santa may start (Christmas Eve)
    WindowLengthHours,  // how long children are reliably asleep
    StopTimeS,          // time spent at each stop (down the chimney and back)
    RouteQuality,       // η: 1 = optimal tour, 1.25 = good heuristic
    HouseholdsPerStop,  // apartment blocks: households served from one stop
    AltitudeM,
    SleighMassKg,       // empty sleigh
    SantaMassKg,
    ReindeerCount,
    ReindeerMassKg,
    DragCoefficient,
    HeatAbsorbedFraction, // Λ: share of the oncoming kinetic-energy flux that heats the lead reindeer
    CookiesPerStop,
    CookieMassG,
    MilkPerStopMl,
    Count
};

struct ParamInfo
{
    Param id;
    std::string_view key;
    std::string_view label;
    std::string_view unit;
    double minimum;
    double maximum;
    double defaultValue;
    double step;
    bool logScale;
    std::string_view sourceKey; // sources registry key, or "assumption"
    std::string_view help;
};

inline constexpr int kParamCount = int(Param::Count);

const std::array<ParamInfo, kParamCount> &parameterTable();
const ParamInfo &parameterInfo(Param id);
const ParamInfo *parameterInfo(std::string_view key);

struct Scenario
{
    std::array<double, kParamCount> values{};
    int year = 2026; // which Christmas Eve (sets the clock times; zone data is for the data year)

    Scenario();

    double operator[](Param id) const { return values[std::size_t(id)]; }
    double &operator[](Param id) { return values[std::size_t(id)]; }

    // Clamps to the parameter's range. Returns false for unknown keys.
    bool set(std::string_view key, double value);
    void set(Param id, double value);

    double windowLengthS() const { return (*this)[Param::WindowLengthHours] * 3600.0; }
    // Window start relative to local midnight starting December 25, in seconds (22:00 → −7200).
    double windowStartRelativeS() const;
};

} // namespace cr
