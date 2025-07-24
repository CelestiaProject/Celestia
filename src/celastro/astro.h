// astro.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <config.h>

#include <cmath>
#include <cstdint>
#include <optional>
#include <type_traits>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celmath/mathlib.h>

namespace celestia::astro
{

// Angle between J2000 mean equator and the ecliptic plane: 23 deg 26' 21".448
// Seidelmann, Explanatory Supplement to the Astronomical Almanac (1992), eqn 3.222-1
constexpr inline double J2000Obliquity    = 23.4392911_deg;

// CODATA 2022
constexpr inline double speedOfLight      = 299792.458; // km/s
constexpr inline double G                 = 6.67430e-11; // N m^2 / kg^2

// IAU 2015 Resolution B3 + CODATA 2022
constexpr inline double SolarMass         = 1.3271244e20 / G; // kg
constexpr inline double EarthMass         = 3.986004e14 / G; // kg
constexpr inline double LunarMass         = 7.346e22; // kg
constexpr inline double JupiterMass       = 1.2668653e17 / G; // kg

// https://mips.as.arizona.edu/~cnaw/sun.html for Johnson V filter
constexpr inline float SOLAR_ABSMAG       = 4.81f;

// IAU 2015 Resolution B3
constexpr inline float SOLAR_IRRADIANCE   = 1361.0f; // W / m^2
constexpr inline float SOLAR_POWER        = 3.828e26f; // W

// Bessel (1979) for Johnson V filter
constexpr inline float VEGAN_IRRADIANCE   = 3.640e-11f; // W / m^2

// Auxiliary magnitude conversion factor
constexpr inline float LN_MAG             = 1.0857362f; // 5/ln(100)

// Lowest screen brightness of a point to render
constexpr inline float LOWEST_IRRADIATION = 1.0f / 255.0f;
//                                        = 1.0f / (255.0f * 12.92f); after implementing gamma correction

namespace detail
{
template<typename T>
using enable_if_fp = std::enable_if_t<std::is_floating_point_v<T>, T>;
}

// calculated in Sollya with a precision of 128 bits
// =648000*149597870700/(9460730472580800*pi)
template<typename T>
constexpr inline auto LY_PER_PARSEC = detail::enable_if_fp<T>(3.26156377716743356213863970704550837409L);

template<typename T>
constexpr inline auto KM_PER_LY = detail::enable_if_fp<T>(9460730472580.8L);

template<typename T>
constexpr inline auto KM_PER_AU = detail::enable_if_fp<T>(149597870.7L);

// calculated in Sollya with a precision of 128 bits
// =9460730472580800/149597870700
template<typename T>
constexpr inline auto AU_PER_LY = detail::enable_if_fp<T>(63241.077084266280268653583182317313558L);

// calculated in Sollya with a precision of 128 bits
// =648000*149597870.700/pi
template<typename T>
constexpr inline auto KM_PER_PARSEC = detail::enable_if_fp<T>(3.08567758149136727891393795779647161073e13L);

constexpr inline double MINUTES_PER_DEG = 60.0;
constexpr inline double SECONDS_PER_DEG = 3600.0;
constexpr inline double DEG_PER_HRA     = 15.0;

template<typename T>
constexpr inline auto EARTH_RADIUS = detail::enable_if_fp<T>(6378.1L); // IAU 2015 Resolution B3

template<typename T>
constexpr inline auto JUPITER_RADIUS = detail::enable_if_fp<T>(71492.0L); // IAU 2015 Resolution B3

template<typename T>
constexpr inline auto SOLAR_RADIUS = detail::enable_if_fp<T>(695700.0L); // IAU 2015 Resolution B3

float reflectedLuminosity(float sunLuminosity, float distanceFromSun, float objRadius);

// Magnitude conversions
float lumToAbsMag(float lum);
float lumToAppMag(float lum, float lyrs);
float lumToIrradiance(float lum, float km);
float absMagToLum(float mag);
float appMagToLum(float mag, float lyrs);
float absMagToIrradiance(float mag, float lyrs);
float magToIrradiance(float mag);
float irradianceToMag(float irradiance);
float faintestMagToExposure(float faintestMag);
float exposureToFaintestMag(float exposure);

template<class T>
CELESTIA_CMATH_CONSTEXPR T
distanceModulus(T lyrs)
{
    using std::log10;
    return T(5) * log10(lyrs / LY_PER_PARSEC<T>) - T(5);
}

template<class T>
CELESTIA_CMATH_CONSTEXPR T
absToAppMag(T absMag, T lyrs)
{
    using std::log10;
    return absMag + distanceModulus(lyrs);
}

template<class T>
CELESTIA_CMATH_CONSTEXPR T
appToAbsMag(T appMag, T lyrs)
{
    using std::log10;
    return appMag - distanceModulus(lyrs);
}

// Distance conversions
template<class T> constexpr T lightYearsToParsecs(T ly)
{
    return ly / LY_PER_PARSEC<T>;
}

template<class T> constexpr T parsecsToLightYears(T pc)
{
    return pc * LY_PER_PARSEC<T>;
}

template<class T> constexpr T lightYearsToKilometers(T ly)
{
    return ly * KM_PER_LY<T>;
}

template<class T> constexpr T kilometersToLightYears(T km)
{
    return km / KM_PER_LY<T>;
}

template<class T> constexpr T lightYearsToAU(T ly)
{
    return ly * AU_PER_LY<T>;
}

template<class T> constexpr T AUtoLightYears(T au)
{
    return au / AU_PER_LY<T>;
}

template<class T> constexpr T AUtoKilometers(T au)
{
    return au * KM_PER_AU<T>;
}

template<class T> constexpr T kilometersToAU(T km)
{
    return km / KM_PER_AU<T>;
}

template<class T> constexpr T microLightYearsToKilometers(T ly)
{
    return ly * (KM_PER_LY<T> * T(1e-6));
}

template<class T> constexpr T kilometersToMicroLightYears(T km)
{
    return km / (KM_PER_LY<T> * T(1e-6));
}

template<class T> constexpr T microLightYearsToAU(T ly)
{
    return ly * (AU_PER_LY<T> * T(1e-6));
}

template<class T> constexpr T AUtoMicroLightYears(T au)
{
    return au / (AU_PER_LY<T> * T(1e-6));
}

void decimalToDegMinSec(double angle, int& degrees, int& minutes, double& seconds);
double degMinSecToDecimal(int degrees, int minutes, double seconds);
void decimalToHourMinSec(double angle, int& hours, int& minutes, double& seconds);

Eigen::Vector3f equatorialToCelestialCart(float ra, float dec, float distance);
Eigen::Vector3d equatorialToCelestialCart(double ra, double dec, double distance);

Eigen::Quaterniond eclipticToEquatorial();
Eigen::Vector3d eclipticToEquatorial(const Eigen::Vector3d& v);
Eigen::Quaterniond equatorialToGalactic();
Eigen::Vector3d equatorialToGalactic(const Eigen::Vector3d& v);

void anomaly(double meanAnomaly, double eccentricity,
             double& trueAnomaly, double& eccentricAnomaly);
double meanEclipticObliquity(double jd);


namespace literals
{

constexpr long double operator "" _au (long double au)
{
    return AUtoKilometers(au);
}
constexpr long double operator "" _ly (long double ly)
{
    return lightYearsToKilometers(ly);
}
constexpr long double operator "" _c (long double n)
{
    return speedOfLight * n;
}

} // end namespace celestia::astro::literals


struct KeplerElements
{
    double semimajorAxis{ 0.0 };
    double eccentricity{ 0.0 };
    double inclination{ 0.0 };
    double longAscendingNode{ 0.0 };
    double argPericenter{ 0.0 };
    double meanAnomaly{ 0.0 };
    double period{ 0.0 };
};


KeplerElements StateVectorToElements(const Eigen::Vector3d&,
                                     const Eigen::Vector3d&,
                                     double);
} // end namespace celestia::astro
