// astro.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <iomanip>
#include <cstdio>
#include <celmath/mathlib.h>
#include "celestia.h"
#include "astro.h"

using namespace std;

const double astro::speedOfLight = 299792.458; // km/s

// epoch J2000: 12 UT on 1 Jan 2000
const double astro::J2000 = 2451545.0;

const double astro::G = 6.672e-11; // N m^2 / kg^2

const double astro::SolarMass = 1.989e30;
const double astro::EarthMass = 5.976e24;
const double astro::LunarMass = 7.354e22;

// epoch B1950: 22:09 UT on 21 Dec 1949
#define B1950         2433282.423

static Mat3f equatorialToCelestial = Mat3f::xrotation(degToRad(23.4392911f));
static Mat3d equatorialToCelestiald = Mat3d::xrotation(degToRad(23.4392911));


float astro::lumToAbsMag(float lum)
{
    return (float) (SOLAR_ABSMAG - log(lum) * LN_MAG);
}

// Return the apparent magnitude of a star with lum times solar
// luminosity viewed at lyrs light years
float astro::lumToAppMag(float lum, float lyrs)
{
    return absToAppMag(lumToAbsMag(lum), lyrs);
}

float astro::absMagToLum(float mag)
{
    return (float) exp((SOLAR_ABSMAG - mag) / LN_MAG);
}

float astro::appMagToLum(float mag, float lyrs)
{
    return absMagToLum(appToAbsMag(mag, lyrs));
}

float astro::absToAppMag(float absMag, float lyrs)
{
    return (float) (absMag - 5 + 5 * log10(lyrs / LY_PER_PARSEC));
}

float astro::appToAbsMag(float appMag, float lyrs)
{
    return (float) (appMag + 5 - 5 * log10(lyrs / LY_PER_PARSEC));
}

float astro::lightYearsToParsecs(float ly)
{
    return ly / (float) LY_PER_PARSEC;
}

float astro::parsecsToLightYears(float pc)
{
    return pc * (float) LY_PER_PARSEC;
}

float astro::lightYearsToKilometers(float ly)
{
    return ly * (float) KM_PER_LY;
}

double astro::lightYearsToKilometers(double ly)
{
    return ly * KM_PER_LY;
}

float astro::kilometersToLightYears(float km)
{
    return km / (float) KM_PER_LY;
}

double astro::kilometersToLightYears(double km)
{
    return km / KM_PER_LY;
}

float astro::lightYearsToAU(float ly)
{
    return ly * (float) AU_PER_LY;
}

double astro::lightYearsToAU(double ly)
{
    return ly * AU_PER_LY;
}

float astro::AUtoLightYears(float au)
{
    return au / (float) AU_PER_LY;
}

float astro::AUtoKilometers(float au)
{
    return au * (float) KM_PER_AU;
}

float astro::kilometersToAU(float km)
{
    return km / (float) KM_PER_AU;
}

double astro::kilometersToAU(double km)
{
    return km / KM_PER_AU;
}

double astro::secondsToJulianDate(double sec)
{
    return sec / 86400.0;
}

double astro::julianDateToSeconds(double jd)
{
    return jd * 86400.0;
}

void astro::decimalToDegMinSec(double angle, int& hours, int& minutes, double& seconds)
{
    double A, B, C;

    hours = (int) angle;

    A = angle - (double) hours;
    B = A * 60.0;
    minutes = (int) B;
    C = B - (double) minutes;
    seconds = C * 60.0;
}

double astro::degMinSecToDecimal(int hours, int minutes, double seconds)
{
    return (double)hours + (seconds/60.0 + (double)minutes)/60.0;
}


// Compute the fraction of a sphere which is illuminated and visible
// to a viewer.  The source of illumination is assumed to be at (0, 0, 0)
float astro::sphereIlluminationFraction(Point3d spherePos,
                                        Point3d viewerPos)
{
    return 1.0f;
}

float astro::microLightYearsToKilometers(float ly)
{
    return ly * ((float) KM_PER_LY * 1e-6f);
}

