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
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <istream>
#include <limits>
#include <string_view>
#include <vector>

#include <Eigen/Core>

#include <celcompat/filesystem.h>
#include <celengine/astro.h>
#include <celmath/mathlib.h>
#include <celutil/bytes.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "orbit.h"
#include "xyzvbinary.h"

using celestia::util::GetLogger;

namespace celestia::ephem
{

namespace
{

// Trajectories are sampled adaptively for rendering.  MaxSampleInterval
// is the maximum time (in days) between samples.  The threshold angle
// is the maximum angle allowed between path segments.
//static const double MinSampleInterval = 1.0 / 1440.0; // one minute
//static const double MaxSampleInterval = 50.0;
//static const double SampleThresholdAngle = 2.0;

// Position-only sample
template<typename T> struct Sample
{
    double t;
    Eigen::Matrix<T, 3, 1> position;
};

// Position + velocity sample
template<typename T> struct SampleXYZV
{
    double t;
    Eigen::Matrix<T, 3, 1> position;
    Eigen::Matrix<T, 3, 1> velocity;
};


template<typename T> bool operator<(const Sample<T>& a, const Sample<T>& b)
{
    return a.t < b.t;
}

template<typename T> bool operator<(const SampleXYZV<T>& a, const SampleXYZV<T>& b)
{
    return a.t < b.t;
}


template<typename T> class SampledOrbit : public CachingOrbit
{
public:
    SampledOrbit(TrajectoryInterpolation /*_interpolation*/);
    ~SampledOrbit() override = default;

    void addSample(double t, const Eigen::Matrix<T, 3, 1>& position);

    double getPeriod() const override;
    double getBoundingRadius() const override;
    Eigen::Vector3d computePosition(double jd) const override;
    Eigen::Vector3d computeVelocity(double jd) const override;

    bool isPeriodic() const override;
    void getValidRange(double& begin, double& end) const override;

    void sample(double startTime, double endTime, OrbitSampleProc& proc) const override;

private:
    std::vector<Sample<T>> samples;
    double boundingRadius;
    double period;
    mutable int lastSample;

    TrajectoryInterpolation interpolation;

    Eigen::Vector3d computePositionLinear(double jd, int n) const;
    Eigen::Vector3d computePositionCubic(double jd, int n) const;
    Eigen::Vector3d computeVelocityLinear(double jd, int n) const;
    Eigen::Vector3d computeVelocityCubic(double jd, int n) const;
};


template<typename T> SampledOrbit<T>::SampledOrbit(TrajectoryInterpolation _interpolation) :
    boundingRadius(0.0),
    period(1.0),
    lastSample(0),
    interpolation(_interpolation)
{
}


template<typename T> void SampledOrbit<T>::addSample(double t, const Eigen::Matrix<T, 3, 1>& position)
{
    double r = position.template cast<double>().norm();
    if (r > boundingRadius)
        boundingRadius = r;

    Sample<T>& samp = samples.emplace_back();
    samp.position = position;
    samp.t = t;
}

template<typename T> double SampledOrbit<T>::getPeriod() const
{
    return samples[samples.size() - 1].t - samples[0].t;
}


template<typename T> bool SampledOrbit<T>::isPeriodic() const
{
    return false;
}


template<typename T> void SampledOrbit<T>::getValidRange(double& begin, double& end) const
{
    begin = samples[0].t;
    end = samples[samples.size() - 1].t;
}


template<typename T> double SampledOrbit<T>::getBoundingRadius() const
{
    return boundingRadius;
}


Eigen::Vector3d cubicInterpolate(const Eigen::Vector3d& p0, const Eigen::Vector3d& v0,
                                 const Eigen::Vector3d& p1, const Eigen::Vector3d& v1,
                                 double t)
{
    return p0 + (((2.0 * (p0 - p1) + v1 + v0) * (t * t * t)) +
                 ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (t * t)) +
                 (v0 * t));
}


Eigen::Vector3d cubicInterpolateVelocity(const Eigen::Vector3d& p0, const Eigen::Vector3d& v0,
                                         const Eigen::Vector3d& p1, const Eigen::Vector3d& v1,
                                         double t)
{
    return ((2.0 * (p0 - p1) + v1 + v0) * (3.0 * t * t)) +
           ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (2.0 * t)) +
           v0;
}


