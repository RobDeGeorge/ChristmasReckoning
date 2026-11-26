// SPDX-License-Identifier: GPL-3.0-or-later
// Whole-pipeline checks on a synthetic world: totals, schedule wiring, the timeline's invariants,
// and the binary world format round trip.
#include "flightplan.h"
#include "testworld.h"
#include "timeline.h"

#include <QTest>

#include <cmath>

class TestFlightPlan : public QObject
{
    Q_OBJECT

private slots:
    void worldFormatRoundTrips()
    {
        const cr::WorldData world = crtest::makeWorld();
        QString error;
        const auto parsed = cr::WorldData::fromBytes(world.toBytes(), &error);
        QVERIFY2(parsed, qPrintable(error));
        QCOMPARE(parsed->zones.size(), world.zones.size());
        QCOMPARE(parsed->zones[1].offsetMinutes, 0);
        QCOMPARE(parsed->zones[2].label(), QStringLiteral("UTC−05:00"));
        QCOMPARE(parsed->zones[1].households, world.zones[1].households);
        QCOMPARE(parsed->zones[1].checkpoints.back().rank, world.zones[1].checkpoints.back().rank);
        QCOMPARE(parsed->countries.front().name, QStringLiteral("Testland"));

        QByteArray truncated = world.toBytes();
        truncated.chop(3);
        QVERIFY(!cr::WorldData::fromBytes(truncated));
        QVERIFY(!cr::WorldData::fromBytes(QByteArrayLiteral("nope")));
    }

    void everyHouseholdIsOnTheRoute()
    {
        const auto world = crtest::makeWorld();
        const auto plan = cr::planFlight(world, 2026);
        QCOMPARE(plan.households, world.totalHouseholds());
        QCOMPARE(plan.zones.size(), world.zones.size());
    }

    void scheduleUsesWindowsAndOffsets()
    {
        const auto world = crtest::makeWorld();
        const auto plan = cr::planFlight(world, 2026);
        // Window 22:00–06:00: UTC+10 opens at 12:00 UTC on the 24th, i.e. −12 h from the reference.
        QCOMPARE(plan.zones[0].windowOpenS, -12.0 * 3600);
        QCOMPARE(plan.zones[2].windowCloseS, (6.0 + 5.0) * 3600);
        for (const auto &z : plan.zones) {
            QVERIFY(z.speedMps > 0 && std::isfinite(z.speedMps));
            QVERIFY(z.startS >= z.windowOpenS - 1e-6);
            QVERIFY(z.endS <= z.windowCloseS + 1e-3);
        }
        QCOMPARE(plan.launchS, plan.zones.front().startS);
        QVERIFY(plan.landingS > plan.lastDeliveryS);
    }

    void timelineIsMonotoneAndEndsAtHome()
    {
        const auto world = crtest::makeWorld();
        const auto r = cr::planFlight(world, 2026);
        const double begin = r.referenceEpochMs + (r.launchS - 3600) * 1000;
        const double end = r.referenceEpochMs + (r.landingS + 3600) * 1000;

        const auto before = cr::stateAt(world, r, begin);
        QCOMPARE(before.phase, cr::Phase::PreLaunch);
        QCOMPARE(before.latitudeDeg, 90.0);
        QCOMPARE(before.householdsDelivered, 0.0);

        double lastHouseholds = 0, lastDistance = 0;
        int lastZone = 0;
        double lastRank = 0;
        for (int i = 0; i <= 2000; ++i) {
            const double t = begin + (end - begin) * i / 2000.0;
            const auto state = cr::stateAt(world, r, t);
            QVERIFY(state.householdsDelivered >= lastHouseholds - 1e-6);
            QVERIFY(state.distanceM >= lastDistance - 1e-3);
            QVERIFY(state.frontierZone > lastZone || (state.frontierZone == lastZone && state.frontierRank >= lastRank - 1e-9));
            QVERIFY(state.latitudeDeg >= -90 && state.latitudeDeg <= 90);
            lastHouseholds = state.householdsDelivered;
            lastDistance = state.distanceM;
            lastZone = state.frontierZone;
            lastRank = state.frontierRank;
        }
        const auto after = cr::stateAt(world, r, end);
        QCOMPARE(after.phase, cr::Phase::Landed);
        QVERIFY(std::abs(after.householdsDelivered - r.households) < 1e-6 * r.households);
        QVERIFY(std::abs(after.distanceM - r.distanceM) < 1e-6 * r.distanceM);
        QVERIFY(std::abs(after.latitudeDeg - 90) < 1e-6);
        QVERIFY(std::abs(after.presentsDelivered - r.children) < 1e-6 * r.children);
    }

    void timelineIsDeterministic()
    {
        const auto world = crtest::makeWorld();
        const auto r = cr::planFlight(world, 2026);
        const double t = r.referenceEpochMs + r.zones[1].startS * 1000 + 1234567;
        const auto a = cr::stateAt(world, r, t);
        const auto b = cr::stateAt(world, cr::planFlight(world, 2026), t);
        QCOMPARE(a.latitudeDeg, b.latitudeDeg);
        QCOMPARE(a.longitudeDeg, b.longitudeDeg);
        QCOMPARE(a.householdsDelivered, b.householdsDelivered);
    }

    void christmasEpochIsCorrect()
    {
        // 2026-12-25T00:00:00Z = 1798156800 s since the epoch.
        QCOMPARE(cr::christmasUtcEpochMs(2026), 1798156800000.0);
        QCOMPARE(cr::christmasUtcEpochMs(2000), 977702400000.0);
    }
};

QTEST_GUILESS_MAIN(TestFlightPlan)
#include "tst_flightplan.moc"
