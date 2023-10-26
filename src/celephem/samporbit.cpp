// samporbit.cpp
//
// Copyright (C) 2002-2009, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Trajectories based on unevenly spaced cartesian positions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "samporbit.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <istream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <Eigen/Core>

#include <celcompat/bit.h>
#include <celengine/astro.h>
#include <celmath/mathlib.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "orbit.h"
#include "sampfile.h"
#include "xyzvbinary.h"

using celestia::util::GetLogger;

namespace celestia::ephem
{

namespace
{

struct InterpolationParameters
{
    Eigen::Vector3d p0;
    Eigen::Vector3d v0;
    Eigen::Vector3d p1;
    Eigen::Vector3d v1;
    double t;
    double ih;
};


Eigen::Vector3d
cubicInterpolate(const InterpolationParameters& params)
{
    Eigen::Vector3d a = 2.0 * (params.p0 - params.p1) + params.v1 + params.v0;
    Eigen::Vector3d b = 3.0 * (params.p1 - params.p0) - 2.0 * params.v0 - params.v1;
    return params.p0 + params.t * (params.v0 + params.t * (b + params.t * a));
}


Eigen::Vector3d
cubicInterpolateVelocity(const InterpolationParameters& params)
{
    Eigen::Vector3d a3 = 3.0 * (2.0 * (params.p0 - params.p1) + params.v1 + params.v0);
    Eigen::Vector3d b2 = 2.0 * (3.0 * (params.p1 - params.p0) - 2.0 * params.v0 - params.v1);
    return (params.v0 + params.t * (b2 + params.t * a3)) * params.ih;
}


// Trajectories are sampled adaptively for rendering.  MaxSampleInterval
// is the maximum time (in days) between samples.  The threshold angle
// is the maximum angle allowed between path segments.
//static const double MinSampleInterval = 1.0 / 1440.0; // one minute
//static const double MaxSampleInterval = 50.0;
//static const double SampleThresholdAngle = 2.0;


template<typename T>
class SampledOrbit : public CachingOrbit
{
public:
    SampledOrbit(TrajectoryInterpolation,
                 std::vector<double>&&,
                 std::vector<Eigen::Matrix<T, 3, 1>>&&);
    ~SampledOrbit() override = default;

    double getPeriod() const override;
    double getBoundingRadius() const override;
    Eigen::Vector3d computePosition(double jd) const override;
    Eigen::Vector3d computeVelocity(double jd) const override;

    bool isPeriodic() const override;
    void getValidRange(double& begin, double& end) const override;

    void sample(double startTime, double endTime, OrbitSampleProc& proc) const override;

private:
    std::vector<double> sampleTimes;
    std::vector<Eigen::Matrix<T, 3, 1>> positions;
    double boundingRadius{ 0.0 };
    mutable std::uint32_t lastSample{ 0 };

    TrajectoryInterpolation interpolation;

    Eigen::Vector3d computePositionLinear(double, std::uint32_t) const;
    Eigen::Vector3d computePositionCubic(double, std::uint32_t, std::uint32_t) const;
    Eigen::Vector3d computeVelocityLinear(std::uint32_t) const;
    Eigen::Vector3d computeVelocityCubic(double, std::uint32_t, std::uint32_t) const;

