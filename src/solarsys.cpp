// solarsys.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include "mathlib.h"
#include "astro.h"
#include "parser.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "solarsys.h"

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
    string cloudTexture;
    string nightTexture;
    bool applyBaseTexture = surfaceData->getString("Texture", baseTexture);
    bool applyBumpMap = surfaceData->getString("BumpMap", bumpTexture);
    bool applyCloudMap = surfaceData->getString("CloudMap", cloudTexture);
    bool applyNightMap = surfaceData->getString("NightTexture", nightTexture);

    float bumpHeight = 2.5f;
    surfaceData->getNumber("BumpHeight", bumpHeight);

    bool blendTexture = false;
    surfaceData->getBoolean("BlendTexture", blendTexture);

    bool compressTexture = false;
    surfaceData->getBoolean("CompressTexture", compressTexture);

    if (blendTexture)
        surface->appearanceFlags |= Surface::BlendTexture;
    if (applyBaseTexture)
        surface->appearanceFlags |= Surface::ApplyBaseTexture;
    if (applyBumpMap)
        surface->appearanceFlags |= Surface::ApplyBumpMap;
    if (applyCloudMap)
        surface->appearanceFlags |= Surface::ApplyCloudMap;
    if (applyNightMap)
        surface->appearanceFlags |= Surface::ApplyNightMap;
    if (surface->specularColor != Color(0.0f, 0.0f, 0.0f))
        surface->appearanceFlags |= Surface::SpecularReflection;

    TextureManager* texMan = GetTextureManager();
    if (applyBaseTexture)
        surface->baseTexture = texMan->getHandle(TextureInfo(baseTexture,
                                                             compressTexture));
    if (applyBumpMap)
        surface->bumpTexture = texMan->getHandle(TextureInfo(bumpTexture,
                                                             bumpHeight));
    if (applyCloudMap)
        surface->cloudTexture = texMan->getHandle(TextureInfo(cloudTexture));
    if (applyNightMap)
        surface->nightTexture = texMan->getHandle(TextureInfo(nightTexture));

    return surface;
}


// Create a body (planet or moon) using the values from a hash
// The usePlanetsUnits flags specifies whether period and semi-major axis
// are in years and AU rather than days and kilometers
static Body* CreatePlanet(PlanetarySystem* system,
                          Hash* planetData,
                          bool usePlanetUnits = true)
{
    Body* body = new Body(system);

    string name("Unnamed");
    planetData->getString("Name", name);
    body->setName(name);
    
    DPRINTF("Reading planet %s\n", name.c_str());

    double semiMajorAxis = 0.0;
    if (!planetData->getNumber("SemiMajorAxis", semiMajorAxis))
    {
        DPRINTF("SemiMajorAxis missing!  Skipping planet . . .\n");
        delete body;
        return NULL;
    }

    double period = 0.0;
    if (!planetData->getNumber("Period", period))
    {
        DPRINTF("Period missing!  Skipping planet . . .\n");
        delete body;
        return NULL;
    }

    double eccentricity = 0.0;
    planetData->getNumber("Eccentricity", eccentricity);

    double inclination = 0.0;
    planetData->getNumber("Inclination", inclination);

    double ascendingNode = 0.0;
    planetData->getNumber("AscendingNode", ascendingNode);

    double argOfPericenter = 0.0;
    if (!planetData->getNumber("ArgOfPericenter", argOfPericenter))
    {
        double longOfPericenter = 0.0;
        if (planetData->getNumber("LongOfPericenter", longOfPericenter))
            argOfPericenter = longOfPericenter - ascendingNode;
    }

    double epoch = astro::J2000;
    planetData->getNumber("Epoch", epoch);

    // Accept either the mean anomaly or mean longitude--use mean anomaly
    // if both are specified.
    double anomalyAtEpoch = 0.0;
    if (!planetData->getNumber("MeanAnomaly", anomalyAtEpoch))
    {
        double longAtEpoch = 0.0;
        if (planetData->getNumber("MeanLongitude", longAtEpoch))
            anomalyAtEpoch = longAtEpoch - (argOfPericenter + ascendingNode);
    }

    if (usePlanetUnits)
    {
        semiMajorAxis = astro::AUtoKilometers(semiMajorAxis);
        period = period * 365.25f;
    }

    body->setOrbit(new EllipticalOrbit(semiMajorAxis,
                                       eccentricity,
                                       degToRad(inclination),
                                       degToRad(ascendingNode),
                                       degToRad(argOfPericenter),
                                       degToRad(anomalyAtEpoch),
                                       period,
                                       epoch));
    
    double obliquity = 0.0;
    planetData->getNumber("Obliquity", obliquity);
    body->setObliquity(degToRad(obliquity));

    double albedo = 0.5;
    planetData->getNumber("Albedo", albedo);
    body->setAlbedo(albedo);

    double radius = 10000.0;
    planetData->getNumber("Radius", radius);
    body->setRadius(radius);

    double oblateness = 0.0;
    planetData->getNumber("Oblateness", oblateness);
    body->setOblateness(oblateness);

    // The default rotation period is the same as the orbital period
    double rotationPeriod = period * 24.0;
    planetData->getNumber("RotationPeriod", rotationPeriod);
    body->setRotationPeriod(rotationPeriod / 24.0);

    double rotationPhase = 0.0;
    planetData->getNumber("RotationPhase", rotationPhase);
    body->setRotationPhase((float) degToRad(rotationPhase));

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
                ResourceHandle texture = GetTextureManager()->getHandle(TextureInfo(textureName));

                body->setRings(RingSystem((float) inner, (float) outer,
                                          color, texture));
            }
        }
    }

    // Read the moons
    // TODO: This is largely cut-and-pasted from the ReadSolarSystem function
    // It would be better if they actually used the same code
    
    Value* moonsDataValue = planetData->getValue("Moons");
    if (moonsDataValue != NULL)
    {
        if (moonsDataValue->getType() != Value::ArrayType)
        {
            cout << "ReadSolarSystem: Moons must be an array.\n";
        }
        else
        {
            Array* moonsData = moonsDataValue->getArray();
            // ASSERT(moonsData != NULL);

            PlanetarySystem* satellites = NULL;
            if (moonsData->size() != 0)
                satellites = new PlanetarySystem(body);
            
            for (unsigned int i = 0; i < moonsData->size(); i++)
            {
                Value* moonValue = (*moonsData)[i];
                if (moonValue != NULL &&
                    moonValue->getType() == Value::HashType)
                {
                    Body* moon = CreatePlanet(satellites,
                                              moonValue->getHash(),
                                              false);
                    if (moon != NULL)
                        satellites->addBody(moon);
                }
                else
                {
                    cout << "ReadSolarSystem: Moon data must be an assoc array.\n";
                }
            }
            body->setSatellites(satellites);
        }
    }

    return body;
}


