// parseobject.cpp
//
// Copyright (C) 2004 Chris Laurel <claurel@shatters.net>
//
// Functions for parsing objects common to star, solar system, and
// deep sky catalogs.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/debug.h>
#include "parseobject.h"
#include "customorbit.h"
#include "spiceorbit.h"
#include "trajmanager.h"
#include "rotationmanager.h"


using namespace std;


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


static EllipticalOrbit*
CreateEllipticalOrbit(Hash* orbitData,
                      bool usePlanetUnits)
{
    // SemiMajorAxis and Period are absolutely required; everything
    // else has a reasonable default.
    double pericenterDistance = 0.0;
    double semiMajorAxis = 0.0;
    if (!orbitData->getNumber("SemiMajorAxis", semiMajorAxis))
    {
        if (!orbitData->getNumber("PericenterDistance", pericenterDistance))
        {
            DPRINTF(0, "SemiMajorAxis/PericenterDistance missing!  Skipping planet . . .\n");
            return NULL;
        }
    }

    double period = 0.0;
    if (!orbitData->getNumber("Period", period))
    {
        DPRINTF(0, "Period missing!  Skipping planet . . .\n");
        return NULL;
    }

    double eccentricity = 0.0;
    orbitData->getNumber("Eccentricity", eccentricity);

    double inclination = 0.0;
    orbitData->getNumber("Inclination", inclination);

    double ascendingNode = 0.0;
    orbitData->getNumber("AscendingNode", ascendingNode);

    double argOfPericenter = 0.0;
    if (!orbitData->getNumber("ArgOfPericenter", argOfPericenter))
    {
        double longOfPericenter = 0.0;
        if (orbitData->getNumber("LongOfPericenter", longOfPericenter))
            argOfPericenter = longOfPericenter - ascendingNode;
    }

    double epoch = astro::J2000;
    ParseDate(orbitData, "Epoch", epoch);

    // Accept either the mean anomaly or mean longitude--use mean anomaly
    // if both are specified.
    double anomalyAtEpoch = 0.0;
    if (!orbitData->getNumber("MeanAnomaly", anomalyAtEpoch))
    {
        double longAtEpoch = 0.0;
        if (orbitData->getNumber("MeanLongitude", longAtEpoch))
            anomalyAtEpoch = longAtEpoch - (argOfPericenter + ascendingNode);
    }

    if (usePlanetUnits)
    {
        semiMajorAxis = astro::AUtoKilometers(semiMajorAxis);
        pericenterDistance = astro::AUtoKilometers(pericenterDistance);
        period = period * 365.25;
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


#ifdef USE_SPICE
static SpiceOrbit*
CreateSpiceOrbit(Hash* orbitData,
                 const string& path,
                 bool usePlanetUnits)
{
    string targetBodyName;
    string originName;
    string kernelFileName;
    if (!orbitData->getString("Kernel", kernelFileName))
    {
        clog << "Kernel filename missing from SPICE orbit\n";
        return NULL;
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

    double beginningTDBJD = 0.0;
    if (!ParseDate(orbitData, "Beginning", beginningTDBJD))
    {
        clog << "Beginning date missing from SPICE orbit\n";
        return NULL;
    }

    double endingTDBJD = 0.0;
    if (!ParseDate(orbitData, "Ending", endingTDBJD))
    {
        clog << "Ending date missing from SPICE orbit\n";
        return NULL;
    }

    // A bounding radius for culling is required for SPICE orbits
    double boundingRadius = 0.0;
    if (!orbitData->getNumber("BoundingRadius", boundingRadius))
    {
        clog << "Bounding Radius missing from SPICE orbit\n";
        return NULL;
    }

    // The period of the orbit may be specified if appropriate; a value
    // of zero for the period (the default), means that the orbit will
    // be considered aperiodic.
    double period = 0.0;
    orbitData->getNumber("Period", period);

    if (usePlanetUnits)
    {
        boundingRadius = astro::AUtoKilometers(boundingRadius);
        period = period * 365.25;
    }

    SpiceOrbit* orbit = new SpiceOrbit(kernelFileName,
                                       targetBodyName,
                                       originName,
                                       period,
                                       boundingRadius,
                                       beginningTDBJD,
                                       endingTDBJD);
    if (!orbit->init(path))
    {
        // Error using SPICE library; destroy the orbit; hopefully a
        // fallback is defined in the SSC file.
        delete orbit;
        orbit = NULL;
    }

    return orbit;
}
#endif


Orbit*
CreateOrbit(PlanetarySystem* system,
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
        DPRINTF(0, "Could not find custom orbit named '%s'\n",
                customOrbitName.c_str());
    }

#ifdef USE_SPICE
    Value* spiceOrbitDataValue = planetData->getValue("SpiceOrbit");
    if (spiceOrbitDataValue != NULL)
    {
        if (spiceOrbitDataValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Object has incorrect spice orbit syntax.\n");
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

    string sampOrbitFile;
    if (planetData->getString("SampledOrbit", sampOrbitFile))
    {
        DPRINTF(1, "Attempting to load sampled orbit file '%s'\n",
                sampOrbitFile.c_str());
        ResourceHandle orbitHandle =
            GetTrajectoryManager()->getHandle(TrajectoryInfo(sampOrbitFile, path));
        orbit = GetTrajectoryManager()->find(orbitHandle);
        if (orbit != NULL)
        {
            return orbit;
        }
        DPRINTF(0, "Could not load sampled orbit file '%s'\n",
                sampOrbitFile.c_str());
    }

    Value* orbitDataValue = planetData->getValue("EllipticalOrbit");
    if (orbitDataValue != NULL)
    {
        if (orbitDataValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Object has incorrect elliptical orbit syntax.\n");
            return NULL;
        }
        else
        {
            return CreateEllipticalOrbit(orbitDataValue->getHash(),
                                         usePlanetUnits);
        }
    }

    Vec3d longlat(0.0, 0.0, 0.0);
    if (planetData->getVector("LongLat", longlat) && system != NULL)
    {
        Body* parent = system->getPrimaryBody();
        if (parent != NULL)
        {
            Vec3f pos = parent->planetocentricToCartesian((float) longlat.x, (float) longlat.y, (float) longlat.z);
            Point3d posd(pos.x, pos.y, pos.z);
            return new SynchronousOrbit(*parent, posd);
        }
        else
        {
            // TODO: Allow fixing objects to the surface of stars.
        }
        return NULL;
    }

    return NULL;
}


static UniformRotationModel*
CreateUniformRotationModel(Hash* rotationData,
                           float syncRotationPeriod)
{
    // Default to synchronous rotation
    float period = syncRotationPeriod;
    if (rotationData->getNumber("Period", period))
    {
        period = period / 24.0f;
    }

    float offset = 0.0f;
    if (rotationData->getNumber("Offset", offset))
    {
        offset = degToRad(offset);
    }

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    float inclination = 0.0f;
    if (rotationData->getNumber("Inclination", inclination))
    {
        inclination = degToRad(inclination);
    }

    float ascendingNode = 0.0f;
    if (rotationData->getNumber("AscendingNode", ascendingNode))
    {
        ascendingNode = degToRad(ascendingNode);
    }

    return new UniformRotationModel(period,
                                    offset,
                                    epoch,
                                    inclination,
                                    ascendingNode);
}


static PrecessingRotationModel*
CreatePrecessingRotationModel(Hash* rotationData,
                              float syncRotationPeriod)
{
    // Default to synchronous rotation
    float period = syncRotationPeriod;
    if (rotationData->getNumber("Period", period))
    {
        period = period / 24.0f;
    }

    float offset = 0.0f;
    if (rotationData->getNumber("Offset", offset))
    {
        offset = degToRad(offset);
    }

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    float inclination = 0.0f;
    if (rotationData->getNumber("Inclination", inclination))
    {
        inclination = degToRad(inclination);
    }

    float ascendingNode = 0.0f;
    if (rotationData->getNumber("AscendingNode", ascendingNode))
    {
        ascendingNode = degToRad(ascendingNode);
    }

    // The default value of 0 is handled specially, interpreted to indicate
    // that there's no precession.
    float precessionPeriod = 0.0f;
    if (rotationData->getNumber("PrecessionPeriod", precessionPeriod))
    {
        // The precession period is specified in the ssc file in units
        // of years, but internally Celestia uses days.
        precessionPeriod = precessionPeriod * 365.25f;
    }

    return new PrecessingRotationModel(period,
                                       offset,
                                       epoch,
                                       inclination,
                                       ascendingNode,
                                       precessionPeriod);
}


// Parse rotation information. Unfortunately, Celestia didn't originally have
// RotationModel objects, so information about the rotation of the object isn't
// grouped into a single subobject--the ssc fields relevant for rotation just
// appear in the top level structure.
RotationModel*
CreateRotationModel(Hash* planetData,
                    const string& path,
                    float syncRotationPeriod)
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
    if (planetData->getString("CustomRotationModel", customRotationModelName))
    {
        //rotationModel = GetCustomRotationModel(customRotationModelName);
        rotationModel = NULL;
        if (rotationModel != NULL)
        {
            return rotationModel;
        }
        DPRINTF(0, "Could not find custom rotation model named '%s'\n",
                customRotationModelName.c_str());
    }

#ifdef USE_SPICE
    // TODO: implement SPICE frame based rotations
#endif

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

        DPRINTF(0, "Could not load rotation model file '%s'\n",
                sampOrientationFile.c_str());
    }

    Value* precessingRotationValue = planetData->getValue("PrecessingRotation");
    if (precessingRotationValue != NULL)
    {
        if (precessingRotationValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Object has incorrect syntax for precessing rotation.\n");
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
            DPRINTF(0, "Object has incorrect uniform rotation syntax.\n");
            return NULL;
        }
        else
        {
            return CreateUniformRotationModel(uniformRotationValue->getHash(),
                                              syncRotationPeriod);
        }
    }

    // For backward compatibility we need to support rotation parameters
    // that appear in the main block of the object definition.
    // Default to synchronous rotation
    bool specified = false;
    float period = syncRotationPeriod;
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

    float precessionRate = 0.0f;
    if (planetData->getNumber("PrecessionRate", precessionRate))
    {
        specified = true;
    }

    if (specified)
    {
        RotationModel* rm = NULL;
        if (precessionRate == 0.0f)
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
                                             -360.0f / precessionRate);
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
    return new UniformRotationModel((float) syncRotationPeriod,
                                    0.0f,
                                    astro::J2000,
                                    0.0f,
                                    0.0f);
}
