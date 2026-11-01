// SPDX-License-Identifier: GPL-3.0-or-later
// Sanity checks on the real data/world.bin produced by tools/data/build_world.py, so a broken or
// implausible rebuild of the data can't slip into the site unnoticed.
#include "analysis.h"
#include "timeline.h"
#include "worlddata.h"

#include <QTest>

#include <cmath>

class TestWorldData : public QObject
{
    Q_OBJECT

    std::optional<cr::WorldData> world;

private slots:
    void initTestCase()
    {
        QString error;
        world = cr::WorldData::fromFile(QStringLiteral(CR_SOURCE_DIR "/data/world.bin"), &error);
        QVERIFY2(world, qPrintable(error));
    }

    void totalsArePlausible()
    {
        // UN WPP 2024: ~8.3 bn people and ~2.4 bn under 18 in 2026. Households with a child: ~1.1 bn.
        QVERIFY(world->totalPopulation() > 7.8e9 && world->totalPopulation() < 8.6e9);
        QVERIFY(world->totalChildren() > 2.2e9 && world->totalChildren() < 2.6e9);
        QVERIFY(world->totalHouseholds() > 0.8e9 && world->totalHouseholds() < 1.4e9);
        QVERIFY(world->totalChildren() / world->totalHouseholds() > 1.5);
        QVERIFY(world->countries.size() > 200);
        for (const cr::Country &c : world->countries) {
            QVERIFY2(c.childrenPerHousehold >= 1.0 && c.childrenPerHousehold < 8, qPrintable(c.name));
            QVERIFY2(c.households <= c.children + 1, qPrintable(c.name));
        }
    }

    void zonesAreOrderedAndComplete()
    {
        QVERIFY(world->zones.size() >= 30 && world->zones.size() <= 45);
        QCOMPARE(world->zones.front().offsetMinutes, 14 * 60); // Kiribati (Line Islands)
        for (std::size_t i = 1; i < world->zones.size(); ++i)
            QVERIFY(world->zones[i].offsetMinutes < world->zones[i - 1].offsetMinutes);
        for (const cr::Zone &z : world->zones) {
            QVERIFY(z.households > 0 && z.pathSumM > 0 && z.stopGoSum > 0);
            QVERIFY(!z.checkpoints.empty());
            float lastH = 0, lastS = 0;
            std::uint32_t lastRank = 0;
            for (const cr::Checkpoint &cp : z.checkpoints) {
                QVERIFY(cp.householdsFrac >= lastH && cp.pathFrac >= lastS && cp.rank > lastRank);
                QVERIFY(std::abs(cp.latitudeDeg) <= 90 && std::abs(cp.longitudeDeg) <= 180.5);
                QVERIFY(cp.country < world->countries.size());
                lastH = cp.householdsFrac;
                lastS = cp.pathFrac;
                lastRank = cp.rank;
            }
            QCOMPARE(z.checkpoints.back().householdsFrac, 1.0f);
            QCOMPARE(z.checkpoints.back().rank, z.cellCount);
        }
    }

    void clusteringShortensTheRoute()
    {
        // Finer grids can only shorten the BHH estimate (Cauchy–Schwarz); uniform-over-land is longest.
        QVERIFY(world->resolution.size() >= 3);
        for (std::size_t i = 1; i < world->resolution.size(); ++i)
            QVERIFY(world->resolution[i].pathSumM >= world->resolution[i - 1].pathSumM);
    }

    void defaultScenarioHeadlinesAreInRange()
    {
        const cr::Results r = cr::analyse(*world, cr::Scenario());
        QVERIFY(r.feasible);
        // Order-of-magnitude targets from the independent model review.
        QVERIFY2(r.pathM > 3e10 && r.pathM < 3e11, qPrintable(QString::number(r.pathM)));
        QVERIFY2(r.peakSpeedMps > 3e5 && r.peakSpeedMps < 5e6, qPrintable(QString::number(r.peakSpeedMps)));
        QVERIFY(r.averageSpeedMps < r.peakSpeedMps);
        QVERIFY(r.timePerStopS > 1e-5 && r.timePerStopS < 1e-3);
        QVERIFY(r.nightSpanS > 30 * 3600 && r.nightSpanS < 36 * 3600);
        QVERIFY(r.clockLagS > 0.01 && r.clockLagS < 10);
        QVERIFY(r.bindingHouseholdShare > 0.3);

        // The whole night, end to end.
        const auto landed = cr::stateAt(*world, r, r.referenceEpochMs + (r.landingS + 60) * 1000);
        QCOMPARE(landed.phase, cr::Phase::Landed);
        QVERIFY(std::abs(landed.householdsDelivered - r.households) < 1e-6 * r.households);
    }
};

QTEST_GUILESS_MAIN(TestWorldData)
#include "tst_worlddata.moc"
