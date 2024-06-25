#include <array>
#include <cmath>

#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celephem/orbit.h>
#include <celmath/mathlib.h>
#include <celutil/array_view.h>

#include <doctest.h>

namespace astro = celestia::astro;
namespace math = celestia::math;

namespace
{
constexpr double GMsun = 0.000296014912;
constexpr double fourpi2 = 4.0 * math::square(celestia::numbers::pi);
constexpr double tau = 2.0 * celestia::numbers::pi;

constexpr std::array testPeriods{
    50.0, 1200.0
};

constexpr std::array testInclinations{
    0.0, 30.0, 90.0, 150.0, 180.0
};

constexpr std::array testAngles{
    -180.0, -150.0, -90.0, -40.0, 0.0, 40.0, 90.0, 150.0, 180.0
};

constexpr std::array fixedZero{ 0.0 };

constexpr double tolerance = 1e-3;

bool ApproxAngle(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < tolerance ||
           std::abs(lhs + tau - rhs) < tolerance ||
           std::abs(lhs - tau - rhs) < tolerance;
}

void TestElements(const astro::KeplerElements& expected,
                  const astro::KeplerElements& actual)
{
    REQUIRE(expected.period == doctest::Approx(actual.period).epsilon(tolerance));
    REQUIRE(expected.semimajorAxis == doctest::Approx(actual.semimajorAxis).epsilon(tolerance));
    REQUIRE(expected.eccentricity == doctest::Approx(actual.eccentricity).epsilon(tolerance));
    REQUIRE(ApproxAngle(expected.inclination, actual.inclination));
    REQUIRE(ApproxAngle(expected.longAscendingNode, actual.longAscendingNode));
    REQUIRE(ApproxAngle(expected.argPericenter, actual.argPericenter));
    REQUIRE(ApproxAngle(expected.meanAnomaly, actual.meanAnomaly));
}

}

TEST_SUITE_BEGIN("Keplerian orbits");

TEST_CASE("Elliptical orbits")
{
    constexpr std::array testEccentricities{ 0.0, 0.2, 0.6 };

    for (double period : testPeriods)
    {
        double semimajor = std::cbrt(GMsun * math::square(period) / fourpi2);
        for (double meanAnomalyDeg : testAngles)
        for (double inclinationDeg : testInclinations)
        {
            auto testNodes = inclinationDeg == 0.0 || inclinationDeg == 180.0
                ? celestia::util::array_view<double>(fixedZero)
                : celestia::util::array_view<double>(testAngles);
            for (double nodeDeg : testNodes)
            for (double eccentricity : testEccentricities)
            {
                auto testPericenters = eccentricity == 0.0
                    ? celestia::util::array_view<double>(fixedZero)
                    : celestia::util::array_view<double>(testAngles);
                for (double pericenterDeg : testPericenters)
                {
                    astro::KeplerElements expected;
                    expected.period = period;
                    expected.semimajorAxis = semimajor;
                    expected.eccentricity = eccentricity;
                    expected.inclination = math::degToRad(inclinationDeg);
                    expected.longAscendingNode = math::degToRad(nodeDeg);
                    expected.argPericenter = math::degToRad(pericenterDeg);
                    expected.meanAnomaly = math::degToRad(meanAnomalyDeg);
                    auto orbit = celestia::ephem::EllipticalOrbit(expected, 0.0);
                    auto position = orbit.positionAtTime(0.0);
                    auto velocity = orbit.velocityAtTime(0.0);

                    auto actual = astro::StateVectorToElements(position, velocity, GMsun);

                    TestElements(expected, actual);
                }
            }
        }
    }
}

TEST_CASE("Hyperbolic orbits")
{
    constexpr std::array testEccentricities{ 1.5, 2.4 };

    for (double period : testPeriods)
    {
        double semimajor = -std::cbrt(GMsun * math::square(period) / fourpi2);
        for (double meanAnomalyDeg : testAngles)
        for (double inclinationDeg : testInclinations)
        {
            auto testNodes = inclinationDeg == 0.0 || inclinationDeg == 180.0
                ? celestia::util::array_view<double>(fixedZero)
                : celestia::util::array_view<double>(testAngles);
            for (double nodeDeg : testNodes)
            for (double eccentricity : testEccentricities)
            for (double pericenterDeg : testAngles)
            {
                astro::KeplerElements expected;
                expected.period = period;
                expected.semimajorAxis = semimajor;
                expected.eccentricity = eccentricity;
                expected.inclination = math::degToRad(inclinationDeg);
                expected.longAscendingNode = math::degToRad(nodeDeg);
                expected.argPericenter = math::degToRad(pericenterDeg);
                expected.meanAnomaly = math::degToRad(meanAnomalyDeg);
                auto orbit = celestia::ephem::HyperbolicOrbit(expected, 0.0);
                auto position = orbit.positionAtTime(0.0);
                auto velocity = orbit.velocityAtTime(0.0);

                auto actual = astro::StateVectorToElements(position, velocity, GMsun);

                TestElements(expected, actual);
            }
        }
    }
}

TEST_SUITE_END();