double astro::microLightYearsToKilometers(double ly)
{
    return ly * (KM_PER_LY * 1e-6);
}

float astro::kilometersToMicroLightYears(float km)
{
    return km / ((float) KM_PER_LY * 1e-6f);
}

double astro::kilometersToMicroLightYears(double km)
{
    return km / (KM_PER_LY * 1e-6);
}

float astro::microLightYearsToAU(float ly)
{
    return ly * AU_PER_LY * 1e-6f;
}

double astro::microLightYearsToAU(double ly)
{
    return ly * AU_PER_LY * 1e-6;
}

float astro::AUtoMicroLightYears(float au)
{
    return au / ((float) AU_PER_LY * 1e-6f);
}

double astro::AUtoMicroLightYears(double au)
{
    return au / (AU_PER_LY * 1e-6);
}


// Convert the position in univeral coordinates to a star-centric
// coordinates in units of kilometers.  Note that there are three different
// precisions used here:  star coordinates are stored as floats in units of
// light years, position within a solar system are doubles in units of
// kilometers, and p is highest-precision in units of light years.
Point3d astro::heliocentricPosition(const UniversalCoord& universal,
                                    const Point3f& starPosition)
{
    // Get the offset vector
    Vec3d v = universal - Point3d(starPosition.x * 1e6,
                                  starPosition.y * 1e6,
                                  starPosition.z * 1e6);

    // . . . and convert it to kilometers
    return Point3d(microLightYearsToKilometers(v.x),
                   microLightYearsToKilometers(v.y),
                   microLightYearsToKilometers(v.z));
}

// universalPosition is the inverse operation of heliocentricPosition
UniversalCoord astro::universalPosition(const Point3d& heliocentric,
                                        const Point3f& starPosition)
{
    return UniversalCoord(Point3d(starPosition.x * 1e6,
                                  starPosition.y * 1e6,
                                  starPosition.z * 1e6)) +
        Vec3d(kilometersToMicroLightYears(heliocentric.x),
              kilometersToMicroLightYears(heliocentric.y),
              kilometersToMicroLightYears(heliocentric.z));
}


// Convert equatorial coordinates to Cartesian celestial (or ecliptical)
// coordinates.
Point3f astro::equatorialToCelestialCart(float ra, float dec, float distance)
{
    double theta = ra / 24.0 * PI * 2 + PI;
    double phi = (dec / 90.0 - 1.0) * PI / 2;
    double x = cos(theta) * sin(phi) * distance;
    double y = cos(phi) * distance;
    double z = -sin(theta) * sin(phi) * distance;

    return (Point3f((float) x, (float) y, (float) z) * equatorialToCelestial);
}


// Convert equatorial coordinates to Cartesian celestial (or ecliptical)
// coordinates.
Point3d astro::equatorialToCelestialCart(double ra, double dec, double distance)
{
    double theta = ra / 24.0 * PI * 2 + PI;
    double phi = (dec / 90.0 - 1.0) * PI / 2;
    double x = cos(theta) * sin(phi) * distance;
    double y = cos(phi) * distance;
    double z = -sin(theta) * sin(phi) * distance;

    return (Point3d(x, y, z) * equatorialToCelestiald);
}


void astro::Anomaly(double meanAnomaly, double eccentricity,
                    double& trueAnomaly, double& eccentricAnomaly)
{
    double e, delta, err;
    double tol = 0.00000001745;
    int iterations = 20;	//limit while() to maximum of 20 iterations.

    e = meanAnomaly - 2*PI * (int)(meanAnomaly/(2*PI));
    err = 1;
    while(abs(err) > tol && iterations > 0)
    {
        err = e - eccentricity*sin(e) - meanAnomaly;
        delta = err / (1 - eccentricity * sin(e));
        e -= delta;
        iterations--;
    }

	trueAnomaly = 2*atan(sqrt((1+eccentricity)/(1-eccentricity))*tan(e/2));
	eccentricAnomaly = e;
}


