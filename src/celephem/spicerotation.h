// spicerotation.h
//
// Rotation model interface to the SPICE Toolkit
//
// Copyright (C) 2008, Celestia Development Team
// Initial implementation by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>

#include <Eigen/Geometry>

#include <celcompat/filesystem.h>
#include "rotation.h"

namespace celestia::ephem
{

class SpiceRotation : public CachingRotationModel
{
 public:
    SpiceRotation(const std::string& frameName,
                  const std::string& baseFrameName,
                  double period,
                  double beginning,
                  double ending);
    SpiceRotation(const std::string& frameName,
                  const std::string& baseFrameName,
                  double period);
    virtual ~SpiceRotation() = default;

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

    bool isPeriodic() const;
    double getPeriod() const;

    // No notion of an equator for SPICE rotation models
    Eigen::Quaterniond computeEquatorOrientation(double /* tdb */) const
    {
        return Eigen::Quaterniond::Identity();
    }

    Eigen::Quaterniond computeSpin(double jd) const;

 private:
    const std::string m_frameName;
    const std::string m_baseFrameName;
    double m_period;
    bool m_spiceErr;
    double m_validIntervalBegin;
    double m_validIntervalEnd;
    bool m_useDefaultTimeInterval;

    bool loadRequiredKernel(const fs::path&, const std::string&);
    bool init();
};

}
