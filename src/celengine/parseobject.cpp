// parseobject.cpp
//
// Copyright (C) 2004-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Functions for parsing objects common to star, solar system, and
// deep sky catalogs.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include "parseobject.h"
#include "body.h"
#include "frame.h"
#include "trajmanager.h"
#include "rotationmanager.h"
#include "universe.h"
#include <celcompat/numbers.h>
#include <celephem/customorbit.h>
#include <celephem/customrotation.h>
#ifdef USE_SPICE
#include <celephem/spiceorbit.h>
#include <celephem/spicerotation.h>
#endif
#include <celephem/scriptorbit.h>
#include <celephem/scriptrotation.h>
#include <celmath/geomutil.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include <cassert>

using namespace Eigen;
using namespace std;
using namespace celmath;
using celestia::ephem::TrajectoryInterpolation;
using celestia::ephem::TrajectoryPrecision;
using celestia::util::GetLogger;

namespace celephem = celestia::ephem;

/**
 * Returns the default units scale for orbits.
 *
 * If the usePlanetUnits flag is set, this returns a distance scale of AU and a
 * time scale of years. Otherwise the distace scale is kilometers and the time
 * scale is days.
 *
 * @param[in] usePlanetUnits Controls whether to return planet units or satellite units.
 * @param[out] distanceScale The default distance scale in kilometers.
 * @param[out] timeScale The default time scale in days.
 */
static void
GetDefaultUnits(bool usePlanetUnits, double& distanceScale, double& timeScale)
{
    if(usePlanetUnits)
    {
        distanceScale = KM_PER_AU<double>;
        timeScale = DAYS_PER_YEAR;
    }
    else
    {
        distanceScale = 1.0;
        timeScale = 1.0;
    }
}


/**
 * Returns the default distance scale for orbits.
 *
 * If the usePlanetUnits flag is set, this returns AU, otherwise it returns
 * kilometers.
 *
 * @param[in] usePlanetUnits Controls whether to return planet units or satellite units.
 * @param[out] distanceScale The default distance scale in kilometers.
 */
static void
GetDefaultUnits(bool usePlanetUnits, double& distanceScale)
{
    distanceScale = usePlanetUnits ? KM_PER_AU<double> : 1.0;
}


bool
ParseDate(const Hash* hash, const string& name, double& jd)
{
    // Check first for a number value representing a Julian date
    if (auto jdVal = hash->getNumber<double>(name); jdVal.has_value())
    {
        jd = *jdVal;
        return true;
    }

    if (const std::string* dateString = hash->getString(name); dateString != nullptr)
    {
        astro::Date date(1, 1, 1);
        if (astro::parseDate(*dateString, date))
        {
            jd = (double) date;
            return true;
        }
    }

    return false;
}


/*!
 * Create a new Keplerian orbit from an ssc property table:
 *
 * \code EllipticalOrbit
 * {
 *     # One of the following is required to specify orbit size:
 *     SemiMajorAxis <number>
 *     PericenterDistance <number>
 *
 *     # Required
 *     Period <number>
 *
 *     Eccentricity <number>   (default: 0.0)
 *     Inclination <degrees>   (default: 0.0)
 *     AscendingNode <degrees> (default: 0.0)
 *
 *     # One or none of the following:
 *     ArgOfPericenter <degrees>  (default: 0.0)
 *     LongOfPericenter <degrees> (default: 0.0)
 *
 *     Epoch <date> (default J2000.0)
 *
 *     # One or none of the following:
 *     MeanAnomaly <degrees>     (default: 0.0)
 *     MeanLongitude <degrees>   (default: 0.0)
 * } \endcode
 *
 * If usePlanetUnits is true:
 *     Period is in Julian years
 *     SemiMajorAxis or PericenterDistance is in AU
 * Otherwise:
 *     Period is in Julian days
 *     SemiMajorAxis or PericenterDistance is in kilometers.
 */
static std::unique_ptr<celestia::ephem::Orbit>
CreateEllipticalOrbit(const Hash* orbitData,
                      bool usePlanetUnits)
{

    // default units for planets are AU and years, otherwise km and days

    double distanceScale;
    double timeScale;
    GetDefaultUnits(usePlanetUnits, distanceScale, timeScale);

    // SemiMajorAxis and Period are absolutely required; everything
    // else has a reasonable default.
    double pericenterDistance = 0.0;
    std::optional<double> semiMajorAxis = orbitData->getLength<double>("SemiMajorAxis", 1.0, distanceScale);
    if (!semiMajorAxis.has_value())
    {
        if (auto pericenter = orbitData->getLength<double>("PericenterDistance", 1.0, distanceScale); pericenter.has_value())
        {
            pericenterDistance = *pericenter;
        }
        else
        {
            GetLogger()->error("SemiMajorAxis/PericenterDistance missing!  Skipping planet . . .\n");
            return nullptr;
        }
    }

    double period = 0.0;
    if (auto periodValue = orbitData->getTime<double>("Period", 1.0, timeScale); periodValue.has_value())
    {
        period = *periodValue;
    }
    else
    {
        GetLogger()->error("Period missing!  Skipping planet . . .\n");
        return nullptr;
    }

    auto eccentricity = orbitData->getNumber<double>("Eccentricity").value_or(0.0);

    auto inclination = orbitData->getAngle<double>("Inclination").value_or(0.0);

    double ascendingNode = orbitData->getAngle<double>("AscendingNode").value_or(0.0);

    double argOfPericenter = 0.0;
    if (auto argPeri = orbitData->getAngle<double>("ArgOfPericenter"); argPeri.has_value())
    {
        argOfPericenter = *argPeri;
    }
    else if (auto longPeri = orbitData->getAngle<double>("LongOfPericenter"); longPeri.has_value())
    {
        argOfPericenter = *longPeri - ascendingNode;
    }

    double epoch = astro::J2000;
    ParseDate(orbitData, "Epoch", epoch);

    // Accept either the mean anomaly or mean longitude--use mean anomaly
    // if both are specified.
    double anomalyAtEpoch = 0.0;
    if (auto meanAnomaly = orbitData->getAngle<double>("MeanAnomaly"); meanAnomaly.has_value())
    {
        anomalyAtEpoch = *meanAnomaly;
    }
    else if (auto meanLongitude = orbitData->getAngle<double>("MeanLongitude"); meanLongitude.has_value())
    {
        anomalyAtEpoch = *meanLongitude - (argOfPericenter + ascendingNode);
    }

    // If we read the semi-major axis, use it to compute the pericenter
    // distance.
    if (semiMajorAxis.has_value())
        pericenterDistance = *semiMajorAxis * (1.0 - eccentricity);

    return std::make_unique<celestia::ephem::EllipticalOrbit>(pericenterDistance,
                                                              eccentricity,
                                                              degToRad(inclination),
                                                              degToRad(ascendingNode),
                                                              degToRad(argOfPericenter),
                                                              degToRad(anomalyAtEpoch),
                                                              period,
                                                              epoch);
}


/*!
 * Create a new sampled orbit from an ssc property table:
 *
 * \code SampledTrajectory
 * {
 *     Source <string>
 *     Interpolation "Cubic" | "Linear"
 *     DoublePrecision <boolean>
 * } \endcode
 *
 * Source is the only required field. Interpolation defaults to cubic, and
 * DoublePrecision defaults to true.
 */
