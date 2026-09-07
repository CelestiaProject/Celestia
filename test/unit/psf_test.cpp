// psf_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cmath>
#include <limits>

#include <doctest.h>

#include <celengine/pointstarrenderer.h>

using celestia::engine::detail::psfGlowOnset;
using celestia::engine::detail::psfPointFade;
using celestia::engine::detail::psfReflectiveGlowOnset;

TEST_SUITE_BEGIN("PSF");

TEST_CASE("PSF glow fades in between peak radiance one and two")
{
    CHECK(psfGlowOnset(0.0f) == 0.0f);
    CHECK(psfGlowOnset(0.5f) == 0.0f);
    CHECK(psfGlowOnset(1.0f) == 0.0f);
    CHECK(psfGlowOnset(1.25f) == doctest::Approx(0.15625f));
    CHECK(psfGlowOnset(1.5f) == doctest::Approx(0.5f));
    CHECK(psfGlowOnset(1.75f) == doctest::Approx(0.84375f));
    CHECK(psfGlowOnset(2.0f) == 1.0f);
    CHECK(psfGlowOnset(100.0f) == 1.0f);
    CHECK(psfGlowOnset(std::numeric_limits<float>::max()) == 1.0f);
}

TEST_CASE("PSF glow onset is continuous at both thresholds")
{
    float aboveOne = psfGlowOnset(std::nextafter(1.0f, 2.0f));
    CHECK(aboveOne > 0.0f);
    CHECK(aboveOne < 1.0e-6f);
    CHECK(psfGlowOnset(std::nextafter(1.0f, 0.0f)) == 0.0f);

    CHECK(psfGlowOnset(std::nextafter(2.0f, 1.0f)) > 1.0f - 1.0e-6f);
    CHECK(psfGlowOnset(std::nextafter(2.0f, 3.0f)) == 1.0f);
}

TEST_CASE("PSF glow onset is bounded and monotonic")
{
    float previous = 0.0f;
    for (int i = 0; i <= 300; ++i)
    {
        float alpha = psfGlowOnset(static_cast<float>(i) / 100.0f);
        CHECK(alpha >= previous);
        CHECK(alpha >= 0.0f);
        CHECK(alpha <= 1.0f);
        previous = alpha;
    }
}

TEST_CASE("PSF point fades out as the disc resolves")
{
    CHECK(psfPointFade(0.0f, 1.5f, 1.0f) == 1.0f);
    CHECK(psfPointFade(0.5f, 1.5f, 1.0f) == 1.0f);
    CHECK(psfPointFade(1.0f, 1.5f, 1.0f) == 1.0f);
    CHECK(psfPointFade(1.25f, 1.5f, 1.0f) == doctest::Approx(0.84375f));
    CHECK(psfPointFade(1.5f, 1.5f, 1.0f) == doctest::Approx(0.5f));
    CHECK(psfPointFade(1.75f, 1.5f, 1.0f) == doctest::Approx(0.15625f));
    CHECK(psfPointFade(2.0f, 1.5f, 1.0f) == 0.0f);
    CHECK(psfPointFade(100.0f, 1.5f, 1.0f) == 0.0f);
}

TEST_CASE("PSF reflective glow fades smoothly at the limb overflow threshold")
{
    CHECK(psfReflectiveGlowOnset(10.0f, 0.0f) == 1.0f);
    for (float linkedPeak : { 0.1f, 1.0f, 10.0f, 1000.0f })
    {
        CAPTURE(linkedPeak);
        CHECK(psfReflectiveGlowOnset(0.0f, linkedPeak) == 0.0f);
        CHECK(psfReflectiveGlowOnset(linkedPeak, linkedPeak) == 0.0f);
        CHECK(psfReflectiveGlowOnset(1.5f * linkedPeak, linkedPeak) == doctest::Approx(0.5f));
        CHECK(psfReflectiveGlowOnset(2.0f * linkedPeak, linkedPeak) == 1.0f);
        CHECK(psfReflectiveGlowOnset(10.0f * linkedPeak, linkedPeak) == 1.0f);
        CHECK(psfReflectiveGlowOnset(std::nextafter(linkedPeak, 0.0f), linkedPeak) == 0.0f);
        CHECK(psfReflectiveGlowOnset(std::nextafter(linkedPeak, 2.0f * linkedPeak), linkedPeak) < 1.0e-6f);
        CHECK(psfReflectiveGlowOnset(std::nextafter(2.0f * linkedPeak, linkedPeak), linkedPeak) > 1.0f - 1.0e-6f);

        float previous = 0.0f;
        for (int i = 0; i <= 300; ++i)
        {
            float alpha = psfReflectiveGlowOnset(linkedPeak * (static_cast<float>(i) / 100.0f), linkedPeak);
            CHECK(alpha >= previous);
            CHECK(alpha >= 0.0f);
            CHECK(alpha <= 1.0f);
            previous = alpha;
        }
    }
}

TEST_CASE("PSF point transition follows the DPI-scaled cone radius")
{
    CHECK(psfPointFade(1.5f, 1.0f, 1.0f) == doctest::Approx(0.5f));
    CHECK(psfPointFade(1.5f, 1.0f, 0.5f) == doctest::Approx(0.5f));
    CHECK(psfPointFade(2.0f, 1.5f, 2.0f) == doctest::Approx(0.5f));
    CHECK(psfPointFade(2.0f, 3.0f, 1.0f) == doctest::Approx(0.5f));
    CHECK(psfPointFade(3.0f, 1.5f, 2.0f) == 0.0f);
    CHECK(psfPointFade(10.5f, 10.0f, 2.0f) == doctest::Approx(0.5f));
    CHECK(psfPointFade(20.0f, 10.0f, 2.0f) == 0.0f);
}

TEST_CASE("PSF point fade is continuous and monotonic across radii and DPI scales")
{
    for (float radius : { 1.0f, 1.5f, 10.0f })
    {
        for (float scale : { 0.5f, 1.0f, 2.0f, 4.0f })
        {
            CAPTURE(radius);
            CAPTURE(scale);
            float fadeEnd = std::max(2.0f, radius * scale);
            CHECK(psfPointFade(1.0f, radius, scale) == 1.0f);
            CHECK(psfPointFade(std::nextafter(1.0f, 2.0f), radius, scale) > 1.0f - 1.0e-6f);
            CHECK(psfPointFade(std::nextafter(fadeEnd, 1.0f), radius, scale) < 1.0e-6f);
            CHECK(psfPointFade(fadeEnd, radius, scale) == 0.0f);

            float previous = 1.0f;
            for (int i = 0; i <= 200; ++i)
            {
                float discRadius = static_cast<float>(i) * fadeEnd / 100.0f;
                float fade = psfPointFade(discRadius, radius, scale);
                CHECK(fade <= previous);
                CHECK(fade >= 0.0f);
                CHECK(fade <= 1.0f);
                previous = fade;
            }
        }
    }
}

TEST_SUITE_END();
