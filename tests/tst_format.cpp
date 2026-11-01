// SPDX-License-Identifier: GPL-3.0-or-later
#include "format.h"

#include <QTest>

namespace F = cr::format;

namespace {
QString q(const std::string &s)
{
    return QString::fromStdString(s);
}
constexpr double kYear = 365.25 * 86400;
} // namespace

class TestFormat : public QObject
{
    Q_OBJECT

private slots:
    void superscripts()
    {
        QCOMPARE(q(F::superscript(-12)), QStringLiteral("⁻¹²"));
        QCOMPARE(q(F::superscript(23)), QStringLiteral("²³"));
    }

    void scientific()
    {
        QCOMPARE(q(F::scientific(1.5e23)), QStringLiteral("1.5×10²³"));
        QCOMPARE(q(F::scientific(-2.4e-7)), QStringLiteral("−2.4×10⁻⁷"));
        QCOMPARE(q(F::scientific(9.96e5)), QStringLiteral("1.0×10⁶")); // rounding carries into the exponent
        QCOMPARE(q(F::scientific(3.0)), QStringLiteral("3.0"));
    }

    void numbers()
    {
        QCOMPARE(q(F::number(850)), QStringLiteral("850"));
        QCOMPARE(q(F::number(1500)), QStringLiteral("1,500"));
        QCOMPARE(q(F::number(12345)), QStringLiteral("12,000"));
        QCOMPARE(q(F::number(0.0123)), QStringLiteral("0.012"));
        QCOMPARE(q(F::number(2.4e9)), QStringLiteral("2.4×10⁹"));
        QCOMPARE(q(F::number(-3.25, 3)), QStringLiteral("−3.25"));
        QCOMPARE(q(F::grouped(299792458)), QStringLiteral("299,792,458"));
    }

    void counts()
    {
        QCOMPARE(q(F::count(2.4e9)), QStringLiteral("2.4 billion"));
        QCOMPARE(q(F::count(110e6)), QStringLiteral("110 million"));
        QCOMPARE(q(F::count(9800)), QStringLiteral("9,800"));
        QCOMPARE(q(F::count(1.2e21)), QStringLiteral("1.2×10²¹"));
    }

    void durations()
    {
        QCOMPARE(q(F::duration(1.1e-4)), QStringLiteral("110 µs"));
        QCOMPARE(q(F::duration(33 * 3600)), QStringLiteral("33 h"));
        QCOMPARE(q(F::duration(90)), QStringLiteral("90 s"));
        QCOMPARE(q(F::duration(19000 * kYear)), QStringLiteral("19,000 years"));
        QCOMPARE(q(F::duration(2.5e-10)), QStringLiteral("2.5×10⁻¹⁰ s"));
    }

    void units()
    {
        QCOMPARE(q(F::speed(1.5e6)), QStringLiteral("1,500 km/s"));
        QCOMPARE(q(F::speed(340)), QStringLiteral("340 m/s"));
        QCOMPARE(q(F::length(90)), QStringLiteral("90 m"));
        QCOMPARE(q(F::length(1e11)), QStringLiteral("100 million km"));
        QCOMPARE(q(F::length(6.6e10)), QStringLiteral("66 million km"));
        QCOMPARE(q(F::mass(16'462'500)), QStringLiteral("16,000 t"));
        QCOMPARE(q(F::mass(2.4e9)), QStringLiteral("2.4 million t"));
        QCOMPARE(q(F::percent(0.81)), QStringLiteral("81%"));
        QCOMPARE(q(F::times(1700)), QStringLiteral("1,700×"));
    }

    void clocks()
    {
        QCOMPARE(q(F::clock(22 * 3600)), QStringLiteral("22:00"));
        QCOMPARE(q(F::clock(-2 * 3600)), QStringLiteral("22:00"));
        QCOMPARE(q(F::clock(30 * 3600 + 30 * 60)), QStringLiteral("06:30"));
    }

    void badNumbers()
    {
        QCOMPARE(q(F::number(std::numeric_limits<double>::infinity())), QStringLiteral("∞"));
        QCOMPARE(q(F::number(std::numeric_limits<double>::quiet_NaN())), QStringLiteral("—"));
    }
};

QTEST_GUILESS_MAIN(TestFormat)
#include "tst_format.moc"
