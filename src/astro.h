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
#include "vecmath.h"
#include "univcoord.h"


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

    extern const double J2000;
};

// Convert a date structure to a Julian date

std::ostream& operator<<(std::ostream& s, const astro::Date);

#endif // _ASTRO_H_

