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

#ifndef _CELENGINE_SPICEROTATION_H_
#define _CELENGINE_SPICEROTATION_H_

#include <string>
#include <list>
#include <celmath/quaternion.h>
#include <celengine/rotation.h>

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
    virtual ~SpiceRotation();

    bool init(const std::string& path,
			  const std::list<std::string>* requiredKernels);

    bool isPeriodic() const;
    double getPeriod() const;

    // No notion of an equator for SPICE rotation models
    Quatd computeEquatorOrientation(double /* tdb */) const
    {
        return Quatd(1.0);
    }

    Quatd computeSpin(double jd) const;

 private:
    const std::string m_frameName;
    const std::string m_baseFrameName;
    double m_period;
    bool m_spiceErr;
    double m_validIntervalBegin;
    double m_validIntervalEnd;
	bool m_useDefaultTimeInterval;
};

#endif // _CELENGINE_SPICEROTATION_H_
