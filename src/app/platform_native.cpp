// SPDX-License-Identifier: GPL-3.0-or-later
// Desktop host, used for development and native testing.
#include "platform.h"
#include "qtcalendar.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QStandardPaths>

#include <cmath>

namespace host {

std::unique_ptr<cr::LocalCalendar> makeLocalCalendar()
{
    return std::make_unique<cr::QtZoneCalendar>(QTimeZone::systemTimeZone());
}

double systemNowMs()
{
    return double(QDateTime::currentMSecsSinceEpoch());
}

double requestedNowMs()
{
    // CR_NOW=2026-12-24T12:00:00Z previews any moment.
    const QByteArray value = qgetenv("CR_NOW");
    if (value.isEmpty())
        return NAN;
    const QDateTime parsed = QDateTime::fromString(QString::fromUtf8(value), Qt::ISODateWithMs);
    return parsed.isValid() ? double(parsed.toMSecsSinceEpoch()) : NAN;
}

QString currentHash()
{
    // CR_ROUTE=mission-control opens a page directly.
    return QString::fromUtf8(qgetenv("CR_ROUTE"));
}

void setHash(const QString &) {}

QUrl assetBase()
{
    // Serve the web/ directory of the source tree, where the textures live.
    return QUrl::fromLocalFile(QStringLiteral(CR_SOURCE_DIR "/web/"));
}

QString timeZoneName()
{
    return QString::fromUtf8(QTimeZone::systemTimeZoneId());
}

bool prefersReducedMotion()
{
    return qEnvironmentVariableIntValue("CR_REDUCED_MOTION") != 0;
}

bool touchFirst()
{
    return false;
}

void markReady()
{
    qInfo("app ready");
}

void publishState(const QString &, const QVariant &) {}

void downloadText(const QString &fileName, const QString &text, const QString &)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QFile file(QDir(dir.isEmpty() ? QDir::homePath() : dir).filePath(fileName));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(text.toUtf8());
    qInfo("saved %s", qPrintable(file.fileName()));
}

void setDocumentTitle(const QString &) {}

int utcOffsetMinutes(double epochMs)
{
    return QDateTime::fromMSecsSinceEpoch(qint64(epochMs)).toLocalTime().offsetFromUtc() / 60;
}

QString localTime(double epochMs)
{
    return QLocale::system().toString(QDateTime::fromMSecsSinceEpoch(qint64(epochMs)).toLocalTime().time(),
                                      QLocale::ShortFormat);
}

QString localDateTime(double epochMs)
{
    return QLocale::system().toString(QDateTime::fromMSecsSinceEpoch(qint64(epochMs)).toLocalTime(),
                                      QStringLiteral("ddd d MMM, ") + QLocale::system().timeFormat(QLocale::ShortFormat));
}

void installHashListener(Platform *) {}

} // namespace host
