// SPDX-License-Identifier: GPL-3.0-or-later
// Known-answer tests for the sun position used to draw the day/night terminator.
#include "solar.h"

#include <QDateTime>
#include <QTest>

#include <cmath>

using namespace cr::solar;

namespace {
std::int64_t utcMs(const char *iso)
{
    return QDateTime::fromString(QString::fromLatin1(iso), Qt::ISODate).toMSecsSinceEpoch();
}
} // namespace

class TestSolar : public QObject
{
    Q_OBJECT

private slots:
    void decemberSolsticeDeclinationEqualsMinusObliquity()
    {
        // 2026 December solstice: 21 Dec 2026, 20:50 UTC (USNO). Obliquity of date ≈ 23.436°.
        const auto point = subsolarPoint(utcMs("2026-12-21T20:50:00Z"));
        QVERIFY2(std::abs(point.latitudeDeg + 23.436) < 0.01, qPrintable(QString::number(point.latitudeDeg)));
    }

    void juneSolsticeDeclinationEqualsObliquity()
    {
        // 2026 June solstice: 21 Jun 2026, 08:24 UTC (USNO).
        const auto point = subsolarPoint(utcMs("2026-06-21T08:24:00Z"));
        QVERIFY2(std::abs(point.latitudeDeg - 23.436) < 0.01, qPrintable(QString::number(point.latitudeDeg)));
    }

    void marchEquinoxDeclinationIsZero()
    {
        // 2026 March equinox: 20 Mar 2026, 14:46 UTC (USNO).
        const auto point = subsolarPoint(utcMs("2026-03-20T14:46:00Z"));
        QVERIFY2(std::abs(point.latitudeDeg) < 0.01, qPrintable(QString::number(point.latitudeDeg)));
    }

    void equationOfTimeExtremes()
    {
        // The equation of time peaks near +16.4 min in early November and dips near -14.2 min in
        // mid-February.
        const double november = subsolarPoint(utcMs("2026-11-03T12:00:00Z")).equationOfTimeMinutes;
        QVERIFY2(november > 16.2 && november < 16.6, qPrintable(QString::number(november)));
        const double february = subsolarPoint(utcMs("2026-02-11T12:00:00Z")).equationOfTimeMinutes;
        QVERIFY2(february > -14.4 && february < -14.0, qPrintable(QString::number(february)));
    }

    void christmasNoonUtcPutsTheSunOverGreenwich()
    {
        // The equation of time crosses zero around 25 December, so at 12:00 UTC the subsolar
        // longitude is within a fraction of a degree of the prime meridian.
        const auto point = subsolarPoint(utcMs("2026-12-25T12:00:00Z"));
        QVERIFY2(std::abs(point.longitudeDeg) < 0.3, qPrintable(QString::number(point.longitudeDeg)));
        QVERIFY(std::abs(point.equationOfTimeMinutes) < 1.0);
    }

    void subsolarLongitudeMovesWestFifteenDegreesPerHour()
    {
        const auto a = subsolarPoint(utcMs("2026-12-24T12:00:00Z"));
        const auto b = subsolarPoint(utcMs("2026-12-24T13:00:00Z"));
        QVERIFY(std::abs((a.longitudeDeg - b.longitudeDeg) - 15.0) < 0.01);
    }

    void directionIsAUnitVector()
    {
        const Vec3 v = sunDirectionEcef(utcMs("2026-12-24T20:00:00Z"));
        QVERIFY(std::abs(std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) - 1.0) < 1e-12);
    }
};

QTEST_GUILESS_MAIN(TestSolar)
#include "tst_solar.moc"
