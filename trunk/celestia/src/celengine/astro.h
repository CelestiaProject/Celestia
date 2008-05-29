// astro.h
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_ASTRO_H_
#define _CELENGINE_ASTRO_H_

#include <iostream>
#include <string>
#include <cmath>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celengine/univcoord.h>

#define SOLAR_ABSMAG  4.83f
#define LN_MAG        1.085736
#define LY_PER_PARSEC 3.26167
#define KM_PER_LY     9460730472580.8
// Old incorrect value; will be required for cel:// URL compatibility
// #define OLD_KM_PER_LY     9466411842000.000
#define KM_PER_AU     149597870.7
#define AU_PER_LY     (KM_PER_LY / KM_PER_AU)

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
        return s * (1.0 / 86400.0);
    }

    inline double daysToSecs(double d)
    {
        return d * 86400.0;
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

    template<class T> T absToAppMag(T absMag, T lyrs)
    {
        return (T) (absMag - 5 + 5 * log10(lyrs / LY_PER_PARSEC));
    }

    template<class T> T appToAbsMag(T appMag, T lyrs)
    {
        return (T) (appMag + 5 - 5 * log10(lyrs / LY_PER_PARSEC));
    }

    // Distance conversions
    float lightYearsToParsecs(float);
    double lightYearsToParsecs(double);
    float parsecsToLightYears(float);
    double parsecsToLightYears(double);
    float lightYearsToKilometers(float);
    double lightYearsToKilometers(double);
    float kilometersToLightYears(float);
    double kilometersToLightYears(double);
    float lightYearsToAU(float);
    double lightYearsToAU(double);

    // TODO: templatize the rest of the conversion functions
    template<class T> T AUtoLightYears(T au)
    {
        return au / (T) AU_PER_LY;
    }

    float AUtoKilometers(float);
    double AUtoKilometers(double);
    float kilometersToAU(float);
    double kilometersToAU(double);

    float microLightYearsToKilometers(float);
    double microLightYearsToKilometers(double);
    float kilometersToMicroLightYears(float);
    double kilometersToMicroLightYears(double);
    float microLightYearsToAU(float);
    double microLightYearsToAU(double);
    float AUtoMicroLightYears(float);
    double AUtoMicroLightYears(double);

    double secondsToJulianDate(double);
    double julianDateToSeconds(double);

    void decimalToDegMinSec(double angle, int& degrees, int& minutes, double& seconds);
    double degMinSecToDecimal(int degrees, int minutes, double seconds);
    void decimalToHourMinSec(double angle, int& hours, int& minutes, double& seconds);

    float sphereIlluminationFraction(Point3d spherePos,
                                     Point3d viewerPos);

    Point3d heliocentricPosition(const UniversalCoord& universal,
                                 const Point3f& starPosition);
    UniversalCoord universalPosition(const Point3d& heliocentric,
                                     const Point3f& starPosition);
    UniversalCoord universalPosition(const Point3d& heliocentric,
                                     const UniversalCoord& starPosition);

    Point3f equatorialToCelestialCart(float ra, float dec, float distance);
    Point3d equatorialToCelestialCart(double ra, double dec, double distance);

    Quatd eclipticToEquatorial();
    Vec3d eclipticToEquatorial(const Vec3d& v);
    Quatd equatorialToGalactic();
    Vec3d equatorialToGalactic(const Vec3d& v);

    void anomaly(double meanAnomaly, double eccentricity,
                 double& trueAnomaly, double& eccentricAnomaly);
    double meanEclipticObliquity(double jd);

    extern const double J2000;
    extern const double speedOfLight; // km/s
    extern const double G; // gravitational constant
    extern const double SolarMass;
    extern const double EarthMass;
    extern const double LunarMass;

    extern const double J2000Obliquity;

    extern const double SOLAR_IRRADIANCE;
    extern const double SOLAR_POWER;  // in Watts
};

// Convert a date structure to a Julian date

std::ostream& operator<<(std::ostream& s, const astro::Date);

#endif // _CELENGINE_ASTRO_H_

