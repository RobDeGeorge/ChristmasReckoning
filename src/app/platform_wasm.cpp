// SPDX-License-Identifier: GPL-3.0-or-later
// Browser host: JavaScript Date for the local calendar, the URL hash for routing.
#include "platform.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QMetaObject>
#include <QPointer>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cmath>
#include <string>

using emscripten::val;

namespace {

QPointer<Platform> g_hashTarget;

class JsDateCalendar final : public cr::LocalCalendar
{
public:
    cr::CivilDate dateAt(std::int64_t epochMs) const override
    {
        const val date = val::global("Date").new_(double(epochMs));
        return {date.call<int>("getFullYear"), date.call<int>("getMonth") + 1,
                date.call<int>("getDate")};
    }

    std::int64_t localMidnightMs(const cr::CivilDate &date) const override
    {
        // new Date(y, m, d) is local midnight, exactly as the original countdown.js computed it.
        const val midnight = val::global("Date").new_(date.year, date.month - 1, date.day);
        return std::int64_t(midnight.call<double>("getTime"));
    }
};

void onHashChange(std::string hash)
{
    const QString value = QString::fromStdString(hash);
    if (g_hashTarget) {
        QMetaObject::invokeMethod(g_hashTarget.data(),
                                  [value] { if (g_hashTarget) g_hashTarget->applyHash(value); },
                                  Qt::QueuedConnection);
    }
}

val crState()
{
    val window = val::global("window");
    val state = window["crState"];
    if (state.isUndefined() || state.isNull()) {
        state = val::object();
        window.set("crState", state);
    }
    return state;
}

} // namespace

EMSCRIPTEN_BINDINGS(christmas_reckoning_platform)
{
    emscripten::function("crOnHashChange", &onHashChange);
}

namespace host {

std::unique_ptr<cr::LocalCalendar> makeLocalCalendar()
{
    return std::make_unique<JsDateCalendar>();
}

double systemNowMs()
{
    return val::global("Date").call<double>("now");
}

double requestedNowMs()
{
    // ?now=2026-12-24T12:00:00Z previews any moment (used for screenshots and smoke tests).
    const val params = val::global("URLSearchParams").new_(val::global("location")["search"]);
    const val now = params.call<val>("get", std::string("now"));
    if (now.isNull() || now.isUndefined())
        return NAN;
    const double parsed = val::global("Date").call<double>("parse", now);
    return std::isfinite(parsed) ? parsed : NAN;
}

QString currentHash()
{
    return QString::fromStdString(val::global("location")["hash"].as<std::string>());
}

void setHash(const QString &hash)
{
    if (currentHash() != hash)
        val::global("location").set("hash", hash.toStdString());
}

QUrl assetBase()
{
    // Resolve against the page itself so the site works from any path (e.g. a local server).
    const std::string base = val::global("document")["baseURI"].as<std::string>();
    QUrl url(QString::fromStdString(base));
    url.setFragment(QString());
    url.setQuery(QString());
    return url.resolved(QUrl(QStringLiteral("./")));
}

QString timeZoneName()
{
    const val options =
        val::global("Intl")["DateTimeFormat"]().call<val>("resolvedOptions");
    const val zone = options["timeZone"];
    return zone.isString() ? QString::fromStdString(zone.as<std::string>()) : QString();
}

bool prefersReducedMotion()
{
    const val query = val::global("window").call<val>(
        "matchMedia", std::string("(prefers-reduced-motion: reduce)"));
    return query["matches"].as<bool>();
}

bool touchFirst()
{
    const val query = val::global("window").call<val>("matchMedia", std::string("(pointer: coarse)"));
    return query["matches"].as<bool>();
}

void markReady()
{
    val::global("window").set("crAppReady", true);
    const val callback = val::global("window")["crOnAppReady"];
    if (callback.typeOf().as<std::string>() == "function")
        callback();
}

void publishState(const QString &key, const QVariant &value)
{
    const QByteArray json = QJsonDocument::fromVariant(QVariantList{value}).toJson(QJsonDocument::Compact);
    const val parsed = val::global("JSON").call<val>("parse", json.toStdString());
    crState().set(key.toStdString(), parsed[0]);
}

void downloadText(const QString &fileName, const QString &text, const QString &mimeType)
{
    val parts = val::array();
    parts.call<void>("push", text.toStdString());
    val options = val::object();
    options.set("type", mimeType.toStdString());
    val blob = val::global("Blob").new_(parts, options);
    val url = val::global("URL").call<val>("createObjectURL", blob);
    val link = val::global("document").call<val>("createElement", std::string("a"));
    link.set("href", url);
    link.set("download", fileName.toStdString());
    val::global("document")["body"].call<void>("appendChild", link);
    link.call<void>("click");
    link.call<void>("remove");
    val::global("URL").call<void>("revokeObjectURL", url);
}

void setDocumentTitle(const QString &title)
{
    const std::string value = title.toStdString();
    val document = val::global("document");
    if (document["title"].as<std::string>() != value)
        document.set("title", value);
}

int utcOffsetMinutes(double epochMs)
{
    return -val::global("Date").new_(epochMs).call<int>("getTimezoneOffset");
}

QString localTime(double epochMs)
{
    val options = val::object();
    options.set("hour", std::string("numeric"));
    options.set("minute", std::string("2-digit"));
    const val date = val::global("Date").new_(epochMs);
    return QString::fromStdString(date.call<std::string>("toLocaleTimeString", val::undefined(), options));
}

QString localDateTime(double epochMs)
{
    val options = val::object();
    options.set("weekday", std::string("short"));
    options.set("day", std::string("numeric"));
    options.set("month", std::string("short"));
    options.set("hour", std::string("numeric"));
    options.set("minute", std::string("2-digit"));
    const val date = val::global("Date").new_(epochMs);
    return QString::fromStdString(date.call<std::string>("toLocaleString", val::undefined(), options));
}

void installHashListener(Platform *platform)
{
    g_hashTarget = platform;
}

} // namespace host