template <typename T> Eigen::Vector3d SampledOrbit<T>::computePosition(double jd) const
{
    Eigen::Vector3d pos;
    if (samples.size() == 0)
    {
        pos = Eigen::Vector3d::Zero();
    }
    else if (samples.size() == 1)
    {
        pos = samples[0].position.template cast<double>();
    }
    else
    {
        Sample<T> samp;
        samp.t = jd;
        int n = lastSample;

        if (n < 1 || n >= (int) samples.size() || jd < samples[n - 1].t || jd > samples[n].t)
        {
            auto iter = std::lower_bound(samples.begin(), samples.end(), samp);
            n = iter == samples.end()
                ? n = samples.size()
                : iter - samples.begin();

            lastSample = n;
        }

        if (n == 0)
        {
            pos = samples[n].position.template cast<double>();
        }
        else if (n < (int) samples.size())
        {
            switch (interpolation)
            {
            case TrajectoryInterpolation::Linear:
                pos = computePositionLinear(jd, n);
                break;
            case TrajectoryInterpolation::Cubic:
                pos = computePositionCubic(jd, n);
                break;
            default: // Unknown interpolation type
                pos = Eigen::Vector3d::Zero();
                break;
            }
        }
        else
        {
            pos = samples[n - 1].position.template cast<double>();
        }
    }

    // Add correction for Celestia's coordinate system
    return Eigen::Vector3d(pos.x(), pos.z(), -pos.y());
}


template<typename T> Eigen::Vector3d SampledOrbit<T>::computePositionLinear(double jd,
                                                                            int n) const
{
    Sample<T> s0 = samples[n - 1];
    Sample<T> s1 = samples[n];

    double t = (jd - s0.t) / (s1.t - s0.t);
    return Eigen::Vector3d(celmath::lerp(t, (double) s0.position.x(), (double) s1.position.x()),
                           celmath::lerp(t, (double) s0.position.y(), (double) s1.position.y()),
                           celmath::lerp(t, (double) s0.position.z(), (double) s1.position.z()));
}


template<typename T> Eigen::Vector3d SampledOrbit<T>::computePositionCubic(double jd,
                                                                           int n) const
{
    Sample<T> s0 = n > 1
        ? samples[n - 2]
        : samples[n - 1];
    Sample<T> s1 = samples[n - 1];
    Sample<T> s2 = samples[n];
    Sample<T> s3 = n < (int) samples.size() - 1
        ? samples[n + 1]
        : samples[n];

    double h = s2.t - s1.t;
    double ih = 1.0 / h;
    double t = (jd - s1.t) * ih;
    Eigen::Vector3d p0 = s1.position.template cast<double>();
    Eigen::Vector3d p1 = s2.position.template cast<double>();

    Eigen::Vector3d v10 = p0 - s0.position.template cast<double>();
    Eigen::Vector3d v21 = p1 - p0;
    Eigen::Vector3d v32 = s3.position.template cast<double>() - p1;

    // Estimate velocities by averaging the differences at adjacent spans
    // (except at the end spans, where we just use a single velocity.)
    Eigen::Vector3d v0 = n > 1
        ? (v10 * (0.5 / (s1.t - s0.t)) + v21 * (0.5 * ih)) * h
        : v21;

    Eigen::Vector3d v1 = n < ((int) samples.size() - 1)
        ? (v21 * (0.5 * ih) + v32 * (0.5 / (s3.t - s2.t))) * h
        : v21;

    return cubicInterpolate(p0, v0, p1, v1, t);
}


