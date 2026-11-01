// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "calendar.h"

#include <QObject>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <memory>

class QQmlEngine;
class QJSEngine;

// Bridge to the host environment: the browser when built for WebAssembly, the desktop otherwise.
// Owns routing (kept in sync with the URL hash), the visitor's local calendar and clock, and the
// hooks the smoke tests use (window.crAppReady / window.crState).
class Platform : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString route READ route NOTIFY routeChanged FINAL)
    Q_PROPERTY(QString page READ page NOTIFY routeChanged FINAL)
    Q_PROPERTY(QString subRoute READ subRoute NOTIFY routeChanged FINAL)
    Q_PROPERTY(QUrl assetBase READ assetBase CONSTANT FINAL)
    Q_PROPERTY(bool web READ isWeb CONSTANT FINAL)
    Q_PROPERTY(QString timeZoneName READ timeZoneName CONSTANT FINAL)
    Q_PROPERTY(bool reducedMotion READ reducedMotion CONSTANT FINAL)
    Q_PROPERTY(bool touchFirst READ touchFirst CONSTANT FINAL)
    Q_PROPERTY(QString sourceUrl READ sourceUrl CONSTANT FINAL)

public:
    static Platform *instance();
    static Platform *create(QQmlEngine *, QJSEngine *);

    QString route() const { return m_route; }
    QString page() const;     // "" (countdown) or "mission-control"
    QString subRoute() const; // e.g. "fido" for "#/mission-control/fido"
    QUrl assetBase() const;
    bool isWeb() const;
    QString timeZoneName() const;
    bool reducedMotion() const;
    bool touchFirst() const;
    QString sourceUrl() const;

    const cr::LocalCalendar &calendar() const { return *m_calendar; }

    // Current time, in ms since the epoch. Honours a ?now=ISO-8601 override (web) or CR_NOW (native)
    // so the Christmas Eve and Christmas Day states can be previewed and tested.
    Q_INVOKABLE double nowMs() const;

    Q_INVOKABLE void navigate(const QString &route);
    Q_INVOKABLE void notifyReady();
    Q_INVOKABLE void publishState(const QString &key, const QVariant &value);
    // Saves text as a file: a browser download on the web, a file in Downloads on the desktop.
    Q_INVOKABLE void downloadText(const QString &fileName, const QString &text, const QString &mimeType);
    // Browser tab title (the countdown shows there too, like the original site).
    Q_INVOKABLE void setDocumentTitle(const QString &title);

    // Called when the browser reports a hash change (back/forward, edited URL, links).
    void applyHash(const QString &hash);

    static QString routeFromHash(const QString &hash);

signals:
    void routeChanged();

private:
    explicit Platform(QObject *parent = nullptr);
    void setRoute(const QString &route);

    QString m_route;
    std::unique_ptr<cr::LocalCalendar> m_calendar;
    double m_clockOffsetMs = 0;
};

// Implemented separately for the browser (platform_wasm.cpp) and the desktop (platform_native.cpp).
namespace host {
std::unique_ptr<cr::LocalCalendar> makeLocalCalendar();
double systemNowMs();
double requestedNowMs(); // NaN when no override was requested
QString currentHash();
void setHash(const QString &hash);
QUrl assetBase();
QString timeZoneName();
bool prefersReducedMotion();
bool touchFirst();
void markReady();
void publishState(const QString &key, const QVariant &value);
void downloadText(const QString &fileName, const QString &text, const QString &mimeType);
void setDocumentTitle(const QString &title);
void installHashListener(Platform *platform);
} // namespace host
