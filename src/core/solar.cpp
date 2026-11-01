// SPDX-License-Identifier: GPL-3.0-or-later
#include "solar.h"

#include <cmath>
#include <numbers>

namespace cr::solar {
namespace {

constexpr double kDegToRad = std::numbers::pi / 180.0;

double wrapDegrees(double degrees)
{
    double wrapped = std::fmod(degrees + 180.0, 360.0);
    if (wrapped < 0)
        wrapped += 360.0;
    return wrapped - 180.0;
}

} // namespace

SubsolarPoint subsolarPoint(std::int64_t epochMs)
{
    const double julianDay = double(epochMs) / 86400000.0 + 2440587.5;
    const double t = (julianDay - 2451545.0) / 36525.0; // Julian centuries since J2000.0

    const double meanLongitude = std::fmod(280.46646 + t * (36000.76983 + t * 0.0003032), 360.0);
    const double meanAnomaly = 357.52911 + t * (35999.05029 - 0.0001537 * t);
    const double eccentricity = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);

    const double m = meanAnomaly * kDegToRad;
    const double equationOfCentre = std::sin(m) * (1.914602 - t * (0.004817 + 0.000014 * t))
        + std::sin(2 * m) * (0.019993 - 0.000101 * t) + std::sin(3 * m) * 0.000289;
    const double trueLongitude = meanLongitude + equationOfCentre;
    const double omega = (125.04 - 1934.136 * t) * kDegToRad;
    const double apparentLongitude = trueLongitude - 0.00569 - 0.00478 * std::sin(omega);

    const double meanObliquity =
        23.0 + (26.0 + (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0;
    const double obliquity = (meanObliquity + 0.00256 * std::cos(omega)) * kDegToRad;

    const double declination =
        std::asin(std::sin(obliquity) * std::sin(apparentLongitude * kDegToRad));

    const double y = std::pow(std::tan(obliquity / 2), 2);
    const double l0 = meanLongitude * kDegToRad;
    const double equationOfTime = 4.0 / kDegToRad
        * (y * std::sin(2 * l0) - 2 * eccentricity * std::sin(m)
           + 4 * eccentricity * y * std::sin(m) * std::cos(2 * l0)
           - 0.5 * y * y * std::sin(4 * l0) - 1.25 * eccentricity * eccentricity * std::sin(2 * m));

    const double msOfDay = std::fmod(double(epochMs), 86400000.0);
    const double utcMinutes = (msOfDay < 0 ? msOfDay + 86400000.0 : msOfDay) / 60000.0;

    SubsolarPoint point;
    point.latitudeDeg = declination / kDegToRad;
    point.equationOfTimeMinutes = equationOfTime;
    point.longitudeDeg = wrapDegrees(-(utcMinutes - 720.0 + equationOfTime) / 4.0);
    return point;
}

Vec3 ecefFromLatLon(double latitudeDeg, double longitudeDeg)
{
    const double lat = latitudeDeg * kDegToRad;
    const double lon = longitudeDeg * kDegToRad;
    return {std::cos(lat) * std::cos(lon), std::cos(lat) * std::sin(lon), std::sin(lat)};
}

Vec3 sunDirectionEcef(std::int64_t epochMs)
{
    const SubsolarPoint point = subsolarPoint(epochMs);
    return ecefFromLatLon(point.latitudeDeg, point.longitudeDeg);
}

} // namespace cr::solar
