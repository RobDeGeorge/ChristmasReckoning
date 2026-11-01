// SPDX-License-Identifier: GPL-3.0-or-later
#include "countdownclock.h"
#include "platform.h"

#include <QVariantMap>

#include <cmath>

CountdownClock::CountdownClock(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &CountdownClock::refresh);
    refresh();
}

void CountdownClock::setRunning(bool running)
{
    if (m_running == running)
        return;
    m_running = running;
    emit runningChanged();
    if (m_running)
        refresh();
    else
        m_timer.stop();
}

void CountdownClock::refresh()
{
    Platform *platform = Platform::instance();
    m_nowMs = platform->nowMs();
    m_state = cr::christmasCountdown(std::int64_t(std::floor(m_nowMs)), platform->calendar());
    emit changed();

    platform->publishState(QStringLiteral("countdown"),
                           QVariantMap{{QStringLiteral("year"), m_state.year},
                                       {QStringLiteral("isChristmas"), m_state.isChristmas},
                                       {QStringLiteral("totalSeconds"), double(m_state.totalSeconds)},
                                       {QStringLiteral("nowMs"), m_nowMs}});
    scheduleNextTick();
}

void CountdownClock::scheduleNextTick()
{
    if (!m_running)
        return;
    // Align with the next wall-clock second (+10 ms of slack), recomputing from the clock each time.
    const int msIntoSecond = int(std::fmod(m_nowMs, 1000.0));
    m_timer.start(1000 - msIntoSecond + 10);
}

QString CountdownClock::accessibleText() const
{
    if (m_state.isChristmas)
        return tr("It is Christmas Day. Merry Christmas!");
    return tr("%1 days, %2 hours, %3 minutes, and %4 seconds until Christmas %5.")
        .arg(m_state.days)
        .arg(m_state.hours)
        .arg(m_state.minutes)
        .arg(m_state.seconds)
        .arg(m_state.year);
}
