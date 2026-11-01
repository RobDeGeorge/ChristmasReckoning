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

// The simulation, exposed to QML as the `Mission` singleton: the user's scenario, everything derived
// from it (consoles, zones, charts), and Santa's state at the simulated time.
class Mission : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool ready READ ready CONSTANT FINAL)
    Q_PROPERTY(QString loadError READ loadError CONSTANT FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY resultsChanged FINAL)
    Q_PROPERTY(QVariantList parameters READ parameters NOTIFY scenarioChanged FINAL)
    Q_PROPERTY(bool scenarioIsDefault READ scenarioIsDefault NOTIFY scenarioChanged FINAL)

    Q_PROPERTY(QVariantList consoles READ consoles NOTIFY resultsChanged FINAL)
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY resultsChanged FINAL)
    Q_PROPERTY(QVariantList zones READ zones NOTIFY resultsChanged FINAL)
    Q_PROPERTY(QVariantList massSeries READ massSeries NOTIFY resultsChanged FINAL)
    Q_PROPERTY(QVariantList resolution READ resolution NOTIFY resultsChanged FINAL)
    Q_PROPERTY(QVariantList sources READ sources CONSTANT FINAL)
    Q_PROPERTY(QVariantMap dataInfo READ dataInfo CONSTANT FINAL)

    Q_PROPERTY(double referenceMs READ referenceMs NOTIFY resultsChanged FINAL)
    Q_PROPERTY(double launchMs READ launchMs NOTIFY resultsChanged FINAL)
    Q_PROPERTY(double lastDeliveryMs READ lastDeliveryMs NOTIFY resultsChanged FINAL)
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

    QVariantList parameters() const;
    bool scenarioIsDefault() const;
    Q_INVOKABLE void setParameter(const QString &key, double value);
    Q_INVOKABLE void resetParameters();

    QVariantList consoles() const { return m_consoles; }
    QVariantMap summary() const { return m_summary; }
    QVariantList zones() const { return m_zones; }
    QVariantList massSeries() const { return m_massSeries; }
    QVariantList resolution() const;
    QVariantList sources() const;
    QVariantMap dataInfo() const;

    double referenceMs() const { return m_results.referenceEpochMs; }
    double launchMs() const { return toMs(m_results.launchS); }
    double lastDeliveryMs() const { return toMs(m_results.lastDeliveryS); }
    double landingMs() const { return toMs(m_results.landingS); }

    double timeMs() const { return m_timeMs; }
    void setTimeMs(double ms);
    QVariantMap state() const { return m_stateMap; }
    int frontierZone() const { return m_state.frontierZone; }
    double frontierRank() const { return m_state.frontierRank; }

    // Santa's state at any instant (for the countdown page's live numbers).
    Q_INVOKABLE QVariantMap stateAt(double epochMs) const;
    Q_INVOKABLE int consoleIndex(const QString &id) const;
    Q_INVOKABLE QString exportJson() const;

    const cr::WorldData *world() const { return m_world ? &*m_world : nullptr; }
    const cr::Results &results() const { return m_results; }

signals:
    void scenarioChanged();
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

    QVariantList m_consoles;
    QVariantMap m_summary;
    QVariantList m_zones;
    QVariantList m_massSeries;
    QVariantMap m_stateMap;
};
