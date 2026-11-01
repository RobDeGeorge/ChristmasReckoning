// SPDX-License-Identifier: GPL-3.0-or-later
// Known-answer tests for the formulas behind every console, and for the units system itself.
#include "atmosphere.h"
#include "constants.h"
#include "physics.h"
#include "units.h"

#include <QTest>

#include <cmath>
#include <type_traits>

using namespace cr::units;
using namespace cr::units::literals;
namespace P = cr::physics;
namespace C = cr::constants;

// The units system must reject dimension mistakes at compile time; these document the algebra.
static_assert(std::is_same_v<decltype(Length{} / Time{}), Speed>);
static_assert(std::is_same_v<decltype(Mass{} * Acceleration{}), Force>);
static_assert(std::is_same_v<decltype(Force{} * Speed{}), Power>);
static_assert(std::is_same_v<decltype(Force{} / Area{}), Pressure>);
static_assert(std::is_same_v<decltype(squared(Speed{}) / Length{}), Acceleration>);
static_assert(std::is_same_v<decltype(Energy{} / Mass{}), SpecificEnergy>);

namespace {
bool near(double actual, double expected, double relTol)
{
    return std::abs(actual - expected) <= relTol * std::abs(expected);
}
} // namespace

#define QVERIFY_NEAR(actual, expected, tol)                                                          \
    QVERIFY2(near((actual), (expected), (tol)),                                                      \
             qPrintable(QStringLiteral("%1 vs expected %2").arg((actual), 0, 'g', 12).arg((expected), 0, 'g', 12)))

class TestPhysics : public QObject
{
    Q_OBJECT

private slots:
    void literalsConvertToSi()
    {
        QCOMPARE((3.0_km).si(), 3000.0);
        QCOMPARE((2.0_h).si(), 7200.0);
        QCOMPARE((1.5_t).si(), 1500.0);
    }

    void standardAtmosphereMatchesTheTables()
    {
        // U.S. Standard Atmosphere 1976, Table I (geometric altitude).
        auto sl = cr::atmosphere::standardAtmosphere(0.0_m);
        QVERIFY_NEAR(sl.temperature.si(), 288.15, 1e-9);
        QVERIFY_NEAR(sl.pressure.si(), 101325.0, 1e-9);
        QVERIFY_NEAR(sl.density.si(), 1.2250, 1e-4);
        QVERIFY_NEAR(sl.speedOfSound.si(), 340.29, 1e-4);

        auto tropopause = cr::atmosphere::standardAtmosphere(Length(11019.0)); // h = 11 km geopotential
        QVERIFY_NEAR(tropopause.temperature.si(), 216.65, 1e-4);
        QVERIFY_NEAR(tropopause.pressure.si(), 22632.0, 1e-3);
        QVERIFY_NEAR(tropopause.density.si(), 0.36392, 1e-3);

        auto twenty = cr::atmosphere::standardAtmosphere(20.0_km);
        QVERIFY_NEAR(twenty.temperature.si(), 216.65, 1e-4);
        QVERIFY_NEAR(twenty.density.si(), 0.088910, 2e-3);

        auto fifty = cr::atmosphere::standardAtmosphere(50.0_km);
        QVERIFY_NEAR(fifty.temperature.si(), 270.65, 1e-3);
        QVERIFY_NEAR(fifty.density.si(), 1.0269e-3, 5e-3);
    }

    void bhhTourScalesWithRootNA()
    {
        // 100 random points in 1 km²: 0.7124 × √(100 × 10⁶ m²) = 7,124 m.
        QVERIFY_NEAR(P::bhhTourLength(100, Area(1e6), C::bhhBeta).si(), 7124.0, 1e-12);
    }

    void orbitalAndEscapeSpeeds()
    {
        const double orbit = P::circularOrbitSpeed(C::earthGM, C::earthMeanRadius).si();
        QVERIFY_NEAR(orbit, 7909.8, 1e-4);
        QVERIFY_NEAR(P::escapeSpeed(C::earthGM, C::earthMeanRadius).si(), orbit * std::sqrt(2.0), 1e-12);
    }

    void downforceAtAMillionMetresPerSecond()
    {
        const double a = P::curvatureDownforce(Speed(1e6), C::earthMeanRadius, C::standardGravity).si();
        QVERIFY_NEAR(a, 1e12 / 6371008.8 - 9.80665, 1e-12);
        QVERIFY_NEAR(a / 9.80665, 16004.4, 1e-4);
    }

