// spiceorbit.cpp
//
// Interface to the SPICE Toolkit
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include "SpiceUsr.h"
#include "astro.h"
#include "spiceorbit.h"

using namespace std;


SpiceOrbit::SpiceOrbit(const std::string& _kernelFile,
                       const std::string& _targetBodyName,
                       const std::string& _originName,
                       double _period,
                       double _boundingRadius) :
    kernelFile(_kernelFile),
    targetBodyName(_targetBodyName),
    originName(_originName),
    period(_period),
    boundingRadius(_boundingRadius),
    spiceErr(false)
{
}


SpiceOrbit::~SpiceOrbit()
{
}


bool
SpiceOrbit::init()
{
    // TODO: convert body name strings to id codes

    furnsh_c(kernelFile.c_str());

    // If there was an error loading the kernel, flag an error
    // and don't bother calling SPICE to compute the orbit.
    if (failed_c())
    {
        spiceErr = true;
        char errMsg[1024];
        getmsg_c("long", sizeof(errMsg), errMsg);
        clog << errMsg << "\n";
    }

    return !spiceErr;
}


Point3d
SpiceOrbit::computePosition(double jd) const
{
    if (spiceErr)
    {
        return Point3d(0.0, 0.0, 0.0);
    }
    else
    {
        // Input time for SPICE is seconds after J2000
        double t = astro::daysToSecs(jd - astro::J2000);
        double state[6];    // State is position and velocity
        double lt;          // One way light travel time

        spkezr_c(targetBodyName.c_str(),
                 t,
                 "eclipj2000",
                 "none",
                 originName.c_str(),
                 state,
                 &lt);

        // TODO: test calculating the orbit at initialization time
        // to try and catch errors earlier.
        if (failed_c())
        {
            char errMsg[1024];
            getmsg_c("long", sizeof(errMsg), errMsg);
            clog << errMsg << "\n";
            //spiceErr = true;
        }

        // Transform into Celestia's coordinate system
        return Point3d(state[0], state[2], -state[1]);
    }
}


