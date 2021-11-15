#pragma once

#include <cmath>
#include <cstdint>
#include <random>

#include <Eigen/Core>

#include "mathlib.h"

namespace celmath
{

// Small noncryptographic pseudorandom number generator by Bob Jenkins
// http://burtleburtle.net/bob/rand/talksmall.html
class Jsf32
{
public:
    using result_type = std::uint32_t;
    Jsf32(std::uint32_t seed);

    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT32_MAX; }

    result_type operator()();

private:
    std::uint32_t a;
    std::uint32_t b;
    std::uint32_t c;
    std::uint32_t d;
};

extern float turbulence(const Eigen::Vector2f& p, float freq);
extern float turbulence(const Eigen::Vector3f& p, float freq);
extern float fractalsum(const Eigen::Vector2f& p, float freq);
extern float fractalsum(const Eigen::Vector3f& p, float freq);

extern float noise(float arg);
extern float noise(const Eigen::Vector2f& arg);
extern float noise(const Eigen::Vector3f& arg);

template<typename T>
struct RealDists
{
    static std::uniform_real_distribution<T> Unit;
    static std::uniform_real_distribution<T> SignedUnit;
    static std::uniform_real_distribution<T> SignedFullAngle;
};

template<typename T>
std::uniform_real_distribution<T>
RealDists<T>::Unit{static_cast<T>(0), static_cast<T>(1)};

template<typename T>
std::uniform_real_distribution<T>
RealDists<T>::SignedUnit{static_cast<T>(-1), static_cast<T>(1)};

template<typename T>
std::uniform_real_distribution<T>
RealDists<T>::SignedFullAngle{static_cast<T>(-PI), static_cast<T>(PI)};

template<typename T, typename RNG = Jsf32>
Eigen::Matrix<T, 2, 1> randomOnCircle(RNG&& rng)
{
    using std::cos;
    using std::sin;
    T phi = RealDists<T>::SignedFullAngle(rng);
    return Eigen::Matrix<T, 2, 1>{cos(phi), sin(phi)};
}

template<typename T, typename RNG = Jsf32>
Eigen::Matrix<T, 3, 1> randomOnSphere(RNG&& rng)
{
    using std::cos;
    using std::sin;
    using std::sqrt;
    T phi = RealDists<T>::SignedFullAngle(rng);
    T cosTheta = RealDists<T>::SignedUnit(rng);
    T xyScale = sqrt(static_cast<T>(1) - square(cosTheta));
    return Eigen::Matrix<T, 3, 1>{xyScale * cos(phi),
                                  xyScale * sin(phi),
                                  cosTheta};
}
}
