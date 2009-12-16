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
#include "frame.h"
#include "trajmanager.h"
#include "rotationmanager.h"
#include "universe.h"
#include "eigenport.h"
#include <celephem/customorbit.h>
#include <celephem/customrotation.h>
#include <celephem/spiceorbit.h>
#include <celephem/spicerotation.h>
#include <celephem/scriptorbit.h>
#include <celephem/scriptrotation.h>
#include <celmath/geomutil.h>
#include <celutil/debug.h>
#include <cassert>

using namespace Eigen;
using namespace std;


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
        distanceScale = KM_PER_AU;
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
    distanceScale = (usePlanetUnits) ? KM_PER_AU : 1.0;
}


bool
ParseDate(Hash* hash, const string& name, double& jd)
{
    // Check first for a number value representing a Julian date
    if (hash->getNumber(name, jd))
        return true;

    string dateString;
    if (hash->getString(name, dateString))
    {
        astro::Date date(1, 1, 1);
        if (astro::parseDate(dateString, date))
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
static EllipticalOrbit*
CreateEllipticalOrbit(Hash* orbitData,
                      bool usePlanetUnits)
{

    // default units for planets are AU and years, otherwise km and days
    
    double distanceScale;
    double timeScale;
    GetDefaultUnits(usePlanetUnits, distanceScale, timeScale);

    // SemiMajorAxis and Period are absolutely required; everything
    // else has a reasonable default.
    double pericenterDistance = 0.0;
    double semiMajorAxis = 0.0;
    if (!orbitData->getLength("SemiMajorAxis", semiMajorAxis, 1.0, distanceScale))
    {
        if (!orbitData->getLength("PericenterDistance", pericenterDistance, 1.0, distanceScale))
        {
            clog << "SemiMajorAxis/PericenterDistance missing!  Skipping planet . . .\n";
            return NULL;
        }
    }
    
    double period = 0.0;
    if (!orbitData->getTime("Period", period, 1.0, timeScale))
    {
        clog << "Period missing!  Skipping planet . . .\n";
        return NULL;
    }
    
    double eccentricity = 0.0;
    orbitData->getNumber("Eccentricity", eccentricity);

    double inclination = 0.0;
    orbitData->getAngle("Inclination", inclination);
    
    double ascendingNode = 0.0;
    orbitData->getAngle("AscendingNode", ascendingNode);
    
    double argOfPericenter = 0.0;
    if (!orbitData->getAngle("ArgOfPericenter", argOfPericenter))
    {
        double longOfPericenter = 0.0;
        if (orbitData->getAngle("LongOfPericenter", longOfPericenter))
        {
            argOfPericenter = longOfPericenter - ascendingNode;
        }
    }

    double epoch = astro::J2000;
    ParseDate(orbitData, "Epoch", epoch);

    // Accept either the mean anomaly or mean longitude--use mean anomaly
    // if both are specified.
    double anomalyAtEpoch = 0.0;
    if (!orbitData->getAngle("MeanAnomaly", anomalyAtEpoch))
    {
        double longAtEpoch = 0.0;
        if (orbitData->getAngle("MeanLongitude", longAtEpoch))
        {
            anomalyAtEpoch = longAtEpoch - (argOfPericenter + ascendingNode);
        }
    }

    // If we read the semi-major axis, use it to compute the pericenter
    // distance.
    if (semiMajorAxis != 0.0)
        pericenterDistance = semiMajorAxis * (1.0 - eccentricity);

    return new EllipticalOrbit(pericenterDistance,
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
static Orbit*
CreateSampledTrajectory(Hash* trajData, const string& path)
{
    string sourceName;
    if (!trajData->getString("Source", sourceName))
    {
        clog << "SampledTrajectory is missing a source.\n";
        return NULL;
    }

    // Read interpolation type; string value must be either "Linear" or "Cubic"
    // Default interpolation type is cubic.
    string interpolationString;
    TrajectoryInterpolation interpolation = TrajectoryInterpolationCubic;
    if (trajData->getString("Interpolation", interpolationString))
    {
        if (!compareIgnoringCase(interpolationString, "linear"))
            interpolation = TrajectoryInterpolationLinear;
        else if (!compareIgnoringCase(interpolationString, "cubic"))
            interpolation = TrajectoryInterpolationCubic;
        else
            clog << "Unknown interpolation type " << interpolationString << endl; // non-fatal error
    }

    // Double precision is true by default
    bool useDoublePrecision = true;
    trajData->getBoolean("DoublePrecision", useDoublePrecision);
    TrajectoryPrecision precision = useDoublePrecision ? TrajectoryPrecisionDouble : TrajectoryPrecisionSingle;

    DPRINTF(1, "Attempting to load sampled trajectory from source '%s'\n", sourceName.c_str());
    ResourceHandle orbitHandle = GetTrajectoryManager()->getHandle(TrajectoryInfo(sourceName, path, interpolation, precision));
    Orbit* orbit = GetTrajectoryManager()->find(orbitHandle);
    if (orbit == NULL)
    {
        clog << "Could not load sampled trajectory from '" << sourceName << "'\n";
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
static Orbit*
CreateFixedPosition(Hash* trajData, const Selection& centralObject, bool usePlanetUnits)
{
    double distanceScale;
    GetDefaultUnits(usePlanetUnits, distanceScale);
    
    Vector3d position = Vector3d::Zero();

    Vector3d v = Vector3d::Zero();    
    if (trajData->getLengthVector("Rectangular", v, 1.0, distanceScale))
    {
        // Convert to Celestia's coordinate system
        position = Vector3d(v.x(), v.z(), -v.y());
    }
    else if (trajData->getSphericalTuple("Planetographic", v))
    {
        if (centralObject.getType() != Selection::Type_Body)
        {
            clog << "FixedPosition planetographic coordinates aren't valid for stars.\n";
            return NULL;
        }

        // TODO: Need function to calculate planetographic coordinates
        // TODO: Change planetocentricToCartesian so that 180 degree offset isn't required
        position = centralObject.body()->planetocentricToCartesian(180.0 + v.x(), v.y(), v.z());
    }
    else if (trajData->getSphericalTuple("Planetocentric", v))
    {
        if (centralObject.getType() != Selection::Type_Body)
        {
            clog << "FixedPosition planetocentric coordinates aren't valid for stars.\n";
            return NULL;
        }

        // TODO: Change planetocentricToCartesian so that 180 degree offset isn't required
        position = centralObject.body()->planetocentricToCartesian(180.0 + v.x(), v.y(), v.z());
    }
    else
    {
        clog << "Missing coordinates for FixedPosition\n";
        return NULL;
    }

    return new FixedOrbit(position);
}


/**
 * Parse a string list--either a single string or an array of strings is permitted.
 */
static bool
ParseStringList(Hash* table,
				const string& propertyName,
				list<string>& stringList)
{
	Value* v = table->getValue(propertyName);
	if (v == NULL)
		return NULL;

	// Check for a single string first.
	if (v->getType() == Value::StringType)
	{
		stringList.push_back(v->getString());
		return true;
	}
	else if (v->getType() == Value::ArrayType)
	{
		Array* array = v->getArray();
		Array::const_iterator iter;

		// Verify that all array entries are strings
		for (iter = array->begin(); iter != array->end(); iter++)
		{
			if ((*iter)->getType() != Value::StringType)
				return false;
		}

		// Add strings to stringList
		for (iter = array->begin(); iter != array->end(); iter++)
			 stringList.push_back((*iter)->getString());

		return true;
	}
	else
	{
		return false;
	}
}


#ifdef USE_SPICE

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
static SpiceOrbit*
CreateSpiceOrbit(Hash* orbitData,
                 const string& path,
                 bool usePlanetUnits)
{
    string targetBodyName;
    string originName;
	list<string> kernelList;
    double distanceScale;
    double timeScale;
    
    GetDefaultUnits(usePlanetUnits, distanceScale, timeScale);
    
	if (orbitData->getValue("Kernel") != NULL)
	{
		// Kernel list is optional; a SPICE orbit may rely on kernels already loaded into
		// the kernel pool.
		if (!ParseStringList(orbitData, "Kernel", kernelList))
		{
			clog << "Kernel list for SPICE orbit is neither a string nor array of strings\n";
			return NULL;
		}
	}

    if (!orbitData->getString("Target", targetBodyName))
    {
        clog << "Target name missing from SPICE orbit\n";
        return NULL;
    }

    if (!orbitData->getString("Origin", originName))
    {
        clog << "Origin name missing from SPICE orbit\n";
        return NULL;
    }

    // A bounding radius for culling is required for SPICE orbits
    double boundingRadius = 0.0;
    if (!orbitData->getLength("BoundingRadius", boundingRadius, 1.0, distanceScale))
    {
        clog << "Bounding Radius missing from SPICE orbit\n";
        return NULL;
    }

    // The period of the orbit may be specified if appropriate; a value
    // of zero for the period (the default), means that the orbit will
    // be considered aperiodic.
    double period = 0.0;
    orbitData->getTime("Period", period, 1.0, timeScale);

	// Either a complete time interval must be specified with Beginning/Ending, or
	// else neither field can be present.
	Value* beginningDate = orbitData->getValue("Beginning");
	Value* endingDate = orbitData->getValue("Ending");
	if (beginningDate != NULL && endingDate == NULL)
	{
		clog << "Beginning specified for SPICE orbit, but ending is missing.\n";
		return NULL;
	}

	if (endingDate != NULL && beginningDate == NULL)
	{
		clog << "Ending specified for SPICE orbit, but beginning is missing.\n";
		return NULL;
	}

	SpiceOrbit* orbit = NULL;
	if (beginningDate != NULL && endingDate != NULL)
	{
		double beginningTDBJD = 0.0;
		if (!ParseDate(orbitData, "Beginning", beginningTDBJD))
		{
			clog << "Invalid beginning date specified for SPICE orbit.\n";
			return NULL;
		}

		double endingTDBJD = 0.0;
		if (!ParseDate(orbitData, "Ending", endingTDBJD))
		{
			clog << "Invalid ending date specified for SPICE orbit.\n";
			return NULL;
		}
	
		orbit = new SpiceOrbit(targetBodyName,
							   originName,
							   period,
							   boundingRadius,
							   beginningTDBJD,
							   endingTDBJD);
	}
	else
	{
		// No time interval given; we'll use whatever coverage window is given
		// in the SPICE kernel.
		orbit = new SpiceOrbit(targetBodyName,
							   originName,
							   period,
							   boundingRadius);
	}

    if (!orbit->init(path, &kernelList))
    {
        // Error using SPICE library; destroy the orbit; hopefully a
        // fallback is defined in the SSC file.
        delete orbit;
        orbit = NULL;
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
static SpiceRotation*
CreateSpiceRotation(Hash* rotationData,
                    const string& path)
{
    string frameName;
    string baseFrameName = "eclipj2000";
	list<string> kernelList;

	if (rotationData->getValue("Kernel") != NULL)
	{
		// Kernel list is optional; a SPICE rotation may rely on kernels already loaded into
		// the kernel pool.
		if (!ParseStringList(rotationData, "Kernel", kernelList))
		{
			clog << "Kernel list for SPICE rotation is neither a string nor array of strings\n";
			return NULL;
		}
	}

    if (!rotationData->getString("Frame", frameName))
    {
        clog << "Frame name missing from SPICE rotation\n";
        return NULL;
    }

    rotationData->getString("BaseFrame", baseFrameName);

    // The period of the rotation may be specified if appropriate; a value
    // of zero for the period (the default), means that the rotation will
    // be considered aperiodic.
    double period = 0.0;
    rotationData->getTime("Period", period, 1.0, 1.0 / HOURS_PER_DAY);

	// Either a complete time interval must be specified with Beginning/Ending, or
	// else neither field can be present.
	Value* beginningDate = rotationData->getValue("Beginning");
	Value* endingDate = rotationData->getValue("Ending");
	if (beginningDate != NULL && endingDate == NULL)
	{
		clog << "Beginning specified for SPICE rotation, but ending is missing.\n";
		return NULL;
	}

	if (endingDate != NULL && beginningDate == NULL)
	{
		clog << "Ending specified for SPICE rotation, but beginning is missing.\n";
		return NULL;
	}

	SpiceRotation* rotation = NULL;
	if (beginningDate != NULL && endingDate != NULL)
	{
		double beginningTDBJD = 0.0;
		if (!ParseDate(rotationData, "Beginning", beginningTDBJD))
		{
			clog << "Invalid beginning date specified for SPICE rotation.\n";
			return NULL;
		}

		double endingTDBJD = 0.0;
		if (!ParseDate(rotationData, "Ending", endingTDBJD))
		{
			clog << "Invalid ending date specified for SPICE rotation.\n";
			return NULL;
		}
	
		rotation = new SpiceRotation(frameName,
				  			         baseFrameName,
							         period,
							         beginningTDBJD,
							         endingTDBJD);
	}
	else
	{
		// No time interval given; rotation is valid at any time.
		rotation = new SpiceRotation(frameName,
                                     baseFrameName,
                                     period);
	}

    if (!rotation->init(path, &kernelList))
    {
        // Error using SPICE library; destroy the rotation.
        delete rotation;
        rotation = NULL;
    }

    return rotation;
}
#endif


static ScriptedOrbit*
CreateScriptedOrbit(Hash* orbitData,
                    const string& path)
{
#if !defined(CELX)
    clog << "ScriptedOrbit not usable without scripting support.\n";
    return NULL;
#else

    // Function name is required
    string funcName;
    if (!orbitData->getString("Function", funcName))
    {
        clog << "Function name missing from script orbit definition.\n";
        return NULL;
    }

    // Module name is optional
    string moduleName;
    orbitData->getString("Module", moduleName);

    string* pathCopy = new string(path);
    Value* pathValue = new Value(*pathCopy);
    orbitData->addValue("AddonPath", *pathValue);

    ScriptedOrbit* scriptedOrbit = new ScriptedOrbit();
    if (scriptedOrbit != NULL)
    {
        if (!scriptedOrbit->initialize(moduleName, funcName, orbitData))
        {
            delete scriptedOrbit;
            scriptedOrbit = NULL;
        }
    }

    return scriptedOrbit;
#endif
}


Orbit*
CreateOrbit(const Selection& centralObject,
            Hash* planetData,
            const string& path,
            bool usePlanetUnits)
{
    Orbit* orbit = NULL;

    string customOrbitName;
    if (planetData->getString("CustomOrbit", customOrbitName))
    {
        orbit = GetCustomOrbit(customOrbitName);
        if (orbit != NULL)
        {
            return orbit;
        }
        clog << "Could not find custom orbit named '" << customOrbitName <<
            "'\n";
    }

#ifdef USE_SPICE
    Value* spiceOrbitDataValue = planetData->getValue("SpiceOrbit");
    if (spiceOrbitDataValue != NULL)
    {
        if (spiceOrbitDataValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect spice orbit syntax.\n";
            return NULL;
        }
        else
        {
            orbit = CreateSpiceOrbit(spiceOrbitDataValue->getHash(), path, usePlanetUnits);
            if (orbit != NULL)
            {
                return orbit;
            }
            clog << "Bad spice orbit\n";
            DPRINTF(0, "Could not load SPICE orbit\n");
        }
    }
#endif

    // Trajectory calculated by Lua script
    Value* scriptedOrbitValue = planetData->getValue("ScriptedOrbit");
    if (scriptedOrbitValue != NULL)
    {
        if (scriptedOrbitValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect scripted orbit syntax.\n";
            return NULL;
        }
        else
        {
            orbit = CreateScriptedOrbit(scriptedOrbitValue->getHash(), path);
            if (orbit != NULL)
                return orbit;
        }
    }

    // New 1.5.0 style for sampled trajectories. Permits specification of
    // precision and interpolation type.
    Value* sampledTrajDataValue = planetData->getValue("SampledTrajectory");
    if (sampledTrajDataValue != NULL)
    {
        if (sampledTrajDataValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect syntax for SampledTrajectory.\n";
            return NULL;
        }
        else
        {
            return CreateSampledTrajectory(sampledTrajDataValue->getHash(), path);
        }
    }

    // Old style for sampled trajectories. Assumes cubic interpolation and
    // single precision.
    string sampOrbitFile;
    if (planetData->getString("SampledOrbit", sampOrbitFile))
    {
        DPRINTF(1, "Attempting to load sampled orbit file '%s'\n",
                sampOrbitFile.c_str());
        ResourceHandle orbitHandle =
            GetTrajectoryManager()->getHandle(TrajectoryInfo(sampOrbitFile,
                                                             path,
                                                             TrajectoryInterpolationCubic,
                                                             TrajectoryPrecisionSingle));
        orbit = GetTrajectoryManager()->find(orbitHandle);
        if (orbit != NULL)
        {
            return orbit;
        }
        clog << "Could not load sampled orbit file '" << sampOrbitFile << "'\n";
    }

    Value* orbitDataValue = planetData->getValue("EllipticalOrbit");
    if (orbitDataValue != NULL)
    {
        if (orbitDataValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect elliptical orbit syntax.\n";
            return NULL;
        }
        else
        {
            return CreateEllipticalOrbit(orbitDataValue->getHash(),
                                         usePlanetUnits);
        }
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
    Value* fixedPositionValue = planetData->getValue("FixedPosition");
    if (fixedPositionValue != NULL)
    {
        Vector3d fixedPosition = Vector3d::Zero();
        double distanceScale;
        GetDefaultUnits(usePlanetUnits, distanceScale);

        if (planetData->getLengthVector("FixedPosition", fixedPosition, 1.0, distanceScale))
        {
            // Convert to Celestia's coordinate system
            fixedPosition = Vector3d(fixedPosition.x(),
                                     fixedPosition.z(),
                                     -fixedPosition.y());
            
            return new FixedOrbit(fixedPosition);
        }
        else if (fixedPositionValue->getType() == Value::HashType)
        {
            return CreateFixedPosition(fixedPositionValue->getHash(), centralObject, usePlanetUnits);
        }
        else
        {
            clog << "Object has incorrect FixedPosition syntax.\n";
        }
    }

    // LongLat will make an object fixed relative to the surface of its center
    // object. This is done by creating an orbit with a period equal to the
    // rotation rate of the parent object. A body-fixed reference frame is a
    // much better way to accomplish this.
    Vector3d longlat = Vector3d::Zero();
    if (planetData->getSphericalTuple("LongLat", longlat))
    {
        Body* centralBody = centralObject.body();
        if (centralBody != NULL)
        {
            Vector3d pos = centralBody->planetocentricToCartesian(longlat.x(), longlat.y(), longlat.z());
            return new SynchronousOrbit(*centralBody, pos);
        }
        else
        {
            // TODO: Allow fixing objects to the surface of stars.
        }
        return NULL;
    }

    return NULL;
}


static ConstantOrientation*
CreateFixedRotationModel(double offset,
                         double inclination,
                         double ascendingNode)
{
    Quaterniond q = YRotation(-PI - offset) *
                    XRotation(-inclination) *
                    YRotation(-ascendingNode);

    return new ConstantOrientation(q);
}


static RotationModel*
CreateUniformRotationModel(Hash* rotationData,
                           double syncRotationPeriod)
{
    // Default to synchronous rotation
    double period = syncRotationPeriod;
    rotationData->getTime("Period", period, 1.0, 1.0 / HOURS_PER_DAY);

    float offset = 0.0f;
    if (rotationData->getAngle("MeridianAngle", offset))
    {
        offset = degToRad(offset);
    }

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    float inclination = 0.0f;
    if (rotationData->getAngle("Inclination", inclination))
    {
        inclination = degToRad(inclination);
    }

    float ascendingNode = 0.0f;
    if (rotationData->getAngle("AscendingNode", ascendingNode))
    {
        ascendingNode = degToRad(ascendingNode);
    }

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
        return new UniformRotationModel(period,
                                        offset,
                                        epoch,
                                        inclination,
                                        ascendingNode);
    }
}


static ConstantOrientation*
CreateFixedRotationModel(Hash* rotationData)
{
    double offset = 0.0;
    if (rotationData->getAngle("MeridianAngle", offset))
    {
        offset = degToRad(offset);
    }

    double inclination = 0.0;
    if (rotationData->getAngle("Inclination", inclination))
    {
        inclination = degToRad(inclination);
    }

    double ascendingNode = 0.0;
    if (rotationData->getAngle("AscendingNode", ascendingNode))
    {
        ascendingNode = degToRad(ascendingNode);
    }

    Quaterniond q = YRotation(-PI - offset) *
                    XRotation(-inclination) *
                    YRotation(-ascendingNode);

    return new ConstantOrientation(q);
}


static ConstantOrientation*
CreateFixedAttitudeRotationModel(Hash* rotationData)
{
    double heading = 0.0;
    if (rotationData->getAngle("Heading", heading))
    {
        heading = degToRad(heading);
    }
    
    double tilt = 0.0;
    if (rotationData->getAngle("Tilt", tilt))
    {
        tilt = degToRad(tilt);
    }
    
    double roll = 0.0;
    if (rotationData->getAngle("Roll", roll))
    {
        roll = degToRad(roll);
    }
    
    Quaterniond q = YRotation(-PI - heading) *
                    XRotation(-tilt) *
                    ZRotation(-roll);

    return new ConstantOrientation(q);
}


static RotationModel*
CreatePrecessingRotationModel(Hash* rotationData,
                              double syncRotationPeriod)
{
    // Default to synchronous rotation
    double period = syncRotationPeriod;
    rotationData->getTime("Period", period, 1.0, 1.0 / HOURS_PER_DAY);

    float offset = 0.0f;
    if (rotationData->getAngle("MeridianAngle", offset))
    {
        offset = degToRad(offset);
    }

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    float inclination = 0.0f;
    if (rotationData->getAngle("Inclination", inclination))
    {
        inclination = degToRad(inclination);
    }

    float ascendingNode = 0.0f;
    if (rotationData->getAngle("AscendingNode", ascendingNode))
    {
        ascendingNode = degToRad(ascendingNode);
    }

    // The default value of 0 is handled specially, interpreted to indicate
    // that there's no precession.
    double precessionPeriod = 0.0;
    rotationData->getTime("PrecessionPeriod", precessionPeriod, 1.0, DAYS_PER_YEAR);

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
        return new PrecessingRotationModel(period,
                                           offset,
                                           epoch,
                                           inclination,
                                           ascendingNode,
                                           precessionPeriod);
    }
}


static ScriptedRotation*
CreateScriptedRotation(Hash* rotationData,
                       const string& path)
{
#if !defined(CELX)
    clog << "ScriptedRotation not usable without scripting support.\n";
    return NULL;
#else

    // Function name is required
    string funcName;
    if (!rotationData->getString("Function", funcName))
    {
        clog << "Function name missing from scripted rotation definition.\n";
        return NULL;
    }

    // Module name is optional
    string moduleName;
    rotationData->getString("Module", moduleName);

    string* pathCopy = new string(path);
    Value* pathValue = new Value(*pathCopy);
    rotationData->addValue("AddonPath", *pathValue);

    ScriptedRotation* scriptedRotation = new ScriptedRotation();
    if (scriptedRotation != NULL)
    {
        if (!scriptedRotation->initialize(moduleName, funcName, rotationData))
        {
            delete scriptedRotation;
            scriptedRotation = NULL;
        }
    }

    return scriptedRotation;
#endif
}


/**
 * Parse rotation information. Unfortunately, Celestia didn't originally have
 * RotationModel objects, so information about the rotation of the object isn't
 * grouped into a single subobject--the ssc fields relevant for rotation just
 * appear in the top level structure.
 */
RotationModel*
CreateRotationModel(Hash* planetData,
                    const string& path,
                    double syncRotationPeriod)
{
    RotationModel* rotationModel = NULL;

    // If more than one rotation model is specified, the following precedence
    // is used to determine which one should be used:
    //   CustomRotation
    //   SPICE C-Kernel
    //   SampledOrientation
    //   PrecessingRotation
    //   UniformRotation
    //   legacy rotation parameters

    string customRotationModelName;
    if (planetData->getString("CustomRotation", customRotationModelName))
    {
        rotationModel = GetCustomRotationModel(customRotationModelName);
        if (rotationModel != NULL)
        {
            return rotationModel;
        }
        clog << "Could not find custom rotation model named '" <<
            customRotationModelName << "'\n";
    }

#ifdef USE_SPICE
    Value* spiceRotationDataValue = planetData->getValue("SpiceRotation");
    if (spiceRotationDataValue != NULL)
    {
        if (spiceRotationDataValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect spice rotation syntax.\n";
            return NULL;
        }
        else
        {
            rotationModel = CreateSpiceRotation(spiceRotationDataValue->getHash(), path);
            if (rotationModel != NULL)
            {
                return rotationModel;
            }
            clog << "Bad spice rotation model\n";
            DPRINTF(0, "Could not load SPICE rotation model\n");
        }
    }
#endif

    Value* scriptedRotationValue = planetData->getValue("ScriptedRotation");
    if (scriptedRotationValue != NULL)
    {
        if (scriptedRotationValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect scripted rotation syntax.\n";
            return NULL;
        }
        else
        {
            rotationModel = CreateScriptedRotation(scriptedRotationValue->getHash(), path);
            if (rotationModel != NULL)
                return rotationModel;
        }
    }

    string sampOrientationFile;
    if (planetData->getString("SampledOrientation", sampOrientationFile))
    {
        DPRINTF(1, "Attempting to load orientation file '%s'\n",
                sampOrientationFile.c_str());
        ResourceHandle orientationHandle =
            GetRotationModelManager()->getHandle(RotationModelInfo(sampOrientationFile, path));
        rotationModel = GetRotationModelManager()->find(orientationHandle);
        if (rotationModel != NULL)
        {
            return rotationModel;
        }

        clog << "Could not load rotation model file '" <<
            sampOrientationFile << "'\n";
    }

    Value* precessingRotationValue = planetData->getValue("PrecessingRotation");
    if (precessingRotationValue != NULL)
    {
        if (precessingRotationValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect syntax for precessing rotation.\n";
            return NULL;
        }
        else
        {
            return CreatePrecessingRotationModel(precessingRotationValue->getHash(),
                                                 syncRotationPeriod);
        }
    }

    Value* uniformRotationValue = planetData->getValue("UniformRotation");
    if (uniformRotationValue != NULL)
    {
        if (uniformRotationValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect UniformRotation syntax.\n";
            return NULL;
        }
        else
        {
            return CreateUniformRotationModel(uniformRotationValue->getHash(),
                                              syncRotationPeriod);
        }
    }

    Value* fixedRotationValue = planetData->getValue("FixedRotation");
    if (fixedRotationValue != NULL)
    {
        if (fixedRotationValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect FixedRotation syntax.\n";
            return NULL;
        }
        else
        {
            return CreateFixedRotationModel(fixedRotationValue->getHash());
        }
    }
    
    Value* fixedAttitudeValue = planetData->getValue("FixedAttitude");
    if (fixedAttitudeValue != NULL)
    {
        if (fixedAttitudeValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect FixedAttitude syntax.\n";
            return NULL;
        }
        else
        {
            return CreateFixedAttitudeRotationModel(fixedAttitudeValue->getHash());
        }
    }

    // For backward compatibility we need to support rotation parameters
    // that appear in the main block of the object definition.
    // Default to synchronous rotation
    bool specified = false;
    double period = syncRotationPeriod;
    if (planetData->getNumber("RotationPeriod", period))
    {
        specified = true;
        period = period / 24.0f;
    }

    float offset = 0.0f;
    if (planetData->getNumber("RotationOffset", offset))
    {
        specified = true;
        offset = degToRad(offset);
    }

    double epoch = astro::J2000;
    if (ParseDate(planetData, "RotationEpoch", epoch))
    {
        specified = true;
    }

    float inclination = 0.0f;
    if (planetData->getNumber("Obliquity", inclination))
    {
        specified = true;
        inclination = degToRad(inclination);
    }

    float ascendingNode = 0.0f;
    if (planetData->getNumber("EquatorAscendingNode", ascendingNode))
    {
        specified = true;
        ascendingNode = degToRad(ascendingNode);
    }

    double precessionRate = 0.0f;
    if (planetData->getNumber("PrecessionRate", precessionRate))
    {
        specified = true;
    }

    if (specified)
    {
        RotationModel* rm = NULL;
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
            rm = new UniformRotationModel(period,
                                          offset,
                                          epoch,
                                          inclination,
                                          ascendingNode);
        }
        else
        {
            rm = new PrecessingRotationModel(period,
                                             offset,
                                             epoch,
                                             inclination,
                                             ascendingNode,
                                             -360.0 / precessionRate);
        }

        return rm;
    }
    else
    {
        // No rotation fields specified
        return NULL;
    }
}


RotationModel* CreateDefaultRotationModel(double syncRotationPeriod)
{
    if (syncRotationPeriod == 0.0)
    {
        // If syncRotationPeriod is 0, the orbit of the object is
        // aperiodic and we'll just return a FixedRotation.
        return new ConstantOrientation(Quaterniond::Identity());
    }
    else
    {
        return new UniformRotationModel(syncRotationPeriod,
                                        0.0f,
                                        astro::J2000,
                                        0.0f,
                                        0.0f);
    }
}


/**
 * Get the center object of a frame definition. Return an empty selection
 * if it's missing or refers to an object that doesn't exist.
 */
static Selection
getFrameCenter(const Universe& universe, Hash* frameData, const Selection& defaultCenter)
{
    string centerName;
    if (!frameData->getString("Center", centerName))
    {
        if (defaultCenter.empty())
            cerr << "No center specified for reference frame.\n";
        return defaultCenter;
    }

    Selection centerObject = universe.findPath(centerName, NULL, 0);
    if (centerObject.empty())
    {
        cerr << "Center object '" << centerName << "' of reference frame not found.\n";
        return Selection();
    }

    // Should verify that center object is a star or planet, and
    // that it is a member of the same star system as the body in which
    // the frame will be used.

    return centerObject;
}


static BodyFixedFrame*
CreateBodyFixedFrame(const Universe& universe,
                     Hash* frameData,
                     const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    if (center.empty())
        return NULL;
    else
        return new BodyFixedFrame(center, center);
}


static BodyMeanEquatorFrame*
CreateMeanEquatorFrame(const Universe& universe,
                       Hash* frameData,
                       const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    if (center.empty())
        return NULL;

    Selection obj = center;
    string objName;
    if (frameData->getString("Object", objName))
    {
        obj = universe.findPath(objName, NULL, 0);
        if (obj.empty())
        {
            clog << "Object '" << objName << "' for mean equator frame not found.\n";
            return NULL;
        }
    }

    clog << "CreateMeanEquatorFrame " << center.getName() << ", " << obj.getName() << "\n";

    double freezeEpoch = 0.0;
    if (ParseDate(frameData, "Freeze", freezeEpoch))
    {
        return new BodyMeanEquatorFrame(center, obj, freezeEpoch);
    }
    else
    {
        return new BodyMeanEquatorFrame(center, obj);
    }
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
getAxis(Hash* vectorData)
{
    string axisLabel;
    if (!vectorData->getString("Axis", axisLabel))
    {
        DPRINTF(0, "Bad two-vector frame: missing axis label for vector.\n");
        return 0;
    }

    int axis = parseAxisLabel(axisLabel);
    if (axis == 0)
    {
        DPRINTF(0, "Bad two-vector frame: vector has invalid axis label.\n");
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
getVectorTarget(const Universe& universe, Hash* vectorData)
{
    string targetName;
    if (!vectorData->getString("Target", targetName))
    {
        clog << "Bad two-vector frame: no target specified for vector.\n";
        return Selection();
    }

    Selection targetObject = universe.findPath(targetName, NULL, 0);
    if (targetObject.empty())
    {
        clog << "Bad two-vector frame: target object '" << targetName << "' of vector not found.\n";
        return Selection();
    }

    return targetObject;
}


/**
 * Get the observer object of a direction vector definition. Return an
 * empty selection if it's missing or refers to an object that doesn't exist.
 */
static Selection
getVectorObserver(const Universe& universe, Hash* vectorData)
{
    string obsName;
    if (!vectorData->getString("Observer", obsName))
    {
        // Omission of observer is permitted; it will default to the
        // frame center.
        return Selection();
    }

    Selection obsObject = universe.findPath(obsName, NULL, 0);
    if (obsObject.empty())
    {
        clog << "Bad two-vector frame: observer object '" << obsObject.getName() << "' of vector not found.\n";
        return Selection();
    }

    return obsObject;
}


static FrameVector*
CreateFrameVector(const Universe& universe,
                  const Selection& center,
                  Hash* vectorData)
{
    Value* value = NULL;

    value = vectorData->getValue("RelativePosition");
    if (value != NULL && value->getHash() != NULL)
    {
        Hash* relPosData = value->getHash();
        Selection observer = getVectorObserver(universe, relPosData);
        Selection target = getVectorTarget(universe, relPosData);
        // Default observer is the frame center
        if (observer.empty())
            observer = center;

        if (observer.empty() || target.empty())
            return NULL;
        else
            return new FrameVector(FrameVector::createRelativePositionVector(observer, target));
    }

    value = vectorData->getValue("RelativeVelocity");
    if (value != NULL && value->getHash() != NULL)
    {
        Hash* relVData = value->getHash();
        Selection observer = getVectorObserver(universe, relVData);
        Selection target = getVectorTarget(universe, relVData);
        // Default observer is the frame center
        if (observer.empty())
            observer = center;

        if (observer.empty() || target.empty())
            return NULL;
        else
            return new FrameVector(FrameVector::createRelativeVelocityVector(observer, target));
    }

    value = vectorData->getValue("ConstantVector");
    if (value != NULL && value->getHash() != NULL)
    {
        Hash* constVecData = value->getHash();
        Vector3d vec = Vector3d::UnitZ();
        constVecData->getVector("Vector", vec);
        if (vec.norm() == 0.0)
        {
            clog << "Bad two-vector frame: constant vector has length zero\n";
            return NULL;
        }
        vec.normalize();
        vec = Vector3d(vec.x(), vec.z(), -vec.y());

        // The frame for the vector is optional; a NULL frame indicates
        // J2000 ecliptic.
        ReferenceFrame* f = NULL;
        Value* frameValue = constVecData->getValue("Frame");
        if (frameValue != NULL)
        {
            f = CreateReferenceFrame(universe, frameValue, center, NULL);
            if (f == NULL)
                return NULL;
        }

        return new FrameVector(FrameVector::createConstantVector(vec, f));
    }
    else
    {
        clog << "Bad two-vector frame: unknown vector type\n";
        return NULL;
    }
}


static TwoVectorFrame*
CreateTwoVectorFrame(const Universe& universe,
                     Hash* frameData,
                     const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    if (center.empty())
        return NULL;

    // Primary and secondary vector definitions are required
    Value* primaryValue = frameData->getValue("Primary");
    if (!primaryValue)
    {
        clog << "Primary axis missing from two-vector frame.\n";
        return NULL;
    }

    Hash* primaryData = primaryValue->getHash();
    if (!primaryData)
    {
        clog << "Bad syntax for primary axis of two-vector frame.\n";
        return NULL;
    }

    Value* secondaryValue = frameData->getValue("Secondary");
    if (!secondaryValue)
    {
        clog << "Secondary axis missing from two-vector frame.\n";
        return NULL;
    }

    Hash* secondaryData = secondaryValue->getHash();
    if (!secondaryData)
    {
        clog << "Bad syntax for secondary axis of two-vector frame.\n";
        return NULL;
    }

    // Get and validate the axes for the direction vectors
    int primaryAxis = getAxis(primaryData);
    int secondaryAxis = getAxis(secondaryData);

    assert(abs(primaryAxis) <= 3);
    assert(abs(secondaryAxis) <= 3);
    if (primaryAxis == 0 || secondaryAxis == 0)
    {
        return NULL;
    }

    if (abs(primaryAxis) == abs(secondaryAxis))
    {
        clog << "Bad two-vector frame: axes for vectors are collinear.\n";
        return NULL;
    }

    FrameVector* primaryVector = CreateFrameVector(universe,
                                                   center,
                                                   primaryData);
    FrameVector* secondaryVector = CreateFrameVector(universe,
                                                     center,
                                                     secondaryData);

    TwoVectorFrame* frame = NULL;
    if (primaryVector != NULL && secondaryVector != NULL)
    {
        frame = new TwoVectorFrame(center,
                                   *primaryVector, primaryAxis,
                                   *secondaryVector, secondaryAxis);
    }

    delete primaryVector;
    delete secondaryVector;

    return frame;
}


static J2000EclipticFrame*
CreateJ2000EclipticFrame(const Universe& universe,
                         Hash* frameData,
                         const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    
    if (center.empty())
        return NULL;
    else
        return new J2000EclipticFrame(center);
}


static J2000EquatorFrame*
CreateJ2000EquatorFrame(const Universe& universe,
                        Hash* frameData,
                        const Selection& defaultCenter)
{
    Selection center = getFrameCenter(universe, frameData, defaultCenter);
    
    if (center.empty())
        return NULL;
    else
        return new J2000EquatorFrame(center);
}


/**
 * Helper function for CreateTopocentricFrame().
 * Creates a two-vector frame with the specified center, target, and observer.
 */
TwoVectorFrame*
CreateTopocentricFrame(const Selection& center,
                       const Selection& target,
                       const Selection& observer)
{
    BodyMeanEquatorFrame* eqFrame = new BodyMeanEquatorFrame(target, target);
    FrameVector north = FrameVector::createConstantVector(Vector3d::UnitY(), eqFrame);
    FrameVector up = FrameVector::createRelativePositionVector(observer, target);
    
    return new TwoVectorFrame(center, up, -2, north, -3);    
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
static TwoVectorFrame*
CreateTopocentricFrame(const Universe& universe,
                       Hash* frameData,
                       const Selection& defaultTarget,
                       const Selection& defaultObserver)
{
    Selection target;
    Selection observer;
    Selection center;

    string centerName;
    if (frameData->getString("Center", centerName))
    {
        // If a center is provided, the default observer is the center and
        // the default target is the center's parent. This gives sensible results
        // when a topocentric frame is used as an orbit frame.
        center = universe.findPath(centerName, NULL, 0);
        if (center.empty())
        {
            cerr << "Center object '" << centerName << "' for topocentric frame not found.\n";
            return NULL;
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

    string targetName;
    if (!frameData->getString("Target", targetName))
    {
        if (target.empty())
        {
            cerr << "No target specified for topocentric frame.\n";
            return NULL;
        }
    }
    else
    {
        target = universe.findPath(targetName, NULL, 0);
        if (target.empty())
        {
            cerr << "Target object '" << targetName << "' for topocentric frame not found.\n";
            return NULL;
        }

        // Should verify that center object is a star or planet, and
        // that it is a member of the same star system as the body in which
        // the frame will be used.
    }

    string observerName;
    if (!frameData->getString("Observer", observerName))
    {
        if (observer.empty())
        {
            cerr << "No observer specified for topocentric frame.\n";
            return NULL;
        }
    }
    else
    {
        observer = universe.findPath(observerName, NULL, 0);
        if (observer.empty())
        {
            cerr << "Observer object '" << observerName << "' for topocentric frame not found.\n";
            return NULL;
        }
    }

    return CreateTopocentricFrame(center, target, observer);
}


static ReferenceFrame*
CreateComplexFrame(const Universe& universe, Hash* frameData, const Selection& defaultCenter, Body* defaultObserver)
{
    Value* value = frameData->getValue("BodyFixed");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect body-fixed frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateBodyFixedFrame(universe, value->getHash(), defaultCenter);
        }
    }

    value = frameData->getValue("MeanEquator");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect mean equator frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateMeanEquatorFrame(universe, value->getHash(), defaultCenter);
        }
    }

    value = frameData->getValue("TwoVector");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect two-vector frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateTwoVectorFrame(universe, value->getHash(), defaultCenter);
        }
    }

    value = frameData->getValue("Topocentric");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect topocentric frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateTopocentricFrame(universe, value->getHash(), defaultCenter, Selection(defaultObserver));
        }
    }

    value = frameData->getValue("EclipticJ2000");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect J2000 ecliptic frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateJ2000EclipticFrame(universe, value->getHash(), defaultCenter);
        }
    }

    value = frameData->getValue("EquatorJ2000");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect J2000 equator frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateJ2000EquatorFrame(universe, value->getHash(), defaultCenter);
        }
    }

    clog << "Frame definition does not have a valid frame type.\n";

    return NULL;
}


ReferenceFrame* CreateReferenceFrame(const Universe& universe,
                                     Value* frameValue,
                                     const Selection& defaultCenter,
                                     Body* defaultObserver)
{
    if (frameValue->getType() == Value::StringType)
    {
        // TODO: handle named frames
        clog << "Invalid syntax for frame definition.\n";
        return NULL;
    }
    else if (frameValue->getType() == Value::HashType)
    {
        return CreateComplexFrame(universe, frameValue->getHash(), defaultCenter, defaultObserver);
    }
    else
    {
        clog << "Invalid syntax for frame definition.\n";
        return NULL;
    }
}
