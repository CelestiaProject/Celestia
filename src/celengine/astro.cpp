// astro.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <cmath>
#include <iomanip>
#include <cstdio>
#include <time.h>
#include <celutil/basictypes.h>
#include <celmath/mathlib.h>
#include "celestia.h"
#include "astro.h"
#include "univcoord.h"
#include <celutil/util.h>
#include <celmath/geomutil.h>

using namespace Eigen;
using namespace std;

const double astro::speedOfLight = 299792.458; // km/s

// epoch J2000: 12 UT on 1 Jan 2000
const double astro::J2000 = 2451545.0;

const double astro::G = 6.672e-11; // N m^2 / kg^2

const double astro::SolarMass = 1.989e30;
const double astro::EarthMass = 5.976e24;
const double astro::LunarMass = 7.354e22;

const double astro::SOLAR_IRRADIANCE  = 1367.6;        // Watts / m^2
const double astro::SOLAR_POWER       =    3.8462e26;  // Watts

// Angle between J2000 mean equator and the ecliptic plane.
// 23 deg 26' 21".448 (Seidelmann, _Explanatory Supplement to the
// Astronomical Almanac_ (1992), eqn 3.222-1.
const double astro::J2000Obliquity = degToRad(23.4392911);

static const Quaterniond ECLIPTIC_TO_EQUATORIAL_ROTATION = XRotation(-astro::J2000Obliquity);
static const Matrix3d ECLIPTIC_TO_EQUATORIAL_MATRIX = ECLIPTIC_TO_EQUATORIAL_ROTATION.toRotationMatrix();

static const Quaterniond EQUATORIAL_TO_ECLIPTIC_ROTATION =
    Quaterniond(AngleAxis<double>(-astro::J2000Obliquity, Vector3d::UnitX()));
static const Matrix3d EQUATORIAL_TO_ECLIPTIC_MATRIX = EQUATORIAL_TO_ECLIPTIC_ROTATION.toRotationMatrix();
static const Matrix3f EQUATORIAL_TO_ECLIPTIC_MATRIX_F = EQUATORIAL_TO_ECLIPTIC_MATRIX.cast<float>();

// Equatorial to galactic coordinate transformation
// North galactic pole at:
// RA 12h 51m 26.282s (192.85958 deg)
// Dec 27 d 07' 42.01" (27.1283361 deg)
// Zero longitude at position angle 122.932
// (J2000 coordinates)
static const double GALACTIC_NODE = 282.85958;
static const double GALACTIC_INCLINATION = 90.0 - 27.1283361;
static const double GALACTIC_LONGITUDE_AT_NODE = 32.932;

static const Quaterniond EQUATORIAL_TO_GALACTIC_ROTATION =
    ZRotation(degToRad(GALACTIC_NODE)) *
    XRotation(degToRad(GALACTIC_INCLINATION)) *
    ZRotation(degToRad(-GALACTIC_LONGITUDE_AT_NODE));
static const Matrix3d EQUATORIAL_TO_GALACTIC_MATRIX = EQUATORIAL_TO_GALACTIC_ROTATION.toRotationMatrix();

// epoch B1950: 22:09 UT on 21 Dec 1949
#define B1950         2433282.423

// Difference in seconds between Terrestrial Time and International
// Atomic Time
static const double dTA = 32.184;

struct LeapSecondRecord
{
    int seconds;
    double t;
};


// Table of leap second insertions. The leap second always
// appears as the last second of the day immediately prior
// to the date in the table.
static const LeapSecondRecord LeapSeconds[] =
{
    { 10, 2441317.5 }, // 1 Jan 1972
    { 11, 2441499.5 }, // 1 Jul 1972
    { 12, 2441683.5 }, // 1 Jan 1973
    { 13, 2442048.5 }, // 1 Jan 1974
    { 14, 2442413.5 }, // 1 Jan 1975
    { 15, 2442778.5 }, // 1 Jan 1976
    { 16, 2443144.5 }, // 1 Jan 1977
    { 17, 2443509.5 }, // 1 Jan 1978
    { 18, 2443874.5 }, // 1 Jan 1979
    { 19, 2444239.5 }, // 1 Jan 1980
    { 20, 2444786.5 }, // 1 Jul 1981
    { 21, 2445151.5 }, // 1 Jul 1982
    { 22, 2445516.5 }, // 1 Jul 1983
    { 23, 2446247.5 }, // 1 Jul 1985
    { 24, 2447161.5 }, // 1 Jan 1988
    { 25, 2447892.5 }, // 1 Jan 1990
    { 26, 2448257.5 }, // 1 Jan 1991
    { 27, 2448804.5 }, // 1 Jul 1992
    { 28, 2449169.5 }, // 1 Jul 1993
    { 29, 2449534.5 }, // 1 Jul 1994
    { 30, 2450083.5 }, // 1 Jan 1996
    { 31, 2450630.5 }, // 1 Jul 1997
    { 32, 2451179.5 }, // 1 Jan 1999
    { 33, 2453736.5 }, // 1 Jan 2006
    { 34, 2454832.5 }, // 1 Jan 2009
    { 35, 2456109.5 }, // 1 Jul 2012
    { 36, 2457204.5 }, // 1 Jul 2015
    { 37, 2457388.5 }, // 1 Jan 2016
};


