// solarsys.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>

#ifndef _WIN32
#include <config.h>
#endif /* _WIN32 */

#include <celutil/debug.h>
#include <celmath/mathlib.h>
#include "astro.h"
#include "parser.h"
#include "customorbit.h"
#include "samporbit.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "universe.h"
#include "multitexture.h"

using namespace std;


static Surface* CreateSurface(Hash* surfaceData)
{
    Surface* surface = new Surface();

    surface->color = Color(1.0f, 1.0f, 1.0f);
    surfaceData->getColor("Color", surface->color);

    Color hazeColor;
    float hazeDensity = 0.0f;
    surfaceData->getColor("HazeColor", hazeColor);
    surfaceData->getNumber("HazeDensity", hazeDensity);
    surface->hazeColor = Color(hazeColor.red(), hazeColor.green(),
                               hazeColor.blue(), hazeDensity);

    surfaceData->getColor("SpecularColor", surface->specularColor);
    surfaceData->getNumber("SpecularPower", surface->specularPower);

    string baseTexture;
    string bumpTexture;
    string nightTexture;
    bool applyBaseTexture = surfaceData->getString("Texture", baseTexture);
    bool applyBumpMap = surfaceData->getString("BumpMap", bumpTexture);
    bool applyNightMap = surfaceData->getString("NightTexture", nightTexture);
    unsigned int baseFlags = Texture::WrapTexture | Texture::AllowSplitting;
    unsigned int bumpFlags = Texture::WrapTexture | Texture::AllowSplitting;
    unsigned int nightFlags = Texture::WrapTexture | Texture::AllowSplitting;
    
    float bumpHeight = 2.5f;
    surfaceData->getNumber("BumpHeight", bumpHeight);

    bool blendTexture = false;
    surfaceData->getBoolean("BlendTexture", blendTexture);

    bool emissive = false;
    surfaceData->getBoolean("Emissive", emissive);

    bool compressTexture = false;
    surfaceData->getBoolean("CompressTexture", compressTexture);
    if (compressTexture)
        baseFlags |= Texture::CompressTexture;

    if (blendTexture)
        surface->appearanceFlags |= Surface::BlendTexture;
    if (emissive)
        surface->appearanceFlags |= Surface::Emissive;
    if (applyBaseTexture)
        surface->appearanceFlags |= Surface::ApplyBaseTexture;
    if (applyBumpMap)
        surface->appearanceFlags |= Surface::ApplyBumpMap;
    if (applyNightMap)
        surface->appearanceFlags |= Surface::ApplyNightMap;
    if (surface->specularColor != Color(0.0f, 0.0f, 0.0f))
        surface->appearanceFlags |= Surface::SpecularReflection;

    if (applyBaseTexture)
        surface->baseTexture.setTexture(baseTexture, baseFlags);
    if (applyBumpMap)
        surface->bumpTexture.setTexture(bumpTexture, bumpHeight, bumpFlags);
    if (applyNightMap)
        surface->nightTexture.setTexture(nightTexture, nightFlags);

    return surface;
}


static EllipticalOrbit* CreateEllipticalOrbit(Hash* orbitData,
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
    orbitData->getNumber("Epoch", epoch);

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
        period = period * 365.25f;
    }

    // If we read the semi-major axis, use it to compute the pericenter
    // distance.
    if (semiMajorAxis != 0.0)
        pericenterDistance = semiMajorAxis * (1.0 - eccentricity);

    // cout << " bounding radius: " << semiMajorAxis * (1.0 + eccentricity) << "km\n";

    return new EllipticalOrbit(pericenterDistance,
                               eccentricity,
                               degToRad(inclination),
                               degToRad(ascendingNode),
                               degToRad(argOfPericenter),
                               degToRad(anomalyAtEpoch),
                               period,
                               epoch);
}


static RotationElements CreateRotationElements(Hash* rotationData,
                                               float orbitalPeriod)
{
    RotationElements re;

    // The default is synchronous rotation (rotation period == orbital period)
    float period = orbitalPeriod * 24.0f;
    rotationData->getNumber("RotationPeriod", period);
    re.period = period / 24.0f;

    float offset = 0.0f;
    rotationData->getNumber("RotationOffset", offset);
    re.offset = degToRad(offset);

    rotationData->getNumber("RotationEpoch", re.epoch);

    float obliquity = 0.0f;
    rotationData->getNumber("Obliquity", obliquity);
    re.obliquity = degToRad(obliquity);

    float axisLongitude = 0.0f;
    rotationData->getNumber("LongOfRotationAxis", axisLongitude);
    re.axisLongitude = degToRad(axisLongitude);

    return re;
}


