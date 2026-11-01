// SPDX-License-Identifier: GPL-3.0-or-later
#include "mission.h"

#include "format.h"
#include "sources.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>

#include <cmath>

namespace F = cr::format;

namespace {

Mission *g_mission = nullptr;

QString qs(const std::string &s)
{
    return QString::fromStdString(s);
}

QString qs(std::string_view s)
{
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}

QStringList qsl(const std::vector<std::string> &list)
{
    QStringList out;
    out.reserve(qsizetype(list.size()));
    for (const std::string &s : list)
        out.append(qs(s));
    return out;
}

const char *phaseLabel(cr::Phase phase)
{
    switch (phase) {
    case cr::Phase::PreLaunch: return "Pre-launch";
    case cr::Phase::Transit: return "In transit";
    case cr::Phase::Delivering: return "Delivering";
    case cr::Phase::Holding: return "Holding";
    case cr::Phase::Returning: return "Returning home";
    case cr::Phase::Landed: return "Landed";
    }
    return "";
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

QVariantList Mission::parameters() const
{
    QVariantList list;
    for (const cr::ParamInfo &info : cr::parameterTable()) {
        list.append(QVariantMap{
            {QStringLiteral("key"), qs(info.key)},
            {QStringLiteral("label"), qs(info.label)},
            {QStringLiteral("unit"), qs(info.unit)},
            {QStringLiteral("min"), info.minimum},
            {QStringLiteral("max"), info.maximum},
            {QStringLiteral("step"), info.step},
            {QStringLiteral("logScale"), info.logScale},
            {QStringLiteral("defaultValue"), info.defaultValue},
            {QStringLiteral("value"), m_scenario[info.id]},
            {QStringLiteral("source"), qs(info.sourceKey)},
            {QStringLiteral("help"), qs(info.help)},
        });
    }
    return list;
}

bool Mission::scenarioIsDefault() const
{
    for (const cr::ParamInfo &info : cr::parameterTable()) {
        if (m_scenario[info.id] != info.defaultValue)
            return false;
    }
    return true;
}

void Mission::setParameter(const QString &key, double value)
{
    const cr::ParamInfo *info = cr::parameterInfo(key.toStdString());
    if (!info)
        return;
    const double before = m_scenario[info->id];
    m_scenario.set(info->id, value);
    if (m_scenario[info->id] == before)
        return;
    emit scenarioChanged();
    recompute();
}

void Mission::resetParameters()
{
    const int year = m_scenario.year;
    m_scenario = cr::Scenario();
    m_scenario.year = year;
    emit scenarioChanged();
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
    const cr::Results &r = m_results;

    m_consoles.clear();
    for (const cr::Console &c : r.consoles) {
        QVariantList readouts;
        for (const cr::Readout &readout : c.readouts)
            readouts.append(QVariantMap{{QStringLiteral("label"), qs(readout.label)},
                                        {QStringLiteral("value"), qs(readout.value)}});
        QVariantList steps;
        for (const cr::Step &step : c.steps) {
            steps.append(QVariantMap{{QStringLiteral("id"), qs(step.id)},
                                     {QStringLiteral("label"), qs(step.label)},
                                     {QStringLiteral("tex"), qs(step.tex)},
                                     {QStringLiteral("plain"), qs(step.plain)},
                                     {QStringLiteral("substitution"), qs(step.substitution)},
                                     {QStringLiteral("result"), qs(step.result)},
                                     {QStringLiteral("validity"), qs(step.validity)},
                                     {QStringLiteral("sources"), qsl(step.sources)}});
        }
        QVariantList bars;
        for (const cr::Bar &bar : c.bars) {
            const QString unit = qs(c.barUnit);
            QString text;
            if (unit == u"m/s")
                text = qs(F::speed(bar.value));
            else if (unit == u"W")
                text = qs(F::power(bar.value));
            else if (unit == u"J")
                text = qs(F::energy(bar.value));
            else if (unit == u"g")
                text = qs(F::acceleration(bar.value));
            else
                text = qs(F::number(bar.value)) + u' ' + unit;
            bars.append(QVariantMap{{QStringLiteral("label"), qs(bar.label)},
                                    {QStringLiteral("value"), bar.value},
                                    {QStringLiteral("text"), text},
                                    {QStringLiteral("highlight"), bar.highlight}});
        }
        m_consoles.append(QVariantMap{{QStringLiteral("id"), qs(c.id)},
                                      {QStringLiteral("callsign"), qs(c.callsign)},
                                      {QStringLiteral("title"), qs(c.title)},
                                      {QStringLiteral("question"), qs(c.question)},
                                      {QStringLiteral("go"), c.go},
                                      {QStringLiteral("status"), qs(c.status)},
                                      {QStringLiteral("readouts"), readouts},
                                      {QStringLiteral("steps"), steps},
                                      {QStringLiteral("comparisons"), qsl(c.comparisons)},
                                      {QStringLiteral("params"), qsl(c.params)},
                                      {QStringLiteral("caveat"), qs(c.caveat)},
                                      {QStringLiteral("barTitle"), qs(c.barTitle)},
                                      {QStringLiteral("barUnit"), qs(c.barUnit)},
                                      {QStringLiteral("bars"), bars}});
    }

    m_zones.clear();
    for (const cr::ZoneResult &z : r.zones) {
        QStringList countries = qsl(z.countries);
        m_zones.append(QVariantMap{{QStringLiteral("label"), qs(z.label)},
                                   {QStringLiteral("offsetMinutes"), z.offsetMinutes},
                                   {QStringLiteral("households"), z.households},
                                   {QStringLiteral("children"), z.children},
                                   {QStringLiteral("speedMps"), z.speedMps},
                                   {QStringLiteral("pathM"), z.pathM},
                                   {QStringLiteral("windowOpenMs"), toMs(z.windowOpenS)},
                                   {QStringLiteral("windowCloseMs"), toMs(z.windowCloseS)},
                                   {QStringLiteral("startMs"), toMs(z.startS)},
                                   {QStringLiteral("endMs"), toMs(z.endS)},
                                   {QStringLiteral("countries"), countries},
                                   {QStringLiteral("binding"), int(z.index) >= r.bindingFirst
                                                                 && int(z.index) <= r.bindingLast}});
    }

    m_massSeries.clear();
    if (r.landingS > r.launchS) {
        constexpr int samples = 160;
        for (int i = 0; i <= samples; ++i) {
            const double t = toMs(r.launchS + (r.landingS - r.launchS) * i / samples);
            const cr::SantaState s = cr::stateAt(*m_world, r, t);
            m_massSeries.append(QVariantMap{{QStringLiteral("t"), t},
                                            {QStringLiteral("massKg"), s.massKg},
                                            {QStringLiteral("households"), s.householdsDelivered},
                                            {QStringLiteral("speedMps"), s.speedMps}});
        }
    }

    const double windowStart = m_scenario[cr::Param::WindowStartHour];
    const double hopTime = r.stopGoAccelMps2 > 0 ? 2 * std::sqrt(r.averageHopM / r.stopGoAccelMps2) : 0.0;
    m_summary = QVariantMap{
        {QStringLiteral("feasible"), r.feasible},
        {QStringLiteral("feasibilityNote"), qs(r.feasibilityNote)},
        {QStringLiteral("children"), r.children},
        {QStringLiteral("households"), r.households},
        {QStringLiteral("presents"), r.presents},
        {QStringLiteral("stops"), r.stops},
        {QStringLiteral("distanceM"), r.distanceM},
        {QStringLiteral("peakSpeedMps"), r.peakSpeedMps},
        {QStringLiteral("averageSpeedMps"), r.averageSpeedMps},
        {QStringLiteral("mach"), r.mach},
        {QStringLiteral("launchMassKg"), r.launchMassKg},
        {QStringLiteral("timePerStopS"), r.timePerStopS},
        {QStringLiteral("nightSpanS"), r.nightSpanS},
        {QStringLiteral("curvatureGs"), r.curvatureGs},
        {QStringLiteral("dragPowerW"), r.dragPowerW},
        {QStringLiteral("kineticEnergyJ"), r.kineticEnergyJ},
        {QStringLiteral("magicShare"), r.magicShare},
        {QStringLiteral("text"), QVariantMap{
             {QStringLiteral("children"), qs(F::count(r.children))},
             {QStringLiteral("households"), qs(F::count(r.households))},
             {QStringLiteral("presents"), qs(F::count(r.presents))},
             {QStringLiteral("distance"), qs(F::length(r.distanceM))},
             {QStringLiteral("peakSpeed"), qs(F::speed(r.peakSpeedMps))},
             {QStringLiteral("averageSpeed"), qs(F::speed(r.averageSpeedMps))},
             {QStringLiteral("timePerStop"), qs(F::duration(r.timePerStopS))},
             {QStringLiteral("night"), qs(F::duration(r.nightSpanS))},
             {QStringLiteral("window"), qs(F::clock(windowStart * 3600) + "–"
                                           + F::clock((windowStart + m_scenario[cr::Param::WindowLengthHours]) * 3600))},
             {QStringLiteral("launchMass"), qs(F::mass(r.launchMassKg))},
             {QStringLiteral("gload"), qs(F::acceleration(r.curvatureGs))},
             {QStringLiteral("hop"), qs(F::length(r.averageHopM))},
             {QStringLiteral("hopTime"), qs(F::duration(hopTime))},
             {QStringLiteral("hopPeak"), qs(F::speed(hopTime > 0 ? 2 * r.averageHopM / hopTime : 0.0))},
             {QStringLiteral("stopGoG"), qs(F::acceleration(r.stopGoAccelMps2 / 9.80665))},
         }},
    };

    if (m_timeMs == 0)
        m_timeMs = launchMs();
    emit resultsChanged();
    updateState();
}

QVariantList Mission::resolution() const
{
    QVariantList list;
    if (!m_world)
        return list;
    const double factor = m_scenario[cr::Param::RouteQuality] * 0.7124
        * std::sqrt(m_scenario[cr::Param::Eligibility] / 100.0 / m_scenario[cr::Param::HouseholdsPerStop]);
    for (const cr::ResolutionSample &r : m_world->resolution) {
        list.append(QVariantMap{{QStringLiteral("label"), r.label},
                                {QStringLiteral("arcSeconds"), r.cellArcSeconds},
                                {QStringLiteral("tourM"), factor * r.pathSumM}});
    }
    return list;
}

QVariantList Mission::sources() const
{
    QVariantList list;
    for (const cr::Source &s : cr::sources()) {
        list.append(QVariantMap{{QStringLiteral("key"), qs(s.key)},
                                {QStringLiteral("title"), qs(s.title)},
                                {QStringLiteral("publisher"), qs(s.publisher)},
                                {QStringLiteral("url"), qs(s.url)},
                                {QStringLiteral("note"), qs(s.note)}});
    }
    return list;
}

QVariantMap Mission::dataInfo() const
{
    if (!m_world)
        return {};
    return QVariantMap{{QStringLiteral("dataYear"), m_world->dataYear},
                       {QStringLiteral("tzVersion"), m_world->tzVersion},
                       {QStringLiteral("zones"), int(m_world->zones.size())},
                       {QStringLiteral("countries"), int(m_world->countries.size())},
                       {QStringLiteral("generated"), m_world->metadata.value(u"generated").toString()}};
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
    const cr::Results &r = m_results;
    QString zoneLabel;
    QStringList countries;
    if (s.zone >= 0 && s.zone < int(r.zones.size())) {
        zoneLabel = qs(r.zones[std::size_t(s.zone)].label);
        countries = qsl(r.zones[std::size_t(s.zone)].countries);
    }
    QString country;
    if (m_world && s.countryIndex >= 0 && s.countryIndex < int(m_world->countries.size()))
        country = m_world->countries[std::size_t(s.countryIndex)].name;
    const double g = s.speedMps > 0
        ? (s.speedMps * s.speedMps / (6'371'008.8 + m_scenario[cr::Param::AltitudeM]) - 9.80665) / 9.80665
        : 0.0;
    return QVariantMap{
        {QStringLiteral("phase"), QString::fromLatin1(cr::phaseName(s.phase))},
        {QStringLiteral("phaseLabel"), QString::fromLatin1(phaseLabel(s.phase))},
        {QStringLiteral("zone"), s.zone},
        {QStringLiteral("zoneLabel"), zoneLabel},
        {QStringLiteral("zoneCountries"), countries},
        {QStringLiteral("country"), country},
        {QStringLiteral("latitude"), s.latitudeDeg},
        {QStringLiteral("longitude"), s.longitudeDeg},
        {QStringLiteral("heading"), s.headingDeg},
        {QStringLiteral("speedMps"), s.speedMps},
        {QStringLiteral("households"), s.householdsDelivered},
        {QStringLiteral("children"), s.childrenDelivered},
        {QStringLiteral("presents"), s.presentsDelivered},
        {QStringLiteral("stops"), s.stopsMade},
        {QStringLiteral("distanceM"), s.distanceM},
        {QStringLiteral("massKg"), s.massKg},
        {QStringLiteral("energyJ"), s.dragEnergyJ},
        {QStringLiteral("elapsedS"), s.missionElapsedS},
        {QStringLiteral("localSeconds"), s.localSecondsOfDay},
        {QStringLiteral("frontierZone"), s.frontierZone},
        {QStringLiteral("frontierRank"), s.frontierRank},
        {QStringLiteral("text"), QVariantMap{
             {QStringLiteral("households"), qs(F::grouped(s.householdsDelivered))},
             {QStringLiteral("presents"), qs(F::grouped(s.presentsDelivered))},
             {QStringLiteral("children"), qs(F::grouped(s.childrenDelivered))},
             {QStringLiteral("distance"), qs(F::length(s.distanceM, 3))},
             {QStringLiteral("speed"), qs(F::speed(s.speedMps, 3))},
             {QStringLiteral("mach"), qs(F::number(r.speedOfSoundMps > 0 ? s.speedMps / r.speedOfSoundMps : 0.0, 2))},
             {QStringLiteral("lightFraction"), qs(F::percent(s.speedMps / 299'792'458.0, 2))},
             {QStringLiteral("gload"), qs(F::acceleration(std::max(0.0, g)))},
             {QStringLiteral("mass"), qs(F::mass(s.massKg, 3))},
             {QStringLiteral("cookies"), qs(F::grouped(s.stopsMade * m_scenario[cr::Param::CookiesPerStop]))},
             {QStringLiteral("kcal"), qs(F::count(s.stopsMade * m_scenario[cr::Param::CookiesPerStop]
                                                  * m_scenario[cr::Param::CookieMassG] * 4.44) + " kcal")},
             {QStringLiteral("energy"), qs(F::energy(s.dragEnergyJ))},
             {QStringLiteral("localTime"), qs(F::clock(s.localSecondsOfDay))},
             {QStringLiteral("timePerStop"), qs(F::duration(r.timePerStopS))},
         }},
    };
}

QVariantList Mission::trail(double endMs, double spanMs, int samples) const
{
    QVariantList points;
    if (!m_world || samples < 2)
        return points;
    const double start = std::max(endMs - spanMs, launchMs());
    if (endMs <= start)
        return points;
    for (int i = 0; i < samples; ++i) {
        const double t = start + (endMs - start) * i / (samples - 1);
        const cr::SantaState s = cr::stateAt(*m_world, m_results, t);
        points.append(QVariantMap{{QStringLiteral("lat"), s.latitudeDeg}, {QStringLiteral("lon"), s.longitudeDeg}});
    }
    return points;
}

int Mission::consoleIndex(const QString &id) const
{
    for (int i = 0; i < int(m_results.consoles.size()); ++i) {
        if (qs(m_results.consoles[std::size_t(i)].id) == id)
            return i;
    }
    return -1;
}

QString Mission::exportJson() const
{
    QJsonObject scenario;
    for (const cr::ParamInfo &info : cr::parameterTable())
        scenario.insert(qs(info.key), m_scenario[info.id]);
    scenario.insert(QStringLiteral("year"), m_scenario.year);
    QJsonArray consoles;
    for (const QVariant &c : m_consoles)
        consoles.append(QJsonObject::fromVariantMap(c.toMap()));
    QJsonObject root{{QStringLiteral("scenario"), scenario},
                     {QStringLiteral("summary"), QJsonObject::fromVariantMap(m_summary)},
                     {QStringLiteral("consoles"), consoles},
                     {QStringLiteral("data"), QJsonObject::fromVariantMap(dataInfo())},
                     {QStringLiteral("generator"), QStringLiteral("https://www.christmasreckoning.com")}};
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}
