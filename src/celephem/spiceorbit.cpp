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

#include <utility>

#include <SpiceUsr.h>

#include <celastro/date.h>
#include <celutil/logger.h>
#include "spiceinterface.h"
#include "spiceorbit.h"

using celestia::util::GetLogger;


namespace celestia::ephem
{

namespace
{

constexpr double MILLISEC = astro::secsToDays(0.001);

} // end unnamed namespace

/*! Create a new SPICE orbit using with a valid interval specified
 *  by beginning and ending.
 */
SpiceOrbit::SpiceOrbit(std::string  _targetBodyName,
                       std::string  _originName,
                       double _period,
                       double _boundingRadius,
                       double _beginning,
                       double _ending) :
    targetBodyName(std::move(_targetBodyName)),
    originName(std::move(_originName)),
    period(_period),
    boundingRadius(_boundingRadius),
    spiceErr(false),
    validIntervalBegin(_beginning),
    validIntervalEnd(_ending),
    useDefaultTimeInterval(false)
{
}


/*! Create a new SPICE orbit. The valid time interval is the first
 *  window over which there is trajectory information for the target
 *  object. All currently loaded kernels are considered when computing
 *  the window. If there's noncontiguous coverage and a time interval
 *  other than the first coverage span is desired, the SPICE orbit must
 *  be constructed with an explicitly specified time range.
 */
SpiceOrbit::SpiceOrbit(std::string  _targetBodyName,
                       std::string  _originName,
                       double _period,
                       double _boundingRadius) :
    targetBodyName(std::move(_targetBodyName)),
    originName(std::move(_originName)),
    period(_period),
    boundingRadius(_boundingRadius),
    spiceErr(false),
    validIntervalBegin(0.0),
    validIntervalEnd(0.0),
    useDefaultTimeInterval(true)
{
}


bool
SpiceOrbit::isPeriodic() const
{
    return period != 0.0;
};


double
SpiceOrbit::getPeriod() const
{
    if (isPeriodic())
        return period;

    return validIntervalEnd - validIntervalBegin;
}


bool
SpiceOrbit::loadRequiredKernel(const std::filesystem::path& path, const std::string& kernel)
{
    // Load required kernel file
    std::filesystem::path filepath = path / "data" / kernel;
    if (LoadSpiceKernel(filepath))
        return true;

    spiceErr = true;
    return false;
}


bool
SpiceOrbit::init()
{
    // Get the ID codes for the target
    if (!GetNaifId(targetBodyName, &targetID))
    {
        GetLogger()->error("Couldn't find SPICE ID for {}\n", targetBodyName);
        spiceErr = true;
        return false;
    }

    if (!GetNaifId(originName, &originID))
    {
        GetLogger()->error("Couldn't find SPICE ID for {}", originName);
        spiceErr = true;
        return false;
    }

    SpiceInt spkCount = 0;
    ktotal_c("spk", &spkCount);

    // Get coverage window for target and origin object
    const int MaxIntervals = 10;
    SPICEDOUBLE_CELL ( targetCoverage, MaxIntervals * 2 );

    // Clear the coverage window.
    scard_c(0, &targetCoverage);

    for (SpiceInt i = 0; i < spkCount; i++)
    {
        SpiceChar filename[512];
        SpiceChar filetype[32];
        SpiceChar source[256];
        SpiceInt handle;
        SpiceBoolean found;

        kdata_c(i, "spk",
                sizeof(filename), sizeof(filetype), sizeof(source),
                filename, filetype, source, &handle, &found);

        // First check the coverage window of the target. No interval
        // is required for ID 0 (the solar system barycenter) which is
        // always at (0, 0, 0).
        if (targetID != 0)
        {
            spkcov_c(filename, targetID, &targetCoverage);
        }
    }

    SpiceInt nIntervals = card_c(&targetCoverage) / 2;
    if (nIntervals <= 0 && targetID != 0)
    {
        GetLogger()->error("Couldn't find object {} in SPICE kernel pool.\n", targetBodyName);
        spiceErr = true;
        if (failed_c())
        {
            reset_c();
        }
        return false;
    }

    // TODO: need to consider the origin object as well as the target
    if (useDefaultTimeInterval)
    {
        // Set the valid time interval for this orbit to the first interval
        // in the coverage window for the target.
        SpiceDouble targetBeginning = -1.0e50;
        SpiceDouble targetEnding    = +1.0e50;

        if (targetID == 0)
        {
            // Time range for solar system barycenter is infinite
            validIntervalBegin = targetBeginning;
            validIntervalEnd = targetEnding;
        }
        else
        {
            wnfetd_c(&targetCoverage, 0, &targetBeginning, &targetEnding);

            // SPICE times are seconds since J2000.0
            validIntervalBegin = astro::secsToDays(targetBeginning) + astro::J2000;
            validIntervalEnd = astro::secsToDays(targetEnding) + astro::J2000;

            // Reduce interval by a millisecond at each end; otherwise, rounding error
            // can cause us to get SPICE errors when computing states right at the edge
            // of the valid window.
            validIntervalBegin += MILLISEC;
            validIntervalEnd -= MILLISEC;
        }
    }
    else
    {
        // Reduce valid interval by a millisecond at each end.
        validIntervalBegin += MILLISEC;
        validIntervalEnd -= MILLISEC;

        SpiceDouble beginningSecondsJ2000 = astro::daysToSecs(validIntervalBegin - astro::J2000);
        SpiceDouble endingSecondsJ2000    = astro::daysToSecs(validIntervalEnd - astro::J2000);

        // A time interval was specified--make sure that it's covered in the SPICE kernel.
        if (targetID != 0 &&
            !wnincd_c(beginningSecondsJ2000, endingSecondsJ2000, &targetCoverage))
        {
            GetLogger()->error("Specified time interval for target {} not available.\n", targetBodyName);
            return false;
        }
    }

    // Test getting the position of the object to make sure that there's
    // adequate data in the kernel to compute the position of the target
    // relative to the origin. Even if both objects are present and have
    // adequate coverage, it's possible that there might be a missing frame
    // definition or itermediate object.
    double beginning = astro::daysToSecs(validIntervalBegin - astro::J2000);
    double position[3];
    double lt = 0.0;
    spkgps_c(targetID, beginning, "eclipj2000", originID,
             position, &lt);
    if (failed_c())
    {
        // Print the error message
        char errMsg[1024];
        getmsg_c("long", sizeof(errMsg), errMsg);
        GetLogger()->error("{}\n", errMsg);
        spiceErr = true;

        reset_c();
    }

    return !spiceErr;
}


Eigen::Vector3d
SpiceOrbit::computePosition(double jd) const
{
    if (jd < validIntervalBegin)
        jd = validIntervalBegin;
    else if (jd > validIntervalEnd)
        jd = validIntervalEnd;

    if (spiceErr)
    {
        return Eigen::Vector3d::Zero();
    }
    else
    {
        // Input time for SPICE is seconds after J2000
        double t = astro::daysToSecs(jd - astro::J2000);
        double position[3];
        double lt;          // One way light travel time

        spkgps_c(targetID,
                 t,
                 "eclipj2000",
                 originID,
                 position,
                 &lt);

        // This shouldn't happen, since we've already computed the valid
        // coverage interval.
        if (failed_c())
        {
            // Print the error message
            char errMsg[1024];
            getmsg_c("long", sizeof(errMsg), errMsg);
            GetLogger()->warn("{}\n", errMsg);

            // Reset the error state
            reset_c();
        }

        // Transform into Celestia's coordinate system
        return Eigen::Vector3d(position[0], position[2], -position[1]);
    }
}


Eigen::Vector3d
SpiceOrbit::computeVelocity(double jd) const
{
    if (jd < validIntervalBegin)
        jd = validIntervalBegin;
    else if (jd > validIntervalEnd)
        jd = validIntervalEnd;

    if (spiceErr)
    {
        return Eigen::Vector3d::Zero();
    }
    else
    {
        // Input time for SPICE is seconds after J2000
        double t = astro::daysToSecs(jd - astro::J2000);
        double state[6];
        double lt;          // One way light travel time

        spkgeo_c(targetID,
                 t,
                 "eclipj2000",
                 originID,
                 state,
                 &lt);

        // This shouldn't happen, since we've already computed the valid
        // coverage interval.
        if (failed_c())
        {
            // Print the error message
            char errMsg[1024];
            getmsg_c("long", sizeof(errMsg), errMsg);
            GetLogger()->warn("{}\n", errMsg);

            // Reset the error state
            reset_c();
        }

        // Transform into Celestia's coordinate system, and from km/s to km/day
        double d2s = astro::daysToSecs(1.0);
        return Eigen::Vector3d(state[3] * d2s, state[5] * d2s, -state[4] * d2s);
    }
}


void SpiceOrbit::getValidRange(double& begin, double& end) const
{
    begin = validIntervalBegin;
    end = validIntervalEnd;
}

}
