// SPDX-License-Identifier: GPL-3.0-or-later
#include "mission.h"

#include "format.h"

#include <QQmlEngine>

#include <cmath>

namespace F = cr::format;

namespace {

Mission *g_mission = nullptr;

QString qs(const std::string &s)
{
    return QString::fromStdString(s);
}

QStringList qsl(const std::vector<std::string> &list)
{
    QStringList out;
    out.reserve(qsizetype(list.size()));
    for (const std::string &s : list)
        out.append(qs(s));
    return out;
}

} // namespace

Mission::Mission(QObject *parent)
    : QObject(parent)
{
    QString error;
    m_world = cr::WorldData::fromFile(QStringLiteral(":/data/world.bin"), &error);
    if (!m_world) {
        m_loadError = error;
        qWarning("Mission data unavailable: %s", qPrintable(error));
    }
    recompute();
}

Mission *Mission::instance()
{
    if (!g_mission)
        g_mission = new Mission;
    return g_mission;
}

Mission *Mission::create(QQmlEngine *, QJSEngine *)
{
    Mission *mission = instance();
    QJSEngine::setObjectOwnership(mission, QJSEngine::CppOwnership);
    return mission;
}

void Mission::setYear(int year)
{
    if (year == m_scenario.year || year < 2000 || year > 2200)
        return;
    m_scenario.year = year;
    recompute();
}

void Mission::recompute()
{
    if (!m_world) {
        m_results = cr::Results();
        m_results.referenceEpochMs = cr::christmasUtcEpochMs(m_scenario.year);
        emit resultsChanged();
        return;
    }
    m_results = cr::analyse(*m_world, m_scenario);

    if (m_timeMs == 0)
        m_timeMs = launchMs();
    emit resultsChanged();
    updateState();
}

void Mission::setTimeMs(double ms)
{
    if (!std::isfinite(ms) || ms == m_timeMs)
        return;
    m_timeMs = ms;
    updateState();
}

void Mission::updateState()
{
    if (!m_world)
        return;
    m_state = cr::stateAt(*m_world, m_results, m_timeMs);
    m_stateMap = toMap(m_state);
    emit stateChanged();
}

QVariantMap Mission::stateAt(double epochMs) const
{
    if (!m_world)
        return {};
    return toMap(cr::stateAt(*m_world, m_results, epochMs));
}

QVariantMap Mission::toMap(const cr::SantaState &s) const
{
    QStringList countries;
    if (s.zone >= 0 && s.zone < int(m_results.zones.size()))
        countries = qsl(m_results.zones[std::size_t(s.zone)].countries);
    QString country;
    if (m_world && s.countryIndex >= 0 && s.countryIndex < int(m_world->countries.size()))
        country = m_world->countries[std::size_t(s.countryIndex)].name;
    return QVariantMap{
        {QStringLiteral("phase"), QString::fromLatin1(cr::phaseName(s.phase))},
        {QStringLiteral("country"), country},
        {QStringLiteral("zoneCountries"), countries},
        {QStringLiteral("latitude"), s.latitudeDeg},
        {QStringLiteral("longitude"), s.longitudeDeg},
        {QStringLiteral("frontierZone"), s.frontierZone},
        {QStringLiteral("frontierRank"), s.frontierRank},
        {QStringLiteral("presents"), qs(F::grouped(s.presentsDelivered))},
    };
}

QVariantList Mission::flightPath(int samples) const
{
    QVariantList points;
    if (!m_world || samples < 2 || landingMs() <= launchMs())
        return points;
    const double start = launchMs(), span = landingMs() - start;
    double lastLat = 1e9, lastLon = 1e9;
    for (int i = 0; i < samples; ++i) {
        const double t = start + span * i / (samples - 1);
        const cr::SantaState s = cr::stateAt(*m_world, m_results, t);
        // Skip the repeats while he holds over one spot, waiting for the next time zone to fall asleep.
        if (s.latitudeDeg == lastLat && s.longitudeDeg == lastLon)
            continue;
        lastLat = s.latitudeDeg;
        lastLon = s.longitudeDeg;
        points.append(QVariantMap{{QStringLiteral("lat"), s.latitudeDeg},
                                  {QStringLiteral("lon"), s.longitudeDeg},
                                  {QStringLiteral("t"), t}});
    }
    return points;
}

QVariantMap Mission::zoneForOffset(int offsetMinutes) const
{
    int best = -1;
    int bestDiff = 0;
    for (int i = 0; i < int(m_results.zones.size()); ++i) {
        const int diff = std::abs(m_results.zones[std::size_t(i)].offsetMinutes - offsetMinutes);
        if (best < 0 || diff < bestDiff) {
            best = i;
            bestDiff = diff;
        }
    }
    if (best < 0)
        return {};
    const cr::ZoneResult &z = m_results.zones[std::size_t(best)];
    return QVariantMap{{QStringLiteral("label"), qs(z.label)},
                       {QStringLiteral("startMs"), toMs(z.startS + z.transitionTimeS)},
                       {QStringLiteral("endMs"), toMs(z.endS)}};
}
