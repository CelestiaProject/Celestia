// solarsys.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mathlib.h"
#include "astro.h"
#include "parser.h"
#include "solarsys.h"


typedef struct
{
    char *name;
    float radius;
    float obliquity;
    float albedo;
    double period;
    double semiMajorAxis;
    double eccentricity;
    double inclination;
    double ascendingNode;
    double argOfPeriapsis;
    double trueAnomaly;
} Planet;


Planet SolSystem[] =
{
    { "Mercury", 2440, 7.0f, 0.06f,
      0.2408, 0.3871, 0.2056, 7.0049, 77.456, 48.331, 252.251 },
    { "Venus", 6052, 177.4f, 0.77f,
      0.6152, 0.7233, 0.0068, 3.3947, 131.533, 76.681, 181.979 },
    { "Earth", 6378, 23.45f, 0.30f,
      1.0000, 1.0000, 0.0167, 0.0001, 102.947, 348.739, 100.464 },
    { "Mars", 3394, 23.98f, 0.15f,
      1.8809, 1.5237, 0.0934, 1.8506, 336.041, 49.579, 355.453 },
    { "Jupiter", 71398, 3.08f, 0.51f,
      11.8622, 5.2034, 0.0484, 1.3053, 14.7539, 100.556, 34.404 },
    { "Saturn", 60330, 26.73f, 0.50f,
      29.4577, 9.5371, 0.0542, 2.4845, 92.432, 113.715, 49.944 },
    { "Uranus", 26200, 97.92f, 0.66f,
      84.0139, 19.1913, 0.0472, 0.7699, 170.964, 74.230, 313.232 },
    { "Neptune", 25225, 28.8f, 0.62f,
      164.793, 30.0690, 0.0086, 1.7692, 44.971, 131.722, 304.880 },
    { "Pluto", 1137, 122.46f, 0.6f,
      248.54, 39.4817, 0.2488, 17.142, 224.067, 110.303, 238.928 },
};


static Surface* CreateSurface(Hash* surfaceData)
{
    Surface* surface = new Surface();

    surface->color = Color(1.0f, 1.0f, 1.0f);
    surfaceData->getColor("Color", surface->color);
    bool applyBaseTexture = surfaceData->getString("Texture", surface->baseTexture);
    bool applyBumpMap = surfaceData->getString("BumpMap", surface->bumpTexture);
    surface->bumpHeight = 2.5f;
    surfaceData->getNumber("BumpHeight", surface->bumpHeight);
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
    if (compressTexture)
        surface->appearanceFlags |= Surface::CompressBaseTexture;

    return surface;
}


static Body* createSatellite(Planet* p)
{
    EllipticalOrbit* orbit = new EllipticalOrbit(astro::AUtoKilometers(p->semiMajorAxis),
                                                 p->eccentricity,
                                                 degToRad(p->inclination),
                                                 degToRad(p->ascendingNode),
                                                 degToRad(p->argOfPeriapsis),
                                                 degToRad(p->trueAnomaly),
                                                 p->period * 365.25f);
    Body* body = new Body(NULL);
    body->setName(string(p->name));
    body->setOrbit(orbit);
    body->setRadius(p->radius);
    body->setObliquity(degToRad(p->obliquity));
    body->setAlbedo(p->albedo);

    return body;
}


// Create a solar system with our nine familiar planets
SolarSystem* CreateOurSolarSystem()
{
    SolarSystem* solarSystem = new SolarSystem(0);

    for (int i = 0; i < sizeof(SolSystem) / sizeof(SolSystem[0]); i++)
    {
        solarSystem->getPlanets()->addBody(createSatellite(&SolSystem[i]));
    }

    return solarSystem;
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
    
    cout << "Reading planet " << name << "\n";

    double semiMajorAxis = 0.0;
    if (!planetData->getNumber("SemiMajorAxis", semiMajorAxis))
    {
        cout << "SemiMajorAxis missing!  Skipping planet . . .\n";
        delete body;
        return NULL;
    }

    double period = 0.0;
    if (!planetData->getNumber("Period", period))
    {
        cout << "Period missing!  Skipping planet . . .\n";
        delete body;
        return NULL;
    }

    double eccentricity = 0.0;
    planetData->getNumber("Eccentricity", eccentricity);

    double inclination = 0.0;
    planetData->getNumber("Inclination", inclination);

    double ascendingNode = 0.0;
    planetData->getNumber("AscendingNode", ascendingNode);

    double longOfPericenter = 0.0;
    planetData->getNumber("ArgOfPeriapsis", longOfPericenter);

    double epoch = astro::J2000;
    planetData->getNumber("Epoch", epoch);

    // Accept either the mean anomaly or mean longitude--use mean anomaly
    // if both are specified.
    double anomalyAtEpoch = 0.0;
    if (!planetData->getNumber("MeanAnomaly", anomalyAtEpoch))
    {
        double longAtEpoch = 0.0;
        if (planetData->getNumber("MeanLongitude", longAtEpoch))
            anomalyAtEpoch = longAtEpoch - longOfPericenter;
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
                                       degToRad(longOfPericenter),
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
    body->setRotationPeriod(rotationPeriod);

#if 0    
    Surface surface;
    surface.color = Color(1.0f, 1.0f, 1.0f);
    planetData->getColor("Color", surface.color);
    bool applyBaseTexture = planetData->getString("Texture", surface.baseTexture);
    bool applyBumpMap = planetData->getString("BumpMap", surface.bumpTexture);
    bool blendTexture = false;
    planetData->getBoolean("BlendTexture", blendTexture);
    bool compressTexture = false;
    planetData->getBoolean("CompressTexture", compressTexture);
    if (blendTexture)
        surface.appearanceFlags |= Surface::BlendTexture;
    if (applyBaseTexture)
        surface.appearanceFlags |= Surface::ApplyBaseTexture;
    if (applyBumpMap)
        surface.appearanceFlags |= Surface::ApplyBumpMap;
    if (compressTexture)
        surface.appearanceFlags |= Surface::CompressBaseTexture;
    body->setSurface(surface);
#endif
    Surface* surface = CreateSurface(planetData);
    body->setSurface(*surface);
    delete surface;
    
    string mesh("");
    planetData->getString("Mesh", mesh);
    body->setMesh(mesh);

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

                string texture;
                ringsData->getString("Texture", texture);

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
            
            for (int i = 0; i < moonsData->size(); i++)
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
    uint32 starNumber = star->getCatalogNumber();

    SolarSystem* solarSys = new SolarSystem(starNumber);
    
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
            for (int i = 0; i < planetsData->size(); i++)
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
            catalog->insert(SolarSystemCatalog::value_type(solarSystem->getStarNumber(),
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
            catalog.insert(SolarSystemCatalog::value_type(solarSystem->getStarNumber(),
                                                          solarSystem));
        }
    }

    // TODO: Return some notification if there's an error parsring the file
    return true;
}


SolarSystem::SolarSystem(uint32 _starNumber) : starNumber(_starNumber)
{
    planets = new PlanetarySystem(starNumber);
}


uint32 SolarSystem::getStarNumber() const
{
    return starNumber;
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



