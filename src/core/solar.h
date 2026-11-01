// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace cr::solar {

struct SubsolarPoint
{
    double latitudeDeg = 0;   // equals the solar declination
    double longitudeDeg = 0;  // in [-180, 180)
    double equationOfTimeMinutes = 0;
};

struct Vec3
{
    double x = 0, y = 0, z = 0;
};

// Sun position using NOAA's solar calculator equations (Meeus, "Astronomical Algorithms").
// Accuracy is about 0.01 degrees for years 1800-2100, far better than the globe needs.
SubsolarPoint subsolarPoint(std::int64_t epochMs);

// Unit vector towards the Sun in Earth-centred, Earth-fixed coordinates:
// x through (0°N, 0°E), y through (0°N, 90°E), z through the North Pole.
Vec3 sunDirectionEcef(std::int64_t epochMs);

// Unit ECEF vector for a geographic position.
Vec3 ecefFromLatLon(double latitudeDeg, double longitudeDeg);

} // namespace cr::solar
