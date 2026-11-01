// SPDX-License-Identifier: GPL-3.0-or-later
#include "qtcalendar.h"

#include <QDateTime>

#include <utility>

namespace cr {

QtZoneCalendar::QtZoneCalendar(QTimeZone zone)
    : m_zone(std::move(zone))
{
}

CivilDate QtZoneCalendar::dateAt(std::int64_t epochMs) const
{
    const QDate date = QDateTime::fromMSecsSinceEpoch(epochMs, m_zone).date();
    return {date.year(), date.month(), date.day()};
}

std::int64_t QtZoneCalendar::localMidnightMs(const CivilDate &date) const
{
    // Like JavaScript's Date, a midnight that falls in a DST gap resolves to the next valid time.
    const QDateTime midnight(QDate(date.year, date.month, date.day), QTime(0, 0), m_zone,
                             QDateTime::TransitionResolution::RelativeToBefore);
    return midnight.toMSecsSinceEpoch();
}

} // namespace cr
