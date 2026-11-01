// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform.h"

#include <QQmlEngine>
#include <QRegularExpression>
#include <QStringList>

#include <cmath>

namespace {
Platform *g_platform = nullptr;

const QStringList kPages = {QString(), QStringLiteral("mission-control")};
} // namespace

Platform::Platform(QObject *parent)
    : QObject(parent)
    , m_calendar(host::makeLocalCalendar())
{
    const double requested = host::requestedNowMs();
    if (std::isfinite(requested))
        m_clockOffsetMs = requested - host::systemNowMs();
    m_route = routeFromHash(host::currentHash());
}

Platform *Platform::instance()
{
    if (!g_platform) {
        g_platform = new Platform;
        host::installHashListener(g_platform);
    }
    return g_platform;
}

Platform *Platform::create(QQmlEngine *, QJSEngine *)
{
    Platform *platform = instance();
    QJSEngine::setObjectOwnership(platform, QJSEngine::CppOwnership);
    return platform;
}

QString Platform::routeFromHash(const QString &hash)
{
    QString path = hash;
    while (path.startsWith(u'#') || path.startsWith(u'/'))
        path.remove(0, 1);
    while (path.endsWith(u'/'))
        path.chop(1);
    path = path.toLower();

    QString first = path.section(u'/', 0, 0);
    if (first == u"live")
        return QStringLiteral("mission-control/live");
    if (!kPages.contains(first))
        return QString();
    // Keep only simple, safe sub-route segments (console ids like "fido").
    const QString second = path.section(u'/', 1, 1);
    static const QRegularExpression safe(QStringLiteral("^[a-z0-9-]{1,32}$"));
    if (!second.isEmpty() && safe.match(second).hasMatch())
        return first + u'/' + second;
    return first;
}

QString Platform::page() const
{
    return m_route.section(u'/', 0, 0);
}

QString Platform::subRoute() const
{
    return m_route.section(u'/', 1, 1);
}

void Platform::setRoute(const QString &route)
{
    if (route == m_route)
        return;
    m_route = route;
    emit routeChanged();
}

void Platform::navigate(const QString &route)
{
    const QString normalized = routeFromHash(route);
    host::setHash(QStringLiteral("#/") + normalized);
    setRoute(normalized);
}

void Platform::applyHash(const QString &hash)
{
    setRoute(routeFromHash(hash));
}

QUrl Platform::assetBase() const { return host::assetBase(); }
bool Platform::isWeb() const
{
#ifdef Q_OS_WASM
    return true;
#else
    return false;
#endif
}
QString Platform::timeZoneName() const { return host::timeZoneName(); }
bool Platform::reducedMotion() const { return host::prefersReducedMotion(); }
bool Platform::touchFirst() const { return host::touchFirst(); }
QString Platform::sourceUrl() const
{
    return QStringLiteral("https://github.com/RobDeGeorge/ChristmasReckoning");
}

double Platform::nowMs() const
{
    return host::systemNowMs() + m_clockOffsetMs;
}

void Platform::notifyReady() { host::markReady(); }

void Platform::publishState(const QString &key, const QVariant &value)
{
    host::publishState(key, value);
}

void Platform::downloadText(const QString &fileName, const QString &text, const QString &mimeType)
{
    host::downloadText(fileName, text, mimeType);
}

void Platform::setDocumentTitle(const QString &title)
{
    host::setDocumentTitle(title);
}

int Platform::utcOffsetMinutes(double epochMs) const
{
    return host::utcOffsetMinutes(epochMs);
}

QString Platform::localTime(double epochMs) const
{
    return host::localTime(epochMs);
}

QString Platform::localDateTime(double epochMs) const
{
    return host::localDateTime(epochMs);
}
