// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform.h"

#include <QDirIterator>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

namespace {

void loadFonts()
{
    QDirIterator it(QStringLiteral(":/fonts"), {QStringLiteral("*.ttf")}, QDir::Files);
    while (it.hasNext()) {
        const QString path = it.next();
        if (QFontDatabase::addApplicationFont(path) < 0)
            qWarning("Could not load font %s", qPrintable(path));
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName(QStringLiteral("Christmas Reckoning"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("christmasreckoning.com"));
    QGuiApplication app(argc, argv);

    loadFonts();
    QFont font(QStringLiteral("DM Sans"));
    font.setPixelSize(16);
    QGuiApplication::setFont(font);
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Make sure the platform bridge (routing, clock) exists before QML starts asking for it.
    Platform::instance();

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("ChristmasReckoning", "Main");

    return app.exec();
}
