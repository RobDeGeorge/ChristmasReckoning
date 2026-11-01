// SPDX-License-Identifier: GPL-3.0-or-later
#include "worlddata.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

#include <cstring>

namespace cr {
namespace {

// Little-endian binary reader with bounds checking.
class Reader
{
public:
    explicit Reader(const QByteArray &bytes) : m_data(bytes) {}

    bool ok() const { return m_ok; }
    qsizetype position() const { return m_pos; }

    template <typename T>
    T read()
    {
        T value{};
        if (m_pos + qsizetype(sizeof(T)) > m_data.size()) {
            m_ok = false;
            return value;
        }
        std::memcpy(&value, m_data.constData() + m_pos, sizeof(T));
        m_pos += sizeof(T);
        return value; // the build targets (x86-64, wasm32) are little-endian like the file format
    }

    QByteArray readBytes(qsizetype count)
    {
        if (count < 0 || m_pos + count > m_data.size()) {
            m_ok = false;
            return {};
        }
        QByteArray out = m_data.mid(m_pos, count);
        m_pos += count;
        return out;
    }

    void align(int boundary)
    {
        while (m_pos % boundary)
            ++m_pos;
    }

private:
    const QByteArray &m_data;
    qsizetype m_pos = 0;
    bool m_ok = true;
};

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

} // namespace

QString Zone::label() const
{
    const int total = offsetMinutes < 0 ? -offsetMinutes : offsetMinutes;
    return QStringLiteral("UTC%1%2:%3")
        .arg(offsetMinutes < 0 ? QChar(0x2212) : QChar(u'+'))
        .arg(total / 60, 2, 10, QChar(u'0'))
        .arg(total % 60, 2, 10, QChar(u'0'));
}

std::optional<WorldData> WorldData::fromFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, QStringLiteral("cannot open %1").arg(path));
        return std::nullopt;
    }
    return fromBytes(file.readAll(), error);
}

std::optional<WorldData> WorldData::fromBytes(const QByteArray &bytes, QString *error)
{
    Reader in(bytes);
    if (in.readBytes(4) != QByteArrayLiteral("CRWD")) {
        setError(error, QStringLiteral("not a Christmas Reckoning world file"));
        return std::nullopt;
    }
    const auto version = in.read<std::uint32_t>();
    if (version != 1) {
        setError(error, QStringLiteral("unsupported world file version %1").arg(version));
        return std::nullopt;
    }
    const auto jsonBytes = in.read<std::uint32_t>();
    const QByteArray json = in.readBytes(jsonBytes);
    in.align(4);
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (!in.ok() || doc.isNull() || !doc.isObject()) {
        setError(error, QStringLiteral("bad metadata: %1").arg(parseError.errorString()));
        return std::nullopt;
    }

    WorldData world;
    const QJsonObject meta = doc.object();
    world.metadata = meta.value(u"provenance").toObject();
    world.dataYear = meta.value(u"dataYear").toInt(2026);
    world.tzVersion = meta.value(u"tzVersion").toString();
    world.landAreaM2 = meta.value(u"landAreaM2").toDouble();

    for (const QJsonValue &value : meta.value(u"countries").toArray()) {
        const QJsonObject o = value.toObject();
        Country c;
        c.iso3 = o.value(u"iso3").toString();
        c.name = o.value(u"name").toString();
        c.population = o.value(u"population").toDouble();
        c.children = o.value(u"children").toDouble();
        c.households = o.value(u"households").toDouble();
        c.avgHouseholdSize = o.value(u"avgHouseholdSize").toDouble();
        c.shareWithChildren = o.value(u"shareWithChildren").toDouble();
        c.surveyYear = o.value(u"surveyYear").toInt();
        c.imputed = o.value(u"imputed").toBool();
        world.countries.push_back(std::move(c));
    }
    for (const QJsonValue &value : meta.value(u"resolution").toArray()) {
        const QJsonObject o = value.toObject();
        world.resolution.push_back({o.value(u"label").toString(), o.value(u"cellArcSeconds").toDouble(),
                                    o.value(u"pathSumM").toDouble()});
    }
    const QJsonArray zoneMeta = meta.value(u"zones").toArray();

    const auto zoneCount = in.read<std::uint32_t>();
    if (!in.ok() || zoneCount > 200 || zoneCount != std::uint32_t(zoneMeta.size())) {
        setError(error, QStringLiteral("bad zone table"));
        return std::nullopt;
    }
    world.zones.resize(zoneCount);
    std::vector<std::uint32_t> checkpointCounts(zoneCount);
    for (std::uint32_t i = 0; i < zoneCount; ++i) {
        Zone &zone = world.zones[i];
        zone.offsetMinutes = in.read<std::int32_t>();
        zone.cellCount = in.read<std::uint32_t>();
        zone.population = in.read<double>();
        zone.children = in.read<double>();
        zone.households = in.read<double>();
        zone.pathSumM = in.read<double>();
        zone.stopGoSum = in.read<double>();
        zone.gapM = in.read<double>();
        zone.transitionM = in.read<double>();
        checkpointCounts[i] = in.read<std::uint32_t>();
        in.read<std::uint32_t>(); // reserved / padding
        for (const QJsonValue &c : zoneMeta[i].toObject().value(u"topCountries").toArray())
            zone.topCountries.push_back(std::uint16_t(c.toInt()));
    }
    for (std::uint32_t i = 0; i < zoneCount && in.ok(); ++i) {
        if (checkpointCounts[i] == 0 || checkpointCounts[i] > 1u << 16) {
            setError(error, QStringLiteral("bad checkpoint count in zone %1").arg(i));
            return std::nullopt;
        }
        auto &checkpoints = world.zones[i].checkpoints;
        checkpoints.resize(checkpointCounts[i]);
        for (Checkpoint &cp : checkpoints) {
            cp.childrenFrac = in.read<float>();
            cp.householdsFrac = in.read<float>();
            cp.pathFrac = in.read<float>();
            cp.stopGoFrac = in.read<float>();
            cp.latitudeDeg = in.read<float>();
            cp.longitudeDeg = in.read<float>();
            cp.rank = in.read<std::uint32_t>();
            cp.country = in.read<std::uint16_t>();
            in.read<std::uint16_t>();
        }
    }
    if (!in.ok() || in.position() != bytes.size()) {
        setError(error, QStringLiteral("world file is truncated or has trailing data"));
        return std::nullopt;
    }
    return world;
}

