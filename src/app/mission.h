// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "analysis.h"
#include "timeline.h"
#include "worlddata.h"

#include <QObject>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <optional>

class QQmlEngine;
class QJSEngine;

// The flight, exposed to QML as the `Mission` singleton: when Santa launches and lands, the route he
// flies, and where he is at any instant.
class Mission : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool ready READ ready CONSTANT FINAL)
    Q_PROPERTY(QString loadError READ loadError CONSTANT FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY resultsChanged FINAL)

    Q_PROPERTY(double referenceMs READ referenceMs NOTIFY resultsChanged FINAL)
    Q_PROPERTY(double launchMs READ launchMs NOTIFY resultsChanged FINAL)
    Q_PROPERTY(double landingMs READ landingMs NOTIFY resultsChanged FINAL)

    Q_PROPERTY(double timeMs READ timeMs WRITE setTimeMs NOTIFY stateChanged FINAL)
    Q_PROPERTY(QVariantMap state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(int frontierZone READ frontierZone NOTIFY stateChanged FINAL)
    Q_PROPERTY(double frontierRank READ frontierRank NOTIFY stateChanged FINAL)

public:
    static Mission *instance();
    static Mission *create(QQmlEngine *, QJSEngine *);

    bool ready() const { return m_world.has_value(); }
    QString loadError() const { return m_loadError; }
    int year() const { return m_scenario.year; }
    void setYear(int year);

    double referenceMs() const { return m_results.referenceEpochMs; }
    double launchMs() const { return toMs(m_results.launchS); }
    double landingMs() const { return toMs(m_results.landingS); }

    double timeMs() const { return m_timeMs; }
    void setTimeMs(double ms);
    QVariantMap state() const { return m_stateMap; }
    int frontierZone() const { return m_state.frontierZone; }
    double frontierRank() const { return m_state.frontierRank; }

    // Santa's state at any instant, whatever the tracker is currently showing.
    Q_INVOKABLE QVariantMap stateAt(double epochMs) const;
    // The whole night's route as [{lat, lon, t}], launch to landing, sampled evenly in time
    // (`t` is ms since the epoch).
    Q_INVOKABLE QVariantList flightPath(int samples) const;
    // When Santa works the time zone with this UTC offset (the visitor's), or the nearest one:
    // {label, startMs, endMs}.
    Q_INVOKABLE QVariantMap zoneForOffset(int offsetMinutes) const;

signals:
    void resultsChanged();
    void stateChanged();

private:
    explicit Mission(QObject *parent = nullptr);
    void recompute();
    void updateState();
    QVariantMap toMap(const cr::SantaState &state) const;
    double toMs(double secondsFromReference) const { return m_results.referenceEpochMs + secondsFromReference * 1000.0; }

    std::optional<cr::WorldData> m_world;
    QString m_loadError;
    cr::Scenario m_scenario;
    cr::Results m_results;
    cr::SantaState m_state;
    double m_timeMs = 0;
    QVariantMap m_stateMap;
};
