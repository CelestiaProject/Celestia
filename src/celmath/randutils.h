#pragma once

#include <cmath>
#include <random>

#include <Eigen/Core>

#include "mathlib.h"

namespace celmath
{
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

template<typename T, typename RNG = std::mt19937>
Eigen::Matrix<T, 2, 1> randomOnCircle(RNG&& rng)
{
    using std::cos, std::sin;
    T phi = RealDists<T>::SignedFullAngle(rng);
    return Eigen::Matrix<T, 2, 1>{cos(phi), sin(phi)};
}

template<typename T, typename RNG = std::mt19937>
Eigen::Matrix<T, 3, 1> randomOnSphere(RNG&& rng)
{
    using std::cos, std::sin, std::sqrt;
    T phi = RealDists<T>::SignedFullAngle(rng);
    T cosTheta = RealDists<T>::SignedUnit(rng);
    T xyScale = sqrt(static_cast<T>(1) - square(cosTheta));
    return Eigen::Matrix<T, 3, 1>{xyScale * cos(phi),
                                  xyScale * sin(phi),
                                  cosTheta};
}

std::mt19937& getRNG();
}
