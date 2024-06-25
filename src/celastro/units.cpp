// units.cpp
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "units.h"

#include <celcompat/numbers.h>
#include "astro.h"
#include "date.h"

namespace celestia::astro
{

// Get scale of given length unit in kilometers
std::optional<double>
getLengthScale(LengthUnit unit)
{
    switch (unit)
    {
    case LengthUnit::Kilometer: return 1.0;
    case LengthUnit::Meter: return 1e-3;
    case LengthUnit::EarthRadius: return EARTH_RADIUS<double>;
    case LengthUnit::JupiterRadius: return JUPITER_RADIUS<double>;
    case LengthUnit::SolarRadius: return SOLAR_RADIUS<double>;
    case LengthUnit::AstronomicalUnit: return KM_PER_AU<double>;
    case LengthUnit::LightYear: return KM_PER_LY<double>;
    case LengthUnit::Parsec: return KM_PER_PARSEC<double>;
    case LengthUnit::Kiloparsec: return 1e3 * KM_PER_PARSEC<double>;
    case LengthUnit::Megaparsec: return 1e6 * KM_PER_PARSEC<double>;
    default: return std::nullopt;
    }
}


// Get scale of given time unit in days
std::optional<double>
getTimeScale(TimeUnit unit)
{
    switch (unit)
    {
    case TimeUnit::Second: return 1.0 / SECONDS_PER_DAY;
    case TimeUnit::Minute: return 1.0 / MINUTES_PER_DAY;
    case TimeUnit::Hour: return 1.0 / HOURS_PER_DAY;
    case TimeUnit::Day: return 1.0;
    case TimeUnit::JulianYear: return DAYS_PER_YEAR;
    default: return std::nullopt;
    }
}


// Get scale of given angle unit in degrees
std::optional<double>
getAngleScale(AngleUnit unit)
{
    switch (unit)
    {
    case AngleUnit::Milliarcsecond: return 1e-3 / SECONDS_PER_DEG;
    case AngleUnit::Arcsecond: return 1.0 / SECONDS_PER_DEG;
    case AngleUnit::Arcminute: return 1.0 / MINUTES_PER_DEG;
    case AngleUnit::Degree: return 1.0;
    case AngleUnit::Hour: return DEG_PER_HRA;
    case AngleUnit::Radian: return 180.0 / celestia::numbers::pi;
    default: return std::nullopt;
    }
}

std::optional<double>
getMassScale(MassUnit unit)
{
    switch (unit)
    {
    case MassUnit::Kilogram: return 1.0 / EarthMass;
    case MassUnit::EarthMass: return 1.0;
    case MassUnit::JupiterMass: return JupiterMass / EarthMass;
    default: return std::nullopt;
    }
}

} // end namespace celestia::astro
