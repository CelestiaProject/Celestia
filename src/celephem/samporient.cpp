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

#include <cassert>
#include <cstdint>
#include <istream>
#include <utility>
#include <vector>

#include <Eigen/Geometry>

#include <celmath/geomutil.h>
#include "rotation.h"
#include "sampfile.h"

namespace celestia::ephem
{

namespace
{

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

/*! SampledOrientation is a rotation model that interpolates a sequence
 *  of quaternion keyframes. Typically, an instance of SampledRotation will
 *  be created from a file with LoadSampledOrientation().
 */
class SampledOrientation : public RotationModel
{
public:
    SampledOrientation(std::vector<double>&&, std::vector<Eigen::Quaternionf>&&);
    ~SampledOrientation() override = default;

    /*! The orientation of a sampled rotation model is entirely due
     *  to spin (i.e. there's no notion of an equatorial frame.)
     */
    Eigen::Quaterniond spin(double tjd) const override;

    bool isPeriodic() const override;
    double getPeriod() const override;

    void getValidRange(double& begin, double& end) const override;

private:
    Eigen::Quaternionf getOrientation(double tjd) const;

    // Storing sample times and rotations separately avoids padding due to
    // the 16-byte alignment of Quaternionf
    std::vector<double> sampleTimes;
    std::vector<Eigen::Quaternionf> rotations;
    mutable std::uint32_t lastSample{0};
};


SampledOrientation::SampledOrientation(std::vector<double>&& _sampleTimes,
                                       std::vector<Eigen::Quaternionf>&& _rotations) :
    sampleTimes(std::move(_sampleTimes)),
    rotations(std::move(_rotations))
{
    assert(!sampleTimes.empty() && sampleTimes.size() == rotations.size());
    sampleTimes.shrink_to_fit();
    rotations.shrink_to_fit();

    // Apply a 90-degree rotation around the x-axis to convert the orientation
    // to Celestia's coordinate system
    for (Eigen::Quaternionf& rotation : rotations)
    {
        rotation *= math::XRot90<float>;
    }
}


Eigen::Quaterniond
SampledOrientation::spin(double tjd) const
{
    // TODO: cache the last value returned
    return getOrientation(tjd).cast<double>();
}


double
SampledOrientation::getPeriod() const
{
    return sampleTimes.back() - sampleTimes.front();
}


bool
SampledOrientation::isPeriodic() const
{
    return false;
}


void
SampledOrientation::getValidRange(double& begin, double& end) const
{
    begin = sampleTimes.front();
    end = sampleTimes.back();
}


Eigen::Quaternionf
SampledOrientation::getOrientation(double tjd) const
{
    if (sampleTimes.size() == 1)
        return rotations.front();

    std::uint32_t n = GetSampleIndex(tjd, lastSample, sampleTimes);
    if (n == 0)
        return rotations.front();
    else if (n == sampleTimes.size())
        return rotations.back();

    auto t = static_cast<float>((tjd - sampleTimes[n - 1]) / (sampleTimes[n] - sampleTimes[n - 1]));
    return rotations[n - 1].slerp(t, rotations[n]);
}

} // end unnamed namespace

std::shared_ptr<const RotationModel>
LoadSampledOrientation(const fs::path& filename)
{
    std::vector<double> sampleTimes;
    std::vector<Eigen::Quaternionf> samples;

    if (!LoadAsciiSamples(filename, sampleTimes, samples,
                          [](std::istream& in, double& tdb, Eigen::Quaternionf& q)
                          {
                              return (in >> tdb >> q.w() >> q.x() >> q.y() >> q.z()).good();
                          }))
    {
        return nullptr;
    }

    return std::make_shared<SampledOrientation>(std::move(sampleTimes),
                                                std::move(samples));
}

} // end namespace celestia::ephem
