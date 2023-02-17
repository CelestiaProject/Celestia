// formatnum.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <climits>
#include "formatnum.h"

// HACK: MS Visual C++ has _snprintf declared in stdio.h but not snprintf
#ifdef _WIN32
#include <celutil/winutil.h>
#define snprintf _snprintf
#endif

FormattedNumber::FormattedNumber(double v,
                                 unsigned int _precision,
                                 unsigned int _flags) :
    value(v),
    precision(_precision),
    flags(_flags)
{
}


double FormattedNumber::getValue() const
{
    return value;
}


double FormattedNumber::getRoundedValue() const
{
    if (flags & SignificantDigits)
    {
        if (value == 0.0)
            return 0.0;

        double m = pow(10.0, floor(log10(fabs(value))) - precision + 1);
        return floor(value / m + 0.5) * m;
    }
    else
    {
        return value;
    }
}


std::ostream& operator<<(std::ostream& out, const FormattedNumber& num)
{
    char fmt[32];
    char buf[32];
    char obuf[64];
    int fmtPrecision;
    double value = num.getRoundedValue();
    const char *grouping = localeconv()->grouping;
#ifndef _WIN32
    const char *decimal_point = localeconv()->decimal_point;
    const char *thousands_sep = localeconv()->thousands_sep;
#else
    static bool initialized = false;
    static char decimal_point[8] = {};
    static char thousands_sep[8] = {};

    if (!initialized)
    {
        std::string s = CurrentCPToUTF8(localeconv()->decimal_point);
        assert(s.length() < 8);
        strncpy(decimal_point, s.c_str(), sizeof(decimal_point) - 1);
        decimal_point[sizeof(decimal_point) - 1 ] = '\0';

        s = CurrentCPToUTF8(localeconv()->thousands_sep);
        assert(s.length() < 8);
        strncpy(thousands_sep, s.c_str(), sizeof(thousands_sep) - 1);
        thousands_sep[sizeof(thousands_sep) - 1] = '\0';

        initialized = true;
    }
#endif

    memset(obuf, 0, sizeof(obuf));

    if (num.flags & FormattedNumber::SignificantDigits)
    {
        if (value == 0.0)
        {
            fmtPrecision = 5;
        }
        else
        {
            fmtPrecision = (int) log10(fabs(value)) - num.precision + 1;
            if (fabs(value) < 1.0)
                fmtPrecision--;
            fmtPrecision = fmtPrecision > 0 ? 0 : -fmtPrecision;
        }
    }
    else
    {
        fmtPrecision = num.precision;
    }
    snprintf(fmt, sizeof(fmt)/sizeof(char), "%%.%df", fmtPrecision);

    snprintf(buf, sizeof(buf)/sizeof(char), fmt, value);

    if (num.flags & FormattedNumber::GroupThousands)
    {
        const char* decimalPosition = strstr(buf, decimal_point);
        int j = sizeof(obuf) - 1;
        int i = strlen(buf);
        int digitCount = 0;
        if (decimalPosition != nullptr)
        {
            int len = strlen(decimalPosition);
            j -= len;
            i -= len;
            memcpy(obuf + j, decimalPosition, len);
            --i;
            --j;
        }

        const char *g = grouping;
        bool does_grouping = *g != 0;
        while (i >= 0)
        {
            if (std::isdigit(static_cast<unsigned char>(buf[i])))
            {
                if (does_grouping && *g != CHAR_MAX)
                {
                    if (digitCount == *g)
                    {
                        const char *c, *ts = thousands_sep;
                        for (c = ts + strlen(ts) - 1; c >= ts; c--)
                        {
                            obuf[j] = *c;
                            j--;
                        }
                        if (*(g+1) != 0) g += 1;
                        digitCount = 0;
                    }
                }
                digitCount++;
            }

            obuf[j] = buf[i];

            j--;
            i--;
        }

        out << (obuf + (j + 1));
    }
    else
    {
        out << buf;
    }

    return out;
}
