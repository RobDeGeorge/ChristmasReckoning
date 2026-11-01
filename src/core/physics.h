// SPDX-License-Identifier: GPL-3.0-or-later
// The individual formulas behind the Mission Control consoles. Each is small, typed with dimensional
// units, and unit-tested against hand-computed values.
#pragma once

#include "units.h"

namespace cr::physics {
using namespace cr::units;

// --- Route ----------------------------------------------------------------------------------------
// Beardwood–Halton–Hammersley: shortest tour through N random points spread over area A.
Length bhhTourLength(double points, Area area, double beta);

// --- Orbits and curvature -------------------------------------------------------------------------
Speed circularOrbitSpeed(GravParameter gm, Length radius);
Speed escapeSpeed(GravParameter gm, Length radius);
// Extra downward acceleration needed to keep following the curved surface at speed v
// (centripetal requirement minus what gravity provides). Negative means gravity is enough.
Acceleration curvatureDownforce(Speed v, Length radius, Acceleration g);

// --- Kinematics -----------------------------------------------------------------------------------
// Bang-bang hop: accelerate for half the distance, brake for the other half, in time t.
Acceleration hopAcceleration(Length hop, Time t);
Speed hopPeakSpeed(Length hop, Time t);
Length stoppingDistance(Speed v, Acceleration a);

// --- Aerodynamics ---------------------------------------------------------------------------------
Force dragForce(Density rho, Speed v, double dragCoefficient, Area frontalArea);
double machNumber(Speed v, Speed speedOfSound);
// Ideal-gas stagnation temperature T₀ = T + v²/(2·c_p). Only physical up to ~800 K.
Temperature stagnationTemperatureIdeal(Temperature ambient, Speed v, SpecificHeat cp);
// Upper bound once air is fully dissociated and ionised (monatomic gas, ionisation energy ignored).
Temperature stagnationTemperaturePlasmaBound(Speed v, Mass meanParticleMass, EnergyPerTemperature kB);
// Sutton–Graves stagnation-point convective heat flux q = k·√(ρ/rₙ)·v³ (fit for ~2–15 km/s).
HeatFlux suttonGravesHeatFlux(Density rho, Length noseRadius, Speed v, double k);
// Hard ceiling: a fraction Λ of the oncoming kinetic-energy flux ½ρv³.
HeatFlux kineticEnergyFlux(Density rho, Speed v);
// Time to deliver enough energy to vaporise a body of mass m and frontal area S.
Time vaporisationTime(Mass m, SpecificEnergy q, double absorbedFraction, Area s, Density rho, Speed v);

// --- Energy -----------------------------------------------------------------------------------------
Energy kineticEnergy(Mass m, Speed v);
// Radius at which a line of energy E_l per metre (a straight hypersonic shock) has spread into
// ambient pressure p₀: R₀ = √(E_l / p₀) — the characteristic blast radius of a cylindrical blast.
Length cylindricalBlastRadius(Force energyPerMetre, Pressure ambient);

// --- Relativity -------------------------------------------------------------------------------------
// γ − 1, computed without cancellation for small speeds.
double lorentzGammaMinusOne(Speed v, Speed c);

// --- Animals and people -----------------------------------------------------------------------------
// Allometric estimate of sustained power for an animal of mass m, scaled from a reference animal.
Power allometricPower(Mass m, Mass referenceMass, Power referencePower, double exponent);
// Compendium of Physical Activities: kcal/min = MET × 3.5 × mass(kg) / 200.
double kcalPerMinute(double met, Mass m);

} // namespace cr::physics
