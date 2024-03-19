// date.cpp
//
// Copyright (C) 2001-2023, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "date.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <locale>
#include <memory>
#include <string_view>

#include <fmt/format.h>

#include "astro.h"

#if defined(__GNUC__) && !defined(_WIN32)
#include <fmt/chrono.h>
#else
#include <iomanip>
#include <sstream>

#include <celutil/gettext.h>
#include <celutil/winutil.h>
#endif

using namespace std::string_view_literals;

namespace celestia::astro
{

namespace
{

// Difference in seconds between Terrestrial Time and International
// Atomic Time
constexpr double dTA = 32.184;

// Table of leap second insertions. The leap second always
// appears as the last second of the day immediately prior
// to the date in the table.
constexpr std::array<LeapSecondRecord, 28> LeapSeconds
{
    LeapSecondRecord{ 10, 2441317.5 }, // 1 Jan 1972
    LeapSecondRecord{ 11, 2441499.5 }, // 1 Jul 1972
    LeapSecondRecord{ 12, 2441683.5 }, // 1 Jan 1973
    LeapSecondRecord{ 13, 2442048.5 }, // 1 Jan 1974
    LeapSecondRecord{ 14, 2442413.5 }, // 1 Jan 1975
    LeapSecondRecord{ 15, 2442778.5 }, // 1 Jan 1976
    LeapSecondRecord{ 16, 2443144.5 }, // 1 Jan 1977
    LeapSecondRecord{ 17, 2443509.5 }, // 1 Jan 1978
    LeapSecondRecord{ 18, 2443874.5 }, // 1 Jan 1979
    LeapSecondRecord{ 19, 2444239.5 }, // 1 Jan 1980
    LeapSecondRecord{ 20, 2444786.5 }, // 1 Jul 1981
    LeapSecondRecord{ 21, 2445151.5 }, // 1 Jul 1982
    LeapSecondRecord{ 22, 2445516.5 }, // 1 Jul 1983
    LeapSecondRecord{ 23, 2446247.5 }, // 1 Jul 1985
    LeapSecondRecord{ 24, 2447161.5 }, // 1 Jan 1988
    LeapSecondRecord{ 25, 2447892.5 }, // 1 Jan 1990
    LeapSecondRecord{ 26, 2448257.5 }, // 1 Jan 1991
    LeapSecondRecord{ 27, 2448804.5 }, // 1 Jul 1992
    LeapSecondRecord{ 28, 2449169.5 }, // 1 Jul 1993
    LeapSecondRecord{ 29, 2449534.5 }, // 1 Jul 1994
    LeapSecondRecord{ 30, 2450083.5 }, // 1 Jan 1996
    LeapSecondRecord{ 31, 2450630.5 }, // 1 Jul 1997
    LeapSecondRecord{ 32, 2451179.5 }, // 1 Jan 1999
    LeapSecondRecord{ 33, 2453736.5 }, // 1 Jan 2006
    LeapSecondRecord{ 34, 2454832.5 }, // 1 Jan 2009
    LeapSecondRecord{ 35, 2456109.5 }, // 1 Jul 2012
    LeapSecondRecord{ 36, 2457204.5 }, // 1 Jul 2015
    LeapSecondRecord{ 37, 2457754.5 }, // 1 Jan 2017
};

celestia::util::array_view<LeapSecondRecord> g_leapSeconds = LeapSeconds; //NOSONAR

#if !(defined(__GNUC__) && !defined(_WIN32))
class MonthAbbreviations
{
public:
    explicit MonthAbbreviations(const std::locale& loc);
    std::string_view operator[](int i) const { return abbreviations[static_cast<std::size_t>(i)]; }

private:
    std::array<std::string, 12> abbreviations;
};

MonthAbbreviations::MonthAbbreviations(const std::locale& loc)
{
    for (int i = 0; i < 12; ++i)
    {
        std::tm tm;
        tm.tm_mon = i;
        std::wostringstream stream;
        stream.imbue(loc);
        stream << std::put_time(&tm, L"%b");
        abbreviations[i] = util::WideToUTF8(stream.str());
    }
}
#endif

inline bool
timeToLocal(const std::time_t& time, std::tm& localt)
{
#ifdef _WIN32
    return localtime_s(&localt, &time) == 0;
#else
    return localtime_r(&time, &localt) != nullptr;
#endif
}

inline bool
timeToUTC(const std::time_t& time, std::tm& utct)
{
#ifdef _WIN32
    return gmtime_s(&utct, &time) == 0;
#else
    return gmtime_r(&time, &utct) != nullptr;
#endif
}

} // end unnamed namespace

void
setLeapSeconds(celestia::util::array_view<LeapSecondRecord> leapSeconds)
{
    g_leapSeconds = leapSeconds;
}

Date::Date(int Y, int M, int D) :
    year(Y),
    month(M),
    day(D)
{
}

Date::Date(double jd)
{
    auto a = (std::int64_t) std::floor(jd + 0.5);
    wday = (a + 1) % 7;
    double c;
    if (a < 2299161)
    {
        c = (double) (a + 1524);
    }
    else
    {
        auto b = (double) ((std::int64_t) std::floor((a - 1867216.25) / 36524.25)); //NOSONAR
        c = a + b - (std::int64_t) std::floor(b / 4) + 1525; //NOSONAR
    }

    auto d = (std::int64_t) std::floor((c - 122.1) / 365.25);
    auto e = (std::int64_t) std::floor(365.25 * d); //NOSONAR
    auto f = (std::int64_t) std::floor((c - e) / 30.6001); //NOSONAR

    double dday = c - e - (std::int64_t) std::floor(30.6001 * f) + ((jd + 0.5) - a); //NOSONAR

    // This following used to be 14.0, but gcc was computing it incorrectly, so
    // it was changed to 14
    month = (int) (f - 1 - 12 * (std::int64_t) (f / 14)); //NOSONAR
    year = (int) (d - 4715 - (std::int64_t) ((7.0 + month) / 10.0));
    day = (int) dday;

    double dhour = (dday - day) * 24;
    hour = (int) dhour;

    double dminute = (dhour - hour) * 60;
    minute = (int) dminute;

    seconds = (dminute - minute) * 60;
    utc_offset = 0;
    tzname = "UTC";
}

std::string
Date::toString(const std::locale& loc, Format format) const
{
    if (format == ISO8601)
    {
        return fmt::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:08.5f}Z"sv,
                           year, month, day, hour, minute, seconds);
    }

    // MinGW's libraries don't have the tm_gmtoff and tm_zone fields for
    // struct tm.
