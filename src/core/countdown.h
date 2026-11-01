// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "calendar.h"

#include <cstdint>

namespace cr {

struct CountdownState
{
    int year = 0;                    // the Christmas being counted down to
    std::int64_t targetMs = 0;       // local midnight starting December 25 of `year`
    bool isChristmas = false;        // the whole local December 25 is the celebration
    std::int64_t remainingMs = 0;
    std::int64_t totalSeconds = 0;   // rounded up, so Christmas is never announced early
    int days = 0;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
};

// Port of the original site's countdown.js: counts real elapsed time to midnight starting
// December 25 in the visitor's local time zone, treats all of local December 25 as zero,
// and rolls over to next year's Christmas on December 26.
CountdownState christmasCountdown(std::int64_t nowMs, const LocalCalendar &calendar);

} // namespace cr
