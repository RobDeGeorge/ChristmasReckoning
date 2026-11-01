// SPDX-License-Identifier: GPL-3.0-or-later
// The precomputed world the simulation runs on (built offline by tools/data/build_world.py).
//
// The night is organised by UTC-offset "zones" (all places sharing a clock on Christmas Eve), in the
// order Santa sweeps them: UTC+14 first, westwards to UTC−11. Each zone carries totals that are
// independent of the user's slider settings, so every slider change is cheap:
//   households      households with at least one child under 18 (H)
//   pathSum S       Σ √(H_i·A_i) over the 30″ population grid, in metres. The Beardwood–Halton–Hammersley
//                   tour length through those households is β·S; scaling households by f scales it by √f.
//   stopGoSum Q     Σ H_i^¾·A_i^¼ (m^½), for the stop-at-every-house kinematics.
//   gap G           extra distance (m) to hop between disconnected clusters (islands, oases).
//   transition      distance (m) from the previous zone's last position (the North Pole for the first).
// Checkpoints sample the fixed visiting order inside the zone with cumulative fractions of those
// sums, which is all the timeline needs to place Santa and light up delivered cells.
#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cr {

struct Checkpoint
{
    float childrenFrac = 0;   // cumulative fraction of the zone total, at the end of this checkpoint
    float householdsFrac = 0;
    float pathFrac = 0;
    float stopGoFrac = 0;
    float latitudeDeg = 0;
    float longitudeDeg = 0;
    std::uint32_t rank = 0;   // visiting rank of the last 0.25° cell covered (matches the delivery texture)
    std::uint16_t country = 0;
};

struct Zone
{
    int offsetMinutes = 0;    // UTC offset on Christmas Eve of the data year
    double population = 0;
    double children = 0;
    double households = 0;
    double pathSumM = 0;
    double stopGoSum = 0;
    double gapM = 0;
    double transitionM = 0;
    std::uint32_t cellCount = 0;
    std::vector<Checkpoint> checkpoints;
    std::vector<std::uint16_t> topCountries; // indices into WorldData::countries, most households first

    QString label() const;    // "UTC+05:30"
};

struct Country
{
    QString iso3;
    QString name;
    double population = 0;
    double children = 0;
    double households = 0;    // households with at least one member under 18
    double childrenPerHousehold = 0; // κ: children per household with children (latest survey)
    double avgHouseholdSize = 0;
    double shareWithChildren = 0; // φ18: share of households with a member under 18
    int surveyYear = 0;
    bool imputed = false;
};

struct ResolutionSample
{
    QString label;            // e.g. "30″ (≈1 km)"
    double cellArcSeconds = 0;
    double pathSumM = 0;      // Σ √(H_i·A_i) when the grid is coarsened to this cell size
};

class WorldData
{
public:
    static std::optional<WorldData> fromBytes(const QByteArray &bytes, QString *error = nullptr);
    static std::optional<WorldData> fromFile(const QString &path, QString *error = nullptr);
    // Serialises in the same format (used by tests; the real file comes from build_world.py).
    QByteArray toBytes() const;

    int dataYear = 2026;
    QString tzVersion;
    QJsonObject metadata;     // provenance: datasets, generation date, notes
    std::vector<Zone> zones;  // in sweep order (largest UTC offset first)
    std::vector<Country> countries;
    std::vector<ResolutionSample> resolution;
    double landAreaM2 = 0;

    double totalPopulation() const;
    double totalChildren() const;
    double totalHouseholds() const;
    double totalPathSumM() const;
};

} // namespace cr
