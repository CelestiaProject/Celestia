// date.h
//
// Copyright (C) 2001-2023, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <locale>
#include <string>

#include <celutil/array_view.h>

namespace celestia::astro
{

// epoch J2000: 12 UT on 1 Jan 2000
constexpr inline double J2000            = 2451545.0;

// Julian year
constexpr inline double DAYS_PER_YEAR = 365.25;

constexpr inline double SECONDS_PER_DAY = 86400.0;
constexpr inline double MINUTES_PER_DAY = 1440.0;
constexpr inline double HOURS_PER_DAY   = 24.0;

constexpr double secsToDays(double s)
{
    return s * (1.0 / SECONDS_PER_DAY);
}

constexpr double daysToSecs(double d)
{
    return d * SECONDS_PER_DAY;
}

struct LeapSecondRecord
{
    int seconds;
    double t;
};

// Provide leap seconds data loaded from an external source
void setLeapSeconds(celestia::util::array_view<LeapSecondRecord>);

constexpr double secondsToJulianDate(double sec)
{
    return sec / SECONDS_PER_DAY;
}
constexpr double julianDateToSeconds(double jd)
{
    return jd * SECONDS_PER_DAY;
}

class Date
{
public:
    Date() = default;
    Date(int Y, int M, int D);
    Date(double);

    enum Format
    {
        Locale          = 0,
        TZName          = 1,
        UTCOffset       = 2,
        ISO8601         = 3,
        FormatCount     = 4,
    };

    std::string toString(const std::locale& loc, Format format = Locale) const;

    operator double() const;

    static Date systemDate();

    int year{ 0 };
    int month{ 0 };
    int day{ 0 };
    int hour{ 0 };
    int minute{ 0 };
    int wday{ 0 };               // week day, 0 Sunday to 6 Saturday
    int utc_offset{ 0 };         // offset from utc in seconds
    std::string tzname{ "UTC" }; // timezone name
    double seconds{ 0.0 };
};

bool parseDate(const std::string&, Date&);

// Time scale conversions
// UTC - Coordinated Universal Time
// TAI - International Atomic Time
// TT  - Terrestrial Time
// TCB - Barycentric Coordinate Time
// TDB - Barycentric Dynamical Time

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

// Convert to and from UTC dates
double UTCtoTAI(const Date& utc);
Date TAItoUTC(double tai);
double UTCtoTDB(const Date& utc);
Date TDBtoUTC(double tdb);
Date TDBtoLocal(double tdb);

} // end namespace celestia::astro