#if !defined(__GNUC__) || defined(_WIN32)
static const char* MonthAbbrList[12] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
#endif


struct UnitDefinition
{
    const char* name;
    double conversion;
};


static const UnitDefinition lengthUnits[] =
{
    { "km", 1.0 },
    { "m",  0.001 },
    { "rE", (double) EARTH_RADIUS },
    { "rJ", (double) JUPITER_RADIUS },
    { "rS", (double) SOLAR_RADIUS },
    { "AU", (double) KM_PER_AU },
    { "ly", (double) KM_PER_LY },
    { "pc", (double) KM_PER_PARSEC },
    { "kpc", 1000.0 * ((double) KM_PER_PARSEC) },
    { "Mpc", 1000000.0 * ((double) KM_PER_PARSEC) },
};


static const UnitDefinition timeUnits[] =
{
    { "s", 1.0 / SECONDS_PER_DAY },
    { "min", 1.0 / MINUTES_PER_DAY },
    { "h", 1.0 / HOURS_PER_DAY },
    { "d", 1.0 },
    { "y", DAYS_PER_YEAR },
};


static const UnitDefinition angleUnits[] =
{
    { "mas", 0.001 / SECONDS_PER_DEG },
    { "arcsec", 1.0 / SECONDS_PER_DEG },
    { "arcmin", 1.0 / MINUTES_PER_DEG },
    { "deg", 1.0 },
    { "hRA", DEG_PER_HRA },
    { "rad", 180.0 / PI },
};


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

float astro::lightYearsToParsecs(float ly)
{
    return ly / (float) LY_PER_PARSEC;
}

double astro::lightYearsToParsecs(double ly)
{
    return ly / (double) LY_PER_PARSEC;
}

float astro::parsecsToLightYears(float pc)
{
    return pc * (float) LY_PER_PARSEC;
}