    void initializeCubic(double, std::uint32_t, std::uint32_t, InterpolationParameters&) const;
};


template<typename T>
SampledOrbit<T>::SampledOrbit(TrajectoryInterpolation _interpolation,
                              std::vector<double>&& _sampleTimes,
                              std::vector<Eigen::Matrix<T, 3, 1>>&& _positions) :
    sampleTimes(std::move(_sampleTimes)),
    positions(std::move(_positions)),
    interpolation(_interpolation)
{
    assert(!sampleTimes.empty() && sampleTimes.size() == positions.size());
    sampleTimes.shrink_to_fit();
    positions.shrink_to_fit();

    // Apply correction for Celestia's coordinate system
    for (Eigen::Matrix<T, 3, 1>& position : positions)
    {
        boundingRadius = std::max(boundingRadius, position.template cast<double>().norm());
        position.template tail<2>().reverseInPlace();
        position.z() = -position.z();
    }
}


template<typename T>
double
SampledOrbit<T>::getPeriod() const
{
    return sampleTimes.back() - sampleTimes.front();
}


template<typename T>
bool
SampledOrbit<T>::isPeriodic() const
{
    return false;
}


template<typename T>
void
SampledOrbit<T>::getValidRange(double& begin, double& end) const
{
    begin = sampleTimes.front();
    end = sampleTimes.back();
}


template<typename T>
double
SampledOrbit<T>::getBoundingRadius() const
{
    return boundingRadius;
}


template<typename T>
Eigen::Vector3d
SampledOrbit<T>::computePosition(double jd) const
{
    if (sampleTimes.size() == 1)
        return positions.front().template cast<double>();

    std::uint32_t n = GetSampleIndex(jd, lastSample, sampleTimes);
    if (n == 0)
        return positions.front().template cast<double>();
    if (n == sampleTimes.size())
        return positions.back().template cast<double>();

    switch (interpolation)
    {
    case TrajectoryInterpolation::Linear:
        return computePositionLinear(jd, n);
    case TrajectoryInterpolation::Cubic:
        return computePositionCubic(jd, n, static_cast<std::uint32_t>(sampleTimes.size() - 1));
    default: // Unknown interpolation type
        assert(0);
        return Eigen::Vector3d::Zero();
    }
}


template<typename T>
Eigen::Vector3d
SampledOrbit<T>::computePositionLinear(double jd, std::uint32_t n) const
{
    assert(n > 0);
    double t = (jd - sampleTimes[n - 1]) / (sampleTimes[n] - sampleTimes[n - 1]);

    const Eigen::Matrix<T, 3, 1>& s0 = positions[n - 1];
    const Eigen::Matrix<T, 3, 1>& s1 = positions[n];
    return Eigen::Vector3d(celmath::lerp(t, static_cast<double>(s0.x()), static_cast<double>(s1.x())),
                           celmath::lerp(t, static_cast<double>(s0.y()), static_cast<double>(s1.y())),
                           celmath::lerp(t, static_cast<double>(s0.z()), static_cast<double>(s1.z())));
}


template<typename T>
Eigen::Vector3d
SampledOrbit<T>::computePositionCubic(double jd, std::uint32_t n2, std::uint32_t nMax) const
{
    InterpolationParameters params;
    initializeCubic(jd, n2, nMax, params);
    return cubicInterpolate(params);
}


template<typename T>
Eigen::Vector3d
SampledOrbit<T>::computeVelocity(double jd) const
{
    if (sampleTimes.size() < 2)
        return Eigen::Vector3d::Zero();

    std::uint32_t n = GetSampleIndex(jd, lastSample, sampleTimes);
    if (n == 0 || n == sampleTimes.size())
        return Eigen::Vector3d::Zero();

    switch (interpolation)
    {
    case TrajectoryInterpolation::Linear:
        return computeVelocityLinear(n);
    case TrajectoryInterpolation::Cubic:
        return computeVelocityCubic(jd, n, static_cast<std::uint32_t>(sampleTimes.size() - 1));
    default: // Unknown interpolation type
        assert(0);
        return Eigen::Vector3d::Zero();
    }
}


template<typename T>
Eigen::Vector3d
SampledOrbit<T>::computeVelocityLinear(std::uint32_t n) const
{
    assert(n > 0);
    double dtRecip = 1.0 / (sampleTimes[n] - sampleTimes[n - 1]);
    return (positions[n].template cast<double>() - positions[n - 1].template cast<double>()) * dtRecip;
}


template<typename T>
Eigen::Vector3d
SampledOrbit<T>::computeVelocityCubic(double jd, std::uint32_t n2, std::uint32_t nMax) const
{
    InterpolationParameters params;
    initializeCubic(jd, n2, nMax, params);
    return cubicInterpolateVelocity(params);
}


template<typename T>
void
SampledOrbit<T>::initializeCubic(double jd,
                                 std::uint32_t n2,
                                 std::uint32_t nMax,
                                 InterpolationParameters& params) const
{
    assert(n2 > 0 && n2 <= nMax);
    std::uint32_t n0 = std::max(n2, std::uint32_t(2)) - 2;
    std::uint32_t n1 = n2 - 1;
    std::uint32_t n3 = std::min(n2 + 1, nMax);

    double h = sampleTimes[n2] - sampleTimes[n1];
    params.ih = 1.0 / h;
    params.t = (jd - sampleTimes[n1]) * params.ih;
    params.p0 = positions[n1].template cast<double>();
    params.p1 = positions[n2].template cast<double>();

    Eigen::Vector3d v10 = params.p0 - positions[n0].template cast<double>();
    Eigen::Vector3d v21 = params.p1 - params.p0;
    Eigen::Vector3d v32 = positions[n3].template cast<double>() - params.p1;

    // Estimate velocities by averaging the differences at adjacent spans
    // (except at the end spans, where we just use a single velocity.)
    params.v0 = n2 > 1
        ? (v10 * (0.5 / (sampleTimes[n1] - sampleTimes[n0])) + v21 * (0.5 * params.ih)) * h
        : v21;

    params.v1 = n2 < nMax
        ? (v21 * (0.5 * params.ih) + v32 * (0.5 / (sampleTimes[n3] - sampleTimes[n2]))) * h
        : v21;
}


template<typename T>
void SampledOrbit<T>::sample(double /* startTime */, double /* endTime */,
                             OrbitSampleProc& proc) const
{
    for (std::uint32_t i = 0; i < sampleTimes.size(); ++i)
    {
        Eigen::Vector3d v;
        Eigen::Vector3d p = positions[i].template cast<double>();

        if (sampleTimes.size() == 1)
        {
            v = Eigen::Vector3d::Zero();
        }
        else if (i == 0)
        {
            double dtRecip = 1.0 / (sampleTimes[i + 1] - sampleTimes[i]);
            v = (positions[i + 1].template cast<double>() - p) * dtRecip;
        }
        else if (i == sampleTimes.size() - 1)
        {
            double dtRecip = 1.0 / (sampleTimes[i] - sampleTimes[i - 1]);
            v = (p - positions[i - 1].template cast<double>()) * dtRecip;
        }
        else
        {
            double dt0Recip = 1.0 / (sampleTimes[i + 1] - sampleTimes[i]);
            Eigen::Vector3d v0 = (positions[i + 1].template cast<double>() - p) * dt0Recip;
            double dt1Recip = 1.0 / (sampleTimes[i] - sampleTimes[i - 1]);
            Eigen::Vector3d v1 = (p - positions[i - 1].template cast<double>()) * dt1Recip;
            v = (v0 + v1) * 0.5;
        }

        proc.sample(sampleTimes[i], p, v);
    }
}


template<typename T>
struct SampleXYZV
{
    Eigen::Matrix<T, 3, 1> position;
    Eigen::Matrix<T, 3, 1> velocity;
};


// Sampled orbit with positions and velocities
template<typename T>
class SampledOrbitXYZV : public CachingOrbit
{
public:
    SampledOrbitXYZV(TrajectoryInterpolation,
                     std::vector<double>&&,
                     std::vector<SampleXYZV<T>>&&);
    ~SampledOrbitXYZV() override = default;