template<typename T> Eigen::Vector3d SampledOrbit<T>::computeVelocity(double jd) const
{
    Eigen::Vector3d vel;
    if (samples.size() < 2)
    {
        vel = Eigen::Vector3d::Zero();
    }
    else
    {
        Sample<T> samp;
        samp.t = jd;
        int n = lastSample;

        if (n < 1 || n >= (int) samples.size() || jd < samples[n - 1].t || jd > samples[n].t)
        {
            auto iter = std::lower_bound(samples.begin(), samples.end(), samp);
            n = iter == samples.end()
                ? samples.size()
                : iter - samples.begin();
            lastSample = n;
        }

        if (n == 0)
        {
            vel = Eigen::Vector3d::Zero();
        }
        else if (n < (int) samples.size())
        {
            switch (interpolation)
            {
            case TrajectoryInterpolation::Linear:
                vel = computeVelocityLinear(jd, n);
                break;
            case TrajectoryInterpolation::Cubic:
                vel = computeVelocityCubic(jd, n);
                break;
            default: // Unknown interpolation type
                vel = Eigen::Vector3d::Zero();
                break;
            }
        }
        else
        {
            vel = Eigen::Vector3d::Zero();
        }
    }

    return Eigen::Vector3d(vel.x(), vel.z(), -vel.y());
}


template<typename T> Eigen::Vector3d SampledOrbit<T>::computeVelocityLinear(double jd,
                                                                            int n) const
{
    Sample<T> s0 = samples[n - 1];
    Sample<T> s1 = samples[n];

    double dtRecip = 1.0 / (s1.t - s0.t);
    return (s1.position.template cast<double>() - s0.position.template cast<double>()) * dtRecip;
}


template<typename T> Eigen::Vector3d SampledOrbit<T>::computeVelocityCubic(double jd,
                                                                           int n) const
{
    Sample<T> s0 = n > 1
        ? samples[n - 2]
        : samples[n - 1];
    Sample<T> s1 = samples[n - 1];
    Sample<T> s2 = samples[n];
    Sample<T> s3 = n < ((int) samples.size() - 1)
        ? samples[n + 1]
        : samples[n];

    double h = s2.t - s1.t;
    double ih = 1.0 / h;
    double t = (jd - s1.t) * ih;
    Eigen::Vector3d p0 = s1.position.template cast<double>();
    Eigen::Vector3d p1 = s2.position.template cast<double>();

    Eigen::Vector3d v10 = p0 - s0.position.template cast<double>();
    Eigen::Vector3d v21 = p1 - p0;
    Eigen::Vector3d v32 = s3.position.template cast<double>() - p1;

    // Estimate velocities by averaging the differences at adjacent spans
    // (except at the end spans, where we just use a single velocity.)
    Eigen::Vector3d v0 = n > 1
        ? (v10 * (0.5 / (s1.t - s0.t)) + v21 * (0.5 * ih)) * h
        : v21;

    Eigen::Vector3d v1 = n < ((int) samples.size() - 1)
        ? (v21 * (0.5 * ih) + v32 * (0.5 / (s3.t - s2.t))) * h
        : v21;

    return cubicInterpolateVelocity(p0, v0, p1, v1, t) * (1.0 / h);
}


template<typename T> void SampledOrbit<T>::sample(double /* startTime */, double /* endTime */,
                                                  OrbitSampleProc& proc) const
{
    for (unsigned int i = 0; i < samples.size(); i++)
    {
        Eigen::Vector3d v;
        Eigen::Vector3d p = samples[i].position.template cast<double>();

        if (samples.size() == 1)
        {
            v = Eigen::Vector3d::Zero();
        }
        else if (i == 0)
        {
            double dtRecip = 1.0 / (samples[i + 1].t - samples[i].t);
            v = (samples[i + 1].position.template cast<double>() - p) * dtRecip;
        }
        else if (i == samples.size() - 1)
        {
            double dtRecip = 1.0 / (samples[i].t - samples[i - 1].t);
            v = (p - samples[i - 1].position.template cast<double>()) * dtRecip;
        }
        else
        {
            double dt0Recip = 1.0 / (samples[i + 1].t - samples[i].t);
            Eigen::Vector3d v0 = (samples[i + 1].position.template cast<double>() - p) * dt0Recip;
            double dt1Recip = 1.0 / (samples[i].t - samples[i - 1].t);
            Eigen::Vector3d v1 = (p - samples[i - 1].position.template cast<double>()) * dt1Recip;
            v = (v0 + v1) * 0.5;
        }

        proc.sample(samples[i].t,
                    Eigen::Vector3d(p.x(), p.z(), -p.y()),
                    Eigen::Vector3d(v.x(), v.z(), -v.y()));
    }
}