    void bangBangHop()
    {
        // 100 m in 1 s: accelerate 50 m, brake 50 m. a = 4·ℓ/t² = 400 m/s², peak = 200 m/s.
        QVERIFY_NEAR(P::hopAcceleration(100.0_m, 1.0_s).si(), 400.0, 1e-12);
        QVERIFY_NEAR(P::hopPeakSpeed(100.0_m, 1.0_s).si(), 200.0, 1e-12);
        QVERIFY_NEAR(P::stoppingDistance(Speed(100), Acceleration(10)).si(), 500.0, 1e-12);
    }

    void dragAndMach()
    {
        QVERIFY_NEAR(P::dragForce(Density(1.225), Speed(100), 1.0, Area(2)).si(), 12250.0, 1e-12);
        QVERIFY_NEAR(P::machNumber(Speed(680.58), Speed(340.29)), 2.0, 1e-12);
    }

    void stagnationTemperatures()
    {
        QVERIFY_NEAR(P::stagnationTemperatureIdeal(Temperature(288.15), Speed(1000), C::airSpecificHeatCp).si(),
                     288.15 + 1e6 / (2 * 1004.68), 1e-12);
        const double plasma = P::stagnationTemperaturePlasmaBound(
            Speed(1e6), C::fullyIonisedAirParticleMassAmu * C::atomicMassUnit, C::boltzmann).si();
        QVERIFY_NEAR(plasma, 1.76 * 1.66053906892e-27 * 1e12 / (5 * 1.380649e-23), 1e-12);
        QVERIFY_NEAR(plasma, 4.234e7, 1e-3);
    }

    void heatingFormulas()
    {
        // Sutton–Graves at 7 km/s, sea level, 10 cm nose: 1.7415e-4 × √12.25 × 7000³.
        QVERIFY_NEAR(P::suttonGravesHeatFlux(Density(1.225), Length(0.1), Speed(7000), C::suttonGravesK).si(),
                     1.7415e-4 * 3.5 * 3.43e11, 1e-12);
        QVERIFY_NEAR(P::kineticEnergyFlux(Density(1.225), Speed(1000)).si(), 6.125e8, 1e-12);
        // 100 kg, 2.6 MJ/kg, Λ = 0.1, 0.6 m², sea level, 1,000 km/s.
        QVERIFY_NEAR(P::vaporisationTime(Mass(100), SpecificEnergy(2.6e6), 0.1, Area(0.6), Density(1.225),
                                         Speed(1e6)).si(),
                     2.6e8 / (0.1 * 0.5 * 1.225 * 1e18 * 0.6), 1e-12);
    }

    void energyAndBlast()
    {
        QVERIFY_NEAR(P::kineticEnergy(Mass(2), Speed(3)).si(), 9.0, 1e-12);
        QVERIFY_NEAR(P::cylindricalBlastRadius(Force(1e9), Pressure(1e5)).si(), 100.0, 1e-12);
    }

    void lorentzFactor()
    {
        QVERIFY_NEAR(P::lorentzGammaMinusOne(Speed(0.6 * 299792458.0), C::speedOfLight), 0.25, 1e-12);
        // Small speeds use the series, avoiding catastrophic cancellation: β = 10⁻³ → β²/2.
        QVERIFY_NEAR(P::lorentzGammaMinusOne(Speed(299792.458), C::speedOfLight), 5e-7 + 3.75e-13, 1e-9);
    }

    void animalsAndPeople()
    {
        // 745.7 W × (100/600)^0.87 ≈ 157 W.
        QVERIFY_NEAR(P::allometricPower(Mass(100), Mass(600), Power(745.7), 0.87).si(),
                     745.7 * std::pow(100.0 / 600.0, 0.87), 1e-12);
        QVERIFY_NEAR(P::allometricPower(Mass(100), Mass(600), Power(745.7), 0.87).si(), 156.9, 2e-3);
        QVERIFY_NEAR(P::kcalPerMinute(3.5, Mass(120)), 7.35, 1e-12);
    }
};

QTEST_GUILESS_MAIN(TestPhysics)
#include "tst_physics.moc"
