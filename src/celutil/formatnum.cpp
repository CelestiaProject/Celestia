// formatnum.cpp
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <climits>
#include "formatnum.h"

// HACK: MS Visual C++ has _snprintf declared in stdio.h but not snprintf
#ifdef _WIN32
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
        {
            return 0.0;
        }
        else
        {
            double m = pow(10.0, floor(log10(fabs(value))) - precision + 1);
            return floor(value / m + 0.5) * m;
        }
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
    double value = num.getRoundedValue();
    char *decimal_point = localeconv()->decimal_point;
    char *thousands_sep = localeconv()->thousands_sep;
    char *grouping = localeconv()->grouping;

    memset(obuf, 0, sizeof(obuf));

    if (num.flags & FormattedNumber::SignificantDigits)
    {
        if (value == 0.0)
        {
            snprintf(fmt, sizeof(fmt)/sizeof(char), "%%.%df", 5);
        }
        else
        {
            int fmtPrecision = (int) log10(fabs(value)) - num.precision + 1;
            if (fabs(value) < 1.0)
                fmtPrecision--;
            snprintf(fmt, sizeof(fmt)/sizeof(char), "%%.%df", fmtPrecision > 0 ? 0 : -fmtPrecision);
        }
    }
    else
    {
        snprintf(fmt, sizeof(fmt)/sizeof(char), "%%.%df", num.precision);
    }

    snprintf(buf, sizeof(buf)/sizeof(char), fmt, value);

    if (num.flags & FormattedNumber::GroupThousands)
    {
        const char* decimalPosition = strstr(buf, decimal_point);
        int j = sizeof(obuf) - 1;
        int i = strlen(buf);
        int digitCount = 0;
        if (decimalPosition != NULL)
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
            if (isdigit(buf[i]))
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
