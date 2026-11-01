// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "calendar.h"

#include <QTimeZone>

namespace cr {

// LocalCalendar backed by a QTimeZone. Used natively and by the tests, which pin explicit zones.
class QtZoneCalendar final : public LocalCalendar
{
public:
    explicit QtZoneCalendar(QTimeZone zone = QTimeZone::systemTimeZone());

    CivilDate dateAt(std::int64_t epochMs) const override;
    std::int64_t localMidnightMs(const CivilDate &date) const override;

    const QTimeZone &zone() const { return m_zone; }

private:
    QTimeZone m_zone;
};

} // namespace cr