// Sampled orbit with positions and velocities
template <typename T> class SampledOrbitXYZV : public CachingOrbit
{
public:
    SampledOrbitXYZV(TrajectoryInterpolation /*_interpolation*/);
    ~SampledOrbitXYZV() override = default;

    void addSample(double t,
                   const Eigen::Matrix<T, 3, 1>& position,
                   const Eigen::Matrix<T, 3, 1>& velocity);

    double getPeriod() const override;
    double getBoundingRadius() const override;
    Eigen::Vector3d computePosition(double jd) const override;
    Eigen::Vector3d computeVelocity(double jd) const override;

    bool isPeriodic() const override;
    void getValidRange(double& begin, double& end) const override;

    void sample(double startTime, double endTime, OrbitSampleProc& proc) const override;

private:
    std::vector<SampleXYZV<T>> samples;
    double boundingRadius;
    double period;
    mutable int lastSample;

    TrajectoryInterpolation interpolation;
};


template <typename T> SampledOrbitXYZV<T>::SampledOrbitXYZV(TrajectoryInterpolation _interpolation) :
    boundingRadius(0.0),
    period(1.0),
    lastSample(0),
    interpolation(_interpolation)
{
}


// Add a new sample to the trajectory:
//    Position in km
//    Velocity in km/Julian day
template<typename T> void SampledOrbitXYZV<T>::addSample(double t,
                                                         const Eigen::Matrix<T, 3, 1>& position,
                                                         const Eigen::Matrix<T, 3, 1>& velocity)
{
    double r = position.template cast<double>().norm();
    if (r > boundingRadius)
        boundingRadius = r;

    SampleXYZV<T>& samp = samples.emplace_back();
    samp.t = t;
    samp.position = position;
    samp.velocity = velocity;
}

template <typename T> double SampledOrbitXYZV<T>::getPeriod() const
{
    if (samples.empty())
        return 0.0;

    return samples[samples.size() - 1].t - samples[0].t;
}


template <typename T> bool SampledOrbitXYZV<T>::isPeriodic() const
{
    return false;
}


template <typename T> void SampledOrbitXYZV<T>::getValidRange(double& begin, double& end) const
{
    begin = samples[0].t;
    end = samples[samples.size() - 1].t;
}


template <typename T> double SampledOrbitXYZV<T>::getBoundingRadius() const
{
    return boundingRadius;
}


template <typename T> Eigen::Vector3d SampledOrbitXYZV<T>::computePosition(double jd) const
{
    Eigen::Vector3d pos;
    if (samples.size() == 0)
    {
        pos = Eigen::Vector3d::Zero();
    }
    else if (samples.size() == 1)
    {
        pos = samples[0].position.template cast<double>();
    }
    else
    {
        SampleXYZV<T> samp;
        samp.t = jd;
        int n = lastSample;

        if (n < 1 || n >= (int) samples.size() || jd < samples[n - 1].t || jd > samples[n].t)
        {
            auto iter = std::lower_bound(samples.begin(), samples.end(), samp);
            if (iter == samples.end())
                n = samples.size();
            else
                n = iter - samples.begin();

            lastSample = n;
        }

        if (n == 0)
        {
            pos = samples[n].position.template cast<double>();
        }
        else if (n < (int) samples.size())
        {
            SampleXYZV<T> s0 = samples[n - 1];
            SampleXYZV<T> s1 = samples[n];

            if (interpolation == TrajectoryInterpolation::Linear)
            {
                double t = (jd - s0.t) / (s1.t - s0.t);

                Eigen::Vector3d p0 = s0.position.template cast<double>();
                Eigen::Vector3d p1 = s1.position.template cast<double>();
                pos = p0 + t * (p1 - p0);
            }
            else if (interpolation == TrajectoryInterpolation::Cubic)
            {
                double h = s1.t - s0.t;
                double ih = 1.0 / h;
                double t = (jd - s0.t) * ih;

                Eigen::Vector3d p0 = s0.position.template cast<double>();
                Eigen::Vector3d v0 = s0.velocity.template cast<double>();
                Eigen::Vector3d p1 = s1.position.template cast<double>();
                Eigen::Vector3d v1 = s1.velocity.template cast<double>();
                pos = cubicInterpolate(p0, v0 * h, p1, v1 * h, t);
            }
            else
            {
                // Unknown interpolation type
                pos = Eigen::Vector3d::Zero();
            }
        }
        else
        {
            pos = samples[n - 1].position.template cast<double>();
        }
    }

    // Add correction for Celestia's coordinate system
    return Eigen::Vector3d(pos.x(), pos.z(), -pos.y());
}