static celestia::ephem::Orbit*
CreateSampledTrajectory(const Hash* trajData, const fs::path& path)
{
    const std::string* sourceName = trajData->getString("Source");
    if (sourceName == nullptr)
    {
        GetLogger()->error("SampledTrajectory is missing a source.\n");
        return nullptr;
    }

    // Read interpolation type; string value must be either "Linear" or "Cubic"
    // Default interpolation type is cubic.
    TrajectoryInterpolation interpolation = TrajectoryInterpolation::Cubic;
    if (const std::string* interpolationString = trajData->getString("Interpolation"); interpolationString != nullptr)
    {
        if (!compareIgnoringCase(*interpolationString, "linear"))
            interpolation = TrajectoryInterpolation::Linear;
        else if (!compareIgnoringCase(*interpolationString, "cubic"))
            interpolation = TrajectoryInterpolation::Cubic;
        else
            GetLogger()->warn("Unknown interpolation type {}\n", *interpolationString); // non-fatal error
    }

    // Double precision is true by default
    bool useDoublePrecision = trajData->getBoolean("DoublePrecision").value_or(true);
    TrajectoryPrecision precision = useDoublePrecision ? TrajectoryPrecision::Double : TrajectoryPrecision::Single;

    GetLogger()->verbose("Attempting to load sampled trajectory from source '{}'\n", *sourceName);
    ResourceHandle orbitHandle = GetTrajectoryManager()->getHandle(TrajectoryInfo(*sourceName, path, interpolation, precision));
    celestia::ephem::Orbit* orbit = GetTrajectoryManager()->find(orbitHandle);
    if (orbit == nullptr)
    {
        GetLogger()->error("Could not load sampled trajectory from '{}'\n", *sourceName);
    }

    return orbit;
}


/** Create a new FixedPosition trajectory.
 *
 * A FixedPosition is a property list with one of the following 3-vector properties:
 *
 * - \c Rectangular
 * - \c Planetographic
 * - \c Planetocentric
 *
 * Planetographic and planetocentric coordinates are given in the order longitude,
 * latitude, altitude. Units of altitude are kilometers. Planetographic and
 * and planetocentric coordinates are only practical when the coordinate system
 * is BodyFixed.
 */
static std::unique_ptr<celestia::ephem::Orbit>
CreateFixedPosition(const Hash* trajData, const Selection& centralObject, bool usePlanetUnits)
{
    double distanceScale;
    GetDefaultUnits(usePlanetUnits, distanceScale);

    Vector3d position = Vector3d::Zero();

    if (auto rectangular = trajData->getLengthVector<double>("Rectangular", 1.0, distanceScale); rectangular.has_value())
    {
        // Convert to Celestia's coordinate system
        position = Vector3d(rectangular->x(), rectangular->z(), -rectangular->y());
    }
    else if (auto planetographic = trajData->getSphericalTuple("Planetographic"); planetographic.has_value())
    {
        if (centralObject.getType() != Selection::Type_Body)
        {
            GetLogger()->error("FixedPosition planetographic coordinates aren't valid for stars.\n");
            return nullptr;
        }

        // TODO: Need function to calculate planetographic coordinates
        // TODO: Change planetocentricToCartesian so that 180 degree offset isn't required
        position = centralObject.body()->planetocentricToCartesian(180.0 + planetographic->x(),
                                                                   planetographic->y(),
                                                                   planetographic->z());
    }
    else if (auto planetocentric = trajData->getSphericalTuple("Planetocentric"); planetocentric.has_value())
    {
        if (centralObject.getType() != Selection::Type_Body)
        {
            GetLogger()->error("FixedPosition planetocentric coordinates aren't valid for stars.\n");
            return nullptr;
        }

        // TODO: Change planetocentricToCartesian so that 180 degree offset isn't required
        position = centralObject.body()->planetocentricToCartesian(180.0 + planetocentric->x(),
                                                                   planetocentric->y(),
                                                                   planetocentric->z());
    }
    else
    {
        GetLogger()->error("Missing coordinates for FixedPosition\n");
        return nullptr;
    }

    return std::make_unique<celestia::ephem::FixedOrbit>(position);
}


#ifdef USE_SPICE
/**
 * Parse a string list--either a single string or an array of strings is permitted.
 */
static bool
ParseStringList(const Hash* table,
                const string& propertyName,
                list<string>& stringList)
{
    const Value* v = table->getValue(propertyName);
    if (v == nullptr)
        return false;

    // Check for a single string first.
    if (const std::string* str = v->getString(); str != nullptr)
    {
        stringList.emplace_back(*str);
        return true;
    }
    if (const ValueArray* array = v->getArray(); array != nullptr)
    {
        ValueArray::const_iterator iter;

        // Verify that all array entries are strings

        if (std::any_of(array->begin(), array->end(), [](const Value& val) { return val.getType() != ValueType::StringType; }))
            return false;

        // Add strings to stringList
        for (const auto& val : *array)
            stringList.emplace_back(*val.getString());

        return true;
    }
    else
    {
        return false;
    }
}


/*! Create a new SPICE orbit. This is just a Celestia wrapper for a trajectory specified
 *  in a SPICE SPK file.
 *
 *  \code SpiceOrbit
 *  {
 *      Kernel <string|string array>   # optional
 *      Target <string>
 *      Origin <string>
 *      BoundingRadius <number>
 *      Period <number>                # optional
 *      Beginning <number>             # optional
 *      Ending <number>                # optional
 *  } \endcode
 *
 *  The Kernel property specifies one or more SPK files that must be loaded. Any
 *  already loaded kernels will also be used if they contain trajectories for
 *  the target or origin.
 *  Target and origin are strings that give NAIF IDs for the target and origin
 *  objects. Either names or integer IDs are valid, but integer IDs still must
 *  be quoted.
 *  BoundingRadius gives a conservative estimate of the maximum distance between
 *  the target and origin objects. It is required by Celestia for visibility
 *  culling when rendering.
 *  Beginning and Ending specify the valid time range of the SPICE orbit. It is
 *  an error to specify Beginning without Ending, and vice versa. If neither is
 *  specified, the valid range is computed from the coverage window in the SPICE
 *  kernel pool. If the coverage window is noncontiguous, the first interval is
 *  used.
 */
