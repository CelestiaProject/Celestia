// samporient.cpp
//
// Copyright (C) 2006-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// The SampledOrientation class models orientation of a body by interpolating
// a sequence of key frames.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "samporient.h"

#include <algorithm>
#include <fstream>
#include <vector>

#include <Eigen/Geometry>

#include <celcompat/numbers.h>
#include <celmath/geomutil.h>
#include "rotation.h"

namespace celestia::ephem
{

namespace
{

struct OrientationSample
{
    Eigen::Quaternionf q;
    double t;
};

using OrientationSampleVector = std::vector<OrientationSample>;

/*!
 * Sampled orientation files are ASCII text files containing a sequence of
 * time stamped quaternion keys. Each record in the file has the form:
 *
 *   <time> <qw> <qx> <qy> <qz>
 *
 * Where (qw qx qy qz) is a unit quaternion representing a rotation of
 *   theta = acos(qw)*2 radians about the axis (qx, qy, qz)*sin(theta/2).
 * The time values are Julian days in Barycentric Dynamical Time. The records
 * in the orientation file should be ordered so that their times are
 * monotonically increasing.
 *
 * A very simple example file:
 *
 *   2454025 1     0     0     0
 *   2454026 0.707 0.707 0     0
 *   2454027 0     0     1     0
 *
 * Note that while each record of this example file is on a separate line,
 * all whitespace is treated identically, so the entire file could be one
 * a single line.
 */


bool operator<(const OrientationSample& a, const OrientationSample& b)
{
    return a.t < b.t;
}

/*! SampledOrientation is a rotation model that interpolates a sequence
 *  of quaternion keyframes. Typically, an instance of SampledRotation will
 *  be created from a file with LoadSampledOrientation().
 */
class SampledOrientation : public RotationModel
{
public:
    SampledOrientation() = default;
    ~SampledOrientation() override = default;

    /*! Add another quaternion key to the sampled orientation. The keys
     *  should have monotonically increasing time values.
     */
    void addSample(double tjd, const Eigen::Quaternionf& q);

    /*! The orientation of a sampled rotation model is entirely due
     *  to spin (i.e. there's no notion of an equatorial frame.)
     */
    Eigen::Quaterniond spin(double tjd) const override;

    bool isPeriodic() const override;
    double getPeriod() const override;

    void getValidRange(double& begin, double& end) const override;

private:
    Eigen::Quaternionf getOrientation(double tjd) const;

private:
    OrientationSampleVector samples;
    mutable int lastSample{0};

    enum InterpolationType
    {
        Linear = 0,
        Cubic = 1,
    };

    InterpolationType interpolation{Linear};
};


void
SampledOrientation::addSample(double t, const Eigen::Quaternionf& q)
{
    // 90 degree rotation about x-axis to convert orientation to Celestia's
    // coordinate system.
    static const Eigen::Quaternionf coordSysCorrection = celmath::XRotation((float) (celestia::numbers::pi / 2.0));

    // TODO: add a check for out of sequence samples
    OrientationSample& samp = samples.emplace_back();
    samp.t = t;
    samp.q = q * coordSysCorrection;
}


Eigen::Quaterniond
SampledOrientation::spin(double tjd) const
{
    // TODO: cache the last value returned
    return getOrientation(tjd).cast<double>();
}


double SampledOrientation::getPeriod() const
{
    return samples[samples.size() - 1].t - samples[0].t;
}


bool SampledOrientation::isPeriodic() const
{
    return false;
}


void SampledOrientation::getValidRange(double& begin, double& end) const
{
    begin = samples[0].t;
    end = samples[samples.size() - 1].t;
}


Eigen::Quaternionf
SampledOrientation::getOrientation(double tjd) const
{
    Eigen::Quaternionf orientation;
    if (samples.size() == 0)
    {
        orientation = Eigen::Quaternionf::Identity();
    }
    else if (samples.size() == 1)
    {
        orientation = samples[0].q;
    }
    else
    {
        OrientationSample samp;
        samp.t = tjd;
        int n = lastSample;

        // Do a binary search to find the samples that define the orientation
        // at the current time. Cache the previous sample used and avoid
        // the search if the covers the requested time.
        if (n < 1 || n >= (int) samples.size() || tjd < samples[n - 1].t || tjd > samples[n].t)
        {
            auto iter = std::lower_bound(samples.begin(),
                                         samples.end(),
                                         samp);
            if (iter == samples.end())
                n = samples.size();
            else
                n = iter - samples.begin();

            lastSample = n;
        }

        if (n == 0)
        {
            orientation = samples[0].q;
        }
        else if (n < (int) samples.size())
        {
            if (interpolation == Linear)
            {
                OrientationSample s0 = samples[n - 1];
                OrientationSample s1 = samples[n];

                auto t = (float) ((tjd - s0.t) / (s1.t - s0.t));
                orientation = s0.q.slerp(t, s1.q);
            }
            else if (interpolation == Cubic)
            {
                // TODO: add support for cubic interpolation of quaternions
                assert(0);
            }
            else
            {
                // Unknown interpolation type
                orientation = Eigen::Quaternionf::Identity();
            }
        }
        else
        {
            orientation = samples[samples.size() - 1].q;
        }
    }

    return orientation;
}

} // end unnamed namespace


std::unique_ptr<RotationModel>
LoadSampledOrientation(const fs::path& filename)
{
    std::ifstream in(filename);
    if (!in.good())
        return nullptr;

    auto sampOrientation = std::make_unique<SampledOrientation>();
    int nSamples = 0;
    while (in.good())
    {
        double tjd;
        Eigen::Quaternionf q;
        in >> tjd;
        in >> q.w();
        in >> q.x();
        in >> q.y();
        in >> q.z();
        q.normalize();

        if (in.good())
        {
            sampOrientation->addSample(tjd, q);
            nSamples++;
        }
    }

    return sampOrientation;
}

} // end namespace celestia::ephem
