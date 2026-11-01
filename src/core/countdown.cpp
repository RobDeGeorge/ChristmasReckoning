// SPDX-License-Identifier: GPL-3.0-or-later
#include "countdown.h"

#include <algorithm>

namespace cr {

CountdownState christmasCountdown(std::int64_t nowMs, const LocalCalendar &calendar)
{
    const CivilDate today = calendar.dateAt(nowMs);

    CountdownState state;
    state.isChristmas = today.month == 12 && today.day == 25;
    state.year = today.year + ((today.month == 12 && today.day > 25) ? 1 : 0);
    state.targetMs = calendar.localMidnightMs({state.year, 12, 25});
    state.remainingMs = state.isChristmas ? 0 : std::max<std::int64_t>(0, state.targetMs - nowMs);
    // Round up so the countdown doesn't announce Christmas before midnight.
    state.totalSeconds = (state.remainingMs + 999) / 1000;
    state.days = int(state.totalSeconds / 86400);
    state.hours = int((state.totalSeconds / 3600) % 24);
    state.minutes = int((state.totalSeconds / 60) % 60);
    state.seconds = int(state.totalSeconds % 60);
    return state;
}

} // namespace cr