static std::unique_ptr<celestia::ephem::SpiceOrbit>
CreateSpiceOrbit(const Hash* orbitData,
                 const fs::path& path,
                 bool usePlanetUnits)
{
    list<string> kernelList;
    double distanceScale;
    double timeScale;

    GetDefaultUnits(usePlanetUnits, distanceScale, timeScale);

    if (orbitData->getValue("Kernel") != nullptr)
    {
        // Kernel list is optional; a SPICE orbit may rely on kernels already loaded into
        // the kernel pool.
        if (!ParseStringList(orbitData, "Kernel", kernelList))
        {
            GetLogger()->error("Kernel list for SPICE orbit is neither a string nor array of strings\n");
            return nullptr;
        }
    }

    const std::string* targetBodyName = orbitData->getString("Target");
    if (targetBodyName == nullptr)
    {
        GetLogger()->error("Target name missing from SPICE orbit\n");
        return nullptr;
    }

    const std::string* originName = orbitData->getString("Origin");
    if (originName == nullptr)
    {
        GetLogger()->error("Origin name missing from SPICE orbit\n");
        return nullptr;
    }

    // A bounding radius for culling is required for SPICE orbits
    double boundingRadius = 0.0;
    if (auto bounding = orbitData->getLength<double>("BoundingRadius", 1.0, distanceScale); bounding.has_value())
    {
        boundingRadius = *bounding;
    }
    else
    {
        GetLogger()->error("Bounding Radius missing from SPICE orbit\n");
        return nullptr;
    }

    // The period of the orbit may be specified if appropriate; a value
    // of zero for the period (the default), means that the orbit will
    // be considered aperiodic.
    auto period = orbitData->getTime<double>("Period", 1.0, timeScale).value_or(0.0);

    // Either a complete time interval must be specified with Beginning/Ending, or
    // else neither field can be present.
    const Value* beginningDate = orbitData->getValue("Beginning");
    const Value* endingDate = orbitData->getValue("Ending");
    if (beginningDate != nullptr && endingDate == nullptr)
    {
        GetLogger()->error("Beginning specified for SPICE orbit, but ending is missing.\n");
        return nullptr;
    }

    if (endingDate != nullptr && beginningDate == nullptr)
    {
        GetLogger()->error("Ending specified for SPICE orbit, but beginning is missing.\n");
        return nullptr;
    }

    std::unique_ptr<celestia::ephem::SpiceOrbit> orbit = nullptr;
    if (beginningDate != nullptr && endingDate != nullptr)
    {
        double beginningTDBJD = 0.0;
        if (!ParseDate(orbitData, "Beginning", beginningTDBJD))
        {
            GetLogger()->error("Invalid beginning date specified for SPICE orbit.\n");
            return nullptr;
        }

        double endingTDBJD = 0.0;
        if (!ParseDate(orbitData, "Ending", endingTDBJD))
        {
            GetLogger()->error("Invalid ending date specified for SPICE orbit.\n");
            return nullptr;
        }

        orbit = std::make_unique<celestia::ephem::SpiceOrbit>(*targetBodyName,
                                                              *originName,
                                                              period,
                                                              boundingRadius,
                                                              beginningTDBJD,
                                                              endingTDBJD);
    }
    else
    {
        // No time interval given; we'll use whatever coverage window is given
        // in the SPICE kernel.
        orbit = std::make_unique<celestia::ephem::SpiceOrbit>(*targetBodyName,
                                                              *originName,
                                                              period,
                                                              boundingRadius);
    }

    if (!orbit->init(path, kernelList.cbegin(), kernelList.cend()))
    {
        // Error using SPICE library; destroy the orbit; hopefully a
        // fallback is defined in the SSC file.
        orbit = nullptr;
    }

    return orbit;
}


/*! Create a new rotation model based on a SPICE frame.
 *
 *  \code SpiceRotation
 *  {
 *      Kernel <string|string array>   # optional
 *      Frame <string>
 *      BaseFrame <string>             # optional (defaults to ecliptic)
 *      Period <number>                # optional (units are hours)
 *      Beginning <number>             # optional
 *      Ending <number>                # optional
 *  } \endcode
 *
 *  The Kernel property specifies one or more SPICE kernel files that must be
 *  loaded in order for the frame to be defined over the required range. Any
 *  already loaded kernels will be used if they contain information relevant
 *  for defining the frame.
 *  Frame and base name are strings that give SPICE names for the frames. The
 *  orientation of the SpiceRotation is the orientation of the frame relative to
 *  the base frame. By default, the base frame is eclipj2000.
 *  Beginning and Ending specify the valid time range of the SPICE rotation.
 *  If the Beginning and Ending are omitted, the rotation model is assumed to
 *  be valid at any time. It is an error to specify Beginning without Ending,
 *  and vice versa.
 *  Period specifies the principal rotation period; it defaults to 0 indicating
 *  that the rotation is aperiodic. It is not essential to provide the rotation
 *  period; it is only used by Celestia for displaying object information such
 *  as sidereal day length.
 */
static std::unique_ptr<celestia::ephem::SpiceRotation>
CreateSpiceRotation(const Hash* rotationData,
                    const fs::path& path)
{
    std::list<std::string> kernelList;

    if (rotationData->getValue("Kernel") != nullptr)
    {
        // Kernel list is optional; a SPICE rotation may rely on kernels already loaded into
        // the kernel pool.
        if (!ParseStringList(rotationData, "Kernel", kernelList))
        {
            GetLogger()->warn("Kernel list for SPICE rotation is neither a string nor array of strings\n");
            return nullptr;
        }
    }

    const std::string* frameName = rotationData->getString("Frame");
    if (frameName == nullptr)
    {
        GetLogger()->error("Frame name missing from SPICE rotation\n");
        return nullptr;
    }

    std::string baseFrameName;
    if (auto baseFrame = rotationData->getString("BaseFrame"); baseFrame == nullptr)
    {
        baseFrameName = "eclipj2000";
    }
    else
    {
        baseFrameName = *baseFrame;
    }

    // The period of the rotation may be specified if appropriate; a value
    // of zero for the period (the default), means that the rotation will
    // be considered aperiodic.
    auto period = rotationData->getTime<double>("Period", 1.0, 1.0 / HOURS_PER_DAY).value_or(0.0);

    // Either a complete time interval must be specified with Beginning/Ending, or
    // else neither field can be present.
    const Value* beginningDate = rotationData->getValue("Beginning");
    const Value* endingDate = rotationData->getValue("Ending");
    if (beginningDate != nullptr && endingDate == nullptr)
    {
        GetLogger()->error("Beginning specified for SPICE rotation, but ending is missing.\n");
        return nullptr;
    }

    if (endingDate != nullptr && beginningDate == nullptr)
    {
        GetLogger()->error("Ending specified for SPICE rotation, but beginning is missing.\n");
        return nullptr;
    }

    std::unique_ptr<celestia::ephem::SpiceRotation> rotation = nullptr;
    if (beginningDate != nullptr && endingDate != nullptr)
    {
        double beginningTDBJD = 0.0;
        if (!ParseDate(rotationData, "Beginning", beginningTDBJD))
        {
            GetLogger()->error("Invalid beginning date specified for SPICE rotation.\n");
            return nullptr;
        }

        double endingTDBJD = 0.0;
        if (!ParseDate(rotationData, "Ending", endingTDBJD))
        {
            GetLogger()->error("Invalid ending date specified for SPICE rotation.\n");
            return nullptr;
        }

        rotation = std::make_unique<celestia::ephem::SpiceRotation>(*frameName,
                                                                    baseFrameName,
                                                                    period,
                                                                    beginningTDBJD,
                                                                    endingTDBJD);
    }
    else
    {
        // No time interval given; rotation is valid at any time.
        rotation = std::make_unique<celestia::ephem::SpiceRotation>(*frameName,
                                                                    baseFrameName,
                                                                    period);
    }

    if (!rotation->init(path, kernelList.cbegin(), kernelList.cend()))
    {
        // Error using SPICE library; destroy the rotation.
        rotation = nullptr;
    }

    return rotation;
}
#endif