// Create a body (planet or moon) using the values from a hash
// The usePlanetsUnits flags specifies whether period and semi-major axis
// are in years and AU rather than days and kilometers
static Body* CreatePlanet(PlanetarySystem* system,
                          Hash* planetData,
                          bool usePlanetUnits = true)
{
    Body* body = new Body(system);

    Orbit* orbit = NULL;
    string customOrbitName;

    if (planetData->getString("CustomOrbit", customOrbitName))
    {
        orbit = GetCustomOrbit(customOrbitName);
        if (orbit == NULL)
            DPRINTF(0, "Could not find custom orbit named '%s'\n",
                    customOrbitName.c_str());
    }

    if (orbit == NULL)
    {
        string sampOrbitFile;
        if (planetData->getString("SampledOrbit", sampOrbitFile))
        {
            DPRINTF(1, "Attempting to load sampled orbit file '%s'\n",
                    sampOrbitFile.c_str());
            orbit = LoadSampledOrbit(string("data/") + sampOrbitFile);
            if (orbit == NULL)
            {
                DPRINTF(0, "Could not load sampled orbit file '%s'\n",
                        sampOrbitFile.c_str());
            }
        }
    }

    if (orbit == NULL)
    {
        Value* orbitDataValue = planetData->getValue("EllipticalOrbit");
        if (orbitDataValue != NULL)
        {
            if (orbitDataValue->getType() != Value::HashType)
            {
                DPRINTF(0, "Object '%s' has incorrect elliptical orbit syntax.\n",
                        body->getName().c_str());
            }
            else
            {
                orbit = CreateEllipticalOrbit(orbitDataValue->getHash(),
                                              usePlanetUnits);
            }
        }
    }
    if (orbit == NULL)
    {
        DPRINTF(0, "No valid orbit specified for object '%s'; skipping . . .\n",
                body->getName().c_str());
        delete body;
        return NULL;
    }
    body->setOrbit(orbit);
    
    double albedo = 0.5;
    planetData->getNumber("Albedo", albedo);
    body->setAlbedo(albedo);

    double radius = 10000.0;
    planetData->getNumber("Radius", radius);
    body->setRadius(radius);

    double oblateness = 0.0;
    planetData->getNumber("Oblateness", oblateness);
    body->setOblateness(oblateness);

    body->setRotationElements(CreateRotationElements(planetData, orbit->getPeriod()));

    Surface* surface = CreateSurface(planetData);
    body->setSurface(*surface);
    delete surface;

    
    
    {
        string mesh("");
        if (planetData->getString("Mesh", mesh))
        {
            ResourceHandle meshHandle = GetMeshManager()->getHandle(MeshInfo(mesh));
            body->setMesh(meshHandle);
        }
    }

    // Read the atmosphere
    {
        Value* atmosDataValue = planetData->getValue("Atmosphere");
        if (atmosDataValue != NULL)
        {
            if (atmosDataValue->getType() != Value::HashType)
            {
                cout << "ReadSolarSystem: Atmosphere must be an assoc array.\n";
            }
            else
            {
                Hash* atmosData = atmosDataValue->getHash();
                assert(atmosData != NULL);
                
                Atmosphere* atmosphere = new Atmosphere();
                atmosData->getNumber("Height", atmosphere->height);
                atmosData->getColor("Lower", atmosphere->lowerColor);
                atmosData->getColor("Upper", atmosphere->upperColor);
                atmosData->getColor("Sky", atmosphere->skyColor);
                atmosData->getNumber("CloudHeight", atmosphere->cloudHeight);
                atmosData->getNumber("CloudSpeed", atmosphere->cloudSpeed);
                atmosphere->cloudSpeed = degToRad(atmosphere->cloudSpeed);

                string cloudTexture;
                if (atmosData->getString("CloudMap", cloudTexture))
                {
                    atmosphere->cloudTexture.setTexture(cloudTexture,
                                                        Texture::WrapTexture);
                }

                body->setAtmosphere(*atmosphere);
                delete atmosphere;
            }
        }
    }

    // Read the ring system
    {
        Value* ringsDataValue = planetData->getValue("Rings");
        if (ringsDataValue != NULL)
        {
            if (ringsDataValue->getType() != Value::HashType)
            {
                cout << "ReadSolarSystem: Rings must be an assoc array.\n";
            }
            else
            {
                Hash* ringsData = ringsDataValue->getHash();
                // ASSERT(ringsData != NULL);

                double inner = 0.0, outer = 0.0;
                ringsData->getNumber("Inner", inner);
                ringsData->getNumber("Outer", outer);

                Color color(1.0f, 1.0f, 1.0f);
                ringsData->getColor("Color", color);

                string textureName;
                ringsData->getString("Texture", textureName);

                body->setRings(RingSystem((float) inner, (float) outer,
                                          color, textureName));
            }
        }
    }

    return body;
}


