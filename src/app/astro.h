// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QPointF>
#include <QVector3D>
#include <QtQml/qqmlregistration.h>

// Small astronomy/geometry helpers for QML: sun position and globe projection.
class Astro : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Astro(QObject *parent = nullptr);

    // Unit vector to the Sun in Earth-fixed coordinates (x: 0°E, y: 90°E, z: North Pole).
    Q_INVOKABLE QVector3D sunDirection(double epochMs) const;
    // Subsolar latitude/longitude in degrees as (x = longitude, y = latitude).
    Q_INVOKABLE QPointF subsolarPoint(double epochMs) const;
};