static celestia::ephem::Orbit*
CreateScriptedOrbit(const Hash* orbitData,
                    const fs::path& path)
{
#if !defined(CELX)
    GetLogger()->warn("ScriptedOrbit not usable without scripting support.\n");
    return nullptr;
#else

    // Function name is required
    const std::string* funcName = orbitData->getString("Function");
    if (funcName == nullptr)
    {
        GetLogger()->error("Function name missing from script orbit definition.\n");
        return nullptr;
    }

    // Module name is optional
    const std::string* moduleName = orbitData->getString("Module");

    //Value* pathValue = new Value(path.string());
    //orbitData->addValue("AddonPath", *pathValue);

    return celestia::ephem::CreateScriptedOrbit(moduleName, *funcName, *orbitData, path).release();
#endif
}


celestia::ephem::Orbit*
CreateOrbit(const Selection& centralObject,
            const Hash* planetData,
            const fs::path& path,
            bool usePlanetUnits)
{
    if (const std::string* customOrbitName = planetData->getString("CustomOrbit"); customOrbitName != nullptr)
    {
        if (auto orbit = celephem::GetCustomOrbit(*customOrbitName); orbit != nullptr)
            return orbit.release();
        GetLogger()->error("Could not find custom orbit named '{}'\n", *customOrbitName);
    }

#ifdef USE_SPICE
    if (const Value* spiceOrbitDataValue = planetData->getValue("SpiceOrbit"); spiceOrbitDataValue != nullptr)
    {
        const Hash* spiceOrbitData = spiceOrbitDataValue->getHash();
        if (spiceOrbitData == nullptr)
        {
            GetLogger()->error("Object has incorrect spice orbit syntax.\n");
            return nullptr;
        }
        else
        {
            auto orbit = CreateSpiceOrbit(spiceOrbitData, path, usePlanetUnits);
            if (orbit != nullptr)
            {
                return orbit.release();
            }
            GetLogger()->error("Bad spice orbit\n");
            GetLogger()->error("Could not load SPICE orbit\n");
        }
    }
#endif

    // Trajectory calculated by Lua script
    if (const Value* scriptedOrbitValue = planetData->getValue("ScriptedOrbit"); scriptedOrbitValue != nullptr)
    {
        const Hash* scriptedOrbitData = scriptedOrbitValue->getHash();
        if (scriptedOrbitData == nullptr)
        {
            GetLogger()->error("Object has incorrect scripted orbit syntax.\n");
            return nullptr;
        }

        auto orbit = CreateScriptedOrbit(scriptedOrbitData, path);
        if (orbit != nullptr)
            return orbit;
    }

    // New 1.5.0 style for sampled trajectories. Permits specification of
    // precision and interpolation type.
    if (const Value* sampledTrajDataValue = planetData->getValue("SampledTrajectory"); sampledTrajDataValue != nullptr)
    {
        const Hash* sampledTrajData = sampledTrajDataValue->getHash();
        if (sampledTrajData == nullptr)
        {
            GetLogger()->error("Object has incorrect syntax for SampledTrajectory.\n");
            return nullptr;
        }

        return CreateSampledTrajectory(sampledTrajData, path);
    }

    // Old style for sampled trajectories. Assumes cubic interpolation and
    // single precision.
    if (const std::string* sampOrbitFile = planetData->getString("SampledOrbit"); sampOrbitFile != nullptr)
    {
        GetLogger()->verbose("Attempting to load sampled orbit file '{}'\n", *sampOrbitFile);
        ResourceHandle orbitHandle =
            GetTrajectoryManager()->getHandle(TrajectoryInfo(*sampOrbitFile,
                                                             path,
                                                             TrajectoryInterpolation::Cubic,
                                                             TrajectoryPrecision::Single));
        if (auto orbit = GetTrajectoryManager()->find(orbitHandle); orbit != nullptr)
        {
            return orbit;
        }
        GetLogger()->error("Could not load sampled orbit file '{}'\n", *sampOrbitFile);
    }

    if (const Value* orbitDataValue = planetData->getValue("EllipticalOrbit"); orbitDataValue != nullptr)
    {
        const Hash* orbitData = orbitDataValue->getHash();
        if (orbitData == nullptr)
        {
            GetLogger()->error("Object has incorrect elliptical orbit syntax.\n");
            return nullptr;
        }

        return CreateEllipticalOrbit(orbitData, usePlanetUnits).release();
    }

    // Create an 'orbit' that places the object at a fixed point in its
    // reference frame. There are two forms for FixedPosition: a simple
    // form with an 3-vector value, and complex form with a properlist
    // value. The simple form:
    //
    // FixedPosition [ x y z ]
    //
    // is a shorthand for:
    //
    // FixedPosition { Rectangular [ x y z ] }
    //
    // In addition to Rectangular, other coordinate types for fixed position are
    // Planetographic and Planetocentric.
    if (const Value* fixedPositionValue = planetData->getValue("FixedPosition"); fixedPositionValue != nullptr)
    {
        double distanceScale;
        GetDefaultUnits(usePlanetUnits, distanceScale);

        if (auto fixed = planetData->getLengthVector<double>("FixedPosition", 1.0, distanceScale); fixed.has_value())
        {
            // Convert to Celestia's coordinate system
            Eigen::Vector3d fixedPosition(fixed->x(), fixed->z(), -fixed->y());
            return new celestia::ephem::FixedOrbit(fixedPosition);
        }

        if (auto fixedPositionData = fixedPositionValue->getHash(); fixedPositionData != nullptr)
        {
            return CreateFixedPosition(fixedPositionData, centralObject, usePlanetUnits).release();
        }

        GetLogger()->error("Object has incorrect FixedPosition syntax.\n");
    }

    // LongLat will make an object fixed relative to the surface of its center
    // object. This is done by creating an orbit with a period equal to the
    // rotation rate of the parent object. A body-fixed reference frame is a
    // much better way to accomplish this.
    if (auto longlat = planetData->getSphericalTuple("LongLat"); longlat.has_value())
    {
        Body* centralBody = centralObject.body();
        if (centralBody != nullptr)
        {
            Vector3d pos = centralBody->planetocentricToCartesian(longlat->x(), longlat->y(), longlat->z());
            return new celestia::ephem::SynchronousOrbit(*centralBody, pos);
        }
        // TODO: Allow fixing objects to the surface of stars.
        return nullptr;
    }

    return nullptr;
}


static std::unique_ptr<celestia::ephem::ConstantOrientation>
CreateFixedRotationModel(double offset,
                         double inclination,
                         double ascendingNode)
{
    Quaterniond q = YRotation(-celestia::numbers::pi - offset) *
                    XRotation(-inclination) *
                    YRotation(-ascendingNode);

    return std::make_unique<celestia::ephem::ConstantOrientation>(q);
}


static std::unique_ptr<celestia::ephem::RotationModel>
CreateUniformRotationModel(const Hash* rotationData,
                           double syncRotationPeriod)
{
    // Default to synchronous rotation
    auto period = rotationData->getTime<double>("Period", 1.0, 1.0 / HOURS_PER_DAY).value_or(syncRotationPeriod);

    auto offset = degToRad(rotationData->getAngle<double>("MeridianAngle").value_or(0.0));

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    auto inclination = degToRad(rotationData->getAngle<double>("Inclination").value_or(0.0));
    auto ascendingNode = degToRad(rotationData->getAngle<double>("AscendingNode").value_or(0.0));

    // No period was specified, and the default synchronous
    // rotation period is zero, indicating that the object
    // doesn't have a periodic orbit. Default to a constant
    // orientation instead.
    if (period == 0.0)
    {
        return CreateFixedRotationModel(offset, inclination, ascendingNode);
    }
    else
    {
        return std::make_unique<celestia::ephem::UniformRotationModel>(period,
                                                                       static_cast<float>(offset),
                                                                       epoch,
                                                                       static_cast<float>(inclination),
                                                                       static_cast<float>(ascendingNode));
    }
}


