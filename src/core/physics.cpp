// SPDX-License-Identifier: GPL-3.0-or-later
#include "physics.h"

#include <cmath>

namespace cr::physics {

Length bhhTourLength(double points, Area area, double beta)
{
    return Length(beta * std::sqrt(points * area.si()));
}

Speed circularOrbitSpeed(GravParameter gm, Length radius)
{
    return sqrt(gm / radius);
}

Speed escapeSpeed(GravParameter gm, Length radius)
{
    return sqrt(2.0 * gm / radius);
}

Acceleration curvatureDownforce(Speed v, Length radius, Acceleration g)
{
    return squared(v) / radius - g;
}

Acceleration hopAcceleration(Length hop, Time t)
{
    return 4.0 * hop / squared(t);
}

Speed hopPeakSpeed(Length hop, Time t)
{
    return 2.0 * hop / t;
}

Length stoppingDistance(Speed v, Acceleration a)
{
    return squared(v) / (2.0 * a);
}

Force dragForce(Density rho, Speed v, double dragCoefficient, Area frontalArea)
{
    return 0.5 * rho * squared(v) * frontalArea * dragCoefficient;
}

double machNumber(Speed v, Speed speedOfSound)
{
    return v / speedOfSound;
}

Temperature stagnationTemperatureIdeal(Temperature ambient, Speed v, SpecificHeat cp)
{
    return ambient + squared(v) / (2.0 * cp);
}

Temperature stagnationTemperaturePlasmaBound(Speed v, Mass meanParticleMass, EnergyPerTemperature kB)
{
    return meanParticleMass * squared(v) / (5.0 * kB);
}

HeatFlux suttonGravesHeatFlux(Density rho, Length noseRadius, Speed v, double k)
{
    // k has units kg^½·m⁻¹ so that k·√(ρ/rₙ)·v³ is W/m²; carry it as a plain number.
    return HeatFlux(k * std::sqrt(rho.si() / noseRadius.si()) * std::pow(v.si(), 3));
}

HeatFlux kineticEnergyFlux(Density rho, Speed v)
{
    return 0.5 * rho * cubed(v);
}

Time vaporisationTime(Mass m, SpecificEnergy q, double absorbedFraction, Area s, Density rho, Speed v)
{
    const Energy needed = m * q;
    const Power delivered = absorbedFraction * kineticEnergyFlux(rho, v) * s;
    return needed / delivered;
}

Energy kineticEnergy(Mass m, Speed v)
{
    return 0.5 * m * squared(v);
}

Length cylindricalBlastRadius(Force energyPerMetre, Pressure ambient)
{
    // Energy per unit length (J/m = N) over pressure (N/m²) is an area; its square root a length.
    return sqrt(energyPerMetre / ambient);
}

double lorentzGammaMinusOne(Speed v, Speed c)
{
    const double b2 = std::pow(v.si() / c.si(), 2);
    if (b2 < 1e-6)
        return b2 / 2 + 3 * b2 * b2 / 8 + 5 * b2 * b2 * b2 / 16;
    return 1.0 / std::sqrt(1.0 - b2) - 1.0;
}

Power allometricPower(Mass m, Mass referenceMass, Power referencePower, double exponent)
{
    return referencePower * std::pow(m / referenceMass, exponent);
}

double kcalPerMinute(double met, Mass m)
{
    return met * 3.5 * m.si() / 200.0;
}

} // namespace cr::physics
