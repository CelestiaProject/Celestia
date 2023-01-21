// spiceorbit.h
//
// Interface to the SPICE Toolkit
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>

#include <Eigen/Core>

#include <celcompat/filesystem.h>
#include "orbit.h"

namespace celestia::ephem
{

class SpiceOrbit : public CachingOrbit
{
 public:
    SpiceOrbit(std::string  _targetBodyName,
               std::string  _originName,
               double _period,
               double _boundingRadius,
               double _beginning,
               double _ending);
    SpiceOrbit(std::string  _targetBodyName,
               std::string  _originName,
               double _period,
               double _boundingRadius);
    ~SpiceOrbit() override = default;

    template<typename It>
    bool init(const fs::path& path, It begin, It end)
    {
        // Load required kernel files
        while (begin != end)
        {
            if (!loadRequiredKernel(path, *(begin++)))
                return false;
        }
        return init();
    }

    bool isPeriodic() const override;
    double getPeriod() const override;

    double getBoundingRadius() const override
    {
        return boundingRadius;
    }

    Eigen::Vector3d computePosition(double jd) const override;
    Eigen::Vector3d computeVelocity(double jd) const override;

    void getValidRange(double& begin, double& end) const override;

 private:
    const std::string targetBodyName;
    const std::string originName;
    double period;
    double boundingRadius;
    bool spiceErr;

    // NAIF ID codes for the target body and origin body
    int targetID;
    int originID;

    double validIntervalBegin;
    double validIntervalEnd;

    bool useDefaultTimeInterval;

    bool init();
    bool loadRequiredKernel(const fs::path&, const std::string&);
};

}
