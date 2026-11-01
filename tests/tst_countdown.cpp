// SPDX-License-Identifier: GPL-3.0-or-later
// Port of the original site's tests/countdown.test.cjs. The JavaScript tests pinned time zones with
// the TZ environment variable; here each case pins an explicit QTimeZone instead.
#include "countdown.h"
#include "qtcalendar.h"

#include <QDateTime>
#include <QTest>

using cr::christmasCountdown;
using cr::QtZoneCalendar;

namespace {

std::int64_t localMs(const QTimeZone &zone, int year, int month, int day, int hour = 0, int minute = 0,
                     int second = 0, int ms = 0)
{
    return QDateTime(QDate(year, month, day), QTime(hour, minute, second, ms), zone).toMSecsSinceEpoch();
}

std::int64_t utcMs(const char *iso)
{
    const QDateTime parsed = QDateTime::fromString(QString::fromLatin1(iso), Qt::ISODateWithMs);
    Q_ASSERT(parsed.isValid());
    return parsed.toMSecsSinceEpoch();
}

const QTimeZone berlin("Europe/Berlin");
const QTimeZone phoenix("America/Phoenix");
const QTimeZone auckland("Pacific/Auckland");
const QTimeZone newYork("America/New_York");

} // namespace

class TestCountdown : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        for (const QTimeZone *zone : {&berlin, &phoenix, &auckland, &newYork})
            QVERIFY2(zone->isValid(), zone->id().constData());
    }

    void christmasEveCountsDownToMidnightOfTheCurrentYear()
    {
        const QtZoneCalendar calendar(berlin);
        const auto state = christmasCountdown(localMs(berlin, 2026, 12, 24), calendar);
        QCOMPARE(state.year, 2026);
        QCOMPARE(state.days, 1);
        QCOMPARE(state.hours, 0);
        QCOMPARE(state.totalSeconds, 86400);
        QCOMPARE(state.isChristmas, false);
    }

    void aFractionOfASecondBeforeChristmasStillHasOneSecondRemaining()
    {
        const QtZoneCalendar calendar(berlin);
        const auto state = christmasCountdown(localMs(berlin, 2026, 12, 24, 23, 59, 59, 999), calendar);
        QCOMPARE(state.totalSeconds, 1);
        QCOMPARE(state.seconds, 1);
        QCOMPARE(state.isChristmas, false);
    }

    void theEntireLocalChristmasDayIsAZeroCountdownCelebration()
    {
        const QtZoneCalendar calendar(berlin);
        for (int hour : {0, 12, 23}) {
            const auto state = christmasCountdown(localMs(berlin, 2026, 12, 25, hour, 59, 59, 999), calendar);
            QVERIFY(state.isChristmas);
            QCOMPARE(state.year, 2026);
            QCOMPARE(state.totalSeconds, 0);
            QCOMPARE(state.days, 0);
            QCOMPARE(state.hours, 0);
            QCOMPARE(state.minutes, 0);
            QCOMPARE(state.seconds, 0);
        }
    }

    void december26RollsForwardToTheNextChristmas()
    {
        const QtZoneCalendar calendar(berlin);
        const auto state = christmasCountdown(localMs(berlin, 2026, 12, 26), calendar);
        QCOMPARE(state.year, 2027);
        QCOMPARE(QDateTime::fromMSecsSinceEpoch(state.targetMs, berlin).date(), QDate(2027, 12, 25));
        QCOMPARE(state.isChristmas, false);
        QCOMPARE(state.days, 364);
    }

    void leapYearsIncludeFebruary29WhenItIsAhead()
    {
        const QtZoneCalendar calendar(berlin);
        QCOMPARE(christmasCountdown(localMs(berlin, 2023, 12, 26), calendar).days, 365);
        QCOMPARE(christmasCountdown(localMs(berlin, 2024, 12, 26), calendar).days, 364);
    }

    void usesTheLocalChristmasDateEvenWhenUtcIsOnADifferentDay()
    {
        const QtZoneCalendar arizona(phoenix);
        const auto eve = christmasCountdown(utcMs("2026-12-25T06:59:59Z"), arizona);
        QCOMPARE(eve.isChristmas, false);
        QCOMPARE(eve.totalSeconds, 1);
        QVERIFY(christmasCountdown(utcMs("2026-12-26T06:59:59Z"), arizona).isChristmas);
        QCOMPARE(christmasCountdown(utcMs("2026-12-26T07:00:00Z"), arizona).year, 2027);

        const QtZoneCalendar newZealand(auckland);
        QVERIFY(christmasCountdown(utcMs("2026-12-24T11:00:00Z"), newZealand).isChristmas);
    }

    void accountsForTheExtraElapsedHourWhenDaylightSavingTimeEnds()
    {
        const QtZoneCalendar eastern(newYork);
        const auto state = christmasCountdown(localMs(newYork, 2026, 10, 31), eastern);
        QCOMPARE(state.days, 55);
        QCOMPARE(state.hours, 1);
        QCOMPARE(state.totalSeconds, 55 * 86400 + 3600);

        const QtZoneCalendar arizona(phoenix);
        const auto noDst = christmasCountdown(localMs(phoenix, 2026, 10, 31), arizona);
        QCOMPARE(noDst.days, 55);
        QCOMPARE(noDst.hours, 0);
    }

    // The JavaScript version rejected invalid Date objects. The C++ API takes integer milliseconds,
    // so the equivalent risk is overflow at extreme instants; make sure those stay well-defined.
    void extremeInstantsStayWellDefined()
    {
        const QtZoneCalendar calendar(berlin);
        const auto farFuture = christmasCountdown(localMs(berlin, 9999, 1, 1), calendar);
        QCOMPARE(farFuture.year, 9999);
        QVERIFY(farFuture.days >= 0 && farFuture.days <= 366);
        const auto past = christmasCountdown(localMs(berlin, 1970, 12, 25, 12), calendar);
        QVERIFY(past.isChristmas);
    }
};

QTEST_GUILESS_MAIN(TestCountdown)
#include "tst_countdown.moc"
