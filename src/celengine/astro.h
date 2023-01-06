// astro.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Geometry>
#include <iosfwd>
#include <optional>
#include <string>
#include <celmath/mathlib.h>
#include <celutil/array_view.h>

constexpr inline float SOLAR_ABSMAG = 4.83f;
constexpr inline float LN_MAG = 1.085736f;

template<typename T>
constexpr inline T LY_PER_PARSEC = static_cast<T>(3.26167);

template<typename T>
constexpr inline T KM_PER_LY = static_cast<T>(9460730472580.8);

// Old incorrect value; will be required for cel:// URL compatibility
// #define OLD_KM_PER_LY     9466411842000.000

template<typename T>
constexpr inline T KM_PER_AU = static_cast<T>(149597870.7);

template<typename T>
constexpr inline T AU_PER_LY = KM_PER_LY<T> / KM_PER_AU<T>;

template<typename T>
constexpr inline T KM_PER_PARSEC = KM_PER_LY<T> * LY_PER_PARSEC<T>;

// Julian year
constexpr inline double DAYS_PER_YEAR = 365.25;

constexpr inline double SECONDS_PER_DAY = 86400.0;
constexpr inline double MINUTES_PER_DAY = 1440.0;
constexpr inline double HOURS_PER_DAY   = 24.0;

constexpr inline double MINUTES_PER_DEG = 60.0;
constexpr inline double SECONDS_PER_DEG = 3600.0;
constexpr inline double DEG_PER_HRA     = 15.0;

template<typename T>
constexpr inline T EARTH_RADIUS = static_cast<T>(6378.14);

template<typename T>
constexpr inline T JUPITER_RADIUS = static_cast<T>(71492.0);

template<typename T>
constexpr inline T SOLAR_RADIUS = static_cast<T>(696000.0);

class UniversalCoord;

namespace astro
{
    class Date
    {
    public:
        Date();
        Date(int Y, int M, int D);
        Date(double);

        enum Format
        {
            Locale          = 0,
            TZName          = 1,
            UTCOffset       = 2,
            ISO8601         = 3,
        };

        const char* toCStr(Format format = Locale) const;

        operator double() const;

        static Date systemDate();


    public:
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int wday;           // week day, 0 Sunday to 6 Saturday
        int utc_offset;     // offset from utc in seconds
        std::string tzname; // timezone name
        double seconds;
    };

    bool parseDate(const std::string&, Date&);

    // Time scale conversions
    // UTC - Coordinated Universal Time
    // TAI - International Atomic Time
    // TT  - Terrestrial Time
    // TCB - Barycentric Coordinate Time
    // TDB - Barycentric Dynamical Time

    inline double secsToDays(double s)
    {
        return s * (1.0 / SECONDS_PER_DAY);
    }

    inline double daysToSecs(double d)
    {
        return d * SECONDS_PER_DAY;
    }

    // Convert to and from UTC dates
    double UTCtoTAI(const astro::Date& utc);
    astro::Date TAItoUTC(double tai);
    double UTCtoTDB(const astro::Date& utc);
    astro::Date TDBtoUTC(double tdb);
    astro::Date TDBtoLocal(double tdb);

    // Convert among uniform time scales
    double TTtoTAI(double tt);
    double TAItoTT(double tai);
    double TTtoTDB(double tt);
    double TDBtoTT(double tdb);

    // Conversions to and from Julian Date UTC--other time systems
    // should be preferred, since UTC Julian Dates aren't defined
    // during leapseconds.
    double JDUTCtoTAI(double utc);
    double TAItoJDUTC(double tai);

    // Magnitude conversions
    float lumToAbsMag(float lum);
    float lumToAppMag(float lum, float lyrs);
    float absMagToLum(float mag);
    float appMagToLum(float mag, float lyrs);

    template<class T> constexpr T absToAppMag(T absMag, T lyrs)
    {
        using std::log10;
        return (T) (absMag - 5 + 5 * log10(lyrs / LY_PER_PARSEC<T>));
    }

