// SPDX-License-Identifier: GPL-3.0-or-later
#include "scenario.h"

#include <algorithm>
#include <cmath>

namespace cr {

const std::array<ParamInfo, kParamCount> &parameterTable()
{
    static const std::array<ParamInfo, kParamCount> table{{
        {Param::Eligibility, "eligibility", "Households visited", "%", 1, 100, 100, 1, false, "assumption",
         "Share of households with children that Santa visits. 100% is every child under 18 on Earth."},
        {Param::PresentsPerChild, "presentsPerChild", "Presents per child", "", 1, 10, 1, 1, false, "assumption",
         "How many presents each child receives."},
        {Param::GiftMassKg, "giftMassKg", "Mass per present", "kg", 0.1, 5, 1, 0.1, false, "assumption",
         "Average wrapped present. A board game is about 1 kg; a smartphone in a box about 0.5 kg."},
        {Param::WindowStartHour, "windowStartHour", "Delivery window opens", "h", 18, 26, 22, 0.5, false, "aasm-sleep",
         "Local time on Christmas Eve when children are reliably asleep. Values past 24 mean after midnight."},
        {Param::WindowLengthHours, "windowLengthHours", "Delivery window length", "h", 1, 12, 8, 0.5, false, "aasm-sleep",
         "How long every child in a time zone stays asleep. Children 6–12 need 9–12 hours of sleep."},
        {Param::StopTimeS, "stopTimeS", "Time at each stop", "s", 0, 60, 0, 0.001, true, "assumption",
         "Down the chimney, presents out, cookie eaten, back up. Zero means presents are dropped in flight."},
        {Param::RouteQuality, "routeQuality", "Route quality η", "×", 1, 1.5, 1, 0.01, false, "bhh-constant",
         "1 is a perfect travelling-salesman tour. Practical heuristics (space-filling curves) are about 1.25."},
        {Param::HouseholdsPerStop, "householdsPerStop", "Households per stop", "", 1, 50, 1, 1, true, "assumption",
         "Apartment blocks let one stop serve many households, shortening the horizontal route by √k."},
        {Param::AltitudeM, "altitudeM", "Cruise altitude", "m", 0, 50000, 100, 10, true, "ussa1976",
         "Height above sea level. Rooftops are about 10–100 m; thinner air higher up means less drag."},
        {Param::SleighMassKg, "sleighMassKg", "Empty sleigh mass", "kg", 50, 2000, 500, 10, false, "assumption",
         "A large wooden sleigh with runners and fittings."},
        {Param::SantaMassKg, "santaMassKg", "Santa’s mass", "kg", 60, 200, 120, 1, false, "assumption",
         "Before the cookies."},
        {Param::ReindeerCount, "reindeerCount", "Reindeer", "", 1, 20, 9, 1, false, "assumption",
         "Dasher, Dancer, Prancer, Vixen, Comet, Cupid, Donner, Blitzen and Rudolph."},
        {Param::ReindeerMassKg, "reindeerMassKg", "Reindeer mass", "kg", 60, 180, 100, 1, false, "reindeer-mass",
         "Draught reindeer weigh 60–180 kg."},
        {Param::DragCoefficient, "dragCoefficient", "Drag coefficient C_D", "", 0.3, 2, 1, 0.05, false, "hoerner",
         "A blunt sack of presents: about 1 (a sphere at hypersonic speed is about 0.9–1)."},
        {Param::HeatAbsorbedFraction, "heatAbsorbedFraction", "Heat absorbed Λ", "", 0.001, 1, 0.1, 0.001, true, "meteor-ablation",
         "Share of the oncoming air's kinetic energy that ends up heating the lead reindeer (meteors: 0.01–1)."},
        {Param::CookiesPerStop, "cookiesPerStop", "Cookies per stop", "", 0, 5, 1, 1, false, "assumption",
         "Santa is polite and eats what is left out."},
        {Param::CookieMassG, "cookieMassG", "Mass per cookie", "g", 5, 60, 15, 1, false, "usda-cookie",
         "A soft chocolate-chip cookie is about 15 g."},
        {Param::MilkPerStopMl, "milkPerStopMl", "Milk per stop", "mL", 0, 500, 150, 10, false, "assumption",
         "A small glass."},
    }};
    return table;
}

const ParamInfo &parameterInfo(Param id)
{
    return parameterTable()[std::size_t(id)];
}

const ParamInfo *parameterInfo(std::string_view key)
{
    for (const ParamInfo &info : parameterTable()) {
        if (info.key == key)
            return &info;
    }
    return nullptr;
}

Scenario::Scenario()
{
    for (const ParamInfo &info : parameterTable())
        values[std::size_t(info.id)] = info.defaultValue;
}

void Scenario::set(Param id, double value)
{
    const ParamInfo &info = parameterInfo(id);
    if (!std::isfinite(value))
        value = info.defaultValue;
    values[std::size_t(id)] = std::clamp(value, info.minimum, info.maximum);
}

bool Scenario::set(std::string_view key, double value)
{
    const ParamInfo *info = parameterInfo(key);
    if (!info)
        return false;
    set(info->id, value);
    return true;
}

double Scenario::windowStartRelativeS() const
{
    // 22 → −2 h (Christmas Eve evening), 25 → +1 h (01:00 on Christmas morning).
    return ((*this)[Param::WindowStartHour] - 24.0) * 3600.0;
}

} // namespace cr