bool LoadSolarSystemObjects(istream& in, Universe& universe)
{
    Tokenizer tokenizer(&in); 
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            cout << "Error parsing solar system file.\n";
            return false;
        }
        string name = tokenizer.getStringValue();

        if (tokenizer.nextToken() != Tokenizer::TokenString)
        {
            cout << "Error parsing solar system file; invalid parent name.\n";
            return false;
        }
        string parentName = tokenizer.getStringValue();

        Value* objectDataValue = parser.readValue();
        if (objectDataValue == NULL)
        {
            cout << "Error reading solar system object.\n";
            return false;
        }

        if (objectDataValue->getType() != Value::HashType)
        {
            cout << "Bad solar system object.\n";
            return false;
        }
        Hash* objectData = objectDataValue->getHash();

        DPRINTF(2, "Reading planet %s %s\n", parentName.c_str(), name.c_str());

        Selection parent = universe.findPath(parentName, NULL, 0);
        PlanetarySystem* parentSystem = NULL;
        bool orbitsPlanet = false;
        if (parent.star != NULL)
        {
            SolarSystem* solarSystem = universe.getSolarSystem(parent.star);
            if (solarSystem == NULL)
            {
                // No solar system defined for this star yet, so we need
                // to create it.
                solarSystem = universe.createSolarSystem(parent.star);
            }
            parentSystem = solarSystem->getPlanets();
#ifdef DEBUG
            if (parentSystem->find(name))
            {
                DPRINTF(0, "Warning duplicate definition of %s %s!\n",
                        parentName.c_str(), name.c_str());
            }
#endif // DEBUG
        }
        else if (parent.body != NULL)
        {
            // Parent is a planet or moon
            parentSystem = parent.body->getSatellites();
            if (parentSystem == NULL)
            {
                // If the planet doesn't already have any satellites, we
                // have to create a new planetary system for it.
                parentSystem = new PlanetarySystem(parent.body);
                parent.body->setSatellites(parentSystem);
            }
#ifdef DEBUG
            else
            {
                if (parentSystem->find(name))
                {
                    DPRINTF(0, "Warning, duplicate definition of %s %s!\n",
                            parentName.c_str(), name.c_str());
                }
            }
#endif // DEBUG
            orbitsPlanet = true;
        }
        else
        {
            cout << "Parent body '" << parentName << "' of '" << name << "' not found.\n";
        }

        if (parentSystem != NULL)
        {
            Body* body = CreatePlanet(parentSystem, objectData, !orbitsPlanet);
            if (body != NULL)
            {
                body->setName(name);
                parentSystem->addBody(body);
            }
        }
    }

    // TODO: Return some notification if there's an error parsing the file
    return true;
}


SolarSystem::SolarSystem(const Star* _star) : star(_star)
{
    planets = new PlanetarySystem(star);
}


const Star* SolarSystem::getStar() const
{
    return star;
}

Point3f SolarSystem::getCenter() const
{
    // TODO: This is a very simple method at the moment, but it will get
    // more complex when planets around multistar systems are supported
    // where the planets may orbit the center of mass of two stars.
    return star->getPosition();
}

PlanetarySystem* SolarSystem::getPlanets() const
{
    return planets;
}
