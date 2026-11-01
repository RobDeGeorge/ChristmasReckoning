// SPDX-License-Identifier: GPL-3.0-or-later
#include "astro.h"
#include "solar.h"

#include <cmath>

Astro::Astro(QObject *parent)
    : QObject(parent)
{
}

QVector3D Astro::sunDirection(double epochMs) const
{
    const cr::solar::Vec3 v = cr::solar::sunDirectionEcef(std::int64_t(std::floor(epochMs)));
    return QVector3D(float(v.x), float(v.y), float(v.z));
}

QPointF Astro::subsolarPoint(double epochMs) const
{
    const cr::solar::SubsolarPoint p = cr::solar::subsolarPoint(std::int64_t(std::floor(epochMs)));
    return QPointF(p.longitudeDeg, p.latitudeDeg);
}
