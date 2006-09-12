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
#include <celengine/orbit.h>

class SpiceOrbit : public CachingOrbit
{
 public:
    SpiceOrbit(const std::string& _kernelFile,
               const std::string& _targetBodyName,
               const std::string& _originName,
               double _period,
               double _boundingRadius);
    virtual ~SpiceOrbit();

    bool init();

    double getPeriod() const
    {
        return period;
    }

    double getBoundingRadius() const
    {
        return boundingRadius;
    }

    Point3d computePosition(double jd) const;

 private:
    const std::string kernelFile;
    const std::string targetBodyName;
    const std::string originName;
    double period;
    double boundingRadius;
    bool spiceErr;
};

#endif // _CELENGINE_SPICEORBIT_H_
