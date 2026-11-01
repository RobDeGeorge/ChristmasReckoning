// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "units.h"

namespace cr::atmosphere {

struct State
{
    units::Temperature temperature;
    units::Pressure pressure;
    units::Density density;
    units::Speed speedOfSound;
};

// U.S. Standard Atmosphere, 1976, for geometric altitudes from 0 to 86 km (clamped outside).
State standardAtmosphere(units::Length geometricAltitude);

} // namespace cr::atmosphere
