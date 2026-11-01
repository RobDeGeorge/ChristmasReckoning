// SPDX-License-Identifier: GPL-3.0-or-later
// Physical constants and reference figures. Every value names its source (see sources.cpp for the
// full citations shown in the app's "Sources & assumptions" panel).
#pragma once

#include "units.h"

namespace cr::constants {
using namespace cr::units;

// --- Exact SI defining constants (BIPM, SI Brochure 9th ed., 2019) -----------------------------
inline constexpr Speed speedOfLight{299'792'458.0};                       // [si]
inline constexpr EnergyPerTemperature boltzmann{1.380649e-23};            // [si]

// --- CODATA 2022 ---------------------------------------------------------------------------------
inline constexpr Mass atomicMassUnit{1.66053906892e-27};                  // [codata]

// --- Earth -----------------------------------------------------------------------------------
inline constexpr Acceleration standardGravity{9.80665};                   // [cgpm-g0]
inline constexpr GravParameter earthGM{3.986004418e14};                   // [iers]
inline constexpr Length earthMeanRadius{6'371'008.8};                     // [iugg-radius]
inline constexpr Frequency earthRotationRate{7.292115e-5};                // [iers] rad/s
inline constexpr Time siderealDay{86'164.0905};                           // [iers]

// --- Air (US Standard Atmosphere 1976) -----------------------------------------------------------
inline constexpr double airHeatCapacityRatio = 1.4;                       // [ussa1976]
inline constexpr SpecificHeat airGasConstant{287.05287};                  // [ussa1976] J/(kg·K)
inline constexpr SpecificHeat airSpecificHeatCp{1004.68};                 // cp = γR/(γ−1) [ussa1976]
inline constexpr Pressure seaLevelPressure{101'325.0};                    // [ussa1976]
// Mean particle mass of fully ionised air: a nitrogen nucleus (14 u) plus its 7 electrons is 8
// particles, 1.75 u each (oxygen: 16/9 = 1.78 u). Used for the plasma bound on stagnation temperature.
inline constexpr double fullyIonisedAirParticleMassAmu = 1.76;            // derived
inline constexpr double airMolarMassAmu = 28.9644;                        // [ussa1976]

// --- Aerothermodynamics --------------------------------------------------------------------------
// Sutton–Graves stagnation-point convective heating constant for Earth air, q = k·√(ρ/rₙ)·v³.
inline constexpr double suttonGravesK = 1.7415e-4;                        // [sutton-graves] kg^½/m
inline constexpr double suttonGravesMaxValidSpeed = 15'000.0;             // m/s, upper end of the fit
// Energy needed to heat and vaporise tissue-like material (water-dominated): ~2.6 MJ/kg.
inline constexpr SpecificEnergy vaporisationEnergy{2.6e6};                // [water-hvap]

// --- Geometry / route ----------------------------------------------------------------------------
// Beardwood–Halton–Hammersley constant for random Euclidean TSP tours, L ≈ β·√(N·A).
inline constexpr double bhhBeta = 0.7124;                                 // [bhh-constant]

// --- Energy yardsticks ---------------------------------------------------------------------------
inline constexpr Energy tonOfTnt{4.184e9};                                // [tnt-convention]
inline constexpr Energy worldPrimaryEnergyPerYear{5.92e20};               // [ei-review-2025] 2024: 592 EJ
inline constexpr Power solarLuminosity{3.828e26};                         // [iau-2015]

// --- Speed yardsticks ----------------------------------------------------------------------------
inline constexpr Speed rifleBulletSpeed{940.0};                           // [bullet] .30-06 muzzle velocity
inline constexpr Speed concordeCruiseSpeed{605.0};                        // [concorde] Mach 2.04 at FL600
inline constexpr Speed issOrbitalSpeed{7'660.0};                          // [iss]
inline constexpr Speed parkerSolarProbeSpeed{192'200.0};                  // [parker] 24 Dec 2024 perihelion
inline constexpr Speed milkyWayEscapeSpeed{528'000.0};                    // [galactic-escape] at the Sun

// --- People, reindeer, snacks --------------------------------------------------------------------
inline constexpr double humanSustainedGTolerance = 5.0;                   // [nasa-g-tolerance]
inline constexpr double humanBriefGTolerance = 40.0;                      // [nasa-g-tolerance] restrained, < 0.1 s
inline constexpr Mass horseMass{600.0};                                   // [horsepower]
inline constexpr Power horseSustainedPower{745.7};                        // [horsepower] 1 hp
inline constexpr double metabolicPowerExponent = 0.87;                    // [weibel-2005] maximal aerobic scaling
inline constexpr double cookieEnergyKcalPer100g = 444.0;                  // [usda-cookie]
inline constexpr double kcalToJoule = 4184.0;
inline constexpr Volume olympicPoolVolume{2'500.0};                       // [fina-pool] 50 × 25 × 2 m
inline constexpr Density presentChargeableDensity{166.67};                // [iata-volumetric] 6000 cm³/kg

} // namespace cr::constants
