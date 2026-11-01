// SPDX-License-Identifier: GPL-3.0-or-later
// Human-readable numbers for the consoles. Results are shown to 2 significant figures by default:
// the inputs aren't known better than that, and false precision is one of the classic failures of
// "Santa physics" write-ups. All strings are UTF-8 and use Unicode superscripts (no markup).
#pragma once

#include <string>

namespace cr::format {

std::string superscript(int exponent);                  // −12 → "⁻¹²"
std::string scientific(double value, int sig = 2);      // 1.5×10²³
std::string number(double value, int sig = 2);          // 850 · 1,500 · 0.012 · 2.4×10⁹
std::string grouped(double value);                      // 2,400,000,000 (rounded to an integer)
std::string count(double value, int sig = 2);           // 2.4 billion · 110 million · 9,800
std::string duration(double seconds, int sig = 2);      // 110 µs · 33 h · 19,000 years
std::string length(double metres, int sig = 2);         // 90 m · 1.0×10⁸ km
std::string speed(double metresPerSecond, int sig = 2); // 1,500 km/s · 340 m/s
std::string mass(double kilograms, int sig = 2);        // 120 kg · 2.4 million t
std::string energy(double joules, int sig = 2);         // 2.7×10²¹ J
std::string power(double watts, int sig = 2);           // 1.5×10²³ W
std::string acceleration(double gs, int sig = 2);       // 36,000 g
std::string temperature(double kelvin, int sig = 2);    // 1.0×10⁹ K
std::string percent(double fraction, int sig = 2);      // 81%
std::string times(double ratio, int sig = 2);           // 1,700×
std::string clock(double secondsOfDay);                 // 22:00

} // namespace cr::format