double astro::parsecsToLightYears(double pc)
{
    return pc * (double) LY_PER_PARSEC;
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

float astro::AUtoKilometers(float au)
{
    return au * (float) KM_PER_AU;
}

double astro::AUtoKilometers(double au)
{
    return au * (double) KM_PER_AU;
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
    return sec / SECONDS_PER_DAY;
}

double astro::julianDateToSeconds(double jd)
{
    return jd * SECONDS_PER_DAY;
}

void astro::decimalToDegMinSec(double angle, int& degrees, int& minutes, double& seconds)
{
    double A, B, C;

    degrees = (int) angle;

    A = angle - (double) degrees;
    B = A * 60.0;
    minutes = (int) B;
    C = B - (double) minutes;
    seconds = C * 60.0;
}

double astro::degMinSecToDecimal(int degrees, int minutes, double seconds)
{
    return (double)degrees + (seconds/60.0 + (double)minutes)/60.0;
}

void astro::decimalToHourMinSec(double angle, int& hours, int& minutes, double& seconds)
{
    double A, B;

    A = angle / 15.0;
    hours = (int) A;
    B = (A - (double) hours) * 60.0;
    minutes = (int) (B);
    seconds = (B - (double) minutes) * 60.0;
}

// Compute the fraction of a sphere which is illuminated and visible
// to a viewer.  The source of illumination is assumed to be at (0, 0, 0)
float astro::sphereIlluminationFraction(Point3d,
                                        Point3d)
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
    return ly * (float) AU_PER_LY * 1e-6f;
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


// Convert equatorial coordinates to Cartesian celestial (or ecliptical)
// coordinates.
Eigen::Vector3f
astro::equatorialToCelestialCart(float ra, float dec, float distance)
{
    double theta = ra / 24.0 * PI * 2 + PI;
    double phi = (dec / 90.0 - 1.0) * PI / 2;
    double x = cos(theta) * sin(phi) * distance;
    double y = cos(phi) * distance;
    double z = -sin(theta) * sin(phi) * distance;

    return EQUATORIAL_TO_ECLIPTIC_MATRIX_F * Vector3f((float) x, (float) y, (float) z);
}


// Convert equatorial coordinates to Cartesian celestial (or ecliptical)
// coordinates.
Eigen::Vector3d
astro::equatorialToCelestialCart(double ra, double dec, double distance)
{
    double theta = ra / 24.0 * PI * 2 + PI;
    double phi = (dec / 90.0 - 1.0) * PI / 2;
    double x = cos(theta) * sin(phi) * distance;
    double y = cos(phi) * distance;
    double z = -sin(theta) * sin(phi) * distance;

    return EQUATORIAL_TO_ECLIPTIC_MATRIX * Vector3d(x, y, z);
}


/** Convert spherical coordinates in the J2000 equatorial frame to cartesian
  * coordinates in the J2000 ecliptic frame. RA in hours, dec in degrees.
  */
Eigen::Vector3f
astro::equatorialToEclipticCartesian(float ra, float dec, float distance)
{
    double theta = ra / 24.0 * PI * 2 + PI;
    double phi = (dec / 90.0 - 1.0) * PI / 2;
    double x = cos(theta) * sin(phi) * distance;
    double y = cos(phi) * distance;
    double z = -sin(theta) * sin(phi) * distance;

    return EQUATORIAL_TO_ECLIPTIC_MATRIX_F * Eigen::Vector3f((float) x, (float) y, (float) z);
}


void astro::anomaly(double meanAnomaly, double eccentricity,
                    double& trueAnomaly, double& eccentricAnomaly)
{
    double e, delta, err;
    double tol = 0.00000001745;
    int iterations = 20;    // limit while() to maximum of 20 iterations.

    e = meanAnomaly - 2*PI * (int) (meanAnomaly / (2*PI));
    err = 1;
    while(abs(err) > tol && iterations > 0)
    {
        err = e - eccentricity*sin(e) - meanAnomaly;
        delta = err / (1 - eccentricity * cos(e));
        e -= delta;
        iterations--;
    }

    trueAnomaly = 2*atan(sqrt((1+eccentricity)/(1-eccentricity))*tan(e/2));
    eccentricAnomaly = e;
}


/*! Return the angle between the mean ecliptic plane and mean equator at
 *  the specified Julian date.
 */
// TODO: replace this with a better precession model
double astro::meanEclipticObliquity(double jd)
{
    double t, de;

    jd -= 2451545.0;
    t = jd / 36525;
    de = (46.815 * t + 0.0006 * t * t - 0.00181 * t * t * t) / 3600;

    return J2000Obliquity - de;
}


/*! Return a quaternion giving the transformation from the J2000 ecliptic
 *  coordinate system to the J2000 Earth equatorial coordinate system.
 */
Quaterniond astro::eclipticToEquatorial()
{
    return ECLIPTIC_TO_EQUATORIAL_ROTATION;
}


/*! Rotate a vector in the J2000 ecliptic coordinate system to
 *  the J2000 Earth equatorial coordinate system.
 */
Vector3d astro::eclipticToEquatorial(const Vector3d& v)
{
    return ECLIPTIC_TO_EQUATORIAL_MATRIX.transpose() * v;
}


/*! Return a quaternion giving the transformation from the J2000 Earth
 *  equatorial coordinate system to the galactic coordinate system.
 */
Quaterniond astro::equatorialToGalactic()
{
    return EQUATORIAL_TO_GALACTIC_ROTATION;
}


/*! Rotate a vector int the J2000 Earth equatorial coordinate system to
 *  the galactic coordinate system.
 */
Vector3d astro::equatorialToGalactic(const Vector3d& v)
{
    return EQUATORIAL_TO_GALACTIC_MATRIX.transpose() * v;
}



astro::Date::Date()
{
    year = 0;
    month = 0;
    day = 0;
    hour = 0;
    minute = 0;
    seconds = 0.0;
    wday = 0;
    utc_offset = 0;
    tzname = "UTC";
}

astro::Date::Date(int Y, int M, int D)
{
    year = Y;
    month = M;
    day = D;
    hour = 0;
    minute = 0;
    seconds = 0.0;
    wday = 0;
    utc_offset = 0;
    tzname = "UTC";
}

astro::Date::Date(double jd)
{
    int64 a = (int64) floor(jd + 0.5);
    wday = (a + 1) % 7;
    double c;
    if (a < 2299161)
    {
        c = (double) (a + 1524);
    }
    else
    {
        double b = (double) ((int64) floor((a - 1867216.25) / 36524.25));
        c = a + b - (int64) floor(b / 4) + 1525;
    }

    int64 d = (int64) floor((c - 122.1) / 365.25);
    int64 e = (int64) floor(365.25 * d);
    int64 f = (int64) floor((c - e) / 30.6001);

    double dday = c - e - (int64) floor(30.6001 * f) + ((jd + 0.5) - a);

    // This following used to be 14.0, but gcc was computing it incorrectly, so
    // it was changed to 14
    month = (int) (f - 1 - 12 * (int64) (f / 14));
    year = (int) (d - 4715 - (int64) ((7.0 + month) / 10.0));
    day = (int) dday;

    double dhour = (dday - day) * 24;
    hour = (int) dhour;

    double dminute = (dhour - hour) * 60;
    minute = (int) dminute;

    seconds = (dminute - minute) * 60;
    utc_offset = 0;
    tzname = "UTC";
}

const char* astro::Date::toCStr(Format format) const
{
    static char date[255];

    // MinGW's libraries don't have the tm_gmtoff and tm_zone fields for
    // struct tm.
#if defined(__GNUC__) && !defined(_WIN32)
    struct tm cal_time;
    memset(&cal_time, 0, sizeof(cal_time));
    cal_time.tm_year = year-1900;
    cal_time.tm_mon = month-1;
    cal_time.tm_mday = day;
    cal_time.tm_hour = hour;
    cal_time.tm_min = minute;
    cal_time.tm_sec = (int)seconds;
    cal_time.tm_wday = wday;
    cal_time.tm_gmtoff = utc_offset;
#if defined(TARGET_OS_MAC) || defined(__FreeBSD__)
    // tm_zone is a non-const string field on the Mac and FreeBSD (why?)
    cal_time.tm_zone = const_cast<char*>(tzname.c_str());
#else
    cal_time.tm_zone = tzname.c_str();
#endif

    const char* strftime_format;
    switch(format)
    {
    case Locale:
        strftime_format = "%c";
        break;
    case TZName:
        strftime_format = "%Y %b %d %H:%M:%S %Z";
        break;
    default:
        strftime_format = "%Y %b %d %H:%M:%S %z";
        break;
    }

    strftime(date, sizeof(date), strftime_format, &cal_time);
#else
    switch(format)
    {
    case Locale:
    case TZName:
        snprintf(date, sizeof(date), "%04d %s %02d %02d:%02d:%02d %s",
                 year, _(MonthAbbrList[month-1]), day,
                 hour, minute, (int)seconds, tzname.c_str());
        break;
    case UTCOffset:
        {
            int sign = utc_offset < 0 ? -1:1;
            int h_offset = sign * utc_offset / 3600;
            int m_offset = (sign * utc_offset - h_offset * 3600) / 60;
            snprintf(date, sizeof(date), "%04d %s %02d %02d:%02d:%02d %c%02d%02d",
                    year, _(MonthAbbrList[month-1]), day,
                    hour, minute, (int)seconds, (sign==1?'+':'-'), h_offset, m_offset);
        }
        break;
    }
#endif

    return date;

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
            day + hour / HOURS_PER_DAY + minute / MINUTES_PER_DAY + seconds / SECONDS_PER_DAY);
}


