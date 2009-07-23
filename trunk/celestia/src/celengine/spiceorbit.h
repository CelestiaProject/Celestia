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

#ifndef _CELENGINE_SPICEORBIT_H_
#define _CELENGINE_SPICEORBIT_H_

#include <string>
#include <list>
#include <celengine/orbit.h>

class SpiceOrbit : public CachingOrbit
{
 public:
    SpiceOrbit(const std::string& _targetBodyName,
               const std::string& _originName,
               double _period,
               double _boundingRadius,
               double _beginning,
               double _ending);
	SpiceOrbit(const std::string& _targetBodyName,
			   const std::string& _originName,
			   double _period,
			   double _boundingRadius);
    virtual ~SpiceOrbit();

    bool init(const std::string& path,
			  const std::list<std::string>* requiredKernels);

    virtual bool isPeriodic() const;
    virtual double getPeriod() const;

    virtual double getBoundingRadius() const
    {
        return boundingRadius;
    }

    Eigen::Vector3d computePosition(double jd) const;
    Eigen::Vector3d computeVelocity(double jd) const;

    virtual void getValidRange(double& begin, double& end) const;

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
};

#endif // _CELENGINE_SPICEORBIT_H_
