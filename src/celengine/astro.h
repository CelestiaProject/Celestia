// astro.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _ASTRO_H_
#define _ASTRO_H_

#include <iostream>
#include <celmath/vecmath.h>
#include <celengine/univcoord.h>

#define SOLAR_ABSMAG  4.83f
#define LN_MAG        1.085736
#define LY_PER_PARSEC 3.26
#define KM_PER_LY     9466411842000.000
#define KM_PER_AU     149597870.7
#define AU_PER_LY     (KM_PER_LY / KM_PER_AU)

namespace astro
{
    class Date
    {
    public:
        Date(int Y, int M, int D);
        Date(double);

        operator double() const;

    public:
        int year;
        int month;
        int day;
        int hour;
        int minute;
        double seconds;
    };

    enum CoordinateSystem
    {
        Universal       = 0,
        Ecliptical      = 1,
        Equatorial      = 2,
        Geographic      = 3,
        ObserverLocal   = 4,
    };

    float lumToAbsMag(float lum);
    float lumToAppMag(float lum, float lyrs);
    float absMagToLum(float mag);
    float appMagToLum(float mag, float lyrs);
    float absToAppMag(float absMag, float lyrs);
    float appToAbsMag(float appMag, float lyrs);
    float lightYearsToParsecs(float);
    float parsecsToLightYears(float);
    float lightYearsToKilometers(float);
    double lightYearsToKilometers(double);
    float kilometersToLightYears(float);
    double kilometersToLightYears(double);
    float lightYearsToAU(float);
    double lightYearsToAU(double);
    float AUtoLightYears(float);
    float AUtoKilometers(float);
    float kilometersToAU(float);

    double secondsToJulianDate(double);
    double julianDateToSeconds(double);

    float sphereIlluminationFraction(Point3d spherePos,
                                     Point3d viewerPos);

    Point3d heliocentricPosition(UniversalCoord universal,
                                 Point3f starPosition);
    UniversalCoord universalPosition(Point3d heliocentric,
                                     Point3f starPosition);

    Point3f equatorialToCelestialCart(float ra, float dec, float distance);
    Point3d equatorialToCelestialCart(double ra, double dec, double distance);

    void Anomaly(double meanAnomaly, double eccentricity,
                 double& trueAnomaly, double& eccentricAnomaly);
    double meanEclipticObliquity(double jd);

    extern const double J2000;
    extern const double speedOfLight; // km/s
};

// Convert a date structure to a Julian date

std::ostream& operator<<(std::ostream& s, const astro::Date);

#endif // _ASTRO_H_