static std::unique_ptr<celestia::ephem::ConstantOrientation>
CreateFixedRotationModel(const Hash* rotationData)
{
    auto offset = degToRad(rotationData->getAngle<double>("MeridianAngle").value_or(0.0));
    auto inclination = degToRad(rotationData->getAngle<double>("Inclination").value_or(0.0));
    auto ascendingNode = degToRad(rotationData->getAngle<double>("AscendingNode").value_or(0.0));

    Quaterniond q = YRotation(-celestia::numbers::pi - offset) *
                    XRotation(-inclination) *
                    YRotation(-ascendingNode);

    return std::make_unique<celestia::ephem::ConstantOrientation>(q);
}


static std::unique_ptr<celestia::ephem::ConstantOrientation>
CreateFixedAttitudeRotationModel(const Hash* rotationData)
{
    auto heading = degToRad(rotationData->getAngle<double>("Heading").value_or(0.0));
    auto tilt = degToRad(rotationData->getAngle<double>("Tilt").value_or(0.0));
    auto roll = degToRad(rotationData->getAngle<double>("Roll").value_or(0.0));

    Quaterniond q = YRotation(-celestia::numbers::pi - heading) *
                    XRotation(-tilt) *
                    ZRotation(-roll);

    return std::make_unique<celestia::ephem::ConstantOrientation>(q);
}


static std::unique_ptr<celestia::ephem::RotationModel>
CreatePrecessingRotationModel(const Hash* rotationData,
                              double syncRotationPeriod)
{
    // Default to synchronous rotation
    double period = rotationData->getTime<double>("Period", 1.0, 1.0 / HOURS_PER_DAY).value_or(syncRotationPeriod);

    auto offset = degToRad(rotationData->getAngle<double>("MeridianAngle").value_or(0.0));

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    auto inclination = degToRad(rotationData->getAngle<double>("Inclination").value_or(0.0));
    auto ascendingNode = degToRad(rotationData->getAngle<double>("AscendingNode").value_or(0.0));

    // The default value of 0 is handled specially, interpreted to indicate
    // that there's no precession.
    auto precessionPeriod = rotationData->getTime<double>("PrecessionPeriod", 1.0, DAYS_PER_YEAR).value_or(0.0);


    // No period was specified, and the default synchronous
    // rotation period is zero, indicating that the object
    // doesn't have a periodic orbit. Default to a constant
    // orientation instead.
    if (period == 0.0)
    {
        return CreateFixedRotationModel(offset, inclination, ascendingNode);
    }
    else
    {
        return std::make_unique<celestia::ephem::PrecessingRotationModel>(period,
                                                                          static_cast<float>(offset),
                                                                          epoch,
                                                                          static_cast<float>(inclination),
                                                                          static_cast<float>(ascendingNode),
                                                                          precessionPeriod);
    }
}


static std::unique_ptr<celestia::ephem::RotationModel>
CreateScriptedRotation(const Hash* rotationData,
                       const fs::path& path)
{
#if !defined(CELX)
    GetLogger()->warn("ScriptedRotation not usable without scripting support.\n");
    return nullptr;
#else

    // Function name is required
    const std::string* funcName = rotationData->getString("Function");
    if (funcName == nullptr)
    {
        GetLogger()->error("Function name missing from scripted rotation definition.\n");
        return nullptr;
    }

    // Module name is optional
    const std::string* moduleName = rotationData->getString("Module");

    //Value* pathValue = new Value(path.string());
    //rotationData->addValue("AddonPath", *pathValue);

    return celestia::ephem::CreateScriptedRotation(moduleName, *funcName, *rotationData, path);
#endif
}


/**
 * Parse rotation information. Unfortunately, Celestia didn't originally have
 * RotationModel objects, so information about the rotation of the object isn't
 * grouped into a single subobject--the ssc fields relevant for rotation just
 * appear in the top level structure.
 */
