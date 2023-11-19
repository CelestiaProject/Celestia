// units.h
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <optional>

namespace celestia::astro
{

enum class LengthUnit : std::uint8_t
{
    Default = 0,
    Kilometer,
    Meter,
    EarthRadius,
    JupiterRadius,
    SolarRadius,
    AstronomicalUnit,
    LightYear,
    Parsec,
    Kiloparsec,
    Megaparsec,
};

enum class TimeUnit : std::uint8_t
{
    Default = 0,
    Second,
    Minute,
    Hour,
    Day,
    JulianYear,
};

enum class AngleUnit : std::uint8_t
{
    Default = 0,
    Milliarcsecond,
    Arcsecond,
    Arcminute,
    Degree,
    Hour,
    Radian,
};

enum class MassUnit : std::uint8_t
{
    Default = 0,
    Kilogram,
    EarthMass,
    JupiterMass,
};

std::optional<double> getLengthScale(LengthUnit unit);
std::optional<double> getTimeScale(TimeUnit unit);
std::optional<double> getAngleScale(AngleUnit unit);
std::optional<double> getMassScale(MassUnit unit);

} // end namespace celestia::astro
