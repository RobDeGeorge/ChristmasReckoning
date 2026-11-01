// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "countdown.h"

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

// Live Christmas countdown for QML. Ticks on each wall-clock second and always recomputes from the
// clock (never accumulates), exactly like the original site's countdown.js.
class CountdownClock : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int year READ year NOTIFY changed FINAL)
    Q_PROPERTY(bool isChristmas READ isChristmas NOTIFY changed FINAL)
    Q_PROPERTY(int days READ days NOTIFY changed FINAL)
    Q_PROPERTY(int hours READ hours NOTIFY changed FINAL)
    Q_PROPERTY(int minutes READ minutes NOTIFY changed FINAL)
    Q_PROPERTY(int seconds READ seconds NOTIFY changed FINAL)
    Q_PROPERTY(double totalSeconds READ totalSeconds NOTIFY changed FINAL)
    Q_PROPERTY(double nowMs READ nowMs NOTIFY changed FINAL)
    Q_PROPERTY(double targetMs READ targetMs NOTIFY changed FINAL)
    Q_PROPERTY(QString accessibleText READ accessibleText NOTIFY changed FINAL)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged FINAL)

public:
    explicit CountdownClock(QObject *parent = nullptr);

    int year() const { return m_state.year; }
    bool isChristmas() const { return m_state.isChristmas; }
    int days() const { return m_state.days; }
    int hours() const { return m_state.hours; }
    int minutes() const { return m_state.minutes; }
    int seconds() const { return m_state.seconds; }
    double totalSeconds() const { return double(m_state.totalSeconds); }
    double nowMs() const { return m_nowMs; }
    double targetMs() const { return double(m_state.targetMs); }
    QString accessibleText() const;

    bool running() const { return m_running; }
    void setRunning(bool running);

    Q_INVOKABLE void refresh();

signals:
    void changed();
    void runningChanged();

private:
    void scheduleNextTick();

    cr::CountdownState m_state;
    double m_nowMs = 0;
    bool m_running = true;
    QTimer m_timer;
};
