// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace cr {

struct CivilDate
{
    int year = 1970;
    int month = 1; // 1..12
    int day = 1;   // 1..31

    friend bool operator==(const CivilDate &, const CivilDate &) = default;
};

// Maps instants to the visitor's local civil calendar.
// The browser build answers with JavaScript's Date (so DST rules match the old site exactly);
// native builds and tests answer with QTimeZone.
class LocalCalendar
{
public:
    virtual ~LocalCalendar() = default;

    // Local calendar date at the given instant (milliseconds since the Unix epoch).
    virtual CivilDate dateAt(std::int64_t epochMs) const = 0;

    // Instant of local midnight at the start of the given date.
    virtual std::int64_t localMidnightMs(const CivilDate &date) const = 0;
};

} // namespace cr