    double getPeriod() const override;
    double getBoundingRadius() const override;
    Eigen::Vector3d computePosition(double jd) const override;
    Eigen::Vector3d computeVelocity(double jd) const override;

    bool isPeriodic() const override;
    void getValidRange(double& begin, double& end) const override;

    void sample(double startTime, double endTime, OrbitSampleProc& proc) const override;

private:
    void initializeCubic(double, std::uint32_t, InterpolationParameters&) const;

    std::vector<double> sampleTimes;
    std::vector<SampleXYZV<T>> samples;
    double boundingRadius{ 0.0 };
    mutable std::uint32_t lastSample{ 0 };

    TrajectoryInterpolation interpolation;
};


template<typename T>
SampledOrbitXYZV<T>::SampledOrbitXYZV(TrajectoryInterpolation _interpolation,
                                      std::vector<double>&& _sampleTimes,
                                      std::vector<SampleXYZV<T>>&& _samples) :
    sampleTimes(std::move(_sampleTimes)),
    samples(std::move(_samples)),
    interpolation(_interpolation)
{
    assert(!sampleTimes.empty() && sampleTimes.size() == samples.size());
    sampleTimes.shrink_to_fit();
    samples.shrink_to_fit();

    // Apply correction for Celestia's coordinate system
    for (SampleXYZV<T>& sample: samples)
    {
        boundingRadius = std::max(boundingRadius,
                                  sample.position.template cast<double>().norm());
        sample.position.template tail<2>().reverseInPlace();
        sample.position.z() = -sample.position.z();
        sample.velocity.template tail<2>().reverseInPlace();
        sample.velocity.z() = -sample.velocity.z();
    }
}


template<typename T>
double
SampledOrbitXYZV<T>::getPeriod() const
{
    return sampleTimes.back() - sampleTimes.front();
}


template<typename T>
bool
SampledOrbitXYZV<T>::isPeriodic() const
{
    return false;
}


template<typename T>
void
SampledOrbitXYZV<T>::getValidRange(double& begin, double& end) const
{
    begin = sampleTimes.front();
    end = sampleTimes.back();
}


template<typename T>
double
SampledOrbitXYZV<T>::getBoundingRadius() const
{
    return boundingRadius;
}


template<typename T>
Eigen::Vector3d
SampledOrbitXYZV<T>::computePosition(double jd) const
{
    if (sampleTimes.size() == 1)
        return samples.front().position.template cast<double>();

    std::uint32_t n = GetSampleIndex(jd, lastSample, sampleTimes);
    if (n == 0)
        return samples.front().position.template cast<double>();
    if (n == sampleTimes.size())
        return samples.back().position.template cast<double>();

    if (interpolation == TrajectoryInterpolation::Linear)
    {
        double t = (jd - sampleTimes[n - 1]) / (sampleTimes[n] - sampleTimes[n - 1]);

        Eigen::Vector3d p0 = samples[n - 1].position.template cast<double>();
        Eigen::Vector3d p1 = samples[n].position.template cast<double>();
        return p0 + t * (p1 - p0);
    }

    if (interpolation == TrajectoryInterpolation::Cubic)
    {
        InterpolationParameters params;
        initializeCubic(jd, n, params);
        return cubicInterpolate(params);
    }

    // Unknown interpolation type
    assert(0);
    return Eigen::Vector3d::Zero();
}


// Velocity is computed as the derivative of the interpolating function
// for position.
template<typename T>
Eigen::Vector3d
SampledOrbitXYZV<T>::computeVelocity(double jd) const
{
    if (sampleTimes.size() < 2)
        return Eigen::Vector3d::Zero();

    std::uint32_t n = GetSampleIndex(jd, lastSample, sampleTimes);
    if (n == 0 || n == sampleTimes.size())
        return Eigen::Vector3d::Zero();

    if (interpolation == TrajectoryInterpolation::Linear)
    {
        double hRecip = 1.0 / (sampleTimes[n] - sampleTimes[n - 1]);
        return (samples[n].position.template cast<double>() - samples[n - 1].position.template cast<double>()) *
                hRecip *
                astro::daysToSecs(1.0);
    }

    if (interpolation == TrajectoryInterpolation::Cubic)
    {
        InterpolationParameters params;
        initializeCubic(jd, n, params);
        return cubicInterpolateVelocity(params);
    }

    // Unknown interpolation type
    assert(0);
    return Eigen::Vector3d::Zero();
}


template<typename T>
void
SampledOrbitXYZV<T>::initializeCubic(double jd,
                                     std::uint32_t n,
                                     InterpolationParameters& params) const
{
    assert(n > 0);
    double h = sampleTimes[n] - sampleTimes[n - 1];
    params.ih = 1.0 / h;
    params.t = (jd - sampleTimes[n - 1]) * params.ih;
    params.p0 = samples[n - 1].position.template cast<double>();
    params.v0 = samples[n - 1].velocity.template cast<double>() * h;
    params.p1 = samples[n].position.template cast<double>();
    params.v1 = samples[n].velocity.template cast<double>() * h;
}


template<typename T>
void
SampledOrbitXYZV<T>::sample(double /* startTime */, double /* endTime */,
                            OrbitSampleProc& proc) const
{
    for (std::uint32_t i = 0; i < sampleTimes.size(); ++i)
    {
        proc.sample(sampleTimes[i],
                    samples[i].position.template cast<double>(),
                    samples[i].velocity.template cast<double>());
    }
}


// Load an ASCII xyz trajectory file. The file contains records with 4 double
// precision values each:
//
// 1: TDB time
// 2: Position x
// 3: Position y
// 4: Position z
//
// Positions are in kilometers.
//
// The numeric data may be preceeded by a comment block. Commented lines begin
// with a #; data is read start fromt the first non-whitespace character outside
// of a comment.

template<typename T> std::unique_ptr<SampledOrbit<T>>
LoadSampledOrbit(const fs::path& filename, TrajectoryInterpolation interpolation, T /*unused*/)
{
    using SampleType = Eigen::Matrix<T, 3, 1>;

    std::vector<double> sampleTimes;
    std::vector<SampleType> samples;
    if (!LoadAsciiSamples(filename, sampleTimes, samples,
                          [](std::istream& in, double& tdb, SampleType& s)
                          {
                              return (in >> tdb >> s.x() >> s.y() >> s.z()).good();
                          }))
    {
        return nullptr;
    }

    return std::make_unique<SampledOrbit<T>>(interpolation, std::move(sampleTimes), std::move(samples));
}


template<typename T>
bool
ReadAsciiSampleXYZV(std::istream& in, double& tdb, SampleXYZV<T>& sample)
{
    in >> tdb
       >> sample.position.x() >> sample.position.y() >> sample.position.z()
       >> sample.velocity.x() >> sample.velocity.y() >> sample.velocity.z();
    if (!in.good())
        return false;

    sample.velocity *= astro::daysToSecs(1.0);
    return true;
}


// Load an xyzv sampled trajectory file. The file contains records with 7 double
// precision values:
//
// 1: TDB time
// 2: Position x
// 3: Position y
// 4: Position z
// 5: Velocity x
// 6: Velocity y
// 7: Velocity z
//
// Positions are in kilometers, velocities are kilometers per second.
//
// The numeric data may be preceeded by a comment block. Commented lines begin
// with a #; data is read start fromt the first non-whitespace character outside
// of a comment.

template<typename T>
std::unique_ptr<SampledOrbitXYZV<T>>
LoadSampledOrbitXYZV(const fs::path& filename, TrajectoryInterpolation interpolation, T /*unused*/)
{
    std::vector<double> sampleTimes;
    std::vector<SampleXYZV<T>> samples;
    if (!LoadAsciiSamples(filename, sampleTimes, samples, &ReadAsciiSampleXYZV<T>))
        return nullptr;

    return std::make_unique<SampledOrbitXYZV<T>>(interpolation, std::move(sampleTimes), std::move(samples));
}


bool
ParseXYZVBinaryHeader(std::istream& in, const fs::path& filename)
{
    std::array<char, sizeof(XYZVBinaryHeader)> header;

    if (!in.read(header.data(), header.size())) /* Flawfinder: ignore */
    {
        GetLogger()->error(_("Error reading header of {}.\n"), filename);
        return false;
    }

    if (std::string_view(header.data() + offsetof(XYZVBinaryHeader, magic), XYZV_MAGIC.size()) != XYZV_MAGIC)
    {
        GetLogger()->error(_("Bad binary xyzv file {}.\n"), filename);
        return false;
    }

    decltype(XYZVBinaryHeader::byteOrder) byteOrder;
    std::memcpy(&byteOrder, header.data() + offsetof(XYZVBinaryHeader, byteOrder), sizeof(byteOrder));
    if (byteOrder != static_cast<decltype(byteOrder)>(celestia::compat::endian::native))
    {
        GetLogger()->error(_("Unsupported byte order {}, expected {} in {}.\n"),
                           byteOrder, static_cast<int>(celestia::compat::endian::native), filename);
        return false;
    }

    decltype(XYZVBinaryHeader::digits) digits;
    std::memcpy(&digits, header.data() + offsetof(XYZVBinaryHeader, digits), sizeof(digits));
    if (digits != std::numeric_limits<double>::digits)
    {
        GetLogger()->error(_("Unsupported digits number {}, expected {} in {}.\n"),
                            digits, std::numeric_limits<double>::digits, filename);
        return false;
    }

    decltype(XYZVBinaryHeader::count) count;
    std::memcpy(&count, header.data() + offsetof(XYZVBinaryHeader, count), sizeof(count));
    if (count == 0)
    {
        GetLogger()->error(_("Invalid record count {} in {}.\n"), count, filename);
        return false;
    }

    return true;
}


template<typename T>
bool
ReadXYZVBinarySample(std::istream& in, double& tdb, SampleXYZV<T>& sample)
{
    std::array<char, sizeof(XYZVBinaryData)> data;
    if (!in.read(data.data(), data.size())) /* Flawfinder: ignore */
        return false;

    std::memcpy(&tdb, data.data() + offsetof(XYZVBinaryData, tdb), sizeof(double));

    if constexpr (std::is_same_v<T, double>)
    {
        std::memcpy(sample.position.data(), data.data() + offsetof(XYZVBinaryData, position), sizeof(double) * 3);
        std::memcpy(sample.velocity.data(), data.data() + offsetof(XYZVBinaryData, velocity), sizeof(double) * 3);

        sample.velocity *= astro::daysToSecs(1.0);
    }
    else
    {
        Eigen::Vector3d position;
        Eigen::Vector3d velocity;

        std::memcpy(position.data(), data.data() + offsetof(XYZVBinaryData, position), sizeof(double) * 3);
        std::memcpy(velocity.data(), data.data() + offsetof(XYZVBinaryData, velocity), sizeof(double) * 3);

        sample.position = position.template cast<T>();
        sample.velocity = (velocity * astro::daysToSecs(1.0)).template cast<T>();
    }

    return true;
}



/* Load a binary xyzv sampled trajectory file.
 */
template <typename T>
std::unique_ptr<SampledOrbitXYZV<T>>
LoadSampledOrbitXYZVBinary(const fs::path& filename, TrajectoryInterpolation interpolation)
{
    using SampleType = SampleXYZV<T>;

    std::vector<double> sampleTimes;
    std::vector<SampleType> samples;

    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in.good())
    {
        GetLogger()->error(_("Error opening binary sample file {}.\n"), filename);
        return nullptr;
    }

