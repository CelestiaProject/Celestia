// Copyright (C) 2026 Celestia Development Team. All rights reserved.
// Licensed under the GNU General Public License, version 2 or later.

#include <array>
#include <cmath>
#include <limits>

#include <doctest.h>

#include <celastro/astro.h>
#include <celengine/staroctree.h>

namespace
{

struct Handler : celestia::engine::StarHandler
{
    unsigned int count{};
    float irradiance{};
    void process(const Star&, float, float brightness) override
    {
        ++count;
        irradiance = brightness;
    }
};

struct Fixture
{
    using Processor = celestia::engine::StarOctreeVisibleObjectsProcessor;
    std::array<Processor::PlaneType, 5> planes{
        Processor::PlaneType(Eigen::Vector3f::UnitZ(), 1000000.0f),
        Processor::PlaneType(Eigen::Vector3f::UnitZ(), 1000000.0f),
        Processor::PlaneType(Eigen::Vector3f::UnitZ(), 1000000.0f),
        Processor::PlaneType(Eigen::Vector3f::UnitZ(), 1000000.0f),
        Processor::PlaneType(Eigen::Vector3f::UnitZ(), 1000000.0f),
    };
    Handler handler;
    const float minimumIrradiance{ celestia::astro::magToIrradiance(4.0f) };
    Processor processor{ &handler, Eigen::Vector3f::Zero(), planes, minimumIrradiance };
};

} // namespace

TEST_SUITE_BEGIN("Star culling");

TEST_CASE("Magnitude prefilter retains stars rounded onto the visibility boundary")
{
    Fixture fixture;
    Star star;
    star.setPosition(Eigen::Vector3f(celestia::astro::LY_PER_PARSEC<float>, 0, 0));
    // At one parsec, this positive magnitude rounds onto -5 even with excess precision.
    star.setAbsoluteMagnitude(std::numeric_limits<float>::min());
    float distance = star.getPosition().norm();
    float minimumIrradiance = celestia::astro::absMagToIrradiance(0.0f, celestia::astro::lightYearsToKilometers(distance));
    Fixture::Processor processor{ &fixture.handler, Eigen::Vector3f::Zero(), fixture.planes, minimumIrradiance };
    REQUIRE(star.getApparentMagnitude(star.getPosition().norm()) == -5.0f);
    REQUIRE(star.getApparentIrradiance(distance) == minimumIrradiance);
    REQUIRE(processor.checkNode(star.getPosition(), 0.0f, 0.0f));
    processor.process(star);
    CHECK(fixture.handler.count == 1);
    CHECK(fixture.handler.irradiance == minimumIrradiance);
    star.setAbsoluteMagnitude(100.0f);
    processor.process(star);
    CHECK(fixture.handler.count == 1);
}

TEST_CASE("Magnitude prefilter does not discard extinction-brightened stars")
{
    for (float distance : { 0.5f, 10.0f })
    {
        CAPTURE(distance);
        Fixture fixture;
        Star star;
        star.setPosition(Eigen::Vector3f(distance, 0, 0));
        star.setAbsoluteMagnitude(20.0f);
        star.setExtinction(-100.0f);
        REQUIRE(fixture.processor.checkNode(star.getPosition(), 0.01f, 0.0f));
        float irradiance = star.getApparentIrradiance(distance);
        REQUIRE(irradiance >= fixture.minimumIrradiance);
        fixture.processor.process(star);
        CHECK(fixture.handler.count == 1);
        CHECK(fixture.handler.irradiance == irradiance);
    }
}

TEST_CASE("Extinction corrections preserve representable stellar irradiance")
{
    struct Sample
    {
        float absoluteMagnitude;
        float extinction;
        float distance;
    };
    for (const auto& sample : std::array{
             Sample{ 20.0f, -10.0f, 10.0f },
             Sample{ -80.0f, 12.0f, 10.0f },
             Sample{ 120.0f, -12.0f, 10.0f },
             Sample{ 116.0f, -35.0f, celestia::astro::LY_PER_PARSEC<float> },
             Sample{ -100.0f, 0.0f, 10000.0f },
             Sample{ -100.0f, 0.001f, 10000.0f },
             Sample{ -100.0f, 12.0f, 10.0f },
         })
    {
        CAPTURE(sample.absoluteMagnitude);
        CAPTURE(sample.extinction);
        CAPTURE(sample.distance);
        Star star;
        star.setAbsoluteMagnitude(sample.absoluteMagnitude);
        star.setExtinction(sample.extinction);
        float expected = celestia::astro::magToIrradiance(star.getApparentMagnitude(sample.distance));
        float actual = star.getApparentIrradiance(sample.distance);
        REQUIRE(std::isfinite(expected));
        REQUIRE(expected > 0.0f);
        REQUIRE(std::isfinite(actual));
        CHECK(actual / expected == doctest::Approx(1.0f));
    }
}

TEST_CASE("Positive extinction preserves conservative irradiance culling")
{
    for (float magnitude : { 0.0f, celestia::astro::SOLAR_ABSMAG, 20.0f })
    {
        CAPTURE(magnitude);
        for (float distance : { 0.5f, celestia::astro::LY_PER_PARSEC<float>, 10.0f })
        {
            CAPTURE(distance);
            Star star;
            star.setPosition(Eigen::Vector3f(distance, 0, 0));
            star.setAbsoluteMagnitude(magnitude);
            float unextincted = star.getApparentIrradiance(distance);
            for (float extinction : { std::numeric_limits<float>::min(), 0.0001f, 0.1f })
            {
                CAPTURE(extinction);
                Fixture fixture;
                star.setExtinction(extinction);
                float irradiance = star.getApparentIrradiance(distance);
                REQUIRE(std::isfinite(irradiance));
                REQUIRE(irradiance > 0.0f);
                CHECK(irradiance <= unextincted);
                Fixture::Processor processor{ &fixture.handler, Eigen::Vector3f::Zero(), fixture.planes, irradiance };
                REQUIRE(processor.checkNode(star.getPosition(), 0.0f, magnitude));
                processor.process(star);
                CHECK(fixture.handler.count == 1);
                CHECK(fixture.handler.irradiance == irradiance);
            }
        }
    }
}

TEST_SUITE_END();
