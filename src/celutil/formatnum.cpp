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
#include "formatnum.h"


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
        double m = pow(10, floor(log10(value)) - precision + 1);
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
    double value = num.getRoundedValue();

    if (num.flags & FormattedNumber::SignificantDigits)
    {
        int fmtPrecision = (int) log10(value) - num.precision + 1;
        if (value < 1.0)
            fmtPrecision--;
        sprintf(fmt, "%%.%df", fmtPrecision > 0 ? 0 : -fmtPrecision);
    }
    else
    {
        sprintf(fmt, "%%.%df", num.precision);
    }

    sprintf(buf, fmt, value);

    if (num.flags & FormattedNumber::GroupThousands)
    {
        int foundDecimal = (strchr(buf, '.') == NULL);
        int digitCount = 0;
        int i = strlen(buf);
        int j = sizeof(obuf) - 1;
        
        while (i >= 0)
        {
            if (foundDecimal)
            {
                if (isdigit(buf[i]))
                {
                    if (digitCount == 3)
                    {
                        obuf[j] = ',';
                        j--;
                        digitCount = 0;
                    }
                    digitCount++;
                }
            }

            obuf[j] = buf[i];

            if (buf[i] == '.')
                foundDecimal = true;

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
