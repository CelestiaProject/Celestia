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

#include <filesystem>
#include <string>

#include <Eigen/Geometry>

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
    bool init(const std::filesystem::path& path, It begin, It end)
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

    // No notion of an equator for SPICE rotation models
    Eigen::Quaterniond computeEquatorOrientation(double /* tdb */) const override
    {
        return Eigen::Quaterniond::Identity();
    }

    Eigen::Quaterniond computeSpin(double jd) const override;

 private:
    const std::string m_frameName;
    const std::string m_baseFrameName;
    double m_period;
    bool m_spiceErr;
    double m_validIntervalBegin;
    double m_validIntervalEnd;

    bool loadRequiredKernel(const std::filesystem::path&, const std::string&);
    bool init();
};

}
