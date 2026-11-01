// SPDX-License-Identifier: GPL-3.0-or-later
#include "format.h"

#include <cmath>
#include <cstdio>
#include <string_view>

namespace cr::format {
namespace {

constexpr std::string_view kSuperDigits[] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};

bool badNumber(double v, std::string *out)
{
    if (std::isnan(v)) {
        *out = "—";
        return true;
    }
    if (std::isinf(v)) {
        *out = v > 0 ? "∞" : "−∞";
        return true;
    }
    return false;
}

// Rounds to `sig` significant figures and returns (mantissa digits as text, decimal exponent).
double roundSig(double v, int sig)
{
    if (v == 0)
        return 0;
    const double magnitude = std::floor(std::log10(std::abs(v)));
    const double scale = std::pow(10.0, sig - 1 - magnitude);
    return std::round(v * scale) / scale;
}

std::string withGrouping(const std::string &digits)
{
    // digits: optional '-', integer part, optional fraction
    std::string sign;
    std::string body = digits;
    if (!body.empty() && body[0] == '-') {
        sign = "−";
        body.erase(0, 1);
    }
    const auto dot = body.find('.');
    std::string integer = body.substr(0, dot);
    const std::string fraction = dot == std::string::npos ? "" : body.substr(dot);
    if (integer.size() > 4) { // 9,800 but 1500 reads oddly ungrouped; group from 5 digits... and 4
        for (int i = int(integer.size()) - 3; i > 0; i -= 3)
            integer.insert(std::size_t(i), ",");
    } else if (integer.size() == 4) {
        integer.insert(1, ",");
    }
    return sign + integer + fraction;
}

std::string fixed(double v, int decimals)
{
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%.*f", decimals < 0 ? 0 : decimals, v);
    return buffer;
}

} // namespace

std::string superscript(int exponent)
{
    std::string out;
    if (exponent < 0) {
        out = "⁻";
        exponent = -exponent;
    }
    const std::string digits = std::to_string(exponent);
    for (char c : digits)
        out += kSuperDigits[c - '0'];
    return out;
}

std::string scientific(double value, int sig)
{
    std::string out;
    if (badNumber(value, &out))
        return out;
    if (value == 0)
        return "0";
    double rounded = roundSig(value, sig);
    int exponent = int(std::floor(std::log10(std::abs(rounded))));
    double mantissa = rounded / std::pow(10.0, exponent);
    std::string text = fixed(mantissa, sig - 1);
    if (text[0] == '-')
        text = "−" + text.substr(1);
    if (exponent == 0)
        return text;
    return text + "×10" + superscript(exponent);
}

std::string number(double value, int sig)
{
    std::string out;
    if (badNumber(value, &out))
        return out;
    const double a = std::abs(value);
    if (a != 0 && (a >= 1e6 || a < 1e-3))
        return scientific(value, sig);
    const double rounded = roundSig(value, sig);
    if (rounded == 0)
        return "0";
    const int magnitude = int(std::floor(std::log10(std::abs(rounded))));
    const int decimals = std::max(0, sig - 1 - magnitude);
    return withGrouping(fixed(rounded, decimals));
}

std::string grouped(double value)
{
    std::string out;
    if (badNumber(value, &out))
        return out;
    return withGrouping(fixed(std::round(value), 0));
}

std::string count(double value, int sig)
{
    std::string out;
    if (badNumber(value, &out))
        return out;
    const double a = std::abs(value);
    struct Scale { double size; const char *name; };
    static const Scale scales[] = {{1e18, "quintillion"}, {1e15, "quadrillion"}, {1e12, "trillion"},
                                   {1e9, "billion"}, {1e6, "million"}};
    if (a >= 1e21)
        return scientific(value, sig);
    for (const Scale &s : scales) {
        if (a >= s.size)
            return number(value / s.size, sig) + " " + s.name;
    }
    return number(value, a >= 1000 ? std::max(sig, int(std::floor(std::log10(a))) + 1) : sig);
}

std::string duration(double seconds, int sig)
{
    std::string out;
    if (badNumber(seconds, &out))
        return out;
    const double a = std::abs(seconds);
    constexpr double minute = 60, hour = 3600, day = 86400, year = 365.25 * 86400;
    if (a == 0)
        return "0 s";
    if (a < 1e-9)
        return scientific(seconds, sig) + " s";
    if (a < 1e-6)
        return number(seconds * 1e9, sig) + " ns";
    if (a < 1e-3)
        return number(seconds * 1e6, sig) + " µs";
    if (a < 1)
        return number(seconds * 1e3, sig) + " ms";
    if (a < 2 * minute)
        return number(seconds, sig) + " s";
    if (a < 2 * hour)
        return number(seconds / minute, sig) + " min";
    if (a < 2 * day)
        return number(seconds / hour, sig) + " h";
    if (a < year)
        return number(seconds / day, sig) + " days";
    const double years = seconds / year;
    if (std::abs(years) >= 1e6)
        return count(years, sig) + " years";
    return number(years, sig) + (std::abs(years) < 1.5 ? " year" : " years");
}

std::string length(double metres, int sig)
{
    std::string out;
    if (badNumber(metres, &out))
        return out;
    const double a = std::abs(metres);
    if (a == 0)
        return "0 m";
    if (a < 1e-6)
        return scientific(metres, sig) + " m";
    if (a < 1e-3)
        return number(metres * 1e6, sig) + " µm";
    if (a < 1)
        return number(metres * 1e3, sig) + " mm";
    if (a < 1e4)
        return number(metres, sig) + " m";
    return number(metres / 1e3, sig) + " km";
}

std::string speed(double mps, int sig)
{
    std::string out;
    if (badNumber(mps, &out))
        return out;
    if (std::abs(mps) < 1e4)
        return number(mps, sig) + " m/s";
    return number(mps / 1e3, sig) + " km/s";
}

std::string mass(double kg, int sig)
{
    std::string out;
    if (badNumber(kg, &out))
        return out;
    const double a = std::abs(kg);
    if (a < 1)
        return number(kg * 1e3, sig) + " g";
    if (a < 1e4)
        return number(kg, sig) + " kg";
    return count(kg / 1e3, sig) + " t";
}

std::string energy(double joules, int sig)
{
    std::string out;
    if (badNumber(joules, &out))
        return out;
    return number(joules, sig) + " J";
}

std::string power(double watts, int sig)
{
    std::string out;
    if (badNumber(watts, &out))
        return out;
    return number(watts, sig) + " W";
}

std::string acceleration(double gs, int sig)
{
    std::string out;
    if (badNumber(gs, &out))
        return out;
    return number(gs, sig) + " g";
}

std::string temperature(double kelvin, int sig)
{
    std::string out;
    if (badNumber(kelvin, &out))
        return out;
    return number(kelvin, sig) + " K";
}

std::string percent(double fraction, int sig)
{
    std::string out;
    if (badNumber(fraction, &out))
        return out;
    const double p = fraction * 100;
    if (std::abs(p) >= 99.5 && std::abs(p) < 100)
        return number(p, 3) + "%";
    return number(p, sig) + "%";
}

std::string times(double ratio, int sig)
{
    std::string out;
    if (badNumber(ratio, &out))
        return out;
    return number(ratio, sig) + "×";
}

std::string clock(double secondsOfDay)
{
    double s = std::fmod(secondsOfDay, 86400.0);
    if (s < 0)
        s += 86400.0;
    const int minutes = int(std::lround(s / 60.0)) % (24 * 60);
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%02d:%02d", minutes / 60, minutes % 60);
    return buffer;
}

} // namespace cr::format
