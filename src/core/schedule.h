// SPDX-License-Identifier: GPL-3.0-or-later
// Scheduling Santa's night across time zones.
//
// Each zone is a job that may only run inside its own window [release, deadline]. It needs `work`
// units of speed-dependent effort (metres of flight, at `rate` metres per second) plus `fixedS`
// seconds that no speed can shorten (time spent at stops).
//
// Every window has the same length and they open in sweep order, so release times and deadlines are
// "agreeable": doing the zones in order, back to back, is optimal (earliest-deadline-first). Then the
// in-order schedule finishes zone j at max over i ≤ j of (release_i + Σ_{k=i..j} time_k), which gives
// Horn's feasibility condition on every contiguous run of zones:
//
//     Σ_{k=i..j} (work_k / rate + fixed_k) ≤ deadline_j − release_i       for all i ≤ j
//
// so the lowest possible peak rate is   max_{i≤j} work_ij / (deadline_j − release_i − fixed_ij).
// For the energy-optimal speed profile we use the Yao–Demers–Shenker critical-interval algorithm,
// which with agreeable windows always picks a contiguous run of zones.
#pragma once

#include <limits>
#include <vector>

namespace cr {

struct ZoneJob
{
    double releaseS = 0;
    double deadlineS = 0;
    double work = 0;   // speed-dependent effort (e.g. metres)
    double fixedS = 0; // speed-independent seconds
};

struct CriticalRun
{
    int first = -1;
    int last = -1;
    double rate = 0;
};

struct ScheduleResult
{
    bool feasible = false;
    // Lowest possible peak rate, and the run of zones that forces it (Horn's bottleneck).
    double peakRate = std::numeric_limits<double>::infinity();
    CriticalRun binding;
    // Energy-optimal rate for each zone (YDS); every rate is ≤ peakRate and the largest equals it.
    std::vector<double> rates;
    // In-order execution with those rates.
    std::vector<double> startS;
    std::vector<double> endS;
};

// Lowest peak rate that makes the jobs feasible; +infinity when even infinite speed cannot fit the
// fixed times. `binding` (optional) receives the bottleneck run.
double minimumPeakRate(const std::vector<ZoneJob> &jobs, CriticalRun *binding = nullptr);

// Largest factor by which every job's fixed time could be multiplied and still fit at infinite rate.
// (With fixed times proportional to a per-stop time τ, this gives the largest workable τ.)
double maximumFixedScale(const std::vector<ZoneJob> &jobs);

// Yao–Demers–Shenker energy-optimal rates (convex power) for agreeable windows.
std::vector<double> energyOptimalRates(const std::vector<ZoneJob> &jobs);

ScheduleResult solveSchedule(const std::vector<ZoneJob> &jobs);

} // namespace cr