celestia::ephem::RotationModel*
CreateRotationModel(const Hash* planetData,
                    const fs::path& path,
                    double syncRotationPeriod)
{
    // If more than one rotation model is specified, the following precedence
    // is used to determine which one should be used:
    //   CustomRotation
    //   SPICE C-Kernel
    //   SampledOrientation
    //   PrecessingRotation
    //   UniformRotation
    //   legacy rotation parameters
    if (const std::string* customRotationModelName = planetData->getString("CustomRotation"); customRotationModelName != nullptr)
    {
        if (auto rotationModel = celestia::ephem::GetCustomRotationModel(*customRotationModelName);
            rotationModel != nullptr)
        {
            return rotationModel;
        }
        GetLogger()->error("Could not find custom rotation model named '{}'\n",
                           *customRotationModelName);
    }

#ifdef USE_SPICE
    if (const Value* spiceRotationDataValue = planetData->getValue("SpiceRotation"); spiceRotationDataValue != nullptr)
    {
        const Hash* spiceRotationData = spiceRotationDataValue->getHash();
        if (spiceRotationData == nullptr)
        {
            GetLogger()->error("Object has incorrect spice rotation syntax.\n");
            return nullptr;
        }
        else
        {
            if (auto rotationModel = CreateSpiceRotation(spiceRotationData, path); rotationModel != nullptr)
            {
                return rotationModel.release();
            }
            GetLogger()->error("Bad spice rotation model\nCould not load SPICE rotation model\n");
        }
    }
#endif

    if (const Value* scriptedRotationValue = planetData->getValue("ScriptedRotation"); scriptedRotationValue != nullptr)
    {
        const Hash* scriptedRotationData = scriptedRotationValue->getHash();
        if (scriptedRotationData == nullptr)
        {
            GetLogger()->error("Object has incorrect scripted rotation syntax.\n");
            return nullptr;
        }

        if (auto rotationModel = CreateScriptedRotation(scriptedRotationData, path); rotationModel != nullptr)
            return rotationModel.release();

    }

    if (const std::string* sampOrientationFile = planetData->getString("SampledOrientation"); sampOrientationFile != nullptr)
    {
        GetLogger()->verbose("Attempting to load orientation file '{}'\n", *sampOrientationFile);
        ResourceHandle orientationHandle =
            GetRotationModelManager()->getHandle(RotationModelInfo(*sampOrientationFile, path));
        if (auto rotationModel = GetRotationModelManager()->find(orientationHandle); rotationModel != nullptr)
        {
            return rotationModel;
        }

        GetLogger()->error("Could not load rotation model file '{}'\n", *sampOrientationFile);
    }

    if (const Value* precessingRotationValue = planetData->getValue("PrecessingRotation"); precessingRotationValue != nullptr)
    {
        const Hash* precessingRotationData = precessingRotationValue->getHash();
        if (precessingRotationData == nullptr)
        {
            GetLogger()->error("Object has incorrect syntax for precessing rotation.\n");
            return nullptr;
        }

        return CreatePrecessingRotationModel(precessingRotationData,
                                             syncRotationPeriod).release();
    }

    if (const Value* uniformRotationValue = planetData->getValue("UniformRotation"); uniformRotationValue != nullptr)
    {
        const Hash* uniformRotationData = uniformRotationValue->getHash();
        if (uniformRotationData == nullptr)
        {
            GetLogger()->error("Object has incorrect UniformRotation syntax.\n");
            return nullptr;
        }
        return CreateUniformRotationModel(uniformRotationData,
                                          syncRotationPeriod).release();
    }

    if (const Value* fixedRotationValue = planetData->getValue("FixedRotation"); fixedRotationValue != nullptr)
    {
        const Hash* fixedRotationData = fixedRotationValue->getHash();
        if (fixedRotationData == nullptr)
        {
            GetLogger()->error("Object has incorrect FixedRotation syntax.\n");
            return nullptr;
        }

        return CreateFixedRotationModel(fixedRotationData).release();
    }

    if (const Value* fixedAttitudeValue = planetData->getValue("FixedAttitude"); fixedAttitudeValue != nullptr)
    {
        const Hash* fixedAttitudeData = fixedAttitudeValue->getHash();
        if (fixedAttitudeData == nullptr)
        {
            GetLogger()->error("Object has incorrect FixedAttitude syntax.\n");
            return nullptr;
        }

        return CreateFixedAttitudeRotationModel(fixedAttitudeData).release();
    }

    // For backward compatibility we need to support rotation parameters
    // that appear in the main block of the object definition.
    // Default to synchronous rotation
    bool specified = false;
    double period = syncRotationPeriod;
    if (auto periodVal = planetData->getNumber<double>("RotationPeriod"); periodVal.has_value())
    {
        specified = true;
        period = *periodVal / 24.0;
    }

    float offset = 0.0f;
    if (auto offsetVal = planetData->getNumber<float>("RotationOffset"); offsetVal.has_value())
    {
        specified = true;
        offset = degToRad(*offsetVal);
    }

    double epoch = astro::J2000;
    if (ParseDate(planetData, "RotationEpoch", epoch))
    {
        specified = true;
    }

    float inclination = 0.0f;
    if (auto inclinationVal = planetData->getNumber<float>("Obliquity"); inclinationVal.has_value())
    {
        specified = true;
        inclination = degToRad(*inclinationVal);
    }

    float ascendingNode = 0.0f;
    if (auto ascendingNodeVal = planetData->getNumber<float>("EquatorAscendingNode"); ascendingNodeVal.has_value())
    {
        specified = true;
        ascendingNode = degToRad(*ascendingNodeVal);
    }

    double precessionRate = 0.0f;
    if (auto precessionVal = planetData->getNumber<double>("PrecessionRate"); precessionVal.has_value())
    {
        specified = true;
        precessionRate = *precessionVal;
    }

    if (specified)
    {
        std::unique_ptr<celestia::ephem::RotationModel> rm = nullptr;
        if (period == 0.0)
        {
            // No period was specified, and the default synchronous
            // rotation period is zero, indicating that the object
            // doesn't have a periodic orbit. Default to a constant
            // orientation instead.
            rm = CreateFixedRotationModel(offset, inclination, ascendingNode);
        }
        else if (precessionRate == 0.0)
        {
            rm = std::make_unique<celestia::ephem::UniformRotationModel>(period,
                                                                         offset,
                                                                         epoch,
                                                                         inclination,
                                                                         ascendingNode);
        }
        else
        {
            rm = std::make_unique<celestia::ephem::PrecessingRotationModel>(period,
                                                                            offset,
                                                                            epoch,
                                                                            inclination,
                                                                            ascendingNode,
                                                                            -360.0 / precessionRate);
        }

        return rm.release();
    }
    else
    {
        // No rotation fields specified
        return nullptr;
    }
}


celestia::ephem::RotationModel*
CreateDefaultRotationModel(double syncRotationPeriod)
{
    std::unique_ptr<celestia::ephem::RotationModel> rotation;
    if (syncRotationPeriod == 0.0)
    {
        // If syncRotationPeriod is 0, the orbit of the object is
        // aperiodic and we'll just return a FixedRotation.
        rotation = std::make_unique<celestia::ephem::ConstantOrientation>(Quaterniond::Identity());
    }
    else
    {
        rotation = std::make_unique<celestia::ephem::UniformRotationModel>(syncRotationPeriod,
                                                                           0.0f,
                                                                           astro::J2000,
                                                                           0.0f,
                                                                           0.0f);
    }

    return rotation.release();
}


/**
 * Get the center object of a frame definition. Return an empty selection
 * if it's missing or refers to an object that doesn't exist.
 */
static Selection
getFrameCenter(const Universe& universe, const Hash* frameData, const Selection& defaultCenter)
{
    const std::string* centerName = frameData->getString("Center");
    if (centerName == nullptr)
    {
        if (defaultCenter.empty())
            GetLogger()->warn("No center specified for reference frame.\n");
        return defaultCenter;
    }

    Selection centerObject = universe.findPath(*centerName, nullptr, 0);
    if (centerObject.empty())
    {
        GetLogger()->error("Center object '{}' of reference frame not found.\n", *centerName);
        return Selection();
    }

    // Should verify that center object is a star or planet, and
    // that it is a member of the same star system as the body in which
    // the frame will be used.

    return centerObject;
}


static BodyFixedFrame::SharedConstPtr
CreateBodyFixedFrame(const Universe& universe,
                     const Hash* frameData,
                     const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    if (center.empty())
        return nullptr;

    return shared_ptr<BodyFixedFrame>(new BodyFixedFrame(center, center));
}


static BodyMeanEquatorFrame::SharedConstPtr
CreateMeanEquatorFrame(const Universe& universe,
                       const Hash* frameData,
                       const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    if (center.empty())
        return nullptr;

    Selection obj = center;
    if (const std::string* objName = frameData->getString("Object"); objName != nullptr)
    {
        obj = universe.findPath(*objName, nullptr, 0);
        if (obj.empty())
        {
            GetLogger()->error("Object '{}' for mean equator frame not found.\n", *objName);
            return nullptr;
        }
    }

    GetLogger()->debug("CreateMeanEquatorFrame {}, {}\n", center.getName(), obj.getName());

    double freezeEpoch = 0.0;
    BodyMeanEquatorFrame *ptr;
    if (ParseDate(frameData, "Freeze", freezeEpoch))
    {
        ptr = new BodyMeanEquatorFrame(center, obj, freezeEpoch);
    }
    else
    {
        ptr = new BodyMeanEquatorFrame(center, obj);
    }
    return shared_ptr<BodyMeanEquatorFrame>(ptr);
}


/**
 * Convert a string to an axis label. Permitted axis labels are
 * x, y, z, -x, -y, and -z. +x, +y, and +z are allowed as synonyms for
 * x, y, z. Case is ignored.
 */
static int
parseAxisLabel(const std::string& label)
{
    if (compareIgnoringCase(label, "x") == 0 ||
        compareIgnoringCase(label, "+x") == 0)
    {
        return 1;
    }

    if (compareIgnoringCase(label, "y") == 0 ||
        compareIgnoringCase(label, "+y") == 0)
    {
        return 2;
    }

    if (compareIgnoringCase(label, "z") == 0 ||
        compareIgnoringCase(label, "+z") == 0)
    {
        return 3;
    }

    if (compareIgnoringCase(label, "-x") == 0)
    {
        return -1;
    }

    if (compareIgnoringCase(label, "-y") == 0)
    {
        return -2;
    }

    if (compareIgnoringCase(label, "-z") == 0)
    {
        return -3;
    }

    return 0;
}