// Velocity is computed as the derivative of the interpolating function
// for position.
template<typename T> Eigen::Vector3d SampledOrbitXYZV<T>::computeVelocity(double jd) const
{
    Eigen::Vector3d vel(Eigen::Vector3d::Zero());

    if (samples.size() >= 2)
    {
        SampleXYZV<T> samp;
        samp.t = jd;
        int n = lastSample;

        if (n < 1 || n >= (int) samples.size() || jd < samples[n - 1].t || jd > samples[n].t)
        {
            auto iter = std::lower_bound(samples.begin(), samples.end(), samp);
            if (iter == samples.end())
                n = samples.size();
            else
                n = iter - samples.begin();

            lastSample = n;
        }

        if (n > 0 && n < (int) samples.size())
        {
            SampleXYZV<T> s0 = samples[n - 1];
            SampleXYZV<T> s1 = samples[n];

            if (interpolation == TrajectoryInterpolation::Linear)
            {
                double hRecip = 1.0 / (s1.t - s0.t);
                vel = (s1.position.template cast<double>() - s0.position.template cast<double>()) *
                      hRecip *
                      astro::daysToSecs(1.0);
            }
            else if (interpolation == TrajectoryInterpolation::Cubic)
            {
                double h = s1.t - s0.t;
                double ih = 1.0 / h;
                double t = (jd - s0.t) * ih;

                Eigen::Vector3d p0 = s0.position.template cast<double>();
                Eigen::Vector3d p1 = s1.position.template cast<double>();
                Eigen::Vector3d v0 = s0.velocity.template cast<double>();
                Eigen::Vector3d v1 = s1.velocity.template cast<double>();

                vel = cubicInterpolateVelocity(p0, v0 * h, p1, v1 * h, t) * ih;
            }
            else
            {
                // Unknown interpolation type
                vel = Eigen::Vector3d::Zero();
            }
        }
    }

    // Add correction for Celestia's coordinate system
    return Eigen::Vector3d(vel.x(), vel.z(), -vel.y());
}


template<typename T> void SampledOrbitXYZV<T>::sample(double /* startTime */, double /* endTime */,
                                                      OrbitSampleProc& proc) const
{
    for (const auto& sample : samples)
    {
        proc.sample(sample.t,
                    Eigen::Vector3d(sample.position.x(), sample.position.z(), -sample.position.y()),
                    Eigen::Vector3d(sample.velocity.x(), sample.velocity.z(), -sample.velocity.y()));
    }
}