double astro::meanEclipticObliquity(double jd)
{
    double t, de;

    jd -= 2451545.0;
    t = jd / 36525;
    de = (46.815 * t + 0.0006 * t * t - 0.00181 * t * t * t) / 3600;

    return 23.439292 - de;
}

astro::Date::Date()
{
    year = 0;
    month = 0;
    day = 0;
    hour = 0;
    minute = 0;
    seconds = 0.0;
}

astro::Date::Date(int Y, int M, int D)
{
    year = Y;
    month = M;
    day = D;
    hour = 0;
    minute = 0;
    seconds = 0.0;
}


astro::Date::Date(double jd)
{
    int a = (int) (jd + 0.5);
    double c;
    if (a < 2299161)
    {
        c = a + 1524;
    }
    else
    {
        double b = (int) ((a - 1867216.25) / 36524.25);
        c = a + b - (int) (b / 4) + 1525;
    }

    int d = (int) ((c - 122.1) / 365.25);
    int e = (int) (365.25 * d);
    int f = (int) ((c - e) / 30.6001);

    double dday = c - e - (int) (30.6001 * f) + ((jd + 0.5) - (int) (jd + 0.5));

    /* This following used to be 14.0, but gcc was computing it incorrectly, so
       it was changed to 14 */
    month = f - 1 - 12 * (int) (f / 14);
    year = d - 4715 - (int) ((7.0 + month) / 10.0);
    day = (int) dday;
    
    double dhour = (dday - day) * 24;
    hour = (int) dhour;

    double dminute = (dhour - hour) * 60;
    minute = (int) dminute;

    seconds = (dminute - minute) * 60;
}


// Convert a calendar date to a Julian date
astro::Date::operator double() const
{
    int y = year, m = month;
    if (month <= 2)
    {
        y = year - 1;
        m = month + 12;
    }

    // Correct for the lost days in Oct 1582 when the Gregorian calendar
    // replaced the Julian calendar.
    int B = -2;
    if (year > 1582 || (year == 1582 && (month > 10 || (month == 10 && day >= 15))))
    {
        B = y / 400 - y / 100;
    }

    return (floor(365.25 * y) +
            floor(30.6001 * (m + 1)) + B + 1720996.5 +
            day + hour / 24.0 + minute / 1440.0 + seconds / 86400.0);
}


bool astro::parseDate(const string& s, astro::Date& date)
{
    int year = 0;
    unsigned int month = 1;
    unsigned int day = 1;
    unsigned int hour = 0;
    unsigned int minute = 0;
    unsigned int second = 0;

    if (sscanf(s.c_str(), " %d %u %u %u:%u:%u ",
               &year, &month, &day, &hour, &minute, &second) == 6 ||
        sscanf(s.c_str(), " %d %u %u %u:%u ",
               &year, &month, &day, &hour, &minute) == 5 ||
        sscanf(s.c_str(), " %d %u %u ", &year, &month, &day) == 3)
    {
        if (month < 1 || month > 12)
            return false;
        if (hour > 23 || minute > 59 || second > 59)
            return false;

        // Cheesy days/month calculation . . .
        int maxDay = 31 - ((0xa50 >> month) & 0x1);
        if (month == 2)
        {
            // Check for a leap year
            if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
                maxDay = 29;
            else
                maxDay = 28;
        }
        if (day > maxDay || day < 1)
            return false;

        date.year = year;
        date.month = month;
        date.day = day;
        date.hour = hour;
        date.minute = minute;
        date.seconds = second;

        return true;
    }

    return false;
}


ostream& operator<<(ostream& s, const astro::Date d)
{
    s << d.year << ' ' << setw(2) << setfill('0') << d.month << ' ';
    s << setw(2) << setfill('0') << d.day << ' ';
    s << setw(2) << setfill('0') << d.hour << ':';
    s << setw(2) << setfill('0') << d.minute << ':';
    s << setw(2) << setfill('0') << (int) d.seconds;
    return s;
}