static SolarSystem* ReadSolarSystem(Parser& parser,
                                    const StarDatabase& starDB)
{
    Value* starName = parser.readValue();
    if (starName == NULL)
    {
        cout << "ReadSolarSystem: Error reading star name.\n";
        return NULL;
    }

    if (starName->getType() != Value::StringType)
    {
        cout << "ReadSolarSystem: Star name is not a string.\n";
        delete starName;
        return NULL;
    }

    Value* solarSystemDataValue = parser.readValue();
    if (solarSystemDataValue == NULL)
    {
        cout << "ReadSolarSystem: Error reading solar system data\n";
        delete starName;
        return NULL;
    }

    if (solarSystemDataValue->getType() != Value::HashType)
    {
        cout << "ReadSolarSystem: Solar system data must be an assoc array\n";
        delete starName;
        delete solarSystemDataValue;
        return NULL;
    }

    Hash* solarSystemData = solarSystemDataValue->getHash();
    // ASSERT(solarSystemData != NULL);

    Star* star = starDB.find(starName->getString());
    if (star == NULL)
    {
        cout << "Cannot find star named '" << starName->getString() << "'\n";
        delete starName;
        delete solarSystemDataValue;
        return NULL;
    }

    SolarSystem* solarSys = new SolarSystem(star);
    
    Value* planetsDataValue = solarSystemData->getValue("Planets");
    if (planetsDataValue != NULL)
    {
        if (planetsDataValue->getType() != Value::ArrayType)
        {
            cout << "ReadSolarSystem: Planets must be an array.\n";
        }
        else
        {
            Array* planetsData = planetsDataValue->getArray();
            // ASSERT(planetsData != NULL);
            for (unsigned int i = 0; i < planetsData->size(); i++)
            {
                Value* planetValue = (*planetsData)[i];
                if (planetValue != NULL &&
                    planetValue->getType() == Value::HashType)
                {
                    Body* body = CreatePlanet(solarSys->getPlanets(),
                                              planetValue->getHash());
                    if (body != NULL)
                        solarSys->getPlanets()->addBody(body);
                }
                else
                {
                    cout << "ReadSolarSystem: Planet data must be an assoc array.\n";
                }
            }
        }
    }

    delete starName;
    delete solarSystemDataValue;

    return solarSys;
}


SolarSystemCatalog* ReadSolarSystemCatalog(istream& in,
                                           StarDatabase& starDB)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    SolarSystemCatalog* catalog = new SolarSystemCatalog();

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        tokenizer.pushBack();
        SolarSystem* solarSystem = ReadSolarSystem(parser, starDB);
        if (solarSystem != NULL)
        {
            catalog->insert(SolarSystemCatalog::value_type(solarSystem->getStar()->getCatalogNumber(),
                                                           solarSystem));
        }
    }

    return catalog;
}


bool ReadSolarSystems(istream& in,
                      const StarDatabase& starDB,
                      SolarSystemCatalog& catalog)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        tokenizer.pushBack();
        SolarSystem* solarSystem = ReadSolarSystem(parser, starDB);
        if (solarSystem != NULL)
        {
            catalog.insert(SolarSystemCatalog::value_type(solarSystem->getStar()->getCatalogNumber(),
                                                          solarSystem));
        }
    }

    // TODO: Return some notification if there's an error parsring the file
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

PlanetarySystem* SolarSystem::getPlanets() const
{
    return planets;
}


#if 0
int SolarSystem::getSystemSize() const
{
    return bodies.size();
}

Body* SolarSystem::getBody(int n) const
{
    return bodies[n];
}

void SolarSystem::addBody(Body* sat)
{
    bodies.insert(bodies.end(), sat);
}


Body* SolarSystem::find(string _name, bool deepSearch) const
{
    for (int i = 0; i < bodies.size(); i++)
    {
        if (bodies[i]->getName() == _name)
        {
            return bodies[i];
        }
        else if (deepSearch && bodies[i]->getSatellites() != NULL)
        {
            Body* body = bodies[i]->getSatellites()->find(_name, deepSearch);
            if (body != NULL)
                return body;
        }
    }

    return NULL;
}
#endif