// Scan past comments. A comment begins with the # character and ends
// with a newline. Return true if the stream state is good. The stream
// position will be at the first non-comment, non-whitespace character.
bool SkipComments(std::istream& in)
{
    bool inComment = false;
    bool done = false;

    int c = in.get();
    while (!done)
    {
        if (in.eof())
        {
            done = true;
        }
        else
        {
            if (inComment)
            {
                if (c == '\n')
                    inComment = false;
            }
            else
            {
                if (c == '#')
                {
                    inComment = true;
                }
                else if (std::isspace(static_cast<unsigned char>(c)) == 0)
                {
                    in.unget();
                    done = true;
                }
            }
        }

        if (!done)
            c = in.get();
    }

    return in.good();
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
    std::ifstream in(filename);
    if (!in.good())
        return nullptr;

    if (!SkipComments(in))
        return nullptr;

    auto orbit = std::make_unique<SampledOrbit<T>>(interpolation);

    double lastSampleTime = -std::numeric_limits<double>::infinity();
    while (in.good())
    {
        double tdb;
        Eigen::Matrix<T, 3, 1> position;
        in >> tdb;
        in >> position.x();
        in >> position.y();
        in >> position.z();

        if (in.good())
        {
            // Skip samples with duplicate times; such trajectories are invalid, but
            // are unfortunately used in some existing add-ons.
            if (tdb != lastSampleTime)
            {
                orbit->addSample(tdb, position);
                lastSampleTime = tdb;
            }
        }
    }

    return orbit;
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

template <typename T> std::unique_ptr<SampledOrbitXYZV<T>>
LoadSampledOrbitXYZV(const fs::path& filename, TrajectoryInterpolation interpolation, T /*unused*/)
{
    std::ifstream in(filename);
    if (!in.good())
        return nullptr;

    if (!SkipComments(in))
        return nullptr;

    auto orbit = std::make_unique<SampledOrbitXYZV<T>>(interpolation);

    double lastSampleTime = -std::numeric_limits<double>::infinity();
    while (in.good())
    {
        double tdb = 0.0;
        Eigen::Matrix<T, 3, 1> position;
        Eigen::Matrix<T, 3, 1> velocity;

        in >> tdb;
        in >> position.x();
        in >> position.y();
        in >> position.z();
        in >> velocity.x();
        in >> velocity.y();
        in >> velocity.z();

        // Convert velocities from km/sec to km/Julian day
        velocity = velocity * astro::daysToSecs(1.0);

        if (in.good())
        {
            if (tdb != lastSampleTime)
            {
                orbit->addSample(tdb, position, velocity);
                lastSampleTime = tdb;
            }
        }
    }

    return orbit;
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
    if (byteOrder != __BYTE_ORDER__)
    {
        GetLogger()->error(_("Unsupported byte order {}, expected {}.\n"),
                            byteOrder, __BYTE_ORDER__);
        return false;
    }

    decltype(XYZVBinaryHeader::digits) digits;
    std::memcpy(&digits, header.data() + offsetof(XYZVBinaryHeader, digits), sizeof(digits));
    if (digits != std::numeric_limits<double>::digits)
    {
        GetLogger()->error(_("Unsupported digits number {}, expected {}.\n"),
                            digits, std::numeric_limits<double>::digits);
        return false;
    }

    decltype(XYZVBinaryHeader::count) count;
    std::memcpy(&count, header.data() + offsetof(XYZVBinaryHeader, count), sizeof(count));
    if (count == 0)
    {
        return false;
    }

    return true;
}

/* Load a binary xyzv sampled trajectory file.
 */
template <typename T> std::unique_ptr<SampledOrbitXYZV<T>>
LoadSampledOrbitXYZVBinary(const fs::path& filename, TrajectoryInterpolation interpolation)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in.good())
    {
        GetLogger()->error(_("Error opening {}.\n"), filename);
        return nullptr;
    }

    ParseXYZVBinaryHeader(in, filename);

    auto orbit = std::make_unique<SampledOrbitXYZV<T>>(interpolation);
    double lastSampleTime = -std::numeric_limits<T>::infinity();

    while (in.good())
    {
        std::array<char, sizeof(XYZVBinaryData)> data;
        if (!in.read(data.data(), data.size())) /* Flawfinder: ignore */
            break;

        double tdb;
        Eigen::Vector3d position;
        Eigen::Vector3d velocity;

        std::memcpy(&tdb,            data.data() + offsetof(XYZVBinaryData, tdb),      sizeof(tdb));
        std::memcpy(position.data(), data.data() + offsetof(XYZVBinaryData, position), sizeof(double) * 3);
        std::memcpy(velocity.data(), data.data() + offsetof(XYZVBinaryData, velocity), sizeof(double) * 3);

        // Convert velocities from km/sec to km/Julian day
        velocity *= astro::daysToSecs(1.0);

        if (tdb != lastSampleTime)
        {
            Eigen::Matrix<T, 3, 1> pos = position.cast<T>();
            Eigen::Matrix<T, 3, 1> vel = velocity.cast<T>();
            orbit->addSample(tdb, pos, vel);
            lastSampleTime = tdb;
        }
    }

    return orbit;
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
