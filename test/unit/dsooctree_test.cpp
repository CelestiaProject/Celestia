// Copyright (C) 2026 Celestia Development Team. All rights reserved.
// Licensed under the GNU General Public License, version 2 or later.

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>

#include <doctest.h>

#include <celastro/astro.h>
#include <celengine/dsooctree.h>
#include <celengine/dsorenderer.h>
#include <celengine/galaxy.h>

namespace
{

struct DSOHandler : celestia::engine::DSOHandler
{
    unsigned int count{};
    float irradiance{};
    double distance{};

    void process(const std::unique_ptr<DeepSkyObject>&, double ly, float brightness) override
    {
        ++count;
        distance = ly;
        irradiance = brightness;
    }
};

using Processor = celestia::engine::DSOOctreeVisibleObjectsProcessor;

std::array<Processor::PlaneType, 5> visiblePlanes()
{
    std::array<Processor::PlaneType, 5> planes;
    planes.fill(Processor::PlaneType(Eigen::Vector3d::UnitZ(), 1.0e20));
    return planes;
}

} // namespace

TEST_SUITE_BEGIN("DSO rendering");

TEST_CASE("DSO culling retains the flux boundary and supplies irradiance to handlers")
{
    namespace astro = celestia::astro;
    auto planes = visiblePlanes();
    DSOHandler handler;
    std::unique_ptr<DeepSkyObject> object = std::make_unique<Galaxy>();
    object->setRadius(2.0f);
    object->setAbsoluteMagnitude(-10.0f);
    object->setPosition(Eigen::Vector3d(2.0 * astro::LY_PER_10PARSEC + 2.0, 0, 0));
    double distance = object->getPosition().norm() - object->getBoundingSphereRadius();
    float threshold = object->getApparentIrradiance(distance);
    Processor processor(&handler, Eigen::Vector3d::Zero(), planes, threshold);
    REQUIRE(processor.checkNode(object->getPosition(), 2.0, object->getAbsoluteMagnitude()));
    processor.process(object);
    CHECK(handler.count == 1);
    CHECK(handler.irradiance == threshold);
    CHECK(handler.distance == distance);

    Processor stricter(&handler, Eigen::Vector3d::Zero(), planes, std::nextafter(threshold, std::numeric_limits<float>::infinity()));
    stricter.process(object);
    CHECK(handler.count == 1);

    celestia::engine::DSOOctreeCloseObjectsProcessor close(&handler, Eigen::Vector3d::Zero(), 100.0);
    close.process(object);
    CHECK(handler.count == 2);
    CHECK(handler.irradiance == threshold);
}

TEST_CASE("DSO visibility retains the close-range cap and uncalibrated catalog objects")
{
    namespace astro = celestia::astro;
    auto planes = visiblePlanes();
    DSOHandler handler;
    std::unique_ptr<DeepSkyObject> object = std::make_unique<Galaxy>();
    object->setAbsoluteMagnitude(5.0f);
    object->setRadius(10.0f);
    object->setPosition(Eigen::Vector3d(5.0, 0, 0));
    float reference = object->getApparentIrradiance(astro::LY_PER_10PARSEC);
    CHECK(object->getApparentIrradiance(-5.0) == reference);
    CHECK(object->getApparentIrradiance(1.0) == reference);
    Processor processor(&handler, Eigen::Vector3d::Zero(), planes, reference);
    REQUIRE(processor.checkNode(object->getPosition(), 10.0, 5.0f));
    processor.process(object);
    CHECK(handler.count == 1);
    CHECK(handler.irradiance == reference);

    object->setAbsoluteMagnitude(DSO_DEFAULT_ABS_MAGNITUDE);
    REQUIRE(processor.checkNode(object->getPosition(), 10.0, DSO_DEFAULT_ABS_MAGNITUDE));
    processor.process(object);
    CHECK(handler.count == 2);
    CHECK(std::isinf(handler.irradiance));
}

TEST_CASE("Logarithmic DSO flux response preserves the legacy display curve")
{
    namespace astro = celestia::astro;
    using celestia::engine::detail::dsoDisplayBrightness;
    using celestia::engine::detail::dsoIrradianceScale;
    constexpr float average = -19.04f;
    constexpr float faintest = 8.0f;
    for (float magnitude : { -19.04f, -7.5f, DSO_DEFAULT_ABS_MAGNITUDE })
    {
        Galaxy object;
        object.setAbsoluteMagnitude(magnitude);
        for (double distance : { -10.0, 0.0, astro::LY_PER_10PARSEC, 1000.0, 1.0e7 })
        {
            CAPTURE(magnitude);
            CAPTURE(distance);
            float apparent = distance >= astro::LY_PER_10PARSEC
                ? astro::absToAppMag(magnitude, distance)
                : magnitude + static_cast<float>(4.0 * std::tanh(distance / astro::LY_PER_10PARSEC - 1.0));
            float r = magnitude / average;
            float expected = std::max(0.0f, (r - (r - 0.2f) * (magnitude - apparent) / (magnitude - faintest)) * 1.7f);
            float actual = dsoDisplayBrightness(object.getLogReferenceIrradiance(), dsoIrradianceScale(distance),
                                                -static_cast<double>(average) / astro::LN_MAG,
                                                -static_cast<double>(faintest) / astro::LN_MAG, 1.7f);
            CHECK(std::isfinite(actual));
            CHECK(actual == doctest::Approx(expected).epsilon(0.0001));
        }
    }
}

TEST_SUITE_END();