    if (!ParseXYZVBinaryHeader(in, filename))
    {
        GetLogger()->error(_("Could not read XYZV binary file {}.\n"), filename);
        return nullptr;
    }

    if (!LoadSamples(in, filename, sampleTimes, samples, &ReadXYZVBinarySample<T>))
        return nullptr;

    return std::make_unique<SampledOrbitXYZV<T>>(interpolation,
                                                 std::move(sampleTimes),
                                                 std::move(samples));
}

} // end unnamed namespace


/*! Load a trajectory file containing single precision positions.
 */
std::unique_ptr<Orbit>
LoadSampledTrajectorySinglePrec(const fs::path& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbit(filename, interpolation, 0.0f);
}


/*! Load a trajectory file containing double precision positions.
 */
std::unique_ptr<Orbit>
LoadSampledTrajectoryDoublePrec(const fs::path& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbit(filename, interpolation, 0.0);
}


/*! Load a trajectory file with single precision positions and velocities.
 */
std::unique_ptr<Orbit>
LoadXYZVTrajectorySinglePrec(const fs::path& filename, TrajectoryInterpolation interpolation)
{
    auto binname = filename;
    binname += "bin";
    if (fs::exists(binname))
    {
        std::unique_ptr<Orbit> ret = LoadSampledOrbitXYZVBinary<float>(binname, interpolation);
        if (ret != nullptr) return ret;
    }

    return LoadSampledOrbitXYZV(filename, interpolation, 0.0f);
}


/*! Load a trajectory file with double precision positions and velocities.
 */
std::unique_ptr<Orbit>
LoadXYZVTrajectoryDoublePrec(const fs::path& filename, TrajectoryInterpolation interpolation)
{
    auto binname = filename;
    binname += "bin";
    if (fs::exists(binname))
    {
        std::unique_ptr<Orbit> ret = LoadSampledOrbitXYZVBinary<double>(binname, interpolation);
        if (ret != nullptr) return ret;
    }

    return LoadSampledOrbitXYZV(filename, interpolation, 0.0);
}

/*! Load a binary trajectory file with single precision positions and velocities.
 */
std::unique_ptr<Orbit>
LoadXYZVBinarySinglePrec(const fs::path& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbitXYZVBinary<float>(filename, interpolation);
}


/*! Load a trajectory file with double precision positions and velocities.
 */
std::unique_ptr<Orbit>
LoadXYZVBinaryDoublePrec(const fs::path& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbitXYZVBinary<double>(filename, interpolation);
}

} // end namespace celestia::ephem