static int
getAxis(const Hash* vectorData)
{
    const std::string* axisLabel = vectorData->getString("Axis");
    if (axisLabel == nullptr)
    {
        GetLogger()->error("Bad two-vector frame: missing axis label for vector.\n");
        return 0;
    }

    int axis = parseAxisLabel(*axisLabel);
    if (axis == 0)
    {
        GetLogger()->error("Bad two-vector frame: vector has invalid axis label.\n");
    }

    // Permute axis labels to match non-standard Celestia coordinate
    // conventions: y <- z, z <- -y
    switch (axis)
    {
    case 2:
        return -3;
    case -2:
        return 3;
    case 3:
        return 2;
    case -3:
        return -2;
    default:
        return axis;
    }

    return axis;
}


/**
 * Get the target object of a direction vector definition. Return an
 * empty selection if it's missing or refers to an object that doesn't exist.
 */
static Selection
getVectorTarget(const Universe& universe, const Hash* vectorData)
{
    const std::string* targetName = vectorData->getString("Target");
    if (targetName == nullptr)
    {
        GetLogger()->warn("Bad two-vector frame: no target specified for vector.\n");
        return Selection();
    }

    Selection targetObject = universe.findPath(*targetName, nullptr, 0);
    if (targetObject.empty())
    {
        GetLogger()->warn("Bad two-vector frame: target object '{}' of vector not found.\n",
                          *targetName);
        return Selection();
    }

    return targetObject;
}


/**
 * Get the observer object of a direction vector definition. Return an
 * empty selection if it's missing or refers to an object that doesn't exist.
 */
static Selection
getVectorObserver(const Universe& universe, const Hash* vectorData)
{
    const std::string* obsName = vectorData->getString("Observer");
    if (obsName == nullptr)
    {
        // Omission of observer is permitted; it will default to the
        // frame center.
        return Selection();
    }

    Selection obsObject = universe.findPath(*obsName, nullptr, 0);
    if (obsObject.empty())
    {
        GetLogger()->warn("Bad two-vector frame: observer object '{}' of vector not found.\n",
                          obsObject.getName());
        return Selection();
    }

    return obsObject;
}


static FrameVector*
CreateFrameVector(const Universe& universe,
                  const Selection& center,
                  const Hash* vectorData)
{
    if (const Value* value = vectorData->getValue("RelativePosition"); value != nullptr)
    {
        if (const Hash* relPosData = value->getHash(); relPosData != nullptr)
        {
            Selection observer = getVectorObserver(universe, relPosData);
            Selection target = getVectorTarget(universe, relPosData);
            // Default observer is the frame center
            if (observer.empty())
                observer = center;

            if (observer.empty() || target.empty())
                return nullptr;

            return new FrameVector(FrameVector::createRelativePositionVector(observer, target));
        }
    }

    if (const Value* value = vectorData->getValue("RelativeVelocity"); value != nullptr)
    {
        if (const Hash* relVData = value->getHash(); relVData != nullptr)
        {
            Selection observer = getVectorObserver(universe, relVData);
            Selection target = getVectorTarget(universe, relVData);
            // Default observer is the frame center
            if (observer.empty())
                observer = center;

            if (observer.empty() || target.empty())
                return nullptr;

            return new FrameVector(FrameVector::createRelativeVelocityVector(observer, target));
        }
    }

    if (const Value* value = vectorData->getValue("ConstantVector"); value != nullptr)
    {
        if (const Hash* constVecData = value->getHash(); constVecData != nullptr)
        {
            auto vec = constVecData->getVector3<double>("Vector").value_or(Vector3d::UnitZ());
            if (vec.norm() == 0.0)
            {
                GetLogger()->error("Bad two-vector frame: constant vector has length zero\n");
                return nullptr;
            }
            vec.normalize();
            vec = Vector3d(vec.x(), vec.z(), -vec.y());

            // The frame for the vector is optional; a nullptr frame indicates
            // J2000 ecliptic.
            ReferenceFrame::SharedConstPtr f;
            const Value* frameValue = constVecData->getValue("Frame");
            if (frameValue != nullptr)
            {
                f = CreateReferenceFrame(universe, frameValue, center, nullptr);
                if (f == nullptr)
                    return nullptr;
            }

            return new FrameVector(FrameVector::createConstantVector(vec, f));
        }
    }

    GetLogger()->error("Bad two-vector frame: unknown vector type\n");
    return nullptr;
}


static shared_ptr<const TwoVectorFrame>
CreateTwoVectorFrame(const Universe& universe,
                     const Hash* frameData,
                     const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    if (center.empty())
        return nullptr;

    // Primary and secondary vector definitions are required
    const Value* primaryValue = frameData->getValue("Primary");
    if (primaryValue == nullptr)
    {
        GetLogger()->error("Primary axis missing from two-vector frame.\n");
        return nullptr;
    }

    const Hash* primaryData = primaryValue->getHash();
    if (primaryData == nullptr)
    {
        GetLogger()->error("Bad syntax for primary axis of two-vector frame.\n");
        return nullptr;
    }

    const Value* secondaryValue = frameData->getValue("Secondary");
    if (secondaryValue == nullptr)
    {
        GetLogger()->error("Secondary axis missing from two-vector frame.\n");
        return nullptr;
    }

    const Hash* secondaryData = secondaryValue->getHash();
    if (secondaryData == nullptr)
    {
        GetLogger()->error("Bad syntax for secondary axis of two-vector frame.\n");
        return nullptr;
    }

    // Get and validate the axes for the direction vectors
    int primaryAxis = getAxis(primaryData);
    int secondaryAxis = getAxis(secondaryData);

    assert(abs(primaryAxis) <= 3);
    assert(abs(secondaryAxis) <= 3);
    if (primaryAxis == 0 || secondaryAxis == 0)
    {
        return nullptr;
    }

    if (abs(primaryAxis) == abs(secondaryAxis))
    {
        GetLogger()->error("Bad two-vector frame: axes for vectors are collinear.\n");
        return nullptr;
    }

    FrameVector* primaryVector = CreateFrameVector(universe,
                                                   center,
                                                   primaryData);
    FrameVector* secondaryVector = CreateFrameVector(universe,
                                                     center,
                                                     secondaryData);

    TwoVectorFrame *frame = nullptr;
    if (primaryVector != nullptr && secondaryVector != nullptr)
    {
        frame = new TwoVectorFrame(center,
                                   *primaryVector, primaryAxis,
                                   *secondaryVector, secondaryAxis);
    }

    return shared_ptr<const TwoVectorFrame>(frame);
}


static shared_ptr<const J2000EclipticFrame>
CreateJ2000EclipticFrame(const Universe& universe,
                         const Hash* frameData,
                         const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);

    if (center.empty())
        return nullptr;

    return shared_ptr<J2000EclipticFrame>(new J2000EclipticFrame(center));
}


static shared_ptr<const J2000EquatorFrame>
CreateJ2000EquatorFrame(const Universe& universe,
                        const Hash* frameData,
                        const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);

    if (center.empty())
        return nullptr;

    return shared_ptr<J2000EquatorFrame>(new J2000EquatorFrame(center));
}


