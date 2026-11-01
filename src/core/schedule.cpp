// SPDX-License-Identifier: GPL-3.0-or-later
#include "schedule.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace cr {
namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

// Density of a run: work / (available time − fixed time). +inf when the fixed time doesn't fit.
double runDensity(double work, double span, double fixed)
{
    const double available = span - fixed;
    if (available <= 0)
        return (work > 0 || fixed > span) ? kInf : 0.0;
    return work / available;
}

} // namespace

double minimumPeakRate(const std::vector<ZoneJob> &jobs, CriticalRun *binding)
{
    double best = 0;
    CriticalRun run;
    const int n = int(jobs.size());
    for (int i = 0; i < n; ++i) {
        double work = 0;
        double fixed = 0;
        for (int j = i; j < n; ++j) {
            work += jobs[j].work;
            fixed += jobs[j].fixedS;
            const double density = runDensity(work, jobs[j].deadlineS - jobs[i].releaseS, fixed);
            if (density > best || (run.first < 0 && density >= best)) {
                best = density;
                run = {i, j, density};
            }
        }
    }
    if (binding)
        *binding = run;
    return best;
}

double maximumFixedScale(const std::vector<ZoneJob> &jobs)
{
    double best = kInf;
    const int n = int(jobs.size());
    for (int i = 0; i < n; ++i) {
        double fixed = 0;
        for (int j = i; j < n; ++j) {
            fixed += jobs[j].fixedS;
            if (fixed > 0)
                best = std::min(best, (jobs[j].deadlineS - jobs[i].releaseS) / fixed);
        }
    }
    return best;
}

std::vector<double> energyOptimalRates(const std::vector<ZoneJob> &input)
{
    struct Job
    {
        int index;
        ZoneJob job;
    };
    std::vector<Job> remaining;
    remaining.reserve(input.size());
    for (int i = 0; i < int(input.size()); ++i)
        remaining.push_back({i, input[i]});

    std::vector<double> rates(input.size(), 0.0);
    while (!remaining.empty()) {
        const int n = int(remaining.size());
        double bestDensity = -1;
        int bestFirst = 0;
        int bestLast = 0;
        for (int i = 0; i < n; ++i) {
            double work = 0;
            double fixed = 0;
            for (int j = i; j < n; ++j) {
                work += remaining[j].job.work;
                fixed += remaining[j].job.fixedS;
                const double density =
                    runDensity(work, remaining[j].job.deadlineS - remaining[i].job.releaseS, fixed);
                if (density > bestDensity) {
                    bestDensity = density;
                    bestFirst = i;
                    bestLast = j;
                }
            }
        }

        const double t1 = remaining[bestFirst].job.releaseS;
        const double t2 = remaining[bestLast].job.deadlineS;
        for (int k = bestFirst; k <= bestLast; ++k)
            rates[remaining[k].index] = bestDensity;
        remaining.erase(remaining.begin() + bestFirst, remaining.begin() + bestLast + 1);

        // Collapse the critical interval out of the timeline for the jobs that are left.
        const double length = t2 - t1;
        auto compress = [&](double t) {
            if (t >= t2)
                return t - length;
            if (t > t1)
                return t1;
            return t;
        };
        for (Job &job : remaining) {
            job.job.releaseS = compress(job.job.releaseS);
            job.job.deadlineS = compress(job.job.deadlineS);
        }
    }
    return rates;
}

ScheduleResult solveSchedule(const std::vector<ZoneJob> &jobs)
{
    ScheduleResult result;
    result.peakRate = minimumPeakRate(jobs, &result.binding);
    result.feasible = std::isfinite(result.peakRate);
    const std::size_t n = jobs.size();
    result.rates.assign(n, 0.0);
    result.startS.assign(n, 0.0);
    result.endS.assign(n, 0.0);
    if (!result.feasible || n == 0)
        return result;

    result.rates = energyOptimalRates(jobs);
    double t = -kInf;
    for (std::size_t k = 0; k < n; ++k) {
        const double rate = result.rates[k];
        const double flying = (jobs[k].work > 0 && rate > 0) ? jobs[k].work / rate : 0.0;
        result.startS[k] = std::max(t, jobs[k].releaseS);
        result.endS[k] = result.startS[k] + flying + jobs[k].fixedS;
        t = result.endS[k];
    }
    return result;
}

} // namespace cr
