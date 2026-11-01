// SPDX-License-Identifier: GPL-3.0-or-later
// Build-time report from the real simulation (never shipped to the browser).
//
//   cr-report [world.bin]              JSON summary for the static HTML fallback (headline numbers)
//   cr-report --formulas [world.bin]   every console formula as TeX, for tools/formulas/render.cjs
//   cr-report --text [world.bin]       a human-readable dump of every console
#include "analysis.h"
#include "format.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace F = cr::format;

namespace {

QString qs(const std::string &s)
{
    return QString::fromStdString(s);
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);
    const bool formulas = args.removeAll(QStringLiteral("--formulas")) > 0;
    const bool text = args.removeAll(QStringLiteral("--text")) > 0;
    const QString path = args.value(0, QStringLiteral(CR_SOURCE_DIR "/data/world.bin"));

    QString error;
    const auto world = cr::WorldData::fromFile(path, &error);
    if (!world) {
        QTextStream(stderr) << "cr-report: " << error << '\n';
        return 1;
    }
    const cr::Results r = cr::analyse(*world, cr::Scenario());
    QTextStream out(stdout);

    if (formulas) {
        QJsonObject all;
        for (const cr::Console &c : r.consoles) {
            for (const cr::Step &s : c.steps)
                all.insert(qs(c.id + "-" + s.id), qs(s.tex));
        }
        out << QJsonDocument(all).toJson(QJsonDocument::Indented);
        return 0;
    }

    if (text) {
        for (const cr::Console &c : r.consoles) {
            out << "\n== " << qs(c.callsign) << " · " << qs(c.title) << " — " << qs(c.question) << '\n';
            out << "   " << qs(c.status) << '\n';
            for (const cr::Readout &readout : c.readouts)
                out << "   " << qs(readout.label) << ": " << qs(readout.value) << '\n';
            for (const cr::Step &s : c.steps) {
                out << "   - " << qs(s.label) << ": " << qs(s.plain) << '\n'
                    << "       " << qs(s.substitution) << "  →  " << qs(s.result) << '\n';
                if (!s.validity.empty())
                    out << "       ! " << qs(s.validity) << '\n';
            }
            for (const std::string &comparison : c.comparisons)
                out << "   * " << qs(comparison) << '\n';
        }
        return 0;
    }

    auto row = [](const QString &label, const std::string &value) {
        return QJsonObject{{QStringLiteral("label"), label}, {QStringLiteral("value"), qs(value)}};
    };
    const QJsonArray headline{
        row(QStringLiteral("Children under 18"), F::count(r.children)),
        row(QStringLiteral("Households to visit"), F::count(r.households)),
        row(QStringLiteral("Length of the night (all time zones)"), F::duration(r.nightSpanS)),
        row(QStringLiteral("Shortest route through every household"), F::length(r.pathM + r.gapM)),
        row(QStringLiteral("Lowest possible top speed"), F::speed(r.peakSpeedMps)),
        row(QStringLiteral("Time per household"), F::duration(r.timePerStopS)),
        row(QStringLiteral("Mass of the presents"), F::mass(r.payloadKg)),
        row(QStringLiteral("g-force just to follow the ground"), F::acceleration(r.curvatureGs)),
        row(QStringLiteral("Reindeer needed"), F::count(r.reindeerNeeded)),
    };
    const QJsonObject report{
        {QStringLiteral("intro"),
         QStringLiteral("Every child under 18 on Earth, one present each, delivered between 22:00 and 06:00 local "
                        "time on Christmas Eve %1, using UN population and household data.")
             .arg(world->dataYear)},
        {QStringLiteral("headline"), headline},
    };
    out << QJsonDocument(report).toJson(QJsonDocument::Indented);
    return 0;
}