/**
 * Helper function for CreateTopocentricFrame().
 * Creates a two-vector frame with the specified center, target, and observer.
 */
shared_ptr<const TwoVectorFrame>
CreateTopocentricFrame(const Selection& center,
                       const Selection& target,
                       const Selection& observer)
{
    auto eqFrame = shared_ptr<BodyMeanEquatorFrame>(new BodyMeanEquatorFrame(target, target));
    FrameVector north = FrameVector::createConstantVector(Vector3d::UnitY(), eqFrame);
    FrameVector up = FrameVector::createRelativePositionVector(observer, target);

    return shared_ptr<TwoVectorFrame>(new TwoVectorFrame(center, up, -2, north, -3));
}


/**
 * Create a new Topocentric frame. The topocentric frame is designed to make it easy
 * to place objects on the surface of a planet or moon. The z-axis will point toward
 * the observer's zenith (which here is the direction away from the center of the
 * planet.) The x-axis will point in the local north direction. The equivalent
 * two-vector frame is:
 *
 * \code TwoVector
 * {
 *    Center <center>
 *    Primary
 *    {
 *       Axis "z"
 *       RelativePosition { Target <target> Observer <observer> }
 *    }
 *    Secondary
 *    {
 *       Axis "x"
 *       ConstantVector
 *       {
 *          Vector [ 0 0 1]
 *          Frame { BodyFixed { Center <target> } }
 *       }
 *    }
 * } \endcode
 *
 * Typically, the topocentric frame is used as a BodyFrame to orient an
 * object on the surface of a planet. In this situation, the observer is
 * object itself and the target object is the planet. In fact, these are
 * the defaults: when no target, observer, or center is specified, the
 * observer and center are both 'self' and the target is the parent
 * object. Thus, for a Mars rover, using a topocentric frame is as simple
 * as:
 *
 * <pre> "Rover" "Sol/Mars"
 * {
 *     BodyFrame { Topocentric { } }
 *     ...
 * } </pre>
 */
static shared_ptr<const TwoVectorFrame>
CreateTopocentricFrame(const Universe& universe,
                       const Hash* frameData,
                       const Selection& defaultTarget,
                       const Selection& defaultObserver)
{
    Selection target;
    Selection observer;
    Selection center;

    if (const std::string* centerName = frameData->getString("Center"); centerName != nullptr)
    {
        // If a center is provided, the default observer is the center and
        // the default target is the center's parent. This gives sensible results
        // when a topocentric frame is used as an orbit frame.
        center = universe.findPath(*centerName, nullptr, 0);
        if (center.empty())
        {
            GetLogger()->error("Center object '{}' for topocentric frame not found.\n", *centerName);
            return nullptr;
       }

       observer = center;
       target = center.parent();
    }
    else
    {
        // When no center is provided, use the default observer as the center. This
        // is typical when a topocentric frame is the body frame. The default observer
        // is usually the object itself.
        target = defaultTarget;
        observer = defaultObserver;
        center = defaultObserver;
    }

    if (const std::string* targetName = frameData->getString("Target"); targetName == nullptr)
    {
        if (target.empty())
        {
            GetLogger()->error("No target specified for topocentric frame.\n");
            return nullptr;
        }
    }
    else
    {
        target = universe.findPath(*targetName, nullptr, 0);
        if (target.empty())
        {
            GetLogger()->error("Target object '{}' for topocentric frame not found.\n", *targetName);
            return nullptr;
        }

        // Should verify that center object is a star or planet, and
        // that it is a member of the same star system as the body in which
        // the frame will be used.
    }

    if (const std::string* observerName = frameData->getString("Observer"); observerName == nullptr)
    {
        if (observer.empty())
        {
            GetLogger()->error("No observer specified for topocentric frame.\n");
            return nullptr;
        }
    }
    else
    {
        observer = universe.findPath(*observerName, nullptr, 0);
        if (observer.empty())
        {
            GetLogger()->error("Observer object '{}' for topocentric frame not found.\n", *observerName);
            return nullptr;
        }
    }

    return CreateTopocentricFrame(center, target, observer);
}


static ReferenceFrame::SharedConstPtr
CreateComplexFrame(const Universe& universe, const Hash* frameData, const Selection& defaultCenter, Body* defaultObserver)
{
    if (const Value* value = frameData->getValue("BodyFixed"); value != nullptr)
    {
        const Hash* bodyFixedData = value->getHash();
        if (bodyFixedData == nullptr)
        {
            GetLogger()->error("Object has incorrect body-fixed frame syntax.\n");
            return nullptr;
        }

        return CreateBodyFixedFrame(universe, bodyFixedData, defaultCenter);
    }

    if (const Value* value = frameData->getValue("MeanEquator"); value != nullptr)
    {
        const Hash* meanEquatorData = value->getHash();
        if (meanEquatorData == nullptr)
        {
            GetLogger()->error("Object has incorrect mean equator frame syntax.\n");
            return nullptr;
        }

        return CreateMeanEquatorFrame(universe, meanEquatorData, defaultCenter);
    }

    if (const Value* value = frameData->getValue("TwoVector"); value != nullptr)
    {
        const Hash* twoVectorData = value->getHash();
        if (twoVectorData == nullptr)
        {
            GetLogger()->error("Object has incorrect two-vector frame syntax.\n");
            return nullptr;
        }

       return CreateTwoVectorFrame(universe, twoVectorData, defaultCenter);
    }

    if (const Value* value = frameData->getValue("Topocentric"); value != nullptr)
    {
        const Hash* topocentricData = value->getHash();
        if (topocentricData == nullptr)
        {
            GetLogger()->error("Object has incorrect topocentric frame syntax.\n");
            return nullptr;
        }

        return CreateTopocentricFrame(universe, topocentricData, defaultCenter, Selection(defaultObserver));
    }

    if (const Value* value = frameData->getValue("EclipticJ2000"); value != nullptr)
    {
        const Hash* eclipticData = value->getHash();
        if (eclipticData == nullptr)
        {
            GetLogger()->error("Object has incorrect J2000 ecliptic frame syntax.\n");
            return nullptr;
        }

        return CreateJ2000EclipticFrame(universe, eclipticData, defaultCenter);
    }

    if (const Value* value = frameData->getValue("EquatorJ2000"); value != nullptr)
    {
        const Hash* equatorData = value->getHash();
        if (equatorData == nullptr)
        {
            GetLogger()->error("Object has incorrect J2000 equator frame syntax.\n");
            return nullptr;
        }

        return CreateJ2000EquatorFrame(universe, equatorData, defaultCenter);
    }

    GetLogger()->error("Frame definition does not have a valid frame type.\n");

    return nullptr;
}


ReferenceFrame::SharedConstPtr CreateReferenceFrame(const Universe& universe,
                                                    const Value* frameValue,
                                                    const Selection& defaultCenter,
                                                    Body* defaultObserver)
{
    // TODO: handle named frames

    const Hash* frameData = frameValue->getHash();
    if (frameData == nullptr)
    {
        GetLogger()->error("Invalid syntax for frame definition.\n");
        return nullptr;
    }

    return CreateComplexFrame(universe, frameData, defaultCenter, defaultObserver);
}