#if defined(__GNUC__) && !defined(_WIN32)
    struct tm cal_time {};
    cal_time.tm_year = year-1900;
    cal_time.tm_mon = month-1;
    cal_time.tm_mday = day;
    cal_time.tm_hour = hour;
    cal_time.tm_min = minute;
    cal_time.tm_sec = (int)seconds;
    cal_time.tm_wday = wday;
    cal_time.tm_gmtoff = utc_offset;
#if defined(__APPLE__) || defined(__FreeBSD__)
    // tm_zone is a non-const string field on the Mac and FreeBSD (why?)
    cal_time.tm_zone = const_cast<char*>(tzname.c_str());
#else
    cal_time.tm_zone = tzname.c_str();
#endif

    switch(format)
    {
    case Locale:
        return fmt::format(loc, "{:%c}"sv, cal_time);
    case TZName:
        return fmt::format(loc, "{:%Y %b %d %H:%M:%S %Z}"sv, cal_time);
    default:
        return fmt::format(loc, "{:%Y %b %d %H:%M:%S %z}"sv, cal_time);
    }
#else
    static const MonthAbbreviations* monthAbbreviations = nullptr;
    if (monthAbbreviations == nullptr)
        monthAbbreviations = std::make_unique<MonthAbbreviations>(loc).release(); //NOSONAR
    switch(format)
    {
    case Locale:
    case TZName:
        return fmt::format("{:04} {} {:02} {:02}:{:02}:{:02} {}"sv,
                           year, (*monthAbbreviations)[month - 1], day,
                           hour, minute, static_cast<int>(seconds), tzname);
    default:
        {
            char sign = utc_offset < 0 ? '-' : '+';
            auto offsets = std::div(std::abs(utc_offset), 3600);
            return fmt::format("{:04} {} {:02} {:02}:{:02}:{:02} {}{:02}{:02}"sv,
                               year, (*monthAbbreviations)[month - 1], day,
                               hour, minute, static_cast<int>(seconds),
                               sign, offsets.quot, offsets.rem / 60);
        }
    }
