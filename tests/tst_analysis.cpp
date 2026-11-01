// SPDX-License-Identifier: GPL-3.0-or-later
// Whole-pipeline checks on a synthetic world: totals, schedule wiring, console integrity, the
// timeline's invariants, and the binary world format round trip.
#include "analysis.h"
#include "sources.h"
#include "testworld.h"
#include "timeline.h"

#include <QSet>
#include <QTest>

#include <cmath>

class TestAnalysis : public QObject
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

    void totalsFollowEligibility()
    {
        const auto world = crtest::makeWorld();
        cr::Scenario s;
        auto all = cr::analyse(world, s);
        QCOMPARE(all.households, world.totalHouseholds());
        s.set(cr::Param::Eligibility, 25);
        auto quarter = cr::analyse(world, s);
        QVERIFY(std::abs(quarter.households - 0.25 * all.households) < 1e-6 * all.households);
        // Tour length scales with √(share) for randomly mixed households.
        QVERIFY(std::abs(quarter.pathM - 0.5 * all.pathM) < 1e-9 * all.pathM);
    }

    void scheduleUsesWindowsAndOffsets()
    {
        const auto world = crtest::makeWorld();
        cr::Scenario s;
        const auto r = cr::analyse(world, s);
        QVERIFY(r.feasible);
        // Window 22:00–06:00: UTC+10 opens at 12:00 UTC on the 24th, i.e. −12 h from the reference.
        QCOMPARE(r.zones[0].windowOpenS, -12.0 * 3600);
        QCOMPARE(r.zones[2].windowCloseS, (6.0 + 5.0) * 3600);
        QCOMPARE(r.nightSpanS, (8.0 + 15.0) * 3600);
        QVERIFY(r.peakSpeedMps > 0 && std::isfinite(r.peakSpeedMps));
        for (const auto &z : r.zones) {
            QVERIFY(z.startS >= z.windowOpenS - 1e-6);
            QVERIFY(z.endS <= z.windowCloseS + 1e-3);
        }
    }

    void tooLongAStopIsReportedNotHidden()
    {
        const auto world = crtest::makeWorld();
        cr::Scenario s;
        const auto baseline = cr::analyse(world, s);
        s.set(cr::Param::StopTimeS, baseline.maxStopTimeS * 2);
        const auto r = cr::analyse(world, s);
        QVERIFY(!r.feasible);
        QVERIFY(!r.feasibilityNote.empty());
        QVERIFY(std::isfinite(r.peakSpeedMps)); // physics falls back to fly-by delivery
    }

    void consolesAreCompleteAndCited()
    {
        const auto world = crtest::makeWorld();
        const auto r = cr::analyse(world, cr::Scenario());
        QCOMPARE(int(r.consoles.size()), 13);
        QSet<QString> ids;
        for (const cr::Console &c : r.consoles) {
            QVERIFY2(!ids.contains(QString::fromStdString(c.id)), c.id.c_str());
            ids.insert(QString::fromStdString(c.id));
            QVERIFY(!c.callsign.empty() && !c.status.empty() && !c.question.empty());
            QVERIFY(!c.readouts.empty());
            for (const cr::Readout &readout : c.readouts) {
                const QString value = QString::fromStdString(readout.value);
                QVERIFY2(!value.contains(u"nan") && !value.isEmpty(), (c.id + ": " + readout.label).c_str());
            }
            for (const cr::Step &step : c.steps) {
                QVERIFY(!step.tex.empty() && !step.plain.empty() && !step.result.empty());
                for (const std::string &key : step.sources)
                    QVERIFY2(cr::findSource(key), (c.id + "/" + step.id + " cites unknown source " + key).c_str());
            }
            for (const std::string &param : c.params)
                QVERIFY2(cr::parameterInfo(param), (c.id + " lists unknown parameter " + param).c_str());
        }
        for (const cr::ParamInfo &info : cr::parameterTable())
            QVERIFY2(cr::findSource(info.sourceKey), std::string(info.key).c_str());
    }

    void timelineIsMonotoneAndEndsAtHome()
    {
        const auto world = crtest::makeWorld();
        const auto r = cr::analyse(world, cr::Scenario());
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
        QVERIFY(std::abs(after.massKg - r.baseMassKg) < 1e-6 * r.launchMassKg);
    }

    void timelineIsDeterministic()
    {
        const auto world = crtest::makeWorld();
        const auto r = cr::analyse(world, cr::Scenario());
        const double t = r.referenceEpochMs + r.zones[1].startS * 1000 + 1234567;
        const auto a = cr::stateAt(world, r, t);
        const auto b = cr::stateAt(world, cr::analyse(world, cr::Scenario()), t);
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

QTEST_GUILESS_MAIN(TestAnalysis)
#include "tst_analysis.moc"
