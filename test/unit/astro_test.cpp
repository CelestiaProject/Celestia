// astro_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <limits>

#include <doctest.h>

#include <celastro/astro.h>

namespace astro = celestia::astro;

TEST_SUITE_BEGIN("Irradiance");

TEST_CASE("Distant irradiance follows inverse-square falloff without overflowing")
{
    float distance = astro::lightYearsToKilometers(1.0f);
    float reference = astro::lumToIrradiance(1.0f, distance);
    for (float scale : { 1000.0f, 10000.0f })
    {
        CAPTURE(scale);
        float actual = astro::lumToIrradiance(1.0f, distance * scale);
        CHECK(actual > 0.0f);
        CHECK(actual * scale * scale == doctest::Approx(reference));
    }
}

TEST_CASE("Finite source and distance extremes retain representable irradiance")
{
    float distance = astro::lightYearsToKilometers(1.0f);
    float reference = astro::lumToIrradiance(1.0f, distance);
    struct Sample
    {
        float luminosity;
        float distance;
    };
    const Sample samples[] = {
        { 1.0e20f, distance },
        { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() },
    };
    for (const auto& sample : samples)
    {
        CAPTURE(sample.luminosity);
        CAPTURE(sample.distance);
        float actual = astro::lumToIrradiance(sample.luminosity, sample.distance);
        double distanceRatio = static_cast<double>(sample.distance) / distance;
        CHECK(std::isfinite(actual));
        CHECK(actual > 0.0f);
        CHECK(actual / static_cast<double>(sample.luminosity) * distanceRatio * distanceRatio
              == doctest::Approx(reference));
    }
}

TEST_CASE("Absolute magnitude conversion preserves representable irradiance beyond float luminosity range")
{
    struct Sample
    {
        float magnitude;
        float distance;
    };
    const Sample samples[] = {
        { -100.0f, 10000.0f },
        { 120.0f, 0.000001f },
    };
    for (const auto& sample : samples)
    {
        CAPTURE(sample.magnitude);
        CAPTURE(sample.distance);
        float expected = astro::magToIrradiance(astro::absToAppMag(sample.magnitude, sample.distance));
        float actual = astro::absMagToIrradiance(sample.magnitude, astro::lightYearsToKilometers(sample.distance));
        REQUIRE(std::isfinite(expected));
        REQUIRE(expected > 0.0f);
        REQUIRE(std::isfinite(actual));
        CHECK(actual / expected == doctest::Approx(1.0f));
    }
}

TEST_SUITE_END();