// TODO: need option to parse UTC times (with leap seconds)
bool astro::parseDate(const string& s, astro::Date& date)
{
    int year = 0;
    unsigned int month = 1;
    unsigned int day = 1;
    unsigned int hour = 0;
    unsigned int minute = 0;
    double second = 0.0;

    if (sscanf(s.c_str(), " %d %u %u %u:%u:%lf ",
               &year, &month, &day, &hour, &minute, &second) == 6 ||
        sscanf(s.c_str(), " %d %u %u %u:%u ",
               &year, &month, &day, &hour, &minute) == 5 ||
        sscanf(s.c_str(), " %d %u %u ", &year, &month, &day) == 3)
    {
        if (month < 1 || month > 12)
            return false;
        if (hour > 23 || minute > 59 || second >= 60.0 || second < 0.0)
            return false;

        // Days / month calculation . . .
        int maxDay = 31 - ((0xa50 >> month) & 0x1);
        if (month == 2)
        {
            // Check for a leap year
            if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
                maxDay = 29;
            else
                maxDay = 28;
        }
        if (day > (unsigned int) maxDay || day < 1)
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


astro::Date
astro::Date::systemDate()
{
    time_t t = time(NULL);
    struct tm *gmt = gmtime(&t);
    astro::Date d;
    d.year = gmt->tm_year + 1900;
    d.month = gmt->tm_mon + 1;
    d.day = gmt->tm_mday;
    d.hour = gmt->tm_hour;
    d.minute = gmt->tm_min;
    d.seconds = (int) gmt->tm_sec;

    return d;
}


ostream& operator<<(ostream& s, const astro::Date d)
{
    s << d.toCStr();
    return s;
}


/********* Time scale conversion functions ***********/

// Convert from Atomic Time to UTC
astro::Date
astro::TAItoUTC(double tai)
{
    unsigned int nRecords = sizeof(LeapSeconds) / sizeof(LeapSeconds[0]);
    double dAT = LeapSeconds[0].seconds;
    /*double dD = 0.0;  Unused*/
    int extraSecs = 0;

    for (unsigned int i = nRecords - 1; i > 0; i--)
    {
        if (tai - secsToDays(LeapSeconds[i].seconds) >= LeapSeconds[i].t)
        {
            dAT = LeapSeconds[i].seconds;
            break;
        }
        else if (tai - secsToDays(LeapSeconds[i - 1].seconds) >= LeapSeconds[i].t)
        {
            dAT = LeapSeconds[i].seconds;
            extraSecs = LeapSeconds[i].seconds - LeapSeconds[i - 1].seconds;
            break;
        }
    }

    Date utcDate(tai - secsToDays(dAT));
    utcDate.seconds += extraSecs;

    return utcDate;
}


// Convert from UTC to Atomic Time
double
astro::UTCtoTAI(const astro::Date& utc)
{
    unsigned int nRecords = sizeof(LeapSeconds) / sizeof(LeapSeconds[0]);
    double dAT = LeapSeconds[0].seconds;
    double utcjd = (double) Date(utc.year, utc.month, utc.day);

    for (unsigned int i = nRecords - 1; i > 0; i--)
    {
        if (utcjd >= LeapSeconds[i].t)
        {
            dAT = LeapSeconds[i].seconds;
            break;
        }
    }

    double tai = utcjd + secsToDays(utc.hour * 3600.0 + utc.minute * 60.0 + utc.seconds + dAT);

    return tai;
}


// Convert from Terrestrial Time to Atomic Time
double
astro::TTtoTAI(double tt)
{
    return tt - secsToDays(dTA);
}


// Convert from Atomic Time to Terrestrial TIme
double
astro::TAItoTT(double tai)
{
    return tai + secsToDays(dTA);
}


// Correction for converting from Terrestrial Time to Barycentric Dynamical
// Time. Constants and algorithm from "Time Routines in CSPICE",
// http://sohowww.nascom.nasa.gov/solarsoft/stereo/gen/exe/icy/doc/time.req
static const double K  = 1.657e-3;
static const double EB = 1.671e-2;
static const double M0 = 6.239996;
static const double M1 = 1.99096871e-7;

// Input is a TDB Julian Date; result is in seconds
double TDBcorrection(double tdb)
{
    // t is seconds from J2000.0
    double t = astro::daysToSecs(tdb - astro::J2000);

    // Approximate calculation of Earth's mean anomaly
    double M = M0 + M1 * t;

    // Compute the eccentric anomaly
    double E = M + EB * sin(M);

    return K * sin(E);
}


// Convert from Terrestrial Time to Barycentric Dynamical Time
double
astro::TTtoTDB(double tt)
{
    return tt + secsToDays(TDBcorrection(tt));
}


// Convert from Barycentric Dynamical Time to Terrestrial Time
double
astro::TDBtoTT(double tdb)
{
    return tdb - secsToDays(TDBcorrection(tdb));
}


// Convert from Coordinated Universal time to Barycentric Dynamical Time
astro::Date
astro::TDBtoUTC(double tdb)
{
    return TAItoUTC(TTtoTAI(TDBtoTT(tdb)));
}

// Convert from Barycentric Dynamical Time to local calendar if possible
// otherwise convert to UTC
astro::Date
astro::TDBtoLocal(double tdb)
{
    double tai = astro::TTtoTAI(astro::TDBtoTT(tdb));
    double jdutc = astro::TAItoJDUTC(tai);
    if (jdutc < 2465442 &&
        jdutc > 2415733)
    {
        time_t time = (int) astro::julianDateToSeconds(jdutc - 2440587.5);
        struct tm *localt = localtime(&time);
        if (localt != NULL)
        {
            astro::Date d;
            d.year = localt->tm_year + 1900;
            d.month = localt->tm_mon + 1;
            d.day = localt->tm_mday;
            d.hour = localt->tm_hour;
            d.minute = localt->tm_min;
            d.seconds = (int) localt->tm_sec;
            d.wday = localt->tm_wday;
        #if defined(__GNUC__) && !defined(_WIN32)
            d.utc_offset = localt->tm_gmtoff;
            d.tzname = localt->tm_zone;
        #else
            {
                astro::Date utcDate = astro::TAItoUTC(tai);
                int daydiff = d.day - utcDate.day;
                int hourdiff;
                if (daydiff == 0)
                    hourdiff = 0;
                else if (daydiff > 1 || daydiff == -1)
                    hourdiff = -24;
                else
                    hourdiff = 24;
                d.utc_offset = (hourdiff + d.hour - utcDate.hour) * 3600
                             + (d.minute - utcDate.minute) * 60;
            }
            d.tzname = localt->tm_isdst ? _("DST"): _("STD");
        #endif
            return d;
        }
    }
    return astro::TDBtoUTC(tdb);
}

// Convert from Barycentric Dynamical Time to UTC
double
astro::UTCtoTDB(const astro::Date& utc)
{
    return TTtoTDB(TAItoTT(UTCtoTAI(utc)));
}


// Convert from TAI to Julian Date UTC. The Julian Date UTC functions should
// generally be avoided because there's no provision for dealing with leap
// seconds.
double
astro::JDUTCtoTAI(double utc)
{
    unsigned int nRecords = sizeof(LeapSeconds) / sizeof(LeapSeconds[0]);
    double dAT = LeapSeconds[0].seconds;

    for (unsigned int i = nRecords - 1; i > 0; i--)
    {
        if (utc > LeapSeconds[i].t)
        {
            dAT = LeapSeconds[i].seconds;
            break;
        }
    }

    return utc + secsToDays(dAT);
}


// Convert from Julian Date UTC to TAI
double
astro::TAItoJDUTC(double tai)
{
    unsigned int nRecords = sizeof(LeapSeconds) / sizeof(LeapSeconds[0]);
    double dAT = LeapSeconds[0].seconds;

    for (unsigned int i = nRecords - 1; i > 0; i--)
    {
        if (tai - secsToDays(LeapSeconds[i - 1].seconds) > LeapSeconds[i].t)
        {
            dAT = LeapSeconds[i].seconds;
            break;
        }
    }

    return tai - secsToDays(dAT);
}


// Get scale of given length unit in kilometers
bool astro::getLengthScale(string unitName, double& scale)
{
    unsigned int nUnits = sizeof(lengthUnits) / sizeof(lengthUnits[0]);
    bool foundMatch = false;
    for(unsigned int i = 0; i < nUnits; i++)
    {
        if (lengthUnits[i].name == unitName)
        {
            foundMatch = true;
            scale = lengthUnits[i].conversion;
            break;
        }
    }

    return foundMatch;
}


// Get scale of given time unit in days
bool astro::getTimeScale(string unitName, double& scale)
{
    unsigned int nUnits = sizeof(timeUnits) / sizeof(timeUnits[0]);
    bool foundMatch = false;
    for(unsigned int i = 0; i < nUnits; i++)
    {
        if (timeUnits[i].name == unitName)
        {
            foundMatch = true;
            scale = timeUnits[i].conversion;
            break;
        }
    }

    return foundMatch;
}


// Get scale of given angle unit in degrees
bool astro::getAngleScale(string unitName, double& scale)
{
    unsigned int nUnits = sizeof(angleUnits) / sizeof(angleUnits[0]);
    bool foundMatch = false;
    for (unsigned int i = 0; i < nUnits; i++)
    {
        if (angleUnits[i].name == unitName)
        {
            foundMatch = true;
            scale = angleUnits[i].conversion;
            break;
        }
    }

    return foundMatch;
}


// Check if unit is a length unit
bool astro::isLengthUnit(string unitName)
{
    double dummy;
    return getLengthScale(unitName, dummy);
}


// Check if unit is a time unit
bool astro::isTimeUnit(string unitName)
{
    double dummy;
    return getTimeScale(unitName, dummy);
}


// Check if unit is an angle unit
bool astro::isAngleUnit(string unitName)
{
    double dummy;
    return getAngleScale(unitName, dummy);
}