#endif
}

// Convert a calendar date to a Julian date
Date::operator double() const
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

    return (std::floor(365.25 * y) +
            std::floor(30.6001 * (m + 1)) + B + 1720996.5 +
            day + hour / HOURS_PER_DAY + minute / MINUTES_PER_DAY + seconds / SECONDS_PER_DAY);
}

Date
Date::systemDate()
{
    time_t t = time(nullptr);
    tm gmt;

    Date d;
    if (timeToUTC(t, gmt))
    {
        d.year = gmt.tm_year + 1900;
        d.month = gmt.tm_mon + 1;
        d.day = gmt.tm_mday;
        d.hour = gmt.tm_hour;
        d.minute = gmt.tm_min;
        d.seconds = gmt.tm_sec;
    }

    return d;
}

// TODO: need option to parse UTC times (with leap seconds)
bool
parseDate(const std::string& s, Date& date)
{
    int year = 0;
    unsigned int month = 1;
    unsigned int day = 1;
    unsigned int hour = 0;
    unsigned int minute = 0;
    double second = 0.0;

    if (std::sscanf(s.c_str(), "%d-%u-%uT%u:%u:%lf",
                    &year, &month, &day, &hour, &minute, &second) != 6 &&
        std::sscanf(s.c_str(), " %d %u %u %u:%u:%lf ",
                    &year, &month, &day, &hour, &minute, &second) != 6 &&
        std::sscanf(s.c_str(), " %d %u %u %u:%u ",
                    &year, &month, &day, &hour, &minute) != 5 &&
        std::sscanf(s.c_str(), " %d %u %u ", &year, &month, &day) != 3)
    {
        return false;
    }

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

/********* Time scale conversion functions ***********/

// Convert from Atomic Time to UTC
Date
TAItoUTC(double tai)
{
    int dAT = g_leapSeconds[0].seconds;
    int extraSecs = 0;

    for (std::size_t i = g_leapSeconds.size() - 1; i > 0; i--) //NOSONAR
    {
        if (tai - secsToDays(g_leapSeconds[i].seconds) >= g_leapSeconds[i].t)
        {
            dAT = g_leapSeconds[i].seconds;
            break;
        }
        if (tai - secsToDays(g_leapSeconds[i - 1].seconds) >= g_leapSeconds[i].t)
        {
            dAT = g_leapSeconds[i].seconds;
            extraSecs = g_leapSeconds[i].seconds - g_leapSeconds[i - 1].seconds;
            break;
        }
    }

    Date utcDate(tai - secsToDays(dAT));
    utcDate.seconds += extraSecs;

    return utcDate;
}

// Convert from UTC to Atomic Time
double
UTCtoTAI(const Date& utc)
{
    double dAT = g_leapSeconds[0].seconds;
    auto utcjd = (double) Date(utc.year, utc.month, utc.day);

    for (std::size_t i = g_leapSeconds.size() - 1; i > 0; i--)
    {
        if (utcjd >= g_leapSeconds[i].t)
        {
            dAT = g_leapSeconds[i].seconds;
            break;
        }
    }

    double tai = utcjd + secsToDays(utc.hour * 3600.0 + utc.minute * 60.0 + utc.seconds + dAT);

    return tai;
}

// Convert from Terrestrial Time to Atomic Time
double
TTtoTAI(double tt)
{
    return tt - secsToDays(dTA);
}

// Convert from Atomic Time to Terrestrial TIme
double
TAItoTT(double tai)
{
    return tai + secsToDays(dTA);
}

// Input is a TDB Julian Date; result is in seconds
double
TDBcorrection(double tdb)
{
    // Correction for converting from Terrestrial Time to Barycentric Dynamical
    // Time. Constants and algorithm from "Time Routines in CSPICE",
    // http://sohowww.nascom.nasa.gov/solarsoft/stereo/gen/exe/icy/doc/time.req
    constexpr double K  = 1.657e-3;
    constexpr double EB = 1.671e-2;
    constexpr double M0 = 6.239996;
    constexpr double M1 = 1.99096871e-7;

    // t is seconds from J2000.0
    double t = daysToSecs(tdb - J2000);

    // Approximate calculation of Earth's mean anomaly
    double M = M0 + M1 * t;

    // Compute the eccentric anomaly
    double E = M + EB * std::sin(M);

    return K * std::sin(E);
}

// Convert from Terrestrial Time to Barycentric Dynamical Time
double
TTtoTDB(double tt)
{
    return tt + secsToDays(TDBcorrection(tt));
}

// Convert from Barycentric Dynamical Time to Terrestrial Time
double
TDBtoTT(double tdb)
{
    return tdb - secsToDays(TDBcorrection(tdb));
}

// Convert from Coordinated Universal time to Barycentric Dynamical Time
Date
TDBtoUTC(double tdb)
{
    return TAItoUTC(TTtoTAI(TDBtoTT(tdb)));
}

// Convert from Barycentric Dynamical Time to local calendar if possible
// otherwise convert to UTC
Date
TDBtoLocal(double tdb)
{
    double tai = TTtoTAI(TDBtoTT(tdb));

    double jdutc = TAItoJDUTC(tai);
    if (jdutc <= 2415733 || jdutc >= 2465442)
        return TDBtoUTC(tdb);

    auto time = static_cast<time_t>(julianDateToSeconds(jdutc - 2440587.5));
    struct tm localt;

    if (!timeToLocal(time, localt)) //NOSONAR
        return TDBtoUTC(tdb);

    Date d;
    d.year = localt.tm_year + 1900;
    d.month = localt.tm_mon + 1;
    d.day = localt.tm_mday;
    d.hour = localt.tm_hour;
    d.minute = localt.tm_min;
    d.seconds = localt.tm_sec;
    d.wday = localt.tm_wday;
#if defined(__GNUC__) && !defined(_WIN32)
    d.utc_offset = static_cast<int>(localt.tm_gmtoff);
    d.tzname = localt.tm_zone;
#else
    Date utcDate = TAItoUTC(tai);
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
    d.tzname = localt.tm_isdst ? _("DST"): _("STD");
#endif
    return d;
}

// Convert from Barycentric Dynamical Time to UTC
double
UTCtoTDB(const Date& utc)
{
    return TTtoTDB(TAItoTT(UTCtoTAI(utc)));
}

// Convert from TAI to Julian Date UTC. The Julian Date UTC functions should
// generally be avoided because there's no provision for dealing with leap
// seconds.
double
JDUTCtoTAI(double utc)
{
    double dAT = g_leapSeconds[0].seconds;

    for (std::size_t i = g_leapSeconds.size() - 1; i > 0; i--)
    {
        if (utc > g_leapSeconds[i].t)
        {
            dAT = g_leapSeconds[i].seconds;
            break;
        }
    }

    return utc + secsToDays(dAT);
}

// Convert from Julian Date UTC to TAI
double
TAItoJDUTC(double tai)
{
    double dAT = g_leapSeconds[0].seconds;

    for (std::size_t i = g_leapSeconds.size() - 1; i > 0; i--)
    {
        if (tai - secsToDays(g_leapSeconds[i - 1].seconds) > g_leapSeconds[i].t)
        {
            dAT = g_leapSeconds[i].seconds;
            break;
        }
    }

    return tai - secsToDays(dAT);
}

} // end namespace celestia::astro