    template<class T> constexpr T appToAbsMag(T appMag, T lyrs)
    {
        using std::log10;
        return (T) (appMag + 5 - 5 * log10(lyrs / LY_PER_PARSEC<T>));
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
        return ly * (KM_PER_LY<T> * 1e-6);
    }
    template<class T> constexpr T kilometersToMicroLightYears(T km)
    {
        return km / (KM_PER_LY<T> * 1e-6);
    }
    template<class T> constexpr T microLightYearsToAU(T ly)
    {
        return ly * (AU_PER_LY<T> * 1e-6);
    }
    template<class T> constexpr T AUtoMicroLightYears(T au)
    {
        return au / (AU_PER_LY<T> * 1e-6);
    }


    constexpr double secondsToJulianDate(double sec)
    {
        return sec / SECONDS_PER_DAY;
    }
    constexpr double julianDateToSeconds(double jd)
    {
        return jd * SECONDS_PER_DAY;
    }

    enum class LengthUnit : std::uint8_t
    {
        Default = 0,
        Kilometer,
        Meter,
        EarthRadius,
        JupiterRadius,
        SolarRadius,
        AstronomicalUnit,
        LightYear,
        Parsec,
        Kiloparsec,
        Megaparsec,
    };

    enum class TimeUnit : std::uint8_t
    {
        Default = 0,
        Second,
        Minute,
        Hour,
        Day,
        JulianYear,
    };

    enum class AngleUnit : std::uint8_t
    {
        Default = 0,
        Milliarcsecond,
        Arcsecond,
        Arcminute,
        Degree,
        Hour,
        Radian,
    };

    enum class MassUnit : std::uint8_t
    {
        Default = 0,
        Kilogram,
        EarthMass,
        JupiterMass,
    };

    std::optional<double> getLengthScale(LengthUnit unit);
    std::optional<double> getTimeScale(TimeUnit unit);
    std::optional<double> getAngleScale(AngleUnit unit);
    std::optional<double> getMassScale(MassUnit unit);

    void decimalToDegMinSec(double angle, int& degrees, int& minutes, double& seconds);
    double degMinSecToDecimal(int degrees, int minutes, double seconds);
    void decimalToHourMinSec(double angle, int& hours, int& minutes, double& seconds);

    Eigen::Vector3f equatorialToCelestialCart(float ra, float dec, float distance);
    Eigen::Vector3d equatorialToCelestialCart(double ra, double dec, double distance);

    Eigen::Vector3f equatorialToEclipticCartesian(float ra, float dec, float distance);

    Eigen::Quaterniond eclipticToEquatorial();
    Eigen::Vector3d eclipticToEquatorial(const Eigen::Vector3d& v);
    Eigen::Quaterniond equatorialToGalactic();
    Eigen::Vector3d equatorialToGalactic(const Eigen::Vector3d& v);

    void anomaly(double meanAnomaly, double eccentricity,
                 double& trueAnomaly, double& eccentricAnomaly);
    double meanEclipticObliquity(double jd);

    // epoch J2000: 12 UT on 1 Jan 2000
    constexpr inline double J2000            = 2451545.0;
    constexpr inline double speedOfLight     = 299792.458; // km/s
    constexpr inline double G                = 6.672e-11; // N m^2 / kg^2; gravitational constant
    constexpr inline double SolarMass        = 1.989e30;
    constexpr inline double EarthMass        = 5.972e24;
    constexpr inline double LunarMass        = 7.346e22;
    constexpr inline double JupiterMass      = 1.898e27;

    // Angle between J2000 mean equator and the ecliptic plane.
    // 23 deg 26' 21".448 (Seidelmann, _Explanatory Supplement to the
    // Astronomical Almanac_ (1992), eqn 3.222-1.
    constexpr inline double J2000Obliquity   = 23.4392911_deg;

    constexpr inline double SOLAR_IRRADIANCE = 1367.6; // Watts / m^2
    constexpr inline double SOLAR_POWER      = 3.8462e26;  // in Watts

    struct LeapSecondRecord
    {
        int seconds;
        double t;
    };

    // Provide leap seconds data loaded from an external source
    void setLeapSeconds(celestia::util::array_view<LeapSecondRecord>);

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
        return astro::speedOfLight * n;
    }
    }
}

// Convert a date structure to a Julian date

std::ostream& operator<<(std::ostream& s, const astro::Date&);
