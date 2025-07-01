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

#include "parseobject.h"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include <Eigen/Core>

#include <celastro/astro.h>
#include <celastro/date.h>
#include <celcompat/numbers.h>
#include <celephem/customorbit.h>
#include <celephem/customrotation.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include <celephem/samporbit.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include <celutil/associativearray.h>
#include <celutil/fsutils.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include "body.h"
#include "rotationmanager.h"
#include "selection.h"
#include "trajmanager.h"
#include "universe.h"

#ifdef CELX
#include <celephem/scriptorbit.h>
#include <celephem/scriptrotation.h>
#endif

#ifdef USE_SPICE
#include <celephem/spiceorbit.h>
#include <celephem/spicerotation.h>
#endif

using celestia::ephem::TrajectoryInterpolation;
using celestia::ephem::TrajectoryPrecision;
using celestia::util::AssociativeArray;
using celestia::util::GetLogger;
using celestia::util::Value;
using celestia::util::ValueArray;

namespace astro = celestia::astro;
namespace engine = celestia::engine;
namespace ephem = celestia::ephem;
namespace math = celestia::math;
namespace numbers = celestia::numbers;
namespace util = celestia::util;

namespace
{

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
void
GetDefaultUnits(bool usePlanetUnits, double& distanceScale, double& timeScale)
{
    if(usePlanetUnits)
    {
        distanceScale = astro::KM_PER_AU<double>;
        timeScale = astro::DAYS_PER_YEAR;
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
void
GetDefaultUnits(bool usePlanetUnits, double& distanceScale)
{
    distanceScale = usePlanetUnits ? astro::KM_PER_AU<double> : 1.0;
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
std::shared_ptr<const ephem::Orbit>
CreateKeplerianOrbit(const AssociativeArray* orbitData,
                     bool usePlanetUnits)
{

    // default units for planets are AU and years, otherwise km and days

    double distanceScale;
    double timeScale;
    GetDefaultUnits(usePlanetUnits, distanceScale, timeScale);

    astro::KeplerElements elements;

    elements.eccentricity = orbitData->getNumber<double>("Eccentricity").value_or(0.0);
    if (elements.eccentricity < 0.0)
    {
        GetLogger()->error("Negative eccentricity is invalid.\n");
        return nullptr;
    }
    else if (elements.eccentricity == 1.0)
    {
        GetLogger()->error("Parabolic orbits are not supported.\n");
        return nullptr;
    }

    // SemiMajorAxis and Period are absolutely required; everything
    // else has a reasonable default.
    if (auto semiMajorAxisValue = orbitData->getLength<double>("SemiMajorAxis", 1.0, distanceScale); semiMajorAxisValue.has_value())
    {
        elements.semimajorAxis = *semiMajorAxisValue;
    }
    else if (auto pericenter = orbitData->getLength<double>("PericenterDistance", 1.0, distanceScale); pericenter.has_value())
    {
        elements.semimajorAxis = *pericenter / (1.0 - elements.eccentricity);
    }
    else
    {
        GetLogger()->error("SemiMajorAxis/PericenterDistance missing from orbit definition.\n");
        return nullptr;
    }

    if (auto periodValue = orbitData->getTime<double>("Period", 1.0, timeScale); periodValue.has_value())
    {
        elements.period = *periodValue;
        if (elements.period == 0.0)
        {
            GetLogger()->error("Period cannot be zero.\n");
            return nullptr;
        }
    }
    else
    {
        GetLogger()->error("Period must be specified in EllipticalOrbit.\n");
        return nullptr;
    }

    elements.inclination = orbitData->getAngle<double>("Inclination").value_or(0.0);

    elements.longAscendingNode = orbitData->getAngle<double>("AscendingNode").value_or(0.0);

    if (auto argPeri = orbitData->getAngle<double>("ArgOfPericenter"); argPeri.has_value())
    {
        elements.argPericenter = *argPeri;
    }
    else if (auto longPeri = orbitData->getAngle<double>("LongOfPericenter"); longPeri.has_value())
    {
        elements.argPericenter = *longPeri - elements.longAscendingNode;
    }

    double epoch = astro::J2000;
    ParseDate(orbitData, "Epoch", epoch);

    // Accept either the mean anomaly or mean longitude--use mean anomaly
    // if both are specified.
    if (auto meanAnomaly = orbitData->getAngle<double>("MeanAnomaly"); meanAnomaly.has_value())
        elements.meanAnomaly = *meanAnomaly;
    else if (auto meanLongitude = orbitData->getAngle<double>("MeanLongitude"); meanLongitude.has_value())
        elements.meanAnomaly = *meanLongitude - (elements.argPericenter + elements.longAscendingNode);

    elements.inclination = math::degToRad(elements.inclination);
    elements.longAscendingNode = math::degToRad(elements.longAscendingNode);
    elements.argPericenter = math::degToRad(elements.argPericenter);
    elements.meanAnomaly = math::degToRad(elements.meanAnomaly);

    if (elements.eccentricity < 1.0)
    {
        return std::make_shared<ephem::EllipticalOrbit>(elements, epoch);
    }

    return std::make_shared<ephem::HyperbolicOrbit>(elements, epoch);
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
std::shared_ptr<const ephem::Orbit>
CreateSampledTrajectory(const AssociativeArray* trajData, const std::filesystem::path& path)
{
    const std::string* source = trajData->getString("Source");
    if (source == nullptr)
    {
        GetLogger()->error("SampledTrajectory is missing a source.\n");
        return nullptr;
    }

    auto sourceFile = util::U8FileName(*source);
    if (!sourceFile.has_value())
    {
        GetLogger()->error("Invalid Source filename for SampledTrajectory\n");
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

    GetLogger()->verbose("Attempting to load sampled trajectory from source '{}'\n", *source);
    auto orbit = engine::GetTrajectoryManager()->find(*sourceFile, path, interpolation, precision);
    if (orbit == nullptr)
        GetLogger()->error("Could not load sampled trajectory from '{}'\n", *source);

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
std::shared_ptr<const ephem::Orbit>
CreateFixedPosition(const AssociativeArray* trajData, const Selection& centralObject, bool usePlanetUnits)
{
    double distanceScale;
    GetDefaultUnits(usePlanetUnits, distanceScale);

    Eigen::Vector3d position = Eigen::Vector3d::Zero();

    if (auto rectangular = trajData->getLengthVector<double>("Rectangular", 1.0, distanceScale); rectangular.has_value())
    {
        // Convert to Celestia's coordinate system
        position = Eigen::Vector3d(rectangular->x(), rectangular->z(), -rectangular->y());
    }
    else if (auto planetographic = trajData->getSphericalTuple("Planetographic"); planetographic.has_value())
    {
        if (centralObject.getType() != SelectionType::Body)
        {
            GetLogger()->error("FixedPosition planetographic coordinates are not valid for stars.\n");
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
        if (centralObject.getType() != SelectionType::Body)
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

    return std::make_shared<ephem::FixedOrbit>(position);
}

#ifdef USE_SPICE
/**
 * Parse a string list--either a single string or an array of strings is permitted.
 */
bool
ParseStringList(const AssociativeArray* table,
                std::string_view propertyName,
                std::vector<std::string>& stringList)
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
        // Verify that all array entries are strings

        if (std::any_of(array->begin(), array->end(),
                        [](const Value& val)
                        {
                            return val.getType() != util::ValueType::StringType;
                        }))
        {
            return false;
        }

        // Add strings to stringList
        for (const auto& val : *array)
            stringList.emplace_back(*val.getString());

        return true;
    }

    return false;
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
std::shared_ptr<const ephem::Orbit>
CreateSpiceOrbit(const AssociativeArray* orbitData,
                 const std::filesystem::path& path,
                 bool usePlanetUnits)
{
    std::vector<std::string> kernelList;
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

    std::shared_ptr<ephem::SpiceOrbit> orbit = nullptr;
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

        orbit = std::make_shared<ephem::SpiceOrbit>(*targetBodyName,
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
        orbit = std::make_shared<ephem::SpiceOrbit>(*targetBodyName,
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
std::shared_ptr<ephem::SpiceRotation>
CreateSpiceRotation(const Value& value,
                    const std::filesystem::path& path)
{
    const AssociativeArray* rotationData = value.getHash();
    if (rotationData == nullptr)
    {
        GetLogger()->error("Object has incorrect spice rotation syntax.\n");
        return nullptr;
    }

    std::vector<std::string> kernelList;

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
        baseFrameName = "eclipj2000";
    else
        baseFrameName = *baseFrame;

    // The period of the rotation may be specified if appropriate; a value
    // of zero for the period (the default), means that the rotation will
    // be considered aperiodic.
    auto period = rotationData->getTime<double>("Period", 1.0, 1.0 / astro::HOURS_PER_DAY).value_or(0.0);

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

    std::shared_ptr<ephem::SpiceRotation> rotation = nullptr;
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

        rotation = std::make_shared<ephem::SpiceRotation>(*frameName,
                                                          baseFrameName,
                                                          period,
                                                          beginningTDBJD,
                                                          endingTDBJD);
    }
    else
    {
        // No time interval given; rotation is valid at any time.
        rotation = std::make_shared<ephem::SpiceRotation>(*frameName,
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

std::shared_ptr<const ephem::Orbit>
CreateScriptedOrbit(const AssociativeArray* orbitData,
                    const std::filesystem::path& path)
{
#ifdef CELX
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

    return ephem::CreateScriptedOrbit(moduleName, *funcName, *orbitData, path);
#else
    GetLogger()->warn("ScriptedOrbit not usable without scripting support.\n");
    return nullptr;
#endif
}

std::shared_ptr<const ephem::ConstantOrientation>
CreateFixedRotationModel(double offset,
                         double inclination,
                         double ascendingNode)
{
    Eigen::Quaterniond q = math::YRotation(-numbers::pi - offset) *
                           math::XRotation(-inclination) *
                           math::YRotation(-ascendingNode);

    return std::make_shared<ephem::ConstantOrientation>(q);
}

std::shared_ptr<const ephem::RotationModel>
CreateUniformRotationModel(const Value& value,
                           double syncRotationPeriod)
{
    const AssociativeArray* rotationData = value.getHash();
    if (rotationData == nullptr)
    {
        GetLogger()->error("Object has incorrect UniformRotation syntax.\n");
        return nullptr;
    }

    // Default to synchronous rotation
    auto period = rotationData->getTime<double>("Period", 1.0, 1.0 / astro::HOURS_PER_DAY).value_or(syncRotationPeriod);

    auto offset = math::degToRad(rotationData->getAngle<double>("MeridianAngle").value_or(0.0));

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    auto inclination = math::degToRad(rotationData->getAngle<double>("Inclination").value_or(0.0));
    auto ascendingNode = math::degToRad(rotationData->getAngle<double>("AscendingNode").value_or(0.0));

    // No period was specified, and the default synchronous
    // rotation period is zero, indicating that the object
    // doesn't have a periodic orbit. Default to a constant
    // orientation instead.
    if (period == 0.0)
        return CreateFixedRotationModel(offset, inclination, ascendingNode);

    return std::make_shared<ephem::UniformRotationModel>(period,
                                                         static_cast<float>(offset),
                                                         epoch,
                                                         static_cast<float>(inclination),
                                                         static_cast<float>(ascendingNode));
}

std::shared_ptr<const ephem::RotationModel>
CreateFixedRotationModel(const Value& value)
{
    const AssociativeArray* rotationData = value.getHash();
    if (rotationData == nullptr)
    {
        GetLogger()->error("Object has incorrect FixedRotation syntax.\n");
        return nullptr;
    }

    auto offset = math::degToRad(rotationData->getAngle<double>("MeridianAngle").value_or(0.0));
    auto inclination = math::degToRad(rotationData->getAngle<double>("Inclination").value_or(0.0));
    auto ascendingNode = math::degToRad(rotationData->getAngle<double>("AscendingNode").value_or(0.0));

    Eigen::Quaterniond q = math::YRotation(-numbers::pi - offset) *
                           math::XRotation(-inclination) *
                           math::YRotation(-ascendingNode);

    return std::make_shared<ephem::ConstantOrientation>(q);
}

std::shared_ptr<const ephem::RotationModel>
CreateFixedAttitudeRotationModel(const Value& value)
{
    const AssociativeArray* rotationData = value.getHash();
    if (rotationData == nullptr)
    {
        GetLogger()->error("Object has incorrect FixedAttitude syntax.\n");
        return nullptr;
    }

    auto heading = math::degToRad(rotationData->getAngle<double>("Heading").value_or(0.0));
    auto tilt = math::degToRad(rotationData->getAngle<double>("Tilt").value_or(0.0));
    auto roll = math::degToRad(rotationData->getAngle<double>("Roll").value_or(0.0));

    Eigen::Quaterniond q = math::YRotation(-numbers::pi - heading) *
                           math::XRotation(-tilt) *
                           math::ZRotation(-roll);

    return std::make_shared<ephem::ConstantOrientation>(q);
}

std::shared_ptr<const ephem::RotationModel>
CreatePrecessingRotationModel(const Value& value,
                              double syncRotationPeriod)
{
    const AssociativeArray* rotationData = value.getHash();
    if (rotationData == nullptr)
    {
        GetLogger()->error("Object has incorrect syntax for precessing rotation.\n");
        return nullptr;
    }

    // Default to synchronous rotation
    double period = rotationData->getTime<double>("Period", 1.0, 1.0 / astro::HOURS_PER_DAY).value_or(syncRotationPeriod);

    auto offset = math::degToRad(rotationData->getAngle<double>("MeridianAngle").value_or(0.0));

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    auto inclination = math::degToRad(rotationData->getAngle<double>("Inclination").value_or(0.0));
    auto ascendingNode = math::degToRad(rotationData->getAngle<double>("AscendingNode").value_or(0.0));

    // The default value of 0 is handled specially, interpreted to indicate
    // that there's no precession.
    auto precessionPeriod = rotationData->getTime<double>("PrecessionPeriod", 1.0, astro::DAYS_PER_YEAR).value_or(0.0);

    // No period was specified, and the default synchronous
    // rotation period is zero, indicating that the object
    // doesn't have a periodic orbit. Default to a constant
    // orientation instead.
    if (period == 0.0)
        return CreateFixedRotationModel(offset, inclination, ascendingNode);

    return std::make_shared<ephem::PrecessingRotationModel>(period,
                                                            static_cast<float>(offset),
                                                            epoch,
                                                            static_cast<float>(inclination),
                                                            static_cast<float>(ascendingNode),
                                                            precessionPeriod);
}

#ifdef CELX

std::shared_ptr<const ephem::RotationModel>
CreateScriptedRotation(const Value& value,
                       const std::filesystem::path& path)
{
    const AssociativeArray* rotationData = value.getHash();
    if (rotationData == nullptr)
    {
        GetLogger()->error("Object has incorrect scripted rotation syntax.\n");
        return nullptr;
    }

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

    return ephem::CreateScriptedRotation(moduleName, *funcName, *rotationData, path);
}

#endif

std::shared_ptr<const ephem::RotationModel>
CreateSampledRotation(std::string_view filename, const std::filesystem::path& path)
{
    auto filePath = util::U8FileName(filename);
    if (!filePath.has_value())
    {
        GetLogger()->error("Invalid filename in SampledOrientation\n");
        return nullptr;
    }

    GetLogger()->verbose("Attempting to load orientation file '{}'\n", filename);

    auto rotationModel = engine::GetRotationModelManager()->find(*filePath, path);
    if (rotationModel == nullptr)
        GetLogger()->error("Could not load rotation model file '{}'\n", filename);

    return rotationModel;
}

/**
 * Get the center object of a frame definition. Return an empty selection
 * if it's missing or refers to an object that doesn't exist.
 */
Selection
getFrameCenter(const Universe& universe, const AssociativeArray* frameData, const Selection& defaultCenter)
{
    const std::string* centerName = frameData->getString("Center");
    if (centerName == nullptr)
    {
        if (defaultCenter.empty())
            GetLogger()->warn("No center specified for reference frame.\n");
        return defaultCenter;
    }

    Selection centerObject = universe.findPath(*centerName, {});
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

BodyFixedFrame::SharedConstPtr
CreateBodyFixedFrame(const Universe& universe,
                     const AssociativeArray* frameData,
                     const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    if (center.empty())
        return nullptr;

    return std::make_shared<BodyFixedFrame>(center, center);
}

BodyMeanEquatorFrame::SharedConstPtr
CreateMeanEquatorFrame(const Universe& universe,
                       const AssociativeArray* frameData,
                       const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    if (center.empty())
        return nullptr;

    Selection obj = center;
    if (const std::string* objName = frameData->getString("Object"); objName != nullptr)
    {
        obj = universe.findPath(*objName, {});
        if (obj.empty())
        {
            GetLogger()->error("Object '{}' for mean equator frame not found.\n", *objName);
            return nullptr;
        }
    }

    if (double freezeEpoch = 0.0; ParseDate(frameData, "Freeze", freezeEpoch))
        return std::make_shared<BodyMeanEquatorFrame>(center, obj, freezeEpoch);

    return std::make_shared<BodyMeanEquatorFrame>(center, obj);
}

/**
 * Convert a string to an axis label. Permitted axis labels are
 * x, y, z, -x, -y, and -z. +x, +y, and +z are allowed as synonyms for
 * x, y, z. Case is ignored.
 */
int
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

int
getAxis(const AssociativeArray* vectorData)
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
Selection
getVectorTarget(const Universe& universe, const AssociativeArray* vectorData)
{
    const std::string* targetName = vectorData->getString("Target");
    if (targetName == nullptr)
    {
        GetLogger()->warn("Bad two-vector frame: no target specified for vector.\n");
        return Selection();
    }

    Selection targetObject = universe.findPath(*targetName, {});
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
Selection
getVectorObserver(const Universe& universe, const AssociativeArray* vectorData)
{
    const std::string* obsName = vectorData->getString("Observer");
    if (obsName == nullptr)
    {
        // Omission of observer is permitted; it will default to the
        // frame center.
        return Selection();
    }

    Selection obsObject = universe.findPath(*obsName, {});
    if (obsObject.empty())
    {
        GetLogger()->warn("Bad two-vector frame: observer object '{}' of vector not found.\n",
                          *obsName);
        return Selection();
    }

    return obsObject;
}

std::unique_ptr<FrameVector>
CreateFrameVector(const Universe& universe,
                  const Selection& center,
                  const AssociativeArray* vectorData)
{
    if (const Value* value = vectorData->getValue("RelativePosition"); value != nullptr)
    {
        if (const AssociativeArray* relPosData = value->getHash(); relPosData != nullptr)
        {
            Selection observer = getVectorObserver(universe, relPosData);
            Selection target = getVectorTarget(universe, relPosData);
            // Default observer is the frame center
            if (observer.empty())
                observer = center;

            if (observer.empty() || target.empty())
                return nullptr;

            return std::make_unique<FrameVector>(FrameVector::createRelativePositionVector(observer, target));
        }
    }

    if (const Value* value = vectorData->getValue("RelativeVelocity"); value != nullptr)
    {
        if (const AssociativeArray* relVData = value->getHash(); relVData != nullptr)
        {
            Selection observer = getVectorObserver(universe, relVData);
            Selection target = getVectorTarget(universe, relVData);
            // Default observer is the frame center
            if (observer.empty())
                observer = center;

            if (observer.empty() || target.empty())
                return nullptr;

            return std::make_unique<FrameVector>(FrameVector::createRelativeVelocityVector(observer, target));
        }
    }

    if (const Value* value = vectorData->getValue("ConstantVector"); value != nullptr)
    {
        if (const AssociativeArray* constVecData = value->getHash(); constVecData != nullptr)
        {
            auto vec = constVecData->getVector3<double>("Vector").value_or(Eigen::Vector3d::UnitZ());
            if (vec.norm() == 0.0)
            {
                GetLogger()->error("Bad two-vector frame: constant vector has length zero\n");
                return nullptr;
            }
            vec.normalize();
            vec = Eigen::Vector3d(vec.x(), vec.z(), -vec.y());

            // The frame for the vector is optional; a nullptr frame indicates
            // J2000 ecliptic.
            ReferenceFrame::SharedConstPtr f;
            if (const Value* frameValue = constVecData->getValue("Frame"); frameValue != nullptr)
            {
                f = CreateReferenceFrame(universe, frameValue, center, nullptr);
                if (f == nullptr)
                    return nullptr;
            }

            return std::make_unique<FrameVector>(FrameVector::createConstantVector(vec, f));
        }
    }

    GetLogger()->error("Bad two-vector frame: unknown vector type\n");
    return nullptr;
}

std::shared_ptr<const TwoVectorFrame>
CreateTwoVectorFrame(const Universe& universe,
                     const AssociativeArray* frameData,
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

    const AssociativeArray* primaryData = primaryValue->getHash();
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

    const AssociativeArray* secondaryData = secondaryValue->getHash();
    if (secondaryData == nullptr)
    {
        GetLogger()->error("Bad syntax for secondary axis of two-vector frame.\n");
        return nullptr;
    }

    // Get and validate the axes for the direction vectors
    int primaryAxis = getAxis(primaryData);
    int secondaryAxis = getAxis(secondaryData);

    assert(std::abs(primaryAxis) <= 3);
    assert(std::abs(secondaryAxis) <= 3);
    if (primaryAxis == 0 || secondaryAxis == 0)
    {
        return nullptr;
    }

    if (std::abs(primaryAxis) == std::abs(secondaryAxis))
    {
        GetLogger()->error("Bad two-vector frame: axes for vectors are collinear.\n");
        return nullptr;
    }

    auto primaryVector = CreateFrameVector(universe, center, primaryData);
    auto secondaryVector = CreateFrameVector(universe, center, secondaryData);

    if (primaryVector != nullptr && secondaryVector != nullptr)
    {
        return std::make_shared<TwoVectorFrame>(center,
                                                *primaryVector,
                                                primaryAxis,
                                                *secondaryVector,
                                                secondaryAxis);
    }

    return nullptr;
}

std::shared_ptr<const J2000EclipticFrame>
CreateJ2000EclipticFrame(const Universe& universe,
                         const AssociativeArray* frameData,
                         const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);

    if (center.empty())
        return nullptr;

    return std::make_shared<J2000EclipticFrame>(center);
}

std::shared_ptr<const J2000EquatorFrame>
CreateJ2000EquatorFrame(const Universe& universe,
                        const AssociativeArray* frameData,
                        const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);

    if (center.empty())
        return nullptr;

    return std::make_shared<J2000EquatorFrame>(center);
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
std::shared_ptr<const TwoVectorFrame>
CreateTopocentricFrame(const Universe& universe,
                       const AssociativeArray* frameData,
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
        center = universe.findPath(*centerName, {});
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
        target = universe.findPath(*targetName, {});
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
        observer = universe.findPath(*observerName, {});
        if (observer.empty())
        {
            GetLogger()->error("Observer object '{}' for topocentric frame not found.\n", *observerName);
            return nullptr;
        }
    }

    return CreateTopocentricFrame(center, target, observer);
}

ReferenceFrame::SharedConstPtr
CreateComplexFrame(const Universe& universe,
                   const AssociativeArray* frameData,
                   const Selection& defaultCenter,
                   Body* defaultObserver)
{
    if (const Value* value = frameData->getValue("BodyFixed"); value != nullptr)
    {
        const AssociativeArray* bodyFixedData = value->getHash();
        if (bodyFixedData == nullptr)
        {
            GetLogger()->error("Object has incorrect body-fixed frame syntax.\n");
            return nullptr;
        }

        return CreateBodyFixedFrame(universe, bodyFixedData, defaultCenter);
    }

    if (const Value* value = frameData->getValue("MeanEquator"); value != nullptr)
    {
        const AssociativeArray* meanEquatorData = value->getHash();
        if (meanEquatorData == nullptr)
        {
            GetLogger()->error("Object has incorrect mean equator frame syntax.\n");
            return nullptr;
        }

        return CreateMeanEquatorFrame(universe, meanEquatorData, defaultCenter);
    }

    if (const Value* value = frameData->getValue("TwoVector"); value != nullptr)
    {
        const AssociativeArray* twoVectorData = value->getHash();
        if (twoVectorData == nullptr)
        {
            GetLogger()->error("Object has incorrect two-vector frame syntax.\n");
            return nullptr;
        }

       return CreateTwoVectorFrame(universe, twoVectorData, defaultCenter);
    }

    if (const Value* value = frameData->getValue("Topocentric"); value != nullptr)
    {
        const AssociativeArray* topocentricData = value->getHash();
        if (topocentricData == nullptr)
        {
            GetLogger()->error("Object has incorrect topocentric frame syntax.\n");
            return nullptr;
        }

        return CreateTopocentricFrame(universe, topocentricData, defaultCenter, Selection(defaultObserver));
    }

    if (const Value* value = frameData->getValue("EclipticJ2000"); value != nullptr)
    {
        const AssociativeArray* eclipticData = value->getHash();
        if (eclipticData == nullptr)
        {
            GetLogger()->error("Object has incorrect J2000 ecliptic frame syntax.\n");
            return nullptr;
        }

        return CreateJ2000EclipticFrame(universe, eclipticData, defaultCenter);
    }

    if (const Value* value = frameData->getValue("EquatorJ2000"); value != nullptr)
    {
        const AssociativeArray* equatorData = value->getHash();
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

} // end unnamed namespace

bool
ParseDate(const AssociativeArray* hash, std::string_view name, double& jd)
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

std::shared_ptr<const ephem::Orbit>
CreateOrbit(const Selection& centralObject,
            const AssociativeArray* planetData,
            const std::filesystem::path& path,
            bool usePlanetUnits)
{
    if (const std::string* customOrbitName = planetData->getString("CustomOrbit"); customOrbitName != nullptr)
    {
        if (auto orbit = ephem::GetCustomOrbit(*customOrbitName); orbit != nullptr)
            return orbit;
        GetLogger()->error("Could not find custom orbit named '{}'\n", *customOrbitName);
    }

    if (const Value* spiceOrbitDataValue = planetData->getValue("SpiceOrbit"); spiceOrbitDataValue != nullptr)
    {
#ifdef USE_SPICE
        const AssociativeArray* spiceOrbitData = spiceOrbitDataValue->getHash();
        if (spiceOrbitData == nullptr)
        {
            GetLogger()->error("Object has incorrect spice orbit syntax.\n");
            return nullptr;
        }
        else
        {
            auto orbit = CreateSpiceOrbit(spiceOrbitData, path, usePlanetUnits);
            if (orbit != nullptr)
                return orbit;

            GetLogger()->error("Bad spice orbit\n");
            GetLogger()->error("Could not load SPICE orbit\n");
        }
#else
        GetLogger()->warn("Spice support is not enabled, ignoring SpiceOrbit definition\n");
#endif
    }

    // Trajectory calculated by Lua script
    if (const Value* scriptedOrbitValue = planetData->getValue("ScriptedOrbit"); scriptedOrbitValue != nullptr)
    {
        const AssociativeArray* scriptedOrbitData = scriptedOrbitValue->getHash();
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
        const AssociativeArray* sampledTrajData = sampledTrajDataValue->getHash();
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
        if (auto sampOrbitFileName = util::U8FileName(*sampOrbitFile); sampOrbitFileName.has_value())
        {
            GetLogger()->verbose("Attempting to load sampled orbit file '{}'\n", *sampOrbitFile);
            if (auto orbit = engine::GetTrajectoryManager()->find(*sampOrbitFileName,
                                                                  path,
                                                                  TrajectoryInterpolation::Cubic,
                                                                  TrajectoryPrecision::Single);
                orbit != nullptr)
            {
                return orbit;
            }

            GetLogger()->error("Could not load sampled orbit file '{}'\n", *sampOrbitFile);
        }
        else
        {
            GetLogger()->error("Invalid filename in SampledOrbit\n");
        }
    }

    if (const Value* orbitDataValue = planetData->getValue("EllipticalOrbit"); orbitDataValue != nullptr)
    {
        const AssociativeArray* orbitData = orbitDataValue->getHash();
        if (orbitData == nullptr)
        {
            GetLogger()->error("Object has incorrect elliptical orbit syntax.\n");
            return nullptr;
        }

        return CreateKeplerianOrbit(orbitData, usePlanetUnits);
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
            return std::make_shared<ephem::FixedOrbit>(fixedPosition);
        }

        if (auto fixedPositionData = fixedPositionValue->getHash(); fixedPositionData != nullptr)
        {
            return CreateFixedPosition(fixedPositionData, centralObject, usePlanetUnits);
        }

        GetLogger()->error("Object has incorrect FixedPosition syntax.\n");
    }

    // LongLat will make an object fixed relative to the surface of its center
    // object. This is done by creating an orbit with a period equal to the
    // rotation rate of the parent object. A body-fixed reference frame is a
    // much better way to accomplish this.
    if (auto longlat = planetData->getSphericalTuple("LongLat"); longlat.has_value())
    {
        if (const Body* centralBody = centralObject.body(); centralBody != nullptr)
        {
#if 0 // TODO: This should be enabled after #542 is fixed
            Eigen::Vector3d pos = centralBody->geodeticToCartesian(*longlat);
#else
            Eigen::Vector3d pos = centralBody->planetocentricToCartesian(longlat->x(), longlat->y(), longlat->z());
#endif
            return std::make_shared<ephem::SynchronousOrbit>(*centralBody, pos);
        }
        // TODO: Allow fixing objects to the surface of stars.
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<const ephem::RotationModel>
CreateLegacyRotationModel(const AssociativeArray* planetData,
                          double syncRotationPeriod)
{
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
        offset = math::degToRad(*offsetVal);
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
        inclination = math::degToRad(*inclinationVal);
    }

    float ascendingNode = 0.0f;
    if (auto ascendingNodeVal = planetData->getNumber<float>("EquatorAscendingNode"); ascendingNodeVal.has_value())
    {
        specified = true;
        ascendingNode = math::degToRad(*ascendingNodeVal);
    }

    double precessionRate = 0.0f;
    if (auto precessionVal = planetData->getNumber<double>("PrecessionRate"); precessionVal.has_value())
    {
        specified = true;
        precessionRate = *precessionVal;
    }

    if (specified)
    {
        if (period == 0.0)
        {
            // No period was specified, and the default synchronous
            // rotation period is zero, indicating that the object
            // doesn't have a periodic orbit. Default to a constant
            // orientation instead.
            return CreateFixedRotationModel(offset, inclination, ascendingNode);
        }

        if (precessionRate == 0.0)
        {
            return std::make_shared<ephem::UniformRotationModel>(period,
                                                                 offset,
                                                                 epoch,
                                                                 inclination,
                                                                 ascendingNode);
        }

        return std::make_shared<ephem::PrecessingRotationModel>(period,
                                                                offset,
                                                                epoch,
                                                                inclination,
                                                                ascendingNode,
                                                                -360.0 / precessionRate);
    }
    else
    {
        // No rotation fields specified
        return nullptr;
    }

}

/**
 * Parse rotation information. Unfortunately, Celestia didn't originally have
 * RotationModel objects, so information about the rotation of the object isn't
 * grouped into a single subobject--the ssc fields relevant for rotation just
 * appear in the top level structure.
 */
std::shared_ptr<const ephem::RotationModel>
CreateRotationModel(const AssociativeArray* planetData,
                    const std::filesystem::path& path,
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
    if (const std::string* name = planetData->getString("CustomRotation"); name != nullptr)
    {
        if (auto rotationModel = ephem::GetCustomRotationModel(*name); rotationModel != nullptr)
            return rotationModel;

        GetLogger()->error("Could not find custom rotation model named '{}'\n", *name);
    }

    if (const Value* value = planetData->getValue("SpiceRotation"); value != nullptr)
    {
#ifdef USE_SPICE
        if (auto rotationModel = CreateSpiceRotation(*value, path); rotationModel != nullptr)
            return rotationModel;
#else
        GetLogger()->warn("Spice support is not enabled, ignoring SpiceRotation definition\n");
#endif
    }

    if (const Value* value = planetData->getValue("ScriptedRotation"); value != nullptr)
    {
#ifdef CELX
        if (auto rotationModel = CreateScriptedRotation(*value, path); rotationModel != nullptr)
            return rotationModel;
#else
        GetLogger()->warn("ScriptedRotation not usable without scripting support.\n");
#endif
    }

    if (const std::string* sampOrientationFile = planetData->getString("SampledOrientation"); sampOrientationFile != nullptr)
    {
        if (auto rotationModel = CreateSampledRotation(*sampOrientationFile, path); rotationModel != nullptr)
            return rotationModel;
    }

    if (const Value* value = planetData->getValue("PrecessingRotation"); value != nullptr)
    {
        if (auto rotationModel = CreatePrecessingRotationModel(*value, syncRotationPeriod); rotationModel != nullptr)
            return rotationModel;
    }

    if (const Value* value = planetData->getValue("UniformRotation"); value != nullptr)
    {
        if (auto rotationModel = CreateUniformRotationModel(*value, syncRotationPeriod); rotationModel != nullptr)
            return rotationModel;
    }

    if (const Value* value = planetData->getValue("FixedRotation"); value != nullptr)
    {
        if (auto rotationModel = CreateFixedRotationModel(*value); rotationModel != nullptr)
            return rotationModel;
    }

    if (const Value* value = planetData->getValue("FixedAttitude"); value != nullptr)
    {
        if (auto rotationModel = CreateFixedAttitudeRotationModel(*value); rotationModel != nullptr)
            return rotationModel;
    }

    // For backward compatibility we need to support rotation parameters
    // that appear in the main block of the object definition.
    return CreateLegacyRotationModel(planetData, syncRotationPeriod);
}

std::shared_ptr<const ephem::RotationModel>
CreateDefaultRotationModel(double syncRotationPeriod)
{
    if (syncRotationPeriod == 0.0)
    {
        // If syncRotationPeriod is 0, the orbit of the object is
        // aperiodic and we'll just return a FixedRotation.
        return ephem::ConstantOrientation::identity();
    }

    return std::make_shared<ephem::UniformRotationModel>(syncRotationPeriod,
                                                         0.0f,
                                                         astro::J2000,
                                                         0.0f,
                                                         0.0f);
}

/**
 * Helper function for CreateTopocentricFrame().
 * Creates a two-vector frame with the specified center, target, and observer.
 */
std::shared_ptr<const TwoVectorFrame>
CreateTopocentricFrame(const Selection& center,
                       const Selection& target,
                       const Selection& observer)
{
    auto eqFrame = std::make_shared<BodyMeanEquatorFrame>(target, target);
    FrameVector north = FrameVector::createConstantVector(Eigen::Vector3d::UnitY(), eqFrame);
    FrameVector up = FrameVector::createRelativePositionVector(observer, target);

    return std::make_shared<TwoVectorFrame>(center, up, -2, north, -3);
}

ReferenceFrame::SharedConstPtr CreateReferenceFrame(const Universe& universe,
                                                    const Value* frameValue,
                                                    const Selection& defaultCenter,
                                                    Body* defaultObserver)
{
    // TODO: handle named frames

    const AssociativeArray* frameData = frameValue->getHash();
    if (frameData == nullptr)
    {
        GetLogger()->error("Invalid syntax for frame definition.\n");
        return nullptr;
    }

    return CreateComplexFrame(universe, frameData, defaultCenter, defaultObserver);
}