QByteArray WorldData::toBytes() const
{
    QJsonObject meta;
    meta.insert(u"dataYear", dataYear);
    meta.insert(u"tzVersion", tzVersion);
    meta.insert(u"landAreaM2", landAreaM2);
    meta.insert(u"provenance", metadata);
    QJsonArray countryArray;
    for (const Country &c : countries) {
        countryArray.append(QJsonObject{{QStringLiteral("iso3"), c.iso3},
                                        {QStringLiteral("name"), c.name},
                                        {QStringLiteral("population"), c.population},
                                        {QStringLiteral("children"), c.children},
                                        {QStringLiteral("households"), c.households},
                                        {QStringLiteral("avgHouseholdSize"), c.avgHouseholdSize},
                                        {QStringLiteral("shareWithChildren"), c.shareWithChildren},
                                        {QStringLiteral("surveyYear"), c.surveyYear},
                                        {QStringLiteral("imputed"), c.imputed}});
    }
    meta.insert(u"countries", countryArray);
    QJsonArray resolutionArray;
    for (const ResolutionSample &r : resolution) {
        resolutionArray.append(QJsonObject{
            {QStringLiteral("label"), r.label}, {QStringLiteral("cellArcSeconds"), r.cellArcSeconds}, {QStringLiteral("pathSumM"), r.pathSumM}});
    }
    meta.insert(u"resolution", resolutionArray);
    QJsonArray zoneArray;
    for (const Zone &z : zones) {
        QJsonArray top;
        for (std::uint16_t c : z.topCountries)
            top.append(int(c));
        zoneArray.append(QJsonObject{{QStringLiteral("label"), z.label()}, {QStringLiteral("topCountries"), top}});
    }
    meta.insert(u"zones", zoneArray);

    QByteArray out;
    auto put = [&out](const auto &value) {
        out.append(reinterpret_cast<const char *>(&value), sizeof(value));
    };
    out.append("CRWD", 4);
    put(std::uint32_t(1));
    const QByteArray json = QJsonDocument(meta).toJson(QJsonDocument::Compact);
    put(std::uint32_t(json.size()));
    out.append(json);
    while (out.size() % 4)
        out.append('\0');
    put(std::uint32_t(zones.size()));
    for (const Zone &z : zones) {
        put(std::int32_t(z.offsetMinutes));
        put(std::uint32_t(z.cellCount));
        put(z.population);
        put(z.children);
        put(z.households);
        put(z.pathSumM);
        put(z.stopGoSum);
        put(z.gapM);
        put(z.transitionM);
        put(std::uint32_t(z.checkpoints.size()));
        put(std::uint32_t(0));
    }
    for (const Zone &z : zones) {
        for (const Checkpoint &cp : z.checkpoints) {
            put(cp.childrenFrac);
            put(cp.householdsFrac);
            put(cp.pathFrac);
            put(cp.stopGoFrac);
            put(cp.latitudeDeg);
            put(cp.longitudeDeg);
            put(cp.rank);
            put(cp.country);
            put(std::uint16_t(0));
        }
    }
    return out;
}

double WorldData::totalPopulation() const
{
    double sum = 0;
    for (const Zone &z : zones)
        sum += z.population;
    return sum;
}

double WorldData::totalChildren() const
{
    double sum = 0;
    for (const Zone &z : zones)
        sum += z.children;
    return sum;
}

double WorldData::totalHouseholds() const
{
    double sum = 0;
    for (const Zone &z : zones)
        sum += z.households;
    return sum;
}

double WorldData::totalPathSumM() const
{
    double sum = 0;
    for (const Zone &z : zones)
        sum += z.pathSumM;
    return sum;
}

} // namespace cr
