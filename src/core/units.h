// SPDX-License-Identifier: GPL-3.0-or-later
// Minimal compile-time dimensional analysis. Quantities carry exponents of mass (M), length (L),
// time (T) and temperature (K); mixing incompatible dimensions is a compile error, which is the point:
// the physics consoles do a lot of arithmetic, and unit slips are the classic way to get it wrong.
// Values are always stored in SI base units.
#pragma once

#include <cmath>
#include <compare>

namespace cr::units {

template <int M, int L, int T, int K>
struct Quantity
{
    double value = 0; // SI

    constexpr Quantity() = default;
    constexpr explicit Quantity(double si) : value(si) {}

    constexpr double si() const { return value; }

    constexpr Quantity operator-() const { return Quantity(-value); }
    constexpr Quantity &operator+=(Quantity other) { value += other.value; return *this; }
    constexpr Quantity &operator-=(Quantity other) { value -= other.value; return *this; }
    constexpr Quantity &operator*=(double k) { value *= k; return *this; }
    constexpr Quantity &operator/=(double k) { value /= k; return *this; }

    friend constexpr Quantity operator+(Quantity a, Quantity b) { return Quantity(a.value + b.value); }
    friend constexpr Quantity operator-(Quantity a, Quantity b) { return Quantity(a.value - b.value); }
    friend constexpr Quantity operator*(Quantity a, double k) { return Quantity(a.value * k); }
    friend constexpr Quantity operator*(double k, Quantity a) { return Quantity(a.value * k); }
    friend constexpr Quantity operator/(Quantity a, double k) { return Quantity(a.value / k); }
    friend constexpr auto operator<=>(Quantity a, Quantity b) { return a.value <=> b.value; }
    friend constexpr bool operator==(Quantity a, Quantity b) { return a.value == b.value; }
};

template <int M1, int L1, int T1, int K1, int M2, int L2, int T2, int K2>
constexpr Quantity<M1 + M2, L1 + L2, T1 + T2, K1 + K2> operator*(Quantity<M1, L1, T1, K1> a,
                                                                 Quantity<M2, L2, T2, K2> b)
{
    return Quantity<M1 + M2, L1 + L2, T1 + T2, K1 + K2>(a.value * b.value);
}

template <int M1, int L1, int T1, int K1, int M2, int L2, int T2, int K2>
constexpr Quantity<M1 - M2, L1 - L2, T1 - T2, K1 - K2> operator/(Quantity<M1, L1, T1, K1> a,
                                                                 Quantity<M2, L2, T2, K2> b)
{
    return Quantity<M1 - M2, L1 - L2, T1 - T2, K1 - K2>(a.value / b.value);
}

template <int M, int L, int T, int K>
constexpr Quantity<-M, -L, -T, -K> operator/(double k, Quantity<M, L, T, K> a)
{
    return Quantity<-M, -L, -T, -K>(k / a.value);
}

// Dimensionless quantities convert to plain numbers.
template <>
struct Quantity<0, 0, 0, 0>
{
    double value = 0;
    constexpr Quantity() = default;
    constexpr Quantity(double v) : value(v) {}
    constexpr operator double() const { return value; }
    constexpr double si() const { return value; }
};

template <int M, int L, int T, int K>
Quantity<M / 2, L / 2, T / 2, K / 2> sqrt(Quantity<M, L, T, K> q)
{
    static_assert(M % 2 == 0 && L % 2 == 0 && T % 2 == 0 && K % 2 == 0,
                  "square root of a quantity with odd dimensions");
    return Quantity<M / 2, L / 2, T / 2, K / 2>(std::sqrt(q.value));
}

template <int M, int L, int T, int K>
constexpr Quantity<2 * M, 2 * L, 2 * T, 2 * K> squared(Quantity<M, L, T, K> q)
{
    return Quantity<2 * M, 2 * L, 2 * T, 2 * K>(q.value * q.value);
}

template <int M, int L, int T, int K>
constexpr Quantity<3 * M, 3 * L, 3 * T, 3 * K> cubed(Quantity<M, L, T, K> q)
{
    return Quantity<3 * M, 3 * L, 3 * T, 3 * K>(q.value * q.value * q.value);
}

template <int M, int L, int T, int K>
Quantity<M, L, T, K> abs(Quantity<M, L, T, K> q)
{
    return Quantity<M, L, T, K>(std::abs(q.value));
}

using Dimensionless = Quantity<0, 0, 0, 0>;
using Mass = Quantity<1, 0, 0, 0>;
using Length = Quantity<0, 1, 0, 0>;
using Time = Quantity<0, 0, 1, 0>;
using Temperature = Quantity<0, 0, 0, 1>;
using Area = Quantity<0, 2, 0, 0>;
using Volume = Quantity<0, 3, 0, 0>;
using Frequency = Quantity<0, 0, -1, 0>;
using Speed = Quantity<0, 1, -1, 0>;
using Acceleration = Quantity<0, 1, -2, 0>;
using Force = Quantity<1, 1, -2, 0>;
using Energy = Quantity<1, 2, -2, 0>;
using Power = Quantity<1, 2, -3, 0>;
using Pressure = Quantity<1, -1, -2, 0>;
using Density = Quantity<1, -3, 0, 0>;
using HeatFlux = Quantity<1, 0, -3, 0>;           // W/m²
using SpecificEnergy = Quantity<0, 2, -2, 0>;      // J/kg
using SpecificHeat = Quantity<0, 2, -2, -1>;       // J/(kg·K)
using EnergyPerTemperature = Quantity<1, 2, -2, -1>; // J/K (Boltzmann)
using GravParameter = Quantity<0, 3, -2, 0>;       // m³/s²
using SpecificPower = Quantity<0, 2, -3, 0>;       // W/kg

namespace literals {
constexpr Mass operator""_kg(long double v) { return Mass(double(v)); }
constexpr Mass operator""_kg(unsigned long long v) { return Mass(double(v)); }
constexpr Mass operator""_t(long double v) { return Mass(double(v) * 1000.0); }
constexpr Length operator""_m(long double v) { return Length(double(v)); }
constexpr Length operator""_m(unsigned long long v) { return Length(double(v)); }
constexpr Length operator""_km(long double v) { return Length(double(v) * 1000.0); }
constexpr Length operator""_km(unsigned long long v) { return Length(double(v) * 1000.0); }
constexpr Time operator""_s(long double v) { return Time(double(v)); }
constexpr Time operator""_s(unsigned long long v) { return Time(double(v)); }
constexpr Time operator""_h(long double v) { return Time(double(v) * 3600.0); }
constexpr Time operator""_h(unsigned long long v) { return Time(double(v) * 3600.0); }
constexpr Temperature operator""_K(long double v) { return Temperature(double(v)); }
constexpr Speed operator""_mps(long double v) { return Speed(double(v)); }
constexpr Speed operator""_kmps(long double v) { return Speed(double(v) * 1000.0); }
} // namespace literals

} // namespace cr::units
