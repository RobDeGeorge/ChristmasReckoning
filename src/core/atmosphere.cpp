// SPDX-License-Identifier: GPL-3.0-or-later
#include "atmosphere.h"
#include "constants.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace cr::atmosphere {
namespace {

struct Layer
{
    double baseGeopotentialM;
    double lapseRateKPerM;
    double baseTemperatureK;
    double basePressurePa;
};

// Table 4 of U.S. Standard Atmosphere, 1976 (NOAA-S/T 76-1562).
constexpr std::array<Layer, 7> kLayers{{
    {0.0, -0.0065, 288.15, 101325.0},
    {11000.0, 0.0, 216.65, 22632.06},
    {20000.0, 0.0010, 216.65, 5474.889},
    {32000.0, 0.0028, 228.65, 868.0187},
    {47000.0, 0.0, 270.65, 110.9063},
    {51000.0, -0.0028, 270.65, 66.93887},
    {71000.0, -0.0020, 214.65, 3.956420},
}};

constexpr double kEarthRadiusForGeopotential = 6356766.0; // r0 in the standard, metres
constexpr double kG0 = 9.80665;
constexpr double kR = 287.05287;

} // namespace

State standardAtmosphere(units::Length geometricAltitude)
{
    const double z = std::clamp(geometricAltitude.si(), 0.0, 86000.0);
    const double h = kEarthRadiusForGeopotential * z / (kEarthRadiusForGeopotential + z);

    const Layer *layer = &kLayers.front();
    for (const Layer &candidate : kLayers) {
        if (h >= candidate.baseGeopotentialM)
            layer = &candidate;
    }

    const double dh = h - layer->baseGeopotentialM;
    const double temperature = layer->baseTemperatureK + layer->lapseRateKPerM * dh;
    double pressure;
    if (layer->lapseRateKPerM == 0.0) {
        pressure = layer->basePressurePa * std::exp(-kG0 * dh / (kR * layer->baseTemperatureK));
    } else {
        pressure = layer->basePressurePa
            * std::pow(layer->baseTemperatureK / temperature, kG0 / (kR * layer->lapseRateKPerM));
    }

    State state;
    state.temperature = units::Temperature(temperature);
    state.pressure = units::Pressure(pressure);
    state.density = units::Density(pressure / (kR * temperature));
    state.speedOfSound = units::Speed(std::sqrt(constants::airHeatCapacityRatio * kR * temperature));
    return state;
}

} // namespace cr::atmosphere
