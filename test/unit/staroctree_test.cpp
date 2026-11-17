// Copyright (C) 2026 Celestia Development Team. All rights reserved.
// Licensed under the GNU General Public License, version 2 or later.

#include <array>
#include <limits>

#include <doctest.h>

#include <celastro/astro.h>
#include <celengine/staroctree.h>

namespace
{

struct Handler : celestia::engine::StarHandler
{
    unsigned int count{};
    void process(const Star&, float, float) override { ++count; }
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
    Processor processor{ &handler, Eigen::Vector3f::Zero(), planes, 4.0f };
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
    Fixture::Processor processor{ &fixture.handler, Eigen::Vector3f::Zero(), fixture.planes, -5.0f };
    REQUIRE(star.getApparentMagnitude(star.getPosition().norm()) == -5.0f);
    REQUIRE(processor.checkNode(star.getPosition(), 0.0f, 0.0f));
    processor.process(star);
    CHECK(fixture.handler.count == 1);
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
        REQUIRE(star.getApparentMagnitude(distance) <= 4.0f);
        fixture.processor.process(star);
        CHECK(fixture.handler.count == 1);
    }
}

TEST_SUITE_END();
