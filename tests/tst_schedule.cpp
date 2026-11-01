// SPDX-License-Identifier: GPL-3.0-or-later
// The scheduler is checked three ways: hand-worked cases, a brute-force bisection that simulates the
// in-order schedule directly, and the YDS execution actually meeting every deadline.
#include "schedule.h"

#include <QRandomGenerator>
#include <QTest>

#include <algorithm>
#include <cmath>

using cr::ZoneJob;

namespace {

// Does the in-order (EDF) schedule meet every deadline at this constant rate?
bool feasibleAt(const std::vector<ZoneJob> &jobs, double rate)
{
    double t = -1e300;
    for (const ZoneJob &job : jobs) {
        const double start = std::max(t, job.releaseS);
        t = start + job.work / rate + job.fixedS;
        if (t > job.deadlineS * (1 + 1e-12) + 1e-9)
            return false;
    }
    return true;
}

double bisectRate(const std::vector<ZoneJob> &jobs)
{
    double lo = 0, hi = 1;
    while (!feasibleAt(jobs, hi)) {
        hi *= 2;
        if (hi > 1e30)
            return INFINITY;
    }
    for (int i = 0; i < 200; ++i) {
        const double mid = 0.5 * (lo + hi);
        (feasibleAt(jobs, mid) ? hi : lo) = mid;
    }
    return hi;
}

// Agreeable windows like the real ones: equal length, opening in order.
std::vector<ZoneJob> randomJobs(QRandomGenerator &rng, int n, bool withFixed)
{
    std::vector<ZoneJob> jobs;
    const double window = 8 * 3600;
    double release = 0;
    for (int i = 0; i < n; ++i) {
        release += rng.bounded(4) * 1800.0; // gaps of 0–1.5 h between zones
        ZoneJob job;
        job.releaseS = release;
        job.deadlineS = release + window;
        job.work = std::pow(10.0, 5 + rng.generateDouble() * 6);
        job.fixedS = withFixed ? rng.generateDouble() * 600 : 0;
        jobs.push_back(job);
    }
    return jobs;
}

} // namespace

class TestSchedule : public QObject
{
    Q_OBJECT

private slots:
    void singleZone()
    {
        const std::vector<ZoneJob> jobs{{0, 100, 1000, 0}};
        const auto result = cr::solveSchedule(jobs);
        QVERIFY(result.feasible);
        QCOMPARE(result.peakRate, 10.0);
        QCOMPARE(result.rates[0], 10.0);
        QCOMPARE(result.startS[0], 0.0);
        QCOMPARE(result.endS[0], 100.0);
    }

    void separateWindowsGetTheirOwnSpeeds()
    {
        const std::vector<ZoneJob> jobs{{0, 100, 1000, 0}, {200, 300, 500, 0}};
        const auto result = cr::solveSchedule(jobs);
        QCOMPARE(result.peakRate, 10.0);
        QCOMPARE(result.binding.first, 0);
        QCOMPARE(result.binding.last, 0);
        QCOMPARE(result.rates[0], 10.0);
        QCOMPARE(result.rates[1], 5.0);
        QCOMPARE(result.startS[1], 200.0); // waits for the second window to open
    }

    void overlappingWindowsShareTheLoad()
    {
        // Windows [0,8], [1,9], [2,10] with 8 units each: the run of all three needs 24/10.
        const std::vector<ZoneJob> jobs{{0, 8, 8, 0}, {1, 9, 8, 0}, {2, 10, 8, 0}};
        const auto result = cr::solveSchedule(jobs);
        QCOMPARE(result.peakRate, 2.4);
        QCOMPARE(result.binding.first, 0);
        QCOMPARE(result.binding.last, 2);
        for (double rate : result.rates)
            QCOMPARE(rate, 2.4);
        QVERIFY(std::abs(result.endS[2] - 10.0) < 1e-12);
    }

    void fixedTimeThatCannotFitIsInfeasible()
    {
        const std::vector<ZoneJob> jobs{{0, 10, 100, 11}};
        QVERIFY(std::isinf(cr::minimumPeakRate(jobs)));
        QVERIFY(!cr::solveSchedule(jobs).feasible);
        QCOMPARE(cr::maximumFixedScale(std::vector<ZoneJob>{{0, 10, 0, 5}}), 2.0);
    }

    void hornMatchesBruteForceBisection()
    {
        QRandomGenerator rng(20261224);
        for (int trial = 0; trial < 300; ++trial) {
            const auto jobs = randomJobs(rng, 1 + int(rng.bounded(40)), trial % 2 == 1);
            const double horn = cr::minimumPeakRate(jobs);
            const double brute = bisectRate(jobs);
            QVERIFY2(std::abs(horn - brute) <= 1e-9 * brute,
                     qPrintable(QStringLiteral("trial %1: %2 vs %3").arg(trial).arg(horn).arg(brute)));
        }
    }

    void energyOptimalScheduleMeetsEveryDeadline()
    {
        QRandomGenerator rng(1225);
        for (int trial = 0; trial < 300; ++trial) {
            const auto jobs = randomJobs(rng, 1 + int(rng.bounded(40)), trial % 3 == 0);
            const auto result = cr::solveSchedule(jobs);
            QVERIFY(result.feasible);
            double maxRate = 0;
            for (std::size_t k = 0; k < jobs.size(); ++k) {
                QVERIFY(result.startS[k] >= jobs[k].releaseS - 1e-6);
                QVERIFY2(result.endS[k] <= jobs[k].deadlineS + 1e-6 * jobs[k].deadlineS + 1e-6,
                         qPrintable(QStringLiteral("trial %1 zone %2 overruns by %3 s").arg(trial).arg(k)
                                        .arg(result.endS[k] - jobs[k].deadlineS)));
                QVERIFY(result.rates[k] <= result.peakRate * (1 + 1e-9));
                maxRate = std::max(maxRate, result.rates[k]);
            }
            QVERIFY(std::abs(maxRate - result.peakRate) <= 1e-9 * result.peakRate);
        }
    }

    void moreWorkNeverLowersThePeak()
    {
        QRandomGenerator rng(42);
        for (int trial = 0; trial < 100; ++trial) {
            auto jobs = randomJobs(rng, 2 + int(rng.bounded(30)), false);
            const double before = cr::minimumPeakRate(jobs);
            jobs[rng.bounded(int(jobs.size()))].work *= 1.5;
            QVERIFY(cr::minimumPeakRate(jobs) >= before);
        }
    }
};

QTEST_GUILESS_MAIN(TestSchedule)
#include "tst_schedule.moc"
